/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fcntl.h>

#include <kernel.h>
#include <net/net_context.h>

/* Zephyr headers */
#include <logging/log.h>
LOG_MODULE_REGISTER(net_sock, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <kernel.h>
#include <net/socket.h>
#include <syscall_handler.h>
#include <sys/fdtable.h>

#include "sockets_internal.h"

const struct socket_op_vtable spair_fd_op_vtable;

int z_impl_zsock_socketpair(int family, int type, int proto, int sv[2]) {
	int res;
	int tmp[2];
	struct net_context *ctx[2];

	if (family != AF_UNIX) {
		errno = EAFNOSUPPORT;
		res = -1;
		goto out;
	}

	if (!(type == SOCK_STREAM || type == SOCK_RAW)) {
		errno = EPROTOTYPE;
		res = -1;
		goto out;
	}

	if (proto != 0) {
		errno = EPROTONOSUPPORT;
		res = -1;
		goto out;
	}

	if (sv == NULL) {
		/* not listed in the normative standard, but probably safe */
		errno = EINVAL;
		res = -1;
		goto out;
	}

	res = z_impl_zsock_socket(family, type, proto);
	if (res == -1) {
		goto out;
	}
	tmp[0] = res;

	res = z_impl_zsock_socket(family, type, proto);
	if (res == -1) {
		goto close_fd_0;
	}
	tmp[1] = res;

	ctx[0] = z_get_fd_obj(tmp[0], (const struct fd_op_vtable *)&spair_fd_op_vtable, 0);
	if (ctx[0] == NULL) {
		goto close_fd_1;
	}

	ctx[1] = z_get_fd_obj(tmp[1], (const struct fd_op_vtable *)&spair_fd_op_vtable, 0);
	if (ctx[1] == NULL) {
		goto close_fd_1;
	}

	/* TODO: create 2 struct spair_obj and connect them */

	sv[0] = tmp[0];
	sv[1] = tmp[1];
	res = 0;

	goto out;

close_fd_1:
	// z_impl_zsock_close(tmp[1]);

close_fd_0:
	// z_impl_zsock_close(tmp[0]);

out:
	return res;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_zsock_socketpair(int family, int type, int proto, int sv[2])
{
	int ret;
	int tmp[2];

	ret = z_impl_zsock_socketpair(family, type, proto, tmp);
	Z_OOPS(z_user_to_copy(sv, tmp, sizeof(tmp));
	return ret;
}
#include <syscalls/zsock_spair_mrsh.c>
#endif /* CONFIG_USERSPACE */


static ssize_t spair_read_vmeth(void *obj, void *buffer, size_t count)
{
	errno = ENOSYS;
	return -1;
}

static ssize_t spair_write_vmeth(void *obj, const void *buffer, size_t count)
{
	errno = ENOSYS;
	return -1;
}

static int spair_ioctl_vmeth(void *obj, unsigned int request, va_list args)
{
	switch (request) {
	case ZFD_IOCTL_CLOSE:
	default:
		errno = EOPNOTSUPP;
		return -1;
	}
}

static int spair_bind_vmeth(void *obj, const struct sockaddr *addr,
			   socklen_t addrlen)
{
	(void) obj;
	(void) addr;
	(void) addrlen;

	errno = EISCONN;
	return -1;
}

static int spair_connect_vmeth(void *obj, const struct sockaddr *addr,
			      socklen_t addrlen)
{
	(void) obj;
	(void) addr;
	(void) addrlen;

	errno = EISCONN;
	return -1;
}

static int spair_listen_vmeth(void *obj, int backlog)
{
	(void) obj;
	(void) backlog;

	errno = EINVAL;
	return -1;
}

static int spair_accept_vmeth(void *obj, struct sockaddr *addr,
			     socklen_t *addrlen)
{
	(void) obj;
	(void) addr;
	(void) addrlen;

	errno = EOPNOTSUPP;
	return -1;
}

static ssize_t spair_sendto_vmeth(void *obj, const void *buf, size_t len,
				 int flags, const struct sockaddr *dest_addr,
				 socklen_t addrlen)
{
	(void) obj;
	(void) buf;
	(void) len;
	(void) flags;
	(void) dest_addr;
	(void) addrlen;

	errno = ENOSYS;
	return -1;
}

static ssize_t spair_sendmsg_vmeth(void *obj, const struct msghdr *msg,
				  int flags)
{
	(void) obj;
	(void) msg;
	(void) flags;

	errno = ENOSYS;
	return -1;
}

static ssize_t spair_recvfrom_vmeth(void *obj, void *buf, size_t max_len,
				   int flags, struct sockaddr *src_addr,
				   socklen_t *addrlen)
{
	(void) obj;
	(void) buf;
	(void) max_len;
	(void) flags;
	(void) src_addr;
	(void) addrlen;

	errno = ENOSYS;
	return -1;
}

static int spair_getsockopt_vmeth(void *obj, int level, int optname,
				 void *optval, socklen_t *optlen)
{
	(void) obj;
	(void) level;
	(void) optname;
	(void) optval;
	(void) optlen;

	errno = ENOSYS;
	return -1;
}

static int spair_setsockopt_vmeth(void *obj, int level, int optname,
				 const void *optval, socklen_t optlen)
{
	(void) obj;
	(void) level;
	(void) optname;
	(void) optval;
	(void) optlen;

	errno = ENOSYS;
	return -1;
}

const struct socket_op_vtable spair_fd_op_vtable = {
	.fd_vtable = {
		.read = spair_read_vmeth,
		.write = spair_write_vmeth,
		.ioctl = spair_ioctl_vmeth,
	},
	.bind = spair_bind_vmeth,
	.connect = spair_connect_vmeth,
	.listen = spair_listen_vmeth,
	.accept = spair_accept_vmeth,
	.sendto = spair_sendto_vmeth,
	.sendmsg = spair_sendmsg_vmeth,
	.recvfrom = spair_recvfrom_vmeth,
	.getsockopt = spair_getsockopt_vmeth,
	.setsockopt = spair_setsockopt_vmeth,
};
