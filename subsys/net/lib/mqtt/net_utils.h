/*
 * Copyright (c) 2017 Texas Instruments, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NET_UTILS_H_
#define _NET_UTILS_H_

#include <stddef.h>
#include <stdbool.h>
#include <net/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

bool net_util_init_tcp_client(struct sockaddr *addr,
			      struct sockaddr *peer_addr,
			      const char *peer_addr_str,
			      u16_t peer_port);

#ifdef __cplusplus
}
#endif

#endif /* _NET_UTILS_H_ */
