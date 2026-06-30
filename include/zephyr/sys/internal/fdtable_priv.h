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

struct fd_entry {
	void *obj;
	const struct fd_op_vtable *vtable;
	atomic_t refcount;
	struct k_mutex lock;
	struct k_condvar cond;
	size_t offset;
	uint32_t mode;
};

struct fd_entry *zvfs_fd_entry_get(int fd);
struct fd_entry *zvfs_fd_syscall_verify(int fd);

int zvfs_fd_obj_refcount(void *obj);

void zvfs_fd_offset_set(struct fd_entry *entry, size_t offset);

/** zvfs_poll_prepare_fd() armed *pev with the fd's wait condition(s) */
#define ZVFS_POLL_PREPARED 0
/** zvfs_poll_prepare_fd() found the fd ready without waiting */
#define ZVFS_POLL_READY    1

struct k_poll_event;
struct zvfs_pollfd;

int zvfs_poll_prepare_fd(struct zvfs_pollfd *pfd, struct k_poll_event **pev,
			 struct k_poll_event *pev_end);
int zvfs_poll_update_fd(struct zvfs_pollfd *pfd, struct k_poll_event **pev);

bool supports_pread_pwrite(uint32_t mode);
int zvfs_rw_unlocked(int fd, void *buf, size_t sz, bool is_write, const size_t *from_offset);

int zvfs_lock_file(FILE *file, k_timeout_t timeout);
int zvfs_unlock_file(FILE *file);
int zvfs_getc_unlocked(FILE *stream);
int zvfs_putc_unlocked(int c, FILE *stream);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_INTERNAL_FDTABLE_PRIV_H_ */
