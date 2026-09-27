/*
 * Copyright (c) 2026, Friedt Professional Engineering Services, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/init.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/math_extras.h>
#include <ksched.h>
#include <wait_q.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

/*
 * The three kernel-reserved processes: the idle process (per-CPU idle threads
 * and the boot thread), the init process (parent of user threads and orphaned
 * children), and the kthread process (parent of kernel threads). Their leader
 * handles are self-anchored sentinels (no single thread anchors these groups):
 * valid identity values, never dereferenced. Tentative definitions; the
 * build-time-wired definitions follow the group/session types below.
 */
struct k_process z_proc_idle;
struct k_process z_proc_init;
struct k_process z_proc_kth;

STRUCT_SECTION_START_EXTERN(k_process);
STRUCT_SECTION_END_EXTERN(k_process);
STRUCT_SECTION_START_EXTERN(k_process_group);
STRUCT_SECTION_END_EXTERN(k_process_group);
STRUCT_SECTION_START_EXTERN(k_session);
STRUCT_SECTION_END_EXTERN(k_session);

enum { RESERVED_IDLE, RESERVED_INIT, RESERVED_KTH, RESERVED_NUM };

/* every live session, via k_session.cell_node; the ground truth for validation */
static sys_dlist_t session_active = SYS_DLIST_STATIC_INIT(&session_active);

/* every live process record, wired at build time for the reserved trio */
static sys_dlist_t process_active;

/*
 * The reserved topology is wired entirely at build time: every reference is
 * intra-file, so C initializers express the circular lists directly - the
 * single-TU analog of what gen_process_topology.py does across the image.
 * Each reserved process leads its own group in its own session; init and the
 * kthread process are children of idle.
 */
static struct k_process_group z_pgrp_reserved[RESERVED_NUM];
static struct k_session z_session_reserved[RESERVED_NUM];

#define RESERVED_SESSION(i, proc)                                                                  \
	[i] = {                                                                                    \
		.pgrps = {.head = &z_pgrp_reserved[i].session_node,                                \
			  .tail = &z_pgrp_reserved[i].session_node},                               \
		.leader = (k_tid_t)&(proc),                                                        \
	}

#define RESERVED_PGRP(i, proc)                                                                     \
	[i] = {                                                                                    \
		.members = {.head = &(proc).pgrp_node, .tail = &(proc).pgrp_node},                 \
		.leader = (k_tid_t)&(proc),                                                        \
		.session = &z_session_reserved[i],                                                 \
		.session_node = {.next = &z_session_reserved[i].pgrps,                             \
				 .prev = &z_session_reserved[i].pgrps},                            \
	}

/* the fields every reserved process shares; parent/children/siblings differ */
#define RESERVED_PROCESS(i, proc)                                                                  \
	.leader = (k_tid_t)&(proc), .threads = SYS_DLIST_STATIC_INIT(&(proc).threads),             \
	.pgrp = &z_pgrp_reserved[i],                                                               \
	.pgrp_node = {.next = &z_pgrp_reserved[i].members, .prev = &z_pgrp_reserved[i].members},   \
	.state = K_PROCESS_ACTIVE, .waitq = Z_WAIT_Q_INIT(&(proc).waitq)

static struct k_session z_session_reserved[RESERVED_NUM] = {
	RESERVED_SESSION(RESERVED_IDLE, z_proc_idle),
	RESERVED_SESSION(RESERVED_INIT, z_proc_init),
	RESERVED_SESSION(RESERVED_KTH, z_proc_kth),
};

static struct k_process_group z_pgrp_reserved[RESERVED_NUM] = {
	RESERVED_PGRP(RESERVED_IDLE, z_proc_idle),
	RESERVED_PGRP(RESERVED_INIT, z_proc_init),
	RESERVED_PGRP(RESERVED_KTH, z_proc_kth),
};

struct k_process z_proc_idle = {
	RESERVED_PROCESS(RESERVED_IDLE, z_proc_idle),
	.children = {.head = &z_proc_init.sibling_node, .tail = &z_proc_kth.sibling_node},
	.active_node = {.next = &z_proc_init.active_node, .prev = &process_active},
};

struct k_process z_proc_init = {
	RESERVED_PROCESS(RESERVED_INIT, z_proc_init),
	.parent = &z_proc_idle,
	.sibling_node = {.next = &z_proc_kth.sibling_node, .prev = &z_proc_idle.children},
	.children = SYS_DLIST_STATIC_INIT(&z_proc_init.children),
	.active_node = {.next = &z_proc_kth.active_node, .prev = &z_proc_idle.active_node},
};

struct k_process z_proc_kth = {
	RESERVED_PROCESS(RESERVED_KTH, z_proc_kth),
	.parent = &z_proc_idle,
	.sibling_node = {.next = &z_proc_idle.children, .prev = &z_proc_init.sibling_node},
	.children = SYS_DLIST_STATIC_INIT(&z_proc_kth.children),
	.active_node = {.next = &process_active, .prev = &z_proc_init.active_node},
};

static sys_dlist_t process_active = {
	.head = &z_proc_idle.active_node,
	.tail = &z_proc_kth.active_node,
};

static bool pgrp_is_static(const struct k_process_group *grp)
{
	if ((grp >= z_pgrp_reserved) && (grp < &z_pgrp_reserved[ARRAY_SIZE(z_pgrp_reserved)])) {
		return true;
	}

	/* a group cell carries no marker; topology groups preset .session */
	return (grp >= STRUCT_SECTION_START(k_process_group)) &&
	       (grp < STRUCT_SECTION_END(k_process_group)) && (grp->session != NULL) &&
	       ((grp->session->flags & K_SESSION_FLAG_STATIC) != 0U);
}

static bool session_is_static(const struct k_session *session)
{
	if ((session >= z_session_reserved) &&
	    (session < &z_session_reserved[ARRAY_SIZE(z_session_reserved)])) {
		return true;
	}

	return (session->flags & K_SESSION_FLAG_STATIC) != 0U;
}

static void pgrp_join_locked(struct k_process *proc, struct k_process_group *grp)
{
	proc->pgrp = grp;
	sys_dlist_append(&grp->members, &proc->pgrp_node);
}

/* a group or session exists exactly as long as it has members; statics persist */
static void pgrp_leave_locked(struct k_process *proc)
{
	struct k_process_group *grp = proc->pgrp;
	struct k_session *session;

	if (grp == NULL) {
		return;
	}

	sys_dlist_remove(&proc->pgrp_node);
	proc->pgrp = NULL;

	if (!sys_dlist_is_empty(&grp->members) || pgrp_is_static(grp)) {
		return;
	}

	session = grp->session;
	sys_dlist_remove(&grp->session_node);
	grp->session = NULL;
	/* -EINVAL means caller-owned memory: reusable as-is */
	(void)z_pgrp_cell_free(grp);

	if (!sys_dlist_is_empty(&session->pgrps) || session_is_static(session)) {
		return;
	}

	sys_dlist_remove(&session->cell_node);
	(void)z_session_cell_free(session);
}

static bool pgrp_is_live_locked(const struct k_process_group *grp);
int z_pgrp_validate(k_pgrp_t pgrp);

static struct k_spinlock process_lock;
/*
 * Lock order: process_lock nests inside the scheduler lock (the thread-halt
 * path holds the scheduler lock when it takes process_lock), so nothing may
 * hold process_lock while entering the scheduler. Blocking waiters therefore
 * drop process_lock and pend via z_sched_waitq_pend_if(), with this
 * generation counter closing the dropped-lock window.
 */
static atomic_t process_wait_gen;

static struct k_process *process_self(void)
{
	struct k_process *proc = _current->process;

	return (proc == NULL) ? &z_proc_idle : proc;
}

/* the kernel's own processes: idle, init and the kernel-thread process */
static bool process_is_reserved(const struct k_process *proc)
{
	return (proc == &z_proc_idle) || (proc == &z_proc_init) || (proc == &z_proc_kth);
}

/*
 * Resolve a handle to its record without dereferencing it: leader identity
 * first (it persists until reap), then any live member thread's handle.
 */
static struct k_process *process_ctx_resolve_locked(k_pid_t pid)
{
	struct k_process *proc;

	SYS_DLIST_FOR_EACH_CONTAINER(&process_active, proc, active_node) {
		if (proc->leader == pid) {
			return proc;
		}
	}

	SYS_DLIST_FOR_EACH_CONTAINER(&process_active, proc, active_node) {
		struct k_thread *t;

		SYS_DLIST_FOR_EACH_CONTAINER(&proc->threads, t, process_node) {
			if (t == pid) {
				return proc;
			}
		}
	}

	return NULL;
}

/* the caller holds process_lock (or is the single-threaded boot path) */
void z_kernel_process_init_one(struct k_process *proc, struct k_process *parent)
{
	proc->parent = parent;
	sys_dlist_init(&proc->children);
	sys_dlist_init(&proc->threads);
	z_waitq_init(&proc->waitq);
	proc->state = K_PROCESS_ACTIVE;
#ifdef CONFIG_USERSPACE
	proc->perm_index = z_process_perm_index_alloc();
#endif /* CONFIG_USERSPACE */
	k_object_init(proc);
	sys_dlist_append(&process_active, &proc->active_node);
	if (parent != NULL) {
		sys_dlist_append(&parent->children, &proc->sibling_node);
	}
	if (proc->pgrp != NULL) {
		/* K_PROCESS_DEFINE_EXT: membership was assembled at link time */
	} else if (parent != NULL) {
		pgrp_join_locked(proc, parent->pgrp);
	}
}

static void thread_join_group(struct k_process *proc, struct k_thread *thread)
{
	K_SPINLOCK(&process_lock) {
		thread->process = proc;
		sys_dlist_append(&proc->threads, &thread->process_node);
	}
}

/* the boot thread dropping to user mode leaves the idle process for init */
void z_process_user_mode_enter(struct k_thread *thread)
{
	if (thread->process != &z_proc_idle) {
		return;
	}

	K_SPINLOCK(&process_lock) {
		sys_dlist_remove(&thread->process_node);
		thread->process = &z_proc_init;
		sys_dlist_append(&z_proc_init.threads, &thread->process_node);
	}

#ifdef CONFIG_USERSPACE
	/* objects granted to it as the boot thread stay usable */
	z_process_perms_inherit(&z_proc_idle, &z_proc_init);
#endif /* CONFIG_USERSPACE */
}

void z_process_init_thread(struct k_thread *thread)
{
	struct k_process *proc = NULL;

	if (z_proc_idle.state == K_PROCESS_UNUSED) {
		/* before process_boot_init(); adopted there */
		thread->process = NULL;
		sys_dnode_init(&thread->process_node);
		return;
	}

	if (z_is_idle_thread_object(thread) || (thread == &z_main_thread)) {
		/* the idle process owns exactly the idle threads and the boot thread */
		proc = &z_proc_idle;
	} else {
		/* a statically defined process's leader anchors its own record */
		STRUCT_SECTION_FOREACH(k_process, ctx) {
			if ((ctx->leader == thread) && (ctx->state == K_PROCESS_ACTIVE)) {
				proc = ctx;
				break;
			}
		}
		if (proc == NULL) {
			/* K_THREAD_DEFINE_EXT: membership preset to a static process */
			STRUCT_SECTION_FOREACH(k_process_thread_preset, preset) {
				if (preset->thread != thread) {
					continue;
				}
				STRUCT_SECTION_FOREACH(k_process, ctx) {
					if ((ctx->leader == preset->pid) &&
					    (ctx->state == K_PROCESS_ACTIVE)) {
						proc = ctx;
						break;
					}
				}
				break;
			}
		}
		if (proc == NULL) {
			proc = (_current == NULL) ? NULL : _current->process;
			if ((proc == NULL) || (proc == &z_proc_idle)) {
				/*
				 * The idle process never parents created threads:
				 * kernel threads belong to the kthread process, user
				 * threads to init.
				 */
				proc = ((thread->base.user_options & K_USER) != 0U)
					       ? &z_proc_init
					       : &z_proc_kth;
			}
		}
	}

	sys_dnode_init(&thread->process_node);
	thread_join_group(proc, thread);
}

#ifdef CONFIG_SIGNAL
static _wait_q_t sigchld_notify_wq;
#endif /* CONFIG_SIGNAL */

/* release a reaped process; the caller has already unlinked it */
static void process_free_locked(struct k_process *proc, bool sched_locked)
{
	k_tid_t leader = proc->leader;

#ifdef CONFIG_SCHED_THREAD_USAGE
	if (proc->parent != NULL) {
		proc->parent->child_cycles += proc->self_cycles + proc->child_cycles;
	}
#endif /* CONFIG_SCHED_THREAD_USAGE */
	pgrp_leave_locked(proc);
	sys_dlist_remove(&proc->active_node);
#ifdef CONFIG_SIGNAL
	z_sig_process_drop(proc);
#endif /* CONFIG_SIGNAL */
	z_process_mmap_drop(proc);
#ifdef CONFIG_PROCESS_VM
	if (proc->vm != NULL) {
		z_process_vm_teardown(proc->vm);
		proc->vm = NULL;
	}
#endif /* CONFIG_PROCESS_VM */
#ifdef CONFIG_USERSPACE
	k_object_uninit(proc);
	z_process_perms_all_clear(proc);
	z_process_perm_index_free(proc->perm_index);
	proc->perm_index = -1;
#endif /* CONFIG_USERSPACE */
	z_process_fds_drop(proc);
	proc->state = K_PROCESS_UNUSED;
	/* the single pid-retirement point: the leader handle may be reused now */
	proc->leader = NULL;
	proc->wstatus = 0;

	/* -EINVAL means static or caller-owned memory: reusable as-is */
	(void)z_process_record_free(proc);

	/* The reap ends the last references to the leader's thread and stack.
	 * Nothing is disposed of under process_lock (disposal can reach the
	 * scheduler lock, which halt_thread() holds while it takes process_lock):
	 * the leader is marked, and k_waitpid() drains once the lock is dropped.
	 */
	ARG_UNUSED(sched_locked);
	if (leader != NULL) {
		z_thread_reap(leader, true);
	}
}

static void process_reap_locked(struct k_process *proc, bool sched_locked)
{
	sys_dlist_remove(&proc->sibling_node);
	process_free_locked(proc, sched_locked);
}

void z_process_exit_thread(struct k_thread *thread)
{
	bool last;
	struct k_process *proc = thread->process;

	if (proc == NULL) {
		return;
	}

	K_SPINLOCK(&process_lock) {
		sys_dlist_remove(&thread->process_node);
		thread->process = NULL;
		/* proc->leader stays: the identity until reap, never dereferenced */
#ifdef CONFIG_SCHED_THREAD_USAGE
		{
			k_thread_runtime_stats_t st;

			if (k_thread_runtime_stats_get(thread, &st) == 0) {
				proc->self_cycles += st.total_cycles;
			}
		}
#endif /* CONFIG_SCHED_THREAD_USAGE */
		last = sys_dlist_is_empty(&proc->threads);
		if (!last || (proc == &z_proc_idle) || (proc == &z_proc_init) ||
		    (proc == &z_proc_kth)) {
			K_SPINLOCK_BREAK;
		}

		proc->state = K_PROCESS_ZOMBIE;
		atomic_inc(&process_wait_gen);

		/* orphaned children and zombies are reparented to init */
		while (!sys_dlist_is_empty(&proc->children)) {
			sys_dnode_t *n = sys_dlist_get(&proc->children);
			struct k_process *child =
				CONTAINER_OF(n, struct k_process, sibling_node);

			child->parent = &z_proc_init;
			if (child->state == K_PROCESS_ZOMBIE) {
				/* init auto-reaps; already unlinked by sys_dlist_get() */
				process_free_locked(child, true);
			} else {
				child->flags |= K_PROCESS_FLAG_ADOPTED;
				sys_dlist_append(&z_proc_init.children,
						 &child->sibling_node);
			}
		}

		/* nothing will ever wait for an adopted orphan: init reaps it here */
		if ((proc->flags & K_PROCESS_FLAG_ADOPTED) != 0U) {
			process_reap_locked(proc, true);
			K_SPINLOCK_BREAK;
		}

		/*
		 * Runs in the halt path with the scheduler lock held: only
		 * lock-held scheduler internals may be used here. Wake every
		 * waiter with a zero return value so k_waitpid() re-scans (it
		 * distinguishes a real timeout, -EAGAIN, from a wake).
		 */
		for (struct k_thread *w = z_waitq_head_locked(&proc->parent->waitq);
		     w != NULL; w = z_waitq_head_locked(&proc->parent->waitq)) {
			arch_thread_return_value_set(w, 0);
			z_try_abort_timeout(&w->base.timeout);
			z_sched_wake_thread_locked(w);
		}

#ifdef CONFIG_SIGNAL
		/*
		 * Signal generation cannot run here (scheduler lock held);
		 * defer K_SIG_CHLD emission to the notifier thread. The flag
		 * check and pend in the notifier are under process_lock, so
		 * this set-then-wake cannot be lost.
		 */
		proc->flags |= K_PROCESS_FLAG_SIGCHLD;
		for (struct k_thread *w = z_waitq_head_locked(&sigchld_notify_wq);
		     w != NULL; w = z_waitq_head_locked(&sigchld_notify_wq)) {
			arch_thread_return_value_set(w, 0);
			z_try_abort_timeout(&w->base.timeout);
			z_sched_wake_thread_locked(w);
		}
#endif /* CONFIG_SIGNAL */
	}
}

/* called only on dead threads: a linked membership node is the release queue */
bool z_process_leader_pinned(struct k_thread *thread)
{
	bool pinned = false;
	struct k_process *proc;
	k_spinlock_key_t key = k_spin_lock(&process_lock);

	SYS_DLIST_FOR_EACH_CONTAINER(&process_active, proc, active_node) {
		if (proc->leader == thread) {
			pinned = true;
			break;
		}
	}
	k_spin_unlock(&process_lock, key);

	return pinned;
}

static int process_boot_init(void)
{
	/* single-threaded boot path: the _locked helpers run without the lock */
#ifdef CONFIG_SIGNAL
	z_waitq_init(&sigchld_notify_wq);
#endif /* CONFIG_SIGNAL */

	STRUCT_SECTION_FOREACH(k_session, session) {
		if ((session->flags & K_SESSION_FLAG_STATIC) != 0U) {
			sys_dlist_append(&session_active, &session->cell_node);
		}
	}
#ifdef CONFIG_NATIVE_BUILD
	/*
	 * Native binaries are host-linked after the Zephyr link, so the
	 * post-build topology assembler cannot patch them (it sees a
	 * relocatable object): assemble the static membership lists here.
	 */
	STRUCT_SECTION_FOREACH(k_session, session) {
		if ((session->flags & K_SESSION_FLAG_STATIC) != 0U) {
			sys_dlist_init(&session->pgrps);
		}
	}
	STRUCT_SECTION_FOREACH(k_process_group, grp) {
		if ((grp->session != NULL) &&
		    ((grp->session->flags & K_SESSION_FLAG_STATIC) != 0U)) {
			sys_dlist_init(&grp->members);
			sys_dlist_append(&grp->session->pgrps, &grp->session_node);
		}
	}
	STRUCT_SECTION_FOREACH(k_process, proc) {
		if ((proc->pgrp != NULL) && (proc->pgrp->session != NULL) &&
		    ((proc->pgrp->session->flags & K_SESSION_FLAG_STATIC) != 0U)) {
			sys_dlist_append(&proc->pgrp->members, &proc->pgrp_node);
		}
	}
#endif /* CONFIG_NATIVE_BUILD */
	for (size_t i = 0; i < RESERVED_NUM; i++) {
		sys_dlist_append(&session_active, &z_session_reserved[i].cell_node);
	}
#ifdef CONFIG_USERSPACE
	z_proc_idle.perm_index = z_process_perm_index_alloc();
	z_proc_init.perm_index = z_process_perm_index_alloc();
	z_proc_kth.perm_index = z_process_perm_index_alloc();
#endif /* CONFIG_USERSPACE */

	STRUCT_SECTION_FOREACH(k_process, proc) {
		z_kernel_process_init_one(proc, &z_proc_init);
	}

	return 0;
}

SYS_INIT(process_boot_init, PRE_KERNEL_1, 0);

#ifdef CONFIG_PROCESS_INIT
/*
 * Default entry point of the init process. Applications override this
 * weak function to run as the system's init process. init must not return on a
 * real system, so the default simply sleeps.
 */
__weak void z_process_init_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_sleep(K_FOREVER);
}

static K_THREAD_STACK_DEFINE(init_process_stack, CONFIG_PROCESS_INIT_STACK_SIZE);
static struct k_thread init_process_thread;
static int process_init_start(void)
{
	k_tid_t tid = k_thread_create(&init_process_thread, init_process_stack,
				      CONFIG_PROCESS_INIT_STACK_SIZE, z_process_init_entry, NULL,
				      NULL, NULL, CONFIG_PROCESS_INIT_PRIORITY, 0, K_FOREVER);

	/* a member of init, not its leader: init's identity stays its sentinel */
	K_SPINLOCK(&process_lock) {
		sys_dlist_remove(&tid->process_node);
		tid->process = NULL;
	}
	thread_join_group(&z_proc_init, tid);

	k_thread_start(tid);

	return 0;
}

SYS_INIT(process_init_start, APPLICATION, 0);
#endif /* CONFIG_PROCESS_INIT */

k_pid_t z_impl_k_getpid(void)
{
	return process_self()->leader;
}

#ifdef CONFIG_USERSPACE
static inline k_pid_t z_vrfy_k_getpid(void)
{
	return z_impl_k_getpid();
}
#include <zephyr/syscalls/k_getpid_mrsh.c>
#endif /* CONFIG_USERSPACE */

k_pid_t z_impl_k_getppid(void)
{
	struct k_process *proc = process_self();

	return (proc->parent == NULL) ? proc->leader : proc->parent->leader;
}

#ifdef CONFIG_USERSPACE
static inline k_pid_t z_vrfy_k_getppid(void)
{
	return z_impl_k_getppid();
}
#include <zephyr/syscalls/k_getppid_mrsh.c>
#endif /* CONFIG_USERSPACE */

k_pgrp_t z_impl_k_getpgid(k_pid_t pid)
{
	struct k_process *proc;
	k_pgrp_t ret = NULL;

	K_SPINLOCK(&process_lock) {
		proc = (pid == NULL) ? process_self() : process_ctx_resolve_locked(pid);
		if (proc != NULL) {
			ret = proc->pgrp;
		}
	}

	return ret;
}

#ifdef CONFIG_USERSPACE
static inline k_pgrp_t z_vrfy_k_getpgid(k_pid_t pid)
{
	return z_impl_k_getpgid(pid);
}
#include <zephyr/syscalls/k_getpgid_mrsh.c>
#endif /* CONFIG_USERSPACE */

k_session_t z_impl_k_getsid(k_pid_t pid)
{
	struct k_process *proc;
	k_session_t ret = NULL;

	K_SPINLOCK(&process_lock) {
		proc = (pid == NULL) ? process_self() : process_ctx_resolve_locked(pid);
		if ((proc != NULL) && (proc->pgrp != NULL)) {
			ret = proc->pgrp->session;
		}
	}

	return ret;
}

#ifdef CONFIG_USERSPACE
static inline k_session_t z_vrfy_k_getsid(k_pid_t pid)
{
	return z_impl_k_getsid(pid);
}
#include <zephyr/syscalls/k_getsid_mrsh.c>
#endif /* CONFIG_USERSPACE */

/* is @a grp a live process group? the caller holds process_lock */
static bool pgrp_is_live_locked(const struct k_process_group *grp)
{
	struct k_session *session;
	struct k_process_group *g;

	SYS_DLIST_FOR_EACH_CONTAINER(&session_active, session, cell_node) {
		SYS_DLIST_FOR_EACH_CONTAINER(&session->pgrps, g, session_node) {
			if (g == grp) {
				return true;
			}
		}
	}

	return false;
}

/* is @a session registered as live? the caller holds process_lock */
static bool session_busy_locked(const struct k_session *session)
{
	struct k_session *sp;

	SYS_DLIST_FOR_EACH_CONTAINER(&session_active, sp, cell_node) {
		if (sp == session) {
			return true;
		}
	}

	return false;
}

/* is @a grp a member of any live session? the caller holds process_lock */
static bool pgrp_busy_locked(const struct k_process_group *grp)
{
	struct k_session *sp;
	struct k_process_group *gp;

	SYS_DLIST_FOR_EACH_CONTAINER(&session_active, sp, cell_node) {
		SYS_DLIST_FOR_EACH_CONTAINER(&sp->pgrps, gp, session_node) {
			if (gp == grp) {
				return true;
			}
		}
	}

	return false;
}

int z_impl_k_setpgid(k_pid_t pid, k_pgrp_t pgrp)
{
	int ret = 0;
	struct k_process *self;
	struct k_process *proc;

	if (pgrp == NULL) {
		return -EINVAL;
	}

	K_SPINLOCK(&process_lock) {
		self = process_self();
		proc = (pid == NULL) ? self : process_ctx_resolve_locked(pid);

		if ((proc == NULL) || ((proc != self) && (proc->parent != self))) {
			ret = -ESRCH;
		} else if (proc->pgrp->session->leader == proc->leader) {
			/* a session leader cannot change its process group */
			ret = -EPERM;
		} else if (pgrp == proc->pgrp) {
			/* already a member */
		} else if (!pgrp_is_live_locked(pgrp) || (pgrp->session != proc->pgrp->session)) {
			/* pgrp must name a live group in proc's session */
			ret = -EINVAL;
		} else {
			pgrp_leave_locked(proc);
			pgrp_join_locked(proc, pgrp);
		}
	}

	return ret;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_k_setpgid(k_pid_t pid, k_pgrp_t pgrp)
{
	if (pgrp != NULL) {
		K_OOPS(K_SYSCALL_VERIFY_MSG(z_pgrp_validate(pgrp) == 0, "invalid process group"));
	}
	return z_impl_k_setpgid(pid, pgrp);
}
#include <zephyr/syscalls/k_setpgid_mrsh.c>
#endif /* CONFIG_USERSPACE */

int k_process_group_init(struct k_process_group *grp, k_pid_t pid)
{
	int ret = 0;

	if (grp == NULL) {
		return -EINVAL;
	}

	K_SPINLOCK(&process_lock) {
		struct k_process *self = process_self();
		struct k_process *proc = (pid == NULL) ? self : process_ctx_resolve_locked(pid);

		if (pgrp_busy_locked(grp)) {
			ret = -EBUSY;
			K_SPINLOCK_BREAK;
		}
		if ((proc == NULL) || ((proc != self) && (proc->parent != self))) {
			ret = -ESRCH;
			K_SPINLOCK_BREAK;
		}
		if (proc->pgrp->session->leader == proc->leader) {
			/* a session leader cannot change its process group */
			ret = -EPERM;
			K_SPINLOCK_BREAK;
		}
		if (proc->pgrp->leader == proc->leader) {
			/* already leads its own group; @a grp is not consumed */
			ret = -EALREADY;
			K_SPINLOCK_BREAK;
		}

		sys_dlist_init(&grp->members);
		grp->leader = proc->leader;
		grp->session = proc->pgrp->session;
		sys_dlist_append(&grp->session->pgrps, &grp->session_node);
		pgrp_leave_locked(proc);
		pgrp_join_locked(proc, grp);
	}

	return ret;
}

int k_session_init(struct k_session *session, struct k_process_group *grp)
{
	int ret = 0;

	if ((session == NULL) || (grp == NULL)) {
		return -EINVAL;
	}

	K_SPINLOCK(&process_lock) {
		struct k_process *proc = process_self();

		if (session_busy_locked(session) || pgrp_busy_locked(grp)) {
			ret = -EBUSY;
			K_SPINLOCK_BREAK;
		}
		if (proc->pgrp->leader == proc->leader) {
			/* a process-group leader cannot create a new session */
			ret = -EPERM;
			K_SPINLOCK_BREAK;
		}

		sys_dlist_init(&session->pgrps);
		session->leader = proc->leader;
		session->flags = 0;
		sys_dlist_append(&session_active, &session->cell_node);

		sys_dlist_init(&grp->members);
		grp->leader = proc->leader;
		grp->session = session;
		sys_dlist_append(&session->pgrps, &grp->session_node);

		pgrp_leave_locked(proc);
		pgrp_join_locked(proc, grp);
	}

	return ret;
}

#ifdef CONFIG_SIGNAL
/*
 * Pick one thread of a process to receive a process-directed signal: the first
 * member that has the signal unblocked, but
 * prefer the calling thread when it is an eligible member so that a signal a
 * process sends to itself is observed by the caller. Falls back to the first
 * live member, which holds the signal pending for the group when every member
 * currently blocks it. The caller must hold process_lock (the member list is
 * walked here).
 */
static k_tid_t process_sig_target(struct k_process *proc, int signo)
{
	struct k_thread *t;
	k_tid_t self = k_current_get();

	if ((self->process == proc) && (k_sig_ismember(&self->base.sig.mask, signo) == 0)) {
		return self;
	}

	SYS_DLIST_FOR_EACH_CONTAINER(&proc->threads, t, process_node) {
		if (k_sig_ismember(&t->base.sig.mask, signo) == 0) {
			return t;
		}
	}

	/* every member has the signal masked: pend at process scope */
	return NULL;
}
#endif /* CONFIG_SIGNAL */

/* is @a proc a live process record? the caller holds process_lock */
static bool process_record_valid_locked(const struct k_process *proc)
{
	struct k_process *p;

	SYS_DLIST_FOR_EACH_CONTAINER(&process_active, p, active_node) {
		if (p == proc) {
			return true;
		}
	}

	return false;
}

/*
 * Validate a user-supplied process-group handle: the pointer must name a live
 * group, and every element of its member list must be a valid process record,
 * before z_impl_ code walks it. TOCTOU between this check and the impl is
 * closed by the impl re-taking process_lock.
 */
int z_pgrp_validate(k_pgrp_t pgrp)
{
	int ret = 0;
	struct k_process *member;

	K_SPINLOCK(&process_lock) {
		if (!pgrp_is_live_locked(pgrp)) {
			ret = -EINVAL;
			K_SPINLOCK_BREAK;
		}
		SYS_DLIST_FOR_EACH_CONTAINER(&pgrp->members, member, pgrp_node) {
			if (!process_record_valid_locked(member)) {
				ret = -EINVAL;
				/* K_SPINLOCK_BREAK would resume this member walk */
				break;
			}
		}
	}

	return ret;
}

int z_impl_k_kill_pgrp(k_pgrp_t pgrp, int signo)
{
	int ret = -ESRCH;
	int found = 0;
	size_t skip = 0;
	bool done = false;
	struct k_thread *targets[8];

	/*
	 * Resolve deliverable threads (one per member process) under the lock
	 * in bounded batches, and signal between batches with the lock dropped:
	 * k_sig_queue() allocates and takes its own locks, and group size is
	 * unbounded at kernel build time. The skip cursor keeps each position
	 * visited once; members joining or leaving between batches are the
	 * usual signal-delivery TOCTOU.
	 */
	k_spinlock_key_t key = k_spin_lock(&process_lock);

	if (pgrp == NULL) {
		pgrp = process_self()->pgrp;
	}

	while (!done) {
		int n = 0;
		size_t pos = 0;
		struct k_process *proc;

		done = true;
		SYS_DLIST_FOR_EACH_CONTAINER(&pgrp->members, proc, pgrp_node) {
			k_tid_t tgt;

			if (pos++ < skip) {
				continue;
			}
			if (n >= (int)ARRAY_SIZE(targets)) {
				done = false;
				break;
			}
			skip++;
			if ((proc->state == K_PROCESS_UNUSED) ||
			    sys_dlist_is_empty(&proc->threads)) {
				continue;
			}
#ifdef CONFIG_SIGNAL
			tgt = process_sig_target(proc, signo);
			if (tgt == NULL) {
				/* every member masked: pend at process scope,
				 * under the lock so reap cannot race the queue
				 */
				if (z_sig_queue_process(proc, signo, (union k_sig_val){0}) == 0) {
					ret = 0;
				}
				continue;
			}
#else
			tgt = SYS_DLIST_PEEK_HEAD_CONTAINER(&proc->threads, tgt, process_node);
#endif /* CONFIG_SIGNAL */
			targets[n++] = tgt;
		}
		k_spin_unlock(&process_lock, key);

		found += n;
#ifdef CONFIG_SIGNAL
		for (int i = 0; i < n; i++) {
			int err = z_sig_queue_thread(targets[i], signo, (union k_sig_val){0});

			if (err == -EINVAL) {
				return -EINVAL;
			}
			if (err == 0) {
				ret = 0;
			}
		}
#endif /* CONFIG_SIGNAL */

		if (!done) {
			key = k_spin_lock(&process_lock);
		}
	}

#ifndef CONFIG_SIGNAL
	ARG_UNUSED(signo);
	ret = (found > 0) ? -ENOTSUP : -ESRCH;
#endif /* !CONFIG_SIGNAL */

	return ret;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_k_kill_pgrp(k_pgrp_t pgrp, int signo)
{
	if (pgrp != NULL) {
		K_OOPS(K_SYSCALL_VERIFY_MSG(z_pgrp_validate(pgrp) == 0, "invalid process group"));
	}
	return z_impl_k_kill_pgrp(pgrp, signo);
}
#include <zephyr/syscalls/k_kill_pgrp_mrsh.c>
#endif /* CONFIG_USERSPACE */

static int process_kill(k_pid_t pid, int signo, union k_sig_val value)
{
	bool found = false;
	bool live = false;
	bool pended = false;
	int ret = 0;
#ifdef CONFIG_SIGNAL
	k_tid_t tid = NULL;
#endif /* CONFIG_SIGNAL */
	struct k_process *proc;

	K_SPINLOCK(&process_lock) {
		proc = (pid == NULL) ? process_self() : process_ctx_resolve_locked(pid);
		if (proc == NULL) {
			K_SPINLOCK_BREAK;
		}
		found = true;
		live = !sys_dlist_is_empty(&proc->threads);
#ifdef CONFIG_SIGNAL
		if ((signo != 0) && live) {
			tid = process_sig_target(proc, signo);
			if (tid == NULL) {
				/* every member masked: pend at process scope,
				 * under the lock so reap cannot race the queue
				 */
				ret = z_sig_queue_process(proc, signo, value);
				pended = true;
			}
		}
#endif /* CONFIG_SIGNAL */
	}

	if (!found) {
		return -ESRCH;
	}
	if (signo == 0) {
		/* existence check only; zombies exist until reaped */
		return 0;
	}
	if (!live) {
		return -ESRCH;
	}
#ifdef CONFIG_SIGNAL
	if (pended) {
		return ret;
	}
	return z_sig_queue_thread(tid, signo, value);
#else
	ARG_UNUSED(pended);
	ARG_UNUSED(ret);
	ARG_UNUSED(value);
	return -ENOTSUP;
#endif /* CONFIG_SIGNAL */
}

int z_impl_k_kill(k_pid_t pid, int signo)
{
	return process_kill(pid, signo, (union k_sig_val){0});
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_k_kill(k_pid_t pid, int signo)
{
	/* identity-value resolution validates the handle; never dereferenced */
	return z_impl_k_kill(pid, signo);
}
#include <zephyr/syscalls/k_kill_mrsh.c>
#endif /* CONFIG_USERSPACE */

static FUNC_NORETURN void process_exit(int wstatus)
{
	struct k_thread *victim;
	struct k_process *proc = process_self();

	k_spinlock_key_t key = k_spin_lock(&process_lock);

	if (proc->state == K_PROCESS_ACTIVE) {
		proc->state = K_PROCESS_EXITING;
		proc->wstatus = wstatus;
	}

	/* abort every other member; each drops out via z_process_exit_thread() */
	while (true) {
		struct k_thread *t;

		victim = NULL;
		SYS_DLIST_FOR_EACH_CONTAINER(&proc->threads, t, process_node) {
			if (t != _current) {
				victim = t;
				break;
			}
		}
		if (victim == NULL) {
			break;
		}
		k_spin_unlock(&process_lock, key);
		k_thread_abort(victim);
		key = k_spin_lock(&process_lock);
	}

	/* zombification happens in the halt path when the last member exits */
	proc->state = K_PROCESS_EXITING;
	k_spin_unlock(&process_lock, key);

	/* POSIX: mappings and descriptors go at termination, not reap; may block here */
	z_process_mmap_exit();
	z_process_fds_exit();

	k_thread_abort(k_current_get());

	CODE_UNREACHABLE;
}

int z_impl_k_sigqueue(k_pid_t pid, int signo, union k_sig_val value)
{
	return process_kill(pid, signo, value);
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_k_sigqueue(k_pid_t pid, int signo, union k_sig_val value)
{
	/* identity-value resolution validates the handle; never dereferenced */
	return z_impl_k_sigqueue(pid, signo, value);
}
#include <zephyr/syscalls/k_sigqueue_mrsh.c>
#endif /* CONFIG_USERSPACE */

int z_impl_k_kill_all(int signo)
{
	int ret = -ESRCH;
	size_t skip = 0;
	bool done = false;
	struct k_thread *targets[8];
	struct k_process *self = NULL;

	/*
	 * Broadcast: every registered process except the reserved system
	 * records and the caller's own process, batched exactly as
	 * k_kill_pgrp() batches group members.
	 */
	k_spinlock_key_t key = k_spin_lock(&process_lock);

	self = process_self();

	while (!done) {
		int n = 0;
		size_t pos = 0;
		struct k_process *proc;

		done = true;
		SYS_DLIST_FOR_EACH_CONTAINER(&process_active, proc, active_node) {
			k_tid_t tgt;

			if (pos++ < skip) {
				continue;
			}
			if (n >= (int)ARRAY_SIZE(targets)) {
				done = false;
				break;
			}
			skip++;
			if (process_is_reserved(proc) || (proc == self) ||
			    (proc->state == K_PROCESS_UNUSED) ||
			    sys_dlist_is_empty(&proc->threads)) {
				continue;
			}
			if (signo == 0) {
				/* existence check only */
				ret = 0;
				continue;
			}
#ifdef CONFIG_SIGNAL
			tgt = process_sig_target(proc, signo);
			if (tgt == NULL) {
				/* every member masked: pend at process scope,
				 * under the lock so reap cannot race the queue
				 */
				if (z_sig_queue_process(proc, signo, (union k_sig_val){0}) == 0) {
					ret = 0;
				}
				continue;
			}
#else
			tgt = SYS_DLIST_PEEK_HEAD_CONTAINER(&proc->threads, tgt, process_node);
#endif /* CONFIG_SIGNAL */
			targets[n++] = tgt;
		}
		k_spin_unlock(&process_lock, key);

#ifdef CONFIG_SIGNAL
		for (int i = 0; i < n; i++) {
			int err = z_sig_queue_thread(targets[i], signo, (union k_sig_val){0});

			if (err == -EINVAL) {
				return -EINVAL;
			}
			if (err == 0) {
				ret = 0;
			}
		}
#else
		if (n > 0) {
			ret = -ENOTSUP;
		}
#endif /* CONFIG_SIGNAL */

		if (!done) {
			key = k_spin_lock(&process_lock);
		}
	}

	return ret;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_k_kill_all(int signo)
{
	return z_impl_k_kill_all(signo);
}
#include <zephyr/syscalls/k_kill_all_mrsh.c>
#endif /* CONFIG_USERSPACE */

void z_impl_k_exit(int code)
{
	process_exit(K_WSTATUS_EXITED(code));
}

void z_process_exit_signaled(int sig)
{
	if (process_is_reserved(process_self())) {
		/* the kernel's own processes are the system: only the thread goes */
		k_thread_abort(k_current_get());
		CODE_UNREACHABLE;
	}

	process_exit(K_WSTATUS_SIGNALED(sig));
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_k_exit(int code)
{
	z_impl_k_exit(code);
}
#include <zephyr/syscalls/k_exit_mrsh.c>
#endif /* CONFIG_USERSPACE */

static bool waitpid_selects(struct k_process *child, k_pid_t pid, k_pgrp_t pgrp, bool by_pgrp)
{
	if (by_pgrp) {
		return child->pgrp == pgrp;
	}
	return (pid == NULL) || (child->leader == pid);
}

static int waitpid_common(k_pid_t pid, k_pgrp_t pgrp, bool by_pgrp, k_pid_t *reaped, int *wstatus,
			  uint32_t options, k_timeout_t timeout)
{
	int ret;
	struct k_process *child;
	struct k_process *self = process_self();
	k_timepoint_t end = sys_timepoint_calc(timeout);

	if ((options & (K_PROCESS_WUNTRACED | K_PROCESS_WCONTINUED)) != 0) {
		return -ENOTSUP;
	}

	while (true) {
		bool have_match = false;
		struct k_process *zombie = NULL;
		atomic_val_t gen = atomic_get(&process_wait_gen);
			bool reaped_one = false;
	k_spinlock_key_t key = k_spin_lock(&process_lock);

		SYS_DLIST_FOR_EACH_CONTAINER(&self->children, child, sibling_node) {
			if (!waitpid_selects(child, pid, pgrp, by_pgrp)) {
				continue;
			}
			have_match = true;
			if (child->state == K_PROCESS_ZOMBIE) {
				zombie = child;
				break;
			}
		}

		if (zombie != NULL) {
			if (reaped != NULL) {
				*reaped = zombie->leader;
			}
			if (wstatus != NULL) {
				*wstatus = zombie->wstatus;
			}
			if ((options & K_PROCESS_WNOWAIT) == 0) {
				process_reap_locked(zombie, false);
				reaped_one = true;
			}
			ret = 0;
		} else if (!have_match) {
			ret = -ECHILD;
		} else if ((options & K_PROCESS_WNOHANG) != 0) {
			ret = -EAGAIN;
		} else {
			ret = 1;
		}

		k_spin_unlock(&process_lock, key);

		if (reaped_one) {
			/* the reaped leader's objects go back to their pools now */
			z_thread_graveyard_flush();
		}
		if (ret <= 0) {
			return ret;
		}

		/* pend without process_lock (see the lock-order comment above) */
		ret = z_sched_waitq_pend_if(&self->waitq, sys_timepoint_timeout(end),
					    &process_wait_gen, gen);
		if (ret == -EAGAIN) {
			return ret;
		}
	}
}

int z_impl_k_waitpid(k_pid_t pid, k_pid_t *reaped, int *wstatus, uint32_t options,
		     k_timeout_t timeout)
{
	return waitpid_common(pid, NULL, false, reaped, wstatus, options, timeout);
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_k_waitpid(k_pid_t pid, k_pid_t *reaped, int *wstatus, uint32_t options,
				   k_timeout_t timeout)
{
	/*
	 * The child selector is an identity value that is never dereferenced,
	 * and the child scan is confined to the caller's own children, so no
	 * object validation is required.
	 */
	if (reaped != NULL) {
		K_OOPS(K_SYSCALL_MEMORY_WRITE(reaped, sizeof(*reaped)));
	}
	if (wstatus != NULL) {
		K_OOPS(K_SYSCALL_MEMORY_WRITE(wstatus, sizeof(*wstatus)));
	}

	return z_impl_k_waitpid(pid, reaped, wstatus, options, timeout);
}
#include <zephyr/syscalls/k_waitpid_mrsh.c>
#endif /* CONFIG_USERSPACE */

int z_impl_k_waitpid_pgrp(k_pgrp_t pgrp, k_pid_t *reaped, int *wstatus, uint32_t options,
			  k_timeout_t timeout)
{
	if (pgrp == NULL) {
		K_SPINLOCK(&process_lock) {
			pgrp = process_self()->pgrp;
		}
	}

	return waitpid_common(NULL, pgrp, true, reaped, wstatus, options, timeout);
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_k_waitpid_pgrp(k_pgrp_t pgrp, k_pid_t *reaped, int *wstatus,
					uint32_t options, k_timeout_t timeout)
{
	if (pgrp != NULL) {
		K_OOPS(K_SYSCALL_VERIFY_MSG(z_pgrp_validate(pgrp) == 0, "invalid process group"));
	}
	if (reaped != NULL) {
		K_OOPS(K_SYSCALL_MEMORY_WRITE(reaped, sizeof(*reaped)));
	}
	if (wstatus != NULL) {
		K_OOPS(K_SYSCALL_MEMORY_WRITE(wstatus, sizeof(*wstatus)));
	}

	return z_impl_k_waitpid_pgrp(pgrp, reaped, wstatus, options, timeout);
}
#include <zephyr/syscalls/k_waitpid_pgrp_mrsh.c>
#endif /* CONFIG_USERSPACE */

int z_impl_k_process_init(struct k_process *proc, const k_tid_t *threads, size_t num_threads)
{
	int ret = 0;

	if ((proc == NULL) || (threads == NULL) || (num_threads == 0)) {
		return -EINVAL;
	}
	for (size_t i = 0; i < num_threads; i++) {
		if (threads[i] == NULL) {
			return -EINVAL;
		}
	}

	K_SPINLOCK(&process_lock) {
		struct k_process *p;
		/* an unreaped record, or one never reaped, is busy */
		bool busy = proc->state != K_PROCESS_UNUSED;

		if (!busy) {
			/* an unreaped process still owns its leader handle */
			SYS_DLIST_FOR_EACH_CONTAINER(&process_active, p, active_node) {
				for (size_t i = 0; i < num_threads; i++) {
					if (p->leader == threads[i]) {
						busy = true;
						break;
					}
				}
				if (busy) {
					break;
				}
			}
		}
		if (busy) {
			ret = -EBUSY;
			K_SPINLOCK_BREAK;
		}

		proc->leader = threads[0];
		proc->pgrp = NULL;
		proc->flags = 0;
		proc->wstatus = 0;
		z_kernel_process_init_one(proc, process_self());

		/* adopt the listed threads out of their creation-inherited groups */
		for (size_t i = 0; i < num_threads; i++) {
			struct k_thread *t = threads[i];

			if (t->process != NULL) {
				sys_dlist_remove(&t->process_node);
			}
			t->process = proc;
			sys_dlist_append(&proc->threads, &t->process_node);
		}
	}

	return ret;
}

#ifdef CONFIG_USERSPACE
/* bounds the on-stack copy of the caller's thread list */
#define PROCESS_INIT_USER_THREADS_MAX 16

static inline int z_vrfy_k_process_init(struct k_process *proc, const k_tid_t *threads,
					size_t num_threads)
{
	k_tid_t copy[PROCESS_INIT_USER_THREADS_MAX];

	K_OOPS(K_SYSCALL_OBJ_NEVER_INIT(proc, K_OBJ_PROCESS));
	K_OOPS(K_SYSCALL_VERIFY_MSG((num_threads > 0) && (num_threads <= ARRAY_SIZE(copy)),
				    "invalid thread count %zu", num_threads));
	K_OOPS(k_usermode_from_copy(copy, threads, num_threads * sizeof(k_tid_t)));
	for (size_t i = 0; i < num_threads; i++) {
		/* the caller must hold permission on every listed thread */
		K_OOPS(K_SYSCALL_OBJ(copy[i], K_OBJ_THREAD));
	}

	return z_impl_k_process_init(proc, copy, num_threads);
}
#include <zephyr/syscalls/k_process_init_mrsh.c>
#endif /* CONFIG_USERSPACE */

int z_impl_k_clone(const struct k_clone_args *args, k_tid_t *child)
{
	int ret;
	k_tid_t tid;
	bool thread_tier;

	if ((args == NULL) || (args->entry == NULL) || (args->thread == NULL) ||
	    (args->stack == NULL) || (args->stack_size == 0) ||
	    ((args->flags & ~(K_CLONE_THREAD | K_CLONE_PAUSED)) != 0U)) {
		return -EINVAL;
	}
	thread_tier = (args->flags & K_CLONE_THREAD) != 0U;
	if (thread_tier != (args->process == NULL)) {
		return -EINVAL;
	}
	if ((args->sigmask != NULL) && !IS_ENABLED(CONFIG_SIGNAL)) {
		return -EINVAL;
	}

	tid = k_thread_create(args->thread, args->stack, args->stack_size, args->entry, args->p1,
			      args->p2, args->p3, args->prio, args->options, K_FOREVER);

	if (!thread_tier) {
		ret = z_impl_k_process_init(args->process, &tid, 1);
		if (ret < 0) {
			k_thread_abort(tid);
			return ret;
		}
	}

#ifdef CONFIG_SIGNAL
	if (args->sigmask != NULL) {
		tid->base.sig.mask = *args->sigmask;
#ifdef CONFIG_USERSPACE
		if (k_is_in_user_syscall()) {
			/* as k_sig_mask(): user-mode requests cannot mask KILL or STOP */
			tid->base.sig.mask.sig[0] &=
				~(BIT64(K_SIG_KILL - 1) | BIT64(K_SIG_STOP - 1));
		}
#endif /* CONFIG_USERSPACE */
	}
#endif /* CONFIG_SIGNAL */

	if (child != NULL) {
		*child = tid;
	}

	if ((args->flags & K_CLONE_PAUSED) == 0U) {
		k_thread_start(tid);
	}

	return 0;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_k_clone(const struct k_clone_args *args, k_tid_t *child)
{
	struct k_clone_args copy;
	size_t total_size, stack_obj_size;
	struct k_object *stack_object;
#ifdef CONFIG_SIGNAL
	struct k_sig_set mask_copy;
#endif /* CONFIG_SIGNAL */

	K_OOPS(k_usermode_from_copy(&copy, args, sizeof(copy)));
	if (child != NULL) {
		K_OOPS(K_SYSCALL_MEMORY_WRITE(child, sizeof(*child)));
	}
#ifdef CONFIG_SIGNAL
	if (copy.sigmask != NULL) {
		K_OOPS(k_usermode_from_copy(&mask_copy, copy.sigmask, sizeof(mask_copy)));
		copy.sigmask = &mask_copy;
	}
#endif /* CONFIG_SIGNAL */

	if (copy.process != NULL) {
		K_OOPS(K_SYSCALL_OBJ_NEVER_INIT(copy.process, K_OBJ_PROCESS));
	}

	/* mirror z_vrfy_k_thread_create(): the thread is created via k_thread_create() */
	K_OOPS(K_SYSCALL_OBJ_NEVER_INIT(copy.thread, K_OBJ_THREAD));
	stack_object = k_object_find(copy.stack);
	K_OOPS(K_SYSCALL_VERIFY_MSG(k_object_validation_check(stack_object, copy.stack,
							      K_OBJ_THREAD_STACK_ELEMENT,
							      _OBJ_INIT_FALSE) == 0,
				    "bad stack object"));
	K_OOPS(K_SYSCALL_VERIFY_MSG(!size_add_overflow(K_THREAD_STACK_RESERVED, copy.stack_size,
						       &total_size),
				    "stack size overflow (%zu+%zu)", copy.stack_size,
				    K_THREAD_STACK_RESERVED));
#ifdef CONFIG_GEN_PRIV_STACKS
	stack_obj_size = stack_object->data.stack_data->size;
#else
	stack_obj_size = stack_object->data.stack_size;
#endif /* CONFIG_GEN_PRIV_STACKS */
	K_OOPS(K_SYSCALL_VERIFY_MSG(total_size <= stack_obj_size,
				    "stack size %zu is too big, max is %zu", total_size,
				    stack_obj_size));
	K_OOPS(K_SYSCALL_VERIFY(copy.options & K_USER));
	K_OOPS(K_SYSCALL_VERIFY(!(copy.options & K_ESSENTIAL)));
	K_OOPS(K_SYSCALL_VERIFY(_is_valid_prio(copy.prio, NULL)));
	K_OOPS(K_SYSCALL_VERIFY(z_is_prio_lower_or_equal(copy.prio, _current->base.prio)));

	return z_impl_k_clone(&copy, child);
}
#include <zephyr/syscalls/k_clone_mrsh.c>
#endif /* CONFIG_USERSPACE */

int z_impl_k_process_prune(void)
{
	k_tid_t victim;

	/* k_thread_abort() cannot run under process_lock: drop and retake */
	while (true) {
		victim = NULL;
		K_SPINLOCK(&process_lock) {
			struct k_thread *t;
			struct k_process *proc = process_self();

			SYS_DLIST_FOR_EACH_CONTAINER(&proc->threads, t, process_node) {
				if (t != _current) {
					victim = t;
					break;
				}
			}
		}
		if (victim == NULL) {
			break;
		}
		k_thread_abort(victim);
	}

	return 0;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_k_process_prune(void)
{
	return z_impl_k_process_prune();
}
#include <zephyr/syscalls/k_process_prune_mrsh.c>
#endif /* CONFIG_USERSPACE */

int z_impl_k_process_cpu_stats(uint64_t *self, uint64_t *children)
{
#ifdef CONFIG_SCHED_THREAD_USAGE
	uint64_t s = 0;
	uint64_t c = 0;

	K_SPINLOCK(&process_lock) {
		struct k_process *proc = process_self();
		struct k_thread *t;

		s = proc->self_cycles;
		c = proc->child_cycles;
		SYS_DLIST_FOR_EACH_CONTAINER(&proc->threads, t, process_node) {
			k_thread_runtime_stats_t st;

			if (k_thread_runtime_stats_get(t, &st) == 0) {
				s += st.total_cycles;
			}
		}
	}

	if (self != NULL) {
		*self = s;
	}
	if (children != NULL) {
		*children = c;
	}

	return 0;
#else
	if (self != NULL) {
		*self = 0;
	}
	if (children != NULL) {
		*children = 0;
	}
	return 0;
#endif /* CONFIG_SCHED_THREAD_USAGE */
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_k_process_cpu_stats(uint64_t *self, uint64_t *children)
{
	if (self != NULL) {
		K_OOPS(K_SYSCALL_MEMORY_WRITE(self, sizeof(*self)));
	}
	if (children != NULL) {
		K_OOPS(K_SYSCALL_MEMORY_WRITE(children, sizeof(*children)));
	}
	return z_impl_k_process_cpu_stats(self, children);
}
#include <zephyr/syscalls/k_process_cpu_stats_mrsh.c>
#endif /* CONFIG_USERSPACE */

#ifdef CONFIG_SIGNAL
static struct k_process *sigchld_pending_get_locked(void)
{
	struct k_process *proc;

	SYS_DLIST_FOR_EACH_CONTAINER(&process_active, proc, active_node) {
		if ((proc->flags & K_PROCESS_FLAG_SIGCHLD) != 0) {
			return proc;
		}
	}

	return NULL;
}

static void sigchld_notifier(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		k_pid_t pid;
		int status;
		bool exited;
		k_tid_t target;
		struct k_process *child;
		struct k_process *parent;
		atomic_val_t gen = atomic_get(&process_wait_gen);

		k_spinlock_key_t key = k_spin_lock(&process_lock);

		child = sigchld_pending_get_locked();
		if (child == NULL) {
			k_spin_unlock(&process_lock, key);
			/* pend without process_lock (see the lock-order comment) */
			(void)z_sched_waitq_pend_if(&sigchld_notify_wq, K_FOREVER,
						    &process_wait_gen, gen);
			continue;
		}

		child->flags &= (uint8_t)~K_PROCESS_FLAG_SIGCHLD;
		pid = child->leader;
		status = child->wstatus;
		exited = K_WIFEXITED(status);
		parent = child->parent;
		target = (parent == NULL) ? NULL : process_sig_target(parent, K_SIG_CHLD);
		if ((target == NULL) && (parent != NULL)) {
			/* pend at the parent's process scope, under the lock
			 * so reap cannot race the queue
			 */
			(void)z_sig_queue_chld(NULL, parent, pid, status, exited);
		}
		k_spin_unlock(&process_lock, key);

		if (target != NULL) {
			/* best effort: the target may exit, or the queue may be full */
			(void)z_sig_queue_chld(target, NULL, pid, status, exited);
		}
	}
}

K_THREAD_DEFINE(z_process_sigchld_notifier, CONFIG_PROCESS_SIGCHLD_NOTIFIER_STACK_SIZE,
		sigchld_notifier, NULL, NULL, NULL, 0, 0, 0);
#endif /* CONFIG_SIGNAL */
