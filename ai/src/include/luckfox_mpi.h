#ifndef _LUCKFOX_MPI_H_
#define _LUCKFOX_MPI_H_

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

#define TEST_ARGB32_PIX_SIZE 4
#define TEST_ARGB32_RED 0xFF0000FF
#define TEST_ARGB32_GREEN 0x00FF00FF
#define TEST_ARGB32_BLUE 0x0000FFFF
#define TEST_ARGB32_TRANS 0x00000000
#define TEST_ARGB32_BLACK 0x000000FF


extern RK_U64 TEST_COMM_GetNowUs();
extern RK_S32 test_rgn_overlay_line_process(int sX ,int sY,int type, int group);
extern RK_S32 rgn_overlay_release(int group);

extern int vi_dev_init();
extern int vi_chn_init(int channelId, int width, int height);
extern int vpss_init(int VpssChn, int width, int height);
extern int venc_init(int chnId, int width, int height, RK_CODEC_ID_E enType);

#endif
