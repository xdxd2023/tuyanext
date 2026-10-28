#include <stdint.h>
#include <netinet/in.h>

#include <iomanip>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>

#include <utils/json.hpp>
#include "utils/log_.h"
#include "lz4/lz4.h"

#include "tuya_cloud_com_defs.h"
#include "tuya_error_code.h"
#include "tuya_iot_wifi_api.h"
#include "tuya_svc_upgrade.h"

#define __NLOHMANN_JSON_CPP
#include "robot_msg.h"
#include "voice.h"
#include "tuya_airbot.h"
#include "tuya_enums.h"
#include "tuya_dp_utils.h"
#include "tuya_dp_function.h"
#include "tuya_utils.h"
#include "tuyaos_sdk.h"

#undef TAG
#define TAG "AIRBOT"

using json = nlohmann::json;

namespace
{
    mars_message::AppSpotClean g_airbotCruiseSpot = {};
    bool g_hasAirbotCruiseSpot = false;
    tuya_message::RobotState g_airbotRobotState = {};
    std::string g_airbotLastCruiseMode = "idle";

    struct AirBotDpHandlerEntry
    {
        int dpId;
        void (*handler)(TY_OBJ_DP_S *);
    };

    struct AirBotRawHandlerEntry
    {
        int dpId;
        void (*handler)(int dpId, uint8_t *data, int len);
    };

    // DP 类型表（依据 AirBot 功能点文档）
    std::map<int, TuyaType> kAirBotDpTypes = {
        {2, TuyaTypeValue},    // pm25  PM2.5 (ro)
        {3, TuyaTypeEnum},     // mode  模式 (rw)
        {5, TuyaTypeValue},    // filter_life  滤芯寿命 (ro)
        {6, TuyaTypeBool},     // anion  负离子 (rw)
        {7, TuyaTypeBool},     // child_lock  童锁 (rw)
        {8, TuyaTypeBool},     // light  灯光 (rw)
        {9, TuyaTypeBool},     // uv  UV 杀菌 (rw)
        {10, TuyaTypeBool},    // wet  加湿 (rw)
        {11, TuyaTypeBool},    // filter_reset  滤芯复位 (rw)
        {12, TuyaTypeValue},   // temp_current  当前温度 (ro)
        {13, TuyaTypeValue},   // humidity  湿度 (ro)
        {14, TuyaTypeValue},   // tvoc  TVOC (ro)
        {15, TuyaTypeValue},   // eco2  eCO2 (ro)
        {16, TuyaTypeValue},   // filter_days  滤芯剩余天数 (ro)
        {17, TuyaTypeValue},   // runtime_total  净化时长 (ro)
        {18, TuyaTypeEnum},    // countdown_set  倒计时 (rw)
        {19, TuyaTypeValue},   // countdown_left  倒计时剩余时间 (ro)
        {20, TuyaTypeValue},   // pm_total  累计吸收颗粒 (ro)
        {21, TuyaTypeEnum},    // air_quality  空气质量 (ro)
        {22, TuyaTypeBitmap},  // fault  故障告警 (ro)
        {23, TuyaTypeEnum},    // temp_unit_convert  温标切换 (rw)
        {24, TuyaTypeValue},   // temp_set  温度设置 (rw)
        {25, TuyaTypeValue},   // temp_set_f  目标温度_F (rw)
        {26, TuyaTypeValue},   // temp_current_f  当前温度_F (ro)
        {27, TuyaTypeValue},   // ch2o_value  甲醛检测值 (ro)
        {28, TuyaTypeBool},    // mute  静音 (rw)
        {29, TuyaTypeEnum},    // filter_type  滤芯类型 (ro)
        {30, TuyaTypeValue},   // pm10  PM10 (ro)
        {33, TuyaTypeValue},   // aal  AAL (ro)
        {34, TuyaTypeBool},    // heat  加热 (rw)
        {41, TuyaTypeBool},    // switch_go  开关 (rw)
        {42, TuyaTypeBool},    // pause_resume  暂停/继续 (rw)
        {43, TuyaTypeBool},    // switch_charge  一键回充 (rw)
        {44, TuyaTypeEnum},    // status  设备状态 (ro)
        {45, TuyaTypeValue},   // battery_percentage  电池电量 (ro)
        {46, TuyaTypeBool},    // basic_private  隐私模式 (rw)
        {47, TuyaTypeValue},   // movement_direction  移动方向 (rw)
        {48, TuyaTypeBool},    // movement_stops  运动停止 (rw)
        {49, TuyaTypeEnum},    // speed_set  速度设置 (rw)
        {50, TuyaTypeEnum},    // basic_nightvision  红外夜视 (rw)
        {51, TuyaTypeBool},    // basic_osd  时间水印 (rw)
        {52, TuyaTypeBool},    // screen_switch  横竖屏切换 (rw)
        {53, TuyaTypeValue},   // volume_set  音量设置 (rw)
        {54, TuyaTypeValue},   // precent_setting  风速设置 (rw)
        {55, TuyaTypeValue},   // brightness_control  灯光亮度 (rw)
        {56, TuyaTypeEnum},    // wind_speed_mode  风速模式 (rw)
        {212, TuyaTypeRaw},    // initiative_message  主动消息推送 (rw)
    };

    void AirBotReportObjectDp(const TY_OBJ_DP_S &dp)
    {
        TY_OBJ_DP_S temp = dp;
        OPERATE_RET ret = dev_report_dp_json_async(NULL, &temp, 1);
        if (ret != OPRT_OK)
        {
            LOGD(TAG, "report object dp failed, dpId:{}, ret:{}", dp.dpid, ret);
        }
    }

    std::string AirBotRawToHex(const uint8_t *data, int len)
    {
        std::ostringstream oss;
        for (int i = 0; i < len; ++i)
        {
            oss << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(data[i]);
        }
        return oss.str();
    }

    void AirBotPublishString(const std::string &channel, const std::string &value)
    {
        mars_message::String msg = {};
        msg.value = value;
        TuyaComm::Get()->Publish(channel, &msg);
    }

    void AirBotPublishRaw(const std::string &channel, int dpId, const uint8_t *data, int len)
    {
        if (data == NULL || len <= 0)
        {
            return;
        }

        json payload;
        payload["dpid"] = dpId;
        payload["len"] = len;
        payload["hex"] = AirBotRawToHex(data, len);
        AirBotPublishString(channel, payload.dump());
    }

    bool AirBotSendRequest(const std::string &channel, int a = 0, int b = 0, int c = 0)
    {
        tuya_message::Request req = {a, b, c};
        tuya_message::Result res = {};
        bool ok = TuyaComm::Get()->Send(channel, &req, &res);
        if (!ok)
        {
            LOGD(TAG, "send failed, channel:{}, req:[{},{},{}]", channel, a, b, c);
        }
        return ok;
    }

    // ---- 通用下发辅助 ----
    void AirBotHandleBoolDp(const std::string &channel, TY_OBJ_DP_S *dp)
    {
        if (dp == NULL)
        {
            return;
        }
        AirBotSendRequest(channel, dp->value.dp_bool, 0, 0);
    }

    void AirBotHandleValueDp(const std::string &channel, TY_OBJ_DP_S *dp)
    {
        if (dp == NULL)
        {
            return;
        }
        AirBotSendRequest(channel, dp->value.dp_value, 0, 0);
    }

    void AirBotHandleEnumDp(const std::string &channel, TY_OBJ_DP_S *dp)
    {
        if (dp == NULL)
        {
            return;
        }
        AirBotSendRequest(channel, dp->value.dp_enum, 0, 0);
    }

    // ---- 导航类 DP ----
    // 41 开关：true 启动 / false 停止
    void AirBotHandleSwitchGo(TY_OBJ_DP_S *dp)
    {
        if (dp == NULL)
        {
            return;
        }
        tuya_message::Request req = {};
        tuya_message::Result res = {};
        if (dp->value.dp_bool)
        {
            TuyaComm::Get()->Send("ty_start", &req, &res);
        }
        else
        {
            TuyaComm::Get()->Send("ty_stop", &req, &res);
        }
    }

    // 42 暂停/继续：true 暂停 / false 继续
    void AirBotHandlePauseResume(TY_OBJ_DP_S *dp)
    {
        if (dp == NULL)
        {
            return;
        }
        tuya_message::Request req = {};
        tuya_message::Result res = {};
        if (dp->value.dp_bool)
        {
            TuyaComm::Get()->Send("ty_pause", &req, &res);
        }
        else
        {
            TuyaComm::Get()->Send("ty_start", &req, &res);
        }
    }

    // 43 一键回充：true 回充启动 / false 回充停止
    void AirBotHandleSwitchCharge(TY_OBJ_DP_S *dp)
    {
        if (dp == NULL)
        {
            return;
        }
        tuya_message::Request req = {dp->value.dp_bool};
        tuya_message::Result res = {};
        if (dp->value.dp_bool)
        {
            TuyaComm::Get()->Send("ty_charge", &req, &res);
        }
        else
        {
            TuyaComm::Get()->Send("ty_pause", &req, &res);
        }
    }

    // 3 模式：current_location / smart / select_room / pose / quick_mapping
    void AirBotHandleMode(TY_OBJ_DP_S *dp)
    {
        AirBotHandleEnumDp("airbot_mode", dp);
    }

    // ---- 空气净化类 DP ----
    void AirBotHandleAnion(TY_OBJ_DP_S *dp)            { AirBotHandleBoolDp("airbot_anion", dp); }            // 6  负离子
    void AirBotHandleChildLock(TY_OBJ_DP_S *dp)        { AirBotHandleBoolDp("airbot_child_lock", dp); }       // 7  童锁
    void AirBotHandleLight(TY_OBJ_DP_S *dp)            { AirBotHandleBoolDp("airbot_light", dp); }            // 8  灯光
    void AirBotHandleUv(TY_OBJ_DP_S *dp)               { AirBotHandleBoolDp("airbot_uv", dp); }               // 9  UV 杀菌
    void AirBotHandleWet(TY_OBJ_DP_S *dp)              { AirBotHandleBoolDp("airbot_wet", dp); }              // 10 加湿
    void AirBotHandleFilterReset(TY_OBJ_DP_S *dp)      { AirBotHandleBoolDp("airbot_filter_reset", dp); }     // 11 滤芯复位
    void AirBotHandleCountdownSet(TY_OBJ_DP_S *dp)     { AirBotHandleEnumDp("airbot_countdown_set", dp); }    // 18 倒计时
    void AirBotHandleTempUnitConvert(TY_OBJ_DP_S *dp)  { AirBotHandleEnumDp("airbot_temp_unit_convert", dp); }// 23 温标切换
    void AirBotHandleTempSet(TY_OBJ_DP_S *dp)          { AirBotHandleValueDp("airbot_temp_set", dp); }         // 24 温度设置
    void AirBotHandleTempSetF(TY_OBJ_DP_S *dp)         { AirBotHandleValueDp("airbot_temp_set_f", dp); }       // 25 目标温度_F
    void AirBotHandleMute(TY_OBJ_DP_S *dp)             { AirBotHandleBoolDp("airbot_mute", dp); }             // 28 静音
    void AirBotHandleHeat(TY_OBJ_DP_S *dp)             { AirBotHandleBoolDp("airbot_heat", dp); }             // 34 加热
    void AirBotHandleBasicPrivate(TY_OBJ_DP_S *dp)     { AirBotHandleBoolDp("airbot_basic_private", dp); }    // 46 隐私模式
    void AirBotHandleMovementDirection(TY_OBJ_DP_S *dp){ AirBotHandleValueDp("airbot_movement_direction", dp); } // 47 移动方向
    void AirBotHandleMovementStops(TY_OBJ_DP_S *dp)    { AirBotHandleBoolDp("airbot_movement_stops", dp); }   // 48 运动停止
    void AirBotHandleSpeedSet(TY_OBJ_DP_S *dp)         { AirBotHandleEnumDp("airbot_speed_set", dp); }        // 49 速度设置
    void AirBotHandleNightVision(TY_OBJ_DP_S *dp)      { AirBotHandleEnumDp("airbot_basic_nightvision", dp); } // 50 红外夜视
    void AirBotHandleOsd(TY_OBJ_DP_S *dp)              { AirBotHandleBoolDp("airbot_basic_osd", dp); }        // 51 时间水印
    void AirBotHandleScreenSwitch(TY_OBJ_DP_S *dp)     { AirBotHandleBoolDp("airbot_screen_switch", dp); }    // 52 横竖屏切换
    void AirBotHandleVolumeSet(TY_OBJ_DP_S *dp)        { AirBotHandleValueDp("airbot_volume_set", dp); }       // 53 音量设置
    void AirBotHandlePrecentSetting(TY_OBJ_DP_S *dp)   { AirBotHandleValueDp("airbot_precent_setting", dp); }  // 54 风速设置
    void AirBotHandleBrightnessControl(TY_OBJ_DP_S *dp){ AirBotHandleValueDp("airbot_brightness_control", dp); } // 55 灯光亮度
    void AirBotHandleWindSpeedMode(TY_OBJ_DP_S *dp)    { AirBotHandleEnumDp("airbot_wind_speed_mode", dp); }   // 56 风速模式

    // 212 主动消息推送（raw）
    void AirBotRawHandleInitiativeMessage(int dpId, uint8_t *data, int len)
    {
        if (data == NULL || len <= 0)
        {
            return;
        }
        AirBotPublishRaw("airbot_initiative_message", dpId, data, len);
    }

    AirBotDpHandlerEntry kAirBotDpHandlers[] = {
        {3, AirBotHandleMode},
        {6, AirBotHandleAnion},
        {7, AirBotHandleChildLock},
        {8, AirBotHandleLight},
        {9, AirBotHandleUv},
        {10, AirBotHandleWet},
        {11, AirBotHandleFilterReset},
        {18, AirBotHandleCountdownSet},
        {23, AirBotHandleTempUnitConvert},
        {24, AirBotHandleTempSet},
        {25, AirBotHandleTempSetF},
        {28, AirBotHandleMute},
        {34, AirBotHandleHeat},
        {41, AirBotHandleSwitchGo},
        {42, AirBotHandlePauseResume},
        {43, AirBotHandleSwitchCharge},
        {46, AirBotHandleBasicPrivate},
        {47, AirBotHandleMovementDirection},
        {48, AirBotHandleMovementStops},
        {49, AirBotHandleSpeedSet},
        {50, AirBotHandleNightVision},
        {51, AirBotHandleOsd},
        {52, AirBotHandleScreenSwitch},
        {53, AirBotHandleVolumeSet},
        {54, AirBotHandlePrecentSetting},
        {55, AirBotHandleBrightnessControl},
        {56, AirBotHandleWindSpeedMode},
    };

    AirBotRawHandlerEntry kAirBotRawDpHandlers[] = {
        {212, AirBotRawHandleInitiativeMessage},
    };

    bool AirBotReportBoolState(int dpId, bool value)
    {
        AirBotReportObjectDp(DPReportBool(dpId, value));
        return true;
    }

    bool AirBotReportEnumState(int dpId, int value)
    {
        AirBotReportObjectDp(DPReportEnum(dpId, value));
        return true;
    }

    bool AirBotReportValueState(int dpId, int value)
    {
        AirBotReportObjectDp(DPReportValue(dpId, value));
        return true;
    }

    bool AirBotReportStringState(int dpId, const std::string &value)
    {
        AirBotReportObjectDp(DPReportString(dpId, value.c_str()));
        return true;
    }

    bool AirBotReportRawState(int dpId, const uint8_t *data, int len, int timeoutMs)
    {
        if (data == NULL || len <= 0)
        {
            return false;
        }

        OPERATE_RET ret = dev_report_dp_raw_sync(NULL, dpId, data, len, timeoutMs);
        if (ret != OPRT_OK)
        {
            LOGD(TAG, "report raw dp failed, dpId:{}, ret:{}", dpId, ret);
            return false;
        }
        return true;
    }

    void AirBotReportCruiseSwitch(bool enabled)
    {
        // AirBot 无巡航开关 DP；建图/巡航以 mode(DP 3) 与 status(DP 44) 体现
        AirBotReportEnumState(3, enabled ? 4 : 0); // 4 = quick_mapping
    }

    bool AirBotSendCruiseRequest(const json &payload, json *reply)
    {
        (void)payload;
        if (reply != nullptr)
        {
            *reply = json::object();
        }
        return true;
    }

    bool AirBotCruiseStatusToSpot(const json &reply, mars_message::AppSpotClean *spot)
    {
        (void)reply;
        if (spot == NULL)
        {
            return false;
        }
        *spot = {};
        return false;
    }

    void AirBotUpdateCruiseSpotCache(const mars_message::AppSpotClean &spot)
    {
        g_airbotCruiseSpot = spot;
        g_hasAirbotCruiseSpot = true;
    }

    bool AirBotCruiseStartMapping()
    {
        json reply;
        if (!AirBotSendCruiseRequest(json{{"cmd", "start_mapping"}}, &reply))
        {
            return false;
        }

        g_airbotLastCruiseMode = "mapping";
        AirBotReportCruiseSwitch(true);
        return true;
    }

    bool AirBotCruiseGoto(const mars_message::AppSpotClean &spot)
    {
        json reply;
        if (!AirBotSendCruiseRequest(json{{"cmd", "goto"}, {"x", spot.spot.x}, {"y", spot.spot.y}}, &reply))
        {
            return false;
        }

        AirBotUpdateCruiseSpotCache(spot);
        g_airbotLastCruiseMode = "goto";
        AirBotReportCruiseSwitch(true);
        return true;
    }

    bool AirBotCruiseSetPatrol(bool enable)
    {
        json reply;
        if (!enable)
        {
            if (!AirBotSendCruiseRequest(json{{"cmd", "stop"}}, &reply))
            {
                return false;
            }
            g_airbotLastCruiseMode = "idle";
            AirBotReportCruiseSwitch(false);
            return true;
        }

        if (!AirBotSendCruiseRequest(json{{"cmd", "random_patrol"}, {"count", 8}, {"loop", true}}, &reply))
        {
            return false;
        }

        g_airbotLastCruiseMode = "random_patrol";
        AirBotReportCruiseSwitch(true);
        return true;
    }

    bool AirBotCruiseGetSpot(mars_message::AppSpotClean *spot)
    {
        if (spot == NULL)
        {
            return false;
        }

        json reply;
        if (AirBotSendCruiseRequest(json{{"cmd", "status"}}, &reply))
        {
            mars_message::AppSpotClean currentSpot = {};
            if (AirBotCruiseStatusToSpot(reply, &currentSpot))
            {
                AirBotUpdateCruiseSpotCache(currentSpot);
                *spot = g_airbotCruiseSpot;
                return true;
            }
        }

        if (!g_hasAirbotCruiseSpot)
        {
            return false;
        }

        *spot = g_airbotCruiseSpot;
        return true;
    }

    int AirBotGetDpId(void (*handler)(TY_OBJ_DP_S *))
    {
        for (size_t i = 0; i < sizeof(kAirBotDpHandlers) / sizeof(kAirBotDpHandlers[0]); ++i)
        {
            if (kAirBotDpHandlers[i].handler == handler)
            {
                return kAirBotDpHandlers[i].dpId;
            }
        }
        return 0;
    }

    int AirBotGetRawDpId(void (*handler)(int dpId, uint8_t *data, int len))
    {
        for (size_t i = 0; i < sizeof(kAirBotRawDpHandlers) / sizeof(kAirBotRawDpHandlers[0]); ++i)
        {
            if (kAirBotRawDpHandlers[i].handler == handler)
            {
                return kAirBotRawDpHandlers[i].dpId;
            }
        }
        return 0;
    }

} // namespace

RobotAirBot::RobotAirBot()
    : initialized(false)
{
}

RobotAirBot *RobotAirBot::Get()
{
    static RobotAirBot robotAirBot;
    return &robotAirBot;
}

void RobotAirBot::Init()
{
    if (initialized)
    {
        return;
    }

    initialized = true;
    TuyaComm::Get()->Subscribe("ty_Map", &RobotAirBot::OnMap, this);
    TuyaComm::Get()->Subscribe("ty_Path", &RobotAirBot::OnPath, this);
    TuyaComm::Get()->Subscribe("ty_robot_state", &RobotAirBot::OnCruiseStatus, this);
    TuyaComm::Get()->Subscribe("ty_SaveMap", &RobotAirBot::OnSaveMap, this);
    TuyaComm::Get()->Subscribe("ty_UpdateMap", &RobotAirBot::OnUpdateMap, this);
    TuyaComm::Get()->Subscribe("Mevt", &RobotAirBot::OnEvent, this);
    TuyaComm::Get()->Subscribe("Kevt", &RobotAirBot::OnEvent, this);
    TuyaComm::Get()->Subscribe("Mtest", &RobotAirBot::OnTest, this);
    TuyaComm::Get()->Subscribe("MAirMsg", &RobotAirBot::OnAirMsg, this);
}

void RobotAirBot::OnMap(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const AppMap *msg)
{
    if (msg == NULL)
    {
        return;
    }
    TuyaMap tuyaMap = ToTuyaMap(*msg);
    SaveTuyaMap(tuyaMap, "/tmp/airbot_map.bin");
}

void RobotAirBot::OnPath(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const AppPath *msg)
{
    if (msg == NULL)
    {
        return;
    }
    path = *msg;
    TuyaPath tuyaPath = ToTuyaPath(*msg, msg->id, 0);
    SaveTuyaPath(tuyaPath, "/tmp/airbot_path.bin");
}

void RobotAirBot::OnCmd(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const Debug *msg)
{
}

void RobotAirBot::OnEvent(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const Event *msg)
{
    if (msg == NULL)
    {
        return;
    }

    LOGD(TAG, "Event: {}", M2S(msg->event));
    switch (msg->event)
    {
    case MSG_RESET_WIFI:
    case MSG_WIFI_SOFTAP:
        tuya_iot_wf_gw_unactive();
        break;
    default:
        break;
    }
}

void RobotAirBot::OnSaveMap(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const tuya_message::RobotEvent *msg)
{
}

void RobotAirBot::OnUpdateMap(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const tuya_message::RobotEvent *msg)
{
}

void RobotAirBot::OnTest(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const Event *msg)
{
}

void RobotAirBot::OnCruiseStatus(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const tuya_message::RobotState *msg)
{
    if (msg == NULL)
    {
        return;
    }

    std::vector<TY_OBJ_DP_S> dps;
    tuya_message::RobotState status = *msg;
    json j = status;
    LOGD(TAG, "AirBotStatus: {}", j.dump(2));

    dps.emplace_back(DPReport(&kAirBotDpTypes, 41, status.switch_go));      // 开关
    dps.emplace_back(DPReport(&kAirBotDpTypes, 42, status.pause));          // 暂停/继续
    dps.emplace_back(DPReport(&kAirBotDpTypes, 43, status.switch_charge));  // 一键回充
    dps.emplace_back(DPReport(&kAirBotDpTypes, 3, status.mode));            // 模式
    dps.emplace_back(DPReport(&kAirBotDpTypes, 44, status.status));         // 设备状态
    dps.emplace_back(DPReport(&kAirBotDpTypes, 45, status.battery));        // 电池电量
    dps.emplace_back(DPReport(&kAirBotDpTypes, 22, status.fault));          // 故障告警
    dps.emplace_back(DPReport(&kAirBotDpTypes, 53, status.volume_set));     // 音量设置
    dps.emplace_back(DPReport(&kAirBotDpTypes, 7, status.child_lock));      // 童锁
    dps.emplace_back(DPReport(&kAirBotDpTypes, 28, 0));                     // 静音
    dps.emplace_back(DPReport(&kAirBotDpTypes, 46, 0));                     // 隐私模式
    dps.emplace_back(DPReport(&kAirBotDpTypes, 51, 0));                     // 时间水印

    for (size_t i = 0; i < dps.size(); ++i)
    {
        dev_report_dp_json_async(NULL, &dps[i], 1);
    }
}

void RobotAirBot::ReportStatus()
{
    // tuya_message::Request req = {0};
    // tuya_message::RobotState state = {0};
    // LOGD(TAG, "获取设备状态");
    // if (TuyaComm::Get()->Send("ty_state", &req, &state, 1000))
    // {
    //     OnCruiseStatus(nullptr, "ty_robot_state", &state);
    // }
    // else
    // {
    //     LOGE(TAG, "无法获取到设备状态");
    // }
}

void RobotAirBot::ReportDeviceInfo()
{
}

void RobotAirBot::ReportMap()
{
}

void RobotAirBot::ReportPath()
{
}

void RobotAirBot::ReportMapAll()
{
}

void RobotAirBot::ReportAll()
{
    ReportStatus();
    ReportDeviceInfo();
}

bool RobotAirBot::CruiseStartMapping()
{
    return AirBotCruiseStartMapping();
}

bool RobotAirBot::CruiseGoto(const mars_message::AppSpotClean &spot)
{
    return AirBotCruiseGoto(spot);
}

bool RobotAirBot::CruiseSetPatrol(bool enable)
{
    return AirBotCruiseSetPatrol(enable);
}

bool RobotAirBot::CruiseStop()
{
    json reply;
    if (!AirBotSendCruiseRequest(json{{"cmd", "stop"}}, &reply))
    {
        return false;
    }
    g_airbotLastCruiseMode = "idle";
    AirBotReportCruiseSwitch(false);
    return true;
}

bool RobotAirBot::CruiseGetSpot(mars_message::AppSpotClean *spot)
{
    return AirBotCruiseGetSpot(spot);
}

const std::map<int, TuyaType> *RobotAirBot::DpTypes() const
{
    return &kAirBotDpTypes;
}

int RobotAirBot::GetDpId(RobotDpHandler handler) const
{
    return AirBotGetDpId(handler);
}

int RobotAirBot::GetRawDpId(RobotRawDpHandler handler) const
{
    return AirBotGetRawDpId(handler);
}

void RobotAirBot::HandleDPCmd(TY_OBJ_DP_S *dp)
{
    if (dp == NULL)
    {
        return;
    }

    LOGD(TAG, "dp cmd:{}, type:{}", dp->dpid, dp->type);
    for (size_t i = 0; i < sizeof(kAirBotDpHandlers) / sizeof(kAirBotDpHandlers[0]); ++i)
    {
        if (kAirBotDpHandlers[i].dpId == dp->dpid)
        {
            kAirBotDpHandlers[i].handler(dp);
            return;
        }
    }

    LOGD(TAG, "handler missing, dpId:{}", dp->dpid);
}

void RobotAirBot::HandleRawDPCmd(int dpId, uint8_t *data, int len)
{
    LOGD(TAG, "raw dp cmd:{}, len:{}", dpId, len);
    for (size_t i = 0; i < sizeof(kAirBotRawDpHandlers) / sizeof(kAirBotRawDpHandlers[0]); ++i)
    {
        if (kAirBotRawDpHandlers[i].dpId == dpId)
        {
            if (kAirBotRawDpHandlers[i].handler != nullptr)
            {
                kAirBotRawDpHandlers[i].handler(dpId, data, len);
            }
            return;
        }
    }

    LOGD(TAG, "raw handler missing, dpId:{}", dpId);
}

void RobotAirBot::OnAirMsg(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const mars_message::AirMsg *msg)
{
    mars_message::AirMsg lastestMsg;
    if(!(lastestMsg == *msg))
    {
        nlohmann::json j = *msg;
        LOGD(TAG, "AirMsg: {}",  j.dump(2));
        lastestMsg = *msg;
    }
    else
    {
        return;
    }
    if(msg->batQua != -1)
    {
        AirBotReportValueState(45, msg->batQua);
    }
    if(msg->airLevel != -1)
    {
        AirBotReportValueState(2, msg->airLevel);
    }
    if(msg->CH2O != -1)
    {
        AirBotReportValueState(27, msg->CH2O);
    }
    if(msg->fanLevel != -1)
    {
        AirBotReportValueState(54, msg->fanLevel);
    }
    if(msg->humidity != 0)
    {
        AirBotReportValueState(13, msg->humidity);
    }
    if(msg->pm10 != 0)
    {
        AirBotReportValueState(30, msg->pm10);
    }
    if(msg->pm25 != 0)
    {
        AirBotReportValueState(2, msg->pm25);
    }
    if(msg->temperature != 0)
    {
        AirBotReportValueState(12, msg->temperature);
    }
    if(msg->workState != 0)
    {
        AirBotReportEnumState(3, msg->workState);
    }
}