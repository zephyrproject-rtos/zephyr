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

#include <offsets_short.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/debug/stack.h>
#include <zephyr/random/rand32.h>
#include <zephyr/linker/sections.h>
#include <zephyr/toolchain.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/linker/linker-defs.h>
#include <ksched.h>
#include <string.h>
#include <zephyr/sys/dlist.h>
#include <kernel_internal.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/tracing/tracing.h>
#include <stdbool.h>
#include <zephyr/debug/gcov.h>
#include <kswap.h>
#include <zephyr/timing/timing.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(os, CONFIG_KERNEL_LOG_LEVEL);

/* the only struct z_kernel instance */
struct z_kernel _kernel;

/* init/main and idle threads */
K_THREAD_PINNED_STACK_DEFINE(z_main_stack, CONFIG_MAIN_STACK_SIZE);
struct k_thread z_main_thread;

#ifdef CONFIG_MULTITHREADING
__pinned_bss
struct k_thread z_idle_threads[CONFIG_MP_NUM_CPUS];

static K_KERNEL_PINNED_STACK_ARRAY_DEFINE(z_idle_stacks,
					  CONFIG_MP_NUM_CPUS,
					  CONFIG_IDLE_STACK_SIZE);
#endif /* CONFIG_MULTITHREADING */

/*
 * storage space for the interrupt stack
 *
 * Note: This area is used as the system stack during kernel initialization,
 * since the kernel hasn't yet set up its own stack areas. The dual purposing
 * of this area is safe since interrupts are disabled until the kernel context
 * switches to the init thread.
 */
K_KERNEL_PINNED_STACK_ARRAY_DEFINE(z_interrupt_stacks,
				   CONFIG_MP_NUM_CPUS,
				   CONFIG_ISR_STACK_SIZE);

extern void idle(void *unused1, void *unused2, void *unused3);


/* LCOV_EXCL_START
 *
 * This code is called so early in the boot process that code coverage
 * doesn't work properly. In addition, not all arches call this code,
 * some like x86 do this with optimized assembly
 */

/**
 * @brief equivalent of memset() for early boot usage
 *
 * Architectures that can't safely use the regular (optimized) memset very
 * early during boot because e.g. hardware isn't yet sufficiently initialized
 * may override this with their own safe implementation.
 */
__boot_func
void __weak z_early_memset(void *dst, int c, size_t n)
{
	(void) memset(dst, c, n);
}

/**
 * @brief equivalent of memcpy() for early boot usage
 *
 * Architectures that can't safely use the regular (optimized) memcpy very
 * early during boot because e.g. hardware isn't yet sufficiently initialized
 * may override this with their own safe implementation.
 */
__boot_func
void __weak z_early_memcpy(void *dst, const void *src, size_t n)
{
	(void) memcpy(dst, src, n);
}

/**
 * @brief Clear BSS
 *
 * This routine clears the BSS region, so all bytes are 0.
 */
__boot_func
void z_bss_zero(void)
{
	z_early_memset(__bss_start, 0, __bss_end - __bss_start);
#if DT_NODE_HAS_STATUS(DT_CHOSEN(zephyr_ccm), okay)
	z_early_memset(&__ccm_bss_start, 0,
		       (uintptr_t) &__ccm_bss_end
		       - (uintptr_t) &__ccm_bss_start);
#endif
#if DT_NODE_HAS_STATUS(DT_CHOSEN(zephyr_dtcm), okay)
	z_early_memset(&__dtcm_bss_start, 0,
		       (uintptr_t) &__dtcm_bss_end
		       - (uintptr_t) &__dtcm_bss_start);
#endif
#if DT_NODE_HAS_STATUS(DT_CHOSEN(zephyr_ocm), okay)
	z_early_memset(&__ocm_bss_start, 0,
		       (uintptr_t) &__ocm_bss_end
		       - (uintptr_t) &__ocm_bss_start);
#endif
#ifdef CONFIG_CODE_DATA_RELOCATION
	extern void bss_zeroing_relocation(void);

	bss_zeroing_relocation();
#endif	/* CONFIG_CODE_DATA_RELOCATION */
#ifdef CONFIG_COVERAGE_GCOV
	z_early_memset(&__gcov_bss_start, 0,
		       ((uintptr_t) &__gcov_bss_end - (uintptr_t) &__gcov_bss_start));
#endif
}

#ifdef CONFIG_LINKER_USE_BOOT_SECTION
/**
 * @brief Clear BSS within the bot region
 *
 * This routine clears the BSS within the boot region.
 * This is separate from z_bss_zero() as boot region may
 * contain symbols required for the boot process before
 * paging is initialized.
 */
__boot_func
void z_bss_zero_boot(void)
{
	z_early_memset(&lnkr_boot_bss_start, 0,
		       (uintptr_t)&lnkr_boot_bss_end
		       - (uintptr_t)&lnkr_boot_bss_start);
}
#endif /* CONFIG_LINKER_USE_BOOT_SECTION */

#ifdef CONFIG_LINKER_USE_PINNED_SECTION
/**
 * @brief Clear BSS within the pinned region
 *
 * This routine clears the BSS within the pinned region.
 * This is separate from z_bss_zero() as pinned region may
 * contain symbols required for the boot process before
 * paging is initialized.
 */
#ifdef CONFIG_LINKER_USE_BOOT_SECTION
__boot_func
#else
__pinned_func
#endif
void z_bss_zero_pinned(void)
{
	z_early_memset(&lnkr_pinned_bss_start, 0,
		       (uintptr_t)&lnkr_pinned_bss_end
		       - (uintptr_t)&lnkr_pinned_bss_start);
}
#endif /* CONFIG_LINKER_USE_PINNED_SECTION */

#ifdef CONFIG_STACK_CANARIES
extern volatile uintptr_t __stack_chk_guard;
#endif /* CONFIG_STACK_CANARIES */

/* LCOV_EXCL_STOP */

__pinned_bss
bool z_sys_post_kernel;

extern void boot_banner(void);

/**
 * @brief Mainline for kernel's background thread
 *
 * This routine completes kernel initialization by invoking the remaining
 * init functions, then invokes application's main() routine.
 */
__boot_func
static void bg_thread_main(void *unused1, void *unused2, void *unused3)
{
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	ARG_UNUSED(unused3);

#ifdef CONFIG_MMU
	/* Invoked here such that backing store or eviction algorithms may
	 * initialize kernel objects, and that all POST_KERNEL and later tasks
	 * may perform memory management tasks (except for z_phys_map() which
	 * is allowed at any time)
	 */
	z_mem_manage_init();
#endif /* CONFIG_MMU */
	z_sys_post_kernel = true;

	z_sys_init_run_level(_SYS_INIT_LEVEL_POST_KERNEL);
#if CONFIG_STACK_POINTER_RANDOM
	z_stack_adjust_initialized = 1;
#endif
	boot_banner();

#if defined(CONFIG_CPLUSPLUS)
	void z_cpp_init_static(void);
	z_cpp_init_static();
#endif

	/* Final init level before app starts */
	z_sys_init_run_level(_SYS_INIT_LEVEL_APPLICATION);

	z_init_static_threads();

#ifdef CONFIG_KERNEL_COHERENCE
	__ASSERT_NO_MSG(arch_mem_coherent(&_kernel));
#endif

#ifdef CONFIG_SMP
	if (!IS_ENABLED(CONFIG_SMP_BOOT_DELAY)) {
		z_smp_init();
	}
	z_sys_init_run_level(_SYS_INIT_LEVEL_SMP);
#endif

#ifdef CONFIG_MMU
	z_mem_manage_boot_finish();
#endif /* CONFIG_MMU */

	extern void main(void);

	main();

	/* Mark nonessential since main() has no more work to do */
	z_main_thread.base.user_options &= ~K_ESSENTIAL;

#ifdef CONFIG_COVERAGE_DUMP
	/* Dump coverage data once the main() has exited. */
	gcov_coverage_dump();
#endif
} /* LCOV_EXCL_LINE ... because we just dumped final coverage data */

#if defined(CONFIG_MULTITHREADING)
__boot_func
static void init_idle_thread(int i)
{
	struct k_thread *thread = &z_idle_threads[i];
	k_thread_stack_t *stack = z_idle_stacks[i];

#ifdef CONFIG_THREAD_NAME

#if CONFIG_MP_NUM_CPUS > 1
	char tname[8];
	snprintk(tname, 8, "idle %02d", i);
#else
	char *tname = "idle";
#endif

#else
	char *tname = NULL;
#endif /* CONFIG_THREAD_NAME */

	z_setup_new_thread(thread, stack,
			  CONFIG_IDLE_STACK_SIZE, idle, &_kernel.cpus[i],
			  NULL, NULL, K_IDLE_PRIO, K_ESSENTIAL,
			  tname);
	z_mark_thread_as_started(thread);

#ifdef CONFIG_SMP
	thread->base.is_idle = 1U;
#endif
}

void z_init_cpu(int id)
{
	init_idle_thread(id);
	_kernel.cpus[id].idle_thread = &z_idle_threads[id];
	_kernel.cpus[id].id = id;
	_kernel.cpus[id].irq_stack =
		(Z_KERNEL_STACK_BUFFER(z_interrupt_stacks[id]) +
		 K_KERNEL_STACK_SIZEOF(z_interrupt_stacks[id]));
#ifdef CONFIG_SCHED_THREAD_USAGE_ALL
	_kernel.cpus[id].usage.track_usage =
		CONFIG_SCHED_THREAD_USAGE_AUTO_ENABLE;
#endif
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
 * @return initial stack pointer for the main thread
 */
__boot_func
static char *prepare_multithreading(void)
{
	char *stack_ptr;

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
	stack_ptr = z_setup_new_thread(&z_main_thread, z_main_stack,
				       CONFIG_MAIN_STACK_SIZE, bg_thread_main,
				       NULL, NULL, NULL,
				       CONFIG_MAIN_THREAD_PRIORITY,
				       K_ESSENTIAL, "main");
	z_mark_thread_as_started(&z_main_thread);
	z_ready_thread(&z_main_thread);

	z_init_cpu(0);

	return stack_ptr;
}

__boot_func
static FUNC_NORETURN void switch_to_main_thread(char *stack_ptr)
{
#ifdef CONFIG_ARCH_HAS_CUSTOM_SWAP_TO_MAIN
	arch_switch_to_main_thread(&z_main_thread, stack_ptr, bg_thread_main);
#else
	ARG_UNUSED(stack_ptr);
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

#if defined(CONFIG_ENTROPY_HAS_DRIVER) || defined(CONFIG_TEST_RANDOM_GENERATOR)
__boot_func
void z_early_boot_rand_get(uint8_t *buf, size_t length)
{
#ifdef CONFIG_ENTROPY_HAS_DRIVER
	const struct device *entropy = DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_entropy));
	int rc;

	if (!device_is_ready(entropy)) {
		goto sys_rand_fallback;
	}

	/* Try to see if driver provides an ISR-specific API */
	rc = entropy_get_entropy_isr(entropy, buf, length, ENTROPY_BUSYWAIT);
	if (rc == -ENOTSUP) {
		/* Driver does not provide an ISR-specific API, assume it can
		 * be called from ISR context
		 */
		rc = entropy_get_entropy(entropy, buf, length);
	}

	if (rc >= 0) {
		return;
	}

	/* Fall through to fallback */

sys_rand_fallback:
#endif

	/* FIXME: this assumes sys_rand32_get() won't use any synchronization
	 * primitive, like semaphores or mutexes.  It's too early in the boot
	 * process to use any of them.  Ideally, only the path where entropy
	 * devices are available should be built, this is only a fallback for
	 * those devices without a HWRNG entropy driver.
	 */
	sys_rand_get(buf, length);
}
/* defined(CONFIG_ENTROPY_HAS_DRIVER) || defined(CONFIG_TEST_RANDOM_GENERATOR) */
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
__boot_func
FUNC_NORETURN void z_cstart(void)
{
	/* gcov hook needed to get the coverage report.*/
	gcov_static_init();

	/* perform any architecture-specific initialization */
	arch_kernel_init();

	LOG_CORE_INIT();

#if defined(CONFIG_MULTITHREADING)
	/* Note: The z_ready_thread() call in prepare_multithreading() requires
	 * a dummy thread even if CONFIG_ARCH_HAS_CUSTOM_SWAP_TO_MAIN=y
	 */
	struct k_thread dummy_thread;

	z_dummy_thread_init(&dummy_thread);
#endif
	/* do any necessary initialization of static devices */
	z_device_state_init();

	/* perform basic hardware initialization */
	z_sys_init_run_level(_SYS_INIT_LEVEL_PRE_KERNEL_1);
	z_sys_init_run_level(_SYS_INIT_LEVEL_PRE_KERNEL_2);

#ifdef CONFIG_STACK_CANARIES
	uintptr_t stack_guard;

	z_early_boot_rand_get((uint8_t *)&stack_guard, sizeof(stack_guard));
	__stack_chk_guard = stack_guard;
	__stack_chk_guard <<= 8;
#endif	/* CONFIG_STACK_CANARIES */

#ifdef CONFIG_TIMING_FUNCTIONS_NEED_AT_BOOT
	timing_init();
	timing_start();
#endif

#ifdef CONFIG_MULTITHREADING
	switch_to_main_thread(prepare_multithreading());
#else
#ifdef ARCH_SWITCH_TO_MAIN_NO_MULTITHREADING
	/* Custom ARCH-specific routine to switch to main()
	 * in the case of no multi-threading.
	 */
	ARCH_SWITCH_TO_MAIN_NO_MULTITHREADING(bg_thread_main,
		NULL, NULL, NULL);
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
#endif /* CONFIG_MULTITHREADING */

	/*
	 * Compiler can't tell that the above routines won't return and issues
	 * a warning unless we explicitly tell it that control never gets this
	 * far.
	 */

	CODE_UNREACHABLE; /* LCOV_EXCL_LINE */
}
