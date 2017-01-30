/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the ARM Ltd MPS2.
 *
 */

#ifndef _ARM_MPS2_REGS_H_
#define _ARM_MPS2_REGS_H_

#include <stdint.h>

/* System Control Register (SYSCON) */
struct mps2_syscon {
	/* Offset: 0x000 (r/w) remap control register */
	volatile uint32_t remap;
	/* Offset: 0x004 (r/w) pmu control register */
	volatile uint32_t pmuctrl;
	/* Offset: 0x008 (r/w) reset option register */
	volatile uint32_t resetop;
	/* Offset: 0x00c (r/w) emi control register */
	volatile uint32_t emictrl;
	/* Offset: 0x010 (r/w) reset information register */
	volatile uint32_t rstinfo;
};

#endif /* _ARM_MPS2_REGS_H_ */
