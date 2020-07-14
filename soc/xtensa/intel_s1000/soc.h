/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_SOC_H
#define __INC_SOC_H

/* macros related to interrupt handling */
#define XTENSA_IRQ_NUM_SHIFT			0
#define CAVS_IRQ_NUM_SHIFT			8
#define INTR_CNTL_IRQ_NUM_SHIFT			16
#define XTENSA_IRQ_NUM_MASK			0xff
#define CAVS_IRQ_NUM_MASK			0xff
#define INTR_CNTL_IRQ_NUM_MASK			0xff

/*
 * IRQs are mapped on 3 levels. 4th level is left 0x00.
 *
 * 1. Peripheral Register bit offset.
 * 2. CAVS logic bit offset.
 * 3. Core interrupt number.
 */
#define XTENSA_IRQ_NUMBER(_irq) \
	((_irq >> XTENSA_IRQ_NUM_SHIFT) & XTENSA_IRQ_NUM_MASK)
#define CAVS_IRQ_NUMBER(_irq) \
	(((_irq >> CAVS_IRQ_NUM_SHIFT) & CAVS_IRQ_NUM_MASK) - 1)
#define INTR_CNTL_IRQ_NUM(_irq) \
	(((_irq >> INTR_CNTL_IRQ_NUM_SHIFT) & INTR_CNTL_IRQ_NUM_MASK) - 1)

/* Macro that aggregates the tri-level interrupt into an IRQ number */
#define SOC_AGGREGATE_IRQ(ictl_irq, cavs_irq, core_irq)		\
	(((core_irq & XTENSA_IRQ_NUM_MASK) << XTENSA_IRQ_NUM_SHIFT) |	\
	(((cavs_irq) & CAVS_IRQ_NUM_MASK) << CAVS_IRQ_NUM_SHIFT) |	\
	(((ictl_irq) & INTR_CNTL_IRQ_NUM_MASK) << INTR_CNTL_IRQ_NUM_SHIFT))

#define CAVS_L2_AGG_INT_LEVEL2			DT_IRQN(DT_INST(0, intel_cavs_intc))
#define CAVS_L2_AGG_INT_LEVEL3			DT_IRQN(DT_INST(1, intel_cavs_intc))
#define CAVS_L2_AGG_INT_LEVEL4			DT_IRQN(DT_INST(2, intel_cavs_intc))
#define CAVS_L2_AGG_INT_LEVEL5			DT_IRQN(DT_INST(3, intel_cavs_intc))

#define CAVS_ICTL_INT_CPU_OFFSET(x)		(0x40 * x)

#define IOAPIC_EDGE				0
#define IOAPIC_HIGH				0

/* DW interrupt controller */
#define DW_ICTL_IRQ_CAVS_OFFSET			CAVS_IRQ_NUMBER(DT_IRQN(DT_INST(0, snps_designware_intc)))
#define DW_ICTL_NUM_IRQS			9

/* GPIO */
#define GPIO_DW_PORT_0_INT_MASK			0

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
	SOC_AGGREGATE_IRQ(0, (i2s_num) + 1, CAVS_L2_AGG_INT_LEVEL5)

#define I2S0_CAVS_IRQ				I2S_CAVS_IRQ(0)
#define I2S1_CAVS_IRQ				I2S_CAVS_IRQ(1)
#define I2S2_CAVS_IRQ				I2S_CAVS_IRQ(2)
#define I2S3_CAVS_IRQ				I2S_CAVS_IRQ(3)

#define SSP_SIZE				0x0000200
#define SSP_BASE(x)				(0x00077000 + (x) * SSP_SIZE)
#define SSP_MN_DIV_SIZE				(8)
#define SSP_MN_DIV_BASE(x)			\
	(0x00078D00 + ((x) * SSP_MN_DIV_SIZE))

/* MCLK control */
#define SOC_MCLK_DIV_CTRL_BASE			0x78C00
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

/* DSP Wall Clock Timers (0 and 1) */
#define DSP_WCT_IRQ(x)	\
	SOC_AGGREGATE_IRQ(0, (23 + x), CAVS_L2_AGG_INT_LEVEL2)

#define DSP_WCT_CS_TA(x)			BIT(x)
#define DSP_WCT_CS_TT(x)			BIT(4 + x)

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

/* L2 Local Memory Registers */
#define SOC_L2RAM_LOCAL_MEM_REG_BASE		0x00071D00
#define SOC_L2RAM_LOCAL_MEM_REG_LSPGCTL		\
	(SOC_L2RAM_LOCAL_MEM_REG_BASE + 0x50)

/* DMIC SHIM Registers */
#define SOC_DMIC_SHIM_REG_BASE			0x00071E80
#define SOC_DMIC_SHIM_DMICLCTL_SPA		BIT(0)
#define SOC_DMIC_SHIM_DMICLCTL_CPA		BIT(8)

struct soc_dmic_shim_regs {
	uint32_t	dmiclcap;
	uint32_t	dmiclctl;
};

/* SOC DSP SHIM Registers */
#define SOC_DSP_SHIM_REG_BASE			0x00071F00
/* SOC DSP SHIM Register - Clock Control */
#define SOC_CLKCTL_REQ_FAST_CLK			BIT(31)
#define SOC_CLKCTL_REQ_SLOW_CLK			BIT(30)
#define SOC_CLKCTL_OCS_FAST_CLK			BIT(2)
/* SOC DSP SHIM Register - Power Control */
#define SOC_PWRCTL_DISABLE_PWR_GATING_DSP0	BIT(0)
#define SOC_PWRCTL_DISABLE_PWR_GATING_DSP1	BIT(1)

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

/* Global Control registers */
#define SOC_S1000_GLB_CTRL_BASE			(0x00081C00)

#define SOC_GNA_POWER_CONTROL_SPA		(BIT(0))
#define SOC_GNA_POWER_CONTROL_CPA		(BIT(8))
#define SOC_GNA_POWER_CONTROL_CLK_EN		(BIT(16))

#define SOC_S1000_GLB_CTRL_DSP1_PWRCTL_CRST	BIT(1)
#define SOC_S1000_GLB_CTRL_DSP1_PWRCTL_CSTALL	BIT(9)
#define SOC_S1000_GLB_CTRL_DSP1_PWRCTL_SPA	BIT(17)
#define SOC_S1000_GLB_CTRL_DSP1_PWRCTL_CPA	BIT(25)

#define SOC_S1000_STRAP_REF_CLK			(BIT_MASK(2) << 3)
#define SOC_S1000_STRAP_REF_CLK_38P4		(0 << 3)
#define SOC_S1000_STRAP_REF_CLK_19P2		(1 << 3)
#define SOC_S1000_STRAP_REF_CLK_24P576		(2 << 3)

struct soc_global_regs {
	uint32_t	reserved1[5];
	uint32_t	cavs_dsp1power_control;
	uint32_t	reserved2[2];
	uint32_t	gna_power_control;
	uint32_t	reserved3[7];
	uint32_t	straps;
};

/* macros for data cache operations */
#define SOC_DCACHE_FLUSH(addr, size)		\
	xthal_dcache_region_writeback((addr), (size))
#define SOC_DCACHE_INVALIDATE(addr, size)	\
	xthal_dcache_region_invalidate((addr), (size))

extern void z_soc_irq_enable(uint32_t irq);
extern void z_soc_irq_disable(uint32_t irq);
extern int z_soc_irq_is_enabled(unsigned int irq);

extern uint32_t soc_get_ref_clk_freq(void);

#endif /* __INC_SOC_H */
