/*
 * Copyright (c) 2022 YuLong Yao<feilongphone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Zephyr headers */
#include "usb_dc_usbip_adapt.h"

#include <zephyr/kernel.h>

#define SOMAXCONN 5

int usbipsocket_socket(void) { return zsock_socket(PF_INET, SOCK_STREAM, 0); }

int usbipsocket_bind(int sock)
{
	struct sockaddr_in srv;

	memset(&srv, 0, sizeof(srv));
	srv.sin_family = AF_INET;
	srv.sin_addr.s_addr = htonl(INADDR_ANY);
	srv.sin_port = htons(CONFIG_USBIP_PORT);

	return zsock_bind(sock, (struct sockaddr *)&srv, sizeof(srv));
}

int usbipsocket_connect(int sock, const struct sockaddr *addr,
			socklen_t addrlen)
{
	return zsock_connect(sock, addr, addrlen);
}

int usbipsocket_listen(int sock) { return zsock_listen(sock, SOMAXCONN); }

int usbipsocket_accept(int sock, struct sockaddr *addr, socklen_t *addrlen)
{
	return zsock_accept(sock, addr, addrlen);
}

int usbipsocket_send(int sock, const void *buf, size_t len, int flags)
{
	return zsock_send(sock, buf, len, flags);
}

int usbipsocket_recv(int sock, void *buf, size_t max_len, int flags)
{
	return zsock_recv(sock, buf, max_len, flags);
}

int usbipsocket_setsockopt(int sock, int level, int optname, const void *optval,
			   socklen_t optlen)
{
	return zsock_setsockopt(sock, level, optname, optval, optlen);
}

int usbipsocket_close(int sock) { return zsock_close(sock); }
