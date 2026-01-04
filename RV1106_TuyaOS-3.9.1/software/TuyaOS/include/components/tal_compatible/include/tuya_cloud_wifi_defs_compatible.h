/**
 * @file base_event_compatible.h
 * @author beihe(JiangBo) (beihe.jiang@tuya.com)
 * @brief 适配base_event.h
 * @version 0.1
 * @date 2023-12-19
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef __TUYA_CLOUDE_WIFI_DEFS_COMPATIBLE__
#define __TUYA_CLOUDE_WIFI_DEFS_COMPATIBLE__

#include "tuya_iot_config.h"
#if defined(ENABLE_WIFI_SERVICE) && (ENABLE_WIFI_SERVICE==1)
#include "tuya_cloud_wifi_defs.h"

#define STAT_BT_ACTIVED         STAT_PROXY_ACTIVED
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __BASE_EVENT_COMPATIBLE__

