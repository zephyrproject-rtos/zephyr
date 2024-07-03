/**
 * Copyright (c) 2023-2024 Marcin Niestroj
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * Linux (bottom) side of NSOS (Native Simulator Offloaded Sockets).
 */

#define _DEFAULT_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "nsos.h"
#include "nsos_errno.h"
#include "nsos_fcntl.h"
#include "nsos_netdb.h"
#include "nsos_socket.h"

#include "board_soc.h"
#include "irq_ctrl.h"
#include "nsi_hws_models_if.h"
#include "nsi_tasks.h"
#include "nsi_tracing.h"

#include <stdio.h>

static int nsos_epoll_fd;
static int nsos_adapt_nfds;

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
#endif

#ifndef CONTAINER_OF
#define CONTAINER_OF(ptr, type, field)                               \
		((type *)(((char *)(ptr)) - offsetof(type, field)))
#endif

int nsos_adapt_get_errno(void)
{
	return errno_to_nsos_mid(errno);
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
	default:
		nsi_print_warning("%s: socket family %d not supported\n", __func__, family_mid);
		return -NSOS_MID_EAFNOSUPPORT;
	}

	return 0;
}

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
		nsi_print_warning("%s: socket family %d not supported\n", __func__, family);
		return -NSOS_MID_EAFNOSUPPORT;
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
	default:
		nsi_print_warning("%s: socket protocol %d not supported\n", __func__, proto_mid);
		return -NSOS_MID_EPROTONOSUPPORT;
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
		nsi_print_warning("%s: socket protocol %d not supported\n", __func__, proto);
		return -NSOS_MID_EPROTONOSUPPORT;
	}

	return 0;
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
		nsi_print_warning("%s: socket type %d not supported\n", __func__, type_mid);
		return -NSOS_MID_ESOCKTNOSUPPORT;
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
		nsi_print_warning("%s: socket type %d not supported\n", __func__, type);
		return -NSOS_MID_ESOCKTNOSUPPORT;
	}

	return 0;
}

static int socket_flags_from_nsos_mid(int flags_mid)
{
	int flags = 0;

	nsos_socket_flag_convert(&flags_mid, NSOS_MID_MSG_PEEK,
				 &flags, MSG_PEEK);
	nsos_socket_flag_convert(&flags_mid, NSOS_MID_MSG_TRUNC,
				 &flags, MSG_TRUNC);
	nsos_socket_flag_convert(&flags_mid, NSOS_MID_MSG_DONTWAIT,
				 &flags, MSG_DONTWAIT);
	nsos_socket_flag_convert(&flags_mid, NSOS_MID_MSG_WAITALL,
				 &flags, MSG_WAITALL);

	if (flags_mid != 0) {
		return -NSOS_MID_EINVAL;
	}

	return flags;
}

int nsos_adapt_socket(int family_mid, int type_mid, int proto_mid)
{
	int family;
	int type;
	int proto;
	int ret;

	ret = socket_family_from_nsos_mid(family_mid, &family);
	if (ret < 0) {
		return ret;
	}

	ret = socket_type_from_nsos_mid(type_mid, &type);
	if (ret < 0) {
		return ret;
	}

	ret = socket_proto_from_nsos_mid(proto_mid, &proto);
	if (ret < 0) {
		return ret;
	}

	ret = socket(family, type, proto);
	if (ret < 0) {
		return -errno_to_nsos_mid(errno);
	}

	return ret;
}

static int sockaddr_from_nsos_mid(struct sockaddr **addr, socklen_t *addrlen,
				  const struct nsos_mid_sockaddr *addr_mid, size_t addrlen_mid)
{
	if (!addr_mid || addrlen_mid == 0) {
		*addr = NULL;
		*addrlen = 0;

		return 0;
	}

	switch (addr_mid->sa_family) {
	case NSOS_MID_AF_INET: {
		const struct nsos_mid_sockaddr_in *addr_in_mid =
			(const struct nsos_mid_sockaddr_in *)addr_mid;
		struct sockaddr_in *addr_in = (struct sockaddr_in *)*addr;

		addr_in->sin_family = AF_INET;
		addr_in->sin_port = addr_in_mid->sin_port;
		addr_in->sin_addr.s_addr = addr_in_mid->sin_addr;

		*addrlen = sizeof(*addr_in);

		return 0;
	}
	case NSOS_MID_AF_INET6: {
		const struct nsos_mid_sockaddr_in6 *addr_in_mid =
			(const struct nsos_mid_sockaddr_in6 *)addr_mid;
		struct sockaddr_in6 *addr_in = (struct sockaddr_in6 *)*addr;

		addr_in->sin6_family = AF_INET6;
		addr_in->sin6_port = addr_in_mid->sin6_port;
		addr_in->sin6_flowinfo = 0;
		memcpy(addr_in->sin6_addr.s6_addr, addr_in_mid->sin6_addr,
		       sizeof(addr_in->sin6_addr.s6_addr));
		addr_in->sin6_scope_id = addr_in_mid->sin6_scope_id;

		*addrlen = sizeof(*addr_in);

		return 0;
	}
	}

	return -NSOS_MID_EINVAL;
}

static int sockaddr_to_nsos_mid(const struct sockaddr *addr, socklen_t addrlen,
				struct nsos_mid_sockaddr *addr_mid, size_t *addrlen_mid)
{
	if (!addr || addrlen == 0) {
		*addrlen_mid = 0;

		return 0;
	}

	switch (addr->sa_family) {
	case AF_INET: {
		struct nsos_mid_sockaddr_in *addr_in_mid =
			(struct nsos_mid_sockaddr_in *)addr_mid;
		const struct sockaddr_in *addr_in = (const struct sockaddr_in *)addr;

		if (addr_in_mid) {
			addr_in_mid->sin_family = NSOS_MID_AF_INET;
			addr_in_mid->sin_port = addr_in->sin_port;
			addr_in_mid->sin_addr = addr_in->sin_addr.s_addr;
		}

		if (addrlen_mid) {
			*addrlen_mid = sizeof(*addr_in);
		}

		return 0;
	}
	case AF_INET6: {
		struct nsos_mid_sockaddr_in6 *addr_in_mid =
			(struct nsos_mid_sockaddr_in6 *)addr_mid;
		const struct sockaddr_in6 *addr_in = (const struct sockaddr_in6 *)addr;

		if (addr_in_mid) {
			addr_in_mid->sin6_family = NSOS_MID_AF_INET6;
			addr_in_mid->sin6_port = addr_in->sin6_port;
			memcpy(addr_in_mid->sin6_addr, addr_in->sin6_addr.s6_addr,
			       sizeof(addr_in_mid->sin6_addr));
			addr_in_mid->sin6_scope_id = addr_in->sin6_scope_id;
		}

		if (addrlen_mid) {
			*addrlen_mid = sizeof(*addr_in);
		}

		return 0;
	}
	}

	nsi_print_warning("%s: socket family %d not supported\n", __func__, addr->sa_family);

	return -NSOS_MID_EINVAL;
}

int nsos_adapt_bind(int fd, const struct nsos_mid_sockaddr *addr_mid, size_t addrlen_mid)
{
	struct sockaddr_storage addr_storage;
	struct sockaddr *addr = (struct sockaddr *)&addr_storage;
	socklen_t addrlen;
	int ret;

	ret = sockaddr_from_nsos_mid(&addr, &addrlen, addr_mid, addrlen_mid);
	if (ret < 0) {
		return ret;
	}

	ret = bind(fd, addr, addrlen);
	if (ret < 0) {
		return -errno_to_nsos_mid(errno);
	}

	return ret;
}

int nsos_adapt_connect(int fd, const struct nsos_mid_sockaddr *addr_mid, size_t addrlen_mid)
{
	struct sockaddr_storage addr_storage;
	struct sockaddr *addr = (struct sockaddr *)&addr_storage;
	socklen_t addrlen;
	int ret;

	ret = sockaddr_from_nsos_mid(&addr, &addrlen, addr_mid, addrlen_mid);
	if (ret < 0) {
		return ret;
	}

	ret = connect(fd, addr, addrlen);
	if (ret < 0) {
		return -errno_to_nsos_mid(errno);
	}

	return ret;
}

int nsos_adapt_listen(int fd, int backlog)
{
	int ret;

	ret = listen(fd, backlog);
	if (ret < 0) {
		return -errno_to_nsos_mid(errno);
	}

	return ret;
}

int nsos_adapt_accept(int fd, struct nsos_mid_sockaddr *addr_mid, size_t *addrlen_mid)
{
	struct sockaddr_storage addr_storage;
	struct sockaddr *addr = (struct sockaddr *)&addr_storage;
	socklen_t addrlen = sizeof(addr_storage);
	int ret;
	int err;

	ret = accept(fd, addr, &addrlen);
	if (ret < 0) {
		return -errno_to_nsos_mid(errno);
	}

	err = sockaddr_to_nsos_mid(addr, addrlen, addr_mid, addrlen_mid);
	if (err) {
		close(ret);
		return err;
	}

	return ret;
}

int nsos_adapt_sendto(int fd, const void *buf, size_t len, int flags,
		      const struct nsos_mid_sockaddr *addr_mid, size_t addrlen_mid)
{
	struct sockaddr_storage addr_storage;
	struct sockaddr *addr = (struct sockaddr *)&addr_storage;
	socklen_t addrlen;
	int ret;

	ret = sockaddr_from_nsos_mid(&addr, &addrlen, addr_mid, addrlen_mid);
	if (ret < 0) {
		return ret;
	}

	ret = sendto(fd, buf, len,
		     socket_flags_from_nsos_mid(flags) | MSG_NOSIGNAL,
		     addr, addrlen);
	if (ret < 0) {
		return -errno_to_nsos_mid(errno);
	}

	return ret;
}

int nsos_adapt_sendmsg(int fd, const struct nsos_mid_msghdr *msg_mid, int flags)
{
	struct sockaddr_storage addr_storage;
	struct sockaddr *addr = (struct sockaddr *)&addr_storage;
	struct msghdr msg;
	struct iovec *msg_iov;
	socklen_t addrlen;
	int ret;

	ret = sockaddr_from_nsos_mid(&addr, &addrlen, msg_mid->msg_name, msg_mid->msg_namelen);
	if (ret < 0) {
		return ret;
	}

	msg_iov = calloc(msg_mid->msg_iovlen, sizeof(*msg_iov));
	if (!msg_iov) {
		ret = -ENOMEM;
		return ret;
	}

	for (size_t i = 0; i < msg_mid->msg_iovlen; i++) {
		msg_iov[i].iov_base = msg_mid->msg_iov[i].iov_base;
		msg_iov[i].iov_len = msg_mid->msg_iov[i].iov_len;
	}

	msg.msg_name = addr;
	msg.msg_namelen = addrlen;
	msg.msg_iov = msg_iov;
	msg.msg_iovlen = msg_mid->msg_iovlen;
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	msg.msg_flags = 0;

	ret = sendmsg(fd, &msg, socket_flags_from_nsos_mid(flags) | MSG_NOSIGNAL);
	if (ret < 0) {
		ret = -errno_to_nsos_mid(errno);
	}

	free(msg_iov);

	return ret;
}

int nsos_adapt_recvfrom(int fd, void *buf, size_t len, int flags,
			struct nsos_mid_sockaddr *addr_mid, size_t *addrlen_mid)
{
	struct sockaddr_storage addr_storage;
	struct sockaddr *addr = (struct sockaddr *)&addr_storage;
	socklen_t addrlen = sizeof(addr_storage);
	int ret;
	int err;

	ret = recvfrom(fd, buf, len, socket_flags_from_nsos_mid(flags),
		       addr, &addrlen);
	if (ret < 0) {
		return -errno_to_nsos_mid(errno);
	}

	err = sockaddr_to_nsos_mid(addr, addrlen, addr_mid, addrlen_mid);
	if (err) {
		return err;
	}

	return ret;
}

static int nsos_adapt_getsockopt_int(int fd, int level, int optname,
				     void *optval, size_t *nsos_mid_optlen)
{
	socklen_t optlen = *nsos_mid_optlen;
	int ret;

	ret = getsockopt(fd, level, optname, optval, &optlen);
	if (ret < 0) {
		return -errno_to_nsos_mid(errno);
	}

	*nsos_mid_optlen = optlen;

	return 0;
}

int nsos_adapt_getsockopt(int fd, int nsos_mid_level, int nsos_mid_optname,
			  void *nsos_mid_optval, size_t *nsos_mid_optlen)
{
	switch (nsos_mid_level) {
	case NSOS_MID_SOL_SOCKET:
		switch (nsos_mid_optname) {
		case NSOS_MID_SO_ERROR: {
			int err;
			socklen_t optlen = sizeof(err);
			int ret;

			ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &optlen);
			if (ret < 0) {
				return -errno_to_nsos_mid(errno);
			}

			*(int *)nsos_mid_optval = errno_to_nsos_mid(err);

			return 0;
		}
		case NSOS_MID_SO_TYPE: {
			int type;
			socklen_t optlen = sizeof(type);
			int ret;
			int err;

			ret = getsockopt(fd, SOL_SOCKET, SO_TYPE, &type, &optlen);
			if (ret < 0) {
				return -errno_to_nsos_mid(errno);
			}

			err = socket_type_to_nsos_mid(type, nsos_mid_optval);
			if (err) {
				return err;
			}

			return 0;
		}
		case NSOS_MID_SO_PROTOCOL: {
			int proto;
			socklen_t optlen = sizeof(proto);
			int ret;
			int err;

			ret = getsockopt(fd, SOL_SOCKET, SO_PROTOCOL, &proto, &optlen);
			if (ret < 0) {
				return -errno_to_nsos_mid(errno);
			}

			err = socket_proto_to_nsos_mid(proto, nsos_mid_optval);
			if (err) {
				return err;
			}

			return 0;
		}
		case NSOS_MID_SO_DOMAIN: {
			int family;
			socklen_t optlen = sizeof(family);
			int ret;
			int err;

			ret = getsockopt(fd, SOL_SOCKET, SO_DOMAIN, &family, &optlen);
			if (ret < 0) {
				return -errno_to_nsos_mid(errno);
			}

			err = socket_family_to_nsos_mid(family, nsos_mid_optval);
			if (err) {
				return err;
			}

			return 0;
		}
		case NSOS_MID_SO_RCVBUF:
			return nsos_adapt_getsockopt_int(fd, SOL_SOCKET, SO_RCVBUF,
							 nsos_mid_optval, nsos_mid_optlen);
		case NSOS_MID_SO_SNDBUF:
			return nsos_adapt_getsockopt_int(fd, SOL_SOCKET, SO_SNDBUF,
							 nsos_mid_optval, nsos_mid_optlen);
		case NSOS_MID_SO_REUSEADDR:
			return nsos_adapt_getsockopt_int(fd, SOL_SOCKET, SO_REUSEADDR,
							 nsos_mid_optval, nsos_mid_optlen);
		case NSOS_MID_SO_REUSEPORT:
			return nsos_adapt_getsockopt_int(fd, SOL_SOCKET, SO_REUSEPORT,
							 nsos_mid_optval, nsos_mid_optlen);
		case NSOS_MID_SO_LINGER:
			return nsos_adapt_getsockopt_int(fd, SOL_SOCKET, SO_LINGER,
							 nsos_mid_optval, nsos_mid_optlen);
		case NSOS_MID_SO_KEEPALIVE:
			return nsos_adapt_getsockopt_int(fd, SOL_SOCKET, SO_KEEPALIVE,
							 nsos_mid_optval, nsos_mid_optlen);
		}
		break;

	case NSOS_MID_IPPROTO_TCP:
		switch (nsos_mid_optname) {
		case NSOS_MID_TCP_NODELAY:
			return nsos_adapt_getsockopt_int(fd, IPPROTO_TCP, TCP_NODELAY,
							 nsos_mid_optval, nsos_mid_optlen);
		case NSOS_MID_TCP_KEEPIDLE:
			return nsos_adapt_getsockopt_int(fd, IPPROTO_TCP, TCP_KEEPIDLE,
							 nsos_mid_optval, nsos_mid_optlen);
		case NSOS_MID_TCP_KEEPINTVL:
			return nsos_adapt_getsockopt_int(fd, IPPROTO_TCP, TCP_KEEPINTVL,
							 nsos_mid_optval, nsos_mid_optlen);
		case NSOS_MID_TCP_KEEPCNT:
			return nsos_adapt_getsockopt_int(fd, IPPROTO_TCP, TCP_KEEPCNT,
							 nsos_mid_optval, nsos_mid_optlen);
		}
		break;

	case NSOS_MID_IPPROTO_IPV6:
		switch (nsos_mid_optname) {
		case NSOS_MID_IPV6_V6ONLY:
			return nsos_adapt_getsockopt_int(fd, IPPROTO_IPV6, IPV6_V6ONLY,
							 nsos_mid_optval, nsos_mid_optlen);
		}
		break;
	}

	return -NSOS_MID_EOPNOTSUPP;
}

static int nsos_adapt_setsockopt_int(int fd, int level, int optname,
				     const void *optval, size_t optlen)
{
	int ret;

	ret = setsockopt(fd, level, optname, optval, optlen);
	if (ret < 0) {
		return -errno_to_nsos_mid(errno);
	}

	return 0;
}

int nsos_adapt_setsockopt(int fd, int nsos_mid_level, int nsos_mid_optname,
			  const void *nsos_mid_optval, size_t nsos_mid_optlen)
{
	switch (nsos_mid_level) {
	case NSOS_MID_SOL_SOCKET:
		switch (nsos_mid_optname) {
		case NSOS_MID_SO_PRIORITY:
			return nsos_adapt_setsockopt_int(fd, SOL_SOCKET, SO_PRIORITY,
							 nsos_mid_optval, nsos_mid_optlen);

		case NSOS_MID_SO_RCVTIMEO: {
			const struct nsos_mid_timeval *nsos_mid_tv = nsos_mid_optval;
			struct timeval tv = {
				.tv_sec = nsos_mid_tv->tv_sec,
				.tv_usec = nsos_mid_tv->tv_usec,
			};
			int ret;

			ret = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO,
					 &tv, sizeof(tv));
			if (ret < 0) {
				return -errno_to_nsos_mid(errno);
			}

			return 0;
		}
		case NSOS_MID_SO_SNDTIMEO: {
			const struct nsos_mid_timeval *nsos_mid_tv = nsos_mid_optval;
			struct timeval tv = {
				.tv_sec = nsos_mid_tv->tv_sec,
				.tv_usec = nsos_mid_tv->tv_usec,
			};
			int ret;

			ret = setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO,
					 &tv, sizeof(tv));
			if (ret < 0) {
				return -errno_to_nsos_mid(errno);
			}

			return 0;
		}
		case NSOS_MID_SO_RCVBUF:
			return nsos_adapt_setsockopt_int(fd, SOL_SOCKET, SO_RCVBUF,
							 nsos_mid_optval, nsos_mid_optlen);
		case NSOS_MID_SO_SNDBUF:
			return nsos_adapt_setsockopt_int(fd, SOL_SOCKET, SO_SNDBUF,
							 nsos_mid_optval, nsos_mid_optlen);
		case NSOS_MID_SO_REUSEADDR:
			return nsos_adapt_setsockopt_int(fd, SOL_SOCKET, SO_REUSEADDR,
							 nsos_mid_optval, nsos_mid_optlen);
		case NSOS_MID_SO_REUSEPORT:
			return nsos_adapt_setsockopt_int(fd, SOL_SOCKET, SO_REUSEPORT,
							 nsos_mid_optval, nsos_mid_optlen);
		case NSOS_MID_SO_LINGER:
			return nsos_adapt_setsockopt_int(fd, SOL_SOCKET, SO_LINGER,
							 nsos_mid_optval, nsos_mid_optlen);
		case NSOS_MID_SO_KEEPALIVE:
			return nsos_adapt_setsockopt_int(fd, SOL_SOCKET, SO_KEEPALIVE,
							 nsos_mid_optval, nsos_mid_optlen);
		}
		break;

	case NSOS_MID_IPPROTO_TCP:
		switch (nsos_mid_optname) {
		case NSOS_MID_TCP_NODELAY:
			return nsos_adapt_setsockopt_int(fd, IPPROTO_TCP, TCP_NODELAY,
							 nsos_mid_optval, nsos_mid_optlen);
		case NSOS_MID_TCP_KEEPIDLE:
			return nsos_adapt_setsockopt_int(fd, IPPROTO_TCP, TCP_KEEPIDLE,
							 nsos_mid_optval, nsos_mid_optlen);
		case NSOS_MID_TCP_KEEPINTVL:
			return nsos_adapt_setsockopt_int(fd, IPPROTO_TCP, TCP_KEEPINTVL,
							 nsos_mid_optval, nsos_mid_optlen);
		case NSOS_MID_TCP_KEEPCNT:
			return nsos_adapt_setsockopt_int(fd, IPPROTO_TCP, TCP_KEEPCNT,
							 nsos_mid_optval, nsos_mid_optlen);
		}
		break;

	case NSOS_MID_IPPROTO_IPV6:
		switch (nsos_mid_optname) {
		case NSOS_MID_IPV6_V6ONLY:
			return nsos_adapt_setsockopt_int(fd, IPPROTO_IPV6, IPV6_V6ONLY,
							 nsos_mid_optval, nsos_mid_optlen);
		}
		break;
	}

	return -NSOS_MID_EOPNOTSUPP;
}

#define MAP_POLL_EPOLL(_event_from, _event_to)	\
	if (events_from & (_event_from)) {	\
		events_from &= ~(_event_from);	\
		events_to |= _event_to;		\
	}

static int nsos_poll_to_epoll_events(int events_from)
{
	int events_to = 0;

	MAP_POLL_EPOLL(POLLIN, EPOLLIN);
	MAP_POLL_EPOLL(POLLOUT, EPOLLOUT);
	MAP_POLL_EPOLL(POLLERR, EPOLLERR);
	MAP_POLL_EPOLL(POLLHUP, EPOLLHUP);

	return events_to;
}

static int nsos_epoll_to_poll_events(int events_from)
{
	int events_to = 0;

	MAP_POLL_EPOLL(EPOLLIN, POLLIN);
	MAP_POLL_EPOLL(EPOLLOUT, POLLOUT);
	MAP_POLL_EPOLL(EPOLLERR, POLLERR);
	MAP_POLL_EPOLL(EPOLLHUP, POLLHUP);

	return events_to;
}

#undef MAP_POLL_EPOLL

static uint64_t nsos_adapt_poll_time = NSI_NEVER;

void nsos_adapt_poll_add(struct nsos_mid_pollfd *pollfd)
{
	struct epoll_event ev = {
		.data.ptr = pollfd,
		.events = nsos_poll_to_epoll_events(pollfd->events),
	};
	int err;

	nsos_adapt_nfds++;

	err = epoll_ctl(nsos_epoll_fd, EPOLL_CTL_ADD, pollfd->fd, &ev);
	if (err) {
		nsi_print_error_and_exit("error in EPOLL_CTL_ADD: errno=%d\n", errno);
		return;
	}

	nsos_adapt_poll_time = nsi_hws_get_time() + 1;
	nsi_hws_find_next_event();
}

void nsos_adapt_poll_remove(struct nsos_mid_pollfd *pollfd)
{
	int err;

	err = epoll_ctl(nsos_epoll_fd, EPOLL_CTL_DEL, pollfd->fd, NULL);
	if (err) {
		nsi_print_error_and_exit("error in EPOLL_CTL_DEL: errno=%d\n", errno);
		return;
	}

	nsos_adapt_nfds--;
}

void nsos_adapt_poll_update(struct nsos_mid_pollfd *pollfd)
{
	struct pollfd fds = {
		.fd = pollfd->fd,
		.events = pollfd->events,
	};
	int ret;

	ret = poll(&fds, 1, 0);
	if (ret < 0) {
		nsi_print_error_and_exit("error in poll(): errno=%d\n", errno);
		return;
	}

	if (ret > 0) {
		pollfd->revents = fds.revents;
	}
}

struct nsos_addrinfo_wrap {
	struct nsos_mid_addrinfo addrinfo_mid;
	struct nsos_mid_sockaddr_storage addr_storage;
	struct addrinfo *addrinfo;
};

static int addrinfo_to_nsos_mid(struct addrinfo *res,
				struct nsos_mid_addrinfo **mid_res)
{
	struct nsos_addrinfo_wrap *nsos_res_wraps;
	size_t idx_res = 0;
	size_t n_res = 0;
	int ret;

	for (struct addrinfo *res_p = res; res_p; res_p = res_p->ai_next) {
		n_res++;
	}

	if (n_res == 0) {
		return 0;
	}

	nsos_res_wraps = calloc(n_res, sizeof(*nsos_res_wraps));
	if (!nsos_res_wraps) {
		return -NSOS_MID_ENOMEM;
	}

	for (struct addrinfo *res_p = res; res_p; res_p = res_p->ai_next, idx_res++) {
		struct nsos_addrinfo_wrap *wrap = &nsos_res_wraps[idx_res];

		wrap->addrinfo = res_p;

		wrap->addrinfo_mid.ai_flags = res_p->ai_flags;

		ret = socket_family_to_nsos_mid(res_p->ai_family, &wrap->addrinfo_mid.ai_family);
		if (ret < 0) {
			goto free_wraps;
		}

		ret = socket_type_to_nsos_mid(res_p->ai_socktype, &wrap->addrinfo_mid.ai_socktype);
		if (ret < 0) {
			goto free_wraps;
		}

		ret = socket_proto_to_nsos_mid(res_p->ai_protocol, &wrap->addrinfo_mid.ai_protocol);
		if (ret < 0) {
			goto free_wraps;
		}

		wrap->addrinfo_mid.ai_addr =
			(struct nsos_mid_sockaddr *)&wrap->addr_storage;
		wrap->addrinfo_mid.ai_addrlen = sizeof(wrap->addr_storage);

		ret = sockaddr_to_nsos_mid(res_p->ai_addr, res_p->ai_addrlen,
					   wrap->addrinfo_mid.ai_addr,
					   &wrap->addrinfo_mid.ai_addrlen);
		if (ret < 0) {
			goto free_wraps;
		}

		wrap->addrinfo_mid.ai_canonname =
			res_p->ai_canonname ? strdup(res_p->ai_canonname) : NULL;
		wrap->addrinfo_mid.ai_next = &wrap[1].addrinfo_mid;
	}

	nsos_res_wraps[n_res - 1].addrinfo_mid.ai_next = NULL;

	*mid_res = &nsos_res_wraps->addrinfo_mid;

	return 0;

free_wraps:
	for (struct nsos_mid_addrinfo *res_p = &nsos_res_wraps[0].addrinfo_mid;
	     res_p;
	     res_p = res_p->ai_next) {
		free(res_p->ai_canonname);
	}

	free(nsos_res_wraps);

	return ret;
}

int nsos_adapt_getaddrinfo(const char *node, const char *service,
			   const struct nsos_mid_addrinfo *hints_mid,
			   struct nsos_mid_addrinfo **res_mid,
			   int *system_errno)
{
	struct addrinfo hints;
	struct addrinfo *res = NULL;
	int ret;

	if (hints_mid) {
		hints.ai_flags = hints_mid->ai_flags;

		ret = socket_family_from_nsos_mid(hints_mid->ai_family, &hints.ai_family);
		if (ret < 0) {
			*system_errno = -ret;
			return NSOS_MID_EAI_SYSTEM;
		}

		ret = socket_type_from_nsos_mid(hints_mid->ai_socktype, &hints.ai_socktype);
		if (ret < 0) {
			*system_errno = -ret;
			return NSOS_MID_EAI_SYSTEM;
		}

		ret = socket_proto_from_nsos_mid(hints_mid->ai_protocol, &hints.ai_protocol);
		if (ret < 0) {
			*system_errno = -ret;
			return NSOS_MID_EAI_SYSTEM;
		}
	}

	ret = getaddrinfo(node, service,
			  hints_mid ? &hints : NULL,
			  &res);
	if (ret < 0) {
		return ret;
	}

	ret = addrinfo_to_nsos_mid(res, res_mid);
	if (ret < 0) {
		*system_errno = -ret;
		return NSOS_MID_EAI_SYSTEM;
	}

	return ret;
}

void nsos_adapt_freeaddrinfo(struct nsos_mid_addrinfo *res_mid)
{
	struct nsos_addrinfo_wrap *wrap =
		CONTAINER_OF(res_mid, struct nsos_addrinfo_wrap, addrinfo_mid);

	for (struct nsos_mid_addrinfo *res_p = res_mid; res_p; res_p = res_p->ai_next) {
		free(res_p->ai_canonname);
	}

	freeaddrinfo(wrap->addrinfo);
	free(wrap);
}

int nsos_adapt_fcntl_getfl(int fd)
{
	int flags;

	flags = fcntl(fd, F_GETFL);

	return fl_to_nsos_mid(flags);
}

int nsos_adapt_fcntl_setfl(int fd, int flags)
{
	int ret;

	ret = fcntl(fd, F_SETFL, fl_from_nsos_mid(flags));
	if (ret < 0) {
		return -errno_to_nsos_mid(errno);
	}

	return 0;
}

int nsos_adapt_fionread(int fd, int *avail)
{
	int ret;

	ret = ioctl(fd, FIONREAD, avail);
	if (ret < 0) {
		return -errno_to_nsos_mid(errno);
	}

	return 0;
}

int nsos_adapt_dup(int oldfd)
{
	int ret;

	ret = dup(oldfd);
	if (ret < 0) {
		return -errno_to_nsos_mid(errno);
	}

	return ret;
}

static void nsos_adapt_init(void)
{
	nsos_epoll_fd = epoll_create(1);
	if (nsos_epoll_fd < 0) {
		nsi_print_error_and_exit("error from epoll_create(): errno=%d\n", errno);
		return;
	}
}

NSI_TASK(nsos_adapt_init, HW_INIT, 500);

static void nsos_adapt_poll_triggered(void)
{
	static struct epoll_event events[1024];
	int ret;

	if (nsos_adapt_nfds == 0) {
		nsos_adapt_poll_time = NSI_NEVER;
		return;
	}

	ret = epoll_wait(nsos_epoll_fd, events, ARRAY_SIZE(events), 0);
	if (ret < 0) {
		if (errno == EINTR) {
			nsi_print_warning("interrupted epoll_wait()\n");
			nsos_adapt_poll_time = nsi_hws_get_time() + 1;
			return;
		}

		nsi_print_error_and_exit("error in nsos_adapt poll(): errno=%d\n", errno);

		nsos_adapt_poll_time = NSI_NEVER;
		return;
	}

	for (int i = 0; i < ret; i++) {
		struct nsos_mid_pollfd *pollfd = events[i].data.ptr;

		pollfd->revents = nsos_epoll_to_poll_events(events[i].events);
	}

	if (ret > 0) {
		hw_irq_ctrl_set_irq(NSOS_IRQ);
		nsos_adapt_poll_time = nsi_hws_get_time() + 1;
	} else {
		nsos_adapt_poll_time = nsi_hws_get_time() + NSOS_EPOLL_WAIT_INTERVAL;
	}
}

NSI_HW_EVENT(nsos_adapt_poll_time, nsos_adapt_poll_triggered, 500);
