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

/**
 * @file
 * @brief Nanokernel initialization module
 *
 * This module contains routines that are used to initialize the nanokernel.
 */

#include <zephyr.h>
#include <offsets_short.h>
#include <kernel.h>
#include <misc/printk.h>
#include <drivers/rand32.h>
#include <sections.h>
#include <toolchain.h>
#include <kernel_structs.h>
#include <device.h>
#include <init.h>
#include <linker-defs.h>
#include <ksched.h>

/* kernel build timestamp items */

#define BUILD_TIMESTAMP "BUILD: " __DATE__ " " __TIME__

#ifdef CONFIG_BUILD_TIMESTAMP
const char * const build_timestamp = BUILD_TIMESTAMP;
#endif

/* boot banner items */

#define BOOT_BANNER "BOOTING ZEPHYR OS"

#if !defined(CONFIG_BOOT_BANNER)
#define PRINT_BOOT_BANNER() do { } while (0)
#elif !defined(CONFIG_BUILD_TIMESTAMP)
#define PRINT_BOOT_BANNER() printk("***** " BOOT_BANNER " *****\n")
#else
#define PRINT_BOOT_BANNER() \
	printk("***** " BOOT_BANNER " - %s *****\n", build_timestamp)
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

/* init/main and idle threads */

#define IDLE_STACK_SIZE CONFIG_IDLE_STACK_SIZE

#if CONFIG_MAIN_STACK_SIZE & (STACK_ALIGN - 1)
    #error "MAIN_STACK_SIZE must be a multiple of the stack alignment"
#endif

#if IDLE_STACK_SIZE & (STACK_ALIGN - 1)
    #error "IDLE_STACK_SIZE must be a multiple of the stack alignment"
#endif

/* Some projects may specify their main thread and parameters in the
 * MDEF file. In this case, we need to use the stack size specified there
 * and not in Kconfig
 */
#if defined(MDEF_MAIN_STACK_SIZE) && \
		(MDEF_MAIN_STACK_SIZE > CONFIG_MAIN_STACK_SIZE)
#define MAIN_STACK_SIZE MDEF_MAIN_STACK_SIZE
#else
#define MAIN_STACK_SIZE CONFIG_MAIN_STACK_SIZE
#endif

static char __noinit __stack main_stack[MAIN_STACK_SIZE];
static char __noinit __stack idle_stack[IDLE_STACK_SIZE];

k_tid_t const _main_thread = (k_tid_t)main_stack;
k_tid_t const _idle_thread = (k_tid_t)idle_stack;

/*
 * storage space for the interrupt stack
 *
 * Note: This area is used as the system stack during nanokernel initialization,
 * since the nanokernel hasn't yet set up its own stack areas. The dual
 * purposing of this area is safe since interrupts are disabled until the
 * nanokernel context switches to the background (or idle) task.
 */
#if CONFIG_ISR_STACK_SIZE & (STACK_ALIGN - 1)
    #error "ISR_STACK_SIZE must be a multiple of the stack alignment"
#endif
char __noinit __stack _interrupt_stack[CONFIG_ISR_STACK_SIZE];

#ifdef CONFIG_SYS_CLOCK_EXISTS
	#include <misc/dlist.h>
	#define initialize_timeouts() do { \
		sys_dlist_init(&_timeout_q); \
	} while ((0))
#else
	#define initialize_timeouts() do { } while ((0))
#endif

extern void idle(void *unused1, void *unused2, void *unused3);

/**
 *
 * @brief Clear BSS
 *
 * This routine clears the BSS region, so all bytes are 0.
 *
 * @return N/A
 */

void _bss_zero(void)
{
	uint32_t *pos = (uint32_t *)&__bss_start;

	for ( ; pos < (uint32_t *)&__bss_end; pos++) {
		*pos = 0;
	}
}


#ifdef CONFIG_XIP
/**
 *
 * @brief Copy the data section from ROM to RAM
 *
 * This routine copies the data section from ROM to RAM.
 *
 * @return N/A
 */
void _data_copy(void)
{
	uint32_t *pROM, *pRAM;

	pROM = (uint32_t *)&__data_rom_start;
	pRAM = (uint32_t *)&__data_ram_start;

	for ( ; pRAM < (uint32_t *)&__data_ram_end; pROM++, pRAM++) {
		*pRAM = *pROM;
	}
}
#endif

/**
 *
 * @brief Mainline for nanokernel's background task
 *
 * This routine completes kernel initialization by invoking the remaining
 * init functions, then invokes application's main() routine.
 *
 * @return N/A
 */
static void _main(void *unused1, void *unused2, void *unused3)
{
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	ARG_UNUSED(unused3);

	_sys_device_do_config_level(_SYS_INIT_LEVEL_POST_KERNEL);

	/* These 3 are deprecated */
	_sys_device_do_config_level(_SYS_INIT_LEVEL_SECONDARY);
	_sys_device_do_config_level(_SYS_INIT_LEVEL_NANOKERNEL);
	_sys_device_do_config_level(_SYS_INIT_LEVEL_MICROKERNEL);

	/* Final init level before app starts */
	_sys_device_do_config_level(_SYS_INIT_LEVEL_APPLICATION);

#ifdef CONFIG_CPLUSPLUS
	/* Process the .ctors and .init_array sections */
	extern void __do_global_ctors_aux(void);
	extern void __do_init_array_aux(void);
	__do_global_ctors_aux();
	__do_init_array_aux();
#endif

	_init_static_threads();

#ifdef CONFIG_BOOT_TIME_MEASUREMENT
	/* record timestamp for kernel's _main() function */
	extern uint64_t __main_tsc;

	__main_tsc = _tsc_read();
#endif

	extern void main(void);

	/* If we're going to load the MDEF main() in this context, we need
	 * to now set the priority to be what was specified in the MDEF file
	 */
#if defined(MDEF_MAIN_THREAD_PRIORITY) && \
		(MDEF_MAIN_THREAD_PRIORITY != CONFIG_MAIN_THREAD_PRIORITY)
	k_thread_priority_set(_main_thread, MDEF_MAIN_THREAD_PRIORITY);
#endif
	main();

	/* Terminate thread normally since it has no more work to do */
	_main_thread->base.flags &= ~K_ESSENTIAL;
}

void __weak main(void)
{
	/* NOP default main() if the application does not provide one. */
}

/**
 *
 * @brief Initializes nanokernel data structures
 *
 * This routine initializes various nanokernel data structures, including
 * the background (or idle) task and any architecture-specific initialization.
 *
 * Note that all fields of "_kernel" are set to zero on entry, which may
 * be all the initialization many of them require.
 *
 * @return N/A
 */
static void prepare_multithreading(struct k_thread *dummy_thread)
{
	/*
	 * Initialize the current execution thread to permit a level of
	 * debugging output if an exception should happen during nanokernel
	 * initialization.  However, don't waste effort initializing the
	 * fields of the dummy thread beyond those needed to identify it as a
	 * dummy thread.
	 */

	_current = dummy_thread;

	/*
	 * Do not insert dummy execution context in the list of fibers, so
	 * that it does not get scheduled back in once context-switched out.
	 */
	dummy_thread->base.flags = K_ESSENTIAL;
	dummy_thread->base.prio = K_PRIO_COOP(0);

	/* _kernel.ready_q is all zeroes */


	/*
	 * The interrupt library needs to be initialized early since a series
	 * of handlers are installed into the interrupt table to catch
	 * spurious interrupts. This must be performed before other nanokernel
	 * subsystems install bonafide handlers, or before hardware device
	 * drivers are initialized.
	 */

	_IntLibInit();

	/* ready the init/main and idle threads */

	for (int ii = 0; ii < K_NUM_PRIORITIES; ii++) {
		sys_dlist_init(&_ready_q.q[ii]);
	}

	_new_thread(main_stack, MAIN_STACK_SIZE, NULL,
		    _main, NULL, NULL, NULL,
		    CONFIG_MAIN_THREAD_PRIORITY, K_ESSENTIAL);
	_mark_thread_as_started(_main_thread);
	_add_thread_to_ready_q(_main_thread);

	_new_thread(idle_stack, IDLE_STACK_SIZE, NULL,
		    idle, NULL, NULL, NULL,
		    K_LOWEST_THREAD_PRIO, K_ESSENTIAL);
	_mark_thread_as_started(_idle_thread);
	_add_thread_to_ready_q(_idle_thread);

	initialize_timeouts();

	/* perform any architecture-specific initialization */

	nanoArchInit();
}

static void switch_to_main_thread(void)
{
	/*
	 * Context switch to main task (entry function is _main()): the
	 * current fake thread is not on a wait queue or ready queue, so it
	 * will never be rescheduled in.
	 */

	_Swap(irq_lock());
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

#if defined(CONFIG_X86)
#define _MOVE_INSTR "movl "
#elif defined(CONFIG_ARM)
#define _MOVE_INSTR "str "
#elif defined(CONFIG_ARC)
#define _MOVE_INSTR "st "
#else
#error "Unknown Architecture type"
#endif /* CONFIG_X86 */

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

	char __stack dummy_thread[_K_THREAD_NO_FLOAT_SIZEOF];

	/*
	 * Initialize nanokernel data structures. This step includes
	 * initializing the interrupt subsystem, which must be performed
	 * before the hardware initialization phase.
	 */

	prepare_multithreading((struct k_thread *)&dummy_thread);

	/* Deprecated */
	_sys_device_do_config_level(_SYS_INIT_LEVEL_PRIMARY);

	/* perform basic hardware initialization */
	_sys_device_do_config_level(_SYS_INIT_LEVEL_PRE_KERNEL_1);
	_sys_device_do_config_level(_SYS_INIT_LEVEL_PRE_KERNEL_2);

	/*
	 * Initialize random number generator
	 * As a platform may implement it in hardware, it has to be
	 * initialized after rest of hardware initialization and
	 * before stack canaries that use it
	 */

	RAND32_INIT();

	/* initialize stack canaries */

	STACK_CANARY_INIT();

	/* display boot banner */

	PRINT_BOOT_BANNER();

	switch_to_main_thread();

	/*
	 * Compiler can't tell that the above routines won't return and issues
	 * a warning unless we explicitly tell it that control never gets this
	 * far.
	 */

	CODE_UNREACHABLE;
}
