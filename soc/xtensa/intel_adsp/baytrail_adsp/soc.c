/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <xtensa/hal.h>
#include <init.h>

#include "soc.h"

#ifdef CONFIG_DYNAMIC_INTERRUPTS
#include <sw_isr_table.h>
#endif

#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(soc);

void z_soc_irq_enable(uint32_t irq)
{
	z_xtensa_irq_enable(XTENSA_IRQ_NUMBER(irq));
}

void z_soc_irq_disable(uint32_t irq)
{
	z_xtensa_irq_disable(XTENSA_IRQ_NUMBER(irq));
}

int z_soc_irq_is_enabled(unsigned int irq)
{
	return z_xtensa_irq_is_enabled(XTENSA_IRQ_NUMBER(irq));
}

#ifdef CONFIG_DYNAMIC_INTERRUPTS
int z_soc_irq_connect_dynamic(unsigned int irq, unsigned int priority,
			      void (*routine)(void *parameter),
			      void *parameter, uint32_t flags)
{
	ARG_UNUSED(flags);
	ARG_UNUSED(priority);

	z_isr_install(irq, routine, parameter);

	return irq;
}
#endif

#ifndef CONFIG_SOF
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
#endif

static int soc_init(struct device *dev)
{
#ifndef CONFIG_SOF
	soc_set_power_and_clock();
#endif
	return 0;
}

SYS_INIT(soc_init, PRE_KERNEL_1, 99);
