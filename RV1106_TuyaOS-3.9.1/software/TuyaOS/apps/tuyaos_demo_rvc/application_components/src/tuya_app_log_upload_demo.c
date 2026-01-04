/*********************************************************************************
 *Copyright(C),2015-2020, TUYA www.tuya.comm
 *FileName:    tuya_app_log_upload_demo
  *brief：The device local log reporting function is to upload the logs stored 
          locally to the cloud, allowing users to download the corresponding logs 
	  from the backend.
 **********************************************************************************/
#include "tuya_app_config.h"
#include "tuya_iot_com_api.h"
#include "uni_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

/**
 * @brief  设备本地日志上报的处理
 * @param  [char*] path 上报的日志路径
 * @param  [int] len
 * @return [*]
 */
void cloud_log_path_cb(OUT char* path, IN CONST int len)
{
    char cmd[128] = { 0 };
    
    //开发者可以将本地的日志如test.log打包到log_all.tar（名称可自行定义），开发者只需要注意，一次打包的文件大小不要超过30M，如果文件够大，可以分批打包分批传递给path
    snprintf(cmd, sizeof(cmd), "tar -cvf /tmp/log_all.tar /tmp/test.log"); 
    system(cmd);
    strcpy(path, "/tmp/log_all.tar");   //将打包好的日志包及路径传递给path即可，SDK会通过cloud_log_path_cb来获取
    PR_DEBUG("goto upload local log finish");

    return;
}

/**
 * @brief  设备本地日志上报之后的回调函数
 * @param  @param  [int] result 处理的结果 0 OK
 * @return [*]
 */
void cloud_log_deal_cb(OUT int result)
{
    char cmd[128] = { 0 };
    snprintf(cmd, sizeof(cmd), "rm -rf /tmp/log_all.tar");    //删除文件
    system(cmd);
    if (OPRT_OK == result) { // 开发者收到上报成功的回调，之后可以循序其他的文件上报
        PR_DEBUG("upload app log , goto delete data! ");
    }
    return;
}

/**
 * @brief  设备本地日志上报初始化
 * @param  [*]
 * @return [*]
 */
void ty_log_upload_init(void)    
{
    TY_RVC_APP_CBS_S app_log_deal = {
        cloud_log_path_cb,
        cloud_log_deal_cb
    };

    tuya_rvc_app_cbs_init(&app_log_deal); // 注册云端日志回调处理函数
}