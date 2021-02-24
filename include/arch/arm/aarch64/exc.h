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
	uint64_t x0;
	uint64_t x1;
	uint64_t x2;
	uint64_t x3;
	uint64_t x4;
	uint64_t x5;
	uint64_t x6;
	uint64_t x7;
	uint64_t x8;
	uint64_t x9;
	uint64_t x10;
	uint64_t x11;
	uint64_t x12;
	uint64_t x13;
	uint64_t x14;
	uint64_t x15;
	uint64_t x16;
	uint64_t x17;
	uint64_t x18;
	uint64_t x30;
	uint64_t spsr;
	uint64_t elr;
};

typedef struct __esf z_arch_esf_t;

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_AARCH64_EXC_H_ */
