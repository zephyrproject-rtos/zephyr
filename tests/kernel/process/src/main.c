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

#ifdef CONFIG_SIGNAL
static void waitpid_chld_drop(void);
#endif /* CONFIG_SIGNAL */

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
#ifdef CONFIG_SIGNAL
	waitpid_chld_drop();
#endif /* CONFIG_SIGNAL */
	/* positive reap paths are covered once sys_clone() creates children */
	waitpid_orphan_reaped();
}

ZTEST(kernel_process, test_setsid)
{
	/* kthreadd is its own process-group leader */
	zassert_is_null(sys_setsid());
}

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

#ifdef CONFIG_SIGNAL
static void disp_noop_handler(int sig, struct k_sig_info *info, void *ucontext)
{
	ARG_UNUSED(sig);
	ARG_UNUSED(info);
	ARG_UNUSED(ucontext);
}

static void disp_child_entry(void *p1, void *p2, void *p3)
{
	int code = 0;
	struct k_sig_action oact;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* a fresh process inherits ignored dispositions, and only those */
	if ((k_sig_action(K_SIG_USR2, NULL, &oact) != 0) || (oact.handler != K_SIG_IGN)) {
		code |= 1;
	}
	if ((k_sig_action(K_SIG_USR1, NULL, &oact) != 0) || (oact.handler != K_SIG_DFL)) {
		code |= 2;
	}
	k_exit(code);
}

static void spawn_dispositions(void)
{
	int ws = -1;
	k_pid_t child = &child_a;
	struct k_sig_action act = {.handler = K_SIG_IGN};
	struct sys_clone_args args = {
		.entry = disp_child_entry,
		.thread = &child_a,
		.stack = child_stack_a,
		.stack_size = STATIC_PROC_STACK_SIZE,
		.prio = K_LOWEST_APPLICATION_THREAD_PRIO,
	};

	zassert_ok(k_sig_emptyset(&act.mask));
	zassert_ok(k_sig_action(K_SIG_USR2, &act, NULL));
	act.handler = disp_noop_handler;
	zassert_ok(k_sig_action(K_SIG_USR1, &act, NULL));

	zassert_ok(sys_clone(&args, NULL));
	zassert_ok(k_waitpid(child, NULL, &ws, 0, K_FOREVER));
	zassert_true(K_WIFEXITED(ws) && (K_WEXITSTATUS(ws) == 0),
		     "dispositions not inherited as a fresh image requires (code=%d)",
		     K_WEXITSTATUS(ws));

	act.handler = K_SIG_DFL;
	zassert_ok(k_sig_action(K_SIG_USR1, &act, NULL));
	zassert_ok(k_sig_action(K_SIG_USR2, &act, NULL));
}
#endif /* CONFIG_SIGNAL */

ZTEST(kernel_process, test_spawn)
{
	int ws = 0;
	int pid;
	k_pid_t reaped = NULL;
	k_pid_t child = &child_a;
	struct sys_clone_args args = {
		.entry = spawn_entry_exit,
		.p1 = (void *)(intptr_t)7,
		.thread = &child_a,
		.stack = child_stack_a,
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

#ifdef CONFIG_SIGNAL
	/* a kernel caller's explicit mask is honored verbatim, KILL included */
	struct sys_clone_args pargs = {
		.flags = SYS_CLONE_PAUSED,
		.entry = spawn_entry_exit,
		.p1 = (void *)(intptr_t)4,
		.thread = &child_b,
		.stack = child_stack_b,
		.stack_size = STATIC_PROC_STACK_SIZE,
		.prio = K_LOWEST_APPLICATION_THREAD_PRIO,
	};
	struct k_sig_set spawn_fill;

	k_sig_fillset(&spawn_fill);
	pargs.sigmask = &spawn_fill;
	zassert_ok(sys_clone(&pargs, NULL));
	zassert_equal(k_sig_ismember(&child_b.base.sig.mask, K_SIG_KILL), 1);
	zassert_equal(k_sig_ismember(&child_b.base.sig.mask, K_SIG_STOP), 1);
	k_thread_start(&child_b);
	zassert_ok(k_waitpid(&child_b, NULL, NULL, 0, K_FOREVER));
#endif /* CONFIG_SIGNAL */

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

	zassert_equal(sys_clone(&(struct sys_clone_args){.flags = SYS_CLONE_THREAD |
							 SYS_CLONE_VM_COPY},
			       NULL),
		      -EINVAL);
	zassert_ok(sys_clone(&targs, &member));
	zassert_equal(member, (k_pid_t)&child_b);
	zassert_ok(k_thread_join(&child_b, K_SECONDS(5)));
	zassert_equal(seen, k_getpid());

#ifdef CONFIG_SIGNAL
	spawn_dispositions();
#endif /* CONFIG_SIGNAL */
}

ZTEST(kernel_process, test_spawn_waitpid_selectors)
{
	int ws = 0;
	k_pid_t reaped = NULL;
	k_pid_t child = &child_b;
	struct sys_clone_args args = {
		.entry = spawn_entry_exit,
		.p1 = (void *)(intptr_t)3,
		.thread = &child_b,
		.stack = child_stack_b,
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

static K_THREAD_STACK_DEFINE(child_stack_c, STATIC_PROC_STACK_SIZE);
static struct k_thread child_c;

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
		.thread = &child_c,
		.stack = child_stack_c,
		.stack_size = STATIC_PROC_STACK_SIZE,
		.prio = K_LOWEST_APPLICATION_THREAD_PRIO,
	};

	zassert_ok(sys_clone(&args, NULL));
	zassert_ok(k_waitpid(&child_c, NULL, &ws, 0, K_FOREVER));
	zassert_true(K_WIFEXITED(ws));
	zassert_equal(K_WEXITSTATUS(ws), 5);
}

ZTEST(kernel_process, test_spawn_invalid)
{
	struct sys_clone_args args = {0};

	zassert_equal(sys_clone(NULL, NULL), -EINVAL);
	zassert_equal(sys_clone(&args, NULL), -EINVAL);

	/* undefined flags are invalid; the fork model awaits its substrate */
	args.flags = BIT64(63);
	zassert_equal(sys_clone(&args, NULL), -EINVAL);
	args.flags = SYS_CLONE_VM_COPY;
	args.entry = NULL;
	zassert_equal(sys_clone(&args, NULL), -ENOTSUP);
	zassert_equal(sys_fork(NULL), -ENOTSUP);
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

static K_SEM_DEFINE(pgrp_gate, 0, 2);

static void pgrp_child_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_sem_take(&pgrp_gate, K_FOREVER);
	k_exit(0);
}

ZTEST(kernel_process, test_setpgid)
{
	k_pid_t child = &child_a;
	struct sys_clone_args args = {
		.entry = pgrp_child_entry,
		.thread = &child_a,
		.stack = child_stack_a,
		.stack_size = STATIC_PROC_STACK_SIZE,
		.prio = K_LOWEST_APPLICATION_THREAD_PRIO,
	};

	zassert_ok(sys_clone(&args, NULL));

	/* a spawned child inherits the caller's process group */
	zassert_equal(k_getpgid(child), k_getpgid(NULL));

	/* a NULL pgrp starts a new group led by the child, in its session */
	zassert_ok(sys_setpgid(child, NULL));
	zassert_not_equal(k_getpgid(child), k_getpgid(NULL));
	zassert_equal(sys_pgrp_id(k_getpgid(child)), sys_process_id(child));
	zassert_equal(k_getsid(child), k_getsid(NULL));

	/* joining the group it already leads is a no-op */
	zassert_ok(k_setpgid(child, k_getpgid(child)));
	zassert_equal(sys_pgrp_id(k_getpgid(child)), sys_process_id(child));

	/* a process that is neither the caller nor its child is not reachable */
	zassert_equal(sys_setpgid(sys_process_find(0), NULL), -ESRCH);

	/* a session leader cannot change its process group */
	zassert_equal(sys_setpgid(NULL, NULL), -EPERM);

	/* release and reap */
	k_sem_give(&pgrp_gate);
	zassert_ok(k_waitpid(child, NULL, NULL, 0, K_FOREVER));
}

#ifdef CONFIG_SIGNAL
/*
 * Each child blocks until it receives K_SIG_CHLD (default-ignore, so it never
 * terminates the thread asynchronously) and exits 0 only if it got the signal
 * the group fan-out was supposed to deliver. Consuming the signal also keeps the
 * shared signal slab from leaking an undelivered entry on exit.
 */
static void pgrp_sigwait_entry(void *p1, void *p2, void *p3)
{
	struct k_sig_set set;
	struct k_sig_info info;
	int ret = -1;
	int64_t deadline = k_uptime_get() + 5000;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_sig_emptyset(&set);
	k_sig_addset(&set, K_SIG_CHLD);

	/*
	 * Block K_SIG_CHLD so a fan-out delivered before this thread reaches
	 * k_sig_timedwait() stays pending (async delivery discards ignored
	 * signals) and is dequeued synchronously below.
	 */
	k_sig_mask(K_SIG_BLOCK, &set, NULL);

	do {
		ret = k_sig_timedwait(&set, &info, K_MSEC(100));
	} while ((ret < 0) && (k_uptime_get() < deadline));

	k_exit(ret == K_SIG_CHLD ? 0 : 1);
}


ZTEST(kernel_process, test_kill_pgrp)
{
	int wsa = 1;
	int wsb = 1;
	k_pid_t a = &child_a;
	k_pid_t b = &child_b;
	struct sys_clone_args args_a = {
		.entry = pgrp_sigwait_entry,
		.thread = &child_a,
		.stack = child_stack_a,
		.stack_size = STATIC_PROC_STACK_SIZE,
		.prio = K_LOWEST_APPLICATION_THREAD_PRIO,
	};
	struct sys_clone_args args_b = {
		.entry = pgrp_sigwait_entry,
		.thread = &child_b,
		.stack = child_stack_b,
		.stack_size = STATIC_PROC_STACK_SIZE,
		.prio = K_LOWEST_APPLICATION_THREAD_PRIO,
	};

	zassert_ok(sys_clone(&args_a, NULL));
	zassert_ok(sys_clone(&args_b, NULL));

	/* gather both children into a single new process group led by a */
	zassert_ok(sys_setpgid(a, NULL));
	k_pgrp_t grp = k_getpgid(a);

	zassert_ok(k_setpgid(b, grp));
	zassert_equal(k_getpgid(b), grp);
	zassert_equal(sys_pgrp_id(grp), sys_process_id(a));

	/* fan-out delivers to one eligible thread of every group member */
	zassert_ok(k_kill_pgrp(grp, K_SIG_CHLD));

	/* the group variant reaps any child belonging to the group */
	zassert_ok(k_waitpid_pgrp(grp, NULL, &wsa, 0, K_FOREVER));
	zassert_ok(k_waitpid_pgrp(grp, NULL, &wsb, 0, K_FOREVER));
	zassert_true(K_WIFEXITED(wsa) && (K_WEXITSTATUS(wsa) == 0), "member a code=%d",
		     K_WEXITSTATUS(wsa));
	zassert_true(K_WIFEXITED(wsb) && (K_WEXITSTATUS(wsb) == 0), "member b code=%d",
		     K_WEXITSTATUS(wsb));
}

/*
 * A process whose leader blocks the signal but whose second thread does not:
 * k_kill() must deliver to the eligible (second) thread, not the leader.
 */
static K_SEM_DEFINE(elig_ready, 0, 1);
static ZTEST_BMEM volatile k_tid_t elig_recipient;
static ZTEST_BMEM volatile bool elig_handler_ran;

static void elig_handler(int sig, struct k_sig_info *info, void *ucontext)
{
	ARG_UNUSED(sig);
	ARG_UNUSED(info);
	ARG_UNUSED(ucontext);

	elig_recipient = k_current_get();
	elig_handler_ran = true;
}

static void elig_sibling(void *p1, void *p2, void *p3)
{
	struct k_sig_set set;
	struct k_sig_action action = {.handler = elig_handler};
	int64_t deadline = k_uptime_get() + 5000;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* this thread is willing to take K_SIG_USR1; the leader is not */
	k_sig_emptyset(&set);
	k_sig_addset(&set, K_SIG_USR1);
	k_sig_mask(K_SIG_UNBLOCK, &set, NULL);
	k_sig_action(K_SIG_USR1, &action, NULL);

	k_sem_give(&elig_ready);

	while (!elig_handler_ran && (k_uptime_get() < deadline)) {
		k_msleep(1);
	}

	action.handler = K_SIG_DFL;
	k_sig_action(K_SIG_USR1, &action, NULL);
	k_exit((elig_handler_ran && (elig_recipient == k_current_get())) ? 0 : 1);
}

static void elig_leader(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* the leader (a kernel thread) blocks all signals by default; spawn the
	 * sibling into this same process and then idle so it is never eligible
	 */
	k_thread_create(&child_b, child_stack_b, STATIC_PROC_STACK_SIZE, elig_sibling,
			NULL, NULL, NULL, K_LOWEST_APPLICATION_THREAD_PRIO, 0, K_NO_WAIT);
	k_sleep(K_FOREVER);
}

ZTEST(kernel_process, test_kill)
{
	int ws = 1;
	k_pid_t child = &child_a;
	struct sys_clone_args args = {
		.entry = elig_leader,
		.thread = &child_a,
		.stack = child_stack_a,
		.stack_size = STATIC_PROC_STACK_SIZE,
		.prio = K_LOWEST_APPLICATION_THREAD_PRIO,
	};

	/* existence checks: self (explicit and NULL), a bogus handle, a zombie */
	zassert_ok(k_kill(k_getpid(), 0));
	zassert_ok(k_kill(NULL, 0));
	zassert_equal(k_kill((k_pid_t)(uintptr_t)0x4, 0), -ESRCH);
	/* zombies exist until reaped */
	zassert_ok(k_kill(proc_exits, 0));

	/* eligible-thread delivery: leader blocks USR1, sibling accepts it */
	elig_handler_ran = false;
	elig_recipient = NULL;
	zassert_ok(sys_clone(&args, NULL));
	zassert_ok(k_sem_take(&elig_ready, K_SECONDS(5)));

	zassert_ok(k_kill(child, K_SIG_USR1));

	zassert_ok(k_waitpid(child, NULL, &ws, 0, K_FOREVER));
	zassert_true(K_WIFEXITED(ws) && (K_WEXITSTATUS(ws) == 0),
		     "signal was not delivered to the eligible non-leader thread (code=%d)",
		     K_WEXITSTATUS(ws));
	/* the process handle is the leader's handle */
	zassert_not_equal(elig_recipient, child,
			  "signal was delivered to the leader, not the eligible thread");
}

ZTEST(kernel_process, test_kill_all)
{
	int ws = 1;
	k_pid_t child = &child_a;
	struct sys_clone_args args = {
		.entry = elig_leader,
		.thread = &child_a,
		.stack = child_stack_a,
		.stack_size = STATIC_PROC_STACK_SIZE,
		.prio = K_LOWEST_APPLICATION_THREAD_PRIO,
	};

	/*
	 * Broadcast excludes the caller's process and the reserved records.
	 * Other test processes (static topology members, unreaped boot-time
	 * processes) may or may not be alive here, so only the error space of
	 * the existence probe is fixed.
	 */
	int probe = k_kill_all(0);

	zassert_true((probe == 0) || (probe == -ESRCH), "k_kill_all(0) returned %d", probe);

	elig_handler_ran = false;
	elig_recipient = NULL;
	zassert_ok(sys_clone(&args, NULL));
	zassert_ok(k_sem_take(&elig_ready, K_SECONDS(5)));

	/* with another process alive, existence and delivery both succeed */
	zassert_ok(k_kill_all(0));
	zassert_ok(k_kill_all(K_SIG_USR1));

	zassert_ok(k_waitpid(child, NULL, &ws, 0, K_FOREVER));
	zassert_true(K_WIFEXITED(ws) && (K_WEXITSTATUS(ws) == 0),
		     "broadcast did not reach the eligible non-leader thread (code=%d)",
		     K_WEXITSTATUS(ws));
	zassert_not_equal(elig_recipient, child,
			  "broadcast was delivered to the leader, not the eligible thread");
}
#endif /* CONFIG_SIGNAL */

#ifdef CONFIG_SIGNAL

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
	k_pid_t child = &child_b;
	struct sys_clone_args args = {
		.entry = spawn_entry_exit,
		.p1 = (void *)(intptr_t)9,
		.thread = &child_b,
		.stack = child_stack_b,
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
		.thread = &child_a,
		.stack = child_stack_a,
		.stack_size = STATIC_PROC_STACK_SIZE,
		.prio = K_LOWEST_APPLICATION_THREAD_PRIO,
	};

	zassert_ok(sys_clone(&args, NULL));
	zassert_ok(k_waitpid(&child_a, NULL, &ws, 0, K_FOREVER));
	zassert_true(K_WIFEXITED(ws));
	zassert_equal(K_WEXITSTATUS(ws), SIGCHLD_OK, "parent reported 0x%x", K_WEXITSTATUS(ws));
}

/* reap retires an unclaimed CHLD notification: no signal-queue slot may leak */
static void waitpid_chld_drop(void)
{
	int ret;
	int ws = 0;
	struct k_sig_set set;
	struct k_sig_info info;
	int64_t deadline;
	struct sys_clone_args args = {
		.entry = spawn_entry_exit,
		.p1 = (void *)(intptr_t)1,
		.thread = &child_a,
		.stack = child_stack_a,
		.stack_size = STATIC_PROC_STACK_SIZE,
		.prio = K_LOWEST_APPLICATION_THREAD_PRIO,
	};

	k_sig_emptyset(&set);
	k_sig_addset(&set, K_SIG_CHLD);

	for (int i = 0; i < CONFIG_SIGNAL_QUEUE_SIZE + 2; ++i) {
		zassert_ok(sys_clone(&args, NULL));
		/* let the child exit and the notifier queue its CHLD first */
		k_sleep(K_MSEC(20));
		zassert_ok(k_waitpid(&child_a, NULL, NULL, 0, K_FOREVER));
	}

	/*
	 * A later notification still finds a queue slot, and the first claim is
	 * that fresh notification: reap left no stale entries to claim instead.
	 */
	args.p1 = (void *)(intptr_t)42;
	zassert_ok(sys_clone(&args, NULL));
	deadline = k_uptime_get() + 5000;
	do {
		ret = k_sig_timedwait(&set, &info, K_MSEC(100));
	} while ((ret < 0) && (k_uptime_get() < deadline));
	zassert_equal(ret, K_SIG_CHLD);
	zassert_equal(info.pid, &child_a);
	zassert_true(K_WIFEXITED(info.status));
	zassert_equal(K_WEXITSTATUS(info.status), 42);
	zassert_ok(k_waitpid(&child_a, NULL, &ws, 0, K_FOREVER));
	zassert_true(K_WIFEXITED(ws));
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

K_SESSION_DEFINE(topo_session);
K_PROCESS_GROUP_DEFINE(topo_group, &topo_session);
K_PROCESS_GROUP_DEFINE(topo_group2, &topo_session);

static K_SEM_DEFINE(topo_proc_ran, 0, 3);
static K_SEM_DEFINE(topo_thread_ran, 0, 2);
static volatile k_pgrp_t topo_seen_pgrp;
static volatile k_session_t topo_seen_session;
static volatile k_pid_t topo_thread_pid;
static volatile k_pid_t topo_thread2_pid;

static void topo_proc_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	if (p1 != NULL) {
		topo_seen_pgrp = k_getpgid(NULL);
		topo_seen_session = k_getsid(NULL);
	}
	k_sem_give(&topo_proc_ran);
	k_sleep(K_FOREVER);
}

static void topo_thread_entry(void *p1, void *p2, void *p3)
{
	volatile k_pid_t *out = p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	*out = k_getpid();
	k_sem_give(&topo_thread_ran);
	k_sleep(K_FOREVER);
}

K_PROCESS_DEFINE_EXT(topo_proc, &topo_group, STATIC_PROC_STACK_SIZE, topo_proc_entry,
		     (void *)1, NULL, NULL, K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);
K_PROCESS_DEFINE_EXT(topo_proc2, &topo_group, STATIC_PROC_STACK_SIZE, topo_proc_entry, NULL,
		     NULL, NULL, K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);
K_PROCESS_DEFINE_EXT(topo_proc3, &topo_group2, STATIC_PROC_STACK_SIZE, topo_proc_entry, NULL,
		     NULL, NULL, K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);
K_THREAD_DEFINE_EXT(topo_thread, topo_proc, STATIC_PROC_STACK_SIZE, topo_thread_entry,
		    (void *)&topo_thread_pid, NULL, NULL, K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);
K_THREAD_DEFINE_EXT(topo_thread2, topo_proc, STATIC_PROC_STACK_SIZE, topo_thread_entry,
		    (void *)&topo_thread2_pid, NULL, NULL, K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

ZTEST(kernel_process, test_static_topology)
{
	for (int i = 0; i < 3; i++) {
		zassert_ok(k_sem_take(&topo_proc_ran, K_SECONDS(5)));
	}
	for (int i = 0; i < 2; i++) {
		zassert_ok(k_sem_take(&topo_thread_ran, K_SECONDS(5)));
	}

	/* the process was placed in the statically defined group and session */
	zassert_equal(topo_seen_pgrp, &topo_group);
	zassert_equal(topo_seen_session, &topo_session);
	zassert_equal(k_getpgid(topo_proc), &topo_group);
	zassert_equal(k_getsid(topo_proc), &topo_session);

	/* two processes share the first group; the second group holds the third */
	zassert_equal(k_getpgid(topo_proc2), &topo_group);
	zassert_equal(k_getpgid(topo_proc3), &topo_group2);
	zassert_not_equal(k_getpgid(topo_proc2), k_getpgid(topo_proc3));

	/* both groups belong to the one statically defined session */
	zassert_equal(k_getsid(topo_proc2), &topo_session);
	zassert_equal(k_getsid(topo_proc3), &topo_session);

	/* both statically defined member threads joined topo_proc, not their creator */
	zassert_equal(topo_thread_pid, topo_proc);
	zassert_equal(topo_thread2_pid, topo_proc);
	zassert_equal(sys_process_id(topo_thread), sys_process_id(topo_proc));
	zassert_equal(sys_process_id(topo_thread2), sys_process_id(topo_proc));

	/* topology is numbered by the lowest-numbered member: deterministic */
	int p1 = sys_process_id(topo_proc);
	int p2 = sys_process_id(topo_proc2);
	int p3 = sys_process_id(topo_proc3);

	zassert_equal(sys_pgrp_id(&topo_group), MIN(p1, p2));
	zassert_equal(sys_pgrp_id(&topo_group2), p3);
	zassert_equal(sys_session_id(&topo_session), MIN(MIN(p1, p2), p3));
}

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
#ifdef CONFIG_SIGNAL
	/* a kernel caller's explicit mask is honored verbatim, KILL included */
	struct k_sig_set fill;

	k_sig_fillset(&fill);
	bad.sigmask = &fill;
#endif /* CONFIG_SIGNAL */
	zassert_ok(k_clone(&bad, &child));
	zassert_equal(child, &child_b);
#ifdef CONFIG_SIGNAL
	zassert_equal(k_sig_ismember(&child->base.sig.mask, K_SIG_KILL), 1);
	zassert_equal(k_sig_ismember(&child->base.sig.mask, K_SIG_STOP), 1);
#endif /* CONFIG_SIGNAL */
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

	list[0] = k_thread_create(&child_a, child_stack_a, STATIC_PROC_STACK_SIZE, pi_entry_exit,
				  (void *)(intptr_t)5, NULL, NULL,
				  K_LOWEST_APPLICATION_THREAD_PRIO, 0, K_FOREVER);
	list[1] = k_thread_create(&child_b, child_stack_b, STATIC_PROC_STACK_SIZE, pi_entry_wait,
				  NULL, NULL, NULL, K_LOWEST_APPLICATION_THREAD_PRIO, 0,
				  K_FOREVER);

	/* caller-provided record: the kernel initializes, callers allocate */
	zassert_ok(k_process_init(&pi_record, list, 2));

	/* re-initializing a busy record, or reusing a busy leader, errors */
	zassert_equal(k_process_init(&pi_record, list, 2), -EBUSY);
	zassert_equal(k_process_init(&pi_record2, list, 1), -EBUSY);

	/* both listed threads are members and observe the leader handle as pid */
	pi_seen_pid_b = NULL;
	k_thread_start(list[1]);
	/* b records its pid before a's k_exit() can abort it */
	while (pi_seen_pid_b == NULL) {
		k_msleep(1);
	}
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
	k_tid_t tid = k_thread_create(&child_a, child_stack_a, STATIC_PROC_STACK_SIZE,
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

static K_SEM_DEFINE(pgi_gate, 0, 1);
static struct k_process_group pgi_cell;
static struct k_process_group pgi_cell2;

static void pgi_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_sem_take(&pgi_gate, K_FOREVER);
	k_exit(0);
}

ZTEST(kernel_process, test_process_group_init)
{
	k_pid_t child = &child_a;
	struct sys_clone_args args = {
		.entry = pgi_entry,
		.thread = &child_a,
		.stack = child_stack_a,
		.stack_size = STATIC_PROC_STACK_SIZE,
		.prio = K_LOWEST_APPLICATION_THREAD_PRIO,
	};

	zassert_ok(sys_clone(&args, NULL));

	/* a caller-provided cell becomes a new group led by the child */
	zassert_ok(k_process_group_init(&pgi_cell, child));
	zassert_equal(k_getpgid(child), &pgi_cell);
	zassert_equal(pgi_cell.leader, child);

	/* re-initializing a busy group cell errors */
	zassert_equal(k_process_group_init(&pgi_cell, child), -EBUSY);

	/* a process already leading its group does not consume the cell */
	zassert_equal(k_process_group_init(&pgi_cell2, child), -EALREADY);

	/* the group dies with its only member; the caller's cell is reusable */
	k_sem_give(&pgi_gate);
	zassert_ok(k_waitpid(child, NULL, NULL, 0, K_FOREVER));
	zassert_is_null(pgi_cell.session);

	zassert_ok(sys_clone(&args, NULL));
	zassert_ok(k_process_group_init(&pgi_cell, child));
	k_sem_give(&pgi_gate);
	zassert_ok(k_waitpid(child, NULL, NULL, 0, K_FOREVER));
}

static K_SEM_DEFINE(si_ready, 0, 1);
static K_SEM_DEFINE(si_gate, 0, 1);
static struct k_session si_session;
static struct k_session si_session2;
static struct k_process_group si_group;
static struct k_process_group si_group2;
static volatile int si_ret;

static void si_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	si_ret = k_session_init(&si_session, &si_group);
	k_sem_give(&si_ready);
	k_sem_take(&si_gate, K_FOREVER);
	k_exit(0);
}

ZTEST(kernel_process, test_session_init)
{
	k_pid_t child = &child_a;
	struct sys_clone_args args = {
		.entry = si_entry,
		.thread = &child_a,
		.stack = child_stack_a,
		.stack_size = STATIC_PROC_STACK_SIZE,
		.prio = K_LOWEST_APPLICATION_THREAD_PRIO,
	};

	/* the calling process leads its own group: refused */
	zassert_equal(k_session_init(&si_session2, &si_group2), -EPERM);

	/* a non-leader child starts a session in caller-provided cells */
	zassert_ok(sys_clone(&args, NULL));
	k_sem_take(&si_ready, K_FOREVER);
	zassert_ok(si_ret);
	zassert_equal(k_getsid(child), &si_session);
	zassert_equal(si_session.leader, child);

	/* busy cells refuse re-initialization */
	zassert_equal(k_session_init(&si_session, &si_group2), -EBUSY);
	zassert_equal(k_session_init(&si_session2, &si_group), -EBUSY);

	/* the session dies with its last member; the cells are reusable */
	k_sem_give(&si_gate);
	zassert_ok(k_waitpid(child, NULL, NULL, 0, K_FOREVER));
	zassert_is_null(si_group.session);
}

ZTEST(kernel_process, test_spawn_pooled_leader)
{
	int ws = 0;
	int pid;
	k_pid_t child = NULL;
	k_pid_t other = NULL;
	k_pid_t reaped = NULL;
	struct sys_clone_args args = {
		.entry = spawn_entry_exit,
		.p1 = (void *)(intptr_t)9,
		.stack_size = STATIC_PROC_STACK_SIZE,
		.prio = K_LOWEST_APPLICATION_THREAD_PRIO,
	};

	if (!IS_ENABLED(CONFIG_SYS_THREAD)) {
		ztest_test_skip();
	}

	zassert_ok(sys_clone(&args, &child));
	pid = sys_process_id(child);
	zassert_true(pid >= 3);

	/* the zombie is observable without reaping; the leader thread is dead */
	zassert_ok(k_waitpid(child, NULL, NULL, K_PROCESS_WNOWAIT, K_FOREVER));

	/* a join is synchronization only: the TCB stays the pid until reap */
	zassert_ok(k_thread_join(child, K_SECONDS(5)));
	zassert_equal(sys_process_id(child), pid);
	zassert_equal(sys_process_find(pid), child);

	/* the joined, unreaped leader must not be recycled into the pool */
	zassert_ok(sys_clone(&args, &other));
	zassert_true(other != child);
	zassert_true(sys_process_id(other) != pid);
	zassert_ok(k_waitpid(other, NULL, NULL, 0, K_FOREVER));

	/* reap retires the pid and hands the leader back to the pool */
	zassert_ok(k_waitpid(child, &reaped, &ws, 0, K_FOREVER));
	zassert_equal(reaped, child);
	zassert_true(K_WIFEXITED(ws));
	zassert_equal(K_WEXITSTATUS(ws), 9);
	zassert_equal(sys_process_id(child), -ESRCH);

	/* the deferred release ran: pooled spawn still succeeds after reap */
	zassert_ok(sys_clone(&args, &child));
	zassert_ok(k_waitpid(child, NULL, NULL, 0, K_FOREVER));
}

ZTEST_SUITE(kernel_process, NULL, NULL, NULL, NULL, NULL);
