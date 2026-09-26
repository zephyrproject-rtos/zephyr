/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_MPIPE_NET_TCP_SERVER_H_
#define ZEPHYR_SUBSYS_MPIPE_NET_TCP_SERVER_H_

#include <stdint.h>

/* Create, bind and listen on a TCP port. Returns the listening fd or -errno. */
int mpipe_net_tcp_listen(uint16_t port);

/* Block until a client connects. Returns the client fd or -errno. */
int mpipe_net_tcp_accept(int server_fd);

/* Close fd if open and reset it to -1. */
void mpipe_net_tcp_close(int *fd);

#endif /* ZEPHYR_SUBSYS_MPIPE_NET_TCP_SERVER_H_ */
