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

static void walk_upper_contexts(stack_trace_callback_fn callback_fn, void *cookie, uint32_t pcxi,
				unsigned int frames)
{
	while (pcxi != 0U && frames < CONFIG_ARCH_STACKWALK_MAX_FRAMES) {
		uintptr_t addr;

		if ((pcxi & PCXI_UL) == 0U) {
			break;
		}

		addr = z_tricore_pcx_to_addr(pcxi);
		if (!valid_csa(addr)) {
			break;
		}

		const z_tricore_upper_context_t *upper = UINT_TO_POINTER(addr);

		if (upper->a11 == 0U) {
			break;
		}
		if (!callback_fn(cookie, upper->a11)) {
			return;
		}

		frames++;
		pcxi = upper->pcxi;
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
	} else if (thread != NULL && thread != _current) {
		/* A switched-out thread's chain starts in callee_saved. */
		pcxi = thread->callee_saved.pcxi;

		/* A syscall switch saves a lower context; emit its return address. */
		if ((pcxi & PCXI_UL) == 0U) {
			uintptr_t addr = z_tricore_pcx_to_addr(pcxi);
			const z_tricore_lower_context_t *lower;

			if (!valid_csa(addr)) {
				return;
			}

			lower = UINT_TO_POINTER(addr);
			if (lower->a11 != 0U) {
				if (!callback_fn(cookie, lower->a11)) {
					return;
				}
				frames++;
			}
			pcxi = lower->pcxi;
		}
	} else {
		unsigned long a11;

		/*
		 * The caller's return address is still in the live A11; the
		 * saved contexts above it hold only older return addresses.
		 */
		__asm volatile("mov.aa %0, %%a11" : "=a"(a11));
		if (a11 != 0U) {
			if (!callback_fn(cookie, a11)) {
				return;
			}
			frames++;
		}

		pcxi = cr_read(TRICORE_PCXI);
	}

	walk_upper_contexts(callback_fn, cookie, pcxi, frames);
}
