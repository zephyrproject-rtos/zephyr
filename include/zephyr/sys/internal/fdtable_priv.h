/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_INTERNAL_FDTABLE_PRIV_H_
#define ZEPHYR_INCLUDE_SYS_INTERNAL_FDTABLE_PRIV_H_

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

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

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_INTERNAL_FDTABLE_PRIV_H_ */
