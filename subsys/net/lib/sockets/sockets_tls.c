/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <net/net_context.h>
#include <net/socket.h>

int ztls_socket(int family, int type, int proto)
{
	return zsock_socket(family, type, proto);
}

int ztls_close(int sock)
{
	return zsock_close(sock);
}

int ztls_bind(int sock, const struct sockaddr *addr, socklen_t addrlen)
{
	return zsock_bind(sock, addr, addrlen);
}

int ztls_connect(int sock, const struct sockaddr *addr, socklen_t addrlen)
{
	return zsock_connect(sock, addr, addrlen);
}

int ztls_listen(int sock, int backlog)
{
	return zsock_listen(sock, backlog);
}

int ztls_accept(int sock, struct sockaddr *addr, socklen_t *addrlen)
{
	return zsock_accept(sock, addr, addrlen);
}

ssize_t ztls_send(int sock, const void *buf, size_t len, int flags)
{
	return zsock_send(sock, buf, len, flags);
}

ssize_t ztls_recv(int sock, void *buf, size_t max_len, int flags)
{
	return zsock_recv(sock, buf, max_len, flags);
}

ssize_t ztls_sendto(int sock, const void *buf, size_t len, int flags,
		    const struct sockaddr *dest_addr, socklen_t addrlen)
{
	return zsock_sendto(sock, buf, len, flags, dest_addr, addrlen);
}

ssize_t ztls_recvfrom(int sock, void *buf, size_t max_len, int flags,
		      struct sockaddr *src_addr, socklen_t *addrlen)
{
	return zsock_recvfrom(sock, buf, max_len, flags, src_addr, addrlen);
}

int ztls_fcntl(int sock, int cmd, int flags)
{
	return zsock_fcntl(sock, cmd, flags);
}

int ztls_poll(struct zsock_pollfd *fds, int nfds, int timeout)
{
	return zsock_poll(fds, nfds, timeout);
}
