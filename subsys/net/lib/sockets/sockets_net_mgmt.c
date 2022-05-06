/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <fcntl.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_sock_mgmt, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/net/socket.h>
#include <zephyr/syscall_handler.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/net/socket_net_mgmt.h>
#include <zephyr/net/ethernet_mgmt.h>

#include "sockets_internal.h"
#include "net_private.h"

#define MSG_ALLOC_TIMEOUT K_MSEC(100)

__net_socket struct net_mgmt_socket {
	/* Network interface related to this socket */
	struct net_if *iface;

	/* A way to separate different sockets that listen same events */
	uintptr_t pid;

	/* net_mgmt mask */
	uint32_t mask;

	/* Message allocation timeout */
	k_timeout_t alloc_timeout;

	/* net_mgmt event timeout */
	k_timeout_t wait_timeout;

	/* Socket protocol */
	int proto;

	/* Is this entry in use (true) or not (false) */
	uint8_t is_in_use : 1;
};

static struct net_mgmt_socket
		mgmt_sockets[CONFIG_NET_SOCKETS_NET_MGMT_MAX_LISTENERS];

static const struct socket_op_vtable net_mgmt_sock_fd_op_vtable;

int znet_mgmt_socket(int family, int type, int proto)
{
	struct net_mgmt_socket *mgmt = NULL;
	int fd, i;

	for (i = 0; i < ARRAY_SIZE(mgmt_sockets); i++) {
		if (mgmt_sockets[i].is_in_use) {
			continue;
		}

		mgmt = &mgmt_sockets[i];
	}

	if (mgmt == NULL) {
		errno = ENOENT;
		return -1;
	}

	fd = z_reserve_fd();
	if (fd < 0) {
		errno = ENOSPC;
		return -1;
	}

	mgmt->is_in_use = true;
	mgmt->proto = proto;
	mgmt->alloc_timeout = MSG_ALLOC_TIMEOUT;
	mgmt->wait_timeout = K_FOREVER;

	z_finalize_fd(fd, mgmt,
		     (const struct fd_op_vtable *)&net_mgmt_sock_fd_op_vtable);

	return fd;
}

static int znet_mgmt_bind(struct net_mgmt_socket *mgmt,
			  const struct sockaddr *addr,
			  socklen_t addrlen)
{
	struct sockaddr_nm *nm_addr = (struct sockaddr_nm *)addr;

	if (addrlen != sizeof(struct sockaddr_nm)) {
		return -EINVAL;
	}

	if (nm_addr->nm_ifindex) {
		mgmt->iface = net_if_get_by_index(nm_addr->nm_ifindex);
		if (!mgmt->iface) {
			errno = ENOENT;
			return -1;
		}
	} else {
		mgmt->iface = NULL;
	}

	mgmt->pid = nm_addr->nm_pid;

	if (mgmt->proto == NET_MGMT_EVENT_PROTO) {
		mgmt->mask = nm_addr->nm_mask;

		if (mgmt->iface) {
			mgmt->mask |= NET_MGMT_IFACE_BIT;
		}
	}

	return 0;
}

ssize_t znet_mgmt_sendto(struct net_mgmt_socket *mgmt,
			 const void *buf, size_t len,
			 int flags, const struct sockaddr *dest_addr,
			 socklen_t addrlen)
{
	if (mgmt->proto == NET_MGMT_EVENT_PROTO) {
		/* For net_mgmt events, we only listen and never send */
		errno = ENOTSUP;
		return -1;
	}

	/* Add handling of other network management operations here when
	 * needed.
	 */

	errno = EINVAL;
	return -1;
}

static ssize_t znet_mgmt_recvfrom(struct net_mgmt_socket *mgmt, void *buf,
				  size_t max_len, int flags,
				  struct sockaddr *src_addr,
				  socklen_t *addrlen)
{
	struct sockaddr_nm *nm_addr = (struct sockaddr_nm *)src_addr;
	k_timeout_t timeout = mgmt->wait_timeout;
	uint32_t raised_event = 0;
	uint8_t *copy_to = buf;
	struct net_mgmt_msghdr hdr;
	struct net_if *iface;
	const uint8_t *info;
	size_t info_len;
	int ret;

	if (flags & ZSOCK_MSG_DONTWAIT) {
		timeout = K_NO_WAIT;
	}

again:
	if (mgmt->iface == NULL) {
		ret = net_mgmt_event_wait(mgmt->mask, &raised_event,
					  &iface, (const void **)&info,
					  &info_len, timeout);
	} else {
		ret = net_mgmt_event_wait_on_iface(mgmt->iface,
						   mgmt->mask,
						   &raised_event,
						   (const void **)&info,
						   &info_len,
						   timeout);
		iface = mgmt->iface;
	}

	if (ret == -ETIMEDOUT) {
		errno = EAGAIN;
		return -1;
	}

	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	if ((mgmt->mask & raised_event) != raised_event) {
		if (K_TIMEOUT_EQ(timeout, K_FOREVER)) {
			goto again;
		}

		errno = EAGAIN;
		return -1;
	}

	if (nm_addr) {
		if (iface) {
			nm_addr->nm_ifindex = net_if_get_by_iface(iface);
		} else {
			nm_addr->nm_ifindex = 0;
		}

		nm_addr->nm_pid = mgmt->pid;
		nm_addr->nm_family = AF_NET_MGMT;
		nm_addr->nm_mask = raised_event;
	}

	if (info) {
		ret = info_len + sizeof(hdr);
		ret = MIN(max_len, ret);
		memcpy(&copy_to[sizeof(hdr)], info, ret);
	} else {
		ret = 0;
	}

	hdr.nm_msg_version = NET_MGMT_SOCKET_VERSION_1;
	hdr.nm_msg_len = ret;

	memcpy(copy_to, &hdr, sizeof(hdr));

	return ret;
}

static int znet_mgmt_getsockopt(struct net_mgmt_socket *mgmt, int level,
				int optname, void *optval, socklen_t *optlen)
{
	if (level != SOL_NET_MGMT_RAW || !optval || !optlen) {
		errno = EINVAL;
		return -1;
	}

	if (mgmt->iface == NULL) {
		errno = ENOENT;
		return -1;
	}

	if (IS_ENABLED(CONFIG_NET_L2_ETHERNET_MGMT)) {
		if (optname == NET_REQUEST_ETHERNET_GET_QAV_PARAM) {
			int ret;

			ret = net_mgmt(NET_REQUEST_ETHERNET_GET_QAV_PARAM,
				       mgmt->iface, (void *)optval, *optlen);
			if (ret < 0) {
				errno = -ret;
				return -1;
			}

			return 0;

		} else {
			errno = EINVAL;
			return -1;
		}
	}

	errno = ENOTSUP;
	return -1;
}

static int znet_mgmt_setsockopt(struct net_mgmt_socket *mgmt, int level,
				int optname, const void *optval,
				socklen_t optlen)
{
	if (level != SOL_NET_MGMT_RAW || !optval || !optlen) {
		errno = EINVAL;
		return -1;
	}

	if (mgmt->iface == NULL) {
		errno = ENOENT;
		return -1;
	}

	if (IS_ENABLED(CONFIG_NET_L2_ETHERNET_MGMT)) {
		if (optname == NET_REQUEST_ETHERNET_SET_QAV_PARAM) {
			int ret;

			ret = net_mgmt(NET_REQUEST_ETHERNET_SET_QAV_PARAM,
				       mgmt->iface, (void *)optval, optlen);
			if (ret < 0) {
				errno = -ret;
				return -1;
			}

			return 0;
		} else {
			errno = EINVAL;
			return -1;
		}
	}

	errno = ENOTSUP;
	return -1;
}

static ssize_t net_mgmt_sock_read(void *obj, void *buffer, size_t count)
{
	return znet_mgmt_recvfrom(obj, buffer, count, 0, NULL, 0);
}

static ssize_t net_mgmt_sock_write(void *obj, const void *buffer,
				   size_t count)
{
	return znet_mgmt_sendto(obj, buffer, count, 0, NULL, 0);
}

static int net_mgmt_sock_ioctl(void *obj, unsigned int request,
			       va_list args)
{
	return 0;
}

static int net_mgmt_sock_bind(void *obj, const struct sockaddr *addr,
			      socklen_t addrlen)
{
	return znet_mgmt_bind(obj, addr, addrlen);
}

/* The connect() function is not needed */
static int net_mgmt_sock_connect(void *obj, const struct sockaddr *addr,
				 socklen_t addrlen)
{
	return 0;
}

/*
 * The listen() and accept() functions are without any functionality.
 */
static int net_mgmt_sock_listen(void *obj, int backlog)
{
	return 0;
}

static int net_mgmt_sock_accept(void *obj, struct sockaddr *addr,
				socklen_t *addrlen)
{
	return 0;
}

static ssize_t net_mgmt_sock_sendto(void *obj, const void *buf,
				    size_t len, int flags,
				    const struct sockaddr *dest_addr,
				    socklen_t addrlen)
{
	return znet_mgmt_sendto(obj, buf, len, flags, dest_addr, addrlen);
}

static ssize_t net_mgmt_sock_recvfrom(void *obj, void *buf,
				      size_t max_len, int flags,
				      struct sockaddr *src_addr,
				      socklen_t *addrlen)
{
	return znet_mgmt_recvfrom(obj, buf, max_len, flags,
				  src_addr, addrlen);
}

static int net_mgmt_sock_getsockopt(void *obj, int level, int optname,
				    void *optval, socklen_t *optlen)
{
	return znet_mgmt_getsockopt(obj, level, optname, optval, optlen);
}

static int net_mgmt_sock_setsockopt(void *obj, int level, int optname,
				    const void *optval, socklen_t optlen)
{
	return znet_mgmt_setsockopt(obj, level, optname, optval, optlen);
}

static const struct socket_op_vtable net_mgmt_sock_fd_op_vtable = {
	.fd_vtable = {
		.read = net_mgmt_sock_read,
		.write = net_mgmt_sock_write,
		.ioctl = net_mgmt_sock_ioctl,
	},
	.bind = net_mgmt_sock_bind,
	.connect = net_mgmt_sock_connect,
	.listen = net_mgmt_sock_listen,
	.accept = net_mgmt_sock_accept,
	.sendto = net_mgmt_sock_sendto,
	.recvfrom = net_mgmt_sock_recvfrom,
	.getsockopt = net_mgmt_sock_getsockopt,
	.setsockopt = net_mgmt_sock_setsockopt,
};

static bool net_mgmt_is_supported(int family, int type, int proto)
{
	if ((type != SOCK_RAW && type != SOCK_DGRAM) ||
	    (proto != NET_MGMT_EVENT_PROTO)) {
		return false;
	}

	return true;
}

NET_SOCKET_REGISTER(af_net_mgmt, NET_SOCKET_DEFAULT_PRIO, AF_NET_MGMT,
		    net_mgmt_is_supported, znet_mgmt_socket);
