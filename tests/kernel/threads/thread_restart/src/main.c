/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>
#include <zephyr/irq_offload.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/test_toolchain.h>
#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>
#include <kthread.h>

/*
 * Per-architecture test parameters:
 * CPU_EXCEPTION_INSN: instruction raising K_ERR_CPU_EXCEPTION in thread
 * context, HAS_CPU_EXCEPTION is 1 when it is defined.
 * ARCH_OVERFLOW_RESTART_OK: a hardware-detected stack overflow of the
 * thread is reported as K_ERR_STACK_CHK_FAIL in its own context.
 */
#if defined(CONFIG_X86)
#define STACK_SIZE               8192
#define CPU_EXCEPTION_INSN       "ud2"
#define HAS_CPU_EXCEPTION        1
#define ARCH_OVERFLOW_RESTART_OK 1
#elif defined(CONFIG_CPU_CORTEX_M)
#define CPU_EXCEPTION_INSN "udf #0"
#define HAS_CPU_EXCEPTION  1
#elif defined(CONFIG_RISCV)
#define CPU_EXCEPTION_INSN "unimp"
#define HAS_CPU_EXCEPTION  1
#ifndef CONFIG_STACK_SENTINEL
/* The sentinel check of the swap out faults again on the guard */
#define ARCH_OVERFLOW_RESTART_OK 1
#endif
#elif defined(CONFIG_ARM64)
#define CPU_EXCEPTION_INSN "udf #0"
#define HAS_CPU_EXCEPTION  1
#elif defined(CONFIG_CPU_AARCH32_CORTEX_A) || defined(CONFIG_CPU_AARCH32_CORTEX_R)
#define CPU_EXCEPTION_INSN "udf #0"
#define HAS_CPU_EXCEPTION  1
#elif defined(CONFIG_ARC)
#define CPU_EXCEPTION_INSN "swi"
#define HAS_CPU_EXCEPTION  1
#elif defined(CONFIG_XTENSA)
#define CPU_EXCEPTION_INSN "ill"
#define HAS_CPU_EXCEPTION  1
#elif defined(CONFIG_SPARC)
#define CPU_EXCEPTION_INSN "unimp 0"
#define HAS_CPU_EXCEPTION  1
#elif defined(CONFIG_OPENRISC)
#define CPU_EXCEPTION_INSN "l.trap 0"
#define HAS_CPU_EXCEPTION  1
#elif defined(CONFIG_MIPS)
#define CPU_EXCEPTION_INSN "break"
#define HAS_CPU_EXCEPTION  1
#elif defined(CONFIG_RX)
#define CPU_EXCEPTION_INSN "brk"
#define HAS_CPU_EXCEPTION  1
#endif

#ifndef STACK_SIZE
#define STACK_SIZE (2048 + CONFIG_TEST_EXTRA_STACK_SIZE)
#endif
#ifndef HAS_CPU_EXCEPTION
#define HAS_CPU_EXCEPTION 0
#endif
#ifndef ARCH_OVERFLOW_RESTART_OK
#define ARCH_OVERFLOW_RESTART_OK 0
#endif

#define THREAD_PRIO  K_PRIO_PREEMPT(1)
/* Same priority as the ztest thread and the system workqueue */
#define COOP_PRIO    K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1)
#define CHANGED_PRIO K_PRIO_PREEMPT(2)
#define JOIN_TIMEOUT K_SECONDS(10)
#define P1           ((void *)0x1111)
#define P2           ((void *)0x2222)
#define P3           ((void *)0x3333)

enum fault_mode {
	MODE_OOPS,
	MODE_CPU_EXCEPTION,
	MODE_PANIC,
	MODE_RETURN,
	MODE_SLEEP,
	MODE_MUTATE,
	MODE_OVERFLOW,
	MODE_OOPS_ONCE,
	MODE_COUNT_OOPS,
	MODE_ISR_OOPS,
};

static K_THREAD_STACK_DEFINE(stack, STACK_SIZE);
static struct k_thread thread;

static volatile enum fault_mode mode;
static volatile unsigned int expected_reason;
static struct k_thread *volatile expected_thread;
static atomic_t runs;
static atomic_t faults;
static volatile bool bad_args;

/* What the second run of a MODE_MUTATE thread observed */
static volatile int seen_prio;
static void *volatile seen_custom_data;
static volatile bool seen_preemptible;
/* Restart count a MODE_COUNT_OOPS thread read at the start of each run */
static volatile uint16_t seen_counts[4];

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *esf)
{
	ARG_UNUSED(esf);

	if ((reason != expected_reason) || (k_current_get() != expected_thread)) {
		printk("Unexpected fatal error %u in %p\n", reason, k_current_get());
		TC_END_REPORT(TC_FAIL);
		k_fatal_halt(reason);
	}

	atomic_inc(&faults);
}

#ifdef CONFIG_IRQ_OFFLOAD
static void oops_isr(const void *arg)
{
	ARG_UNUSED(arg);

	k_oops();
}
#endif /* CONFIG_IRQ_OFFLOAD */

static void cpu_exception(void)
{
#if HAS_CPU_EXCEPTION
	__asm__ volatile(CPU_EXCEPTION_INSN);
#endif
}

TOOLCHAIN_DISABLE_WARNING(TOOLCHAIN_WARNING_PRAGMAS)
TOOLCHAIN_DISABLE_WARNING(TOOLCHAIN_WARNING_INFINITE_RECURSION)

static __no_optimization int stack_smasher(int val)
{
	return stack_smasher(val * 2) + stack_smasher(val * 3);
}

TOOLCHAIN_ENABLE_WARNING(TOOLCHAIN_WARNING_PRAGMAS)
TOOLCHAIN_ENABLE_WARNING(TOOLCHAIN_WARNING_INFINITE_RECURSION)

static void fault_entry(void *p1, void *p2, void *p3)
{
	atomic_val_t run = atomic_inc(&runs);

	if ((p1 != P1) || (p2 != P2) || (p3 != P3)) {
		bad_args = true;
	}

	switch (mode) {
	case MODE_OOPS:
		k_oops();
		break;
	case MODE_CPU_EXCEPTION:
		cpu_exception();
		break;
	case MODE_PANIC:
		k_panic();
		break;
	case MODE_SLEEP:
		k_sleep(K_FOREVER);
		break;
	case MODE_MUTATE:
		if (run == 0) {
			k_thread_priority_set(k_current_get(), CHANGED_PRIO);
			k_thread_custom_data_set(&runs);
			k_sched_lock();
			k_oops();
		}
		seen_prio = k_thread_priority_get(k_current_get());
		seen_custom_data = k_thread_custom_data_get();
		seen_preemptible = k_is_preempt_thread() != 0;
		break;
	case MODE_OVERFLOW:
		if (run == 0) {
			(void)stack_smasher(37);
		}
		break;
	case MODE_OOPS_ONCE:
		if (run == 0) {
			k_oops();
		}
		k_sleep(K_FOREVER);
		break;
	case MODE_COUNT_OOPS:
		if (run < ARRAY_SIZE(seen_counts)) {
			seen_counts[run] = k_thread_restart_count_get(k_current_get());
		}
		k_oops();
		break;
	case MODE_ISR_OOPS:
#ifdef CONFIG_IRQ_OFFLOAD
		irq_offload(oops_isr, NULL);
#endif /* CONFIG_IRQ_OFFLOAD */
		break;
	case MODE_RETURN:
	default:
		break;
	}
}

static k_tid_t start_thread_prio(enum fault_mode m, unsigned int reason, uint16_t limit,
				 int prio)
{
	k_tid_t tid = k_thread_create(&thread, stack, K_THREAD_STACK_SIZEOF(stack), fault_entry,
				      P1, P2, P3, prio, 0, K_FOREVER);

	mode = m;
	expected_reason = reason;
	expected_thread = tid;
	zassert_ok(k_thread_restart_limit_set(tid, limit));
	k_thread_start(tid);

	return tid;
}

static k_tid_t start_thread(enum fault_mode m, unsigned int reason, uint16_t limit)
{
	return start_thread_prio(m, reason, limit, THREAD_PRIO);
}

static bool wait_for(atomic_t *counter, atomic_val_t value)
{
	for (int i = 0; i < 500; i++) {
		if (atomic_get(counter) == value) {
			return true;
		}
		k_msleep(10);
	}

	return false;
}

static void check_restarts(k_tid_t tid, uint16_t restarts, atomic_val_t fault_count)
{
	/* k_thread_join() returns at every death, wait for the last run */
	zassert_true(wait_for(&runs, restarts + 1));
	zassert_true(wait_for(&faults, fault_count));
	zassert_ok(k_thread_join(tid, JOIN_TIMEOUT));
	/* No further restart follows */
	k_msleep(50);
	zassert_equal(atomic_get(&runs), restarts + 1);
	zassert_equal(atomic_get(&faults), fault_count);
	zassert_equal(k_thread_restart_count_get(tid), restarts);
	zassert_false(bad_args, "entry arguments changed across restarts");
}

ZTEST(thread_restart, test_restart_on_oops)
{
	k_tid_t tid = start_thread(MODE_OOPS, K_ERR_KERNEL_OOPS, 2);

	/* The third fault exceeds the limit and kills the thread */
	check_restarts(tid, 2, 3);
}

ZTEST(thread_restart, test_restart_on_cpu_exception)
{
	k_tid_t tid;

	if (HAS_CPU_EXCEPTION == 0) {
		ztest_test_skip();
	}

	tid = start_thread(MODE_CPU_EXCEPTION, K_ERR_CPU_EXCEPTION, 2);

	check_restarts(tid, 2, 3);
}

ZTEST(thread_restart, test_restart_cooperative)
{
	/* The exception exit must not resume the faulting cooperative thread */
	check_restarts(start_thread_prio(MODE_OOPS, K_ERR_KERNEL_OOPS, 2, COOP_PRIO), 2, 3);

	if (HAS_CPU_EXCEPTION != 0) {
		atomic_clear(&runs);
		atomic_clear(&faults);
		check_restarts(start_thread_prio(MODE_CPU_EXCEPTION, K_ERR_CPU_EXCEPTION, 2,
						 COOP_PRIO),
			       2, 3);
	}
}

ZTEST(thread_restart, test_restart_above_workqueue_priority)
{
	int prio = CONFIG_SYSTEM_WORKQUEUE_PRIORITY - 1;

	if (prio < K_HIGHEST_THREAD_PRIO) {
		ztest_test_skip();
	}

	/* The restarted thread preempts the workqueue that restarts it, then
	 * reads its count and faults again
	 */
	check_restarts(start_thread_prio(MODE_COUNT_OOPS, K_ERR_KERNEL_OOPS, 2, prio), 2, 3);
	zassert_equal(seen_counts[0], 0);
	zassert_equal(seen_counts[1], 1);
	zassert_equal(seen_counts[2], 2);
}

ZTEST(thread_restart, test_creation_state_restored)
{
	k_tid_t tid = k_thread_create(&thread, stack, K_THREAD_STACK_SIZEOF(stack), fault_entry,
				      P1, P2, P3, THREAD_PRIO, K_RESTARTABLE, K_FOREVER);

	mode = MODE_MUTATE;
	expected_reason = K_ERR_KERNEL_OOPS;
	expected_thread = tid;
	zassert_ok(k_thread_name_set(tid, "restarted"));
	k_thread_start(tid);

	check_restarts(tid, 1, 1);
	zassert_equal(seen_prio, THREAD_PRIO);
	zassert_is_null(seen_custom_data);
	zassert_true(seen_preemptible, "scheduler lock survived the restart");
	zassert_str_equal(k_thread_name_get(tid), "restarted");
}

ZTEST(thread_restart, test_no_restart_on_return_or_abort)
{
	k_tid_t tid = start_thread(MODE_RETURN, K_ERR_KERNEL_OOPS, 3);

	check_restarts(tid, 0, 0);

	atomic_clear(&runs);
	tid = start_thread(MODE_SLEEP, K_ERR_KERNEL_OOPS, 3);
	zassert_true(wait_for(&runs, 1));
	k_thread_abort(tid);
	check_restarts(tid, 0, 0);
}

ZTEST(thread_restart, test_no_restart_on_panic)
{
	k_tid_t tid = start_thread(MODE_PANIC, K_ERR_KERNEL_PANIC, 3);

	check_restarts(tid, 0, 1);
}

static K_SEM_DEFINE(block_sem, 0, 1);

static void block_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	(void)k_sem_take(&block_sem, K_FOREVER);
}

static bool state_has(k_tid_t tid, const char *state)
{
	char buf[32];

	return strstr(k_thread_state_str(tid, buf, sizeof(buf)), state) != NULL;
}

/* Start a thread whose first fault leaves its restart pending */
static k_tid_t start_pending_thread(struct k_work *block_work, enum fault_mode m, uint16_t limit)
{
	k_tid_t tid;

	/* Hold the system workqueue so the restart stays pending */
	zassert_true(k_work_submit(block_work) >= 0);
	tid = start_thread(m, K_ERR_KERNEL_OOPS, limit);

	/* The fault handler runs before the restart is queued and the thread
	 * aborted, possibly on another CPU: wait for the thread to be dead
	 */
	zassert_ok(k_thread_join(tid, JOIN_TIMEOUT));
	zassert_equal(atomic_get(&faults), 1);
	zassert_equal(k_thread_restart_count_get(tid), 1);

	/* The thread is dead until it is created again */
	zassert_true(state_has(tid, "dead"));

	return tid;
}

ZTEST(thread_restart, test_pending_window)
{
	static K_WORK_DEFINE(block_work, block_handler);
	k_tid_t tid;

	/* The restart runs once the system workqueue gets to it */
	tid = start_pending_thread(&block_work, MODE_OOPS_ONCE, 1);
	k_msleep(20);
	zassert_equal(atomic_get(&runs), 1);
	k_sem_give(&block_sem);
	zassert_true(wait_for(&runs, 2));
	zassert_false(state_has(tid, "dead"));
	k_thread_abort(tid);
	check_restarts(tid, 1, 1);

	/* Aborted while pending: the restart is cancelled */
	atomic_clear(&runs);
	atomic_clear(&faults);
	tid = start_pending_thread(&block_work, MODE_OOPS, 3);
	k_thread_abort(tid);
	k_sem_give(&block_sem);
	k_msleep(50);
	zassert_equal(atomic_get(&runs), 1);
	zassert_ok(k_thread_join(tid, K_NO_WAIT));
}

ZTEST(thread_restart, test_create_cancels_pending_restart)
{
	static K_WORK_DEFINE(block_work, block_handler);
	k_tid_t tid = start_pending_thread(&block_work, MODE_OOPS, 3);

	/* Reusing the object cancels the restart of the faulted thread */
	tid = k_thread_create(&thread, stack, K_THREAD_STACK_SIZEOF(stack), fault_entry, P1, P2,
			      P3, THREAD_PRIO, 0, K_FOREVER);
	zassert_equal(k_thread_restart_count_get(tid), 0);

	mode = MODE_RETURN;
	k_thread_start(tid);
	zassert_ok(k_thread_join(tid, JOIN_TIMEOUT));

	/* The cancelled restart never runs */
	k_sem_give(&block_sem);
	k_msleep(50);
	zassert_equal(atomic_get(&runs), 2);
	zassert_equal(atomic_get(&faults), 1);
	zassert_equal(k_thread_restart_count_get(tid), 0);
}

ZTEST(thread_restart, test_dynamic_stack_free_pending)
{
#ifdef CONFIG_DYNAMIC_THREAD
	static K_WORK_DEFINE(block_work, block_handler);
	k_thread_stack_t *dyn_stack = k_thread_stack_alloc(CONFIG_DYNAMIC_THREAD_STACK_SIZE, 0);
	k_tid_t tid;

	zassert_not_null(dyn_stack);
	zassert_true(k_work_submit(&block_work) >= 0);
	tid = k_thread_create(&thread, dyn_stack, CONFIG_DYNAMIC_THREAD_STACK_SIZE, fault_entry,
			      P1, P2, P3, THREAD_PRIO, 0, K_FOREVER);
	mode = MODE_OOPS_ONCE;
	expected_reason = K_ERR_KERNEL_OOPS;
	expected_thread = tid;
	zassert_ok(k_thread_restart_limit_set(tid, 1));
	k_thread_start(tid);
	zassert_true(wait_for(&faults, 1));

	/* The pending restart runs the thread on this stack again */
	zassert_equal(k_thread_stack_free(dyn_stack), -EBUSY);
	k_sem_give(&block_sem);
	zassert_true(wait_for(&runs, 2));
	zassert_equal(k_thread_stack_free(dyn_stack), -EBUSY);

	k_thread_abort(tid);
	check_restarts(tid, 1, 1);
	zassert_ok(k_thread_stack_free(dyn_stack));
#else
	ztest_test_skip();
#endif /* CONFIG_DYNAMIC_THREAD */
}

ZTEST(thread_restart, test_restart_on_stack_overflow)
{
	if ((ARCH_OVERFLOW_RESTART_OK == 0) || !IS_ENABLED(CONFIG_HW_STACK_PROTECTION)) {
		ztest_test_skip();
	}

	check_restarts(start_thread(MODE_OVERFLOW, K_ERR_STACK_CHK_FAIL, 1), 1, 1);
}

ZTEST(thread_restart, test_limit_set_errors)
{
	static K_THREAD_STACK_DEFINE(essential_stack, STACK_SIZE);
	static struct k_thread essential;
	k_tid_t tid;

	tid = k_thread_create(&essential, essential_stack, K_THREAD_STACK_SIZEOF(essential_stack),
			      fault_entry, P1, P2, P3, THREAD_PRIO, K_ESSENTIAL, K_FOREVER);
	zassert_equal(k_thread_restart_limit_set(tid, 1), -EINVAL);
	/* A restart would restore K_ESSENTIAL */
	z_thread_essential_clear(tid);
	zassert_false(z_is_thread_essential(tid));
	zassert_equal(k_thread_restart_limit_set(tid, 1), -EINVAL);
	k_thread_abort(tid);

	/* Disarming also clears the count, the next fault kills the thread */
	atomic_clear(&runs);
	tid = k_thread_create(&thread, stack, K_THREAD_STACK_SIZEOF(stack), fault_entry, P1, P2,
			      P3, THREAD_PRIO, K_RESTARTABLE, K_FOREVER);
	mode = MODE_OOPS;
	expected_reason = K_ERR_KERNEL_OOPS;
	expected_thread = tid;
	zassert_ok(k_thread_restart_limit_set(tid, 0));
	k_thread_start(tid);
	check_restarts(tid, 0, 1);
}

ZTEST(thread_restart, test_rejected_restart_not_counted)
{
	k_tid_t tid;

	/* The plugged system workqueue rejects the restart: the thread stays
	 * dead and the restart does not count against its limit
	 */
	zassert_true(k_work_queue_drain(&k_sys_work_q, true) >= 0);
	tid = start_thread(MODE_OOPS, K_ERR_KERNEL_OOPS, 1);
	zassert_ok(k_thread_join(tid, JOIN_TIMEOUT));
	zassert_ok(k_work_queue_unplug(&k_sys_work_q));

	k_msleep(50);
	zassert_equal(atomic_get(&faults), 1);
	zassert_equal(atomic_get(&runs), 1);
	zassert_equal(k_thread_restart_count_get(tid), 0);
}

ZTEST(thread_restart, test_limit_max)
{
	k_tid_t tid;

	/* Above the maximum: rejected, the default limit stays in effect */
	tid = k_thread_create(&thread, stack, K_THREAD_STACK_SIZEOF(stack), fault_entry, P1, P2,
			      P3, THREAD_PRIO, K_RESTARTABLE, K_FOREVER);
	mode = MODE_OOPS;
	expected_reason = K_ERR_KERNEL_OOPS;
	expected_thread = tid;
	zassert_equal(k_thread_restart_limit_set(tid, CONFIG_THREAD_RESTART_LIMIT_MAX + 1),
		      -EINVAL);
	k_thread_start(tid);
	check_restarts(tid, CONFIG_THREAD_RESTART_LIMIT_DEFAULT,
		       CONFIG_THREAD_RESTART_LIMIT_DEFAULT + 1);

	/* At the maximum: restarted that many times, then aborted */
	atomic_clear(&runs);
	atomic_clear(&faults);
	tid = start_thread(MODE_OOPS, K_ERR_KERNEL_OOPS, CONFIG_THREAD_RESTART_LIMIT_MAX);
	check_restarts(tid, CONFIG_THREAD_RESTART_LIMIT_MAX, CONFIG_THREAD_RESTART_LIMIT_MAX + 1);
}

static void user_oops_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_oops();
}

static void user_creator_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p3);

	/* K_RESTARTABLE is ignored for user threads instead of panicking */
	(void)k_thread_create(p1, p2, STACK_SIZE, user_oops_entry, NULL, NULL, NULL, THREAD_PRIO,
			      K_USER | K_RESTARTABLE, K_NO_WAIT);
}

static void enter_user_entry(void *p1, void *p2, void *p3)
{
	k_thread_user_mode_enter(user_oops_entry, p1, p2, p3);
}

static ZTEST_BMEM volatile int user_child_runs;
static K_SEM_DEFINE(probe_sem, 0, 1);

static void user_child_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	user_child_runs++;
}

static void user_recreate_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p3);

	/* Reuse the object of the faulted supervisor thread */
	(void)k_thread_create(p1, p2, STACK_SIZE, user_child_entry, NULL, NULL, NULL, THREAD_PRIO,
			      K_USER, K_NO_WAIT);
	(void)k_thread_join(p1, K_FOREVER);
}

static void probe_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	k_sem_give(&probe_sem);
}

ZTEST(thread_restart, test_user_create_cancels_pending_restart)
{
	static K_THREAD_STACK_DEFINE(user_stack, STACK_SIZE);
	static K_THREAD_STACK_DEFINE(child_stack, STACK_SIZE);
	static struct k_thread user;
	static K_WORK_DEFINE(block_work, block_handler);
	static K_WORK_DEFINE(probe_work, probe_handler);
	k_tid_t tid;
	k_tid_t user_tid;

	if (!IS_ENABLED(CONFIG_USERSPACE)) {
		ztest_test_skip();
	}

	/* A user thread allowed to use the object of a faulted thread
	 * creates its own thread on it while the restart is pending
	 */
	user_child_runs = 0;
	tid = start_pending_thread(&block_work, MODE_OOPS, 3);
	user_tid = k_thread_create(&user, user_stack, K_THREAD_STACK_SIZEOF(user_stack),
				   user_recreate_entry, tid, child_stack, NULL, THREAD_PRIO,
				   K_USER, K_FOREVER);
	k_thread_access_grant(user_tid, tid, child_stack);
	k_thread_start(user_tid);
	zassert_ok(k_thread_join(user_tid, JOIN_TIMEOUT));
	zassert_equal(user_child_runs, 1);

	/* The restart is cancelled and the system workqueue keeps going */
	k_sem_give(&block_sem);
	zassert_true(k_work_submit(&probe_work) >= 0);
	zassert_ok(k_sem_take(&probe_sem, JOIN_TIMEOUT));
	k_msleep(50);
	zassert_equal(user_child_runs, 1);
	zassert_equal(atomic_get(&runs), 1);
	zassert_equal(k_thread_restart_count_get(tid), 0);
}

static void user_stack_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p3);

	/* Create a thread of its own on the stack of the faulted thread */
	(void)k_thread_create(p1, p2, STACK_SIZE, user_child_entry, NULL, NULL, NULL, THREAD_PRIO,
			      K_USER, K_NO_WAIT);
}

ZTEST(thread_restart, test_user_create_on_pending_stack)
{
	static K_THREAD_STACK_DEFINE(user_stack, STACK_SIZE);
	static struct k_thread user;
	static struct k_thread child;
	static K_WORK_DEFINE(block_work, block_handler);
	k_tid_t tid;
	k_tid_t user_tid;

	if (!IS_ENABLED(CONFIG_USERSPACE)) {
		ztest_test_skip();
	}

	/* A user thread allowed to use the stack of a faulted thread cannot
	 * take it while the restart is pending: its creation oopses
	 */
	user_child_runs = 0;
	tid = start_pending_thread(&block_work, MODE_OOPS_ONCE, 1);
	user_tid = k_thread_create(&user, user_stack, K_THREAD_STACK_SIZEOF(user_stack),
				   user_stack_entry, &child, stack, NULL, THREAD_PRIO, K_USER,
				   K_FOREVER);
	k_thread_access_grant(user_tid, &child, stack);
	expected_thread = user_tid;
	k_thread_start(user_tid);
	zassert_ok(k_thread_join(user_tid, JOIN_TIMEOUT));
	zassert_equal(atomic_get(&faults), 2);
	zassert_equal(user_child_runs, 0);

	/* The faulted thread is restarted on its stack */
	expected_thread = tid;
	k_sem_give(&block_sem);
	zassert_true(wait_for(&runs, 2));
	k_thread_abort(tid);
	check_restarts(tid, 1, 2);
}

static void user_abort_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_thread_abort(p1);
}

ZTEST(thread_restart, test_user_abort_cancels_pending_restart)
{
	static K_THREAD_STACK_DEFINE(user_stack, STACK_SIZE);
	static struct k_thread user;
	static K_WORK_DEFINE(block_work, block_handler);
	k_tid_t tid;
	k_tid_t user_tid;

	if (!IS_ENABLED(CONFIG_USERSPACE)) {
		ztest_test_skip();
	}

	/* The abort left the thread object uninitialized while its restart
	 * is pending, a user mode abort must still cancel it
	 */
	tid = start_pending_thread(&block_work, MODE_OOPS, 3);
	user_tid = k_thread_create(&user, user_stack, K_THREAD_STACK_SIZEOF(user_stack),
				   user_abort_entry, tid, NULL, NULL, THREAD_PRIO, K_USER,
				   K_FOREVER);
	k_thread_access_grant(user_tid, tid);
	k_thread_start(user_tid);
	zassert_ok(k_thread_join(user_tid, JOIN_TIMEOUT));

	k_sem_give(&block_sem);
	k_msleep(50);
	zassert_equal(atomic_get(&runs), 1);
	zassert_equal(atomic_get(&faults), 1);
}

ZTEST(thread_restart, test_user_thread_not_supported)
{
	static K_THREAD_STACK_DEFINE(child_stack, STACK_SIZE);
	static struct k_thread child;
	k_tid_t tid;

	if (!IS_ENABLED(CONFIG_USERSPACE)) {
		ztest_test_skip();
	}

	tid = k_thread_create(&thread, stack, K_THREAD_STACK_SIZEOF(stack), fault_entry, P1, P2,
			      P3, THREAD_PRIO, K_USER, K_FOREVER);
	zassert_equal(k_thread_restart_limit_set(tid, 1), -ENOTSUP);
	k_thread_abort(tid);

	/* A user thread creates a K_RESTARTABLE child, which is not restarted */
	tid = k_thread_create(&thread, stack, K_THREAD_STACK_SIZEOF(stack), user_creator_entry,
			      &child, child_stack, NULL, THREAD_PRIO, K_USER, K_FOREVER);
	k_thread_access_grant(tid, &child, child_stack);
	expected_reason = K_ERR_KERNEL_OOPS;
	expected_thread = &child;
	k_thread_start(tid);
	zassert_ok(k_thread_join(tid, JOIN_TIMEOUT));
	zassert_ok(k_thread_join(&child, JOIN_TIMEOUT));
	zassert_equal(atomic_get(&faults), 1);
	zassert_equal(k_thread_restart_count_get(&child), 0);

	/* An armed thread that drops to user mode is no longer restarted */
	atomic_clear(&faults);
	tid = k_thread_create(&thread, stack, K_THREAD_STACK_SIZEOF(stack), enter_user_entry, NULL,
			      NULL, NULL, THREAD_PRIO, 0, K_FOREVER);
	zassert_ok(k_thread_restart_limit_set(tid, 3));
	expected_thread = tid;
	k_thread_start(tid);
	zassert_ok(k_thread_join(tid, JOIN_TIMEOUT));
	zassert_equal(atomic_get(&faults), 1);
	zassert_equal(k_thread_restart_count_get(tid), 0);
}

ZTEST(thread_restart, test_dynamic_object_not_restarted)
{
#ifdef CONFIG_DYNAMIC_OBJECTS
	struct k_thread *dyn_thread = k_object_alloc(K_OBJ_THREAD);
	k_thread_stack_t *dyn_stack = k_object_alloc_size(K_OBJ_THREAD_STACK_ELEMENT, STACK_SIZE);
	k_tid_t tid;

	zassert_not_null(dyn_thread);
	zassert_not_null(dyn_stack);
	mode = MODE_OOPS;
	expected_reason = K_ERR_KERNEL_OOPS;

	/* The abort could free a dynamic thread object or stack: no restart */
	tid = k_thread_create(dyn_thread, stack, K_THREAD_STACK_SIZEOF(stack), fault_entry, P1,
			      P2, P3, THREAD_PRIO, K_RESTARTABLE, K_FOREVER);
	zassert_equal(k_thread_restart_limit_set(tid, 1), -ENOTSUP);
	zassert_ok(k_thread_restart_limit_set(tid, 0));
	expected_thread = tid;
	k_thread_start(tid);
	check_restarts(tid, 0, 1);

	atomic_clear(&runs);
	atomic_clear(&faults);
	tid = k_thread_create(&thread, dyn_stack, STACK_SIZE, fault_entry, P1, P2, P3,
			      THREAD_PRIO, K_RESTARTABLE, K_FOREVER);
	zassert_equal(k_thread_restart_limit_set(tid, 1), -ENOTSUP);
	expected_thread = tid;
	k_thread_start(tid);
	check_restarts(tid, 0, 1);

	k_object_release(dyn_thread);
	k_object_release(dyn_stack);

	/* Released by its creator, the faulting thread holds the last
	 * permission on its object, so its abort frees the object
	 */
	atomic_clear(&runs);
	atomic_clear(&faults);
	dyn_thread = k_object_alloc(K_OBJ_THREAD);
	zassert_not_null(dyn_thread);
	tid = k_thread_create(dyn_thread, stack, K_THREAD_STACK_SIZEOF(stack), fault_entry, P1,
			      P2, P3, THREAD_PRIO, K_RESTARTABLE, K_FOREVER);
	expected_thread = tid;
	k_object_release(dyn_thread);
	k_thread_start(tid);
	zassert_true(wait_for(&faults, 1));
	k_msleep(50);
	zassert_equal(atomic_get(&runs), 1);
#else
	ztest_test_skip();
#endif /* CONFIG_DYNAMIC_OBJECTS */
}

K_THREAD_DEFINE(static_thread, STACK_SIZE, fault_entry, P1, P2, P3, THREAD_PRIO, K_RESTARTABLE,
		SYS_FOREVER_MS);

ZTEST(thread_restart, test_static_thread_default_limit)
{
	mode = MODE_OOPS;
	expected_reason = K_ERR_KERNEL_OOPS;
	expected_thread = static_thread;
	k_thread_start(static_thread);

	check_restarts(static_thread, CONFIG_THREAD_RESTART_LIMIT_DEFAULT,
		       CONFIG_THREAD_RESTART_LIMIT_DEFAULT + 1);
}

ZTEST(thread_restart, test_smp_isr_fault_restarted)
{
	if (!IS_ENABLED(CONFIG_IRQ_OFFLOAD) || !IS_ENABLED(CONFIG_SMP) || (arch_num_cpus() < 2)) {
		ztest_test_skip();
	}

	/* Aborted in interrupt context, the thread is restarted once the
	 * interrupt exit is done with its stack, possibly by another CPU
	 */
	check_restarts(start_thread(MODE_ISR_OOPS, K_ERR_KERNEL_OOPS,
				    CONFIG_THREAD_RESTART_LIMIT_MAX),
		       CONFIG_THREAD_RESTART_LIMIT_MAX, CONFIG_THREAD_RESTART_LIMIT_MAX + 1);
}

static atomic_t waker_stop;

static void waker_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (atomic_get(&waker_stop) == 0) {
		k_wakeup(p1);
	}
}

ZTEST(thread_restart, test_smp_wakeup_during_restart)
{
	static K_THREAD_STACK_DEFINE(waker_stack, STACK_SIZE);
	static struct k_thread waker;
	k_tid_t tid;

	if (!IS_ENABLED(CONFIG_SMP) || (arch_num_cpus() < 2)) {
		ztest_test_skip();
	}

	/* Another CPU keeps waking the thread up while it is created again:
	 * it must start once each restart, once it is complete
	 */
	tid = k_thread_create(&thread, stack, K_THREAD_STACK_SIZEOF(stack), fault_entry, P1, P2,
			      P3, THREAD_PRIO, 0, K_FOREVER);
	mode = MODE_OOPS;
	expected_reason = K_ERR_KERNEL_OOPS;
	expected_thread = tid;
	zassert_ok(k_thread_restart_limit_set(tid, CONFIG_THREAD_RESTART_LIMIT_MAX));
	atomic_clear(&waker_stop);
	k_thread_create(&waker, waker_stack, K_THREAD_STACK_SIZEOF(waker_stack), waker_entry, tid,
			NULL, NULL, K_LOWEST_APPLICATION_THREAD_PRIO, 0, K_NO_WAIT);
	k_thread_start(tid);

	check_restarts(tid, CONFIG_THREAD_RESTART_LIMIT_MAX, CONFIG_THREAD_RESTART_LIMIT_MAX + 1);
	atomic_set(&waker_stop, 1);
	zassert_ok(k_thread_join(&waker, JOIN_TIMEOUT));
}

ZTEST(thread_restart, test_smp_abort_race)
{
	if (!IS_ENABLED(CONFIG_SMP) || (arch_num_cpus() < 2)) {
		ztest_test_skip();
	}

	for (uint32_t i = 0U; i < 20U; i++) {
		k_tid_t tid = start_thread(MODE_OOPS, K_ERR_KERNEL_OOPS,
					   CONFIG_THREAD_RESTART_LIMIT_MAX);

		/* Abort at varying points of the fault and restart cycle */
		k_busy_wait(1000U + ((i * 397U) % 5000U));
		k_thread_abort(tid);
		zassert_ok(k_thread_join(tid, K_NO_WAIT));
		zassert_false(bad_args, "entry arguments changed across restarts");
		atomic_clear(&runs);
		atomic_clear(&faults);
	}
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	atomic_clear(&runs);
	atomic_clear(&faults);
	bad_args = false;
}

ZTEST_SUITE(thread_restart, NULL, NULL, before, NULL, NULL);
