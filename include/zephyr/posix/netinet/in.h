/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_NETINET_IN_H_
#define ZEPHYR_INCLUDE_POSIX_NETINET_IN_H_

#include <stdint.h>

#include <zephyr/net/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t in_port_t;
typedef uint32_t in_addr_t;

#define in_addr  net_in_addr
#define in6_addr net_in6_addr

#define INET_ADDRSTRLEN  NET_INET_ADDRSTRLEN
#define INET6_ADDRSTRLEN NET_INET6_ADDRSTRLEN

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_NETINET_IN_H_ */
