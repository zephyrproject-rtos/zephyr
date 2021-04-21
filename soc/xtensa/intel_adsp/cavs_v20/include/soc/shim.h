/* SPDX-License-Identifier: Apache-2.0
 *
 * Copyright(c) 2017 Intel Corporation. All rights reserved.
 *
 * Author: Liam Girdwood <liam.r.girdwood@linux.intel.com>
 *         Keyon Jie <yang.jie@linux.intel.com>
 *         Rander Wang <rander.wang@intel.com>
 */

#ifndef __PLATFORM_LIB_SHIM_H__
#define __PLATFORM_LIB_SHIM_H__

#include <sys/util.h>
#include <soc/memory.h>

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
#define IPC_DIPCTDR		0x00
#define IPC_DIPCTDA		0x04
#define IPC_DIPCTDD		0x08
#define IPC_DIPCIDR		0x10
#define IPC_DIPCIDA		0x14
#define IPC_DIPCIDD		0x18
#define IPC_DIPCCTL		0x28

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
#define REG_IRQ_IL2MD_ALL		0x03F181F0

#define REG_IRQ_IL3MSD(xcpu)	(0x10 + (xcpu * IRQ_CPU_OFFSET))
#define REG_IRQ_IL3MCD(xcpu)	(0x14 + (xcpu * IRQ_CPU_OFFSET))
#define REG_IRQ_IL3MD(xcpu)	(0x18 + (xcpu * IRQ_CPU_OFFSET))
#define REG_IRQ_IL3SD(xcpu)	(0x1c + (xcpu * IRQ_CPU_OFFSET))

/* all mask valid bits */
#define REG_IRQ_IL3MD_ALL		0x807F81FF

#define REG_IRQ_IL4MSD(xcpu)	(0x20 + (xcpu * IRQ_CPU_OFFSET))
#define REG_IRQ_IL4MCD(xcpu)	(0x24 + (xcpu * IRQ_CPU_OFFSET))
#define REG_IRQ_IL4MD(xcpu)	(0x28 + (xcpu * IRQ_CPU_OFFSET))
#define REG_IRQ_IL4SD(xcpu)	(0x2c + (xcpu * IRQ_CPU_OFFSET))

/* all mask valid bits */
#define REG_IRQ_IL4MD_ALL		0x807F81FF

#define REG_IRQ_IL5MSD(xcpu)	(0x30 + (xcpu * IRQ_CPU_OFFSET))
#define REG_IRQ_IL5MCD(xcpu)	(0x34 + (xcpu * IRQ_CPU_OFFSET))
#define REG_IRQ_IL5MD(xcpu)	(0x38 + (xcpu * IRQ_CPU_OFFSET))
#define REG_IRQ_IL5SD(xcpu)	(0x3c + (xcpu * IRQ_CPU_OFFSET))

/* all mask valid bits */
#define REG_IRQ_IL5MD_ALL		0xFFFFC0CF

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

/** \brief Request HP RING Oscillator Clock */
#define SHIM_CLKCTL_RHROSCC	BIT(31)

/** \brief Request XTAL Oscillator Clock */
#define SHIM_CLKCTL_RXOSCC	BIT(30)

/** \brief Request LP RING Oscillator Clock */
#define SHIM_CLKCTL_RLROSCC	BIT(29)

/** \brief Tensilica Core Prevent Local Clock Gating */
#define SHIM_CLKCTL_TCPLCG_EN(x)	BIT(16 + (x))
#define SHIM_CLKCTL_TCPLCG_DIS(x)	0
#define SHIM_CLKCTL_TCPLCG_DIS_ALL	(SHIM_CLKCTL_TCPLCG_DIS(0) | \
					 SHIM_CLKCTL_TCPLCG_DIS(1) | \
					 SHIM_CLKCTL_TCPLCG_DIS(2) | \
					 SHIM_CLKCTL_TCPLCG_DIS(3))

/** \brief Oscillator Clock Select*/
#define SHIM_CLKCTL_OCS_HP_RING		BIT(2)
#define SHIM_CLKCTL_OCS_LP_RING		0

/** \brief LP Memory Clock Select */
#define SHIM_CLKCTL_LMCS_DIV2	0
#define SHIM_CLKCTL_LMCS_DIV4	BIT(1)

/** \brief HP Memory Clock Select */
#define SHIM_CLKCTL_HMCS_DIV2	0
#define SHIM_CLKCTL_HMCS_DIV4	BIT(0)

/* Core clock PLL divisor */
#define SHIM_CLKCTL_DPCS_MASK(x)	BIT(2)

/* Prevent Audio PLL Shutdown */
#define SHIM_CLKCTL_TCPAPLLS	BIT(7)

/* 0--from PLL, 1--from oscillator */
#define SHIM_CLKCTL_HDCS	BIT(4)

/* Oscillator select */
#define SHIM_CLKCTL_HDOCS	BIT(2)

/* HP memory clock PLL divisor */
#define SHIM_CLKCTL_HPMPCS	BIT(0)

/** \brief Mask for requesting clock
 */
#define SHIM_CLKCTL_OSC_REQUEST_MASK \
	(SHIM_CLKCTL_RHROSCC | SHIM_CLKCTL_RXOSCC | \
	SHIM_CLKCTL_RLROSCC)

/** \brief Mask for setting previously requested clock
 */
#define SHIM_CLKCTL_OSC_SOURCE_MASK \
	(SHIM_CLKCTL_OCS_HP_RING | SHIM_CLKCTL_LMCS_DIV4 | \
	SHIM_CLKCTL_HMCS_DIV4)

/** \brief Clock status */
#define SHIM_CLKSTS		0x7C

/** \brief HP RING Oscillator Clock Status */
#define SHIM_CLKSTS_HROSCCS	BIT(31)

/** \brief XTAL Oscillator Clock Status */
#define SHIM_CLKSTS_XOSCCS	BIT(30)

/** \brief LP RING Oscillator Clock Status */
#define SHIM_CLKSTS_LROSCCS	BIT(29)

#define SHIM_PWRCTL		0x90
#define SHIM_PWRCTL_TCPDSPPG(x)	BIT(x)
#define SHIM_PWRCTL_TCPCTLPG	BIT(4)

#define SHIM_PWRSTS		0x92

#define SHIM_LPSCTL		0x94
#define SHIM_LPSCTL_BID		BIT(7)
#define SHIM_LPSCTL_FDSPRUN	BIT(9)
#define SHIM_LPSCTL_BATTR_0	BIT(12)

/** \brief GPDMA shim registers Control */
#define SHIM_GPDMA_BASE_OFFSET	0x6500
#define SHIM_GPDMA_BASE(x)	(SHIM_GPDMA_BASE_OFFSET + (x) * 0x100)

/** \brief GPDMA Clock Control */
#define SHIM_GPDMA_CLKCTL(x)	(SHIM_GPDMA_BASE(x) + 0x4)
/* LP GPDMA Force Dynamic Clock Gating bits, 0--enable */
#define SHIM_CLKCTL_LPGPDMAFDCGB	BIT(0)

/** \brief GPDMA Channel Linear Link Position Control */
#define SHIM_GPDMA_CHLLPC(x, y)		(SHIM_GPDMA_BASE(x) + 0x10 + (y) * 0x10)
#define SHIM_GPDMA_CHLLPC_EN		BIT(7)
#define SHIM_GPDMA_CHLLPC_DHRS(x)	SET_BITS(6, 0, x)

#define L2LMCAP			0x71D00
#define L2MPAT			0x71D04

#define HSPGCTL0		0x71D10
#define HSRMCTL0		0x71D14
#define HSPGISTS0		0x71D18

#define SHIM_HSPGCTL(x)		(HSPGCTL0 + 0x10 * (x))
#define SHIM_HSPGISTS(x)	(HSPGISTS0 + 0x10 * (x))

#define HSPGCTL1		0x71D20
#define HSRMCTL1		0x71D24
#define HSPGISTS1		0x71D28

#define LSPGCTL			0x71D50
#define LSRMCTL			0x71D54
#define LSPGISTS		0x71D58

#define SHIM_L2_MECS		(SHIM_BASE + 0xd0)

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

#define DSP_INIT_LPGPDMA(x)	(0x71A60 + (2*x))
#define LPGPDMA_CTLOSEL_FLAG	BIT(15)
#define LPGPDMA_CHOSEL_FLAG	0xFF

#define DSP_INIT_IOPO	0x71A68
#define IOPO_DMIC_FLAG		BIT(0)
#define IOPO_I2S_FLAG GENMASK(DAI_NUM_SSP_BASE + DAI_NUM_SSP_EXT + 7, 8)

#define DSP_INIT_GENO	0x71A6C
#define GENO_MDIVOSEL		BIT(1)
#define GENO_DIOPTOSEL		BIT(2)

#define DSP_INIT_ALHO	0x71A70
#define ALHO_ASO_FLAG		BIT(0)
#define ALHO_CSO_FLAG		BIT(1)
#define ALHO_CFO_FLAG		BIT(2)

#define SHIM_SVCFG			0xF4
#define SHIM_SVCFG_FORCE_L1_EXIT	BIT(1)

/* host windows */
#define DMWBA(x)		(HOST_WIN_BASE(x) + 0x0)
#define DMWLO(x)		(HOST_WIN_BASE(x) + 0x4)

#define DMWBA_ENABLE		BIT(0)
#define DMWBA_READONLY		BIT(1)

/* DMIC power ON bit */
#define DMICLCTL_SPA	((uint32_t) BIT(0))

/* DMIC disable clock gating */
#define DMIC_DCGD	((uint32_t) BIT(30))

#endif /* __PLATFORM_LIB_SHIM_H__ */
