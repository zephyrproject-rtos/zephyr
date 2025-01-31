/*
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_SYS_FDTABLE_H_
#define ZEPHYR_INCLUDE_SYS_FDTABLE_H_

#include <stdarg.h>
#include <time.h>

/* FIXME: For native_posix ssize_t, off_t. */
#include <sys/types.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#ifdef CONFIG_PICOLIBC
#define ZVFS_O_APPEND 0x0400
#define ZVFS_O_CREAT  0x0040
#define ZVFS_O_TRUNC  0x0200
#else
#define ZVFS_O_APPEND 0x0008
#define ZVFS_O_CREAT  0x0200
#define ZVFS_O_TRUNC  0x0400
#endif

#define ZVFS_O_RDONLY 00
#define ZVFS_O_WRONLY 01
#define ZVFS_O_RDWR   02

#define ZVFS_O_EXCL     0x0800
#define ZVFS_O_NONBLOCK 0x4000

#define ZVFS_F_DUPFD 0
#define ZVFS_F_GETFL 3
#define ZVFS_F_SETFL 4

/* File mode bits */
#define ZVFS_MODE_IFMT   0170000
#define ZVFS_MODE_UNSPEC 0000000
#define ZVFS_MODE_IFIFO  0010000
#define ZVFS_MODE_IFCHR  0020000
#define ZVFS_MODE_IMSGQ  0030000
#define ZVFS_MODE_IFDIR  0040000
#define ZVFS_MODE_IFSEM  0050000
#define ZVFS_MODE_IFBLK  0060000
#define ZVFS_MODE_IFSHM  0070000
#define ZVFS_MODE_IFREG  0100000
#define ZVFS_MODE_IFLNK  0120000
#define ZVFS_MODE_IFSOCK 0140000

#define ZVFS_POLLIN   BIT(0)
#define ZVFS_POLLPRI  BIT(1)
#define ZVFS_POLLOUT  BIT(2)
#define ZVFS_POLLERR  BIT(3)
#define ZVFS_POLLHUP  BIT(4)
#define ZVFS_POLLNVAL BIT(5)

#ifdef __cplusplus
extern "C" {
#endif

/* FIXME: use k_off_t and k_ssize_t to avoid the POSIX->Zephyr->POSIX dependency cycle */
#ifdef CONFIG_NEWLIB_LIBC
#ifndef _OFF_T_DECLARED
typedef __off_t off_t;
#define _OFF_T_DECLARED
#endif
#ifndef _SSIZE_T_DECLARED
typedef _ssize_t ssize_t;
#define _SSIZE_T_DECLARED
#endif
#endif

/**
 * File descriptor virtual method table.
 * Currently all operations beyond read/write/close go thru ioctl method.
 */
struct fd_op_vtable {
	union {
		ssize_t (*read)(void *obj, void *buf, size_t sz);
		ssize_t (*read_offs)(void *obj, void *buf, size_t sz, size_t offset);
	};
	union {
		ssize_t (*write)(void *obj, const void *buf, size_t sz);
		ssize_t (*write_offs)(void *obj, const void *buf, size_t sz, size_t offset);
	};
	union {
		int (*close)(void *obj);
		int (*close2)(void *obj, int fd);
	};
	int (*ioctl)(void *obj, unsigned int request, va_list args);
};

/**
 * @brief Reserve file descriptor.
 *
 * This function allows to reserve a space for file descriptor entry in
 * the underlying table, and thus allows caller to fail fast if no free
 * descriptor is available. If this function succeeds, zvfs_finalize_fd()
 * or zvfs_free_fd() must be called mandatorily.
 *
 * @return Allocated file descriptor, or -1 in case of error (errno is set)
 */
int zvfs_reserve_fd(void);

/**
 * @brief Finalize creation of file descriptor, with type.
 *
 * This function should be called exactly once after zvfs_reserve_fd(), and
 * should not be called in any other case.
 *
 * The difference between this function and @ref zvfs_finalize_fd is that the
 * latter does not relay type information of the created file descriptor.
 *
 * Values permitted for @a mode are one of `ZVFS_MODE_..`.
 *
 * @param fd File descriptor previously returned by zvfs_reserve_fd()
 * @param obj pointer to I/O object structure
 * @param vtable pointer to I/O operation implementations for the object
 * @param mode File type as specified above.
 */
void zvfs_finalize_typed_fd(int fd, void *obj, const struct fd_op_vtable *vtable, uint32_t mode);

/**
 * @brief Finalize creation of file descriptor.
 *
 * This function should be called exactly once after zvfs_reserve_fd(), and
 * should not be called in any other case.
 *
 * @param fd File descriptor previously returned by zvfs_reserve_fd()
 * @param obj pointer to I/O object structure
 * @param vtable pointer to I/O operation implementations for the object
 */
static inline void zvfs_finalize_fd(int fd, void *obj, const struct fd_op_vtable *vtable)
{
	zvfs_finalize_typed_fd(fd, obj, vtable, ZVFS_MODE_UNSPEC);
}

/**
 * @brief Allocate file descriptor for underlying I/O object.
 *
 * This function combines operations of zvfs_reserve_fd() and zvfs_finalize_fd()
 * in one step, and provided for convenience.
 *
 * @param obj pointer to I/O object structure
 * @param vtable pointer to I/O operation implementations for the object
 *
 * @return Allocated file descriptor, or -1 in case of error (errno is set)
 */
int zvfs_alloc_fd(void *obj, const struct fd_op_vtable *vtable);

/**
 * @brief Release reserved file descriptor.
 *
 * This function may be called once after zvfs_reserve_fd(), and should
 * not be called in any other case.
 *
 * @param fd File descriptor previously returned by zvfs_reserve_fd()
 */
void zvfs_free_fd(int fd);

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
 * @param fd File descriptor previously returned by zvfs_reserve_fd()
 * @param vtable Expected object vtable or NULL
 * @param err errno value to set if object vtable doesn't match
 *
 * @return Object pointer or NULL, with errno set
 */
void *zvfs_get_fd_obj(int fd, const struct fd_op_vtable *vtable, int err);

/**
 * @brief Get underlying object pointer and vtable pointer from file descriptor.
 *
 * @param fd File descriptor previously returned by zvfs_reserve_fd()
 * @param vtable A pointer to a pointer variable to store the vtable
 * @param lock An optional pointer to a pointer variable to store the mutex
 *        preventing concurrent descriptor access. The lock is not taken,
 *        it is just returned for the caller to use if necessary. Pass NULL
 *        if the lock is not needed by the caller.
 *
 * @return Object pointer or NULL, with errno set
 */
void *zvfs_get_fd_obj_and_vtable(int fd, const struct fd_op_vtable **vtable,
			      struct k_mutex **lock);

/**
 * @brief Get the mutex and condition variable associated with the given object and vtable.
 *
 * @param obj Object previously returned by a call to e.g. @ref zvfs_get_fd_obj.
 * @param vtable A pointer the vtable associated with @p obj.
 * @param lock An optional pointer to a pointer variable to store the mutex
 *        preventing concurrent descriptor access. The lock is not taken,
 *        it is just returned for the caller to use if necessary. Pass NULL
 *        if the lock is not needed by the caller.
 * @param cond An optional pointer to a pointer variable to store the condition variable
 *        to notify waiting threads in the case of concurrent descriptor access. Pass NULL
 *        if the condition variable is not needed by the caller.
 *
 * @return `true` on success, `false` otherwise.
 */
bool zvfs_get_obj_lock_and_cond(void *obj, const struct fd_op_vtable *vtable, struct k_mutex **lock,
			     struct k_condvar **cond);

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
static inline int zvfs_fdtable_call_ioctl(const struct fd_op_vtable *vtable, void *obj,
				       unsigned long request, ...)
{
	va_list args;
	int res;

	va_start(args, request);
	res = vtable->ioctl(obj, request, args);
	va_end(args);

	return res;
}

struct zvfs_pollfd {
	int fd;
	short events;
	short revents;
};

__syscall int zvfs_poll(struct zvfs_pollfd *fds, int nfds, int poll_timeout);

struct zvfs_fd_set {
	uint32_t bitset[(CONFIG_ZVFS_OPEN_MAX + 31) / 32];
};

/** @brief Number of file descriptors which can be added @ref zvfs_fd_set */
#define ZVFS_FD_SETSIZE (sizeof(((struct zvfs_fd_set *)0)->bitset) * 8)

void ZVFS_FD_CLR(int fd, struct zvfs_fd_set *fdset);
int ZVFS_FD_ISSET(int fd, struct zvfs_fd_set *fdset);
void ZVFS_FD_SET(int fd, struct zvfs_fd_set *fdset);
void ZVFS_FD_ZERO(struct zvfs_fd_set *fdset);

__syscall int zvfs_select(int nfds, struct zvfs_fd_set *ZRESTRICT readfds,
			  struct zvfs_fd_set *ZRESTRICT writefds,
			  struct zvfs_fd_set *ZRESTRICT errorfds,
			  const struct timespec *ZRESTRICT timeout, const void *ZRESTRICT sigmask);

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
	ZFD_IOCTL_STAT,
	ZFD_IOCTL_TRUNCATE,
	ZFD_IOCTL_MMAP,

	/* Codes above 0x5400 and below 0x5500 are reserved for termios, FIO, etc */
	ZFD_IOCTL_FIONREAD = 0x541B,
	ZFD_IOCTL_FIONBIO = 0x5421,
};

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/fdtable.h>

#endif /* ZEPHYR_INCLUDE_SYS_FDTABLE_H_ */
