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
#include <random/rand32.h>
#include <linker/sections.h>
#include <toolchain.h>
#include <kernel_structs.h>
#include <device.h>
#include <init.h>
#include <linker/linker-defs.h>
#include <ksched.h>
#include <version.h>
#include <string.h>
#include <misc/dlist.h>
#include <kernel_internal.h>
#include <kswap.h>

/* kernel build timestamp items */
#define BUILD_TIMESTAMP "BUILD: " __DATE__ " " __TIME__

/* boot banner items */
#if defined(CONFIG_BOOT_DELAY) && CONFIG_BOOT_DELAY > 0
#define BOOT_DELAY_BANNER " (delayed boot "	\
	STRINGIFY(CONFIG_BOOT_DELAY) "ms)"
static const unsigned int boot_delay = CONFIG_BOOT_DELAY;
#else
#define BOOT_DELAY_BANNER ""
static const unsigned int boot_delay;
#endif

#ifdef BUILD_VERSION
#define BOOT_BANNER "Booting Zephyr OS "	\
	 STRINGIFY(BUILD_VERSION) BOOT_DELAY_BANNER
#else
#define BOOT_BANNER "Booting Zephyr OS "	\
	 KERNEL_VERSION_STRING BOOT_DELAY_BANNER
#endif

#if !defined(CONFIG_BOOT_BANNER)
#define PRINT_BOOT_BANNER() do { } while (0)
#else
#define PRINT_BOOT_BANNER() printk("***** " BOOT_BANNER " *****\n")
#endif

/* boot time measurement items */

#ifdef CONFIG_BOOT_TIME_MEASUREMENT
u64_t __noinit __start_time_stamp; /* timestamp when kernel starts */
u64_t __noinit __main_time_stamp;  /* timestamp when main task starts */
u64_t __noinit __idle_time_stamp;  /* timestamp when CPU goes idle */
#endif

/* init/main and idle threads */

#define IDLE_STACK_SIZE CONFIG_IDLE_STACK_SIZE

#if CONFIG_MAIN_STACK_SIZE & (STACK_ALIGN - 1)
    #error "MAIN_STACK_SIZE must be a multiple of the stack alignment"
#endif

#if IDLE_STACK_SIZE & (STACK_ALIGN - 1)
    #error "IDLE_STACK_SIZE must be a multiple of the stack alignment"
#endif

#define MAIN_STACK_SIZE CONFIG_MAIN_STACK_SIZE

K_THREAD_STACK_DEFINE(_main_stack, MAIN_STACK_SIZE);
K_THREAD_STACK_DEFINE(_idle_stack, IDLE_STACK_SIZE);

static struct k_thread _main_thread_s;
static struct k_thread _idle_thread_s;

k_tid_t const _main_thread = (k_tid_t)&_main_thread_s;
k_tid_t const _idle_thread = (k_tid_t)&_idle_thread_s;

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
K_THREAD_STACK_DEFINE(_interrupt_stack, CONFIG_ISR_STACK_SIZE);

/*
 * Similar idle thread & interrupt stack definitions for the
 * auxiliary CPUs.  The declaration macros aren't set up to define an
 * array, so do it with a simple test for up to 4 processors.  Should
 * clean this up in the future.
 */
#if defined(CONFIG_SMP) && CONFIG_MP_NUM_CPUS > 1
K_THREAD_STACK_DEFINE(_idle_stack1, IDLE_STACK_SIZE);
static struct k_thread _idle_thread1_s;
k_tid_t const _idle_thread1 = (k_tid_t)&_idle_thread1_s;
K_THREAD_STACK_DEFINE(_interrupt_stack1, CONFIG_ISR_STACK_SIZE);
#endif

#if defined(CONFIG_SMP) && CONFIG_MP_NUM_CPUS > 2
K_THREAD_STACK_DEFINE(_idle_stack2, IDLE_STACK_SIZE);
static struct k_thread _idle_thread2_s;
k_tid_t const _idle_thread2 = (k_tid_t)&_idle_thread2_s;
K_THREAD_STACK_DEFINE(_interrupt_stack2, CONFIG_ISR_STACK_SIZE);
#endif

#if defined(CONFIG_SMP) && CONFIG_MP_NUM_CPUS > 3
K_THREAD_STACK_DEFINE(_idle_stack3, IDLE_STACK_SIZE);
static struct k_thread _idle_thread3_s;
k_tid_t const _idle_thread3 = (k_tid_t)&_idle_thread3_s;
K_THREAD_STACK_DEFINE(_interrupt_stack3, CONFIG_ISR_STACK_SIZE);
#endif

#ifdef CONFIG_SYS_CLOCK_EXISTS
	#define initialize_timeouts() do { \
		sys_dlist_init(&_timeout_q); \
	} while ((0))
#else
	#define initialize_timeouts() do { } while ((0))
#endif

extern void idle(void *unused1, void *unused2, void *unused3);

#if defined(CONFIG_INIT_STACKS) && defined(CONFIG_PRINTK)
extern K_THREAD_STACK_DEFINE(sys_work_q_stack,
			     CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE);


void k_call_stacks_analyze(void)
{
	printk("Kernel stacks:\n");
	STACK_ANALYZE("main     ", _main_stack);
	STACK_ANALYZE("idle     ", _idle_stack);
	STACK_ANALYZE("interrupt", _interrupt_stack);
	STACK_ANALYZE("workqueue", sys_work_q_stack);
}
#else
void k_call_stacks_analyze(void) { }
#endif

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
		 ((u32_t) &__bss_end - (u32_t) &__bss_start));
#ifdef CONFIG_CCM_BASE_ADDRESS
	memset(&__ccm_bss_start, 0,
		((u32_t) &__ccm_bss_end - (u32_t) &__ccm_bss_start));
#endif
#ifdef CONFIG_APPLICATION_MEMORY
	memset(&__app_bss_start, 0,
		 ((u32_t) &__app_bss_end - (u32_t) &__app_bss_start));
#endif
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
		 ((u32_t) &__data_ram_end - (u32_t) &__data_ram_start));
#ifdef CONFIG_CCM_BASE_ADDRESS
	memcpy(&__ccm_data_start, &__ccm_data_rom_start,
		 ((u32_t) &__ccm_data_end - (u32_t) &__ccm_data_start));
#endif
#ifdef CONFIG_APPLICATION_MEMORY
	memcpy(&__app_data_ram_start, &__app_data_rom_start,
		 ((u32_t) &__app_data_ram_end - (u32_t) &__app_data_ram_start));
#endif
}
#endif

/**
 *
 * @brief Mainline for kernel's background thread
 *
 * This routine completes kernel initialization by invoking the remaining
 * init functions, then invokes application's main() routine.
 *
 * @return N/A
 */
static void bg_thread_main(void *unused1, void *unused2, void *unused3)
{
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	ARG_UNUSED(unused3);

	_sys_device_do_config_level(_SYS_INIT_LEVEL_POST_KERNEL);
	if (boot_delay > 0) {
		printk("***** delaying boot " STRINGIFY(CONFIG_BOOT_DELAY)
		       "ms (per build configuration) *****\n");
		k_busy_wait(CONFIG_BOOT_DELAY * USEC_PER_MSEC);
	}
	PRINT_BOOT_BANNER();

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

#ifdef CONFIG_SMP
	smp_init();
#endif

#ifdef CONFIG_BOOT_TIME_MEASUREMENT
	/* record timestamp for kernel's _main() function */
	extern u64_t __main_time_stamp;

	__main_time_stamp = (u64_t)k_cycle_get_32();
#endif

	extern void main(void);

	main();

	/* Terminate thread normally since it has no more work to do */
	_main_thread->base.user_options &= ~K_ESSENTIAL;
}

void __weak main(void)
{
	/* NOP default main() if the application does not provide one. */
}

#if defined(CONFIG_MULTITHREADING)
static void init_idle_thread(struct k_thread *thr, k_thread_stack_t *stack)
{
#ifdef CONFIG_SMP
	thr->base.is_idle = 1;
#endif

	_setup_new_thread(thr, stack,
			  IDLE_STACK_SIZE, idle, NULL, NULL, NULL,
			  K_LOWEST_THREAD_PRIO, K_ESSENTIAL);
	_mark_thread_as_started(thr);
}
#endif

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
	dummy_thread->base.thread_state = _THREAD_DUMMY;
#ifdef CONFIG_THREAD_STACK_INFO
	dummy_thread->stack_info.start = 0;
	dummy_thread->stack_info.size = 0;
#endif
#ifdef CONFIG_USERSPACE
	dummy_thread->mem_domain_info.mem_domain = 0;
#endif
#endif

	/* _kernel.ready_q is all zeroes */
	_sched_init();

#ifndef CONFIG_SMP
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
#endif

	_setup_new_thread(_main_thread, _main_stack,
			  MAIN_STACK_SIZE, bg_thread_main,
			  NULL, NULL, NULL,
			  CONFIG_MAIN_THREAD_PRIORITY, K_ESSENTIAL);
	_mark_thread_as_started(_main_thread);
	_ready_thread(_main_thread);

#ifdef CONFIG_MULTITHREADING
	init_idle_thread(_idle_thread, _idle_stack);
	_kernel.cpus[0].idle_thread = _idle_thread;
#endif

#if defined(CONFIG_SMP) && CONFIG_MP_NUM_CPUS > 1
	init_idle_thread(_idle_thread1, _idle_stack1);
	_kernel.cpus[1].idle_thread = _idle_thread1;
	_kernel.cpus[1].id = 1;
	_kernel.cpus[1].irq_stack = K_THREAD_STACK_BUFFER(_interrupt_stack1)
		+ CONFIG_ISR_STACK_SIZE;
#endif

#if defined(CONFIG_SMP) && CONFIG_MP_NUM_CPUS > 2
	init_idle_thread(_idle_thread2, _idle_stack2);
	_kernel.cpus[2].idle_thread = _idle_thread2;
	_kernel.cpus[2].id = 2;
	_kernel.cpus[2].irq_stack = K_THREAD_STACK_BUFFER(_interrupt_stack2)
		+ CONFIG_ISR_STACK_SIZE;
#endif

#if defined(CONFIG_SMP) && CONFIG_MP_NUM_CPUS > 3
	init_idle_thread(_idle_thread3, _idle_stack3);
	_kernel.cpus[3].idle_thread = _idle_thread3;
	_kernel.cpus[3].id = 3;
	_kernel.cpus[3].irq_stack = K_THREAD_STACK_BUFFER(_interrupt_stack3)
		+ CONFIG_ISR_STACK_SIZE;
#endif

	initialize_timeouts();

	/* perform any architecture-specific initialization */

	kernel_arch_init();
}

static void switch_to_main_thread(void)
{
#ifdef CONFIG_ARCH_HAS_CUSTOM_SWAP_TO_MAIN
	_arch_switch_to_main_thread(_main_thread, _main_stack, MAIN_STACK_SIZE,
				    bg_thread_main);
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
extern void *__stack_chk_guard;
#endif

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
	struct k_thread *dummy_thread = NULL;
#else
	/* Normally, kernel objects are not allowed on the stack, special case
	 * here since this is just being used to bootstrap the first _Swap()
	 */
	char dummy_thread_memory[sizeof(struct k_thread)];
	struct k_thread *dummy_thread = (struct k_thread *)&dummy_thread_memory;

	memset(dummy_thread_memory, 0, sizeof(dummy_thread_memory));
#endif
	/*
	 * The interrupt library needs to be initialized early since a series
	 * of handlers are installed into the interrupt table to catch
	 * spurious interrupts. This must be performed before other kernel
	 * subsystems install bonafide handlers, or before hardware device
	 * drivers are initialized.
	 */

	_IntLibInit();

	/* perform basic hardware initialization */
	_sys_device_do_config_level(_SYS_INIT_LEVEL_PRE_KERNEL_1);
	_sys_device_do_config_level(_SYS_INIT_LEVEL_PRE_KERNEL_2);

	/* initialize stack canaries */
#ifdef CONFIG_STACK_CANARIES
	__stack_chk_guard = (void *)sys_rand32_get();
#endif
	prepare_multithreading(dummy_thread);

	/* display boot banner */

	switch_to_main_thread();

	/*
	 * Compiler can't tell that the above routines won't return and issues
	 * a warning unless we explicitly tell it that control never gets this
	 * far.
	 */

	CODE_UNREACHABLE;
}
