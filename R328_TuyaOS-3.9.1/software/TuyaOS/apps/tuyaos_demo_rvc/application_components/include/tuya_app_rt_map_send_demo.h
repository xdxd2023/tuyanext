/*********************************************************************************
  *Copyright(C),2015-2020, TUYA www.tuya.comm
  *FileName:    tuya_app_rt_map_send_demo
**********************************************************************************/

#ifndef INCLUDE_TUYA_APP_RT_MAP_SNED_DEMO_H_
#define INCLUDE_TUYA_APP_RT_MAP_ESND_DEMO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include<stdbool.h>

/**
 * @brief 实时文件类型发送枚举
 */
typedef enum {
    RVC_TYPE_MAP_SEND = 0,       // 实时地图上报
    RVC_TYPE_CLEANPATH_SEND,     // 实时路径上报
    RVC_TYPE_THUMBNAILS_SEND,    // 实时缩略图上报
    RVC_ALBUM_FILE_NUM_MAX = RVC_TYPE_THUMBNAILS_SEND,  //固定上报的最大文件数
    RVC_TYPE_ORIGINAL_SEND,      // 原始图片上报
    RVC_TYPE_MAX,    
    RVC_AI_FILE_NUM_MAX = RVC_TYPE_MAX,  //支持AI图文件，上报的最大文件数
    RVC_TYPE_ROOM_CONER = RVC_TYPE_MAX,         // 房间轮廓信息读取
} RVC_FILE_SEND_TYPE_E;


/**
 * @brief  扫地机实时地图上传的数据长度及内容
 */
typedef struct {
    INT_T len;
    CHAR_T* buff;
} RVC_FILE_INFO;

/**
 * @brief  扫地机实时地图上传服务启动
 * @param  [*]
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET ty_user_sweeper_rt_map_init(void);

/**
 * @brief 实时地图上报接口
 * @param[in] buf: 输入的地图数据
 * @param[in] buf_len:输入的地图数据长度
 * @param[out] RVC_MAP_DATA_S: 实时数据转换为RVC_MAP_DATA_S结构体输出
 * @return OPERATE_RET: 0 成功，其余错误码表示失败
 */
OPERATE_RET ty_rvc_map_data_unpack(CHAR_T* buf, int buf_len, OUT RVC_MAP_DATA_S* rvc_map_info);

/**
 * @brief  读取文件
 * @param  [int] fileType 文件对应的类型
 * @param  [RVC_FILE_INFO *] pStrFileInfo
 * @return [INT]结果
 */
int rvc_read_file(int fileType, RVC_FILE_INFO* pStrFileInfo);

/**
 * @brief  路径数据转换
 * @param  [in] path_buf 路径内存数据
 * @param  [in] path_len 路径数据长度
 * @param  [out] path_info 转换为RVC_PATH_INFO_S，后输出
 */
void __ty_path_data_unpack(IN CHAR_T* path_buf, IN INT_T path_len, RVC_PATH_INFO_S* path_info);

/**
 * @brief  实时地图及路径传输状态回调
 * @param [in] onoff 一共两种事件，0表示停止传输，1表示开始传输
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET set_trans_event_cb(int onoff);

#ifdef __cplusplus
}
#endif
#endif /* INCLUDE_TUYA_APP_RT_MAP_SEND_DEMO_H_ */
