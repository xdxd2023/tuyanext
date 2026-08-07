#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <time.h>
#include <unistd.h>
#include <vector>
#include <libgen.h>
#include "rtsp_demo.h"

#include "yolov5.h"
// #include "arm/ai/postprocess.h"
// #include "arm/ai/luckfox_mpi.h"
#include "luckfox_mpi.h"
#include "postprocess.h"
#include "log_.h"

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include "mpi.hpp"
#include "libai.h"
#include "rk_mpi_rgn.h"
#include "im2d.h"
#include "RgaUtils.h"


#define TAG "TuyaAI"

// RGN OSD 配置
#define RGN_MAX_DETECT_BOX  10       // 最大检测框数量
#define RGN_OVERLAY_HANDLE  100      // 时间 Overlay 的 handle
#define RGN_OVERLAY_WIDTH   256      // Overlay 宽度 (16 对齐)
#define RGN_OVERLAY_HEIGHT  48       // Overlay 高度 (16 对齐)

// #define ENABLE_VPSS   // 板子 RGA 驱动问题导致 VPSS 通道无输出 (BUF_EMPTY),改用 VI 直出
// #define ENABLE_RTSP 1
#define ENABLE_AI
#define ENABLE_ENCODE
// RK1106 RGA 支持 YUV->RGBA 转换
#define USE_RGA_ACCELERATION 1

// 帧率限制: 降低 CPU 占用 (可按需调整, 15~30)
#define TARGET_FPS  20

typedef struct
{
	int    	DISP_WIDTH;
	int		DISP_HEIGHT;
	double aY;
	double bY;
	double cY;
	double dY;
	double aX;
	double bX;
	double cX;
}AiCalParSt_;
AiCalParSt_ AiCalParSt={0};

// #define DISP_WIDTH 720
// #define DISP_HEIGHT 480

int DISP_WIDTH = 1280;
int DISP_HEIGHT = 720;

float W_DUTY = 0;	//720.00/DISP_WIDTH
float H_DUTY = 0;	//480.00/DISP_HEIGHT

// #define DISP_WIDTH 1920
// #define DISP_HEIGHT 1080



// disp size
int width = DISP_WIDTH;
int height = DISP_HEIGHT;

// model size
int model_width = 640;
int model_height = 640;
float scale;
int leftPadding;
int topPadding;


bool _enableAI = false;
bool _enableEncode = false;
bool _enableTime = true;
bool _enableFPS = true;

void aiEnableProfile(AI_PROFILE_TYPE_E profile,bool enable)
{
	switch(profile)
	{
		case PROFILE_AI:
			_enableAI = enable;
			break;
		case PROFILE_ENCODE:
			_enableEncode = enable;
			break;
		case PROFILE_TIME:
			_enableTime = enable;
			break;
		case PROFILE_FPS:
			_enableFPS = enable;
			break;
		default:
			break;
	}
}
bool aiIsProfileEnabled(AI_PROFILE_TYPE_E profile)
{
	switch(profile)
	{
		case PROFILE_AI:
			return _enableAI;
		case PROFILE_ENCODE:
			return _enableEncode;
		case PROFILE_TIME:
			return _enableTime;
		case PROFILE_FPS:
			return _enableFPS;
		default:
			return false;
	}
}
// cv::Mat letterbox(cv::Mat input)
// {
// 	float scaleX = (float)model_width / (float)width;
// 	float scaleY = (float)model_height / (float)height;
// 	scale = scaleX < scaleY ? scaleX : scaleY;

// 	int inputWidth = (int)((float)width * scale);
// 	int inputHeight = (int)((float)height * scale);

// 	leftPadding = (model_width - inputWidth) / 2;
// 	topPadding = (model_height - inputHeight) / 2;

// 	cv::Mat inputScale;
// 	cv::resize(input, inputScale, cv::Size(inputWidth, inputHeight), 0, 0, cv::INTER_NEAREST);
// 	cv::Mat letterboxImage(model_width, model_height, CV_8UC3, cv::Scalar(0, 0, 0));
// 	cv::Rect roi(leftPadding, topPadding, inputWidth, inputHeight);
// 	inputScale.copyTo(letterboxImage(roi));

// 	return letterboxImage;
// }

cv::Mat letterbox(const cv::Mat& input)
{
    int width = input.cols;
    int height = input.rows;

    float scale = std::min(
        (float)model_width / width,
        (float)model_height / height
    );

    int newW = cvRound(width * scale);
    int newH = cvRound(height * scale);

    int padW = (model_width - newW) / 2;
    int padH = (model_height - newH) / 2;

    static cv::Mat resized;
    static cv::Mat output;

    resized.create(newH, newW, CV_8UC3);
    output.create(model_height, model_width, CV_8UC3);
    output.setTo(cv::Scalar(0,0,0));

    cv::resize(input, resized, cv::Size(newW, newH), 0, 0, cv::INTER_NEAREST);

    resized.copyTo(output(cv::Rect(padW, padH, newW, newH)));

    return output;
}



void mapCoordinates(int *x, int *y)
{
	int mx = *x - leftPadding;
	int my = *y - topPadding;

	*x = (int)((float)mx / scale);
	*y = (int)((float)my / scale);
}

int test_vpss_init(int VpssChn, int width, int height) {
	printf("================================%s==================================\n",
	       __func__);
	int s32Ret;
	VPSS_CHN_ATTR_S stVpssChnAttr;
	VPSS_GRP_ATTR_S stGrpVpssAttr;

	int s32Grp = 0;

	stGrpVpssAttr.u32MaxW = 4096;
	stGrpVpssAttr.u32MaxH = 4096;
	stGrpVpssAttr.enPixelFormat = RK_FMT_YUV420SP;
	stGrpVpssAttr.stFrameRate.s32SrcFrameRate = -1;
	stGrpVpssAttr.stFrameRate.s32DstFrameRate = -1;
	stGrpVpssAttr.enCompressMode = COMPRESS_MODE_NONE;

	stVpssChnAttr.enChnMode = VPSS_CHN_MODE_USER;
	stVpssChnAttr.enDynamicRange = DYNAMIC_RANGE_SDR8;
	stVpssChnAttr.enPixelFormat = RK_FMT_BGR888;
	stVpssChnAttr.stFrameRate.s32SrcFrameRate = -1;
	stVpssChnAttr.stFrameRate.s32DstFrameRate = -1;
	stVpssChnAttr.u32Width = width;
	stVpssChnAttr.u32Height = height;
	stVpssChnAttr.enCompressMode = COMPRESS_MODE_NONE;

	s32Ret = RK_MPI_VPSS_CreateGrp(s32Grp, &stGrpVpssAttr);
	if (s32Ret != RK_SUCCESS) {
		return s32Ret;
	}

	s32Ret = RK_MPI_VPSS_SetChnAttr(s32Grp, VpssChn, &stVpssChnAttr);
	if (s32Ret != RK_SUCCESS) {
		return s32Ret;
	}
	s32Ret = RK_MPI_VPSS_EnableChn(s32Grp, VpssChn);
	if (s32Ret != RK_SUCCESS) {
		return s32Ret;
	}

	s32Ret = RK_MPI_VPSS_StartGrp(s32Grp);
	if (s32Ret != RK_SUCCESS) {
		return s32Ret;
	}
	printf("VPSS init success ===========================\n");
	return s32Ret;
}

media_vpss vpss(0, width, height);

//根据像素推算物体距离 直线拟合
float A_CatchDisTem(float yData)
{
	//计算距离	
	// double a = 2913.13838259309;
	// double b = -23.1301824871146;
	// double c = 0.062136978150688;
	// double d = -5.59458230641153E-05;
	
	double a = AiCalParSt.aY;
	double b = AiCalParSt.bY;
	double c = AiCalParSt.cY;
	double d = AiCalParSt.dY;

	double X = yData ;
	return a + b*X + c*X*X+ d*X*X*X;
}

//根据像素推算物体距离 直线拟合
float A_CatchAngleTem(float xData)
{
	//计算距离	
	// double a = 9.49999999999999;
	// double b = -0.0416666666666636;
	// double c = 0.0208333333333332;

	double a = AiCalParSt.aX;
	double b = AiCalParSt.bX;
	double c = AiCalParSt.cX;

	double X = fabs(xData) ;
	return (a + b*X + c*X*X) * ((xData>0)?(1):(-1));
}

bool enableFlow = false;

void SetAiFlowPower(bool power)
{
	enableFlow = power;
}

// ==================== RGA 硬件加速函数 ====================
#if USE_RGA_ACCELERATION
// RGA 硬件加速：YUV420SP -> RGBA -> resize -> BGR (YOLO 输入)
// RK1106 RGA 支持 YUV->RGBA 转换，但不支持直接转 BGR/RGB
// 输出 letterbox 的 padding 和 scale 参数供坐标反算使用
static int rga_letterbox(void* src_yuv, int src_w, int src_h,
                         void* dst_bgr, int dst_w, int dst_h,
                         int* left_padding, int* top_padding, float* out_scale) {
    // 计算缩放比例和 padding
    float scaleX = (float)dst_w / (float)src_w;
    float scaleY = (float)dst_h / (float)src_h;
    float scale_factor = scaleX < scaleY ? scaleX : scaleY;
    
    int scaled_w = (int)((float)src_w * scale_factor);
    int scaled_h = (int)((float)src_h * scale_factor);
    
    *left_padding = (dst_w - scaled_w) / 2;
    *top_padding = (dst_h - scaled_h) / 2;
    *out_scale = scale_factor;
    
    // 分配临时 RGBA 缓冲区用于 CSC (RK1106 RGA 支持 YUV->RGBA)
    int rgba_size = src_w * src_h * 4;
    void* tmp_rgba = malloc(rgba_size);
    if (!tmp_rgba) {
        printf("RGA: malloc tmp_rgba failed\n");
        return -1;
    }
    
    // 第一步：YUV420SP -> RGBA (颜色空间转换，RK1106 支持)
    rga_buffer_t src_buf = wrapbuffer_virtualaddr(src_yuv, src_w, src_h, RK_FORMAT_YCbCr_420_SP);
    rga_buffer_t tmp_buf = wrapbuffer_virtualaddr(tmp_rgba, src_w, src_h, RK_FORMAT_RGBA_8888);
    
    IM_STATUS status = imcvtcolor(src_buf, tmp_buf, RK_FORMAT_YCbCr_420_SP, RK_FORMAT_RGBA_8888);
    if (status != IM_STATUS_SUCCESS) {
        printf("RGA imcvtcolor YUV->RGBA failed: %s\n", imStrError(status));
        free(tmp_rgba);
        return -1;
    }
    
    // 第二步：resize RGBA 到目标大小
    int resize_rgba_size = scaled_w * scaled_h * 4;
    void* resize_rgba = malloc(resize_rgba_size);
    if (!resize_rgba) {
        printf("RGA: malloc resize_rgba failed\n");
        free(tmp_rgba);
        return -1;
    }
    
    rga_buffer_t resize_src = wrapbuffer_virtualaddr(tmp_rgba, src_w, src_h, RK_FORMAT_RGBA_8888);
    rga_buffer_t resize_dst = wrapbuffer_virtualaddr(resize_rgba, scaled_w, scaled_h, RK_FORMAT_RGBA_8888);
    
    status = imresize(resize_src, resize_dst);
    if (status != IM_STATUS_SUCCESS) {
        printf("RGA imresize failed: %s\n", imStrError(status));
        free(tmp_rgba);
        free(resize_rgba);
        return -1;
    }
    
    free(tmp_rgba);
    
    // 第三步：RGBA -> BGR 并复制到目标位置 (带 padding)
    // YOLO 需要 BGR 格式，所以需要转换并去掉 Alpha 通道
    // 清空目标缓冲区 (黑色 padding)
    memset(dst_bgr, 0, dst_w * dst_h * 3);
    
    unsigned char* dst_ptr = (unsigned char*)dst_bgr;
    unsigned char* rgba_ptr = (unsigned char*)resize_rgba;
    
    for (int y = 0; y < scaled_h; y++) {
        unsigned char* dst_row = dst_ptr + ((y + *top_padding) * dst_w + *left_padding) * 3;
        unsigned char* src_row = rgba_ptr + y * scaled_w * 4;
        for (int x = 0; x < scaled_w; x++) {
            // RGBA -> BGR: R=src[0], G=src[1], B=src[2], A=src[3]
            dst_row[x * 3 + 0] = src_row[x * 4 + 2];  // B
            dst_row[x * 3 + 1] = src_row[x * 4 + 1];  // G
            dst_row[x * 3 + 2] = src_row[x * 4 + 0];  // R
        }
    }
    
    free(resize_rgba);
    
    return 0;
}
#endif

// ==================== RGN OSD 辅助函数 ====================

// 创建 Overlay RGN (用于文字叠加)
static int rgn_create_overlay(RGN_HANDLE handle, int vencChn, int w, int h, int posX, int posY) {
	RGN_ATTR_S stRgnAttr;
	RGN_CHN_ATTR_S stChnAttr;
	MPP_CHN_S stChn;
	RK_S32 s32Ret;

	memset(&stRgnAttr, 0, sizeof(stRgnAttr));
	stRgnAttr.enType = OVERLAY_RGN;
	stRgnAttr.unAttr.stOverlay.enPixelFmt = RK_FMT_ARGB8888;
	stRgnAttr.unAttr.stOverlay.stSize.u32Width = w;
	stRgnAttr.unAttr.stOverlay.stSize.u32Height = h;
	stRgnAttr.unAttr.stOverlay.u32CanvasNum = 2;

	s32Ret = RK_MPI_RGN_Create(handle, &stRgnAttr);
	if (s32Ret != RK_SUCCESS) {
		printf("RK_MPI_RGN_Create OVERLAY %d failed: %x\n", handle, s32Ret);
		return s32Ret;
	}

	stChn.enModId = RK_ID_VENC;
	stChn.s32DevId = 0;
	stChn.s32ChnId = vencChn;

	memset(&stChnAttr, 0, sizeof(stChnAttr));
	stChnAttr.bShow = RK_TRUE;
	stChnAttr.enType = OVERLAY_RGN;
	stChnAttr.unChnAttr.stOverlayChn.stPoint.s32X = (posX / 16) * 16;  // 16 对齐
	stChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y = (posY / 16) * 16;
	stChnAttr.unChnAttr.stOverlayChn.u32FgAlpha = 255;
	stChnAttr.unChnAttr.stOverlayChn.u32BgAlpha = 128;  // 半透明背景
	stChnAttr.unChnAttr.stOverlayChn.u32Layer = 0;

	s32Ret = RK_MPI_RGN_AttachToChn(handle, &stChn, &stChnAttr);
	if (s32Ret != RK_SUCCESS) {
		printf("RK_MPI_RGN_AttachToChn OVERLAY %d failed: %x\n", handle, s32Ret);
	}
	return s32Ret;
}

// 更新 Overlay 内容 (使用 OpenCV 绘制后更新)
static int rgn_update_overlay(RGN_HANDLE handle, cv::Mat& bgraImg) {
	RGN_CANVAS_INFO_S stCanvasInfo;
	RK_S32 s32Ret;

	s32Ret = RK_MPI_RGN_GetCanvasInfo(handle, &stCanvasInfo);
	if (s32Ret != RK_SUCCESS) {
		return s32Ret;
	}

	memcpy((void*)stCanvasInfo.u64VirAddr, bgraImg.data, 
		   bgraImg.cols * bgraImg.rows * 4);

	return RK_MPI_RGN_UpdateCanvas(handle);
}

// 销毁 RGN
static void rgn_destroy(RGN_HANDLE handle, int vencChn) {
	MPP_CHN_S stChn;
	stChn.enModId = RK_ID_VENC;
	stChn.s32DevId = 0;
	stChn.s32ChnId = vencChn;

	RK_MPI_RGN_DetachFromChn(handle, &stChn);
	RK_MPI_RGN_Destroy(handle);
}

char* ai_get_current_process_dir(char* buf, size_t size) {
    ssize_t len = readlink("/proc/self/exe", buf, size - 1);
    if (len != -1) {
        buf[len] = '\0';
        dirname(buf); // modifies buf in place
        return buf;
    }
    return NULL;
}

static char *readLine(FILE *fp, char *buffer, int *len)
{
    int ch;
    int i = 0;
    size_t buff_len = 0;

    buffer = (char *)malloc(buff_len + 1);
    if (!buffer)
        return NULL; // Out of memory

    while ((ch = fgetc(fp)) != '\n' && ch != EOF && ch != '/0')
    {
        buff_len++;
        void *tmp = realloc(buffer, buff_len + 1);
        if (tmp == NULL)
        {
            free(buffer);
            return NULL; // Out of memory
        }
        buffer = (char *)tmp;

        buffer[i] = (char)ch;
        i++;
    }
    buffer[i] = '\0';

    *len = buff_len;

    // Detect end
    if (ch == EOF && (i == 0 || ferror(fp)))
    {
        free(buffer);
        return NULL;
    }
    return buffer;
}

static int readLines(const char *fileName, char *par[], int max_line)
{
    FILE *file = fopen(fileName, "r");
    char *s;
    int i = 0;
    int n = 0;

    if (file == NULL)
    {
        printf("Open %s fail!\n", fileName);
        return -1;
    }

    while ((s = readLine(file, s, &n)) != NULL)
    {
        par[i++] = s;
        if (i >= max_line)
            break;
    }
    fclose(file);
    return i;
}

static int loadPar(const char *locationFilename, AiCalParSt_ *st)
{
	char *parBuf[20];
    printf("load ai Par %s\n", locationFilename);
    int numCnt = readLines(locationFilename, parBuf, 20);
    printf("\r\n===============================================================================================\r\n");
    printf("load ai par num =  %d\n", numCnt);

    char *ptr;
	st->aY = strtod(parBuf[0], &ptr);
	st->bY = strtod(parBuf[1], &ptr);
	st->cY = strtod(parBuf[2], &ptr);
	st->dY = strtod(parBuf[3], &ptr);
	st->aX = strtod(parBuf[4], &ptr);
	st->bX = strtod(parBuf[5], &ptr);
	st->cX = strtod(parBuf[6], &ptr);

	st->DISP_WIDTH  = strtod(parBuf[7], &ptr);
	st->DISP_HEIGHT = strtod(parBuf[8], &ptr);

	printf("%f\r\n",st->aY);
	printf("%f\r\n",st->bY);
	printf("%f\r\n",st->cY);
	printf("%f\r\n",st->dY);
	printf("%f\r\n",st->aX);
	printf("%f\r\n",st->bX);
	printf("%f\r\n",st->cX);
	printf("%d\r\n",st->DISP_WIDTH);
	printf("%d\r\n",st->DISP_HEIGHT);
    return 0;
}

void *yoloThread(void *argv)
{
	LOGD(TAG, "yoloThread start!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	//system("RkLunch-stop.sh");

    char  cfg_path[256] = {0};
    ai_get_current_process_dir(cfg_path, 256);
    snprintf(cfg_path + strlen(cfg_path), 256, "/ai_calibration_parameters.json", optarg);
  
    if (0 != loadPar(cfg_path,&AiCalParSt))
    {
        printf("ai cfg parse failed\n");
		DISP_WIDTH = 1280;
		DISP_HEIGHT = 720;
		W_DUTY = 720.00/DISP_WIDTH;
		H_DUTY = 480.00/DISP_HEIGHT;
    }else{
		DISP_WIDTH 	= AiCalParSt.DISP_WIDTH;
		DISP_HEIGHT = AiCalParSt.DISP_HEIGHT;
		W_DUTY = 720.00/DISP_WIDTH;
		H_DUTY = 480.00/DISP_HEIGHT;
	}
	width = DISP_WIDTH;
	height = DISP_HEIGHT;
	printf("vido config width=%d high = %dd\r\n",DISP_WIDTH,DISP_HEIGHT);

	fn_video_cb cb = (fn_video_cb)argv;

	RK_S32 s32Ret = 0;
	int sX, sY, eX, eY;

	uint64_t aiScanTimeSn = 0;

	// venc 编码输出流
	VENC_STREAM_S stFrame;
	stFrame.pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S));
	RK_U32 H264_TimeRef = 0;
	VIDEO_FRAME_INFO_S stViFrame;

	// 用于 AI 推理和绘制的 BGR 图像
	cv::Mat frame(cv::Size(width, height), CV_8UC3);

	printf("init rknn\n");
	// Rknn model
	char text[64];
	rknn_app_context_t rknn_app_ctx;
	object_detect_result_list od_results;
	const char *model_path = "./model/yolov5.rknn";
	memset(&rknn_app_ctx, 0, sizeof(rknn_app_context_t));
	init_yolov5_model(model_path, &rknn_app_ctx);
	printf("init rknn model success!\n");
	init_post_process();

	// rkaiq init
	RK_BOOL multi_sensor = RK_FALSE;
	const char *iq_dir = "/etc/iqfiles";
	rk_aiq_working_mode_t hdr_mode = RK_AIQ_WORKING_MODE_NORMAL;
	// hdr_mode = RK_AIQ_WORKING_MODE_ISP_HDR2;
	SAMPLE_COMM_ISP_Init(0, hdr_mode, multi_sensor, iq_dir);
	SAMPLE_COMM_ISP_Run(0);

#ifdef ENABLE_RTSP
	// rtsp init	
	rtsp_demo_handle g_rtsplive = NULL;
	rtsp_session_handle g_rtsp_session;
	g_rtsplive = create_rtsp_demo(554);
	g_rtsp_session = rtsp_new_session(g_rtsplive, "/live/0");
	rtsp_set_video(g_rtsp_session, RTSP_CODEC_ID_VIDEO_H264, NULL, 0);
	rtsp_sync_video_ts(g_rtsp_session, rtsp_get_reltime(), rtsp_get_ntptime());
#endif

	// rkmpi init
	if (media_sys::init() != RK_SUCCESS)
	{
		RK_LOGE("rk mpi sys init fail!");
		return NULL;
	}

	// 编码直接使用 VI 帧, 无需 MB pool 和每帧 memcpy

	media_vi vi(0, width, height);
	if (vi.init() != RK_SUCCESS)
	{
		printf("media_vi init failed!\n");
		return NULL;
	}
	// vpss init
	//
	//test_vpss_init(0, width, height);
#ifdef ENABLE_VPSS
	if (vpss.init() != RK_SUCCESS)
	{
		printf("media_vpss init failed!\n");
		return NULL;
	}
	printf("vpss ---------------------------------------------\n");
	// bind vi and vpss
	if (media_sys::bind(&vi, &vpss) != RK_SUCCESS)
	{
		printf("media_sys bind vi and vpss failed!\n");
		return NULL;
	}
#endif
	

	media_venc venc(0, width, height, RK_VIDEO_ID_AVC);
	if (venc.init() != RK_SUCCESS)
	{
		printf("media_venc init failed!\n");
		return NULL;
	}

#ifdef ENABLE_ENCODE

	// 创建时间显示 Overlay
	
	int rgnOverlayW = width/16*16;
	int rgnOverlayH = height/16*16;
	int overlayPosX = (width - rgnOverlayW)/2;
	int overlayPosY = (height - rgnOverlayH)/2;
	rgn_create_overlay(RGN_OVERLAY_HANDLE, 0, rgnOverlayW, rgnOverlayH, overlayPosX, overlayPosY);
	
	// 用于绘制时间的 BGRA 图像
	cv::Mat osdTimeMat(rgnOverlayH, rgnOverlayW, CV_8UC4, cv::Scalar(0, 0, 0, 128));
	
	printf("RGN OSD init success!\n");
#endif

	int count = 0;
	media_profile fps;
	int fpsLast = 0;
	uint64_t lastOsdUpdateMs = 0;   // OSD 文本刷新节流时间戳
	while (1)
	{
		uint64_t frameStartUs = media_sys::get_time_us();   // 帧率限制用
		// fps.fps("yoloThread fps-----------------------------------------");
		// get vi frame
		bool enableAI = _enableAI;
		bool enableEncode = _enableEncode;
		bool enableTime = _enableTime;
		bool enableFPS = _enableFPS;

#ifdef ENABLE_VPSS
		s32Ret = vpss.get_chn_frame(&stViFrame, -1);
#else
  		s32Ret = vi.get_chn_frame(&stViFrame, -1);
#endif
		if (s32Ret == RK_SUCCESS)
		{
			// 计算函数运行时间
			// Start timing
			media_profile profile(false);
			// Process the frame after timing calculation
			void *vi_data = media_sys::get_mb_viraddr(stViFrame.stVFrame.pMbBlk);
			profile.time_ms("get_chn_frame");
#if 1			
		
#endif
			// 计算函数运行时间
			profile.time_ms("cvtColor");
			

			uint64_t timeThisMs = media_sys::get_time_ms();
			int      difTime = timeThisMs - aiScanTimeSn;
			bool     osdDirty = false;   // 本帧 OSD 是否有绘制内容
			// OSD 文本(时间/FPS)每秒才变,降低刷新频率(5Hz)节省 CPU; AI 画框时强制刷新
			bool     osdNeedUpdate = (timeThisMs - lastOsdUpdateMs >= 200);
			if(abs(difTime) > 500)
			{
				// 清屏: AI 帧(2Hz)或文本刷新节拍(5Hz)时才清,避免每帧 3.7MB
				if (enableAI || osdNeedUpdate)
				{
					osdTimeMat.setTo(cv::Scalar(0, 0, 0, 128));  // 半透明黑色背景
				}
				if(enableAI)
				{
					osdNeedUpdate = true;   // AI 画框需要立即刷新 OSD
#if USE_RGA_ACCELERATION
					// 使用 RGA 硬件加速：YUV420SP -> BGR + letterbox
					unsigned char* letterbox_data = (unsigned char*)rknn_app_ctx.input_mems[0]->virt_addr;
					int ret = rga_letterbox(vi_data, width, height, 
											letterbox_data, model_width, model_height,
											&leftPadding, &topPadding, &scale);
					if (ret != 0) {
						printf("RGA letterbox failed, fallback to OpenCV\n");
						// 回退到 OpenCV
						cv::Mat yuv420sp(height + height / 2, width, CV_8UC1, vi_data);
						cv::Mat bgr(height, width, CV_8UC3);
						cv::cvtColor(yuv420sp, bgr, cv::COLOR_YUV420sp2BGR);
						cv::Mat letterboxImage = letterbox(bgr);
						memcpy(letterbox_data, letterboxImage.data, model_width * model_height * 3);
					}
					aiScanTimeSn = timeThisMs;
					profile.time_ms("RGA letterbox");
#else
					// 使用 OpenCV: YUV420SP -> BGR + letterbox
					cv::Mat yuv420sp(height + height / 2, width, CV_8UC1, vi_data);
					cv::Mat bgr(height, width, CV_8UC3);
					cv::cvtColor(yuv420sp, bgr, cv::COLOR_YUV420sp2BGR);
					profile.time_ms("OpenCV cvtColor");
					frame = bgr;
					aiScanTimeSn = timeThisMs;
					cv::Mat letterboxImage = letterbox(frame);
					memcpy(rknn_app_ctx.input_mems[0]->virt_addr, letterboxImage.data, model_width * model_height * 3);
					profile.time_ms("OpenCV letterbox");
#endif
					inference_yolov5_model(&rknn_app_ctx, &od_results);
					profile.time_ms("inference_yolov5_model");
					
					for (int i = 0; i < od_results.count; i++)
					{
						if (od_results.count >= 1 && (od_results.results[i].prop * 100) > 60)
						{
							object_detect_result *det_result = &(od_results.results[i]);

							sX = (int)(det_result->box.left);
							sY = (int)(det_result->box.top);
							eX = (int)(det_result->box.right);
							eY = (int)(det_result->box.bottom);
							mapCoordinates(&sX, &sY);
							mapCoordinates(&eX, &eY);

							// printf("%s @ (%d %d %d %d) %.3f\n", coco_cls_to_name(det_result->cls_id),
							// 	sX, sY, eX, eY, det_result->prop);

	#ifdef ENABLE_ENCODE
							// 使用 RGN 硬件画框
							cv::rectangle(osdTimeMat, cv::Point(sX, sY),
										cv::Point(eX, eY),
										cv::Scalar(255, 0, 255, 0), 2);

	#else
							cv::rectangle(frame, cv::Point(sX, sY),
										cv::Point(eX, eY),
										cv::Scalar(255, 0, 0), 1);
	#endif

							
							//显示文本
							{
								int eYdis = eY * H_DUTY + 1;
								int eXdis = (((sX+eX)/2) + 1)*100/DISP_WIDTH;
								float dis = A_CatchDisTem((float)eYdis);
								float angle = A_CatchAngleTem(eXdis-50);
								if(dis > 100){ dis = 100; }
								if(dis < 0){ dis = 0; }
								sprintf(text, "%s(%d) A=%0.1f D=%0.1f", coco_cls_to_name(det_result->cls_id), (int)(det_result->prop * 100),(angle),dis);
	#ifdef ENABLE_ENCODE
								cv::putText(osdTimeMat, text, cv::Point(sX+5, sY-10),
											cv::FONT_HERSHEY_COMPLEX, 1,
											cv::Scalar(255, 0, 255, 0), 2);
	#else
								cv::putText(frame, text, cv::Point(sX+5, sY-10),
											cv::FONT_HERSHEY_COMPLEX, 1,
											cv::Scalar(0, 255, 0), 1);
	#endif	
								// tuya_message::Request req = {0};
								// req.a = det_result->cls_id;make
								// req.b = angle;
								// req.c = dis;
								// TuyaComm::Get()->Publish("ai_obs", &req);
								LOGD(TAG, "Ai OBS source eXdiLOGDs={} eYdis={} ",  eXdis , eYdis );
								LOGD(TAG, "Ai OBS Pub id={} angle={} dis={}",  det_result->cls_id, angle, dis);
							}
						}
					}
					profile.time_ms("draw rectangle");
				}
				if(enableTime == true && osdNeedUpdate){
					time_t now;
					struct tm *current_time;
					now = time(NULL);
					current_time = localtime(&now);
					memset((uint8_t *)text,0x00,sizeof(text));
					sprintf(text, "%02d:%02d:%02d", current_time->tm_hour,current_time->tm_min,current_time->tm_sec);
#ifdef ENABLE_ENCODE
					// 使用 RGN Overlay 显示时间
					cv::putText(osdTimeMat, text, cv::Point(rgnOverlayW-180, 90), 
								cv::FONT_HERSHEY_SIMPLEX, 1.0, 
								cv::Scalar(0, 0, 0, 255), 2);  // 白色文字
					cv::putText(osdTimeMat, text, cv::Point(rgnOverlayW-180-2, 90-2), 
								cv::FONT_HERSHEY_SIMPLEX, 1.0, 
								cv::Scalar(255, 255, 255, 255), 2);  // 白色文字
					osdDirty = true;
#else
					cv::putText(frame, text, cv::Point(DISP_WIDTH-180, 90 ), cv::FONT_HERSHEY_COMPLEX, 1, cv::Scalar(0, 0, 0), 1);
					cv::putText(frame, text, cv::Point(DISP_WIDTH-180-2, 90-2 ), cv::FONT_HERSHEY_COMPLEX, 1, cv::Scalar(255, 255, 255), 1);
#endif
				}
			}
			if(enableFPS)
			{
				int f = fps.fps("yoloThread fps-----------------------------------------");
				if (f > 0) { fpsLast = f; }  // 每秒更新一次,中间保持上次值避免闪烁
				if (osdNeedUpdate)   // 文本 5Hz 刷新, 降低 CPU
				{
					memset((uint8_t *)text,0x00,sizeof(text));
					sprintf(text, "FPS: %d", fpsLast);
#ifdef ENABLE_ENCODE
					// 使用 RGN Overlay 显示 FPS
					cv::putText(osdTimeMat, text, cv::Point(rgnOverlayW-180, 150), 
								cv::FONT_HERSHEY_SIMPLEX, 1.0, 
								cv::Scalar(0, 0, 0, 255), 2);  // 黑色文字
					cv::putText(osdTimeMat, text, cv::Point(rgnOverlayW-180-2, 150-2), 
								cv::FONT_HERSHEY_SIMPLEX, 1.0, 
								cv::Scalar(255, 255, 255, 255), 2);  // 白色文字
					osdDirty = true;
#else
					cv::putText(frame, text, cv::Point(DISP_WIDTH-180, 150), cv::FONT_HERSHEY_COMPLEX, 1, cv::Scalar(0, 0, 0), 1);
					cv::putText(frame, text, cv::Point(DISP_WIDTH-180-2, 150-2), cv::FONT_HERSHEY_COMPLEX, 1, cv::Scalar(255, 255, 255), 1);
#endif
				}
			}
#ifdef ENABLE_ENCODE
			// OSD 文本 5Hz 刷新一次, AI 画框时强制刷新, 降低 CPU
			if (osdDirty && osdNeedUpdate)
			{
				rgn_update_overlay(RGN_OVERLAY_HANDLE, osdTimeMat);
				lastOsdUpdateMs = timeThisMs;
			}
#endif
			// encode h264
#ifdef ENABLE_ENCODE
			if(enableEncode)
			{
				// 直接把 VI 帧(YUV420SP)喂给编码器, 省掉每帧 1.38MB memcpy 和 MB pool
				stViFrame.stVFrame.u32TimeRef = H264_TimeRef++;
				stViFrame.stVFrame.u64PTS = media_sys::get_time_us();
				venc.send_frame(&stViFrame, -1);
				s32Ret = venc.get_stream(&stFrame, -1);
				if (s32Ret == RK_SUCCESS)
				{
					void *pData = media_sys::get_mb_viraddr(stFrame.pstPack->pMbBlk);
					if (cb)
					{
						// Call the callback function with the video frame data
						cb((uint8_t *)pData, stFrame.pstPack->u32Len, stFrame.pstPack->u64PTS, stFrame.stH264Info.enRefType);
					}
	#ifdef ENABLE_RTSP
				// rtsp

					if(g_rtsplive && g_rtsp_session)
					{
						//printf("len = %d PTS = %d \n",stFrame.pstPack->u32Len, stFrame.pstPack->u64PTS);	
						rtsp_tx_video(g_rtsp_session, (uint8_t *)pData, stFrame.pstPack->u32Len,
									stFrame.pstPack->u64PTS);
						rtsp_do_event(g_rtsplive);
					}
	#endif
				}
				profile.time_ms("encode h264");
			}	
		}
		// 帧率限制, 降低 CPU 占用: 把循环控制在 TARGET_FPS
		{
			uint64_t frameElapsedUs = media_sys::get_time_us() - frameStartUs;
			uint64_t frameIntervalUs = 1000000 / TARGET_FPS;
			if (frameElapsedUs < frameIntervalUs)
			{
				usleep(frameIntervalUs - frameElapsedUs);
			}
		}
#ifdef ENABLE_VPSS
		s32Ret = vpss.release_chn_frame(&stViFrame);
		if (s32Ret != RK_SUCCESS)
		{
			RK_LOGE("RK_MPI_VPSS_ReleaseChnFrame fail %x", s32Ret);
		}
#else
		// release frame
		s32Ret = vi.release_chn_frame(&stViFrame);
		if (s32Ret != RK_SUCCESS)
		{
			RK_LOGE("RK_MPI_VI_ReleaseChnFrame fail %x", s32Ret);
		}
#endif
		if(enableEncode)
		{
			s32Ret = venc.release_stream(&stFrame);
			if (s32Ret != RK_SUCCESS)
			{
				RK_LOGE("RK_MPI_VENC_ReleaseStream fail %x", s32Ret);
			}
		}
#endif
		memset((uint8_t *)text,0x00,sizeof(text));
	}
	SAMPLE_COMM_ISP_Stop(0);

#ifdef ENABLE_ENCODE
	rgn_destroy(RGN_OVERLAY_HANDLE, 0);
#endif

	free(stFrame.pstPack);
#ifdef ENABLE_RTSP
	if (g_rtsplive)
		rtsp_del_demo(g_rtsplive);
#endif
	media_sys::deinit();
	// Release rknn model
	release_yolov5_model(&rknn_app_ctx);
	deinit_post_process();
	return 0;
}
