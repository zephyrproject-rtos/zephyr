/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Internet address family.
 * @ingroup posix
 *
 * Provides the fundamental Internet address and port types used across the
 * BSD socket API and the IP protocol family headers.
 *
 * @posix_header{netinet_in.h}
 */

#ifndef ZEPHYR_INCLUDE_POSIX_NETINET_IN_H_
#define ZEPHYR_INCLUDE_POSIX_NETINET_IN_H_

#include <stdint.h>

#include <zephyr/net/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Unsigned 16-bit Internet port number.
 */
typedef uint16_t in_port_t;

/**
 * @brief Unsigned 32-bit IPv4 address.
 */
typedef uint32_t in_addr_t;

/**
 * @brief IPv4 address structure.
 */
#define in_addr  net_in_addr

/**
 * @brief IPv6 address structure.
 */
#define in6_addr net_in6_addr

/**
 * @brief Buffer length sufficient to hold the text form of an IPv4 address.
 */
#define INET_ADDRSTRLEN  NET_INET_ADDRSTRLEN

/**
 * @brief Buffer length sufficient to hold the text form of an IPv6 address.
 */
#define INET6_ADDRSTRLEN NET_INET6_ADDRSTRLEN

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_NETINET_IN_H_ */
