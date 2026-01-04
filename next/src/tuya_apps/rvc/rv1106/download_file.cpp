#include "download_file.h"

#ifdef USE_CURL
#include "curl/curl.h"
typedef std::function<int(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)> curl_info_cb;

curl_info_cb global_progress_callback;
// 包装函数，用于调用 std::function 对象
int progress_callback_wrapper(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    if (global_progress_callback) {
        return global_progress_callback(clientp, dltotal, dlnow, ultotal, ulnow);
    }
    return 0;
}
int download_file(const char *file_name, const char *url, curl_info_cb info_cb)
{
	FILE *fp = fopen(file_name, "wb");
	if(!fp){
		LOGE(TAG, "open file %s fail.\n", file_name);
		return -1;
	}
	CURL *curl = curl_easy_init();
	if(!curl){
		LOGE(TAG, "curl easy init fail.\n");
		fclose(fp);
		return -1;
	}
    global_progress_callback = info_cb;
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(curl, CURLOPT_XFERINFODATA, NULL);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback_wrapper);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    LOGL(TAG);
	CURLcode ret = curl_easy_perform(curl);
	if(ret != CURLE_OK){
		LOGE(TAG, "curl_easy_perform download fail, reason : {}, {}", ret, curl_easy_strerror(ret));
	}
	fclose(fp);
	curl_easy_cleanup(curl);
	curl_global_cleanup();
	return ret;
}

#else
#include "uni_log.h"
#include "tuya_iot_internal_api.h"
#endif


struct DownloadParam {
    download_cb cb;
    FILE* file;
    void *param;
};

int download_file(const char *file_name, const char *url, download_cb cb, std::string &md5)
{
    uint8_t md5_str[32] = {0};
    OPERATE_RET op_ret = OPRT_OK;
    FILE *p_file = NULL;

    p_file = fopen(file_name, "wb");
    if (p_file == NULL) {
        PR_ERR("open file[%s] failed", file_name);
        return -1;
    }
    // 这里没有额外的 user param，暂设置为 NULL；若需要传入自定义 user_data，可扩展函数签名
    DownloadParam dp = {cb, p_file, NULL};
    int file_size = 0;
    op_ret = iot_httpc_download_file(url, 50 * 1024, [](IN PVOID_T pri_data,
    IN CONST UINT_T total_len,
    IN CONST UINT_T offset,
    IN CONST BYTE_T* data,
    IN CONST UINT_T len,
    OUT UINT_T* remain_len){
        PR_DEBUG("download progress: ------------------------------------------ %u/%u-%u", offset, total_len, *remain_len);
        DownloadParam* param = (DownloadParam*)pri_data;
        FILE *p_file = param->file;
        if (p_file) { //判断文件是否存在
            fwrite(data, sizeof(char), len, p_file); //有数据就写入 fp
        } else {
            PR_ERR("file not opened");
        }
        //开发者在这里可以根据下载的进度和下载的结果做 dp 同步，注意不要在该接口中处理，可以通过任务事件传送的方式。
        if(param->cb) {
            param->cb(param->param, total_len, offset);
        }
        return OPRT_OK;
    }, (PVOID_T)&dp, file_size, md5_str);
    if (op_ret != OPRT_OK) {
        PR_ERR("download file failed[%d]", op_ret);
        fclose(p_file);
        return op_ret;
    }
    fclose(p_file);
    //md5 = std::string(md5_str);
    return OPRT_OK;
}