/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Definitions for internet operations.
 * @ingroup posix
 *
 * Provides conversion between the binary and text forms of IPv4 and IPv6
 * addresses, and macros to convert values between host and network byte order.
 *
 * @posix_header{arpa_inet.h}
 */

#ifndef ZEPHYR_INCLUDE_POSIX_ARPA_INET_H_
#define ZEPHYR_INCLUDE_POSIX_ARPA_INET_H_

#include <stddef.h>

#include <zephyr/posix/netinet/in.h>
#include <zephyr/posix/sys/socket.h>

#include <zephyr/net/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

/* in_addr_t: documented in netinet/in.h */
typedef uint32_t in_addr_t;

/**
 * @brief Convert an IPv4 address from dotted-decimal text to binary.
 *
 * @param cp Dotted-decimal IPv4 address string (e.g. "192.0.2.1").
 *
 * @return IPv4 address in network byte order.
 * @retval INADDR_NONE The string is not a valid IPv4 address.
 *
 * @deprecated Use inet_pton() for new code.
 *
 * @posix_api{POSIX_NETWORKING,inet_addr}
 */
in_addr_t inet_addr(const char *cp);

/**
 * @brief Convert an IPv4 address from binary to dotted-decimal text.
 *
 * @note The returned pointer is to a static buffer that may be overwritten by
 *       subsequent calls.
 *
 * @param in IPv4 address in network byte order.
 *
 * @return Pointer to a dotted-decimal string, or NULL on failure.
 *
 * @deprecated Use inet_ntop() for new code.
 *
 * @posix_api{POSIX_NETWORKING,inet_ntoa}
 */
char *inet_ntoa(struct in_addr in);

/**
 * @brief Convert an IPv4 or IPv6 address from binary to text form.
 *
 * @param family   Address family: @c AF_INET or @c AF_INET6.
 * @param src      Source address in network byte order.
 * @param[out] dst Buffer for the text form.
 * @param size     Size of @p dst in bytes (@c INET_ADDRSTRLEN / @c INET6_ADDRSTRLEN).
 *
 * @return @p dst on success, or NULL with errno set on failure.
 *
 * @posix_api{POSIX_NETWORKING,inet_ntop}
 */
char *inet_ntop(sa_family_t family, const void *src, char *dst, size_t size);

/**
 * @brief Convert an IPv4 or IPv6 address from text form to binary.
 *
 * @param family   Address family: @c AF_INET or @c AF_INET6.
 * @param src      Text form of the address.
 * @param[out] dst Buffer for the binary address.
 *
 * @return 1 on success, 0 if @p src is not a valid address, -1 on error.
 *
 * @posix_api{POSIX_NETWORKING,inet_pton}
 */
int inet_pton(sa_family_t family, const char *src, void *dst);

#define ntohs(x)  net_ntohs(x)  /**< Convert a 16-bit value from network to host byte order. */

#define ntohl(x)  net_ntohl(x)  /**< Convert a 32-bit value from network to host byte order. */

#define ntohll(x) net_ntohll(x) /**< Convert a 64-bit value from network to host byte order. */

#define htons(x)  net_htons(x)  /**< Convert a 16-bit value from host to network byte order. */

#define htonl(x)  net_htonl(x)  /**< Convert a 32-bit value from host to network byte order. */

#define htonll(x) net_htonll(x) /**< Convert a 64-bit value from host to network byte order. */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_ARPA_INET_H_ */
