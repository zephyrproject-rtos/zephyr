/*
 * Copyright (c) 2026, Friedt Professional Engineering Services, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/process.h>
#include <zephyr/ztest.h>

#define STATIC_PROC_STACK_SIZE 1024
#define EXIT_CODE 42

static K_SEM_DEFINE(static_exits_ran, 0, 1);
static K_SEM_DEFINE(static_returns_ran, 0, 1);
static volatile k_pid_t static_exits_self;
static volatile k_pid_t static_exits_parent;
static volatile k_pid_t static_returns_self;
static volatile k_pid_t static_returns_parent;

static void proc_entry_exit_code(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	static_exits_self = k_getpid();
	static_exits_parent = k_getppid();
	k_sem_give(&static_exits_ran);

	k_exit(EXIT_CODE);
}

static void proc_entry_return(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	static_returns_self = k_getpid();
	static_returns_parent = k_getppid();
	k_sem_give(&static_returns_ran);
}

K_PROCESS_DEFINE(proc_exits, STATIC_PROC_STACK_SIZE, proc_entry_exit_code, NULL, NULL, NULL,
		 K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);
K_PROCESS_DEFINE(proc_returns, STATIC_PROC_STACK_SIZE, proc_entry_return, NULL, NULL, NULL,
		 K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

ZTEST(kernel_process, test_reserved_processes)
{
	k_pid_t idle = sys_process_find(0);
	k_pid_t init = sys_process_find(1);
	k_pid_t kth = sys_process_find(2);

	zassert_not_null(idle);
	zassert_not_null(init);
	zassert_not_null(kth);
	zassert_true((idle != init) && (init != kth) && (idle != kth));

	zassert_equal(sys_process_id(idle), 0);
	zassert_equal(sys_process_id(init), 1);
	zassert_equal(sys_process_id(kth), 2);

	/* each reserved process leads its own process group and session */
	zassert_not_null(k_getpgid(idle));
	zassert_true((k_getpgid(idle) != k_getpgid(init)) &&
		     (k_getpgid(init) != k_getpgid(kth)));
	zassert_true((k_getsid(idle) != k_getsid(init)) && (k_getsid(init) != k_getsid(kth)));
	zassert_equal(sys_pgrp_id(k_getpgid(idle)), 0);
	zassert_equal(sys_session_id(k_getsid(idle)), 0);
	zassert_equal(sys_pgrp_id(k_getpgid(init)), 1);
	zassert_equal(sys_session_id(k_getsid(init)), 1);
	zassert_equal(sys_pgrp_id(k_getpgid(kth)), 2);
	zassert_equal(sys_session_id(k_getsid(kth)), 2);
}

ZTEST(kernel_process, test_static_pids)
{
	int pid_a = sys_process_id(proc_exits);
	int pid_b = sys_process_id(proc_returns);

	zassert_true(pid_a >= 3);
	zassert_true(pid_b >= 3);
	zassert_not_equal(pid_a, pid_b);
	zassert_equal(sys_process_find(pid_a), proc_exits);
	zassert_equal(sys_process_find(pid_b), proc_returns);

	zassert_is_null(sys_process_find(-1));
	zassert_is_null(sys_process_find(1000000));
	zassert_equal(sys_process_id(NULL), -ESRCH);
}

ZTEST(kernel_process, test_membership)
{
	k_pid_t self = k_getpid();

	/* ztest threads are kernel threads created during boot: the kthread process */
	zassert_equal(self, sys_process_find(2));
	zassert_equal(sys_process_id(self), 2);
	zassert_equal(k_getppid(), sys_process_find(0));
	zassert_equal(k_getpgid(NULL), k_getpgid(self));
	zassert_equal(k_getsid(NULL), k_getsid(self));

	/* Linux-style: a live member thread's handle names its group */
	zassert_equal(sys_process_id(k_current_get()), 2);
}

static void child_thread_entry(void *p1, void *p2, void *p3)
{
	k_pid_t *out = p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	*out = k_getpid();
}

static K_THREAD_STACK_DEFINE(child_stack_a, STATIC_PROC_STACK_SIZE);
static struct k_thread child_a;

ZTEST(kernel_process, test_membership_inherit)
{
	k_pid_t seen = NULL;

	k_tid_t tid = k_thread_create(&child_a, child_stack_a, STATIC_PROC_STACK_SIZE,
				      child_thread_entry, &seen, NULL, NULL,
				      K_LOWEST_APPLICATION_THREAD_PRIO, 0, K_NO_WAIT);

	zassert_ok(k_thread_join(tid, K_SECONDS(5)));
	zassert_equal(seen, k_getpid());
}

ZTEST(kernel_process, test_static_process_lifecycle)
{
	zassert_ok(k_sem_take(&static_exits_ran, K_SECONDS(5)));
	zassert_ok(k_sem_take(&static_returns_ran, K_SECONDS(5)));

	/* each static process runs as its own process, parented by init */
	zassert_equal(static_exits_self, proc_exits);
	zassert_equal(static_exits_parent, sys_process_find(1));
	zassert_equal(static_returns_self, proc_returns);
	zassert_equal(static_returns_parent, sys_process_find(1));

	/*
	 * Wait for both leaders to exit. Joining is safe here: these leader
	 * thread objects are static and are never reaped or recycled in this
	 * test, so the handles stay valid thread objects throughout.
	 */
	zassert_ok(k_thread_join(proc_exits, K_SECONDS(5)));
	zassert_ok(k_thread_join(proc_returns, K_SECONDS(5)));

	/* an unreaped (zombie) process keeps its identity and still exists */
	zassert_equal(sys_process_find(sys_process_id(proc_exits)), proc_exits);
	zassert_equal(sys_process_find(sys_process_id(proc_returns)), proc_returns);
}

static void waitpid_orphan_reaped(void);

ZTEST(kernel_process, test_waitpid)
{
	int ws;
	k_pid_t reaped;

	/* the test's process (kthreadd) has no children */
	zassert_equal(k_waitpid(NULL, &reaped, &ws, 0, K_NO_WAIT), -ECHILD);
	zassert_equal(k_waitpid(NULL, &reaped, &ws, K_PROCESS_WNOHANG, K_NO_WAIT),
		      -ECHILD);
	zassert_equal(k_waitpid(NULL, &reaped, &ws, K_PROCESS_WUNTRACED, K_NO_WAIT),
		      -ENOTSUP);
	/* positive reap paths are covered once sys_clone() creates children */
	waitpid_orphan_reaped();
}

static K_THREAD_STACK_DEFINE(spawn_stack_a, STATIC_PROC_STACK_SIZE);
static K_THREAD_STACK_DEFINE(spawn_stack_b, STATIC_PROC_STACK_SIZE);
static struct k_thread spawn_thread_a;
static struct k_thread spawn_thread_b;

static K_THREAD_STACK_DEFINE(child_stack_b, STATIC_PROC_STACK_SIZE);
static struct k_thread child_b;

static volatile k_pid_t spawn_seen_ppid;

static void spawn_entry_exit(void *p1, void *p2, void *p3)
{
	int code = (int)(intptr_t)p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	spawn_seen_ppid = k_getppid();
	k_exit(code);
}

static volatile k_pid_t orphan_grandchild;

static void orphan_grandchild_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* outlive the parent: adopted by init, then exit with no one waiting */
	k_msleep(50);
	k_exit(3);
}

static void orphan_parent_entry(void *p1, void *p2, void *p3)
{
	struct sys_clone_args args = {
		.entry = orphan_grandchild_entry,
		.thread = &child_b,
		.stack = child_stack_b,
		.stack_size = STATIC_PROC_STACK_SIZE,
		.prio = K_LOWEST_APPLICATION_THREAD_PRIO,
	};

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	if (sys_clone(&args, (k_pid_t *)&orphan_grandchild) != 0) {
		k_exit(1);
	}
	k_exit(0);
}

/* a child that outlives its parent is reparented to init, which reaps it on exit */
static void waitpid_orphan_reaped(void)
{
	int ws = -1;
	int pid;
	struct sys_clone_args args = {
		.entry = orphan_parent_entry,
		.thread = &child_a,
		.stack = child_stack_a,
		.stack_size = STATIC_PROC_STACK_SIZE,
		.prio = K_LOWEST_APPLICATION_THREAD_PRIO,
	};

	orphan_grandchild = NULL;
	zassert_ok(sys_clone(&args, NULL));
	zassert_ok(k_waitpid(&child_a, NULL, &ws, 0, K_SECONDS(5)));
	zassert_true(K_WIFEXITED(ws) && (K_WEXITSTATUS(ws) == 0));
	zassert_equal(orphan_grandchild, (k_pid_t)&child_b);
	pid = sys_process_id(&child_b);
	zassert_true(pid > 0);
	/* alive and adopted: init is its parent now, and it is not our child */
	zassert_equal(sys_process_find(pid), (k_pid_t)&child_b);
	zassert_equal(k_waitpid(&child_b, NULL, &ws, K_PROCESS_WNOHANG, K_NO_WAIT), -ECHILD);

	/* its exit retires the pid without any waitpid() */
	for (int i = 0; (i < 100) && (sys_process_find(pid) != NULL); i++) {
		k_msleep(10);
	}
	zassert_is_null(sys_process_find(pid), "adopted orphan not reaped");
	zassert_equal(sys_process_id(&child_b), -ESRCH);
}

ZTEST(kernel_process, test_spawn)
{
	int ws = 0;
	int pid;
	k_pid_t reaped = NULL;
	k_pid_t child = &spawn_thread_a;
	struct sys_clone_args args = {
		.entry = spawn_entry_exit,
		.p1 = (void *)(intptr_t)7,
		.thread = &spawn_thread_a,
		.stack = spawn_stack_a,
		.stack_size = STATIC_PROC_STACK_SIZE,
		.prio = K_LOWEST_APPLICATION_THREAD_PRIO,
	};

	spawn_seen_ppid = NULL;
	zassert_ok(sys_clone(&args, NULL));

	/* the initial thread's handle is the process handle */
	pid = sys_process_id(child);
	zassert_true(pid >= 3);
	zassert_equal(sys_process_find(pid), child);

	/* observe the zombie without reaping: the handle is still owned */
	zassert_ok(k_waitpid(child, NULL, NULL, K_PROCESS_WNOWAIT, K_FOREVER));
	zassert_equal(sys_clone(&args, NULL), -EBUSY);
	zassert_equal(spawn_seen_ppid, k_getpid());

	/* blocking reap retires the handle */
	zassert_ok(k_waitpid(child, &reaped, &ws, 0, K_FOREVER));
	zassert_equal(reaped, child);
	zassert_true(K_WIFEXITED(ws));
	zassert_equal(K_WEXITSTATUS(ws), 7);
	zassert_equal(sys_process_id(child), -ESRCH);
	zassert_is_null(sys_process_find(pid));

	/* the leader thread object is reusable as a new leader after reap */
	zassert_ok(sys_clone(&args, NULL));
	/* a timed wait: the wake must leave no timeout behind for the next pend */
	zassert_ok(k_waitpid(child, NULL, NULL, 0, K_SECONDS(10)));

	/* the thread tier: a member of this process, not a new process */
	k_pid_t seen = NULL;
	k_tid_t member = NULL;
	struct sys_clone_args targs = {
		.flags = SYS_CLONE_THREAD,
		.entry = child_thread_entry,
		.p1 = &seen,
		.thread = &child_b,
		.stack = child_stack_b,
		.stack_size = STATIC_PROC_STACK_SIZE,
		.prio = K_LOWEST_APPLICATION_THREAD_PRIO,
	};

	zassert_ok(sys_clone(&targs, &member));
	zassert_equal(member, (k_pid_t)&child_b);
	zassert_ok(k_thread_join(&child_b, K_SECONDS(5)));
	zassert_equal(seen, k_getpid());
}

ZTEST(kernel_process, test_spawn_waitpid_selectors)
{
	int ws = 0;
	k_pid_t reaped = NULL;
	k_pid_t child = &spawn_thread_b;
	struct sys_clone_args args = {
		.entry = spawn_entry_exit,
		.p1 = (void *)(intptr_t)3,
		.thread = &spawn_thread_b,
		.stack = spawn_stack_b,
		.stack_size = STATIC_PROC_STACK_SIZE,
		.prio = K_LOWEST_APPLICATION_THREAD_PRIO,
	};

	zassert_ok(sys_clone(&args, NULL));

	/* WNOHANG returns EAGAIN while the child still runs, 0 once it exits */
	int ret;

	do {
		ret = k_waitpid(NULL, &reaped, &ws, K_PROCESS_WNOHANG, K_NO_WAIT);
		if (ret == -EAGAIN) {
			k_sleep(K_MSEC(10));
		}
	} while (ret == -EAGAIN);

	zassert_ok(ret);
	zassert_equal(reaped, child);
	zassert_equal(K_WEXITSTATUS(ws), 3);
}

static K_THREAD_STACK_DEFINE(spawn_stack_c, STATIC_PROC_STACK_SIZE);
static struct k_thread spawn_thread_c;

static void spawn_entry_libc_exit(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* libc exit(): runs atexit handlers, then _exit() -> k_exit() */
	exit(5);
}

ZTEST(kernel_process, test_spawn_libc_exit)
{
	int ws = 0;

	if (IS_ENABLED(CONFIG_NATIVE_LIBC) || !IS_ENABLED(CONFIG_COMMON_LIBC_EXIT)) {
		/* exit() only reaches k_exit() through the common libc exit path */
		ztest_test_skip();
		return;
	}

	struct sys_clone_args args = {
		.entry = spawn_entry_libc_exit,
		.thread = &spawn_thread_c,
		.stack = spawn_stack_c,
		.stack_size = STATIC_PROC_STACK_SIZE,
		.prio = K_LOWEST_APPLICATION_THREAD_PRIO,
	};

	zassert_ok(sys_clone(&args, NULL));
	zassert_ok(k_waitpid(&spawn_thread_c, NULL, &ws, 0, K_FOREVER));
	zassert_true(K_WIFEXITED(ws));
	zassert_equal(K_WEXITSTATUS(ws), 5);
}

ZTEST(kernel_process, test_spawn_invalid)
{
	struct sys_clone_args args = {0};

	zassert_equal(sys_clone(NULL), -EINVAL);
	zassert_equal(sys_clone(&args, NULL), -EINVAL);
}

#if (CONFIG_SYS_PROCESS_MAX >= 4) && (CONFIG_HEAP_MEM_POOL_SIZE > 0)
#define HEAP_KIDS 6

static K_THREAD_STACK_ARRAY_DEFINE(heap_stacks, HEAP_KIDS, STATIC_PROC_STACK_SIZE);
static struct k_thread heap_threads[HEAP_KIDS];
static K_SEM_DEFINE(heap_gate, 0, HEAP_KIDS);

static void heap_child_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_sem_take(&heap_gate, K_FOREVER);
	k_exit(0);
}

ZTEST(kernel_process, test_spawn_heap_growth)
{
	struct sys_clone_args args = {
		.entry = heap_child_entry,
		.stack_size = STATIC_PROC_STACK_SIZE,
		.prio = K_LOWEST_APPLICATION_THREAD_PRIO,
	};

	/* more concurrent processes than the static minimum: the tail is heap-grown */
	for (int i = 0; i < HEAP_KIDS; i++) {
		args.thread = &heap_threads[i];
		args.stack = heap_stacks[i];
		zassert_ok(sys_clone(&args, NULL), "spawn %d", i);
	}
	for (int i = 0; i < HEAP_KIDS; i++) {
		k_sem_give(&heap_gate);
	}
	for (int i = 0; i < HEAP_KIDS; i++) {
		int ws = 1;
		k_pid_t reaped = NULL;

		zassert_ok(k_waitpid(NULL, &reaped, &ws, 0, K_FOREVER));
		zassert_not_null(reaped);
		zassert_true(K_WIFEXITED(ws) && (K_WEXITSTATUS(ws) == 0));
	}
	zassert_equal(k_waitpid(NULL, NULL, NULL, 0, K_NO_WAIT), -ECHILD);
}
#endif /* CONFIG_SYS_PROCESS_MAX >= 4 && CONFIG_HEAP_MEM_POOL_SIZE > 0 */

#ifdef CONFIG_SIGNAL
static K_THREAD_STACK_DEFINE(sig_parent_stack, STATIC_PROC_STACK_SIZE);
static K_THREAD_STACK_DEFINE(sig_child_stack, STATIC_PROC_STACK_SIZE);
static struct k_thread sig_parent_thread;
static struct k_thread sig_child_thread;

#define SIGCHLD_OK	   0x51
#define SIGCHLD_BAD_SPAWN  0x52
#define SIGCHLD_BAD_WAIT   0x53
#define SIGCHLD_BAD_INFO   0x54

/*
 * K_SIG_CHLD is delivered to an eligible thread of the parent process, so the
 * assertions run inside a spawned parent process; the result is reported
 * through the parent's own exit status.
 */
static void sig_parent_entry(void *p1, void *p2, void *p3)
{
	int ws;
	struct k_sig_set set;
	struct k_sig_info info;
	k_pid_t child = &sig_child_thread;
	struct sys_clone_args args = {
		.entry = spawn_entry_exit,
		.p1 = (void *)(intptr_t)9,
		.thread = &sig_child_thread,
		.stack = sig_child_stack,
		.stack_size = STATIC_PROC_STACK_SIZE,
		.prio = K_LOWEST_APPLICATION_THREAD_PRIO,
	};

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_sig_emptyset(&set);
	k_sig_addset(&set, K_SIG_CHLD);

	if (sys_clone(&args, NULL) != 0) {
		k_exit(SIGCHLD_BAD_SPAWN);
	}

	{
		/*
		 * k_sig_timedwait() can return before its full timeout when no
		 * signal is queued yet; poll in finite chunks as the POSIX
		 * sigwait() implementation does.
		 */
		int ret;
		int64_t deadline = k_uptime_get() + 5000;

		do {
			ret = k_sig_timedwait(&set, &info, K_MSEC(100));
		} while ((ret < 0) && (k_uptime_get() < deadline));

		if (ret != K_SIG_CHLD) {
			k_exit(SIGCHLD_BAD_WAIT);
		}
	}

	if ((info.signo != K_SIG_CHLD) || (info.code != K_SI_CLD_EXITED) ||
	    (info.pid != child) || !K_WIFEXITED(info.status) ||
	    (K_WEXITSTATUS(info.status) != 9)) {
		k_exit(SIGCHLD_BAD_INFO);
	}

	/* the child is still reapable after notification */
	if (k_waitpid(child, NULL, &ws, 0, K_FOREVER) != 0) {
		k_exit(SIGCHLD_BAD_WAIT);
	}

	k_exit(SIGCHLD_OK);
}

ZTEST(kernel_process, test_sigchld)
{
	int ws = 0;
	struct sys_clone_args args = {
		.entry = sig_parent_entry,
		.thread = &sig_parent_thread,
		.stack = sig_parent_stack,
		.stack_size = STATIC_PROC_STACK_SIZE,
		.prio = K_LOWEST_APPLICATION_THREAD_PRIO,
	};

	zassert_ok(sys_clone(&args, NULL));
	zassert_ok(k_waitpid(&sig_parent_thread, NULL, &ws, 0, K_FOREVER));
	zassert_true(K_WIFEXITED(ws));
	zassert_equal(K_WEXITSTATUS(ws), SIGCHLD_OK, "parent reported 0x%x", K_WEXITSTATUS(ws));
}
#endif /* CONFIG_SIGNAL */

#ifdef CONFIG_PROCESS_INIT
static K_SEM_DEFINE(init_ran, 0, 1);
static volatile k_pid_t init_self;
static volatile k_pid_t init_parent;

void z_process_init_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	init_self = k_getpid();
	init_parent = k_getppid();
	k_sem_give(&init_ran);

	k_sleep(K_FOREVER);
}

ZTEST(kernel_process, test_init_process)
{
	zassert_ok(k_sem_take(&init_ran, K_SECONDS(5)), "init thread did not run");

	/* the init thread runs in the init process (pid 1), parented by idle */
	zassert_equal(init_self, sys_process_find(1));
	zassert_equal(sys_process_id(init_self), 1);
	zassert_equal(init_parent, sys_process_find(0));
}
#endif /* CONFIG_PROCESS_INIT */

static K_THREAD_STACK_DEFINE(pi_stack_a, STATIC_PROC_STACK_SIZE);
static K_THREAD_STACK_DEFINE(pi_stack_b, STATIC_PROC_STACK_SIZE);
static struct k_thread pi_thread_a;
static struct k_thread pi_thread_b;
static struct k_process pi_record;
static struct k_process pi_record2;

ZTEST(kernel_process, test_k_clone)
{
	int ws = 0;
	k_tid_t child = NULL;
	k_pid_t seen = NULL;
	struct k_clone_args args = {
		.entry = spawn_entry_exit,
		.p1 = (void *)(intptr_t)9,
		.thread = &child_a,
		.stack = child_stack_a,
		.stack_size = STATIC_PROC_STACK_SIZE,
		.prio = K_LOWEST_APPLICATION_THREAD_PRIO,
		.process = &pi_record,
	};
	struct k_clone_args bad;

	zassert_equal(k_clone(NULL, NULL), -EINVAL);
	bad = args;
	bad.entry = NULL;
	zassert_equal(k_clone(&bad, NULL), -EINVAL);
	bad = args;
	bad.flags = K_CLONE_THREAD;
	/* the thread tier and a process record are mutually exclusive */
	zassert_equal(k_clone(&bad, NULL), -EINVAL);
	bad = args;
	bad.process = NULL;
	zassert_equal(k_clone(&bad, NULL), -EINVAL);

	/* new process from caller-provided record, thread, and stack: no allocation */
	zassert_ok(k_clone(&args, &child));
	zassert_equal(child, &child_a);
	zassert_ok(k_waitpid(child, NULL, &ws, 0, K_FOREVER));
	zassert_true(K_WIFEXITED(ws));
	zassert_equal(K_WEXITSTATUS(ws), 9);

	/* thread tier, paused: the new thread is a member of this process */
	bad = (struct k_clone_args){
		.flags = K_CLONE_THREAD | K_CLONE_PAUSED,
		.entry = child_thread_entry,
		.p1 = &seen,
		.thread = &child_b,
		.stack = child_stack_b,
		.stack_size = STATIC_PROC_STACK_SIZE,
		.prio = K_LOWEST_APPLICATION_THREAD_PRIO,
	};
	zassert_ok(k_clone(&bad, &child));
	zassert_equal(child, &child_b);
	k_thread_start(child);
	zassert_ok(k_thread_join(child, K_SECONDS(5)));
	zassert_equal(seen, k_getpid());
}
static volatile k_pid_t pi_seen_pid_a;
static volatile k_pid_t pi_seen_pid_b;

static void pi_entry_exit(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	pi_seen_pid_a = k_getpid();
	k_exit((int)(intptr_t)p1);
}

static void pi_entry_wait(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	pi_seen_pid_b = k_getpid();
	k_sleep(K_FOREVER);
}

static void process_init_invalid(void)
{
	k_tid_t list[1] = {NULL};

	zassert_equal(k_process_init(NULL, list, 1), -EINVAL);
	zassert_equal(k_process_init(&pi_record, NULL, 1), -EINVAL);
	zassert_equal(k_process_init(&pi_record, list, 0), -EINVAL);
	zassert_equal(k_process_init(&pi_record, list, 1), -EINVAL);
}

static void process_init_two_threads(void)
{
	int ws = 0;
	k_tid_t list[2];

	list[0] = k_thread_create(&pi_thread_a, pi_stack_a, STATIC_PROC_STACK_SIZE, pi_entry_exit,
				  (void *)(intptr_t)5, NULL, NULL,
				  K_LOWEST_APPLICATION_THREAD_PRIO, 0, K_FOREVER);
	list[1] = k_thread_create(&pi_thread_b, pi_stack_b, STATIC_PROC_STACK_SIZE, pi_entry_wait,
				  NULL, NULL, NULL, K_LOWEST_APPLICATION_THREAD_PRIO, 0,
				  K_FOREVER);

	/* caller-provided record: the kernel initializes, callers allocate */
	zassert_ok(k_process_init(&pi_record, list, 2));

	/* re-initializing a busy record, or reusing a busy leader, errors */
	zassert_equal(k_process_init(&pi_record, list, 2), -EBUSY);
	zassert_equal(k_process_init(&pi_record2, list, 1), -EBUSY);

	/* both listed threads are members and observe the leader handle as pid */
	k_thread_start(list[1]);
	k_thread_start(list[0]);
	zassert_ok(k_waitpid(list[0], NULL, &ws, 0, K_FOREVER));
	zassert_true(K_WIFEXITED(ws));
	zassert_equal(K_WEXITSTATUS(ws), 5);
	zassert_equal(pi_seen_pid_a, list[0]);
	zassert_equal(pi_seen_pid_b, list[0]);
}

static void process_init_reuse(void)
{
	int ws = 0;
	k_tid_t tid = k_thread_create(&pi_thread_a, pi_stack_a, STATIC_PROC_STACK_SIZE,
				      pi_entry_exit, (void *)(intptr_t)6, NULL, NULL,
				      K_LOWEST_APPLICATION_THREAD_PRIO, 0, K_FOREVER);

	/* a reaped record and a reaped leader are both reusable */
	zassert_ok(k_process_init(&pi_record, &tid, 1));
	k_thread_start(tid);
	zassert_ok(k_waitpid(tid, NULL, &ws, 0, K_FOREVER));
	zassert_true(K_WIFEXITED(ws));
	zassert_equal(K_WEXITSTATUS(ws), 6);
}

ZTEST(kernel_process, test_process_init)
{
	process_init_invalid();
	process_init_two_threads();
	process_init_reuse();
}

ZTEST_SUITE(kernel_process, NULL, NULL, NULL, NULL, NULL);
