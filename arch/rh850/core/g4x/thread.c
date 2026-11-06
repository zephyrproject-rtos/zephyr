/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <kswap.h>
#include <zephyr/arch/rh850/g4x/g4x.h>

static k_thread_entry_t s_main_entry;

/* ---- Export offsets so switch.S can load them at runtime (no hardcode) ---- */
const unsigned int __k_thread_sp_offs = offsetof(struct k_thread, callee_saved.sp);
const unsigned int __k_thread_key_offs = offsetof(struct k_thread, arch.key);
const unsigned int __k_thread_swap_ret_offs = offsetof(struct k_thread, arch.swap_return_value);

/* Some Zephyr internals we need */
extern struct z_kernel _kernel;

extern void z_thread_entry(k_thread_entry_t entry, void *p1, void *p2, void *p3);

void __noinline z_rh850_thread_boot(void)
{
	/* Enable acknowledgment of EI interrrupts */
	__asm__ volatile("ei" : : : "memory");
	z_thread_entry(s_main_entry, NULL, NULL, NULL);
}

void __noinline z_rh850_exit(void)
{
	while (1) {
		__asm__ volatile("nop" : : : "memory");
	}
}

void arch_switch_to_main_thread(struct k_thread *main_thread, char *stack_ptr,
				k_thread_entry_t _main)
{
	z_current_thread_set(main_thread);

	s_main_entry = _main;

	__asm__ volatile("mov %0, sp\n"                    /* Set sp register */
		       "stsr  5, r10, 0\n"               /* r10 <- PSW */
		       "ldsr  r10, 3, 0\n"               /* FEPSW <- r10 */
		       "mov _z_rh850_exit, lp\n"         /* lp <- #_z_rh850_exit */
		       "mov _z_rh850_thread_boot, r10\n" /* Branch to z_rh850_thread_boot */
		       "ldsr r10, 2, 0\n"                /* FEPC <- #_z_rh850_thread_boot */
		       "feret\n"                         /* Apply PSW and PC to start user mode */
		       :
		       : "r"(stack_ptr)
		       : "r10", "memory", "cc");

	CODE_UNREACHABLE;
}

int arch_coprocessors_disable(struct k_thread *thread)
{
	return 0;
}

void arch_new_thread(struct k_thread *thread, k_thread_stack_t *stack, char *stack_ptr,
		     k_thread_entry_t entry, void *p1, void *p2, void *p3)
{
	ARG_UNUSED(stack);

	/* TP: Text Pointer */
	uint32_t tp = 0;

	__asm__ volatile("mov r5, %0" : "=r"(tp));

	struct arch_esf *iframe;

	iframe = Z_STACK_PTR_TO_FRAME(struct arch_esf, stack_ptr);

	/* System regs (top of frame) */
	uint32_t psw_ebv_cux = PSW_EBV;

#ifdef CONFIG_FXU
	/* Enable PSW_CU1 to enable FXU coprocessor */
	psw_ebv_cux |= PSW_CU1;
	/* Set initial value for FXU registers */
	iframe->fxxc = 0U;
	iframe->fxsr = FXSR_FS;
#endif

#ifdef CONFIG_FPU
	/* Always enable PSW_CU0 regardless of CONFIG_FPU_SHARING to prevent UCPOP exception */
	psw_ebv_cux |= PSW_CU0;

/* Set initial value for FPU registers if this thread uses the FPU */
#ifdef CONFIG_FPU_SHARING
	iframe->fpepc = 0U;
	iframe->fpsr = FPSR_FS;
#endif
#endif

	/* Set initial PSW value, this value will be written into PSW when entering this thread */
	iframe->eipsw = psw_ebv_cux;

	/* Set initial PC value when entering this thread */
	iframe->eipc = (uint32_t)z_thread_entry;
	iframe->eiwr = 0U;
	iframe->ctpsw = psw_ebv_cux;
	iframe->ctpc = 0U;

	/* r1-r2: initial value when enterring this thread (for debugging) */
	iframe->r1 = 0x11111111U;
	iframe->r2 = 0x22222222U;

	/* r5-r30: fill patterns + args */
	iframe->r5 = tp;

	/* RH850 ABI: r6..r9 = a1..a4
	 * Call: z_thread_entry(entry, p1, p2, p3)
	 */
	iframe->r6 = (uint32_t)entry; /* a1 */
	iframe->r7 = (uint32_t)p1;    /* a2 */
	iframe->r8 = (uint32_t)p2;    /* a3 */
	iframe->r9 = (uint32_t)p3;    /* a4 */

	/* r10-r30: initial value when enterring this thread (for debugging) */
	iframe->r10 = 0x10101010U;
	iframe->r11 = 0x11111111U;
	iframe->r12 = 0x12121212U;
	iframe->r13 = 0x13131313U;
	iframe->r14 = 0x14141414U;
	iframe->r15 = 0x15151515U;
	iframe->r16 = 0x16161616U;
	iframe->r17 = 0x17171717U;
	iframe->r18 = 0x18181818U;
	iframe->r19 = 0x19191919U;
	iframe->r20 = 0x20202020U;
	iframe->r21 = 0x21212121U;
	iframe->r22 = 0x22222222U;
	iframe->r23 = 0x23232323U;
	iframe->r24 = 0x24242424U;
	iframe->r25 = 0x25252525U;
	iframe->r26 = 0x26262626U;
	iframe->r27 = 0x27272727U;
	iframe->r28 = 0x28282828U;
	iframe->r29 = 0x29292929U;
	iframe->r30 = 0x30303030U;

	/* SP of new thread = address of this frame (EIPSW) */
	*(uint32_t *)((uint8_t *)thread + __k_thread_sp_offs) = (uint32_t)iframe;

	thread->arch.key = 0U;
	thread->arch.swap_return_value = 0U;

	thread->switch_handle = (void *)iframe;
}

/* arch_swap() */
int arch_swap(unsigned int key)
{
	_cpu_t *cpu = arch_curr_cpu();
	struct k_thread *current = cpu->current;

	/* Store key & return value */
	*(unsigned int *)((uint8_t *)current + __k_thread_key_offs) = key;
	*(int *)((uint8_t *)current + __k_thread_swap_ret_offs) = -EAGAIN;

	void *interrupted_handle = current->switch_handle;
	void *next_handle = z_get_next_switch_handle(interrupted_handle);

	if (next_handle != interrupted_handle) {
		arch_switch(next_handle, &current->switch_handle);
	}

	return *(int *)((uint8_t *)arch_curr_cpu()->current + __k_thread_swap_ret_offs);
}

int arch_smp_init(void)
{
	/* TODO: perform initialization for smp support */
	return 0;
}

void arch_sched_broadcast_ipi(void)
{
	/* TODO: raise IPIn to all other cores */
}

void arch_sched_directed_ipi(uint32_t cpu_bitmap)
{
	ARG_UNUSED(cpu_bitmap);
	/* TODO: raise IPIn to selected cores per bitmap */
}

/* CPU idle primitives */
void arch_cpu_idle(void)
{
#if defined(CONFIG_TRACING)
	sys_trace_idle();
#endif

	__asm__ volatile("halt\n"
			 "ei\n"
			 :
			 :
			 : "memory");

#if defined(CONFIG_TRACING)
	sys_trace_idle_exit();
#endif
}

void arch_cpu_atomic_idle(unsigned int key)
{
#if defined(CONFIG_TRACING)
	sys_trace_idle();
#endif

	__asm__ volatile("halt" : : : "memory");

#if defined(CONFIG_TRACING)
	sys_trace_idle_exit();
#endif

	irq_unlock(key);
}
