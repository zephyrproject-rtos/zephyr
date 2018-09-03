/*
 * Copyright (c) 2018 Intel Corporation
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

#define IOAPIC_EDGE				0
#define IOAPIC_HIGH				0

/* DW interrupt controller */
#define DW_ICTL_IRQ_CAVS_OFFSET			CAVS_IRQ_NUMBER(DW_ICTL_IRQ)
#define DW_ICTL_NUM_IRQS			9

/* GPIO */
#define GPIO_DW_0_BASE_ADDR			0x00080C00
#define GPIO_DW_0_BITS				32
#define GPIO_DW_PORT_0_INT_MASK			0
#define GPIO_DW_0_IRQ_FLAGS			0
#define GPIO_DW_0_IRQ				0x00040706
#define GPIO_DW_0_IRQ_ICTL_OFFSET		INTR_CNTL_IRQ_NUM(GPIO_DW_0_IRQ)

/* low power DMACs */
#define LP_GP_DMA_SIZE				0x00001000
#define DW_DMA0_BASE_ADDR			0x0007C000
#define DW_DMA1_BASE_ADDR			(0x0007C000 +\
						1 * LP_GP_DMA_SIZE)
#define DW_DMA2_BASE_ADDR			(0x0007C000 +\
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

#define DMA_HANDSHAKE_SSP0_TX			2
#define DMA_HANDSHAKE_SSP0_RX			3
#define DMA_HANDSHAKE_SSP1_TX			4
#define DMA_HANDSHAKE_SSP1_RX			5
#define DMA_HANDSHAKE_SSP2_TX			6
#define DMA_HANDSHAKE_SSP2_RX			7
#define DMA_HANDSHAKE_SSP3_TX			8
#define DMA_HANDSHAKE_SSP3_RX			9

/* I2S */
#define I2S0_CAVS_IRQ				0x00000010
#define I2S1_CAVS_IRQ				0x00000110
#define I2S2_CAVS_IRQ				0x00000210
#define I2S3_CAVS_IRQ				0x00000310

#define SSP_SIZE				0x0000200
#define SSP_BASE(x)				(0x00077000 + (x) * SSP_SIZE)
#define SSP_MN_DIV_SIZE				(8)
#define SSP_MN_DIV_BASE(x)		(0x00078D00 + ((x) * SSP_MN_DIV_SIZE))

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
		u16_t	lpgpdmacxo[SOC_NUM_LPGPDMAC];
		u16_t	reserved[4];
	};
	u32_t	dspiopo;
	u32_t	geno;
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
	u32_t	reserved[8];
	u64_t	walclk;
	u64_t	dspwctcs;
	u64_t	dspwct0c;
	u64_t	dspwct1c;
	u32_t	reserved1[14];
	u32_t	clkctl;
	u32_t	clksts;
	u32_t	reserved2[4];
	u16_t	pwrctl;
	u16_t	pwrsts;
	u32_t	lpsctl;
	u32_t	lpsdmas0;
	u32_t	lpsdmas1;
	u32_t	reserved3[22];
};

#define USB_DW_BASE				0x000A0000
#define USB_DW_IRQ				0x00000806

/* Global Control registers */
#define SOC_S1000_GLB_CTRL_BASE			(0x00081C00)

#define SOC_S1000_GLB_CTRL_STRAPS		(SOC_S1000_GLB_CTRL_BASE + 0x40)
#define SOC_S1000_STRAP_REF_CLK			(BIT_MASK(2) << 3)
#define SOC_S1000_STRAP_REF_CLK_38P4		(0 << 3)
#define SOC_S1000_STRAP_REF_CLK_19P2		(1 << 3)
#define SOC_S1000_STRAP_REF_CLK_24P576		(2 << 3)

extern void _soc_irq_enable(u32_t irq);
extern void _soc_irq_disable(u32_t irq);
extern void dcache_writeback_region(void *addr, size_t size);
extern void dcache_invalidate_region(void *addr, size_t size);
extern u32_t soc_get_ref_clk_freq(void);

#endif /* __INC_SOC_H */
