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

#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <offsets_short.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/debug/stack.h>
#include <zephyr/random/random.h>
#include <zephyr/linker/sections.h>
#include <zephyr/toolchain.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/platform/hooks.h>
#include <ksched.h>
#include <kthread.h>
#include <zephyr/sys/dlist.h>
#include <kernel_internal.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/tracing/tracing.h>
#include <zephyr/debug/gcov.h>
#include <kswap.h>
#include <zephyr/timing/timing.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/internal/syscall_handler.h>
LOG_MODULE_REGISTER(os, CONFIG_KERNEL_LOG_LEVEL);

/* the only struct z_kernel instance */
__pinned_bss
struct z_kernel _kernel;

#ifdef CONFIG_PM
__pinned_bss atomic_t _cpus_active;
#endif

/* init/main and idle threads */
K_THREAD_PINNED_STACK_DEFINE(z_main_stack, CONFIG_MAIN_STACK_SIZE);
struct k_thread z_main_thread;

#ifdef CONFIG_MULTITHREADING
__pinned_bss
struct k_thread z_idle_threads[CONFIG_MP_MAX_NUM_CPUS];

static K_KERNEL_PINNED_STACK_ARRAY_DEFINE(z_idle_stacks,
					  CONFIG_MP_MAX_NUM_CPUS,
					  CONFIG_IDLE_STACK_SIZE);

static void z_init_static_threads(void)
{
	STRUCT_SECTION_FOREACH(_static_thread_data, thread_data) {
		z_setup_new_thread(
			thread_data->init_thread,
			thread_data->init_stack,
			thread_data->init_stack_size,
			thread_data->init_entry,
			thread_data->init_p1,
			thread_data->init_p2,
			thread_data->init_p3,
			thread_data->init_prio,
			thread_data->init_options,
			thread_data->init_name);

		thread_data->init_thread->init_data = thread_data;
	}

#ifdef CONFIG_USERSPACE
	STRUCT_SECTION_FOREACH(k_object_assignment, pos) {
		for (int i = 0; pos->objects[i] != NULL; i++) {
			k_object_access_grant(pos->objects[i],
					      pos->thread);
		}
	}
#endif /* CONFIG_USERSPACE */

	/*
	 * Non-legacy static threads may be started immediately or
	 * after a previously specified delay. Even though the
	 * scheduler is locked, ticks can still be delivered and
	 * processed. Take a sched lock to prevent them from running
	 * until they are all started.
	 *
	 * Note that static threads defined using the legacy API have a
	 * delay of K_FOREVER.
	 */
	k_sched_lock();
	STRUCT_SECTION_FOREACH(_static_thread_data, thread_data) {
		k_timeout_t init_delay = Z_THREAD_INIT_DELAY(thread_data);

		if (!K_TIMEOUT_EQ(init_delay, K_FOREVER)) {
			thread_schedule_new(thread_data->init_thread,
					    init_delay);
		}
	}
	k_sched_unlock();
}
#else
#define z_init_static_threads() do { } while (false)
#endif /* CONFIG_MULTITHREADING */

extern const struct init_entry __init_start[];
extern const struct init_entry __init_EARLY_start[];
extern const struct init_entry __init_PRE_KERNEL_1_start[];
extern const struct init_entry __init_PRE_KERNEL_2_start[];
extern const struct init_entry __init_POST_KERNEL_start[];
extern const struct init_entry __init_APPLICATION_start[];
extern const struct init_entry __init_end[];

enum init_level {
	INIT_LEVEL_EARLY = 0,
	INIT_LEVEL_PRE_KERNEL_1,
	INIT_LEVEL_PRE_KERNEL_2,
	INIT_LEVEL_POST_KERNEL,
	INIT_LEVEL_APPLICATION,
#ifdef CONFIG_SMP
	INIT_LEVEL_SMP,
#endif /* CONFIG_SMP */
};

#ifdef CONFIG_SMP
extern const struct init_entry __init_SMP_start[];
#endif /* CONFIG_SMP */

/*
 * storage space for the interrupt stack
 *
 * Note: This area is used as the system stack during kernel initialization,
 * since the kernel hasn't yet set up its own stack areas. The dual purposing
 * of this area is safe since interrupts are disabled until the kernel context
 * switches to the init thread.
 */
K_KERNEL_PINNED_STACK_ARRAY_DEFINE(z_interrupt_stacks,
				   CONFIG_MP_MAX_NUM_CPUS,
				   CONFIG_ISR_STACK_SIZE);

extern void idle(void *unused1, void *unused2, void *unused3);

#ifdef CONFIG_OBJ_CORE_SYSTEM
static struct k_obj_type obj_type_cpu;
static struct k_obj_type obj_type_kernel;

#ifdef CONFIG_OBJ_CORE_STATS_SYSTEM
static struct k_obj_core_stats_desc  cpu_stats_desc = {
	.raw_size = sizeof(struct k_cycle_stats),
	.query_size = sizeof(struct k_thread_runtime_stats),
	.raw   = z_cpu_stats_raw,
	.query = z_cpu_stats_query,
	.reset = NULL,
	.disable = NULL,
	.enable  = NULL,
};

static struct k_obj_core_stats_desc  kernel_stats_desc = {
	.raw_size = sizeof(struct k_cycle_stats) * CONFIG_MP_MAX_NUM_CPUS,
	.query_size = sizeof(struct k_thread_runtime_stats),
	.raw   = z_kernel_stats_raw,
	.query = z_kernel_stats_query,
	.reset = NULL,
	.disable = NULL,
	.enable  = NULL,
};
#endif /* CONFIG_OBJ_CORE_STATS_SYSTEM */
#endif /* CONFIG_OBJ_CORE_SYSTEM */

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
	if (IS_ENABLED(CONFIG_SKIP_BSS_CLEAR)) {
		return;
	}

	z_early_memset(__bss_start, 0, __bss_end - __bss_start);
#if DT_NODE_HAS_STATUS_OKAY(DT_CHOSEN(zephyr_ccm))
	z_early_memset(&__ccm_bss_start, 0,
		       (uintptr_t) &__ccm_bss_end
		       - (uintptr_t) &__ccm_bss_start);
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_CHOSEN(zephyr_dtcm))
	z_early_memset(&__dtcm_bss_start, 0,
		       (uintptr_t) &__dtcm_bss_end
		       - (uintptr_t) &__dtcm_bss_start);
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_CHOSEN(zephyr_ocm))
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
#endif /* CONFIG_COVERAGE_GCOV */
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
#endif /* CONFIG_LINKER_USE_BOOT_SECTION */
void z_bss_zero_pinned(void)
{
	z_early_memset(&lnkr_pinned_bss_start, 0,
		       (uintptr_t)&lnkr_pinned_bss_end
		       - (uintptr_t)&lnkr_pinned_bss_start);
}
#endif /* CONFIG_LINKER_USE_PINNED_SECTION */

#ifdef CONFIG_REQUIRES_STACK_CANARIES
#ifdef CONFIG_STACK_CANARIES_TLS
extern Z_THREAD_LOCAL volatile uintptr_t __stack_chk_guard;
#else
extern volatile uintptr_t __stack_chk_guard;
#endif /* CONFIG_STACK_CANARIES_TLS */
#endif /* CONFIG_REQUIRES_STACK_CANARIES */

/* LCOV_EXCL_STOP */

__pinned_bss
bool z_sys_post_kernel;

static int do_device_init(const struct device *dev)
{
	int rc = 0;

	if (dev->ops.init != NULL) {
		rc = dev->ops.init(dev);
		/* Mark device initialized. If initialization
		 * failed, record the error condition.
		 */
		if (rc != 0) {
			if (rc < 0) {
				rc = -rc;
			}
			if (rc > UINT8_MAX) {
				rc = UINT8_MAX;
			}
			dev->state->init_res = rc;
		}
	}

	dev->state->initialized = true;

	if (rc == 0) {
		/* Run automatic device runtime enablement */
		(void)pm_device_runtime_auto_enable(dev);
	}

	return rc;
}

/**
 * @brief Execute all the init entry initialization functions at a given level
 *
 * @details Invokes the initialization routine for each init entry object
 * created by the INIT_ENTRY_DEFINE() macro using the specified level.
 * The linker script places the init entry objects in memory in the order
 * they need to be invoked, with symbols indicating where one level leaves
 * off and the next one begins.
 *
 * @param level init level to run.
 */
static void z_sys_init_run_level(enum init_level level)
{
	static const struct init_entry *levels[] = {
		__init_EARLY_start,
		__init_PRE_KERNEL_1_start,
		__init_PRE_KERNEL_2_start,
		__init_POST_KERNEL_start,
		__init_APPLICATION_start,
#ifdef CONFIG_SMP
		__init_SMP_start,
#endif /* CONFIG_SMP */
		/* End marker */
		__init_end,
	};
	const struct init_entry *entry;

	for (entry = levels[level]; entry < levels[level+1]; entry++) {
		const struct device *dev = entry->dev;
		int result = 0;

		sys_trace_sys_init_enter(entry, level);
		if (dev != NULL) {
			if ((dev->flags & DEVICE_FLAG_INIT_DEFERRED) == 0U) {
				result = do_device_init(dev);
			}
		} else {
			result = entry->init_fn();
		}
		sys_trace_sys_init_exit(entry, level, result);
	}
}


int z_impl_device_init(const struct device *dev)
{
	if (dev->state->initialized) {
		return -EALREADY;
	}

	return do_device_init(dev);
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_device_init(const struct device *dev)
{
	K_OOPS(K_SYSCALL_OBJ_INIT(dev, K_OBJ_ANY));

	return z_impl_device_init(dev);
}
#include <zephyr/syscalls/device_init_mrsh.c>
#endif

extern void boot_banner(void);

#ifdef CONFIG_BOOTARGS
extern const char *get_bootargs(void);
static char **prepare_main_args(int *argc)
{
#ifdef CONFIG_DYNAMIC_BOOTARGS
	const char *bootargs = get_bootargs();
#else
	const char bootargs[] = CONFIG_BOOTARGS_STRING;
#endif

	/* beginning of the buffer contains argument's strings, end of it contains argvs */
	static char args_buf[CONFIG_BOOTARGS_ARGS_BUFFER_SIZE];
	char *strings_end = (char *)args_buf;
	char **argv_begin = (char **)WB_DN(
		args_buf + CONFIG_BOOTARGS_ARGS_BUFFER_SIZE - sizeof(char *));
	int i = 0;

	*argc = 0;
	*argv_begin = NULL;

#ifdef CONFIG_DYNAMIC_BOOTARGS
	if (!bootargs) {
		return argv_begin;
	}
#endif

	while (1) {
		while (isspace(bootargs[i])) {
			i++;
		}

		if (bootargs[i] == '\0') {
			return argv_begin;
		}

		if (strings_end + sizeof(char *) >= (char *)argv_begin) {
			LOG_WRN("not enough space in args buffer to accommodate all bootargs"
				" - bootargs truncated");
			return argv_begin;
		}

		argv_begin--;
		memmove(argv_begin, argv_begin + 1, *argc * sizeof(char *));
		argv_begin[*argc] = strings_end;

		bool quoted = false;

		if (bootargs[i] == '\"' || bootargs[i] == '\'') {
			char delimiter = bootargs[i];

			for (int j = i + 1; bootargs[j] != '\0'; j++) {
				if (bootargs[j] == delimiter) {
					quoted = true;
					break;
				}
			}
		}

		if (quoted) {
			char delimiter  = bootargs[i];

			i++; /* strip quotes */
			while (bootargs[i] != delimiter
				&& strings_end < (char *)argv_begin) {
				*strings_end++ = bootargs[i++];
			}
			i++; /* strip quotes */
		} else {
			while (!isspace(bootargs[i])
				&& bootargs[i] != '\0'
				&& strings_end < (char *)argv_begin) {
				*strings_end++ = bootargs[i++];
			}
		}

		if (strings_end < (char *)argv_begin) {
			*strings_end++ = '\0';
		} else {
			LOG_WRN("not enough space in args buffer to accommodate all bootargs"
				" - bootargs truncated");
			argv_begin[*argc] = NULL;
			return argv_begin;
		}
		(*argc)++;
	}
}
#endif

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
	 * may perform memory management tasks (except for
	 * k_mem_map_phys_bare() which is allowed at any time)
	 */
	z_mem_manage_init();
#endif /* CONFIG_MMU */
	z_sys_post_kernel = true;

#if CONFIG_IRQ_OFFLOAD
	arch_irq_offload_init();
#endif
	z_sys_init_run_level(INIT_LEVEL_POST_KERNEL);
#if CONFIG_SOC_LATE_INIT_HOOK
	soc_late_init_hook();
#endif
#if CONFIG_BOARD_LATE_INIT_HOOK
	board_late_init_hook();
#endif

#if defined(CONFIG_STACK_POINTER_RANDOM) && (CONFIG_STACK_POINTER_RANDOM != 0)
	z_stack_adjust_initialized = 1;
#endif /* CONFIG_STACK_POINTER_RANDOM */
	boot_banner();

	void z_init_static(void);
	z_init_static();

	/* Final init level before app starts */
	z_sys_init_run_level(INIT_LEVEL_APPLICATION);

	z_init_static_threads();

#ifdef CONFIG_KERNEL_COHERENCE
	__ASSERT_NO_MSG(arch_mem_coherent(&_kernel));
#endif /* CONFIG_KERNEL_COHERENCE */

#ifdef CONFIG_SMP
	if (!IS_ENABLED(CONFIG_SMP_BOOT_DELAY)) {
		z_smp_init();
	}
	z_sys_init_run_level(INIT_LEVEL_SMP);
#endif /* CONFIG_SMP */

#ifdef CONFIG_MMU
	z_mem_manage_boot_finish();
#endif /* CONFIG_MMU */

#ifdef CONFIG_BOOTARGS
	extern int main(int, char **);

	int argc = 0;
	char **argv = prepare_main_args(&argc);
	(void)main(argc, argv);
#else
	extern int main(void);

	(void)main();
#endif /* CONFIG_BOOTARGS */

	/* Mark non-essential since main() has no more work to do */
	z_thread_essential_clear(&z_main_thread);

#ifdef CONFIG_COVERAGE_DUMP
	/* Dump coverage data once the main() has exited. */
	gcov_coverage_dump();
#endif /* CONFIG_COVERAGE_DUMP */
} /* LCOV_EXCL_LINE ... because we just dumped final coverage data */

#if defined(CONFIG_MULTITHREADING)
__boot_func
static void init_idle_thread(int i)
{
	struct k_thread *thread = &z_idle_threads[i];
	k_thread_stack_t *stack = z_idle_stacks[i];
	size_t stack_size = K_KERNEL_STACK_SIZEOF(z_idle_stacks[i]);

#ifdef CONFIG_THREAD_NAME

#if CONFIG_MP_MAX_NUM_CPUS > 1
	char tname[8];
	snprintk(tname, 8, "idle %02d", i);
#else
	char *tname = "idle";
#endif /* CONFIG_MP_MAX_NUM_CPUS */

#else
	char *tname = NULL;
#endif /* CONFIG_THREAD_NAME */

	z_setup_new_thread(thread, stack,
			  stack_size, idle, &_kernel.cpus[i],
			  NULL, NULL, K_IDLE_PRIO, K_ESSENTIAL,
			  tname);
	z_mark_thread_as_not_sleeping(thread);

#ifdef CONFIG_SMP
	thread->base.is_idle = 1U;
#endif /* CONFIG_SMP */
}

void z_init_cpu(int id)
{
	init_idle_thread(id);
	_kernel.cpus[id].idle_thread = &z_idle_threads[id];
	_kernel.cpus[id].id = id;
	_kernel.cpus[id].irq_stack =
		(K_KERNEL_STACK_BUFFER(z_interrupt_stacks[id]) +
		 K_KERNEL_STACK_SIZEOF(z_interrupt_stacks[id]));
#ifdef CONFIG_SCHED_THREAD_USAGE_ALL
	_kernel.cpus[id].usage = &_kernel.usage[id];
	_kernel.cpus[id].usage->track_usage =
		CONFIG_SCHED_THREAD_USAGE_AUTO_ENABLE;
#endif

#ifdef CONFIG_PM
	/*
	 * Increment number of CPUs active. The pm subsystem
	 * will keep track of this from here.
	 */
	atomic_inc(&_cpus_active);
#endif

#ifdef CONFIG_OBJ_CORE_SYSTEM
	k_obj_core_init_and_link(K_OBJ_CORE(&_kernel.cpus[id]), &obj_type_cpu);
#ifdef CONFIG_OBJ_CORE_STATS_SYSTEM
	k_obj_core_stats_register(K_OBJ_CORE(&_kernel.cpus[id]),
				  _kernel.cpus[id].usage,
				  sizeof(struct k_cycle_stats));
#endif
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
#endif /* CONFIG_SMP */
	stack_ptr = z_setup_new_thread(&z_main_thread, z_main_stack,
				       K_THREAD_STACK_SIZEOF(z_main_stack),
				       bg_thread_main,
				       NULL, NULL, NULL,
				       CONFIG_MAIN_THREAD_PRIORITY,
				       K_ESSENTIAL, "main");
	z_mark_thread_as_not_sleeping(&z_main_thread);
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
#endif /* CONFIG_ARCH_HAS_CUSTOM_SWAP_TO_MAIN */
	CODE_UNREACHABLE; /* LCOV_EXCL_LINE */
}
#endif /* CONFIG_MULTITHREADING */

__boot_func
void __weak z_early_rand_get(uint8_t *buf, size_t length)
{
	static uint64_t state = (uint64_t)CONFIG_TIMER_RANDOM_INITIAL_STATE;
	int rc;

#ifdef CONFIG_ENTROPY_HAS_DRIVER
	const struct device *const entropy = DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_entropy));

	if ((entropy != NULL) && device_is_ready(entropy)) {
		/* Try to see if driver provides an ISR-specific API */
		rc = entropy_get_entropy_isr(entropy, buf, length, ENTROPY_BUSYWAIT);
		if (rc > 0) {
			length -= rc;
			buf += rc;
		}
	}
#endif /* CONFIG_ENTROPY_HAS_DRIVER */

	while (length > 0) {
		uint32_t val;

		state = state + k_cycle_get_32();
		state = state * 2862933555777941757ULL + 3037000493ULL;
		val = (uint32_t)(state >> 32);
		rc = MIN(length, sizeof(val));
		z_early_memcpy((void *)buf, &val, rc);

		length -= rc;
		buf += rc;
	}
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
__boot_func
FUNC_NO_STACK_PROTECTOR
FUNC_NORETURN void z_cstart(void)
{
	/* gcov hook needed to get the coverage report.*/
	gcov_static_init();

	/* initialize early init calls */
	z_sys_init_run_level(INIT_LEVEL_EARLY);

	/* perform any architecture-specific initialization */
	arch_kernel_init();

	LOG_CORE_INIT();

#if defined(CONFIG_MULTITHREADING)
	z_dummy_thread_init(&_thread_dummy);
#endif /* CONFIG_MULTITHREADING */
	/* do any necessary initialization of static devices */
	z_device_state_init();

#if CONFIG_SOC_EARLY_INIT_HOOK
	soc_early_init_hook();
#endif
#if CONFIG_BOARD_EARLY_INIT_HOOK
	board_early_init_hook();
#endif
	/* perform basic hardware initialization */
	z_sys_init_run_level(INIT_LEVEL_PRE_KERNEL_1);
#if defined(CONFIG_SMP)
	arch_smp_init();
#endif
	z_sys_init_run_level(INIT_LEVEL_PRE_KERNEL_2);

#ifdef CONFIG_REQUIRES_STACK_CANARIES
	uintptr_t stack_guard;

	z_early_rand_get((uint8_t *)&stack_guard, sizeof(stack_guard));
	__stack_chk_guard = stack_guard;
	__stack_chk_guard <<= 8;
#endif	/* CONFIG_REQUIRES_STACK_CANARIES */

#ifdef CONFIG_TIMING_FUNCTIONS_NEED_AT_BOOT
	timing_init();
	timing_start();
#endif /* CONFIG_TIMING_FUNCTIONS_NEED_AT_BOOT */

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
#endif /* ARCH_SWITCH_TO_MAIN_NO_MULTITHREADING */
#endif /* CONFIG_MULTITHREADING */

	/*
	 * Compiler can't tell that the above routines won't return and issues
	 * a warning unless we explicitly tell it that control never gets this
	 * far.
	 */

	CODE_UNREACHABLE; /* LCOV_EXCL_LINE */
}

#ifdef CONFIG_OBJ_CORE_SYSTEM
static int init_cpu_obj_core_list(void)
{
	/* Initialize CPU object type */

	z_obj_type_init(&obj_type_cpu, K_OBJ_TYPE_CPU_ID,
			offsetof(struct _cpu, obj_core));

#ifdef CONFIG_OBJ_CORE_STATS_SYSTEM
	k_obj_type_stats_init(&obj_type_cpu, &cpu_stats_desc);
#endif /* CONFIG_OBJ_CORE_STATS_SYSTEM */

	return 0;
}

static int init_kernel_obj_core_list(void)
{
	/* Initialize kernel object type */

	z_obj_type_init(&obj_type_kernel, K_OBJ_TYPE_KERNEL_ID,
			offsetof(struct z_kernel, obj_core));

#ifdef CONFIG_OBJ_CORE_STATS_SYSTEM
	k_obj_type_stats_init(&obj_type_kernel, &kernel_stats_desc);
#endif /* CONFIG_OBJ_CORE_STATS_SYSTEM */

	k_obj_core_init_and_link(K_OBJ_CORE(&_kernel), &obj_type_kernel);
#ifdef CONFIG_OBJ_CORE_STATS_SYSTEM
	k_obj_core_stats_register(K_OBJ_CORE(&_kernel), _kernel.usage,
				  sizeof(_kernel.usage));
#endif /* CONFIG_OBJ_CORE_STATS_SYSTEM */

	return 0;
}

SYS_INIT(init_cpu_obj_core_list, PRE_KERNEL_1,
	 CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);

SYS_INIT(init_kernel_obj_core_list, PRE_KERNEL_1,
	 CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);
#endif /* CONFIG_OBJ_CORE_SYSTEM */
