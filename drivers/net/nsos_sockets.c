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

#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/offloaded_netdev.h>
#include <zephyr/net/socket_offload.h>
#include <zephyr/sys/fdtable.h>

#include "sockets_internal.h"
#include "nsos.h"
#include "nsos_errno.h"

#include "nsi_host_trampolines.h"

/* Increment by 1 to make sure we do not store the value of 0, which has
 * a special meaning in the fdtable subsys.
 */
#define FD_TO_OBJ(fd) ((void *)(intptr_t)((fd) + 1))
#define OBJ_TO_FD(obj) (((int)(intptr_t)(obj)) - 1)

static int socket_family_to_nsos_mid(int family, int *family_mid)
{
	switch (family) {
	case AF_UNSPEC:
		*family_mid = NSOS_MID_AF_UNSPEC;
		break;
	case AF_INET:
		*family_mid = NSOS_MID_AF_INET;
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
	int sock;
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

	sock = nsos_adapt_socket(family_mid, type_mid, proto_mid);
	if (sock < 0) {
		errno = errno_from_nsos_mid(-sock);
		goto free_fd;
	}

	z_finalize_fd(fd, FD_TO_OBJ(sock), &nsos_socket_fd_op_vtable.fd_vtable);

	return fd;

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
	int ret;

	ret = nsi_host_read(OBJ_TO_FD(obj), buf, sz);
	if (ret < 0) {
		errno = nsos_adapt_get_zephyr_errno();
	}

	return ret;
}

static ssize_t nsos_write(void *obj, const void *buf, size_t sz)
{
	int ret;

	ret = nsi_host_write(OBJ_TO_FD(obj), buf, sz);
	if (ret < 0) {
		errno = nsos_adapt_get_zephyr_errno();
	}

	return ret;
}

static int nsos_close(void *obj)
{
	int ret;

	ret = nsi_host_close(OBJ_TO_FD(obj));
	if (ret < 0) {
		errno = nsos_adapt_get_zephyr_errno();
	}

	return ret;
}

static int nsos_ioctl(void *obj, unsigned int request, va_list args)
{
	switch (request) {
	case ZFD_IOCTL_POLL_PREPARE:
		return -EXDEV;

	case ZFD_IOCTL_POLL_UPDATE:
		return -EOPNOTSUPP;

	case ZFD_IOCTL_POLL_OFFLOAD:
		return -EOPNOTSUPP;
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
	}

	return -NSOS_MID_EINVAL;
}

static int nsos_bind(void *obj, const struct sockaddr *addr, socklen_t addrlen)
{
	struct nsos_mid_sockaddr_storage addr_storage_mid;
	struct nsos_mid_sockaddr *addr_mid = (struct nsos_mid_sockaddr *)&addr_storage_mid;
	size_t addrlen_mid;
	int ret;

	ret = sockaddr_to_nsos_mid(addr, &addrlen, &addr_mid, &addrlen_mid);
	if (ret < 0) {
		goto return_ret;
	}

	ret = nsos_adapt_bind(OBJ_TO_FD(obj), addr_mid, addrlen_mid);

return_ret:
	if (ret < 0) {
		errno = errno_from_nsos_mid(-ret);
		return -1;
	}

	return ret;
}

static int nsos_connect(void *obj, const struct sockaddr *addr, socklen_t addrlen)
{
	struct nsos_mid_sockaddr_storage addr_storage_mid;
	struct nsos_mid_sockaddr *addr_mid = (struct nsos_mid_sockaddr *)&addr_storage_mid;
	size_t addrlen_mid;
	int ret;

	ret = sockaddr_to_nsos_mid(addr, &addrlen, &addr_mid, &addrlen_mid);
	if (ret < 0) {
		goto return_ret;
	}

	ret = nsos_adapt_connect(OBJ_TO_FD(obj), addr_mid, addrlen_mid);

return_ret:
	if (ret < 0) {
		errno = errno_from_nsos_mid(-ret);
		return -1;
	}

	return ret;
}

static int nsos_listen(void *obj, int backlog)
{
	int ret;

	ret = nsos_adapt_listen(OBJ_TO_FD(obj), backlog);
	if (ret < 0) {
		errno = errno_from_nsos_mid(-ret);
		return -1;
	}

	return ret;
}

static int nsos_accept(void *obj, struct sockaddr *addr, socklen_t *addrlen)
{
	struct nsos_mid_sockaddr_storage addr_storage_mid;
	struct nsos_mid_sockaddr *addr_mid = (struct nsos_mid_sockaddr *)&addr_storage_mid;
	size_t addrlen_mid = sizeof(addr_storage_mid);
	int ret;

	ret = nsos_adapt_accept(OBJ_TO_FD(obj), addr_mid, &addrlen_mid);
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

	z_finalize_fd(zephyr_fd, FD_TO_OBJ(adapt_fd), &nsos_socket_fd_op_vtable.fd_vtable);

	return zephyr_fd;

close_adapt_fd:
	nsi_host_close(adapt_fd);

	return -1;
}

static ssize_t nsos_sendto(void *obj, const void *buf, size_t len, int flags,
			   const struct sockaddr *addr, socklen_t addrlen)
{
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

	ret = nsos_adapt_sendto(OBJ_TO_FD(obj), buf, len, flags_mid,
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

static ssize_t nsos_recvfrom(void *obj, void *buf, size_t len, int flags,
			     struct sockaddr *addr, socklen_t *addrlen)
{
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

	ret = nsos_adapt_recvfrom(OBJ_TO_FD(obj), buf, len, flags_mid,
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

static int nsos_socket_offload_init(const struct device *arg)
{
	ARG_UNUSED(arg);

	return 0;
}

static void nsos_iface_api_init(struct net_if *iface)
{
	iface->if_dev->socket_offload = nsos_socket_create;
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
