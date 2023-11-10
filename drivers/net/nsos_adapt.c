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

#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "nsos.h"
#include "nsos_errno.h"

#include "nsi_tracing.h"

#include <stdio.h>

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
	default:
		nsi_print_warning("%s: socket family %d not supported\n", __func__, family_mid);
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
