/*
 * Copyright (c) 2022 Alexandre Mergnat <amergnat@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RISCV_BARRIER_H_
#define ZEPHYR_INCLUDE_ARCH_RISCV_BARRIER_H_

#ifndef _ASMLANGUAGE

#define z_full_mb()	__asm__ volatile ("fence iorw, iorw" : : : "memory")
#define z_read_mb()	__asm__ volatile ("fence ir, ir" : : : "memory")
#define z_write_mb()	__asm__ volatile ("fence ow, ow" : : : "memory")

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_RISCV_BARRIER_H_ */
