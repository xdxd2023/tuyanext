/***********************************************************
*  File: upload_ntification.h
*  Author: luotong
*  Date: 20241023
***********************************************************/
#ifndef UPLOAD_NOTIFICATION_RVC_H
#define UPLOAD_NOTIFICATION_RVC_H

#include "tuya_cloud_types.h"
#include "tuya_cloud_error_code.h"
#include "cloud_operation.h"
#include "upload_notification.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief  扫地机楼层地图上传的结构体
 */
typedef struct {
    CHAR_T file_name[32]; 
    CHAR_T cloud_file[128]; 
    INT_T len;
    CHAR_T* buff;
} RVC_FLOOR_MAP_INFO;

/**
 * @brief  sdk ability param
 */
typedef struct {
    UCHAR_T isMapEnable : 1; 
    UCHAR_T isVideo_Audio_Enable : 1;
    UCHAR_T isNav_Sdk_Enable : 1; 
    UCHAR_T isLrm_Channel_Enable : 1;
    UCHAR_T isPet_Detection_Enable : 1; 
} ROBOT_SDK_ABILITY_T;

/***********************************************************
*@Function: upload_map_files
*@brief:  rvc function. upload rvc map info
*@param[in] bitmap_file: upload map file
*@param[in] datamap_file: upload customize map file
*@param[in] extend_msg: upload file descript info
*@param[out] map_id: cloud return map id
*@return OPERATE_RET: 0 for success, other error codes indicate failure
***********************************************************/
OPERATE_RET upload_map_files(IN CONST CHAR_T *bitmap_file, IN CONST CHAR_T *datamap_file, IN CONST CHAR_T *extend_msg, OUT CONST UINT_T *map_id);
/***********************************************************
*@Function: upload_map_buffer_pack
*@brief:  rvc function. upload rvc map info
*@param[in] bitmap_info: upload map info
*@param[in] datamap_info: upload customize map info
*@param[in] extend_msg: upload file descript info
*@param[out] map_id: cloud return map id
*@return OPERATE_RET: 0 for success, other error codes indicate failure
***********************************************************/
OPERATE_RET upload_map_buffer_pack(IN CONST RVC_FLOOR_MAP_INFO *bitmap_info, IN CONST RVC_FLOOR_MAP_INFO *datamap_info, IN CONST CHAR_T *extend_msg, OUT CONST UINT_T *map_id);
/***********************************************************
*  Function: upload_map_buffer_subpack
*  Desc:  rvc function. upload rvc map from buffer by subpack
*  Input: bitmap_total_len  
*  Input: bitmap
*  Input: bitmap_len 
*  Input: datamap_total_len
*  Input: datamap
*  Input: datamap_len
*  Input: extend_msg
*  output: map_id 
*  Return: OPERATE_RET
***********************************************************/
OPERATE_RET upload_map_buffer_subpack(IN CONST UINT_T bitmap_total_len,
                                       IN CONST CHAR_T *bitmap,
                                       IN CONST UINT_T bitmap_len,
                                       IN CONST UINT_T datamap_total_len,
                                       IN CONST CHAR_T *datamap,
                                       IN CONST UINT_T datamap_len,
                                       IN CONST CHAR_T *extend_msg,
                                       OUT CONST UINT_T *map_id);

/***********************************************************
*@Function: update_map_files
*@brief:  rvc function. update rvc map info
*@param[in] map_id: updata map file to cloud map id 
*@param[in] bitmap_file: updata map file
*@param[in] data_map_file updata customize map file
*@param[in] cloud_bitmap_file:  get cloud bitmap file
*@param[in] cloud_data_map_file cloud customize map file
*@return OPERATE_RET: 0 for success, other error codes indicate failure
***********************************************************/
OPERATE_RET update_map_files(IN CONST UINT_T map_id, IN CONST CHAR_T *bitmap_file, IN CONST CHAR_T *data_map_file,
                             IN CONST CHAR_T *cloud_bitmap_file, IN CONST CHAR_T *cloud_data_map_file);

/***********************************************************
*@Function: update_map_buffer_pack
*@brief:  rvc function. update rvc map info
*@param[in] map_id: updata map file to cloud map id 
*@param[in] bitmap_info: updata map info
*@param[in] data_map_info updata customize map info
*@return OPERATE_RET: 0 for success, other error codes indicate failure
***********************************************************/                         
OPERATE_RET update_map_buffer_pack(IN CONST UINT_T map_id, IN CONST RVC_FLOOR_MAP_INFO *bitmap_info, IN CONST RVC_FLOOR_MAP_INFO *data_map_info);

/***********************************************************
*  Function: update_map_buffer_subpack
*  Desc:  rvc function. update rvc map from buffer by subpack
*  Input: map_id 
*  Input: bitmap_total_len 
*  Input: bitmap 
*  Input: bitmap_len 
*  Input: datamap_total_len 
*  Input: datamap
*  Input: datamap_len
*@return OPERATE_RET: 0 for success, other error codes indicate failure
***********************************************************/
OPERATE_RET update_map_buffer_subpack(IN CONST UINT_T map_id,
                                       IN CONST UINT_T bitmap_total_len,
                                       IN CONST CHAR_T *bitmap,
                                       IN CONST UINT_T bitmap_len,
                                       IN CONST UINT_T datamap_total_len,
                                       IN CONST CHAR_T *datamap,
                                       IN CONST UINT_T datamap_len);

/***********************************************************
*@Function: delete_map_files
*@brief:  rvc function. delete map files in cloud
*@param[in] map_id: cloud map id
*@return OPERATE_RET 0 for success, other error codes indicate failure
***********************************************************/
OPERATE_RET delete_map_files(IN CONST UINT_T map_id);

/***********************************************************
*@Function: get_map_file_list
*@brief:  rvc function. get map files
*@param[in]  id: cloud map id
*@param[in] map_url: map files download url.
*@return OPERATE_RET 0 for success, other error codes indicate failure
***********************************************************/
OPERATE_RET get_map_file_list(IN CONST UINT_T id, OUT CHAR_T *map_url);

/***********************************************************
*@Function: get_map_all_file_list
*@brief:  rvc function. get map files
*@param[out]  result: cloud map list
*@return OPERATE_RET 0 for success, other error codes indicate failure
***********************************************************/
OPERATE_RET get_map_all_file_list(OUT ty_cJSON **result);

/***********************************************************
*@Function: upload_file_v2
*@brief:  rvc function. upload log file
*@param[in] file: file data
*@param[in] cloud_file_name: upload file name
*@param[in] extend_msg: upload file descript info
*@return OPERATE_RET 0 for success, other error codes indicate failure
***********************************************************/
OPERATE_RET upload_file_v2(IN CONST CHAR_T *file, IN CONST CHAR_T *cloud_file_name, IN CONST CHAR_T *extend_msg);

/***********************************************************
*@Function: robot_device_sdk_ability_init
*@brief:  robot function. robot device ability init
*@return OPERATE_RET 0 for success, other error codes indicate failure
***********************************************************/
OPERATE_RET robot_device_sdk_ability_init(void);

/***********************************************************
*@Function: robot_sdk_ability_get
*@brief: robot function. get robot device ability
*@return OPERATE_RET 0 for success, other error codes indicate failure
***********************************************************/
OPERATE_RET robot_sdk_ability_get(void);

/***********************************************************
*@Function: robot_sdk_get_rt_map_enable
*@brief: 获取实时地图的高级能力值，只有在设备激活后调用才有效
*@return OPERATE_RET 0 for success, other error codes indicate failure
***********************************************************/
OPERATE_RET robot_sdk_get_rt_map_enable(void);

/***********************************************************
*@Function: robot_sdk_get_video_audio_enable
*@brief: 获取音视频的高级能力值，只有在设备激活后调用才有效K
*@return OPERATE_RET 0 for success, other error codes indicate failure
***********************************************************/
OPERATE_RET robot_sdk_get_video_audio_enable(void);

/***********************************************************
*@Function: robot_sdk_get_lrm_channel_enable
*@brief: 获取蓝牙大数据通道的高级能力值，只有在设备激活后调用才有效
*@return OPERATE_RET 0 for success, other error codes indicate failure
***********************************************************/
OPERATE_RET robot_sdk_get_lrm_channel_enable(void);

/***********************************************************
*@Function: robot_sdk_get_pet_detection_enable
*@brief: 获取宠物识别SDK的高级能力值，只有在设备激活后调用才有效
*@return OPERATE_RET 0 for success, other error codes indicate failure
***********************************************************/
OPERATE_RET robot_sdk_get_pet_detection_enable(void);

/***********************************************************
*@Function: robot_sdk_get_nav_enable
*@brief: 获取算法SDK的高级能力值，只有在设备激活后调用才有效
*@return OPERATE_RET 0 for success, other error codes indicate failure
***********************************************************/
OPERATE_RET robot_sdk_get_nav_enable(void);

#ifdef __cplusplus
}
#endif

#endif//UPLOAD_NOTIFICATION_RVC_H

