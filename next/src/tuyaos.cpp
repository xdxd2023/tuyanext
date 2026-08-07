#include "utils/log_.h"

#include <stdio.h>
//#include "tuya_robot_sdk_init.h"

#include "tuya_robot.h"
#include "tuyaos.h"
#include "voice.h"

#ifdef RK3326_TUYAOS
#include "uni_log.h"
#include "tuya_cloud_types.h"
#include "tuya_cloud_com_defs.h"
#include "tuya_cloud_wifi_defs.h"
#include "tuya_bt.h"
#include "tuya_sdk_simple_start.h"
#include "tuya_robot_sdk_init.h"
#include "tuya_sdk_common.h"
OPERATE_RET tuya_sdk_mqtt_online_sys_loop(void);
#endif

#if defined(R328_TUYAOS) || defined(RV1106_TUYAOS)
#include "tuya_error_code.h"
#include "tuya_wifi_netcfg.h"
#include "utilities/uni_log.h"
#include "tuya_iot_com_api.h"
#include "tuya_iot_config.h"
#include "tuya_iot_wifi_api.h"
#include "ty_rvc_api.h"
#include "tuya_app_simple_start.h"
#include "tuya_app_rt_map_send_demo.h"
#include "tuya_app_net_sync_demo.h"
#include "tuya_app_av_demo.h"
#endif

bool tuya_is_online(void);
OPERATE_RET ty_user_sweeper_rt_map_init(void);


#define TY_ROBOT_MEDIA_ENABLE 1


int tuyaos_init(const char *ty_uuid, const char *ty_authkey, const char *ty_pid, const char *system_version, const char *mcu_version)
{
    ty_sys_start(WF_START_AP_ONLY, "token", ty_uuid, ty_authkey, ty_pid, system_version, mcu_version);
    return 0;
}

bool tuyaos_is_online(void)
{
    return tuya_is_online();
}

void tuyaos_ble_on(void)
{
#ifdef RK3326_TUYAOS
#if defined(TY_BT_MOD) && TY_BT_MOD == 1
    tuya_ble_set_serv_switch(true);    //蓝牙能力开关
#endif
#endif
}

int tuyaos_post_init(void)
{
    int ret = -1;
    ret = tuya_sdk_mqtt_online_sys_loop(); // 设备首次上线处理
    if (ret != OPRT_OK)
    {
        return ret;
    }

    tuya_message::Request req;
    tuya_message::Result res = {};
    TuyaComm::Get()->Send("ty_wifi_connect", &req, &res);
    
    ret = ty_user_sweeper_rt_map_init(); // 实时地图业务初始化
    if (ret != OPRT_OK)
    {
        return ret;
    }
#if defined(TY_ROBOT_MEDIA_ENABLE) && (TY_ROBOT_MEDIA_ENABLE == 1)
    ret = ty_media_local_storage(); // 本地存储
    if (ret != OPRT_OK)
    {
        PR_ERR("tuya media local storage failed");
        return ret;
    }

    // 开启音频存储，视频录制时长 2 秒以上
    ret = ty_media_cloud_storage(TRUE, 2); // 云端存储
    if (ret != OPRT_OK)
    {
        PR_ERR("tuya media cloud storage failed");
        return ret;
    }

    // ret = tuya_enable_storage_deal(); // 音视频存储任务
    // if (ret != OPRT_OK)
    // {
    //     return ret;
    // }
#endif

#ifdef RK3326_TUYAOS
    ret = tuya_media_server_customize_init(TY_SDK_P2P_NUM_MAX); // 初始化媒体流服务
    if (ret != OPRT_OK)
    {
        PR_ERR("tuya media server init failed");
        return ret;
    }
#endif

#if defined(TY_ROBOT_MEDIA_ENABLE) && (TY_ROBOT_MEDIA_ENABLE == 1)
    //tuya_av_start(); // 开始塞音视频流数据
#ifdef RV1106_TUYAOS
    ret = tuya_av_stream_init(); //音视频任务初始化，需要在设备上线之后去启动
    if (ret != OPRT_OK) {
        return ret;
    }
#endif
#endif
    return 0;
}