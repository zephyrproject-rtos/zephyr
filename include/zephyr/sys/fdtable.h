/*
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_SYS_FDTABLE_H_
#define ZEPHYR_INCLUDE_SYS_FDTABLE_H_

#include <stdarg.h>
#include <sys/types.h>
/* FIXME: For native_posix ssize_t, off_t. */
#include <zephyr/fs/fs.h>
#include <zephyr/sys/mutex.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * File descriptor virtual method table.
 * Currently all operations beyond read/write/close go thru ioctl method.
 */
struct fd_op_vtable {
	ssize_t (*read)(void *obj, void *buf, size_t sz);
	ssize_t (*write)(void *obj, const void *buf, size_t sz);
	int (*close)(void *obj);
	int (*ioctl)(void *obj, unsigned int request, va_list args);
};

/**
 * @brief Reserve file descriptor.
 *
 * This function allows to reserve a space for file descriptor entry in
 * the underlying table, and thus allows caller to fail fast if no free
 * descriptor is available. If this function succeeds, z_finalize_fd()
 * or z_free_fd() must be called mandatorily.
 *
 * @return Allocated file descriptor, or -1 in case of error (errno is set)
 */
int z_reserve_fd(void);

/**
 * @brief Finalize creation of file descriptor.
 *
 * This function should be called exactly once after z_reserve_fd(), and
 * should not be called in any other case.
 *
 * @param fd File descriptor previously returned by z_reserve_fd()
 * @param obj pointer to I/O object structure
 * @param vtable pointer to I/O operation implementations for the object
 */
void z_finalize_fd(int fd, void *obj, const struct fd_op_vtable *vtable);

/**
 * @brief Allocate file descriptor for underlying I/O object.
 *
 * This function combines operations of z_reserve_fd() and z_finalize_fd()
 * in one step, and provided for convenience.
 *
 * @param obj pointer to I/O object structure
 * @param vtable pointer to I/O operation implementations for the object
 *
 * @return Allocated file descriptor, or -1 in case of error (errno is set)
 */
int z_alloc_fd(void *obj, const struct fd_op_vtable *vtable);

/**
 * @brief Release reserved file descriptor.
 *
 * This function may be called once after z_reserve_fd(), and should
 * not be called in any other case.
 *
 * @param fd File descriptor previously returned by z_reserve_fd()
 */
void z_free_fd(int fd);

/**
 * @brief Get underlying object pointer from file descriptor.
 *
 * This function is useful for functions other than read/write/ioctl
 * to look up underlying I/O object by fd, optionally checking its
 * type (using vtable reference). If fd refers to invalid entry,
 * NULL will be returned with errno set to EBADF. If fd is valid,
 * but vtable param is not NULL and doesn't match object's vtable,
 * NULL is returned and errno set to err param.
 *
 * @param fd File descriptor previously returned by z_reserve_fd()
 * @param vtable Expected object vtable or NULL
 * @param err errno value to set if object vtable doesn't match
 *
 * @return Object pointer or NULL, with errno set
 */
void *z_get_fd_obj(int fd, const struct fd_op_vtable *vtable, int err);

/**
 * @brief Get underlying object pointer and vtable pointer from file descriptor.
 *
 * @param fd File descriptor previously returned by z_reserve_fd()
 * @param vtable A pointer to a pointer variable to store the vtable
 * @param lock An optional pointer to a pointer variable to store the mutex
 *        preventing concurrent descriptor access. The lock is not taken,
 *        it is just returned for the caller to use if necessary. Pass NULL
 *        if the lock is not needed by the caller.
 *
 * @return Object pointer or NULL, with errno set
 */
void *z_get_fd_obj_and_vtable(int fd, const struct fd_op_vtable **vtable,
			      struct k_mutex **lock);

/**
 * @brief Call ioctl vmethod on an object using varargs.
 *
 * We need this helper function because ioctl vmethod is declared to
 * take va_list and the only portable way to construct va_list is from
 * function's ... parameters.
 *
 * @param vtable vtable containing ioctl function pointer
 * @param obj Object to call ioctl on
 * @param request ioctl request number
 * @param ... Variadic arguments to ioctl
 */
static inline int z_fdtable_call_ioctl(const struct fd_op_vtable *vtable, void *obj,
				       unsigned long request, ...)
{
	va_list args;
	int res;

	va_start(args, request);
	res = vtable->ioctl(obj, request, args);
	va_end(args);

	return res;
}

/**
 * Request codes for fd_op_vtable.ioctl().
 *
 * Note that these codes are internal Zephyr numbers, for internal
 * Zephyr operations (and subject to change without notice, not part
 * of "stable ABI"). These are however expected to co-exist with
 * "well-known" POSIX/Linux ioctl numbers, and not clash with them.
 */
enum {
	/* Codes below 0x100 are reserved for fcntl() codes. */
	ZFD_IOCTL_FSYNC = 0x100,
	ZFD_IOCTL_LSEEK,
	ZFD_IOCTL_POLL_PREPARE,
	ZFD_IOCTL_POLL_UPDATE,
	ZFD_IOCTL_POLL_OFFLOAD,
	ZFD_IOCTL_SET_LOCK,
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_FDTABLE_H_ */
