#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <uni_thread.h>
#include <unistd.h>

#include "mem_pool.h"
#include "tuya_hal_fs.h"
#include "tuya_hal_semaphore.h"
#include "tuya_app_config.h"
#include "tuya_app_dp_demo.h"
#include "tuya_app_rt_map_send_demo.h"
#include "tuya_app_net_sync_demo.h"
#include "uni_time.h"

#define GET_FILE_NUM_MAX   10   /*测试文件最大支持 10 个*/
#define RT_MAP_TRANS_SLEEP_TIME 2000 /*默认每隔 2000mS 上报一次路径数据。上报的频率根据扫地机移动的速度来推算，以 0.3M/s 的行走速度为例，2S 上报一次*/

/**
 * @brief  扫地机实时地图上传资源管理结构体
 */
typedef struct {
    int original_num; //原始图片上报计数
    char original_fileName[GET_FILE_NUM_MAX][48]; // 原始图片文件名称
    int onoff;      //一共两种事件，0 表示停止传输，1 表示开始传输
    void* deal_sem; /* 实时地图发送任务处理信号量 */
} RVC_RT_MAP_CTRL;

/**
 * @brief  扫地机实时地图上传资源全局变量
 */
RVC_RT_MAP_CTRL g_rvc_rt_map_ctrl = { 0 };

char g_file_path_name[GET_FILE_NUM_MAX][256] = { 0 }; //测试文件的路径缓存

/**
 * @brief  获取原始图片名称
 * @param  [CHAR_T] ai_name 获取原始图片名称
 * @return [INT] 结果
 */
INT_T original_sets_handler(CHAR_T* ai_name)
{
    OPERATE_RET ret = OPRT_OK;

    g_rvc_rt_map_ctrl.original_num++;
    PR_DEBUG("original num: %d", g_rvc_rt_map_ctrl.original_num);
    if (g_rvc_rt_map_ctrl.original_num > GET_FILE_NUM_MAX) {
        g_rvc_rt_map_ctrl.original_num = GET_FILE_NUM_MAX;    //公版面板默认一次下载一张原始图片，最大支持 10 张原始图片同时下载，由于自定义面板逻辑。
    }
    memcpy(&g_rvc_rt_map_ctrl.original_fileName[g_rvc_rt_map_ctrl.original_num - 1], ai_name, \
    sizeof(g_rvc_rt_map_ctrl.original_fileName[g_rvc_rt_map_ctrl.original_num - 1]));
    g_rvc_rt_map_ctrl.onoff = TRUE;              //开始事件传输
    ret = tuya_hal_semaphore_post(g_rvc_rt_map_ctrl.deal_sem); //释放信号量
    if (ret != OPRT_OK) {
        PR_ERR("post ctrl deal sem err %d\n", ret);
        return OPRT_COM_ERROR;
    }
    return ret;
}

/**
 * @brief  实时地图及路径传输状态回调
 * @param [in] onoff 一共两种事件，0 表示停止传输，1 表示开始传输
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET set_trans_event_cb(int onoff)
{
    OPERATE_RET ret = OPRT_OK;
    PR_DEBUG("Callback function called with: %d", onoff);
    g_rvc_rt_map_ctrl.onoff = onoff;                           //事件传输
    ret = tuya_hal_semaphore_post(g_rvc_rt_map_ctrl.deal_sem); //释放信号量
    if (ret != OPRT_OK) {
        PR_ERR("post ctrl deal sem err %d\n", ret);
        return OPRT_COM_ERROR;
    }
    return OPRT_OK;
}

/**
 * @brief  读取文件
 * @param  [int] fileType 文件对应的类型
 * @param  [RVC_FILE_INFO *] pStrFileInfo
 * @return [INT] 结果
 */
int rvc_read_file(int fileType, RVC_FILE_INFO* pStrFileInfo)
{
    char fullpath[256] = { 0 };

    strcpy(fullpath, g_file_path_name[fileType]);

    //高清图片是需要对应的坐标来或者数据，在获取高清图片数据前，先验证 APP 下发的坐标本地是否存在。这里是测试文件
    if (fileType == RVC_TYPE_ORIGINAL_SEND) { 
        if(g_rvc_rt_map_ctrl.original_num) {    //APP 有下发获取高清图片数据，并且根据文件名称与本地文件做对比
            if (OPRT_OK != strncmp(g_rvc_rt_map_ctrl.original_fileName[g_rvc_rt_map_ctrl.original_num - 1], "aiHD_0000001e_00000040.bin", strlen("aiHD_0000001e_00000040.bin"))) {
                return -1;
            } 
        } else {
            return -1;
        }
        
    } 
    PR_DEBUG(" read file :[%s]\n", fullpath);

    TUYA_FILE fp = tuya_hal_fopen(fullpath, "rb");
    if (fp == NULL) {
        PR_ERR("File Open Failed:%s", fullpath);
        return -1;
    }

    tuya_hal_fseek(fp, 0, SEEK_END);
    int len = tuya_hal_ftell(fp);
    if (len <= 0) {
        PR_ERR("file len is :%d", len);
        tuya_hal_fclose(fp);
        return -1;
    }
    tuya_hal_fseek(fp, 0, SEEK_SET);

    pStrFileInfo->buff = (char*)Malloc(len);
    if (pStrFileInfo->buff == NULL) {
        PR_ERR("malloc failed :%d", len);
        tuya_hal_fclose(fp);
        return -1;
    }

    int read_len = tuya_hal_fread(pStrFileInfo->buff, len, fp);
    if (read_len != len) {
        PR_ERR("tuya_hal_fread failed!");
        tuya_hal_fclose(fp);
        Free(pStrFileInfo->buff);
        return -1;
    }

    tuya_hal_fclose(fp);
    fp = NULL;

    pStrFileInfo->len = len;
    PR_DEBUG("pStrFileInfo->len %d\n", pStrFileInfo->len);
    return 0;
    //根据固定的文件类型获取对应的文件内容，只是 demo 测试用，开发者可以根据自己业务的需求实现
}

/**
 * @brief  业务实时地图上传启动的逻辑
 * @param  [void*] arg
 * @return [*]
 */
void* thread_album_send(void* arg)
{
    OPERATE_RET ret = OPRT_OK;
    
    PR_DEBUG("THREAD_ALBUM_SEND start...\n");
    while (1) {
        ret = tuya_hal_semaphore_waittimeout(g_rvc_rt_map_ctrl.deal_sem, RT_MAP_TRANS_SLEEP_TIME); //默认最长等待 2s
        if (OPRT_OK == ret) { // 等待信号量成功
            PR_DEBUG("recv sem goto rt map send!\r\n");
        } else {
            PR_DEBUG("real-time map sleep time ok goto send!\r\n");
        }

        if(g_rvc_rt_map_ctrl.onoff == FALSE) {    //接收到暂停事件，停止传输，开发者可以根据机器的运行状态或者地图及路径的更新情况，来上报数据
            continue;
        }

#if defined(TY_ROBOT_AI_ENABLE) && (TY_ROBOT_AI_ENABLE == 1) // AI 功能是选配，开发者根据需求适当开启
        int total_fileType =  RVC_AI_FILE_NUM_MAX; //最多需要传输的地图文件数据
#else
        int total_fileType = RVC_ALBUM_FILE_NUM_MAX; //最多需要传输的地图文件数据
#endif
        for (int fileType = 0; fileType < total_fileType; fileType++) {

            RVC_FILE_INFO strFileInfo = { 0 };
            if(fileType < total_fileType) {
               ret = rvc_read_file(fileType, &strFileInfo); //获取文件数据，准备上报
                if (0 != ret) {
                    PR_DEBUG("__get_rvc_file_bufferr err %d\n", ret);
                    if (strFileInfo.buff != NULL) { //释放申请的 buff
                        Free(strFileInfo.buff);
                        strFileInfo.buff = NULL;
                    }
                    continue;
                } 
            }
            /*
            该接口通过 P2P 通道将地图数据发送给 APP，
            第一个参数是地图文件的长度
            第二个参数是地图文件的内容
            */
            /*默认每隔 2000mS 上报一次路径数据。这个上报周期，上报的频率根据扫地机移动的速度来推算，以 0.3M/s 的行走速度为例，2S 上报一次*/
            if (RVC_TYPE_MAP_SEND == fileType) {
            #if defined(RT_MAP_STRUCTURED_TRANS_ENABLE) && (RT_MAP_STRUCTURED_TRANS_ENABLE == 1)
                ret = ty_rvc_rt_map_send_v2((RVC_MAP_DATA_S*)strFileInfo.buff);   //地图数据上报
            #else 
                ret = ty_rvc_rt_map_send_v1(strFileInfo.buff, strFileInfo.len);   //地图数据上报
            #endif
            } else if (RVC_TYPE_CLEANPATH_SEND == fileType) {
            #if defined(RT_MAP_STRUCTURED_TRANS_ENABLE) && (RT_MAP_STRUCTURED_TRANS_ENABLE == 1)
                ret = ty_rvc_rt_cleanpath_send_v2((RVC_PATH_INFO_S*)strFileInfo.buff); //路径上报接口
            #else 
                ret = ty_rvc_rt_cleanpath_send_v1(strFileInfo.buff, strFileInfo.len); //路径上报接口
            #endif
            }
        #if defined(TY_ROBOT_AI_ENABLE) && (TY_ROBOT_AI_ENABLE == 1) // AI 功能是选配，开发者根据需求适当开启
            else if (RVC_TYPE_THUMBNAILS_SEND == fileType) {
                // 多个物体 ai.bin 的例子如下：00 01 03 0000001e00000040 00ff 0000003000000060 01ff 0000002000000050 02ff
                //本 demo 上做了一个物体上报
                RVC_AI_OBJECT_THUMBNAILS_INFO_S* thumbnails_info = NULL;
                int object_num = strFileInfo.buff[0x02];
                thumbnails_info = Malloc(sizeof(RVC_AI_OBJECT_THUMBNAILS_INFO_S) + object_num * sizeof(OBJECT_THUMBNAILS_PARAM_S));
                if (thumbnails_info == NULL) {
                    PR_ERR("malloc failed");
                    if (strFileInfo.buff != NULL) { //释放申请的文件 buff
                        Free(strFileInfo.buff);
                        strFileInfo.buff = NULL;
                    }
                    continue;
                }
                thumbnails_info->map_id = (strFileInfo.buff[0x00] << 0x08) | (strFileInfo.buff[0x01] & 0xff);
                thumbnails_info->object_num = strFileInfo.buff[0x02];
                thumbnails_info->param = (OBJECT_THUMBNAILS_PARAM_S*)((UCHAR_T*)thumbnails_info + sizeof(RVC_AI_OBJECT_THUMBNAILS_INFO_S));
                for (int i = 0; i < thumbnails_info->object_num; i++) {
                    thumbnails_info->param[i].object_type = strFileInfo.buff[0x0B + (i * 10)];  //物体类型
                    thumbnails_info->param[i].accuracy = strFileInfo.buff[0x0C + (i * 10)];     //识别准确率
                    //对应的物体坐标
                    thumbnails_info->param[i].points.x = (strFileInfo.buff[0x03 + (i * 10)] << 24) | (strFileInfo.buff[0x04 + (i * 10)] << 16) | (strFileInfo.buff[0x05 + (i * 10)] << 8) | (strFileInfo.buff[0x06 + (i * 10)] & 0xff);  
                    thumbnails_info->param[i].points.y = (strFileInfo.buff[0x07 + (i * 10)] << 24) | (strFileInfo.buff[0x08 + (i * 10)] << 16) | (strFileInfo.buff[0x09 + (i * 10)] << 8) | (strFileInfo.buff[0x0A + (i * 10)] & 0xff);
                }
                ret = ty_rvc_rt_ai_thumbnails_send(thumbnails_info); // AI 原始缩略图上报
                if (thumbnails_info != NULL) {
                    Free(thumbnails_info);
                }
            } else if (RVC_TYPE_ORIGINAL_SEND == fileType) {
                if (g_rvc_rt_map_ctrl.original_num) { //有高清图片需要上报
                    //目前地图类型支持如下：jpg,png,gif,svg,bmp,ic0 方式，开发者需要按照例程的方式填写
                    ret = ty_rvc_ai_original_send("jpg", strFileInfo.buff, strFileInfo.len);   //填入图片的原始数据
                    memset(g_rvc_rt_map_ctrl.original_fileName[g_rvc_rt_map_ctrl.original_num - 1], 0x00, \
                    sizeof(g_rvc_rt_map_ctrl.original_fileName[g_rvc_rt_map_ctrl.original_num - 1]));
                    g_rvc_rt_map_ctrl.original_num--;
                } else {
                    ret = OPRT_NOT_FOUND;
                }
            }
        #endif
            else {
                ret = OPRT_NOT_FOUND;
            }
            if (strFileInfo.buff != NULL) { //释放申请的 buff
                Free(strFileInfo.buff);
                strFileInfo.buff = NULL;
            }
            if (OPRT_OK == ret) { //发送完成
                PR_DEBUG("real-time data send finish %d", fileType);
            } else {
                PR_ERR("real-time data send err %d", ret);
            }
        }
    }
    PR_DEBUG("thread_sweeper_proc exit...\n");
}

/**
 * @brief  用户设置扫地机自定义名称
 * @param  [*]
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET ty_user_sweeper_set_customize_name(VOID)
{
    OPERATE_RET ret = 0;
#if defined(TUYA_SWEEPER_ALBUM_CUSTOMIZE_FILE_NUM_MAX) && (TUYA_SWEEPER_ALBUM_CUSTOMIZE_FILE_NUM_MAX > 0)
    TUYA_SWEEPER_CUSTOMIZE_FILEINFO customize_name = { 0 };
    customize_name.itemCount = TUYA_SWEEPER_ALBUM_CUSTOMIZE_FILE_NUM_MAX;
    strncpy(customize_name.itemArr[0].filename, "aiHD_XXXX_YYYY.bin", strlen("aiHD_XXXX_YYYY.bin"));
    strncpy(customize_name.itemArr[1].filename, "ai.bin.stream", strlen("ai.bin.stream"));
    ret = tuya_sweeper_set_customize_name(&customize_name);
    if (OPRT_OK != ret) {
        PR_ERR("user set customize name failed!");
    }
#endif
    return ret;
}

/**
 * @brief  扫地机实时地图上传服务启动
 * @param  [*]
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET ty_user_sweeper_rt_map_init(void)
{
    OPERATE_RET ret = 0;
    THRD_PARAM_S thrd_param;
    THRD_HANDLE thrd; /* 任务句柄 */

    ret = ty_rvc_server_init(set_trans_event_cb); // 初始化设备服务
    if (ret != OPRT_OK) {
        PR_ERR("tuya media server init failed");
        return ret;
    }

    /* 创建实时地图上报的线程任务 */
    thrd_param.stackDepth = 1024 * 1024;
    thrd_param.thrdname = "rt_map_trans_thrd";
    thrd_param.priority = TRD_PRIO_1;

    /* 创建机器状态管理任务 */
    ret = CreateAndStart(&thrd, NULL, NULL, (THREAD_FUNC_CB)thread_album_send, NULL, &thrd_param);
    if (OPRT_OK != ret) {
        PR_ERR("rt_map_trans_thrd create failed!");
        return ret;
    }

    ret = tuya_hal_semaphore_create_init(&g_rvc_rt_map_ctrl.deal_sem, 0, GET_FILE_NUM_MAX); //创建一个实时地图上报的信号量

    if (OPRT_OK != ret) {
        PR_ERR("real-time map sem create err!\r\n");
        return ret;
    }
    PR_DEBUG("THREAD_ALBUM_SEND start...\n");
    sprintf(g_file_path_name[0], "%s%s", "", "/tmp/map.bin"); //测试用，创建临时的文件
    sprintf(g_file_path_name[1], "%s%s", "", "/tmp/cleanPath.bin");
    sprintf(g_file_path_name[2], "%s%s", "", "/tmp/ai.bin");
    sprintf(g_file_path_name[3], "%s%s", "", "/tmp/aiHD_005b_0120.bin");    // ai 原始图片
    return OPRT_OK;
}

