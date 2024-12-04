/**
 * Copyright (c) 2023-2024 Marcin Niestroj
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * Zephyr (top) side of NSOS (Native Simulator Offloaded Sockets).
 */

#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nsos_sockets);

#include <soc.h>
#include <string.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/offloaded_netdev.h>
#include <zephyr/net/socket_offload.h>
#include <zephyr/posix/fcntl.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/sys/dlist.h>

#include "sockets_internal.h"
#include "nsos.h"
#include "nsos_errno.h"
#include "nsos_fcntl.h"
#include "nsos_netdb.h"
#include "nsos_socket.h"

#include "nsi_host_trampolines.h"

BUILD_ASSERT(CONFIG_HEAP_MEM_POOL_SIZE > 0);

#define NSOS_IRQ_FLAGS		(0)
#define NSOS_IRQ_PRIORITY	(2)

struct nsos_socket;

struct nsos_socket_poll {
	struct nsos_mid_pollfd mid;
	struct k_poll_signal signal;

	sys_dnode_t node;
};

struct nsos_socket {
	int fd;

	k_timeout_t recv_timeout;
	k_timeout_t send_timeout;

	struct nsos_socket_poll poll;
};

static sys_dlist_t nsos_polls = SYS_DLIST_STATIC_INIT(&nsos_polls);

static int socket_family_to_nsos_mid(int family, int *family_mid)
{
	switch (family) {
	case AF_UNSPEC:
		*family_mid = NSOS_MID_AF_UNSPEC;
		break;
	case AF_INET:
		*family_mid = NSOS_MID_AF_INET;
		break;
	case AF_INET6:
		*family_mid = NSOS_MID_AF_INET6;
		break;
	case AF_UNIX:
		*family_mid = NSOS_MID_AF_UNIX;
		break;
	case AF_PACKET:
		*family_mid = NSOS_MID_AF_PACKET;
		break;
	default:
		return -NSOS_MID_EAFNOSUPPORT;
	}

	return 0;
}

static int socket_proto_to_nsos_mid(int proto, int *proto_mid)
{
	switch (proto) {
	case IPPROTO_IP:
		*proto_mid = NSOS_MID_IPPROTO_IP;
		break;
	case IPPROTO_ICMP:
		*proto_mid = NSOS_MID_IPPROTO_ICMP;
		break;
	case IPPROTO_IGMP:
		*proto_mid = NSOS_MID_IPPROTO_IGMP;
		break;
	case IPPROTO_IPIP:
		*proto_mid = NSOS_MID_IPPROTO_IPIP;
		break;
	case IPPROTO_TCP:
		*proto_mid = NSOS_MID_IPPROTO_TCP;
		break;
	case IPPROTO_UDP:
		*proto_mid = NSOS_MID_IPPROTO_UDP;
		break;
	case IPPROTO_IPV6:
		*proto_mid = NSOS_MID_IPPROTO_IPV6;
		break;
	case IPPROTO_RAW:
		*proto_mid = NSOS_MID_IPPROTO_RAW;
		break;
	case htons(IPPROTO_ETH_P_ALL):
		*proto_mid = NSOS_MID_IPPROTO_ETH_P_ALL;
		break;
	default:
		return -NSOS_MID_EPROTONOSUPPORT;
	}

	return 0;
}

static int socket_type_to_nsos_mid(int type, int *type_mid)
{
	switch (type) {
	case SOCK_STREAM:
		*type_mid = NSOS_MID_SOCK_STREAM;
		break;
	case SOCK_DGRAM:
		*type_mid = NSOS_MID_SOCK_DGRAM;
		break;
	case SOCK_RAW:
		*type_mid = NSOS_MID_SOCK_RAW;
		break;
	default:
		return -NSOS_MID_ESOCKTNOSUPPORT;
	}

	return 0;
}

static int socket_flags_to_nsos_mid(int flags)
{
	int flags_mid = 0;

	nsos_socket_flag_convert(&flags, ZSOCK_MSG_PEEK,
				 &flags_mid, NSOS_MID_MSG_PEEK);
	nsos_socket_flag_convert(&flags, ZSOCK_MSG_TRUNC,
				 &flags_mid, NSOS_MID_MSG_TRUNC);
	nsos_socket_flag_convert(&flags, ZSOCK_MSG_DONTWAIT,
				 &flags_mid, NSOS_MID_MSG_DONTWAIT);
	nsos_socket_flag_convert(&flags, ZSOCK_MSG_WAITALL,
				 &flags_mid, NSOS_MID_MSG_WAITALL);

	if (flags != 0) {
		return -NSOS_MID_EINVAL;
	}

	return flags_mid;
}

static const struct socket_op_vtable nsos_socket_fd_op_vtable;

static int nsos_socket_create(int family, int type, int proto)
{
	int fd;
	struct nsos_socket *sock;
	int family_mid;
	int type_mid;
	int proto_mid;
	int err;

	err = socket_family_to_nsos_mid(family, &family_mid);
	if (err) {
		errno = errno_from_nsos_mid(-err);
		return -1;
	}

	err = socket_type_to_nsos_mid(type, &type_mid);
	if (err) {
		errno = errno_from_nsos_mid(-err);
		return -1;
	}

	err = socket_proto_to_nsos_mid(proto, &proto_mid);
	if (err) {
		errno = errno_from_nsos_mid(-err);
		return -1;
	}

	fd = zvfs_reserve_fd();
	if (fd < 0) {
		return -1;
	}

	sock = k_malloc(sizeof(*sock));
	if (!sock) {
		errno = ENOMEM;
		goto free_fd;
	}

	sock->fd = fd;
	sock->recv_timeout = K_FOREVER;
	sock->send_timeout = K_FOREVER;

	sock->poll.mid.fd = nsos_adapt_socket(family_mid, type_mid, proto_mid);
	if (sock->poll.mid.fd < 0) {
		errno = errno_from_nsos_mid(-sock->poll.mid.fd);
		goto free_sock;
	}

	zvfs_finalize_typed_fd(fd, sock, &nsos_socket_fd_op_vtable.fd_vtable, ZVFS_MODE_IFSOCK);

	return fd;

free_sock:
	k_free(sock);

free_fd:
	zvfs_free_fd(fd);

	return -1;
}

static int nsos_adapt_get_zephyr_errno(void)
{
	return errno_from_nsos_mid(nsos_adapt_get_errno());
}

static ssize_t nsos_read(void *obj, void *buf, size_t sz)
{
	struct nsos_socket *sock = obj;
	int ret;

	ret = nsi_host_read(sock->poll.mid.fd, buf, sz);
	if (ret < 0) {
		errno = nsos_adapt_get_zephyr_errno();
	}

	return ret;
}

static ssize_t nsos_write(void *obj, const void *buf, size_t sz)
{
	struct nsos_socket *sock = obj;
	int ret;

	ret = nsi_host_write(sock->poll.mid.fd, buf, sz);
	if (ret < 0) {
		errno = nsos_adapt_get_zephyr_errno();
	}

	return ret;
}

static int nsos_close(void *obj)
{
	struct nsos_socket *sock = obj;
	struct nsos_socket_poll *poll;
	int ret;

	ret = nsi_host_close(sock->poll.mid.fd);
	if (ret < 0) {
		errno = nsos_adapt_get_zephyr_errno();
	}

	SYS_DLIST_FOR_EACH_CONTAINER(&nsos_polls, poll, node) {
		if (poll == &sock->poll) {
			poll->mid.revents = ZSOCK_POLLHUP;
			poll->mid.cb(&poll->mid);
		}
	}

	k_free(sock);

	return ret;
}

static void pollcb(struct nsos_mid_pollfd *mid)
{
	struct nsos_socket_poll *poll = CONTAINER_OF(mid, struct nsos_socket_poll, mid);

	k_poll_signal_raise(&poll->signal, poll->mid.revents);
}

static int nsos_poll_prepare(struct nsos_socket *sock, struct zsock_pollfd *pfd,
			     struct k_poll_event **pev, struct k_poll_event *pev_end,
			     struct nsos_socket_poll *poll)
{
	unsigned int signaled;
	int flags;

	poll->mid.events = pfd->events;
	poll->mid.revents = 0;
	poll->mid.cb = pollcb;

	if (*pev == pev_end) {
		return -ENOMEM;
	}

	k_poll_signal_init(&poll->signal);
	k_poll_event_init(*pev, K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &poll->signal);

	sys_dlist_append(&nsos_polls, &poll->node);

	nsos_adapt_poll_add(&poll->mid);

	/* Let other sockets use another k_poll_event */
	(*pev)++;

	signaled = 0;
	flags = 0;

	k_poll_signal_check(&poll->signal, &signaled, &flags);
	if (!signaled) {
		return 0;
	}

	/* Events are ready, don't wait */
	return -EALREADY;
}

static int nsos_poll_update(struct nsos_socket *sock, struct zsock_pollfd *pfd,
			    struct k_poll_event **pev, struct nsos_socket_poll *poll)
{
	unsigned int signaled;
	int flags;

	(*pev)++;

	signaled = 0;
	flags = 0;

	if (!sys_dnode_is_linked(&poll->node)) {
		nsos_adapt_poll_update(&poll->mid);
		return 0;
	}

	nsos_adapt_poll_remove(&poll->mid);
	sys_dlist_remove(&poll->node);

	k_poll_signal_check(&poll->signal, &signaled, &flags);
	if (!signaled) {
		return 0;
	}

	pfd->revents = flags;

	return 0;
}

static int nsos_ioctl(void *obj, unsigned int request, va_list args)
{
	struct nsos_socket *sock = obj;

	switch (request) {
	case ZFD_IOCTL_POLL_PREPARE: {
		struct zsock_pollfd *pfd;
		struct k_poll_event **pev;
		struct k_poll_event *pev_end;

		pfd = va_arg(args, struct zsock_pollfd *);
		pev = va_arg(args, struct k_poll_event **);
		pev_end = va_arg(args, struct k_poll_event *);

		return nsos_poll_prepare(obj, pfd, pev, pev_end, &sock->poll);
	}

	case ZFD_IOCTL_POLL_UPDATE: {
		struct zsock_pollfd *pfd;
		struct k_poll_event **pev;

		pfd = va_arg(args, struct zsock_pollfd *);
		pev = va_arg(args, struct k_poll_event **);

		return nsos_poll_update(obj, pfd, pev, &sock->poll);
	}

	case ZFD_IOCTL_POLL_OFFLOAD:
		return -EOPNOTSUPP;

	case F_GETFL: {
		int flags;

		flags = nsos_adapt_fcntl_getfl(sock->poll.mid.fd);

		return fl_from_nsos_mid(flags);
	}

	case F_SETFL: {
		int flags = va_arg(args, int);
		int ret;

		ret = fl_to_nsos_mid_strict(flags);
		if (ret < 0) {
			return -errno_from_nsos_mid(-ret);
		}

		ret = nsos_adapt_fcntl_setfl(sock->poll.mid.fd, flags);

		return -errno_from_nsos_mid(-ret);
	}

	case ZFD_IOCTL_FIONREAD: {
		int *avail = va_arg(args, int *);
		int ret;

		ret = nsos_adapt_fionread(sock->poll.mid.fd, avail);

		return -errno_from_nsos_mid(-ret);
	}
	}

	return -EINVAL;
}

static int sockaddr_to_nsos_mid(const struct sockaddr *addr, socklen_t addrlen,
				struct nsos_mid_sockaddr **addr_mid, size_t *addrlen_mid)
{
	if (!addr || !addrlen) {
		*addr_mid = NULL;
		*addrlen_mid = 0;

		return 0;
	}

	switch (addr->sa_family) {
	case AF_INET: {
		const struct sockaddr_in *addr_in =
			(const struct sockaddr_in *)addr;
		struct nsos_mid_sockaddr_in *addr_in_mid =
			(struct nsos_mid_sockaddr_in *)*addr_mid;

		if (addrlen < sizeof(*addr_in)) {
			return -NSOS_MID_EINVAL;
		}

		addr_in_mid->sin_family = NSOS_MID_AF_INET;
		addr_in_mid->sin_port = addr_in->sin_port;
		addr_in_mid->sin_addr = addr_in->sin_addr.s_addr;

		*addrlen_mid = sizeof(*addr_in_mid);

		return 0;
	}
	case AF_INET6: {
		const struct sockaddr_in6 *addr_in =
			(const struct sockaddr_in6 *)addr;
		struct nsos_mid_sockaddr_in6 *addr_in_mid =
			(struct nsos_mid_sockaddr_in6 *)*addr_mid;

		if (addrlen < sizeof(*addr_in)) {
			return -NSOS_MID_EINVAL;
		}

		addr_in_mid->sin6_family = NSOS_MID_AF_INET6;
		addr_in_mid->sin6_port = addr_in->sin6_port;
		memcpy(addr_in_mid->sin6_addr, addr_in->sin6_addr.s6_addr,
		       sizeof(addr_in_mid->sin6_addr));
		addr_in_mid->sin6_scope_id = addr_in->sin6_scope_id;

		*addrlen_mid = sizeof(*addr_in_mid);

		return 0;
	}
	case AF_UNIX: {
		const struct sockaddr_un *addr_un =
			(const struct sockaddr_un *)addr;
		struct nsos_mid_sockaddr_un *addr_un_mid =
			(struct nsos_mid_sockaddr_un *)*addr_mid;

		if (addrlen < sizeof(*addr_un)) {
			return -NSOS_MID_EINVAL;
		}

		addr_un_mid->sun_family = NSOS_MID_AF_UNIX;
		memcpy(addr_un_mid->sun_path, addr_un->sun_path,
		       sizeof(addr_un_mid->sun_path));

		*addrlen_mid = sizeof(*addr_un_mid);

		return 0;
	}
	case AF_PACKET: {
		const struct sockaddr_ll *addr_ll =
			(const struct sockaddr_ll *)addr;
		struct nsos_mid_sockaddr_ll *addr_ll_mid =
			(struct nsos_mid_sockaddr_ll *)*addr_mid;

		if (addrlen < sizeof(*addr_ll)) {
			return -NSOS_MID_EINVAL;
		}

		addr_ll_mid->sll_family = NSOS_MID_AF_UNIX;
		addr_ll_mid->sll_protocol = addr_ll->sll_protocol;
		addr_ll_mid->sll_ifindex = addr_ll->sll_ifindex;
		addr_ll_mid->sll_hatype = addr_ll->sll_hatype;
		addr_ll_mid->sll_pkttype = addr_ll->sll_pkttype;
		addr_ll_mid->sll_halen = addr_ll->sll_halen;
		memcpy(addr_ll_mid->sll_addr, addr_ll->sll_addr,
		       sizeof(addr_ll->sll_addr));

		*addrlen_mid = sizeof(*addr_ll_mid);

		return 0;
	}
	}

	return -NSOS_MID_EINVAL;
}

static int sockaddr_from_nsos_mid(struct sockaddr *addr, socklen_t *addrlen,
				  const struct nsos_mid_sockaddr *addr_mid, size_t addrlen_mid)
{
	if (!addr || !addrlen) {
		return 0;
	}

	switch (addr_mid->sa_family) {
	case NSOS_MID_AF_INET: {
		const struct nsos_mid_sockaddr_in *addr_in_mid =
			(const struct nsos_mid_sockaddr_in *)addr_mid;
		struct sockaddr_in addr_in;

		addr_in.sin_family = AF_INET;
		addr_in.sin_port = addr_in_mid->sin_port;
		addr_in.sin_addr.s_addr = addr_in_mid->sin_addr;

		memcpy(addr, &addr_in, MIN(*addrlen, sizeof(addr_in)));
		*addrlen = sizeof(addr_in);

		return 0;
	}
	case NSOS_MID_AF_INET6: {
		const struct nsos_mid_sockaddr_in6 *addr_in_mid =
			(const struct nsos_mid_sockaddr_in6 *)addr_mid;
		struct sockaddr_in6 addr_in;

		addr_in.sin6_family = AF_INET6;
		addr_in.sin6_port = addr_in_mid->sin6_port;
		memcpy(addr_in.sin6_addr.s6_addr, addr_in_mid->sin6_addr,
		       sizeof(addr_in.sin6_addr.s6_addr));
		addr_in.sin6_scope_id = addr_in_mid->sin6_scope_id;

		memcpy(addr, &addr_in, MIN(*addrlen, sizeof(addr_in)));
		*addrlen = sizeof(addr_in);

		return 0;
	}
	}

	return -NSOS_MID_EINVAL;
}

static int nsos_wait_for_poll(struct nsos_socket *sock, int events,
			      k_timeout_t timeout)
{
	struct zsock_pollfd pfd = {
		.fd = sock->fd,
		.events = events,
	};
	struct k_poll_event poll_events[1];
	struct k_poll_event *pev = poll_events;
	struct k_poll_event *pev_end = poll_events + ARRAY_SIZE(poll_events);
	struct nsos_socket_poll socket_poll = {};
	int ret;

	ret = nsos_adapt_dup(sock->poll.mid.fd);
	if (ret < 0) {
		goto return_ret;
	}

	socket_poll.mid.fd = ret;

	ret = nsos_poll_prepare(sock, &pfd, &pev, pev_end, &socket_poll);
	if (ret == -EALREADY) {
		ret = 0;
		goto poll_update;
	} else if (ret < 0) {
		goto close_dup;
	}

	ret = k_poll(poll_events, ARRAY_SIZE(poll_events), timeout);
	if (ret != 0 && ret != -EAGAIN && ret != -EINTR) {
		goto poll_update;
	}

	ret = 0;

poll_update:
	pev = poll_events;
	nsos_poll_update(sock, &pfd, &pev, &socket_poll);

close_dup:
	nsi_host_close(socket_poll.mid.fd);

return_ret:
	if (ret < 0) {
		return -errno_to_nsos_mid(-ret);
	}

	return 0;
}

static int nsos_poll_if_blocking(struct nsos_socket *sock, int events,
				 k_timeout_t timeout, int flags)
{
	int sock_flags;
	bool non_blocking;

	if (flags & ZSOCK_MSG_DONTWAIT) {
		non_blocking = true;
	} else {
		sock_flags = nsos_adapt_fcntl_getfl(sock->poll.mid.fd);
		non_blocking = sock_flags & NSOS_MID_O_NONBLOCK;
	}

	if (!non_blocking) {
		return nsos_wait_for_poll(sock, events, timeout);
	}

	return 0;
}

static int nsos_bind(void *obj, const struct sockaddr *addr, socklen_t addrlen)
{
	struct nsos_socket *sock = obj;
	struct nsos_mid_sockaddr_storage addr_storage_mid;
	struct nsos_mid_sockaddr *addr_mid = (struct nsos_mid_sockaddr *)&addr_storage_mid;
	size_t addrlen_mid;
	int ret;

	ret = sockaddr_to_nsos_mid(addr, addrlen, &addr_mid, &addrlen_mid);
	if (ret < 0) {
		goto return_ret;
	}

	ret = nsos_adapt_bind(sock->poll.mid.fd, addr_mid, addrlen_mid);

return_ret:
	if (ret < 0) {
		errno = errno_from_nsos_mid(-ret);
		return -1;
	}

	return ret;
}

static int nsos_connect_blocking(struct nsos_socket *sock,
				 struct nsos_mid_sockaddr *addr_mid,
				 size_t addrlen_mid,
				 int fcntl_flags)
{
	int clear_nonblock_ret;
	int ret;

	ret = nsos_adapt_fcntl_setfl(sock->poll.mid.fd, fcntl_flags | NSOS_MID_O_NONBLOCK);
	if (ret < 0) {
		return ret;
	}

	ret = nsos_adapt_connect(sock->poll.mid.fd, addr_mid, addrlen_mid);
	if (ret == -NSOS_MID_EINPROGRESS) {
		int so_err;
		size_t so_err_len = sizeof(so_err);

		ret = nsos_wait_for_poll(sock, ZSOCK_POLLOUT, sock->send_timeout);
		if (ret < 0) {
			goto clear_nonblock;
		}

		ret = nsos_adapt_getsockopt(sock->poll.mid.fd, NSOS_MID_SOL_SOCKET,
					    NSOS_MID_SO_ERROR, &so_err, &so_err_len);
		if (ret < 0) {
			goto clear_nonblock;
		}

		ret = so_err;
	}

clear_nonblock:
	clear_nonblock_ret = nsos_adapt_fcntl_setfl(sock->poll.mid.fd, fcntl_flags);
	if (clear_nonblock_ret < 0) {
		LOG_ERR("Failed to clear O_NONBLOCK: %d", clear_nonblock_ret);
	}

	return ret;
}

static int nsos_connect(void *obj, const struct sockaddr *addr, socklen_t addrlen)
{
	struct nsos_socket *sock = obj;
	struct nsos_mid_sockaddr_storage addr_storage_mid;
	struct nsos_mid_sockaddr *addr_mid = (struct nsos_mid_sockaddr *)&addr_storage_mid;
	size_t addrlen_mid;
	int flags;
	int ret;

	ret = sockaddr_to_nsos_mid(addr, addrlen, &addr_mid, &addrlen_mid);
	if (ret < 0) {
		goto return_ret;
	}

	flags = nsos_adapt_fcntl_getfl(sock->poll.mid.fd);

	if (flags & NSOS_MID_O_NONBLOCK) {
		ret = nsos_adapt_connect(sock->poll.mid.fd, addr_mid, addrlen_mid);
	} else {
		ret = nsos_connect_blocking(sock, addr_mid, addrlen_mid, flags);
	}

return_ret:
	if (ret < 0) {
		errno = errno_from_nsos_mid(-ret);
		return -1;
	}

	return ret;
}

static int nsos_listen(void *obj, int backlog)
{
	struct nsos_socket *sock = obj;
	int ret;

	ret = nsos_adapt_listen(sock->poll.mid.fd, backlog);
	if (ret < 0) {
		errno = errno_from_nsos_mid(-ret);
		return -1;
	}

	return ret;
}

static int nsos_accept(void *obj, struct sockaddr *addr, socklen_t *addrlen)
{
	struct nsos_socket *accept_sock = obj;
	struct nsos_mid_sockaddr_storage addr_storage_mid;
	struct nsos_mid_sockaddr *addr_mid = (struct nsos_mid_sockaddr *)&addr_storage_mid;
	size_t addrlen_mid = sizeof(addr_storage_mid);
	int adapt_fd;
	int zephyr_fd;
	struct nsos_socket *conn_sock;
	int ret;

	ret = nsos_poll_if_blocking(accept_sock, ZSOCK_POLLIN,
				    accept_sock->recv_timeout, 0);
	if (ret < 0) {
		goto return_ret;
	}

	ret = nsos_adapt_accept(accept_sock->poll.mid.fd, addr_mid, &addrlen_mid);
	if (ret < 0) {
		goto return_ret;
	}

	adapt_fd = ret;

	ret = sockaddr_from_nsos_mid(addr, addrlen, addr_mid, addrlen_mid);
	if (ret < 0) {
		goto close_adapt_fd;
	}

	zephyr_fd = zvfs_reserve_fd();
	if (zephyr_fd < 0) {
		ret = -errno_to_nsos_mid(-zephyr_fd);
		goto close_adapt_fd;
	}

	conn_sock = k_malloc(sizeof(*conn_sock));
	if (!conn_sock) {
		ret = -NSOS_MID_ENOMEM;
		goto free_zephyr_fd;
	}

	conn_sock->fd = zephyr_fd;
	conn_sock->poll.mid.fd = adapt_fd;

	zvfs_finalize_typed_fd(zephyr_fd, conn_sock, &nsos_socket_fd_op_vtable.fd_vtable,
			       ZVFS_MODE_IFSOCK);

	return zephyr_fd;

free_zephyr_fd:
	zvfs_free_fd(zephyr_fd);

close_adapt_fd:
	nsi_host_close(adapt_fd);

return_ret:
	errno = errno_from_nsos_mid(-ret);
	return -1;
}

static ssize_t nsos_sendto(void *obj, const void *buf, size_t len, int flags,
			   const struct sockaddr *addr, socklen_t addrlen)
{
	struct nsos_socket *sock = obj;
	struct nsos_mid_sockaddr_storage addr_storage_mid;
	struct nsos_mid_sockaddr *addr_mid = (struct nsos_mid_sockaddr *)&addr_storage_mid;
	size_t addrlen_mid = sizeof(addr_storage_mid);
	int flags_mid;
	int ret;

	ret = socket_flags_to_nsos_mid(flags);
	if (ret < 0) {
		goto return_ret;
	}

	flags_mid = ret;

	ret = sockaddr_to_nsos_mid(addr, addrlen, &addr_mid, &addrlen_mid);
	if (ret < 0) {
		goto return_ret;
	}

	ret = nsos_poll_if_blocking(sock, ZSOCK_POLLOUT, sock->send_timeout, flags);
	if (ret < 0) {
		goto return_ret;
	}

	ret = nsos_adapt_sendto(sock->poll.mid.fd, buf, len, flags_mid,
				addr_mid, addrlen_mid);

return_ret:
	if (ret < 0) {
		errno = errno_from_nsos_mid(-ret);
		return -1;
	}

	return ret;
}

static ssize_t nsos_sendmsg(void *obj, const struct msghdr *msg, int flags)
{
	struct nsos_socket *sock = obj;
	struct nsos_mid_sockaddr_storage addr_storage_mid;
	struct nsos_mid_sockaddr *addr_mid = (struct nsos_mid_sockaddr *)&addr_storage_mid;
	size_t addrlen_mid = sizeof(addr_storage_mid);
	struct nsos_mid_msghdr msg_mid;
	struct nsos_mid_iovec *msg_iov;
	int flags_mid;
	int ret;

	ret = socket_flags_to_nsos_mid(flags);
	if (ret < 0) {
		goto return_ret;
	}

	flags_mid = ret;

	ret = sockaddr_to_nsos_mid(msg->msg_name, msg->msg_namelen, &addr_mid, &addrlen_mid);
	if (ret < 0) {
		goto return_ret;
	}

	msg_iov = k_calloc(msg->msg_iovlen, sizeof(*msg_iov));
	if (!msg_iov) {
		ret = -NSOS_MID_ENOMEM;
		goto return_ret;
	}

	for (size_t i = 0; i < msg->msg_iovlen; i++) {
		msg_iov[i].iov_base = msg->msg_iov[i].iov_base;
		msg_iov[i].iov_len = msg->msg_iov[i].iov_len;
	}

	msg_mid.msg_name = addr_mid;
	msg_mid.msg_namelen = addrlen_mid;
	msg_mid.msg_iov = msg_iov;
	msg_mid.msg_iovlen = msg->msg_iovlen;
	msg_mid.msg_control = NULL;
	msg_mid.msg_controllen = 0;
	msg_mid.msg_flags = 0;

	ret = nsos_poll_if_blocking(sock, ZSOCK_POLLOUT, sock->send_timeout, flags);
	if (ret < 0) {
		goto free_msg_iov;
	}

	ret = nsos_adapt_sendmsg(sock->poll.mid.fd, &msg_mid, flags_mid);

free_msg_iov:
	k_free(msg_iov);

return_ret:
	if (ret < 0) {
		errno = errno_from_nsos_mid(-ret);
		return -1;
	}

	return ret;
}

static ssize_t nsos_recvfrom(void *obj, void *buf, size_t len, int flags,
			     struct sockaddr *addr, socklen_t *addrlen)
{
	struct nsos_socket *sock = obj;
	struct nsos_mid_sockaddr_storage addr_storage_mid;
	struct nsos_mid_sockaddr *addr_mid = (struct nsos_mid_sockaddr *)&addr_storage_mid;
	size_t addrlen_mid = sizeof(addr_storage_mid);
	int flags_mid;
	int ret;

	ret = socket_flags_to_nsos_mid(flags);
	if (ret < 0) {
		goto return_ret;
	}

	flags_mid = ret;

	ret = nsos_poll_if_blocking(sock, ZSOCK_POLLIN, sock->recv_timeout, flags);
	if (ret < 0) {
		goto return_ret;
	}

	ret = nsos_adapt_recvfrom(sock->poll.mid.fd, buf, len, flags_mid,
				  addr_mid, &addrlen_mid);
	if (ret < 0) {
		goto return_ret;
	}

	sockaddr_from_nsos_mid(addr, addrlen, addr_mid, addrlen_mid);

return_ret:
	if (ret < 0) {
		errno = errno_from_nsos_mid(-ret);
		return -1;
	}

	return ret;
}

static ssize_t nsos_recvmsg(void *obj, struct msghdr *msg, int flags)
{
	errno = ENOTSUP;
	return -1;
}

static int socket_type_from_nsos_mid(int type_mid, int *type)
{
	switch (type_mid) {
	case NSOS_MID_SOCK_STREAM:
		*type = SOCK_STREAM;
		break;
	case NSOS_MID_SOCK_DGRAM:
		*type = SOCK_DGRAM;
		break;
	case NSOS_MID_SOCK_RAW:
		*type = SOCK_RAW;
		break;
	default:
		return -NSOS_MID_ESOCKTNOSUPPORT;
	}

	return 0;
}

static int socket_proto_from_nsos_mid(int proto_mid, int *proto)
{
	switch (proto_mid) {
	case NSOS_MID_IPPROTO_IP:
		*proto = IPPROTO_IP;
		break;
	case NSOS_MID_IPPROTO_ICMP:
		*proto = IPPROTO_ICMP;
		break;
	case NSOS_MID_IPPROTO_IGMP:
		*proto = IPPROTO_IGMP;
		break;
	case NSOS_MID_IPPROTO_IPIP:
		*proto = IPPROTO_IPIP;
		break;
	case NSOS_MID_IPPROTO_TCP:
		*proto = IPPROTO_TCP;
		break;
	case NSOS_MID_IPPROTO_UDP:
		*proto = IPPROTO_UDP;
		break;
	case NSOS_MID_IPPROTO_IPV6:
		*proto = IPPROTO_IPV6;
		break;
	case NSOS_MID_IPPROTO_RAW:
		*proto = IPPROTO_RAW;
		break;
	case NSOS_MID_IPPROTO_ETH_P_ALL:
		*proto = htons(IPPROTO_ETH_P_ALL);
		break;
	default:
		return -NSOS_MID_EPROTONOSUPPORT;
	}

	return 0;
}

static int socket_family_from_nsos_mid(int family_mid, int *family)
{
	switch (family_mid) {
	case NSOS_MID_AF_UNSPEC:
		*family = AF_UNSPEC;
		break;
	case NSOS_MID_AF_INET:
		*family = AF_INET;
		break;
	case NSOS_MID_AF_INET6:
		*family = AF_INET6;
		break;
	case NSOS_MID_AF_UNIX:
		*family = AF_UNIX;
		break;
	case NSOS_MID_AF_PACKET:
		*family = AF_PACKET;
		break;
	default:
		return -NSOS_MID_EAFNOSUPPORT;
	}

	return 0;
}

static int nsos_getsockopt_int(struct nsos_socket *sock, int nsos_mid_level, int nsos_mid_optname,
			       void *optval, socklen_t *optlen)
{
	size_t nsos_mid_optlen = sizeof(int);
	int err;

	if (*optlen != sizeof(int)) {
		errno = EINVAL;
		return -1;
	}

	err = nsos_adapt_getsockopt(sock->poll.mid.fd, NSOS_MID_SOL_SOCKET,
				    NSOS_MID_SO_KEEPALIVE, optval, &nsos_mid_optlen);
	if (err) {
		errno = errno_from_nsos_mid(-err);
		return -1;
	}

	*optlen = nsos_mid_optlen;

	return 0;
}

static int nsos_getsockopt(void *obj, int level, int optname,
			   void *optval, socklen_t *optlen)
{
	struct nsos_socket *sock = obj;

	switch (level) {
	case SOL_SOCKET:
		switch (optname) {
		case SO_ERROR: {
			int nsos_mid_err;
			int err;

			if (*optlen != sizeof(int)) {
				errno = EINVAL;
				return -1;
			}

			err = nsos_adapt_getsockopt(sock->poll.mid.fd, NSOS_MID_SOL_SOCKET,
						    NSOS_MID_SO_ERROR, &nsos_mid_err, NULL);
			if (err) {
				errno = errno_from_nsos_mid(-err);
				return -1;
			}

			*(int *)optval = errno_from_nsos_mid(nsos_mid_err);

			return 0;
		}
		case SO_TYPE: {
			int nsos_mid_type;
			int err;

			if (*optlen != sizeof(nsos_mid_type)) {
				errno = EINVAL;
				return -1;
			}

			err = nsos_adapt_getsockopt(sock->poll.mid.fd, NSOS_MID_SOL_SOCKET,
						    NSOS_MID_SO_TYPE, &nsos_mid_type, NULL);
			if (err) {
				errno = errno_from_nsos_mid(-err);
				return -1;
			}

			err = socket_type_from_nsos_mid(nsos_mid_type, optval);
			if (err) {
				errno = errno_from_nsos_mid(-err);
				return -1;
			}

			return 0;
		}
		case SO_PROTOCOL: {
			int nsos_mid_proto;
			int err;

			if (*optlen != sizeof(nsos_mid_proto)) {
				errno = EINVAL;
				return -1;
			}

			err = nsos_adapt_getsockopt(sock->poll.mid.fd, NSOS_MID_SOL_SOCKET,
						    NSOS_MID_SO_PROTOCOL, &nsos_mid_proto, NULL);
			if (err) {
				errno = errno_from_nsos_mid(-err);
				return -1;
			}

			err = socket_proto_from_nsos_mid(nsos_mid_proto, optval);
			if (err) {
				errno = errno_from_nsos_mid(-err);
				return -1;
			}

			return 0;
		}
		case SO_DOMAIN: {
			int nsos_mid_family;
			int err;

			if (*optlen != sizeof(nsos_mid_family)) {
				errno = EINVAL;
				return -1;
			}

			err = nsos_adapt_getsockopt(sock->poll.mid.fd, NSOS_MID_SOL_SOCKET,
						    NSOS_MID_SO_DOMAIN, &nsos_mid_family, NULL);
			if (err) {
				errno = errno_from_nsos_mid(-err);
				return -1;
			}

			err = socket_family_from_nsos_mid(nsos_mid_family, optval);
			if (err) {
				errno = errno_from_nsos_mid(-err);
				return -1;
			}

			return 0;
		}
		case SO_RCVBUF:
			return nsos_getsockopt_int(sock,
						   NSOS_MID_SOL_SOCKET, NSOS_MID_SO_RCVBUF,
						   optval, optlen);
		case SO_SNDBUF:
			return nsos_getsockopt_int(sock,
						   NSOS_MID_SOL_SOCKET, NSOS_MID_SO_SNDBUF,
						   optval, optlen);
		case SO_REUSEADDR:
			return nsos_getsockopt_int(sock,
						   NSOS_MID_SOL_SOCKET, NSOS_MID_SO_REUSEADDR,
						   optval, optlen);
		case SO_REUSEPORT:
			return nsos_getsockopt_int(sock,
						   NSOS_MID_SOL_SOCKET, NSOS_MID_SO_REUSEPORT,
						   optval, optlen);
		case SO_KEEPALIVE:
			return nsos_getsockopt_int(sock,
						   NSOS_MID_SOL_SOCKET, NSOS_MID_SO_KEEPALIVE,
						   optval, optlen);
		}
		break;

	case IPPROTO_TCP:
		switch (optname) {
		case TCP_NODELAY:
			return nsos_getsockopt_int(sock,
						   NSOS_MID_IPPROTO_TCP, NSOS_MID_TCP_NODELAY,
						   optval, optlen);
		case TCP_KEEPIDLE:
			return nsos_getsockopt_int(sock,
						   NSOS_MID_IPPROTO_TCP, NSOS_MID_TCP_KEEPIDLE,
						   optval, optlen);
		case TCP_KEEPINTVL:
			return nsos_getsockopt_int(sock,
						   NSOS_MID_IPPROTO_TCP, NSOS_MID_TCP_KEEPINTVL,
						   optval, optlen);
		case TCP_KEEPCNT:
			return nsos_getsockopt_int(sock,
						   NSOS_MID_IPPROTO_TCP, NSOS_MID_TCP_KEEPCNT,
						   optval, optlen);
		}
		break;

	case IPPROTO_IPV6:
		switch (optname) {
		case IPV6_V6ONLY:
			return nsos_getsockopt_int(sock,
						   NSOS_MID_IPPROTO_IPV6, NSOS_MID_IPV6_V6ONLY,
						   optval, optlen);
		}
		break;
	}

	errno = EOPNOTSUPP;
	return -1;
}

static int nsos_setsockopt_int(struct nsos_socket *sock, int nsos_mid_level, int nsos_mid_optname,
			       const void *optval, socklen_t optlen)
{
	int err;

	if (optlen != sizeof(int)) {
		errno = EINVAL;
		return -1;
	}

	err = nsos_adapt_setsockopt(sock->poll.mid.fd, nsos_mid_level, nsos_mid_optname,
				    optval, optlen);
	if (err) {
		errno = errno_from_nsos_mid(-err);
		return -1;
	}

	return 0;
}

static int nsos_setsockopt(void *obj, int level, int optname,
			   const void *optval, socklen_t optlen)
{
	struct nsos_socket *sock = obj;

	switch (level) {
	case SOL_SOCKET:
		switch (optname) {
		case SO_PRIORITY: {
			int nsos_mid_priority;
			int err;

			if (optlen != sizeof(uint8_t)) {
				errno = EINVAL;
				return -1;
			}

			nsos_mid_priority = *(uint8_t *)optval;

			err = nsos_adapt_setsockopt(sock->poll.mid.fd, NSOS_MID_SOL_SOCKET,
						    NSOS_MID_SO_PRIORITY, &nsos_mid_priority,
						    sizeof(nsos_mid_priority));
			if (err) {
				errno = errno_from_nsos_mid(-err);
				return -1;
			}

			return 0;
		}
		case SO_RCVTIMEO: {
			const struct zsock_timeval *tv = optval;
			struct nsos_mid_timeval nsos_mid_tv;
			int err;

			if (optlen != sizeof(struct zsock_timeval)) {
				errno = EINVAL;
				return -1;
			}

			nsos_mid_tv.tv_sec = tv->tv_sec;
			nsos_mid_tv.tv_usec = tv->tv_usec;

			err = nsos_adapt_setsockopt(sock->poll.mid.fd, NSOS_MID_SOL_SOCKET,
						    NSOS_MID_SO_RCVTIMEO, &nsos_mid_tv,
						    sizeof(nsos_mid_tv));
			if (err) {
				errno = errno_from_nsos_mid(-err);
				return -1;
			}

			if (tv->tv_sec == 0 && tv->tv_usec == 0) {
				sock->recv_timeout = K_FOREVER;
			} else {
				sock->recv_timeout = K_USEC(tv->tv_sec * 1000000LL + tv->tv_usec);
			}

			return 0;
		}
		case SO_SNDTIMEO: {
			const struct zsock_timeval *tv = optval;
			struct nsos_mid_timeval nsos_mid_tv;
			int err;

			if (optlen != sizeof(struct zsock_timeval)) {
				errno = EINVAL;
				return -1;
			}

			nsos_mid_tv.tv_sec = tv->tv_sec;
			nsos_mid_tv.tv_usec = tv->tv_usec;

			err = nsos_adapt_setsockopt(sock->poll.mid.fd, NSOS_MID_SOL_SOCKET,
						    NSOS_MID_SO_SNDTIMEO, &nsos_mid_tv,
						    sizeof(nsos_mid_tv));
			if (err) {
				errno = errno_from_nsos_mid(-err);
				return -1;
			}

			if (tv->tv_sec == 0 && tv->tv_usec == 0) {
				sock->send_timeout = K_FOREVER;
			} else {
				sock->send_timeout = K_USEC(tv->tv_sec * 1000000LL + tv->tv_usec);
			}

			return 0;
		}
		case SO_RCVBUF:
			return nsos_setsockopt_int(sock,
						   NSOS_MID_SOL_SOCKET, NSOS_MID_SO_RCVBUF,
						   optval, optlen);
		case SO_SNDBUF:
			return nsos_setsockopt_int(sock,
						   NSOS_MID_SOL_SOCKET, NSOS_MID_SO_SNDBUF,
						   optval, optlen);
		case SO_REUSEADDR:
			return nsos_setsockopt_int(sock,
						   NSOS_MID_SOL_SOCKET, NSOS_MID_SO_REUSEADDR,
						   optval, optlen);
		case SO_REUSEPORT:
			return nsos_setsockopt_int(sock,
						   NSOS_MID_SOL_SOCKET, NSOS_MID_SO_REUSEPORT,
						   optval, optlen);
		case SO_LINGER:
			return nsos_setsockopt_int(sock,
						   NSOS_MID_SOL_SOCKET, NSOS_MID_SO_LINGER,
						   optval, optlen);
		case SO_KEEPALIVE:
			return nsos_setsockopt_int(sock,
						   NSOS_MID_SOL_SOCKET, NSOS_MID_SO_KEEPALIVE,
						   optval, optlen);
		}
		break;

	case IPPROTO_TCP:
		switch (optname) {
		case TCP_NODELAY:
			return nsos_setsockopt_int(sock,
						   NSOS_MID_IPPROTO_TCP, NSOS_MID_TCP_NODELAY,
						   optval, optlen);
		case TCP_KEEPIDLE:
			return nsos_setsockopt_int(sock,
						   NSOS_MID_IPPROTO_TCP, NSOS_MID_TCP_KEEPIDLE,
						   optval, optlen);
		case TCP_KEEPINTVL:
			return nsos_setsockopt_int(sock,
						   NSOS_MID_IPPROTO_TCP, NSOS_MID_TCP_KEEPINTVL,
						   optval, optlen);
		case TCP_KEEPCNT:
			return nsos_setsockopt_int(sock,
						   NSOS_MID_IPPROTO_TCP, NSOS_MID_TCP_KEEPCNT,
						   optval, optlen);
		}
		break;

	case IPPROTO_IPV6:
		switch (optname) {
		case IPV6_V6ONLY:
			return nsos_setsockopt_int(sock,
						   NSOS_MID_IPPROTO_IPV6, NSOS_MID_IPV6_V6ONLY,
						   optval, optlen);
		}
		break;
	}

	errno = EOPNOTSUPP;
	return -1;
}

static const struct socket_op_vtable nsos_socket_fd_op_vtable = {
	.fd_vtable = {
		.read = nsos_read,
		.write = nsos_write,
		.close = nsos_close,
		.ioctl = nsos_ioctl,
	},
	.bind = nsos_bind,
	.connect = nsos_connect,
	.listen = nsos_listen,
	.accept = nsos_accept,
	.sendto = nsos_sendto,
	.sendmsg = nsos_sendmsg,
	.recvfrom = nsos_recvfrom,
	.recvmsg = nsos_recvmsg,
	.getsockopt = nsos_getsockopt,
	.setsockopt = nsos_setsockopt,
};

static bool nsos_is_supported(int family, int type, int proto)
{
	int dummy;
	int err;

	err = socket_family_to_nsos_mid(family, &dummy);
	if (err) {
		return false;
	}

	err = socket_type_to_nsos_mid(type, &dummy);
	if (err) {
		return false;
	}

	err = socket_proto_to_nsos_mid(proto, &dummy);
	if (err) {
		return false;
	}

	return true;
}

NET_SOCKET_OFFLOAD_REGISTER(nsos, CONFIG_NET_SOCKETS_OFFLOAD_PRIORITY, AF_UNSPEC,
			    nsos_is_supported, nsos_socket_create);

struct zsock_addrinfo_wrap {
	struct zsock_addrinfo addrinfo;
	struct sockaddr_storage addr_storage;
	struct nsos_mid_addrinfo *addrinfo_mid;
};

/*
 * (Zephyr)
 * zsock_addrinfo_wrap
 * -----------------------
 * | zsock_addrinfo      |
 * -----------------------    (trampoline)
 * | sockaddr_storage    |    nsos_addrinfo_wrap
 * -----------------------    -----------------------------
 * | nsos_mid_addrinfo * | -> | nsos_mid_addrinfo         |
 * -----------------------    -----------------------------
 *                            | nsos_mid_sockaddr_storage |
 *                            -----------------------------    (Linux host)
 *                            | addrinfo *                | -> addrinfo
 *                            -----------------------------
 */

static int addrinfo_from_nsos_mid(struct nsos_mid_addrinfo *nsos_res,
				  struct zsock_addrinfo **res)
{
	struct zsock_addrinfo_wrap *res_wraps;
	size_t idx_res = 0;
	size_t n_res = 0;

	for (struct nsos_mid_addrinfo *res_p = nsos_res; res_p; res_p = res_p->ai_next) {
		n_res++;
	}

	if (n_res == 0) {
		return 0;
	}

	res_wraps = k_calloc(n_res, sizeof(*res_wraps));
	if (!res_wraps) {
		return -ENOMEM;
	}

	for (struct nsos_mid_addrinfo *res_p = nsos_res; res_p; res_p = res_p->ai_next, idx_res++) {
		struct zsock_addrinfo_wrap *wrap = &res_wraps[idx_res];

		wrap->addrinfo_mid = res_p;

		wrap->addrinfo.ai_flags = res_p->ai_flags;
		wrap->addrinfo.ai_family = res_p->ai_family;
		wrap->addrinfo.ai_socktype = res_p->ai_socktype;
		wrap->addrinfo.ai_protocol = res_p->ai_protocol;

		wrap->addrinfo.ai_addr =
			(struct sockaddr *)&wrap->addr_storage;
		wrap->addrinfo.ai_addrlen = sizeof(wrap->addr_storage);

		sockaddr_from_nsos_mid(wrap->addrinfo.ai_addr, &wrap->addrinfo.ai_addrlen,
				       res_p->ai_addr, res_p->ai_addrlen);

		wrap->addrinfo.ai_canonname =
			res_p->ai_canonname ? strdup(res_p->ai_canonname) : NULL;
		wrap->addrinfo.ai_next = &wrap[1].addrinfo;
	}

	res_wraps[n_res - 1].addrinfo.ai_next = NULL;

	*res = &res_wraps->addrinfo;

	return 0;
}

static int nsos_getaddrinfo(const char *node, const char *service,
			    const struct zsock_addrinfo *hints,
			    struct zsock_addrinfo **res)
{
	struct nsos_mid_addrinfo hints_mid;
	struct nsos_mid_addrinfo *res_mid;
	int system_errno;
	int ret;

	if (!res) {
		return -EINVAL;
	}

	if (hints) {
		hints_mid.ai_flags    = hints->ai_flags;
		hints_mid.ai_family   = hints->ai_family;
		hints_mid.ai_socktype = hints->ai_socktype;
		hints_mid.ai_protocol = hints->ai_protocol;
	}

	ret = nsos_adapt_getaddrinfo(node, service,
				     hints ? &hints_mid : NULL,
				     &res_mid,
				     &system_errno);
	if (ret < 0) {
		if (ret == NSOS_MID_EAI_SYSTEM) {
			errno = errno_from_nsos_mid(system_errno);
		}

		return eai_from_nsos_mid(ret);
	}

	ret = addrinfo_from_nsos_mid(res_mid, res);
	if (ret < 0) {
		errno = -ret;
		return DNS_EAI_SYSTEM;
	}

	return ret;
}

static void nsos_freeaddrinfo(struct zsock_addrinfo *res)
{
	struct zsock_addrinfo_wrap *wrap =
		CONTAINER_OF(res, struct zsock_addrinfo_wrap, addrinfo);

	for (struct zsock_addrinfo *res_p = res; res_p; res_p = res_p->ai_next) {
		free(res_p->ai_canonname);
	}

	nsos_adapt_freeaddrinfo(wrap->addrinfo_mid);
	k_free(wrap);
}

static const struct socket_dns_offload nsos_dns_ops = {
	.getaddrinfo = nsos_getaddrinfo,
	.freeaddrinfo = nsos_freeaddrinfo,
};

static void nsos_isr(const void *obj)
{
	struct nsos_socket_poll *poll;

	SYS_DLIST_FOR_EACH_CONTAINER(&nsos_polls, poll, node) {
		if (poll->mid.revents) {
			poll->mid.cb(&poll->mid);
		}
	}
}

static int nsos_socket_offload_init(const struct device *arg)
{
	ARG_UNUSED(arg);

	IRQ_CONNECT(NSOS_IRQ, NSOS_IRQ_PRIORITY,
		    nsos_isr, NULL, NSOS_IRQ_FLAGS);
	irq_enable(NSOS_IRQ);

	return 0;
}

static void nsos_iface_api_init(struct net_if *iface)
{
	iface->if_dev->socket_offload = nsos_socket_create;

	socket_offload_dns_register(&nsos_dns_ops);
}

static int nsos_iface_enable(const struct net_if *iface, bool enabled)
{
	ARG_UNUSED(iface);
	ARG_UNUSED(enabled);
	return 0;
}

static struct offloaded_if_api nsos_iface_offload_api = {
	.iface_api.init = nsos_iface_api_init,
	.enable = nsos_iface_enable,
};

NET_DEVICE_OFFLOAD_INIT(nsos_socket, "nsos_socket",
			nsos_socket_offload_init,
			NULL,
			NULL, NULL,
			0, &nsos_iface_offload_api, NET_ETH_MTU);
