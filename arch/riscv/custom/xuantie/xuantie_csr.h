/*
 * Copyright (C) 2017-2024 Alibaba Group Holding Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_CUSTOM_XUANTIE_CSR_H
#define ZEPHYR_CUSTOM_XUANTIE_CSR_H

#ifdef __ASSEMBLER__
/* clang-format off */
	.equ mxstatus, 0x07c0 /* m machine, x non-standard */
/* clang-format no */
#else

#ifdef __cplusplus
extern "C" {
#endif

	#define CSR_PMPADDR0    0x3B0
	#define CSR_PMPADDR1    0x3B1
	#define CSR_PMPADDR2    0x3B2
	#define CSR_PMPADDR3    0x3B3
	#define CSR_PMPADDR4    0x3B4
	#define CSR_PMPADDR5    0x3B5
	#define CSR_PMPADDR6    0x3B6
	#define CSR_PMPADDR7    0x3B7
	#define CSR_PMPCFG0     0x3A0
	#define CSR_MCOR        0x7C2
	#define CSR_MHCR        0x7C1
	#define CSR_MCCR2       0x7C3
	#define CSR_MHINT       0x7C5
	#define CSR_MSMPR       0x7F3
	#define CSR_MIE         0x304
	#define CSR_MSTATUS     0x300
	#define CSR_MXSTATUS    0x7C0
	#define CSR_MTVEC       0x305
	#define CSR_PLIC_BASE_ADDR  0x8000000
	#define CSR_CLINT_BASE_ADDR 0xC000000

#ifdef __cplusplus
}
#endif

#endif /* __ASSEMBLER__ */

#endif /* ZEPHYR_CUSTOM_XUANTIE_CSR_H */
