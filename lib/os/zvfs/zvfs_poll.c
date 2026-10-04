/*
 * Copyright (c) 2017-2018 Linaro Limited
 * Copyright (c) 2021 Nordic Semiconductor
 * Copyright (c) 2023 Arm Limited (or its affiliates). All rights reserved.
 * Copyright (c) 2024 Tenstorrent AI ULC
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

static bool zvfs_poll_is_held(const uint32_t *held, int fd)
{
	return (fd < ZVFS_OPEN_SIZE) && ((held[fd / 32] & BIT(fd % 32)) != 0);
}

static void zvfs_poll_put_all(const uint32_t *held)
{
	for (int fd = 0; fd < ZVFS_OPEN_SIZE; fd++) {
		if (zvfs_poll_is_held(held, fd)) {
			zvfs_fd_put(fd);
		}
	}
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
	/* Index into fds of the entry that prepared each event */
	int pev_owner[CONFIG_ZVFS_POLL_MAX];
	int pev_count;
	/* The fds held until we return, so that close() cannot free their slots */
	uint32_t held[DIV_ROUND_UP(ZVFS_OPEN_SIZE, 32)] = {0};
	const struct fd_op_vtable *vtable;
	k_timepoint_t end;
	bool offload = false;
	const struct fd_op_vtable *offl_vtable = NULL;
	void *offl_ctx = NULL;

	end = sys_timepoint_calc(timeout);

	pev = poll_events;
	for (pfd = fds, i = nfds; i--; pfd++) {
		struct k_poll_event *pev_start = pev;
		void *ctx;
		int result;

		/* Per POSIX, negative fd's are just ignored */
		if (pfd->fd < 0) {
			continue;
		}

		if (!zvfs_poll_is_held(held, pfd->fd)) {
			if (zvfs_fd_get(pfd->fd) < 0) {
				/* Will set POLLNVAL in return loop */
				continue;
			}
			held[pfd->fd / 32] |= BIT(pfd->fd % 32);
		}

		ctx = zvfs_fd_get_locked(pfd->fd, &vtable);
		if (ctx == NULL) {
			/* Will set POLLNVAL in return loop */
			continue;
		}

		result = zvfs_fdtable_call_ioctl(vtable, ctx, ZFD_IOCTL_POLL_PREPARE, pfd, &pev,
						 pev_end);
		if (result == -EALREADY) {
			/* If POLL_PREPARE returned with EALREADY, it means
			 * it already detected that some socket is ready. In
			 * this case, we still perform a k_poll to pick up
			 * as many events as possible, but without any wait.
			 */
			timeout = K_NO_WAIT;
			end = sys_timepoint_calc(timeout);
			result = 0;
		} else if (result == -EXDEV) {
			/* If POLL_PREPARE returned EXDEV, it means
			 * it detected an offloaded socket.
			 * If offloaded socket is used with native TLS, the TLS
			 * wrapper for the offloaded poll will be used.
			 * In case the fds array contains a mixup of offloaded
			 * and non-offloaded sockets, the offloaded poll handler
			 * shall return an error.
			 */
			offload = true;
			if (offl_vtable == NULL || net_socket_is_tls(ctx)) {
				offl_vtable = vtable;
				offl_ctx = ctx;
			}

			result = 0;
		}

		zvfs_fd_unlock_put(pfd->fd);

		if (result < 0) {
			errno = -result;
			ret = -1;
			goto out;
		}

		for (; pev_start < pev; pev_start++) {
			pev_owner[pev_start - poll_events] = pfd - fds;
		}
	}

	pev_count = pev - poll_events;

	if (offload) {
		int poll_timeout;

		if (K_TIMEOUT_EQ(timeout, K_FOREVER)) {
			poll_timeout = SYS_FOREVER_MS;
		} else {
			poll_timeout = k_ticks_to_ms_floor32(timeout.ticks);
		}

		ret = zvfs_fdtable_call_ioctl(offl_vtable, offl_ctx, ZFD_IOCTL_POLL_OFFLOAD, fds,
					      nfds, poll_timeout);
		goto out;
	}

	timeout = sys_timepoint_timeout(end);

	do {
		ret = k_poll(poll_events, pev_count, timeout);
		/* EAGAIN when timeout expired, EINTR when cancelled (i.e. EOF) */
		if (ret != 0 && ret != -EAGAIN && ret != -EINTR) {
			errno = -ret;
			ret = -1;
			goto out;
		}

		retry = false;
		ret = 0;

		pev = poll_events;
		for (pfd = fds, i = nfds; i--; pfd++) {
			void *ctx = NULL;
			int result;

			pfd->revents = 0;

			if (pfd->fd < 0) {
				continue;
			}

			if (zvfs_poll_is_held(held, pfd->fd)) {
				ctx = zvfs_fd_get_locked(pfd->fd, &vtable);
			}

			if (ctx == NULL) {
				/* Skip the events it prepared before close() */
				while ((pev < poll_events + pev_count) &&
				       (pev_owner[pev - poll_events] == pfd - fds)) {
					pev++;
				}

				pfd->revents = ZVFS_POLLNVAL;
				ret++;
				continue;
			}

			result = zvfs_fdtable_call_ioctl(vtable, ctx, ZFD_IOCTL_POLL_UPDATE, pfd,
							 &pev);
			zvfs_fd_unlock_put(pfd->fd);

			if (result == -EAGAIN) {
				retry = true;
				continue;
			} else if (result != 0) {
				errno = -result;
				ret = -1;
				goto out;
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

out:
	zvfs_poll_put_all(held);

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

	ret = z_impl_zvfs_poll(fds_copy, nfds, timeout);

	if (ret >= 0) {
		k_usermode_to_copy((void *)fds, fds_copy, fds_size);
	}
	k_free(fds_copy);

	return ret;
}
#include <zephyr/syscalls/zvfs_poll_mrsh.c>
#endif
