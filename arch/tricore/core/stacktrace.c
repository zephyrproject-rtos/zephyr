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

extern union z_tricore_context z_tricore_csa[];

static inline bool valid_csa(uintptr_t addr)
{
	uintptr_t base = (uintptr_t)z_tricore_csa;

	return (addr & 0x3FU) == 0U &&
	       addr - base < (uintptr_t)CONFIG_TRICORE_CSA_COUNT * sizeof(union z_tricore_context);
}

static void walk_contexts(stack_trace_callback_fn callback_fn, void *cookie, uint32_t pcxi,
			  unsigned int frames)
{
	for (; pcxi != 0U && frames < CONFIG_ARCH_STACKWALK_MAX_FRAMES; frames++) {
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

		if (a11 != 0U && !callback_fn(cookie, a11)) {
			return;
		}

		pcxi = csa->lower.pcxi;
	}
}

void arch_stack_walk(stack_trace_callback_fn callback_fn, void *cookie,
		     const struct k_thread *thread, const struct arch_esf *esf)
{
	unsigned int frames = 0U;
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

	/* CSA reads may be buffered by the hardware; make them visible. */
	__asm volatile("dsync" ::: "memory");

	if (esf != NULL) {
		/* The lower context saved by svlcx on interrupt entry. */
		if (!valid_csa((uintptr_t)esf)) {
			return;
		}

		if (esf->a11 != 0U) {
			if (!callback_fn(cookie, esf->a11)) {
				return;
			}

			frames++;
		}
		pcxi = esf->pcxi;
	} else if (thread != _current || k_is_in_isr()) {
		/* Saved by a context switch or the outermost interrupt entry. */
		pcxi = thread->callee_saved.pcxi;
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
			frames++;
		}

		pcxi = cr_read(TRICORE_PCXI);
	}

	walk_contexts(callback_fn, cookie, pcxi, frames);
}
