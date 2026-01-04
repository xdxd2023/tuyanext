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

#ifndef __BASE_EVENT_COMPATIBLE__
#define __BASE_EVENT_COMPATIBLE__

#include "base_event.h"

#pragma pack(1)

#pragma pack()

typedef EVENT_RAW_DATA_T event_raw_data_t;

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __BASE_EVENT_COMPATIBLE__

