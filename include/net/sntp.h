/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_SNTP_H_
#define ZEPHYR_INCLUDE_NET_SNTP_H_

#include <net/socket.h>

struct sntp_ctx;

/**
 * @typedef sntp_resp_cb_t
 * @brief SNTP response callback.
 *
 * @details The callback is called after a sntp response is received
 *
 * @param ctx Address of sntp context.
 * @param status Error code of sntp response.
 * @param epoch_time Seconds since 1 January 1970.
 */

typedef void (*sntp_resp_cb_t)(struct sntp_ctx *ctx, int status,
			       u64_t epoch_time);

/** SNTP context */
struct sntp_ctx {
	struct {
		struct pollfd fds[1];
		int nfds;
		int fd;
	} sock;

	/** Timestamp when the request was sent from client to server.
	 *  This is used to check if the originated timestamp in the server
	 *  reply matches the one in client request.
	 */
	u32_t expected_orig_ts;

	/** SNTP response callback */
	sntp_resp_cb_t cb;
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
 * @brief Send SNTP request
 *
 * @param ctx Address of sntp context.
 * @param timeout Timeout of waiting for sntp response (in milliseconds).
 * @param callback Callback function of sntp response.
 *
 * @return 0 if ok, <0 if error.
 */
int sntp_request(struct sntp_ctx *ctx, u32_t timeout,
		 sntp_resp_cb_t callback);

/**
 * @brief Release SNTP context
 *
 * @param ctx Address of sntp context.
 */
void sntp_close(struct sntp_ctx *ctx);

#endif
