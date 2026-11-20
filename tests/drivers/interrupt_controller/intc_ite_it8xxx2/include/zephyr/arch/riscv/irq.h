/*
 * Copyright 2026 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RISCV_IRQ_H_
#define ZEPHYR_INCLUDE_ARCH_RISCV_IRQ_H_

#define MIP_MEIP (1UL << 11)
#define csr_read(csr) 0UL
#define csr_set(csr, val) \
	do {              \
	} while (0)

#endif /* ZEPHYR_INCLUDE_ARCH_RISCV_IRQ_H_ */
