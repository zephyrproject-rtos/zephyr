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

#include <cavs-shim.h>
#include <cavs-idc.h>
#include "soc.h"

#ifdef CONFIG_DYNAMIC_INTERRUPTS
#include <sw_isr_table.h>
#endif

#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(soc);

#ifndef CONFIG_SOC_SERIES_INTEL_CAVS_V15
# define SHIM_GPDMA_BASE_OFFSET   0x6500
# define SHIM_GPDMA_BASE(x)       (SHIM_GPDMA_BASE_OFFSET + (x) * 0x100)
# define SHIM_GPDMA_CLKCTL(x)     (SHIM_GPDMA_BASE(x) + 0x4)
# define SHIM_CLKCTL_LPGPDMAFDCGB BIT(0)

# define DSP_INIT_LPGPDMA(x)	  (0x71A60 + (2*x))
# define LPGPDMA_CTLOSEL_FLAG	  BIT(15)
# define LPGPDMA_CHOSEL_FLAG	  0xFF

# define DSP_INIT_GENO	          0x71A6C
# define GENO_MDIVOSEL		  BIT(1)
# define GENO_DIOPTOSEL           BIT(2)
#endif

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

static __imr void power_init_v15(void)
{
	/* HP domain clocked by PLL
	 * LP domain clocked by PLL
	 * DSP Core 0 PLL Clock Select divide by 1
	 * DSP Core 1 PLL Clock Select divide by 1
	 * High Power Domain PLL Clock Select device by 2
	 * Low Power Domain PLL Clock Select device by 4
	 * Disable Tensilica Core Prevent Audio PLL Shutdown (TCPAPLLS)
	 * Disable Tensilica Core Prevent Local Clock Gating (Core 0)
	 * Disable Tensilica Core Prevent Local Clock Gating (Core 1)
	 *   - Disabling "prevent clock gating" means allowing clock gating
	 */
	CAVS_SHIM.clkctl = CAVS15_CLKCTL_LMPCS;

	/* Rewrite the low power sequencing control bits */
	CAVS_SHIM.lpsctl = CAVS_SHIM.lpsctl;
}

static __imr void power_init(void)
{
	/* Request HP ring oscillator and
	 * wait for status to indicate it's ready.
	 */
	CAVS_SHIM.clkctl |= CAVS_CLKCTL_RHROSCC;
	while ((CAVS_SHIM.clkctl & CAVS_CLKCTL_RHROSCC) != CAVS_CLKCTL_RHROSCC) {
		k_busy_wait(10);
	}

	/* Request HP Ring Oscillator
	 * Select HP Ring Oscillator
	 * High Power Domain PLL Clock Select device by 2
	 * Low Power Domain PLL Clock Select device by 4
	 * Disable Tensilica Core(s) Prevent Local Clock Gating
	 *   - Disabling "prevent clock gating" means allowing clock gating
	 */
	CAVS_SHIM.clkctl = (CAVS_CLKCTL_RHROSCC |
			    CAVS_CLKCTL_OCS |
			    CAVS_CLKCTL_LMCS);

#ifndef CONFIG_SOC_SERIES_INTEL_CAVS_V15
	/* Prevent LP GPDMA 0 & 1 clock gating */
	sys_write32(SHIM_CLKCTL_LPGPDMAFDCGB, SHIM_GPDMA_CLKCTL(0));
	sys_write32(SHIM_CLKCTL_LPGPDMAFDCGB, SHIM_GPDMA_CLKCTL(1));
#endif

	/* Disable power gating for first cores */
	CAVS_SHIM.pwrctl |= CAVS_PWRCTL_TCPDSPPG(0);

#ifndef CONFIG_SOC_SERIES_INTEL_CAVS_V15
	/* On cAVS 1.8+, we must demand ownership of the timestamping
	 * and clock generator registers.  Lacking the former will
	 * prevent wall clock timer interrupts from arriving, even
	 * though the device itself is operational.
	 */
	sys_write32(GENO_MDIVOSEL | GENO_DIOPTOSEL, DSP_INIT_GENO);
	sys_write32(LPGPDMA_CHOSEL_FLAG | LPGPDMA_CTLOSEL_FLAG,
		    DSP_INIT_LPGPDMA(0));
	sys_write32(LPGPDMA_CHOSEL_FLAG | LPGPDMA_CTLOSEL_FLAG,
		    DSP_INIT_LPGPDMA(1));
#endif
}

static __imr int soc_init(const struct device *dev)
{
	if (IS_ENABLED(CONFIG_SOC_SERIES_INTEL_CAVS_V15)) {
		power_init_v15();
	} else {
		power_init();
	}

	soc_idc_init();
	return 0;
}

SYS_INIT(soc_init, PRE_KERNEL_1, 99);
