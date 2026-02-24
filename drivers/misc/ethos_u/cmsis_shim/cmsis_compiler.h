/*
 * Copyright (c) 2026 BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 *
 * <cmsis_compiler.h> replacement for AArch64.
 *
 * The Ethos-U core driver includes <cmsis_compiler.h> for __WFE() and __SEV().
 * The CMSIS Core header uses Cortex-M builtins such as __builtin_arm_get_fpscr,
 * which fail to compile on AArch64 with GCC 14 and later. Other architectures
 * get the CMSIS Core header.
 */

#ifndef ETHOS_U_CMSIS_SHIM_H_
#define ETHOS_U_CMSIS_SHIM_H_

#ifdef __aarch64__

#ifndef __WFE
#define __WFE() __asm__ volatile("wfe" ::: "memory")
#endif
#ifndef __SEV
#define __SEV() __asm__ volatile("sev" ::: "memory")
#endif

#else
#include_next <cmsis_compiler.h>
#endif

#endif /* ETHOS_U_CMSIS_SHIM_H_ */
