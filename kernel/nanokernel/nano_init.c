/* nanokernel initialization module */

/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
uint64_t __noinit __main_tsc;  /* timestamp when main task starts */
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

/* constructor initialization */

extern void _Ctors(void);

#ifdef CONFIG_NANO_TIMEOUTS
	#include <misc/dlist.h>
	#define initialize_nano_timeouts() sys_dlist_init(&_nanokernel.timeout_q)
#else
	#define initialize_nano_timeouts() do { } while ((0))
#endif

#ifdef CONFIG_NANOKERNEL
/**
 *
 * @brief Mainline for nanokernel's background task
 *
 * This routine completes kernel initialization by invoking the remaining
 * init functions, then invokes application's main() routine.
 *
 * @return N/A
 */

static void _main(void)
{
	_sys_device_do_config_level(_SYS_INIT_LEVEL_NANOKERNEL);
	_sys_device_do_config_level(_SYS_INIT_LEVEL_APPLICATION);

	extern void main(void);
	main();
}
#else
/* microkernel has its own implementation of _main() */

extern void _main(void);
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
	 * Initialize the thread control block (TCS) for the main task (either
	 * background or idle task). The entry point for this thread is '_main'.
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

	_sys_device_do_config_level(_SYS_INIT_LEVEL_PRIMARY);
	_sys_device_do_config_level(_SYS_INIT_LEVEL_SECONDARY);

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

	/* context switch to main task (entry function is _main()) */

	_nano_fiber_swap();

	/*
	 * Compiler can't tell that the above routines won't return and issues
	 * a warning unless we explicitly tell it that control never gets this
	 * far.
	 */

	CODE_UNREACHABLE;
}
