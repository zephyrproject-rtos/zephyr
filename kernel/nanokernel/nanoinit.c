/* nanoinit.c - VxMicro nanokernel initialization module */

/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
DESCRIPTION
This module contains routines that are used to initialize the nanokernel.
*/

#include <offsets.h>
#include <cputype.h>
#include <nanokernel.h>
#include <nanokernel/cpu.h>
#include <misc/printk.h>
#include <drivers/rand32.h>
#include <sections.h>
#include <toolchain.h>
#include <nanok.h>

#ifdef CONFIG_MICROKERNEL
#include <minik.h>
#endif /* CONFIG_MICROKERNEL */

/* kernel build timestamp items */

#define BUILD_TIMESTAMP "BUILD: " __DATE__ " " __TIME__

#ifdef CONFIG_BUILD_TIMESTAMP
const char * const build_timestamp = BUILD_TIMESTAMP;
#endif

/* boot banner items */

#define BOOT_BANNER "****** BOOTING VXMICRO ******"

#if !defined(CONFIG_BOOT_BANNER)
#define PRINT_BOOT_BANNER() do { } while (0)
#elif !defined(CONFIG_BUILD_TIMESTAMP)
#define PRINT_BOOT_BANNER() printk(BOOT_BANNER "\n")
#else
#define PRINT_BOOT_BANNER() printk(BOOT_BANNER " %s\n", build_timestamp)
#endif

/* stack space for the background task context */

static char __noinit _k_init_and_idle_task_stack[CONFIG_MAIN_STACK_SIZE];

/* storage space for the interrupt stack */

#ifndef CONFIG_NO_ISRS

/*
 * The symbol for the interrupt stack area is NOT static since it's
 * referenced from a BSPs crt0.s module when setting up the temporary
 * stack used during the execution of kernel initialization sequence, i.e.
 * up until the first context switch.  The dual-purposing of this area of
 * memory is safe since interrupts are disabled until the first context
 * switch.
 *
 * The NO_ISRS option is only supported with the 'lxPentium4' BSP, but
 * this BSP doesn't need to setup the aforementioned temporary stack.
 */

char __noinit _interrupt_stack[CONFIG_ISR_STACK_SIZE];
#endif

/*
 * entry point for background task in a nanokernel-only system,
 * or the idle task in a microkernel system
 */

extern void main(void);

/* hardware initialization routine provided by BSP */

extern void _InitHardware(void);

/* constructor initialization */

extern void _Ctors(void);

/*******************************************************************************
*
* _nano_init - Initializes the nanokernel layer
*
* This function is invoked from a BSP's initialization routine, which is in
* turn invoked by crt0.s.  The following is a summary of the early nanokernel
* initialization sequence:
*
*   crt0.s  -> _Cstart() -> _nano_init()
*                        -> _nano_fiber_swap()
*
*   main () -> kernel_init () -> task_fiber_start(... K_swapper ...)
*
* The _nano_init() routine initializes a context for the main() routine
* (aka background context which is a task context)), and sets _nanokernel.task
* to the 'tCCS *' for the new context.  The _nanokernel.current field is set to
* the provided <dummyOutContext> tCCS, however _nanokernel.fiber is set to
* NULL.
*
* Thus the subsequent invocation of _nano_fiber_swap() depicted above results
* in a context switch into the main() routine.  The <dummyOutContext> will
* be used as the context save area for the swap.  Typically, <dummyOutContext>
* is a temp area on the current stack (as setup by crt0.s).
*
* RETURNS: N/A
*
* \NOMANUAL
*/

void _nano_init(tCCS *dummyOutContext, int argc, char *argv[], char *envp[])
{
	/*
	 * Setup enough information re: the current execution context to permit
	 * a level of debugging output if an exception should happen to occur
	 * during _IntLibInit() or _AppContextInit(), for example. However,
	 * don't
	 * waste effort initializing the fields of the dummy context beyond
	 * those
	 * needed to identify it as a dummy context.
	 */

	_nanokernel.current = dummyOutContext;

	dummyOutContext->link =
		(tCCS *)NULL; /* context not inserted into list */
	dummyOutContext->flags = FIBER | ESSENTIAL;
	dummyOutContext->prio = 0;


#ifndef CONFIG_NO_ISRS
	/*
	 * The interrupt library needs to be initialized early since a series of
	 * handlers are installed into the interrupt table to catch spurious
	 * interrupts.  This must be performed before other nanokernel
	 * subsystems
	 * install bonafide handlers, or before hardware device drivers are
	 * initialized (in the BSPs' _InitHardware).
	 */

	_IntLibInit();
#endif

	/*
	 * Initialize the context control block (CCS) for the main (aka
	 * background)
	 * context.  The entry point for the background context is hardcoded as
	 * 'main'.
	 */

	_nanokernel.task =
		_NewContext(_k_init_and_idle_task_stack,			 /* pStackMem */
			    CONFIG_MAIN_STACK_SIZE, /* stackSize */
			    (_ContextEntry)main,	 /* pEntry */
			    (_ContextArg)argc,		 /* parameter1 */
			    (_ContextArg)argv,		 /* parameter2 */
			    (_ContextArg)envp,		 /* parameter3 */
			    -1,				 /* priority */
			    0				 /* options */
			    );

	/* indicate that failure of this task may be fatal to the entire system
	 */

	_nanokernel.task->flags |= ESSENTIAL;

#if defined(CONFIG_MICROKERNEL)
	/* fill in microkernel's TCB, which is the last element in _k_task_list[]
	 */

	_k_task_list[_k_task_count].workspace = (char *)_nanokernel.task;
	_k_task_list[_k_task_count].worksize = CONFIG_MAIN_STACK_SIZE;
#endif

	/*
	 * Initialize the nanokernel (tNANO) structure specifying the dummy
	 * context
	 * as the currently executing fiber context.
	 */

	_nanokernel.fiber = NULL;
#ifdef CONFIG_FP_SHARING
	_nanokernel.current_fp = NULL;
#endif /* CONFIG_FP_SHARING */

	nanoArchInit();
}

#ifdef CONFIG_STACK_CANARIES
/*******************************************************************************
 *
 * STACK_CANARY_INIT - initialize the kernel's stack canary
 *
 * This macro, only called from the BSP's _Cstart() routine, is used to
 * initialize the kernel's stack canary.  Both of the supported Intel and GNU
 * compilers currently support the stack canary global variable
 * <__stack_chk_guard>.  However, as this might not hold true for all future
 * releases, the initialization of the kernel stack canary has been abstracted
 * out for maintenance and backwards compatibility reasons.
 *
 * INTERNAL
 * Modifying __stack_chk_guard directly at runtime generates a build error
 * with ICC 13.0.2 20121114 on Windows 7. In-line assembly is used as a
 * workaround.
 */

extern void *__stack_chk_guard;

#if defined(VXMICRO_ARCH_x86)
#define _MOVE_INSTR "movl "
#elif defined(VXMICRO_ARCH_arm)
#define _MOVE_INSTR "str "
#else
#error "Unknown VXMICRO_ARCH type"
#endif /* VXMICRO_ARCH */

#define STACK_CANARY_INIT()                                \
	do {                                               \
		register void *tmp;                        \
		_Rand32Init();                             \
		tmp = (void *)_Rand32Get();                \
		__asm__ volatile(_MOVE_INSTR "%1, %0;\n\t" \
				 : "=m"(__stack_chk_guard) \
				 : "r"(tmp));              \
	} while (0)

#else /* !CONFIG_STACK_CANARIES */
#define STACK_CANARY_INIT()
#endif /* CONFIG_STACK_CANARIES */

/*******************************************************************************
*
* _Cstart - initialize nanokernel
*
* This routine is invoked by the BSP when the system is ready to run C code.
* The processor must be running in 32-bit mode, and the BSS must have been
* cleared/zeroed.
*
* RETURNS: Does not return
*/

FUNC_NORETURN void _Cstart(void)
{
	/* floating point operations are NOT performed during nanokernel init */

	char dummyCCS[__tCCS_NOFLOAT_SIZEOF];

	/*
	 * Initialize the nanokernel.  This step includes initializing the
	 * interrupt subsystem, which must be performed before the
	 * hardware initialization phase (by _InitHardware).
	 *
	 * For the time being don't pass any arguments to the nanokernel.
	 */

	_nano_init((tCCS *)&dummyCCS, 0, (char **)0, (char **)0);

	/* perform basic hardware initialization */

	_InitHardware();

	STACK_CANARY_INIT();

	/* invoke C++ constructors */
	_Ctors();

	/* display boot banner */

	PRINT_BOOT_BANNER();

	/* context switch into background context (entry function is main()) */

	_nano_fiber_swap();

	/*
	 * Compiler can't tell that the above routines won't return and issues
	 * a warning unless we explicitly tell it that control never gets this
	 * far.
	 */

	CODE_UNREACHABLE;
}
