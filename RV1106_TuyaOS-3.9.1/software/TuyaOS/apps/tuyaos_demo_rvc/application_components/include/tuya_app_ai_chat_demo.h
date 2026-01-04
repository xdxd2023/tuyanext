/*********************************************************************************
 *Copyright(C),2015-2020, TUYA www.tuya.comm
 *FileName:    tuya_app_ai_chat_demo
  *brief：AI command callback function, providing AI command types including speech recognition, 
  emotion analysis, motion control, etc.Alternative translation (more technical):
 **********************************************************************************/
#ifndef INCLUDE_TUYA_APP_AI_CHAT_DEMO_H_
#define INCLUDE_TUYA_APP_AI_CHAT_DEMO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "tuya_ipc_ai_station.h"

/**
 * @brief AI助手初始化
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tuya_app_init_AI(void);

#ifdef __cplusplus
}
#endif
#endif /*INCLUDE_TUYA_APP_AI_CHAT_DEMO_H_*/