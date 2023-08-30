/* Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>
#include <xtensa/corebits.h>
#include <xtensa/config/core-isa.h>
#include <zephyr/toolchain.h>
#include <zephyr/kernel_structs.h>
#include <ksched.h>
#include <zephyr/arch/xtensa/xtensa-win0.h>
#include <_soc_inthandlers.h>

/* These are used as part of a mocking layer to test syscall handling
 * without a full userspace.  Will be removed.
 */
__weak int _mock_priv_stack, _k_syscall_table;

/* FIXME: the interrupt handling below is almost-but-not-quite
 * identical to the code in asm2 (the difference being that win0
 * passes a NULL to get_next_switch_handle() as it implements a
 * partial context save and will set the switch handle itself later.
 * Work out the parameterization and move it somewhere shared.
 *
 * (Also this is using printk and not LOG_ERR(), as logging is not
 * always configured on all apps (and rarely works early enough for
 * arch-level code).  Low level error logging is not a pretty
 * situation in Zephyr.)
 */
void z_irq_spurious(const __unused void *arg)
{
	int irqs, ie;

	__asm__ volatile("rsr.interrupt %0" : "=r"(irqs));
	__asm__ volatile("rsr.intenable %0" : "=r"(ie));
	printk(" ** Spurious INTERRUPT(s) %p, INTENABLE = %p\n",
	       (void *)irqs, (void *)ie);
	z_xtensa_fatal_error(K_ERR_SPURIOUS_IRQ, NULL);
}

#define DEF_INT_C_HANDLER(l)						\
void *xtensa_int##l##_c(void *interrupted)		   		\
{									\
	uint32_t irqs, intenable, m;					\
	IF_ENABLED(CONFIG_SCHED_THREAD_USAGE, z_sched_usage_stop());	\
	__asm__ volatile("rsr.interrupt %0" : "=r"(irqs));		\
	__asm__ volatile("rsr.intenable %0" : "=r"(intenable));		\
	irqs &= intenable;						\
	while ((m = _xtensa_handle_one_int##l(irqs))) {			\
		irqs ^= m;						\
		__asm__ volatile("wsr.intclear %0" : : "r"(m));		\
	}								\
	return z_get_next_switch_handle(NULL);				\
}

#if XCHAL_NMILEVEL >= 2
DEF_INT_C_HANDLER(2)
#endif
#if XCHAL_NMILEVEL >= 3
DEF_INT_C_HANDLER(3)
#endif
#if XCHAL_NMILEVEL >= 4
DEF_INT_C_HANDLER(4)
#endif
#if XCHAL_NMILEVEL >= 5
DEF_INT_C_HANDLER(5)
#endif
#if XCHAL_NMILEVEL >= 6
DEF_INT_C_HANDLER(6)
#endif
#if XCHAL_NMILEVEL >= 7
DEF_INT_C_HANDLER(7)
#endif

static ALWAYS_INLINE DEF_INT_C_HANDLER(1)

/* FIXME: in win0 that reason argument in a2 is rotated out and hidden
 * when it gets to the handler.  Need to either dig it out or find
 * some other convention.  Right now we just abort the thread.
 */
__unused static char arch_except_pc;
void xtensa_arch_except(__unused int reason_p)
{
	__asm__("arch_except_pc: ill");
}

/* FIXME: same, need a mechanism for spilling rotated frames inside
 * the handler.  Also this isn't a "stack" (in the sense of a call
 * stack), it's dumping the interrupted context, which just happens to
 * be stored on the stack in asm2...
 */
void z_xtensa_dump_stack(const __unused z_arch_esf_t *stack)
{
}

void *xtensa_excint1_c(xtensa_win0_ctx_t *ctx)
{
	uint32_t cause, vaddr, reason = K_ERR_CPU_EXCEPTION;

	__asm__ volatile("rsr %0, EXCCAUSE" : "=r"(cause));
	if (cause == EXCCAUSE_LEVEL1_INTERRUPT) {
		return xtensa_int1_c(NULL);
	}

	/* In win0, everything else is fatal (syscalls and TLB
	 * exceptions have their own path in the asm upstream, alloca
	 * exceptions don't happen)
	 */
	__asm__ volatile("rsr %0, EXCVADDR" : "=r"(vaddr));
	printk(" ** FATAL EXCEPTION\n");
	printk(" ** CPU %d EXCCAUSE %d\n", arch_curr_cpu()->id, cause);
	printk(" **  PC %p VADDR %p\n", (void *)ctx->pc, (void *)vaddr);
	printk(" **  PS 0x%x\n", ctx->ps);

	z_xtensa_fatal_error(reason, (void *)ctx);
	return z_get_next_switch_handle(NULL);
}

void arch_new_thread(struct k_thread *thread, k_thread_stack_t *stack,
		     char *stack_ptr, k_thread_entry_t entry,
		     void *p1, void *p2, void *p3)
{
#ifdef CONFIG_KERNEL_COHERENCE
	__ASSERT((((size_t)stack) % XCHAL_DCACHE_LINESIZE) == 0, "");
	__ASSERT((((size_t)stack_ptr) % XCHAL_DCACHE_LINESIZE) == 0, "");
	sys_cache_data_flush_and_invd_range(stack, stack_ptr - (char *)stack);
#endif

	memset(&thread->arch.ctx, 0, sizeof(thread->arch.ctx));
	thread->arch.ctx.pc = (uint32_t) z_thread_entry;
	thread->arch.ctx.a1 = (uint32_t) stack_ptr;
	thread->arch.ctx.a2 = (uint32_t) entry;
	thread->arch.ctx.a3 = (uint32_t) p1;
	thread->arch.ctx.a4 = (uint32_t) p2;
	thread->arch.ctx.a5 = (uint32_t) p3;

	thread->switch_handle = &thread->arch.ctx;
}

/* FIXME: these two are 100% cut and paste from xtensa-asm2.c, move
 * somewhere shared
 */
int z_xtensa_irq_is_enabled(unsigned int irq)
{
	uint32_t ie;

	__asm__ volatile("rsr.intenable %0" : "=r"(ie));

	return (ie & (1 << irq)) != 0U;
}
#ifdef CONFIG_XTENSA_MORE_SPIN_RELAX_NOPS
__no_optimization void arch_spin_relax(void)
{
#define NOP1(_, __) __asm__ volatile("nop.n;");
	LISTIFY(CONFIG_XTENSA_NUM_SPIN_RELAX_NOPS, NOP1, (;))
#undef NOP1
}
#endif /* CONFIG_XTENSA_MORE_SPIN_RELAX_NOPS */
