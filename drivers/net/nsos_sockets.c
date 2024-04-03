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

#include "nsi_host_trampolines.h"

BUILD_ASSERT(CONFIG_HEAP_MEM_POOL_SIZE > 0);

#define NSOS_IRQ_FLAGS		(0)
#define NSOS_IRQ_PRIORITY	(2)

struct nsos_socket {
	int fd;
	struct nsos_mid_pollfd pollfd;
	struct k_poll_signal poll;

	sys_dnode_t node;
};

static sys_dlist_t nsos_sockets = SYS_DLIST_STATIC_INIT(&nsos_sockets);

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

	fd = z_reserve_fd();
	if (fd < 0) {
		return -1;
	}

	sock = k_malloc(sizeof(*sock));
	if (!sock) {
		errno = ENOMEM;
		goto free_fd;
	}

	sock->fd = fd;

	sock->pollfd.fd = nsos_adapt_socket(family_mid, type_mid, proto_mid);
	if (sock->pollfd.fd < 0) {
		errno = errno_from_nsos_mid(-sock->pollfd.fd);
		goto free_sock;
	}

	z_finalize_fd(fd, sock, &nsos_socket_fd_op_vtable.fd_vtable);

	return fd;

free_sock:
	k_free(sock);

free_fd:
	z_free_fd(fd);

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

	ret = nsi_host_read(sock->pollfd.fd, buf, sz);
	if (ret < 0) {
		errno = nsos_adapt_get_zephyr_errno();
	}

	return ret;
}

static ssize_t nsos_write(void *obj, const void *buf, size_t sz)
{
	struct nsos_socket *sock = obj;
	int ret;

	ret = nsi_host_write(sock->pollfd.fd, buf, sz);
	if (ret < 0) {
		errno = nsos_adapt_get_zephyr_errno();
	}

	return ret;
}

static int nsos_close(void *obj)
{
	struct nsos_socket *sock = obj;
	int ret;

	ret = nsi_host_close(sock->pollfd.fd);
	if (ret < 0) {
		errno = nsos_adapt_get_zephyr_errno();
	}

	return ret;
}

static void pollcb(struct nsos_mid_pollfd *pollfd)
{
	struct nsos_socket *sock = CONTAINER_OF(pollfd, struct nsos_socket, pollfd);

	k_poll_signal_raise(&sock->poll, sock->pollfd.revents);
}

static int nsos_poll_prepare(struct nsos_socket *sock, struct zsock_pollfd *pfd,
			     struct k_poll_event **pev, struct k_poll_event *pev_end)
{
	unsigned int signaled;
	int flags;

	sock->pollfd.events = pfd->events;
	sock->pollfd.revents = 0;
	sock->pollfd.cb = pollcb;

	if (*pev == pev_end) {
		errno = ENOMEM;
		return -1;
	}

	k_poll_signal_init(&sock->poll);
	k_poll_event_init(*pev, K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &sock->poll);

	sys_dlist_append(&nsos_sockets, &sock->node);

	nsos_adapt_poll_add(&sock->pollfd);

	/* Let other sockets use another k_poll_event */
	(*pev)++;

	signaled = 0;
	flags = 0;

	k_poll_signal_check(&sock->poll, &signaled, &flags);
	if (!signaled) {
		return 0;
	}

	/* Events are ready, don't wait */
	return -EALREADY;
}

static int nsos_poll_update(struct nsos_socket *sock, struct zsock_pollfd *pfd,
			    struct k_poll_event **pev)
{
	unsigned int signaled;
	int flags;

	(*pev)++;

	signaled = 0;
	flags = 0;

	nsos_adapt_poll_remove(&sock->pollfd);
	sys_dlist_remove(&sock->node);

	k_poll_signal_check(&sock->poll, &signaled, &flags);
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

		return nsos_poll_prepare(obj, pfd, pev, pev_end);
	}

	case ZFD_IOCTL_POLL_UPDATE: {
		struct zsock_pollfd *pfd;
		struct k_poll_event **pev;

		pfd = va_arg(args, struct zsock_pollfd *);
		pev = va_arg(args, struct k_poll_event **);

		return nsos_poll_update(obj, pfd, pev);
	}

	case ZFD_IOCTL_POLL_OFFLOAD:
		return -EOPNOTSUPP;

	case F_GETFL: {
		int flags;

		flags = nsos_adapt_fcntl_getfl(sock->pollfd.fd);

		return fl_from_nsos_mid(flags);
	}

	case F_SETFL: {
		int flags = va_arg(args, int);
		int ret;

		ret = fl_to_nsos_mid_strict(flags);
		if (ret < 0) {
			return -errno_from_nsos_mid(-ret);
		}

		ret = nsos_adapt_fcntl_setfl(sock->pollfd.fd, flags);

		return -errno_from_nsos_mid(-ret);
	}
	}

	return -EINVAL;
}

static int sockaddr_to_nsos_mid(const struct sockaddr *addr, socklen_t *addrlen,
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

		if (*addrlen < sizeof(*addr_in)) {
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

		if (*addrlen < sizeof(*addr_in)) {
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

static int nsos_bind(void *obj, const struct sockaddr *addr, socklen_t addrlen)
{
	struct nsos_socket *sock = obj;
	struct nsos_mid_sockaddr_storage addr_storage_mid;
	struct nsos_mid_sockaddr *addr_mid = (struct nsos_mid_sockaddr *)&addr_storage_mid;
	size_t addrlen_mid;
	int ret;

	ret = sockaddr_to_nsos_mid(addr, &addrlen, &addr_mid, &addrlen_mid);
	if (ret < 0) {
		goto return_ret;
	}

	ret = nsos_adapt_bind(sock->pollfd.fd, addr_mid, addrlen_mid);

return_ret:
	if (ret < 0) {
		errno = errno_from_nsos_mid(-ret);
		return -1;
	}

	return ret;
}

static int nsos_connect(void *obj, const struct sockaddr *addr, socklen_t addrlen)
{
	struct nsos_socket *sock = obj;
	struct nsos_mid_sockaddr_storage addr_storage_mid;
	struct nsos_mid_sockaddr *addr_mid = (struct nsos_mid_sockaddr *)&addr_storage_mid;
	size_t addrlen_mid;
	int ret;

	ret = sockaddr_to_nsos_mid(addr, &addrlen, &addr_mid, &addrlen_mid);
	if (ret < 0) {
		goto return_ret;
	}

	ret = nsos_adapt_connect(sock->pollfd.fd, addr_mid, addrlen_mid);

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

	ret = nsos_adapt_listen(sock->pollfd.fd, backlog);
	if (ret < 0) {
		errno = errno_from_nsos_mid(-ret);
		return -1;
	}

	return ret;
}

static int nsos_wait_for_pollin(struct nsos_socket *sock)
{
	struct zsock_pollfd pfd = {
		.fd = sock->fd,
		.events = ZSOCK_POLLIN,
	};
	struct k_poll_event poll_events[1];
	struct k_poll_event *pev = poll_events;
	struct k_poll_event *pev_end = poll_events + ARRAY_SIZE(poll_events);
	int ret;

	ret = nsos_poll_prepare(sock, &pfd, &pev, pev_end);
	if (ret == -EALREADY) {
		return 0;
	} else if (ret < 0) {
		return ret;
	}

	ret = k_poll(poll_events, ARRAY_SIZE(poll_events), K_FOREVER);
	if (ret != 0 && ret != -EAGAIN && ret != -EINTR) {
		return ret;
	}

	pev = poll_events;
	nsos_poll_update(sock, &pfd, &pev);

	return 0;
}

static int nsos_accept_with_poll(struct nsos_socket *sock,
				 struct nsos_mid_sockaddr *addr_mid,
				 size_t *addrlen_mid)
{
	int ret = 0;
	int flags;

	flags = nsos_adapt_fcntl_getfl(sock->pollfd.fd);

	if (!(flags & NSOS_MID_O_NONBLOCK)) {
		ret = nsos_wait_for_pollin(sock);
		if (ret < 0) {
			return ret;
		}
	}

	ret = nsos_adapt_accept(sock->pollfd.fd, addr_mid, addrlen_mid);
	if (ret < 0) {
		return -errno_from_nsos_mid(-ret);
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

	ret = nsos_accept_with_poll(accept_sock, addr_mid, &addrlen_mid);
	if (ret < 0) {
		errno = errno_from_nsos_mid(-ret);
		return -1;
	}

	adapt_fd = ret;

	ret = sockaddr_from_nsos_mid(addr, addrlen, addr_mid, addrlen_mid);
	if (ret < 0) {
		errno = errno_from_nsos_mid(-ret);
		goto close_adapt_fd;
	}

	zephyr_fd = z_reserve_fd();
	if (zephyr_fd < 0) {
		goto close_adapt_fd;
	}

	conn_sock = k_malloc(sizeof(*conn_sock));
	if (!conn_sock) {
		errno = ENOMEM;
		goto free_zephyr_fd;
	}

	conn_sock->fd = zephyr_fd;
	conn_sock->pollfd.fd = adapt_fd;

	z_finalize_fd(zephyr_fd, conn_sock, &nsos_socket_fd_op_vtable.fd_vtable);

	return zephyr_fd;

free_zephyr_fd:
	z_free_fd(zephyr_fd);

close_adapt_fd:
	nsi_host_close(adapt_fd);

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

	ret = sockaddr_to_nsos_mid(addr, &addrlen, &addr_mid, &addrlen_mid);
	if (ret < 0) {
		goto return_ret;
	}

	ret = nsos_adapt_sendto(sock->pollfd.fd, buf, len, flags_mid,
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
	errno = ENOTSUP;
	return -1;
}

static int nsos_recvfrom_with_poll(struct nsos_socket *sock, void *buf, size_t len, int flags,
				   struct nsos_mid_sockaddr *addr_mid, size_t *addrlen_mid)
{
	int ret = 0;
	int sock_flags;
	bool non_blocking;

	if (flags & ZSOCK_MSG_DONTWAIT) {
		non_blocking = true;
	} else {
		sock_flags = nsos_adapt_fcntl_getfl(sock->pollfd.fd);
		non_blocking = sock_flags & NSOS_MID_O_NONBLOCK;
	}

	if (!non_blocking) {
		ret = nsos_wait_for_pollin(sock);
		if (ret < 0) {
			return ret;
		}
	}

	ret = nsos_adapt_recvfrom(sock->pollfd.fd, buf, len, flags,
				  addr_mid, addrlen_mid);
	if (ret < 0) {
		return -errno_from_nsos_mid(-ret);
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

	ret = nsos_recvfrom_with_poll(sock, buf, len, flags_mid,
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
	struct nsos_socket *sock;

	SYS_DLIST_FOR_EACH_CONTAINER(&nsos_sockets, sock, node) {
		if (sock->pollfd.revents) {
			sock->pollfd.cb(&sock->pollfd);
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
