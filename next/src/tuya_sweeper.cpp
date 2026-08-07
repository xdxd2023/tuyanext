#include <netinet/in.h>
#include <stdint.h>

#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <vector>
#include "utils/log_.h"
#include "utils/json.hpp"

#include "tuya_cloud_com_defs.h"
#include "tuya_error_code.h"
#include "tuya_iot_wifi_api.h"
#include "tuya_hal_wifi.h"
#include "tuyaos_sdk.h"

#define __NLOHMANN_JSON_CPP
#include "robot_msg.h"
#include "tuya_sweeper.h"
#include "tuya_dp_function.h"
#include "tuya_dp_utils.h"
#include "tuya_enums.h"
#include "tuya_robot.h"
#include "tuya_utils.h"

#undef TAG
#define TAG "SWPR"

using json = nlohmann::json;

extern "C"
{
    OPERATE_RET tkl_wifi_get_ip(IN CONST WF_IF_E wf, OUT NW_IP_S *ip);
    OPERATE_RET tkl_wifi_get_mac(IN CONST WF_IF_E wf, INOUT NW_MAC_S *mac);
    OPERATE_RET tkl_wifi_station_get_conn_ap_rssi(OUT SCHAR_T *rssi);
    OPERATE_RET tkl_wifi_get_ssid(char *ssid);
}

int GetRawDpId(void (*dpHanlder)(int dpId, uint8_t *data, int len));
int TuyaUploadMapFile(int currId, uint32_t &tuyaMapId);
int TuyaUpdateMapFile(int currId, uint32_t tuyaMapId);
void TuyaReportMapSaveResult(int dpId, uint8_t cmd, uint8_t ret);

extern void *rkcamera;
extern std::string id;
extern std::string uuid;
extern std::string authKey;
extern std::string soft_version;
extern std::string MCU_version;
extern std::string system_version;


namespace {

std::map<int, TuyaType> sweeperDpType =
{
    {1, TuyaTypeBool},
    {2, TuyaTypeBool},
    {3, TuyaTypeBool},
    {4, TuyaTypeEnum},
    {5, TuyaTypeEnum},
    {6, TuyaTypeValue},
    {7, TuyaTypeValue},
    {8, TuyaTypeValue},
    {9, TuyaTypeEnum},
    {10, TuyaTypeEnum},
    {11, TuyaTypeBool},
    {12, TuyaTypeEnum},
    {13, TuyaTypeBool},
    {14, TuyaTypeRaw},
    {15, TuyaTypeRaw},
    {16, TuyaTypeEnum},
    {17, TuyaTypeValue},
    {18, TuyaTypeBool},
    {19, TuyaTypeValue},
    {20, TuyaTypeBool},
    {21, TuyaTypeValue},
    {22, TuyaTypeBool},
    {23, TuyaTypeValue},
    {24, TuyaTypeBool},
    {25, TuyaTypeBool},
    {26, TuyaTypeValue},
    {27, TuyaTypeBool},
    {28, TuyaTypeBitmap},   
    {29, TuyaTypeValue},
    {30, TuyaTypeValue},
    {31, TuyaTypeValue},
    {32, TuyaTypeRaw},
    {33, TuyaTypeRaw},
    {34, TuyaTypeRaw},
    {35, TuyaTypeRaw},
    {36, TuyaTypeEnum},
    {37, TuyaTypeValue},
    {38, TuyaTypeBool},
    {39, TuyaTypeBool},
    {40, TuyaTypeEnum},
    {41, TuyaTypeEnum},
    {42, TuyaTypeEnum},
    {43, TuyaTypeValue},
    {44, TuyaTypeEnum},
    {45, TuyaTypeBool},
    {46, TuyaTypeBool},
    {47, TuyaTypeBool},
    {48, TuyaTypeBool},
    {49, TuyaTypeBool},
    {50, TuyaTypeBool},
    {51, TuyaTypeValue},
    {101, TuyaTypeEnum},
    {102, TuyaTypeBool},
    {103, TuyaTypeBool},
    {104, TuyaTypeBool},
    {107, TuyaTypeValue},
    {147, TuyaTypeBool},
    {149, TuyaTypeBool},
    {151, TuyaTypeBool},
    {166, TuyaTypeBool},
};

std::map<int, RobotDpHandler> sweeperDpHandler =
{
    {1, DP_HandleSwichGo},
    {2, DP_HandlePause},
    {3, DP_HandleSwichCharge},
    {4, DP_HandleMode},
    {9, DP_HandleSuction},
    {10, DP_HandleCistern},
    {11, DP_HandleSeek},
    {12, DP_HandleDirectionControl},
    {13, DP_HandleMapReset},
    {16, DP_HandleRequest},
    {17, DP_HandleEdgeBrushLife},
    {18, DP_HandleEdgeBrushLifeReset},
    {19, DP_HandleRollBrushLife},
    {20, DP_HandleRollBrushLifeReset},
    {22, DP_HandleFilterLifeReset},
    {24, DP_HandleRagLifeReset},
    {25, DP_HandleDonotDisturb},
    {26, DP_HandleVolumeSet},
    {27, DP_HandleBreakClean},
    {28, DP_HandleFault},
    {36, DP_HandleLanguage},
    {37, DP_HandleDustCollectionNum},
    {38, DP_HandleDustCollectionSwitch},
    {39, DP_HandleCustomizeModeSwitch},
    {41, DP_HandleWorkMode},
    {42, DP_HandleUnitSet},
    {44, DP_HandleCarpetCleanPrefer},
    {45, DP_HandleAutoBoost},
    {46, DP_HandleCruiseSwitch},
    {47, DP_HandleChildLock},
    {48, DP48_HandleYMop},
    {49, DP_HandleSelfCleanMop},
    {50, DP_HandleDryingSwitch},
    {51, DP_HandleMopSlefCleaningFrequency},
    {52, DP_HandleMopSlefCleaningStrength},
    {53, DP_HandleWipingStrength},
    {101, DP_HandleAvoidObsMode},
    {102, DP_HandleStationLightDisplaySwitch},
    {103, DP_HandleKeyLightDisplaySwitch},
    {104, DP_HandleStationSelfClean},
    {107, DP_HandleStationHotFanTime},
    {147, DP_HandleStationHotFan},
    {149, DP_HandleStationHotWater},
    {151, DP_HandleSmartEdge},
    {166, DP_HandleUploadLogs},
};

struct TuyaDPRawHandler
{
    int dpId;
    void (*dpHanlder)(int dpId, uint8_t *data, int len);
};

TuyaDPRawHandler _dpRawHandler[] =
{
    {14, DPRaw_HandlePathData},
    {15, DPRaw_HandleCommand},
    {32, DPRaw_HandleDeviceTimer},
    {33, DPRaw_HandleDisturbTimeSet},
    {35, DPRaw_HandleVoiceData},
};

int g_sweeperBattery = 0;

void video_main_cb(void *ptr, size_t len, bool is_key_frame)
{
}

void video_sub_cb(void *ptr, size_t len, bool is_key_frame)
{
}

} // namespace

RobotSweeper::RobotSweeper()
    : initialized(false)
{
}

RobotSweeper *RobotSweeper::Get()
{
    static RobotSweeper sweeper;
    return &sweeper;
}

std::map<int, TuyaType> &RobotSweeper::GetDpTypes()
{
    return sweeperDpType;
}

std::map<int, RobotDpHandler> &RobotSweeper::GetDpHandlers()
{
    return sweeperDpHandler;
}

const std::map<int, TuyaType> *RobotSweeper::DpTypes() const
{
    return &sweeperDpType;
}

int RobotSweeper::GetDpId(RobotDpHandler handler) const
{
    for (auto &entry : sweeperDpHandler)
    {
        if (handler == entry.second)
        {
            return entry.first;
        }
    }
    return 0;
}

int RobotSweeper::GetRawDpId(RobotRawDpHandler dpHanlder) const
{
    for(int i=0; i<sizeof(_dpRawHandler)/sizeof(_dpRawHandler[0]); i++)
    {
        if(dpHanlder == _dpRawHandler[i].dpHanlder)
        {
            LOGD(TAG, "GetRawDpId id: {}", _dpRawHandler[i].dpId);
            return _dpRawHandler[i].dpId;
        }
    }
    LOGD(TAG, "GetRawDpId id: 0");
    return 0;
}


void RobotSweeper::HandleRawDPCmd(int dpId, uint8_t *data, int len)
{
    LOGD(TAG, "Raw DP cmd: {}", dpId);
    for(int i=0; i<sizeof(_dpRawHandler)/sizeof(_dpRawHandler[0]); i++)
    {
        if(dpId == _dpRawHandler[i].dpId)
        {
            _dpRawHandler[i].dpHanlder(dpId, data, len);
        }
    }
}

void RobotSweeper::HandleDPCmd(TY_OBJ_DP_S *dp)
{
    if (dp == NULL)
    {
        return;
    }

    auto it = sweeperDpHandler.find(dp->dpid);
    if (it == sweeperDpHandler.end())
    {
        return;
    }

    it->second(dp);
}

void RobotSweeper::TuyaReportRobotStatus(tuya_message::RobotState status)
{
    nlohmann::json j = status;
    LOGD(TAG, "ReportStatus: {}", j.dump(2));
    sweeper = status;
    g_sweeperBattery = status.battery;

    std::vector<TY_OBJ_DP_S> dps;
    dps.emplace_back(DPReport(&sweeperDpType, 1, status.switch_go));
    dps.emplace_back(DPReport(&sweeperDpType, 2, status.pause));
    dps.emplace_back(DPReport(&sweeperDpType, 3, status.switch_charge));
    dps.emplace_back(DPReport(&sweeperDpType, 4, status.mode));
    dps.emplace_back(DPReport(&sweeperDpType, 5, status.status));
    dps.emplace_back(DPReport(&sweeperDpType, 8, status.battery));
    dps.emplace_back(DPReport(&sweeperDpType, 9, status.suction));
    dps.emplace_back(DPReport(&sweeperDpType, 10, status.cistern));
    dps.emplace_back(DPReport(&sweeperDpType, 25, status.do_not_disturb));
    dps.emplace_back(DPReport(&sweeperDpType, 26, status.volume_set));
    dps.emplace_back(DPReport(&sweeperDpType, 27, status.break_clean));
    dps.emplace_back(DPReport(&sweeperDpType, 28, status.fault));
    dps.emplace_back(DPReport(&sweeperDpType, 36, status.language));
    dps.emplace_back(DPReport(&sweeperDpType, 37, status.dust_collection_num));
    dps.emplace_back(DPReport(&sweeperDpType, 38, status.dust_collection_switch));
    dps.emplace_back(DPReport(&sweeperDpType, 39, status.costomize_mode_switch));
    dps.emplace_back(DPReport(&sweeperDpType, 40, status.mop_state));
    dps.emplace_back(DPReport(&sweeperDpType, 41, status.work_mode));
    dps.emplace_back(DPReport(&sweeperDpType, 42, status.unit_set));
    dps.emplace_back(DPReport(&sweeperDpType, 44, status.carpet_clean_prefer));
    dps.emplace_back(DPReport(&sweeperDpType, 45, status.auto_boost));
    dps.emplace_back(DPReport(&sweeperDpType, 46, status.cruise_switch));
    dps.emplace_back(DPReport(&sweeperDpType, 47, status.child_lock));
    dps.emplace_back(DPReport(&sweeperDpType, 48, status.y_mop));
    dps.emplace_back(DPReport(&sweeperDpType, 49, status.self_clean));
    dps.emplace_back(DPReport(&sweeperDpType, 50, status.drying));
    dps.emplace_back(DPReport(&sweeperDpType, 51, status.self_clean_frequency));
    dps.emplace_back(DPReport(&sweeperDpType, 52, status.self_clean_strength));
    dps.emplace_back(DPReport(&sweeperDpType, 53, status.land_strength));
    dps.emplace_back(DPReport(&sweeperDpType, 101, status.smart_collision));
    if(status.extra_dp.size() >= 1)
    {
        dps.emplace_back(DPReport(&sweeperDpType, 103, status.extra_dp[0]));
    }
    if(status.extra_dp.size() >= 2)
    {
        dps.emplace_back(DPReport(&sweeperDpType, 102, status.extra_dp[1]));
    }
    if(status.extra_dp.size() >= 3)
    {
        dps.emplace_back(DPReport(&sweeperDpType, 104, status.extra_dp[2]));
    }
    dps.emplace_back(DPReport(&sweeperDpType, 107, status.mop_drying_timee));
    dps.emplace_back(DPReport(&sweeperDpType, 147, status.base_heat));
    dps.emplace_back(DPReport(&sweeperDpType, 149, status.base_mop_hot));
    dps.emplace_back(DPReport(&sweeperDpType, 151, status.smart_along_wall));
    dps.emplace_back(DPReport(&sweeperDpType, 166, status.log_upload));


    for (size_t i = 0; i < dps.size(); ++i)
    {
        dev_report_dp_json_async(NULL, &dps[i], 1);
    }
}

void RobotSweeper::Init()
{
    if (initialized)
    {
        return;
    }

    initialized = true;
    TuyaComm::Get()->Subscribe("ty_Map", &RobotSweeper::OnMap, this);
    TuyaComm::Get()->Subscribe("ty_Path", &RobotSweeper::OnPath, this);
    TuyaComm::Get()->Subscribe("ty_VirtualWall", &RobotSweeper::OnVirtualWall, this);
    TuyaComm::Get()->Subscribe("ty_RestrictedArea", &RobotSweeper::OnRestrictedArea, this);
    TuyaComm::Get()->Subscribe("ty_CleanRecord", &RobotSweeper::OnCleanRecord, this);
    TuyaComm::Get()->Subscribe("ty_m_schedule", &RobotSweeper::OnSchedule, this);
    TuyaComm::Get()->Subscribe("ty_RoomClean", &RobotSweeper::OnRoomClean, this);
    TuyaComm::Get()->Subscribe("ty_robot_state", &RobotSweeper::OnSweeperStatus, this);
    TuyaComm::Get()->Subscribe("ty_PartsLife", &RobotSweeper::OnPartsLife, this);
    TuyaComm::Get()->Subscribe("ty_CleanInfo", &RobotSweeper::OnCleanInfo, this);
    TuyaComm::Get()->Subscribe("ty_SaveMap", &RobotSweeper::OnSaveMap, this);
    TuyaComm::Get()->Subscribe("ty_UpdateMap", &RobotSweeper::OnUpdateMap, this);
    TuyaComm::Get()->Subscribe("Mevt", &RobotSweeper::OnEvent, this);
    TuyaComm::Get()->Subscribe("Kevt", &RobotSweeper::OnEvent, this);
    TuyaComm::Get()->Subscribe("Mtest", &RobotSweeper::OnTest, this);
}

void RobotSweeper::ReportDeviceInfo()
{
    LOGD(TAG, "上传设备信息");
    char ssid[128] = {0};
    tkl_wifi_get_ssid(ssid);
    std::string wifiName = ssid;

    SCHAR_T rssiT;
    tkl_wifi_station_get_conn_ap_rssi(&rssiT);
    int rssi = rssiT;
    LOGD(TAG, "wifiName:{} rssi:{}", wifiName, rssi);

    NW_IP_S ips = {0};
    tkl_wifi_get_ip(WF_STATION, &ips);
#ifdef RK3326_TUYAOS
    std::string ip = ips.ip;
#else
    std::string ip = ips.nwipstr;
#endif
    NW_MAC_S macs = {0};
    char macStr[32] = {0};
    tkl_wifi_get_mac(WF_STATION, &macs);
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", macs.mac[0], macs.mac[1], macs.mac[2], macs.mac[3], macs.mac[4], macs.mac[5]);
    std::string mac = macStr;
    std::string mcu_version;
    std::ifstream in_file("./mcu_version.txt");
    if (in_file.is_open())
    {
        getline(in_file, mcu_version);
        LOGD(TAG, "mcu version:{}", mcu_version);
        in_file.close();
    }
    std::string mcuVersion = mcu_version;
    std::string firmwareVersion = soft_version;
    std::string deviceSN = id;
    std::string moduleUUID = uuid;
    std::string baseStationSN = "BASE";
    std::string baseStationVersion = "1.0.0";
    std::string baseStationLocalVersion = "1.0.0";
    DP_ReportDeviceInfo(34, wifiName, rssiT, ip, mac, mcuVersion, firmwareVersion, deviceSN, moduleUUID, baseStationSN, baseStationVersion, baseStationLocalVersion);
}

void RobotSweeper::ReportStatus()
{
    tuya_message::Request req = {0};
    tuya_message::RobotState state = {0};
    LOGD(TAG, "获取设备状态");
    if (TuyaComm::Get()->Send("ty_state", &req, &state, 1000))
    {
        TuyaReportRobotStatus(state);
    }
    else
    {
        LOGE(TAG, "无法获取到设备状态");
    }
}

void RobotSweeper::OnMap(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const AppMap *msg)
{
    TuyaMap tuyaMap = ToTuyaMap(*msg);
    SaveTuyaMap(tuyaMap, "/tmp/map.bin");
}

void RobotSweeper::OnPath(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const AppPath *msg)
{
    path = *msg;
    TuyaPath tuyaPath = ToTuyaPath(*msg, msg->id, 0);
    SaveTuyaPath(tuyaPath, "/tmp/cleanPath.bin");
}

void RobotSweeper::OnCmd(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const Debug *msg)
{
}

void RobotSweeper::OnSweeperStatus(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const tuya_message::RobotState *msg)
{
    TuyaReportRobotStatus(*msg);
}

void RobotSweeper::OnPartsLife(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const AppPartsLife *msg)
{
    std::vector<TY_OBJ_DP_S> dps;
    if (partsLife.edgeBrushLifeMinutes != msg->edgeBrushLifeMinutes)
    {
        dps.emplace_back(DPReport(&sweeperDpType, 17, msg->edgeBrushLifeMinutes / 60));
    }
    if (partsLife.haipaLifeMinutes != msg->haipaLifeMinutes)
    {
        dps.emplace_back(DPReport(&sweeperDpType, 21, msg->haipaLifeMinutes / 60));
    }
    if (partsLife.mainBrushLifeMinutes != msg->mainBrushLifeMinutes)
    {
        dps.emplace_back(DPReport(&sweeperDpType, 19, msg->mainBrushLifeMinutes / 60));
    }
    if (partsLife.ragLifeMinutes != msg->ragLifeMinutes)
    {
        dps.emplace_back(DPReport(&sweeperDpType, 23, msg->ragLifeMinutes / 60));
    }
    if (!dps.empty())
    {
        dev_report_dp_json_async(NULL, &dps[0], dps.size());
    }
    partsLife = *msg;
}

void RobotSweeper::OnCleanInfo(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const AppCleanInfo *msg)
{
    LOGD(TAG, "cleanTime:{} cleanArea:{} cleanAreaTotal:{} cleanCountTotal:{} cleanTimeTotal:{}",
         msg->cleanTimeSecond, msg->cleanArea, msg->cleanAreaTotal, msg->cleanCountTotal, msg->cleanTimeTotalMinutes);

    std::vector<TY_OBJ_DP_S> dps;
    if (cleanInfo.cleanTimeSecond != msg->cleanTimeSecond)
    {
        dps.emplace_back(DPReport(&sweeperDpType, 6, msg->cleanTimeSecond / 60));
    }
    if (cleanInfo.cleanArea != msg->cleanArea)
    {
        dps.emplace_back(DPReport(&sweeperDpType, 7, msg->cleanArea));
    }
    if (cleanInfo.cleanAreaTotal != msg->cleanAreaTotal)
    {
        dps.emplace_back(DPReport(&sweeperDpType, 29, msg->cleanAreaTotal));
    }
    if (cleanInfo.cleanCountTotal != msg->cleanCountTotal)
    {
        dps.emplace_back(DPReport(&sweeperDpType, 30, msg->cleanCountTotal));
    }
    if (cleanInfo.cleanTimeTotalMinutes != msg->cleanTimeTotalMinutes)
    {
        dps.emplace_back(DPReport(&sweeperDpType, 31, msg->cleanTimeTotalMinutes));
    }
    if (cleanInfo.esimateArea != msg->esimateArea)
    {
        dps.emplace_back(DPReport(&sweeperDpType, 43, msg->esimateArea));
    }
    if (!dps.empty())
    {
        dev_report_dp_json_async(NULL, &dps[0], dps.size());
    }
    cleanInfo = *msg;
}

void RobotSweeper::OnVirtualWall(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const AppVirtualWall *msg)
{
    virtualWall = *msg;
    nlohmann::json j = *msg;
    LOGD(TAG, "OnVirtualWall: {}", j.dump(2));
    if (msg->version == 0)
    {
        TuyaReportVirtualWall(GetRawDpId(DPRaw_HandleCommand), 0x13, (AppVirtualWall *)msg);
    }
    else
    {
        TuyaReportVirtualWall(GetRawDpId(DPRaw_HandleCommand), 0x49, (AppVirtualWall *)msg);
    }
}

void RobotSweeper::OnRestrictedArea(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const AppRestrictedArea *msg)
{
    restrictedArea = *msg;
    nlohmann::json j = *msg;
    LOGD(TAG, "OnRestrictedArea: {}", j.dump(2));
    if (msg->version == 0)
    {
        TuyaReportRestrictedArea(GetRawDpId(DPRaw_HandleCommand), 0x1B, (AppRestrictedArea *)msg);
    }
    else
    {
        TuyaReportRestrictedArea(GetRawDpId(DPRaw_HandleCommand), 0x39, (AppRestrictedArea *)msg);
    }
}

void RobotSweeper::OnCleanRecord(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const AppCleanRecord *msg)
{
    for (int i = 0; i < 3; ++i)
    {
        if (ReportCleanRecords(msg->recordId, msg->cleanTimeSecond / 60.0 + 0.5, msg->cleanArea, msg->cleanMode, (CleanMethod)msg->cleanMethod, msg->finishResult, msg->startReason))
        {
            break;
        }
    }
}

void RobotSweeper::OnSchedule(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const AppLocalAlert *msg)
{
    TuyaReportLocalAlert(32, msg->verison == 0 ? 0x31 : 0x45, (AppLocalAlert *)msg);
}

void RobotSweeper::OnEvent(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const Event *msg)
{
    LOGD(TAG, "Event: {}", M2S(msg->event));
    switch (msg->event)
    {
    case MSG_RESET_WIFI:
        tuya_iot_wf_gw_unactive();
        break;
    }
}

void RobotSweeper::OnRoomClean(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const AppRoomClean *msg)
{
    TuyaReportRoomClean(GetRawDpId(DPRaw_HandleCommand), 0x15, (AppRoomClean *)msg);
}

void RobotSweeper::OnSaveMap(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const tuya_message::RobotEvent *msg)
{
    tuya_message::Request req = {0};
    tuya_message::Result res;
    LOGD(TAG, "OnSaveMap +++++++++++++++++++++++++++++++++++++++");
    if (TuyaComm::Get()->Send("ty_save_map", &req, &res, 1000) && res.code == 0)
    {
        LOGD(TAG, "保存成功");
        uint32_t tuyaMapId;
        int ret = TuyaUploadMapFile(res.a, tuyaMapId);
        if (ret == 0)
        {
            req.a = res.a;
            req.b = tuyaMapId;
            if (TuyaComm::Get()->Send("ty_connect_tuya_id", &req, &res))
            {
                TuyaReportMapSaveResult(GetRawDpId(DPRaw_HandleCommand), 0x2B, 0x01);
            }
            else
            {
                LOGD(TAG, "连接本地和云端地图失败");
                TuyaReportMapSaveResult(GetRawDpId(DPRaw_HandleCommand), 0x2B, 0x00);
            }
        }
    }
    else
    {
        LOGD(TAG, "保存失败");
        TuyaReportMapSaveResult(GetRawDpId(DPRaw_HandleCommand), 0x2B, 0x00);
    }
}

void RobotSweeper::OnUpdateMap(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const tuya_message::RobotEvent *msg)
{
    int mapId = msg->a;
    int tuyaMapId = msg->b;
    TuyaUpdateMapFile(mapId, tuyaMapId);
}

void RobotSweeper::OnTest(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const Event *msg)
{
    LOGD(TAG, "OnTest: {}", M2S(msg->event));
    if (msg->event == 'CAME')
    {
    }
}

void RobotSweeper::ReportPartsLife()
{
    LOGD(TAG, "ReportPartsLife");
    std::vector<TY_OBJ_DP_S> dps;
    AppPartsLife life = {0};
    tuya_message::Request req = {0};
    if (TuyaComm::Get()->Send("ty_get_parts_life", &req, &life))
    {
        AppPartsLife *msg = &life;
        dps.emplace_back(DPReport(&sweeperDpType, 17, msg->edgeBrushLifeMinutes / 60));
        dps.emplace_back(DPReport(&sweeperDpType, 19, msg->mainBrushLifeMinutes / 60));
        dps.emplace_back(DPReport(&sweeperDpType, 21, msg->haipaLifeMinutes / 60));
        dps.emplace_back(DPReport(&sweeperDpType, 23, msg->ragLifeMinutes / 60));
        if (!dps.empty())
        {
            dev_report_dp_json_async(NULL, &dps[0], dps.size());
        }
    }
    else
    {
        LOGE(TAG, "无法获取到 PartsLife");
    }
}

void RobotSweeper::ReportCleanInfo()
{
    LOGD(TAG, "ReportCleanInfo");
    std::vector<TY_OBJ_DP_S> dps;
    AppCleanInfo info = {};
    tuya_message::Request req = {0};
    if (TuyaComm::Get()->Send("ty_get_clean_info", &req, &info))
    {
        dps.emplace_back(DPReport(&sweeperDpType, 6, info.cleanTimeSecond / 60));
        dps.emplace_back(DPReport(&sweeperDpType, 7, info.cleanArea));
        dps.emplace_back(DPReport(&sweeperDpType, 29, info.cleanAreaTotal));
        dps.emplace_back(DPReport(&sweeperDpType, 30, info.cleanCountTotal));
        dps.emplace_back(DPReport(&sweeperDpType, 31, info.cleanTimeTotalMinutes));
        dps.emplace_back(DPReport(&sweeperDpType, 43, info.esimateArea));

        if (!dps.empty())
        {
            dev_report_dp_json_async(NULL, &dps[0], dps.size());
        }
    }
    else
    {
        LOGE(TAG, "无法获取到 CleanInfo");
    }

    LOGD(TAG, "ReportCleanSequence");
    AppSetCleaningSequence order = {};
    if (TuyaComm::Get()->Send("ty_get_cleaning_sequence", &req, &order))
    {
        TuyaReportCleaningSequence(GetRawDpId(DPRaw_HandleCommand), 0x27, &order);
    }
    else
    {
        LOGE(TAG, "无法获取到 CleanSequence");
    }
}

bool RobotSweeper::ReportCleanRecords(int recordId, int cleanTime, int cleanArea, int cleanMode, int workMode, int cleaningResult, int startMethod)
{
    time_t nowTime = time(NULL);
    struct tm *local = localtime(&nowTime);
    char endTime[50] = {0};
    strftime(endTime, 50, "%Y%m%d_%H%M%S", local);
    LOGD(TAG, "{}", endTime);

    char descript[128] = {0};

    std::string mapBin = "/tmp/map.bin";
    std::ifstream mapFile(mapBin, std::ios::binary);
    if (!mapFile.is_open())
    {
        LOGE(TAG, "Failed to open map file: {}", mapBin);
        return false;
    }
    std::vector<uint8_t> mapData((std::istreambuf_iterator<char>(mapFile)), std::istreambuf_iterator<char>());
    mapFile.close();

    std::string pathBin = "/tmp/cleanPath.bin";
    std::ifstream pathFile(pathBin, std::ios::binary);
    if (!pathFile.is_open())
    {
        LOGE(TAG, "Failed to open path file: {}", pathBin);
        return false;
    }
    std::vector<uint8_t> pathData((std::istreambuf_iterator<char>(pathFile)), std::istreambuf_iterator<char>());
    pathFile.close();

    AppVirtualWall wall;
    std::vector<uint8_t> tuyaVirtualWall;
    wall.version = 0;
    wall.count = 0;
    tuya_message::Request req = {};
    if (TuyaComm::Get()->Send("ty_get_virtual_wall", &req, &wall) && wall.version >= 0)
    {
        tuyaVirtualWall = ToVirtualWallData(wall.version == 0 ? 0x13 : 0x49, &wall);
    }
    else
    {
        LOGE(TAG, "无法获取到虚拟墙信息");
    }

    AppRestrictedArea area;
    std::vector<uint8_t> tuyaRestrictedArea;
    if (TuyaComm::Get()->Send("ty_get_restricted_area", &req, &area) && area.version >= 0)
    {
        tuyaRestrictedArea = ToRestrictedAreaData(area.version == 0 ? 0x1B : 0x39, &area);
    }
    else
    {
        LOGE(TAG, "无法获取到禁区/划区信息");
    }

    sprintf(descript, "%d_%s_%d_%d_%d_%d_%d_%d_%d_%d_%d", recordId, endTime,
            cleanTime, cleanArea, mapData.size(), pathData.size(), tuyaVirtualWall.size() + tuyaRestrictedArea.size(), cleanMode, workMode, cleaningResult, startMethod);
    LOGD(TAG, "upload_record record id: {} , descript: {}", recordId, descript);

    std::vector<uint8_t> combinedData;
    combinedData.insert(combinedData.end(), mapData.begin(), mapData.end());
    combinedData.insert(combinedData.end(), pathData.begin(), pathData.end());
    combinedData.insert(combinedData.end(), tuyaVirtualWall.begin(), tuyaVirtualWall.end());
    combinedData.insert(combinedData.end(), tuyaRestrictedArea.begin(), tuyaRestrictedArea.end());

    OPERATE_RET op_ret = tuya_iot_map_record_upload_buffer(
        recordId, combinedData.data(), combinedData.size(), ((strlen(descript) == 0) ? NULL : descript));
    if (op_ret == OPRT_OK)
    {
        LOGD(TAG, "upload map id:{} record files OK", recordId);
    }
    else
    {
        LOGE(TAG, "upload map id:{} record files fail, ret {}", recordId, op_ret);
        return false;
    }

    return true;
}

void RobotSweeper::ReportMap()
{
}

void RobotSweeper::ReportPath()
{
}

void RobotSweeper::ReportMapAll()
{
    LOGD(TAG, "ReportMapAll");
    AppRestrictedArea area;
    tuya_message::Request req = {};
    if (TuyaComm::Get()->Send("ty_get_restricted_area", &req, &area) && area.version >= 0)
    {
        LOGD(TAG, "ReportMapAll --------------------------------------- {}", GetRawDpId(DPRaw_HandleCommand));
        TuyaReportRestrictedArea(GetRawDpId(DPRaw_HandleCommand), area.version == 0 ? 0x1B : 0x39, &area);
    }
    else
    {
        LOGE(TAG, "无法获取到禁区信息");
    }

    AppZoneClean zone;
    if (TuyaComm::Get()->Send("ty_get_zone_clean", &req, &zone) && zone.count > 0)
    {
        TuyaReportZoneClean(GetRawDpId(DPRaw_HandleCommand), zone.version == 0 ? 0x29 : 0x3A, &zone);
    }
    else
    {
        LOGE(TAG, "无法获取到划区信息");
    }

    AppRoomClean room;
    if (TuyaComm::Get()->Send("ty_get_clean_blocks", &req, &room) && room.count > 0)
    {
        TuyaReportRoomClean(GetRawDpId(DPRaw_HandleCommand), 0x15, &room);
    }
    else
    {
        LOGE(TAG, "无法获取到分区清扫信息");
    }

    AppVirtualWall wall;
    if (TuyaComm::Get()->Send("ty_get_virtual_wall", &req, &wall) && wall.version >= 0)
    {
        TuyaReportVirtualWall(GetRawDpId(DPRaw_HandleCommand), wall.version == 0 ? 0x13 : 0x49, &wall);
    }
    else
    {
        LOGE(TAG, "无法获取到虚拟墙信息");
    }
}

void RobotSweeper::ReportAll()
{
    AppLocalAlert alert;
    tuya_message::Request req = {};
    if (TuyaComm::Get()->Send("ty_get_m_orders", &req, &alert) && alert.verison >= 0)
    {
        TuyaReportLocalAlert(32, alert.verison == 0 ? 0x31 : 0x45, &alert);
    }
    else
    {
        LOGE(TAG, "无法获取到本地定时信息");
    }

    AppNotDisturbTime disturb;
    if (TuyaComm::Get()->Send("ty_get_dnd", &req, &disturb) && disturb.version >= 0)
    {
        TuyaReportNotDisturbTime(33, disturb.version == 0 ? 0x33 : 0x41, &disturb);
    }
    else
    {
        LOGE(TAG, "无法获取到勿扰时间信息");
    }

    ReportStatus();
    ReportDeviceInfo();
    ReportPartsLife();
    ReportCleanInfo();
    ReportMapAll();
}
