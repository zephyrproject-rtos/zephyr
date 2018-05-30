/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <xtensa_api.h>
#include <xtensa/xtruntime.h>
#include <logging/sys_log.h>
#include <board.h>
#include <irq_nextlevel.h>
#include <xtensa/hal.h>

void _soc_irq_enable(u32_t irq)
{
	struct device *dev_cavs, *dev_ictl;

	switch (XTENSA_IRQ_NUMBER(irq)) {
	case CAVS_ICTL_0_IRQ:
		dev_cavs = device_get_binding(CONFIG_CAVS_ICTL_0_NAME);
		break;
	case CAVS_ICTL_1_IRQ:
		dev_cavs = device_get_binding(CONFIG_CAVS_ICTL_1_NAME);
		break;
	case CAVS_ICTL_2_IRQ:
		dev_cavs = device_get_binding(CONFIG_CAVS_ICTL_2_NAME);
		break;
	case CAVS_ICTL_3_IRQ:
		dev_cavs = device_get_binding(CONFIG_CAVS_ICTL_3_NAME);
		break;
	default:
		/* regular interrupt */
		_xtensa_irq_enable(XTENSA_IRQ_NUMBER(irq));
		return;
	}

	if (!dev_cavs) {
		SYS_LOG_DBG("board: CAVS device binding failed\n");
		return;
	}

	/* If the control comes here it means the specified interrupt
	 * is in either CAVS interrupt logic or DW interrupt controller
	 */
	_xtensa_irq_enable(XTENSA_IRQ_NUMBER(irq));

	switch (CAVS_IRQ_NUMBER(irq)) {
	case DW_ICTL_IRQ_CAVS_OFFSET:
		dev_ictl = device_get_binding(CONFIG_DW_ICTL_NAME);
		break;
	default:
		/* The source of the interrupt is in CAVS interrupt logic */
		irq_enable_next_level(dev_cavs, CAVS_IRQ_NUMBER(irq));
		return;
	}

	if (!dev_ictl) {
		SYS_LOG_DBG("board: DW intr_control device binding failed\n");
		return;
	}

	/* If the control comes here it means the specified interrupt
	 * is in DW interrupt controller
	 */
	irq_enable_next_level(dev_cavs, CAVS_IRQ_NUMBER(irq));

	/* Manipulate the relevant bit in the interrupt controller
	 * register as needed
	 */
	irq_enable_next_level(dev_ictl, INTR_CNTL_IRQ_NUM(irq));
}

void _soc_irq_disable(u32_t irq)
{
	struct device *dev_cavs, *dev_ictl;

	switch (XTENSA_IRQ_NUMBER(irq)) {
	case CAVS_ICTL_0_IRQ:
		dev_cavs = device_get_binding(CONFIG_CAVS_ICTL_0_NAME);
		break;
	case CAVS_ICTL_1_IRQ:
		dev_cavs = device_get_binding(CONFIG_CAVS_ICTL_1_NAME);
		break;
	case CAVS_ICTL_2_IRQ:
		dev_cavs = device_get_binding(CONFIG_CAVS_ICTL_2_NAME);
		break;
	case CAVS_ICTL_3_IRQ:
		dev_cavs = device_get_binding(CONFIG_CAVS_ICTL_3_NAME);
		break;
	default:
		/* regular interrupt */
		_xtensa_irq_disable(XTENSA_IRQ_NUMBER(irq));
		return;
	}

	if (!dev_cavs) {
		SYS_LOG_DBG("board: CAVS device binding failed\n");
		return;
	}

	/* If the control comes here it means the specified interrupt
	 * is in either CAVS interrupt logic or DW interrupt controller
	 */

	switch (CAVS_IRQ_NUMBER(irq)) {
	case DW_ICTL_IRQ_CAVS_OFFSET:
		dev_ictl = device_get_binding(CONFIG_DW_ICTL_NAME);
		break;
	default:
		/* The source of the interrupt is in CAVS interrupt logic */
		irq_disable_next_level(dev_cavs, CAVS_IRQ_NUMBER(irq));

		/* Disable the parent IRQ if all children are disabled */
		if (!irq_is_enabled_next_level(dev_cavs)) {
			_xtensa_irq_disable(XTENSA_IRQ_NUMBER(irq));
		}
		return;
	}

	if (!dev_ictl) {
		SYS_LOG_DBG("board: DW intr_control device binding failed\n");
		return;
	}

	/* If the control comes here it means the specified interrupt
	 * is in DW interrupt controller.
	 * Manipulate the relevant bit in the interrupt controller
	 * register as needed
	 */
	irq_disable_next_level(dev_ictl, INTR_CNTL_IRQ_NUM(irq));

	/* Disable the parent IRQ if all children are disabled */
	if (!irq_is_enabled_next_level(dev_ictl)) {
		irq_disable_next_level(dev_cavs, CAVS_IRQ_NUMBER(irq));

		if (!irq_is_enabled_next_level(dev_cavs)) {
			_xtensa_irq_disable(XTENSA_IRQ_NUMBER(irq));
		}
	}
}

/* Setup DMA ownership registers */
void setup_ownership_dma0(void)
{
	*(volatile u16_t *)CAVS_DMA0_OWNERSHIP_REG = 0x80FF;
}

void setup_ownership_dma1(void)
{
	*(volatile u16_t *)CAVS_DMA1_OWNERSHIP_REG = 0x80FF;
}

void setup_ownership_dma2(void)
{
	*(volatile u16_t *)CAVS_DMA2_OWNERSHIP_REG = 0x80FF;
}

void dcache_writeback_region(void *addr, size_t size)
{
	xthal_dcache_region_writeback(addr, size);
}

void dcache_invalidate_region(void *addr, size_t size)
{
	xthal_dcache_region_invalidate(addr, size);
}

void setup_ownership_i2s(void)
{
	u32_t value = I2S_OWNSEL(0) | I2S_OWNSEL(1) |
			 I2S_OWNSEL(2) | I2S_OWNSEL(3);
	*(volatile u32_t *)SUE_DSPIOPO_REG |= value;

	value = DSP_RES_ALLOC_GENO_MDIVOSEL;
	*(volatile u32_t *)DSP_RES_ALLOC_GEN_OWNER |= value;
}

u32_t soc_get_ref_clk_freq(void)
{
	u32_t bootstrap;
	static u32_t freq;

	if (freq == 0) {
		/* if bootstraps have not been read before, read them */

		bootstrap = *((volatile u32_t *)SOC_S1000_GLB_CTRL_STRAPS);

		bootstrap &= SOC_S1000_STRAP_REF_CLK;

		switch (bootstrap) {
		case SOC_S1000_STRAP_REF_CLK_19P2:
			freq = 19200000;
			break;
		case SOC_S1000_STRAP_REF_CLK_24P576:
			freq = 24576000;
			break;
		case SOC_S1000_STRAP_REF_CLK_38P4:
		default:
			freq = 38400000;
			break;
		}
	}

	return freq;
}
