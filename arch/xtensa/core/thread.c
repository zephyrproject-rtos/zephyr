/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_INIT_STACKS
#include <string.h>
#endif /* CONFIG_INIT_STACKS */
#ifdef CONFIG_DEBUG
#include <misc/printk.h>
#endif
#include <kernel_structs.h>
#include <wait_q.h>
#include <xtensa_config.h>
#include <kernel_internal.h>

extern void _xt_user_exit(void);

/*
 * @brief Initialize a new thread
 *
 * Any coprocessor context data is put at the lower address of the stack. An
 * initial context, to be "restored" by __return_from_coop(), is put at
 * the other end of the stack, and thus reusable by the stack when not
 * needed anymore.
 *
 * The initial context is a basic stack frame that contains arguments for
 * _thread_entry() return address, that points at _thread_entry()
 * and status register.
 *
 * <options> is currently unused.
 *
 * @param thread pointer to k_thread memory
 * @param pStackmem the pointer to aligned stack memory
 * @param stackSize the stack size in bytes
 * @param pEntry thread entry point routine
 * @param p1 first param to entry point
 * @param p2 second param to entry point
 * @param p3 third param to entry point
 * @param priority thread priority
 * @param options is unused (saved for future expansion)
 *
 * @return N/A
 */

void _new_thread(struct k_thread *thread, k_thread_stack_t *stack,
		size_t stackSize, k_thread_entry_t pEntry,
		void *p1, void *p2, void *p3,
		int priority, unsigned int options)
{
	char *pStack = K_THREAD_STACK_BUFFER(stack);

	/* Align stack end to maximum alignment requirement. */
	char *stackEnd = (char *)ROUND_DOWN(pStack + stackSize, 16);
#if XCHAL_CP_NUM > 0
	u32_t *cpSA;
	char *cpStack;
#endif

	_new_thread_init(thread, pStack, stackSize, priority, options);

#ifdef CONFIG_DEBUG
	printk("\nstackPtr = %p, stackSize = %d\n", pStack, stackSize);
	printk("stackEnd = %p\n", stackEnd);
#endif
#if XCHAL_CP_NUM > 0
	/* Ensure CP state descriptor is correctly initialized */
	cpStack = thread->arch.preempCoprocReg.cpStack; /* short hand alias */
	/* Set to zero to avoid bad surprises */
	(void)memset(cpStack, 0, XT_CP_ASA);
	/* Coprocessor's stack is allocated just after the k_thread */
	cpSA = (u32_t *)(thread->arch.preempCoprocReg.cpStack + XT_CP_ASA);
	/* Coprocessor's save area alignment is at leat 16 bytes */
	*cpSA = ROUND_UP(cpSA + 1,
		(XCHAL_TOTAL_SA_ALIGN < 16 ? 16 : XCHAL_TOTAL_SA_ALIGN));
#ifdef CONFIG_DEBUG
	printk("cpStack  = %p\n", thread->arch.preempCoprocReg.cpStack);
	printk("cpAsa    = %p\n",
	       *(void **)(thread->arch.preempCoprocReg.cpStack + XT_CP_ASA));
#endif
#endif
	/* Thread's first frame alignment is granted as both operands are
	 * aligned
	 */
	XtExcFrame *pInitCtx =
		(XtExcFrame *)(stackEnd - (XT_XTRA_SIZE - XT_CP_SIZE));
#ifdef CONFIG_DEBUG
	printk("pInitCtx = %p\n", pInitCtx);
#endif
	/* Explicitly initialize certain saved registers */

	 /* task entrypoint */
	pInitCtx->pc   = (u32_t)_thread_entry;

	/* physical top of stack frame */
	pInitCtx->a1   = (u32_t)pInitCtx + XT_STK_FRMSZ;

	/* user exception exit dispatcher */
	pInitCtx->exit = (u32_t)_xt_user_exit;

	/* Set initial PS to int level 0, EXCM disabled, user mode.
	 * Also set entry point argument arg.
	 */
#ifdef __XTENSA_CALL0_ABI__
	pInitCtx->a2 = (u32_t)pEntry;
	pInitCtx->a3 = (u32_t)p1;
	pInitCtx->a4 = (u32_t)p2;
	pInitCtx->a5 = (u32_t)p3;
	pInitCtx->ps = PS_UM | PS_EXCM;
#else
	/* For windowed ABI set also WOE and CALLINC
	 * (pretend task is 'call4')
	 */
	pInitCtx->a6 = (u32_t)pEntry;
	pInitCtx->a7 = (u32_t)p1;
	pInitCtx->a8 = (u32_t)p2;
	pInitCtx->a9 = (u32_t)p3;
	pInitCtx->ps = PS_UM | PS_EXCM | PS_WOE | PS_CALLINC(1);
#endif
	thread->callee_saved.topOfStack = pInitCtx;
	thread->arch.flags = 0;
	/* initial values in all other registers/k_thread entries are
	 * irrelevant
	 */
}

