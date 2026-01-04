/*********************************************************************************
  *Copyright(C),2015-2020, TUYA www.tuya.comm
  *FileName:    tuya_app_download_demo
**********************************************************************************/
#ifndef INCLUDE_TUYA_APP_DOWNLOAD_DEMO_H_
#define INCLUDE_TUYA_APP_DOWNLOAD_DEMO_H_

#ifdef __cplusplus
extern "C" {
#endif	

#include <stdio.h>
#include "uni_log.h"
#include "tuya_cloud_types.h"
#include "tuya_cloud_com_defs.h"

/**
 * @brief  语音文件下载函数
 * @param
 * @return [*]
 */
OPERATE_RET robotics_svc_download_voice_file(void);

/**
 * @brief  地图文件下载函数
 * @param
 * @return [*]
 */
OPERATE_RET robotics_svc_download_map_file(void);

#ifdef __cplusplus
}
#endif
#endif /*INCLUDE_TUYA_APP_DOWNLOAD_DEMO_H_*/
