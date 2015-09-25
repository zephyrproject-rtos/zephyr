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
#include <nano_private.h>
#include <device.h>
#include <init.h>

/* kernel build timestamp items */

#define BUILD_TIMESTAMP "BUILD: " __DATE__ " " __TIME__

#ifdef CONFIG_BUILD_TIMESTAMP
const char * const build_timestamp = BUILD_TIMESTAMP;
#endif

/* boot banner items */

#define BOOT_BANNER "****** BOOTING ZEPHYR OS ******"

#if !defined(CONFIG_BOOT_BANNER)
#define PRINT_BOOT_BANNER() do { } while (0)
#elif !defined(CONFIG_BUILD_TIMESTAMP)
#define PRINT_BOOT_BANNER() printk(BOOT_BANNER "\n")
#else
#define PRINT_BOOT_BANNER() printk(BOOT_BANNER " %s\n", build_timestamp)
#endif

/* boot time measurement items */

#ifdef CONFIG_BOOT_TIME_MEASUREMENT
uint64_t __noinit __start_tsc; /* timestamp when kernel starts */
uint64_t __noinit __main_tsc;  /* timestamp when main() starts */
uint64_t __noinit __idle_tsc;  /* timestamp when CPU goes idle */
#endif

/* random number generator items */
#if defined(CONFIG_TEST_RANDOM_GENERATOR) || \
	defined(CONFIG_CUSTOM_RANDOM_GENERATOR)
#define RAND32_INIT() sys_rand32_init()
#else
#define RAND32_INIT()
#endif

/* stack space for the background (or idle) task */

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

/* constructor initialization */

extern void _Ctors(void);

#ifdef CONFIG_NANO_TIMEOUTS
	#include <misc/dlist.h>
	#define initialize_nano_timeouts() sys_dlist_init(&_nanokernel.timeout_q)
#else
	#define initialize_nano_timeouts() do { } while ((0))
#endif

/**
 *
 * In the nanokernel only configuration we still want to run the
 * app_{early,late}_init levels to maintain the correct semantics. In
 * a microkernel configuration these init levels are run in the
 * microkernel initialization.
 *
 */

#ifdef CONFIG_NANOKERNEL
static void _main(void)
{
	_sys_device_do_config_level(NANO_EARLY);
	_sys_device_do_config_level(NANO_LATE);
	_sys_device_do_config_level(APP_EARLY);
	_sys_device_do_config_level(APP_EARLY);
	main();
}
#else
static void _main(void)
{
	_sys_device_do_config_level(NANO_EARLY);
	_sys_device_do_config_level(NANO_LATE);
	main();
}

#endif

/**
 *
 * @brief Initializes nanokernel data structures
 *
 * This routine initializes various nanokernel data structures, including
 * the background (or idle) task and any architecture-specific initialization.
 *
 * Note that all fields of "_nanokernel" are set to zero on entry, which may
 * be all the initialization many of them require.
 *
 * @return N/A
 */

static void nano_init(struct tcs *dummyOutContext)
{
	/*
	 * Initialize the current execution thread to permit a level of debugging
	 * output if an exception should happen during nanokernel initialization.
	 * However, don't waste effort initializing the fields of the dummy thread
	 * beyond those needed to identify it as a dummy thread.
	 */

	_nanokernel.current = dummyOutContext;

	/*
	 * Do not insert dummy execution context in the list of fibers, so that it
	 * does not get scheduled back in once context-switched out.
	 */
	dummyOutContext->link = (struct tcs *)NULL;

	dummyOutContext->flags = FIBER | ESSENTIAL;
	dummyOutContext->prio = 0;


#ifndef CONFIG_NO_ISRS
	/*
	 * The interrupt library needs to be initialized early since a series of
	 * handlers are installed into the interrupt table to catch spurious
	 * interrupts. This must be performed before other nanokernel subsystems
	 * install bonafide handlers, or before hardware device drivers are
	 * initialized.
	 */

	_IntLibInit();
#endif

	/*
	 * Initialize the thread control block (TCS) for the background task
	 * (or idle task). The entry point for this thread is 'main'.
	 */

	_nanokernel.task = (struct tcs *) main_task_stack;

	_new_thread(main_task_stack,	/* pStackMem */
			    CONFIG_MAIN_STACK_SIZE, /* stackSize */
			    (_thread_entry_t)_main,	 /* pEntry */
			    (_thread_arg_t)0,	 /* parameter1 */
			    (_thread_arg_t)0,	 /* parameter2 */
			    (_thread_arg_t)0,	 /* parameter3 */
			    -1,				 /* priority */
			    0				 /* options */
			    );

	/* indicate that failure of this task may be fatal to the entire system */

	_nanokernel.task->flags |= ESSENTIAL;

	initialize_nano_timeouts();

	/* perform any architecture-specific initialization */

	nanoArchInit();
}

#ifdef CONFIG_STACK_CANARIES
/**
 *
 * @brief Initialize the kernel's stack canary
 *
 * This macro initializes the kernel's stack canary global variable,
 * __stack_chk_guard, with a random value.
 *
 * INTERNAL
 * Depending upon the compiler, modifying __stack_chk_guard directly at runtime
 * may generate a build error.  In-line assembly is used as a workaround.
 */

extern void *__stack_chk_guard;

#if defined(CONFIG_X86_32)
#define _MOVE_INSTR "movl "
#elif defined(CONFIG_ARM)
#define _MOVE_INSTR "str "
#else
#error "Unknown Architecture type"
#endif /* CONFIG_X86_32 */

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

/**
 *
 * @brief Initialize nanokernel
 *
 * This routine is invoked when the system is ready to run C code. The
 * processor must be running in 32-bit mode, and the BSS must have been
 * cleared/zeroed.
 *
 * @return Does not return
 */

FUNC_NORETURN void _Cstart(void)
{
	/* floating point operations are NOT performed during nanokernel init */

	char dummyTCS[__tTCS_NOFLOAT_SIZEOF];

	/*
	 * Initialize nanokernel data structures. This step includes
	 * initializing the interrupt subsystem, which must be performed
	 * before the hardware initialization phase.
	 */

	nano_init((struct tcs *)&dummyTCS);

	/* perform basic hardware initialization */

	_sys_device_do_config_level(PURE_CORE);
	_sys_device_do_config_level(PRE_KERNEL_EARLY);
	_sys_device_do_config_level(PRE_KERNEL_LATE);

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

	/* context switch into background thread (entry function is main()) */

	_nano_fiber_swap();

	/*
	 * Compiler can't tell that the above routines won't return and issues
	 * a warning unless we explicitly tell it that control never gets this
	 * far.
	 */

	CODE_UNREACHABLE;
}
