/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SNTP (Simple Network Time Protocol)
 */

#ifndef ZEPHYR_INCLUDE_NET_SNTP_H_
#define ZEPHYR_INCLUDE_NET_SNTP_H_

#include <zephyr/net/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Simple Network Time Protocol API
 * @defgroup sntp SNTP
 * @ingroup networking
 * @{
 */

/** Time as returned by SNTP API, fractional seconds since 1 Jan 1970 */
struct sntp_time {
	uint64_t seconds;        /**< Second value */
	uint32_t fraction;       /**< Fractional seconds value */
#if defined(CONFIG_SNTP_UNCERTAINTY)
	uint64_t uptime_us;      /**< Uptime in microseconds */
	uint32_t uncertainty_us; /**< Uncertainty in microseconds */
#endif
};

/** SNTP context */
struct sntp_ctx {

/** @cond INTERNAL_HIDDEN */
	struct {
		struct zsock_pollfd fds[1];
		int nfds;
		int fd;
	} sock;
/** @endcond */

	/** Timestamp when the request was sent from client to server.
	 *  This is used to check if the originated timestamp in the server
	 *  reply matches the one in client request.
	 */
	struct sntp_time expected_orig_ts;
};

/**
 * @brief Initialize SNTP context
 *
 * @param ctx Address of sntp context.
 * @param addr IP address of NTP/SNTP server.
 * @param addr_len IP address length of NTP/SNTP server.
 *
 * @return 0 if ok, <0 if error.
 */
int sntp_init(struct sntp_ctx *ctx, struct sockaddr *addr,
	      socklen_t addr_len);

/**
 * @brief Perform SNTP query
 *
 * @param ctx Address of sntp context.
 * @param timeout Timeout of waiting for sntp response (in milliseconds).
 * @param time Timestamp including integer and fractional seconds since
 * 1 Jan 1970 (output).
 *
 * @return 0 if ok, <0 if error (-ETIMEDOUT if timeout).
 */
int sntp_query(struct sntp_ctx *ctx, uint32_t timeout,
	       struct sntp_time *time);

/**
 * @brief Release SNTP context
 *
 * @param ctx Address of sntp context.
 */
void sntp_close(struct sntp_ctx *ctx);

/**
 * @brief Convenience function to query SNTP in one-shot fashion
 *
 * Convenience wrapper which calls getaddrinfo(), sntp_init(),
 * sntp_query(), and sntp_close().
 *
 * @param server Address of server in format addr[:port]
 * @param timeout Query timeout
 * @param ts Timestamp including integer and fractional seconds since
 * 1 Jan 1970 (output).
 *
 * @return 0 if ok, <0 if error (-ETIMEDOUT if timeout).
 */
int sntp_simple(const char *server, uint32_t timeout,
		struct sntp_time *ts);

/**
 * @brief Convenience function to query SNTP in one-shot fashion
 * using a pre-initialized address struct
 *
 * Convenience wrapper which calls sntp_init(), sntp_query() and
 * sntp_close().
 *
 * @param addr IP address of NTP/SNTP server.
 * @param addr_len IP address length of NTP/SNTP server.
 * @param timeout Query timeout
 * @param ts Timestamp including integer and fractional seconds since
 * 1 Jan 1970 (output).
 *
 * @return 0 if ok, <0 if error (-ETIMEDOUT if timeout).
 */
int sntp_simple_addr(struct sockaddr *addr, socklen_t addr_len, uint32_t timeout,
		     struct sntp_time *ts);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif
