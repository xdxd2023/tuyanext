/*********************************************************************************
 *Copyright(C),2015-2020, TUYA www.tuya.comm
 *FileName:    tuya_app_rt_map_demo
 *brief：1.This article introduces the implementation of real-time map and path
           reporting features based on the Ubuntu environment.
         2.It provides fixed support for interfaces that report maps and paths.
         3.You can customize the filenames for real-time reporting,
           such as for AI object recognition, and you can adjust the reporting
           frequency of maps and paths according to your needs.
         4.You can build your application development based on the following demo.
 **********************************************************************************/
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

#define GET_FILE_NUM_MAX   10   /*测试文件最大支持10个*/
#define RT_MAP_TRANS_SLEEP_TIME 2000 /*默认每隔2000mS上报一次路径数据。上报的频率根据扫地机移动的速度来推算，以0.3M/s的行走速度为例，2S上报一次*/

/**
 * @brief  扫地机实时地图上传资源管理结构体
 */
typedef struct {
    int original_num; //原始图片上报计数
    char original_fileName[GET_FILE_NUM_MAX][48]; // 原始图片文件名称
    int onoff;      //一共两种事件，0表示停止传输，1表示开始传输
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
 * @return [INT]结果
 */
INT_T original_sets_handler(CHAR_T* ai_name)
{
    OPERATE_RET ret = OPRT_OK;

    g_rvc_rt_map_ctrl.original_num++;
    PR_DEBUG("original num: %d", g_rvc_rt_map_ctrl.original_num);
    if (g_rvc_rt_map_ctrl.original_num > GET_FILE_NUM_MAX) {
        g_rvc_rt_map_ctrl.original_num = GET_FILE_NUM_MAX;    //公版面板默认一次下载一张原始图片，最大支持10张原始图片同时下载，由于自定义面板逻辑。
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
 * @param [in] onoff 一共两种事件，0表示停止传输，1表示开始传输
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
 * @return [INT]结果
 */
int rvc_read_file(int fileType, RVC_FILE_INFO* pStrFileInfo)
{
    char fullpath[256] = { 0 };

    strcpy(fullpath, g_file_path_name[fileType]);

    //高清图片是需要对应的坐标来或者数据，在获取高清图片数据前，先验证APP下发的坐标本地是否存在。这里是测试文件
    if (fileType == RVC_TYPE_ORIGINAL_SEND) { 
        if(g_rvc_rt_map_ctrl.original_num) {    //APP有下发获取高清图片数据,并且根据文件名称与本地文件做对比
            if (OPRT_OK != strncmp(g_rvc_rt_map_ctrl.original_fileName[g_rvc_rt_map_ctrl.original_num - 1], "aiHD_0000001e_00000040.bin", strlen("aiHD_0000001e_00000040.bin"))) {
                return -1;
            } 
        } else {
            return -1;
        }
        
    } 
    PR_DEBUG(" read file :[%s]\n", fullpath);

    TUYA_FILE* fp = tuya_hal_fopen(fullpath, "rb");
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
    //根据固定的文件类型获取对应的文件内容，只是demo测试用，开发者可以根据自己业务的需求实现
}

/**
 * @brief  十六进制数据打印
 * @param  [in] data 数据内容
 * @param  [in] length 数据长度
 * @return [*]
 */
void print_hex(const unsigned char *data, size_t length) {
    for (size_t i = 0; i < length; i++) {
        // 每 32 字节打印一次
        if (i % 32 == 0 && i != 0) {
            printf("\n"); // 每 4 字节换行
        }
        printf("%02X ", data[i]); // 打印每个字节的十六进制值
    }
    printf("\n"); // 最后换行
}

/**
 * @brief  二维数组内存申请
 * @param  [in] map_buf 一维数据内容
 * @param  [in] height 地图像素高度
 * @param  [in] width 地图像素宽度
 * @return [out]
 */
int ty_map_buffer_allocate(unsigned char*** map_buf, int height, int width)
{
    // 分配二维数组的行
    *map_buf = (unsigned char**)malloc(height * sizeof(unsigned char*));
    if (*map_buf == NULL) {
        return -1; // 分配失败
    }

    // 分配每一行的列
    for (int i = 0; i < height; i++) {
        (*map_buf)[i] = (unsigned char*)malloc(width * sizeof(unsigned char));
        if ((*map_buf)[i] == NULL) {
            // 释放已分配的内存
            for (int j = 0; j < i; j++) {
                free((*map_buf)[j]);
            }
            free(*map_buf);
            return -1; // 分配失败
        }
    }

    return 0; // 成功
}

/**
 * @brief  路径数据转换
 * @param  [in] path_buf 路径内存数据
 * @param  [in] path_len 路径数据长度
 * @param  [out] path_info 转换为RVC_PATH_INFO_S，后输出
 */
void __ty_path_data_unpack(IN CHAR_T* path_buf, IN INT_T path_len, RVC_PATH_INFO_S* path_info)
{
    path_info->path_id = path_buf[0] << 8 | path_buf[1];
    path_info->path_type = path_buf[2];
    path_info->direction = path_buf[3] << 8 | path_buf[4];    
    path_info->point_count =  path_buf[5] << 24 | path_buf[6] << 16 | path_buf[7] << 8 | path_buf[8];
    path_info->path_points = Malloc(path_info->point_count * sizeof(PATH_POINT_S));

    if(path_len <= 8){
        path_info->point_count = 0;
        path_info->path_points = NULL;
        return;
    }

    char* dest_data = Malloc(path_info->point_count *4);
    if(dest_data == NULL){
        PR_ERR("malloc failed");
        return;
    }
    memset(dest_data, 0,path_info->point_count *4);
    memcpy(dest_data, &path_buf[9], path_info->point_count *4);
    int temp_x = 0,temp_y = 0;
    for(int i = 0; i < path_info->point_count;i++){
        temp_x = ((dest_data[4*i] << 8) & 0xff00) | (dest_data[4*i+1] & 0xff);
        temp_y = (dest_data[4*i+2] << 8) | (dest_data[4*i+3]);
        path_info->path_points[i].type = ((temp_x & 0x0001) << 1) | (temp_y & 0x0001);
        path_info->path_points[i].coordinates.x = temp_x & 0xfffe;
        path_info->path_points[i].coordinates.y = temp_y & 0xfffe;
        //PR_DEBUG("point[%d] type:%x,x:%x, y:%x", i, path_info->path_points[i].type, path_info->path_points[i].coordinates.x,path_info->path_points[i].coordinates.y);
    }
    Free(dest_data);
}

/**
 * @brief 实时地图上报接口
 * @param[in] buf: 输入的地图数据
 * @param[in] buf_len:输入的地图数据长度
 * @param[out] RVC_MAP_DATA_S: 实时数据转换为RVC_MAP_DATA_S结构体输出
 * @return OPERATE_RET: 0 成功，其余错误码表示失败
 */
OPERATE_RET ty_rvc_map_data_unpack(CHAR_T* buf, INT_T buf_len, OUT RVC_MAP_DATA_S* rvc_map_info)
{
    OPERATE_RET ret = 0;
    
    int i = 0;
    int j = 0;
    /*地图协议头部固定字节赋值*/
    rvc_map_info->header.map_id = 3;   //地图ID
    rvc_map_info->header.status = 1;    //0x00  地图非稳定状态，0x01  地图稳定状态
    rvc_map_info->header.width = 130;   //地图像素宽度
    rvc_map_info->header.height = 113;  //地图像素高度    
    rvc_map_info->header.origin.x = 33; //机器原点 X 坐标
    rvc_map_info->header.origin.y = 68; //机器原点 Y 坐标
    rvc_map_info->header.resolution = 5; //分辨率              
    rvc_map_info->header.charge_point.x = 30; //充电桩 X 坐标
    rvc_map_info->header.charge_point.y = 50; //充电桩 Y 坐标
    rvc_map_info->header.charge_angle = 0; //充电桩朝向

    // 调用函数分配内存
    rvc_map_info->map_buff = NULL;
    if (ty_map_buffer_allocate(&rvc_map_info->map_buff, rvc_map_info->header.height, rvc_map_info->header.width) != 0) {
        PR_ERR("Failed to allocate memory for map_buf");
        return -1;
    }

    for (i = 0; i < rvc_map_info->header.height; i++) {
        for (j = 0; j < rvc_map_info->header.width; j++) {
            rvc_map_info->map_buff[i][j] = buf[rvc_map_info->header.width* i + j];
        }
    }

        /*读取房间轮廓信息并传递给SDK接口*/
    RVC_FILE_INFO map_buf_info = { 0 };
    ret = rvc_read_file(RVC_TYPE_ROOM_CONER, &map_buf_info); //获取文件数据，准备上报
    if (0 != ret) {
        PR_DEBUG("__get_map_buf_info err %d\n", ret);
        if (map_buf_info.buff != NULL) { //释放申请的buff
            Free(map_buf_info.buff);
            map_buf_info.buff = NULL;
        }
        if(rvc_map_info->room_polygons_data != NULL) {
            Free(rvc_map_info->room_polygons_data);
        }
        return -1;
    } 

    if (map_buf_info.buff != NULL) {
        int room_count = map_buf_info.buff[0];    //房间数量
        PR_DEBUG("room count:%d",  room_count);
        RVC_ROOM_POLYGON_S *tmp_room_polygon = NULL;
        int tmp_points_size = 0;
        int offset_len = 0;        //数据偏移
        int all_points_size = 0;   //所有坐标数量
        for (i = 0; i < room_count; i++) {     //遍历房间轮廓信息
            all_points_size += (map_buf_info.buff[8 + 8 * i] << 24) | (map_buf_info.buff[7 + 8 * i] << 16) | (map_buf_info.buff[6 + 8 * i] << 8) | map_buf_info.buff[5 + 8 * i];
            
        }
        tmp_room_polygon = (RVC_ROOM_POLYGON_S*)Malloc(room_count * sizeof(RVC_ROOM_POLYGON_S) + all_points_size * sizeof(RVC_POINT_S));  //申请内存
        rvc_map_info->room_polygons_data = tmp_room_polygon;
        float tmp_origin_x = (float)rvc_map_info->header.origin.x /1.0f;    //获取原点坐标
        float tmp_origin_y = (float)rvc_map_info->header.origin.y /1.0f;

        RVC_POINT_S* points_memory =(RVC_POINT_S*)(tmp_room_polygon + room_count);  //坐标点数据指向二级指针

        for ( i = 0; i < room_count; i++) {     //遍历房间轮廓信息
            tmp_room_polygon->id = (map_buf_info.buff[4 + 8 * i ] << 24) | (map_buf_info.buff[3 + 8 * i] << 16) | (map_buf_info.buff[2 + 8 * i] << 8) | map_buf_info.buff[1 + 8 * i];
            tmp_room_polygon->polygon.points_size = (map_buf_info.buff[8 + 8 * i] << 24) | (map_buf_info.buff[7 + 8 * i] << 16) | (map_buf_info.buff[6 + 8 * i] << 8) | map_buf_info.buff[5 + 8 * i];
            PR_DEBUG("room id :%d, points size :%d", tmp_room_polygon->id, tmp_room_polygon->polygon.points_size);
            offset_len = 1 + 8 * room_count + tmp_points_size; //数据偏移

            tmp_room_polygon->polygon.points = points_memory;
            for (j = 0; j < tmp_room_polygon->polygon.points_size; j++) {    //上报给面板都的坐标系需要是屏幕坐标系，开发者根据自身的地图算法做调整
                memcpy(&tmp_room_polygon->polygon.points[j].x, map_buf_info.buff + (offset_len + j * 12), sizeof(RVC_POINT_S));
                tmp_room_polygon->polygon.points[j].x = tmp_room_polygon->polygon.points[j].x * 100 / 5 + (tmp_origin_x);   //例程中需要坐标转换,以世界坐标系的原点坐标和分辨率转为屏幕坐标系。
                tmp_room_polygon->polygon.points[j].y = (tmp_origin_y) - (tmp_room_polygon->polygon.points[j].y * 100 / 5) ;
                tmp_room_polygon->polygon.points[j].z = tmp_room_polygon->polygon.points[j].z * 100 / 5;  
                //PR_DEBUG("point[%d] x:%f,y:%f, z:%f", j, tmp_room_polygon->polygon.points[j].x, tmp_room_polygon->polygon.points[j].y,tmp_room_polygon->polygon.points[j].z);
            }

            tmp_points_size += (tmp_room_polygon->polygon.points_size * sizeof(RVC_POINT_S));   //累加上房间的坐标点数量 
            points_memory += tmp_room_polygon->polygon.points_size;   //二级指针地址偏移
            tmp_room_polygon++;         //获取下一个房间轮廓信息
        }
        rvc_map_info->room_polygons_count = room_count;
    }
    if(map_buf_info.buff != NULL) {
        Free(map_buf_info.buff);
    }

    return ret;
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
        ret = tuya_hal_semaphore_waittimeout(g_rvc_rt_map_ctrl.deal_sem, RT_MAP_TRANS_SLEEP_TIME); //默认最长等待2s
        if (OPRT_OK == ret) { // 等待信号量成功
            PR_DEBUG("recv sem goto rt map send!\r\n");
        } else {
            PR_DEBUG("real-time map sleep time ok goto send!\r\n");
        }

        if(g_rvc_rt_map_ctrl.onoff == FALSE) {    //接收到暂停事件，停止传输，开发者可以根据机器的运行状态或者地图及路径的更新情况，来上报数据
            continue;
        }

#if defined(TY_ROBOT_AI_OBJECT_ENABLE) && (TY_ROBOT_AI_OBJECT_ENABLE == 1) // AI功能是选配，开发者根据需求适当开启
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
                    if (strFileInfo.buff != NULL) { //释放申请的buff
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
            /*默认每隔2000mS上报一次路径数据。这个上报周期，上报的频率根据扫地机移动的速度来推算，以0.3M/s的行走速度为例，2S上报一次*/
            if (RVC_TYPE_MAP_SEND == fileType) {
            #if defined(RT_MAP_STRUCTURED_TRANS_ENABLE) && (RT_MAP_STRUCTURED_TRANS_ENABLE == 1)
                RVC_MAP_DATA_S rvc_map_info = { 0 };
                ty_rvc_map_data_unpack(strFileInfo.buff, strFileInfo.len, &rvc_map_info);
                ret = ty_rvc_rt_map_send_v2(&rvc_map_info);   //地图数据上报
                if(rvc_map_info.room_polygons_data != NULL) {
                    Free(rvc_map_info.room_polygons_data);
                } 
            #else 
                ret = ty_rvc_rt_map_send_v1(strFileInfo.buff, strFileInfo.len);   //地图数据上报
            #endif
            } else if (RVC_TYPE_CLEANPATH_SEND == fileType) {
            #if defined(RT_MAP_STRUCTURED_TRANS_ENABLE) && (RT_MAP_STRUCTURED_TRANS_ENABLE == 1)
                RVC_PATH_INFO_S path_data_proper = {0};
                __ty_path_data_unpack(strFileInfo.buff,strFileInfo.len, &path_data_proper);
                ret = ty_rvc_rt_cleanpath_send_v2(&path_data_proper); //路径上报接口
                if(path_data_proper.path_points != NULL) {
                    Free(path_data_proper.path_points);
                }
            #else 
                ret = ty_rvc_rt_cleanpath_send_v1(strFileInfo.buff, strFileInfo.len); //路径上报接口
            #endif
            }
        #if defined(TY_ROBOT_AI_OBJECT_ENABLE) && (TY_ROBOT_AI_OBJECT_ENABLE == 1) // AI功能是选配，开发者根据需求适当开启
            else if (RVC_TYPE_THUMBNAILS_SEND == fileType) {
                // 多个物体ai.bin的例子如下：00 01 03 0000001e00000040 00ff 0000003000000060 01ff 0000002000000050 02ff
                //本demo上做了一个物体上报
                RVC_AI_OBJECT_THUMBNAILS_INFO_S* thumbnails_info = NULL;
                int object_num = strFileInfo.buff[0x02];
                thumbnails_info = Malloc(sizeof(RVC_AI_OBJECT_THUMBNAILS_INFO_S) + object_num * sizeof(OBJECT_THUMBNAILS_PARAM_S));
                if (thumbnails_info == NULL) {
                    PR_ERR("malloc failed");
                    if (strFileInfo.buff != NULL) { //释放申请的文件buff
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
                ret = ty_rvc_rt_ai_thumbnails_send(thumbnails_info); // AI原始缩略图上报
                if (thumbnails_info != NULL) {
                    Free(thumbnails_info);
                }
            } else if (RVC_TYPE_ORIGINAL_SEND == fileType) {
                if (g_rvc_rt_map_ctrl.original_num) { //有高清图片需要上报
                    //目前地图类型支持如下：jpg,png,gif,svg,bmp,ic0方式，开发者需要按照例程的方式填写
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
            if (strFileInfo.buff != NULL) { //释放申请的buff
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
 * @brief  扫地机实时地图上传服务启动
 * @param  [*]
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET ty_user_sweeper_rt_map_init(void)
{
    OPERATE_RET ret = 0;
    THRD_PARAM_S thrd_param;
    THRD_HANDLE thrd; /* 任务句柄 */

    /* 创建实时地图上报的线程任务 */
    thrd_param.stackDepth = 1024 * 1024;
    thrd_param.thrdname = "rt_map_trans_thrd";
    thrd_param.priority = TRD_PRIO_1;

    ret = tuya_hal_semaphore_create_init(&g_rvc_rt_map_ctrl.deal_sem, 0, GET_FILE_NUM_MAX); //创建一个实时地图上报的信号量
    if (OPRT_OK != ret) {
        PR_ERR("real-time map sem create err!\r\n");
        return ret;
    }
    
    /* 创建机器状态管理任务 */
    ret = CreateAndStart(&thrd, NULL, NULL, (void*)thread_album_send, NULL, &thrd_param);
    if (OPRT_OK != ret) {
        PR_ERR("rt_map_trans_thrd create failed!");
        return ret;
    }

    extern char s_raw_path[];
    PR_DEBUG("THREAD_ALBUM_SEND start...\n");
#if defined(RT_MAP_STRUCTURED_TRANS_ENABLE) && (RT_MAP_STRUCTURED_TRANS_ENABLE == 1)
    sprintf(g_file_path_name[0], "%s%s", s_raw_path, "/resource/tuya_rvc_file/temp_grid_map.bin"); //测试用，创建临时的文件
    sprintf(g_file_path_name[1], "%s%s", s_raw_path, "/resource/tuya_rvc_file/temp_cleanPath.bin");        
    sprintf(g_file_path_name[4], "%s%s", s_raw_path, "/resource/tuya_rvc_file/temp_room_point.bin"); 
#else 
    sprintf(g_file_path_name[0], "%s%s", s_raw_path, "/resource/tuya_rvc_file/map.bin"); //测试用，创建临时的文件
    sprintf(g_file_path_name[1], "%s%s", s_raw_path, "/resource/tuya_rvc_file/cleanPath.bin");
#endif
    sprintf(g_file_path_name[2], "%s%s", s_raw_path, "/resource/tuya_rvc_file/ai.bin");
    sprintf(g_file_path_name[3], "%s%s", s_raw_path, "/resource/tuya_rvc_file/aiHD_0000001e_00000040.bin");
#if defined(TY_ROBOT_AI_OBJECT_ENABLE) && (TY_ROBOT_AI_OBJECT_ENABLE == 1) // AI功能是选配，开发者根据需求适当开启
    ret = ty_rvc_ai_original_init(original_sets_handler); //注册原始图片发送回调
#endif
    return ret;
}
