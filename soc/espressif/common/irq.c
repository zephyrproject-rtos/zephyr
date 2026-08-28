/*
 * Copyright (c) 2021-2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * SoC interrupt glue for the Espressif parts.
 *
 * This is the arch-facing half of the interrupt support; the espressif,esp32-intc
 * device driver itself lives in drivers/interrupt_controller/intc_esp32.c. The
 * split is by audience:
 *
 *   here          the entry points the kernel and the arch layer call -
 *                 arch_irq_enable/disable/is_enabled (RISC-V), z_soc_irq_*,
 *                 arch_irq_disconnect_dynamic, z_riscv_irq_priority_set - and
 *                 esp_intr_noniram_disable/enable for the flash path.
 *
 *   intc_esp32.c  the interrupt-matrix registers, the level-2 and level-3
 *                 dispatchers, the level-1 hooks in esp_soc_irq.h and the
 *                 irq_next_level API that level-2 and level-3 IRQs route into.
 *
 * Every ISR is expected to be IRAM-resident, so no CPU line has to be masked
 * while the flash cache is disabled.
 */

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/irq_multilevel.h>
#include <zephyr/sw_isr_table.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <soc.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <esp_soc_irq.h>
#include <esp_attr.h>
#include <esp_cpu.h>
#include <esp_private/rtc_ctrl.h>

/*
 * Row of the per-core state for the running core: only an SMP image manages
 * more than one CPU. An AMP APPCPU image reports core id 1 while managing a
 * single CPU, so it must index row 0.
 */
static inline int z_soc_irq_mp_core(void)
{
	return (CONFIG_MP_MAX_NUM_CPUS > 1) ? esp_cpu_get_core_id() : 0;
}

/*
 * Called by the HAL around every flash operation (spi_flash/cache_utils.c).
 * With every ISR in IRAM no CPU line needs masking; only the RTC handlers are
 * switched off, and the calls must still pair up.
 */
static bool non_iram_int_disabled_flag[CONFIG_MP_MAX_NUM_CPUS];

void IRAM_ATTR esp_intr_noniram_disable(void)
{
	unsigned int key = irq_lock();
	int cpu = z_soc_irq_mp_core();

	if (non_iram_int_disabled_flag[cpu]) {
		abort();
	}
	non_iram_int_disabled_flag[cpu] = true;
	rtc_isr_noniram_disable(cpu);
	irq_unlock(key);
}

void IRAM_ATTR esp_intr_noniram_enable(void)
{
	unsigned int key = irq_lock();
	int cpu = z_soc_irq_mp_core();

	if (!non_iram_int_disabled_flag[cpu]) {
		abort();
	}
	non_iram_int_disabled_flag[cpu] = false;
	rtc_isr_noniram_enable(cpu);
	irq_unlock(key);
}

#if defined(CONFIG_RISCV)
#if defined(CONFIG_MULTI_LEVEL_INTERRUPTS)
/*
 * Xtensa gets these declarations from <zephyr/arch/xtensa/irq.h>; the RISC-V
 * arch has no such hook, so declare them here for the arch_irq_* below.
 */
void z_soc_irq_enable(unsigned int irq);
void z_soc_irq_disable(unsigned int irq);
int z_soc_irq_is_enabled(unsigned int irq);
#endif

/*
 * arch_irq_* is the RISC-V entry point for irq_enable()/irq_disable(). With the
 * multi-level backend, irq is a multilevel-encoded DT_IRQN and goes through
 * z_soc_irq_*. Without it the argument is a bare CPU line.
 */
void arch_irq_enable(unsigned int irq)
{
#if defined(CONFIG_MULTI_LEVEL_INTERRUPTS)
	z_soc_irq_enable(irq);
#else
	esp_cpu_intr_enable(1 << irq);
#endif
}

void arch_irq_disable(unsigned int irq)
{
#if defined(CONFIG_MULTI_LEVEL_INTERRUPTS)
	z_soc_irq_disable(irq);
#else
	esp_cpu_intr_disable(1 << irq);
#endif
}

int arch_irq_is_enabled(unsigned int irq)
{
#if defined(CONFIG_MULTI_LEVEL_INTERRUPTS)
	return z_soc_irq_is_enabled(irq);
#else
	return !!(esp_cpu_intr_get_enabled_mask() & (1 << irq));
#endif
}
#endif /* CONFIG_RISCV */

#if defined(CONFIG_MULTI_LEVEL_INTERRUPTS)

#include "sw_isr_common.h"

#include <zephyr/device.h>
#include <zephyr/irq_nextlevel.h>

/*
 * A level-1 IRQ is a bare CPU line and goes to the level-1 hooks of the intc
 * driver. Anything deeper is resolved to the intc device through the generic
 * intc_table lookup and handed to its irq_next_level API.
 */
void z_soc_irq_enable(unsigned int irq)
{
	if (irq_get_level(irq) == 1) {
		esp_intc_line_enable(irq);
		return;
	}

	const struct device *dev = z_get_sw_isr_device_from_irq(irq);

	if (dev != NULL) {
		irq_enable_next_level(dev, irq);
	}
}

void z_soc_irq_disable(unsigned int irq)
{
	if (irq_get_level(irq) == 1) {
		esp_intc_line_disable(irq);
		return;
	}

	const struct device *dev = z_get_sw_isr_device_from_irq(irq);

	if (dev != NULL) {
		irq_disable_next_level(dev, irq);
	}
}

int z_soc_irq_is_enabled(unsigned int irq)
{
	if (irq_get_level(irq) == 1) {
		return !!(esp_cpu_intr_get_enabled_mask() & (1 << irq));
	}

	const struct device *dev = z_get_sw_isr_device_from_irq(irq);

	return (dev != NULL) ? irq_line_is_enabled_next_level(dev, irq) : 0;
}

void z_soc_irq_init(void)
{
	/* All setup happens in esp_intc_init(). */
}

#if defined(CONFIG_DYNAMIC_INTERRUPTS) && !defined(CONFIG_SHARED_INTERRUPTS)
/*
 * Upstream defines arch_irq_disconnect_dynamic() only under
 * CONFIG_SHARED_INTERRUPTS, which this design does not use, while the radio
 * adapters call irq_disconnect_dynamic(). The slot is restored directly, as
 * z_isr_uninstall() is CONFIG_SHARED_INTERRUPTS-only too, using the same index
 * as the connect path (on RISC-V, Z_RISCV_IRQ_TABLE_IRQ()).
 */
int arch_irq_disconnect_dynamic(unsigned int irq, unsigned int priority,
				void (*routine)(const void *parameter), const void *parameter,
				uint32_t flags)
{
	unsigned int index;

	ARG_UNUSED(priority);
	ARG_UNUSED(routine);
	ARG_UNUSED(parameter);
	ARG_UNUSED(flags);

#if defined(CONFIG_RISCV)
	index = z_get_sw_isr_table_idx(Z_RISCV_IRQ_TABLE_IRQ(irq));
#else
	index = z_get_sw_isr_table_idx(irq);
#endif

	_sw_isr_table[index].isr = z_irq_spurious;
	_sw_isr_table[index].arg = NULL;

	return 0;
}
#endif /* CONFIG_DYNAMIC_INTERRUPTS && !CONFIG_SHARED_INTERRUPTS */
#endif /* CONFIG_MULTI_LEVEL_INTERRUPTS */

/*
 * Only a real function when the RISC-V arch expects one (PLIC/CLIC/AIA); for the
 * plain Espressif interrupt-matrix parts z_riscv_irq_priority_set is a no-op
 * macro from <zephyr/arch/riscv/irq.h>, and esp_intc_init() sets the line
 * priorities instead.
 */
#if defined(CONFIG_RISCV) &&                                                                       \
	(defined(CONFIG_RISCV_HAS_PLIC) || defined(CONFIG_RISCV_HAS_CLIC) ||                        \
	 defined(CONFIG_RISCV_HAS_AIA))
void z_riscv_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags)
{
	ARG_UNUSED(flags);

#if defined(CONFIG_MULTI_LEVEL_INTERRUPTS)
	/*
	 * A level-2 or level-3 IRQ carries the intmux key in its level-1 field,
	 * not the CPU line it is routed to. Lines shared through the interrupt
	 * matrix keep the priority esp_intc_init() gave them.
	 */
	if (irq_get_level(irq) != 1) {
		return;
	}
#endif

	/* 0 is not a valid RISC-V interrupt priority; keep the line's default. */
	if (prio == 0) {
		return;
	}
	esp_cpu_intr_set_priority(irq, prio);
}
#endif
