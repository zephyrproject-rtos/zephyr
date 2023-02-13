/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>

#include "sockets_internal.h"

LOG_MODULE_REGISTER(net_sock_dispatcher, CONFIG_NET_SOCKETS_LOG_LEVEL);

__net_socket struct dispatcher_context {
	int fd;
	int family;
	int type;
	int proto;
	bool is_used;
};

static struct dispatcher_context
	dispatcher_context[CONFIG_NET_SOCKETS_OFFLOAD_DISPATCHER_CONTEXT_MAX];

static K_MUTEX_DEFINE(dispatcher_lock);

static int sock_dispatch_create(int family, int type, int proto);

static bool is_tls(int proto)
{
	if ((proto >= IPPROTO_TLS_1_0 && proto <= IPPROTO_TLS_1_2) ||
	    (proto >= IPPROTO_DTLS_1_0 && proto <= IPPROTO_DTLS_1_2)) {
		return true;
	}

	return false;
}

static void dispatcher_ctx_free(struct dispatcher_context *ctx)
{
	(void)k_mutex_lock(&dispatcher_lock, K_FOREVER);

	/* Free the dispatcher entry. */
	memset(ctx, 0, sizeof(*ctx));

	k_mutex_unlock(&dispatcher_lock);
}

static int sock_dispatch_socket(struct dispatcher_context *ctx,
				net_socket_create_t socket_create)
{
	int new_fd, fd;
	const struct socket_op_vtable *vtable;
	void *obj;

	new_fd = socket_create(ctx->family, ctx->type, ctx->proto);
	if (new_fd < 0) {
		LOG_INF("Failed to create socket to dispatch");
		return -1;
	}

	obj = z_get_fd_obj_and_vtable(new_fd,
				      (const struct fd_op_vtable **)&vtable,
				      NULL);
	if (obj == NULL) {
		return -1;
	}

	/* Reassing FD with new obj and entry. */
	fd = ctx->fd;
	z_finalize_fd(fd, obj, (const struct fd_op_vtable *)vtable);

	/* Release FD that is no longer in use. */
	z_free_fd(new_fd);

	dispatcher_ctx_free(ctx);

	return fd;
}

static struct net_socket_register *sock_dispatch_find(int family, int type,
						      int proto, bool native_only)
{
	STRUCT_SECTION_FOREACH(net_socket_register, sock_family) {
		/* Ignore dispatcher itself. */
		if (sock_family->handler == sock_dispatch_create) {
			continue;
		}

		if (native_only && sock_family->is_offloaded) {
			continue;
		}

		if (sock_family->family != family &&
		    sock_family->family != AF_UNSPEC) {
			continue;
		}

		NET_ASSERT(sock_family->is_supported);

		if (!sock_family->is_supported(family, type, proto)) {
			continue;
		}

		return sock_family;
	}

	return NULL;
}

static int sock_dispatch_native(struct dispatcher_context *ctx)
{
	struct net_socket_register *sock_family;

	sock_family = sock_dispatch_find(ctx->family, ctx->type,
					 ctx->proto, true);
	if (sock_family == NULL) {
		errno = ENOENT;
		return -1;
	}

	return sock_dispatch_socket(ctx, sock_family->handler);
}

static int sock_dispatch_default(struct dispatcher_context *ctx)
{
	struct net_socket_register *sock_family;

	sock_family = sock_dispatch_find(ctx->family, ctx->type,
					 ctx->proto, false);
	if (sock_family == NULL) {
		errno = ENOENT;
		return -1;
	}

	return sock_dispatch_socket(ctx, sock_family->handler);
}

static ssize_t sock_dispatch_read_vmeth(void *obj, void *buffer, size_t count)
{
	int fd;
	const struct fd_op_vtable *vtable;
	void *new_obj;

	fd = sock_dispatch_default(obj);
	if (fd < 0) {
		return -1;
	}

	new_obj = z_get_fd_obj_and_vtable(fd, &vtable, NULL);
	if (new_obj == NULL) {
		return -1;
	}

	return vtable->read(new_obj, buffer, count);
}

static ssize_t sock_dispatch_write_vmeth(void *obj, const void *buffer,
					 size_t count)
{
	int fd;
	const struct fd_op_vtable *vtable;
	void *new_obj;

	fd = sock_dispatch_default(obj);
	if (fd < 0) {
		return -1;
	}

	new_obj = z_get_fd_obj_and_vtable(fd, &vtable, NULL);
	if (new_obj == NULL) {
		return -1;
	}

	return vtable->write(new_obj, buffer, count);
}

static int sock_dispatch_ioctl_vmeth(void *obj, unsigned int request,
				     va_list args)
{
	int fd;
	const struct fd_op_vtable *vtable;
	void *new_obj;

	if (request == ZFD_IOCTL_SET_LOCK) {
		/* Ignore set lock, used by FD logic. */
		return 0;
	}

	fd = sock_dispatch_default(obj);
	if (fd < 0) {
		return -1;
	}

	new_obj = z_get_fd_obj_and_vtable(fd, &vtable, NULL);
	if (new_obj == NULL) {
		return -1;
	}

	return vtable->ioctl(new_obj, request, args);
}

static int sock_dispatch_shutdown_vmeth(void *obj, int how)
{
	int fd = sock_dispatch_default(obj);

	if (fd < 0) {
		return -1;
	}

	return zsock_shutdown(fd, how);
}

static int sock_dispatch_bind_vmeth(void *obj, const struct sockaddr *addr,
				    socklen_t addrlen)
{
	int fd = sock_dispatch_default(obj);

	if (fd < 0) {
		return -1;
	}

	return zsock_bind(fd, addr, addrlen);
}

static int sock_dispatch_connect_vmeth(void *obj, const struct sockaddr *addr,
				       socklen_t addrlen)
{
	int fd = sock_dispatch_default(obj);

	if (fd < 0) {
		return -1;
	}

	return zsock_connect(fd, addr, addrlen);
}

static int sock_dispatch_listen_vmeth(void *obj, int backlog)
{
	int fd = sock_dispatch_default(obj);

	if (fd < 0) {
		return -1;
	}

	return zsock_listen(fd, backlog);
}

static int sock_dispatch_accept_vmeth(void *obj, struct sockaddr *addr,
				      socklen_t *addrlen)
{
	int fd = sock_dispatch_default(obj);

	if (fd < 0) {
		return -1;
	}

	return zsock_accept(fd, addr, addrlen);
}

static ssize_t sock_dispatch_sendto_vmeth(void *obj, const void *buf,
					  size_t len, int flags,
					  const struct sockaddr *addr,
					  socklen_t addrlen)
{
	int fd = sock_dispatch_default(obj);

	if (fd < 0) {
		return -1;
	}

	return zsock_sendto(fd, buf, len, flags, addr, addrlen);
}

static ssize_t sock_dispatch_sendmsg_vmeth(void *obj, const struct msghdr *msg,
					   int flags)
{
	int fd = sock_dispatch_default(obj);

	if (fd < 0) {
		return -1;
	}

	return zsock_sendmsg(fd, msg, flags);
}

static ssize_t sock_dispatch_recvfrom_vmeth(void *obj, void *buf,
					    size_t max_len, int flags,
					    struct sockaddr *addr,
					    socklen_t *addrlen)
{
	int fd = sock_dispatch_default(obj);

	if (fd < 0) {
		return -1;
	}

	return zsock_recvfrom(fd, buf, max_len, flags, addr, addrlen);
}

static int sock_dispatch_getsockopt_vmeth(void *obj, int level, int optname,
					  void *optval, socklen_t *optlen)
{
	int fd = sock_dispatch_default(obj);

	if (fd < 0) {
		return -1;
	}

	return zsock_getsockopt(fd, level, optname, optval, optlen);
}

static int sock_dispatch_setsockopt_vmeth(void *obj, int level, int optname,
					  const void *optval, socklen_t optlen)
{
	int fd;

	if ((level == SOL_SOCKET) && (optname == SO_BINDTODEVICE)) {
		struct net_if *iface;
		const struct device *dev;
		const struct ifreq *ifreq = optval;

		if ((ifreq == NULL) || (optlen != sizeof(*ifreq))) {
			errno = EINVAL;
			return -1;
		}

		dev = device_get_binding(ifreq->ifr_name);
		if (dev == NULL) {
			errno = ENODEV;
			return -1;
		}

		iface = net_if_lookup_by_dev(dev);
		if (iface == NULL) {
			errno = ENODEV;
			return -1;
		}

		if (net_if_socket_offload(iface) != NULL) {
			/* Offloaded socket interface - use associated socket implementation. */
			fd = sock_dispatch_socket(obj, net_if_socket_offload(iface));
		} else {
			/* Native interface - use native socket implementation. */
			fd = sock_dispatch_native(obj);
		}
	} else if ((level == SOL_TLS) && (optname == TLS_NATIVE)) {
		const int *tls_native = optval;
		struct dispatcher_context *ctx = obj;

		if ((tls_native == NULL) || (optlen != sizeof(int))) {
			errno = EINVAL;
			return -1;
		}

		if (!is_tls(ctx->proto)) {
			errno = ENOPROTOOPT;
			return -1;
		}

		if (*tls_native) {
			fd = sock_dispatch_native(obj);
		} else {
			/* No action needed */
			return 0;
		}
	} else {
		fd = sock_dispatch_default(obj);
	}

	if (fd < 0) {
		return -1;
	}

	return zsock_setsockopt(fd, level, optname, optval, optlen);
}

static int sock_dispatch_close_vmeth(void *obj)
{
	dispatcher_ctx_free(obj);

	return 0;
}

static int sock_dispatch_getpeername_vmeth(void *obj, struct sockaddr *addr,
					   socklen_t *addrlen)
{
	int fd = sock_dispatch_default(obj);

	if (fd < 0) {
		return -1;
	}

	return zsock_getpeername(fd, addr, addrlen);
}

static int sock_dispatch_getsockname_vmeth(void *obj, struct sockaddr *addr,
					   socklen_t *addrlen)
{
	int fd = sock_dispatch_default(obj);

	if (fd < 0) {
		return -1;
	}

	return zsock_getsockname(fd, addr, addrlen);
}

static const struct socket_op_vtable sock_dispatch_fd_op_vtable = {
	.fd_vtable = {
		.read = sock_dispatch_read_vmeth,
		.write = sock_dispatch_write_vmeth,
		.close = sock_dispatch_close_vmeth,
		.ioctl = sock_dispatch_ioctl_vmeth,
	},
	.shutdown = sock_dispatch_shutdown_vmeth,
	.bind = sock_dispatch_bind_vmeth,
	.connect = sock_dispatch_connect_vmeth,
	.listen = sock_dispatch_listen_vmeth,
	.accept = sock_dispatch_accept_vmeth,
	.sendto = sock_dispatch_sendto_vmeth,
	.sendmsg = sock_dispatch_sendmsg_vmeth,
	.recvfrom = sock_dispatch_recvfrom_vmeth,
	.getsockopt = sock_dispatch_getsockopt_vmeth,
	.setsockopt = sock_dispatch_setsockopt_vmeth,
	.getpeername = sock_dispatch_getpeername_vmeth,
	.getsockname = sock_dispatch_getsockname_vmeth,
};

static int sock_dispatch_create(int family, int type, int proto)
{
	struct dispatcher_context *entry = NULL;
	int fd = -1;

	(void)k_mutex_lock(&dispatcher_lock, K_FOREVER);

	for (int i = 0; i < ARRAY_SIZE(dispatcher_context); i++) {
		if (dispatcher_context[i].is_used) {
			continue;
		}

		entry = &dispatcher_context[i];
		break;
	}

	if (entry == NULL) {
		errno = ENOMEM;
		goto out;
	}

	if (sock_dispatch_find(family, type, proto, false) == NULL) {
		errno = EAFNOSUPPORT;
		goto out;
	}

	fd = z_reserve_fd();
	if (fd < 0) {
		goto out;
	}

	entry->fd = fd;
	entry->family = family;
	entry->type = type;
	entry->proto = proto;
	entry->is_used = true;

	z_finalize_fd(fd, entry,
		      (const struct fd_op_vtable *)&sock_dispatch_fd_op_vtable);

out:
	k_mutex_unlock(&dispatcher_lock);
	return fd;
}

static bool is_supported(int family, int type, int proto)
{
	return true;
}

NET_SOCKET_REGISTER(sock_dispatch, 0, AF_UNSPEC, is_supported,
		    sock_dispatch_create);
