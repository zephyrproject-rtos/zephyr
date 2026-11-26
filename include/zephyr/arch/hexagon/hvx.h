/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Hexagon HVX coprocessor management
 *
 * With CONFIG_HEXAGON_HVX, a thread picks up an HVX context the first time
 * it runs an HVX instruction and holds on to it until it exits or calls
 * hexagon_hvx_release(). Don't use HVX from ISRs.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_HEXAGON_HVX_H_
#define ZEPHYR_INCLUDE_ARCH_HEXAGON_HVX_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the number of hardware HVX contexts.
 *
 * @return Number of contexts threads can own, 0 if HVX is unavailable.
 */
uint32_t hexagon_hvx_contexts(void);

/**
 * @brief Give up the calling thread's HVX context.
 *
 * Whatever is in the thread's HVX registers is thrown away. The next HVX
 * instruction picks up a context again, with every register zeroed.
 * Call it from supervisor mode.
 *
 * @retval 0 Success.
 * @retval -EPERM Called from an ISR.
 */
int hexagon_hvx_release(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_HEXAGON_HVX_H_ */
