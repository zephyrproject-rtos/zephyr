/* SPDX-License-Identifier: Apache-2.0
 *
 * Copyright(c) 2016 Intel Corporation. All rights reserved.
 *
 * Author: Liam Girdwood <liam.r.girdwood@linux.intel.com>
 */

#ifndef __INC_SOC_SHIM_H
#define __INC_SOC_SHIM_H

#include <soc/memory.h>
#include <stdint.h>

#define SHIM_CSR		0x00
#define SHIM_PISR		0x08
#define SHIM_PISRH		0x0c
#define SHIM_PIMR		0x10
#define SHIM_PIMRH		0x14
#define SHIM_ISRX		0x18
#define SHIM_ISRD		0x20
#define SHIM_IMRX		0x28
#define SHIM_IMRD		0x30
#define SHIM_IPCXL		0x38 /* IPC IA -> SST */
#define SHIM_IPCXH		0x3c /* IPC IA -> SST */
#define SHIM_IPCDL		0x40 /* IPC SST -> IA */
#define SHIM_IPCDH		0x44 /* IPC SST -> IA */
#define SHIM_ISRSC		0x48
#define SHIM_ISRLPESC		0x50
#define SHIM_IMRSCL		0x58
#define SHIM_IMRSCH		0x5c
#define SHIM_IMRLPESC		0x60
#define SHIM_IPCSCL		0x68
#define SHIM_IPCSCH		0x6c
#define SHIM_IPCLPESCL		0x70
#define SHIM_IPCLPESCH		0x74
#define SHIM_CLKCTL		0x78
#define SHIM_FR_LAT_REQ		0x80
#define SHIM_MISC		0x88
#define SHIM_EXT_TIMER_CNTLL	0xC0
#define SHIM_EXT_TIMER_CNTLH	0xC4
#define SHIM_EXT_TIMER_STAT	0xC8
#define SHIM_SSP0_DIVL		0xE8
#define SHIM_SSP0_DIVH		0xEC
#define SHIM_SSP1_DIVL		0xF0
#define SHIM_SSP1_DIVH		0xF4
#define SHIM_SSP2_DIVL		0xF8
#define SHIM_SSP2_DIVH		0xFC
#define SHIM_SSP3_DIVL		0x100
#define SHIM_SSP3_DIVH		0x104
#define SHIM_SSP4_DIVL		0x108
#define SHIM_SSP4_DIVH		0x10c
#define SHIM_SSP5_DIVL		0x110
#define SHIM_SSP5_DIVH		0x114

#define SHIM_SHIM_BEGIN		SHIM_CSR

#if defined CONFIG_BAYTRAIL

#define SHIM_SHIM_END		SHIM_SSP2_DIVH

#elif defined CONFIG_CHERRYTRAIL

#define SHIM_SHIM_END		SHIM_SSP5_DIVH

#endif

/* CSR 0x0 */
#define SHIM_CSR_RST		(0x1 << 0)
#define SHIM_CSR_VECTOR_SEL	(0x1 << 1)
#define SHIM_CSR_STALL		(0x1 << 2)
#define SHIM_CSR_PWAITMODE	(0x1 << 3)

/* PISR */
#define SHIM_PISR_EXT_TIMER	(1 << 10)

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
#define SHIM_IMRD_DMAC0		(0x1 << 21)
#define SHIM_IMRD_DMAC1		(0x1 << 22)
#define SHIM_IMRD_DMAC		(SHIM_IMRD_DMAC0 | SHIM_IMRD_DMAC1)

/*  IPCX / IPCCH */
#define	SHIM_IPCXH_DONE		(0x1 << 30)
#define	SHIM_IPCXH_BUSY		(0x1 << 31)

/*  IPCDH */
#define	SHIM_IPCDH_DONE		(0x1 << 30)
#define	SHIM_IPCDH_BUSY		(0x1 << 31)

/* ISRLPESC */
#define SHIM_ISRLPESC_DONE	(0x1 << 0)
#define SHIM_ISRLPESC_BUSY	(0x1 << 1)

/* IMRLPESC */
#define	SHIM_IMRLPESC_BUSY	(0x1 << 1)
#define	SHIM_IMRLPESC_DONE	(0x1 << 0)

/* IPCSCH */
#define SHIM_IPCSCH_DONE	(0x1 << 30)
#define SHIM_IPCSCH_BUSY	(0x1 << 31)

/* IPCLPESCH */
#define SHIM_IPCLPESCH_DONE	(0x1 << 30)
#define SHIM_IPCLPESCH_BUSY	(0x1 << 31)

/* CLKCTL */
#define SHIM_CLKCTL_SSP2_EN	(1 << 18)
#define SHIM_CLKCTL_SSP1_EN	(1 << 17)
#define SHIM_CLKCTL_SSP0_EN	(1 << 16)
#define SHIM_CLKCTL_FRCHNGGO	(1 << 5)
#define SHIM_CLKCTL_FRCHNGACK	(1 << 4)

/* SHIM_FR_LAT_REQ */
#define SHIM_FR_LAT_CLK_MASK	0x7

/* ext timer */
#define SHIM_EXT_TIMER_RUN	(1 << 31)
#define SHIM_EXT_TIMER_CLEAR	(1 << 30)

/* SSP M/N */
#define SHIM_SSP_DIV_BYP	(1 << 31)
#define SHIM_SSP_DIV_ENA	(1 << 30)
#define SHIM_SSP_DIV_UPD	(1 << 29)

#endif /* __INC_SOC_SHIM_H */
