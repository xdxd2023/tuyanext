/*********************************************************************************
 *Copyright(C),2015-2020, TUYA www.tuya.comm
 *FileName:    tuya_app_ai_chat_demo
  *brief：AI command callback function, providing AI command types including speech recognition, 
  emotion analysis, motion control, etc.Alternative translation (more technical):
 **********************************************************************************/
#include <stdio.h>
#include <time.h>
#include "tuya_app_config.h"
#include "tuya_ipc_ai_station.h"
#include "utilities/uni_log.h"

/**
 * @brief AI指令回调函数
 * @param[in] cmd AI命令类型包括语音识别、情感分析、移动控制等
 * @param[in] args 命令参数指针
 */
static VOID tuya_rvc_ai_cmd_cb(TUYA_AI_CMD_TYPE_E cmd, CONST PVOID_T args)
{
    switch (cmd) 
    {
        case E_AI_CMD_SERVER_VAD:  // 云端VAD触发
            PR_DEBUG("trigger cloud VAD, audio and video collection ends");
            tuya_ipc_ai_station_stop_act();
        break;
        
        case E_AI_CMD_CHAT_START: {  // AI对话开始
            TUYA_AI_AUDIO_ATTR_T *attr = (TUYA_AI_AUDIO_ATTR_T *)args;
            PR_DEBUG("ai chat start, codec type: %d, sample_rate:%d, channels:%d, bit_depth:%d",
                attr->codec_type, attr->sample_rate, attr->channels, attr->bit_depth);
        }
        break;
        
        case E_AI_CMD_CHAT_FIN:  // AI对话结束
            PR_DEBUG("ai chat finish");
            break;
            
        case E_AI_CMD_MOVE_LEFT:  // 向左转动指令
            PR_DEBUG("please turn left");
            break;
            
        case E_AI_CMD_MOVE_RIGHT:  // 向右转动指令
            PR_DEBUG("please turn right");
            break;
            
        case E_AI_CMD_MOVE_UP:  // 向上转动指令
            PR_DEBUG("please turn upward");
            break;
            
        case E_AI_CMD_MOVE_DOWN:  // 向下转动指令
            PR_DEBUG("please turn downward");
            break;
            
        case E_AI_CMD_MOVE_AROUND:  // 旋转指令
            PR_DEBUG("please turn around");
            break;
            
        case E_AI_CMD_ASR_TEXT: {  // 语音识别文本
            CHAR_T * asr = (CHAR_T *)args;
            PR_DEBUG("ASR TEXT: %s", asr);
        }
        break;
        
        case E_AI_CMD_NLG_TEXT: {  // 自然语言生成文本
            CHAR_T * nlg = (CHAR_T *)args;
            PR_DEBUG("NLG TEXT: %s", nlg);
        }
        break;
        
        case E_AI_CMD_EMOTION: {  // 情感分析结果
            CHAR_T *emo = (CHAR_T *)args;
            PR_DEBUG("emotion: %s", emo);
        }
        break;
        
        default: 
            break; 
    }
    
    return;
}

/**
 * @brief 音频播放回调函数
 * @param[in] buf 音频数据缓冲区
 * @param[in] len 音频数据长度
 * @note 处理音频数据的保存，将音频数据写入PCM文件
 *       当buf为NULL时表示数据结束，关闭文件
 */
static VOID tuya_rvc_audio_play_cb(CONST CHAR_T *buf, UINT_T len)
{
    STATIC FILE *s_audio_file = NULL;  // 音频文件指针
    char audio_file_path[100] = {0};
    extern char s_raw_path[];

    sprintf(audio_file_path, "%s%s", s_raw_path, "/resource");
    // 数据结束处理
    if (buf == NULL) {
        if (s_audio_file) {
            fclose(s_audio_file);
            PR_DEBUG("File closed and saved");
            s_audio_file = NULL;
        }
        return;
    }

    // 首次调用时创建新文件
    if (!s_audio_file) {
        time_t now = time(NULL);
        struct tm* tm_info = localtime(&now);

        // 生成带时间戳的文件名
        char filename[256];
        snprintf(filename, sizeof(filename), "%s/%04d%02d%02d_%02d%02d%02d.pcm",
                audio_file_path,
                tm_info->tm_year + 1900,
                tm_info->tm_mon + 1,
                tm_info->tm_mday,
                tm_info->tm_hour,
                tm_info->tm_min,
                tm_info->tm_sec); 

        // 创建并打开文件
        s_audio_file = fopen(filename, "wb");
        if (!s_audio_file) {
            PR_ERR("failed to open file");
            return;
        }
        PR_DEBUG("file opened: %s\n", filename);
    }

    // 写入音频数据
    fwrite(buf, 1, len, s_audio_file);
    PR_DEBUG("write %zu bytes to file.\n", len);
    return;
}

/**
 * @brief AI助手初始化
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tuya_app_init_AI(void)
{
    PR_DEBUG("tuya_app_init_AI");
#if defined(ENABLE_IPC_AI)&&(ENABLE_IPC_AI==1)
    tuya_ipc_ai_station_init();
    tuya_ipc_ai_chat_init((TUYA_IPC_AI_CMD_CB)tuya_rvc_ai_cmd_cb, (TUYA_IPC_AI_AUDIO_PLAY_CB)tuya_rvc_audio_play_cb);
#endif
    return OPRT_OK;
}