/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Cortex-A public exception handling
 *
 * ARM-specific kernel exception handling interface. Included by arm64/arch.h.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_AARCH64_EXC_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_AARCH64_EXC_H_

/* for assembler, only works with constants */

#ifdef _ASMLANGUAGE
#else
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct __esf {
	struct __basic_sf {
		uint64_t regs[20];
		uint64_t spsr;
		uint64_t elr;
	} basic;
};

typedef struct __esf z_arch_esf_t;

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_AARCH64_EXC_H_ */
