/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief File descriptor table
 *
 * This file provides generic file descriptor table implementation, suitable
 * for any I/O object implementing POSIX I/O semantics (i.e. read/write +
 * aux operations).
 *
 * A file descriptor is a slot in a descriptor table pairing a number with an
 * open file description and the descriptor's own FD_CLOEXEC flag. The open
 * file descriptions live in a system-wide pool; dup() and inheritance make
 * several descriptors share one description, and with it the file offset,
 * as POSIX requires.
 */

#include <errno.h>
#include <string.h>
#include <stdio.h>

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/process.h>
#include <zephyr/sys/elastipool.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/sys/internal/fdtable_priv.h>
#include <zephyr/sys/internal/kobject_internal.h>
#include <zephyr/sys/speculation.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/sys/atomic.h>
#ifdef CONFIG_ZVFS_RESERVE_STDIO_FDS
static const struct fd_op_vtable stdinout_fd_op_vtable;
BUILD_ASSERT(ZVFS_OPEN_SIZE >= 3, "ZVFS_OPEN_SIZE >= 3 to reserve stdin/stdout/stderr fds");
#endif

/* the system-wide pool of open file descriptions */
static struct fd_entry fdtable[ZVFS_OPEN_SIZE] = {
#ifdef CONFIG_ZVFS_RESERVE_STDIO_FDS
	/*
	 * Predefine descriptions for stdin/stdout/stderr.
	 */
	{
		/* STDIN */
		.vtable = &stdinout_fd_op_vtable,
		.refcount = ATOMIC_INIT(1),
		.lock = Z_MUTEX_INITIALIZER(fdtable[0].lock),
		.cond = Z_CONDVAR_INITIALIZER(fdtable[0].cond),
		.mode = ZVFS_MODE_IFCHR,
	},
	{
		/* STDOUT */
		.vtable = &stdinout_fd_op_vtable,
		.refcount = ATOMIC_INIT(1),
		.lock = Z_MUTEX_INITIALIZER(fdtable[1].lock),
		.cond = Z_CONDVAR_INITIALIZER(fdtable[1].cond),
		.mode = ZVFS_MODE_IFCHR,
	},
	{
		/* STDERR */
		.vtable = &stdinout_fd_op_vtable,
		.refcount = ATOMIC_INIT(1),
		.lock = Z_MUTEX_INITIALIZER(fdtable[2].lock),
		.cond = Z_CONDVAR_INITIALIZER(fdtable[2].cond),
		.mode = ZVFS_MODE_IFCHR,
	},
#else
	{0},
#endif
};

/* the boot descriptor table, shared by every thread without its own */
static struct fd_slot fd_boot_slots[ZVFS_OPEN_SIZE] = {
#ifdef CONFIG_ZVFS_RESERVE_STDIO_FDS
	{.entry = &fdtable[0]},
	{.entry = &fdtable[1]},
	{.entry = &fdtable[2]},
#endif
};

static K_MUTEX_DEFINE(fdtable_lock);

#ifdef CONFIG_PROCESS
#if CONFIG_ZVFS_PROC_OPEN_MAX > 0
#define ZVFS_PROC_OPEN_SIZE CONFIG_ZVFS_PROC_OPEN_MAX
/* fd_set bitsets and the description pool are sized by the system limit */
BUILD_ASSERT(CONFIG_ZVFS_PROC_OPEN_MAX <= ZVFS_OPEN_SIZE,
	     "per-process descriptor limit exceeds the system-wide table size");
#else
#define ZVFS_PROC_OPEN_SIZE ZVFS_OPEN_SIZE
#endif

/* a process's descriptor table; kernel processes share the boot table */
struct zvfs_proc_fds {
	sys_dnode_t node;
#ifdef CONFIG_ZVFS_FS
	/* the process's working directory, a copy of its parent's at clone */
	char cwd[CONFIG_ZVFS_PATH_MAX];
#endif /* CONFIG_ZVFS_FS */
	struct fd_slot slots[ZVFS_PROC_OPEN_SIZE];
#ifdef CONFIG_ZVFS_MLOCK
	/* zvfs_mlock(ZVFS_MLOCK_FUTURE) is in force: new mappings are locked */
	bool mlock_future;
#endif /* CONFIG_ZVFS_MLOCK */
};

static struct k_spinlock fds_graveyard_lock;
static sys_dlist_t fds_graveyard = SYS_DLIST_STATIC_INIT(&fds_graveyard);

#ifdef CONFIG_SYS_PROCESS
/* one table per dynamically created process: sized as the process records */
#define FDS_POOL_MAX (SYS_PROCESS_MIN + CONFIG_SYS_PROCESS_MAX)

__aligned(WB_UP(__alignof(struct zvfs_proc_fds))) static uint8_t
	fds_pool_storage[ROUND_UP(sizeof(struct zvfs_proc_fds), __alignof(struct zvfs_proc_fds)) *
			 SYS_PROCESS_MIN];
SYS_ELASTIPOOL_DEFINE_ADVANCED(fds_pool, sizeof(struct zvfs_proc_fds),
			       __alignof(struct zvfs_proc_fds), SYS_PROCESS_MIN, FDS_POOL_MAX,
			       SYS_ELASTIPOOL_KERNEL_HEAP_ALLOC_FN(SYS_PROCESS_MIN, FDS_POOL_MAX),
			       fds_pool_storage, static);
#endif /* CONFIG_SYS_PROCESS */
#endif /* CONFIG_PROCESS */

#ifdef CONFIG_ZVFS_MLOCK
static bool boot_mlock_future;

bool *z_zvfs_proc_mlock_future(void)
{
#ifdef CONFIG_PROCESS
	struct k_process *proc = k_current_get()->process;

	if ((proc != NULL) && (proc->fds != NULL)) {
		return &((struct zvfs_proc_fds *)proc->fds)->mlock_future;
	}
#endif /* CONFIG_PROCESS */
	return &boot_mlock_future;
}
#endif /* CONFIG_ZVFS_MLOCK */

/* the calling thread's descriptor table */
static struct fd_slot *fd_slots_n(int *n)
{
#ifdef CONFIG_PROCESS
	/*
	 * Library-side user-mode callers (fileno, FILE locking) cannot read
	 * the thread struct or a heap-allocated table; they resolve the boot
	 * table, as before per-process tables. Syscall paths resolve here in
	 * kernel mode and see the caller's own table.
	 */
	if (!k_is_user_context()) {
		k_tid_t self = k_current_get();
		struct k_process *proc = (self == NULL) ? NULL : self->process;

		if ((proc != NULL) && (proc->fds != NULL)) {
			*n = ZVFS_PROC_OPEN_SIZE;
			return ((struct zvfs_proc_fds *)proc->fds)->slots;
		}
	}
#endif
	*n = ZVFS_OPEN_SIZE;
	return fd_boot_slots;
}

static struct fd_slot *fd_slots(void)
{
	int unused;

	return fd_slots_n(&unused);
}

#if defined(CONFIG_PROCESS) && defined(CONFIG_ZVFS_FS)
char *z_zvfs_proc_cwd(void)
{
	/* as fd_slots_n(): only syscall paths, in kernel mode, see the caller's table */
	if (!k_is_user_context()) {
		k_tid_t self = k_current_get();
		struct k_process *proc = (self == NULL) ? NULL : self->process;

		if ((proc != NULL) && (proc->fds != NULL)) {
			return ((struct zvfs_proc_fds *)proc->fds)->cwd;
		}
	}

	return NULL;
}
#endif /* CONFIG_PROCESS && CONFIG_ZVFS_FS */

static int fd_entry_ref(struct fd_entry *entry)
{
	return atomic_inc(&entry->refcount) + 1;
}

static int fd_entry_unref(struct fd_entry *entry)
{
	atomic_val_t old_rc;

	/* Reference counter must be checked to avoid decrement refcount below
	 * zero causing file descriptor leak. Loop statement below executes
	 * atomic decrement if refcount value is grater than zero. Otherwise,
	 * refcount is not going to be written.
	 */
	do {
		old_rc = atomic_get(&entry->refcount);
		if (!old_rc) {
			return 0;
		}
	} while (!atomic_cas(&entry->refcount, old_rc, old_rc - 1));

	if (old_rc != 1) {
		return old_rc - 1;
	}

	entry->obj = NULL;
	entry->vtable = NULL;

#ifdef CONFIG_USERSPACE
	{
		struct k_object *ko = k_object_find(entry);

		/* closed => no thread may retain a grant on this description */
		if (ko != NULL) {
			(void)memset(ko->perms, 0, sizeof(ko->perms));
			ko->flags &= ~(K_OBJ_FLAG_INITIALIZED | K_OBJ_FLAG_PUBLIC);
		}
	}
#endif

	return 0;
}

static struct fd_entry *_find_description(void)
{
	for (int i = 0; i < ARRAY_SIZE(fdtable); i++) {
		if (!atomic_get(&fdtable[i].refcount)) {
			return &fdtable[i];
		}
	}

	errno = ENFILE;
	return NULL;
}

static int _find_fd_slot_at_or_above(int minfd)
{
	int n;
	struct fd_slot *slots = fd_slots_n(&n);
	int fd;

	if (minfd < 0) {
		errno = EINVAL;
		return -1;
	}

	for (fd = minfd; fd < n; fd++) {
		if (slots[fd].entry == NULL) {
			return fd;
		}
	}

	errno = EMFILE;
	return -1;
}

int zvfs_fd_obj_refcount(void *obj)
{
	int count = 0;

	if (obj == NULL) {
		return 0;
	}

	for (int i = 0; i < ARRAY_SIZE(fdtable); i++) {
		atomic_val_t rc = atomic_get(&fdtable[i].refcount);

		if ((rc > 0) && (fdtable[i].obj == obj)) {
			count += rc;
		}
	}

#ifdef CONFIG_ZVFS_MMAP
	/* a memory mapping holds the object open like a descriptor does */
	count += z_zvfs_mmap_obj_refs(obj);
#endif /* CONFIG_ZVFS_MMAP */

	return count;
}

void zvfs_fd_offset_set(struct fd_entry *entry, size_t offset)
{
	/* descriptors sharing this description share the one offset */
	entry->offset = offset;
}

static int zvfs_dup_entry_locked(struct fd_entry *src, int newfd)
{
	struct fd_slot *slots = fd_slots();

	/* the new descriptor shares src's description, offset included */
	(void)fd_entry_ref(src);
	slots[newfd].entry = src;
	slots[newfd].flags = 0;

	return newfd;
}

int z_impl_zvfs_dup(int oldfd, int minfd)
{
	struct fd_entry *src = zvfs_fd_entry_get(oldfd);
	int fd;

	if (src == NULL) {
		return -1;
	}

	(void)k_mutex_lock(&fdtable_lock, K_FOREVER);

	fd = _find_fd_slot_at_or_above(minfd);
	if (fd >= 0) {
		fd = zvfs_dup_entry_locked(src, fd);
	}

	k_mutex_unlock(&fdtable_lock);

	return fd;
}

#ifdef CONFIG_USERSPACE
static int z_vrfy_zvfs_dup(int oldfd, int minfd)
{
	if (zvfs_fd_syscall_verify(oldfd) == NULL) {
		return -1;
	}

	return z_impl_zvfs_dup(oldfd, minfd);
}
#include <zephyr/syscalls/zvfs_dup_mrsh.c>
#endif /* CONFIG_USERSPACE */

int z_impl_zvfs_dup2(int oldfd, int newfd)
{
	struct fd_entry *src;

	if (oldfd == newfd) {
		if (zvfs_fd_entry_get(oldfd) == NULL) {
			return -1;
		}

		return newfd;
	}

	src = zvfs_fd_entry_get(oldfd);
	if (src == NULL) {
		return -1;
	}

	{
		int n;

		(void)fd_slots_n(&n);
		if (newfd < 0 || newfd >= n) {
			errno = EBADF;
			return -1;
		}
	}

	if (zvfs_fd_entry_get(newfd) != NULL) {
		if (zvfs_close(newfd) < 0) {
			return -1;
		}
	}

	(void)k_mutex_lock(&fdtable_lock, K_FOREVER);

	if (fd_slots()[newfd].entry != NULL) {
		errno = EBADF;
		k_mutex_unlock(&fdtable_lock);
		return -1;
	}

	newfd = zvfs_dup_entry_locked(src, newfd);

	k_mutex_unlock(&fdtable_lock);

	return newfd;
}

#ifdef CONFIG_USERSPACE
static int z_vrfy_zvfs_dup2(int oldfd, int newfd)
{
	if (zvfs_fd_syscall_verify(oldfd) == NULL) {
		return -1;
	}

	/*
	 * The implementation closes an in-use newfd via z_impl_zvfs_close(),
	 * which skips the ownership check; verify it here so a user thread
	 * cannot close a descriptor it has no K_OBJ_FILE grant for. An unused
	 * newfd returns NULL, which is not an error.
	 */
	if (oldfd != newfd) {
		(void)zvfs_fd_syscall_verify(newfd);
	}

	return z_impl_zvfs_dup2(oldfd, newfd);
}
#include <zephyr/syscalls/zvfs_dup2_mrsh.c>
#endif /* CONFIG_USERSPACE */

struct fd_slot *zvfs_fd_slot_get(int fd)
{
	int n;
	struct fd_slot *slots = fd_slots_n(&n);

	if ((fd < 0) || (fd >= n)) {
		errno = EBADF;
		return NULL;
	}

	fd = k_array_index_sanitize(fd, n);

	if (slots[fd].entry == NULL) {
		errno = EBADF;
		return NULL;
	}

	return &slots[fd];
}

struct fd_entry *zvfs_fd_entry_get(int fd)
{
	struct fd_slot *slot = zvfs_fd_slot_get(fd);

	return (slot == NULL) ? NULL : slot->entry;
}

struct fd_entry *zvfs_fd_syscall_verify(int fd)
{
	struct fd_entry *entry = zvfs_fd_entry_get(fd);

#if defined(CONFIG_USERSPACE)
	if (entry != NULL && k_is_in_user_syscall()) {
		K_OOPS(K_SYSCALL_OBJ(entry, K_OBJ_FILE));
	}
#endif

	return entry;
}

static int _check_fd(int fd)
{
	if (zvfs_fd_entry_get(fd) == NULL) {
		return -1;
	}

	return 0;
}

#if defined(CONFIG_ZTEST) && defined(CONFIG_USERSPACE)
void *zvfs_test_fd_entry_kobj(int fd)
{
	int n;
	struct fd_slot *slots = fd_slots_n(&n);

	if ((fd < 0) || (fd >= n)) {
		return NULL;
	}

	return slots[k_array_index_sanitize(fd, n)].entry;
}
#endif

void *zvfs_get_fd_obj(int fd, const struct fd_op_vtable *vtable, int err)
{
	struct fd_entry *entry;

	entry = zvfs_fd_syscall_verify(fd);
	if (entry == NULL) {
		return NULL;
	}

	if ((vtable != NULL) && (entry->vtable != vtable)) {
		errno = err;
		return NULL;
	}

	return entry->obj;
}

bool zvfs_get_obj_lock_and_cond(void *obj, const struct fd_op_vtable *vtable, struct k_mutex **lock,
				struct k_condvar **cond)
{
	struct fd_entry *entry = NULL;

	for (int i = 0; i < ARRAY_SIZE(fdtable); i++) {
		if (atomic_get(&fdtable[i].refcount) && fdtable[i].obj == obj &&
		    fdtable[i].vtable == vtable) {
			entry = &fdtable[i];
			break;
		}
	}

	if (entry == NULL) {
		errno = ENFILE;
		return false;
	}

	if (lock) {
		*lock = &entry->lock;
	}

	if (cond) {
		*cond = &entry->cond;
	}

	return true;
}

void *zvfs_get_fd_obj_and_vtable(int fd, const struct fd_op_vtable **vtable, struct k_mutex **lock)
{
	struct fd_entry *entry;

	entry = zvfs_fd_syscall_verify(fd);
	if (entry == NULL) {
		return NULL;
	}

	*vtable = entry->vtable;

	if (lock != NULL) {
		*lock = &entry->lock;
	}

	return entry->obj;
}

int zvfs_reserve_fd(void)
{
	struct fd_entry *entry;
	int fd;

	(void)k_mutex_lock(&fdtable_lock, K_FOREVER);

	fd = _find_fd_slot_at_or_above(0);
	if (fd >= 0) {
		/* EMFILE from slot exhaustion, ENFILE when the pool is dry */
		entry = _find_description();
		if (entry == NULL) {
			fd = -1;
		} else {
#if defined(CONFIG_USERSPACE)
			k_object_recycle(entry);
#endif
			/* Mark description as used, zvfs_finalize_fd() will fill it in. */
			(void)fd_entry_ref(entry);
			entry->obj = NULL;
			entry->vtable = NULL;
			entry->offset = 0;
			k_mutex_init(&entry->lock);
			k_condvar_init(&entry->cond);
			fd_slots()[fd].entry = entry;
			fd_slots()[fd].flags = 0;
		}
	}

	k_mutex_unlock(&fdtable_lock);

	return fd;
}

void zvfs_finalize_typed_fd(int fd, void *obj, const struct fd_op_vtable *vtable, uint32_t mode)
{
	struct fd_entry *entry = fd_slots()[fd].entry;

	/* Assumes fd was already bounds-checked. */
#ifdef CONFIG_USERSPACE
	/* descriptor context objects are inserted into the table when they
	 * are ready for use. Mark the object as initialized and grant the
	 * caller (and only the caller) access.
	 *
	 * This call is a no-op if obj is invalid or points to something
	 * not a kernel object.
	 */
	k_object_recycle(obj);
#endif
	entry->obj = obj;
	entry->vtable = vtable;
	entry->mode = mode;

	/* Let the object know about the lock just in case it needs it
	 * for something. For BSD sockets, the lock is used with condition
	 * variables to avoid keeping the lock for a long period of time.
	 */
	if (vtable && vtable->ioctl) {
		int prev_errno = errno;

		(void)zvfs_fdtable_call_ioctl(vtable, obj, ZFD_IOCTL_SET_LOCK, &entry->lock);
		if ((prev_errno != EOPNOTSUPP) && (errno == EOPNOTSUPP)) {
			/* restore backed-up errno value if the backend does not support locking */
			errno = prev_errno;
		}
	}
}

void zvfs_free_fd(int fd)
{
	struct fd_slot *slot;

	(void)k_mutex_lock(&fdtable_lock, K_FOREVER);

	slot = zvfs_fd_slot_get(fd);
	if (slot != NULL) {
		struct fd_entry *entry = slot->entry;

		slot->entry = NULL;
		slot->flags = 0;
		(void)fd_entry_unref(entry);
	}

	k_mutex_unlock(&fdtable_lock);
}

int zvfs_alloc_fd(void *obj, const struct fd_op_vtable *vtable)
{
	int fd;

	fd = zvfs_reserve_fd();
	if (fd >= 0) {
		zvfs_finalize_fd(fd, obj, vtable);
	}

	return fd;
}

FILE *zvfs_fdopen(int fd, const char *mode)
{
	ARG_UNUSED(mode);

	if (_check_fd(fd) < 0) {
		return NULL;
	}

	return (FILE *)fd_slots()[fd].entry;
}

int zvfs_fileno(FILE *file)
{
	int n;
	struct fd_slot *slots;

	if (!IS_ARRAY_ELEMENT(fdtable, file)) {
		errno = EBADF;
		return -1;
	}

	slots = fd_slots_n(&n);
	for (int fd = 0; fd < n; fd++) {
		if (slots[fd].entry == (struct fd_entry *)file) {
			return fd;
		}
	}

	errno = EBADF;
	return -1;
}

#ifdef CONFIG_PROCESS
#ifdef CONFIG_SYS_PROCESS
/* mirror zvfs_close() for a descriptor no thread can resolve anymore */
static void fds_orphan_close(struct fd_slot *slot, int fd)
{
	struct fd_entry *entry = slot->entry;

	(void)k_mutex_lock(&entry->lock, K_FOREVER);
	if (entry->vtable != NULL && entry->vtable->close != NULL && entry->obj != NULL &&
	    zvfs_fd_obj_refcount(entry->obj) <= 1) {
		if (entry->mode & ZVFS_MODE_IFSOCK) {
			(void)entry->vtable->close2(entry->obj, fd);
		} else {
			(void)entry->vtable->close(entry->obj);
		}
	}
	k_mutex_unlock(&entry->lock);

	slot->entry = NULL;
	slot->flags = 0;
	(void)fd_entry_unref(entry);
}

static void fds_graveyard_drain(void)
{
	while (true) {
		sys_dnode_t *node = NULL;
		struct zvfs_proc_fds *t;

		K_SPINLOCK(&fds_graveyard_lock) {
			node = sys_dlist_get(&fds_graveyard);
		}
		if (node == NULL) {
			break;
		}
		t = CONTAINER_OF(node, struct zvfs_proc_fds, node);
		for (int fd = 0; fd < ZVFS_PROC_OPEN_SIZE; fd++) {
			if (t->slots[fd].entry != NULL) {
				fds_orphan_close(&t->slots[fd], fd);
			}
		}
		K_SPINLOCK(&fds_graveyard_lock) {
			(void)sys_elastipool_free(&fds_pool, t);
		}
	}
}

int z_zvfs_fds_clone(struct k_process *child, k_tid_t leader)
{
	int n;
	struct fd_slot *parent;
	struct zvfs_proc_fds *t;

	ARG_UNUSED(leader);

	fds_graveyard_drain();

	t = NULL;
	K_SPINLOCK(&fds_graveyard_lock) {
		(void)sys_elastipool_alloc(&fds_pool, (void **)&t);
	}
	if (t == NULL) {
		return -ENOMEM;
	}
	memset(t, 0, sizeof(*t));
	sys_dnode_init(&t->node);
#ifdef CONFIG_ZVFS_FS
	z_zvfs_cwd_copy(t->cwd, sizeof(t->cwd));
#endif /* CONFIG_ZVFS_FS */

	(void)k_mutex_lock(&fdtable_lock, K_FOREVER);
	parent = fd_slots_n(&n);
	for (int fd = 0; fd < MIN(n, ZVFS_PROC_OPEN_SIZE); fd++) {
		struct fd_entry *entry = parent[fd].entry;

		if (entry == NULL) {
			continue;
		}
		/* the child's descriptor shares the description, flags copied */
		(void)fd_entry_ref(entry);
		t->slots[fd].entry = entry;
		t->slots[fd].flags = parent[fd].flags;
#ifdef CONFIG_USERSPACE
		k_object_access_grant(entry, leader);
		if ((entry->obj != NULL) && (k_object_find(entry->obj) != NULL)) {
			k_object_access_grant(entry->obj, leader);
		}
#endif
	}
	child->fds = t;
	k_mutex_unlock(&fdtable_lock);

	return 0;
}

void zvfs_fds_cloexec(void)
{
	int n;
	struct fd_slot *slots = fd_slots_n(&n);

	/* close every descriptor the caller marked FD_CLOEXEC */
	for (int fd = 0; fd < n; fd++) {
		if ((slots[fd].entry != NULL) && ((slots[fd].flags & ZVFS_FD_CLOEXEC) != 0)) {
			(void)zvfs_close(fd);
		}
	}
}
#endif /* CONFIG_SYS_PROCESS */

void z_process_fds_exit(void)
{
	int n;
	struct fd_slot *slots = fd_slots_n(&n);

	/* kernel processes share the boot table; it is never swept */
	if (slots == fd_boot_slots) {
		return;
	}

	for (int fd = 0; fd < n; fd++) {
		if (slots[fd].entry != NULL) {
			(void)zvfs_close(fd);
		}
	}
}

void z_process_fds_drop(struct k_process *proc)
{
	struct zvfs_proc_fds *t = proc->fds;

	if (t == NULL) {
		return;
	}
	proc->fds = NULL;

	/* the caller holds process_lock: closes and frees defer to the drain */
	K_SPINLOCK(&fds_graveyard_lock) {
		sys_dlist_append(&fds_graveyard, &t->node);
	}
}

#ifdef CONFIG_SYS_PROCESS
/*
 * Child-table operations: the child is created paused, so its table has
 * no concurrent user; the caller's table and the shared pool are covered
 * by fdtable_lock.
 */
static struct zvfs_proc_fds *child_fds(k_tid_t child)
{
	struct k_process *proc = (child == NULL) ? NULL : child->process;

	return (proc == NULL) ? NULL : proc->fds;
}

int z_zvfs_fds_child_set(k_tid_t child, int childfd, int parentfd)
{
	int ret = 0;
	struct zvfs_proc_fds *t = child_fds(child);

	if ((t == NULL) || (childfd < 0) || (childfd >= ZVFS_PROC_OPEN_SIZE)) {
		return -EBADF;
	}

	(void)k_mutex_lock(&fdtable_lock, K_FOREVER);

	struct fd_slot *src = zvfs_fd_slot_get(parentfd);

	if (src == NULL) {
		ret = -EBADF;
	} else {
		if (t->slots[childfd].entry != NULL) {
			fds_orphan_close(&t->slots[childfd], childfd);
		}
		(void)fd_entry_ref(src->entry);
		t->slots[childfd].entry = src->entry;
		t->slots[childfd].flags = src->flags;
#ifdef CONFIG_USERSPACE
		k_object_access_grant(src->entry, child);
		if ((src->entry->obj != NULL) && (k_object_find(src->entry->obj) != NULL)) {
			k_object_access_grant(src->entry->obj, child);
		}
#endif
	}

	k_mutex_unlock(&fdtable_lock);

	return ret;
}

int z_zvfs_fds_child_dup2(k_tid_t child, int oldfd, int newfd)
{
	int ret = 0;
	struct zvfs_proc_fds *t = child_fds(child);

	if ((t == NULL) || (oldfd < 0) || (oldfd >= ZVFS_PROC_OPEN_SIZE) || (newfd < 0) ||
	    (newfd >= ZVFS_PROC_OPEN_SIZE)) {
		return -EBADF;
	}

	(void)k_mutex_lock(&fdtable_lock, K_FOREVER);

	if (t->slots[oldfd].entry == NULL) {
		ret = -EBADF;
	} else if (oldfd != newfd) {
		if (t->slots[newfd].entry != NULL) {
			fds_orphan_close(&t->slots[newfd], newfd);
		}
		(void)fd_entry_ref(t->slots[oldfd].entry);
		t->slots[newfd].entry = t->slots[oldfd].entry;
		/* dup2() semantics: the new descriptor's FD_CLOEXEC is clear */
		t->slots[newfd].flags = 0;
	}

	k_mutex_unlock(&fdtable_lock);

	return ret;
}

int z_zvfs_fds_child_close(k_tid_t child, int fd)
{
	int ret = 0;
	struct zvfs_proc_fds *t = child_fds(child);

	if ((t == NULL) || (fd < 0) || (fd >= ZVFS_PROC_OPEN_SIZE)) {
		return -EBADF;
	}

	(void)k_mutex_lock(&fdtable_lock, K_FOREVER);

	if (t->slots[fd].entry == NULL) {
		ret = -EBADF;
	} else {
		fds_orphan_close(&t->slots[fd], fd);
	}

	k_mutex_unlock(&fdtable_lock);

	return ret;
}
#endif /* CONFIG_SYS_PROCESS */
#endif /* CONFIG_PROCESS */

#if defined(CONFIG_POSIX_DEVICE_IO) || defined(CONFIG_ZVFS_RESERVE_STDIO_FDS)
/*
 * fd operations for stdio/stdout/stderr
 */

int z_impl_zephyr_write_stdout(const char *buf, int nbytes);

static ssize_t stdinout_read_vmeth(void *obj, void *buffer, size_t count)
{
	return 0;
}

static ssize_t stdinout_write_vmeth(void *obj, const void *buffer, size_t count)
{
#if defined(CONFIG_PICOLIBC) || defined(CONFIG_NEWLIB_LIBC) || defined(CONFIG_ARCMWDT_LIBC) || defined(CONFIG_MINIMAL_LIBC)
	return z_impl_zephyr_write_stdout(buffer, count);
#else
	return 0;
#endif
}

static int stdinout_ioctl_vmeth(void *obj, unsigned int request, va_list args)
{
	switch (request) {
	case ZVFS_F_GETFL:
		return ZVFS_O_RDWR;
	case ZVFS_F_SETFL:
		/* nothing the console honors yet; accept for fcntl() users */
		return 0;
	default:
		errno = EINVAL;
		return -1;
	}
}

static const struct fd_op_vtable stdinout_fd_op_vtable = {
	.read = stdinout_read_vmeth,
	.write = stdinout_write_vmeth,
	.ioctl = stdinout_ioctl_vmeth,
};

#endif /* defined(CONFIG_POSIX_DEVICE_IO) || defined(CONFIG_ZVFS_RESERVE_STDIO_FDS) */

#if defined(CONFIG_USERSPACE) && defined(CONFIG_ZVFS_RESERVE_STDIO_FDS)
static int stdinout_access_init(void)
{
	/* all threads share stdin/stdout/stderr */
	for (int fd = 0; fd < 3; fd++) {
		k_object_init(&fdtable[fd]);
		k_object_access_all_grant(&fdtable[fd]);
	}

	return 0;
}
SYS_INIT(stdinout_access_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif
