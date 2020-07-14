/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <errno.h>

#include <sys/sys_io.h>

#include "memory.h"

#ifndef __INC_SOC_H
#define __INC_SOC_H

/* macros related to interrupt handling */
#define XTENSA_IRQ_NUM_SHIFT			0
#define CAVS_IRQ_NUM_SHIFT			8
#define XTENSA_IRQ_NUM_MASK			0xff
#define CAVS_IRQ_NUM_MASK			0xff

/*
 * IRQs are mapped on 2 levels. 3rd and 4th level are left as 0x00.
 *
 * 1. Peripheral Register bit offset.
 * 2. CAVS logic bit offset.
 */
#define XTENSA_IRQ_NUMBER(_irq) \
	((_irq >> XTENSA_IRQ_NUM_SHIFT) & XTENSA_IRQ_NUM_MASK)
#define CAVS_IRQ_NUMBER(_irq) \
	(((_irq >> CAVS_IRQ_NUM_SHIFT) & CAVS_IRQ_NUM_MASK) - 1)

/* Macro that aggregates the bi-level interrupt into an IRQ number */
#define SOC_AGGREGATE_IRQ(cavs_irq, core_irq)		\
	( \
	 ((core_irq & XTENSA_IRQ_NUM_MASK) << XTENSA_IRQ_NUM_SHIFT) | \
	 (((cavs_irq + 1) & CAVS_IRQ_NUM_MASK) << CAVS_IRQ_NUM_SHIFT) \
	)

#define CAVS_L2_AGG_INT_LEVEL2			DT_IRQN(DT_INST(0, intel_cavs_intc))
#define CAVS_L2_AGG_INT_LEVEL3			DT_IRQN(DT_INST(1, intel_cavs_intc))
#define CAVS_L2_AGG_INT_LEVEL4			DT_IRQN(DT_INST(2, intel_cavs_intc))
#define CAVS_L2_AGG_INT_LEVEL5			DT_IRQN(DT_INST(3, intel_cavs_intc))

#define CAVS_ICTL_INT_CPU_OFFSET(x)		(0x40 * x)

#define IOAPIC_EDGE				0
#define IOAPIC_HIGH				0

/* low power DMACs */
#define LP_GP_DMA_SIZE				0x00001000
#define DW_DMA0_BASE_ADDR			0x0000C000
#define DW_DMA1_BASE_ADDR			(0x0000C000 +\
						1 * LP_GP_DMA_SIZE)
#define DW_DMA2_BASE_ADDR			(0x0000C000 +\
						2 * LP_GP_DMA_SIZE)

#define DW_DMA0_IRQ				0x00001110
#define DW_DMA1_IRQ				0x0000010A
#define DW_DMA2_IRQ				0x0000010D

/* address of DMA ownership register. We need to properly configure
 * this register in order to access the DMA registers.
 */
#define CAVS_DMA0_OWNERSHIP_REG			(0x00071A60)
#define CAVS_DMA1_OWNERSHIP_REG			(0x00071A62)
#define CAVS_DMA2_OWNERSHIP_REG			(0x00071A64)

#define DMA_HANDSHAKE_DMIC_RXA			0
#define DMA_HANDSHAKE_DMIC_RXB			1
#define DMA_HANDSHAKE_SSP0_TX			2
#define DMA_HANDSHAKE_SSP0_RX			3
#define DMA_HANDSHAKE_SSP1_TX			4
#define DMA_HANDSHAKE_SSP1_RX			5
#define DMA_HANDSHAKE_SSP2_TX			6
#define DMA_HANDSHAKE_SSP2_RX			7
#define DMA_HANDSHAKE_SSP3_TX			8
#define DMA_HANDSHAKE_SSP3_RX			9

/* DMA Channel Allocation
 * FIXME: I2S Driver assigns channel in Kconfig.
 * Perhaps DTS is a better option
 */
#define DMIC_DMA_DEV_NAME			CONFIG_DMA_0_NAME
#define DMA_CHANNEL_DMIC_RXA			0
#define DMA_CHANNEL_DMIC_RXB			1

/* I2S */
#define I2S_CAVS_IRQ(i2s_num)			\
	SOC_AGGREGATE_IRQ(0, (i2s_num), CAVS_L2_AGG_INT_LEVEL5)

#define I2S0_CAVS_IRQ				I2S_CAVS_IRQ(0)
#define I2S1_CAVS_IRQ				I2S_CAVS_IRQ(1)
#define I2S2_CAVS_IRQ				I2S_CAVS_IRQ(2)
#define I2S3_CAVS_IRQ				I2S_CAVS_IRQ(3)

#define SSP_MN_DIV_SIZE				(8)
#define SSP_MN_DIV_BASE(x)			\
	(0x00078D00 + ((x) * SSP_MN_DIV_SIZE))

/* MCLK control */
#define SOC_MCLK_DIV_CTRL_BASE			0x00008E00
#define SOC_NUM_MCLK_OUTPUTS			2
#define SOC_MDIVCTRL_MCLK_OUT_EN(mclk)		BIT(mclk)
#define SOC_MDIVXR_SET_DIVIDER_BYPASS		BIT_MASK(12)

struct soc_mclk_control_regs {
	uint32_t	mdivctrl;
	uint32_t	reserved[31];
	uint32_t	mdivxr[SOC_NUM_MCLK_OUTPUTS];
};

#define PDM_BASE				0x00010000

#define SOC_NUM_LPGPDMAC			3
#define SOC_NUM_CHANNELS_IN_DMAC		8

/* SOC Resource Allocation Registers */
#define SOC_RESOURCE_ALLOC_REG_BASE		0x00071A60
/* bit field definition for LP GPDMA ownership register */
#define SOC_LPGPDMAC_OWNER_DSP			\
	(BIT(15) | BIT_MASK(SOC_NUM_CHANNELS_IN_DMAC))

#define SOC_NUM_I2S_INSTANCES			4
/* bit field definition for IO peripheral ownership register */
#define SOC_DSPIOP_I2S_OWNSEL_DSP		\
	(BIT_MASK(SOC_NUM_I2S_INSTANCES) << 8)
#define SOC_DSPIOP_DMIC_OWNSEL_DSP		BIT(0)

/* bit field definition for general ownership register */
#define SOC_GENO_TIMESTAMP_OWNER_DSP		BIT(2)
#define SOC_GENO_MNDIV_OWNER_DSP		BIT(1)

struct soc_resource_alloc_regs {
	union {
		uint16_t	lpgpdmacxo[SOC_NUM_LPGPDMAC];
		uint16_t	reserved[4];
	};
	uint32_t	dspiopo;
	uint32_t	geno;
};

/* DMIC SHIM Registers */
#define SOC_DMIC_SHIM_REG_BASE			0x00071E80
#define SOC_DMIC_SHIM_DMICLCTL_SPA		BIT(0)
#define SOC_DMIC_SHIM_DMICLCTL_CPA		BIT(8)

struct soc_dmic_shim_regs {
	uint32_t	dmiclcap;
	uint32_t	dmiclctl;
};

/* SOC DSP SHIM Registers */
#define SOC_DSP_SHIM_REG_BASE			0x00001000

/* SOC DSP SHIM Register - Clock Control */
#define SOC_CLKCTL_REQ_AUDIO_PLL_CLK		BIT(31)
#define SOC_CLKCTL_REQ_XTAL_CLK			BIT(30)
#define SOC_CLKCTL_REQ_FAST_CLK			BIT(29)

#define SOC_CLKCTL_TCPLCG_POS(x)		(16 + x)
#define SOC_CLKCTL_TCPLCG_DIS(x)		(1 << SOC_CLKCTL_TCPLCG_POS(x))

#define SOC_CLKCTL_DPCS_POS(x)			(8 + x)
#define SOC_CLKCTL_DPCS_DIV1(x)			(0 << SOC_CLKCTL_DPCS_POS(x))
#define SOC_CLKCTL_DPCS_DIV2(x)			(1 << SOC_CLKCTL_DPCS_POS(x))
#define SOC_CLKCTL_DPCS_DIV4(x)			(3 << SOC_CLKCTL_DPCS_POS(x))

#define SOC_CLKCTL_TCPAPLLS			BIT(7)

#define SOC_CLKCTL_LDCS_POS			(5)
#define SOC_CLKCTL_LDCS_LMPCS			(0 << SOC_CLKCTL_LDCS_POS)
#define SOC_CLKCTL_LDCS_LDOCS			(1 << SOC_CLKCTL_LDCS_POS)

#define SOC_CLKCTL_HDCS_POS			(4)
#define SOC_CLKCTL_HDCS_HMPCS			(0 << SOC_CLKCTL_HDCS_POS)
#define SOC_CLKCTL_HDCS_HDOCS			(1 << SOC_CLKCTL_HDCS_POS)

#define SOC_CLKCTL_LDOCS_POS			(3)
#define SOC_CLKCTL_LDOCS_PLL			(0 << SOC_CLKCTL_LDOCS_POS)
#define SOC_CLKCTL_LDOCS_FAST			(1 << SOC_CLKCTL_LDOCS_POS)

#define SOC_CLKCTL_HDOCS_POS			(2)
#define SOC_CLKCTL_HDOCS_PLL			(0 << SOC_CLKCTL_HDOCS_POS)
#define SOC_CLKCTL_HDOCS_FAST			(1 << SOC_CLKCTL_HDOCS_POS)

#define SOC_CLKCTL_LPMEM_PLL_CLK_SEL_POS	(1)
#define SOC_CLKCTL_LPMEM_PLL_CLK_SEL_DIV2	\
	(0 << SOC_CLKCTL_LPMEM_PLL_CLK_SEL_POS)
#define SOC_CLKCTL_LPMEM_PLL_CLK_SEL_DIV4	\
	(1 << SOC_CLKCTL_LPMEM_PLL_CLK_SEL_POS)

#define SOC_CLKCTL_HPMEM_PLL_CLK_SEL_POS	(0)
#define SOC_CLKCTL_HPMEM_PLL_CLK_SEL_DIV2	\
	(0 << SOC_CLKCTL_HPMEM_PLL_CLK_SEL_POS)
#define SOC_CLKCTL_HPMEM_PLL_CLK_SEL_DIV4	\
	(1 << SOC_CLKCTL_HPMEM_PLL_CLK_SEL_POS)

/* SOC DSP SHIM Register - Power Control */
#define SOC_PWRCTL_DISABLE_PWR_GATING_DSP0	BIT(0)
#define SOC_PWRCTL_DISABLE_PWR_GATING_DSP1	BIT(1)

/* DSP Wall Clock Timers (0 and 1) */
#define DSP_WCT_IRQ(x) \
	SOC_AGGREGATE_IRQ((22 + x), CAVS_L2_AGG_INT_LEVEL2)

#define DSP_WCT_CS_TA(x)			BIT(x)
#define DSP_WCT_CS_TT(x)			BIT(4 + x)

struct soc_dsp_shim_regs {
	uint32_t	reserved[8];
	union {
		struct {
			uint32_t walclk32_lo;
			uint32_t walclk32_hi;
		};
		uint64_t	walclk;
	};
	uint32_t	dspwctcs;
	uint32_t	reserved1[1];
	union {
		struct {
			uint32_t dspwct0c32_lo;
			uint32_t dspwct0c32_hi;
		};
		uint64_t	dspwct0c;
	};
	union {
		struct {
			uint32_t dspwct1c32_lo;
			uint32_t dspwct1c32_hi;
		};
		uint64_t	dspwct1c;
	};
	uint32_t	reserved2[14];
	uint32_t	clkctl;
	uint32_t	clksts;
	uint32_t	reserved3[4];
	uint16_t	pwrctl;
	uint16_t	pwrsts;
	uint32_t	lpsctl;
	uint32_t	lpsdmas0;
	uint32_t	lpsdmas1;
	uint32_t	reserved4[22];
};

/* macros for data cache operations */
#define SOC_DCACHE_FLUSH(addr, size)		\
	xthal_dcache_region_writeback((addr), (size))
#define SOC_DCACHE_INVALIDATE(addr, size)	\
	xthal_dcache_region_invalidate((addr), (size))

extern void z_soc_irq_enable(uint32_t irq);
extern void z_soc_irq_disable(uint32_t irq);
extern int z_soc_irq_is_enabled(unsigned int irq);

#endif /* __INC_SOC_H */
