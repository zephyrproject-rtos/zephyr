/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <soc.h>
#include <esp_cpu.h>
#include <soc/periph_defs.h>

LOG_MODULE_REGISTER(soc_irq, CONFIG_SOC_LOG_LEVEL);

extern void esp_rom_route_intr_matrix(int cpu_core, uint32_t periph_intr_id, uint32_t cpu_intr_num);

void z_soc_irq_enable(unsigned int irq, unsigned int source)
{
	if (irq >= CONFIG_NUM_IRQS) {
		LOG_ERR("Invalid IRQ %u", irq);
		return;
	}

	/* If source is provided, it's for interrupt matrix routing */
	if (source == 0 || source >= ETS_MAX_INTR_SOURCE) {
		LOG_ERR("Invalid interrupt source %u", source);
		return;
	}

	/* Route the peripheral interrupt source to CPU interrupt line */
	esp_rom_route_intr_matrix(esp_cpu_get_core_id(), source, irq);

	/* Enable the CPU interrupt line */
	esp_cpu_intr_enable(1 << irq);
}

void z_soc_irq_disable(unsigned int irq, unsigned int source)
{
	if (irq >= CONFIG_NUM_IRQS) {
		LOG_ERR("Invalid IRQ %u", irq);
		return;
	}

	if (source != 0 && source < ETS_MAX_INTR_SOURCE) {
		/* Route to an invalid/disabled interrupt number */
		esp_rom_route_intr_matrix(esp_cpu_get_core_id(), source, ETS_INVALID_INUM);
	}

#if !defined(CONFIG_SHARED_INTERRUPTS)
	esp_cpu_intr_disable(1 << irq);
#endif
}

#if defined(CONFIG_RISCV)
void z_riscv_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags)
{
	esp_cpu_intr_set_priority(irq, prio);
}
#endif
