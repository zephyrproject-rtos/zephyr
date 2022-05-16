/** @file
 * @brief Modem socket / packet size handler
 *
 * Generic modem socket and packet size implementation for modem context
 */

/*
 * Copyright (c) 2019-2020 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/fdtable.h>

#include "modem_socket.h"

/*
 * Packet Size Support Functions
 */

uint16_t modem_socket_next_packet_size(struct modem_socket_config *cfg, struct modem_socket *sock)
{
	uint16_t total = 0U;

	k_sem_take(&cfg->sem_lock, K_FOREVER);

	if (!sock || !sock->packet_count) {
		goto exit;
	}

	total = sock->packet_sizes[0];

exit:
	k_sem_give(&cfg->sem_lock);
	return total;
}

static uint16_t modem_socket_packet_get_total(struct modem_socket *sock)
{
	int i;
	uint16_t total = 0U;

	if (!sock || !sock->packet_count) {
		return 0U;
	}

	for (i = 0; i < sock->packet_count; i++) {
		total += sock->packet_sizes[i];
	}

	return total;
}

static int modem_socket_packet_drop_first(struct modem_socket *sock)
{
	int i;

	if (!sock || !sock->packet_count) {
		return -EINVAL;
	}

	sock->packet_count--;
	for (i = 0; i < sock->packet_count; i++) {
		sock->packet_sizes[i] = sock->packet_sizes[i + 1];
	}

	sock->packet_sizes[sock->packet_count] = 0U;
	return 0;
}

int modem_socket_packet_size_update(struct modem_socket_config *cfg, struct modem_socket *sock,
				    int new_total)
{
	uint16_t old_total = 0U;

	if (!sock) {
		return -EINVAL;
	}

	k_sem_take(&cfg->sem_lock, K_FOREVER);

	if (new_total < 0) {
		new_total += modem_socket_packet_get_total(sock);
	}

	if (new_total <= 0) {
		/* reset outstanding value here */
		sock->packet_count = 0U;
		sock->packet_sizes[0] = 0U;
		k_poll_signal_reset(&sock->sig_data_ready);
		k_sem_give(&cfg->sem_lock);
		return 0;
	}

	old_total = modem_socket_packet_get_total(sock);
	if (new_total == old_total) {
		goto data_ready;
	}

	/* remove sent packets */
	if (new_total < old_total) {
		/* remove packets that are not included in new_size */
		while (old_total > new_total && sock->packet_count > 0) {
			/* handle partial read */
			if (old_total - new_total < sock->packet_sizes[0]) {
				sock->packet_sizes[0] -= old_total - new_total;
				break;
			}

			old_total -= sock->packet_sizes[0];
			modem_socket_packet_drop_first(sock);
		}

		goto data_ready;
	}

	/* new packet to add */
	if (sock->packet_count >= CONFIG_MODEM_SOCKET_PACKET_COUNT) {
		k_sem_give(&cfg->sem_lock);
		return -ENOMEM;
	}

	if (new_total - old_total > 0) {
		sock->packet_sizes[sock->packet_count] = new_total - old_total;
		sock->packet_count++;
	} else {
		k_sem_give(&cfg->sem_lock);
		return -EINVAL;
	}

data_ready:
	if (sock->packet_sizes[0]) {
		k_poll_signal_raise(&sock->sig_data_ready, 0);
	} else {
		k_poll_signal_reset(&sock->sig_data_ready);
	}
	k_sem_give(&cfg->sem_lock);
	return new_total;
}

/*
 * Socket Support Functions
 */

int modem_socket_get(struct modem_socket_config *cfg, int family, int type, int proto)
{
	int i;

	k_sem_take(&cfg->sem_lock, K_FOREVER);

	for (i = 0; i < cfg->sockets_len; i++) {
		if (cfg->sockets[i].id < cfg->base_socket_num) {
			break;
		}
	}

	if (i >= cfg->sockets_len) {
		k_sem_give(&cfg->sem_lock);
		return -ENOMEM;
	}

	/* FIXME: 4 fds max now due to POSIX_OS conflict */
	cfg->sockets[i].sock_fd = z_reserve_fd();
	if (cfg->sockets[i].sock_fd < 0) {
		k_sem_give(&cfg->sem_lock);
		return -errno;
	}

	cfg->sockets[i].family = family;
	cfg->sockets[i].type = type;
	cfg->sockets[i].ip_proto = proto;
	/* socket # needs assigning */
	cfg->sockets[i].id = cfg->sockets_len + 1;
	z_finalize_fd(cfg->sockets[i].sock_fd, &cfg->sockets[i],
		      (const struct fd_op_vtable *)cfg->vtable);

	k_sem_give(&cfg->sem_lock);
	return cfg->sockets[i].sock_fd;
}

struct modem_socket *modem_socket_from_fd(struct modem_socket_config *cfg, int sock_fd)
{
	int i;

	k_sem_take(&cfg->sem_lock, K_FOREVER);

	for (i = 0; i < cfg->sockets_len; i++) {
		if (cfg->sockets[i].sock_fd == sock_fd) {
			k_sem_give(&cfg->sem_lock);
			return &cfg->sockets[i];
		}
	}

	k_sem_give(&cfg->sem_lock);

	return NULL;
}

struct modem_socket *modem_socket_from_id(struct modem_socket_config *cfg, int id)
{
	int i;

	if (id < cfg->base_socket_num) {
		return NULL;
	}

	k_sem_take(&cfg->sem_lock, K_FOREVER);

	for (i = 0; i < cfg->sockets_len; i++) {
		if (cfg->sockets[i].id == id) {
			k_sem_give(&cfg->sem_lock);
			return &cfg->sockets[i];
		}
	}

	k_sem_give(&cfg->sem_lock);

	return NULL;
}

struct modem_socket *modem_socket_from_newid(struct modem_socket_config *cfg)
{
	return modem_socket_from_id(cfg, cfg->sockets_len + 1);
}

void modem_socket_put(struct modem_socket_config *cfg, int sock_fd)
{
	struct modem_socket *sock = modem_socket_from_fd(cfg, sock_fd);

	if (!sock) {
		return;
	}

	k_sem_take(&cfg->sem_lock, K_FOREVER);

	sock->id = cfg->base_socket_num - 1;
	sock->sock_fd = -1;
	sock->is_waiting = false;
	sock->is_connected = false;
	(void)memset(&sock->src, 0, sizeof(struct sockaddr));
	(void)memset(&sock->dst, 0, sizeof(struct sockaddr));
	memset(&sock->packet_sizes, 0, sizeof(sock->packet_sizes));
	sock->packet_count = 0;
	k_sem_reset(&sock->sem_data_ready);
	k_poll_signal_reset(&sock->sig_data_ready);

	k_sem_give(&cfg->sem_lock);
}

/*
 * Generic Poll Function
 */

/*
 * FIXME: The design here makes the poll function non-reentrant for same sockets.
 * If two different threads poll on two identical sockets we'll end up with unexpected
 * behavior - the higher priority thread will be unblocked, regardless on which
 * socket it polled. I think we could live with such limitation though in the
 * initial implementation, but this should be improved in the future.
 */
int modem_socket_poll(struct modem_socket_config *cfg, struct zsock_pollfd *fds, int nfds,
		      int msecs)
{
	struct modem_socket *sock;
	int ret, i;
	uint8_t found_count = 0;

	if (!cfg || nfds > CONFIG_NET_SOCKETS_POLL_MAX) {
		return -EINVAL;
	}
	struct k_poll_event events[nfds];
	int eventcount = 0;

	for (i = 0; i < nfds; i++) {
		sock = modem_socket_from_fd(cfg, fds[i].fd);
		if (sock) {
			/*
			 * Handle user check for POLLOUT events:
			 * we consider the socket to always be writable.
			 */
			if (fds[i].events & ZSOCK_POLLOUT) {
				found_count++;
				break;
			} else if (fds[i].events & ZSOCK_POLLIN) {
				k_poll_event_init(&events[eventcount++], K_POLL_TYPE_SIGNAL,
						  K_POLL_MODE_NOTIFY_ONLY, &sock->sig_data_ready);
				if (sock->packet_sizes[0] > 0U) {
					found_count++;
					break;
				}
			}
		}
	}

	/* Avoid waiting on semaphore if we have already found an event */
	ret = 0;
	if (!found_count) {
		k_timeout_t timeout = K_FOREVER;

		if (msecs >= 0) {
			timeout = K_MSEC(msecs);
		}
		ret = k_poll(events, eventcount, timeout);
	}
	/* Reset counter as we reiterate on all polled sockets */
	found_count = 0;

	for (i = 0; i < nfds; i++) {
		sock = modem_socket_from_fd(cfg, fds[i].fd);
		if (!sock) {
			continue;
		}

		/*
		 * Handle user check for ZSOCK_POLLOUT events:
		 * we consider the socket to always be writable.
		 */
		if (fds[i].events & ZSOCK_POLLOUT) {
			fds[i].revents |= ZSOCK_POLLOUT;
			found_count++;
		} else if ((fds[i].events & ZSOCK_POLLIN) && (sock->packet_sizes[0] > 0U)) {
			fds[i].revents |= ZSOCK_POLLIN;
			found_count++;
		}
	}

	/* EBUSY, EAGAIN and ETIMEDOUT aren't true errors */
	if (ret < 0 && ret != -EBUSY && ret != -EAGAIN && ret != -ETIMEDOUT) {
		errno = ret;
		return -1;
	}

	errno = 0;
	return found_count;
}

int modem_socket_poll_prepare(struct modem_socket_config *cfg, struct modem_socket *sock,
			      struct zsock_pollfd *pfd, struct k_poll_event **pev,
			      struct k_poll_event *pev_end)
{
	if (pfd->events & ZSOCK_POLLIN) {
		if (*pev == pev_end) {
			errno = ENOMEM;
			return -1;
		}

		k_poll_event_init(*pev, K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY,
				  &sock->sig_data_ready);
		(*pev)++;
	}

	if (pfd->events & ZSOCK_POLLOUT) {
		if (*pev == pev_end) {
			errno = ENOMEM;
			return -1;
		}
		/* Not Implemented */
		errno = ENOTSUP;
		return -1;
	}

	return 0;
}

int modem_socket_poll_update(struct modem_socket *sock, struct zsock_pollfd *pfd,
			     struct k_poll_event **pev)
{
	ARG_UNUSED(sock);

	if (pfd->events & ZSOCK_POLLIN) {
		if ((*pev)->state != K_POLL_STATE_NOT_READY) {
			pfd->revents |= ZSOCK_POLLIN;
		}
		(*pev)++;
	}

	if (pfd->events & ZSOCK_POLLOUT) {
		/* Not implemented, but the modem socket is always ready to transmit,
		 * so set the revents
		 */
		pfd->revents |= ZSOCK_POLLOUT;
		(*pev)++;
	}

	return 0;
}

void modem_socket_wait_data(struct modem_socket_config *cfg, struct modem_socket *sock)
{
	k_sem_take(&cfg->sem_lock, K_FOREVER);
	sock->is_waiting = true;
	k_sem_give(&cfg->sem_lock);

	k_sem_take(&sock->sem_data_ready, K_FOREVER);
}

void modem_socket_data_ready(struct modem_socket_config *cfg, struct modem_socket *sock)
{
	k_sem_take(&cfg->sem_lock, K_FOREVER);

	if (sock->is_waiting) {
		/* unblock sockets waiting on recv() */
		sock->is_waiting = false;
		k_sem_give(&sock->sem_data_ready);
	}

	k_sem_give(&cfg->sem_lock);
}

int modem_socket_init(struct modem_socket_config *cfg, const struct socket_op_vtable *vtable)
{
	int i;

	k_sem_init(&cfg->sem_lock, 1, 1);
	for (i = 0; i < cfg->sockets_len; i++) {
		k_sem_init(&cfg->sockets[i].sem_data_ready, 0, 1);
		k_poll_signal_init(&cfg->sockets[i].sig_data_ready);
		cfg->sockets[i].id = cfg->base_socket_num - 1;
	}

	cfg->vtable = vtable;

	return 0;
}
