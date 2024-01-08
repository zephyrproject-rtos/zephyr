/*
 * Copyright (c) 2022 Zephyr authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_OS_MGMT_PROCESSOR_
#define H_OS_MGMT_PROCESSOR_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Processor name (used in uname output command)
 * Will be unknown if processor type is not listed
 * (List extracted from /cmake/gcc-m-cpu.cmake)
 */
#if defined(CONFIG_ARM)
#if defined(CONFIG_CPU_CORTEX_M0)
#define PROCESSOR_NAME "cortex-m0"
#elif defined(CONFIG_CPU_CORTEX_M0PLUS)
#define PROCESSOR_NAME "cortex-m0plus"
#elif defined(CONFIG_CPU_CORTEX_M1)
#define PROCESSOR_NAME "cortex-m1"
#elif defined(CONFIG_CPU_CORTEX_M3)
#define PROCESSOR_NAME "cortex-m3"
#elif defined(CONFIG_CPU_CORTEX_M4)
#define PROCESSOR_NAME "cortex-m4"
#elif defined(CONFIG_CPU_CORTEX_M7)
#define PROCESSOR_NAME "cortex-m7"
#elif defined(CONFIG_CPU_CORTEX_M23)
#define PROCESSOR_NAME "cortex-m23"
#elif defined(CONFIG_CPU_CORTEX_M33)
#if defined(CONFIG_ARMV8_M_DSP)
#define PROCESSOR_NAME "cortex-m33"
#else
#define PROCESSOR_NAME "cortex-m33+nodsp"
#endif
#elif defined(CONFIG_CPU_CORTEX_M55)
#if defined(CONFIG_ARMV8_1_M_MVEF)
#define PROCESSOR_NAME "cortex-m55"
#elif defined(CONFIG_ARMV8_1_M_MVEI)
#define PROCESSOR_NAME "cortex-m55+nomve.fp"
#elif defined(CONFIG_ARMV8_M_DSP)
#define PROCESSOR_NAME "cortex-m55+nomve"
#else
#define PROCESSOR_NAME "cortex-m55+nodsp"
#endif
#elif defined(CONFIG_CPU_CORTEX_R4)
#if defined(CONFIG_FPU) && defined(CONFIG_CPU_HAS_VFP)
#define PROCESSOR_NAME "cortex-r4f"
#else
#define PROCESSOR_NAME "cortex-r4"
#endif
#elif defined(CONFIG_CPU_CORTEX_R5)
#if defined(CONFIG_FPU) && defined(CONFIG_CPU_HAS_VFP)
#if !defined(CONFIG_VFP_FEATURE_DOUBLE_PRECISION)
#define PROCESSOR_NAME "cortex-r5+nofp.dp"
#else
#define PROCESSOR_NAME "cortex-r5"
#endif
#else
#define PROCESSOR_NAME "cortex-r5+nofp"
#endif
#elif defined(CONFIG_CPU_CORTEX_R7)
#if defined(CONFIG_FPU) && defined(CONFIG_CPU_HAS_VFP)
#if !defined(CONFIG_VFP_FEATURE_DOUBLE_PRECISION)
#define PROCESSOR_NAME "cortex-r7+nofp.dp"
#else
#define PROCESSOR_NAME "cortex-r7"
#endif
#else
#define PROCESSOR_NAME "cortex-r7+nofp"
#endif
#elif defined(CONFIG_CPU_CORTEX_R52)
#if defined(CONFIG_FPU) && defined(CONFIG_CPU_HAS_VFP)
#if !defined(CONFIG_VFP_FEATURE_DOUBLE_PRECISION)
#define PROCESSOR_NAME "cortex-r52+nofp.dp"
#else
#define PROCESSOR_NAME "cortex-r52"
#endif
#else
#define PROCESSOR_NAME "cortex-r52"
#endif
#elif defined(CONFIG_CPU_CORTEX_A9)
#define PROCESSOR_NAME "cortex-a9"
#endif
#elif defined(CONFIG_ARM64)
#if defined(CONFIG_CPU_CORTEX_A53)
#define PROCESSOR_NAME "cortex-a53"
#elif defined(CONFIG_CPU_CORTEX_A55)
#define PROCESSOR_NAME "cortex-a55"
#elif defined(CONFIG_CPU_CORTEX_A57)
#define PROCESSOR_NAME "cortex-a57"
#elif defined(CONFIG_CPU_CORTEX_A72)
#define PROCESSOR_NAME "cortex-a72"
#elif defined(CONFIG_CPU_CORTEX_A76_A55)
#define PROCESSOR_NAME "cortex-a76"
#elif defined(CONFIG_CPU_CORTEX_A76)
#define PROCESSOR_NAME "cortex-a76"
#elif defined(CONFIG_CPU_CORTEX_R82)
#define PROCESSOR_NAME "armv8.4-a+nolse"
#endif
#elif defined(CONFIG_ARC)
#if defined(CONFIG_CPU_EM4_FPUS)
#define PROCESSOR_NAME "em4_fpus"
#elif defined(CONFIG_CPU_EM4_DMIPS)
#define PROCESSOR_NAME "em4_dmips"
#elif defined(CONFIG_CPU_EM4_FPUDA)
#define PROCESSOR_NAME "em4_fpuda"
#elif defined(CONFIG_CPU_HS3X)
#define PROCESSOR_NAME "archs"
#elif defined(CONFIG_CPU_HS4X)
#define PROCESSOR_NAME "hs4x"
#elif defined(CONFIG_CPU_HS5X)
#define PROCESSOR_NAME "hs5x"
#elif defined(CONFIG_CPU_HS6X)
#define PROCESSOR_NAME "hs6x"
#elif defined(CONFIG_CPU_EM4)
#define PROCESSOR_NAME "arcem"
#elif defined(CONFIG_CPU_EM6)
#define PROCESSOR_NAME "arcem"
#endif
#elif defined(CONFIG_X86)
#if defined(CONFIG_X86_64)
#define PROCESSOR_NAME "x86_64"
#else
#define PROCESSOR_NAME "x86"
#endif
#elif defined(CONFIG_RISCV)
#define PROCESSOR_NAME "riscv"
#elif defined(CONFIG_XTENSA)
#define PROCESSOR_NAME "xtensa"
#elif defined(CONFIG_SPARC)
#define PROCESSOR_NAME "sparc"
#endif

#ifndef PROCESSOR_NAME
#warning "Processor type could not be determined"
#define PROCESSOR_NAME "unknown"
#endif

#ifdef __cplusplus
}
#endif

#endif /* H_OS_MGMT_PROCESSOR_ */
