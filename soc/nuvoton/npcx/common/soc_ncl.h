/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NUVOTON_NPCX_SOC_NCL_H_
#define _NUVOTON_NPCX_SOC_NCL_H_

#ifdef __cplusplus
extern "C" {
#endif

/* The status code returns from Nuvoton Cryptographic Library ROM APIs */
enum ncl_status {
	NCL_STATUS_OK = 0xA5A5,
	NCL_STATUS_FAIL = 0x5A5A,
	NCL_STATUS_INVALID_PARAM = 0x02,
	NCL_STATUS_PARAM_NOT_SUPPORTED = 0x03,
	NCL_STATUS_SYSTEM_BUSY = 0x04,
	NCL_STATUS_AUTHENTICATION_FAIL = 0x05,
	NCL_STATUS_NO_RESPONSE = 0x06,
	NCL_STATUS_HARDWARE_ERROR = 0x07
};

enum ncl_sha_type {
	NCL_SHA_TYPE_2_256 = 0,
	NCL_SHA_TYPE_2_384 = 1,
	NCL_SHA_TYPE_2_512 = 2,
	NCL_SHA_TYPE_NUM
};

/*
 * This enum defines the security strengths supported by this DRBG mechanism.
 * The internally generated entropy and nonce sizes are derived from these
 * values. The supported actual sizes:
 *       Security strength (bits)    112 128 192 256 128_Test 256_Test
 *
 *       Entropy size (Bytes)        32  48  64  96  111      128
 *       Nonce size (Bytes)          16  16  24  32  16       0
 */
enum ncl_drbg_security_strength {
	NCL_DRBG_SECURITY_STRENGTH_112B = 0,
	NCL_DRBG_SECURITY_STRENGTH_128B,
	NCL_DRBG_SECURITY_STRENGTH_192B,
	NCL_DRBG_SECURITY_STRENGTH_256B,
	NCL_DRBG_SECURITY_STRENGTH_128B_TEST,
	NCL_DRBG_SECURITY_STRENGTH_256B_TEST,
	NCL_DRBG_MAX_SECURITY_STRENGTH
};

#ifdef __cplusplus
}
#endif

#endif /* _NUVOTON_NPCX_SOC_NCL_H_ */
