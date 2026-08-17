/*
 * Copyright (c) 2026, Friedt Professional Engineering Services, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * System-layer process support: everything the kernel deliberately does not
 * own because it involves runtime allocation or integer namespaces. Records
 * and group/session cells are handed to the kernel already numbered; the
 * frees retire the number. Frees may run in the thread-halt path (under the
 * scheduler lock, where the heap must not be used), so heap reclamation is
 * deferred to graveyards drained on the next allocation.
 */

#include <errno.h>
#include <string.h>

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/sys/elastipool.h>
#include <zephyr/sys/process.h>
#include <zephyr/sys/thread.h>
#include <kernel_internal.h>
#include <ksched.h>

STRUCT_SECTION_START_EXTERN(k_process);
STRUCT_SECTION_END_EXTERN(k_process);
STRUCT_SECTION_START_EXTERN(k_process_group);
STRUCT_SECTION_END_EXTERN(k_process_group);
STRUCT_SECTION_START_EXTERN(k_session);
STRUCT_SECTION_END_EXTERN(k_session);

/*
 * Numbering tables. Entries hold record pointers, dereferenced only while
 * live: pooled entries are removed at free, strictly before the record can
 * be recycled; static records never recycle, and their entries die with the
 * record's K_PROCESS_UNUSED state at reap.
 */
struct sys_process_id_entry {
	struct k_process *proc;
	int id;
};

struct sys_pgrp_id_entry {
	struct k_process_group *grp;
	int id;
};

struct sys_session_id_entry {
	struct k_session *session;
	int id;
};

static struct k_spinlock id_lock;
static struct sys_process_id_entry process_ids[CONFIG_SYS_PROCESS_IDS];
static struct sys_pgrp_id_entry pgrp_ids[CONFIG_SYS_PROCESS_IDS];
static struct sys_session_id_entry session_ids[CONFIG_SYS_PROCESS_IDS];

static bool process_entry_live(const struct sys_process_id_entry *entry)
{
	return (entry->proc != NULL) && (entry->proc->state != K_PROCESS_UNUSED);
}

static int lowest_free_process_id_locked(void)
{
	for (int id = 0;; id++) {
		bool used = false;

		for (size_t i = 0; i < ARRAY_SIZE(process_ids); i++) {
			if (process_entry_live(&process_ids[i]) && (process_ids[i].id == id)) {
				used = true;
				break;
			}
		}
		if (!used) {
			return id;
		}
	}
}

static void process_id_assign(struct k_process *proc)
{
	K_SPINLOCK(&id_lock) {
		struct sys_process_id_entry *entry = NULL;

		for (size_t i = 0; i < ARRAY_SIZE(process_ids); i++) {
			if (!process_entry_live(&process_ids[i])) {
				entry = &process_ids[i];
				if (entry->proc == NULL) {
					break;
				}
			}
		}
		if (entry != NULL) {
			entry->id = lowest_free_process_id_locked();
			entry->proc = proc;
		}
	}
}

static void process_id_retire(struct k_process *proc)
{
	K_SPINLOCK(&id_lock) {
		for (size_t i = 0; i < ARRAY_SIZE(process_ids); i++) {
			if (process_ids[i].proc == proc) {
				process_ids[i].proc = NULL;
				break;
			}
		}
	}
}

static int process_number(struct k_process *proc)
{
	int ret = 0;

	K_SPINLOCK(&id_lock) {
		for (size_t i = 0; i < ARRAY_SIZE(process_ids); i++) {
			if (process_ids[i].proc == proc) {
				ret = process_ids[i].id;
				break;
			}
		}
	}

	return ret;
}

static void pgrp_id_assign(struct k_process_group *grp, int id)
{
	K_SPINLOCK(&id_lock) {
		for (size_t i = 0; i < ARRAY_SIZE(pgrp_ids); i++) {
			if (pgrp_ids[i].grp == NULL) {
				pgrp_ids[i].id = id;
				pgrp_ids[i].grp = grp;
				break;
			}
		}
	}
}

static void pgrp_id_retire(struct k_process_group *grp)
{
	K_SPINLOCK(&id_lock) {
		for (size_t i = 0; i < ARRAY_SIZE(pgrp_ids); i++) {
			if (pgrp_ids[i].grp == grp) {
				pgrp_ids[i].grp = NULL;
				break;
			}
		}
	}
}

static void session_id_assign(struct k_session *session, int id)
{
	K_SPINLOCK(&id_lock) {
		for (size_t i = 0; i < ARRAY_SIZE(session_ids); i++) {
			if (session_ids[i].session == NULL) {
				session_ids[i].id = id;
				session_ids[i].session = session;
				break;
			}
		}
	}
}

static void session_id_retire(struct k_session *session)
{
	K_SPINLOCK(&id_lock) {
		for (size_t i = 0; i < ARRAY_SIZE(session_ids); i++) {
			if (session_ids[i].session == session) {
				session_ids[i].session = NULL;
				break;
			}
		}
	}
}

int z_impl_sys_process_id(k_pid_t pid)
{
	int ret = -ESRCH;

	if (pid == NULL) {
		return -ESRCH;
	}

	K_SPINLOCK(&id_lock) {
		for (size_t i = 0; i < ARRAY_SIZE(process_ids); i++) {
			if (process_entry_live(&process_ids[i]) &&
			    (process_ids[i].proc->leader == pid)) {
				ret = process_ids[i].id;
				break;
			}
		}
	}

	if (ret == -ESRCH) {
		/* a live member thread's handle names its group */
		K_SPINLOCK(&id_lock) {
			bool found = false;

			for (size_t i = 0; i < ARRAY_SIZE(process_ids); i++) {
				struct k_thread *t;

				if (!process_entry_live(&process_ids[i])) {
					continue;
				}
				SYS_DLIST_FOR_EACH_CONTAINER(&process_ids[i].proc->threads, t,
							     process_node) {
					if (t == pid) {
						ret = process_ids[i].id;
						found = true;
						break;
					}
				}
				if (found) {
					break;
				}
			}
		}
	}

	return ret;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_sys_process_id(k_pid_t pid)
{
	return z_impl_sys_process_id(pid);
}
#include <zephyr/syscalls/sys_process_id_mrsh.c>
#endif /* CONFIG_USERSPACE */

k_pid_t z_impl_sys_process_find(int id)
{
	k_pid_t ret = NULL;

	K_SPINLOCK(&id_lock) {
		for (size_t i = 0; i < ARRAY_SIZE(process_ids); i++) {
			if (process_entry_live(&process_ids[i]) && (process_ids[i].id == id)) {
				ret = process_ids[i].proc->leader;
				break;
			}
		}
	}

	return ret;
}

#ifdef CONFIG_USERSPACE
static inline k_pid_t z_vrfy_sys_process_find(int id)
{
	return z_impl_sys_process_find(id);
}
#include <zephyr/syscalls/sys_process_find_mrsh.c>
#endif /* CONFIG_USERSPACE */

int z_impl_sys_pgrp_id(k_pgrp_t pgrp)
{
	int ret = -ESRCH;

	K_SPINLOCK(&id_lock) {
		for (size_t i = 0; i < ARRAY_SIZE(pgrp_ids); i++) {
			if ((pgrp_ids[i].grp != NULL) && (pgrp_ids[i].grp == pgrp)) {
				ret = pgrp_ids[i].id;
				break;
			}
		}
	}

	return ret;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_sys_pgrp_id(k_pgrp_t pgrp)
{
	return z_impl_sys_pgrp_id(pgrp);
}
#include <zephyr/syscalls/sys_pgrp_id_mrsh.c>
#endif /* CONFIG_USERSPACE */

k_pgrp_t z_impl_sys_pgrp_find(int id)
{
	k_pgrp_t ret = NULL;

	K_SPINLOCK(&id_lock) {
		for (size_t i = 0; i < ARRAY_SIZE(pgrp_ids); i++) {
			if ((pgrp_ids[i].grp != NULL) && (pgrp_ids[i].id == id)) {
				ret = pgrp_ids[i].grp;
				break;
			}
		}
	}

	return ret;
}

#ifdef CONFIG_USERSPACE
static inline k_pgrp_t z_vrfy_sys_pgrp_find(int id)
{
	return z_impl_sys_pgrp_find(id);
}
#include <zephyr/syscalls/sys_pgrp_find_mrsh.c>
#endif /* CONFIG_USERSPACE */

int z_impl_sys_session_id(k_session_t session)
{
	int ret = -ESRCH;

	K_SPINLOCK(&id_lock) {
		for (size_t i = 0; i < ARRAY_SIZE(session_ids); i++) {
			if ((session_ids[i].session != NULL) && (session_ids[i].session == session)) {
				ret = session_ids[i].id;
				break;
			}
		}
	}

	return ret;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_sys_session_id(k_session_t session)
{
	return z_impl_sys_session_id(session);
}
#include <zephyr/syscalls/sys_session_id_mrsh.c>
#endif /* CONFIG_USERSPACE */

k_session_t z_impl_sys_session_find(int id)
{
	k_session_t ret = NULL;

	K_SPINLOCK(&id_lock) {
		for (size_t i = 0; i < ARRAY_SIZE(session_ids); i++) {
			if ((session_ids[i].session != NULL) && (session_ids[i].id == id)) {
				ret = session_ids[i].session;
				break;
			}
		}
	}

	return ret;
}

#ifdef CONFIG_USERSPACE
static inline k_session_t z_vrfy_sys_session_find(int id)
{
	return z_impl_sys_session_find(id);
}
#include <zephyr/syscalls/sys_session_find_mrsh.c>
#endif /* CONFIG_USERSPACE */

/*
 * Pools: elastipool provides the static minimums and the heap tier, and the
 * companion cells K_PROCESS_DEFINE() emits into the iterable sections extend
 * the group/session pools. Frees may run under the scheduler lock, where the
 * heap is off limits, so pool releases pass through graveyards drained on the
 * next allocation.
 */
#define CELL_POOL_MIN (SYS_PROCESS_MIN + 3)
#define CELL_POOL_MAX (CELL_POOL_MIN + CONFIG_SYS_PROCESS_MAX)
#define RECORD_POOL_MAX (SYS_PROCESS_MIN + CONFIG_SYS_PROCESS_MAX)

__aligned(WB_UP(__alignof(struct k_process))) static uint8_t
	record_pool_storage[ROUND_UP(sizeof(struct k_process), __alignof(struct k_process)) *
			    SYS_PROCESS_MIN];
SYS_ELASTIPOOL_DEFINE_ADVANCED(record_pool, sizeof(struct k_process), __alignof(struct k_process),
			       SYS_PROCESS_MIN, RECORD_POOL_MAX,
			       SYS_ELASTIPOOL_KERNEL_HEAP_ALLOC_FN(SYS_PROCESS_MIN,
								   RECORD_POOL_MAX),
			       record_pool_storage, static);

__aligned(WB_UP(__alignof(struct k_process_group))) static uint8_t
	pgrp_pool_storage[ROUND_UP(sizeof(struct k_process_group),
				   __alignof(struct k_process_group)) *
			  CELL_POOL_MIN];
SYS_ELASTIPOOL_DEFINE_ADVANCED(pgrp_pool, sizeof(struct k_process_group),
			       __alignof(struct k_process_group), CELL_POOL_MIN, CELL_POOL_MAX,
			       SYS_ELASTIPOOL_KERNEL_HEAP_ALLOC_FN(CELL_POOL_MIN, CELL_POOL_MAX),
			       pgrp_pool_storage, static);

__aligned(WB_UP(__alignof(struct k_session))) static uint8_t
	session_pool_storage[ROUND_UP(sizeof(struct k_session), __alignof(struct k_session)) *
			     CELL_POOL_MIN];
SYS_ELASTIPOOL_DEFINE_ADVANCED(session_pool, sizeof(struct k_session),
			       __alignof(struct k_session), CELL_POOL_MIN, CELL_POOL_MAX,
			       SYS_ELASTIPOOL_KERNEL_HEAP_ALLOC_FN(CELL_POOL_MIN, CELL_POOL_MAX),
			       session_pool_storage, static);

static struct k_spinlock pool_lock;
#ifdef CONFIG_SYS_THREAD
/* dead pool-owned leaders parked at reap until the next spawn can destroy them */
#endif /* CONFIG_SYS_THREAD */
static sys_dlist_t record_graveyard = SYS_DLIST_STATIC_INIT(&record_graveyard);
static sys_dlist_t pgrp_graveyard = SYS_DLIST_STATIC_INIT(&pgrp_graveyard);
static sys_dlist_t session_graveyard = SYS_DLIST_STATIC_INIT(&session_graveyard);
/* free cells emitted into the iterable sections by K_PROCESS_DEFINE() */
static sys_dlist_t pgrp_section_cells = SYS_DLIST_STATIC_INIT(&pgrp_section_cells);
static sys_dlist_t session_section_cells = SYS_DLIST_STATIC_INIT(&session_section_cells);

static void graveyards_drain_locked(void)
{
	sys_dnode_t *n;

	for (n = sys_dlist_get(&record_graveyard); n != NULL;
	     n = sys_dlist_get(&record_graveyard)) {
		(void)sys_elastipool_free(&record_pool,
					  CONTAINER_OF(n, struct k_process, sibling_node));
	}
	for (n = sys_dlist_get(&pgrp_graveyard); n != NULL; n = sys_dlist_get(&pgrp_graveyard)) {
		(void)sys_elastipool_free(&pgrp_pool,
					  CONTAINER_OF(n, struct k_process_group, session_node));
	}
	for (n = sys_dlist_get(&session_graveyard); n != NULL;
	     n = sys_dlist_get(&session_graveyard)) {
		(void)sys_elastipool_free(&session_pool,
					  CONTAINER_OF(n, struct k_session, cell_node));
	}
}

static struct k_process *record_alloc(void)
{
	void *obj = NULL;

	K_SPINLOCK(&pool_lock) {
		graveyards_drain_locked();
		(void)sys_elastipool_alloc(&record_pool, &obj);
	}
	if (obj == NULL) {
		return NULL;
	}

	memset(obj, 0, sizeof(struct k_process));
	process_id_assign(obj);

	return obj;
}

int z_process_record_free(struct k_process *proc)
{
	int ret = 0;

	K_SPINLOCK(&pool_lock) {
		if (sys_elastipool_check(&record_pool, proc) != 0) {
			ret = -EINVAL;
			K_SPINLOCK_BREAK;
		}
		process_id_retire(proc);
		sys_dlist_append(&record_graveyard, &proc->sibling_node);
	}

	return ret;
}

static struct k_process_group *pgrp_cell_alloc(void)
{
	void *obj = NULL;
	sys_dnode_t *n;

	K_SPINLOCK(&pool_lock) {
		graveyards_drain_locked();
		n = sys_dlist_get(&pgrp_section_cells);
		if (n != NULL) {
			obj = CONTAINER_OF(n, struct k_process_group, session_node);
			K_SPINLOCK_BREAK;
		}
		(void)sys_elastipool_alloc(&pgrp_pool, &obj);
	}
	if (obj != NULL) {
		memset(obj, 0, sizeof(struct k_process_group));
	}

	return obj;
}

int z_pgrp_cell_free(struct k_process_group *grp)
{
	int ret = 0;

	K_SPINLOCK(&pool_lock) {
		if ((grp >= STRUCT_SECTION_START(k_process_group)) &&
		    (grp < STRUCT_SECTION_END(k_process_group))) {
			pgrp_id_retire(grp);
			sys_dlist_append(&pgrp_section_cells, &grp->session_node);
			K_SPINLOCK_BREAK;
		}
		if (sys_elastipool_check(&pgrp_pool, grp) != 0) {
			ret = -EINVAL;
			K_SPINLOCK_BREAK;
		}
		pgrp_id_retire(grp);
		sys_dlist_append(&pgrp_graveyard, &grp->session_node);
	}

	return ret;
}

static struct k_session *session_cell_alloc(void)
{
	void *obj = NULL;
	sys_dnode_t *n;

	K_SPINLOCK(&pool_lock) {
		graveyards_drain_locked();
		n = sys_dlist_get(&session_section_cells);
		if (n != NULL) {
			obj = CONTAINER_OF(n, struct k_session, cell_node);
			K_SPINLOCK_BREAK;
		}
		(void)sys_elastipool_alloc(&session_pool, &obj);
	}
	if (obj != NULL) {
		memset(obj, 0, sizeof(struct k_session));
	}

	return obj;
}

int z_session_cell_free(struct k_session *session)
{
	int ret = 0;

	K_SPINLOCK(&pool_lock) {
		if ((session >= STRUCT_SECTION_START(k_session)) &&
		    (session < STRUCT_SECTION_END(k_session))) {
			session_id_retire(session);
			sys_dlist_append(&session_section_cells, &session->cell_node);
			K_SPINLOCK_BREAK;
		}
		if (sys_elastipool_check(&session_pool, session) != 0) {
			ret = -EINVAL;
			K_SPINLOCK_BREAK;
		}
		session_id_retire(session);
		sys_dlist_append(&session_graveyard, &session->cell_node);
	}

	return ret;
}

static int process_sys_init(void)
{
	static struct k_process *const boot_order[] = {&z_proc_idle, &z_proc_init, &z_proc_kth};

	STRUCT_SECTION_FOREACH(k_session, session) {
		if ((session->flags & K_SESSION_FLAG_STATIC) == 0U) {
			sys_dlist_append(&session_section_cells, &session->cell_node);
		}
	}
	STRUCT_SECTION_FOREACH(k_process_group, grp) {
		if (grp->session == NULL) {
			sys_dlist_append(&pgrp_section_cells, &grp->session_node);
		}
	}

	/* boot order gives the reserved processes the IDs 0, 1, and 2 */
	for (size_t i = 0; i < ARRAY_SIZE(boot_order); i++) {
		struct k_process *proc = boot_order[i];

		process_id_assign(proc);
		pgrp_id_assign(proc->pgrp, process_number(proc));
		session_id_assign(proc->pgrp->session, process_number(proc));
	}
	STRUCT_SECTION_FOREACH(k_process, proc) {
		process_id_assign(proc);
	}

	/*
	 * Number link-time topology by its lowest-numbered member:
	 * deterministic regardless of the linker's section ordering.
	 */
	STRUCT_SECTION_FOREACH(k_process_group, grp) {
		struct k_process *proc;
		struct k_process *lowest = NULL;

		if (grp->session == NULL) {
			continue;
		}
		SYS_DLIST_FOR_EACH_CONTAINER(&grp->members, proc, pgrp_node) {
			if ((lowest == NULL) ||
			    (process_number(proc) < process_number(lowest))) {
				lowest = proc;
			}
		}
		if (lowest != NULL) {
			grp->leader = lowest->leader;
			pgrp_id_assign(grp, process_number(lowest));
		}
	}
	STRUCT_SECTION_FOREACH(k_session, session) {
		struct k_process_group *grp;
		struct k_process *lowest = NULL;

		if ((session->flags & K_SESSION_FLAG_STATIC) == 0U) {
			continue;
		}
		SYS_DLIST_FOR_EACH_CONTAINER(&session->pgrps, grp, session_node) {
			struct k_process *proc;

			SYS_DLIST_FOR_EACH_CONTAINER(&grp->members, proc, pgrp_node) {
				if ((lowest == NULL) ||
				    (process_number(proc) < process_number(lowest))) {
					lowest = proc;
				}
			}
		}
		if (lowest != NULL) {
			session->leader = lowest->leader;
			session_id_assign(session, process_number(lowest));
		}
	}

	return 0;
}

/* after the kernel's process_boot_init() (PRE_KERNEL_1, 0) */
SYS_INIT(process_sys_init, PRE_KERNEL_1, 1);



#ifdef CONFIG_SYS_THREAD
/* a leader without a caller-supplied k_thread is drawn from the system pool */
static int leader_create(const struct sys_clone_args *args, k_thread_entry_t entry, void *p1,
			 void *p2, void *p3, k_tid_t *tid)
{
	if (args->thread == NULL) {
		return sys_thread_create_paused(tid, NULL, args->stack_size, 0, entry, p1, p2, p3,
						args->prio, args->options);
	}

	*tid = k_thread_create(args->thread, args->stack, args->stack_size, entry, p1, p2, p3,
			       args->prio, args->options, K_FOREVER);

	return 0;
}

static void leader_discard(const struct sys_clone_args *args, k_tid_t tid)
{
	k_thread_abort(tid);
	if (args->thread == NULL) {
		(void)sys_thread_destroy(tid);
	}
}
#else
static int leader_create(const struct sys_clone_args *args, k_thread_entry_t entry, void *p1,
			 void *p2, void *p3, k_tid_t *tid)
{
	*tid = k_thread_create(args->thread, args->stack, args->stack_size, entry, p1, p2, p3,
			       args->prio, args->options, K_FOREVER);

	return 0;
}

static void leader_discard(const struct sys_clone_args *args, k_tid_t tid)
{
	ARG_UNUSED(args);

	k_thread_abort(tid);
}
#endif /* CONFIG_SYS_THREAD */

/* the process tier of sys_clone(); the dispatch has validated args */
int z_sys_clone_process(const struct sys_clone_args *args, k_pid_t *child)
{
	int ret;
	k_tid_t tid;
	struct k_process *proc;

	proc = record_alloc();
	if (proc == NULL) {
		return -EAGAIN;
	}

	/* create the leader paused; k_process_init() re-homes it */
	ret = leader_create(args, args->entry, args->p1, args->p2, args->p3, &tid);
	if (ret < 0) {
		(void)z_process_record_free(proc);
		return ret;
	}

	ret = k_process_init(proc, &tid, 1);
	if (ret < 0) {
		leader_discard(args, tid);
		(void)z_process_record_free(proc);
		return ret;
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

	if ((args->flags & SYS_CLONE_PAUSED) == 0U) {
		k_thread_start(tid);
	}

	return 0;
}

k_session_t z_impl_sys_setsid(void)
{
	int ret;
	int n;
	struct k_session *session;
	struct k_process_group *grp;

	session = session_cell_alloc();
	grp = (session == NULL) ? NULL : pgrp_cell_alloc();
	if (grp == NULL) {
		if (session != NULL) {
			(void)z_session_cell_free(session);
		}
		return NULL;
	}

	ret = k_session_init(session, grp);
	if (ret < 0) {
		(void)z_session_cell_free(session);
		(void)z_pgrp_cell_free(grp);
		return NULL;
	}

	n = z_impl_sys_process_id(k_getpid());
	session_id_assign(session, n);
	pgrp_id_assign(grp, n);

	return session;
}

#ifdef CONFIG_USERSPACE
static inline k_session_t z_vrfy_sys_setsid(void)
{
	return z_impl_sys_setsid();
}
#include <zephyr/syscalls/sys_setsid_mrsh.c>
#endif /* CONFIG_USERSPACE */

int z_impl_sys_setpgid(k_pid_t pid, k_pgrp_t pgrp)
{
	int ret;
	struct k_process_group *grp;

	if (pgrp != NULL) {
		return k_setpgid(pid, pgrp);
	}

	grp = pgrp_cell_alloc();
	if (grp == NULL) {
		return -EAGAIN;
	}

	ret = k_process_group_init(grp, pid);
	if (ret == 0) {
		pgrp_id_assign(grp, z_impl_sys_process_id((pid == NULL) ? k_getpid() : pid));
		return 0;
	}

	(void)z_pgrp_cell_free(grp);

	/* the target already leads its own group: success, the cell returns */
	return (ret == -EALREADY) ? 0 : ret;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_sys_setpgid(k_pid_t pid, k_pgrp_t pgrp)
{
	if (pgrp != NULL) {
		K_OOPS(K_SYSCALL_VERIFY_MSG(z_pgrp_validate(pgrp) == 0, "invalid process group"));
	}
	return z_impl_sys_setpgid(pid, pgrp);
}
#include <zephyr/syscalls/sys_setpgid_mrsh.c>
#endif /* CONFIG_USERSPACE */
