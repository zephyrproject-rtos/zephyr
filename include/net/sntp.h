/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SNTP_H
#define __SNTP_H

#include <net/net_app.h>

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
 * @param user_data The user data given in sntp_request().
 */

typedef void (*sntp_resp_cb_t)(struct sntp_ctx *ctx,
			       int status,
			       u64_t epoch_time,
			       void *user_data);

/** SNTP context */
struct sntp_ctx {
	/** Network application context */
	struct net_app_ctx  net_app_ctx;

	/** Timestamp when the request is departed the client for the server.
	 *  This is used to check if the originated timestamp in the server
	 *  reply matches the one in client request.
	 */
	u32_t		    expected_orig_ts;

	/** SNTP response callback */
	sntp_resp_cb_t      cb;

	/** The user data of SNTP response callback */
	void                *user_data;

	/** Is this context setup or not */
	bool                is_init;
};

/**
 * @brief Initialize SNTP context
 *
 * @param ctx Address of sntp context.
 * @param srv_addr IP address of NTP/SNTP server.
 * @param srv_port Port number of NTP/SNTP server.
 * @param timeout Timeout of sntp context initialization (in milliseconds).
 *
 * @return 0 if ok, <0 if error.
 */
int sntp_init(struct sntp_ctx *ctx,
	      const char *srv_addr,
	      u16_t srv_port,
	      u32_t timeout);

/**
 * @brief Send SNTP request
 *
 * @param ctx Address of sntp context.
 * @param timeout Timeout of sending sntp request (in milliseconds).
 * @param callback Callback function of sntp response.
 * @param user_data User data that will be passed to callback function.
 *
 * @return 0 if ok, <0 if error.
 */
int sntp_request(struct sntp_ctx *ctx,
		 u32_t timeout,
		 sntp_resp_cb_t callback,
		 void *user_data);

/**
 * @brief Release SNTP context
 *
 * @param ctx Address of sntp context.
 */
void sntp_close(struct sntp_ctx *ctx);

#endif
