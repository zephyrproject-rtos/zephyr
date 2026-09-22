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

#ifndef ZEPHYR_INCLUDE_ARCH_ARM64_EXCEPTION_H_
#define ZEPHYR_INCLUDE_ARCH_ARM64_EXCEPTION_H_

/* for assembler, only works with constants */

#ifdef _ASMLANGUAGE
#else
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct arch_esf {
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
#ifdef CONFIG_EXTRA_EXCEPTION_INFO
	/*
	 * Callee-saved registers (x19-x28). Saved on every exception
	 * entry and restored on every exception exit, adding 80 bytes
	 * of stack space and ~10 STP/LDP pairs to the interrupt
	 * latency of every exception.
	 */
	uint64_t x19;
	uint64_t x20;
	uint64_t x21;
	uint64_t x22;
	uint64_t x23;
	uint64_t x24;
	uint64_t x25;
	uint64_t x26;
	uint64_t x27;
	uint64_t x28;
#endif
	uint64_t lr;
	uint64_t spsr;
	uint64_t elr;
#ifdef CONFIG_FRAME_POINTER
	uint64_t fp;
#endif
#ifdef CONFIG_ARM64_SAFE_EXCEPTION_STACK
	uint64_t sp;
#endif
} __aligned(16);

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM64_EXCEPTION_H_ */
