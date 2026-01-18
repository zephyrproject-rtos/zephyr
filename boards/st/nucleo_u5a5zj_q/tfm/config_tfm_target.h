/*
 * Copyright (c) 2022-2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __CONFIG_TFM_TARGET_H__
#define __CONFIG_TFM_TARGET_H__

/* Use stored NV seed to provide entropy */
#define CRYPTO_NV_SEED                         0

/* Use external RNG to provide entropy */
#define CRYPTO_EXT_RNG                         1

/* Partition size 112kB in flash_layout.h */
#undef ITS_NUM_ASSETS
#define ITS_NUM_ASSETS                         32
#undef ITS_MAX_ASSET_SIZE
#define ITS_MAX_ASSET_SIZE                     2048

/* Partition size 16kB in flash_layout.h */
#undef PS_NUM_ASSETS
#define PS_NUM_ASSETS                          32
#undef PS_MAX_ASSET_SIZE
#define PS_MAX_ASSET_SIZE                      2048

#endif /* __CONFIG_TFM_TARGET_H__ */
