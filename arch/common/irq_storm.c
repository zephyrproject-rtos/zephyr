/*
 * Copyright (c) 2026 Picoheart Inc.
 * Copyright (c) 2026 Zhan Gao <gaozhan.9426@picoheart.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Interrupt storm detection: rate-limits IRQ lines via the shared dispatch wrapper.
 */

#include <zephyr/init.h>
#include <zephyr/irq.h>
#ifdef CONFIG_MULTI_LEVEL_INTERRUPTS
#include <zephyr/irq_multilevel.h>
#endif
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/irq_storm.h>
#include <zephyr/sys/util.h>

#include "irq_dispatch.h"
#include "sw_isr_common.h"

LOG_MODULE_REGISTER(irq_storm);

#define STORM_THRESHOLD CONFIG_IRQ_STORM_THRESHOLD
#define STORM_WINDOW_MS CONFIG_IRQ_STORM_WINDOW_MS

/*
 * Per-slot state, kept small since IRQ_TABLE_SIZE entries are
 * allocated.  window_start_ms is a k_uptime_get_32() sample; the
 * subtraction below is unsigned wraparound-safe, so a single 32-bit
 * rollover of k_uptime_get_32() does not cause a false miss.
 * flags: bit0 = disabled, bit1 = tracked.
 */
struct storm_state {
	uint32_t count;
	uint32_t window_start_ms;
	uint8_t storm_count;
	uint8_t flags;
};

#define STORM_FLAG_DISABLED BIT(0)
#define STORM_FLAG_TRACKED  BIT(1)

static struct storm_state storm_state[IRQ_TABLE_SIZE];

static struct k_spinlock storm_lock;

/*
 * Map an ISR-table slot back to its zephyr IRQ number.
 */
unsigned int z_irq_storm_idx_to_irq(unsigned int idx)
{
#ifdef CONFIG_MULTI_LEVEL_INTERRUPTS
	return z_multilevel_idx_to_irq(idx);
#else
	if (idx >= IRQ_TABLE_SIZE) {
		return 0;
	}

	return idx + CONFIG_GEN_IRQ_START_VECTOR;
#endif
}

#ifndef CONFIG_IRQ_STORM_CUSTOM_ON_DISABLE_HOOK
/*
 * Default no-op for z_irq_storm_on_disable().  Enable
 * CONFIG_IRQ_STORM_CUSTOM_ON_DISABLE_HOOK and provide a strong
 * definition to plug in custom recovery instead.
 */
void z_irq_storm_on_disable(unsigned int irq)
{
	ARG_UNUSED(irq);
}
#endif /* !CONFIG_IRQ_STORM_CUSTOM_ON_DISABLE_HOOK */

/* Registered below as an irq_dispatch_hook, run on every wrapped interrupt. */
static void z_irq_storm_account(unsigned int idx)
{
	struct storm_state *s = &storm_state[idx];
	k_spinlock_key_t key;
	uint32_t now;

	if (!(s->flags & STORM_FLAG_TRACKED)) {
		return;
	}

	if (s->flags & STORM_FLAG_DISABLED) {
		/* An interrupt already latched/pending before irq_disable()
		 * took effect can still dispatch once after masking. Ignore
		 * it without touching the counter.
		 */
		return;
	}

	now = k_uptime_get_32();

	if ((now - s->window_start_ms) >= (uint32_t)STORM_WINDOW_MS) {
		s->window_start_ms = now;
		s->count = 0;
	}

	s->count++;

	/*
	 * count is unlocked, so two cores can both cross the threshold
	 * for the same idx before either one's STORM_FLAG_DISABLED write
	 * is visible to the other.  Most interrupt controllers have a
	 * claim-complete mechanism that keeps a given line from running
	 * concurrently on two cores, but this cannot be assumed in
	 * general.  Worst case is a duplicate
	 * irq_disable()/LOG_ERR()/z_irq_storm_on_disable() call, which is
	 * harmless since irq_disable() is idempotent.
	 */
	if (s->count > (uint32_t)STORM_THRESHOLD) {
		unsigned int irq;

		key = k_spin_lock(&storm_lock);
		s->flags |= STORM_FLAG_DISABLED;
		k_spin_unlock(&storm_lock, key);

		if (s->storm_count < UINT8_MAX) {
			s->storm_count++;
		}

		irq = z_irq_storm_idx_to_irq(idx);

		irq_disable(irq);

		LOG_ERR("irq storm: irq=0x%x count=%u in <%dms disabled.", irq, s->count,
			STORM_WINDOW_MS);

		z_irq_storm_on_disable(irq);
	}
}

IRQ_DISPATCH_HOOK_DEFINE(irq_storm_hook, z_irq_storm_account);

static int storm_init(void)
{
	uint32_t now = k_uptime_get_32();

	for (unsigned int idx = 0; idx < IRQ_TABLE_SIZE; idx++) {
		storm_state[idx].count = 0;
		storm_state[idx].window_start_ms = now;
		storm_state[idx].storm_count = 0;
		storm_state[idx].flags |= STORM_FLAG_TRACKED;
	}

#ifdef CONFIG_MULTI_LEVEL_INTERRUPTS
	/*
	 * An aggregator's parent slot runs its dispatch handler on
	 * every level-2 IRQ behind it, so it is not itself a real
	 * interrupt source.  Exclude it from counting; real storms are
	 * caught one level deeper.
	 */
	STRUCT_SECTION_FOREACH_ALTERNATE(intc_table, _irq_parent_entry, intc) {
		unsigned int parent_idx = z_get_sw_isr_table_idx(intc->irq);

		if (parent_idx < IRQ_TABLE_SIZE) {
			storm_state[parent_idx].flags &= ~STORM_FLAG_TRACKED;
		}
	}
#endif

	return 0;
}

/* PRE_KERNEL_2: must run before any driver can enable an IRQ, since
 * storm_state has to be initialized first.
 */
SYS_INIT(storm_init, PRE_KERNEL_2, 1);
