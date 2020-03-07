/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM64 Cortex-A interrupt management
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <device.h>
#include <tracing/tracing.h>
#include <irq.h>
#include <irq_nextlevel.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <sw_isr_table.h>

void z_arm64_fatal_error(unsigned int reason, const z_arch_esf_t *esf);

void arch_irq_enable(unsigned int irq)
{
	struct device *dev = _sw_isr_table[0].arg;

	irq_enable_next_level(dev, (irq >> 8) - 1);
}

void arch_irq_disable(unsigned int irq)
{
	struct device *dev = _sw_isr_table[0].arg;

	irq_disable_next_level(dev, (irq >> 8) - 1);
}

int arch_irq_is_enabled(unsigned int irq)
{
	struct device *dev = _sw_isr_table[0].arg;

	return irq_is_enabled_next_level(dev);
}

void z_arm64_irq_priority_set(unsigned int irq, unsigned int prio, u32_t flags)
{
	struct device *dev = _sw_isr_table[0].arg;

	if (irq == 0)
		return;

	irq_set_priority_next_level(dev, (irq >> 8) - 1, prio, flags);
}

void z_irq_spurious(void *unused)
{
	ARG_UNUSED(unused);

	z_arm64_fatal_error(K_ERR_SPURIOUS_IRQ, NULL);
}
