/**
 * @file ty_rvc_func.h
 * @brief The ty_rvc_func module is used to register RVC complex functions.
 * @author guantc
 * @version 1.0
 * @date 2024-11-14 19:40:00
 *
 * Copyright (c) tuya.inc 2014-2021.
 *
 */

#ifndef __TY_RVC_FUNC_H__
#define __TY_RVC_FUNC_H__

#include "tuya_cloud_com_defs.h"
#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************
************************macro define************************
***********************************************************/
#define RESERVE_VAL (-1)

/***********************************************************
***********************typedef define***********************
***********************************************************/
/**
 * @brief Complex function command set
 */
typedef enum {
    VIRTUAL_WALL_SET = 0, // Virtual wall setting
    VIRTUAL_WALL_QUERY, // Virtual wall query
    RESTRICTED_AREA_SET, // Restricted area setting
    RESTRICTED_AREA_QUERY, // Restricted area query
    ROOM_CLEAN_SET, // Selected area cleaning
    ROOM_CLEAN_QUERY, // Selected area query
    SPOT_CLEAN_SET, // Spot cleaning
    SPOT_CLEAN_QUERY, // Spot cleaning query
    ZONE_CLEAN_SET, // Zone cleaning
    ZONE_CLEAN_QUERY, // Zone cleaning query
    ROOM_PROPERTY_SET, // Room property setting
    ROOM_PROPERTY_QUERY, // Room property query
    SCHEDULE_SET, // Scheduled timing setting
    SCHEDULE_QUERY, // Scheduled timing query
    QUIET_HOUR_SET, // Do not disturb mode setting
    QUIET_HOUR_QUERY, // Do not disturb mode query
    PART_DIVI_SET, // Manual zone division
    PART_MERGE_SET, // Manual zone merging
    PART_DEFAULT_SET, // Restore default zone
    RESET_CURR_MAP_SET, // Clear current map
    SAVE_CURR_MAP_SET, // Save current map
    DELETE_CLOUD_MAP_SET, // Delete cloud map
    VOICE_LANGUAGE_QUERY, // Voice information query
    DEV_INFO_QUERY, // Device information query
    AI_OBJECT_QUERY, // AI object recognition query
    AI_OBJECT_TYPE_SET, // Modify object type
    AI_OBJECT_IGNORE_SET, // Ignore obstacles
    MAP_EMPTY_QUERY,      // Map empty statue query
    PASSWORD_STATE_QUERY, // Video password state query
    PASSWORD_CHECK,     // Video password check
    PASSWORD_SET,       // Video password set
    MCS_ROOM_INFO_QUERY, //MCS room info query
    COMPLEX_CMD_MAX, // Padding, meaningless
} COMPLEX_CMD_E;

/**
 * @brief Map partition division result
 */
typedef enum {
    PART_DIV_SUC = 0x00, // Division successful
    PART_DIV_FAILED, // Division failed
    PART_DIV_NUM_LIMIT, // Reached partition number limit, division failed
    PART_DIV_AREA_SMALL, // Division area too small, division failed
    PART_DIV_LINE_RANGE_OUT, // Division line out of range, division failed
} PART_DIV_ST_E;

/**
 * @brief Map partition merge result
 */
typedef enum {
    PART_MERGE_SUC = 0x00, // Merge successful
    PART_MERGE_FAILED, // Merge failed
    PART_MERGE_ROOM_NOT_NEIGHBOR, // Merging rooms are not adjacent, merge failed
    PART_MERGE_AREA_BIG, // Area too large after merging, merge failed
} PART_MERGE_ST_E;

/**
 * @brief Map partition restore default result
 */
typedef enum {
    PART_RESET_SUC = 0x00, // Restore successful
    PART_RESET_FAILED, // Restore failed
} PART_RESET_ST_E;

/**
 * @brief Home map reset result
 */
typedef enum {
    MAP_RESET_SUC = 0x00, // Reset successful
    MAP_RESET_FAILED, // Reset failed
} MAP_RESET_ST_E;

/**
 * @brief Map save result
 */
typedef enum {
    MAP_SAVE_SUC = 0x00, // Save successful
    MAP_SAVE_FAILED, // Save failed
    MAP_SAVE_SPACE_FULL, // Space is full, save failed
} MAP_SAVE_ST_E;

/**
 * @brief Map delete result
 */
typedef enum {
    MAP_DELETE_SUC = 0x00, // Delete successful
    MAP_DELETE_FAILED, // Delete failed
} MAP_DELETE_ST_E;

/**
 * @brief Map use result
 */
typedef enum {
    MAP_USE_SUC = 0x00, // Map use successful
    MAP_USE_FAILED, // Map use failed
    MAP_DOWNLOAD_FAILED, // Map download failed
} MAP_USE_ST_E;

/**
 * @brief Voice use result
 */
typedef enum {
    VOICE_USE_SUC = 0x00, // Voice use successful
    VOICE_USE_FAILED, // Voice use failed
    VOICE_DOWNLOAD_FAILED, // Voice download failed
} VOICE_USE_ST_E;

/**
 * @brief Voice download status
 */
typedef enum {
    RVC_DOWNLOAD_ST_FAILED = 0, // Voice download error
    RVC_DOWNLOAD_ST_LOADING, // Voice downloading
    RVC_DOWNLOAD_ST_SUCC, // Voice download successful
    RVC_DOWNLOAD_ST_USING // Voice in use
} VOICE_DOWNLOAD_ST_E;

/**
 * @brief Forbidden modes
 */
typedef enum {
    FORBIT_ALL = 0, // All prohibited
    FORBIT_SWEEP, // Prohibit mopping
    FORBIT_MOP, // Prohibit sweeping
    FORBIT_RESERVE, // Not set, reserved
} FORBIT_MODE_E;

typedef enum {
    MAP_SAVE_LOCAL = 0, // Save map locally
    MAP_SAVE_CLOUD, // Save map to cloud
    MAP_SAVE_MAX,
} MAP_SAVE_MODE_E;

typedef struct {
    int len; // string length
    char* name; // string memory points
} STR_ELEMENT_S;

/* @brief
 */
typedef struct {
    int num; // string num
    STR_ELEMENT_S* cur_name; // string elements
} STR_ARRAY_S;

/**
 * @brief Coordinate point structure
 */
typedef struct {
    int x; // X value of the coordinate point
    int y; // Y value of the coordinate point
} POINT_COOR_S;

/**
 * @brief Virtual line coordinate structure
 */
typedef struct {
    FORBIT_MODE_E mode; // Forbidden mode
    POINT_COOR_S points[2]; // Endpoints of the line, first element is the starting point of the virtual wall, second element is the ending point of the virtual wall
} VIRTUAL_LINE_S;

/**
 * @brief Division line coordinate structure
 */
typedef struct {
    POINT_COOR_S points[2]; // Endpoints of the line
} DIVIDE_LINE_S;

/**
 * @brief Virtual wall structure
 */
typedef struct {
    int num; // Number of virtual walls
    int map_id; // Current command operation corresponding map ID, when the device reports, it's the map ID where the device currently is, the panel uses this field to confirm whether the reported parameters and the parameters set are on the same map
    VIRTUAL_LINE_S* line; // Virtual wall coordinate points and forbidden mode
} VIRTUAL_WALL_S;

/**
 * @brief Area coordinate point structure
 */
typedef struct {
    int point_num; // Number of endpoints forming the area
    POINT_COOR_S* point; // Endpoint coordinates forming the restricted area
} AREA_S;

/**
 * @brief Virtual area structure
 */
typedef struct {
    FORBIT_MODE_E mode; // Forbidden mode
    AREA_S area; // Endpoint coordinates of the restricted area
    STR_ELEMENT_S cur_name; // Name of the restricted area
} VIRTUAL_AREA_S;

/**
 * @brief Restricted area structure
 */
typedef struct {
    int num; // Number of restricted areas
    int map_id; // Current command operation corresponding map ID, when the device reports, it's the map ID where the device currently is, the panel uses this field to confirm whether the reported parameters and the parameters set are on the same map
    VIRTUAL_AREA_S* restrict_zone; // Coordinates and forbidden mode of the restricted area
} RESTRICTED_AREA_S;

/**
 * @brief Cleaning parameters
 */
typedef struct {
    int valid; // Whether the parameter is valid
    STR_ELEMENT_S suction; // Suction power setting, specific values should be agreed upon with the project
    STR_ELEMENT_S cistern; // Water volume setting, specific values should be agreed upon with the project
    bool y_mop; // Y-shaped mopping, TRUE for Y-shaped mopping to take effect
    int clean_cnt; // Number of room cleanings
} CLEAN_PARAM_S;

/**
 * @brief Selected area cleaning structure
 */
typedef struct {
    int num; // Number of selected areas
    int map_id; // Current command operation corresponding map ID, when the device reports, it's the map ID where the device currently is, the panel uses this field to confirm whether the reported parameters and the parameters set are on the same map
    int* ids; // List of IDs for the selected area rooms
    STR_ELEMENT_S* sweep_mode; // sweep mode for the selected area rooms
    CLEAN_PARAM_S* param; // Cleaning parameters for selected areas, each room corresponds to one cleaning parameter, param[0] corresponds to ids[0] area, if not set valid is 0
} ROOM_CLEAN_S;

/**
 * @brief Spot cleaning structure
 */
typedef struct {
    int num; // Number of spot cleanings
    int map_id; // Current command operation corresponding map ID, when the device reports, it's the map ID where the device currently is, the panel uses this field to confirm whether the reported parameters and the parameters set are on the same map
    POINT_COOR_S* points; // List of spot coordinates, each spot area corresponds to one points element
    STR_ELEMENT_S* mode; // Cleaning mode for the zone, each area corresponds to one cleaning mode, mode[0] corresponds to point[0] area
    CLEAN_PARAM_S* param; // Cleaning parameters for selected areas, param[0] corresponds to point[0] area, if not set valid is 0
} SPOT_CLEAN_S;

/**
 * @brief Zone parameters structure
 */
typedef struct {
    STR_ELEMENT_S mode; // Cleaning mode
    CLEAN_PARAM_S param; // Cleaning parameters
    AREA_S area; // Area coordinates
    STR_ELEMENT_S cur_name; // Area name
} ZONE_AREA_PARAM_S;

/**
 * @brief Zone cleaning structure
 */
typedef struct {
    int num; // Number of zones
    int map_id; // Current command operation corresponding map ID, when the device reports, it's the map ID where the device currently is, the panel uses this field to confirm whether the reported parameters and the parameters set are on the same map
    ZONE_AREA_PARAM_S* clean_zone; // Cleaning parameters for zone cleaning, each zone corresponds to one cleaning parameter
} ZONE_CLEAN_S;

/**
 * @brief Manual partition structure
 */
typedef struct {
    int num; // Number of partitions to be divided
    int map_id; // Current command operation corresponding map ID, when the device reports, it's the map ID where the device currently is, the panel uses this field to confirm whether the reported parameters and the parameters set are on the same map
    int* ids; // List of IDs to be divided
    DIVIDE_LINE_S* divi_line; // Partition division line coordinates, each room ID corresponds to one division line, divi_line[0] corresponds to ids[0]
} PART_DIVI_S;

/**
 * @brief Manual area merge structure
 */
typedef struct {
    int num; // Number of partitions to be merged
    int map_id; // Current command operation corresponding map ID, when the device reports, it's the map ID where the device currently is, the panel uses this field to confirm whether the reported parameters and the parameters set are on the same map
    int* ids; // List of IDs to be merged
} PART_MERGE_S;

/**
 * @brief Partition restore default structure
 */
typedef struct {
    int map_id; // Current command operation corresponding map ID, when the device reports, it's the map ID where the device currently is, the panel uses this field to confirm whether the reported parameters and the parameters set are on the same map
} MAP_PART_RESET_S;

/**
 * @brief Home map reset structure, no parameters, used to inform the application of the command's corresponding parameters
 */
typedef struct {

} CURRENT_MAP_RESET_S;

/**
 * @brief Map save structure
 */
typedef struct {
    MAP_SAVE_MODE_E save_mode; // Save mode
} MAP_SAVE_S;

/**
 * @brief Room property structure
 */
typedef struct {
    int num; // Number of rooms
    int map_id; // Current command operation corresponding map ID, when the device reports, it's the map ID where the device currently is, the panel uses this field to confirm whether the reported parameters and the parameters set are on the same map
    int* ids; // Room ID numbers, each room corresponds to one ID number
	STR_ELEMENT_S* sweep_mode; // room sweep mode,each room corresponds to one sweep mode
    CLEAN_PARAM_S* param; // Cleaning parameters for rooms, each room corresponds to one cleaning setting parameter, param[0] corresponds to ids[0] area
    int* orders; // Cleaning order for rooms, each room corresponds to one cleaning order, cleaning order is represented by non-zero Arabic numerals, smaller values indicate higher cleaning priority, zero represents the lowest priority
    STR_ELEMENT_S* cus_name; // Room naming list, each room corresponds to a name, cus_name[0] corresponds to id[0]
    int* floor_type; // Room floor types,each room corresponds to one floor type
} ROOM_PROPERTY_S;

/**
 * @brief Scheduled time structure
 */
typedef struct {
    char hour; // Scheduled hour value
    char min; // Scheduled minute value
} SCHEDULE_TIME_S;

/**
 * @brief Scheduled information structure
 */
typedef struct {
    unsigned char active; // Scheduled switch, 0 for off, 1 for on
    unsigned char cycle; // Whether to loop the schedule, value 0 indicates a one-time schedule, non-zero indicates a looping schedule, cycle's bit0~bit6 correspond to Monday~Sunday
    SCHEDULE_TIME_S start_time; // Scheduled effective time
    int num; // Number of rooms corresponding to each schedule
    bool customed_switch; //the switch of the clean param,when it is true,the clean param of the app send is unvalid
    int* ids; // Room ID categories corresponding to each schedule
    STR_ELEMENT_S* mode; // List of cleaning modes, mode[0] corresponds to ids[0]
    CLEAN_PARAM_S* param; // List of cleaning parameters, param[0] corresponds to ids[0]
} SCHEDULE_PARAM_S;

/**
 * @brief Local schedule structure
 */
typedef struct {
    int num; // Number of schedules
    int map_id; // Current command operation corresponding map ID, when the device reports, it's the map ID where the device currently is, the panel uses this field to confirm whether the reported parameters and the parameters set are on the same map
    SCHEDULE_PARAM_S* time_sets; // Parameter categories for each schedule
} SCHEDULE_S;

/**
 * @brief Do not disturb settings structure
 */
typedef struct {
    unsigned char active; // Do not disturb switch, 0 for off, 1 for on
    unsigned char other_day; // Effective time, 0 for today, 1 for the next day
    SCHEDULE_TIME_S start_time; // Do not disturb start time
    SCHEDULE_TIME_S end_time; // Do not disturb end time
} QUIET_HOURS_S;

typedef struct {
    STR_ELEMENT_S password;
}PASSWORD_CHECK_S;

typedef struct {
    STR_ELEMENT_S password;
    STR_ELEMENT_S old_password;
}PASSWORD_SET_S;
/**
 * @brief Delete cloud map structure
 */
typedef struct {
    unsigned int map_id; // Current command operation corresponding map ID, when the device reports, it's the map ID where the device currently is, the panel uses this field to confirm whether the reported parameters and the parameters set are on the same map
} DELETE_CLOUD_MAP_S;

/**
 * @brief Use map structure
 */
typedef struct {
    unsigned int id; // Map ID number
    unsigned int url_len; // Length of the map file URL
    char* url; // URL address of the map file
} USE_CLOUD_MAP_S;

/**
 * @brief Use map response structure
 */
typedef struct {
    unsigned int id; // Map ID number
} USE_CLOUD_MAP_RESPONSE_S;

/**
 * @brief Use voice structure
 */
typedef struct {
    unsigned int id; // Voice ID number
    unsigned int url_len; // Length of the voice file URL address
    unsigned int md5_len; // Length of the MD5 checksum of the voice file
    char* url; // URL address of the voice file
    char* md5; // MD5 checksum of the voice file
} USE_VOICE_LANGUAGE_S;

/**
 * @brief Use voice response structure
 */
typedef struct {
    unsigned int id; // Voice ID number
    VOICE_DOWNLOAD_ST_E download_status; // Voice download status
    int percent;  // Voice download progress percentage,range is 0~100
} USE_VOICE_LANGUAGE_RESPONSE_S; 

/**
 * @brief  MCS Room info structure
 */
typedef struct {
    unsigned int map_id;  //Current command operation corresponding map ID, when the device reports, it's the map ID where the device currently is, the panel uses this field to confirm whether the reported parameters and the parameters set are on the same map
    unsigned int curr_name_id; //The current room ID where the machine is located.
    STR_ELEMENT_S curr_name;   //The current room name where the machine is located.
} MCS_ROOM_INFO_S;
/***********************************************************
********************function declaration********************
***********************************************************/

/**
 * @brief Sweeper complex function setting callback function pointer
 * @param [COMPLEX_CMD_E] cmd, protocol command, the SDK uses this parameter to notify the application of the current setting command to be processed, see the list for setting commands, all ending with SET
 * @param [void *] param, parameters corresponding to the command, the application converts the param parameter into the command corresponding parameter structure based on the cmd command field, the structure correspondence can refer to the accompanying demo
 */
typedef OPERATE_RET (*FUNC_SET_CB)(IN COMPLEX_CMD_E cmd, IN void* param);

/**
 * @brief Sweeper complex function query callback function pointer
 * @param [COMPLEX_CMD_E] cmd, protocol command, the SDK uses this parameter to notify the application of the current query command to be processed, see the list for query commands, all ending with QUERY
 * @param [void *] param, parameters corresponding to the command, current query command, its parameters are meaningless and do not need to be processed
 * @note  When the application gets the corresponding command parameters in the query callback function and needs to report, please call the corresponding response reporting function, for example, after processing the VIRTUAL_WALL_QUERY command, when needing to report parameters, use the ty_rvc_virtual_wall_data_response interface to report
 */
typedef OPERATE_RET (*FUNC_QUERY_CB)(IN COMPLEX_CMD_E cmd, IN void* param);

/**
 * @brief Register complex capability setting and query callback functions
 * @param[in] sets_handler: Command setting callback function that the application needs to implement
 * @param[in] query_handler: Command query callback function that the application needs to implement
 * @return OPERATE_RET: 0 success, other error codes indicate failure
 */
OPERATE_RET ty_rvc_complex_func_register(IN FUNC_SET_CB sets_handler, IN FUNC_QUERY_CB query_handler);

/**
 * @brief Report virtual wall information
 * @param[in] p_virtual_wall: Virtual wall data, passed in by the application, parameters are defined in the structure
 * @param[in] errcode: Command execution result, 0 for success, other values for failure
 * @return OPERATE_RET: 0 success, other error codes indicate failure
 */
OPERATE_RET ty_rvc_virtual_wall_data_response(VIRTUAL_WALL_S* p_virtual_wall, int errcode);

/**
 * @brief Report restricted area data
 * @param[in] p_restricted_area: Restricted area data input parameter, passed in by the application, parameters are defined in the structure
 * @param[in] errcode: Execution result corresponding to the parameter setting, 0 for success, other values for failure
 * @return OPERATE_RET: 0 success, other error codes indicate failure
 */
OPERATE_RET ty_rvc_restricted_area_data_response(RESTRICTED_AREA_S* p_restricted_area, int errcode);

/**
 * @brief Report room property data
 * @param[in] p_room_clean: Room property data, passed in by the application, parameters are defined in the structure
 * @param[in] errcode: Execution result corresponding to the parameter setting, 0 for success, other values for failure
 * @return OPERATE_RET: 0 success, other error codes indicate failure
 */
OPERATE_RET ty_rvc_room_property_data_response(ROOM_PROPERTY_S* p_room_clean, int errcode);

/**
 * @brief Report selected area cleaning parameter values
 * @param[in] p_room_clean: Cleaning parameter input parameter, passed in by the application, parameters are defined in the structure
 * @param[in] errcode: Execution error code, no error, input parameter is 0
 * @return OPERATE_RET: 0 success, other error codes indicate failure
 */
OPERATE_RET ty_rvc_room_clean_data_response(ROOM_CLEAN_S* p_room_clean, int errcode);

/**
 * @brief Report spot cleaning data
 * @param[in] spot_clean_data: Spot cleaning data input parameter, passed in by the application, parameters are defined in the structure
 * @param[in] errcode: Execution error code, input 0 when no error
 * @return OPERATE_RET: 0 success, other error codes indicate failure
 */
OPERATE_RET ty_rvc_spot_clean_data_response(SPOT_CLEAN_S* spot_clean_data, int errcode);

/**
 * @brief Report zone cleaning data
 * @param[in] p_zone_area: Zone cleaning data input parameter, passed in by the application, parameters are defined in the structure
 * @param[in] errcode: Execution error code, input 0 when no error
 * @return OPERATE_RET: 0 success, other error codes indicate failure
 */
OPERATE_RET ty_rvc_zone_clean_data_response(ZONE_CLEAN_S* p_zone_area, int errcode);

/**
 * @brief Partition setting reply
 * @param[in] p_part_division: Partition parameters, passed in by the application, parameters are defined in the structure
 * @param[in] errcode: Execution error code, input 0 when no error
 * @return OPERATE_RET: 0 success, other error codes indicate failure
 */
OPERATE_RET ty_rvc_part_division_data_response(PART_DIVI_S* p_part_division, PART_DIV_ST_E errcode);

/**
 * @brief Partition merge command reply
 * @param[in] p_part_merge: Partition setting reply parameters, passed in by the application, parameters are defined in the structure
 * @param[in] errcode: Execution error code, input 0 when no error
 * @return OPERATE_RET: 0 success, other error codes indicate failure
 */
OPERATE_RET ty_rvc_part_merge_data_response(PART_MERGE_S* p_part_merge, PART_MERGE_ST_E errcode);

/**
 * @brief Partition restore default command response
 * @param[in] p_part_merge: Structure parameters, passed in by the application, parameters are defined in the structure
 * @param[in] errcode: Execution error code
 * @return OPERATE_RET: 0 success, other error codes indicate failure
 */
OPERATE_RET ty_rvc_map_part_default_data_response(MAP_PART_RESET_S* p_part_merge, PART_RESET_ST_E errcode);

/**
 * @brief Clear home map reply
 * @param[in] p_map_reset: Map reset structure, passed in by the application, parameters are defined in the structure
 * @param[in] errcode: Execution error code
 * @return OPERATE_RET: 0 success, other error codes indicate failure
 */
OPERATE_RET ty_rvc_map_clear_data_response(CURRENT_MAP_RESET_S* p_map_reset, MAP_RESET_ST_E errcode);

/**
 * @brief Map save command reply
 * @param[in] p_map_save: Map save reply parameter input, passed in by the application, parameters are defined in the structure
 * @param[in] errcode: Error code
 * @return OPERATE_RET: 0 success, other error codes indicate failure
 */
OPERATE_RET ty_rvc_map_save_data_response(MAP_SAVE_S* p_map_save, MAP_SAVE_ST_E errcode);

/**
 * @brief Map delete command reply
 * @param[in] p_map_delete: Map delete reply parameters, passed in by the application, parameters are defined in the structure
 * @param[in] errcode: Error code
 * @return OPERATE_RET: 0 success, other error codes indicate failure
 */
OPERATE_RET ty_rvc_map_delete_data_response(DELETE_CLOUD_MAP_S* p_map_delete, MAP_DELETE_ST_E errcode);

/**
 * @brief Use map command reply
 * @param[in] p_map_use_res: Use map reply parameters, passed in by the application, parameters are defined in the structure
 * @param[in] errcode: Error code
 * @return OPERATE_RET: 0 success, other error codes indicate failure
 */
OPERATE_RET ty_rvc_map_use_data_response(USE_CLOUD_MAP_RESPONSE_S* p_map_use_res, MAP_USE_ST_E errcode);

/**
 * @brief Scheduled data report
 * @param[in] p_schedule: Scheduled data, passed in by the application, parameters are defined in the structure
 * @param[in] current_map_id:
 * @param[in] errcode:
 * @return OPERATE_RET: 0 success, other error codes indicate failure
 */
OPERATE_RET ty_rvc_schedule_data_response(SCHEDULE_S* p_schedule, int errcode);

/**
 * @brief Do not disturb scheduled data report
 * @param[in] p_quiet_hours: Do not disturb scheduled data input, passed in by the application, parameters are defined in the structure
 * @param[in] errcode: Error code
 * @return OPERATE_RET: 0 success, other error codes indicate failure
 */
OPERATE_RET ty_rvc_quiet_hours_data_response(QUIET_HOURS_S* p_quiet_hours, int errcode);

/**
 * @brief Voice use parameter report
 * @param[in] p_voice_language_res: Voice use parameter report, passed in by the application, parameters are defined in the structure
 * @param[in] errcode: Execution error code
 * @return OPERATE_RET: 0 success, other error codes indicate failure
 */
OPERATE_RET ty_rvc_voice_language_data_response(USE_VOICE_LANGUAGE_RESPONSE_S* p_voice_language_res, VOICE_USE_ST_E errcode);

/**
 * @brief Device information report, info field is concatenated by the customer in key:value format in JSON, when the app panel displays device information, it shows each value in the order of the key list
 * The panel needs to correctly configure multilingual support for each key value
 * This interface is responsible for completing data reporting via the MQTT channel
 *
 * @param[char*] info: Application's JSON format string
 * @param [int] len: String length
 * @return OPERATE_RET, OPRT_OK indicates success, other error codes indicate failure
 */
OPERATE_RET ty_rvc_device_info_data_response(char* info, int len);

/**
 * @brief Clear the current map report
 * @param[in] empty_flag: true: the current map is empty, false: the current map is not empty
 * @param[in] errcode: Execution error code
 * @return OPERATE_RET: on success, or an error code on failure
 */
OPERATE_RET ty_rvc_map_empty_response(IN bool empty_flag, int errcode);

/**
 * @brief Video password check report
 * @param[in] state: true:password is exist, false:password is unexist
 * @param[in] errcode: Execution error code
 * @return OPERATE_RET: on success, or an error code on failure
 */
OPERATE_RET ty_rvc_password_check_response(IN bool state, int errcode);

/**
 * @brief Video password state query report
 * @param[in] state: true:password is exist, false:password is unexist
 * @param[in] errcode: Execution error code
 * @return OPERATE_RET: on success, or an error code on failure
 */
OPERATE_RET ty_rvc_password_state_response(IN bool state, int errcode);

/**
 * @brief Video password setting report
 * @param[in] state: true:password is exist, false:password not set
 * @param[in] errcode: Execution error code
 * @return OPERATE_RET: on success, or an error code on failure
 */
OPERATE_RET ty_rvc_password_set_response(IN bool state, int errcode);

/**
 * @brief Report dev room info
 * @param[in] room_info: Room information parameters, passed in by the application, are defined in the structure.
 * @param[in] errcode: Execution result corresponding to the parameter setting, 0 for success, other values for failure
 * @return OPERATE_RET: 0 success, other error codes indicate failure
 */
OPERATE_RET ty_rvc_mcs_room_info_response(MCS_ROOM_INFO_S* room_info, int errcode);

#ifdef __cplusplus
}
#endif

#endif /* __TY_RVC_FUNC_H__ */
