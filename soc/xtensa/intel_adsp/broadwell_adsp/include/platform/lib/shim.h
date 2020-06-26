/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2016 Intel Corporation. All rights reserved.
 *
 * Author: Liam Girdwood <liam.r.girdwood@linux.intel.com>
 */

#ifdef __SOF_LIB_SHIM_H__

#ifndef __PLATFORM_LIB_SHIM_H__
#define __PLATFORM_LIB_SHIM_H__

#include <sof/lib/io.h>
#include <sof/lib/memory.h>
#include <stdint.h>

#define SHIM_CSR		0x00
#define SHIM_ISRX		0x18
#define SHIM_ISRD		0x20
#define SHIM_IMRX		0x28
#define SHIM_IMRD		0x30
#define SHIM_IPCX		0x38 /* IPC IA -> SST */
#define SHIM_IPCD		0x40 /* IPC SST -> IA */

#define SHIM_CLKCTL		0x78

#define SHIM_CSR2		0x80
#define SHIM_LTRC		0xE0
#define SHIM_HMDC		0xE8

#define SHIM_SHIM_BEGIN		SHIM_CSR
#define SHIM_SHIM_END		SHIM_HMDC

/* CSR 0x0 */
#define SHIM_CSR_RST		(0x1 << 1)
#define SHIM_CSR_STALL		(0x1 << 10)
#define SHIM_CSR_SDPM0          (0x1 << 11)
#define SHIM_CSR_SDPM1          (0x1 << 12)
#define SHIM_CSR_PCE		(0x1 << 15)
#define SHIM_CSR_SFCR0          (0x1 << 27)
#define SHIM_CSR_SFCR1          (0x1 << 28)
#define SHIM_CSR_DCS(x)         (x << 4)
#define SHIM_CSR_DCS_MASK       (0x7 << 4)
#define SHIM_CSR_SFCR_SSP(x)	(1 << (27 + x))

/*  ISRX 0x18 */
#define SHIM_ISRX_BUSY		(0x1 << 1)
#define SHIM_ISRX_DONE		(0x1 << 0)

/*  ISRD / ISD */
#define SHIM_ISRD_BUSY		(0x1 << 1)
#define SHIM_ISRD_DONE		(0x1 << 0)

/* IMRX / IMC */
#define SHIM_IMRX_BUSY		(0x1 << 1)
#define SHIM_IMRX_DONE		(0x1 << 0)

/* IMRD / IMD */
#define SHIM_IMRD_DONE		(0x1 << 0)
#define SHIM_IMRD_BUSY		(0x1 << 1)
#define SHIM_IMRD_SSP0		(0x1 << 16)
#define SHIM_IMRD_SSP1		(0x1 << 17)
#define SHIM_IMRD_DMAC0		(0x1 << 21)
#define SHIM_IMRD_DMAC1		(0x1 << 22)
#define SHIM_IMRD_DMAC		(SHIM_IMRD_DMAC0 | SHIM_IMRD_DMAC1)

/*  IPCX / IPCCH */
#define	SHIM_IPCX_DONE		(0x1 << 30)
#define	SHIM_IPCX_BUSY		(0x1 << 31)

/*  IPCDH */
#define	SHIM_IPCD_DONE		(0x1 << 30)
#define	SHIM_IPCD_BUSY		(0x1 << 31)

/* CLKCTL */
#define SHIM_CLKCTL_SMOS(x)	(x << 24)
#define SHIM_CLKCTL_MASK		(3 << 24)
#define SHIM_CLKCTL_DCPLCG	(1 << 18)
#define SHIM_CLKCTL_EN_SSP(x)	(1 << (16 + x))

/* CSR2 / CS2 */
#define SHIM_CSR2_SDFD_SSP0	(1 << 1)
#define SHIM_CSR2_SDFD_SSP1	(1 << 2)

/* LTRC */
#define SHIM_LTRC_VAL(x)	(x << 0)

/* HMDC */
#define SHIM_HMDC_HDDA0(x)      (x << 0)
#define SHIM_HMDC_HDDA1(x)      (x << 8)
#define SHIM_HMDC_HDDA_CH_MASK  0xFF
#define SHIM_HMDC_HDDA_E0_ALLCH SHIM_HMDC_HDDA0(SHIM_HMDC_HDDA_CH_MASK)
#define SHIM_HMDC_HDDA_E1_ALLCH SHIM_HMDC_HDDA1(SHIM_HMDC_HDDA_CH_MASK)
#define SHIM_HMDC_HDDA_ALLCH    (SHIM_HMDC_HDDA_E0_ALLCH | \
				SHIM_HMDC_HDDA_E1_ALLCH)

/* PMCS */
#define PCI_PMCS		0x84
#define PCI_PMCS_PS_MASK	0x3

static inline uint32_t shim_read(uint32_t reg)
{
	return *((volatile uint32_t*)(SHIM_BASE + reg));
}

static inline void shim_write(uint32_t reg, uint32_t val)
{
	*((volatile uint32_t*)(SHIM_BASE + reg)) = val;
}

static inline void shim_update_bits(uint32_t reg, uint32_t mask,
	uint32_t value)
{
	io_reg_update_bits(SHIM_BASE + reg, mask, value);
}

#endif /* __PLATFORM_LIB_SHIM_H__ */

#else

#error "This file shouldn't be included from outside of sof/lib/shim.h"

#endif /* __SOF_LIB_SHIM_H__ */
