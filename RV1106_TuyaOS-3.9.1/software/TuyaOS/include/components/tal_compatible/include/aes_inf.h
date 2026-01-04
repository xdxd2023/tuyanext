/**
 * @file aes_inf.h
 * @author beihe(JiangBo) (beihe.jiang@tuya.com)
 * @brief 适配老的aes_inf.h文件
 * @version 0.1
 * @date 2023-12-08
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef __AES_INF__
#define __AES_INF__

#include "tal_symmetry.h"
#include "tuya_cloud_com_defs.h"

#define AES128_ENCRYPT_KEY_LEN ENCRYPT_KEY_LEN

#pragma pack(1)

#pragma pack()

#ifdef __cplusplus
extern "C" {
#endif

#define aes128_ecb_encode(data, len, ec_data, ec_len, key)  tal_aes128_ecb_encode(data, len, ec_data, ec_len, key)
#define aes128_ecb_decode(data, len, dec_data, dec_len, key) tal_aes128_ecb_decode(data, len, dec_data, dec_len, key)
#define aes_free_data(data)                                  tal_aes_free_data(data)
#define aes_get_actual_length(dec_data, dec_data_len)        tal_get_actual_length(dec_data, dec_data_len)
#ifdef __cplusplus
} // extern "C"
#endif

#endif // __AES_INF__
