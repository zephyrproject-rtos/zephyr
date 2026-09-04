/*
 * Copyright (c) 2021-2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * SoC interrupt glue for the Espressif parts.
 *
 * This is the arch-facing half of the interrupt support; the INTMUX device
 * driver itself lives in drivers/interrupt_controller/intc_esp32.c. The split is
 * by audience rather than by SoC:
 *
 *   here          the entry points the kernel and the arch layer call -
 *                 arch_irq_enable/disable/is_enabled, arch_irq_connect_dynamic,
 *                 z_soc_irq_*, z_riscv_irq_priority_set - plus the two pieces of
 *                 bookkeeping they own: the per-CPU-line IRAM flags and the
 *                 legacy esp_intr_* vector descriptors.
 *
 *   intc_esp32.c  the espressif,esp32-intc device: interrupt-matrix registers,
 *                 the multi-level dispatchers, and the irq_next_level_api that
 *                 the calls below route into.
 *
 * The two files share no state. Everything each side owns is static to its own
 * file, and they meet only at the public interfaces: esp_soc_irq.h, the
 * irq_nextlevel API, and the generic _sw_isr_table lookups.
 */

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/irq_multilevel.h>
#include <zephyr/sw_isr_table.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <soc.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <esp_soc_irq.h>
#include <esp_memory_utils.h>
#include <esp_attr.h>
#include <esp_cpu.h>
#include <esp_rom_sys.h>
#include <esp_private/rtc_ctrl.h>
#include <soc/soc.h>
#include <soc/soc_caps.h>
#include <rom/ets_sys.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(intc_esp32, CONFIG_LOG_DEFAULT_LEVEL);

/*
 * Per-CPU-line client counters live in esp_intr_clients[] (esp_soc_irq.h),
 * alongside the level-2 dispatcher's enabled-source mask, so a line's whole
 * runtime state is one record. Only the two derived per-core summaries below
 * stay separate, because the flash path wants them as whole words.
 */

/* Lines to mask while the flash cache is disabled: set per line by
 * z_soc_irq_mask_update() from that line's non_iram_clients count.
 */
static uint32_t non_iram_int_mask[CONFIG_MP_MAX_NUM_CPUS];
/* This bitmask has 1 in it if the int was disabled using esp_intr_noniram_disable. */
static uint32_t non_iram_int_disabled[CONFIG_MP_MAX_NUM_CPUS];
static bool non_iram_int_disabled_flag[CONFIG_MP_MAX_NUM_CPUS];

/*
 * Row of esp_intr_clients[] for the running core: the array has one row per
 * Zephyr-managed CPU. Only an SMP image has more than one, so anything else
 * indexes row 0 whichever hardware core it runs on - which is what keeps an AMP
 * APPCPU image, where esp_cpu_get_core_id() returns 1, from indexing past the
 * end of a single-row array. Mirrors esp_intr_mp_core() in intc_esp32.c, and is
 * defensive against how the AMP builds report their core id rather than
 * something this code wants - see the comment there.
 */
static inline int z_soc_irq_mp_core(void)
{
	return (CONFIG_MP_MAX_NUM_CPUS > 1) ? esp_cpu_get_core_id() : 0;
}

/*
 * Reduce a (possibly multilevel-encoded) IRQ to its level-1 CPU interrupt line.
 * The per-line IRAM bookkeeping below is indexed by CPU line, and
 * non_iram_int_mask is a 32-bit mask of CPU lines, so an encoded IRQ (e.g. a
 * level-2 leaf connected via IRQ_CONNECT(DT_IRQN(...))) must be decoded first.
 */
static inline unsigned int z_soc_irq_cpu_line(unsigned int irq)
{
#if defined(CONFIG_MULTI_LEVEL_INTERRUPTS)
	if (irq_get_level(irq) != 1) {
		return irq & BIT_MASK(CONFIG_1ST_LEVEL_INTERRUPT_BITS);
	}
#endif
	return irq;
}

static void z_soc_irq_mask_update(int cpu, unsigned int irq)
{
	struct esp_intr_line *line = &esp_intr_clients[cpu][irq];

	line->iram_capable = (line->non_iram_clients == 0U);

	if (line->iram_capable) {
		non_iram_int_mask[cpu] &= ~BIT(irq);
	} else {
		non_iram_int_mask[cpu] |= BIT(irq);
	}
}

int z_soc_irq_flags_apply(unsigned int irq, uint32_t flags)
{
	int cpu = z_soc_irq_mp_core();
	bool is_iram = (flags & ESP_INTR_FLAG_IRAM) != 0;
	struct esp_intr_line *line;

	irq = z_soc_irq_cpu_line(irq);

	if (irq >= SOC_CPU_INTR_NUM) {
		return -EINVAL;
	}

	line = &esp_intr_clients[cpu][irq];

	if (line->total_clients == UINT8_MAX) {
		return -ENOMEM;
	}

	if (line->total_clients > 0) {
		if (is_iram && line->non_iram_clients > 0) {
			return -EINVAL;
		}
		if (!is_iram && line->non_iram_clients == 0) {
			return -EINVAL;
		}
	}

	line->total_clients++;
	if (!is_iram) {
		line->non_iram_clients++;
	}

	z_soc_irq_mask_update(cpu, irq);
	return 0;
}

int z_soc_irq_flags_clear(unsigned int irq, uint32_t flags)
{
	int cpu = z_soc_irq_mp_core();
	bool is_iram = (flags & ESP_INTR_FLAG_IRAM) != 0;
	struct esp_intr_line *line;

	irq = z_soc_irq_cpu_line(irq);

	if (irq >= SOC_CPU_INTR_NUM) {
		return -EINVAL;
	}

	line = &esp_intr_clients[cpu][irq];

	/*
	 * Validate everything before mutating anything, as z_soc_irq_flags_apply()
	 * does: a caller that gets an error back must be able to assume no state
	 * changed. The previous order decremented total_clients first and could
	 * then bail on the non-IRAM check, leaving the count one short and skipping
	 * the mask update.
	 */
	if (line->total_clients == 0U) {
		return -EINVAL;
	}
	if (!is_iram && line->non_iram_clients == 0U) {
		return -EINVAL;
	}

	line->total_clients--;
	if (!is_iram) {
		line->non_iram_clients--;
	}

	z_soc_irq_mask_update(cpu, irq);
	return 0;
}

int z_soc_irq_validate(void (*isr)(const void *parameter), uint32_t flags)
{
	if ((flags & ESP_INTR_FLAG_IRAM) && isr != NULL && !esp_ptr_in_iram(isr) &&
		!esp_ptr_in_rtc_iram_fast(isr) && !esp_ptr_in_rom(isr)) {
		return -EINVAL;
	}

	return 0;
}

#if defined(CONFIG_ZTEST)
uint8_t z_soc_irq_line_total_clients_get(unsigned int irq)
{
	int cpu = z_soc_irq_mp_core();

	if (irq >= SOC_CPU_INTR_NUM) {
		return 0;
	}

	return esp_intr_clients[cpu][irq].total_clients;
}

uint8_t z_soc_irq_line_non_iram_clients_get(unsigned int irq)
{
	int cpu = z_soc_irq_mp_core();

	if (irq >= SOC_CPU_INTR_NUM) {
		return 0;
	}

	return esp_intr_clients[cpu][irq].non_iram_clients;
}

uint32_t z_soc_irq_non_iram_int_mask_get(unsigned int irq)
{
	int cpu = z_soc_irq_mp_core();

	if (irq >= SOC_CPU_INTR_NUM) {
		return 0;
	}

	return (non_iram_int_mask[cpu] >> irq) & 1U;
}
#endif


/*
 * Mask/unmask the interrupts that cannot run with the flash cache disabled.
 * These are NOT legacy: hal_espressif's spi_flash/cache_utils.c calls them
 * around every flash operation, using the per-line non_iram_clients bookkeeping
 * above via non_iram_int_mask.
 */
void IRAM_ATTR esp_intr_noniram_disable(void)
{
	unsigned int key = irq_lock();
	int oldint;
	int cpu = z_soc_irq_mp_core();
	int non_iram_ints = non_iram_int_mask[cpu];

	if (non_iram_int_disabled_flag[cpu]) {
		abort();
	}
	non_iram_int_disabled_flag[cpu] = true;
	oldint = esp_cpu_intr_get_enabled_mask();
	esp_cpu_intr_disable(non_iram_ints);
	rtc_isr_noniram_disable(cpu);
	/* Save which ints we did disable */
	non_iram_int_disabled[cpu] = oldint & non_iram_ints;
	irq_unlock(key);
}

void IRAM_ATTR esp_intr_noniram_enable(void)
{
	unsigned int key = irq_lock();
	int cpu = z_soc_irq_mp_core();
	int non_iram_ints = non_iram_int_disabled[cpu];

	if (!non_iram_int_disabled_flag[cpu]) {
		abort();
	}
	non_iram_int_disabled_flag[cpu] = false;
	esp_cpu_intr_enable(non_iram_ints);
	rtc_isr_noniram_enable(cpu);
	irq_unlock(key);
}

#if defined(CONFIG_RISCV)
#if defined(CONFIG_MULTI_LEVEL_INTERRUPTS)
/*
 * The multi-level backend is defined further down. Xtensa gets these
 * declarations from <zephyr/arch/xtensa/irq.h>; the RISC-V arch has no such
 * hook, so declare them here for the delegating arch_irq_* below.
 */
void z_soc_irq_enable(unsigned int irq);
void z_soc_irq_disable(unsigned int irq);
int z_soc_irq_is_enabled(unsigned int irq);
#endif

/*
 * arch_irq_* is the RISC-V entry point for irq_enable()/irq_disable(). With the
 * multi-level backend, irq is a multilevel-encoded DT_IRQN, so routing it
 * through the INTMUX and installing the shared dispatcher must go through
 * z_soc_irq_*. Without it (flat matrix, pre-migration SoCs) the argument is a
 * bare CPU line and the line is toggled directly.
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

/* All z_soc_irq_* takes the multilevel-encoded IRQ.
 * A level-1 IRQ is a bare CPU line and is toggled directly;
 * Anything deeper is resolved to the intc device through the
 * generic intc_table lookup and handed to its irq_next_level_api
 * in intc_esp32.c, which owns the matrix routing and bookkeeping.
 */
void z_soc_irq_enable(unsigned int irq)
{
	if (irq_get_level(irq) == 1) {
		esp_cpu_intr_enable(1 << irq);
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
		esp_cpu_intr_disable(1 << irq);
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
	/* All setup happens in esp_intc_init() and lazily on first enable. */
}

/*
 * Xtensa only: on RISC-V arch_irq_connect_dynamic() is a strong definition in
 * arch/riscv/core/irq_manage.c, while the Xtensa arch relies on the __weak
 * fallback in arch/common/dynamic_isr.c, which this overrides to run the
 * ESP flag bookkeeping.
 */
#if defined(CONFIG_XTENSA) && defined(CONFIG_DYNAMIC_INTERRUPTS)
int arch_irq_connect_dynamic(unsigned int irq, unsigned int priority,
			     void (*routine)(const void *parameter), const void *parameter,
			     uint32_t flags)
{
	int rc;

	ARG_UNUSED(priority);

	/*
	 * A 3rd-level window is packed densely - one slot per status bit that has
	 * a handler - and only gen_isr_tables.py knows which bits those are. The
	 * generic z_get_sw_isr_table_idx() below computes offset + raw bit, which
	 * would land on the wrong slot, so level-3 leaves must be connected
	 * statically through their devicetree aggregator.
	 *
	 * Levels 1 and 2 are both fine: the intc_table entries make the generic
	 * lookup agree with the flat source-indexed window (see below).
	 */
	if (irq_get_level(irq) == 3) {
		return -ENOTSUP;
	}

	rc = z_soc_irq_validate(routine, flags);
	if (rc < 0) {
		return rc;
	}

	/*
	 * Apply the interrupt flags before installing the ISR: z_soc_irq_flags_apply()
	 * returns every error before it mutates any state, so a failure needs no
	 * rollback - nothing has been installed yet.
	 */
	rc = z_soc_irq_flags_apply(irq, flags);
	if (rc < 0) {
		return rc;
	}

	/*
	 * The intc_table entries that intc_esp32.c defines make the generic lookup
	 * agree with the single source-indexed window: a level-2 IRQ resolves to
	 * CONFIG_2ND_LVL_ISR_TBL_OFFSET + source, a level-1 IRQ to its own slot,
	 * mirroring gen_isr_tables.py so static and dynamic connects agree.
	 */
	unsigned int index = z_get_sw_isr_table_idx(irq);

	_sw_isr_table[index].isr = routine;
	_sw_isr_table[index].arg = parameter;

	return irq;
}
#endif /* CONFIG_XTENSA && CONFIG_DYNAMIC_INTERRUPTS */

#if defined(CONFIG_DYNAMIC_INTERRUPTS) && !defined(CONFIG_SHARED_INTERRUPTS)
/*
 * The counterpart of the connect path above. Upstream only defines
 * arch_irq_disconnect_dynamic() under CONFIG_SHARED_INTERRUPTS - both the RISC-V
 * definition in arch/riscv/core/irq_manage.c and the __weak one in
 * arch/common/shared_irq.c live behind that symbol - and this design deliberately
 * does not use shared interrupts. Without a definition the symbol is simply
 * missing at link time for anything that calls irq_disconnect_dynamic().
 *
 * It also gives z_soc_irq_flags_clear() its only caller here, so the per-line
 * client counters can actually come back down; connecting without a matching
 * disconnect would leak a client on the line forever and pin its IRAM capability.
 *
 * The slot is restored directly rather than via z_isr_uninstall(), which is
 * itself CONFIG_SHARED_INTERRUPTS-only, and mirrors how the connect path writes
 * it. Guarded against SHARED_INTERRUPTS so it never collides with the upstream
 * definitions if that option is ever turned on.
 */
int arch_irq_disconnect_dynamic(unsigned int irq, unsigned int priority,
				void (*routine)(const void *parameter), const void *parameter,
				uint32_t flags)
{
	unsigned int index;
	int rc;

	ARG_UNUSED(priority);
	ARG_UNUSED(routine);
	ARG_UNUSED(parameter);

	/* Level-3 leaves are never connected dynamically, so never disconnected. */
	if (irq_get_level(irq) == 3) {
		return -ENOTSUP;
	}

	rc = z_soc_irq_flags_clear(irq, flags);
	if (rc < 0) {
		return rc;
	}

	/*
	 * Mirror whichever connect path installed the entry, or this clears the
	 * wrong slot. The Xtensa path is arch_irq_connect_dynamic() above, which
	 * uses the index directly; the RISC-V one is arch/riscv/core/irq_manage.c,
	 * which offsets by the reserved-IRQ count before z_isr_install() applies the
	 * same lookup. That offset is 0 on esp32c3/c6 but not on esp32c5/p4.
	 */
#if defined(CONFIG_RISCV)
	index = z_get_sw_isr_table_idx(irq + CONFIG_RISCV_RESERVED_IRQ_ISR_TABLES_OFFSET);
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
 * plain espressif interrupt-matrix parts z_riscv_irq_priority_set is a no-op
 * macro from <zephyr/arch/riscv/irq.h>, and per-line priorities are applied from
 * the intmux nodes in esp_intc_init() instead.
 */
#if defined(CONFIG_RISCV) &&                                                                       \
	(defined(CONFIG_RISCV_HAS_PLIC) || defined(CONFIG_RISCV_HAS_CLIC) ||                        \
	 defined(CONFIG_RISCV_HAS_AIA))
void z_riscv_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags)
{
	ARG_UNUSED(flags);

#if defined(CONFIG_MULTI_LEVEL_INTERRUPTS)
	/*
	 * ARCH_IRQ_CONNECT passes the raw multilevel-encoded DT_IRQN; a level-2+
	 * leaf must be reduced to its CPU line, otherwise the encoded value trips
	 * esp_cpu_intr_set_priority()'s 0..31 range check.
	 */
	if (irq_get_level(irq) != 1) {
		irq &= BIT_MASK(CONFIG_1ST_LEVEL_INTERRUPT_BITS);
	}
#endif

	/* 0 is not a valid RISC-V interrupt priority; keep the line's default. */
	if (prio == 0) {
		return;
	}
	esp_cpu_intr_set_priority(irq, prio);
}
#endif
