/**
 * @file app_agent.h
 * @author beihe(JiangBo) (beihe.jiang@tuya.com)
 * @brief 兼容原app_agent.h
 * @version 0.1
 * @date 2023-12-08
 * 
 * @copyright Copyright (c) 2023
 * 
 */



#ifndef __APP_AGENT__
#define __APP_AGENT__

#include "tuya_cloud_types.h"
#if defined(ENABLE_TUYA_LAN) && (ENABLE_TUYA_LAN==1)
#include "tuya_svc_lan.h"

#pragma pack(1)

#pragma pack()

#ifdef __cplusplus
extern "C" {
#endif

#define lan_pro_cntl_cfg(cfg, data) tuya_svc_lan_cfg(cfg, data)
#define lan_pro_cntl_register_cb(frame_type, handler) tuya_svc_lan_register_cb(frame_type, handler)
#define lan_data_report_cb(fr_type, ret_code, data, len) tuya_svc_lan_data_report(fr_type, ret_code, data, len)

#ifdef __cplusplus
} // extern "C"
#endif
#endif

#endif // __APP_AGENT__
