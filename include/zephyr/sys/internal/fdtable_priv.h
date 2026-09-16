/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_INTERNAL_FDTABLE_PRIV_H_
#define ZEPHYR_INCLUDE_SYS_INTERNAL_FDTABLE_PRIV_H_

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

struct fd_op_vtable;

/* an open file description; shared by dup'd and inherited descriptors */
struct fd_entry {
	void *obj;
	const struct fd_op_vtable *vtable;
	atomic_t refcount;
	struct k_mutex lock;
	struct k_condvar cond;
	size_t offset;
	uint32_t mode;
};

/* a file descriptor: one slot of a descriptor table */
struct fd_slot {
	struct fd_entry *entry;
	uint8_t flags; /* ZVFS_FD_CLOEXEC */
};

struct fd_slot *zvfs_fd_slot_get(int fd);
struct fd_entry *zvfs_fd_entry_get(int fd);
struct fd_entry *zvfs_fd_syscall_verify(int fd);

struct k_process;
#if defined(CONFIG_ZVFS_FDTABLE) && defined(CONFIG_ZVFS_FS) && defined(CONFIG_PROCESS)
/* the caller's process working directory buffer; NULL shares the boot directory */
char *z_zvfs_proc_cwd(void);
#else
static inline char *z_zvfs_proc_cwd(void)
{
	return NULL;
}
#endif

#ifdef CONFIG_ZVFS_FS
/* snapshot the caller's working directory into @a buf */
void z_zvfs_cwd_copy(char *buf, size_t size);
#endif /* CONFIG_ZVFS_FS */

#if defined(CONFIG_ZVFS_FDTABLE) && defined(CONFIG_SYS_PROCESS)
/* give @a child its own descriptor table: a copy of the caller's */
int z_zvfs_fds_clone(struct k_process *child, k_tid_t leader);
/*
 * Operations on a paused child's table: install the caller's @a parentfd
 * as the child's @a childfd, dup within the child's table, and close in
 * the child's table.
 */
int z_zvfs_fds_child_set(k_tid_t child, int childfd, int parentfd);
int z_zvfs_fds_child_dup2(k_tid_t child, int oldfd, int newfd);
int z_zvfs_fds_child_close(k_tid_t child, int fd);
/* close every descriptor in the caller's table marked FD_CLOEXEC */
void zvfs_fds_cloexec(void);
#else
static inline int z_zvfs_fds_clone(struct k_process *child, k_tid_t leader)
{
	ARG_UNUSED(child);
	ARG_UNUSED(leader);
	return 0;
}

static inline int z_zvfs_fds_child_set(k_tid_t child, int childfd, int parentfd)
{
	ARG_UNUSED(child);
	ARG_UNUSED(childfd);
	ARG_UNUSED(parentfd);
	return 0;
}

static inline int z_zvfs_fds_child_dup2(k_tid_t child, int oldfd, int newfd)
{
	ARG_UNUSED(child);
	ARG_UNUSED(oldfd);
	ARG_UNUSED(newfd);
	return 0;
}

static inline int z_zvfs_fds_child_close(k_tid_t child, int fd)
{
	ARG_UNUSED(child);
	ARG_UNUSED(fd);
	return 0;
}

static inline void zvfs_fds_cloexec(void)
{
}
#endif

#ifdef CONFIG_ZVFS_MLOCK
/* the caller's "lock every new mapping" flag: per process, one for the kernel's own */
bool *z_zvfs_proc_mlock_future(void);
#endif

struct k_mem_domain;

#if defined(CONFIG_ZVFS_MMAP) && defined(CONFIG_PROCESS_VM)
/* give @a child, whose address space is a copy of the caller's, the caller's mappings */
int z_zvfs_mmap_clone(struct k_process *child, struct k_mem_domain *domain);
/* a shared mapping of the caller starts at @a start: its pages are not to be copied */
bool z_zvfs_mmap_is_shared(uintptr_t start);
#else
static inline int z_zvfs_mmap_clone(struct k_process *child, struct k_mem_domain *domain)
{
	ARG_UNUSED(child);
	ARG_UNUSED(domain);
	return 0;
}

static inline bool z_zvfs_mmap_is_shared(uintptr_t start)
{
	ARG_UNUSED(start);
	return false;
}
#endif

/** zvfs_poll_prepare_fd() armed *pev with the fd's wait condition(s) */
#define ZVFS_POLL_PREPARED 0
/** zvfs_poll_prepare_fd() found the fd ready without waiting */
#define ZVFS_POLL_READY    1

struct k_poll_event;
struct zvfs_flock;
struct zvfs_pollfd;

int zvfs_poll_prepare_fd(struct zvfs_pollfd *pfd, struct k_poll_event **pev,
			 struct k_poll_event *pev_end);
int zvfs_poll_update_fd(struct zvfs_pollfd *pfd, struct k_poll_event **pev);

int zvfs_fd_obj_refcount(void *obj);
/* number of memory mappings referencing an I/O object (CONFIG_ZVFS_MMAP) */
int z_zvfs_mmap_obj_refs(const void *obj);

void zvfs_fd_offset_set(struct fd_entry *entry, size_t offset);

bool supports_pread_pwrite(uint32_t mode);
int zvfs_rw_unlocked(int fd, void *buf, size_t sz, bool is_write, const size_t *from_offset);

int zvfs_lock_file(FILE *file, k_timeout_t timeout);
int zvfs_unlock_file(FILE *file);
int zvfs_getc_unlocked(FILE *stream);
int zvfs_putc_unlocked(int c, FILE *stream);

int zvfs_record_lock_fcntl(struct fd_entry *entry, int cmd, struct zvfs_flock *lock);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_INTERNAL_FDTABLE_PRIV_H_ */
