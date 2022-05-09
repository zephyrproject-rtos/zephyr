/*
 * Copyright (c) 2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef ZEPHYR_DRIVERS_I2S_MCUX_H_
#define ZEPHYR_DRIVERS_I2S_MCUX_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/device.h>
#include <fsl_sai.h>
#include <fsl_edma.h>

#define SAI_WORD_SIZE_BITS_MIN 8
#define SAI_WORD_SIZE_BITS_MAX 32

#define SAI_WORD_PER_FRAME_MIN 0
#define SAI_WORD_PER_FRAME_MAX 32

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_I2S_MCUX_H_ */
