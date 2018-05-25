/*
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NET_UTILS_H_
#define _NET_UTILS_H_

#include <stdint.h>
#include <stdbool.h>

#ifndef __ZEPHYR__
#include <sys/socket.h>
#else
#include <net/socket.h>
#endif

#define min(a, b) (((a) < (b)) ? (a) : (b))

#ifdef __cplusplus
extern "C" {
#endif

bool net_util_init_tcp_client(struct sockaddr *addr,
			      struct sockaddr *peer_addr,
			      const char *peer_addr_str,
			      uint16_t peer_port);

#ifdef __cplusplus
}
#endif

#endif /* _NET_UTILS_H_ */
