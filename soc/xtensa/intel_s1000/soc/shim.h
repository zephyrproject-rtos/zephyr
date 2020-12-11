/* SPDX-License-Identifier: Apache-2.0
 *
 * Copyright(c) 2016 Intel Corporation. All rights reserved.
 *
 * Author: Liam Girdwood <liam.r.girdwood@linux.intel.com>
 *         Keyon Jie <yang.jie@linux.intel.com>
 */

#ifndef __PLATFORM_LIB_SHIM_H__
#define __PLATFORM_LIB_SHIM_H__

#include <sys/util.h>
#include <memory.h>

#ifndef ASSEMBLY
#include <stdint.h>
#endif

#if !defined(__ASSEMBLER__) && !defined(LINKER)
#include <sys/sys_io.h>
#include <arch/common/sys_io.h>
#endif

#ifndef BIT
#define BIT(b)			(1 << (b))
#endif

/* DSP IPC for Host Registers */
#define IPC_DIPCT		0x00
#define IPC_DIPCTE		0x04
#define IPC_DIPCI		0x08
#define IPC_DIPCIE		0x0c
#define IPC_DIPCCTL		0x10

/* DIPCT */
#define IPC_DIPCT_BUSY		BIT(31)
#define IPC_DIPCT_MSG_MASK	0x7FFFFFFF

/* DIPCTE */
#define IPC_DIPCTE_MSG_MASK	0x3FFFFFFF

/* DIPCI */
#define IPC_DIPCI_BUSY		BIT(31)
#define IPC_DIPCI_MSG_MASK	0x7FFFFFFF

/* DIPCIE */
#define IPC_DIPCIE_DONE		BIT(30)
#define IPC_DIPCIE_MSG_MASK	0x3FFFFFFF

/* DIPCCTL */
#define IPC_DIPCCTL_IPCIDIE	BIT(1)
#define IPC_DIPCCTL_IPCTBIE	BIT(0)

#define IPC_DSP_OFFSET		0x10

/* DSP IPC for intra DSP communication */
#define IPC_IDCTFC(x)		(0x0 + x * IPC_DSP_OFFSET)
#define IPC_IDCTEFC(x)		(0x4 + x * IPC_DSP_OFFSET)
#define IPC_IDCITC(x)		(0x8 + x * IPC_DSP_OFFSET)
#define IPC_IDCIETC(x)		(0xc + x * IPC_DSP_OFFSET)
#define IPC_IDCCTL		0x50

/* IDCTFC */
#define IPC_IDCTFC_BUSY		BIT(31)
#define IPC_IDCTFC_MSG_MASK	0x7FFFFFFF

/* IDCTEFC */
#define IPC_IDCTEFC_MSG_MASK	0x3FFFFFFF

/* IDCITC */
#define IPC_IDCITC_BUSY		BIT(31)
#define IPC_IDCITC_MSG_MASK	0x7FFFFFFF

/* IDCIETC */
#define IPC_IDCIETC_DONE	BIT(30)
#define IPC_IDCIETC_MSG_MASK	0x3FFFFFFF

/* IDCCTL */
#define IPC_IDCCTL_IDCIDIE(x)	(0x100 << (x))
#define IPC_IDCCTL_IDCTBIE(x)	BIT(x)

#define IRQ_CPU_OFFSET	0x40

#define REG_IRQ_IL2MSD(xcpu)	(0x0 + (xcpu * IRQ_CPU_OFFSET))
#define REG_IRQ_IL2MCD(xcpu)	(0x4 + (xcpu * IRQ_CPU_OFFSET))
#define REG_IRQ_IL2MD(xcpu)	(0x8 + (xcpu * IRQ_CPU_OFFSET))
#define REG_IRQ_IL2SD(xcpu)	(0xc + (xcpu * IRQ_CPU_OFFSET))

/* all mask valid bits */
#define REG_IRQ_IL2MD_ALL	0x03F181F0

#define REG_IRQ_IL3MSD(xcpu)	(0x10 + (xcpu * IRQ_CPU_OFFSET))
#define REG_IRQ_IL3MCD(xcpu)	(0x14 + (xcpu * IRQ_CPU_OFFSET))
#define REG_IRQ_IL3MD(xcpu)	(0x18 + (xcpu * IRQ_CPU_OFFSET))
#define REG_IRQ_IL3SD(xcpu)	(0x1c + (xcpu * IRQ_CPU_OFFSET))

/* all mask valid bits */
#define REG_IRQ_IL3MD_ALL	0x807F81FF

#define REG_IRQ_IL4MSD(xcpu)	(0x20 + (xcpu * IRQ_CPU_OFFSET))
#define REG_IRQ_IL4MCD(xcpu)	(0x24 + (xcpu * IRQ_CPU_OFFSET))
#define REG_IRQ_IL4MD(xcpu)	(0x28 + (xcpu * IRQ_CPU_OFFSET))
#define REG_IRQ_IL4SD(xcpu)	(0x2c + (xcpu * IRQ_CPU_OFFSET))

/* all mask valid bits */
#define REG_IRQ_IL4MD_ALL	0x807F81FF

#define REG_IRQ_IL5MSD(xcpu)	(0x30 + (xcpu * IRQ_CPU_OFFSET))
#define REG_IRQ_IL5MCD(xcpu)	(0x34 + (xcpu * IRQ_CPU_OFFSET))
#define REG_IRQ_IL5MD(xcpu)	(0x38 + (xcpu * IRQ_CPU_OFFSET))
#define REG_IRQ_IL5SD(xcpu)	(0x3c + (xcpu * IRQ_CPU_OFFSET))

/* all mask valid bits */
#define REG_IRQ_IL5MD_ALL	0xFFFFC0CF

#define REG_IRQ_IL2RSD		0x100
#define REG_IRQ_IL3RSD		0x104
#define REG_IRQ_IL4RSD		0x108
#define REG_IRQ_IL5RSD		0x10c

#define REG_IRQ_LVL5_LP_GPDMA0_MASK		(0xff << 16)
#define REG_IRQ_LVL5_LP_GPDMA1_MASK		(0xff << 24)

/* DSP Shim Registers */
#define SHIM_DSPWC		0x20 /* DSP Wall Clock */
#define SHIM_DSPWCTCS		0x28 /* DSP Wall Clock Timer Control & Status */
#define SHIM_DSPWCT0C		0x30 /* DSP Wall Clock Timer 0 Compare */
#define SHIM_DSPWCT1C		0x38 /* DSP Wall Clock Timer 1 Compare */

#define SHIM_DSPWCTCS_T1T	BIT(5) /* Timer 1 triggered */
#define SHIM_DSPWCTCS_T0T	BIT(4) /* Timer 0 triggered */
#define SHIM_DSPWCTCS_T1A	BIT(1) /* Timer 1 armed */
#define SHIM_DSPWCTCS_T0A	BIT(0) /* Timer 0 armed */

/** \brief Clock control */
#define SHIM_CLKCTL		0x78

/** \brief Clock status */
#define SHIM_CLKSTS		0x7C

/** \brief Request Audio PLL Clock */
#define SHIM_CLKCTL_RAPLLC	BIT(31)

/** \brief Request XTAL Oscillator Clock */
#define SHIM_CLKCTL_RXOSCC	BIT(30)

/** \brief Request Fast RING Oscillator Clock */
#define SHIM_CLKCTL_RFROSCC	BIT(29)

/** \brief LP GPDMA Force Dynamic Clock Gating bits, 0: enable */
#define SHIM_CLKCTL_LPGPDMAFDCGB(x)	BIT(26 + x)

/** \brief DMIC Force Dynamic Clock Gating */
#define SHIM_CLKCTL_DMICFDCGB           BIT(24)

/** \brief I2S Force Dynamic Clock Gating */
#define SHIM_CLKCTL_I2SFDCGB(x)		BIT(20 + x)

/** \brief I2S Extension Force Dynamic Clock Gating */
#define SHIM_CLKCTL_I2SEFDCGB(x)	BIT(18 + x)

/** \brief Tensilica Core Prevent Local Clock Gating */
#define SHIM_CLKCTL_TCPLCG_EN(x)	BIT(16 + (x))
#define SHIM_CLKCTL_TCPLCG_DIS(x)	0

/** \brief Core clock PLL divisor */
#define SHIM_CLKCTL_DPCS_MASK(x)	(0x3 << (8 + x * 2))
#define SHIM_CLKCTL_DPCS_DIV1(x)	(0x0 << (8 + x * 2))
#define SHIM_CLKCTL_DPCS_DIV2(x)	(0x1 << (8 + x * 2))
#define SHIM_CLKCTL_DPCS_DIV4(x)	(0x3 << (8 + x * 2))

/** \brief Tensilica Core Prevent Audio PLL Shutdown */
#define SHIM_CLKCTL_TCPAPLLS_EN		BIT(7)
#define SHIM_CLKCTL_TCPAPLLS_DIS	0

/** \brief LP domain clock select, 0: PLL, 1: oscillator */
#define SHIM_CLKCTL_LDCS_XTAL	BIT(5)
#define SHIM_CLKCTL_LDCS_PLL	0

/** \brief HP domain clock select */
#define SHIM_CLKCTL_HDCS	BIT(4)
#define SHIM_CLKCTL_HDCS_XTAL	BIT(4)
#define SHIM_CLKCTL_HDCS_PLL	0

/** \brief LP domain oscillator clock select select, 0: XTAL, 1: Fast RING */
#define SHIM_CLKCTL_LDOCS	BIT(3)

/** \brief HP domain oscillator clock select select, 0: XTAL, 1: Fast RING */
#define SHIM_CLKCTL_HDOCS	BIT(2)

/** \brief LP memory clock PLL divisor, 0: div by 2, 1: div by 4 */
#define SHIM_CLKCTL_LPMPCS_DIV4	BIT(1)
#define SHIM_CLKCTL_LPMPCS_DIV2	0

/** \brief HP memory clock PLL divisor, 0: div by 2, 1: div by 4 */
#define SHIM_CLKCTL_HPMPCS_DIV4	BIT(0)
#define SHIM_CLKCTL_HPMPCS_DIV2	0

#define SHIM_PWRCTL		0x90
#define SHIM_PWRSTS		0x92
#define SHIM_LPSCTL		0x94

/* HP & LP SRAM Power Gating */
#define SHIM_HSPGCTL		0x80
#define SHIM_LSPGCTL		0x84
#define SHIM_SPSREQ		0xa0

#define SHIM_SPSREQ_RVNNP	BIT(0)

/** \brief GPDMA shim registers Control */
#define SHIM_GPDMA_BASE_OFFSET	0xC00
#define SHIM_GPDMA_BASE(x)	(SHIM_GPDMA_BASE_OFFSET + (x) * 0x80)

/** \brief GPDMA Channel Linear Link Position Control */
#define SHIM_GPDMA_CHLLPC(x, y)		(SHIM_GPDMA_BASE(x) + (y) * 0x10)
#define SHIM_GPDMA_CHLLPC_EN		BIT(5)
#define SHIM_GPDMA_CHLLPC_DHRS(x)	SET_BITS(4, 0, x)

/** \brief LDO Control */
#define SHIM_LDOCTL		0xA4
#define SHIM_LDOCTL_HPSRAM_MASK	(3 << 0)
#define SHIM_LDOCTL_LPSRAM_MASK	(3 << 2)
#define SHIM_LDOCTL_HPSRAM_LDO_ON	(3 << 0)
#define SHIM_LDOCTL_LPSRAM_LDO_ON	(3 << 2)
#define SHIM_LDOCTL_HPSRAM_LDO_BYPASS	BIT(0)
#define SHIM_LDOCTL_LPSRAM_LDO_BYPASS	BIT(2)
#define SHIM_LDOCTL_HPSRAM_LDO_OFF	(0 << 0)
#define SHIM_LDOCTL_LPSRAM_LDO_OFF	(0 << 2)

#define SHIM_HSPGISTS		0xb0
#define SHIM_LSPGISTS		0xb4
#define LSPGISTS		(SHIM_BASE + SHIM_LSPGISTS)


#define SHIM_LPSCTL_FDSPRUN	BIT(9)
#define SHIM_LPSCTL_FDMARUN	BIT(8)

#define SHIM_L2_MECS		(SHIM_BASE + 0xd0)

#define SHIM_LPGPDMAC(x)	(0x1110 + (2 * x))
#define SHIM_LPGPDMAC_CTLOSEL	BIT(15)
#define SHIM_LPGPDMAC_CHOSEL	0xFF

#define SHIM_DSPIOPO		0x1118
#define SHIM_DSPIOPO_DMICOSEL	BIT(0)
#define SHIM_DSPIOPO_I2SOSEL	(0x3F << 8)

#define SHIM_GENO		0x111C
#define SHIM_GENO_SHIMOSEL	BIT(0)
#define SHIM_GENO_MDIVOSEL	BIT(1)
#define SHIM_GENO_DIOPTOSEL	BIT(2)

#define SHIM_L2_CACHE_CTRL	(SHIM_BASE + 0x500)
#define SHIM_L2_PREF_CFG	(SHIM_BASE + 0x508)
#define SHIM_L2_CACHE_PREF	(SHIM_BASE + 0x510)

#define SHIM_SVCFG			0xF4
#define SHIM_SVCFG_FORCE_L1_EXIT	BIT(1)

/* host windows */
#define DMWBA(x)		(HOST_WIN_BASE(x) + 0x0)
#define DMWLO(x)		(HOST_WIN_BASE(x) + 0x4)

#define DMWBA_ENABLE		BIT(0)
#define DMWBA_READONLY		BIT(1)


#endif /* __PLATFORM_LIB_SHIM_H__ */
