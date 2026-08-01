/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/sys/fdtable.h>

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
bool net_socket_is_tls(void *obj);
#else
#define net_socket_is_tls(obj) false
#endif

/*
 * fds whose backends implement no poll ioctls have no waitable condition:
 * they are always ready for both input and output (e.g. regular files, as
 * POSIX requires of poll()).
 */
static bool zvfs_poll_always_ready(uint32_t mode)
{
	switch (mode & ZVFS_MODE_IFMT) {
	case ZVFS_MODE_IFBLK:
	case ZVFS_MODE_IFCHR:
	case ZVFS_MODE_IFDIR:
	case ZVFS_MODE_IFREG:
	case ZVFS_MODE_IFSHM:
	case ZVFS_MODE_IMSGQ:
		return true;
	default:
		return false;
	}
}

int zvfs_poll_prepare_fd(struct zvfs_pollfd *pfd, struct k_poll_event **pev,
			 struct k_poll_event *pev_end)
{
	int result;
	struct fd_entry *entry = zvfs_fd_entry_get(pfd->fd);

	if (entry == NULL) {
		return -EBADF;
	}

	if (zvfs_poll_always_ready(entry->mode)) {
		return ZVFS_POLL_READY;
	}

	(void)k_mutex_lock(&entry->lock, K_FOREVER);
	if (entry->obj == NULL) {
		k_mutex_unlock(&entry->lock);
		return -EBADF;
	}
	result = zvfs_fdtable_call_ioctl(entry->vtable, entry->obj, ZFD_IOCTL_POLL_PREPARE, pfd,
					 pev, pev_end);
	k_mutex_unlock(&entry->lock);

	if (result == -EALREADY) {
		return ZVFS_POLL_READY;
	}

	if (result < 0) {
		return result;
	}

	return ZVFS_POLL_PREPARED;
}

int zvfs_poll_update_fd(struct zvfs_pollfd *pfd, struct k_poll_event **pev)
{
	int result;
	struct fd_entry *entry = zvfs_fd_entry_get(pfd->fd);

	if (entry == NULL) {
		pfd->revents = ZVFS_POLLNVAL;
		return 0;
	}

	if (zvfs_poll_always_ready(entry->mode)) {
		pfd->revents = pfd->events & (ZVFS_POLLIN | ZVFS_POLLRDNORM | ZVFS_POLLOUT |
					      ZVFS_POLLWRNORM);
		return 0;
	}

	(void)k_mutex_lock(&entry->lock, K_FOREVER);
	if (entry->obj == NULL) {
		k_mutex_unlock(&entry->lock);
		pfd->revents = ZVFS_POLLNVAL;
		return 0;
	}
	result = zvfs_fdtable_call_ioctl(entry->vtable, entry->obj, ZFD_IOCTL_POLL_UPDATE, pfd,
					 pev);
	k_mutex_unlock(&entry->lock);

	return result;
}

int zvfs_poll_internal(struct zvfs_pollfd *fds, int nfds, k_timeout_t timeout)
{
	bool retry;
	int ret = 0;
	int i;
	struct zvfs_pollfd *pfd;
	struct k_poll_event poll_events[CONFIG_ZVFS_POLL_MAX];
	struct k_poll_event *pev;
	struct k_poll_event *pev_end = poll_events + ARRAY_SIZE(poll_events);
	const struct fd_op_vtable *vtable;
	struct k_mutex *lock;
	k_timepoint_t end;
	bool offload = false;
	const struct fd_op_vtable *offl_vtable = NULL;
	void *offl_ctx = NULL;

	end = sys_timepoint_calc(timeout);

	pev = poll_events;
	for (pfd = fds, i = nfds; i--; pfd++) {
		int result;

		/* Per POSIX, negative fd's are just ignored */
		if (pfd->fd < 0) {
			continue;
		}

		result = zvfs_poll_prepare_fd(pfd, &pev, pev_end);
		if (result == -EBADF) {
			/* Will set POLLNVAL in return loop */
			continue;
		} else if (result == ZVFS_POLL_READY) {
			/* Some fd is already ready. In this case, we still
			 * perform a k_poll to pick up as many events as
			 * possible, but without any wait.
			 */
			timeout = K_NO_WAIT;
			end = sys_timepoint_calc(timeout);
		} else if (result == -EXDEV) {
			/* An offloaded socket was detected.
			 * If offloaded socket is used with native TLS, the TLS
			 * wrapper for the offloaded poll will be used.
			 * In case the fds array contains a mixup of offloaded
			 * and non-offloaded sockets, the offloaded poll handler
			 * shall return an error.
			 */
			void *ctx = zvfs_get_fd_obj_and_vtable(pfd->fd, &vtable, &lock);

			offload = true;
			if (offl_vtable == NULL || net_socket_is_tls(ctx)) {
				offl_vtable = vtable;
				offl_ctx = ctx;
			}
		} else if (result < 0) {
			errno = -result;
			return -1;
		}
	}

	if (offload) {
		int poll_timeout;

		if (K_TIMEOUT_EQ(timeout, K_FOREVER)) {
			poll_timeout = SYS_FOREVER_MS;
		} else {
			poll_timeout = k_ticks_to_ms_floor32(timeout.ticks);
		}

		return zvfs_fdtable_call_ioctl(offl_vtable, offl_ctx, ZFD_IOCTL_POLL_OFFLOAD, fds,
					       nfds, poll_timeout);
	}

	timeout = sys_timepoint_timeout(end);

	do {
		ret = k_poll(poll_events, pev - poll_events, timeout);
		/* EAGAIN when timeout expired, EINTR when cancelled (i.e. EOF) */
		if (ret != 0 && ret != -EAGAIN && ret != -EINTR) {
			errno = -ret;
			return -1;
		}

		retry = false;
		ret = 0;

		pev = poll_events;
		for (pfd = fds, i = nfds; i--; pfd++) {
			int result;

			pfd->revents = 0;

			if (pfd->fd < 0) {
				continue;
			}

			result = zvfs_poll_update_fd(pfd, &pev);
			if (result == -EAGAIN) {
				retry = true;
				continue;
			} else if (result != 0) {
				errno = -result;
				return -1;
			}

			if (pfd->revents != 0) {
				ret++;
			}
		}

		if (retry) {
			if (ret > 0) {
				break;
			}

			timeout = sys_timepoint_timeout(end);

			if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
				break;
			}
		}
	} while (retry);

	return ret;
}

int z_impl_zvfs_poll(struct zvfs_pollfd *fds, int nfds, int poll_timeout)
{
	k_timeout_t timeout;

	if (poll_timeout < 0) {
		timeout = K_FOREVER;
	} else {
		timeout = K_MSEC(poll_timeout);
	}

	return zvfs_poll_internal(fds, nfds, timeout);
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_zvfs_poll(struct zvfs_pollfd *fds, int nfds, int timeout)
{
	struct zvfs_pollfd *fds_copy;
	size_t fds_size;
	int ret;

	/* Copy fds array from user mode */
	if (size_mul_overflow(nfds, sizeof(struct zvfs_pollfd), &fds_size)) {
		errno = EFAULT;
		return -1;
	}
	fds_copy = k_usermode_alloc_from_copy((void *)fds, fds_size);
	if (!fds_copy) {
		errno = ENOMEM;
		return -1;
	}

	/*
	 * An in-use fd the caller has no K_OBJ_FILE grant for faults here; an
	 * fd that names no open file passes through and reports ZVFS_POLLNVAL.
	 */
	for (int i = 0; i < nfds; i++) {
		if (fds_copy[i].fd >= 0) {
			(void)zvfs_fd_syscall_verify(fds_copy[i].fd);
		}
	}

	ret = z_impl_zvfs_poll(fds_copy, nfds, timeout);

	if (ret >= 0) {
		k_usermode_to_copy((void *)fds, fds_copy, fds_size);
	}
	k_free(fds_copy);

	return ret;
}
#include <zephyr/syscalls/zvfs_poll_mrsh.c>
#endif
