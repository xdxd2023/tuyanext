/*********************************************************************************
 *Copyright(C),2015-2020, TUYA www.tuya.comm
 *FileName:    tuya_app_download_demo
  *brief：This article introduces a demo for the download file download feature.
          Developers can directly build upon this demo for further development
          if needed.
 **********************************************************************************/
#include "tuya_app_config.h"
#include "tuya_app_dp_demo.h"
#include "uni_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

/**
 * @brief  语音文件下载回调函数
 * @param [int] result 表示下载结果，返回OPRT_OK表示下载成功，其他表示失败
 * @param [int] voice_id 下载结束后返回的语音ID
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
STATIC OPERATE_RET set_voice_load_cb(int result, int voice_id)
{
    static int last_voice_id = 0; //上次下载的语音ID
    ROBOT_DP_S* dp_set = get_dp_handler_core_sets();
    PR_DEBUG("Callback function called with voice_id: %d", voice_id);
    USE_VOICE_LANGUAGE_RESPONSE_S p_voice_language_set = { 0 };
    p_voice_language_set.id = voice_id;    
    p_voice_language_set.download_status = RVC_DOWNLOAD_ST_USING;
    if (result == OPRT_OK) {         //下载结果成功
        dp_set->voice_id = voice_id; //保存语音ID，这里测试用，暂时放到全局变量中，开发者需要保存到flash中。
        if (last_voice_id != voice_id) {
            last_voice_id = voice_id;
            //开发者将下载到的语音包，进行解压、校验后，把老的本地音文件替换掉，并删除压缩包
            
            p_voice_language_set.percent = 100;
            ty_rvc_voice_language_data_response(&p_voice_language_set, 0); // 替换成功之后，需要开发者上报使用状态。至于下载过程中的进度由SDK内部已经完成
        } else {
            //该语音文件本地已经存在，不需要进行替换，可以做一些提示
            PR_DEBUG("language is now using, don't need download");
            p_voice_language_set.percent = 0;
            ty_rvc_voice_language_data_response(&p_voice_language_set, 0); // 无需替换语音包，也是需要开发者上报使用状态。
        }
    }

    return OPRT_OK;
}

/**
 * @brief  语音文件下载函数
 * @param [*]
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET robotics_svc_download_voice_file(void)
{
    OPERATE_RET ret = 0;
    char cmd[128] = { 0 };
    snprintf(cmd, sizeof(cmd), TY_TMP_VOICE_FILE_PATH);    //开发者需创建一个语音包文件如：tmp_voice.gz，并提供存储的路径，如：/tmp/
    ret = ty_rvc_voice_download_init(cmd, "tmp_voice.gz",set_voice_load_cb);  //开发者调用下载语音初始化接口，注册语音下载回调函数，SDK会自动将语音文件下载到对应的存储路径上
    return ret;
}

/**
 * @brief  地图文件下载回调函数
 * @param [int] result 表示下载结果，返回 OPRT_OK 表示下载成功，其他表示失败
 * @param [int] map_id 下载结束后返回的云端地图ID
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
STATIC OPERATE_RET set_map_load_cb(int result, int map_id)
{
    PR_DEBUG("Callback function called with map_id: %d", map_id);
    if (result == OPRT_OK) {   //下载成功之后，设备端需要给APP回复接口
        USE_CLOUD_MAP_RESPONSE_S map_use_set = { 0 };    //回复需要按照USE_CLOUD_MAP_RESPONSE_S的格式来，并带上map_id
        map_use_set.id = map_id;    
        ty_rvc_map_use_data_response(&map_use_set, 0); // 通知APP端，下载已经成功，用于APP端显示结果
        //注意：下载到的地图文件，开发者自行替换，并做对应的逻辑处理，如更新实时地图上报等。
    }
    return OPRT_OK;
}

/**
 * @brief  地图文件下载函数
 * @param
 * @return [*]
 */
OPERATE_RET robotics_svc_download_map_file(void)
{
    OPERATE_RET ret = 0;
    char cmd[128] = { 0 };
    snprintf(cmd, sizeof(cmd), TY_TMP_MAP_FILE_PATH);   //开发者需创建一个地图文件如：user_map.bin，并提供存储的路径如：/tmp/
    ret = ty_rvc_map_download_init(cmd, "user_map.bin",TRUE, set_map_load_cb);  //开发者调用下载地图初始化接口，注册地图下载回调函数，SDK会自动将地图文件下载到对应的存储路径上
    /*注意：开发者如果需要下载云端的地图数据，请将以上接口的第三个入参填入"TRUE",反之"FALSE"。是否需要下载云端的地图数据，取决与开发者的使用地图的逻辑是如何设计。
           如果开发者地图逻辑是以云端存储为主，那么在APP端点击使用地图后，就应该去云端拉去对应的地图数据，存到本地；如果开发者地图逻辑是以本地存储为主，只需要知道云端的对应地图的ID，
           那么就不需要去云端拉去地图数据。*/
    return ret;
}
