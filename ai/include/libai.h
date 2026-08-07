#ifndef __LIB_AI_H__
#define __LIB_AI_H__
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
typedef int (*fn_video_cb)(const uint8_t *frame, int len, uint64_t ts, int fraemetype);
int aiTask(fn_video_cb cb);

enum AI_PROFILE_TYPE_E
{
    PROFILE_AI,
    PROFILE_ENCODE,
    PROFILE_TIME,
    PROFILE_FPS
};
void aiEnableProfile(AI_PROFILE_TYPE_E profile,bool enable);
bool aiIsProfileEnabled(AI_PROFILE_TYPE_E profile);
#ifdef __cplusplus
}
#endif
#endif