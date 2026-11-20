/*
 * Copyright (c) 2026 BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 *
 * Shim for <cmsis_compiler.h> on AArch64 targets.
 *
 * The Ethos-U core driver includes <cmsis_compiler.h> for __WFE() and
 * __SEV() intrinsics.  The real CMSIS Core header pulls in Cortex-M
 * specific builtins (__builtin_arm_get_fpscr, etc.) that don't exist
 * on AArch64 and cause build failures with GCC >= 14.
 *
 * On AArch64, provide just the two intrinsics the driver needs and
 * skip the full CMSIS chain.  On other architectures, fall through to
 * the real header.
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
