/*
 * Copyright (c) 2020 Tobias Svehagen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <wait_q.h>
#include <posix/sys/eventfd.h>
#include <net/socket.h>
#include <ksched.h>

struct eventfd {
	struct k_poll_signal read_sig;
	struct k_poll_signal write_sig;
	struct k_spinlock lock;
	_wait_q_t wait_q;
	eventfd_t cnt;
	int flags;
};

K_MUTEX_DEFINE(eventfd_mtx);
static struct eventfd efds[CONFIG_EVENTFD_MAX];

static int eventfd_poll_prepare(struct eventfd *efd,
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

static int eventfd_poll_update(struct eventfd *efd,
			       struct zsock_pollfd *pfd,
			       struct k_poll_event **pev)
{
	ARG_UNUSED(efd);

	if (pfd->events & ZSOCK_POLLIN) {
		if ((*pev)->state != K_POLL_STATE_NOT_READY) {
			pfd->revents |= ZSOCK_POLLIN;
		}
		(*pev)++;
	}

	if (pfd->events & ZSOCK_POLLOUT) {
		if ((*pev)->state != K_POLL_STATE_NOT_READY) {
			pfd->revents |= ZSOCK_POLLOUT;
		}
		(*pev)++;
	}

	return 0;
}

static ssize_t eventfd_read_op(void *obj, void *buf, size_t sz)
{
	struct eventfd *efd = obj;
	int result = 0;
	eventfd_t count = 0;
	k_spinlock_key_t key;

	if (sz < sizeof(eventfd_t)) {
		errno = EINVAL;
		return -1;
	}

	for (;;) {
		key = k_spin_lock(&efd->lock);
		if ((efd->flags & EFD_NONBLOCK) && efd->cnt == 0) {
			result = EAGAIN;
			break;
		} else if (efd->cnt == 0) {
			z_pend_curr(&efd->lock, key, &efd->wait_q, K_FOREVER);
		} else {
			count = (efd->flags & EFD_SEMAPHORE) ? 1 : efd->cnt;
			efd->cnt -= count;
			if (efd->cnt == 0) {
				k_poll_signal_reset(&efd->read_sig);
			}
			k_poll_signal_raise(&efd->write_sig, 0);
			break;
		}
	}
	if (z_unpend_all(&efd->wait_q) != 0) {
		z_reschedule(&efd->lock, key);
	} else {
		k_spin_unlock(&efd->lock, key);
	}

	if (result != 0) {
		errno = result;
		return -1;
	}

	*(eventfd_t *)buf = count;

	return sizeof(eventfd_t);
}

static ssize_t eventfd_write_op(void *obj, const void *buf, size_t sz)
{
	struct eventfd *efd = obj;
	int result = 0;
	eventfd_t count;
	bool overflow;
	k_spinlock_key_t key;

	if (sz < sizeof(eventfd_t)) {
		errno = EINVAL;
		return -1;
	}

	count = *((eventfd_t *)buf);

	if (count == UINT64_MAX) {
		errno = EINVAL;
		return -1;
	}

	if (count == 0) {
		return sizeof(eventfd_t);
	}

	for (;;) {
		key = k_spin_lock(&efd->lock);
		overflow = UINT64_MAX - count <= efd->cnt;
		if ((efd->flags & EFD_NONBLOCK) && overflow) {
			result = EAGAIN;
			break;
		} else if (overflow) {
			z_pend_curr(&efd->lock, key, &efd->wait_q, K_FOREVER);
		} else {
			efd->cnt += count;
			if (efd->cnt == (UINT64_MAX - 1)) {
				k_poll_signal_reset(&efd->write_sig);
			}
			k_poll_signal_raise(&efd->read_sig, 0);
			break;
		}
	}
	if (z_unpend_all(&efd->wait_q) != 0) {
		z_reschedule(&efd->lock, key);
	} else {
		k_spin_unlock(&efd->lock, key);
	}

	if (result != 0) {
		errno = result;
		return -1;
	}

	return sizeof(eventfd_t);
}

static int eventfd_close_op(void *obj)
{
	struct eventfd *efd = (struct eventfd *)obj;

	efd->flags = 0;

	return 0;
}

static int eventfd_ioctl_op(void *obj, unsigned int request, va_list args)
{
	struct eventfd *efd = (struct eventfd *)obj;

	switch (request) {
	case F_GETFL:
		return efd->flags & EFD_FLAGS_SET;

	case F_SETFL: {
		int flags;

		flags = va_arg(args, int);

		if (flags & ~EFD_FLAGS_SET) {
			errno = EINVAL;
			return -1;
		}

		efd->flags = flags;

		return 0;
	}

	case ZFD_IOCTL_POLL_PREPARE: {
		struct zsock_pollfd *pfd;
		struct k_poll_event **pev;
		struct k_poll_event *pev_end;

		pfd = va_arg(args, struct zsock_pollfd *);
		pev = va_arg(args, struct k_poll_event **);
		pev_end = va_arg(args, struct k_poll_event *);

		return eventfd_poll_prepare(obj, pfd, pev, pev_end);
	}

	case ZFD_IOCTL_POLL_UPDATE: {
		struct zsock_pollfd *pfd;
		struct k_poll_event **pev;

		pfd = va_arg(args, struct zsock_pollfd *);
		pev = va_arg(args, struct k_poll_event **);

		return eventfd_poll_update(obj, pfd, pev);
	}

	default:
		errno = EOPNOTSUPP;
		return -1;
	}
}

static const struct fd_op_vtable eventfd_fd_vtable = {
	.read = eventfd_read_op,
	.write = eventfd_write_op,
	.close = eventfd_close_op,
	.ioctl = eventfd_ioctl_op,
};

int eventfd(unsigned int initval, int flags)
{
	struct eventfd *efd = NULL;
	int fd = -1;
	int i;

	if (flags & ~EFD_FLAGS_SET) {
		errno = EINVAL;
		return -1;
	}

	k_mutex_lock(&eventfd_mtx, K_FOREVER);

	for (i = 0; i < ARRAY_SIZE(efds); ++i) {
		if (!(efds[i].flags & EFD_IN_USE)) {
			efd = &efds[i];
			break;
		}
	}

	if (efd == NULL) {
		errno = ENOMEM;
		goto exit_mtx;
	}

	fd = z_reserve_fd();
	if (fd < 0) {
		goto exit_mtx;
	}

	efd->flags = EFD_IN_USE | flags;
	efd->cnt = initval;

	k_poll_signal_init(&efd->write_sig);
	k_poll_signal_init(&efd->read_sig);
	z_waitq_init(&efd->wait_q);

	if (initval != 0) {
		k_poll_signal_raise(&efd->read_sig, 0);
	}
	k_poll_signal_raise(&efd->write_sig, 0);

	z_finalize_fd(fd, efd, &eventfd_fd_vtable);

exit_mtx:
	k_mutex_unlock(&eventfd_mtx);
	return fd;
}
