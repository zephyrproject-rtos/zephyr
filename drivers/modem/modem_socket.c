/** @file
 * @brief Modem socket / packet size handler
 *
 * Generic modem socket and packet size implementation for modem context
 */

/*
 * Copyright (c) 2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <net/socket_offload.h>
#include <sys/fdtable.h>

#include "modem_socket.h"

/*
 * Packet Size Support Functions
 */

static u16_t modem_socket_packet_get_total(struct modem_socket *sock)
{
	int i;
	u16_t total = 0U;

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
		sock->packet_sizes[i] =
			sock->packet_sizes[i + 1];
	}

	sock->packet_sizes[sock->packet_count] = 0U;
	return 0;
}

int modem_socket_packet_size_update(struct modem_socket_config *cfg,
				    struct modem_socket *sock, int new_total)
{
	u16_t old_total = 0U;

	if (!sock) {
		return -EINVAL;
	}

	if (new_total < 0) {
		new_total += modem_socket_packet_get_total(sock);
	}

	if (new_total <= 0) {
		/* reset outstanding value here */
		sock->packet_count = 0U;
		sock->packet_sizes[0] = 0U;
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
			old_total -= sock->packet_sizes[0];
			modem_socket_packet_drop_first(sock);
		}

		goto data_ready;
	}

	/* new packet to add */
	if (sock->packet_count >= CONFIG_MODEM_SOCKET_PACKET_COUNT) {
		return -ENOMEM;
	}

	if (new_total - old_total > 0) {
		sock->packet_sizes[sock->packet_count] = new_total - old_total;
		sock->packet_count++;
	} else {
		return -EINVAL;
	}

data_ready:
	return new_total;
}

/*
 * VTable OPS
 */

static ssize_t modem_socket_read_op(void *obj, void *buf, size_t sz)
{
	/* TODO: NOT IMPLEMENTED */
	return -ENOTSUP;
}

static ssize_t modem_socket_write_op(void *obj, const void *buf, size_t sz)
{
	/* TODO: NOT IMPLEMENTED */
	return -ENOTSUP;
}

static int modem_socket_ioctl_op(void *obj, unsigned int request, va_list args)
{
	/* TODO: NOT IMPLEMENTED */
	return -ENOTSUP;
}

static const struct fd_op_vtable modem_sock_fd_vtable = {
	.read = modem_socket_read_op,
	.write = modem_socket_write_op,
	.ioctl = modem_socket_ioctl_op,
};

/*
 * Socket Support Functions
 */

int modem_socket_get(struct modem_socket_config *cfg,
		     int family, int type, int proto)
{
	int i;

	for (i = 0; i < cfg->sockets_len; i++) {
		if (cfg->sockets[i].id < cfg->base_socket_num) {
			break;
		}
	}

	if (i >= cfg->sockets_len) {
		return -ENOMEM;
	}

	/* FIXME: 4 fds max now due to POSIX_OS conflict */
	cfg->sockets[i].sock_fd = z_reserve_fd();
	if (cfg->sockets[i].sock_fd < 0) {
		return -errno;
	}

	cfg->sockets[i].family = family;
	cfg->sockets[i].type = type;
	cfg->sockets[i].ip_proto = proto;
	/* socket # needs assigning */
	cfg->sockets[i].id = cfg->sockets_len + 1;
	z_finalize_fd(cfg->sockets[i].sock_fd, cfg, &modem_sock_fd_vtable);

	return cfg->sockets[i].sock_fd;
}

struct modem_socket *modem_socket_from_fd(struct modem_socket_config *cfg,
					  int sock_fd)
{
	int i;

	for (i = 0; i < cfg->sockets_len; i++) {
		if (cfg->sockets[i].sock_fd == sock_fd) {
			return &cfg->sockets[i];
		}
	}

	return NULL;
}

struct modem_socket *modem_socket_from_id(struct modem_socket_config *cfg,
					  int id)
{
	int i;

	if (id < cfg->base_socket_num) {
		return NULL;
	}

	for (i = 0; i < cfg->sockets_len; i++) {
		if (cfg->sockets[i].id == id) {
			return &cfg->sockets[i];
		}
	}

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

	z_free_fd(sock->sock_fd);
	sock->id = cfg->base_socket_num - 1;
	sock->sock_fd = -1;
	(void)memset(&sock->src, 0, sizeof(struct sockaddr));
	(void)memset(&sock->dst, 0, sizeof(struct sockaddr));
}

/*
 * Generic Poll Function
 */

/*
 * FIXME: The design here makes the poll function non-reentrant. If two
 * different threads poll on two different sockets we'll end up with unexpected
 * behavior - the higher priority thread will be unblocked, regardless on which
 * socket it polled. I think we could live with such limitation though in the
 * initial implementation, but this should be improved in the future.
 */
int modem_socket_poll(struct modem_socket_config *cfg,
		      struct pollfd *fds, int nfds, int msecs)
{
	struct modem_socket *sock;
	int ret, i;
	u8_t found_count = 0;

	if (!cfg) {
		return -EINVAL;
	}

	for (i = 0; i < nfds; i++) {
		sock = modem_socket_from_fd(cfg, fds[i].fd);
		if (sock) {
			/*
			 * Handle user check for POLLOUT events:
			 * we consider the socket to always be writeable.
			 */
			if (fds[i].events & POLLOUT) {
				fds[i].revents |= POLLOUT;
				found_count++;
			} else if (fds[i].events & POLLIN) {
				sock->is_polled = true;
			}
		}
	}

	/* exit early if we've found rdy sockets */
	if (found_count) {
		errno = 0;
		return found_count;
	}

	ret = k_sem_take(&cfg->sem_poll, msecs);
	for (i = 0; i < nfds; i++) {
		sock = modem_socket_from_fd(cfg, fds[i].fd);
		if (!sock) {
			continue;
		}

		if (fds[i].events & POLLIN && sock->packet_sizes[0] > 0U) {
			fds[i].revents |= POLLIN;
			found_count++;
		}

		sock->is_polled = false;
	}

	/* EBUSY, EAGAIN and ETIMEDOUT aren't true errors */
	if (ret < 0 && ret != -EBUSY && ret != -EAGAIN && ret != -ETIMEDOUT) {
		errno = ret;
		return -1;
	}

	errno = 0;
	return found_count;
}

int modem_socket_init(struct modem_socket_config *cfg)
{
	int i;

	k_sem_init(&cfg->sem_poll, 0, 1);
	for (i = 0; i < cfg->sockets_len; i++) {
		k_sem_init(&cfg->sockets[i].sem_data_ready, 0, 1);
		cfg->sockets[i].id = cfg->base_socket_num - 1;
	}

	return 0;
}
