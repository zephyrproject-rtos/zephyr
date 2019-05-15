/*
 * Copyright 2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _FSL_POWERQUAD_DATA_H_
#define _FSL_POWERQUAD_DATA_H_

#include <stdint.h>

/*******************************************************************************
 * Definitions
 ******************************************************************************/

extern int32_t dct16_twiddle[32];
extern int32_t dct32_twiddle[64];
extern int32_t dct64_twiddle[128];
extern int32_t dct128_twiddle[256];
extern int32_t dct256_twiddle[512];
extern int32_t dct512_twiddle[1024];
extern int32_t idct16_twiddle[32];
extern int32_t idct32_twiddle[64];
extern int32_t idct64_twiddle[128];
extern int32_t idct128_twiddle[256];
extern int32_t idct256_twiddle[512];
extern int32_t idct512_twiddle[1024];
extern int32_t dct16_cosFactor[32];
extern int32_t dct32_cosFactor[64];
extern int32_t dct64_cosFactor[128];
extern int32_t dct128_cosFactor[256];
extern int32_t dct256_cosFactor[512];
extern int32_t dct512_cosFactor[1024];

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

#if defined(__cplusplus)
}

#endif /* __cplusplus */

#endif /* _FSL_POWERQUAD_DATA_H_ */
