/*
 * Copyright (c) 2025 Atym Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/posix/fcntl.h>
#include <zephyr/zvfs/timerfd.h>
#include <zephyr/sys/bitarray.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/sys/timeutil.h>

#define ZVFS_TFD_IN_USE    0x1
#define ZVFS_TFD_FLAGS_SET (ZVFS_TFD_NONBLOCK)

struct zvfs_timerfd {
	struct k_timer ztimer;
	struct k_poll_signal read_sig;
	struct k_spinlock lock;
	zvfs_timerfd_t cnt;
	k_timeout_t period;
	int flags;
};

static ssize_t zvfs_timerfd_read_op(void *obj, void *buf, size_t sz);

SYS_BITARRAY_DEFINE_STATIC(tfds_bitarray, CONFIG_ZVFS_TIMERFD_MAX);
static struct zvfs_timerfd tfds[CONFIG_ZVFS_TIMERFD_MAX];
static const struct fd_op_vtable zvfs_timerfd_fd_vtable;

static inline bool zvfs_timerfd_is_in_use(struct zvfs_timerfd *tfd)
{
	return (tfd->flags & ZVFS_TFD_IN_USE) != 0;
}

static inline bool zvfs_timerfd_is_blocking(struct zvfs_timerfd *tfd)
{
	return (tfd->flags & ZVFS_TFD_NONBLOCK) == 0;
}

static int zvfs_timerfd_poll_prepare(struct zvfs_timerfd *tfd,
				struct zsock_pollfd *pfd,
				struct k_poll_event **pev,
				struct k_poll_event *pev_end)
{
	if (pfd->events & ZSOCK_POLLIN) {
		if (*pev == pev_end) {
			errno = ENOMEM;
			return -1;
		}

		(*pev)->obj = &tfd->read_sig;
		(*pev)->type = K_POLL_TYPE_SIGNAL;
		(*pev)->mode = K_POLL_MODE_NOTIFY_ONLY;
		(*pev)->state = K_POLL_STATE_NOT_READY;
		(*pev)++;
	}

	return 0;
}

static int zvfs_timerfd_poll_update(struct zvfs_timerfd *tfd,
			       struct zsock_pollfd *pfd,
			       struct k_poll_event **pev)
{
	if (pfd->events & ZSOCK_POLLIN) {
		pfd->revents |= ZSOCK_POLLIN * (tfd->cnt > 0);
		(*pev)++;
	}

	return 0;
}

static int zvfs_timerfd_read_locked(struct zvfs_timerfd *tfd, zvfs_timerfd_t *value)
{
	if (!zvfs_timerfd_is_in_use(tfd)) {
		/* file descriptor has been closed */
		return -EBADF;
	}

	if (tfd->cnt == 0) {
		/* would block / try again */
		return -EAGAIN;
	}

	/* successful read */
	*value = tfd->cnt;
	tfd->cnt = 0;

	k_poll_signal_reset(&tfd->read_sig);

	return 0;
}

static int zvfs_timerfd_close_op(void *obj)
{
	int ret;
	int err;
	k_spinlock_key_t key;
	struct k_mutex *lock = NULL;
	struct k_condvar *cond = NULL;
	struct zvfs_timerfd *tfd = (struct zvfs_timerfd *)obj;

	if (k_is_in_isr()) {
		/* not covered by the man page, but necessary in Zephyr */
		errno = EWOULDBLOCK;
		return -1;
	}

	err = (int)zvfs_get_obj_lock_and_cond(obj, &zvfs_timerfd_fd_vtable, &lock, &cond);
	__ASSERT((bool)err, "zvfs_get_obj_lock_and_cond() failed");
	__ASSERT_NO_MSG(lock != NULL);
	__ASSERT_NO_MSG(cond != NULL);

	key = k_spin_lock(&tfd->lock);

	if (!zvfs_timerfd_is_in_use(tfd)) {
		errno = EBADF;
		ret = -1;
		goto unlock;
	}

	k_timer_stop(&tfd->ztimer);

	err = sys_bitarray_free(&tfds_bitarray, 1, (struct zvfs_timerfd *)obj - tfds);
	__ASSERT(err == 0, "sys_bitarray_free() failed: %d", err);

	tfd->flags = 0;
	tfd->cnt = 0;

	ret = 0;

unlock:
	k_spin_unlock(&tfd->lock, key);
	/* when closing a zvfs_timerfd, broadcast to all waiters */
	k_condvar_broadcast(cond);

	return ret;
}

static int zvfs_timerfd_ioctl_op(void *obj, unsigned int request, va_list args)
{
	int ret;
	k_spinlock_key_t key;
	struct zvfs_timerfd *tfd = (struct zvfs_timerfd *)obj;

	/* note: zsock_poll_internal() has already taken the mutex */
	key = k_spin_lock(&tfd->lock);

	if (!zvfs_timerfd_is_in_use(tfd)) {
		errno = EBADF;
		ret = -1;
		goto unlock;
	}

	switch (request) {
	case ZVFS_TFD_IOC_SET_TICKS: {
		uint64_t new_value = va_arg(args, uint64_t);

		if (new_value == 0) {
			errno = EINVAL;
			ret = -1;
		} else {
			tfd->cnt = new_value;
			ret = 0;
		}
	} break;

	case F_GETFL:
		ret = tfd->flags & ZVFS_TFD_FLAGS_SET;
		break;

	case F_SETFL: {
		int flags;

		flags = va_arg(args, int);

		if (flags & ~ZVFS_TFD_FLAGS_SET) {
			errno = EINVAL;
			ret = -1;
		} else {
			int prev_flags = tfd->flags & ~ZVFS_TFD_FLAGS_SET;

			tfd->flags = flags | prev_flags;
			ret = 0;
		}
	} break;

	case ZFD_IOCTL_STAT: {
    	struct stat *buf = va_arg(args, struct stat *);
    	memset(buf, 0, sizeof(struct stat));
        /* mode is rw even though we don't support write */
    	buf->st_mode = 0600;
    	ret = 0;
	} break;

	case ZFD_IOCTL_POLL_PREPARE: {
		struct zsock_pollfd *pfd;
		struct k_poll_event **pev;
		struct k_poll_event *pev_end;

		pfd = va_arg(args, struct zsock_pollfd *);
		pev = va_arg(args, struct k_poll_event **);
		pev_end = va_arg(args, struct k_poll_event *);

		ret = zvfs_timerfd_poll_prepare(obj, pfd, pev, pev_end);
	} break;

	case ZFD_IOCTL_POLL_UPDATE: {
		struct zsock_pollfd *pfd;
		struct k_poll_event **pev;

		pfd = va_arg(args, struct zsock_pollfd *);
		pev = va_arg(args, struct k_poll_event **);

		ret = zvfs_timerfd_poll_update(obj, pfd, pev);
	} break;

	default:
		errno = EOPNOTSUPP;
		ret = -1;
		break;
	}

unlock:
	k_spin_unlock(&tfd->lock, key);

	return ret;
}

static const struct fd_op_vtable zvfs_timerfd_fd_vtable = {
	.read = zvfs_timerfd_read_op,
	.close = zvfs_timerfd_close_op,
	.ioctl = zvfs_timerfd_ioctl_op,
};

static ssize_t zvfs_timerfd_read_op(void *obj, void *buf, size_t sz)
{
	int err;
	ssize_t ret;
	k_spinlock_key_t key;
	struct zvfs_timerfd *tfd = obj;
	struct k_mutex *lock = NULL;
	struct k_condvar *cond = NULL;

	if (sz < sizeof(zvfs_timerfd_t)) {
		errno = EINVAL;
		return -1;
	}

	if (buf == NULL) {
		errno = EFAULT;
		return -1;
	}

	key = k_spin_lock(&tfd->lock);

	if (!zvfs_timerfd_is_blocking(tfd)) {
		/*
		 * Handle the non-blocking case entirely within this scope
		 */
		ret = zvfs_timerfd_read_locked(tfd, buf);
		if (ret < 0) {
			errno = -ret;
			ret = -1;
		} else {
			ret = sizeof(zvfs_timerfd_t);
		}

		goto unlock_spin;
	}

	/*
	 * Handle the blocking case below
	 */
	__ASSERT_NO_MSG(zvfs_timerfd_is_blocking(tfd));

	if (k_is_in_isr()) {
		/* not covered by the man page, but necessary in Zephyr */
		errno = EWOULDBLOCK;
		ret = -1;
		goto unlock_spin;
	}

	err = (int)zvfs_get_obj_lock_and_cond(obj, &zvfs_timerfd_fd_vtable, &lock, &cond);
	__ASSERT((bool)err, "zvfs_get_obj_lock_and_cond() failed");
	__ASSERT_NO_MSG(lock != NULL);
	__ASSERT_NO_MSG(cond != NULL);

	/* do not hold a spinlock when taking a mutex */
	k_spin_unlock(&tfd->lock, key);

	while (true) {
		/* retake the spinlock */
		key = k_spin_lock(&tfd->lock);

		ret = zvfs_timerfd_read_locked(tfd, buf);
		switch (ret) {
		case -EAGAIN:
			/* not an error in blocking mode. break and try again */
			break;
		case 0:
			/* success! */
			ret = sizeof(zvfs_timerfd_t);
			goto unlock;
		default:
			/* some other error */
			__ASSERT_NO_MSG(ret < 0);
			errno = -ret;
			ret = -1;
			goto unlock;
		}

		/* do not hold a spinlock when taking a mutex */
		k_spin_unlock(&tfd->lock, key);

		/* wait for an expiration or close */
		err = k_condvar_wait(cond, lock, K_FOREVER);
		__ASSERT(err == 0, "k_condvar_wait() failed: %d", err);
	}

unlock:
	k_spin_unlock(&tfd->lock, key);
	/* only wake a single waiter */
	err = k_condvar_signal(cond);
	__ASSERT(err == 0, "k_condvar_signal() failed: %d", err);
	goto out;

unlock_spin:
	k_spin_unlock(&tfd->lock, key);

out:
	return ret;
}

static void zvfs_timerfd_expired(struct k_timer *timer_id) {
   	int err;
    k_spinlock_key_t key;
    struct zvfs_timerfd *tfd = (struct zvfs_timerfd *)timer_id;
	struct k_mutex *lock = NULL;
	struct k_condvar *cond = NULL;

	key = k_spin_lock(&tfd->lock);

	err = (int)zvfs_get_obj_lock_and_cond(tfd, &zvfs_timerfd_fd_vtable, &lock, &cond);
	__ASSERT((bool)err, "zvfs_get_obj_lock_and_cond() failed");
	__ASSERT_NO_MSG(lock != NULL);
	__ASSERT_NO_MSG(cond != NULL);

	tfd->cnt++;
	k_poll_signal_raise(&tfd->read_sig, 0);

	k_spin_unlock(&tfd->lock, key);

	/* only wake a single waiter */
	err = k_condvar_signal(cond);
	__ASSERT(err == 0, "k_condvar_signal() failed: %d", err);
}

/*
 * Public-facing API
 */

int zvfs_timerfd(unsigned int initval, int flags)
{
	int fd = 1;
	size_t offset;
	struct zvfs_timerfd *tfd = NULL;

	if (flags & ~ZVFS_TFD_FLAGS_SET) {
		errno = EINVAL;
		return -1;
	}

	if (sys_bitarray_alloc(&tfds_bitarray, 1, &offset) < 0) {
		errno = ENOMEM;
		return -1;
	}

	tfd = &tfds[offset];

	fd = zvfs_reserve_fd();
	if (fd < 0) {
		sys_bitarray_free(&tfds_bitarray, 1, offset);
		return -1;
	}

	tfd->flags = ZVFS_TFD_IN_USE | flags;
	tfd->cnt = 0;

	k_poll_signal_init(&tfd->read_sig);

	k_timer_init(&tfd->ztimer, zvfs_timerfd_expired, NULL);

	zvfs_finalize_fd(fd, tfd, &zvfs_timerfd_fd_vtable);

	return fd;
}

/**
 * @brief Get amount of time left for expiration on a per-process timer.
 *
 * See IEEE 1003.1
 */

int zvfs_timerfd_read(int fd, zvfs_timerfd_t *value)
{
	int ret;
	void *obj;
	int err;
	struct k_mutex *lock = NULL;
	struct k_condvar *cond = NULL;

	obj = zvfs_get_fd_obj(fd, &zvfs_timerfd_fd_vtable, EBADF);
	if (obj == NULL) {
		return -1;
	}

	err = (int)zvfs_get_obj_lock_and_cond(obj, &zvfs_timerfd_fd_vtable, &lock, &cond);
	__ASSERT((bool)err, "zvfs_get_obj_lock_and_cond() failed");
	__ASSERT_NO_MSG(lock != NULL);
	__ASSERT_NO_MSG(cond != NULL);

	err = k_mutex_lock(lock, K_FOREVER);
	__ASSERT(err == 0, "k_mutex_lock() failed: %d", err);

	ret = zvfs_timerfd_read_op(obj, value, sizeof(zvfs_timerfd_t));
	__ASSERT_NO_MSG(ret == -1 || ret == sizeof(zvfs_timerfd_t));

	err = k_mutex_unlock(lock);
	__ASSERT(err == 0, "k_mutex_unlock() failed: %d", err);

	if (ret < 0) {
		return -1;
	}

	return 0;
}

/**
 * @brief Get amount of time left for expiration on a per-process timer.
 *
 * See IEEE 1003.1
 */
int zvfs_timerfd_gettime(int fd, uint32_t *remaining, uint32_t *period)
{
    int err;
	struct zvfs_timerfd *tfd;

	tfd = zvfs_get_fd_obj(fd, &zvfs_timerfd_fd_vtable, EBADF);
	if (tfd == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (period != NULL) {
		k_spinlock_key_t key;
    	struct k_mutex *lock = NULL;
    	struct k_condvar *cond = NULL;
    	key = k_spin_lock(&tfd->lock);

    	err = (int)zvfs_get_obj_lock_and_cond(tfd, &zvfs_timerfd_fd_vtable, &lock, &cond);
    	__ASSERT((bool)err, "zvfs_get_obj_lock_and_cond() failed");
    	__ASSERT_NO_MSG(lock != NULL);
    	__ASSERT_NO_MSG(cond != NULL);

    	/* do not hold a spinlock when taking a mutex */
    	k_spin_unlock(&tfd->lock, key);
    	err = k_mutex_lock(lock, K_FOREVER);
    	__ASSERT(err == 0, "k_mutex_lock() failed: %d", err);

    	key = k_spin_lock(&tfd->lock);

    	*period = k_ticks_to_ms_floor32(tfd->period.ticks);

    	k_spin_unlock(&tfd->lock, key);
        err = k_mutex_unlock(lock);
        __ASSERT(err == 0, "k_mutex_unlock() failed: %d", err);
	}

	if (remaining != NULL) {
		*remaining = k_timer_remaining_get(&tfd->ztimer);
	}

	return 0;
}

/**
 * @brief Sets expiration time of per-process timer.
 *
 * See IEEE 1003.1
 */
int zvfs_timerfd_settime(int fd, k_timeout_t duration, k_timeout_t period)
{
    int err;
	struct zvfs_timerfd *tfd;
	k_spinlock_key_t key;
   	struct k_mutex *lock = NULL;
   	struct k_condvar *cond = NULL;

	tfd = zvfs_get_fd_obj(fd, &zvfs_timerfd_fd_vtable, EBADF);
	if (tfd == NULL) {
		errno = EINVAL;
		return -1;
	}

	k_timer_stop(&tfd->ztimer);

	if ((duration.ticks != 0) || (period.ticks != 0)) {
		k_timer_start(&tfd->ztimer, duration, period);
	}

   	key = k_spin_lock(&tfd->lock);

   	err = (int)zvfs_get_obj_lock_and_cond(tfd, &zvfs_timerfd_fd_vtable, &lock, &cond);
   	__ASSERT((bool)err, "zvfs_get_obj_lock_and_cond() failed");
   	__ASSERT_NO_MSG(lock != NULL);
   	__ASSERT_NO_MSG(cond != NULL);

   	/* do not hold a spinlock when taking a mutex */
   	k_spin_unlock(&tfd->lock, key);
   	err = k_mutex_lock(lock, K_FOREVER);
   	__ASSERT(err == 0, "k_mutex_lock() failed: %d", err);

   	key = k_spin_lock(&tfd->lock);

	tfd->period = period;

	k_spin_unlock(&tfd->lock, key);
    err = k_mutex_unlock(lock);
    __ASSERT(err == 0, "k_mutex_unlock() failed: %d", err);

	return 0;
}
