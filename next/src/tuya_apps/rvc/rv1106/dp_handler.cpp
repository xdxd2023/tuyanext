#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "tuya_iot_com_api.h"
#include "utilities/uni_log.h"

#include "tuya_robot.h"
#include "tuya_app_dp_demo.h"

void TuyaHandleDPCmd(TY_OBJ_DP_S *dp);

/**
 * @brief  接收到 sdk obj dp 处理回调，注册到 SDK 中
 * @param  [TY_SDK_RECV_OBJ_DP_S] *dp_rev
 * @return [*]
 */
VOID ty_cmd_handle_dp_cmd_objs(IN CONST TY_RECV_OBJ_DP_S* dp_rev)
{
    PR_DEBUG("[user cmd]recv cmd dp");
    for(int i=0; i<dp_rev->dps_cnt; i++)
    {
        TuyaHandleDPCmd((TY_OBJ_DP_S *)&dp_rev->dps[i]);
    }
}


/**
 * @brief  接收到 sdk raw dp 处理回调，注册到 SDK 中
 * @param  [TY_SDK_RECV_OBJ_DP_S] *dp_rev
 * @return [*]
 */
VOID ty_cmd_handle_dp_raw_objs(IN CONST TY_RECV_RAW_DP_S* dp_rev)
{
    PR_DEBUG("[user raw cmd]recv cmd dp");
    TuyaHandleRawDPCmd(dp_rev->dpid, (uint8_t *)dp_rev->data, dp_rev->len);
}


/**
 * @brief  设备上线上报相关信息
 * @param  [*]
 * @return [*]
 */
void ty_cmd_handle_sync_to_cloud(void)
{
    /***attention：
     * 当网络连接成功后，需要将机器内的数据进行上传同步
     * ***/
   //TuyaComm::Get()->ReportAll();
}


/**
 * @brief  接收到 sdk query dp 处理回调，注册到 SDK 中
 * @param  [TY_SDK_RECV_OBJ_DP_S] *dp_rev
 * @return [*]
 */
VOID ty_cmd_handle_dp_query_objs(IN CONST TY_DP_QUERY_S* dp_query)
{
    PR_DEBUG("[user cmd]recv query dp");
    TuyaComm::Get()->ReportStatus();
}