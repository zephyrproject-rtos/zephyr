/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_SOCKETUTILS_H_
#define ZEPHYR_INCLUDE_NET_SOCKETUTILS_H_

#include <zephyr/net/socket.h>

/**
 * @brief Find port in addr:port string.
 *
 * @param addr_str String of addr[:port] format
 *
 * @return Pointer to "port" part, or NULL is none.
 */
const char *net_addr_str_find_port(const char *addr_str);

/**
 * @brief Call getaddrinfo() on addr:port string
 *
 * Convenience function to split addr[:port] string into address vs port
 * components (or use default port number), and call getaddrinfo() on the
 * result.
 *
 * @param addr_str String of addr[:port] format
 * @param def_port Default port number to use if addr_str doesn't contain it
 * @param hints getaddrinfo() hints
 * @param res Result of getaddrinfo() (freeaddrinfo() should be called on it
 *            as usual.
 *
 * @return Result of getaddrinfo() call.
 */
int net_getaddrinfo_addr_str(const char *addr_str, const char *def_port,
			     const struct zsock_addrinfo *hints,
			     struct zsock_addrinfo **res);

#endif /* ZEPHYR_INCLUDE_NET_SOCKETUTILS_H_ */
