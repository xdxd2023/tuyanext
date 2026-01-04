/*********************************************************************************
 *Copyright(C),2015-2020, TUYA www.tuya.comm
 *FileName:    tuya_app_dp_demo
 *brief：1.This article introduces demos for receiving and sending various types
           of DP (Data Points) for the vacuum cleaner.
         2.Developers can use the following demos as a basis for directly
           developing DP points, adding DP that exist in the PID, and removing DP
           that are not present in the PID.
 *Note: 1.DP should be reported as needed.
        2.Do not report DPs that do not exist under the PID, do not report
          nonexistent DP values, and do not report DP values that exceed the
          defined range.
 **********************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "tuya_iot_com_api.h"
#include "tuya_app_dp_demo.h"
#include "utilities/uni_log.h"
#if defined(TY_ROBOT_MEDIA_ENABLE) && (TY_ROBOT_MEDIA_ENABLE == 1)
#include "tuya_app_sd_card_demo.h"
#endif

static ROBOT_DP_S dp_set = { 0 }; // DP点资源全局变量

/**
 * @brief  设备端接口obj-dp类型
 */
typedef VOID (*TUYA_DP_HANDLER)(IN TY_OBJ_DP_S* p_obj_dp);
typedef struct
{
    BYTE_T dp_id; // dp点的id
    TUYA_DP_HANDLER handler; // 函数句柄
} TUYA_DP_INFO_S;

/** 
 * @brief  设备端接口raw-dp类型
 */
typedef void (*TUYA_RAW_DP_HANDLER)(unsigned char* p_obj_dp, unsigned int len);
typedef struct
{
    BYTE_T dp_id; // dp点的id
    TUYA_RAW_DP_HANDLER handler; // 函数句柄
} TUYA_RAW_DP_INFO_S;

/***************以下是dp的接收与发送的函数声明*******************/
#ifdef TUYA_DP_SWITCH_GO
static void __dp_handle_switch_go(TY_OBJ_DP_S* p_obj_dp);
static void dp_handle_switch_go_response(bool switch_go);
#endif

#ifdef TUYA_DP_PAUSE
static void __dp_handle_pause(TY_OBJ_DP_S* p_obj_dp);
static void dp_handle_pause_response(bool pause);
#endif

#ifdef TUYA_DP_SWITCH_CHARGE
static void __dp_handle_charge(TY_OBJ_DP_S* p_obj_dp);
static void dp_handle_charge_response(bool switch_charge);
#endif

#ifdef TUYA_DP_WORK_MODE
static void __dp_handle_work_mode_set(TY_OBJ_DP_S* p_obj_dp);
static void dp_handle_work_mode_response(ROBOT_WORK_MODE_E mode);
#endif

#ifdef TUYA_DP_STATUS
static void dp_handle_status_response(ROBOT_STATUS_E status);
static void dp_handle_status_restart_response(void);
#endif

#ifdef TUYA_DP_CLEAN_MODE
static void __dp_handle_clean_mode_set(TY_OBJ_DP_S* p_obj_dp);
#endif

#ifdef TUYA_DP_SUCTION
static void __dp_handle_suction_set(TY_OBJ_DP_S* p_obj_dp);
#endif

#ifdef TUYA_DP_CISTERN
static void __dp_handle_cistern_set(TY_OBJ_DP_S* p_obj_dp);
#endif

#ifdef TUYA_DP_DIRECTION_CONTROL
static void __dp_handle_direction_control(TY_OBJ_DP_S* p_obj_dp);
#endif

#ifdef TUYA_DP_MAP_RESET
static void __dp_handle_map_reset(TY_OBJ_DP_S* p_obj_dp);
#endif

#ifdef TUYA_DP_EDGE_BRUSH_LIEF
static void __dp_handle_edge_brush_used_time_reset(TY_OBJ_DP_S* p_obj_dp);
#endif

#ifdef TUYA_DP_ROLL_BRUSH_LIEF
static void __dp_handle_roll_brush_used_time_reset(TY_OBJ_DP_S* p_obj_dp);
#endif

#ifdef TUYA_DP_FILTER_LIEF
static void __dp_handle_filter_used_time_reset(TY_OBJ_DP_S* p_obj_dp);
#endif

#ifdef TUYA_DP_RAG_LIEF
static void __dp_handle_rag_used_time_reset(TY_OBJ_DP_S* p_obj_dp);
#endif

#ifdef TUYA_DP_FAULT
// static void __dp_handle_fault(TY_OBJ_DP_S* p_obj_dp);
#endif

#ifdef TUYA_DP_HOME_PHOTOGRAPHY_ENABLE
static void __dp_handle_home_photography_enable(TY_OBJ_DP_S* p_obj_dp);
#endif

#ifdef TUYA_DP_MEDIA_SWITCH_GO
 static void __dp_handle_media_switch_go(TY_OBJ_DP_S* p_obj_dp);
#endif

#ifdef TUYA_DP_LOG_SWITCH_GO
 static void __dp_handle_log_switch_go(TY_OBJ_DP_S* p_obj_dp);
#endif

#ifdef TUYA_DP_SD_STATUS_ONLY_GET
static void __dp_handle_sd_status_get(TY_OBJ_DP_S* p_obj_dp);
#endif

#ifdef TUYA_DP_SD_STORAGE_ONLY_GET
static void __dp_handle_sd_storage_get(TY_OBJ_DP_S* p_obj_dp);
#endif

#ifdef TUYA_DP_SD_RECORD_ENABLE
static void __dp_handle_sd_record_enable(TY_OBJ_DP_S* p_obj_dp);
#endif

#ifdef TUYA_DP_SD_RECORD_MODE
static void __dp_handle_sd_record_mode(TY_OBJ_DP_S* p_obj_dp);
#endif

#ifdef TUYA_DP_SD_UMOUNT
static void __dp_handle_sd_umount(TY_OBJ_DP_S* p_obj_dp);
#endif

#ifdef TUYA_DP_SD_FORMAT
static void __dp_handle_sd_format(TY_OBJ_DP_S* p_obj_dp);
#endif

#ifdef TUYA_DP_SD_FORMAT_STATUS_ONLY_GET
static void __dp_handle_sd_format_status_get(TY_OBJ_DP_S* p_obj_dp);
#endif

/**
 * @brief  处理dp类型数据函数的集合，用于数据解析；dp点与数据处理函数一一对应
 */
STATIC TUYA_DP_INFO_S s_dp_table[] = {

#ifdef TUYA_DP_SWITCH_GO
    { TUYA_DP_SWITCH_GO, __dp_handle_switch_go },
#endif

#ifdef TUYA_DP_PAUSE
    { TUYA_DP_PAUSE, __dp_handle_pause },
#endif

#ifdef TUYA_DP_SWITCH_CHARGE
    { TUYA_DP_SWITCH_CHARGE, __dp_handle_charge },
#endif

#ifdef TUYA_DP_WORK_MODE
    { TUYA_DP_WORK_MODE, __dp_handle_work_mode_set },
#endif

#ifdef TUYA_DP_CLEAN_MODE
    { TUYA_DP_CLEAN_MODE, __dp_handle_clean_mode_set },
#endif

#ifdef TUYA_DP_SUCTION
    { TUYA_DP_SUCTION, __dp_handle_suction_set },
#endif

#ifdef TUYA_DP_CISTERN
    { TUYA_DP_CISTERN, __dp_handle_cistern_set },
#endif

#ifdef TUYA_DP_DIRECTION_CONTROL
    { TUYA_DP_DIRECTION_CONTROL, __dp_handle_direction_control },
#endif

#ifdef TUYA_DP_MAP_RESET
    { TUYA_DP_MAP_RESET, __dp_handle_map_reset },
#endif

#ifdef TUYA_DP_EDGE_BRUSH_LIEF
    { TUYA_DP_EDGE_BRUSH_LIEF, __dp_handle_edge_brush_used_time_reset },
#endif

#ifdef TUYA_DP_ROLL_BRUSH_LIEF
    { TUYA_DP_ROLL_BRUSH_LIEF, __dp_handle_roll_brush_used_time_reset },
#endif

#ifdef TUYA_DP_FILTER_LIEF
    { TUYA_DP_FILTER_LIEF, __dp_handle_filter_used_time_reset },
#endif

#ifdef TUYA_DP_RAG_LIEF
    { TUYA_DP_RAG_LIEF, __dp_handle_rag_used_time_reset },
#endif

#ifdef TUYA_DP_HOME_PHOTOGRAPHY_ENABLE
    { TUYA_DP_HOME_PHOTOGRAPHY_ENABLE, __dp_handle_home_photography_enable },
#endif

#ifdef TUYA_DP_MEDIA_SWITCH_GO
    { TUYA_DP_MEDIA_SWITCH_GO, __dp_handle_media_switch_go },
#endif

#ifdef TUYA_DP_LOG_SWITCH_GO
    { TUYA_DP_LOG_SWITCH_GO, __dp_handle_log_switch_go },
#endif

#ifdef TUYA_DP_SD_STATUS_ONLY_GET
    { TUYA_DP_SD_STATUS_ONLY_GET, __dp_handle_sd_status_get },
#endif

#ifdef TUYA_DP_SD_STORAGE_ONLY_GET
    { TUYA_DP_SD_STORAGE_ONLY_GET, __dp_handle_sd_storage_get },
#endif

#ifdef TUYA_DP_SD_RECORD_ENABLE
    { TUYA_DP_SD_RECORD_ENABLE, __dp_handle_sd_record_enable },
#endif

#ifdef TUYA_DP_SD_RECORD_MODE
    { TUYA_DP_SD_RECORD_MODE, __dp_handle_sd_record_mode },
#endif

#ifdef TUYA_DP_SD_UMOUNT
    { TUYA_DP_SD_UMOUNT, __dp_handle_sd_umount },
#endif

#ifdef TUYA_DP_SD_FORMAT
    { TUYA_DP_SD_FORMAT, __dp_handle_sd_format },
#endif

#ifdef TUYA_DP_SD_FORMAT_STATUS_ONLY_GET
    { TUYA_DP_SD_FORMAT_STATUS_ONLY_GET, __dp_handle_sd_format_status_get },
#endif

};

/* raw dp 点处理 */
static TUYA_RAW_DP_INFO_S s_raw_dp_table[] = {

};

/**
 * @brief  累加和的方式计算check sum
 * @param  [char*] buf
 * @param  [char] len
 * @return [char] 结果
 */
unsigned char calc_check_sum(unsigned char* buf, int len)
{
    unsigned char sum = 0;
    while (len-- > 0) {
        sum += *(buf++);
    }
    return sum;
}

/**
 * @brief  布尔量DP点数值check
 * @param  [TY_OBJ_DP_S] *p_obj_dp
 * @return [int] 结果
 */
int check_dp_bool_invalid(TY_OBJ_DP_S* p_obj_dp)
{
    if (p_obj_dp == NULL) { // 判断数据是否合法
        PR_ERR("error! input is null");
        return -1;
    }

    if (p_obj_dp->type != PROP_BOOL) { // 判断类型是否合法
        PR_ERR("error! input is not bool %d", p_obj_dp->type);
        return -2;
    }

    if (0 == p_obj_dp->value.dp_bool) {
        return FALSE;
    } else if (1 == p_obj_dp->value.dp_bool) {
        return TRUE;
    } else {
        PR_ERR("Error!! type invalid %d ", p_obj_dp->value.dp_bool);
        return -2;
    }
}

/**
 * @brief  接收obj dp函数
 * @param  [TY_RECV_OBJ_DP_S*] p_obj_dp
 * @return [*]
 */
VOID robotics_app_dp_cmd_objs(IN CONST TY_RECV_OBJ_DP_S* dp_rev)
{
    TY_OBJ_DP_S* dp_data = (TY_OBJ_DP_S*)(dp_rev->dps);
    UINT_T cnt = dp_rev->dps_cnt;
    INT_T table_idx = 0;
    INT_T table_count = (sizeof(s_dp_table) / sizeof(s_dp_table[0]));
    INT_T index = 0;
    for (index = 0; index < cnt; index++) {
        TY_OBJ_DP_S* p_dp_obj = dp_data + index;
        PR_DEBUG("recv dpid:[%d]\r\n", p_dp_obj->dpid);

        for (table_idx = 0; table_idx < table_count; table_idx++) {
            if (s_dp_table[table_idx].dp_id == p_dp_obj->dpid) {
                s_dp_table[table_idx].handler(p_dp_obj);
                /*开发者在此处接收到的数据，只能做一些简单解析处理，具体要执行的行为，请放到别的任务中去*/
                break;
            }
        }
    }
}

/**
 * @brief  接收raw dp函数
 * @param  [TY_RECV_RAW_DP_S*] p_obj_dp
 * @return [*]
 */
VOID robotics_app_dp_raw_objs(IN CONST TY_RECV_RAW_DP_S* dp_rev)
{
    unsigned char* dp_data = (unsigned char*)dp_rev->data;
    int table_idx = 0;
    int table_count = (sizeof(s_raw_dp_table) / sizeof(s_raw_dp_table[0]));

    PR_DEBUG("dpid:%d cmd_tp:%d", dp_rev->dpid, dp_rev->cmd_tp);
    for (table_idx = 0; table_idx < table_count; table_idx++) {
        if (s_raw_dp_table[table_idx].dp_id == dp_rev->dpid) {
            s_raw_dp_table[table_idx].handler(dp_data, dp_rev->len);
            /*收到的数据可参考涂鸦激光扫地机的协议*/
            /*开发者在此处接收到的数据，只能做一些简单解析处理，具体要执行的行为，请放到别的任务中去*/
            break;
        }
    }
}

/**
 * @brief  接收查询dp函数
 * @param  [TY_DP_QUERY_S*] p_obj_dp
 * @return [*]
 */
VOID robotics_app_dp_query_objs(IN CONST TY_DP_QUERY_S* dp_query)
{
    if (!dp_query->cnt) {
        PR_DEBUG("recv query objs cnt = 0");
        return;
    }
    dp_handle_sync_to_cloud(); // APP来查询时，同步设备所有的DP数据
}

/**
 * @brief  接收到sdk obj dp处理回调，注册到SDK中
 * @param  [TY_SDK_RECV_OBJ_DP_S] *dp_rev
 * @return [*]
 */
VOID ty_cmd_handle_dp_cmd_objs(IN CONST TY_RECV_OBJ_DP_S* dp_rev)
{
    PR_DEBUG("[user cmd]recv cmd dp");
    robotics_app_dp_cmd_objs(dp_rev);
}

/**
 * @brief  接收到sdk raw dp处理回调，注册到SDK中
 * @param  [TY_SDK_RECV_OBJ_DP_S] *dp_rev
 * @return [*]
 */
VOID ty_cmd_handle_dp_raw_objs(IN CONST TY_RECV_RAW_DP_S* dp_rev)
{
    PR_DEBUG("[user raw cmd]recv cmd dp");
    robotics_app_dp_raw_objs(dp_rev);
}

/**
 * @brief  接收到sdk query dp处理回调，注册到SDK中
 * @param  [TY_SDK_RECV_OBJ_DP_S] *dp_rev
 * @return [*]
 */
VOID ty_cmd_handle_dp_query_objs(IN CONST TY_DP_QUERY_S* dp_query)
{
    PR_DEBUG("[user cmd]recv query dp");
    robotics_app_dp_query_objs(dp_query);
}

/**
 * @brief  调用sdk中的接口进行数值型dp回复
 * @param  [BYTE_T] dp_id
 * @param  [INT_T] val
 * @return [*]
 */
VOID respone_dp_value(BYTE_T dp_id, INT_T val)
{
    TY_OBJ_DP_S dp_obj_data[1] = { { 0 } };
    dp_obj_data[0].dpid = dp_id;
    dp_obj_data[0].type = PROP_VALUE;
    dp_obj_data[0].value.dp_value = val;
    /*以下是dp异步上报，会过滤重复的dp值，当您不关心实际的上报次数，只需最后一次上报的状态和设备的实际状态保持一致时，就可以选择这种上报方式，如开关量、累计值等*/
    dev_report_dp_json_async(NULL, dp_obj_data, 1);
    /*以下是dp异步强制上报，不会过滤重复的dp值，如果您没有这方面的需求，请不要使用，以免dp上报受限制（云端最大一天3500条dp记录）*/
    // dev_report_dp_json_async_force(NULL, dp_obj_data, 1);
    /*注意：异步上报是SDK会缓存上报数据，等到SDK内的上报线程被调度时再将数据上报，不会堵塞应用线程。*/
}

/**
 * @brief  调用sdk中的接口进行布尔量dp回复
 * @param  [BYTE_T] dp_id
 * @param  [BOOL_T] true_false
 * @return [*]
 */
VOID respone_dp_bool(BYTE_T dp_id, BOOL_T true_false)
{
    TY_OBJ_DP_S dp_obj_data[1] = { { 0 } };
    dp_obj_data[0].dpid = dp_id;
    dp_obj_data[0].type = PROP_BOOL;
    dp_obj_data[0].value.dp_bool = true_false;

    dev_report_dp_json_async(NULL, dp_obj_data, 1); // dp异步上报
}

/**
 * @brief  调用sdk中的接口进行枚举dp回复
 * @param  [BYTE_T] dp_id
 * @param  [INT_T] p_val_enum
 * @return [*]
 */
VOID respone_dp_enum(BYTE_T dp_id, INT_T p_val_enum)
{
    TY_OBJ_DP_S dp_obj_data[1] = { { 0 } };
    dp_obj_data[0].dpid = dp_id;
    dp_obj_data[0].type = PROP_ENUM;
    dp_obj_data[0].value.dp_enum = p_val_enum;

    dev_report_dp_json_async(NULL, dp_obj_data, 1); // dp异步上报
    // dev_report_dp_stat_sync(NULL, dp_obj_data, 1, 5); // dp同步上报
    /*注意：SDK立即上报数据，会堵塞应用线程，且不会过滤相同值的 DP 数据。
           对应用的堆栈深度要求也会比较高。堵塞时间可通过接口参数进行配置，一般设置 5S。
           有些场景对 DP 的上报次数需要精准控制，应用需要立即知道这次上报的结果，做出相应处理。*/
}

/**
 * @brief  调用sdk中的接口进行图位型dp回复
 * @param  [BYTE_T] dp_id
 * @param  [INT_T] p_val_str
 * @return [*]
 */
void respone_dp_bitmap(BYTE_T dp_id, INT_T p_val_str)
{
    TY_OBJ_DP_S dp_obj_data[1] = { { 0 } };
    dp_obj_data[0].dpid = dp_id;
    dp_obj_data[0].type = PROP_BITMAP;
    dp_obj_data[0].value.dp_bitmap = p_val_str;

    dev_report_dp_json_async(NULL, dp_obj_data, 1); // dp异步上报
}

/**
 * @brief  调用sdk中的接口进行字符串dp回复
 * @param  [BYTE_T] dp_id
 * @param  [CHAR_T] *p_val_str
 * @return [*]
 */
VOID respone_dp_str(BYTE_T dp_id, CHAR_T* p_val_str)
{
    TY_OBJ_DP_S dp_obj_data[1] = { { 0 } };
    dp_obj_data[0].dpid = dp_id;
    dp_obj_data[0].type = PROP_STR;
    dp_obj_data[0].value.dp_str = p_val_str;

    dev_report_dp_json_async(NULL, dp_obj_data, 1); // dp异步上报
}

/**
 * @brief  调用sdk中的接口进行raw dp回复
 * @param  [unsigned int] dp_id
 * @param  [unsigned char] *p_val_str
 * @return [*]
 */
void response_dp_raw(unsigned int dp_id, unsigned char* p_val_raw, unsigned int len)
{
    dev_report_dp_raw_sync(NULL, dp_id, p_val_raw, len, 5); // raw数据同步上报
    /*RAW类型 DP 只支持同步上报，且SDK不会对该 DP 数据进行缓存，即也不会有过滤和数据同步机制*/
}

/**
 * @brief  获取DP 处理核心参数设置
 * @return [*]
 */
ROBOT_DP_S* get_dp_handler_core_sets(void)
{
    return &dp_set;
}

#ifdef TUYA_DP_SWITCH_GO
/**
 * @brief  结合work_mode、trans raw数据, 根据switch go开启
 * @param  [ROBOT_DP_S*] dp_set
 * @return [*]
 */
static void __dp_handle_switch_go_deal(int switch_go)
{
    /***attention：
     *  1.当收到switch_go指令前，app必然会下发清扫模式、选区清扫等指令告知当前清扫目标；
     *  2.根据当前收到的switch_go状态及app下发的清扫模式控制机器执行相应的动作；
     * ***/

    ROBOT_DP_S* dp_handle = get_dp_handler_core_sets();

    if (switch_go) {
        switch (dp_handle->work_mode) {
        case ROBOT_MODE_SMART: {

            /** 当前执行的智能清扫任务，可将指令告知业务执行智能清扫； **/
        } break;

        case ROBOT_MODE_SELECT_ROOM: {

            /** 当前执行选区清扫任务，结合选区参数执行清扫任务**/
        } break;

        case ROBOT_MODE_ZONE: {

            /** 当前执行划区清扫任务，结合划区参数执行清扫任务**/
        } break;

        case ROBOT_MODE_POSE: {

            /** 当前执行定点清扫任务，结合当前点的地图坐标执行清扫任务**/
        } break;

        default:
            /** 其他清扫模式根据业务需求及协议实现 **/
            break;
        }

    } else {

        /** 清扫任务结束，可将指令告知业务结束清扫任务 **/
    }
}

const STR_ELEMENT_S mode_string_table[] = {
    { .len = strlen("both_work"), .name = "both_work" },
    { .len = strlen("only_sweep"), .name = "only_sweep" },
    { .len = strlen("only_mop"), .name = "only_mop" },    
    { .len = strlen("clean_before_mop"), .name = "clean_before_mop" },
};

const STR_ELEMENT_S sunction_string_table[] = {
    { .len = strlen("closed"), .name = "closed" },
    { .len = strlen("gentle"), .name = "gentle" },
    { .len = strlen("normal"), .name = "normal" },
    { .len = strlen("strong"), .name = "strong" },
    { .len = strlen("max"), .name = "max" },
};

const STR_ELEMENT_S cistern_string_table[] = {
    { .len = strlen("closed"), .name = "closed" },
    { .len = strlen("low"), .name = "low" },
    { .len = strlen("middle"), .name = "middle" },
    { .len = strlen("high"), .name = "high" },
};

/**
 * @brief  获取吸力和水量枚举类型对应的字符串
 * @param  [out] str_name 枚举类型对应的字符串数据
 * @param  [in] type 控制类枚举类型
 * @return [OPERATE_RET]  OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
static OPERATE_RET enum_index_get_from_string(char* str_name, ENUM_CMD_TYPE_E type)
{
    int index = 0;
    switch (type) {
    case MODE_ENUM_TYPE: {
        for (index = 0; index < sizeof(mode_string_table) / sizeof(mode_string_table[0]); index++) {
            if ((strlen(str_name) == mode_string_table[index].len) && (0 == strncmp(str_name, mode_string_table[index].name, mode_string_table[index].len))) {
                break;
            }
        }
        if (index < (sizeof(mode_string_table) / sizeof(mode_string_table[0]))) {
            return index;
        } else {
            PR_ERR("enum string not found");
            return OPRT_NOT_FOUND;
        }

    } break;
    case CISTERN_ENUM_TYPE: {
        for (index = 0; index < sizeof(cistern_string_table) / sizeof(cistern_string_table[0]); index++) {
            if ((strlen(str_name) == cistern_string_table[index].len) && (0 == strncmp(str_name, cistern_string_table[index].name, cistern_string_table[index].len))) {
                break;
            }
        }

        if (index < (sizeof(cistern_string_table) / sizeof(cistern_string_table[0]))) {
            return index;
        } else {
            PR_ERR("enum string not found");
            return OPRT_NOT_FOUND;
        }
    } break;
    case SUCTION_ENUM_TYPE: {
        for (index = 0; index < sizeof(sunction_string_table) / sizeof(sunction_string_table[0]); index++) {
            if ((strlen(str_name) == sunction_string_table[index].len) && (0 == strncmp(str_name, sunction_string_table[index].name, sunction_string_table[index].len))) {
                break;
            }
        }
        if (index < (sizeof(sunction_string_table) / sizeof(sunction_string_table[0]))) {
            return index;
        } else {
            PR_ERR("enum string not found");
            return OPRT_NOT_FOUND;
        }
    } break;
    default: {
        PR_NOTICE("enum type change not support");
    } break;
    }
    return OPRT_OK;
}

/**
 * @brief  获取吸力和水量枚举值对应的字符串
 * @param  [in] index 枚举值的初始化值
 * @param  [in] type 控制类枚举类型
 * @return [char *]  获取到枚举值对应的字符串
 */
char* enum_string_get_from_index(int index, ENUM_CMD_TYPE_E type)
{
    char* str_name = NULL;
    switch (type) {
    case MODE_ENUM_TYPE: {
        if (index >= (sizeof(mode_string_table) / sizeof(mode_string_table[0]))) {
            PR_ERR("enum index overflow");
            str_name = mode_string_table[0].name;
        } else {
            str_name = mode_string_table[index].name;
        }
    } break;
    case CISTERN_ENUM_TYPE: {
        if (index >= (sizeof(cistern_string_table) / sizeof(cistern_string_table[0]))) {
            PR_ERR("enum index overflow");
            str_name = cistern_string_table[0].name;
        } else {
            str_name = cistern_string_table[index].name;
        }
    } break;
    case SUCTION_ENUM_TYPE: {
        if (index >= (sizeof(sunction_string_table) / sizeof(sunction_string_table[0]))) {
            PR_ERR("enum index overflow");
            str_name = sunction_string_table[0].name;
        } else {
            str_name = sunction_string_table[index].name;
        }
    } break;
    default: {
        PR_NOTICE("enum type change not support");
        return NULL;
    } break;
    }
    PR_DEBUG("index string:%s,%p", str_name,str_name);
    return str_name;
}

/**
 * @brief  定点清扫信息回复
 * @return [*]
 */
OPERATE_RET robot_pose_sets_response(ROBOT_WORK_POSE_EXTRA_S pose_sets, OPERATE_RET errcode)
{
    CLEAN_PARAM_S param[3] = { 0 };
    STR_ELEMENT_S clean_mode[3] = { 0 };
    int map_id = 0; //设备当前地图id，需要开发根据实际情况去获取
    //将数据格式进行转换，并回复给APP
    param[0].valid = 1;
    param[0].clean_cnt = pose_sets.clean_times;
    param[0].y_mop = 1;
    char* temp_str = enum_string_get_from_index(0, CISTERN_ENUM_TYPE);
    if(temp_str == NULL){
        PR_DEBUG("get enum string err");
        return OPRT_COM_ERROR;
    }
    PR_DEBUG("str get ok:%p", temp_str);
    PR_DEBUG("str get strlen:%d", strlen(temp_str));
    param[0].cistern.len = strlen(temp_str);
    param[0].cistern.name = temp_str;

    temp_str = enum_string_get_from_index(0, SUCTION_ENUM_TYPE);
    if(temp_str == NULL){
        PR_DEBUG("get enum string err");
        return OPRT_COM_ERROR;
    }
    param[0].suction.len = strlen(temp_str);
    param[0].suction.name = temp_str;

    POINT_COOR_S point_array_test[1];
    point_array_test[0].x = (int)(pose_sets.points[0].vertices[0].x * 100 / 5);
    point_array_test[0].y = (int)(pose_sets.points[0].vertices[0].y * 100 / 5);

    SPOT_CLEAN_S p_spot_clean = { 0 };

    p_spot_clean.num = 1;
    p_spot_clean.points = point_array_test;
    p_spot_clean.mode = clean_mode;
    temp_str = enum_string_get_from_index(0, MODE_ENUM_TYPE);
    if(temp_str == NULL){
        PR_DEBUG("get enum string err");
        return OPRT_COM_ERROR;
    }
    p_spot_clean.mode[0].len = strlen(temp_str);
    p_spot_clean.mode[0].name = temp_str;
    p_spot_clean.param = param;
    p_spot_clean.map_id = map_id;
    OPERATE_RET ret = ty_rvc_spot_clean_data_response(&p_spot_clean,  0);

    return ret;
}

void dp_handle_pose_clean_info_response()
{
    robot_pose_sets_response(dp_set.pose_sets, 0);
}

/**
 * @brief  划区清扫信息回复
 * @return [*]
 */
OPERATE_RET robot_zone_clean_sets_response(ROBOT_WORK_AREA_EXTRA_S zone_clean_sets, OPERATE_RET errcode)
{
    ZONE_CLEAN_S zone_clean = { 0 };
    ZONE_AREA_PARAM_S zone_area_param[COORDINATE_MAX_LEN];
    int map_id = 0; //设备当前地图id，需要开发根据实际情况去获取
    //将数据格式进行转换，并回复给APP
    POINT_COOR_S point_array_test[COORDINATE_MAX_LEN][4] = { 0 };
    char name_array[COORDINATE_MAX_LEN][NAME_MAX_LEN] = { 0 };

       zone_clean.num = zone_clean_sets.area_count;
    zone_clean.clean_zone = zone_area_param;

    for (int j = 0; j < zone_clean.num; j++) {
        point_array_test[j][0].x = (int)(zone_clean_sets.points[j].vertices[0].x * 100 / 5);
        point_array_test[j][0].y = (int)(zone_clean_sets.points[j].vertices[0].y * 100 / 5);
        point_array_test[j][1].x = (int)(zone_clean_sets.points[j].vertices[1].x * 100 / 5);
        point_array_test[j][1].y = (int)(zone_clean_sets.points[j].vertices[1].y * 100 / 5);
        point_array_test[j][2].x = (int)(zone_clean_sets.points[j].vertices[2].x * 100 / 5);
        point_array_test[j][2].y = (int)(zone_clean_sets.points[j].vertices[2].y * 100 / 5);
        point_array_test[j][3].x = (int)(zone_clean_sets.points[j].vertices[3].x * 100 / 5);
        point_array_test[j][3].y = (int)(zone_clean_sets.points[j].vertices[3].y * 100 / 5);

        char* temp_str = enum_string_get_from_index(0, MODE_ENUM_TYPE);
        zone_area_param[j].mode.len = strlen(temp_str);
        zone_area_param[j].mode.name = temp_str;

        temp_str = enum_string_get_from_index(0, CISTERN_ENUM_TYPE);
        zone_area_param[j].param.cistern.len = strlen(temp_str);
        zone_area_param[j].param.cistern.name = temp_str;

        temp_str = enum_string_get_from_index(0, SUCTION_ENUM_TYPE);
        zone_area_param[j].param.suction.len = strlen(temp_str);
        zone_area_param[j].param.suction.name = temp_str;

        zone_area_param[j].param.y_mop = 0;
        zone_area_param[j].param.clean_cnt = 1;
        zone_area_param[j].area.point_num = 4;
        zone_area_param[j].area.point = point_array_test[j];
        strncpy(name_array[j], zone_clean_sets.names[j].name, zone_clean_sets.names[j].name_len);
        zone_area_param[j].cur_name.len = zone_clean_sets.names[j].name_len;
        zone_area_param[j].cur_name.name = name_array[j];
    }

    PR_DEBUG("p_zone_clean_area:%x,%x,%x", &zone_clean, zone_clean.clean_zone);
    zone_clean.map_id = map_id;
    OPERATE_RET ret = ty_rvc_zone_clean_data_response(&zone_clean, 0);

    return ret;
}

void dp_handle_area_clean_info_response(void)
{
    robot_zone_clean_sets_response(dp_set.area_clean_sets, 0);
}

/**
 * @brief  选区清扫信息回复
 * @return [*]
 */
OPERATE_RET robot_room_clean_sets_response(ROBOT_WORK_ROOM_EXTRA_S room_clean_sets, OPERATE_RET errcode)
{
    CLEAN_PARAM_S param[ROOM_CLEAN_MAX_NUM] = { 0 };
    int room_ids[ROOM_CLEAN_MAX_NUM] = { 0 };
    STR_ELEMENT_S clean_mode[ROOM_CLEAN_MAX_NUM] = { 0 };
    ROOM_CLEAN_S room_clean_test;
    int map_id = 0; //设备当前地图id，需要开发根据实际情况去获取
    //将数据格式进行转换，并回复给APP
    room_clean_test.num = room_clean_sets.room_count;
    room_clean_test.ids = room_ids;
    room_clean_test.param = param;
    room_clean_test.sweep_mode = clean_mode;
    for (int i = 0; i < room_clean_test.num; i++) {
        char* temp_str = enum_string_get_from_index(room_clean_sets.points[i].sweep_mode, MODE_ENUM_TYPE);
        clean_mode[i].len = strlen(temp_str);
        clean_mode[i].name = temp_str;

        param[i].valid = 1;
        param[i].clean_cnt = room_clean_sets.points[i].sweep_count;
        param[i].y_mop = room_clean_sets.points[i].Y_mop;
        temp_str = enum_string_get_from_index(room_clean_sets.points[i].water_level, CISTERN_ENUM_TYPE);
        param[i].cistern.len = strlen(temp_str);
        param[i].cistern.name = temp_str;

        temp_str = enum_string_get_from_index(room_clean_sets.points[i].fan_level, SUCTION_ENUM_TYPE);
        param[i].suction.len = strlen(temp_str);
        param[i].suction.name = temp_str;

        room_ids[i] = room_clean_sets.points[i].room_ids;
    }
    room_clean_test.map_id = map_id;
    OPERATE_RET ret = ty_rvc_room_clean_data_response(&room_clean_test, 0);

    return ret;
}

void dp_handle_room_clean_info_response(void)
{
    robot_room_clean_sets_response(dp_set.room_clean_sets, 0);
}

/**
 * @brief  收到switch_go指令后，进行相应的回复app
 * @param  [int] switch_go
 * @return [*]
 */
static void __dp_handle_switch_go_response(int switch_go)
{
    /***attention：
     *  1.switch_go这个dp点下发前会下发模式、清扫模式等；因此在收到siwitch_go这个dp点时需要对之前的信息进行回复；
     *  2.每个模式需要回复的内容根据协议确定；
     * ***/

    ROBOT_DP_S* dp_handle = get_dp_handler_core_sets();

    if (switch_go) { // 当前siwtch_go == ture , 执行清扫任务,以下数据是设备当前状态同步给面板展示。

        dp_handle_pause_response(FALSE); //回复暂停及回充开关为FALSE
        dp_handle_charge_response(FALSE);
        PR_DEBUG("dp_handle->work_mode %d", dp_handle->work_mode);
        switch (dp_handle->work_mode) {
        case ROBOT_MODE_SMART: {
            dp_handle_switch_go_response(TRUE); // 回复siwitch_go=TRUE开始清扫;
            dp_handle_work_mode_response(ROBOT_MODE_SMART); // 回复work_mode;
            dp_handle_status_response(DP_ST_SMART); // 回复当前机器状态;
        } break;

        case ROBOT_MODE_SELECT_ROOM: {
            dp_handle_room_clean_info_response(); //选区清扫数据回复
            dp_handle_switch_go_response(TRUE); // 回复siwitch_go=TRUE开始清扫;
            dp_handle_work_mode_response(ROBOT_MODE_SELECT_ROOM); // 回复work_mode;
            dp_handle_status_response(DP_ST_SELECT_ROOM); // 回复当前机器状态;
        } break;

        case ROBOT_MODE_ZONE: {
            dp_handle_area_clean_info_response(); //划区清扫数据回复
            dp_handle_switch_go_response(TRUE); // 回复siwitch_go=TRUE开始清扫;
            dp_handle_work_mode_response(ROBOT_MODE_ZONE); // 回复work_mode;
            dp_handle_status_response(DP_ST_ZONE_CLEAN); // 回复当前机器状态;
        } break;

        case ROBOT_MODE_POSE: {
            dp_handle_pose_clean_info_response(); //定点清扫数据回复
            dp_handle_switch_go_response(TRUE); // 回复siwitch_go=TRUE开始清扫;
            dp_handle_work_mode_response(ROBOT_MODE_POSE); // 回复work_mode;
            dp_handle_status_response(DP_ST_GOTO_POS); // 回复当前机器状态;
        } break;

        default:
            break;
        }
    } else {
        dp_handle_switch_go_response(FALSE); //当前switch_go == false ， 结束任务
        dp_handle_status_response(DP_ST_STANDBY); // 回复当前机器状态;
    }
}

/**
 * @brief  switch_go dp接收处理并上报
 * @param  [TY_SDK_OBJ_DP_S*] p_obj_dp
 * @return [*]
 */
static void __dp_handle_switch_go(TY_OBJ_DP_S* p_obj_dp)
{
    int switch_go;

    switch_go = check_dp_bool_invalid(p_obj_dp); //验证数据内容及类型
    if (switch_go < 0) {
        PR_ERR("switch dp set err!");
        return;
    }

    dp_set.switch_go = switch_go;

    PR_DEBUG("switch go goto deal %d!!!", switch_go);
    /*****这里可以做相对应的业务处理******/
    __dp_handle_switch_go_deal(switch_go);

    /*****这里对数据进行相应的回复：可以在本文件中执行，也可告知业务功能，在业务中进行回复*****/
    __dp_handle_switch_go_response(switch_go);

    return;
}

/**
 * @brief  switch_go dp 回复云端
 * @return [*]
 */
void dp_handle_switch_go_response(bool switch_go)
{
    PR_DEBUG("goto set switch_go = %d", switch_go);

    respone_dp_bool(TUYA_DP_SWITCH_GO, switch_go);
    return;
}
#endif

#ifdef TUYA_DP_PAUSE
/**
 * @brief  结合work_mode、trans raw数据, 根据switch go开启
 * @param  [ROBOT_DP_S*] dp_set
 * @return [*]
 */
static void __dp_handle_pause_deal(int pause)
{
    /***attention：
     *  收到pause == true ，表明需要暂停的任务；pause == false 表明继续当前的任务
     * ***/

    // ROBOT_DP_S* dp_handle = get_dp_handler_core_sets();

    if (pause) {
        /**
         * 这里可以做相对应的业务处理:
         * 收到app下发暂停任务指令，发布指令给业务将当前执行的任务暂停；
         * ***/

    } else {
        /**
         * 这里可以做相对应的业务处理:
         * 收到app下发继续任务指令，发布指令给业务继续执行原有任务
         * ***/
    }
}

/**
 * @brief  收到switch_go指令后，进行相应的回复app
 * @param  [int] switch_go
 * @return [*]
 */
static void __dp_handle_pause_response(int pause)
{
    /***attention：
     *  对app进行回复的时候，需要包含暂停指令回复、当前状态回复
     * ***/

    if (pause) {
        dp_handle_pause_response(TRUE); // 回复app，当前收到继续
        dp_handle_status_response(DP_ST_PAUSED); // 当前为暂停状态
    } else {
        dp_handle_pause_response(FALSE); // 回复app，当前收到继续
        dp_handle_status_restart_response(); // 回复当前状态，当前状态根据之前的状态进行上报；
    }
}

/**
 * @brief  暂停开始DP点处理
 * @param  TY_OBJ_DP_S* p_obj_dp
 * @return [TY_OBJ_DP_S*] p_obj_dp
 */
static void __dp_handle_pause(TY_OBJ_DP_S* p_obj_dp)
{
    int pause_onoff = check_dp_bool_invalid(p_obj_dp); // 验证数据内容及类型
    if (pause_onoff < 0) {
        PR_ERR("pause dp set err!");
        return;
    }

    dp_set.pause = pause_onoff;

    PR_DEBUG("pause goto deal %d!!!", pause_onoff);
    /*****这里可以做相对应的业务处理******/
    __dp_handle_pause_deal(pause_onoff);

    /*****这里对数据进行相应的回复******/
    __dp_handle_pause_response(pause_onoff);

    return;
}

/**
 * @brief  暂停DP设置 & 上报
 * @return [*]
 */
static void dp_handle_pause_response(bool pause)
{
    PR_DEBUG("goto set pause %d", pause);

    respone_dp_bool(TUYA_DP_PAUSE, pause);
    return;
}
#endif

#ifdef TUYA_DP_SWITCH_CHARGE
/**
 * @brief  设置回充
 * @param  TY_OBJ_DP_S* p_obj_dp
 * @return [*]
 */
static void __dp_handle_charge(TY_OBJ_DP_S* p_obj_dp)
{
    int switch_charge = check_dp_bool_invalid(p_obj_dp); // 验证数据内容及类型

    if (switch_charge < 0) {
        PR_ERR("pause dp set err!");
        return;
    }
    PR_DEBUG("switch charge goto deal %d!!!", switch_charge);
    /**
     * 这里可以做相对应的业务处理:
     * 此处收到app下发回充指令，发布给业务进行相应启动回充任务
     * ***/

    /*****这里需要对数据进行的回复******/
    // attention:回复回充信息不单单只回复回充指令，还需将当前状态切换成回充状态发送给app
    dp_handle_charge_response(switch_charge);
    if (switch_charge) {
        dp_handle_status_response(DP_ST_GOTO_CHARGE);
    } else {
        dp_handle_status_response(DP_ST_STANDBY);
    }
    return;
}

/**
 * @brief  回充DP上报回复
 * @return [*]
 */
static void dp_handle_charge_response(bool switch_charge)
{
    PR_DEBUG("goto set charge switch dp %d", switch_charge);

    respone_dp_bool(TUYA_DP_SWITCH_CHARGE, switch_charge); // 调用接口回复云端

    return;
}
#endif

#ifdef TUYA_DP_WORK_MODE
/**
 * @brief  运行模式设置（自动、定点、区域、房间等）
 * @param  [TY_OBJ_DP_S*] p_obj_dp
 * @return [*]
 */
static void __dp_handle_work_mode_set(TY_OBJ_DP_S* p_obj_dp)
{
    if ((NULL == p_obj_dp) || (p_obj_dp->type != PROP_ENUM)) { // 验证数据内容及类型
        PR_ERR("Error!! type invalid ");
        return;
    }

    int set_mode = p_obj_dp->value.dp_enum;

    PR_DEBUG(">>>>>>>>>>>>>>>>>>>>>>>>set mode is %d %d", set_mode, p_obj_dp->value.dp_enum);
    /**
     * 这里可以做相对应的业务处理:
     * 工作模式需要保存，提供给switch_go指令使用；保存形式根据自身业务而定；
     * ***/

    dp_set.work_mode = set_mode;
    if ((dp_set.work_mode == ROBOT_MODE_ZONE) || (dp_set.work_mode == ROBOT_MODE_SELECT_ROOM) || (dp_set.work_mode == ROBOT_MODE_POSE)) {
        __dp_handle_switch_go_response(true); //清扫任务都需要回复清扫开关状态
    }
}

/**
 * @brief  运行模式 DP点回复 APP
 * @param  [ROBOT_WORK_MODE_E] mode
 * @return [*]
 */
static void dp_handle_work_mode_response(ROBOT_WORK_MODE_E mode)
{
    PR_DEBUG("goto set mode %d", mode);

    if (mode >= ROBOT_DP_SET_MAX) {
        mode = 0;
    } else {
        ;
    }

    /*****
     * 这里可以做相对应的业务处理
     * 1.将该数据加载到业务中，当开始清扫的时候使用该参数；保存形式根据自身业务而定
     * ******/
    dp_set.work_mode = mode;

    if (0 <= mode && mode < ROBOT_DP_SET_MAX) { // 枚举值上报需要做判断，超限值不要上报，可能会导致设备异常。
        /****** 需要对数据进行回复*******/
        respone_dp_enum(TUYA_DP_WORK_MODE, mode);
    } else {
        PR_ERR("dp %d response value out of range, value: %d.", TUYA_DP_WORK_MODE, mode);
    }
}
#endif

#ifdef TUYA_DP_STATUS
/**
 * @brief  暂停恢复任务任务状态自动获取 & 恢复
 * @return [*]
 */
static void dp_handle_status_restart_response(void)
{

    /*****attention
     * 当前机器从断点需扫或暂停重新开始的情况下，需要上报当前的机器状态；
     * 因此需要从当前的工作模式中获取当前的工作状态，并进行上报；
     * 此处使用全局变量保存工作模式，此方式根据自身业务功能自行决定；
     * ******/

    switch (dp_set.work_mode) {
    case ROBOT_MODE_SMART:
        dp_set.status = DP_ST_SMART;
        break;

    case ROBOT_MODE_GOTO_CHARGE:
        dp_set.status = DP_ST_GOTO_CHARGE;
        break;

    case ROBOT_MODE_ZONE:
        dp_set.status = DP_ST_ZONE_CLEAN;
        break;

    case ROBOT_MODE_POSE:
        dp_set.status = DP_ST_GOTO_POS;
        break;

    case ROBOT_MODE_SELECT_ROOM:
        dp_set.status = DP_ST_SELECT_ROOM;
        break;

    default:
        PR_ERR("Unknown work mode %d", dp_set.work_mode);
        return;
    }

    if (0 <= dp_set.status && dp_set.status < DP_ST_STATUS_MAX) { // attention:枚举值上报需要做判断，超限值不要上报，可能会导致设备异常。
        /****** 上报当前的状态 *******/
        respone_dp_enum(TUYA_DP_STATUS, dp_set.status);
    } else {
        PR_ERR("dp %d response value out of range, value: %d.", TUYA_DP_STATUS, dp_set.status);
    }

    return;
}

/**
 * @brief  status DP 设置 & 回复
 * @param  [ROBOT_STATUS_E] status
 * @return [*]
 */
static void dp_handle_status_response(ROBOT_STATUS_E status)
{
    PR_DEBUG("goto set status %d", status);

    /****机器状态需要配合其他清扫任务进行进行回复 ******/
    dp_set.status = status;

    if (0 <= dp_set.status && dp_set.status < DP_ST_STATUS_MAX) {
        /****** 需要对数据进行回复*******/
        respone_dp_enum(TUYA_DP_STATUS, dp_set.status);
    } else {
        PR_ERR("dp %d response value out of range, value: %d.", TUYA_DP_STATUS, dp_set.status);
    }
}
#endif

#ifdef TUYA_DP_CLEAN_MODE
/**
 * @brief  清扫工作模式DP点设置（只扫、只拖、扫拖）
 * @param  [TY_SDK_OBJ_DP_S*] p_obj_dp
 * @return [*]
 */
static void __dp_handle_clean_mode_set(TY_OBJ_DP_S* p_obj_dp)
{
    if ((NULL == p_obj_dp) || (p_obj_dp->type != PROP_ENUM)) {
        PR_ERR("Error!! type invalid ");
        return;
    }

    int set_clean_mode = p_obj_dp->value.dp_enum;

    PR_DEBUG(">>>>>>>>>>>>>>>>>>>>>>>>set clean mode is %d", set_clean_mode);

    if (dp_set.clean_mode != set_clean_mode) {

        /*****
         * 这里可以做相对应的业务处理
         * 1.参数进行更新存储，因为重启后该数据仍需有效；
         * 2.将该数据加载到业务中，当开始清扫的时候使用该参数
         * ******/
        dp_set.clean_mode = set_clean_mode;
    }

    if (0 <= dp_set.clean_mode && dp_set.clean_mode < CLEAN_MODE_MAX) {
        /****** 需要对数据进行回复*******/
        respone_dp_enum(TUYA_DP_CLEAN_MODE, dp_set.clean_mode);
    } else {
        PR_ERR("dp %d response value out of range, value: %d.", TUYA_DP_CLEAN_MODE, dp_set.clean_mode);
    }
}
#endif

#ifdef TUYA_DP_DIRECTION_CONTROL
/**
 * @brief  遥控控制
 * @param  [TY_SDK_OBJ_DP_S*] p_obj_dp
 * @return [*]
 */
static void __dp_handle_direction_control(TY_OBJ_DP_S* p_obj_dp)
{
    if ((NULL == p_obj_dp) || (p_obj_dp->type != PROP_ENUM)) {
        PR_ERR("Error!! type invalid ");
        return;
    }

    if (0 <= p_obj_dp->value.dp_enum && p_obj_dp->value.dp_enum < CTRL_STATE_MAX) {
        /****** 需要对数据进行回复* ******/
        respone_dp_enum(TUYA_DP_DIRECTION_CONTROL, p_obj_dp->value.dp_enum);
    } else {
        PR_ERR("dp %d response value out of range, value: %d.", TUYA_DP_DIRECTION_CONTROL, p_obj_dp->value.dp_enum);
    }
    /*****
     * 这里可以做相对应的业务处理
     * 需要将收到的按键信息发布给业务，让业务控制扫地机器按照数据方向进行运动
     * ******/
}
#endif

#ifdef TUYA_DP_MAP_RESET
/**
 * @brief  地图重置
 * @param  [TY_SDK_OBJ_DP_S*] p_obj_dp
 * @return [*]
 */
static void __dp_handle_map_reset(TY_OBJ_DP_S* p_obj_dp)
{
    int map_reset = check_dp_bool_invalid(p_obj_dp); // 验证数据内容及类型

    if (map_reset < 0) {
        PR_ERR("switch dp set err!");
        return;
    }

    map_reset = p_obj_dp->value.dp_enum;

    /*****
     * 需要对数据进行回复
     * ******/
    respone_dp_bool(TUYA_DP_MAP_RESET, map_reset);

    /*****
     * 这里可以做相对应的业务处理
     * 需要将收到的重置地图信息发布给业务，让业务将原有地图删除
     * ******/
}
#endif

#ifdef TUYA_DP_SUCTION
/**
 * @brief  设置吸力
 * @param  [TY_OBJ_DP_S*] p_obj_dp
 * @return [*]
 */
static void __dp_handle_suction_set(TY_OBJ_DP_S* p_obj_dp)
{
    if ((NULL == p_obj_dp) || (p_obj_dp->type != PROP_ENUM)) { // 验证数据内容及类型
        PR_ERR("Error!! type invalid ");
        return;
    }
    int set_suction = p_obj_dp->value.dp_enum;

    PR_DEBUG(">>>>>>>>>>>>>>>>>>>>>>>>set suction is %d", set_suction);

    if (dp_set.suction_select != set_suction) {
        /*****
         * 这里可以做相对应的业务处理
         * 1.参数进行更新存储，因为重启后该数据仍需有效；
         * 2.将该数据加载到业务中，当开始清扫的时候使用该参数
         * ******/
        dp_set.suction_select = set_suction;
    }

    if (0 <= set_suction && set_suction < E_MOTOR_VACCUM_STATE_MAX) { // 枚举值上报需要做判断，超限值不要上报，可能会导致设备异常。
        /****** 需要对数据进行回复*******/
        respone_dp_enum(TUYA_DP_SUCTION, set_suction);
    } else {
        PR_ERR("dp %d response value out of range, value: %d.", TUYA_DP_SUCTION, set_suction);
    }

    return;
}
#endif

#ifdef TUYA_DP_CISTERN
/**
 * @brief  设置水量
 * @param  [TY_OBJ_DP_S*] p_obj_dp
 * @return [*]
 */
static void __dp_handle_cistern_set(TY_OBJ_DP_S* p_obj_dp)
{
    if ((NULL == p_obj_dp) || (p_obj_dp->type != PROP_ENUM)) { // 验证数据内容及类型
        PR_ERR("Error!! type invalid ");
        return;
    }

    int set_cistern = p_obj_dp->value.dp_enum;

    PR_DEBUG(">>>>>>>>>>>>>>>>>>>>>>>>set cistern is %d", set_cistern);

    if (dp_set.cistern_select != set_cistern) {
        /*****
         * 这里可以做相对应的业务处理
         * 1.参数进行更新存储，因为重启后该数据仍需有效；
         * 2.将该数据加载到业务中，当开始清扫的时候使用该参数
         * ******/
        dp_set.cistern_select = set_cistern;
    }

    if (0 <= set_cistern && set_cistern < E_MOTOR_PUMPER_STATE_MAX) { // 枚举值上报需要做判断，超限值不要上报，可能会导致设备异常。
        /****** 需要对数据进行回复*******/
        respone_dp_enum(TUYA_DP_CISTERN, set_cistern);
    } else {
        PR_ERR("dp %d response value out of range, value: %d.", TUYA_DP_CISTERN, set_cistern);
    }
}
#endif

#ifdef TUYA_DP_EDGE_BRUSH_LIEF
/**
 * @brief  边刷使用时长重置处理
 * @param  [TY_OBJ_DP_S*] p_obj_dp
 * @return [*]
 */
static void __dp_handle_edge_brush_used_time_reset(TY_OBJ_DP_S* p_obj_dp)
{
    if ((p_obj_dp == NULL) || (p_obj_dp->type != PROP_VALUE)) {
        PR_ERR("Error!! type invalid ");
        return;
    }

    if (p_obj_dp->value.dp_value != 0) {
        PR_ERR("Error!! edge brush used time set value invalid %d", p_obj_dp->value.dp_value);
        return;
    }
    /*****
     * 这里可以做相对应的业务处理
     * 1.重置边刷使用时长需要对该参数进行更新，并发布到业务中进行使用；
     * 2.更新后的使用时长信息进行保存，以便重启后数据正确使用；
     * ******/
    dp_set.edge_brush_used_time = 9999; // 9999是设置的最长边刷时长，此参数根据扫地机边刷使用情况自定

    /****** 需要对数据进行回复*******/
    respone_dp_value(TUYA_DP_EDGE_BRUSH_LIEF, dp_set.edge_brush_used_time);
}
#endif

#ifdef TUYA_DP_ROLL_BRUSH_LIEF
/**
 * @brief  滚刷使用时长重置处理
 * @param  [TY_SDK_OBJ_DP_S*] p_obj_dp
 * @return [*]
 */
static void __dp_handle_roll_brush_used_time_reset(TY_OBJ_DP_S* p_obj_dp)
{
    if ((p_obj_dp == NULL) || (p_obj_dp->type != PROP_VALUE)) {
        PR_ERR("Error!! type invalid ");
        return;
    }

    if (p_obj_dp->value.dp_value != 0) {
        PR_ERR("Error!! roll brush used time set value invalid %d", p_obj_dp->value.dp_value);
        return;
    }

    /*****
     * 这里可以做相对应的业务处理
     * 1.重置滚刷使用时长需要对该参数进行更新，并发布到业务中进行使用；
     * 2.更新后的使用时长信息进行保存，以便重启后数据正确使用；
     * ******/
    dp_set.roll_brush_used_time = 9999; // 9999是设置的最长滚刷时长，此参数根据扫地机滚刷使用情况自定

    /****** 需要对数据进行回复*******/
    respone_dp_value(TUYA_DP_ROLL_BRUSH_LIEF, dp_set.roll_brush_used_time);
}
#endif

#ifdef TUYA_DP_FILTER_LIEF
/**
 * @brief  过滤滤芯使用时长重置
 * @param  [TY_OBJ_DP_S*] p_obj_dp
 * @return [*]
 */
static void __dp_handle_filter_used_time_reset(TY_OBJ_DP_S* p_obj_dp)
{
    if ((p_obj_dp == NULL) || (p_obj_dp->type != PROP_VALUE)) {
        PR_ERR("Error!! type invalid ");
        return;
    }

    if (p_obj_dp->value.dp_value != 0) {
        PR_ERR("Error!! filter used time set value invalid %d", p_obj_dp->value.dp_value);
        return;
    }

    /*****
     * 这里可以做相对应的业务处理
     * 1.重置滤芯使用时长需要对该参数进行更新，并发布到业务中进行使用；
     * 2.更新后的使用时长信息进行保存，以便重启后数据正确使用；
     * ******/
    dp_set.filter_used_time = 9999; // 9999是设置的最长滤芯使用时长，此参数根据扫地机滤芯使用情况自定

    /****** 需要对数据进行回复*******/
    respone_dp_value(TUYA_DP_FILTER_LIEF, dp_set.filter_used_time);
}
#endif

#ifdef TUYA_DP_RAG_LIEF
/**
 * @brief  拖布使用时长重置
 * @param  [TY_OBJ_DP_S*] p_obj_dp
 * @return [*]
 */
static void __dp_handle_rag_used_time_reset(TY_OBJ_DP_S* p_obj_dp)
{

    if ((p_obj_dp == NULL) || (p_obj_dp->type != PROP_VALUE)) {
        PR_ERR("Error!! type invalid ");
        return;
    }

    if (p_obj_dp->value.dp_value != 0) {
        PR_ERR("Error!! rag used time set value invalid %d", p_obj_dp->value.dp_value);
        return;
    }

    /*****
     * 这里可以做相对应的业务处理
     * 1.重置拖布使用时长需要对该参数进行更新，并发布到业务中进行使用；
     * 2.更新后的使用时长信息进行保存，以便重启后数据正确使用；
     * ******/
    dp_set.rag_used_time = 9999; // 9999是设置的最长拖布使用时长，此参数根据扫地机拖布使用情况自定

    /****** 需要对数据进行回复*******/
    respone_dp_value(TUYA_DP_RAG_LIEF, dp_set.rag_used_time);
}
#endif

#ifdef TUYA_DP_LOG_SWITCH_GO
/**
 * @brief  本地日志上报开关
 * @param  [TY_OBJ_DP_S*] p_obj_dp
 * @return [*]
 */
static void __dp_handle_log_switch_go(TY_OBJ_DP_S* p_obj_dp)
{

    int log_onoff = check_dp_bool_invalid(p_obj_dp); // 验证数据内容及类型
    if (log_onoff < 0) {
        PR_ERR("log_onoff dp set err!");
        return;
    }

    dp_set.log_onoff = log_onoff;

    PR_DEBUG("log goto deal %d!!!", log_onoff);

    /*****这里对数据进行相应的回复******/
    respone_dp_bool(TUYA_DP_LOG_SWITCH_GO, log_onoff);

    return;
}
#endif

#ifdef TUYA_DP_HOME_PHOTOGRAPHY_ENABLE
static void __dp_handle_home_photography_enable(TY_OBJ_DP_S* p_obj_dp)
{
    int home_onoff = check_dp_bool_invalid(p_obj_dp); // 验证数据内容及类型
    if (home_onoff < 0) {
        PR_ERR("home_onoff dp set err!");
        return;
    }
    dp_set.home_onoff = home_onoff;
    PR_DEBUG("home onoff goto deal %d!!!", home_onoff);

    respone_dp_bool(TUYA_DP_HOME_PHOTOGRAPHY_ENABLE, home_onoff);
}
#endif

#ifdef TUYA_DP_MEDIA_SWITCH_GO
/**
 * @brief  视频开关
 * @param  [TY_OBJ_DP_S*] p_obj_dp
 * @return [*]
 */
static void __dp_handle_media_switch_go(TY_OBJ_DP_S* p_obj_dp)
{

    int media_onoff = check_dp_bool_invalid(p_obj_dp); // 验证数据内容及类型
    if (media_onoff < 0) {
        PR_ERR("media_onoff dp set err!");
        return;
    }

    dp_set.media_onoff = media_onoff;

    PR_DEBUG("media goto deal %d!!!", media_onoff);

    /*****这里对数据进行相应的回复******/
    respone_dp_bool(TUYA_DP_MEDIA_SWITCH_GO, media_onoff);

    return;
}
#endif

#ifdef TUYA_DP_SD_STATUS_ONLY_GET
/**
 * @brief  获取SD卡状态
 * @param  [TY_OBJ_DP_S*] p_obj_dp
 * @return [*]
 */
static void __dp_handle_sd_status_get(TY_OBJ_DP_S* p_obj_dp)
{
    ROBOT_SD_STATUS_E sd_status = DP_SD_ST_NULL;
    // todo:这里由开发者根据SD的真实状态填写
    sd_status = DP_SD_ST_NORMAL;
    respone_dp_value(TUYA_DP_SD_STATUS_ONLY_GET, sd_status);
}

/**
 * @brief  SD status DP 设置 & 回复
 * @param  [ROBOT_SD_STATUS_E] sd_status
 * @return [*]
 */
void dp_handle_sd_status_response(ROBOT_SD_STATUS_E sd_status)
{
    if ((sd_status < DP_SD_ST_NULL) || (sd_status >= DP_SD_ST_MAX)) {
        return;
    }
    respone_dp_value(TUYA_DP_SD_STATUS_ONLY_GET, sd_status);
}
#endif

#ifdef TUYA_DP_SD_STORAGE_ONLY_GET
/**
 * @brief  获取SD卡存储容量
 * @param  [TY_OBJ_DP_S*] p_obj_dp
 * @return [*]
 */
static void __dp_handle_sd_storage_get(TY_OBJ_DP_S* p_obj_dp)
{
    char tmp_str[100] = { 0 };

    int total = 100;
    int used = 0;
    int empty = 100;
    tuya_robot_get_sd_storage(&total, &used, &empty); //获取SD容量

    //"total capacity|Current usage|remaining capacity"
    snprintf(tmp_str, 100, "%u|%u|%u", total, used, empty);
    respone_dp_str(TUYA_DP_SD_STORAGE_ONLY_GET, tmp_str); //上报云端
}

/**
 * @brief  SD卡存储容量上报
 * @param  []
 * @return [*]
 */
void dp_handle_sd_storage_response(void)
{
    char tmp_str[100] = { 0 };

    int total = 100;
    int used = 0;
    int empty = 100;
    tuya_robot_get_sd_storage(&total, &used, &empty); //获取SD容量

    //"total capacity|Current usage|remaining capacity"
    snprintf(tmp_str, 100, "%u|%u|%u", total, used, empty);
    respone_dp_str(TUYA_DP_SD_STORAGE_ONLY_GET, tmp_str);
}
#endif

#ifdef TUYA_DP_SD_RECORD_MODE
/**
 * @brief  设置录像模式
 * @param  [int]  sd_record_mode 1为事件录像，2为连续录像
 * @return [*]
 */
void dp_handle_set_sd_record_mode(int sd_record_mode)
{
    tuya_robot_set_sd_record_mode(sd_record_mode);
    dp_set.sd_record_mode = sd_record_mode;
}

/**
 * @brief  获取录像模式
 * @param  [int]  sd_record_mode 1为事件录像，2为连续录像
 * @return [*]
 */
int dp_handle_get_sd_record_mode(void)
{
    return dp_set.sd_record_mode;
}

/**
 * @brief  SD卡录像模式
 * @param  [TY_OBJ_DP_S*] p_obj_dp
 * @return [*]
 */
static void __dp_handle_sd_record_mode(TY_OBJ_DP_S* p_obj_dp)
{
    if ((p_obj_dp == NULL) || (p_obj_dp->type != PROP_ENUM)) {
        PR_ERR("Error!! type invalid %d \r\n", p_obj_dp->type);
        return;
    }

    dp_handle_set_sd_record_mode(p_obj_dp->value.dp_enum);
    int mode = dp_handle_get_sd_record_mode();
    respone_dp_enum(TUYA_DP_SD_RECORD_MODE, mode);
}
#endif

#ifdef TUYA_DP_SD_RECORD_ENABLE
/**
 * @brief  SD卡录像开关状态
 * @param  [TY_OBJ_DP_S*] p_obj_dp
 * @return [*]
 */
static void __dp_handle_sd_record_enable(TY_OBJ_DP_S* p_obj_dp)
{
    BOOL_T sd_record_on_off = check_dp_bool_invalid(p_obj_dp);

    /* 注意：开发者自行保存开关状态 */
#ifdef TUYA_DP_SD_RECORD_MODE
    if (sd_record_on_off == TRUE) {
        dp_handle_set_sd_record_mode(dp_handle_get_sd_record_mode());
    } else {
        tuya_robot_set_sd_record_mode(0);
    }
#endif
    dp_set.sd_record_on_off = sd_record_on_off;
    tuya_robot_set_sd_record_onoff(dp_set.sd_record_on_off); //设置录像开关状态

    respone_dp_bool(TUYA_DP_SD_RECORD_ENABLE, sd_record_on_off);
}

#endif

#ifdef TUYA_DP_SD_UMOUNT
/**
 * @brief  卸载SD存储卡
 * @param  [TY_OBJ_DP_S*] p_obj_dp
 * @return [*]
 */
static void __dp_handle_sd_umount(TY_OBJ_DP_S* p_obj_dp)
{
    BOOL_T umount_ok = TRUE;
    /*开发者自行卸载SD卡动作，并将结果上报*/

    respone_dp_bool(TUYA_DP_SD_UMOUNT, umount_ok);
}
#endif

#ifdef TUYA_DP_SD_FORMAT
/**
 * @brief  SD存储卡格式化
 * @param  [TY_OBJ_DP_S*] p_obj_dp
 * @return [*]
 */
static void __dp_handle_sd_format(TY_OBJ_DP_S* p_obj_dp)
{
    tuya_robot_format_sd_card();
    respone_dp_bool(TUYA_DP_SD_FORMAT, TRUE);
}
#endif

#ifdef TUYA_DP_SD_FORMAT_STATUS_ONLY_GET
/**
 * @brief  格式化状态获取
 * @param  [TY_OBJ_DP_S*] p_obj_dp
 * @return [*]
 */
static void __dp_handle_sd_format_status_get(TY_OBJ_DP_S* p_obj_dp)
{
    int progress = dp_set.s_sd_format_progress;

    respone_dp_value(TUYA_DP_SD_FORMAT_STATUS_ONLY_GET, progress);
}

/**
 * @brief  SD卡式化状态上报
 * @param  [int] status 状态
 * @return [*]
 */
void dp_handle_report_sd_format_status(int status)
{
    respone_dp_value(TUYA_DP_SD_FORMAT_STATUS_ONLY_GET, status);
}

#endif

/**
 * @brief  保存在flash或内存中的数据加载
 * @return [*]
 */
void dp_handle_all_dp_load(void)
{
    memset(&dp_set, 0, sizeof(ROBOT_DP_S));
    /*****
     * 这里可以做相对应的业务处理
     * 暂存在内存的数据在启动的时候需要重新加载，如吸力选择，水量选择等；
     * ******/
}

/**
 * @brief  上报机器电池电量
 * @param  [int] percent 0~100
 * @return [*]
 */
void ty_cmd_handle_battery_capacity_response(int percent)
{
    /***attention：
     * 当电池电量发生变化的时候，将数据上报给app
     * ***/
#ifdef TUYA_DP_BATTERY_PERCENTAGE
    respone_dp_value(TUYA_DP_BATTERY_PERCENTAGE, percent);
#else
    PR_NOTICE("battert upload dp not define, please check!");
#endif
    return;
}

#if defined(MCS_CMD_SUPPORT) && (MCS_CMD_SUPPORT == 1)
/**
 * @brief  MCS语音控制错误码转换（Alexa使用）
 * @param  [TY_MGS_FAULT_TYPE_E] errno_type
 * @return [将原有的机器错误码转换为mcs固定的错位码]
 */
int mcs_fault_transform(FAULT_TYPE_E errno_type)
{
    int errno = 0;
    switch (errno_type) {
        case E_MSG_FT_WHEEL_LOCK:    //机器轮子卡住异常
        case E_MSG_FT_BRUSH_LOCK:    //机器边刷卡住
        case E_MSG_FT_ROLL_LOCK: {    //机器滚刷卡住  
            errno = E_MCS_DEVICE_STUCK; 
        } break;

        case E_MSG_FT_DUST_FULL: {          //尘袋满或者风机堵塞
            errno = E_MCS_MAINTENANCE_REQUIRED; 
        } break;
        case E_MSG_FW_UNFIND_DOCK: {   //找不到充电桩，无法充电
            errno = E_MCS_UNABLE_TO_CHARGE; 
        } break;

        case E_MSG_FW_ENTER_DISTURB_MODE: {  //进入勿扰模式
            errno = E_MCS_DO_NOT_DISTURB_MODE; 
        } break;
        case E_MSG_FT_LOW_POWER:            //电池低电量
        case E_MSG_FT_LOW_POWER_DUCK: {   //电量不足20%，无法工作
            errno = E_MCS_ENDPOINT_LOW_POWER; 
        } break;
        case E_MSG_FW_NO_WATER: {    //水量不足
            errno = E_MCS_INSUFFICIENT_RESOURCE; 
        } break;

        default: {
            PR_ERR("mcs fault errno err");          
        } break;
    }
    PR_DEBUG("mcs fault errno:%d", errno);          
    return errno;
}
#endif
/**
 * @brief  上报机器异常错误
 * @param  [FAULT_TYPE_E] errno_type
 * @return [*]
 */
void ty_cmd_handle_fault_response(FAULT_TYPE_E errno_type)
{
    /***attention：
     * 外部发生异常情况的时候，调用该接口进行上报
     * ***/
#ifdef TUYA_DP_FAULT
    if (errno_type > E_MSG_FF_MAX) { // 枚举值上报需要做判断，超限值不要上报，可能会导致设备异常。
        PR_ERR("errno type upload value set err %d", errno_type);
        return;
    }
    unsigned char errno[10] = {0};
    errno[0] = (unsigned char)errno_type;  //如果有多个异常同时发送，可以使用该接口组合上报

#if defined(MCS_CMD_SUPPORT) && (MCS_CMD_SUPPORT == 1)
    errno[1] = mcs_fault_transform(errno_type);   //转换为MCS语音控制错误码
    if((errno[1] >= E_MCS_DEVICE_STUCK) && (errno[1] < E_MCS_ERR_MAX)) {
        response_dp_raw(TUYA_DP_FAULT, errno, 2); //MCS语音控制错误码（Alexa使用），需组合做上报
    } else {
        response_dp_raw(TUYA_DP_FAULT, errno, 1); //非MCS语音控制错误码，不做上报
    }       
#else
    response_dp_raw(TUYA_DP_FAULT, errno, 1);
#endif

#else
    PR_ERR("default errno  DP not set, please check!");
#endif
    return;
}

/**
 * @brief  设备上线同步DP
 * @param  [unsigned char*] p_obj_dp
 * @param  [unsigned int] len
 * @return [*]
 */
OPERATE_RET dp_handle_sync_to_cloud(void)
{
    /*****
     * 这里可以做相对应的业务处理
     * 1.云端有个限制：一天不要超过3500条，10分钟内上报的dp点数量不要超过1200条，这个是云端对dp上报数量的限流。
     * 因此，将其组合上报，可解决上述问题
     * ******/

    PR_DEBUG("update all dp to cloud");
    OPERATE_RET ret = 0;
    TY_OBJ_DP_S* dp_obj_buf = NULL;

    char num = 7;

    dp_obj_buf = malloc(num * sizeof(TY_OBJ_DP_S));
    if (dp_obj_buf == NULL) {
        PR_ERR("ty_sdk_adapter_dp_num_report malloc error");
    }

    memset(dp_obj_buf, 0, num * sizeof(TY_OBJ_DP_S));

    TY_OBJ_DP_S* obj_data_tmp = dp_obj_buf;

    // switch go DP点
    obj_data_tmp->dpid = TUYA_DP_SWITCH_GO;
    obj_data_tmp->type = PROP_BOOL;
    obj_data_tmp->value.dp_bool = 0;

    obj_data_tmp += 1;

    // pause DP点
    obj_data_tmp->dpid = TUYA_DP_PAUSE;
    obj_data_tmp->type = PROP_BOOL;
    obj_data_tmp->value.dp_bool = 0;

    obj_data_tmp += 1;

    // switch charge DP点
    obj_data_tmp->dpid = TUYA_DP_SWITCH_CHARGE;
    obj_data_tmp->type = PROP_BOOL;
    obj_data_tmp->value.dp_bool = 0;

    obj_data_tmp += 1;

    // work_mode DP点
    obj_data_tmp->dpid = TUYA_DP_WORK_MODE;
    obj_data_tmp->type = PROP_ENUM;
    obj_data_tmp->value.dp_enum = 0;

    obj_data_tmp += 1;

    // status DP点
    obj_data_tmp->dpid = TUYA_DP_STATUS;
    obj_data_tmp->type = PROP_ENUM;
    obj_data_tmp->value.dp_enum = DP_ST_STANDBY;

    obj_data_tmp += 1;

    // CLEAN TIME DP点
    obj_data_tmp->dpid = TUYA_DP_CLEAN_TIME;
    obj_data_tmp->type = PROP_VALUE;
    obj_data_tmp->value.dp_value = 10;

    obj_data_tmp += 1;
    // CLEAN AREA DP点
    obj_data_tmp->dpid = TUYA_DP_CLEAN_AREA;
    obj_data_tmp->type = PROP_VALUE;
    obj_data_tmp->value.dp_value = 5;

    obj_data_tmp += 1;

    ret = dev_report_dp_json_async(NULL, dp_obj_buf, num); //异步上报

    if (dp_obj_buf) {
        free(dp_obj_buf);
        dp_obj_buf = NULL;
    }
    // ty_cmd_handle_fault_response(E_MSG_FT_BUMER_ERR);   //异常上报接口
    return ret;
}

/**
 * @brief  上报机器设备信息
 * @param  [*]
 * @return [*]
 */
OPERATE_RET dp_handle_device_info_respone(void)
{
    /*****
     * 这里可以做相对应的业务处理
     * 获取机器当前的mac、sn 、uuid等参数进行上报
     * ******/

    ty_cJSON* device_info = ty_cJSON_CreateObject(); //创建cjon
    if (NULL == device_info) {
        PR_ERR("json err");
        return OPRT_CR_CJSON_ERR;
    }
    char tmp_data[128] = { 0 };
    //获取wifi名称
    sprintf(tmp_data, "wifi_test_%04d", 0001); //测试用，开发者请获取实际的SSID
    ty_cJSON* wifi = ty_cJSON_CreateString(tmp_data);
    ty_cJSON_AddItemToObject(device_info, "WiFi_Name", wifi);
    //获取RSSI
    ty_cJSON* rssi = ty_cJSON_CreateNumber(50);   //测试用,开发者请获取实际的RSSI
    ty_cJSON_AddItemToObject(device_info, "RSSI", rssi);
    //获取IP
    memset(tmp_data, 0, sizeof(tmp_data));
    sprintf(tmp_data, "%d:%d:%d:%d", 192, 168, 0, 101); //测试用，开发者请获取实际的IP
    ty_cJSON* ip = ty_cJSON_CreateString(tmp_data);
    ty_cJSON_AddItemToObject(device_info, "IP", ip);
    //获取mac信息
    
    memset(tmp_data, 0, sizeof(tmp_data));
    sprintf(tmp_data, "%02X:%02X:%02X:%02X:%02X:%02X", 1, 2, 3, 4, 5, 6); //测试用,开发者请获取实际的mac
    ty_cJSON* mac = ty_cJSON_CreateString(tmp_data);
    ty_cJSON_AddItemToObject(device_info, "Mac", mac);
    //获取版本信息
    ty_cJSON* firmware = ty_cJSON_CreateString(TY_APP_VERSION);
    ty_cJSON_AddItemToObject(device_info, "Firmware_Version", firmware);
    //获取sn信息
    memset(tmp_data, 0, sizeof(tmp_data));
    sprintf(tmp_data, "test_sn_%04d", 0001); //测试用，开发者请获取实际的SN
    ty_cJSON* device_sn = ty_cJSON_CreateString(tmp_data);
    ty_cJSON_AddItemToObject(device_info, "Device_SN", device_sn);
    //获取uuid
    memset(tmp_data, 0, sizeof(tmp_data));
    sprintf(tmp_data, "test_uuid_%04d", 0001); //测试用,开发者请获取实际的uuid
    ty_cJSON* uuid = ty_cJSON_CreateString(tmp_data);  
    ty_cJSON_AddItemToObject(device_info, "Module_UUID", uuid);

    char* device_info_buf = ty_cJSON_Print(device_info); //获取cjon为字符串
    PR_DEBUG("device info :%s", device_info_buf);
    PR_DEBUG("device info len:%d", strlen(device_info_buf));
    ty_cJSON_Delete(device_info); //删除cjson
    ty_rvc_device_info_data_response(device_info_buf, strlen(device_info_buf));
    if (device_info_buf != NULL) {
        ty_cJSON_FreeBuffer(device_info_buf); //释放buff
        device_info_buf = NULL;
    }
    return OPRT_OK;
}

#if defined(MCS_CMD_SUPPORT) && (MCS_CMD_SUPPORT == 1)
/**
 * @brief 扫地机高级功能查询当前房间名称
 * @param  [*]
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET ty_mcs_current_room_info_upload(void)
{
    OPERATE_RET ret = OPRT_OK;
    MCS_ROOM_INFO_S room_info = { 0 };
    char room_name[19]  = { 0 };

    room_info.map_id = 1;     //根据设备当前的真实地图id填写
    room_info.curr_name_id = 1;    //根据设备当前的真实房间id填写
    room_info.curr_name.len = strlen("room_1");   //根据设备当前的真实房间名称填写
    memcpy(room_name, "room_1", room_info.curr_name.len);
    room_info.curr_name.name = room_name;
    //注意：当前房间信息需要在地图有变化的时候就主动上报一次，同时设备所在房间有变化的时候也上报一次。
    //地图重置后，也需要上报一次空的当前房间名称。
    ty_rvc_mcs_room_info_response(&room_info, 0); 
    return ret;
}
#endif
/**
 * @brief 扫地机高级功能设置回调函数指针
 */
OPERATE_RET rvc_advance_function_set(IN COMPLEX_CMD_E cmd, IN void* param)
{
    int i = 0, j = 0;
    int map_id = 0; //当前地图ID
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
        dp_set.virtual_wall_sets.virtual_wall_count = p_virtual_wall->num; //结构体数据转化，保存到全局结构体中，开发者按需求保存到flash中
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
        //开发者根据APP下发的virtual_wall_data虚拟墙参数，做业务处理后再给面板回复。
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
        //结构体数据转化，保存到全局结构体中，开发者按需求保存到flash中
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
        //开发者根据APP下发的restricted_zone禁区参数，做业务处理后再给面板回复。
        p_restricted_area->map_id = map_id;
        ty_rvc_restricted_area_data_response(p_restricted_area, 0); // 这里直接上报数据到云端，测试使用

    } break;
    case ROOM_CLEAN_SET: {
        ROOM_CLEAN_S* p_room_clean = (ROOM_CLEAN_S*)param;
        PR_DEBUG("room_clean num:%d", p_room_clean->num);
        //结构体数据转化，保存到全局结构体中，开发者按需求保存到flash中
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
        //开发者根据APP下发的room_clean_sets选区清扫参数，做业务处理后再给面板回复。
        p_room_clean->map_id = map_id;
        ty_rvc_room_clean_data_response(p_room_clean, 0); // 这里直接上报数据到云端，测试使用

    } break;
    case SPOT_CLEAN_SET: {
        SPOT_CLEAN_S* p_spot_clean = (SPOT_CLEAN_S*)param;
        PR_DEBUG("spot_clean num:%d", p_spot_clean->num);
        //结构体数据转化，保存到全局结构体中，开发者按需求保存到flash中
        dp_set.pose_sets.clean_times = p_spot_clean->param[0].clean_cnt;
        dp_set.pose_sets.points[0].vertices[0].x = p_spot_clean->points[0].x;
        dp_set.pose_sets.points[0].vertices[0].y = p_spot_clean->points[0].y;

        dp_set.pose_sets.raw_data_len = sizeof(ROBOT_WORK_POSE_EXTRA_S) - sizeof(RAW_DATA_S);
        memcpy(&dp_set.pose_sets.raw_data, &dp_set.pose_sets, dp_set.pose_sets.raw_data_len); // 将原始结构体数据保存起来
        //开发者根据APP下发的pose_clean_sets定点清扫参数，做业务处理后再给面板回复。
        p_spot_clean->map_id = map_id;
        ty_rvc_spot_clean_data_response(p_spot_clean, 0); // 这里直接上报数据到云端，测试使用

    } break;

    case ZONE_CLEAN_SET: {
        ZONE_CLEAN_S* p_zone_clean = (ZONE_CLEAN_S*)param;
        PR_DEBUG("p_zone_clean num:%d", p_zone_clean->num);
        //结构体数据转化，保存到全局结构体中，开发者按需求保存到flash中
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
        //开发者根据APP下发的area_clean_sets划区清扫参数，做业务处理后再给面板回复。
        p_zone_clean->map_id = map_id;
        ty_rvc_zone_clean_data_response(p_zone_clean, 0); // 这里直接上报数据到云端，测试使用

    } break;
    case ROOM_PROPERTY_SET: {
        ROOM_PROPERTY_S* p_room_property = (ROOM_PROPERTY_S*)param;
        PR_DEBUG("p_room_property num:%d", p_room_property->num);
        //结构体数据转化，保存到全局结构体中，开发者按需求保存到flash中
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
        //开发者根据AAP下发room_attr_sets房间属性参数，做业务处理后再给面板回复。
        p_room_property->map_id = map_id;
        ty_rvc_room_property_data_response(p_room_property, 0); // 这里直接上报数据到云端，测试使用

    } break;
    case SCHEDULE_SET: {
        SCHEDULE_S* p_schedule_sets = (SCHEDULE_S*)param;
        PR_DEBUG("p_schedule_sets num:%d", p_schedule_sets->num);
        //结构体数据转化，保存到全局结构体中，开发者按需求保存到flash中
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
            // 当前业务对所有房间统一设置清扫参数，故这里取房间id为0时的参数，后期如有需要再进行业务扩展
            dp_set.local_timer_sets.points[j].clean_mode = enum_index_get_from_string(p_schedule_sets->time_sets[j].mode[0].name, MODE_ENUM_TYPE);
            dp_set.local_timer_sets.points[j].fan_level = enum_index_get_from_string(p_schedule_sets->time_sets[j].param[0].suction.name, SUCTION_ENUM_TYPE);
            dp_set.local_timer_sets.points[j].water_level = enum_index_get_from_string(p_schedule_sets->time_sets[j].param[0].cistern.name, CISTERN_ENUM_TYPE);
            dp_set.local_timer_sets.points[j].sweep_count = p_schedule_sets->time_sets[j].param[0].clean_cnt;
        }
        dp_set.local_timer_sets.raw_data.len = sizeof(ROBOT_LOCAL_TIMER_SET_S) - sizeof(LOCAL_TIME_RAW_DATA_S);

        PR_DEBUG("local time raw data len:%d", dp_set.local_timer_sets.raw_data.len);
        memcpy(dp_set.local_timer_sets.raw_data.buf, &dp_set.local_timer_sets, dp_set.local_timer_sets.raw_data.len); // 将原始结构体数据保存起来
        //开发者根据APP下发的local_time_sets预约清扫参数，做业务处理后再给面板回复。
        p_schedule_sets->map_id = map_id;
        ty_rvc_schedule_data_response(p_schedule_sets, 0); // 这里直接上报数据到云端，测试使用

    } break;
    case QUIET_HOUR_SET: {
        QUIET_HOURS_S* p_quiet_hours = (QUIET_HOURS_S*)param;
        //结构体数据转化，保存到全局结构体中，开发者按需求保存到flash中
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
        //开发者根据APP下发的undisturb_set勿扰模式结构体的参数，做业务处理后再给面板回复。
        ty_rvc_quiet_hours_data_response(p_quiet_hours, 0); // 这里直接上报数据到云端，测试使用
    } break;
    case PART_DIVI_SET: {
        PART_DIVI_S* divide_set = (PART_DIVI_S*)param;
        //结构体数据转化，保存到全局结构体中，开发者按需求保存到flash中
        dp_set.division_sets.room_ids = divide_set->ids[0];
        for (int i = 0; i < 2; i++) {
            dp_set.division_sets.points[0].vertices[i].x = divide_set->divi_line[0].points[i].x;
            dp_set.division_sets.points[0].vertices[i].y = divide_set->divi_line[0].points[i].y;
        }
        //开发者根据APP下发的division_sets手动分区结构体的参数，做业务处理后再给面板回复。
        ty_rvc_part_division_data_response(divide_set, 0); // 这里直接上报数据到云端，测试使用
         //房间分割或者合并，改变了房间属性，需要上报当前房间名称
#if defined(MCS_CMD_SUPPORT) && (MCS_CMD_SUPPORT == 1)
        ty_mcs_current_room_info_upload();
#endif
    } break;
    case PART_MERGE_SET: {
        PART_MERGE_S* merge_set = (PART_MERGE_S*)param;
        PR_DEBUG("merge_set num:%d", merge_set->num);
        //结构体数据转化，保存到全局结构体中，开发者按需求保存到flash中
        dp_set.merge_sets.len = merge_set->num;
        for (i = 0; i < dp_set.merge_sets.len; i++) {
            dp_set.merge_sets.room_ids[i] = merge_set->ids[i];
        }
        //开发者根据APP下发的pmerge_set分区合并结构体的参数，做业务处理后再给面板回复。
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
}

/**
 * @brief 扫地机高级功能查询回调函数指针
 */
OPERATE_RET rvc_advance_function_query(IN COMPLEX_CMD_E cmd, IN void* param)
{
    OPERATE_RET ret = OPRT_OK;
    int map_id = 0; //当前地图ID
    switch (cmd) {
    case VIRTUAL_WALL_QUERY: {
        VIRTUAL_WALL_S test_virtual_wall;
        VIRTUAL_LINE_S virtual_line[COORDINATE_MAX_LEN] = { 0 };
        //将业务上的虚拟墙数据转化为SDK需要的格式
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
        ty_rvc_virtual_wall_data_response(&test_virtual_wall, 0); //调用SDK接口，上报云端

    } break;
    case RESTRICTED_AREA_QUERY: {
        RESTRICTED_AREA_S test_restrict_area;
        VIRTUAL_AREA_S restrict_area[COORDINATE_MAX_LEN];
        POINT_COOR_S point_array_test[COORDINATE_MAX_LEN][4] = { 0 };
        char name_array[COORDINATE_MAX_LEN][NAME_MAX_LEN] = { 0 };
        //将业务上的禁区数据转化为SDK需要的格式
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
            restrict_area[j].cur_name.name = name_array[j];     //注意房间名称不能使用NULL数据传入
        }

        test_restrict_area.restrict_zone = restrict_area;
        PR_DEBUG("p_restricted_area:%x,%x,%x", &test_restrict_area, test_restrict_area.restrict_zone);
        test_restrict_area.map_id = map_id;
        ty_rvc_restricted_area_data_response(&test_restrict_area, 0); //调用SDK接口，上报云端

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
    
        room_property_test.num = 4;   //测试用，测试的地图上是2个房间，开发者根据实际数量填写
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
            param[i].y_mop = 0;   //是否支持Y字托，开发者根据实际数量填写
    
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
            memcpy(room_name[i], "test_name", sizeof("test_name") - 1);   //根据实际填写,没有设置就为空
            name_attr[i].len = sizeof("test_name") - 1;
            name_attr[i].name = room_name[i];   //注意房间名称不能使用NULL数据传入
        }
        room_property_test.map_id = map_id;
        ty_rvc_room_property_data_response(&room_property_test, 0); //调用SDK接口，上报云端

    } break;
    case QUIET_HOUR_QUERY: {
        //去flash获取勿扰数据，并传递给下面数据
        QUIET_HOURS_S p_quiet_hours = { 0 };
        p_quiet_hours.active = dp_set.disturb_timer_sets.disturb_switch;
        p_quiet_hours.start_time.hour = dp_set.disturb_timer_sets.start_time[0];
        p_quiet_hours.start_time.min = dp_set.disturb_timer_sets.start_time[1];
        p_quiet_hours.end_time.hour = dp_set.disturb_timer_sets.end_time[0];
        p_quiet_hours.end_time.min = dp_set.disturb_timer_sets.end_time[1];
        p_quiet_hours.other_day = dp_set.disturb_timer_sets.end_day;
        ty_rvc_quiet_hours_data_response(&p_quiet_hours, 0); //调用SDK接口，上报云端

    } break;
    case SCHEDULE_QUERY: {
        //去flash获取时间数据，并传递给下面数据
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
                    // 当前业务对所有房间统一设置清扫参数，故这里取房间id为0时的参数，后期如有需要再进行业务扩展

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
        ty_rvc_schedule_data_response(&schedule_set, 0); //调用SDK接口，上报云端
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
        ty_rvc_voice_language_data_response(&p_voice_language_set, 0); //调用SDK接口，上报云端
    } break;
    case MAP_EMPTY_QUERY: {
        PR_DEBUG("map empty info query >>>>>");
        /*开发者自行判断本地的首页地图是否存在，如果不存在回复TRUE，存储就回复FALSE*/
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
}

/**
 * @brief  DP点服务的初始化
 * @return [*]
 */
void ty_cmd_dp_init(void)
{
    dp_handle_all_dp_load(); // DP点数据装载
    ty_rvc_complex_func_register(rvc_advance_function_set, rvc_advance_function_query);  //注册扫地机高级协议回调（如虚拟墙，房间属性之类）
    PR_DEBUG("ty_rvc_complex_func_register >>>>> ok");
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
    dp_handle_sync_to_cloud(); //数据打包上报
}
