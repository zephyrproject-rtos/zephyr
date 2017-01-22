/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Kernel initialization module
 *
 * This module contains routines that are used to initialize the kernel.
 */

#include <zephyr.h>
#include <offsets_short.h>
#include <kernel.h>
#include <misc/printk.h>
#include <misc/stack.h>
#include <drivers/rand32.h>
#include <sections.h>
#include <toolchain.h>
#include <kernel_structs.h>
#include <device.h>
#include <init.h>
#include <linker-defs.h>
#include <ksched.h>
#include <version.h>
#include <string.h>

/* kernel build timestamp items */

#define BUILD_TIMESTAMP "BUILD: " __DATE__ " " __TIME__

#ifdef CONFIG_BUILD_TIMESTAMP
const char * const build_timestamp = BUILD_TIMESTAMP;
#endif

/* boot banner items */

#define BOOT_BANNER "BOOTING ZEPHYR OS v" KERNEL_VERSION_STRING

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

char __noinit __stack _main_stack[MAIN_STACK_SIZE];
char __noinit __stack _idle_stack[IDLE_STACK_SIZE];

k_tid_t const _main_thread = (k_tid_t)_main_stack;
k_tid_t const _idle_thread = (k_tid_t)_idle_stack;

/*
 * storage space for the interrupt stack
 *
 * Note: This area is used as the system stack during kernel initialization,
 * since the kernel hasn't yet set up its own stack areas. The dual purposing
 * of this area is safe since interrupts are disabled until the kernel context
 * switches to the init thread.
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

void k_call_stacks_analyze(void)
{
#if defined(CONFIG_INIT_STACKS) && defined(CONFIG_PRINTK)
	extern char sys_work_q_stack[CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE];
#if defined(CONFIG_ARC)
	extern char _firq_stack[CONFIG_FIRQ_STACK_SIZE];
#endif /* CONFIG_ARC */

	printk("Kernel stacks:\n");
	stack_analyze("main     ", _main_stack, sizeof(_main_stack));
	stack_analyze("idle     ", _idle_stack, sizeof(_idle_stack));
#if defined(CONFIG_ARC)
	stack_analyze("firq     ", _firq_stack, sizeof(_firq_stack));
#endif /* CONFIG_ARC */
	stack_analyze("interrupt", _interrupt_stack,
		      sizeof(_interrupt_stack));
	stack_analyze("workqueue", sys_work_q_stack,
		      sizeof(sys_work_q_stack));

#endif /* CONFIG_INIT_STACKS && CONFIG_PRINTK */
}

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
	memset(&__bss_start, 0,
		 ((uint32_t) &__bss_end - (uint32_t) &__bss_start));
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
	memcpy(&__data_ram_start, &__data_rom_start,
		 ((uint32_t) &__data_ram_end - (uint32_t) &__data_ram_start));
}
#endif

/**
 *
 * @brief Mainline for kernel's background task
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
	_main_thread->base.user_options &= ~K_ESSENTIAL;
}

void __weak main(void)
{
	/* NOP default main() if the application does not provide one. */
}

/**
 *
 * @brief Initializes kernel data structures
 *
 * This routine initializes various kernel data structures, including
 * the init and idle threads and any architecture-specific initialization.
 *
 * Note that all fields of "_kernel" are set to zero on entry, which may
 * be all the initialization many of them require.
 *
 * @return N/A
 */
static void prepare_multithreading(struct k_thread *dummy_thread)
{
#ifdef CONFIG_ARCH_HAS_CUSTOM_SWAP_TO_MAIN
	ARG_UNUSED(dummy_thread);
#else
	/*
	 * Initialize the current execution thread to permit a level of
	 * debugging output if an exception should happen during kernel
	 * initialization.  However, don't waste effort initializing the
	 * fields of the dummy thread beyond those needed to identify it as a
	 * dummy thread.
	 */

	_current = dummy_thread;

	dummy_thread->base.user_options = K_ESSENTIAL;
#endif

	/* _kernel.ready_q is all zeroes */


	/*
	 * The interrupt library needs to be initialized early since a series
	 * of handlers are installed into the interrupt table to catch
	 * spurious interrupts. This must be performed before other kernel
	 * subsystems install bonafide handlers, or before hardware device
	 * drivers are initialized.
	 */

	_IntLibInit();

	/* ready the init/main and idle threads */

	for (int ii = 0; ii < K_NUM_PRIORITIES; ii++) {
		sys_dlist_init(&_ready_q.q[ii]);
	}

	/*
	 * prime the cache with the main thread since:
	 *
	 * - the cache can never be NULL
	 * - the main thread will be the one to run first
	 * - no other thread is initialized yet and thus their priority fields
	 *   contain garbage, which would prevent the cache loading algorithm
	 *   to work as intended
	 */
	_ready_q.cache = _main_thread;

	_new_thread(_main_stack, MAIN_STACK_SIZE,
		    _main, NULL, NULL, NULL,
		    CONFIG_MAIN_THREAD_PRIORITY, K_ESSENTIAL);
	_mark_thread_as_started(_main_thread);
	_add_thread_to_ready_q(_main_thread);

#ifdef CONFIG_MULTITHREADING
	_new_thread(_idle_stack, IDLE_STACK_SIZE,
		    idle, NULL, NULL, NULL,
		    K_LOWEST_THREAD_PRIO, K_ESSENTIAL);
	_mark_thread_as_started(_idle_thread);
	_add_thread_to_ready_q(_idle_thread);
#endif

	initialize_timeouts();

	/* perform any architecture-specific initialization */

	nanoArchInit();
}

static void switch_to_main_thread(void)
{
#ifdef CONFIG_ARCH_HAS_CUSTOM_SWAP_TO_MAIN
	_arch_switch_to_main_thread(_main_stack, MAIN_STACK_SIZE, _main);
#else
	/*
	 * Context switch to main task (entry function is _main()): the
	 * current fake thread is not on a wait queue or ready queue, so it
	 * will never be rescheduled in.
	 */

	_Swap(irq_lock());
#endif
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
#define _MOVE_INSTR "movl %1, %0"
#define _MOVE_MEM "=m"
#elif defined(CONFIG_ARM)
#define _MOVE_INSTR "str %1, %0"
#define _MOVE_MEM "=m"
#elif defined(CONFIG_ARC)
#define _MOVE_INSTR "st %1, %0"
#define _MOVE_MEM "=m"
#elif defined(CONFIG_RISCV32)
#define _MOVE_INSTR "sw %1, 0x00(%0)"
#define _MOVE_MEM "=r"
#else
#error "Unknown Architecture type"
#endif /* CONFIG_X86 */

#define STACK_CANARY_INIT()                                \
	do {                                               \
		register void *tmp;                        \
		tmp = (void *)sys_rand32_get();            \
		__asm__ volatile(_MOVE_INSTR ";\n\t" \
				 : _MOVE_MEM(__stack_chk_guard) \
				 : "r"(tmp));              \
	} while (0)

#else /* !CONFIG_STACK_CANARIES */
#define STACK_CANARY_INIT()
#endif /* CONFIG_STACK_CANARIES */

/**
 *
 * @brief Initialize kernel
 *
 * This routine is invoked when the system is ready to run C code. The
 * processor must be running in 32-bit mode, and the BSS must have been
 * cleared/zeroed.
 *
 * @return Does not return
 */
FUNC_NORETURN void _Cstart(void)
{
#ifdef CONFIG_ARCH_HAS_CUSTOM_SWAP_TO_MAIN
	void *dummy_thread = NULL;
#else
	/* floating point is NOT used during kernel init */

	char __stack dummy_stack[_K_THREAD_NO_FLOAT_SIZEOF];
	void *dummy_thread = dummy_stack;
#endif

	/*
	 * Initialize kernel data structures. This step includes
	 * initializing the interrupt subsystem, which must be performed
	 * before the hardware initialization phase.
	 */

	prepare_multithreading(dummy_thread);

	/* Deprecated */
	_sys_device_do_config_level(_SYS_INIT_LEVEL_PRIMARY);

	/* perform basic hardware initialization */
	_sys_device_do_config_level(_SYS_INIT_LEVEL_PRE_KERNEL_1);
	_sys_device_do_config_level(_SYS_INIT_LEVEL_PRE_KERNEL_2);

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
