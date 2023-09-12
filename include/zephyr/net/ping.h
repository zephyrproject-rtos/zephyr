/*
 * Copyright (c) 2022 T-Mobile USA, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_PING_H_
#define ZEPHYR_INCLUDE_NET_PING_H_

/**
 * @brief Ping API
 * @defgroup ping Ping
 * @ingroup networking
 * @{
 */
#include <zephyr/net/net_if.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Asynchronous ping notification callback registered by the
 *        application.
 *
 * @param[in] ms Ping response time in milliseconds
 */
typedef void (*ping_callback_handler_t)(uint32_t ms);

/**
 * @brief Ping result handler
 *
 */
struct net_ping_handler {
	sys_snode_t node;
	ping_callback_handler_t handler;
};

/**
 * @brief Register ping result handler
 *
 * @param handler Handler to register
 */
void net_ping_cb_register(struct net_ping_handler *handler);

/**
 * @brief Unregister ping result handler
 *
 * @param cb Handler to remove
 */
void net_ping_cb_unregister(struct net_ping_handler *cb);

/**
 * @brief Used by the system to notify an event.
 *
 * @param ms Ping response time in ms
 */
void net_ping_resp_notify(uint32_t ms);

/**
 * @brief
 *
 * @param iface Network interface
 * @param dst IP address of the target host
 * @param size Size of the Payload Data in bytes.
 * @return 0 on success, a negative error code otherwise.
 */
int net_ping(struct net_if *iface, struct sockaddr *dst, size_t size);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_PING_H_ */
