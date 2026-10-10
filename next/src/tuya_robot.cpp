#include <stdint.h>
#include <netinet/in.h>

#include <algorithm>
#include <cstring>
#include <vector>
#include <map>
#include <fstream>

#include <utils/json.hpp>
#include "utils/log_.h"
#include "lz4/lz4.h"

#include "tuya_cloud_com_defs.h"
#include "tuya_error_code.h"
#include "tuya_iot_wifi_api.h"
#include "tuya_svc_upgrade.h"
#include "ty_rvc_func.h"

#define __NLOHMANN_JSON_CPP
#include "robot_msg.h"
#include "voice.h"
#include "tuya_robot.h"
#include "tuya_pet.h"
#include "tuya_airbot.h"
#include "tuya_sweeper.h"
#include "tuya_enums.h"
#include "tuya_dp_function.h"
#include "tuya_utils.h"
#include "tuya_dp_utils.h"
#include "tuyaos_sdk.h"


#define TAG "TUY"

#define APP_VERSION_FILE "/userdata/version/soft_version.json" 
extern std::string robot_type;

void DPRaw_HandleCommand(int dpId, uint8_t *data, int len);
int TuyaUploadMapFile(int currId, uint32_t &tuyaMapId);

void TuyaHandleDPQuery(int dpId)
{
    LOGD(TAG, "DP query: {}", dpId);
    switch (dpId)
    {
    case 1:
        break;
    case 4:
        break;
    default:
        break;
    }
}
namespace {

RobotDevice *ResolveRobot()
{
    if(robot_type == "Sweeper")
    {
        return RobotSweeper::Get();
    }
    else if(robot_type == "AirBot")
    {
        return RobotAirBot::Get();
    }
    else
    {
        return RobotPet::Get();
    }
}

std::string ToStdString(const STR_ELEMENT_S &element)
{
    if (element.name == nullptr || element.len <= 0)
    {
        return {};
    }
    return std::string(element.name, element.name + element.len);
}

int ToTuyaModeValue(const STR_ELEMENT_S &element)
{
    const std::string name = ToStdString(element);
    if (name == "chargego" || name == "goto_charge")
    {
        return DP4_CHARGE_GO;
    }
    if (name == "zone")
    {
        return DP4_ZONE;
    }
    if (name == "pose")
    {
        return DP4_POSE;
    }
    if (name == "part")
    {
        return DP4_PART;
    }
    if (name == "select_room")
    {
        return DP4_SELECT_ROOM;
    }
    return DP4_SMART;
}

int ToTuyaSuctionValue(const STR_ELEMENT_S &element)
{
    const std::string name = ToStdString(element);
    if (name == "closed")
    {
        return DP9_CLOSED;
    }
    if (name == "gentle" || name == "quiet")
    {
        return DP9_GENTLE;
    }
    if (name == "strong")
    {
        return DP9_STRONG;
    }
    if (name == "max")
    {
        return DP9_MAX;
    }
    return DP9_NORMAL;
}

int ToTuyaCisternValue(const STR_ELEMENT_S &element)
{
    const std::string name = ToStdString(element);
    if (name == "closed")
    {
        return DP10_CLOSED;
    }
    if (name == "low")
    {
        return DP10_LOW;
    }
    if (name == "high")
    {
        return DP10_HIGH;
    }
    return DP10_MIDDLE;
}

std::string ToRoomName(const STR_ELEMENT_S &element)
{
    std::string name = ToStdString(element);
    const size_t zeroPos = name.find('\0');
    if (zeroPos != std::string::npos)
    {
        name.resize(zeroPos);
    }
    return name;
}

int CStringLen(const char *text)
{
    if (text == nullptr)
    {
        return 0;
    }

    int len = 0;
    while (text[len] != '\0')
    {
        ++len;
    }
    return len;
}

void AssignChars(std::vector<char> &out, const char *text, int len)
{
    out.clear();
    if (text == nullptr || len <= 0)
    {
        return;
    }

    out.reserve(len);
    for (int i = 0; i < len; ++i)
    {
        out.push_back(text[i]);
    }
}

void AssignChars(std::vector<char> &out, const std::string &text)
{
    out.clear();
    out.reserve(text.size());
    for (size_t i = 0; i < text.size(); ++i)
    {
        out.push_back(text[i]);
    }
}

const char *ToModeName(int mode)
{
    switch (mode)
    {
    case DP4_CHARGE_GO:
        return "chargego";
    case DP4_ZONE:
        return "zone";
    case DP4_POSE:
        return "pose";
    case DP4_PART:
        return "part";
    case DP4_SELECT_ROOM:
        return "select_room";
    default:
        return "smart";
    }
}

const char *ToSuctionName(int suction)
{
    switch (suction)
    {
    case DP9_CLOSED:
        return "closed";
    case DP9_GENTLE:
        return "gentle";
    case DP9_STRONG:
        return "strong";
    case DP9_MAX:
        return "max";
    default:
        return "normal";
    }
}

const char *ToCisternName(int cistern)
{
    switch (cistern)
    {
    case DP10_CLOSED:
        return "closed";
    case DP10_LOW:
        return "low";
    case DP10_HIGH:
        return "high";
    default:
        return "middle";
    }
}

const char *ComplexCmdName(COMPLEX_CMD_E cmd)
{
    switch (cmd)
    {
    case VIRTUAL_WALL_SET:
        return "VIRTUAL_WALL_SET";
    case VIRTUAL_WALL_QUERY:
        return "VIRTUAL_WALL_QUERY";
    case RESTRICTED_AREA_SET:
        return "RESTRICTED_AREA_SET";
    case RESTRICTED_AREA_QUERY:
        return "RESTRICTED_AREA_QUERY";
    case ROOM_CLEAN_SET:
        return "ROOM_CLEAN_SET";
    case ROOM_CLEAN_QUERY:
        return "ROOM_CLEAN_QUERY";
    case SPOT_CLEAN_SET:
        return "SPOT_CLEAN_SET";
    case SPOT_CLEAN_QUERY:
        return "SPOT_CLEAN_QUERY";
    case ZONE_CLEAN_SET:
        return "ZONE_CLEAN_SET";
    case ZONE_CLEAN_QUERY:
        return "ZONE_CLEAN_QUERY";
    case ROOM_PROPERTY_SET:
        return "ROOM_PROPERTY_SET";
    case ROOM_PROPERTY_QUERY:
        return "ROOM_PROPERTY_QUERY";
    case SCHEDULE_SET:
        return "SCHEDULE_SET";
    case SCHEDULE_QUERY:
        return "SCHEDULE_QUERY";
    case QUIET_HOUR_SET:
        return "QUIET_HOUR_SET";
    case QUIET_HOUR_QUERY:
        return "QUIET_HOUR_QUERY";
    case PART_DIVI_SET:
        return "PART_DIVI_SET";
    case PART_MERGE_SET:
        return "PART_MERGE_SET";
    case PART_DEFAULT_SET:
        return "PART_DEFAULT_SET";
    case RESET_CURR_MAP_SET:
        return "RESET_CURR_MAP_SET";
    case SAVE_CURR_MAP_SET:
        return "SAVE_CURR_MAP_SET";
    case DELETE_CLOUD_MAP_SET:
        return "DELETE_CLOUD_MAP_SET";
    case VOICE_LANGUAGE_QUERY:
        return "VOICE_LANGUAGE_QUERY";
    case DEV_INFO_QUERY:
        return "DEV_INFO_QUERY";
    case AI_OBJECT_QUERY:
        return "AI_OBJECT_QUERY";
    case AI_OBJECT_TYPE_SET:
        return "AI_OBJECT_TYPE_SET";
    case AI_OBJECT_IGNORE_SET:
        return "AI_OBJECT_IGNORE_SET";
    case MAP_EMPTY_QUERY:
        return "MAP_EMPTY_QUERY";
    case PASSWORD_STATE_QUERY:
        return "PASSWORD_STATE_QUERY";
    case PASSWORD_CHECK:
        return "PASSWORD_CHECK";
    case PASSWORD_SET:
        return "PASSWORD_SET";
    case MCS_ROOM_INFO_QUERY:
        return "MCS_ROOM_INFO_QUERY";
    default:
        return "UNKNOWN_COMPLEX_CMD";
    }
}

void LogAdvanceResult(const char *kind, COMPLEX_CMD_E cmd, bool success, const char *detail = nullptr)
{
    if (success)
    {
        LOGD(TAG, "{} {} success{}{}", kind, ComplexCmdName(cmd), detail ? ": " : "", detail ? detail : "");
    }
    else
    {
        LOGE(TAG, "{} {} failed{}{}", kind, ComplexCmdName(cmd), detail ? ": " : "", detail ? detail : "");
    }
}


} // namespace

RobotDevice *GetRobot()
{
    return ResolveRobot();
}


void TuyaHandleRawDPCmd(int dpId, uint8_t *data, int len)
{
    LOGD(TAG, "Raw DP cmd: {}", dpId);
    GetRobot()->HandleRawDPCmd(dpId, data, len);
}

int GetDpId(void (*dpHanlder)(TY_OBJ_DP_S*))
{
    return GetRobot()->GetDpId(dpHanlder);
}


void TuyaHandleDPCmd(TY_OBJ_DP_S *dp)
{
    LOGD(TAG, "DP cmd: {}", dp->dpid);
    GetRobot()->HandleDPCmd(dp);
}

int TuyaAdvanceFuctionSet(COMPLEX_CMD_E cmd, void* param)
{
    LOGD(TAG, "Advance function set: {}", (int)cmd);
    int ret = OPRT_OK;

    switch (cmd)
    {
    case VIRTUAL_WALL_SET:
        {
            auto *src = static_cast<VIRTUAL_WALL_S *>(param);
            if (src == nullptr)
            {
                LogAdvanceResult("set", cmd, false, "param is null");
                return -1;
            }

            AppVirtualWall wall;
            wall.version = 1;
            wall.count = src->num;
            for (int i = 0; i < src->num; ++i)
            {
                wall.mode.push_back(src->line[i].mode);
                wall.wall.push_back(Segment{TuyaXYToMars(src->line[i].points[0].x, src->line[i].points[0].y),
                                            TuyaXYToMars(src->line[i].points[1].x, src->line[i].points[1].y)});
            }

            tuya_message::Result res = {};
            if (TuyaComm::Get()->Send("ty_virtual_wall", &wall, &res, 1000))
            {
                src->map_id = 0;
                ty_rvc_virtual_wall_data_response(src, 0);
                LogAdvanceResult("set", cmd, true);
            }
            else
            {
                ret = OPRT_COM_ERROR;
                LogAdvanceResult("set", cmd, false, "send ty_virtual_wall failed");
            }
        }
        break;
    case RESTRICTED_AREA_SET:
        {
            auto *src = static_cast<RESTRICTED_AREA_S *>(param);
            if (src == nullptr)
            {
                LogAdvanceResult("set", cmd, false, "param is null");
                return -1;
            }

            AppRestrictedArea area;
            area.version = 1;
            area.count = src->num;
            for (int i = 0; i < src->num; ++i)
            {
                area.mode.push_back(src->restrict_zone[i].mode);
                area.type.push_back(0);
                Polygon polygon;
                polygon.count = src->restrict_zone[i].area.point_num;
                for (int j = 0; j < polygon.count; ++j)
                {
                    polygon.vetex.push_back(TuyaXYToMars(src->restrict_zone[i].area.point[j].x,
                                                         src->restrict_zone[i].area.point[j].y));
                }
                area.polygon.emplace_back(polygon);
                area.name.push_back(ToRoomName(src->restrict_zone[i].cur_name));
            }

            tuya_message::Result res = {};
            if (TuyaComm::Get()->Send("ty_restricted_area", &area, &res, 1000))
            {
                src->map_id = 0;
                ty_rvc_restricted_area_data_response(src, 0);
                LogAdvanceResult("set", cmd, true);
            }
            else
            {
                ret = OPRT_COM_ERROR;
                LogAdvanceResult("set", cmd, false, "send ty_restricted_area failed");
            }
        }
        break;
    case ROOM_CLEAN_SET:
        {
            auto *src = static_cast<ROOM_CLEAN_S *>(param);
            if (src == nullptr)
            {
                LogAdvanceResult("set", cmd, false, "param is null");
                return -1;
            }

            AppRoomCleanV2 room;
            room.version = 1;
            room.count = src->num;
            for (int i = 0; i < src->num; ++i)
            {
                room.id.push_back(src->ids[i]);
                room.fan_level.push_back(ToTuyaSuctionValue(src->param[i].suction));
                room.water_level.push_back(ToTuyaCisternValue(src->param[i].cistern));
                room.y_mop.push_back(src->param[i].y_mop);
                room.cleanRepeat.push_back(src->param[i].clean_cnt);
            }

            tuya_message::Result res = {};
            if (TuyaComm::Get()->Send("ty_clean_blocks_v2", &room, &res, 1000))
            {
                src->map_id = 0;
                ty_rvc_room_clean_data_response(src, 0);
                LogAdvanceResult("set", cmd, true);
            }
            else
            {
                ret = OPRT_COM_ERROR;
                LogAdvanceResult("set", cmd, false, "send ty_clean_blocks_v2 failed");
            }
        }
        break;
    case SPOT_CLEAN_SET:
        {
            auto *src = static_cast<SPOT_CLEAN_S *>(param);
            if (src == nullptr || src->num <= 0)
            {
                LogAdvanceResult("set", cmd, false, "param invalid");
                return -1;
            }

            AppSpotClean clean;
            clean.version = 0;
            clean.cleanRepeat = src->param[0].clean_cnt;
            clean.spot = TuyaXYToMars(src->points[0].x, src->points[0].y);

            tuya_message::Result res = {};
            if (TuyaComm::Get()->Send("ty_point_point", &clean, &res, 1000))
            {
                src->map_id = 0;
                ty_rvc_spot_clean_data_response(src, 0);
                LogAdvanceResult("set", cmd, true);
            }
            else
            {
                ret = OPRT_COM_ERROR;
                LogAdvanceResult("set", cmd, false, "send ty_point_point failed");
            }
        }
        break;
    case ZONE_CLEAN_SET:
        {
            auto *src = static_cast<ZONE_CLEAN_S *>(param);
            if (src == nullptr)
            {
                LogAdvanceResult("set", cmd, false, "param is null");
                return -1;
            }

            AppZoneClean zone;
            zone.version = 1;
            zone.count = src->num;
            zone.cleanRepeat.push_back(src->num > 0 ? src->clean_zone[0].param.clean_cnt : 1);
            for (int i = 0; i < src->num; ++i)
            {
                Polygon polygon;
                polygon.count = src->clean_zone[i].area.point_num;
                for (int j = 0; j < polygon.count; ++j)
                {
                    polygon.vetex.push_back(TuyaXYToMars(src->clean_zone[i].area.point[j].x,
                                                         src->clean_zone[i].area.point[j].y));
                }
                zone.zone.emplace_back(polygon);
                zone.name.push_back(ToRoomName(src->clean_zone[i].cur_name));
            }

            tuya_message::Result res = {};
            if (TuyaComm::Get()->Send("ty_zone_clean", &zone, &res, 1000))
            {
                src->map_id = 0;
                ty_rvc_zone_clean_data_response(src, 0);
                LogAdvanceResult("set", cmd, true);
            }
            else
            {
                ret = OPRT_COM_ERROR;
                LogAdvanceResult("set", cmd, false, "send ty_zone_clean failed");
            }
        }
        break;
    case ROOM_PROPERTY_SET:
        {
            auto *src = static_cast<ROOM_PROPERTY_S *>(param);
            if (src == nullptr)
            {
                LogAdvanceResult("set", cmd, false, "param is null");
                return -1;
            }

            AppSetRoomProperties properties;
            AppSetRoomName names;
            AppSetCleaningSequence sequence;
            std::vector<std::pair<int, int>> sequenceOrder;

            properties.version = 0;
            properties.count = src->num;
            names.version = 0;
            names.count = src->num;
            sequence.version = 0;
            sequence.count = src->num;

            for (int i = 0; i < src->num; ++i)
            {
                properties.roomId.push_back(src->ids[i]);
                properties.enable.push_back(src->param[i].valid ? 1 : 0);
                properties.fan.push_back(ToTuyaSuctionValue(src->param[i].suction));
                properties.waterLevel.push_back(ToTuyaCisternValue(src->param[i].cistern));
                properties.yMop.push_back(src->param[i].y_mop);
                properties.cleanRepeat.push_back(src->param[i].clean_cnt);

                names.roomId.push_back(src->ids[i]);
                names.name.push_back(ToRoomName(src->cus_name[i]));

                sequenceOrder.emplace_back(src->orders[i], src->ids[i]);
            }

            std::sort(sequenceOrder.begin(), sequenceOrder.end());
            for (const auto &item : sequenceOrder)
            {
                sequence.roomId.push_back(item.second);
            }

            tuya_message::Result res = {};
            bool okProperties = TuyaComm::Get()->Send("ty_set_room_properties", &properties, &res, 1000);
            bool okNames = okProperties && TuyaComm::Get()->Send("ty_set_room_name", &names, &res, 1000);
            bool okSequence = okNames && TuyaComm::Get()->Send("ty_clean_sequence", &sequence, &res, 1000);
            bool ok = okProperties && okNames && okSequence;
            if (ok)
            {
                src->map_id = 0;
                ty_rvc_room_property_data_response(src, 0);
                LogAdvanceResult("set", cmd, true);
            }
            if (!ok)
            {
                ret = OPRT_COM_ERROR;
                if (!okProperties)
                {
                    LogAdvanceResult("set", cmd, false, "send ty_set_room_properties failed");
                }
                else if (!okNames)
                {
                    LogAdvanceResult("set", cmd, false, "send ty_set_room_name failed");
                }
                else
                {
                    LogAdvanceResult("set", cmd, false, "send ty_clean_sequence failed");
                }
            }
        }
        break;
    case SCHEDULE_SET:
        {
            auto *src = static_cast<SCHEDULE_S *>(param);
            if (src == nullptr)
            {
                LogAdvanceResult("set", cmd, false, "param is null");
                return -1;
            }

            AppLocalAlert alert;
            alert.verison = 0;
            alert.timeZone = 0x08;
            alert.number = src->num;
            alert.timedTaskInfo.resize(src->num);

            for (int i = 0; i < src->num; ++i)
            {
                auto &task = alert.timedTaskInfo[i];
                task.valid = src->time_sets[i].active;
                task.weekDayMask = src->time_sets[i].cycle;
                task.time = (src->time_sets[i].start_time.hour << 8) | src->time_sets[i].start_time.min;
                task.mapId = src->map_id;
                task.roomN = 0;
                task.zoomN = 0;

                for (int j = 0; j < src->time_sets[i].num; ++j)
                {
                    if (src->time_sets[i].ids[j] < 0)
                    {
                        continue;
                    }
                    task.roomId.push_back(src->time_sets[i].ids[j]);
                }
                task.roomN = task.roomId.size();

                if (src->time_sets[i].num > 0)
                {
                    task.cleanMode = ToTuyaModeValue(src->time_sets[i].mode[0]);
                    task.fan = ToTuyaSuctionValue(src->time_sets[i].param[0].suction);
                    task.water = ToTuyaCisternValue(src->time_sets[i].param[0].cistern);
                    task.cleanRepeat = src->time_sets[i].param[0].clean_cnt;
                }
            }

            tuya_message::Result res = {};
            if (TuyaComm::Get()->Send("ty_set_m_orders", &alert, &res, 1000))
            {
                src->map_id = 0;
                ty_rvc_schedule_data_response(src, 0);
                LogAdvanceResult("set", cmd, true);
            }
            else
            {
                ret = OPRT_COM_ERROR;
                LogAdvanceResult("set", cmd, false, "send ty_set_m_orders failed");
            }
        }
        break;
    case QUIET_HOUR_SET:
        {
            auto *src = static_cast<QUIET_HOURS_S *>(param);
            if (src == nullptr)
            {
                LogAdvanceResult("set", cmd, false, "param is null");
                return -1;
            }

            AppNotDisturbTime time;
            time.version = 1;
            time.onoff = src->active;
            time.timeZone = 0x08;
            time.starTime.hour = src->start_time.hour;
            time.starTime.minute = src->start_time.min;
            time.endTime.hour = src->end_time.hour;
            time.endTime.minute = src->end_time.min;
            time.startWeekDay = 0;
            time.endWeekDay = src->other_day;

            tuya_message::Result res = {};
            if (TuyaComm::Get()->Send("ty_set_dnd", &time, &res, 1000))
            {
                ty_rvc_quiet_hours_data_response(src, 0);
                LogAdvanceResult("set", cmd, true);
            }
            else
            {
                ret = OPRT_COM_ERROR;
                LogAdvanceResult("set", cmd, false, "send ty_set_dnd failed");
            }
        }
        break;
    case PART_DIVI_SET:
        {
            auto *src = static_cast<PART_DIVI_S *>(param);
            if (src == nullptr)
            {
                LogAdvanceResult("set", cmd, false, "param is null");
                return -1;
            }

            AppPartitionDivision division;
            division.version = 0;
            division.roomId = src->ids[0];
            division.begin = TuyaXYToMars(src->divi_line[0].points[0].x, src->divi_line[0].points[0].y);
            division.end = TuyaXYToMars(src->divi_line[0].points[1].x, src->divi_line[0].points[1].y);
            division.ret = 1;

            tuya_message::Result res = {};
            if (TuyaComm::Get()->Send("ty_split_partition", &division, &res, 1000))
            {
                ty_rvc_part_division_data_response(src, res.code == 0 ? PART_DIV_SUC : PART_DIV_FAILED);
                LogAdvanceResult("set", cmd, res.code == 0, res.code == 0 ? nullptr : "partition divide response failed");
            }
            else
            {
                ret = OPRT_COM_ERROR;
                LogAdvanceResult("set", cmd, false, "send ty_split_partition failed");
            }
        }
        break;
    case PART_MERGE_SET:
        {
            auto *src = static_cast<PART_MERGE_S *>(param);
            if (src == nullptr)
            {
                LogAdvanceResult("set", cmd, false, "param is null");
                return -1;
            }

            AppPartitionMerge merge;
            merge.version = 0;
            merge.count = src->num;
            merge.ret = 1;
            for (int i = 0; i < src->num; ++i)
            {
                merge.roomId.push_back(src->ids[i]);
            }

            tuya_message::Result res = {};
            if (TuyaComm::Get()->Send("ty_merge_partition", &merge, &res, 1000))
            {
                ty_rvc_part_merge_data_response(src, res.code == 0 ? PART_MERGE_SUC : PART_MERGE_FAILED);
                LogAdvanceResult("set", cmd, res.code == 0, res.code == 0 ? nullptr : "partition merge response failed");
            }
            else
            {
                ret = OPRT_COM_ERROR;
                LogAdvanceResult("set", cmd, false, "send ty_merge_partition failed");
            }
        }
        break;
    case RESET_CURR_MAP_SET:
        {
            tuya_message::Request req = {0};
            tuya_message::Result res = {};
            if (TuyaComm::Get()->Send("ty_reset_map", &req, &res, 1000))
            {
                CURRENT_MAP_RESET_S mapReset = {};
                ty_rvc_map_clear_data_response(&mapReset, MAP_RESET_SUC);
                LogAdvanceResult("set", cmd, true);
            }
            else
            {
                CURRENT_MAP_RESET_S mapReset = {};
                ty_rvc_map_clear_data_response(&mapReset, MAP_RESET_FAILED);
                ret = OPRT_COM_ERROR;
                LogAdvanceResult("set", cmd, false, "send ty_reset_map failed");
            }
        }
        break;
    case SAVE_CURR_MAP_SET:
        {
            auto *src = static_cast<MAP_SAVE_S *>(param);
            tuya_message::Request req = {0};
            tuya_message::Result res = {};
            if (TuyaComm::Get()->Send("ty_save_map", &req, &res, 1000) && res.code == 0)
            {
                uint32_t tuyaMapId = 0;
                if (TuyaUploadMapFile(res.a, tuyaMapId) == 0)
                {
                    req.a = res.a;
                    req.b = tuyaMapId;
                    if (TuyaComm::Get()->Send("ty_connect_tuya_id", &req, &res, 1000))
                    {
                        ty_rvc_map_save_data_response(src, MAP_SAVE_SUC);
                        LogAdvanceResult("set", cmd, true);
                    }
                    else
                    {
                        ty_rvc_map_save_data_response(src, MAP_SAVE_FAILED);
                        ret = OPRT_COM_ERROR;
                        LogAdvanceResult("set", cmd, false, "send ty_connect_tuya_id failed");
                    }
                }
                else
                {
                    ty_rvc_map_save_data_response(src, MAP_SAVE_FAILED);
                    ret = OPRT_COM_ERROR;
                    LogAdvanceResult("set", cmd, false, "TuyaUploadMapFile failed");
                }
            }
            else
            {
                ty_rvc_map_save_data_response(src, MAP_SAVE_FAILED);
                ret = OPRT_COM_ERROR;
                LogAdvanceResult("set", cmd, false, "send ty_save_map failed");
            }
        }
        break;
    case DELETE_CLOUD_MAP_SET:
        {
            auto *src = static_cast<DELETE_CLOUD_MAP_S *>(param);
            if (src == nullptr)
            {
                return -1;
            }

            tuya_message::Request req = {static_cast<int>(src->map_id)};
            tuya_message::Result res = {};
            const bool localDeleted = TuyaComm::Get()->Send("ty_delete_map_by_tuya_id", &req, &res, 1000);
            const bool cloudDeleted = tuya_iot_map_delete(src->map_id) == 0;
            LOGD(TAG, "delete map local={}, cloud={}, map_id={}", localDeleted, cloudDeleted, src->map_id);
            ty_rvc_map_delete_data_response(src, cloudDeleted ? MAP_DELETE_SUC : MAP_DELETE_FAILED);
            if (!cloudDeleted)
            {
                ret = OPRT_COM_ERROR;
                LogAdvanceResult("set", cmd, false, "delete cloud map failed");
            }
            else
            {
                LogAdvanceResult("set", cmd, true, localDeleted ? "local and cloud delete done" : "cloud delete done, local delete skipped or failed");
            }
        }
        break;
    case PART_DEFAULT_SET:
        {
            MAP_PART_RESET_S mapReset = {};
            tuya_message::Request req = {0};
            tuya_message::Result res = {};
            if (TuyaComm::Get()->Send("ty_re_split_region", &req, &res, 3000))
            {
                ty_rvc_map_part_default_data_response(&mapReset, res.code == 0 ? PART_RESET_SUC : PART_RESET_FAILED);
                LogAdvanceResult("set", cmd, res.code == 0, res.code == 0 ? nullptr : "restore default response failed");
            }
            else
            {
                ty_rvc_map_part_default_data_response(&mapReset, PART_RESET_FAILED);
                ret = OPRT_COM_ERROR;
                LogAdvanceResult("set", cmd, false, "send ty_re_split_region failed");
            }
        }
        break;
    default:
        LOGW(TAG, "Unsupported advance function set: {}", (int)cmd);
        ret = OPRT_NOT_SUPPORTED;
        LogAdvanceResult("set", cmd, false, "unsupported command");
        break;
    }

    return ret;
}

int TuyaAdvanceFuctionQuery(COMPLEX_CMD_E cmd, void* param)
{
    LOGD(TAG, "Advance function query: {}", (int)cmd);
    (void)param;

    int ret = OPRT_OK;
    tuya_message::Request req = {};

    switch (cmd)
    {
    case VIRTUAL_WALL_QUERY:
        {
            AppVirtualWall wall;
            if (TuyaComm::Get()->Send("ty_get_virtual_wall", &req, &wall, 1000) && wall.version >= 0)
            {
                std::vector<VIRTUAL_LINE_S> lines(wall.count);
                VIRTUAL_WALL_S payload = {};
                payload.num = wall.count;
                payload.map_id = 0;
                payload.line = lines.data();
                for (int i = 0; i < wall.count; ++i)
                {
                    lines[i].mode = static_cast<FORBIT_MODE_E>(wall.mode.empty() ? FORBIT_ALL : wall.mode[i]);
                    lines[i].points[0].x = MarsXToTuya(wall.wall[i].begin);
                    lines[i].points[0].y = MarsYToTuya(wall.wall[i].begin);
                    lines[i].points[1].x = MarsXToTuya(wall.wall[i].end);
                    lines[i].points[1].y = MarsYToTuya(wall.wall[i].end);
                }
                ty_rvc_virtual_wall_data_response(&payload, 0);
                LogAdvanceResult("query", cmd, true);
            }
            else
            {
                ret = OPRT_COM_ERROR;
                LogAdvanceResult("query", cmd, false, "get virtual wall failed");
            }
        }
        break;
    case RESTRICTED_AREA_QUERY:
        {
            AppRestrictedArea area;
            if (TuyaComm::Get()->Send("ty_get_restricted_area", &req, &area, 1000) && area.version >= 0)
            {
                std::vector<VIRTUAL_AREA_S> zones(area.count);
                std::vector<std::vector<POINT_COOR_S>> points(area.count);
                std::vector<std::vector<char>> names(area.count);
                RESTRICTED_AREA_S payload = {};
                payload.num = area.count;
                payload.map_id = 0;
                payload.restrict_zone = zones.data();

                for (int i = 0; i < area.count; ++i)
                {
                    zones[i].mode = static_cast<FORBIT_MODE_E>(area.mode[i]);
                    points[i].resize(area.polygon[i].count);
                    for (int j = 0; j < area.polygon[i].count; ++j)
                    {
                        points[i][j].x = MarsXToTuya(area.polygon[i].vetex[j]);
                        points[i][j].y = MarsYToTuya(area.polygon[i].vetex[j]);
                    }
                    zones[i].area.point_num = points[i].size();
                    zones[i].area.point = points[i].data();

                    std::string name = i < area.name.size() ? area.name[i] : std::string();
                    AssignChars(names[i], name);
                    names[i].push_back('\0');
                    zones[i].cur_name.len = name.size();
                    zones[i].cur_name.name = names[i].data();
                }

                ty_rvc_restricted_area_data_response(&payload, 0);
                LogAdvanceResult("query", cmd, true);
            }
            else
            {
                ret = OPRT_COM_ERROR;
                LogAdvanceResult("query", cmd, false, "get restricted area failed");
            }
        }
        break;
    case ROOM_CLEAN_QUERY:
        {
            AppRoomClean room;
            if (TuyaComm::Get()->Send("ty_get_clean_blocks", &req, &room, 1000) && room.count > 0)
            {
                std::vector<int> ids(room.count);
                std::vector<STR_ELEMENT_S> sweepModes(room.count);
                std::vector<std::vector<char>> modeStrings(room.count);
                std::vector<CLEAN_PARAM_S> params(room.count);
                ROOM_CLEAN_S payload = {};
                payload.num = room.count;
                payload.map_id = 0;
                payload.ids = ids.data();
                payload.sweep_mode = sweepModes.data();
                payload.param = params.data();

                for (int i = 0; i < room.count; ++i)
                {
                    ids[i] = room.id[i];
                    const char *modeName = "smart";
                    const int modeLen = CStringLen(modeName);
                    AssignChars(modeStrings[i], modeName, modeLen);
                    modeStrings[i].push_back('\0');
                    sweepModes[i].len = modeLen;
                    sweepModes[i].name = modeStrings[i].data();
                    params[i].valid = 1;
                    params[i].clean_cnt = room.cleanRepeat;
                    params[i].y_mop = false;
                    params[i].suction.len = 0;
                    params[i].suction.name = nullptr;
                    params[i].cistern.len = 0;
                    params[i].cistern.name = nullptr;
                }

                ty_rvc_room_clean_data_response(&payload, 0);
                LogAdvanceResult("query", cmd, true);
            }
            else
            {
                ret = OPRT_COM_ERROR;
                LogAdvanceResult("query", cmd, false, "get clean blocks failed");
            }
        }
        break;
    case SPOT_CLEAN_QUERY:
        {
            AppSpotClean clean;
            if (TuyaComm::Get()->Send("ty_get_spot", &req, &clean, 1000))
            {
                POINT_COOR_S point = {MarsXToTuya(clean.spot), MarsYToTuya(clean.spot)};
                std::vector<char> modeString;
                std::vector<char> suctionString;
                std::vector<char> cisternString;
                const char *modeName = ToModeName(clean.mode);
                const char *suctionName = ToSuctionName(clean.suction);
                const char *cisternName = ToCisternName(clean.cistern);
                const int modeLen = CStringLen(modeName);
                const int suctionLen = CStringLen(suctionName);
                const int cisternLen = CStringLen(cisternName);
                AssignChars(modeString, modeName, modeLen);
                modeString.push_back('\0');
                AssignChars(suctionString, suctionName, suctionLen);
                suctionString.push_back('\0');
                AssignChars(cisternString, cisternName, cisternLen);
                cisternString.push_back('\0');

                STR_ELEMENT_S mode = {modeLen, modeString.data()};
                CLEAN_PARAM_S paramInfo = {};
                paramInfo.valid = 1;
                paramInfo.clean_cnt = clean.cleanRepeat;
                paramInfo.y_mop = false;
                paramInfo.suction.len = suctionLen;
                paramInfo.suction.name = suctionString.data();
                paramInfo.cistern.len = cisternLen;
                paramInfo.cistern.name = cisternString.data();

                SPOT_CLEAN_S payload = {};
                payload.num = 1;
                payload.map_id = 0;
                payload.points = &point;
                payload.mode = &mode;
                payload.param = &paramInfo;
                ty_rvc_spot_clean_data_response(&payload, 0);
                LogAdvanceResult("query", cmd, true);
            }
            else
            {
                ret = OPRT_COM_ERROR;
                LogAdvanceResult("query", cmd, false, "get spot clean failed");
            }
        }
        break;
    case ZONE_CLEAN_QUERY:
        {
            AppZoneClean zone;
            if (TuyaComm::Get()->Send("ty_get_zone_clean", &req, &zone, 1000) && zone.count > 0)
            {
                std::vector<ZONE_AREA_PARAM_S> zones(zone.count);
                std::vector<std::vector<POINT_COOR_S>> points(zone.count);
                std::vector<std::vector<char>> zoneNames(zone.count);
                std::vector<std::vector<char>> modeNames(zone.count);
                std::vector<std::vector<char>> suctionNames(zone.count);
                std::vector<std::vector<char>> cisternNames(zone.count);
                ZONE_CLEAN_S payload = {};
                payload.num = zone.count;
                payload.map_id = 0;
                payload.clean_zone = zones.data();

                for (int i = 0; i < zone.count; ++i)
                {
                    const char *modeName = ToModeName(zone.mode.empty() ? 0 : zone.mode[std::min(i, static_cast<int>(zone.mode.size() - 1))]);
                    const char *suctionName = ToSuctionName(zone.suction.empty() ? DP9_NORMAL : zone.suction[std::min(i, static_cast<int>(zone.suction.size() - 1))]);
                    const char *cisternName = ToCisternName(zone.cistern.empty() ? DP10_MIDDLE : zone.cistern[std::min(i, static_cast<int>(zone.cistern.size() - 1))]);
                    const int cleanRepeat = zone.cleanRepeat.empty() ? 1 : zone.cleanRepeat[std::min(i, static_cast<int>(zone.cleanRepeat.size() - 1))];

                    points[i].resize(zone.zone[i].count);
                    for (int j = 0; j < zone.zone[i].count; ++j)
                    {
                        points[i][j].x = MarsXToTuya(zone.zone[i].vetex[j]);
                        points[i][j].y = MarsYToTuya(zone.zone[i].vetex[j]);
                    }

                    const int modeLen = CStringLen(modeName);
                    const int suctionLen = CStringLen(suctionName);
                    const int cisternLen = CStringLen(cisternName);
                    AssignChars(modeNames[i], modeName, modeLen);
                    modeNames[i].push_back('\0');
                    AssignChars(suctionNames[i], suctionName, suctionLen);
                    suctionNames[i].push_back('\0');
                    AssignChars(cisternNames[i], cisternName, cisternLen);
                    cisternNames[i].push_back('\0');
                    std::string name = i < zone.name.size() ? zone.name[i] : std::string();
                    AssignChars(zoneNames[i], name);
                    zoneNames[i].push_back('\0');

                    zones[i].mode.len = modeLen;
                    zones[i].mode.name = modeNames[i].data();
                    zones[i].param.valid = 1;
                    zones[i].param.clean_cnt = cleanRepeat;
                    zones[i].param.y_mop = false;
                    zones[i].param.suction.len = suctionLen;
                    zones[i].param.suction.name = suctionNames[i].data();
                    zones[i].param.cistern.len = cisternLen;
                    zones[i].param.cistern.name = cisternNames[i].data();
                    zones[i].area.point_num = points[i].size();
                    zones[i].area.point = points[i].data();
                    zones[i].cur_name.len = name.size();
                    zones[i].cur_name.name = zoneNames[i].data();
                }

                ty_rvc_zone_clean_data_response(&payload, 0);
                LogAdvanceResult("query", cmd, true);
            }
            else
            {
                ret = OPRT_COM_ERROR;
                LogAdvanceResult("query", cmd, false, "get zone clean failed");
            }
        }
        break;
    case ROOM_PROPERTY_QUERY:
        LOGW(TAG, "room property query is not supported without room property/name query channels");
        ret = OPRT_NOT_SUPPORTED;
        LogAdvanceResult("query", cmd, false, "missing room property/name query channels");
        break;
    case QUIET_HOUR_QUERY:
        {
            AppNotDisturbTime time;
            if (TuyaComm::Get()->Send("ty_get_dnd", &req, &time, 1000) && time.version >= 0)
            {
                QUIET_HOURS_S payload = {};
                payload.active = time.version == 0 ? 1 : time.onoff;
                payload.other_day = time.endWeekDay;
                payload.start_time.hour = time.starTime.hour;
                payload.start_time.min = time.starTime.minute;
                payload.end_time.hour = time.endTime.hour;
                payload.end_time.min = time.endTime.minute;
                ty_rvc_quiet_hours_data_response(&payload, 0);
                LogAdvanceResult("query", cmd, true);
            }
            else
            {
                ret = OPRT_COM_ERROR;
                LogAdvanceResult("query", cmd, false, "get dnd failed");
            }
        }
        break;
    case SCHEDULE_QUERY:
        {
            AppLocalAlert alert;
            if (TuyaComm::Get()->Send("ty_get_m_orders", &req, &alert, 1000) && alert.verison >= 0)
            {
                std::vector<SCHEDULE_PARAM_S> timeSets(alert.number);
                std::vector<std::vector<int>> ids(alert.number);
                std::vector<std::vector<STR_ELEMENT_S>> modes(alert.number);
                std::vector<std::vector<std::vector<char>>> modeStorage(alert.number);
                std::vector<std::vector<CLEAN_PARAM_S>> params(alert.number);
                std::vector<std::vector<std::vector<char>>> suctionStorage(alert.number);
                std::vector<std::vector<std::vector<char>>> cisternStorage(alert.number);
                SCHEDULE_S payload = {};
                payload.num = alert.number;
                payload.map_id = 0;
                payload.time_sets = timeSets.data();

                for (int i = 0; i < alert.number; ++i)
                {
                    const auto &task = alert.timedTaskInfo[i];
                    timeSets[i].active = task.valid;
                    timeSets[i].cycle = task.weekDayMask;
                    timeSets[i].start_time.hour = (task.time >> 8) & 0xFF;
                    timeSets[i].start_time.min = task.time & 0xFF;
                    timeSets[i].customed_switch = false;

                    if (task.roomN == 0)
                    {
                        ids[i].push_back(-1);
                    }
                    else
                    {
                        for (int j = 0; j < task.roomN; ++j)
                        {
                            ids[i].push_back(task.roomId[j]);
                        }
                    }

                    const int roomCount = ids[i].size();
                    modes[i].resize(roomCount);
                    modeStorage[i].resize(roomCount);
                    params[i].resize(roomCount);
                    suctionStorage[i].resize(roomCount);
                    cisternStorage[i].resize(roomCount);
                    timeSets[i].num = roomCount;
                    timeSets[i].ids = ids[i].data();
                    timeSets[i].mode = modes[i].data();
                    timeSets[i].param = params[i].data();

                    for (int j = 0; j < roomCount; ++j)
                    {
                        const char *modeName = ToModeName(task.cleanMode);
                        const char *suctionName = ToSuctionName(task.fan);
                        const char *cisternName = ToCisternName(task.water);
                        const int modeLen = CStringLen(modeName);
                        const int suctionLen = CStringLen(suctionName);
                        const int cisternLen = CStringLen(cisternName);
                        AssignChars(modeStorage[i][j], modeName, modeLen);
                        modeStorage[i][j].push_back('\0');
                        AssignChars(suctionStorage[i][j], suctionName, suctionLen);
                        suctionStorage[i][j].push_back('\0');
                        AssignChars(cisternStorage[i][j], cisternName, cisternLen);
                        cisternStorage[i][j].push_back('\0');

                        modes[i][j].len = modeLen;
                        modes[i][j].name = modeStorage[i][j].data();
                        params[i][j].valid = 1;
                        params[i][j].clean_cnt = task.cleanRepeat;
                        params[i][j].y_mop = false;
                        params[i][j].suction.len = suctionLen;
                        params[i][j].suction.name = suctionStorage[i][j].data();
                        params[i][j].cistern.len = cisternLen;
                        params[i][j].cistern.name = cisternStorage[i][j].data();
                    }
                }

                ty_rvc_schedule_data_response(&payload, 0);
                LogAdvanceResult("query", cmd, true);
            }
            else
            {
                ret = OPRT_COM_ERROR;
                LogAdvanceResult("query", cmd, false, "get m_orders failed");
            }
        }
        break;
    default:
        LOGW(TAG, "Unsupported advance function query: {}", (int)cmd);
        ret = OPRT_NOT_SUPPORTED;
        LogAdvanceResult("query", cmd, false, "unsupported command");
        break;
    }

    return ret;
}

void PlayVoice(int v, int param)
{
    Event voice;
    voice.event = v;
    voice.param = param;
    TuyaComm::Get()->Publish("Voice", &voice);
}

void DevicePairingSuccess(void)
{
    PlayVoice(V_SERVER_CONNECTED, PLAY_QUEUE);
    tuya_message::Request req;
    tuya_message::Result res = {};
    TuyaComm::Get()->Send("ty_wifi_connect", &req, &res);
    
}

int camera_led[2] = {0, 0};

void CameraLed(int on, int reason)
{
    if(on)
    {
        LOGD(TAG, "Camera led on: {}", reason);
        camera_led[reason] = 1;
    }
    if(!on)
    {
        LOGD(TAG, "Camera led off: {}", reason);
        camera_led[reason] = 0;
        int sum = 0;
        for(int i=0; i<2; i++)
        {
            sum += camera_led[i];
        }
        if(sum != 0)
        {
            LOGD(TAG, "Camera led still on: {}", sum);
            return;
        }
    }
    Event voice;
    voice.event = 'CLED';
    voice.param = on;
    TuyaComm::Get()->Publish("Kevt", &voice);
}

void BleOn(bool on)
{
    Event e;
    e.event = on;
    TuyaComm::Get()->Publish("BleOn", &e);
}