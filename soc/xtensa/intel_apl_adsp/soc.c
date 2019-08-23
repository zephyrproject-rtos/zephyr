/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <xtensa_api.h>
#include <xtensa/xtruntime.h>
#include <irq_nextlevel.h>
#include <xtensa/hal.h>
#include <init.h>

#include "soc.h"

#ifdef CONFIG_DYNAMIC_INTERRUPTS
#include <sw_isr_table.h>
#endif

#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(soc);

static u32_t ref_clk_freq;

void z_soc_irq_enable(u32_t irq)
{
	struct device *dev_cavs;

	switch (XTENSA_IRQ_NUMBER(irq)) {
	case DT_CAVS_ICTL_0_IRQ:
		dev_cavs = device_get_binding(CONFIG_CAVS_ICTL_0_NAME);
		break;
	case DT_CAVS_ICTL_1_IRQ:
		dev_cavs = device_get_binding(CONFIG_CAVS_ICTL_1_NAME);
		break;
	case DT_CAVS_ICTL_2_IRQ:
		dev_cavs = device_get_binding(CONFIG_CAVS_ICTL_2_NAME);
		break;
	case DT_CAVS_ICTL_3_IRQ:
		dev_cavs = device_get_binding(CONFIG_CAVS_ICTL_3_NAME);
		break;
	default:
		/* regular interrupt */
		z_xtensa_irq_enable(XTENSA_IRQ_NUMBER(irq));
		return;
	}

	if (!dev_cavs) {
		LOG_DBG("board: CAVS device binding failed");
		return;
	}

	/*
	 * The specified interrupt is in CAVS interrupt controller.
	 * So enable core interrupt first.
	 */
	z_xtensa_irq_enable(XTENSA_IRQ_NUMBER(irq));

	/* Then enable the interrupt in CAVS interrupt controller */
	irq_enable_next_level(dev_cavs, CAVS_IRQ_NUMBER(irq));
}

void z_soc_irq_disable(u32_t irq)
{
	struct device *dev_cavs;

	switch (XTENSA_IRQ_NUMBER(irq)) {
	case DT_CAVS_ICTL_0_IRQ:
		dev_cavs = device_get_binding(CONFIG_CAVS_ICTL_0_NAME);
		break;
	case DT_CAVS_ICTL_1_IRQ:
		dev_cavs = device_get_binding(CONFIG_CAVS_ICTL_1_NAME);
		break;
	case DT_CAVS_ICTL_2_IRQ:
		dev_cavs = device_get_binding(CONFIG_CAVS_ICTL_2_NAME);
		break;
	case DT_CAVS_ICTL_3_IRQ:
		dev_cavs = device_get_binding(CONFIG_CAVS_ICTL_3_NAME);
		break;
	default:
		/* regular interrupt */
		z_xtensa_irq_disable(XTENSA_IRQ_NUMBER(irq));
		return;
	}

	if (!dev_cavs) {
		LOG_DBG("board: CAVS device binding failed");
		return;
	}

	/*
	 * The specified interrupt is in CAVS interrupt controller.
	 * So disable the interrupt in CAVS interrupt controller.
	 */
	irq_disable_next_level(dev_cavs, CAVS_IRQ_NUMBER(irq));

	/* Then disable the parent IRQ if all children are disabled */
	if (!irq_is_enabled_next_level(dev_cavs)) {
		z_xtensa_irq_disable(XTENSA_IRQ_NUMBER(irq));
	}
}

int z_soc_irq_is_enabled(unsigned int irq)
{
	struct device *dev_cavs;
	int ret = 0;

	switch (XTENSA_IRQ_NUMBER(irq)) {
	case DT_CAVS_ICTL_0_IRQ:
		dev_cavs = device_get_binding(CONFIG_CAVS_ICTL_0_NAME);
		break;
	case DT_CAVS_ICTL_1_IRQ:
		dev_cavs = device_get_binding(CONFIG_CAVS_ICTL_1_NAME);
		break;
	case DT_CAVS_ICTL_2_IRQ:
		dev_cavs = device_get_binding(CONFIG_CAVS_ICTL_2_NAME);
		break;
	case DT_CAVS_ICTL_3_IRQ:
		dev_cavs = device_get_binding(CONFIG_CAVS_ICTL_3_NAME);
		break;
	default:
		/* regular interrupt */
		ret = z_xtensa_irq_is_enabled(XTENSA_IRQ_NUMBER(irq));
		goto out;
	}

	if (!dev_cavs) {
		LOG_DBG("board: CAVS device binding failed");
		ret = -ENODEV;
		goto out;
	}

	/* Then enable the interrupt in CAVS interrupt controller */
	ret = irq_line_is_enabled_next_level(dev_cavs, CAVS_IRQ_NUMBER(irq));

out:
	return ret;
}

#ifdef CONFIG_DYNAMIC_INTERRUPTS
int z_soc_irq_connect_dynamic(unsigned int irq, unsigned int priority,
			      void (*routine)(void *parameter),
			      void *parameter, u32_t flags)
{
	uint32_t table_idx;
	uint32_t cavs_irq;
	int ret;

        ARG_UNUSED(flags);
        ARG_UNUSED(priority);

	/* extract 2nd level interrupt number */
	cavs_irq = CAVS_IRQ_NUMBER(irq);
	ret = irq;

	if (cavs_irq == 0) {
		/* Not affecting 2nd level interrupts */
		z_isr_install(irq, routine, parameter);
		goto irq_connect_out;
	}

	/* Figure out the base index. */
	switch (XTENSA_IRQ_NUMBER(irq)) {
	case DT_CAVS_ICTL_0_IRQ:
		table_idx = CONFIG_CAVS_ISR_TBL_OFFSET;
		break;
	case DT_CAVS_ICTL_1_IRQ:
		table_idx = CONFIG_CAVS_ISR_TBL_OFFSET +
			    CONFIG_MAX_IRQ_PER_AGGREGATOR;
		break;
	case DT_CAVS_ICTL_2_IRQ:
		table_idx = CONFIG_CAVS_ISR_TBL_OFFSET +
			    CONFIG_MAX_IRQ_PER_AGGREGATOR * 2;
		break;
	case DT_CAVS_ICTL_3_IRQ:
		table_idx = CONFIG_CAVS_ISR_TBL_OFFSET +
			    CONFIG_MAX_IRQ_PER_AGGREGATOR * 3;
		break;
	default:
		ret = -EINVAL;
		goto irq_connect_out;
	}

	table_idx += cavs_irq;

	_sw_isr_table[table_idx].arg = parameter;
        _sw_isr_table[table_idx].isr = routine;

irq_connect_out:
	return ret;
}
#endif

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

u32_t soc_get_ref_clk_freq(void)
{
	return ref_clk_freq;
}

static inline void soc_set_audio_mclk(void)
{
#if (CONFIG_AUDIO)
	int mclk;
	volatile struct soc_mclk_control_regs *mclk_regs =
		(volatile struct soc_mclk_control_regs *)SOC_MCLK_DIV_CTRL_BASE;

	for (mclk = 0; mclk < SOC_NUM_MCLK_OUTPUTS; mclk++) {
		/*
		 * set divider to bypass mode which makes MCLK output frequency
		 * to be the same as referece clock frequency
		 */
		mclk_regs->mdivxr[mclk] = SOC_MDIVXR_SET_DIVIDER_BYPASS;
		mclk_regs->mdivctrl |= SOC_MDIVCTRL_MCLK_OUT_EN(mclk);
	}
#endif
}

static inline void soc_set_dmic_power(void)
{
#if (CONFIG_AUDIO_INTEL_DMIC)
	volatile struct soc_dmic_shim_regs *dmic_shim_regs =
		(volatile struct soc_dmic_shim_regs *)SOC_DMIC_SHIM_REG_BASE;

	/* enable power */
	dmic_shim_regs->dmiclctl |= SOC_DMIC_SHIM_DMICLCTL_SPA;

	while ((dmic_shim_regs->dmiclctl & SOC_DMIC_SHIM_DMICLCTL_CPA) == 0U) {
		/* wait for power status */
	}
#endif
}

static inline void soc_set_power_and_clock(void)
{
	volatile struct soc_dsp_shim_regs *dsp_shim_regs =
		(volatile struct soc_dsp_shim_regs *)SOC_DSP_SHIM_REG_BASE;

	dsp_shim_regs->clkctl |= SOC_CLKCTL_REQ_FAST_CLK |
		SOC_CLKCTL_OCS_FAST_CLK;
	dsp_shim_regs->pwrctl |= SOC_PWRCTL_DISABLE_PWR_GATING_DSP1 |
		SOC_PWRCTL_DISABLE_PWR_GATING_DSP0;

	soc_set_dmic_power();
	soc_set_audio_mclk();
}

static int soc_init(struct device *dev)
{
#if 0
	soc_read_bootstraps();

	LOG_INF("Reference clock frequency: %u Hz", ref_clk_freq);

	soc_set_resource_ownership();
	soc_set_power_and_clock();
#endif

	return 0;
}

SYS_INIT(soc_init, PRE_KERNEL_1, 99);
