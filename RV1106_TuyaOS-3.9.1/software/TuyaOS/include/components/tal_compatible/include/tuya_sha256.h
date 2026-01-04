/**
 * @file tuya_sha256.h
 * @author beihe(JiangBo) (beihe.jiang@tuya.com)
 * @brief 兼容tuya_sha256.h
 * @version 0.1
 * @date 2023-12-08
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef __TUYA_SHA256__
#define __TUYA_SHA256__

#include "tal_hash.h"

#pragma pack(1)

#pragma pack()

#ifdef __cplusplus
extern "C" {
#endif

#define sha2_hmac(key, keylen, input, ilen, output, is224) \
    if (!is224) {                                          \
        tal_sha256_mac(key, keylen, input, ilen, output);  \
    } else {                                               \
        PR_ERR("not support 224");                         \
    }

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __TUYA_SHA256__
