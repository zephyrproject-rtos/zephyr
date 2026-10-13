/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/hexagon/exception.h>
#include <zephyr/kernel_structs.h>
#include <switch_frame.h>
#include <event_context.h>

#define MAX_STACK_FRAMES CONFIG_ARCH_STACKWALK_MAX_FRAMES

#ifdef CONFIG_MULTITHREADING
/* EVENT_EXIT context switch return addresses, see event_handlers.S. */
extern const uintptr_t z_hexagon_event_switch_ret[];
#endif

static bool is_event_switch_ret(uintptr_t lr)
{
#ifdef CONFIG_MULTITHREADING
	for (const uintptr_t *ret = z_hexagon_event_switch_ret; *ret != 0U; ret++) {
		if (*ret == lr) {
			return true;
		}
	}
#else
	ARG_UNUSED(lr);
#endif

	return false;
}

#if defined(CONFIG_THREAD_STACK_INFO) || defined(CONFIG_USERSPACE)
static bool in_region(uintptr_t addr, size_t length, uintptr_t start, size_t size)
{
	return (addr >= start) && (addr - start <= size) &&
		(length <= size - (addr - start));
}
#endif

static bool in_stack_bound(uintptr_t addr, size_t length,
			   const struct k_thread *thread)
{
	if (thread == NULL) {
		return false;
	}

#ifdef CONFIG_THREAD_STACK_INFO
	if (in_region(addr, length, thread->stack_info.start, thread->stack_info.size)) {
		return true;
	}
#endif

#ifdef CONFIG_USERSPACE
	if (k_thread_is_user_thread(thread) &&
	    in_region(addr, length, (uintptr_t)thread->arch.priv_stack,
		      sizeof(thread->arch.priv_stack))) {
		return true;
	}
#endif

	return false;
}

static bool in_stack_frame(uintptr_t addr, const struct k_thread *thread)
{
	return in_stack_bound(addr, 2U * sizeof(uint32_t), thread);
}

static bool in_same_stack_region(uintptr_t addr1, uintptr_t addr2,
				 const struct k_thread *thread)
{
	if (thread == NULL) {
		return false;
	}

#ifdef CONFIG_THREAD_STACK_INFO
	if (in_region(addr1, 2U * sizeof(uint32_t), thread->stack_info.start,
		      thread->stack_info.size) &&
	    in_region(addr2, 2U * sizeof(uint32_t), thread->stack_info.start,
		      thread->stack_info.size)) {
		return true;
	}
#endif

#ifdef CONFIG_USERSPACE
	if (k_thread_is_user_thread(thread) &&
	    in_region(addr1, 2U * sizeof(uint32_t), (uintptr_t)thread->arch.priv_stack,
		      sizeof(thread->arch.priv_stack)) &&
	    in_region(addr2, 2U * sizeof(uint32_t), (uintptr_t)thread->arch.priv_stack,
		      sizeof(thread->arch.priv_stack))) {
		return true;
	}
#endif

	return false;
}

static void walk_stackframe(stack_trace_callback_fn callback_fn, void *cookie,
			    const struct k_thread *thread, uintptr_t fp, uintptr_t lr)
{
	for (uint32_t i = 0U; i < MAX_STACK_FRAMES; i++) {
		if (lr == 0U || !callback_fn(cookie, lr)) {
			return;
		}

		if (fp == 0U || !in_stack_frame(fp, thread)) {
			return;
		}

		const uint32_t *frame = (const uint32_t *)fp;

		lr = frame[1];
		uintptr_t next_fp = frame[0];

		if ((next_fp <= fp) &&
		    (!in_stack_frame(next_fp, thread) ||
		     in_same_stack_region(fp, next_fp, thread))) {
			return;
		}
		fp = next_fp;
	}
}

void arch_stack_walk(stack_trace_callback_fn callback_fn, void *cookie,
			     const struct k_thread *thread, const struct arch_esf *esf)
{
	uintptr_t fp;
	uintptr_t lr;

	if (callback_fn == NULL) {
		return;
	}

	if (thread == NULL) {
		thread = _current;
	}

	if (esf != NULL) {
		fp = esf->r30_fp;
		lr = esf->pc;
	} else if ((thread == NULL) || (thread == _current)) {
		__asm__ volatile("%[fp] = r30" : [fp] "=r"(fp));
		__asm__ volatile("%[lr] = r31" : [lr] "=r"(lr));
		thread = _current;

		/* r31 already holds the return address saved in this frame. */
		if (in_stack_frame(fp, thread)) {
			fp = ((const uint32_t *)fp)[0];
		}
	} else {
		uintptr_t frame_addr = (uintptr_t)thread->switch_handle;
		const uint32_t *frame;

		if (!in_stack_bound(frame_addr, SWITCH_FRAME_SIZE, thread)) {
			return;
		}

		frame = (const uint32_t *)frame_addr;
		fp = frame[SWITCH_FP_LR / 4];
		lr = frame[SWITCH_FP_LR / 4 + 1];

		/*
		 * Preempted by an event: fp is the EVENT_ENTRY frame, right
		 * above the event context. Resume at the interrupted PC and
		 * frame, as for an ESF.
		 */
		if (is_event_switch_ret(lr)) {
			const struct event_context *ctx;

			if ((fp < EVENT_CTX_SIZE) ||
			    !in_stack_bound(fp - EVENT_CTX_SIZE,
					    EVENT_CTX_SIZE + EVENT_ENTRY_ALLOCFRAME_OVERHEAD,
					    thread)) {
				return;
			}

			ctx = (const struct event_context *)(fp - EVENT_CTX_SIZE);
			lr = ctx->gelr;
			fp = ((const uint32_t *)fp)[0];
		}
	}

	walk_stackframe(callback_fn, cookie, thread, fp, lr);
}
