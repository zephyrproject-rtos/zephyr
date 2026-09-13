/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM64 Cortex-A interrupt management
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/tracing/tracing.h>
#include <zephyr/irq.h>
#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/sw_isr_table.h>

void z_arm64_fatal_error(unsigned int reason, struct arch_esf *esf);

/*
 * The architecture interrupt control functions map onto the root interrupt
 * controller API (intc_root_*) provided by the controller driver, see
 * include/zephyr/arch/arm64/irq.h. With CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER
 * the SoC provides them instead.
 */

#ifdef CONFIG_DYNAMIC_INTERRUPTS
int arch_irq_connect_dynamic(unsigned int irq, unsigned int priority,
			     void (*routine)(const void *parameter),
			     const void *parameter, uint32_t flags)
{
	z_isr_install(irq, routine, parameter);
	z_arm64_irq_priority_set(irq, priority, flags);
	return irq;
}
#endif

void z_irq_spurious(const void *unused)
{
	ARG_UNUSED(unused);

	z_arm64_fatal_error(K_ERR_SPURIOUS_IRQ, NULL);
}
