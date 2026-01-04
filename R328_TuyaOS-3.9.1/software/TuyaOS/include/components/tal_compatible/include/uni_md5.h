/**
 * @file uni_md5.h
 * @author beihe(JiangBo) (beihe.jiang@tuya.com)
 * @brief 兼容uni_md5.h
 * @version 0.1
 * @date 2023-12-08
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef __UNI_MD5__
#define __UNI_MD5__

#include "tal_hash.h"


#pragma pack(1)
typedef TKL_HASH_HANDLE UNI_MD5_CTX_S;
#pragma pack()

#ifdef __cplusplus
    extern "C" {
#endif
void uni_md5_init(UNI_MD5_CTX_S *context);
void uni_md5_update(UNI_MD5_CTX_S *context, const unsigned char *input, const unsigned int inputlen);
void uni_md5_final(UNI_MD5_CTX_S *context, unsigned char digest[16]);
#ifdef __cplusplus
} // extern "C"
#endif

#endif // __UNI_MD5__
