/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/hexagon/arch.h>
#include <zephyr/arch/hexagon/exception.h>
#include <event_context.h>
#include <zephyr/logging/log.h>
#include <zephyr/fatal.h>
#include <kswap.h>
#include <hexagon_vm.h>
#include <hexagon_fatal.h>
#include <irq.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

/* Get current exception stack frame */
static void z_hexagon_get_current_esf(struct arch_esf *esf)
{
	uint32_t g_val;

	memset(esf, 0, sizeof(*esf));

	__asm__ volatile("%0 = g0" : "=r"(g_val));
	esf->event_info[0] = g_val;
	__asm__ volatile("%0 = g1" : "=r"(g_val));
	esf->event_info[1] = g_val;
	__asm__ volatile("%0 = g2" : "=r"(g_val));
	esf->event_info[2] = g_val;
	/* g3 is GBADVA, including for a stack-limit exception (cause 0x27). */
	__asm__ volatile("%0 = g3" : "=r"(g_val));
	esf->event_info[3] = g_val;
}

#ifdef CONFIG_THREAD_STACK_INFO
static bool z_hexagon_event_frame_in_stack(uintptr_t frame_addr,
					   uintptr_t stack_start,
					   size_t stack_size)
{
	size_t frame_offset;

	if (stack_start > UINTPTR_MAX - stack_size || frame_addr < stack_start) {
		return false;
	}

	frame_offset = frame_addr - stack_start;
	return frame_offset <= stack_size &&
		stack_size - frame_offset >= (EVENT_CTX_SIZE + sizeof(uint32_t));
}
#endif

bool z_hexagon_event_frame_fp_get(const struct event_context *ctx, uint32_t *fp)
{
	uintptr_t frame_addr;
	uintptr_t fp_addr;
#ifdef CONFIG_THREAD_STACK_INFO
	uintptr_t stack_start;
	size_t stack_size;
	bool in_stack;
#endif

	if ((ctx == NULL) || (fp == NULL)) {
		return false;
	}

	frame_addr = (uintptr_t)ctx;
	if (frame_addr > UINTPTR_MAX - EVENT_CTX_SIZE ||
		frame_addr + EVENT_CTX_SIZE > UINTPTR_MAX - sizeof(uint32_t)) {
		return false;
	}

#ifdef CONFIG_THREAD_STACK_INFO
	if (_current == NULL) {
		return false;
	}

	stack_start = _current->stack_info.start;
	stack_size = _current->stack_info.size;
#ifdef CONFIG_HW_STACK_PROTECTION
	/* A stack-limit event frame sits in the headroom below FRAMELIMIT. */
	if ((_current->arch.flags & HEXAGON_THREAD_FLAG_STACK_PROT) != 0U &&
	    stack_start >= HEXAGON_STACK_LIMIT_HEADROOM) {
		stack_start -= HEXAGON_STACK_LIMIT_HEADROOM;
		stack_size += HEXAGON_STACK_LIMIT_HEADROOM;
	}
#endif

	in_stack = z_hexagon_event_frame_in_stack(frame_addr, stack_start, stack_size);
#ifdef CONFIG_USERSPACE
	in_stack = in_stack ||
		z_hexagon_event_frame_in_stack(frame_addr,
					       (uintptr_t)_current->arch.priv_stack,
					       sizeof(_current->arch.priv_stack));
#endif
	if (!in_stack) {
		return false;
	}
#endif

	fp_addr = frame_addr + EVENT_CTX_SIZE;
	if ((fp_addr & (sizeof(uint32_t) - 1U)) != 0U) {
		return false;
	}

	*fp = *(const uint32_t *)fp_addr;
	return true;
}


static void z_hexagon_esf_from_ctx(struct arch_esf *esf,
					unsigned int event_type,
					const struct event_context *ctx)
{
	uint32_t fp;

	memset(esf, 0, sizeof(*esf));

	if (ctx == NULL) {
		return;
	}

	esf->r0 = ctx->r0_r1[0];
	esf->r1 = ctx->r0_r1[1];
	esf->r2 = ctx->r2_r3[0];
	esf->r3 = ctx->r2_r3[1];
	esf->r4 = ctx->r4_r5[0];
	esf->r5 = ctx->r4_r5[1];
	esf->r6 = ctx->r6_r7[0];
	esf->r7 = ctx->r6_r7[1];
	esf->r8 = ctx->r8_r9[0];
	esf->r9 = ctx->r8_r9[1];
	esf->r10 = ctx->r10_r11[0];
	esf->r11 = ctx->r10_r11[1];
	esf->r12 = ctx->r12_r13[0];
	esf->r13 = ctx->r12_r13[1];
	esf->r14 = ctx->r14_r15[0];
	esf->r15 = ctx->r14_r15[1];
#ifdef CONFIG_DEBUG_COREDUMP
	/* Only fault events have r16-r27 in the frame. */
	if (event_type <= HEXAGON_EVENT_DEBUG) {
		esf->r16 = ctx->r16_r27[0];
		esf->r17 = ctx->r16_r27[1];
		esf->r18 = ctx->r16_r27[2];
		esf->r19 = ctx->r16_r27[3];
		esf->r20 = ctx->r16_r27[4];
		esf->r21 = ctx->r16_r27[5];
		esf->r22 = ctx->r16_r27[6];
		esf->r23 = ctx->r16_r27[7];
		esf->r24 = ctx->r16_r27[8];
		esf->r25 = ctx->r16_r27[9];
		esf->r26 = ctx->r16_r27[10];
		esf->r27 = ctx->r16_r27[11];
	}
#endif
	esf->r28 = ctx->r28;
	esf->r31_lr = ctx->link_reg;
	esf->pc = ctx->gelr;
	esf->event_type = event_type;
	esf->event_info[0] = ctx->gelr;
	esf->event_info[1] = ctx->gsr;
	esf->event_info[2] = ctx->gosp;
	esf->event_info[3] = ctx->badva;
	esf->r29_sp = (uint32_t)(uintptr_t)ctx + EVENT_CTX_SIZE +
		      EVENT_ENTRY_ALLOCFRAME_OVERHEAD;
	if (z_hexagon_event_frame_fp_get(ctx, &fp)) {
		esf->r30_fp = fp;
	}
}

static void z_hexagon_dump_esf(const struct arch_esf *esf)
{
	LOG_ERR(" r0: 0x%08x r1: 0x%08x r2: 0x%08x r3: 0x%08x",
		esf->r0, esf->r1, esf->r2, esf->r3);
	LOG_ERR(" r4: 0x%08x r5: 0x%08x r6: 0x%08x r7: 0x%08x",
		esf->r4, esf->r5, esf->r6, esf->r7);
	LOG_ERR(" r8: 0x%08x r9: 0x%08x r10: 0x%08x r11: 0x%08x",
		esf->r8, esf->r9, esf->r10, esf->r11);
	LOG_ERR("r12: 0x%08x r13: 0x%08x r14: 0x%08x r15: 0x%08x",
		esf->r12, esf->r13, esf->r14, esf->r15);
	LOG_ERR("r28: 0x%08x sp: 0x%08x fp: 0x%08x lr: 0x%08x",
		esf->r28, esf->r29_sp, esf->r30_fp, esf->r31_lr);
	LOG_ERR(" pc: 0x%08x gsr: 0x%08x gosp: 0x%08x gbadva: 0x%08x",
		esf->pc, esf->event_info[1], esf->event_info[2], esf->event_info[3]);
}

/*
 * Unlike the trap-based ARCH_EXCEPT() of other arches, ours is a plain
 * noreturn call, so the compiler drops whatever code follows k_panic()
 * in the caller. z_fatal_error() does return when the faulting thread
 * was already dead before the panic -- e.g. an essential thread
 * aborting itself, where z_thread_halt() marks _current dead and then
 * calls k_panic(), relying on the swap after it that we never reach.
 * Swap away from the dead thread here instead of falling off the end
 * of a noreturn function.
 */
static FUNC_NORETURN void z_hexagon_fatal_error_return(unsigned int reason)
{
	/* arch_is_in_isr() tracks IRQ nesting, not exception/machine-check events. */
	if (!arch_is_in_isr() && z_is_thread_dead(_current)) {
		z_swap_unlocked();
	}

	k_fatal_halt(reason);
}

FUNC_NORETURN void z_hexagon_fatal_error(unsigned int reason)
{
	struct arch_esf esf = {0};
	/* Our allocframe holds the caller's fp and lr, with its sp just above. */
	const uint32_t *frame = __builtin_frame_address(0);

	z_hexagon_get_current_esf(&esf);
	esf.r31_lr = (uint32_t)(uintptr_t)__builtin_return_address(0);
	/*
	 * The call to us is noreturn, so lr can point past the end of the
	 * caller. Step back into the call so the pc maps to the right line.
	 */
	esf.pc = esf.r31_lr - 4;
	esf.r30_fp = frame[0];
	esf.r29_sp = (uint32_t)(uintptr_t)&frame[2];
	z_fatal_error(reason, &esf);

	z_hexagon_fatal_error_return(reason);
}

FUNC_NORETURN void z_hexagon_fatal_error_ctx(unsigned int reason,
					      unsigned int event_type,
					      const struct event_context *ctx)
{
	struct arch_esf esf;

	z_hexagon_esf_from_ctx(&esf, event_type, ctx);
	z_hexagon_dump_esf(&esf);
	z_fatal_error(reason, &esf);

	z_hexagon_fatal_error_return(reason);
}

#ifndef CONFIG_USERSPACE
FUNC_NORETURN void arch_syscall_oops(void *ssf)
{
	ARG_UNUSED(ssf);
	z_hexagon_fatal_error(K_ERR_KERNEL_OOPS);
	CODE_UNREACHABLE;
}
#endif

FUNC_NORETURN void z_do_kernel_oops(const struct arch_esf *esf)
{
	/* Extract reason from r0 in the ESF, following Zephyr convention */
	unsigned int reason = esf->r0;

	z_fatal_error(reason, esf);

	CODE_UNREACHABLE;
}

FUNC_NORETURN void z_arch_except(unsigned int reason)
{
	struct arch_esf esf;

	z_hexagon_get_current_esf(&esf);
	z_fatal_error(reason, &esf);

	z_hexagon_fatal_error_return(reason);
}

FUNC_NORETURN void arch_system_halt(unsigned int reason)
{
	ARG_UNUSED(reason);
	hexagon_vm_stop(VM_STOP_HALT);

	while (1) {
	}
}
