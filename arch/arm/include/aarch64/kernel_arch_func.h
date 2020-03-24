/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com<
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private kernel definitions (ARM64)
 *
 * This file contains private kernel function definitions and various
 * other definitions for the ARM Cortex-A processor architecture family.
 *
 * This file is also included by assembly language files which must #define
 * _ASMLANGUAGE before including this header file.  Note that kernel
 * assembly source files obtains structure offset values via "absolute symbols"
 * in the offsets.o module.
 */

#ifndef ZEPHYR_ARCH_ARM_INCLUDE_AARCH64_KERNEL_ARCH_FUNC_H_
#define ZEPHYR_ARCH_ARM_INCLUDE_AARCH64_KERNEL_ARCH_FUNC_H_

#include <kernel_arch_data.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE

static ALWAYS_INLINE void arch_kernel_init(void)
{
}

static inline void arch_switch(void *switch_to, void **switched_from)
{
	z_arm64_call_svc(switch_to, switched_from);

	return;
}

extern void z_arm64_fatal_error(const z_arch_esf_t *esf, unsigned int reason);

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_ARM_INCLUDE_AARCH64_KERNEL_ARCH_FUNC_H_ */
