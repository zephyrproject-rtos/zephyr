/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_WEBSOCKET_CONSOLE_H_
#define ZEPHYR_INCLUDE_NET_WEBSOCKET_CONSOLE_H_

#include <net/websocket.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Websocket console library
 * @defgroup websocket_console Websocket Console Library
 * @{
 */

/**
 * @brief Enable websocket console.
 *
 * @details The console can be sent over websocket to browser.
 *
 * @param ctx HTTP context
 *
 * @return 0 if ok, <0 if error
 */
int ws_console_enable(struct http_ctx *ctx);

/**
 * @brief Disable websocket console.
 *
 * @param ctx HTTP context
 *
 * @return 0 if ok, <0 if error
 */
int ws_console_disable(struct http_ctx *ctx);

/**
 * @brief Receive data from outside system and feed it into Zephyr.
 *
 * @param ctx HTTP context
 * @param pkt Network packet containing the received data.
 *
 * @return 0 if ok, <0 if error
 */
int ws_console_recv(struct http_ctx *ctx, struct net_pkt *pkt);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_WEBSOCKET_CONSOLE_H_ */
