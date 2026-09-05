/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Single-client TCP server helpers shared by the net plugin elements.
 * Private to subsys/mp/net, not part of the public MP API.
 */

#ifndef ZEPHYR_SUBSYS_MP_NET_TCP_SERVER_H_
#define ZEPHYR_SUBSYS_MP_NET_TCP_SERVER_H_

#include <stdint.h>

/*
 * Bind a listening socket on @p port and block until one client connects.
 * When IPv6 is enabled a single IPv6 socket is used, with V6ONLY cleared so
 * IPv4 clients are accepted through IPv4-mapped addresses.
 *
 * On success both file descriptors are stored; on failure both are left at -1.
 *
 * Returns 0 on success, negative errno on failure.
 */
int mp_net_tcp_accept_one(uint16_t port, int *server_fd, int *client_fd);

/* Close whichever of the two sockets are open and reset them to -1. */
void mp_net_tcp_close(int *server_fd, int *client_fd);

#endif /* ZEPHYR_SUBSYS_MP_NET_TCP_SERVER_H_ */
