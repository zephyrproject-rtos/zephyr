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

#define CAVS_L2_AGG_INT_LEVEL2			DT_CAVS_ICTL_0_IRQ
#define CAVS_L2_AGG_INT_LEVEL3			DT_CAVS_ICTL_1_IRQ
#define CAVS_L2_AGG_INT_LEVEL4			DT_CAVS_ICTL_2_IRQ
#define CAVS_L2_AGG_INT_LEVEL5			DT_CAVS_ICTL_3_IRQ

#define IOAPIC_EDGE				0
#define IOAPIC_HIGH				0

/* DW interrupt controller */
#define DW_ICTL_IRQ_CAVS_OFFSET			CAVS_IRQ_NUMBER(DT_DW_ICTL_IRQ)
#define DW_ICTL_NUM_IRQS			9

/* GPIO */
#define GPIO_DW_PORT_0_INT_MASK			0

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
	SOC_AGGREGATE_IRQ(0, (i2s_num) + 1, CAVS_L2_AGG_INT_LEVEL5)

#define I2S0_CAVS_IRQ				I2S_CAVS_IRQ(0)
#define I2S1_CAVS_IRQ				I2S_CAVS_IRQ(1)
#define I2S2_CAVS_IRQ				I2S_CAVS_IRQ(2)
#define I2S3_CAVS_IRQ				I2S_CAVS_IRQ(3)

#define SSP_SIZE				0x0000200
#define SSP_BASE(x)				(0x00008000 + (x) * SSP_SIZE)
#define SSP_MN_DIV_SIZE				(8)
#define SSP_MN_DIV_BASE(x)			\
	(0x00078D00 + ((x) * SSP_MN_DIV_SIZE))

/* MCLK control */
#define SOC_MCLK_DIV_CTRL_BASE			0x00008E00
#define SOC_NUM_MCLK_OUTPUTS			2
#define SOC_MDIVCTRL_MCLK_OUT_EN(mclk)		BIT(mclk)
#define SOC_MDIVXR_SET_DIVIDER_BYPASS		BIT_MASK(12)

struct soc_mclk_control_regs {
	u32_t	mdivctrl;
	u32_t	reserved[31];
	u32_t	mdivxr[SOC_NUM_MCLK_OUTPUTS];
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
		u16_t	lpgpdmacxo[SOC_NUM_LPGPDMAC];
		u16_t	reserved[4];
	};
	u32_t	dspiopo;
	u32_t	geno;
};

/* DMIC SHIM Registers */
#define SOC_DMIC_SHIM_REG_BASE			0x00071E80
#define SOC_DMIC_SHIM_DMICLCTL_SPA		BIT(0)
#define SOC_DMIC_SHIM_DMICLCTL_CPA		BIT(8)

struct soc_dmic_shim_regs {
	u32_t	dmiclcap;
	u32_t	dmiclctl;
};

/* SOC DSP SHIM Registers */
#define SOC_DSP_SHIM_REG_BASE			0x00001000
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

#define SOC_GNA_POWER_CONTROL_SPA		(BIT(0))
#define SOC_GNA_POWER_CONTROL_CPA		(BIT(8))
#define SOC_GNA_POWER_CONTROL_CLK_EN		(BIT(16))

#define SOC_S1000_STRAP_REF_CLK			(BIT_MASK(2) << 3)
#define SOC_S1000_STRAP_REF_CLK_38P4		(0 << 3)
#define SOC_S1000_STRAP_REF_CLK_19P2		(1 << 3)
#define SOC_S1000_STRAP_REF_CLK_24P576		(2 << 3)

struct soc_global_regs {
	u32_t	reserved1[8];
	u32_t	gna_power_control;
	u32_t	reserved2[7];
	u32_t	straps;
};

/* macros for data cache operations */
#define SOC_DCACHE_FLUSH(addr, size)		\
	xthal_dcache_region_writeback((addr), (size))
#define SOC_DCACHE_INVALIDATE(addr, size)	\
	xthal_dcache_region_invalidate((addr), (size))

extern void z_soc_irq_enable(u32_t irq);
extern void z_soc_irq_disable(u32_t irq);

extern u32_t soc_get_ref_clk_freq(void);

static inline uint32_t ipc_read(uint32_t reg)
{
	return sys_read32(IPC_HOST_BASE + reg);
}

static inline void ipc_write(uint32_t reg, uint32_t val)
{
	sys_write32(val, (IPC_HOST_BASE + reg));
}

/* SRAM window 0 FW "registers" */
#define SRAM_REG_ROM_STATUS                     0x0
#define SRAM_REG_FW_STATUS                      0x4
#define SRAM_REG_FW_TRACEP                      0x8
#define SRAM_REG_FW_IPC_RECEIVED_COUNT          0xc
#define SRAM_REG_FW_IPC_PROCESSED_COUNT         0x10
#define SRAM_REG_FW_END                         0x14

static inline int memcpy_s(uint8_t *dest, size_t dest_size,
			   const uint8_t *src, size_t src_size)
{
	if (!dest || !src)
		return -EINVAL;

	if ((dest >= src && dest < (src + src_size)) ||
	    (src >= dest && src < (dest + dest_size)))
		return -EINVAL;

	if (src_size > dest_size)
		return -EINVAL;

        memcpy(dest, src, src_size);

        return 0;
}

static inline void mailbox_sw_reg_write(size_t offset, uint32_t src)
{
	*((volatile uint32_t*)(MAILBOX_SW_REG_BASE + offset)) = src;
	SOC_DCACHE_FLUSH((void *)(MAILBOX_SW_REG_BASE + offset), sizeof(src));
}

static inline int mailbox_dspbox_read(void *dest, size_t dest_size,
				      size_t offset, size_t bytes)
{
	int ret;

        SOC_DCACHE_INVALIDATE((void *)(MAILBOX_DSPBOX_BASE + offset), bytes);
        ret = memcpy_s(dest, dest_size,
		       (void *)(MAILBOX_DSPBOX_BASE + offset),
		       bytes);

	return ret;
}

static inline int mailbox_dspbox_write(size_t offset, const void *src,
				       size_t bytes)
{
	int ret;

	ret = memcpy_s((void *)(MAILBOX_DSPBOX_BASE + offset),
		       MAILBOX_DSPBOX_SIZE - offset, src, bytes);
	if (ret == 0) {
		SOC_DCACHE_FLUSH((void *)(MAILBOX_DSPBOX_BASE + offset),
				 bytes);
	}

	return ret;
}

/* Clock control */
#define SHIM_CLKCTL			0x78

/* Power control and status */
#define SHIM_PWRCTL			0x90
#define SHIM_PWRSTS			0x92
#define SHIM_LPSCTL			0x94

#define SHIM_CLKCTL_HDCS_PLL		0
#define SHIM_CLKCTL_LDCS_PLL		0
#define SHIM_CLKCTL_DPCS_DIV1(x)	(0x0 << (8 + x * 2))
#define SHIM_CLKCTL_HPMPCS_DIV2		0
#define SHIM_CLKCTL_LPMPCS_DIV4		BIT(1)
#define SHIM_CLKCTL_TCPAPLLS_DIS	0
#define SHIM_CLKCTL_TCPLCG_DIS(x)	0

static inline uint32_t shim_read(uint32_t reg)
{
	return sys_read32(SHIM_BASE + reg);
}

static inline void shim_write(uint32_t reg, uint32_t val)
{
	sys_write32(val, (SHIM_BASE + reg));
}

#endif /* __INC_SOC_H */
