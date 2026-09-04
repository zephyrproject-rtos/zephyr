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
#include <zephyr/sys/internal/fdtable_priv.h>
#include <zephyr/sys/process.h>
#include <zephyr/sys/libc-hooks.h>
#include <zephyr/sys/thread.h>
#include <kernel_internal.h>
#include "sys_process_priv.h"
#ifdef CONFIG_SYS_PROCESS_EXEC
#include <stdlib.h>
#include <zephyr/llext/llext.h>
#include <zephyr/llext/fs_loader.h>
#endif /* CONFIG_SYS_PROCESS_EXEC */
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



/*
 * Duplicate the calling process's writable memory into a child domain at
 * identical virtual addresses. This is the M4 address-space substrate: with
 * copied memory the child's execution cannot disturb the parent (the vfork
 * shared-stack hazard does not exist), and a plain setjmp/longjmp resumes
 * the child at the clone point because every stack address it restores is
 * valid in its own copy. Until per-process virtual address spaces exist
 * (per-domain page tables mapping fresh frames at the caller's addresses),
 * there is nothing to duplicate onto.
 */
#ifdef CONFIG_PROCESS_VM
#include <zephyr/kernel/mm.h>

#define VM_CLONE_MAX_RANGES (CONFIG_MAX_DOMAIN_PARTITIONS + 1)

struct vm_clone_range {
	uintptr_t start;
	size_t size;
	void *copy;
};

struct vm_clone_ctx {
	struct k_mem_domain domain;
	sys_dnode_t free_node;
	struct vm_clone_range ranges[VM_CLONE_MAX_RANGES];
	size_t nframes;
#ifdef CONFIG_ARCH_HAS_FORK_RESUME
	/* staged parent syscall frame the child's first run resumes from */
	void *fork_frame;
#endif /* CONFIG_ARCH_HAS_FORK_RESUME */
};

/* one child-domain context per pool record; freed to a graveyard at record free */
static struct vm_clone_ctx vm_ctx_slab[SYS_PROCESS_MIN];
/* arch domain state is not reclaimable: a slot's domain is initialized once */
static bool vm_ctx_domain_ready[SYS_PROCESS_MIN];
static sys_dlist_t vm_ctx_free = SYS_DLIST_STATIC_INIT(&vm_ctx_free);
static bool vm_ctx_ready;

static void vm_ctx_init(void)
{
	if (!vm_ctx_ready) {
		for (size_t i = 0; i < ARRAY_SIZE(vm_ctx_slab); i++) {
			sys_dlist_append(&vm_ctx_free, &vm_ctx_slab[i].free_node);
		}
		vm_ctx_ready = true;
	}
}

/*
 * A process's VM is torn down from the exit path, under the scheduler lock,
 * where the heap is off limits: the copied frames are freed later, from the
 * next fork's caller context, like the other graveyards above.
 */
static sys_dlist_t vm_graveyard = SYS_DLIST_STATIC_INIT(&vm_graveyard);

static void vm_graveyard_drain(void)
{
	sys_dnode_t *n;

	do {
		struct vm_clone_ctx *ctx = NULL;

		K_SPINLOCK(&pool_lock) {
			n = sys_dlist_get(&vm_graveyard);
			if (n != NULL) {
				ctx = CONTAINER_OF(n, struct vm_clone_ctx, free_node);
			}
		}
		if (ctx == NULL) {
			break;
		}
		for (size_t i = 0; i < ctx->nframes; i++) {
			k_free(ctx->ranges[i].copy);
		}
		ctx->nframes = 0;
		K_SPINLOCK(&pool_lock) {
			sys_dlist_append(&vm_ctx_free, &ctx->free_node);
		}
	} while (true);
}

static size_t page_up(size_t v)
{
	return ROUND_UP(v, CONFIG_MMU_PAGE_SIZE);
}

static int vm_clone_range(struct vm_clone_ctx *ctx, uintptr_t start, size_t size)
{
	void *frames;
	int ret;

	if (ctx->nframes >= ARRAY_SIZE(ctx->ranges)) {
		return -ENOMEM;
	}

	frames = k_aligned_alloc(CONFIG_MMU_PAGE_SIZE, page_up(size));
	if (frames == NULL) {
		return -ENOMEM;
	}

	memcpy(frames, (void *)start, size);

	ret = z_mem_domain_clone_remap(&ctx->domain, (void *)start, page_up(size),
				       k_mem_phys_addr(frames));
	if (ret != 0) {
		k_free(frames);
		return ret;
	}

#ifdef CONFIG_SIGNAL
	/* a fresh image (fork+exec-like birth): only ignored dispositions inherit */
	ret = z_sig_actions_clone(_current->process, proc, false);
	if (ret < 0) {
		leader_discard(args, tid);
		z_process_fds_drop(proc);
		z_process_domain_drop(proc);
		z_process_domain_drop(proc);
		z_process_args_drop(proc);
		z_process_args_drop(proc);
		(void)z_process_record_free(proc);
		return -EAGAIN;
	}
#endif /* CONFIG_SIGNAL */

	ret = k_process_init(proc, &tid, 1);
	if (ret < 0) {
		leader_discard(args, tid);
#ifdef CONFIG_SIGNAL
		z_sig_process_drop(proc);
#endif /* CONFIG_SIGNAL */
#ifdef CONFIG_SIGNAL
	/* the fork model: the child inherits every parent disposition */
	ret = z_sig_actions_clone(parent, proc, true);
	if (ret < 0) {
		k_thread_abort(tid);
		z_process_mmap_drop(proc);
		z_process_fds_drop(proc);
		(void)z_process_record_free(proc);
		return -EAGAIN;
	}
#endif /* CONFIG_SIGNAL */

	ret = k_process_init(proc, &tid, 1);
	if (ret < 0) {
		k_thread_abort(tid);
#ifdef CONFIG_SIGNAL
		z_sig_process_drop(proc);
#endif /* CONFIG_SIGNAL */
	ctx->ranges[ctx->nframes++] = (struct vm_clone_range){
		.start = start,
		.size = size,
		.copy = frames,
	};

	return 0;
}


static void vm_domain_empty(struct k_mem_domain *d)
{
	while (true) {
		struct k_mem_partition *part = NULL;

		for (uint8_t i = 0; i < CONFIG_MAX_DOMAIN_PARTITIONS; i++) {
			if (d->partitions[i].size != 0U) {
				part = &d->partitions[i];
				break;
			}
		}
		if ((part == NULL) || (k_mem_domain_remove_partition(d, part) != 0)) {
			break;
		}
	}
}

static int process_vm_clone(struct k_process *parent, struct k_process *proc)
{
	struct k_mem_domain *src = _current->mem_domain_info.mem_domain;
	struct vm_clone_ctx *ctx;
	sys_dnode_t *n;
	int ret;

	ARG_UNUSED(parent);

	if (src == NULL) {
		return -EINVAL;
	}

	vm_graveyard_drain();
	K_SPINLOCK(&pool_lock) {
		vm_ctx_init();
		n = sys_dlist_get(&vm_ctx_free);
	}
	if (n == NULL) {
		return -EAGAIN;
	}
	ctx = CONTAINER_OF(n, struct vm_clone_ctx, free_node);
	ctx->nframes = 0;

	if (vm_ctx_domain_ready[ctx - vm_ctx_slab]) {
		vm_domain_empty(&ctx->domain);
	} else {
		ret = k_mem_domain_init(&ctx->domain, 0, NULL);
		if (ret != 0) {
			goto err_return_ctx;
		}
		vm_ctx_domain_ready[ctx - vm_ctx_slab] = true;
	}

	/* mirror every parent partition into the child (perms + shared frames) */
	for (uint8_t i = 0; i < src->num_partitions; i++) {
		struct k_mem_partition *part = &src->partitions[i];

		if (part->size == 0U) {
			continue;
		}
		ret = k_mem_domain_add_partition(&ctx->domain, part);
		if (ret != 0) {
			goto err_return_ctx;
		}
	}

	/* the child's leader runs on the caller's stack range, copied */
	{
		struct k_mem_partition stack = {
			.start = _current->stack_info.start,
			.size = page_up(_current->stack_info.size),
			.attr = K_MEM_PARTITION_P_RW_U_RW,
		};

		ret = k_mem_domain_add_partition(&ctx->domain, &stack);
		if (ret != 0) {
			goto err_return_ctx;
		}
	}

	/* now that user perms are set in the child tables, remap onto copies */
	for (uint8_t i = 0; i < src->num_partitions; i++) {
		struct k_mem_partition *part = &src->partitions[i];

		if ((part->size == 0U) || !K_MEM_PARTITION_IS_WRITABLE(part->attr) ||
		    z_zvfs_mmap_is_shared(part->start)) {
			continue;
		}
		ret = vm_clone_range(ctx, part->start, part->size);
		if (ret != 0) {
			goto err_free_frames;
		}
	}
	ret = vm_clone_range(ctx, _current->stack_info.start, _current->stack_info.size);
	if (ret != 0) {
		goto err_free_frames;
	}

	proc->vm = ctx;

	return 0;

err_free_frames:
	for (size_t i = 0; i < ctx->nframes; i++) {
		k_free(ctx->ranges[i].copy);
	}
err_return_ctx:
	K_SPINLOCK(&pool_lock) {
		sys_dlist_append(&vm_ctx_free, &ctx->free_node);
	}

	return ret;
}

void z_process_vm_teardown(void *vm)
{
	struct vm_clone_ctx *ctx = vm;

	/* the exit path, under the scheduler lock: the heap is off limits here */
	K_SPINLOCK(&pool_lock) {
		sys_dlist_append(&vm_graveyard, &ctx->free_node);
	}
}

struct k_mem_domain *z_process_vm_domain(void *vm)
{
	struct vm_clone_ctx *ctx = vm;

	return &ctx->domain;
}
#endif /* CONFIG_PROCESS_VM */

#if defined(CONFIG_PROCESS_VM) && defined(CONFIG_ARCH_HAS_FORK_RESUME)
/* lib/os/thread.c: pool create that skips user-syscall revalidation */
int sys_thread_create_paused_trusted(struct k_thread **th, k_thread_stack_t *stack,
				     uint32_t stack_size, uint32_t guard_size,
				     k_thread_entry_t entry, void *p1, void *p2, void *p3, int prio,
				     uint32_t options);

static void fork_child_entry(void *p1, void *p2, void *p3)
{
	struct vm_clone_ctx *ctx = _current->process->vm;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* first run of a forked child: enter the parent's captured context */
	arch_fork_resume(ctx->fork_frame);
}
#endif /* CONFIG_PROCESS_VM && CONFIG_ARCH_HAS_FORK_RESUME */

/* the fork tier of sys_clone(); the dispatch has validated args */
int z_sys_clone_vm_copy(const struct sys_clone_args *args, k_pid_t *child)
{
#ifdef CONFIG_PROCESS_VM
	int ret;
	k_tid_t tid;
	bool fork_resume = false;
	k_thread_entry_t entry = args->entry;
	struct k_process *proc;
	struct k_process *parent = _current->process;

	if (entry == NULL) {
		/*
		 * The fork model: the child's first run returns to user mode
		 * from the parent's captured syscall frame. Only meaningful
		 * for a user-mode caller mid-syscall.
		 */
		if (!IS_ENABLED(CONFIG_ARCH_HAS_FORK_RESUME) ||
		    (_current->syscall_frame == NULL)) {
			return -ENOTSUP;
		}
		fork_resume = true;
#ifdef CONFIG_ARCH_HAS_FORK_RESUME
		entry = fork_child_entry;
#endif /* CONFIG_ARCH_HAS_FORK_RESUME */
	}

	proc = record_alloc();
	if (proc == NULL) {
		return -EAGAIN;
	}

	ret = process_vm_clone(parent, proc);
	if (ret < 0) {
		(void)z_process_record_free(proc);
		return ret;
	}

	/*
	 * The child's leader runs on the COPIED stack: k_process_init() adopts
	 * it, we move it into the cloned domain, inherit the parent's object
	 * permissions, then start it. In the fork model the leader is created
	 * as a supervisor thread whose first run stages into the parent's
	 * captured syscall frame (fork_child_entry -> arch_fork_resume) and
	 * becomes a user thread there; an explicit entry starts normally.
	 */
#ifdef CONFIG_ARCH_HAS_FORK_RESUME
	if (fork_resume) {
		/*
		 * The child leader boots in supervisor mode to run the fork
		 * resume trampoline, then resumes into user mode and issues
		 * syscalls from there, so it needs a user-capable stack (on
		 * x86, the privilege stack the trampoline and every later
		 * syscall run on). A trusted create with K_USER would enter
		 * user mode at the trampoline instead of running it, so
		 * allocate the user-capable stack up front and create the
		 * leader, kernel-side, on it.
		 */
		k_thread_stack_t *cstack = NULL;
		uint32_t cstack_size = CONFIG_MMU_PAGE_SIZE;

		ret = sys_thread_stack_alloc(&cstack, &cstack_size, 0, K_USER);
		if (ret == 0) {
			ret = sys_thread_create_paused_trusted(&tid, cstack, cstack_size, 0, entry,
							       args->p1, args->p2, args->p3,
							       k_thread_priority_get(k_current_get()),
							       K_INHERIT_PERMS);
			if (ret < 0) {
				(void)sys_thread_stack_destroy(cstack);
			} else {
				/* drawn for the leader: it lives and dies with it */
				z_kobj_bind_stack(cstack);
			}
		}
	} else
#endif /* CONFIG_ARCH_HAS_FORK_RESUME */
	{
		ret = sys_thread_create_paused(&tid, NULL, CONFIG_MMU_PAGE_SIZE, 0, entry,
					       args->p1, args->p2, args->p3,
					       k_thread_priority_get(k_current_get()),
					       K_USER | K_INHERIT_PERMS);
	}
	if (ret < 0) {
		(void)z_process_record_free(proc);
		return ret;
	}

#ifdef CONFIG_SIGNAL
	/* a copy of the caller, its signal mask included */
	tid->base.sig.mask = _current->base.sig.mask;
#endif /* CONFIG_SIGNAL */

	(void)k_mem_domain_add_thread(z_process_vm_domain(proc->vm), tid);

	/* the child's descriptor table: a copy of the caller's */
	ret = z_zvfs_fds_clone(proc, tid);
	if (ret < 0) {
		k_thread_abort(tid);
		(void)z_process_record_free(proc);
		return ret;
	}

	/* and the caller's memory mappings, in the copied address space */
	ret = z_zvfs_mmap_clone(proc, z_process_vm_domain(proc->vm));
	if (ret < 0) {
		k_thread_abort(tid);
		z_process_fds_drop(proc);
		(void)z_process_record_free(proc);
		return ret;
	}

	ret = k_process_init(proc, &tid, 1);
	if (ret < 0) {
		k_thread_abort(tid);
		z_process_mmap_drop(proc);
		z_process_fds_drop(proc);
		(void)z_process_record_free(proc);
		return ret;
	}
	/* the leader is bound and numbered: its thread number becomes the pid */
	process_id_assign(proc);

	z_process_perms_inherit(parent, proc);
	k_object_access_grant(tid, tid);
	k_object_access_grant(tid->stack_obj, tid);

#ifdef CONFIG_ARCH_HAS_FORK_RESUME
	if (fork_resume) {
		struct vm_clone_ctx *ctx = proc->vm;

		ctx->fork_frame = arch_fork_frame_init(tid, _current->syscall_frame);
		/* the child is a user thread from its first instruction on */
		tid->base.user_options |= K_USER;
#ifdef CONFIG_THREAD_LOCAL_STORAGE
		/*
		 * The child's TLS is the parent's address: the clone already
		 * copied that region (the child resumes on the parent's stack
		 * address, and TLS lives above it), so captured TLS-variable
		 * addresses and register-relative accesses agree, as fork
		 * demands. Kernel mode never dereferences user TLS in this
		 * configuration - PROCESS_VM excludes CURRENT_THREAD_USE_TLS,
		 * whose kernel-mode reads would alias the parent's physical
		 * memory through the shared kernel tables.
		 */
		tid->tls = _current->tls;
#endif /* CONFIG_THREAD_LOCAL_STORAGE */
	}
#endif /* CONFIG_ARCH_HAS_FORK_RESUME */

	if (child != NULL) {
		*child = tid;
	}

	if ((args->flags & SYS_CLONE_PAUSED) == 0U) {
		k_thread_start(tid);
	}

	return 0;
#else
	ARG_UNUSED(args);
	ARG_UNUSED(child);
	return -ENOTSUP;
#endif /* CONFIG_PROCESS_VM */
}

/*
 * Vectors handed to a process-tier clone are copied into the child's own
 * memory: the leader's trampoline reserves the room in a frame that lives as
 * long as the process and claims the staged copy with sys_process_args().
 * Until claimed the block hangs off the record, so a child aborted before its
 * first instruction does not leak it.
 */
struct clone_argblock {
	struct sys_process_start start;
	int argc;
	int envc;
	size_t bytes;
	char data[];
};

static int clone_vec_count(char *const *vec, size_t *strs)
{
	int n = 0;

	for (; (vec != NULL) && (vec[n] != NULL); n++) {
		*strs += strlen(vec[n]) + 1;
	}

	return n;
}

/* the tables hold offsets into the block until relocated into the claimant's buffer */
static void clone_vec_pack(char *const *vec, int n, char **tab, char *data, size_t *off)
{
	for (int i = 0; i < n; i++) {
		size_t len = strlen(vec[i]) + 1;

		tab[i] = (char *)*off;
		(void)memcpy(&data[*off], vec[i], len);
		*off += len;
	}
	tab[n] = NULL;
}

static int clone_argblock_pack(const struct sys_clone_args *args, struct clone_argblock **out)
{
	size_t strs = 0;
	int argc = clone_vec_count(args->argv, &strs);
	int envc = clone_vec_count(args->envp, &strs);
	size_t tables = (size_t)(argc + envc + 2) * sizeof(char *);
	size_t bytes = tables + strs;
	struct clone_argblock *blk;

	if (bytes > CONFIG_SYS_PROCESS_ARG_BYTES) {
		return -E2BIG;
	}

	if (K_HEAP_MEM_POOL_SIZE == 0) {
		/* nowhere to stage the block until the leader claims it */
		return -ENOTSUP;
	}

	blk = k_malloc(sizeof(*blk) + bytes);
	if (blk == NULL) {
		return -ENOMEM;
	}

	blk->start.entry = args->entry;
	blk->start.main = NULL;
	blk->argc = argc;
	blk->envc = envc;
	blk->bytes = bytes;
	clone_vec_pack(args->argv, argc, (char **)blk->data, blk->data, &tables);
	clone_vec_pack(args->envp, envc, &((char **)blk->data)[argc + 1], blk->data, &tables);

	*out = blk;

	return 0;
}

void z_process_args_drop(struct k_process *proc)
{
	k_free(proc->args);
	proc->args = NULL;
}

int z_impl_sys_process_args(struct sys_process_start *start, void *buf, size_t size)
{
	struct clone_argblock *blk = _current->process->args;
	char *base = buf;
	char **argv = buf;
	int argc;

	if ((blk == NULL) || ((blk->start.entry == NULL) && (blk->start.main == NULL))) {
		return -ENOENT;
	}
	if (blk->bytes > size) {
		return -E2BIG;
	}
	*start = blk->start;

	(void)memcpy(base, blk->data, blk->bytes);
	for (int i = 0; i < blk->argc; i++) {
		argv[i] = base + (uintptr_t)argv[i];
	}
	for (int i = 0; i < blk->envc; i++) {
		argv[blk->argc + 1 + i] = base + (uintptr_t)argv[blk->argc + 1 + i];
	}
	argc = blk->argc;
	_current->process->args = NULL;
	k_free(blk);

	return argc;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_sys_process_args(struct sys_process_start *start, void *buf, size_t size)
{
	K_OOPS(K_SYSCALL_MEMORY_WRITE(start, sizeof(*start)));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(buf, size));
	return z_impl_sys_process_args(start, buf, size);
}
#include <zephyr/syscalls/sys_process_args_mrsh.c>
#endif /* CONFIG_USERSPACE */

/* the leader's trampoline, in whichever mode the child was asked for */
static void process_leader_entry(void *p1, void *p2, void *p3)
{
	struct sys_process_start start = {0};
	/* this frame never returns: the vectors live here for the process's lifetime */
	char buf[CONFIG_SYS_PROCESS_ARG_BYTES] __aligned(sizeof(char *));
	char **argv = (char **)buf;
	int argc = sys_process_args(&start, buf, sizeof(buf));

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	__ASSERT_NO_MSG(argc >= 0);
#ifdef CONFIG_SYS_PROCESS_EXEC
	if (start.main != NULL) {
		/* the image ABI: a normal return from main() is exit(status) */
		exit(start.main(argc, argv, &argv[argc + 1]));
	}
#endif /* CONFIG_SYS_PROCESS_EXEC */
	start.entry(argv, &argv[argc + 1], (void *)(uintptr_t)argc);
}

#ifdef CONFIG_USERSPACE
/*
 * A user-mode process owns a memory domain. A process entered at a base-image
 * function mirrors its creator's partitions; a process whose image
 * sys_exec_load() brings in gets only the C library's shared state, and its
 * image's blocks on top: three partitions, which is what MPU parts with few
 * regions can afford. Arch domain state (page tables) is not reclaimable, so
 * pool domains are initialized once and emptied for reuse.
 */
static struct k_mem_domain domain_pool[RECORD_POOL_MAX];
static uint8_t domain_state[RECORD_POOL_MAX]; /* 0 free, 1 used, 2 free but initialized */

enum process_domain_base {
	PROCESS_DOMAIN_MIRROR,	/* the creator's partitions: base-image code */
	PROCESS_DOMAIN_LIBC,	/* the C library's shared state: an image process */
	PROCESS_DOMAIN_EMPTY,	/* nothing: a kernel-mode process on an MPU part */
};

static struct k_mem_domain *process_domain_alloc(enum process_domain_base base,
						 struct k_mem_domain *src)
{
	struct k_mem_domain *d = NULL;
	bool initialized = false;
	size_t slot = 0;
	int ret;

	K_SPINLOCK(&pool_lock) {
		for (size_t i = 0; i < ARRAY_SIZE(domain_pool); i++) {
			if (domain_state[i] != 1U) {
				initialized = (domain_state[i] == 2U);
				domain_state[i] = 1U;
				d = &domain_pool[i];
				slot = i;
				break;
			}
		}
	}
	if (d == NULL) {
		return NULL;
	}

	if (initialized) {
		while (d->num_partitions > 0U) {
			struct k_mem_partition *part = NULL;

			for (uint8_t i = 0; i < CONFIG_MAX_DOMAIN_PARTITIONS; i++) {
				if (d->partitions[i].size != 0U) {
					part = &d->partitions[i];
					break;
				}
			}
			if ((part == NULL) || (k_mem_domain_remove_partition(d, part) != 0)) {
				break;
			}
		}
	} else {
		ret = k_mem_domain_init(d, 0, NULL);
		if (ret != 0) {
			K_SPINLOCK(&pool_lock) {
				domain_state[slot] = 0U;
			}
			return NULL;
		}
	}

	if (base == PROCESS_DOMAIN_MIRROR) {
		for (uint8_t i = 0; (src != NULL) && (i < CONFIG_MAX_DOMAIN_PARTITIONS); i++) {
			struct k_mem_partition *part = &src->partitions[i];

			if (part->size == 0U) {
				continue;
			}
			ret = k_mem_domain_add_partition(d, part);
			if (ret != 0) {
				goto err;
			}
		}
	} else if (base == PROCESS_DOMAIN_LIBC) {
		struct k_mem_partition *libc[] = {
#if Z_LIBC_PARTITION_EXISTS
			&z_libc_partition,
#endif
#if Z_MALLOC_PARTITION_EXISTS
			&z_malloc_partition,
#endif
		};

		for (size_t i = 0; i < ARRAY_SIZE(libc); i++) {
			if (libc[i]->size == 0U) {
				continue;
			}
			ret = k_mem_domain_add_partition(d, libc[i]);
			if (ret != 0) {
				goto err;
			}
		}
	}

	return d;

err:
	K_SPINLOCK(&pool_lock) {
		domain_state[slot] = 2U;
	}
	return NULL;
}

static void process_domain_release(struct k_mem_domain *d)
{
	size_t slot = (size_t)(d - domain_pool);

	K_SPINLOCK(&pool_lock) {
		domain_state[slot] = 2U;
	}
}

static int process_domain_create(struct k_process *proc, bool mirror)
{
	enum process_domain_base base = mirror ? PROCESS_DOMAIN_MIRROR : PROCESS_DOMAIN_LIBC;
	struct k_mem_domain *d = process_domain_alloc(base, _current->mem_domain_info.mem_domain);

	if (d == NULL) {
		return -ENOMEM;
	}
	proc->domain = d;

	return 0;
}

void z_process_domain_drop(struct k_process *proc)
{
	if (proc->domain != NULL) {
		process_domain_release(proc->domain);
		proc->domain = NULL;
	}
	if (proc->domain_next != NULL) {
		process_domain_release(proc->domain_next);
		proc->domain_next = NULL;
	}
}
#else
void z_process_domain_drop(struct k_process *proc)
{
	ARG_UNUSED(proc);
}
#endif /* CONFIG_USERSPACE */

#ifdef CONFIG_SYS_PROCESS_EXEC
/*
 * Reap may run under the process and scheduler locks, where llext_unload()
 * (a mutex, the heap) must not be called: images of reaped processes wait in
 * a graveyard drained before the next load or exec, as the record pool does.
 * At most two images per record (current and pending) can be waiting.
 */
static struct llext *image_graveyard[2 * RECORD_POOL_MAX];
static struct k_spinlock image_graveyard_lock;

static void image_graveyard_drain(void)
{
	while (true) {
		struct llext *ext = NULL;

		K_SPINLOCK(&image_graveyard_lock) {
			for (size_t i = 0; i < ARRAY_SIZE(image_graveyard); i++) {
				if (image_graveyard[i] != NULL) {
					ext = image_graveyard[i];
					image_graveyard[i] = NULL;
					break;
				}
			}
		}
		if (ext == NULL) {
			break;
		}
		(void)llext_unload(&ext);
	}
}

static void image_bury(struct llext *ext)
{
	bool buried = false;

	if (ext == NULL) {
		return;
	}
	K_SPINLOCK(&image_graveyard_lock) {
		for (size_t i = 0; i < ARRAY_SIZE(image_graveyard); i++) {
			if (image_graveyard[i] == NULL) {
				image_graveyard[i] = ext;
				buried = true;
				break;
			}
		}
	}
	__ASSERT(buried, "image graveyard full");
}

void z_process_image_drop(struct k_process *proc)
{
	struct llext *ext = proc->image;
	struct llext *next = proc->image_next;

	proc->image = NULL;
	proc->image_next = NULL;
	image_bury(ext);
	image_bury(next);
}

#ifdef CONFIG_USERSPACE
/*
 * The domain an image's blocks join: the process's own, else the thread's.
 * A kernel-mode process needs one only on MPU parts, where RAM is not
 * executable by default: it gets a domain of its own then, so the image's
 * regions become dynamic MPU regions of that thread alone.
 */
static struct k_mem_domain *process_image_domain(struct k_process *proc, k_tid_t leader)
{
	if ((leader->base.user_options & K_USER) == 0U) {
		if (!IS_ENABLED(CONFIG_MPU)) {
			return NULL;
		}
		if (proc->domain == NULL) {
			proc->domain = process_domain_alloc(PROCESS_DOMAIN_EMPTY, NULL);
			if ((proc->domain == NULL) ||
			    (k_mem_domain_add_thread(proc->domain, leader) != 0)) {
				return NULL;
			}
		}
	}
	return (proc->domain != NULL) ? proc->domain : leader->mem_domain_info.mem_domain;
}

/*
 * A self-exec cannot add the next image beside the running one on parts
 * with few MPU regions, and the running image's regions cannot go while it
 * still executes: the next image gets a pending domain holding everything
 * of the current one except the current image, which the exec switch
 * installs.
 */
static struct k_mem_domain *process_next_domain(struct k_process *proc)
{
	struct k_mem_domain *cur = proc->domain;
	struct k_mem_domain *d;
	struct llext *img = proc->image;

	if (proc->domain_next != NULL) {
		return proc->domain_next;
	}
	d = process_domain_alloc(PROCESS_DOMAIN_EMPTY, NULL);
	if (d == NULL) {
		return NULL;
	}
	for (uint8_t i = 0; (cur != NULL) && (i < CONFIG_MAX_DOMAIN_PARTITIONS); i++) {
		struct k_mem_partition *part = &cur->partitions[i];
		bool of_image = false;

		if (part->size == 0U) {
			continue;
		}
		for (int m = 0; (img != NULL) && (m < LLEXT_MEM_PARTITIONS); m++) {
			if ((img->mem_parts[m].size != 0U) &&
			    (img->mem_parts[m].start == part->start)) {
				of_image = true;
				break;
			}
		}
		if (!of_image && (k_mem_domain_add_partition(d, part) != 0)) {
			process_domain_release(d);
			return NULL;
		}
	}
	proc->domain_next = d;

	return d;
}
#endif /* CONFIG_USERSPACE */

int z_impl_sys_exec_load(k_pid_t child, const char *path)
{
	static atomic_t exec_instance;
	struct llext_fs_loader fldr = LLEXT_FS_LOADER(path);
	struct llext_load_param param = LLEXT_LOAD_PARAM_DEFAULT;
	struct llext *ext = NULL;
	struct k_process *proc = child->process;
	struct clone_argblock *blk;
	char name[LLEXT_MAX_NAME_LEN + 1];
	char suffix[8];
	const char *base;
	const void *main_sym;
	int ret;

	if (child == _current) {
		/* the caller's own next image, pending until sys_exec_start() */
		if (proc == NULL) {
			return -ESRCH;
		}
		blk = NULL;
	} else {
		/* a paused, argument-staged child of the caller that has no entry yet */
		if ((proc == NULL) || (proc->leader != child) ||
		    (proc->parent != _current->process) || (proc->args == NULL) ||
		    (proc->image != NULL) || !z_is_thread_prevented_from_running(child)) {
			return -ESRCH;
		}
		blk = proc->args;
		if ((blk->start.entry != NULL) || (blk->start.main != NULL)) {
			return -ESRCH;
		}
	}

	/*
	 * Each load is a private instance named by the path's last component
	 * plus an instance number: llext shares loaded extensions by name,
	 * which would hand this process another running image's globals.
	 */
	base = strrchr(path, '/');
	base = (base == NULL) ? path : (base + 1);
	(void)snprintf(suffix, sizeof(suffix), ".%x",
		       (unsigned int)atomic_inc(&exec_instance) & 0xffffU);
	(void)snprintf(name, sizeof(name), "%.*s%s", (int)(LLEXT_MAX_NAME_LEN - strlen(suffix)),
		       base, suffix);

	/* images of already-reaped processes give their memory back first */
	image_graveyard_drain();

	ret = llext_load(&fldr.loader, name, &ext, &param);
	if (ret < 0) {
		return (ret == -ENOMEM) ? -ENOMEM : -ENOENT;
	}

	main_sym = llext_find_sym(&ext->exp_tab, "main");
	if (main_sym == NULL) {
		(void)llext_unload(&ext);
		return -ENOEXEC;
	}

#ifdef CONFIG_USERSPACE
	{
		struct k_mem_domain *domain = process_image_domain(proc, child);

		if ((domain != NULL) && (blk == NULL) && (proc->domain != NULL)) {
			domain = process_next_domain(proc);
			if (domain == NULL) {
				(void)llext_unload(&ext);
				return -ENOMEM;
			}
		}
		if (domain != NULL) {
			ret = llext_add_domain(ext, domain);
			if (ret != 0) {
				(void)llext_unload(&ext);
				return -ENOMEM;
			}
		}
	}
#endif /* CONFIG_USERSPACE */

	if (blk == NULL) {
		struct llext *prev = proc->image_next;

		proc->image_next = ext;
		if (prev != NULL) {
#ifdef CONFIG_USERSPACE
			struct k_mem_domain *domain = (proc->domain_next != NULL)
							      ? proc->domain_next
							      : process_image_domain(proc, child);

			if (domain != NULL) {
				(void)llext_remove_domain(prev, domain);
			}
#endif /* CONFIG_USERSPACE */
			(void)llext_unload(&prev);
		}
	} else {
		proc->image = ext;
		*(const void **)&blk->start.main = main_sym;
	}

	return 0;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_sys_exec_load(k_pid_t child, const char *path)
{
	char *kpath;
	int ret;

	/* a thread may always name itself; another thread needs the object grant */
	if (child != _current) {
		K_OOPS(K_SYSCALL_OBJ(child, K_OBJ_THREAD));
	}
	kpath = k_usermode_string_alloc_copy(path, CONFIG_SYS_PROCESS_EXEC_PATH_MAX);
	if (kpath == NULL) {
		return -ENAMETOOLONG;
	}
	ret = z_impl_sys_exec_load(child, kpath);
	k_free(kpath);

	return ret;
}
#include <zephyr/syscalls/sys_exec_load_mrsh.c>
#endif /* CONFIG_USERSPACE */
#else
int z_impl_sys_exec_load(k_pid_t child, const char *path)
{
	ARG_UNUSED(child);
	ARG_UNUSED(path);
	return -ENOTSUP;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_sys_exec_load(k_pid_t child, const char *path)
{
	return z_impl_sys_exec_load(child, path);
}
#include <zephyr/syscalls/sys_exec_load_mrsh.c>
#endif /* CONFIG_USERSPACE */
#endif /* CONFIG_SYS_PROCESS_EXEC */

#ifdef CONFIG_SYS_THREAD
static void process_leader_restart(void *p1)
{
	ARG_UNUSED(p1);
	process_leader_entry(NULL, NULL, NULL);
}
#endif /* CONFIG_SYS_THREAD */

/*
 * Replace the caller's image: stage the vectors, swap in the pending image,
 * and restart the leader on a fresh stack in its own mode. The old image's
 * frames are abandoned; nothing of it executes again once we are here.
 */
static int exec_start(k_thread_entry_t entry, char *const *argv, char *const *envp,
		      bool free_vectors)
{
	struct k_process *proc = _current->process;
	struct clone_argblock *blk = NULL;
	const void *main_sym = NULL;
	int ret = 0;

#ifdef CONFIG_SYS_PROCESS_EXEC
	image_graveyard_drain();
#endif /* CONFIG_SYS_PROCESS_EXEC */
	if (proc == NULL) {
		ret = -EINVAL;
	} else if (entry == NULL) {
#ifdef CONFIG_SYS_PROCESS_EXEC
		if (proc->image_next == NULL) {
			ret = -ENOENT;
		} else {
			main_sym = llext_find_sym(&((struct llext *)proc->image_next)->exp_tab,
						  "main");
			if (main_sym == NULL) {
				ret = -ENOEXEC;
			}
		}
#else
		ret = -ENOENT;
#endif /* CONFIG_SYS_PROCESS_EXEC */
	}
	if (ret == 0) {
		struct sys_clone_args args = {
			.entry = entry,
			.argv = argv,
			.envp = envp,
		};

		ret = clone_argblock_pack(&args, &blk);
	}
	if (free_vectors) {
#ifdef CONFIG_USERSPACE
		z_clone_vec_free((char **)argv);
		z_clone_vec_free((char **)envp);
#endif /* CONFIG_USERSPACE */
	}
	if (ret < 0) {
		return ret;
	}

	/* point of no return: descriptors marked close-on-exec go first */
	zvfs_fds_cloexec();
	*(const void **)&blk->start.main = main_sym;
	k_free(proc->args);
	proc->args = blk;
#ifdef CONFIG_SYS_PROCESS_EXEC
	{
		struct llext *old = proc->image;

		proc->image = proc->image_next;
		proc->image_next = NULL;
#ifdef CONFIG_USERSPACE
		if (proc->domain_next != NULL) {
			/* the switch: the old domain, old image and all, is retired */
			struct k_mem_domain *retired = proc->domain;

			(void)k_mem_domain_add_thread(proc->domain_next, _current);
			proc->domain = proc->domain_next;
			proc->domain_next = NULL;
			if (retired != NULL) {
				process_domain_release(retired);
			}
		} else if (old != NULL) {
			struct k_mem_domain *domain = process_image_domain(proc, _current);

			if (domain != NULL) {
				(void)llext_remove_domain(old, domain);
			}
		}
#endif /* CONFIG_USERSPACE */
		if (old != NULL) {
			(void)llext_unload(&old);
		}
	}
#endif /* CONFIG_SYS_PROCESS_EXEC */

#ifdef CONFIG_USERSPACE
	if ((_current->base.user_options & K_USER) != 0U) {
		z_thread_user_mode_restart(process_leader_entry, NULL, NULL, NULL);
	}
#if defined(CONFIG_MPU) && defined(CONFIG_SYS_PROCESS_EXEC)
	if (proc->image != NULL) {
		/* domain changes program the MPU on context switch, so take one */
		k_sleep(K_TICKS(1));
	}
#endif /* CONFIG_MPU && CONFIG_SYS_PROCESS_EXEC */
#endif /* CONFIG_USERSPACE */
#ifdef CONFIG_SYS_THREAD
	(void)sys_thread_restart(process_leader_restart, 0);
#endif /* CONFIG_SYS_THREAD */
	/* no stack jump on this arch: run on the live frames */
	process_leader_entry(NULL, NULL, NULL);
	CODE_UNREACHABLE;
}

int z_impl_sys_exec_start(k_thread_entry_t entry, char *const *argv, char *const *envp)
{
	return exec_start(entry, argv, envp, false);
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_sys_exec_start(k_thread_entry_t entry, char *const *argv,
					char *const *envp)
{
	int ret = z_clone_user_vectors(&argv, &envp);

	if (ret < 0) {
		return ret;
	}
	return exec_start(entry, argv, envp, true);
}
#include <zephyr/syscalls/sys_exec_start_mrsh.c>
#endif /* CONFIG_USERSPACE */

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
	struct clone_argblock *blk = NULL;
	k_thread_entry_t entry = args->entry;
	void *p1 = args->p1;
	void *p2 = args->p2;
	void *p3 = args->p3;

	if (args->argv != NULL) {
		ret = clone_argblock_pack(args, &blk);
		if (ret < 0) {
			return ret;
		}
		entry = process_leader_entry;
		p1 = NULL;
		p2 = NULL;
		p3 = NULL;
	}

	proc = record_alloc();
	if (proc == NULL) {
		k_free(blk);
		return -EAGAIN;
	}
	proc->args = blk;
#ifdef CONFIG_SYS_PROCESS_EXEC
	proc->image = NULL;
	proc->image_next = NULL;
#endif /* CONFIG_SYS_PROCESS_EXEC */

	/* create the leader paused; k_process_init() re-homes it */
	ret = leader_create(args, entry, p1, p2, p3, &tid);
	if (ret < 0) {
		z_process_args_drop(proc);
		(void)z_process_record_free(proc);
		return ret;
	}

#ifdef CONFIG_USERSPACE
	proc->domain = NULL;
	proc->domain_next = NULL;
	if ((args->options & K_USER) != 0U) {
		/* a user-mode process owns its domain; see process_domain_create() */
		ret = process_domain_create(proc, args->entry != NULL);
		if (ret == 0) {
			ret = k_mem_domain_add_thread(proc->domain, tid);
		}
		if (ret < 0) {
			leader_discard(args, tid);
			z_process_domain_drop(proc);
			z_process_args_drop(proc);
			(void)z_process_record_free(proc);
			return ret;
		}
	}
#endif /* CONFIG_USERSPACE */

	/* the child's descriptor table: a copy of the caller's */
	ret = z_zvfs_fds_clone(proc, tid);
	if (ret < 0) {
		leader_discard(args, tid);
		z_process_domain_drop(proc);
		z_process_args_drop(proc);
		(void)z_process_record_free(proc);
		return ret;
	}

	ret = k_process_init(proc, &tid, 1);
	if (ret < 0) {
		leader_discard(args, tid);
		z_process_fds_drop(proc);
		(void)z_process_record_free(proc);
		return ret;
	}
#ifdef CONFIG_USERSPACE
	/* grants made at creation (the leader, its stack, the cloned descriptors) landed on the
	 * caller's process: the child inherits them, as a copied process does
	 */
	z_process_perms_inherit(k_current_get()->process, proc);
	k_object_access_grant(tid, tid);
	k_object_access_grant(tid->stack_obj, tid);
#endif /* CONFIG_USERSPACE */

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

int z_impl_sys_fork(k_pid_t *child)
{
	struct sys_clone_args args = {
		.flags = SYS_CLONE_VM_COPY,
	};

	return z_impl_sys_clone(&args, child);
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_sys_fork(k_pid_t *child)
{
	if (child != NULL) {
		K_OOPS(K_SYSCALL_MEMORY_WRITE(child, sizeof(*child)));
	}
	return z_impl_sys_fork(child);
}
#include <zephyr/syscalls/sys_fork_mrsh.c>
#endif /* CONFIG_USERSPACE */

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
