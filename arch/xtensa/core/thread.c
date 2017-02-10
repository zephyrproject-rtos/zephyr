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

extern void _xt_user_exit(void);
#if CONFIG_MICROKERNEL
extern FUNC_NORETURN void _TaskAbort(void);
#endif
extern void fiber_abort(void);

#if defined(CONFIG_THREAD_MONITOR)
#define THREAD_MONITOR_INIT(tcs) _thread_monitor_init(tcs)
#else
#define THREAD_MONITOR_INIT(tcs) \
	do {/* do nothing */     \
	} while ((0))
#endif

#if defined(CONFIG_THREAD_MONITOR)
/**
 *
 * @brief Initialize thread monitoring support
 *
 * Currently only inserts the new thread in the list of active threads.
 *
 * @return N/A
 */

static inline void _thread_monitor_init(struct tcs *tcs)
{
	unsigned int key;

	/*
	 * Add the newly initialized thread to head of the list of threads.
	 * This singly linked list of threads maintains ALL the threads in the
	 * system:
	 * both tasks and fibers regardless of whether they are runnable.
	 */

	key = irq_lock();
	tcs->next_thread = _nanokernel.threads;
	_nanokernel.threads = tcs;
	irq_unlock(key);
}
#endif /* CONFIG_THREAD_MONITOR */

/*
 * @brief Initialize a new thread from its stack space
 *
 * The control structure (TCS) is put at the lower address of the stack. An
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
 * @param pStackmem the pointer to aligned stack memory
 * @param stackSize the stack size in bytes
 * @param pEntry thread entry point routine
 * @param p1 first param to entry point
 * @param p2 second param to entry point
 * @param p3 third param to entry point
 * @param fiber prio, -1 for task
 * @param options is unused (saved for future expansion)
 *
 * @return N/A
 */

void _new_thread(char *pStack, size_t stackSize,
		void (*pEntry)(void *, void *, void *),
		void *p1, void *p2, void *p3,
		int prio, unsigned int options)
{
	/* Align stack end to maximum alignment requirement. */
	char *stackEnd = (char *)ROUND_DOWN(pStack + stackSize,
		(XCHAL_TOTAL_SA_ALIGN < 16 ? 16 : XCHAL_TOTAL_SA_ALIGN));
	/* TCS is located at top of stack while frames are located at end
	 * of it
	 */
	struct tcs *tcs = (struct tcs *)(pStack);
#if XCHAL_CP_NUM > 0
	uint32_t *cpSA;
#endif

#ifdef CONFIG_DEBUG
	printk("\nstackPtr = %p, stackSize = %d\n", pStack, stackSize);
	printk("stackEnd = %p\n", stackEnd);
#endif
#ifdef CONFIG_INIT_STACKS
	memset(pStack, 0xaa, stackSize);
#endif
#if XCHAL_CP_NUM > 0
	/* Coprocessor's stack is allocated just after the TCS */
	tcs->arch.preempCoprocReg.cpStack = pStack + sizeof(struct k_thread);
	cpSA = (uint32_t *)(tcs->arch.preempCoprocReg.cpStack + XT_CP_ASA);
	/* Coprocessor's save area alignment is at leat 16 bytes */
	*cpSA = ROUND_UP(cpSA + 1,
		(XCHAL_TOTAL_SA_ALIGN < 16 ? 16 : XCHAL_TOTAL_SA_ALIGN));
#ifdef CONFIG_DEBUG
	printk("cpStack  = %p\n", tcs->arch.preempCoprocReg.cpStack);
	printk("cpAsa    = %p\n",
	       *(void **)(tcs->arch.preempCoprocReg.cpStack + XT_CP_ASA));
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
	pInitCtx->pc   = (uint32_t)_thread_entry;

	/* physical top of stack frame */
	pInitCtx->a1   = (uint32_t)pInitCtx + XT_STK_FRMSZ;

	/* user exception exit dispatcher */
	pInitCtx->exit = (uint32_t)_xt_user_exit;

	/* Set initial PS to int level 0, EXCM disabled, user mode.
	 * Also set entry point argument arg.
	 */
#ifdef __XTENSA_CALL0_ABI__
	pInitCtx->a2 = (uint32_t)pEntry;
	pInitCtx->a3 = (uint32_t)p1;
	pInitCtx->a4 = (uint32_t)p2;
	pInitCtx->a5 = (uint32_t)p3;
	pInitCtx->ps = PS_UM | PS_EXCM;
#else
	/* For windowed ABI set also WOE and CALLINC
	 * (pretend task is 'call4')
	 */
	pInitCtx->a6 = (uint32_t)pEntry;
	pInitCtx->a7 = (uint32_t)p1;
	pInitCtx->a8 = (uint32_t)p2;
	pInitCtx->a9 = (uint32_t)p3;
	pInitCtx->ps = PS_UM | PS_EXCM | PS_WOE | PS_CALLINC(1);
#endif
	tcs->callee_saved.topOfStack = pInitCtx;
	tcs->arch.flags = 0;
	_init_thread_base(&tcs->base, prio, _THREAD_PRESTART, options);
	/* static threads overwrite it afterwards with real value */
	tcs->init_data = NULL;
	tcs->fn_abort = NULL;
#ifdef CONFIG_THREAD_CUSTOM_DATA
	/* Initialize custom data field (value is opaque to kernel) */
	tcs->custom_data = NULL;
#endif
#ifdef CONFIG_THREAD_MONITOR
	/*
	 * In debug mode tcs->entry give direct access to the thread entry
	 * and the corresponding parameters.
	 */
	tcs->entry = (struct __thread_entry *)(pInitCtx);
#endif
	/* initial values in all other registers/TCS entries are irrelevant */

	THREAD_MONITOR_INIT(tcs);
}

