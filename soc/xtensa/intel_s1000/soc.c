/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_LEVEL	SYS_LOG_LEVEL_INFO
#define SYS_LOG_DOMAIN	"soc/s1000"

#include <device.h>
#include <xtensa_api.h>
#include <xtensa/xtruntime.h>
#include <logging/sys_log.h>
#include <board.h>
#include <irq_nextlevel.h>
#include <xtensa/hal.h>
#include <init.h>

static u32_t ref_clk_freq;

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

static inline void soc_set_resource_ownership(void)
{
	volatile struct soc_resource_alloc_regs *regs =
		(volatile struct soc_resource_alloc_regs *)
		SOC_RESOURCE_ALLOC_REG_BASE;
	int index;


	/* set ownership of DMA controllers and channels */
	for (index = 0; index < SOC_NUM_LPGPDMAC; index++) {
		regs->lpgpdmacxo[index] = SOC_LPGPDMAC_OWNER_DSP;
	}

	/* set ownership of I2S and DMIC controllers */
	regs->dspiopo = SOC_DSPIOP_I2S_OWNSEL_DSP |
		SOC_DSPIOP_DMIC_OWNSEL_DSP;

	/* set ownership of timestamp and M/N dividers */
	regs->geno = SOC_GENO_TIMESTAMP_OWNER_DSP |
		SOC_GENO_MNDIV_OWNER_DSP;
}

void dcache_writeback_region(void *addr, size_t size)
{
	xthal_dcache_region_writeback(addr, size);
}

void dcache_invalidate_region(void *addr, size_t size)
{
	xthal_dcache_region_invalidate(addr, size);
}

u32_t soc_get_ref_clk_freq(void)
{
	return ref_clk_freq;
}

static void soc_set_power_and_clock(void)
{
	volatile struct soc_dsp_shim_regs *regs =
		(volatile struct soc_dsp_shim_regs *)
		SOC_DSP_SHIM_REG_BASE;

	regs->clkctl |= SOC_CLKCTL_REQ_FAST_CLK | SOC_CLKCTL_OCS_FAST_CLK;
	regs->pwrctl |= SOC_PWRCTL_DISABLE_PWR_GATING_DSP1 |
		SOC_PWRCTL_DISABLE_PWR_GATING_DSP0;
}

static void soc_read_bootstraps(void)
{
	u32_t bootstrap;

	bootstrap = *((volatile u32_t *)SOC_S1000_GLB_CTRL_STRAPS);

	bootstrap &= SOC_S1000_STRAP_REF_CLK;

	switch (bootstrap) {
	case SOC_S1000_STRAP_REF_CLK_19P2:
		ref_clk_freq = 19200000;
		break;
	case SOC_S1000_STRAP_REF_CLK_24P576:
		ref_clk_freq = 24576000;
		break;
	case SOC_S1000_STRAP_REF_CLK_38P4:
	default:
		ref_clk_freq = 38400000;
		break;
	}
}

static int soc_init(struct device *dev)
{
	soc_read_bootstraps();

	ref_clk_freq = soc_get_ref_clk_freq();
	SYS_LOG_INF("Reference clock frequency: %u Hz", ref_clk_freq);

	soc_set_resource_ownership();
	soc_set_power_and_clock();
	return 0;
}

SYS_INIT(soc_init, PRE_KERNEL_1, 99);
