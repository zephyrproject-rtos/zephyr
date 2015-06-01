/* nanokernel initialization module */

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
#include <nanokernel.h>
#include <misc/printk.h>
#include <drivers/rand32.h>
#include <sections.h>
#include <toolchain.h>
#include <nanok.h>

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

/* random number generator items */
#if defined(CONFIG_TEST_RANDOM_GENERATOR) || \
	defined(CONFIG_CUSTOM_RANDOM_GENERATOR)
#define RAND32_INIT() sys_rand32_init()
#else
#define RAND32_INIT()
#endif

/* stack space for the background (or idle) task context */

char __noinit __stack main_task_stack[CONFIG_MAIN_STACK_SIZE];

/*
 * storage space for the interrupt stack
 *
 * Note: This area is used as the system stack during nanokernel initialization,
 * since the nanokernel hasn't yet set up its own stack areas. The dual
 * purposing of this area is safe since interrupts are disabled until the
 * nanokernel context switches to the background (or idle) task.
 */

#ifndef CONFIG_NO_ISRS
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
* nano_init - initializes nanokernel data structures
*
* This routine initializes various nanokernel data structures, including
* the background (or idle) task and any architecture-specific initialization.
*
* Note that all fields of "_nanokernel" are set to zero on entry, which may
* be all the initialization many of them require.
*
* RETURNS: N/A
*/

static void nano_init(tCCS *dummyOutContext)
{
	/*
	 * Initialize the current execution context to permit a level of debugging
	 * output if an exception should happen during nanokernel initialization.
	 * However, don't waste effort initializing the fields of the dummy context
	 * beyond those needed to identify it as a dummy context.
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
	 * interrupts. This must be performed before other nanokernel subsystems
	 * install bonafide handlers, or before hardware device drivers are
	 * initialized (in the BSPs' _InitHardware).
	 */

	_IntLibInit();
#endif

	/*
	 * Initialize the context control block (CCS) for the background task
	 * (or idle task). The entry point for this context is 'main'.
	 */

	_nanokernel.task = (tCCS *) main_task_stack;

	_NewContext(main_task_stack,	/* pStackMem */
			    CONFIG_MAIN_STACK_SIZE, /* stackSize */
			    (_ContextEntry)main,	 /* pEntry */
			    (_ContextArg)0,	 /* parameter1 */
			    (_ContextArg)0,	 /* parameter2 */
			    (_ContextArg)0,	 /* parameter3 */
			    -1,				 /* priority */
			    0				 /* options */
			    );

	/* indicate that failure of this task may be fatal to the entire system */

	_nanokernel.task->flags |= ESSENTIAL;

	/* perform any architecture-specific initialization */

	nanoArchInit();
}

#ifdef CONFIG_STACK_CANARIES
/*******************************************************************************
 *
 * STACK_CANARY_INIT - initialize the kernel's stack canary
 *
 * This macro initializes the kernel's stack canary global variable,
 * __stack_chk_guard, with a random value.
 *
 * INTERNAL
 * Depending upon the compiler, modifying __stack_chk_guard directly at runtime
 * may generate a build error.  In-line assembly is used as a workaround.
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
		tmp = (void *)sys_rand32_get();            \
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
	 * Initialize nanokernel data structures. This step includes
	 * initializing the interrupt subsystem, which must be performed
	 * before the hardware initialization phase.
	 */

	nano_init((tCCS *)&dummyCCS);

	/* perform basic hardware initialization */

	_InitHardware();

	/*
	 * Initialize random number generator
	 * As a platform may implement it in hardware, it has to be
	 * initialized after rest of hardware initialization and
	 * before stack canaries that use it
	 */

	RAND32_INIT();

	/* initialize stack canaries */

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
