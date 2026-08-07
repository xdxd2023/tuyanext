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


#define TAG "TuyaAI"


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

// #define ENABLE_VPSS
#define ENABLE_RTSP 0

// disp size
int width = DISP_WIDTH;
int height = DISP_HEIGHT;

// model size
int model_width = 640;
int model_height = 640;
float scale;
int leftPadding;
int topPadding;

bool enShowTime = true;
bool enableAi = true;
bool enableFps = true;

void A_SetShowVidoTime(bool power)
{
	enShowTime = power;
}

cv::Mat letterbox(cv::Mat input)
{
	float scaleX = (float)model_width / (float)width;
	float scaleY = (float)model_height / (float)height;
	scale = scaleX < scaleY ? scaleX : scaleY;

	int inputWidth = (int)((float)width * scale);
	int inputHeight = (int)((float)height * scale);

	leftPadding = (model_width - inputWidth) / 2;
	topPadding = (model_height - inputHeight) / 2;

	cv::Mat inputScale;
	cv::resize(input, inputScale, cv::Size(inputWidth, inputHeight), 0, 0, cv::INTER_NEAREST);
	cv::Mat letterboxImage(model_width, model_height, CV_8UC3, cv::Scalar(0, 0, 0));
	cv::Rect roi(leftPadding, topPadding, inputWidth, inputHeight);
	inputScale.copyTo(letterboxImage(roi));

	return letterboxImage;
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
	printf("yoloThread start!\n");
	system("RkLunch-stop.sh");

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

	// h264_frame
	VENC_STREAM_S stFrame;
	stFrame.pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S));
	RK_U32 H264_TimeRef = 0;
	VIDEO_FRAME_INFO_S stViFrame;

	// Create Pool
	MB_POOL src_Pool = media_sys::create_pool(width * height * 3, 1, MB_ALLOC_TYPE_DMA);
	// Get MB from Pool
	MB_BLK src_Blk = media_sys::get_mb(src_Pool, width * height * 3, RK_TRUE);

	// Build h264_frame
	VIDEO_FRAME_INFO_S h264_frame;
	h264_frame.stVFrame.u32Width = width;
	h264_frame.stVFrame.u32Height = height;
	h264_frame.stVFrame.u32VirWidth = width;
	h264_frame.stVFrame.u32VirHeight = height;
	h264_frame.stVFrame.enPixelFormat = RK_FMT_BGR888;
	h264_frame.stVFrame.u32FrameFlag = 160; // FRAME_FLAG_I | FRAME_FLAG_P;
	h264_frame.stVFrame.pMbBlk = src_Blk;
	unsigned char *data = (unsigned char *)media_sys::get_mb_viraddr(src_Blk);
	cv::Mat frame(cv::Size(width, height), CV_8UC3, data);

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
	// if (vpss.init() != RK_SUCCESS)
	// {
	// 	printf("media_vpss init failed!\n");
	// 	return NULL;
	// }
	printf("vpss ---------------------------------------------\n");
	// bind vi and vpss
	// if (media_sys::bind(&vi, &vpss) != RK_SUCCESS)
	// {
	// 	printf("media_sys bind vi and vpss failed!\n");
	// 	return NULL;
	// }
#endif
	

	media_venc venc(0, width, height, RK_VIDEO_ID_AVC);
	if (venc.init() != RK_SUCCESS)
	{
		printf("media_venc init failed!\n");
		return NULL;
	}

	int count = 0;
	media_profile fps;
	uint64_t fpsWindowStartUs = media_sys::get_time_us();
	int fpsFrameCount = 0;
	double displayFps = 0.0;
	while (1)
	{
		// fps.fps("yoloThread fps-----------------------------------------");
		usleep(1000);
		// get vi frame
#ifdef ENABLE_VPSS
		s32Ret = vpss.get_chn_frame(&stViFrame, -1);
#else
  		s32Ret = vi.get_chn_frame(&stViFrame, -1);
#endif
		if (s32Ret == RK_SUCCESS)
		{
			// 计算函数运行时间
			// Start timing
			media_profile profile(true);
			// Process the frame after timing calculation
			void *vi_data = media_sys::get_mb_viraddr(stViFrame.stVFrame.pMbBlk);
			profile.time_ms("get_chn_frame");
#if 1			
			// 从摄像头读取的数据是 YUV420SP 格式
			cv::Mat yuv420sp(height + height / 2, width, CV_8UC1, vi_data);
			// 将 YUV420SP 格式转换为 BGR 格式
			cv::Mat bgr(height, width, CV_8UC3, data);
			cv::cvtColor(yuv420sp, bgr, cv::COLOR_YUV420sp2BGR);
			frame = bgr;
			fpsFrameCount++;
			uint64_t fpsNowUs = media_sys::get_time_us();
			uint64_t fpsElapsedUs = fpsNowUs - fpsWindowStartUs;
			if (fpsElapsedUs >= 1000000)
			{
				displayFps = fpsFrameCount * 1000000.0 / fpsElapsedUs;
				fpsFrameCount = 0;
				fpsWindowStartUs = fpsNowUs;
			}
#endif
#if 1
			// 计算函数运行时间
			profile.time_ms("cvtColor");
			

			uint64_t timeThisMs = media_sys::get_time_ms();
			int      difTime = timeThisMs - aiScanTimeSn;
			LOGD(TAG, "Ai OBS difTime={} ms, enableAi={}", difTime, enableAi);
			if(enableAi && abs(difTime) > 500)
			{
				
				aiScanTimeSn = timeThisMs; 
				// letterbox
				cv::Mat letterboxImage = letterbox(frame);
				memcpy(rknn_app_ctx.input_mems[0]->virt_addr, letterboxImage.data, model_width * model_height * 3);
				// 计算函数运行时间
				profile.time_ms("letterbox");
				inference_yolov5_model(&rknn_app_ctx, &od_results);
				profile.time_ms("inference_yolov5_model");
			}
			if(enableAi)
			{
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

						printf("%s @ (%d %d %d %d) %.3f\n", coco_cls_to_name(det_result->cls_id),
							sX, sY, eX, eY, det_result->prop);

						cv::rectangle(frame, cv::Point(sX, sY),
									cv::Point(eX, eY),
									cv::Scalar(255, 0, 0), 1);

						// memset((uint8_t *)text,0x00,sizeof(text));
						// sprintf(text, "%s", coco_cls_to_name(det_result->cls_id));
						// cv::putText(frame, text, cv::Point(sX+10, sY-10),
						// 			cv::FONT_HERSHEY_COMPLEX, 1,
						// 			cv::Scalar(255, 0, 0), 1);
						//显示文本
						{
							int eYdis = eY * H_DUTY + 1;
							int eXdis = (((sX+eX)/2) + 1)*100/DISP_WIDTH;
							float dis = A_CatchDisTem((float)eYdis);
							float angle = A_CatchAngleTem(eXdis-50);
							if(dis > 100){ dis = 100; }
							if(dis < 0){ dis = 0; }
							sprintf(text, "%s(%d) A=%0.1f D=%0.1f", coco_cls_to_name(det_result->cls_id), (int)(det_result->prop * 100),(angle),dis);
							cv::putText(frame,text,cv::Point(sX, sY - 8),
												cv::FONT_HERSHEY_COMPLEX_SMALL,1,
												cv::Scalar(255,0,0),1);

							// tuya_message::Request req = {0};
							// req.a = det_result->cls_id;
							// req.b = angle;
							// req.c = dis;
							// TuyaComm::Get()->Publish("ai_obs", &req);
							LOGD(TAG, "Ai OBS source eXdiLOGDs={} eYdis={} ",  eXdis , eYdis );
							LOGD(TAG, "Ai OBS Pub id={} angle={} dis={}",  det_result->cls_id, angle, dis);
							if(enableFlow == true && false)
							{
	#if 0
								int direction =4;
								//物体位姿居中，则调整前后距离
								if(eXdis > 45 && eXdis < 55){
									if(dis > 20){
										direction = 0;
										tuya_message::Request req = {direction};
										tuya_message::Result res = {};
										TuyaComm::Get()->Send("ty_direction", &req, &res);
									}else{
										direction = 4;
										tuya_message::Request req = {direction};
										tuya_message::Result res = {};
										TuyaComm::Get()->Send("ty_direction", &req, &res);
									}
								}
								//位姿不居中则调整左右位姿
								else{
									if(eXdis > 55){
										direction = 3;
										tuya_message::Request req = {direction};
										tuya_message::Result res = {};
										TuyaComm::Get()->Send("ty_direction", &req, &res);
									}else{
										direction = 2;
										tuya_message::Request req = {direction};
										tuya_message::Result res = {};
										TuyaComm::Get()->Send("ty_direction", &req, &res);
									}
								}
	#endif
							}
						}
					}
				}
			}
			
			profile.time_ms("draw rectangle");
#endif
			if(enShowTime == true){
				time_t now;
				struct tm *current_time;
				now = time(NULL);
				current_time = localtime(&now);
				memset((uint8_t *)text,0x00,sizeof(text));
				sprintf(text, "%02d:%02d:%02d", current_time->tm_hour,current_time->tm_min,current_time->tm_sec);
				cv::putText(frame, text, cv::Point(DISP_WIDTH-180, 90 ), cv::FONT_HERSHEY_COMPLEX, 1, cv::Scalar(0, 0, 0), 1);
				cv::putText(frame, text, cv::Point(DISP_WIDTH-180-2, 90-2 ), cv::FONT_HERSHEY_COMPLEX, 1, cv::Scalar(255, 255, 255), 1);
			}
			if(enableFps)
			{
				memset((uint8_t *)text,0x00,sizeof(text));
				snprintf(text, sizeof(text), "FPS: %.1f", displayFps);
				cv::putText(frame, text, cv::Point(DISP_WIDTH-180, 125 ), cv::FONT_HERSHEY_COMPLEX, 1, cv::Scalar(0, 0, 0), 1);
				cv::putText(frame, text, cv::Point(DISP_WIDTH-180-2, 125-2 ), cv::FONT_HERSHEY_COMPLEX, 1, cv::Scalar(255, 255, 255), 1);
			}
			// encode h264
			h264_frame.stVFrame.u32TimeRef = H264_TimeRef++;
			h264_frame.stVFrame.u64PTS = media_sys::get_time_us();
			memcpy(data, frame.data, width * height * 3 / 2);
			venc.send_frame(&h264_frame, -1);
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
		s32Ret = venc.release_stream(&stFrame);
		if (s32Ret != RK_SUCCESS)
		{
			RK_LOGE("RK_MPI_VENC_ReleaseStream fail %x", s32Ret);
		}
		memset((uint8_t *)text,0x00,sizeof(text));
	}
	// Destory MB
	media_sys::release_mb(src_Blk);
	// Destory Pool
	media_sys::destroy_pool(src_Pool);

	SAMPLE_COMM_ISP_Stop(0);

	free(stFrame.pstPack);
#if ENABLE_RTSP
	if (g_rtsplive)
		rtsp_del_demo(g_rtsplive);
#endif
	media_sys::deinit();
	// Release rknn model
	release_yolov5_model(&rknn_app_ctx);
	deinit_post_process();
	return 0;
}