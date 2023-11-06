/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2023 Arm Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief CMSIS interface file
 *
 * This header contains the interface to the ARM CMSIS Core headers.
 */

#ifndef ZEPHYR_MODULES_CMSIS_CMSIS_M_H_
#define ZEPHYR_MODULES_CMSIS_CMSIS_M_H_

#if defined(CONFIG_CMSIS_M_CHECK_DEVICE_DEFINES) && CONFIG_CMSIS_M_CHECK_DEVICE_DEFINES == 1U
#define __CHECK_DEVICE_DEFINES 1U
#endif

#include <zephyr/arch/arm/cortex_m/nvic.h>

#include <soc.h>

#if __NVIC_PRIO_BITS != NUM_IRQ_PRIO_BITS
#error "NUM_IRQ_PRIO_BITS and __NVIC_PRIO_BITS are not set to the same value"
#endif

#if __MPU_PRESENT != CONFIG_CPU_HAS_ARM_MPU
#error "__MPU_PRESENT and CONFIG_CPU_HAS_ARM_MPU are not set to the same value"
#endif

#if __FPU_PRESENT != CONFIG_CPU_HAS_FPU
#error "__FPU_PRESENT and CONFIG_CPU_HAS_FPU are not set to the same value"
#endif


/* VTOR is only optional on armv6-m and armv8-m baseline. __VTOR_PRESENT is often
 * left undefined on platform where it is not optional.
 */
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE) && \
	(__VTOR_PRESENT != CONFIG_CPU_CORTEX_M_HAS_VTOR)
#error "__VTOR_PRESENT and CONFIG_CPU_CORTEX_M_HAS_VTOR are not set to the same value."
#endif

/* Some platform’s sdk incorrectly define __DSP_PRESENT for Cortex-M4 & Cortex-M7
 * DSP extension. __ARM_FEATURE_DSP is set by the compiler for these. So ignore
 * __DSP_PRESENT discrepancy when __ARM_FEATURE_DSP is defined.
 */
#if !defined(__ARM_FEATURE_DSP) && (__DSP_PRESENT != CONFIG_ARMV8_M_DSP)
#error "__DSP_PRESENT and CONFIG_ARMV8_M_DSP are not set to the same value"
#endif

#if  __ICACHE_PRESENT != CONFIG_CPU_HAS_ICACHE
#error "__ICACHE_PRESENT and CONFIG_CPU_HAS_ICACHE are not set to the same value"
#endif

#if __DCACHE_PRESENT != CONFIG_CPU_HAS_DCACHE
#error "__DCACHE_PRESENT and CONFIG_CPU_HAS_DCACHE are not set to the same value"
#endif

#if __MVE_PRESENT != CONFIG_ARMV8_1_M_MVEI
#error "__MVE_PRESENT and CONFIG_ARMV8_1_M_MVEI are not set to the same value"
#endif

#if __SAUREGION_PRESENT != CONFIG_CPU_HAS_ARM_SAU
#error "__SAUREGION_PRESENT and CONFIG_CPU_HAS_ARM_SAU are not set to the same value"
#endif

#endif /* ZEPHYR_MODULES_CMSIS_CMSIS_M_H_ */
