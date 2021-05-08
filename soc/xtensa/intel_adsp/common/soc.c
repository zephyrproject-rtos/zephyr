/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
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

#define CAVS_INTC_NODE(n) DT_INST(n, intel_cavs_intc)

void z_soc_irq_enable(uint32_t irq)
{
	const struct device *dev_cavs;

	switch (XTENSA_IRQ_NUMBER(irq)) {
	case DT_IRQN(CAVS_INTC_NODE(0)):
		dev_cavs = device_get_binding(DT_LABEL(CAVS_INTC_NODE(0)));
		break;
	case DT_IRQN(CAVS_INTC_NODE(1)):
		dev_cavs = device_get_binding(DT_LABEL(CAVS_INTC_NODE(1)));
		break;
	case DT_IRQN(CAVS_INTC_NODE(2)):
		dev_cavs = device_get_binding(DT_LABEL(CAVS_INTC_NODE(2)));
		break;
	case DT_IRQN(CAVS_INTC_NODE(3)):
		dev_cavs = device_get_binding(DT_LABEL(CAVS_INTC_NODE(3)));
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

void z_soc_irq_disable(uint32_t irq)
{
	const struct device *dev_cavs;

	switch (XTENSA_IRQ_NUMBER(irq)) {
	case DT_IRQN(CAVS_INTC_NODE(0)):
		dev_cavs = device_get_binding(DT_LABEL(CAVS_INTC_NODE(0)));
		break;
	case DT_IRQN(CAVS_INTC_NODE(1)):
		dev_cavs = device_get_binding(DT_LABEL(CAVS_INTC_NODE(1)));
		break;
	case DT_IRQN(CAVS_INTC_NODE(2)):
		dev_cavs = device_get_binding(DT_LABEL(CAVS_INTC_NODE(2)));
		break;
	case DT_IRQN(CAVS_INTC_NODE(3)):
		dev_cavs = device_get_binding(DT_LABEL(CAVS_INTC_NODE(3)));
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
	const struct device *dev_cavs;
	int ret = 0;

	switch (XTENSA_IRQ_NUMBER(irq)) {
	case DT_IRQN(CAVS_INTC_NODE(0)):
		dev_cavs = device_get_binding(DT_LABEL(CAVS_INTC_NODE(0)));
		break;
	case DT_IRQN(CAVS_INTC_NODE(1)):
		dev_cavs = device_get_binding(DT_LABEL(CAVS_INTC_NODE(1)));
		break;
	case DT_IRQN(CAVS_INTC_NODE(2)):
		dev_cavs = device_get_binding(DT_LABEL(CAVS_INTC_NODE(2)));
		break;
	case DT_IRQN(CAVS_INTC_NODE(3)):
		dev_cavs = device_get_binding(DT_LABEL(CAVS_INTC_NODE(3)));
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

	/* Then check the interrupt in CAVS interrupt controller */
	ret = irq_line_is_enabled_next_level(dev_cavs, CAVS_IRQ_NUMBER(irq));

out:
	return ret;
}

#ifdef CONFIG_DYNAMIC_INTERRUPTS
int z_soc_irq_connect_dynamic(unsigned int irq, unsigned int priority,
			      void (*routine)(const void *parameter),
			      const void *parameter, uint32_t flags)
{
	uint32_t table_idx;
	uint32_t cavs_irq, cavs_idx;
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
	case DT_IRQN(CAVS_INTC_NODE(0)):
		cavs_idx = 0;
		break;
	case DT_IRQN(CAVS_INTC_NODE(1)):
		cavs_idx = 1;
		break;
	case DT_IRQN(CAVS_INTC_NODE(2)):
		cavs_idx = 2;
		break;
	case DT_IRQN(CAVS_INTC_NODE(3)):
		cavs_idx = 3;
		break;
	default:
		ret = -EINVAL;
		goto irq_connect_out;
	}

	table_idx = CONFIG_CAVS_ISR_TBL_OFFSET +
		CONFIG_MAX_IRQ_PER_AGGREGATOR * cavs_idx;
	table_idx += cavs_irq;

	_sw_isr_table[table_idx].arg = parameter;
	_sw_isr_table[table_idx].isr = routine;

irq_connect_out:
	return ret;
}
#endif

static inline void soc_set_power_and_clock(void)
{
	volatile struct soc_dsp_shim_regs *dsp_shim_regs =
		(volatile struct soc_dsp_shim_regs *)SOC_DSP_SHIM_REG_BASE;

	/*
	 * DSP Core 0 PLL Clock Select divide by 1
	 * DSP Core 1 PLL Clock Select divide by 1
	 * Low Power Domain Clock Select depends on LMPCS bit
	 * High Power Domain Clock Select depands on HMPCS bit
	 * Low Power Domain PLL Clock Select device by 4
	 * High Power Domain PLL Clock Select device by 2
	 * Tensilica Core Prevent Audio PLL Shutdown (TCPAPLLS)
	 * Tensilica Core Prevent Local Clock Gating (Core 0)
	 * Tensilica Core Prevent Local Clock Gating (Core 1)
	 */
	dsp_shim_regs->clkctl =
		SOC_CLKCTL_DPCS_DIV1(0) |
		SOC_CLKCTL_DPCS_DIV1(1) |
		SOC_CLKCTL_LDCS_LMPCS |
		SOC_CLKCTL_HDCS_HMPCS |
		SOC_CLKCTL_LPMEM_PLL_CLK_SEL_DIV4 |
		SOC_CLKCTL_HPMEM_PLL_CLK_SEL_DIV2 |
		SOC_CLKCTL_TCPAPLLS |
		SOC_CLKCTL_TCPLCG_DIS(0) |
		SOC_CLKCTL_TCPLCG_DIS(1);

	/* Disable power gating for both cores */
	dsp_shim_regs->pwrctl |= SOC_PWRCTL_DISABLE_PWR_GATING_DSP1 |
		SOC_PWRCTL_DISABLE_PWR_GATING_DSP0;

	/* Rewrite the low power sequencing control bits */
	dsp_shim_regs->lpsctl = dsp_shim_regs->lpsctl;
}

static int soc_init(const struct device *dev)
{
	soc_set_power_and_clock();
	return 0;
}

SYS_INIT(soc_init, PRE_KERNEL_1, 99);
