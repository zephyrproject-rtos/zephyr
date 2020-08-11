/*
 * Copyright (c) 2020 Tobias Svehagen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <posix/sys/eventfd.h>

#include <net/socket.h>

struct eventfd {
	struct k_sem read_sem;
	struct k_sem write_sem;
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
	ARG_UNUSED(efd);

	if (pfd->events & ZSOCK_POLLIN) {
		if (*pev == pev_end) {
			errno = ENOMEM;
			return -1;
		}

		(*pev)->obj = &efd->read_sem;
		(*pev)->type = K_POLL_TYPE_SEM_AVAILABLE;
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

	if (pfd->events & ZSOCK_POLLOUT) {
		pfd->revents |= ZSOCK_POLLOUT;
	}

	if (pfd->events & ZSOCK_POLLIN) {
		if ((*pev)->state != K_POLL_STATE_NOT_READY) {
			pfd->revents |= ZSOCK_POLLIN;
		}
		(*pev)++;
	}

	return 0;
}

static ssize_t eventfd_read_op(void *obj, void *buf, size_t sz)
{
	struct eventfd *efd = obj;
	eventfd_t count;
	int ret;

	if (sz < sizeof(eventfd_t)) {
		errno = EINVAL;
		return -1;
	}

	ret = k_sem_take(&efd->read_sem,
			 (efd->flags & EFD_NONBLOCK) ? K_NO_WAIT : K_FOREVER);

	if (ret < 0) {
		errno = EAGAIN;
		return -1;
	}

	count = (efd->flags & EFD_SEMAPHORE) ? 1 : efd->cnt;
	efd->cnt -= count;
	*(eventfd_t *)buf = count;
	k_sem_give(&efd->write_sem);

	return sizeof(eventfd_t);
}

static ssize_t eventfd_write_op(void *obj, const void *buf, size_t sz)
{
	eventfd_t count;
	int ret = 0;

	struct eventfd *efd = obj;

	if (sz < sizeof(eventfd_t)) {
		errno = EINVAL;
		return -1;
	}

	count = *((eventfd_t *)buf);

	if (count == UINT64_MAX) {
		errno = EINVAL;
		return -1;
	}

	if (UINT64_MAX - count <= efd->cnt) {
		if (efd->flags & EFD_NONBLOCK) {
			ret = -EAGAIN;
		} else {
			ret = k_sem_take(&efd->write_sem, K_FOREVER);
		}
	}

	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	efd->cnt += count;
	if (count) {
		k_sem_give(&efd->read_sem);
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
	int i, fd;

	if (flags & ~EFD_FLAGS_SET) {
		errno = EINVAL;
		return -1;
	}

	k_mutex_lock(&eventfd_mtx, K_FOREVER);

	fd = z_reserve_fd();
	if (fd < 0) {
		k_mutex_unlock(&eventfd_mtx);
		return -1;
	}

	for (i = 0; i < ARRAY_SIZE(efds); ++i) {
		if (!(efds[i].flags & EFD_IN_USE)) {
			efd = &efds[i];
			break;
		}
	}

	if (efd == NULL) {
		z_free_fd(fd);
		errno = ENOMEM;
		k_mutex_unlock(&eventfd_mtx);
		return -1;
	}

	efd->flags = EFD_IN_USE | flags;
	efd->cnt = 0;
	k_sem_init(&efd->read_sem, 0, 1);
	k_sem_init(&efd->write_sem, 0, UINT32_MAX);

	z_finalize_fd(fd, efd, &eventfd_fd_vtable);

	k_mutex_unlock(&eventfd_mtx);

	return fd;
}
