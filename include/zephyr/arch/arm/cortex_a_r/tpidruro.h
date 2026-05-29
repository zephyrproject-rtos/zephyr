/*
 * Copyright (c) 2023 Arm Limited (or its affiliates). All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief tpidruro bits allocation
 *
 * Among other things, the tpidruro holds the address for the current
 * CPU's struct _cpu instance. But such a pointer is at least 4-bytes
 * aligned. That leaves two of free bits for other purposes.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_A_R_TPIDRURO_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_A_R_TPIDRURO_H_

#define TPIDRURO_CURR_CPU 0xFFFFFFFCUL

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_A_R_TPIDRURO_H_ */
