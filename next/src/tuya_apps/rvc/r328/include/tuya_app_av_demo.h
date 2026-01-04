/*********************************************************************************
  *Copyright(C),2015-2020, TUYA www.tuya.comm
  *FileName:    tuya_app_av_demo
**********************************************************************************/
#ifndef INCLUDE_TUYA_APP_AV_DEMO_H_
#define INCLUDE_TUYA_APP_AV_DEMO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

/**
 * @brief  创建音视频塞流任务
 * @param
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tuya_av_stream_init(void);

#ifdef __cplusplus
}
#endif
#endif /*INCLUDE_TUYA_APP_AV_DEMO_H_*/