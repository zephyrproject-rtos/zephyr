/*
 * Copyright (c) 2026 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_RISCV_CUSTOM_THEAD_CACHE_XTHEADCMO_CSR_H_
#define ZEPHYR_ARCH_RISCV_CUSTOM_THEAD_CACHE_XTHEADCMO_CSR_H_

/* "a Hardware CSR" */
#define THEAD_MHCR "0x7C1"

/* I-cache enable */
#define THEAD_MHCR_IE	BIT(0)
/* D-cache enable */
#define THEAD_MHCR_DE	BIT(1)
/* Cache write-back enable */
#define THEAD_MHCR_WB	BIT(2)
/* Cache write-allocate enable */
#define THEAD_MHCR_WA	BIT(3)
/* Return stack enable */
#define THEAD_MHCR_RS	BIT(4)

#endif /* ZEPHYR_ARCH_RISCV_CUSTOM_THEAD_CACHE_XTHEADCMO_CSR_H_ */
