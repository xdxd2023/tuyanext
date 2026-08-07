#ifndef _MPI_HPP_
#define _MPI_HPP_
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

#include "sample_comm.h"

class media_hw
{
protected:
    int modid = 0;
    int devid = 0;
    int chnid = 0;

    friend class media_sys;
};

class media_sys
{

public:
    static uint64_t get_time_ms()
    {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (uint64_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
    }
    static uint64_t get_time_us()
    {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (uint64_t)(ts.tv_sec * 1000000 + ts.tv_nsec / 1000);
    }
    static int init()
    {
        int ret = RK_MPI_SYS_Init();
        if (ret != RK_SUCCESS)
        {
            printf("RK_MPI_SYS_Init failed: %x\n", ret);
            return -1;
        }
        printf("RK_MPI_SYS_Init success\n");
        return ret;
    }
    static int deinit()
    {
        int ret = RK_MPI_SYS_Exit();
        if (ret != RK_SUCCESS)
        {
            printf("RK_MPI_SYS_Deinit failed: %x\n", ret);
            return -1;
        }
        printf("RK_MPI_SYS_Deinit success\n");
        return ret;
    }
    static int bind(media_hw *hwsrc, media_hw *hwsink)
    {
        int ret = 0;
        if (hwsrc == NULL || hwsink == NULL)
        {
            printf("media_sys bind error: hwsrc or hwsink is NULL\n");
            return -1;
        }
        printf("media_sys bind: src=%d, sink=%d\n", hwsrc->chnid, hwsink->chnid);

        MPP_CHN_S chn_src, chn_sink;
        chn_src.enModId = (MOD_ID_E)hwsrc->modid;
        chn_src.s32DevId = hwsrc->devid;
        chn_src.s32ChnId = hwsrc->chnid;

        chn_sink.enModId = (MOD_ID_E)hwsink->modid;
        chn_sink.s32DevId = hwsink->devid;
        chn_sink.s32ChnId = hwsink->chnid;
        ret = RK_MPI_SYS_Bind(&chn_src, &chn_sink);
        if (ret != RK_SUCCESS)
        {
            printf("RK_MPI_SYS_Bind failed: %x\n", ret);
            return -1;
        }
        return ret;
    }
    static MB_POOL create_pool(uint64_t size, uint32_t cnt, int type)
    {
        // 依次尝试多种配置(内核/rockit 版本差异可能导致某一种失败)
        // {allocType, dmaType, remapMode, bPreAlloc}
        static const int variants[][4] = {
            {MB_ALLOC_TYPE_DMA,    MB_DMA_TYPE_NONE,    MB_REMAP_MODE_NONE,   0}, // 0: 默认(IOMMU, 不预分配)
            {MB_ALLOC_TYPE_DMA,    MB_DMA_TYPE_CMA,     MB_REMAP_MODE_CACHED, 1}, // 1: CMA + 预分配 + 缓存映射
            {MB_ALLOC_TYPE_DMA,    MB_DMA_TYPE_NONE,    MB_REMAP_MODE_CACHED, 1}, // 2: IOMMU + 预分配
            {MB_ALLOC_TYPE_DMA,    MB_DMA_TYPE_CMA,     MB_REMAP_MODE_NONE,   0}, // 3: CMA, 不预分配
            {MB_ALLOC_TYPE_DMA,    MB_DMA_TYPE_NONE,    MB_REMAP_MODE_CACHED, 0}, // 4: IOMMU + 缓存映射, 不预分配
            {MB_ALLOC_TYPE_MALLOC, 0,                   0,                    0}, // 5: 普通 malloc, 不依赖 DMA heap
        };
        const int variant_cnt = sizeof(variants) / sizeof(variants[0]);
        MB_POOL pool = 0;

        for (int i = 0; i < variant_cnt; i++)
        {
            MB_POOL_CONFIG_S PoolCfg;
            memset(&PoolCfg, 0, sizeof(MB_POOL_CONFIG_S));
            PoolCfg.u64MBSize = size;
            PoolCfg.u32MBCnt = cnt;
            PoolCfg.enAllocType = (MB_ALLOC_TYPE_E)variants[i][0];
            PoolCfg.enDmaType = (MB_DMA_TYPE_E)variants[i][1];
            PoolCfg.enRemapMode = (MB_REMAP_MODE_E)variants[i][2];
            PoolCfg.bPreAlloc = (RK_BOOL)variants[i][3];
            PoolCfg.bNotDelete = RK_FALSE;

            pool = RK_MPI_MB_CreatePool(&PoolCfg);
            if (pool != 0 && pool != MB_INVALID_POOLID)
            {
                printf("create_pool variant %d OK (alloc=%d dma=%d remap=%d pre=%d), pool=%u\n",
                       i, variants[i][0], variants[i][1], variants[i][2], variants[i][3], pool);
                return pool;
            }
            printf("create_pool variant %d FAILED (alloc=%d dma=%d remap=%d pre=%d)\n",
                   i, variants[i][0], variants[i][1], variants[i][2], variants[i][3]);
        }

        printf("RK_MPI_MB_CreatePool all variants failed\n");
        return 0;
    }
    static MB_BLK get_mb(MB_POOL pool, uint64_t u64Size, int block)
    {
        return RK_MPI_MB_GetMB(pool, u64Size, (RK_BOOL)block);
    }
    static int destroy_pool(MB_POOL pool)
    {
        if (pool == NULL)
        {
            printf("destroy_pool: pool is NULL\n");
            return -1;
        }
        int ret = RK_MPI_MB_DestroyPool(pool);
        if (ret != RK_SUCCESS)
        {
            printf("RK_MPI_MB_DestroyPool failed: %x\n", ret);
            return -1;
        }
        return ret;
    }
    static void *get_mb_viraddr(MB_BLK mb)
    {
        if (mb == NULL)
        {
            printf("get_mb_viraddr: mb is NULL\n");
            return NULL;
        }
        void *vir_addr = RK_MPI_MB_Handle2VirAddr(mb);
        if (vir_addr == NULL)
        {
            printf("RK_MPI_MB_Handle2VirAddr failed\n");
            return NULL;
        }
        return vir_addr;
    }
    static int release_mb(MB_BLK blk)
    {
        return RK_MPI_MB_ReleaseMB(blk);
    }
};

class media_profile
{
    uint64_t us_start;
    uint64_t us_last;
    uint64_t us_fps = 0;
    int fps_count = 0;
    bool out = true; // output to console
public:
    media_profile(bool out = true)
    {
        this->out = out;
        reset();
    }
    int printf(const char *fmt, ...)
    {
        if (!out)
            return 0;
        va_list args;
        va_start(args, fmt);
        int ret = vprintf(fmt, args);
        va_end(args);
        return ret;
    }
    void reset()
    {
        us_start = media_sys::get_time_us();
        us_last = us_start;
    }
    void time_us(const char *msg)
    {
        uint64_t us_end = media_sys::get_time_us();
        printf("%s: %llu us, total: %llu us\n", msg, us_end - us_last, us_end - us_start);
        us_last = us_end;
    }
    void time_ms(const char *msg)
    {
        uint64_t us_end = media_sys::get_time_us();
        printf("%s: %llu ms, total: %llu ms\n", msg, (us_end - us_last) / 1000, (us_end - us_start) / 1000);
        us_last = us_end;
    }
    int fps(const char *msg)
    {
        int r = 0;
        uint64_t us_end = media_sys::get_time_us();
        if (us_fps == 0) // 1 second
        {
            us_fps = us_end;
        }
        if(us_end - us_fps >= 1000000)
        {
            printf("%s: %d fps\n", msg, fps_count);
            r = fps_count;
            us_fps = us_end;
            fps_count = 0;
        }
        fps_count++;
        return r;
    }
};

class media_vi : public media_hw
{
    int width;
    int height;
    int pipeid;

public:
    media_vi(int chn, int w, int h) : width(w), height(h)
    {
        modid = RK_ID_VI; // VI module ID
        chnid = chn;
        pipeid = devid;
        printf("media_vi init: channel=%d, width=%d, height=%d\n", chnid, width, height);
    }
    int init()
    {
        int ret = 0;
        VI_DEV_ATTR_S stDevAttr;
        memset(&stDevAttr, 0, sizeof(stDevAttr));
        // 0. get dev config status
        ret = RK_MPI_VI_GetDevAttr(devid, &stDevAttr);
        if (ret == RK_ERR_VI_NOT_CONFIG)
        {
            // 0-1.config dev
            ret = RK_MPI_VI_SetDevAttr(devid, &stDevAttr);
            if (ret != RK_SUCCESS)
            {
                printf("RK_MPI_VI_SetDevAttr %x\n", ret);
                return -1;
            }
        }
        else
        {
            printf("RK_MPI_VI_SetDevAttr already\n");
        }
        // 1.get dev enable status
        ret = RK_MPI_VI_GetDevIsEnable(devid);
        if (ret != RK_SUCCESS)
        {
            // 1-2.enable dev
            ret = RK_MPI_VI_EnableDev(devid);
            if (ret != RK_SUCCESS)
            {
                printf("RK_MPI_VI_EnableDev %x\n", ret);
                return -1;
            }
            // 1-3.bind dev/pipe
            VI_DEV_BIND_PIPE_S stBindPipe;
            memset(&stBindPipe, 0, sizeof(stBindPipe));
            stBindPipe.u32Num = 1;
            stBindPipe.PipeId[0] = pipeid;
            ret = RK_MPI_VI_SetDevBindPipe(devid, &stBindPipe);
            if (ret != RK_SUCCESS)
            {
                printf("RK_MPI_VI_SetDevBindPipe %x\n", ret);
                return -1;
            }
        }
        else
        {
            printf("RK_MPI_VI_EnableDev already\n");
        }

        // VI init
        VI_CHN_ATTR_S vi_chn_attr;
        memset(&vi_chn_attr, 0, sizeof(vi_chn_attr));
        vi_chn_attr.stIspOpt.u32BufCount = 2;
        vi_chn_attr.stIspOpt.enMemoryType =
            VI_V4L2_MEMORY_TYPE_DMABUF; // VI_V4L2_MEMORY_TYPE_MMAP;
        vi_chn_attr.stSize.u32Width = width;
        vi_chn_attr.stSize.u32Height = height;
        vi_chn_attr.enPixelFormat = RK_FMT_YUV420SP;
        vi_chn_attr.enCompressMode = COMPRESS_MODE_NONE; // COMPRESS_AFBC_16x16;
        vi_chn_attr.u32Depth = 1;                        // 0, get fail, 1 - u32BufCount, can get, if bind to other device, must be < u32BufCount
        ret = RK_MPI_VI_SetChnAttr(0, chnid, &vi_chn_attr);
        ret |= RK_MPI_VI_EnableChn(0, chnid);
        if (ret)
        {
            printf("ERROR: create VI error! ret=%d\n", ret);
            return ret;
        }

        return ret;
    }

    int get_chn_frame(VIDEO_FRAME_INFO_S *frame, int timeout_ms = 1000)
    {
        int ret = RK_MPI_VI_GetChnFrame(0, chnid, frame, timeout_ms);
        if (ret != RK_SUCCESS)
        {
            printf("ERROR: get VI frame error! ret=%d\n", ret);
            return ret;
        }
        return ret;
    }
    int release_chn_frame(VIDEO_FRAME_INFO_S *frame)
    {
        if(frame == NULL || frame->stVFrame.pMbBlk == NULL)
        {
            printf("ERROR: release VI frame, frame is NULL!\n");
            return 0;
        }
        int ret = RK_MPI_VI_ReleaseChnFrame(0, chnid, frame);
        if (ret != RK_SUCCESS)
        {
            printf("ERROR: release VI frame error! ret=%d\n", ret);
            return ret;
        }
        return ret;
    }
    ~media_vi()
    {
        RK_MPI_VI_DisableChn(pipeid, chnid);
	    RK_MPI_VI_DisableDev(devid);
    }
};

class media_vpss : public media_hw
{
    int width;
    int height;
    int grpEnPixelFormat; // default pixel format
    int chnEnPixelFormat;   // default pixel format for channel

public:
    media_vpss(int chn, int w, int h, int grpFmt = RK_FMT_YUV420SP, int chnFmt = RK_FMT_BGR888)
    {
        width = w;
        height = h;
        grpEnPixelFormat = grpFmt;
        chnEnPixelFormat = chnFmt;
        modid = RK_ID_VPSS; // VPSS module ID
        chnid = chn;
        // printf("media_vpss init: chnId=%d, width=%d, height=%d\n", chnid, width, height);
    }
    int init()
    {

#if 1
        int VpssChn = 0;
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
        stVpssChnAttr.enPixelFormat = RK_FMT_YUV420SP;
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
#else
        printf("vpss init 1------------------------\n");
        int s32Ret;
        VPSS_CHN_ATTR_S stVpssChnAttr;
        VPSS_GRP_ATTR_S stGrpVpssAttr;

        int s32Grp = 0;

        stGrpVpssAttr.u32MaxW = 4096;
        stGrpVpssAttr.u32MaxH = 4096;
        stGrpVpssAttr.enPixelFormat = (PIXEL_FORMAT_E)grpEnPixelFormat;
        stGrpVpssAttr.stFrameRate.s32SrcFrameRate = -1;
        stGrpVpssAttr.stFrameRate.s32DstFrameRate = -1;
        stGrpVpssAttr.enCompressMode = COMPRESS_MODE_NONE;

        stVpssChnAttr.enChnMode = VPSS_CHN_MODE_USER;
        stVpssChnAttr.enDynamicRange = DYNAMIC_RANGE_SDR8;
        stVpssChnAttr.enPixelFormat = (PIXEL_FORMAT_E)chnEnPixelFormat;
        stVpssChnAttr.stFrameRate.s32SrcFrameRate = -1;
        stVpssChnAttr.stFrameRate.s32DstFrameRate = -1;
        stVpssChnAttr.u32Width = width;
        stVpssChnAttr.u32Height = height;
        stVpssChnAttr.enCompressMode = COMPRESS_MODE_NONE;

        s32Ret = RK_MPI_VPSS_CreateGrp(s32Grp, &stGrpVpssAttr);
        if (s32Ret != RK_SUCCESS)
        {
            printf("RK_MPI_VPSS_CreateGrp failed: 0x%x\n", s32Ret);
            return s32Ret;
        }
        printf("vpss init 2------------------------\n");

        s32Ret = RK_MPI_VPSS_SetChnAttr(s32Grp, chnid, &stVpssChnAttr);
        if (s32Ret != RK_SUCCESS)
        {
            printf("RK_MPI_VPSS_SetChnAttr failed: 0x%x\n", s32Ret);
            return s32Ret;
        }
        printf("vpss init 3------------------------\n");

        s32Ret = RK_MPI_VPSS_EnableChn(s32Grp, chnid);
        if (s32Ret != RK_SUCCESS)
        {
            printf("RK_MPI_VPSS_EnableChn failed: 0x%x\n", s32Ret);
            return s32Ret;
        }
        printf("vpss init 4------------------------\n");

        s32Ret = RK_MPI_VPSS_StartGrp(s32Grp);
        if (s32Ret != RK_SUCCESS)
        {
            printf("RK_MPI_VPSS_StartGrp failed: 0x%x\n", s32Ret);
            return s32Ret;
        }
        // s32Ret = RK_MPI_VPSS_ResetGrp(s32Grp);
        // if (s32Ret != RK_SUCCESS) {
        //     RK_LOGE("RK_MPI_VPSS_ResetGrp failed with %#x", s32Ret);
        //     return s32Ret;
        // }
        printf("VPSS init success\n");
        return s32Ret;
#endif
    }
    int send_frame(VIDEO_FRAME_INFO_S *frame, int timeout_ms = 1000)
    {
        int ret = RK_MPI_VPSS_SendFrame(0, chnid, frame, timeout_ms);
        if (ret != RK_SUCCESS)
        {
            printf("RK_MPI_VPSS_SendFrame failed: %d\n", ret);
            return ret;
        }
        return ret;
    }
    int get_grp_frame(VIDEO_FRAME_INFO_S *frame)
    {
        int ret = RK_MPI_VPSS_GetGrpFrame(0, chnid, frame);
        if (ret != RK_SUCCESS)
        {
            printf("media_vpss get_grp_frame failed: %d\n", ret);
            return ret;
        }
        return ret;
    }
    int release_grp_frame(VIDEO_FRAME_INFO_S *frame)
    {
        if(frame == NULL || frame->stVFrame.pMbBlk == NULL)
        {
            printf("media_vpss release_grp_frame: frame is NULL\n");
            return 0;
        }
        int ret = RK_MPI_VPSS_ReleaseGrpFrame(0, chnid, frame);
        if (ret != RK_SUCCESS)
        {
            printf("media_vpss release_grp_frame failed: %d\n", ret);
            return ret;
        }
        return ret;
    }
    int get_chn_frame(VIDEO_FRAME_INFO_S *frame, int timeout_ms = 1000)
    {
        int ret = RK_MPI_VPSS_GetChnFrame(0, chnid, frame, timeout_ms);
        if (ret != RK_SUCCESS)
        {
            printf("media_vpss get_chn_frame failed: 0x%x\n", ret);
            return ret;
        }
        return ret;
    }
    int release_chn_frame(VIDEO_FRAME_INFO_S *frame)
    {
        if(frame == NULL || frame->stVFrame.pMbBlk == NULL)
        {
            printf("media_vpss release_chn_frame: frame is NULL\n");
            return 0;
        }
        int ret = RK_MPI_VPSS_ReleaseChnFrame(0, chnid, frame);
        if (ret != RK_SUCCESS)
        {
            printf("media_vpss release_chn_frame failed: 0x%x\n", ret);
            return ret;
        }
        return ret;
    }
    ~media_vpss()
    {
        RK_MPI_VPSS_DisableChn(0, chnid);
        RK_MPI_VPSS_StopGrp(0);
        RK_MPI_VPSS_DestroyGrp(0);
    }
};

class media_venc : public media_hw
{
    int width;
    int height;
    RK_CODEC_ID_E enType;

public:
    media_venc(int chn, int w, int h, RK_CODEC_ID_E type)
        : width(w), height(h), enType(type)
    {
        modid = RK_ID_VENC; // VENC module ID
        chnid = chn;
        printf("media_venc init: chnId=%d, width=%d, height=%d, enType=%d\n", chnid, width, height, enType);
    }
    int init()
    {
        VENC_RECV_PIC_PARAM_S stRecvParam;
        VENC_CHN_ATTR_S stAttr;
        memset(&stAttr, 0, sizeof(VENC_CHN_ATTR_S));

        if (enType == RK_VIDEO_ID_AVC)
        {
            // CBR + 合理码率(720p 建议 1~4Mbps),原 AVBR 模式却用 CBR 结构体,参数全为 0
            stAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
            stAttr.stRcAttr.stH264Cbr.u32BitRate = 2 * 1024 * 1024;
            stAttr.stRcAttr.stH264Cbr.u32Gop = 20;
        }
        else if (enType == RK_VIDEO_ID_HEVC)
        {
            stAttr.stRcAttr.enRcMode = VENC_RC_MODE_H265CBR;
            stAttr.stRcAttr.stH265Cbr.u32BitRate = 10 * 1024;
            stAttr.stRcAttr.stH265Cbr.u32Gop = 60;
        }
        else if (enType == RK_VIDEO_ID_MJPEG)
        {
            stAttr.stRcAttr.enRcMode = VENC_RC_MODE_MJPEGCBR;
            stAttr.stRcAttr.stMjpegCbr.u32BitRate = 10 * 1024;
        }

        stAttr.stVencAttr.enType = enType;
        stAttr.stVencAttr.enPixelFormat = RK_FMT_YUV420SP;
        if (enType == RK_VIDEO_ID_AVC)
            stAttr.stVencAttr.u32Profile = H264E_PROFILE_HIGH;
        stAttr.stVencAttr.u32PicWidth = width;
        stAttr.stVencAttr.u32PicHeight = height;
        stAttr.stVencAttr.u32VirWidth = width;
        stAttr.stVencAttr.u32VirHeight = height;
        stAttr.stVencAttr.u32StreamBufCnt = 4;
        stAttr.stVencAttr.u32BufSize = width * height * 3 / 2;
        stAttr.stVencAttr.enMirror = MIRROR_NONE;

        RK_MPI_VENC_CreateChn(chnid, &stAttr);

        memset(&stRecvParam, 0, sizeof(VENC_RECV_PIC_PARAM_S));
        stRecvParam.s32RecvPicNum = -1;
        RK_MPI_VENC_StartRecvFrame(chnid, &stRecvParam);
        return 0;
    }
    int send_frame(VIDEO_FRAME_INFO_S *frame, int timeout_ms = 1000)
    {
        int ret = RK_MPI_VENC_SendFrame(chnid, frame, timeout_ms);
        if (ret != RK_SUCCESS)
        {
            printf("RK_MPI_VENC_SendFrame failed: %d\n", ret);
            return ret;
        }
        return RK_SUCCESS;
    }
    int get_stream(VENC_STREAM_S *stream, int timeout_ms = 1000)
    {
        int ret = RK_MPI_VENC_GetStream(chnid, stream, timeout_ms);
        if (ret != RK_SUCCESS)
        {
            printf("RK_MPI_VENC_GetStream failed: %d\n", ret);
            return ret;
        }
        return ret;
    }
    int release_stream(VENC_STREAM_S *stream)
    {
        if (stream == NULL || stream->pstPack == NULL)
        {
            printf("release_stream: stream is NULL\n");
            return -1;
        }
        int ret = RK_MPI_VENC_ReleaseStream(chnid, stream);
        if (ret != RK_SUCCESS)
        {
            printf("RK_MPI_VENC_ReleaseStream failed: %d\n", ret);
            return ret;
        }
        return RK_SUCCESS;
    }

    ~media_venc()
    {
        RK_MPI_VENC_StopRecvFrame(chnid);
        RK_MPI_VENC_DestroyChn(chnid);
    }
};

#endif // _MPI_HPP_