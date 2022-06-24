/*
 * Copyright (c) 2022 YuLong Yao<feilongphone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* For accept4() */
#define _GNU_SOURCE 1

#include "usb_dc_usbip_adapt.h"

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netinet/in.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

int usbipsocket_socket(void)
{
	return socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
}

int usbipsocket_bind(int sock)
{
	struct sockaddr_in srv;

	memset(&srv, 0, sizeof(srv));
	srv.sin_family = AF_INET;
	srv.sin_addr.s_addr = htonl(INADDR_ANY);
	srv.sin_port = htons(CONFIG_USBIP_PORT);

	return bind(sock, &srv, sizeof(srv));
}

int usbipsocket_connect(int sock, const struct sockaddr *addr,
			socklen_t addrlen)
{
	return connect(sock, addr, addrlen);
}

int usbipsocket_listen(int sock) { return listen(sock, SOMAXCONN); }

int usbipsocket_accept(int sock, struct sockaddr *addr, socklen_t *addrlen)
{
	return accept4(sock, addr, addrlen, SOCK_NONBLOCK);
}

int usbipsocket_send(int sock, const void *buf, size_t len, int flags)
{
	return send(sock, buf, len, flags);
}

int usbipsocket_recv(int sock, void *buf, size_t max_len, int flags)
{
	return recv(sock, buf, max_len, flags);
}

int usbipsocket_setsockopt(int sock, int level, int optname, const void *optval,
			   socklen_t optlen)
{
	return setsockopt(sock, level, optname, optval, optlen);
}

int usbipsocket_close(int sock) { return close(sock); }
