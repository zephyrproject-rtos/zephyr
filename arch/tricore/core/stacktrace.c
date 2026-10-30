/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/arch_interface.h>
#include <zephyr/arch/tricore/cr.h>
#include <zephyr/sys/util.h>

#include <kernel_arch_data.h>

/* PCXI.UL tells a lower context from an upper one. */
#define PCXI_UL BIT(20)

/* Bound snapshot storage and the number of frames collected under the lock. */
#define STACK_SNAPSHOT_MAX_FRAMES MIN(CONFIG_ARCH_STACKWALK_MAX_FRAMES, 32)
#define STACK_SNAPSHOT_CAPACITY   MAX(1, STACK_SNAPSHOT_MAX_FRAMES)

extern union z_tricore_context z_tricore_csa[];

static inline bool valid_csa(uintptr_t addr)
{
	uintptr_t base = (uintptr_t)z_tricore_csa;

	return (addr & 0x3FU) == 0U &&
	       addr - base < (uintptr_t)CONFIG_TRICORE_CSA_COUNT * sizeof(union z_tricore_context);
}

static void walk_contexts(stack_trace_callback_fn callback_fn, void *cookie, uint32_t pcxi,
			  unsigned int max_frames)
{
	for (unsigned int frames = 0U; pcxi != 0U && frames < max_frames; frames++) {
		uintptr_t addr = z_tricore_pcx_to_addr(pcxi);
		const union z_tricore_context *csa;
		uint32_t a11;

		if (!valid_csa(addr)) {
			break;
		}

		csa = UINT_TO_POINTER(addr);
		if ((pcxi & PCXI_UL) != 0U) {
			a11 = csa->upper.a11;
			if (a11 == 0U) {
				break;
			}
		} else {
			a11 = csa->lower.a11;
		}

		if (a11 != 0U) {
			if (!callback_fn(cookie, a11)) {
				return;
			}
		}

		pcxi = csa->lower.pcxi;
	}
}

static void walk_exception(stack_trace_callback_fn callback_fn, void *cookie,
			   const struct arch_esf *esf, unsigned int max_frames)
{
	/* The lower context saved by svlcx on interrupt entry. */
	if (!valid_csa((uintptr_t)esf)) {
		return;
	}

	if (esf->a11 != 0U) {
		if (!callback_fn(cookie, esf->a11)) {
			return;
		}

		max_frames--;
	}

	walk_contexts(callback_fn, cookie, esf->pcxi, max_frames);
}

struct stack_snapshot {
	unsigned long addrs[STACK_SNAPSHOT_CAPACITY];
	size_t count;
};

static bool saved_chain_contains_esf(uint32_t pcxi, const struct arch_esf *esf)
{
	for (unsigned int i = 0U; pcxi != 0U && i < CONFIG_TRICORE_CSA_COUNT; i++) {
		uintptr_t addr = z_tricore_pcx_to_addr(pcxi);
		const union z_tricore_context *csa;

		if (!valid_csa(addr)) {
			break;
		}

		if (addr == (uintptr_t)esf) {
			return (pcxi & PCXI_UL) == 0U;
		}

		csa = UINT_TO_POINTER(addr);
		pcxi = csa->lower.pcxi;
	}

	return false;
}

static bool snapshot_frame(void *cookie, unsigned long addr)
{
	struct stack_snapshot *snapshot = cookie;

	snapshot->addrs[snapshot->count++] = addr;
	return snapshot->count < ARRAY_SIZE(snapshot->addrs);
}

/* Keep the snapshot buffer off current-thread walks. */
static void __noinline walk_saved_thread(stack_trace_callback_fn callback_fn, void *cookie,
					 const struct k_thread *thread, const struct arch_esf *esf)
{
	struct stack_snapshot snapshot = {.count = 0U};
	unsigned int key;
	uint32_t pcxi;

	/* Prevent scheduling or an ISR abort from changing the saved chain. */
	key = irq_lock();
	__asm volatile("dsync" ::: "memory");
	pcxi = thread->callee_saved.pcxi;
	if (esf == NULL) {
		walk_contexts(snapshot_frame, &snapshot, pcxi, STACK_SNAPSHOT_MAX_FRAMES);
	} else if (saved_chain_contains_esf(pcxi, esf)) {
		walk_exception(snapshot_frame, &snapshot, esf, STACK_SNAPSHOT_MAX_FRAMES);
	}
	irq_unlock(key);

	for (size_t i = 0U; i < snapshot.count; i++) {
		if (!callback_fn(cookie, snapshot.addrs[i])) {
			break;
		}
	}
}

void arch_stack_walk(stack_trace_callback_fn callback_fn, void *cookie,
		     const struct k_thread *thread, const struct arch_esf *esf)
{
	unsigned int max_frames = CONFIG_ARCH_STACKWALK_MAX_FRAMES;
	uint32_t pcxi;

	if (callback_fn == NULL) {
		return;
	}

	if (CONFIG_ARCH_STACKWALK_MAX_FRAMES < 1) {
		return;
	}

	if (thread == NULL) {
		thread = _current;
	}

	if (thread != _current) {
		walk_saved_thread(callback_fn, cookie, thread, esf);
		return;
	}

	/* CSA reads may be buffered by the hardware; make them visible. */
	__asm volatile("dsync" ::: "memory");

	if (esf != NULL) {
		walk_exception(callback_fn, cookie, esf, max_frames);
		return;
	}

	if (k_is_in_isr()) {
		const struct k_thread *interrupted = _current_cpu->arch.to_reclaim;

		/* An ISR abort can replace _current before the outermost IRQ exits. */
		if (interrupted == NULL) {
			interrupted = thread;
		}

		pcxi = interrupted->callee_saved.pcxi;
	} else {
		unsigned long return_addr = (unsigned long)__builtin_return_address(0);

		/*
		 * The caller's return address is the current sample; the saved
		 * contexts above it hold only older return addresses.
		 */
		if (return_addr != 0UL) {
			if (!callback_fn(cookie, return_addr)) {
				return;
			}
			max_frames--;
		}

		pcxi = cr_read(TRICORE_PCXI);
	}

	walk_contexts(callback_fn, cookie, pcxi, max_frames);
}
