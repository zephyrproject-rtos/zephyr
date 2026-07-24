/*
 * Copyright (c) 2026 Colt Ma <fae@coltsmart.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * Macros for placing code and data in the zero-wait flash region
 * (first 128 KB of CH32V flash, 0x00000-0x20000).
 *
 *   ZERO_WAIT_TEXT   — place a function in zero-wait flash
 *   ZERO_WAIT_RODATA — place read-only data in zero-wait flash
 *
 * Typical usage:
 *
 *   #include <soc/zero_wait.h>
 *
 *   ZERO_WAIT_TEXT void my_isr_handler(const void *arg) { ... }
 *
 *   ZERO_WAIT_RODATA static const int table[] = { 1, 2, 3 };
 */

#ifndef ZERO_WAIT_H
#define ZERO_WAIT_H

#ifdef __ASSEMBLER__
#define ZERO_WAIT_TEXT   .section .zero_wait_text, "ax"
#define ZERO_WAIT_RODATA .section .zero_wait_rodata, "a"
#else
#define ZERO_WAIT_TEXT \
	__attribute__((section(".zero_wait_text")))
#define ZERO_WAIT_RODATA \
	__attribute__((section(".zero_wait_rodata")))
#endif

#endif /* ZERO_WAIT_H */
