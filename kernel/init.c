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
#include <string.h>
#include <sys/dlist.h>
#include <kernel_internal.h>
#include <kswap.h>
#include <drivers/entropy.h>
#include <logging/log_ctrl.h>
#include <tracing/tracing.h>
#include <stdbool.h>
#include <debug/gcov.h>
#include <kswap.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(os, CONFIG_KERNEL_LOG_LEVEL);

/* boot time measurement items */
#ifdef CONFIG_BOOT_TIME_MEASUREMENT
uint32_t __noinit z_timestamp_main;  /* timestamp when main task starts */
uint32_t __noinit z_timestamp_idle;  /* timestamp when CPU goes idle */
#endif

/* init/main and idle threads */
K_THREAD_STACK_DEFINE(z_main_stack, CONFIG_MAIN_STACK_SIZE);
struct k_thread z_main_thread;

#ifdef CONFIG_MULTITHREADING
struct k_thread z_idle_threads[CONFIG_MP_NUM_CPUS];
static K_KERNEL_STACK_ARRAY_DEFINE(z_idle_stacks, CONFIG_MP_NUM_CPUS,
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
K_KERNEL_STACK_ARRAY_DEFINE(z_interrupt_stacks, CONFIG_MP_NUM_CPUS,
			    CONFIG_ISR_STACK_SIZE);

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
#if DT_NODE_HAS_STATUS(DT_CHOSEN(zephyr_ccm), okay)
	(void)memset(&__ccm_bss_start, 0,
		     ((uint32_t) &__ccm_bss_end - (uint32_t) &__ccm_bss_start));
#endif
#if DT_NODE_HAS_STATUS(DT_CHOSEN(zephyr_dtcm), okay)
	(void)memset(&__dtcm_bss_start, 0,
		     ((uint32_t) &__dtcm_bss_end - (uint32_t) &__dtcm_bss_start));
#endif
#ifdef CONFIG_CODE_DATA_RELOCATION
	extern void bss_zeroing_relocation(void);

	bss_zeroing_relocation();
#endif	/* CONFIG_CODE_DATA_RELOCATION */
#ifdef CONFIG_COVERAGE_GCOV
	(void)memset(&__gcov_bss_start, 0,
		 ((uintptr_t) &__gcov_bss_end - (uintptr_t) &__gcov_bss_start));
#endif
}

#ifdef CONFIG_STACK_CANARIES
extern volatile uintptr_t __stack_chk_guard;
#endif /* CONFIG_STACK_CANARIES */

/* LCOV_EXCL_STOP */

bool z_sys_post_kernel;
extern void boot_banner(void);

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

	z_sys_post_kernel = true;

	z_sys_init_run_level(_SYS_INIT_LEVEL_POST_KERNEL);
#if CONFIG_STACK_POINTER_RANDOM
	z_stack_adjust_initialized = 1;
#endif
	boot_banner();

#ifdef CONFIG_CPLUSPLUS
	/* Process the .ctors and .init_array sections */
	extern void __do_global_ctors_aux(void);
	extern void __do_init_array_aux(void);
	__do_global_ctors_aux();
	__do_init_array_aux();
#endif

	/* Final init level before app starts */
	z_sys_init_run_level(_SYS_INIT_LEVEL_APPLICATION);

	z_init_static_threads();

#ifdef KERNEL_COHERENCE
	__ASSERT_NO_MSG(arch_mem_coherent(_kernel));
#endif

#ifdef CONFIG_SMP
	z_smp_init();
	z_sys_init_run_level(_SYS_INIT_LEVEL_SMP);
#endif

#ifdef CONFIG_BOOT_TIME_MEASUREMENT
	z_timestamp_main = k_cycle_get_32();
#endif

	extern void main(void);

	main();

	/* Mark nonessenrial since main() has no more work to do */
	z_main_thread.base.user_options &= ~K_ESSENTIAL;

#ifdef CONFIG_COVERAGE_DUMP
	/* Dump coverage data once the main() has exited. */
	gcov_coverage_dump();
#endif
} /* LCOV_EXCL_LINE ... because we just dumped final coverage data */

/* LCOV_EXCL_START */

void __weak main(void)
{
	/* NOP default main() if the application does not provide one. */
	arch_nop();
}

/* LCOV_EXCL_STOP */

#if defined(CONFIG_MULTITHREADING)
static void init_idle_thread(int i)
{
	struct k_thread *thread = &z_idle_threads[i];
	k_thread_stack_t *stack = z_idle_stacks[i];

#ifdef CONFIG_THREAD_NAME
	char tname[8];

	snprintk(tname, 8, "idle %02d", i);
#else
	char *tname = NULL;
#endif /* CONFIG_THREAD_NAME */

	z_setup_new_thread(thread, stack,
			  CONFIG_IDLE_STACK_SIZE, idle, &_kernel.cpus[i],
			  NULL, NULL, K_LOWEST_THREAD_PRIO, K_ESSENTIAL,
			  tname);
	z_mark_thread_as_started(thread);

#ifdef CONFIG_SMP
	thread->base.is_idle = 1U;
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
static char *prepare_multithreading(void)
{
	char *stack_ptr;
	uint32_t opt;

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

	opt = K_ESSENTIAL;
#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
	/* Enable FPU in main thread */
	opt |= K_FP_REGS;
#endif

	stack_ptr = z_setup_new_thread(&z_main_thread, z_main_stack,
				       CONFIG_MAIN_STACK_SIZE, bg_thread_main,
				       NULL, NULL, NULL,
				       CONFIG_MAIN_THREAD_PRIORITY,
				       opt, "main");
	z_mark_thread_as_started(&z_main_thread);
	z_ready_thread(&z_main_thread);

	for (int i = 0; i < CONFIG_MP_NUM_CPUS; i++) {
		init_idle_thread(i);
		_kernel.cpus[i].idle_thread = &z_idle_threads[i];
		_kernel.cpus[i].id = i;
		_kernel.cpus[i].irq_stack =
			(Z_KERNEL_STACK_BUFFER(z_interrupt_stacks[i]) +
			 K_KERNEL_STACK_SIZEOF(z_interrupt_stacks[i]));
	}

	initialize_timeouts();

	return stack_ptr;
}

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
void z_early_boot_rand_get(uint8_t *buf, size_t length)
{
	int n = sizeof(uint32_t);
#ifdef CONFIG_ENTROPY_HAS_DRIVER
	const struct device *entropy = device_get_binding(DT_CHOSEN_ZEPHYR_ENTROPY_LABEL);
	int rc;

	if (entropy == NULL) {
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

	while (length > 0) {
		uint32_t rndbits;
		uint8_t *p_rndbits = (uint8_t *)&rndbits;

		rndbits = sys_rand32_get();

		if (length < sizeof(uint32_t)) {
			n = length;
		}

		for (int i = 0; i < n; i++) {
			*buf = *p_rndbits;
			buf++;
			p_rndbits++;
		}

		length -= n;
	}
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
FUNC_NORETURN void z_cstart(void)
{
	/* gcov hook needed to get the coverage report.*/
	gcov_static_init();

	LOG_CORE_INIT();

	/* perform any architecture-specific initialization */
	arch_kernel_init();

#if defined(CONFIG_MULTITHREADING)
	/* Note: The z_ready_thread() call in prepare_multithreading() requires
	 * a dummy thread even if CONFIG_ARCH_HAS_CUSTOM_SWAP_TO_MAIN=y
	 */
	struct k_thread dummy_thread;

	z_dummy_thread_init(&dummy_thread);
#endif

	/* perform basic hardware initialization */
	z_sys_init_run_level(_SYS_INIT_LEVEL_PRE_KERNEL_1);
	z_sys_init_run_level(_SYS_INIT_LEVEL_PRE_KERNEL_2);

#ifdef CONFIG_STACK_CANARIES
	uintptr_t stack_guard;

	z_early_boot_rand_get((uint8_t *)&stack_guard, sizeof(stack_guard));
	__stack_chk_guard = stack_guard;
	__stack_chk_guard <<= 8;
#endif	/* CONFIG_STACK_CANARIES */

#ifdef CONFIG_THREAD_RUNTIME_STATS_USE_TIMING_FUNCTIONS
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
