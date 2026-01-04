/***********************************************************
 *  File: ty_rvc_sdk_init.h
 *  Author: luotong
 *  Date: 20241023
 ***********************************************************/

#ifndef __TUYA_IOT_RVC_SDK_INIT_H__
#define __TUYA_IOT_RVC_SDK_INIT_H__

#include "cJSON.h"
#include "tuya_cloud_com_defs.h"
#include "tuya_cloud_types.h"
#include "tuya_cloud_wifi_defs.h"
#include "tuya_error_code.h"
#include "tuya_iot_config.h"
#include <stdio.h>
#include <time.h>
#if defined(ENABLE_IPC_GW_BOTH) && (ENABLE_IPC_GW_BOTH == 1)
#include "tuya_gw_init_api.h"
#endif

#include "tuya_ipc_media_stream.h"
#include "tuya_ipc_p2p.h"
#include "tuya_ipc_stream_storage.h"
#include "cloud_operation.h"
#ifdef __cplusplus
extern "C" {
#endif

#define WIFI_QRCODE_PIN         10      // max string length of wifi qrcode pin 
/**
 * Definition of Wi-Fi product info
 */
typedef struct {
    CHAR_T uuid[GW_UUID_LEN + 1];       /** uuid, assigned by TUYA */
    CHAR_T auth_key[AUTH_KEY_LEN + 1];  /** auth key */
#if defined(ENABLE_WIFI_SERVICE) && (ENABLE_WIFI_SERVICE==1)
    CHAR_T country_code[COUNTRY_CODE_LEN]; /** country code */
#endif
    CHAR_T pin[WIFI_QRCODE_PIN];        /** pin for wifi qrcode */
} RVC_WF_PROD_INFO_S;

/**
 * @brief audio and video stream treans cb
 */
typedef INT_T (*RVC_AV_STREAM_EVENT_CB)(IN CONST INT_T channel, IN CONST MEDIA_STREAM_EVENT_E event, IN PVOID_T args);

/**
 * @brief real time map and path treans cb
 */
typedef INT_T (*RVC_TRANS_EVENT_CB)(IN CONST int onoff);

/*
 * @brief 
 */
typedef INT_T (*TY_RVC_MF_DATA_EVENT_CB)(IN void* param);

/**
 * @brief svc media cb init param
 */
typedef struct
{
    BOOL_T enable; /*enable == false ,other var is invalid*/
    TRANS_DEFAULT_QUALITY_E live_mode; /* for multi-streaming, the default quality for live preview */
    MEDIA_STREAM_EVENT_CB media_event_cb; /*media event callback*/
    MEDIA_RECV_AUDIO_CB rev_audio_cb; /*app to dev audio callback*/
    MEDIA_RECV_VIDEO_CB rev_video_cb; /*app to dev audio callback*/
    MEDIA_RECV_FILE_CB rev_file_cb; /*app to dev file callback*/
    MEDIA_GET_SNAPSHOT_CB get_snapshot_cb;
} TY_SDK_MEDIA_ADAPTER_S;

/**
 * @brief av encode and decode param
 */
typedef struct
{
    IPC_MEDIA_INFO_T av_encode_info; /* encoder info */
    MEDIA_AUDIO_DECODE_INFO_T audio_decode_info; /* decoder info */
    INT_T max_pic_len; /* max picture size, in KB */
} DEVICE_SDK_MEDIA_INFO_S;

/***********************************************************
 *@function: ty_rvc_iot_init
 *@brief initialize tuya robot event
 *@param[*] VOID
 *@return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 ***********************************************************/
OPERATE_RET ty_rvc_iot_init(VOID);

/***********************************************************
 *@function: ty_rvc_media_av_event_init
 *@brief:  sdk audio and video registration callback int
 *@param  [in] event_cb: audio and video stream treans cb
 *@return [*]OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 ***********************************************************/
OPERATE_RET ty_rvc_media_av_event_init(RVC_AV_STREAM_EVENT_CB event_cb);

/***********************************************************
 *@function: ty_rvc_media_adapter_init
 *@brief initialize tuya sdk for media adapter
 *@param[in] p_media_adatper_info: TY_SDK_MEDIA_ADAPTER_S
 *@param[in] p_media_infos: DEVICE_SDK_MEDIA_INFO_S
 *@return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
***********************************************************/
OPERATE_RET ty_rvc_media_adapter_init(TY_SDK_MEDIA_ADAPTER_S* p_media_adatper_info, DEVICE_SDK_MEDIA_INFO_S* p_media_infos);

/***********************************************************
 *@function: ty_rvc_media_ring_buffer_init
 *@brief  initialize media ring buffer init
 *@param[in] pMediaInfo: DEVICE_SDK_MEDIA_INFO_S
 *@param[in] max_buffer_seconds: buffer timer 6~10S
 *@param[in] channel:  media channel
 *@return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 ***********************************************************/
OPERATE_RET ty_rvc_media_ring_buffer_init(CONST DEVICE_SDK_MEDIA_INFO_S* pMediaInfo, INT_T max_buffer_seconds, INT_T channel);

/***********************************************************
 *@Function: tuya_sdk_media_server_init
 *@brief  initialize media server init
 *@param[in] event_handler The status callback for real-time data transmission, 
             with statuses including: start and end.
 *@return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 ***********************************************************/
OPERATE_RET ty_rvc_server_init(RVC_TRANS_EVENT_CB event_handler);

/**
 * @brief 扫地机产测授权初始化接口
 *
 * @param[in] user_sw_ver 固件版本
 * @param[in] mf_info_cb 授权信息回调
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET ty_rvc_mf_init(IN CHAR_T *user_sw_ver, TY_RVC_MF_DATA_EVENT_CB mf_info_cb);

#ifdef __cplusplus
}
#endif

#endif
