#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "tuya_iot_com_api.h"
#include "utilities/uni_log.h"

#include "tuya_robot.h"
#include "tuya_sweeper.h"
#include "tuya_app_dp_demo.h"

void TuyaHandleDPCmd(TY_OBJ_DP_S *dp);
int TuyaAdvanceFuctionSet(COMPLEX_CMD_E cmd, void* param);
int TuyaAdvanceFuctionQuery(COMPLEX_CMD_E cmd, void* param);
/**
 * @brief  接收到 sdk obj dp 处理回调，注册到 SDK 中
 * @param  [TY_SDK_RECV_OBJ_DP_S] *dp_rev
 * @return [*]
 */
VOID ty_cmd_handle_dp_cmd_objs(IN CONST TY_RECV_OBJ_DP_S* dp_rev)
{
    PR_DEBUG("[user cmd]recv cmd dp");
    for(int i=0; i<dp_rev->dps_cnt; i++)
    {
        TuyaHandleDPCmd((TY_OBJ_DP_S *)&dp_rev->dps[i]);
    }
}


/**
 * @brief  接收到 sdk raw dp 处理回调，注册到 SDK 中
 * @param  [TY_SDK_RECV_OBJ_DP_S] *dp_rev
 * @return [*]
 */
VOID ty_cmd_handle_dp_raw_objs(IN CONST TY_RECV_RAW_DP_S* dp_rev)
{
    PR_DEBUG("[user raw cmd]recv cmd dp");
    TuyaHandleRawDPCmd(dp_rev->dpid, (uint8_t *)dp_rev->data, dp_rev->len);
}


/**
 * @brief  设备上线上报相关信息
 * @param  [*]
 * @return [*]
 */
void ty_cmd_handle_sync_to_cloud(void)
{
    /***attention：
     * 当网络连接成功后，需要将机器内的数据进行上传同步
     * ***/
   //TuyaComm::Get()->ReportAll();
}


/**
 * @brief  接收到 sdk query dp 处理回调，注册到 SDK 中
 * @param  [TY_SDK_RECV_OBJ_DP_S] *dp_rev
 * @return [*]
 */
VOID ty_cmd_handle_dp_query_objs(IN CONST TY_DP_QUERY_S* dp_query)
{
    PR_DEBUG("[user cmd]recv query dp");
    RobotSweeper::Get()->ReportStatus();
}


/**
 * @brief 扫地机高级功能设置回调函数指针
 */
OPERATE_RET rvc_advance_function_set(IN COMPLEX_CMD_E cmd, IN void* param)
{
    
#if 0
    int i = 0, j = 0;
    int map_id = 0; //当前地图 ID
    switch (cmd) {
    case VIRTUAL_WALL_SET: {
        VIRTUAL_WALL_S* p_virtual_wall = (VIRTUAL_WALL_S*)param;
        PR_DEBUG("virtual wall num:%d", p_virtual_wall->num);
        for (i = 0; i < p_virtual_wall->num; i++) {
            PR_DEBUG("mode:%d", p_virtual_wall->line[i].mode);
            PR_DEBUG("line:%d", p_virtual_wall->line[i].points[0].x);
            PR_DEBUG("line:%d", p_virtual_wall->line[i].points[0].y);
            PR_DEBUG("line:%d", p_virtual_wall->line[i].points[1].x);
            PR_DEBUG("line:%d", p_virtual_wall->line[i].points[1].y);
        }
        dp_set.virtual_wall_sets.virtual_wall_count = p_virtual_wall->num; //结构体数据转化，保存到全局结构体中，开发者按需求保存到 flash 中
        int virtual_index = 0, i = 0;
        for (virtual_index = 0; virtual_index < dp_set.virtual_wall_sets.virtual_wall_count; virtual_index++) {
            for (i = 0; i < 2; i++) {
                dp_set.virtual_wall_sets.points[virtual_index].vertices[i].x = p_virtual_wall->line[virtual_index].points[i].x;
                dp_set.virtual_wall_sets.points[virtual_index].vertices[i].y = p_virtual_wall->line[virtual_index].points[i].y;
            }
        }
        dp_set.virtual_wall_sets.raw_data_len = sizeof(ROBOT_VIRTUAL_WALL_S) - sizeof(RAW_DATA_S);
        PR_DEBUG("virtual wall data len set:%d", dp_set.virtual_wall_sets.raw_data_len);
        memcpy(&dp_set.virtual_wall_sets.raw_data, &dp_set.virtual_wall_sets, dp_set.virtual_wall_sets.raw_data_len); // 将原始结构体数据保存起来
        //开发者根据 APP 下发的 virtual_wall_data 虚拟墙参数，做业务处理后再给面板回复。
        p_virtual_wall->map_id = map_id;
        ty_rvc_virtual_wall_data_response(p_virtual_wall, 0); // 这里直接上报数据到云端，测试使用

    } break;
    case RESTRICTED_AREA_SET: {
        RESTRICTED_AREA_S* p_restricted_area = (RESTRICTED_AREA_S*)param;
        PR_DEBUG("restricted area num:%d", p_restricted_area->num);
        for (i = 0; i < p_restricted_area->num; i++) {
            PR_DEBUG("mode:%d", p_restricted_area->restrict_zone[i].mode);
            PR_DEBUG("polyon points:%d", p_restricted_area->restrict_zone[i].area.point_num);
            for (j = 0; j < p_restricted_area->restrict_zone[i].area.point_num; j++) {
                PR_DEBUG("polyon points:%d", p_restricted_area->restrict_zone[i].area.point[j].x);
                PR_DEBUG("polyon points:%d", p_restricted_area->restrict_zone[i].area.point[j].y);
            }
            PR_DEBUG("name_len:%d", p_restricted_area->restrict_zone[i].cur_name.len);
            PR_DEBUG("name:%s", p_restricted_area->restrict_zone[i].cur_name.name);
        }
        //结构体数据转化，保存到全局结构体中，开发者按需求保存到 flash 中
        dp_set.restricted_zone_sets.zone_count = p_restricted_area->num;
        for (i = 0; i < p_restricted_area->num; i++) {
            dp_set.restricted_zone_sets.mode[i] = p_restricted_area->restrict_zone[i].mode;
            PR_DEBUG("polyon points:%d", p_restricted_area->restrict_zone[i].area.point_num);
            for (j = 0; j < p_restricted_area->restrict_zone[i].area.point_num; j++) {
                dp_set.restricted_zone_sets.points[i].vertices[j].x = p_restricted_area->restrict_zone[i].area.point[j].x;
                dp_set.restricted_zone_sets.points[i].vertices[j].y = p_restricted_area->restrict_zone[i].area.point[j].y;
            }
            dp_set.restricted_zone_sets.names[i].name_len = p_restricted_area->restrict_zone[i].cur_name.len;
            strncpy(dp_set.restricted_zone_sets.names[i].name, p_restricted_area->restrict_zone[i].cur_name.name, p_restricted_area->restrict_zone[i].cur_name.len);
        }
        dp_set.restricted_zone_sets.raw_data_len = sizeof(ROBOT_RESTRICTED_ZONE_S) - sizeof(RESTRICTED_ZONE_RAW_DATA_S);
        PR_DEBUG("restricted zone data len set:%d", dp_set.restricted_zone_sets.raw_data_len);
        memcpy(&dp_set.restricted_zone_sets.raw_data, &dp_set.restricted_zone_sets, dp_set.restricted_zone_sets.raw_data_len); // 将原始结构体数据保存起来
        for (int i = 0; i < 10; i++) {
            PR_DEBUG("restricted zone raw data:%x", dp_set.restricted_zone_sets.raw_data[i]);
        }
        //开发者根据 APP 下发的 restricted_zone 禁区参数，做业务处理后再给面板回复。
        p_restricted_area->map_id = map_id;
        ty_rvc_restricted_area_data_response(p_restricted_area, 0); // 这里直接上报数据到云端，测试使用

    } break;
    case ROOM_CLEAN_SET: {
        ROOM_CLEAN_S* p_room_clean = (ROOM_CLEAN_S*)param;
        PR_DEBUG("room_clean num:%d", p_room_clean->num);
        //结构体数据转化，保存到全局结构体中，开发者按需求保存到 flash 中
        dp_set.room_clean_sets.room_count = p_room_clean->num;
        for (i = 0; i < dp_set.room_clean_sets.room_count; i++) {
            dp_set.room_clean_sets.clean_times = p_room_clean->param[i].clean_cnt;
            dp_set.room_clean_sets.room_ids[i] = p_room_clean->ids[i];
            dp_set.room_clean_sets.points[i].room_ids = p_room_clean->ids[i];
            dp_set.room_clean_sets.points[i].fan_level = enum_index_get_from_string(p_room_clean->param[i].suction.name, SUCTION_ENUM_TYPE);
            dp_set.room_clean_sets.points[i].water_level = enum_index_get_from_string(p_room_clean->param[i].cistern.name, CISTERN_ENUM_TYPE);
            dp_set.room_clean_sets.points[i].Y_mop = p_room_clean->param[i].y_mop;
            dp_set.room_clean_sets.points[i].sweep_count = p_room_clean->param[i].clean_cnt;
            dp_set.room_clean_sets.points[i].sweep_mode = enum_index_get_from_string(p_room_clean->sweep_mode[i].name, MODE_ENUM_TYPE);
        }
        dp_set.room_clean_sets.raw_data_len = sizeof(ROBOT_WORK_ROOM_EXTRA_S) - sizeof(RAW_DATA_S);
        memcpy(&dp_set.room_clean_sets.raw_data, &dp_set.room_clean_sets, dp_set.room_clean_sets.raw_data_len); // 将原始结构体数据保存起来
        //开发者根据 APP 下发的 room_clean_sets 选区清扫参数，做业务处理后再给面板回复。
        p_room_clean->map_id = map_id;
        ty_rvc_room_clean_data_response(p_room_clean, 0); // 这里直接上报数据到云端，测试使用

    } break;
    case SPOT_CLEAN_SET: {
        SPOT_CLEAN_S* p_spot_clean = (SPOT_CLEAN_S*)param;
        PR_DEBUG("spot_clean num:%d", p_spot_clean->num);
        //结构体数据转化，保存到全局结构体中，开发者按需求保存到 flash 中
        dp_set.pose_sets.clean_times = p_spot_clean->param[0].clean_cnt;
        dp_set.pose_sets.points[0].vertices[0].x = p_spot_clean->points[0].x;
        dp_set.pose_sets.points[0].vertices[0].y = p_spot_clean->points[0].y;

        dp_set.pose_sets.raw_data_len = sizeof(ROBOT_WORK_POSE_EXTRA_S) - sizeof(RAW_DATA_S);
        memcpy(&dp_set.pose_sets.raw_data, &dp_set.pose_sets, dp_set.pose_sets.raw_data_len); // 将原始结构体数据保存起来
        //开发者根据 APP 下发的 pose_clean_sets 定点清扫参数，做业务处理后再给面板回复。
        p_spot_clean->map_id = map_id;
        ty_rvc_spot_clean_data_response(p_spot_clean, 0); // 这里直接上报数据到云端，测试使用

    } break;

    case ZONE_CLEAN_SET: {
        ZONE_CLEAN_S* p_zone_clean = (ZONE_CLEAN_S*)param;
        PR_DEBUG("p_zone_clean num:%d", p_zone_clean->num);
        //结构体数据转化，保存到全局结构体中，开发者按需求保存到 flash 中
        dp_set.area_clean_sets.area_count = p_zone_clean->num;
        dp_set.area_clean_sets.clean_times = p_zone_clean->clean_zone[0].param.clean_cnt;

        for (int j = 0; j < dp_set.area_clean_sets.area_count; j++) {
            dp_set.area_clean_sets.points[j].vertices[0].x = p_zone_clean->clean_zone[j].area.point[0].x;
            dp_set.area_clean_sets.points[j].vertices[0].y = p_zone_clean->clean_zone[j].area.point[0].y;
            dp_set.area_clean_sets.points[j].vertices[1].x = p_zone_clean->clean_zone[j].area.point[1].x;
            dp_set.area_clean_sets.points[j].vertices[1].y = p_zone_clean->clean_zone[j].area.point[1].y;
            dp_set.area_clean_sets.points[j].vertices[2].x = p_zone_clean->clean_zone[j].area.point[2].x;
            dp_set.area_clean_sets.points[j].vertices[2].y = p_zone_clean->clean_zone[j].area.point[2].y;
            dp_set.area_clean_sets.points[j].vertices[3].x = p_zone_clean->clean_zone[j].area.point[3].x;
            dp_set.area_clean_sets.points[j].vertices[3].y = p_zone_clean->clean_zone[j].area.point[3].y;

            dp_set.area_clean_sets.names[j].name_len = p_zone_clean->clean_zone[j].cur_name.len;

            strncpy(dp_set.area_clean_sets.names[j].name, p_zone_clean->clean_zone[j].cur_name.name, dp_set.area_clean_sets.names[j].name_len);
        }
        dp_set.area_clean_sets.raw_data_len = sizeof(ROBOT_WORK_AREA_EXTRA_S) - sizeof(RAW_DATA_S);
        memcpy(&dp_set.area_clean_sets.raw_data, &dp_set.area_clean_sets, dp_set.area_clean_sets.raw_data_len); // 将原始结构体数据保存起来
        //开发者根据 APP 下发的 area_clean_sets 划区清扫参数，做业务处理后再给面板回复。
        p_zone_clean->map_id = map_id;
        ty_rvc_zone_clean_data_response(p_zone_clean, 0); // 这里直接上报数据到云端，测试使用

    } break;
    case ROOM_PROPERTY_SET: {
        ROOM_PROPERTY_S* p_room_property = (ROOM_PROPERTY_S*)param;
        PR_DEBUG("p_room_property num:%d", p_room_property->num);
        //结构体数据转化，保存到全局结构体中，开发者按需求保存到 flash 中
        dp_set.attribute_senior_sets.room_count = p_room_property->num;
        PR_DEBUG("room cout:%d", dp_set.attribute_senior_sets.room_count);
        for (i = 0; i < dp_set.attribute_senior_sets.room_count; i++) {
            // 房间属性设置
            dp_set.attribute_senior_sets.attrs[i].room_ids = p_room_property->ids[i];
            dp_set.attribute_senior_sets.attrs[i].fan_level = enum_index_get_from_string(p_room_property->param[i].suction.name, SUCTION_ENUM_TYPE);
            dp_set.attribute_senior_sets.attrs[i].water_level = enum_index_get_from_string(p_room_property->param[i].cistern.name, CISTERN_ENUM_TYPE);
            dp_set.attribute_senior_sets.attrs[i].sweep_mode = enum_index_get_from_string(p_room_property->sweep_mode[i].name, MODE_ENUM_TYPE);
            dp_set.attribute_senior_sets.attrs[i].Y_mop = p_room_property->param[i].y_mop;
            dp_set.attribute_senior_sets.attrs[i].sweep_count = p_room_property->param[i].clean_cnt;

            //名字设置
            dp_set.attribute_senior_sets.names[i].room_ids = p_room_property->ids[i];
            dp_set.attribute_senior_sets.names[i].name_len = p_room_property->cus_name[i].len;
            strncpy(dp_set.attribute_senior_sets.names[i].name, p_room_property->cus_name[i].name, p_room_property->cus_name[i].len);
            PR_DEBUG("room_ids:%d, name[%s], name_len:[%d]", dp_set.attribute_senior_sets.names[i].room_ids, dp_set.attribute_senior_sets.names[i].name, dp_set.attribute_senior_sets.names[i].name_len);
            //清扫顺序设置
            dp_set.attribute_senior_sets.orders[i] = p_room_property->orders[i];
            PR_DEBUG("orders[%d] set :%d", i, dp_set.attribute_senior_sets.orders[i]);
        }
        dp_set.attribute_senior_sets.raw_data_len = sizeof(ROBOT_ROOM_ATTR_SENIOR_S) - sizeof(ROBOT_ROOM_ATTR_SENIOR_DATA_LEN_S);
        memcpy(&dp_set.attribute_senior_sets.raw_data, &dp_set.attribute_senior_sets, dp_set.attribute_senior_sets.raw_data_len); // 将原始结构体数据保存起来
        //开发者根据 AAP 下发 room_attr_sets 房间属性参数，做业务处理后再给面板回复。
        p_room_property->map_id = map_id;
        ty_rvc_room_property_data_response(p_room_property, 0); // 这里直接上报数据到云端，测试使用

    } break;
    case SCHEDULE_SET: {
        SCHEDULE_S* p_schedule_sets = (SCHEDULE_S*)param;
        PR_DEBUG("p_schedule_sets num:%d", p_schedule_sets->num);
        //结构体数据转化，保存到全局结构体中，开发者按需求保存到 flash 中
        dp_set.local_timer_sets.time_zone = 0x08;
        dp_set.local_timer_sets.number = p_schedule_sets->num;

        for (int j = 0; j < dp_set.local_timer_sets.number; j++) {
            dp_set.local_timer_sets.points[j].effectiveness = p_schedule_sets->time_sets[j].active;
            dp_set.local_timer_sets.points[j].week = p_schedule_sets->time_sets[j].cycle;
            dp_set.local_timer_sets.points[j].time[0] = p_schedule_sets->time_sets[j].start_time.hour;
            dp_set.local_timer_sets.points[j].time[1] = p_schedule_sets->time_sets[j].start_time.min;

            dp_set.local_timer_sets.points[j].rooms = p_schedule_sets->time_sets[j].num;

            if (dp_set.local_timer_sets.points[j].rooms) {
                if ((dp_set.local_timer_sets.points[j].rooms == 1) && (p_schedule_sets->time_sets[j].ids[i] == -1)) {
                    dp_set.local_timer_sets.points[j].rooms = 0;
                } else {
                    for (i = 0; i < dp_set.local_timer_sets.points[j].rooms; i++) {
                        dp_set.local_timer_sets.points[j].id[i] = p_schedule_sets->time_sets[j].ids[i];
                        PR_DEBUG("local_time_sets.points[%d].id[%d]: %d", j, i, dp_set.local_timer_sets.points[j].id[i]);
                    }
                }
            }
            dp_set.local_timer_sets.points[j].customed_switch = p_schedule_sets->time_sets->customed_switch; // 自定义参数的使能开关
            // 当前业务对所有房间统一设置清扫参数，故这里取房间 id 为 0 时的参数，后期如有需要再进行业务扩展
            dp_set.local_timer_sets.points[j].clean_mode = enum_index_get_from_string(p_schedule_sets->time_sets[j].mode[0].name, MODE_ENUM_TYPE);
            dp_set.local_timer_sets.points[j].fan_level = enum_index_get_from_string(p_schedule_sets->time_sets[j].param[0].suction.name, SUCTION_ENUM_TYPE);
            dp_set.local_timer_sets.points[j].water_level = enum_index_get_from_string(p_schedule_sets->time_sets[j].param[0].cistern.name, CISTERN_ENUM_TYPE);
            dp_set.local_timer_sets.points[j].sweep_count = p_schedule_sets->time_sets[j].param[0].clean_cnt;
        }
        dp_set.local_timer_sets.raw_data.len = sizeof(ROBOT_LOCAL_TIMER_SET_S) - sizeof(LOCAL_TIME_RAW_DATA_S);

        PR_DEBUG("local time raw data len:%d", dp_set.local_timer_sets.raw_data.len);
        memcpy(dp_set.local_timer_sets.raw_data.buf, &dp_set.local_timer_sets, dp_set.local_timer_sets.raw_data.len); // 将原始结构体数据保存起来
        //开发者根据 APP 下发的 local_time_sets 预约清扫参数，做业务处理后再给面板回复。
        p_schedule_sets->map_id = map_id;
        ty_rvc_schedule_data_response(p_schedule_sets, 0); // 这里直接上报数据到云端，测试使用

    } break;
    case QUIET_HOUR_SET: {
        QUIET_HOURS_S* p_quiet_hours = (QUIET_HOURS_S*)param;
        //结构体数据转化，保存到全局结构体中，开发者按需求保存到 flash 中
        dp_set.disturb_timer_sets.version = 0x01;
        dp_set.disturb_timer_sets.disturb_switch = p_quiet_hours->active;
        dp_set.disturb_timer_sets.time_zone = 0x08; //仅测试
        dp_set.disturb_timer_sets.start_time[0] = p_quiet_hours->start_time.hour;
        dp_set.disturb_timer_sets.start_time[1] = p_quiet_hours->start_time.min;
        dp_set.disturb_timer_sets.end_time[0] = p_quiet_hours->end_time.hour;
        dp_set.disturb_timer_sets.end_time[1] = p_quiet_hours->end_time.min;
        dp_set.disturb_timer_sets.end_day = p_quiet_hours->other_day;
        dp_set.disturb_timer_sets.raw_data.len = sizeof(ROBOT_DISTURB_TIMER_SET_S) - sizeof(RAW_DATA_S);
        memcpy(dp_set.disturb_timer_sets.raw_data.buf, &dp_set.disturb_timer_sets, dp_set.disturb_timer_sets.raw_data.len); // 将原始结构体数据保存起来
        //开发者根据 APP 下发的 undisturb_set 勿扰模式结构体的参数，做业务处理后再给面板回复。
        ty_rvc_quiet_hours_data_response(p_quiet_hours, 0); // 这里直接上报数据到云端，测试使用
    } break;
    case PART_DIVI_SET: {
        PART_DIVI_S* divide_set = (PART_DIVI_S*)param;
        //结构体数据转化，保存到全局结构体中，开发者按需求保存到 flash 中
        dp_set.division_sets.room_ids = divide_set->ids[0];
        for (int i = 0; i < 2; i++) {
            dp_set.division_sets.points[0].vertices[i].x = divide_set->divi_line[0].points[i].x;
            dp_set.division_sets.points[0].vertices[i].y = divide_set->divi_line[0].points[i].y;
        }
        //开发者根据 APP 下发的 division_sets 手动分区结构体的参数，做业务处理后再给面板回复。
        ty_rvc_part_division_data_response(divide_set, 0); // 这里直接上报数据到云端，测试使用
         //房间分割或者合并，改变了房间属性，需要上报当前房间名称
#if defined(MCS_CMD_SUPPORT) && (MCS_CMD_SUPPORT == 1)
        ty_mcs_current_room_info_upload();
#endif
    } break;
    case PART_MERGE_SET: {
        PART_MERGE_S* merge_set = (PART_MERGE_S*)param;
        PR_DEBUG("merge_set num:%d", merge_set->num);
        //结构体数据转化，保存到全局结构体中，开发者按需求保存到 flash 中
        dp_set.merge_sets.len = merge_set->num;
        for (i = 0; i < dp_set.merge_sets.len; i++) {
            dp_set.merge_sets.room_ids[i] = merge_set->ids[i];
        }
        //开发者根据 APP 下发的 pmerge_set 分区合并结构体的参数，做业务处理后再给面板回复。
        ty_rvc_part_merge_data_response(merge_set, 0); // 这里直接上报数据到云端，测试使用
         //房间分割或者合并，改变了房间属性，需要上报当前房间名称
#if defined(MCS_CMD_SUPPORT) && (MCS_CMD_SUPPORT == 1)
        ty_mcs_current_room_info_upload();
#endif
    } break;
    case RESET_CURR_MAP_SET: {
        PR_DEBUG("reset curr map set");
        ty_rvc_map_clear_data_response(NULL, 0); // 直接上报数据到云端，测试使用
#if defined(MCS_CMD_SUPPORT) && (MCS_CMD_SUPPORT == 1)
        //注意地图重置成功后，也需要上报一次空的当前房间名称。
        MCS_ROOM_INFO_S room_info = { 0 };
        ty_rvc_mcs_room_info_response(&room_info, 0);
#endif
    } break;
    case SAVE_CURR_MAP_SET: {
        // int* map_save_oper = (int*)param;  //地图保存功能需要与面板对齐

        ty_rvc_map_save_data_response(NULL, 0); // 直接上报数据到云端，测试使用
    } break;
    case DELETE_CLOUD_MAP_SET: {
        DELETE_CLOUD_MAP_S* map_use_set = (DELETE_CLOUD_MAP_S*)param;
        int map_id = map_use_set->map_id;
        PR_DEBUG("delete cloud map: %d", map_id);
        ty_rvc_map_delete_data_response(map_use_set, 0); // 直接上报数据到云端，测试使用
    } break;
    case PART_DEFAULT_SET: {
        ty_rvc_map_part_default_data_response(NULL, 0); // 直接上报数据到云端，测试使用
    } break;
    default:
        break;
    }
    return 0;
#else
    return TuyaAdvanceFuctionSet(cmd, param);
#endif
}

/**
 * @brief 扫地机高级功能查询回调函数指针
 */
OPERATE_RET rvc_advance_function_query(IN COMPLEX_CMD_E cmd, IN void* param)
{
#if 0
    OPERATE_RET ret = OPRT_OK;
    int map_id = 0; //当前地图 ID
    switch (cmd) {
    case VIRTUAL_WALL_QUERY: {
        VIRTUAL_WALL_S test_virtual_wall;
        VIRTUAL_LINE_S virtual_line[COORDINATE_MAX_LEN] = { 0 };
        //将业务上的虚拟墙数据转化为 SDK 需要的格式
        test_virtual_wall.num = dp_set.virtual_wall_sets.virtual_wall_count;
        for (int i = 0; i < test_virtual_wall.num; i++) {
            virtual_line[i].mode = FORBIT_ALL;
            virtual_line[i].points[0].x = (int)(dp_set.virtual_wall_sets.points[i].vertices[0].x * 100 / 5);
            virtual_line[i].points[0].y = (int)(dp_set.virtual_wall_sets.points[i].vertices[0].y * 100 / 5);
            virtual_line[i].points[1].x = (int)(dp_set.virtual_wall_sets.points[i].vertices[1].x * 100 / 5);
            virtual_line[i].points[1].y = (int)(dp_set.virtual_wall_sets.points[i].vertices[1].y * 100 / 5);
        }
        test_virtual_wall.line = virtual_line;
        test_virtual_wall.map_id = map_id;
        ty_rvc_virtual_wall_data_response(&test_virtual_wall, 0); //调用 SDK 接口，上报云端

    } break;
    case RESTRICTED_AREA_QUERY: {
        RESTRICTED_AREA_S test_restrict_area;
        VIRTUAL_AREA_S restrict_area[COORDINATE_MAX_LEN];
        POINT_COOR_S point_array_test[COORDINATE_MAX_LEN][4] = { 0 };
        char name_array[COORDINATE_MAX_LEN][NAME_MAX_LEN] = { 0 };
        //将业务上的禁区数据转化为 SDK 需要的格式
        test_restrict_area.num = dp_set.restricted_zone_sets.zone_count;

        for (int j = 0; j < dp_set.restricted_zone_sets.zone_count; j++) {
            point_array_test[j][0].x = (int)(dp_set.restricted_zone_sets.points[j].vertices[0].x * 100 / 5);
            point_array_test[j][0].y = (int)(dp_set.restricted_zone_sets.points[j].vertices[0].y * 100 / 5);
            point_array_test[j][1].x = (int)(dp_set.restricted_zone_sets.points[j].vertices[1].x * 100 / 5);
            point_array_test[j][1].y = (int)(dp_set.restricted_zone_sets.points[j].vertices[1].y * 100 / 5);
            point_array_test[j][2].x = (int)(dp_set.restricted_zone_sets.points[j].vertices[2].x * 100 / 5);
            point_array_test[j][2].y = (int)(dp_set.restricted_zone_sets.points[j].vertices[2].y * 100 / 5);
            point_array_test[j][3].x = (int)(dp_set.restricted_zone_sets.points[j].vertices[3].x * 100 / 5);
            point_array_test[j][3].y = (int)(dp_set.restricted_zone_sets.points[j].vertices[3].y * 100 / 5);

            strncpy(name_array[j], dp_set.restricted_zone_sets.names[j].name, dp_set.restricted_zone_sets.names[j].name_len);

            restrict_area[j].mode = dp_set.restricted_zone_sets.mode[j];
            restrict_area[j].area.point_num = 4;
            restrict_area[j].area.point = point_array_test[j];
            restrict_area[j].cur_name.len = dp_set.restricted_zone_sets.names[j].name_len;
            restrict_area[j].cur_name.name = name_array[j];     //注意房间名称不能使用 NULL 数据传入
        }

        test_restrict_area.restrict_zone = restrict_area;
        PR_DEBUG("p_restricted_area:%x,%x,%x", &test_restrict_area, test_restrict_area.restrict_zone);
        test_restrict_area.map_id = map_id;
        ty_rvc_restricted_area_data_response(&test_restrict_area, 0); //调用 SDK 接口，上报云端

    } break;
    case ROOM_CLEAN_QUERY: {
        dp_handle_room_clean_info_response(); //选区清扫数据查询
    } break;

    case SPOT_CLEAN_QUERY: {
        dp_handle_pose_clean_info_response(); //定点清扫数据查询

    } break;
    case ZONE_CLEAN_QUERY: {
        dp_handle_area_clean_info_response(); //划区清扫数据查询

    } break;
    case ROOM_PROPERTY_QUERY: {
        ROOM_PROPERTY_S room_property_test = { 0 };
        //开发者根据自己的逻辑填写
        CLEAN_PARAM_S param[COORDINATE_MAX_LEN] = { 0 };
        int room_ids[COORDINATE_MAX_LEN] = { 0 };
        int room_floor_type[COORDINATE_MAX_LEN] = { 0 };
        int room_orders[COORDINATE_MAX_LEN] = { 0 };
        char room_name[COORDINATE_MAX_LEN][19] = { 0 };
        STR_ELEMENT_S  name_attr[COORDINATE_MAX_LEN] = { 0 };
        STR_ELEMENT_S sweep_mode[COORDINATE_MAX_LEN] = { 0 }; // 清扫模式
    
        room_property_test.num = 4;   //测试用，测试的地图上是 2 个房间，开发者根据实际数量填写
        room_property_test.ids = room_ids;
        room_property_test.param = param;
        room_property_test.orders = room_orders;
        room_property_test.cus_name = name_attr;
        room_property_test.sweep_mode = sweep_mode;
        room_property_test.floor_type = room_floor_type;
    
        for (int i = 0; i < room_property_test.num; i++) {
            char* temp_str = enum_string_get_from_index(0, MODE_ENUM_TYPE);
            sweep_mode[i].len = strlen(temp_str);
            sweep_mode[i].name = temp_str;
    
            param[i].valid = 1;
            param[i].clean_cnt = 1;   //清扫次数，开发者根据实际数量填写
            param[i].y_mop = 0;   //是否支持 Y 字托，开发者根据实际数量填写
    
            temp_str = enum_string_get_from_index(E_MOTOR_PUMPER_MID, CISTERN_ENUM_TYPE);   //水量等级，根据实际填写
            param[i].cistern.len = strlen(temp_str);
            param[i].cistern.name = temp_str; // 指针赋值
    
            temp_str = enum_string_get_from_index(E_MOTOR_VACCUM_12PA, SUCTION_ENUM_TYPE);  //吸力等级，根据实际填写
            param[i].suction.len = strlen(temp_str);
            param[i].suction.name = temp_str;
    
            room_ids[i] = i;    //根据实际填写
            PR_DEBUG("room_id:%d", room_ids[i]);
    
            room_floor_type[i] = -1;
            // 清扫顺序
            room_orders[i] = i; //根据实际填写
    
            // 房间名字
            memcpy(room_name[i], "test_name", sizeof("test_name") - 1);   //根据实际填写，没有设置就为空
            name_attr[i].len = sizeof("test_name") - 1;
            name_attr[i].name = room_name[i];   //注意房间名称不能使用 NULL 数据传入
        }
        room_property_test.map_id = map_id;
        ty_rvc_room_property_data_response(&room_property_test, 0); //调用 SDK 接口，上报云端

    } break;
    case QUIET_HOUR_QUERY: {
        //去 flash 获取勿扰数据，并传递给下面数据
        QUIET_HOURS_S p_quiet_hours = { 0 };
        p_quiet_hours.active = dp_set.disturb_timer_sets.disturb_switch;
        p_quiet_hours.start_time.hour = dp_set.disturb_timer_sets.start_time[0];
        p_quiet_hours.start_time.min = dp_set.disturb_timer_sets.start_time[1];
        p_quiet_hours.end_time.hour = dp_set.disturb_timer_sets.end_time[0];
        p_quiet_hours.end_time.min = dp_set.disturb_timer_sets.end_time[1];
        p_quiet_hours.other_day = dp_set.disturb_timer_sets.end_day;
        ty_rvc_quiet_hours_data_response(&p_quiet_hours, 0); //调用 SDK 接口，上报云端

    } break;
    case SCHEDULE_QUERY: {
        //去 flash 获取时间数据，并传递给下面数据
        SCHEDULE_S schedule_set = { 0 };
        int ids_array[LOCAL_TIMER_MAX_NUM][ROOM_CLEAN_MAX_NUM] = { 0 };

        STR_ELEMENT_S mode[LOCAL_TIMER_MAX_NUM][ROOM_CLEAN_MAX_NUM] = { 0 }; // 清扫的模式
        CLEAN_PARAM_S param[LOCAL_TIMER_MAX_NUM][ROOM_CLEAN_MAX_NUM] = { 0 };
        SCHEDULE_PARAM_S clean_data[LOCAL_TIMER_MAX_NUM] = { 0 };

        schedule_set.num = dp_set.local_timer_sets.number;
        for (int j = 0; j < dp_set.local_timer_sets.number; j++) {
            clean_data[j].active = dp_set.local_timer_sets.points[j].effectiveness;
            clean_data[j].cycle = dp_set.local_timer_sets.points[j].week;
            clean_data[j].start_time.hour = dp_set.local_timer_sets.points[j].time[0];
            clean_data[j].start_time.min = dp_set.local_timer_sets.points[j].time[1];
            
            if (dp_set.local_timer_sets.points[j].rooms == 0) {
                dp_set.local_timer_sets.points[j].rooms = 1;
                dp_set.local_timer_sets.points[j].id[0] = 0xff;
            }
            clean_data[j].num = dp_set.local_timer_sets.points[j].rooms;
            clean_data[j].customed_switch = dp_set.local_timer_sets.points[j].customed_switch; // 自定义参数的使能开关
            if (dp_set.local_timer_sets.points[j].rooms) {
                for (int i = 0; i < dp_set.local_timer_sets.points[j].rooms; i++) {
                    clean_data[j].ids = ids_array[j];
                    clean_data[j].mode = mode[j];
                    clean_data[j].param = param[j];
                    clean_data[j].ids[i] = dp_set.local_timer_sets.points[j].id[i];
                    if (clean_data[j].ids[i] == 0xff) {
                        clean_data[j].ids[i] = -1; //数组强转有问题，需要调整 to do
                    }
                    PR_DEBUG("local_time_data.points[%d].id[%d] %d", j, i, dp_set.local_timer_sets.points[j].id[i]);
                    // 当前业务对所有房间统一设置清扫参数，故这里取房间 id 为 0 时的参数，后期如有需要再进行业务扩展

                    char* temp_enum_str = enum_string_get_from_index(dp_set.local_timer_sets.points[j].clean_mode, MODE_ENUM_TYPE);
                    clean_data[j].mode[i].len = strlen(temp_enum_str);
                    clean_data[j].mode[i].name = temp_enum_str;
                    temp_enum_str = enum_string_get_from_index(dp_set.local_timer_sets.points[j].fan_level, SUCTION_ENUM_TYPE);
                    clean_data[j].param[i].suction.len = strlen(temp_enum_str);
                    clean_data[j].param[i].suction.name = temp_enum_str;
                    temp_enum_str = enum_string_get_from_index(dp_set.local_timer_sets.points[j].water_level, CISTERN_ENUM_TYPE);
                    clean_data[j].param[i].cistern.len = strlen(temp_enum_str);
                    clean_data[j].param[i].cistern.name = temp_enum_str;

                    clean_data[j].param[i].clean_cnt = dp_set.local_timer_sets.points[j].sweep_count;
                    PR_DEBUG("clean count:%d", clean_data[j].param[i].clean_cnt);

                }
            }
        }
        schedule_set.time_sets = clean_data;
        schedule_set.map_id = map_id;
        ty_rvc_schedule_data_response(&schedule_set, 0); //调用 SDK 接口，上报云端
    } break;
    case DEV_INFO_QUERY: {
        PR_DEBUG("dev info query >>>>>");
        dp_handle_device_info_respone(); //获取设备信息
    } break;

    case VOICE_LANGUAGE_QUERY: {
        PR_DEBUG("voice language info query >>>>>");
        USE_VOICE_LANGUAGE_RESPONSE_S p_voice_language_set = { 0 }; //获取当前语音状态
        p_voice_language_set.id = dp_set.voice_id;
        p_voice_language_set.download_status = RVC_DOWNLOAD_ST_USING;
        p_voice_language_set.percent = 100;
        ty_rvc_voice_language_data_response(&p_voice_language_set, 0); //调用 SDK 接口，上报云端
    } break;
    case MAP_EMPTY_QUERY: {
        PR_DEBUG("map empty info query >>>>>");
        /*开发者自行判断本地的首页地图是否存在，如果不存在回复 TRUE，存储就回复 FALSE*/
        if (ret == OPRT_OK) { 
            PR_DEBUG("history map upload can not read the map");
            ty_rvc_map_empty_response(TRUE, 0);
        } else {
            PR_ERR("history map upload can read the map:%d", ret);
            ty_rvc_map_empty_response(FALSE, 0);
        }
        
    } break;
#if defined(MCS_CMD_SUPPORT) && (MCS_CMD_SUPPORT == 1)
    case MCS_ROOM_INFO_QUERY: {      //查询设备当前所在位置
        ty_mcs_current_room_info_upload();
    } break;
#endif
    default: {
        PR_ERR("cmd data query not support now:%d", cmd);
        ret = OPRT_NOT_SUPPORTED;
    } break;
    }
    return ret;
#else
    return TuyaAdvanceFuctionQuery(cmd, param);
#endif
}
