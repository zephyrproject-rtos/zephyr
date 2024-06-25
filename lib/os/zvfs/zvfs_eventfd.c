/*
 * Copyright (c) 2020 Tobias Svehagen
 * Copyright (c) 2023, Meta
 * Copyright (c) 2024, Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/posix/fcntl.h>
#include <zephyr/zvfs/eventfd.h>
#include <zephyr/sys/bitarray.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/sys/math_extras.h>

#define ZVFS_EFD_IN_USE    0x1
#define ZVFS_EFD_FLAGS_SET (ZVFS_EFD_SEMAPHORE | ZVFS_EFD_NONBLOCK)

struct zvfs_eventfd {
	struct k_poll_signal read_sig;
	struct k_poll_signal write_sig;
	struct k_spinlock lock;
	zvfs_eventfd_t cnt;
	int flags;
};

static ssize_t zvfs_eventfd_rw_op(void *obj, void *buf, size_t sz,
			     int (*op)(struct zvfs_eventfd *efd, zvfs_eventfd_t *value));

SYS_BITARRAY_DEFINE_STATIC(efds_bitarray, CONFIG_ZVFS_EVENTFD_MAX);
static struct zvfs_eventfd efds[CONFIG_ZVFS_EVENTFD_MAX];
static const struct fd_op_vtable zvfs_eventfd_fd_vtable;

static inline bool zvfs_eventfd_is_in_use(struct zvfs_eventfd *efd)
{
	return (efd->flags & ZVFS_EFD_IN_USE) != 0;
}

static inline bool zvfs_eventfd_is_semaphore(struct zvfs_eventfd *efd)
{
	return (efd->flags & ZVFS_EFD_SEMAPHORE) != 0;
}

static inline bool zvfs_eventfd_is_blocking(struct zvfs_eventfd *efd)
{
	return (efd->flags & ZVFS_EFD_NONBLOCK) == 0;
}

static int zvfs_eventfd_poll_prepare(struct zvfs_eventfd *efd,
				struct zsock_pollfd *pfd,
				struct k_poll_event **pev,
				struct k_poll_event *pev_end)
{
	if (pfd->events & ZSOCK_POLLIN) {
		if (*pev == pev_end) {
			errno = ENOMEM;
			return -1;
		}

		(*pev)->obj = &efd->read_sig;
		(*pev)->type = K_POLL_TYPE_SIGNAL;
		(*pev)->mode = K_POLL_MODE_NOTIFY_ONLY;
		(*pev)->state = K_POLL_STATE_NOT_READY;
		(*pev)++;
	}

	if (pfd->events & ZSOCK_POLLOUT) {
		if (*pev == pev_end) {
			errno = ENOMEM;
			return -1;
		}

		(*pev)->obj = &efd->write_sig;
		(*pev)->type = K_POLL_TYPE_SIGNAL;
		(*pev)->mode = K_POLL_MODE_NOTIFY_ONLY;
		(*pev)->state = K_POLL_STATE_NOT_READY;
		(*pev)++;
	}

	return 0;
}

static int zvfs_eventfd_poll_update(struct zvfs_eventfd *efd,
			       struct zsock_pollfd *pfd,
			       struct k_poll_event **pev)
{
	if (pfd->events & ZSOCK_POLLIN) {
		pfd->revents |= ZSOCK_POLLIN * (efd->cnt > 0);
		(*pev)++;
	}

	if (pfd->events & ZSOCK_POLLOUT) {
		pfd->revents |= ZSOCK_POLLOUT * (efd->cnt < UINT64_MAX - 1);
		(*pev)++;
	}

	return 0;
}

static int zvfs_eventfd_read_locked(struct zvfs_eventfd *efd, zvfs_eventfd_t *value)
{
	if (!zvfs_eventfd_is_in_use(efd)) {
		/* file descriptor has been closed */
		return -EBADF;
	}

	if (efd->cnt == 0) {
		/* would block / try again */
		return -EAGAIN;
	}

	/* successful read */
	if (zvfs_eventfd_is_semaphore(efd)) {
		*value = 1;
		--efd->cnt;
	} else {
		*value = efd->cnt;
		efd->cnt = 0;
	}

	if (efd->cnt == 0) {
		k_poll_signal_reset(&efd->read_sig);
	}

	k_poll_signal_raise(&efd->write_sig, 0);

	return 0;
}

static int zvfs_eventfd_write_locked(struct zvfs_eventfd *efd, zvfs_eventfd_t *value)
{
	zvfs_eventfd_t result;

	if (!zvfs_eventfd_is_in_use(efd)) {
		/* file descriptor has been closed */
		return -EBADF;
	}

	if (*value == UINT64_MAX) {
		/* not a permitted value */
		return -EINVAL;
	}

	if (u64_add_overflow(efd->cnt, *value, &result) || result == UINT64_MAX) {
		/* would block / try again */
		return -EAGAIN;
	}

	/* successful write */
	efd->cnt = result;

	if (efd->cnt == (UINT64_MAX - 1)) {
		k_poll_signal_reset(&efd->write_sig);
	}

	k_poll_signal_raise(&efd->read_sig, 0);

	return 0;
}

static ssize_t zvfs_eventfd_read_op(void *obj, void *buf, size_t sz)
{
	return zvfs_eventfd_rw_op(obj, buf, sz, zvfs_eventfd_read_locked);
}

static ssize_t zvfs_eventfd_write_op(void *obj, const void *buf, size_t sz)
{
	return zvfs_eventfd_rw_op(obj, (zvfs_eventfd_t *)buf, sz, zvfs_eventfd_write_locked);
}

static int zvfs_eventfd_close_op(void *obj)
{
	int ret;
	int err;
	k_spinlock_key_t key;
	struct k_mutex *lock = NULL;
	struct k_condvar *cond = NULL;
	struct zvfs_eventfd *efd = (struct zvfs_eventfd *)obj;

	if (k_is_in_isr()) {
		/* not covered by the man page, but necessary in Zephyr */
		errno = EWOULDBLOCK;
		return -1;
	}

	err = (int)zvfs_get_obj_lock_and_cond(obj, &zvfs_eventfd_fd_vtable, &lock, &cond);
	__ASSERT((bool)err, "zvfs_get_obj_lock_and_cond() failed");
	__ASSERT_NO_MSG(lock != NULL);
	__ASSERT_NO_MSG(cond != NULL);

	err = k_mutex_lock(lock, K_FOREVER);
	__ASSERT(err == 0, "k_mutex_lock() failed: %d", err);

	key = k_spin_lock(&efd->lock);

	if (!zvfs_eventfd_is_in_use(efd)) {
		errno = EBADF;
		ret = -1;
		goto unlock;
	}

	err = sys_bitarray_free(&efds_bitarray, 1, (struct zvfs_eventfd *)obj - efds);
	__ASSERT(err == 0, "sys_bitarray_free() failed: %d", err);

	efd->flags = 0;
	efd->cnt = 0;

	ret = 0;

unlock:
	k_spin_unlock(&efd->lock, key);
	/* when closing an zvfs_eventfd, broadcast to all waiters */
	err = k_condvar_broadcast(cond);
	__ASSERT(err == 0, "k_condvar_broadcast() failed: %d", err);
	err = k_mutex_unlock(lock);
	__ASSERT(err == 0, "k_mutex_unlock() failed: %d", err);

	return ret;
}

static int zvfs_eventfd_ioctl_op(void *obj, unsigned int request, va_list args)
{
	int ret;
	k_spinlock_key_t key;
	struct zvfs_eventfd *efd = (struct zvfs_eventfd *)obj;

	/* note: zsock_poll_internal() has already taken the mutex */
	key = k_spin_lock(&efd->lock);

	if (!zvfs_eventfd_is_in_use(efd)) {
		errno = EBADF;
		ret = -1;
		goto unlock;
	}

	switch (request) {
	case F_GETFL:
		ret = efd->flags & ZVFS_EFD_FLAGS_SET;
		break;

	case F_SETFL: {
		int flags;

		flags = va_arg(args, int);

		if (flags & ~ZVFS_EFD_FLAGS_SET) {
			errno = EINVAL;
			ret = -1;
		} else {
			int prev_flags = efd->flags & ~ZVFS_EFD_FLAGS_SET;

			efd->flags = flags | prev_flags;
			ret = 0;
		}
	} break;

	case ZFD_IOCTL_POLL_PREPARE: {
		struct zsock_pollfd *pfd;
		struct k_poll_event **pev;
		struct k_poll_event *pev_end;

		pfd = va_arg(args, struct zsock_pollfd *);
		pev = va_arg(args, struct k_poll_event **);
		pev_end = va_arg(args, struct k_poll_event *);

		ret = zvfs_eventfd_poll_prepare(obj, pfd, pev, pev_end);
	} break;

	case ZFD_IOCTL_POLL_UPDATE: {
		struct zsock_pollfd *pfd;
		struct k_poll_event **pev;

		pfd = va_arg(args, struct zsock_pollfd *);
		pev = va_arg(args, struct k_poll_event **);

		ret = zvfs_eventfd_poll_update(obj, pfd, pev);
	} break;

	default:
		errno = EOPNOTSUPP;
		ret = -1;
		break;
	}

unlock:
	k_spin_unlock(&efd->lock, key);

	return ret;
}

static const struct fd_op_vtable zvfs_eventfd_fd_vtable = {
	.read = zvfs_eventfd_read_op,
	.write = zvfs_eventfd_write_op,
	.close = zvfs_eventfd_close_op,
	.ioctl = zvfs_eventfd_ioctl_op,
};

/* common to both zvfs_eventfd_read_op() and zvfs_eventfd_write_op() */
static ssize_t zvfs_eventfd_rw_op(void *obj, void *buf, size_t sz,
			     int (*op)(struct zvfs_eventfd *efd, zvfs_eventfd_t *value))
{
	int err;
	ssize_t ret;
	k_spinlock_key_t key;
	struct zvfs_eventfd *efd = obj;
	struct k_mutex *lock = NULL;
	struct k_condvar *cond = NULL;

	if (sz < sizeof(zvfs_eventfd_t)) {
		errno = EINVAL;
		return -1;
	}

	if (buf == NULL) {
		errno = EFAULT;
		return -1;
	}

	key = k_spin_lock(&efd->lock);

	if (!zvfs_eventfd_is_blocking(efd)) {
		/*
		 * Handle the non-blocking case entirely within this scope
		 */
		ret = op(efd, buf);
		if (ret < 0) {
			errno = -ret;
			ret = -1;
		} else {
			ret = sizeof(zvfs_eventfd_t);
		}

		goto unlock_spin;
	}

	/*
	 * Handle the blocking case below
	 */
	__ASSERT_NO_MSG(zvfs_eventfd_is_blocking(efd));

	if (k_is_in_isr()) {
		/* not covered by the man page, but necessary in Zephyr */
		errno = EWOULDBLOCK;
		ret = -1;
		goto unlock_spin;
	}

	err = (int)zvfs_get_obj_lock_and_cond(obj, &zvfs_eventfd_fd_vtable, &lock, &cond);
	__ASSERT((bool)err, "zvfs_get_obj_lock_and_cond() failed");
	__ASSERT_NO_MSG(lock != NULL);
	__ASSERT_NO_MSG(cond != NULL);

	/* do not hold a spinlock when taking a mutex */
	k_spin_unlock(&efd->lock, key);
	err = k_mutex_lock(lock, K_FOREVER);
	__ASSERT(err == 0, "k_mutex_lock() failed: %d", err);

	while (true) {
		/* retake the spinlock */
		key = k_spin_lock(&efd->lock);

		ret = op(efd, buf);
		switch (ret) {
		case -EAGAIN:
			/* not an error in blocking mode. break and try again */
			break;
		case 0:
			/* success! */
			ret = sizeof(zvfs_eventfd_t);
			goto unlock_mutex;
		default:
			/* some other error */
			__ASSERT_NO_MSG(ret < 0);
			errno = -ret;
			ret = -1;
			goto unlock_mutex;
		}

		/* do not hold a spinlock when taking a mutex */
		k_spin_unlock(&efd->lock, key);

		/* wait for a write or close */
		err = k_condvar_wait(cond, lock, K_FOREVER);
		__ASSERT(err == 0, "k_condvar_wait() failed: %d", err);
	}

unlock_mutex:
	k_spin_unlock(&efd->lock, key);
	/* only wake a single waiter */
	err = k_condvar_signal(cond);
	__ASSERT(err == 0, "k_condvar_signal() failed: %d", err);
	err = k_mutex_unlock(lock);
	__ASSERT(err == 0, "k_mutex_unlock() failed: %d", err);
	goto out;

unlock_spin:
	k_spin_unlock(&efd->lock, key);

out:
	return ret;
}

/*
 * Public-facing API
 */

int zvfs_eventfd(unsigned int initval, int flags)
{
	int fd = 1;
	size_t offset;
	struct zvfs_eventfd *efd = NULL;

	if (flags & ~ZVFS_EFD_FLAGS_SET) {
		errno = EINVAL;
		return -1;
	}

	if (sys_bitarray_alloc(&efds_bitarray, 1, &offset) < 0) {
		errno = ENOMEM;
		return -1;
	}

	efd = &efds[offset];

	fd = zvfs_reserve_fd();
	if (fd < 0) {
		sys_bitarray_free(&efds_bitarray, 1, offset);
		return -1;
	}

	efd->flags = ZVFS_EFD_IN_USE | flags;
	efd->cnt = initval;

	k_poll_signal_init(&efd->write_sig);
	k_poll_signal_init(&efd->read_sig);

	if (initval != 0) {
		k_poll_signal_raise(&efd->read_sig, 0);
	}

	k_poll_signal_raise(&efd->write_sig, 0);

	zvfs_finalize_fd(fd, efd, &zvfs_eventfd_fd_vtable);

	return fd;
}

int zvfs_eventfd_read(int fd, zvfs_eventfd_t *value)
{
	int ret;
	void *obj;

	obj = zvfs_get_fd_obj(fd, &zvfs_eventfd_fd_vtable, EBADF);
	if (obj == NULL) {
		return -1;
	}

	ret = zvfs_eventfd_rw_op(obj, value, sizeof(zvfs_eventfd_t), zvfs_eventfd_read_locked);
	__ASSERT_NO_MSG(ret == -1 || ret == sizeof(zvfs_eventfd_t));
	if (ret < 0) {
		return -1;
	}

	return 0;
}

int zvfs_eventfd_write(int fd, zvfs_eventfd_t value)
{
	int ret;
	void *obj;

	obj = zvfs_get_fd_obj(fd, &zvfs_eventfd_fd_vtable, EBADF);
	if (obj == NULL) {
		return -1;
	}

	ret = zvfs_eventfd_rw_op(obj, &value, sizeof(zvfs_eventfd_t), zvfs_eventfd_write_locked);
	__ASSERT_NO_MSG(ret == -1 || ret == sizeof(zvfs_eventfd_t));
	if (ret < 0) {
		return -1;
	}

	return 0;
}
