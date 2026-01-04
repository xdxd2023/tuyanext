#pragma once

#define TY_ROBOT_MEDIA_ENABLE  1           //打开音视频开关

#if defined(TY_ROBOT_MEDIA_ENABLE) && (TY_ROBOT_MEDIA_ENABLE == 1)
#include "tuya_ipc_cloud_storage.h"
#include "tuya_ipc_stream_storage.h"
#include "tuya_sdk_media_demo.h"
#include "tuya_sdk_sd_card_demo.h"
#include "tuya_sdk_storage_demo.h"
#include "tuya_sdk_av_demo.h"
#endif

#include "tuya_sdk_app_reset_demo.h"
#include "tuya_sdk_dp_demo.h"
#include "tuya_sdk_floor_map_demo.h"
#include "tuya_sdk_log_upload_demo.h"
#include "tuya_sdk_net_sync_demo.h"
#include "tuya_sdk_rt_map_demo.h"
#include "tuya_sdk_sd_card_demo.h"
#include "tuya_sdk_storage_demo.h"
#include "tuya_sdk_upgrade_demo.h"
#include "tuya_sdk_voice_demo.h"