/*********************************************************************************
  *Copyright(C),2015-2020, TUYA www.tuya.comm
  *FileName:    tuya_app_config
**********************************************************************************/
#ifndef INCLUDE_TUYA_APP_COMMON_H_
#define INCLUDE_TUYA_APP_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "uni_log.h"
#include "tuya_cloud_types.h"
#include "tuya_cloud_com_defs.h"
#include "ty_rvc_sdk_init.h"
#include "ty_rvc_api.h"
#include "ty_rvc_func.h"

#define TY_APP_STORAGE_PATH    "/tmp/"     //db文件信息存放到虚拟存储路径（真实设备需要开发者提供本地存储空间存放）
#define TY_SDK_ONLINE_LOG_PATH "/tmp/"     //在线日志信息存放到虚拟存储路径（真实设备需要开发者提供本地存储空间存放）
#define TY_TMP_VOICE_FILE_PATH "/tmp/"     //语音文件存放到虚拟存储路径（真实设备需要开发者提供本地存储空间存放）
#define TY_TMP_MAP_FILE_PATH   "/tmp/"     //地图文件存放到虚拟存储路径（真实设备需要开发者提供本地存储空间存放）
#define TY_APP_VERSION         "1.0.1"     //主机上报的版本
#define TY_MCU_VERSION         "1.0.0"     //mcu 上报的版本
#define TY_ROBOT_MEDIA_ENABLE  1           //打开音视频开关
#define TY_ROBOT_AI_OBJECT_ENABLE     0    //是否打开AI物体识别使能
#define RT_MAP_STRUCTURED_TRANS_ENABLE  1  /*开启地图高级版本传输模式*/
#define MCS_CMD_SUPPORT                 0  //支持MCS语音控制
#define TY_ROBOT_AI_CHAT_ENABLE         1  //是否打开AI Chat助手使能


#ifdef __cplusplus
}
#endif
#endif /*INCLUDE_TUYA_APP_COMMON_H_*/
