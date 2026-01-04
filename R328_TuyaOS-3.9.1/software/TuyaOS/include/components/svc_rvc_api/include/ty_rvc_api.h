/***********************************************************
 *  File: ty_rvc_api.h
 *  Author: luotong
 *  Date: 20241023
 ***********************************************************/

#ifndef __TY_RVC_API_H
#define __TY_RVC_API_H

#include "tuya_cloud_com_defs.h"
#include "tuya_cloud_types.h"
#include "tuya_ipc_media_stream_event.h"
#include "ty_rvc_func.h"


#ifdef __cplusplus
extern "C" {
#endif

#define MAX_M_MAP_INFO_NUM 5 // Maximum number of map list entries that can be retrieved

#define DESCRIPT_STR_LMT 64 // Length of the description information
#define BITMAP_FILE_STR_LMT 64 // Length of the map file name displayed in the App
#define DATAMAP_FILE_STR_LMT 64 // Length of the custom file name

#define MAX_RVC_URL_LEN (1024*2) //now sdk support the url max length of map download 
/**
 * @brief Structure for obtaining the cloud list
 */
typedef struct {
    UINT_T map_id; // Cloud map ID
    UINT_T time; // Report timestamp
    CHAR_T descript[DESCRIPT_STR_LMT]; // Description information
    CHAR_T bitmap_file[BITMAP_FILE_STR_LMT]; // Map file displayed in the App
    CHAR_T datamap_file[DATAMAP_FILE_STR_LMT]; // Custom file
} M_MAP_INFO;

/**
 * @brief Structure for coordinate points
 */
typedef struct {
    int x; // X value of the coordinate point
    int y; // Y value of the coordinate point
} OBJECT_POINT_S;

/**
 * @brief Names of recognized objects
 */
typedef enum {
    OBJECTS_TYPE_WIRE = 0, // Wire
    OBJECTS_TYPE_SHOES, // Shoes
    OBJECTS_TYPE_SOCKS, // Socks
    OBJECTS_TYPE_TOYS, // Toys
    OBJECTS_TYPE_CHAIR, // Chair
    OBJECTS_TYPE_TABLE, // Table
    OBJECTS_TYPE_TRASH_CAN, // Trash can
    OBJECTS_TYPE_POTTED_PLANT, // Potted plant
    OBJECTS_TYPE_OTHER, // Other
    OBJECTS_TYPE_MAX,
} AI_OBJECTS_TYPE_E;

/**
 * @brief thumbnail param structure
 */
typedef struct {
    OBJECT_POINT_S points; // Points of objects
    AI_OBJECTS_TYPE_E object_type; // Recognized objects
    unsigned char accuracy; // Recognition accuracy, report FF if not used
} OBJECT_THUMBNAILS_PARAM_S;

/**
 * @brief AI thumbnail param structure
 */
typedef struct {
    int map_id; // Map ID
    int object_num; // Number of objects
    OBJECT_THUMBNAILS_PARAM_S* param; // Points of objects,type and accuracy
} RVC_AI_OBJECT_THUMBNAILS_INFO_S;

/**
 * @brief Pixel Point Types
 */
typedef enum {
    RVC_PIXEL_BLANK=0,     //Free area,laser scanning points of unpartitioned rooms.
    RVC_PIXEL_OBS,       // Obstacle
    RVC_PIXEL_CARPET,   // Carpet Area
    RVC_PIXEL_UNKNOW,   // Unknown Area,the background area of the map rectangular canvas.
    RVC_PIXEL_MAX,      // Pad Data,no use
} RVC_PIXEL_TYPE_E;  


/**
 * @brief Parameters for the map protocol header of the vacuum cleaner
 */
typedef struct {
    unsigned short map_id;           // Map ID
    int status;                      // Map status, 0 indicates unstable, 1 indicates stable
    int height;                      // Height of the map in pixels
    int width;                       // Width of the map in pixels
    int resolution;                  // Map resolution (width/length of each grid), reported as original value, unit: centimeters 
    POINT_COOR_S origin;             // Machine origin X/Y coordinates, given relative to the screen coordinate system, reported as original value
    POINT_COOR_S charge_point;       // Charging station X/Y coordinates, given relative to the screen coordinate system, reported as original value
    int charge_angle;                // Orientation of the charging station
} RVC_MAP_HEAD_INFO_S;

/**
 * @brief Point in the corresponding coordinate system
 */
typedef struct {
    float x; // X-axis coordinate in the corresponding coordinate system
    float y; // Y-axis coordinate in the corresponding coordinate system
    float z; // Z-axis coordinate in the corresponding coordinate system
} RVC_POINT_S;

/**
 * @brief Polygon parameters
 */
typedef struct {
    RVC_POINT_S* points; // Polygon points
    int points_size; // Number of points
} ROOM_POLYGON_S;

/** 
 * @brief Area polygon parameters
 */
typedef struct {
    int id;                             // Area ID
    ROOM_POLYGON_S polygon;      // Polygon
} RVC_ROOM_POLYGON_S;

typedef enum {
    RVC_FULL_PATH_NORMAL_E = 0x01,  // Full path of normal mode
    RVC_FULL_PATH_COMPLEX_E,        // Full path of complex mode
    RVC_NAVIGATION_PATH_E,          // Navigtion path
}RVC_PATH_TYPE_E;

typedef enum {
    RVC_CLEAN_PATH_POINT_E = 0X00,  // the clean path point
    RVC_TRANSITION_PATH_POINT_E,    // the transition path point
    RVC_RECHARGE_PATH_POINT_E,      // the recharge path point
    RVC_MOPPING_PATH_POINT_E,       // the mopping path point
}RVC_PATH_POINT_TYPE_E;

typedef struct {
    RVC_PATH_POINT_TYPE_E type; // the path point type
    POINT_COOR_S coordinates;   // the path point coordinates
} PATH_POINT_S;

typedef struct {
    unsigned short path_id;     // the path id
    RVC_PATH_TYPE_E path_type;  // the path type
    int direction;              // the path direction, the range is 0~360
    int point_count;            // the point count of the path 
    PATH_POINT_S* path_points;  // the point data buffer of the path
} RVC_PATH_INFO_S;


/**
 * @brief map data structer
 */
typedef struct{
    RVC_MAP_HEAD_INFO_S header;     // the map header infomation
    unsigned char** map_buff;       // the map data buffer of the map,and the map buffer is a two dimensional array, the two dimensions represent the height and width of the map
    int room_polygons_count;        // the room count of the map
    RVC_ROOM_POLYGON_S* room_polygons_data;  // the room boundary contour coordinates data of the map
}RVC_MAP_DATA_S;

/**
 * @brief  The cleaning modes 
 */
typedef enum {
    RVC_CLEAN_MODE_SMART = 0,       // smart/auto clean
    RVC_CLEAN_MODE_SELECT_ROOM,     // select room clean
    RVC_CLEAN_MODE_POSE,            // pose clean
    RVC_CLEAN_MODE_ZONE,            // zone clean
    RVC_CLEAN_MODE_STAND_POSE,      // spot clean
    RVC_CLEAN_MODE_SMART_PART,      // smart part clean
    RVC_CLEAN_MODE_SMART_PART_AREA, // smart part area clean
    RVC_CLEAN_MODE_DEPTH_GLOBAL,    // deep clean
    RVC_CLEAN_MODE_EDGE,            // edge clean
} RVC_CLEAN_MODE_E;

/**
 * @brief The Cleaning work mode
 */
typedef enum {
    RVC_CLEAN_BOTH = 0,     // sweep and mop
    RVC_CLEAN_ONLY_SWEEP,   // sweep only
    RVC_CLEAN_ONLY_MOP,     // mop only
    RVC_CLEAN_ADAPTIVE,     // adaptive
    RVC_CLEAN_MODE_MAX
} RVC_WORK_MODE_E;

/**
 * @brief  clean end result of the record 
 */
typedef enum {
    RVC_CLEAN_RESULT_ERR = 0, // clean end of error
    RVC_CLEAN_RESULT_OK, // clean end of ok
    RVC_CLEAN_RESULT_MAX
} RVC_CLEAN_RESULT_E;

/**
 * @brief  cleaner start method
 */
typedef enum {
    RVC_START_METHOD_REMOTE = 0,    // start of the remote control
    RVC_START_METHOD_APP,           // start of the app with the mobile phone
    RVC_START_METHOD_RESERVED,      // start of the schdule timer
    RVC_START_METHOD_BUTTON,        // start of physical button
    RVC_START_METHOD_SELF,          // auto-start (secondary charging, etc.)
    RVC_START_METHOD_MAX,
} RVC_START_METHOD_E;

/**
 * @brief  clean data structer
 */
typedef struct {
    int total_area_size;      // coverage area, in square meters.
    int covered_time;         // coverage time, in minutes.
    int total_clean_count;    // the number of cleanings (can be used as cleaning record ID).
    TIME_T end_stamp;        // cleaning end time (UTC, in seconds)

    RVC_CLEAN_MODE_E clean_mode;      // the clean mode
    RVC_WORK_MODE_E work_mode;        // the sweep mode
    RVC_CLEAN_RESULT_E end_status;    // the clean end status
    RVC_START_METHOD_E start_method;  // the clean start method

    int all_clean_area_size;    // total cleaning area
    int all_clean_time;     // total cleaning time
    int all_clean_count;    // total cleaning count
    RVC_PATH_INFO_S path_info; // the clean path data
} RVC_CLEAN_DATA_S;

/**
 * @brief Structure for additional map information such as virtual walls and no-go zones.
 */
typedef struct {
    VIRTUAL_WALL_S virtual_wall;        // the virtual wall data of the map 
    RESTRICTED_AREA_S restricted_area;  // the restricted area data of the map
    ROOM_PROPERTY_S room_proper;        // the room property data of the map
} RVC_MAP_ADDITIONAL_DATA_S;

/**
 * @brief Callback for AI high-definition image data, ai_name is the AI high-definition image file name sent by the App
 */
typedef INT_T (*RVC_AI_DATA_LOAD_CB)(CHAR_T* ai_name);

/**
 * @brief Callback function for map file download, result is download result, map_id is the cloud map ID
 */
typedef OPERATE_RET (*RVC_MAP_LOAD_CB)(int result, int map_id);

/**
 * @brief Callback function for voice file download, result is download result, voice_id is the voice ID
 */
typedef OPERATE_RET (*RVC_VOICE_LOAD_CB)(int result, int voice_id);

/***********************************************************
 *@function: tuya_iot_map_upload_files
 *@brief:  sweeper function. upload floor map info
 *@param[in] bitmap: upload map file
 *@param[in] datamap: upload customize map file
 *@param[in] descript: upload file descript info
 *@param[out] map_id: cloud return map id
 *@return OPERATE_RET: 0 for success, other error codes indicate failure
 ***********************************************************/
OPERATE_RET tuya_iot_map_upload_files(IN CONST CHAR_T* bitmap_file, IN CONST CHAR_T* datamap_file, IN CONST CHAR_T* descript, OUT CONST UINT_T* map_id);

/***********************************************************
 *@function: tuya_iot_map_upload_files_v2
 *@brief:  rvc function. upload floor map info v2
 *@param[in] custom_datamap_file: customize map file
 *@param[in] local_map_id: local map id
 *@param[in] map_data: map data
 *@param[in] map_addition: additional map data(such as virtual walls and no-go zones)
 *@param[out] map_id: cloud return map id
 *@return OPERATE_RET: 0 for success, other error codes indicate failure
 ***********************************************************/
OPERATE_RET tuya_iot_map_files_upload_v2(IN CONST CHAR_T* custom_datamap_file, IN CONST INT_T local_map_id, IN CONST RVC_MAP_DATA_S* map_data, IN RVC_MAP_ADDITIONAL_DATA_S* map_addition, OUT UINT_T* map_id);

/***********************************************************
 *@function: tuya_iot_map_update_files
 *@brief:  sweeper function. update floor map info
 *@param[in] map_id: update map file to cloud map id
 *@param[in] bitmap: update map file
 *@param[in] datamap update customize map file
 *@return OPERATE_RET: 0 for success, other error codes indicate failure
 ***********************************************************/
OPERATE_RET tuya_iot_map_update_files(IN CONST UINT_T map_id, IN CONST CHAR_T* bitmap_file, IN CONST CHAR_T* data_map_file);

/***********************************************************
 *@function: tuya_iot_map_update_files_v2
 *@brief:  rvc function. update floor map info v2
 *@param[in] map_id: cloud map id
 *@param[in] custom_datamap_file: customize map file
 *@param[in] local_map_id: local map id
 *@param[in] map_data: map data
 *@param[in] map_addition: additional map data(such as virtual walls and no-go zones)
 *@return OPERATE_RET: 0 for success, other error codes indicate failure
 ***********************************************************/
OPERATE_RET tuya_iot_map_files_update_v2(IN CONST UINT_T map_id, IN CONST CHAR_T* custom_datamap_file, IN CONST INT_T local_map_id, IN RVC_MAP_DATA_S* map_data, IN RVC_MAP_ADDITIONAL_DATA_S* map_addition);

/***********************************************************
 *@function: tuya_iot_map_delete
 *@brief:  sweeper function. delete floor map files in cloud
 *@param[in] map_id: cloud map id
 *@return OPERATE_RET 0 for success, other error codes indicate failure
 ***********************************************************/
OPERATE_RET tuya_iot_map_delete(IN CONST UINT_T map_id);

/***********************************************************
 *@function: tuya_iot_get_map_files
 *@brief:  sweeper function. get floor map files
 *@param[in]  map_id: cloud map id
 *@param[in] map_path: map files under map_path path.
 *@return OPERATE_RET 0 for success, other error codes indicate failure
 ***********************************************************/
OPERATE_RET tuya_iot_get_map_files(IN CONST UINT_T map_id, IN CONST CHAR_T* map_path);

/***********************************************************
 *@function: ty_rvc_map_download_init
 * @brief Floor map download registration callback function for the rvc
 * @param[in] path: Path where the map file is stored
 * @param[in] file_name: map file name
 * @param[in] download_flag:Set to TRUE to allow downloads, set to FALSE to only retrieve the cloud map ID
 * @param[in] sets_handler: Callback function for settings
 * @return OPERATE_RET: 0 for success, other error codes indicate failure
 ***********************************************************/
OPERATE_RET ty_rvc_map_download_init(IN CHAR_T* path, IN CHAR_T* file_name, IN bool download_flag, IN RVC_MAP_LOAD_CB sets_handler);

/***********************************************************
 * @function: ty_rvc_voice_download_init
 * @brief Voice download registration callback function for the rvc
 * @param[in] path: Path where the voice file is stored
 * @param[in] file_name: voice file name
 * @param[in] sets_handler: Callback function for settings
 * @return OPERATE_RET: 0 for success, other error codes indicate failure
 ***********************************************************/
OPERATE_RET ty_rvc_voice_download_init(IN CHAR_T* path, IN CHAR_T* file_name, IN RVC_VOICE_LOAD_CB sets_handler);

/***********************************************************
 *@function: tuya_iot_get_all_maps_info
 *@brief:  sweeper function. get floor map files
 *@param[in]  info_len: info len Maximum 5
 *@param[out] map_info :get all map info (Maximum 5 sets of data)
 *@return OPERATE_RET 0 for success, other error codes indicate failure
 ***********************************************************/
OPERATE_RET tuya_iot_get_all_maps_info(OUT M_MAP_INFO* map_info, INOUT UINT8_T* info_len);

/***********************************************************
 *@function: tuya_iot_map_record_upload_buffer
 *@brief: Sweeper function to auto retrans upload record map info
 *@param[in] map_id:local map ID
 *@param[in] buffer: Buffer for sweeping record data
 *@param[in] len: Length of sweeping record data
 *@param[in] descript: Record description
 *@return OPERATE_RET: 0 for success, other error codes indicate failure
 ***********************************************************/
OPERATE_RET tuya_iot_map_record_upload_buffer(IN CONST INT_T map_id, IN CONST BYTE_T* buffer, IN CONST UINT_T len, IN CONST CHAR_T* descript);

/***********************************************************
 *@function: tuya_iot_map_record_upload_buffer_v2
 *@brief: Sweeper function to auto retrans upload record map info
 *@param[in] local_map_id:local map ID
 *@param[in] map_data: map data
 *@param[in] clean_data: clean data
 *@param[in] map_addition: additional map data(such as virtual walls and no-go zones)
 *@return OPERATE_RET: 0 for success, other error codes indicate failure
 ***********************************************************/
OPERATE_RET ty_rvc_map_record_upload_v2(IN CONST INT_T local_map_id, IN CONST RVC_MAP_DATA_S* map_data, IN RVC_CLEAN_DATA_S* clean_data, IN RVC_MAP_ADDITIONAL_DATA_S* map_addition);

/***********************************************************
 *@function: tuya_iot_map_record_upload_buffer_with_filename
 *@brief: sweeper function. auto retrans upload record map info
 *@param[in] map_id:local map ID
 *@param[in] buffer：Buffer for sweeping record data
 *@param[in] len：Length of sweeping record data
 *@param[in] cloud_file_name：customize cloud file name
 *@param[in] descript：Record description
 *@return OPERATE_RET 0 for success, other error codes indicate failure
 ***********************************************************/
OPERATE_RET tuya_iot_map_record_upload_buffer_with_filename(IN CONST INT_T map_id, IN CONST BYTE_T* buffer, IN CONST UINT_T len, IN CONST CHAR_T* cloud_file_name, IN CONST CHAR_T* descript);

/***********************************************************
 *@function: ty_rvc_rt_map_send_v1
 *@brief:  send realtime map data to app
 *@param[in] file_buff: realtime map buff
 *@param[in] file_len: realtime map len
 *@return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 ***********************************************************/

OPERATE_RET ty_rvc_rt_map_send_v1(IN CHAR_T* file_buff, IN INT_T file_len);

/***********************************************************
 *@function: ty_rvc_rt_map_send_v2
 *@brief:  Send real-time structured map data to the app
 *@param[in] map_info: map data, including fixed header data, pixel data, 
                       the number of room zones, and boundary grid coordinate point data.
 *@return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 ** *********************************************************/
OPERATE_RET ty_rvc_rt_map_send_v2(RVC_MAP_DATA_S* map_info);

/***********************************************************
 *@function: ty_rvc_rt_cleanpath_send_v1
 *@brief send realtime cleanpath data to app
 *@param[in] file_buff: realtime cleanpath buff
 *@param[in] file_len: realtime cleanpath len
 *@return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 ***********************************************************/
OPERATE_RET ty_rvc_rt_cleanpath_send_v1(IN CHAR_T* file_buff, IN INT_T file_len);

/***********************************************************
 *@function: ty_rvc_rt_cleanpath_send_v2
 *@brief send realtime structured cleanpath data to app
 *@param[in] path_info: realtime path data which is need to be upload
 *@return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 ***********************************************************/
OPERATE_RET ty_rvc_rt_cleanpath_send_v2(RVC_PATH_INFO_S* path_info);

/***********************************************************
 *@function: ty_rvc_rt_ai_thumbnails_send
 *@brief send realtime ai thumbnails to app
 *@param[in] object_info:ai object thumbnails param
 *@return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 ***********************************************************/
OPERATE_RET ty_rvc_rt_ai_thumbnails_send(IN RVC_AI_OBJECT_THUMBNAILS_INFO_S* object_info);

/***********************************************************
 *@function: ty_rvc_ai_original_init
 *@brief Callback function for get aiHD name
 *@param[in] sets_handler: set unpload aiHD name cb
 *@return OPERATE_RET: 0 for success, other error codes indicate failure
 ***********************************************************/
OPERATE_RET ty_rvc_ai_original_init(RVC_AI_DATA_LOAD_CB sets_handler);

/***********************************************************
 *@Function: ty_rvc_ai_original_send
 *@brief send ai origina to app
*@param[in] file_type: ai origina types include jpg,png,gif,svg,bmp,ic0
 *@param[in] file_buff: ai origina buff
 *@param[in] file_len: ai origina len
 *@return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 ***********************************************************/
OPERATE_RET ty_rvc_ai_original_send(IN CHAR_T* file_type, IN CHAR_T* file_buff, IN INT_T file_len);

#ifdef __cplusplus
}
#endif

#endif
