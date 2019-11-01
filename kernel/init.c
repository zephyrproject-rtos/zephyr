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
#include <sys/printk.h>
#include <debug/stack.h>
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
#include <sys/dlist.h>
#include <kernel_internal.h>
#include <kswap.h>
#include <drivers/entropy.h>
#include <logging/log_ctrl.h>
#include <debug/tracing.h>
#include <stdbool.h>
#include <debug/gcov.h>

#define IDLE_THREAD_NAME	"idle"
#define LOG_LEVEL CONFIG_KERNEL_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(os);

/* boot banner items */
#if defined(CONFIG_MULTITHREADING) && defined(CONFIG_BOOT_DELAY) \
	&& CONFIG_BOOT_DELAY > 0
#define BOOT_DELAY_BANNER " (delayed boot "	\
	STRINGIFY(CONFIG_BOOT_DELAY) "ms)"
#else
#define BOOT_DELAY_BANNER ""
#endif

#ifdef BUILD_VERSION
#define BOOT_BANNER "Booting Zephyr OS build "		\
	 STRINGIFY(BUILD_VERSION) BOOT_DELAY_BANNER
#else
#define BOOT_BANNER "Booting Zephyr OS version "	\
	 KERNEL_VERSION_STRING BOOT_DELAY_BANNER
#endif

#if !defined(CONFIG_BOOT_BANNER)
#define PRINT_BOOT_BANNER() do { } while (false)
#else
#define PRINT_BOOT_BANNER() printk("***** " BOOT_BANNER " *****\n")
#endif

/* boot time measurement items */

#ifdef CONFIG_BOOT_TIME_MEASUREMENT
u32_t __noinit z_timestamp_main;  /* timestamp when main task starts */
u32_t __noinit z_timestamp_idle;  /* timestamp when CPU goes idle */
#endif

/* init/main and idle threads */
K_THREAD_STACK_DEFINE(z_main_stack, CONFIG_MAIN_STACK_SIZE);
K_THREAD_STACK_DEFINE(z_idle_stack, CONFIG_IDLE_STACK_SIZE);

struct k_thread z_main_thread;
struct k_thread z_idle_thread;

/*
 * storage space for the interrupt stack
 *
 * Note: This area is used as the system stack during kernel initialization,
 * since the kernel hasn't yet set up its own stack areas. The dual purposing
 * of this area is safe since interrupts are disabled until the kernel context
 * switches to the init thread.
 */
K_THREAD_STACK_DEFINE(_interrupt_stack, CONFIG_ISR_STACK_SIZE);

/*
 * Similar idle thread & interrupt stack definitions for the
 * auxiliary CPUs.  The declaration macros aren't set up to define an
 * array, so do it with a simple test for up to 4 processors.  Should
 * clean this up in the future.
 */
#if defined(CONFIG_SMP) && CONFIG_MP_NUM_CPUS > 1
K_THREAD_STACK_DEFINE(_idle_stack1, CONFIG_IDLE_STACK_SIZE);
static struct k_thread _idle_thread1_s;
k_tid_t const _idle_thread1 = (k_tid_t)&_idle_thread1_s;
K_THREAD_STACK_DEFINE(_interrupt_stack1, CONFIG_ISR_STACK_SIZE);
#endif

#if defined(CONFIG_SMP) && CONFIG_MP_NUM_CPUS > 2
K_THREAD_STACK_DEFINE(_idle_stack2, CONFIG_IDLE_STACK_SIZE);
static struct k_thread _idle_thread2_s;
k_tid_t const _idle_thread2 = (k_tid_t)&_idle_thread2_s;
K_THREAD_STACK_DEFINE(_interrupt_stack2, CONFIG_ISR_STACK_SIZE);
#endif

#if defined(CONFIG_SMP) && CONFIG_MP_NUM_CPUS > 3
K_THREAD_STACK_DEFINE(_idle_stack3, CONFIG_IDLE_STACK_SIZE);
static struct k_thread _idle_thread3_s;
k_tid_t const _idle_thread3 = (k_tid_t)&_idle_thread3_s;
K_THREAD_STACK_DEFINE(_interrupt_stack3, CONFIG_ISR_STACK_SIZE);
#endif

#ifdef CONFIG_SYS_CLOCK_EXISTS
	#define initialize_timeouts() do { \
		sys_dlist_init(&_timeout_q); \
	} while (false)
#else
	#define initialize_timeouts() do { } while ((0))
#endif

extern void idle(void *unused1, void *unused2, void *unused3);


/* LCOV_EXCL_START
 *
 * This code is called so early in the boot process that code coverage
 * doesn't work properly. In addition, not all arches call this code,
 * some like x86 do this with optimized assembly
 */

/**
 *
 * @brief Clear BSS
 *
 * This routine clears the BSS region, so all bytes are 0.
 *
 * @return N/A
 */
void z_bss_zero(void)
{
	(void)memset(__bss_start, 0, __bss_end - __bss_start);
#ifdef DT_CCM_BASE_ADDRESS
	(void)memset(&__ccm_bss_start, 0,
		     ((u32_t) &__ccm_bss_end - (u32_t) &__ccm_bss_start));
#endif
#ifdef DT_DTCM_BASE_ADDRESS
	(void)memset(&__dtcm_bss_start, 0,
		     ((u32_t) &__dtcm_bss_end - (u32_t) &__dtcm_bss_start));
#endif
#ifdef CONFIG_CODE_DATA_RELOCATION
	extern void bss_zeroing_relocation(void);

	bss_zeroing_relocation();
#endif	/* CONFIG_CODE_DATA_RELOCATION */
#ifdef CONFIG_COVERAGE_GCOV
	(void)memset(&__gcov_bss_start, 0,
		 ((u32_t) &__gcov_bss_end - (u32_t) &__gcov_bss_start));
#endif
}

#ifdef CONFIG_STACK_CANARIES
extern volatile uintptr_t __stack_chk_guard;
#endif /* CONFIG_STACK_CANARIES */


#ifdef CONFIG_XIP
/**
 *
 * @brief Copy the data section from ROM to RAM
 *
 * This routine copies the data section from ROM to RAM.
 *
 * @return N/A
 */
void z_data_copy(void)
{
	(void)memcpy(&__data_ram_start, &__data_rom_start,
		 __data_ram_end - __data_ram_start);
#ifdef CONFIG_ARCH_HAS_RAMFUNC_SUPPORT
	(void)memcpy(&_ramfunc_ram_start, &_ramfunc_rom_start,
		 (uintptr_t) &_ramfunc_ram_size);
#endif /* CONFIG_ARCH_HAS_RAMFUNC_SUPPORT */
#ifdef DT_CCM_BASE_ADDRESS
	(void)memcpy(&__ccm_data_start, &__ccm_data_rom_start,
		 __ccm_data_end - __ccm_data_start);
#endif
#ifdef DT_DTCM_BASE_ADDRESS
	(void)memcpy(&__dtcm_data_start, &__dtcm_data_rom_start,
		 __dtcm_data_end - __dtcm_data_start);
#endif
#ifdef CONFIG_CODE_DATA_RELOCATION
	extern void data_copy_xip_relocation(void);

	data_copy_xip_relocation();
#endif	/* CONFIG_CODE_DATA_RELOCATION */
#ifdef CONFIG_USERSPACE
#ifdef CONFIG_STACK_CANARIES
	/* stack canary checking is active for all C functions.
	 * __stack_chk_guard is some uninitialized value living in the
	 * app shared memory sections. Preserve it, and don't make any
	 * function calls to perform the memory copy. The true canary
	 * value gets set later in z_cstart().
	 */
	uintptr_t guard_copy = __stack_chk_guard;
	u8_t *src = (u8_t *)&_app_smem_rom_start;
	u8_t *dst = (u8_t *)&_app_smem_start;
	u32_t count = _app_smem_end - _app_smem_start;

	guard_copy = __stack_chk_guard;
	while (count > 0) {
		*(dst++) = *(src++);
		count--;
	}
	__stack_chk_guard = guard_copy;
#else
	(void)memcpy(&_app_smem_start, &_app_smem_rom_start,
		 _app_smem_end - _app_smem_start);
#endif /* CONFIG_STACK_CANARIES */
#endif /* CONFIG_USERSPACE */
}
#endif /* CONFIG_XIP */

/* LCOV_EXCL_STOP */

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

#if defined(CONFIG_BOOT_DELAY) && CONFIG_BOOT_DELAY > 0
	static const unsigned int boot_delay = CONFIG_BOOT_DELAY;
#else
	static const unsigned int boot_delay;
#endif

	z_sys_device_do_config_level(_SYS_INIT_LEVEL_POST_KERNEL);
#if CONFIG_STACK_POINTER_RANDOM
	z_stack_adjust_initialized = 1;
#endif
	if (boot_delay > 0 && IS_ENABLED(CONFIG_MULTITHREADING)) {
		printk("***** delaying boot " STRINGIFY(CONFIG_BOOT_DELAY)
		       "ms (per build configuration) *****\n");
		k_busy_wait(CONFIG_BOOT_DELAY * USEC_PER_MSEC);
	}
	PRINT_BOOT_BANNER();

	/* Final init level before app starts */
	z_sys_device_do_config_level(_SYS_INIT_LEVEL_APPLICATION);

#ifdef CONFIG_CPLUSPLUS
	/* Process the .ctors and .init_array sections */
	extern void __do_global_ctors_aux(void);
	extern void __do_init_array_aux(void);
	__do_global_ctors_aux();
	__do_init_array_aux();
#endif

	z_init_static_threads();

#ifdef CONFIG_SMP
	z_smp_init();
#endif

#ifdef CONFIG_BOOT_TIME_MEASUREMENT
	z_timestamp_main = k_cycle_get_32();
#endif

	extern void main(void);

	main();

	/* Mark nonessenrial since main() has no more work to do */
	z_main_thread.base.user_options &= ~K_ESSENTIAL;

	/* Dump coverage data once the main() has exited. */
	gcov_coverage_dump();
} /* LCOV_EXCL_LINE ... because we just dumped final coverage data */

/* LCOV_EXCL_START */

void __weak main(void)
{
	/* NOP default main() if the application does not provide one. */
	z_arch_nop();
}

/* LCOV_EXCL_STOP */

#if defined(CONFIG_MULTITHREADING)
static void init_idle_thread(struct k_thread *thr, k_thread_stack_t *stack)
{
	z_setup_new_thread(thr, stack,
			  CONFIG_IDLE_STACK_SIZE, idle, NULL, NULL, NULL,
			  K_LOWEST_THREAD_PRIO, K_ESSENTIAL, IDLE_THREAD_NAME);
	z_mark_thread_as_started(thr);

#ifdef CONFIG_SMP
	thr->base.is_idle = 1U;
#endif
}
#endif /* CONFIG_MULTITHREADING */

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
#ifdef CONFIG_MULTITHREADING
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
	dummy_thread->base.user_options = K_ESSENTIAL;
	dummy_thread->base.thread_state = _THREAD_DUMMY;
#ifdef CONFIG_THREAD_STACK_INFO
	dummy_thread->stack_info.start = 0U;
	dummy_thread->stack_info.size = 0U;
#endif
#ifdef CONFIG_USERSPACE
	dummy_thread->mem_domain_info.mem_domain = 0;
#endif
#endif /* CONFIG_ARCH_HAS_CUSTOM_SWAP_TO_MAIN */

	/* _kernel.ready_q is all zeroes */
	z_sched_init();

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
	_kernel.ready_q.cache = &z_main_thread;
#endif

	z_setup_new_thread(&z_main_thread, z_main_stack,
			   CONFIG_MAIN_STACK_SIZE, bg_thread_main,
			   NULL, NULL, NULL,
			   CONFIG_MAIN_THREAD_PRIORITY, K_ESSENTIAL, "main");
	sys_trace_thread_create(&z_main_thread);

	z_mark_thread_as_started(&z_main_thread);
	z_ready_thread(&z_main_thread);

	init_idle_thread(&z_idle_thread, z_idle_stack);
	_kernel.cpus[0].idle_thread = &z_idle_thread;
	sys_trace_thread_create(&z_idle_thread);

#if defined(CONFIG_SMP) && CONFIG_MP_NUM_CPUS > 1
	init_idle_thread(_idle_thread1, _idle_stack1);
	_kernel.cpus[1].idle_thread = _idle_thread1;
	_kernel.cpus[1].id = 1;
	_kernel.cpus[1].irq_stack = Z_THREAD_STACK_BUFFER(_interrupt_stack1)
		+ CONFIG_ISR_STACK_SIZE;
#endif

#if defined(CONFIG_SMP) && CONFIG_MP_NUM_CPUS > 2
	init_idle_thread(_idle_thread2, _idle_stack2);
	_kernel.cpus[2].idle_thread = _idle_thread2;
	_kernel.cpus[2].id = 2;
	_kernel.cpus[2].irq_stack = Z_THREAD_STACK_BUFFER(_interrupt_stack2)
		+ CONFIG_ISR_STACK_SIZE;
#endif

#if defined(CONFIG_SMP) && CONFIG_MP_NUM_CPUS > 3
	init_idle_thread(_idle_thread3, _idle_stack3);
	_kernel.cpus[3].idle_thread = _idle_thread3;
	_kernel.cpus[3].id = 3;
	_kernel.cpus[3].irq_stack = Z_THREAD_STACK_BUFFER(_interrupt_stack3)
		+ CONFIG_ISR_STACK_SIZE;
#endif

	initialize_timeouts();

}

static FUNC_NORETURN void switch_to_main_thread(void)
{
#ifdef CONFIG_ARCH_HAS_CUSTOM_SWAP_TO_MAIN
	z_arch_switch_to_main_thread(&z_main_thread, z_main_stack,
				     K_THREAD_STACK_SIZEOF(z_main_stack),
				     bg_thread_main);
#else
	/*
	 * Context switch to main task (entry function is _main()): the
	 * current fake thread is not on a wait queue or ready queue, so it
	 * will never be rescheduled in.
	 */
	z_swap_unlocked();
#endif
	CODE_UNREACHABLE; /* LCOV_EXCL_LINE */
}
#endif /* CONFIG_MULTITHREADING */

u32_t z_early_boot_rand32_get(void)
{
#ifdef CONFIG_ENTROPY_HAS_DRIVER
	struct device *entropy = device_get_binding(CONFIG_ENTROPY_NAME);
	int rc;
	u32_t retval;

	if (entropy == NULL) {
		goto sys_rand32_fallback;
	}

	/* Try to see if driver provides an ISR-specific API */
	rc = entropy_get_entropy_isr(entropy, (u8_t *)&retval,
				     sizeof(retval), ENTROPY_BUSYWAIT);
	if (rc == -ENOTSUP) {
		/* Driver does not provide an ISR-specific API, assume it can
		 * be called from ISR context
		 */
		rc = entropy_get_entropy(entropy, (u8_t *)&retval,
					 sizeof(retval));
	}

	if (rc >= 0) {
		return retval;
	}

	/* Fall through to fallback */

sys_rand32_fallback:
#endif

	/* FIXME: this assumes sys_rand32_get() won't use any synchronization
	 * primitive, like semaphores or mutexes.  It's too early in the boot
	 * process to use any of them.  Ideally, only the path where entropy
	 * devices are available should be built, this is only a fallback for
	 * those devices without a HWRNG entropy driver.
	 */
	return sys_rand32_get();
}

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
FUNC_NORETURN void z_cstart(void)
{
	/* gcov hook needed to get the coverage report.*/
	gcov_static_init();

	LOG_CORE_INIT();

	/* perform any architecture-specific initialization */
	z_arch_kernel_init();

#ifdef CONFIG_MULTITHREADING
	struct k_thread dummy_thread = {
		 .base.thread_state = _THREAD_DUMMY,
# ifdef CONFIG_SCHED_CPU_MASK
		 .base.cpu_mask = -1,
# endif
	};

	_current = &dummy_thread;
#endif

#ifdef CONFIG_USERSPACE
	z_app_shmem_bss_zero();
#endif

	/* perform basic hardware initialization */
	z_sys_device_do_config_level(_SYS_INIT_LEVEL_PRE_KERNEL_1);
	z_sys_device_do_config_level(_SYS_INIT_LEVEL_PRE_KERNEL_2);

#ifdef CONFIG_STACK_CANARIES
	__stack_chk_guard = z_early_boot_rand32_get();
#endif

#ifdef CONFIG_MULTITHREADING
	prepare_multithreading(&dummy_thread);
	switch_to_main_thread();
#else
	bg_thread_main(NULL, NULL, NULL);

	/* LCOV_EXCL_START
	 * We've already dumped coverage data at this point.
	 */
	irq_lock();
	while (true) {
	}
	/* LCOV_EXCL_STOP */
#endif

	/*
	 * Compiler can't tell that the above routines won't return and issues
	 * a warning unless we explicitly tell it that control never gets this
	 * far.
	 */

	CODE_UNREACHABLE; /* LCOV_EXCL_LINE */
}
