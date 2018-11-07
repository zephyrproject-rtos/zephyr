/** @file
 @brief Generic connection handling.

 This is not to be included by the application.
 */

/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CONNECTION_RAW_H
#define __CONNECTION_RAW_H

#include <zephyr/types.h>

#include <misc/util.h>

#include <net/net_core.h>
#include <net/net_ip.h>
#include <net/net_pkt.h>

#ifdef __cplusplus
extern "C" {
#endif

struct net_conn_raw;

struct net_conn_raw_handle;

/**
 * @brief Function that is called by connection subsystem when a raw
 * packet is received
 *
 * @param conn Connection handle that can be used when unregistering
 * @param pkt Network packet holding received data
 * @param user_data User data related to this connection
 *
 * @return NET_OK if the packet was consumed by the application
 *         NET_DROP if the application was not interested about the packet
 */
typedef enum net_verdict (*net_conn_raw_cb_t)(struct net_conn_raw *conn,
					      struct net_pkt *pkt,
					      void *user_data);

/**
 * @brief Information about a raw connection in the system.
 *
 * Stores the raw connection information.
 *
 */
struct net_conn_raw {
	/** Callback to be called when matching UDP packet is received */
	net_conn_raw_cb_t cb;

	/** Possible user to pass to the callback */
	void *user_data;

	/** Connection protocol (IEEE 802.3 protocol number in network byte
	 * order)
	 */
	u16_t proto;

	/** Flags for the connection */
	u8_t flags;
};

/**
 * @brief Register a callback to be called when a network packet is received
 *
 * @param proto Protocol for the connection
 * @param cb Callback to be called
 * @param user_data User data supplied by caller.
 * @param handle Connection handle that can be used when unregistering
 *
 * @return Return 0 if the registration succeed, <0 otherwise.
 */
int net_conn_raw_register(u16_t proto,
			  net_conn_raw_cb_t cb,
			  void *user_data,
			  struct net_conn_raw_handle **handle);

/**
 * @brief Unregister connection handler.
 *
 * @param handle Handle from registering.
 *
 * @return Return 0 if the unregistration succeed, <0 otherwise.
 */
int net_conn_raw_unregister(struct net_conn_raw_handle *handle);

/**
 * @brief Change the callback and user_data for a registered connection
 * handle.
 *
 * @param handle A handle registered with net_conn_raw_register()
 * @param cb Callback to be called
 * @param user_data User data supplied by caller.
 *
 * @return Return 0 if the the change succeed, <0 otherwise.
 */
int net_conn_raw_change_callback(struct net_conn_raw_handle *handle,
				 net_conn_raw_cb_t cb, void *user_data);

/**
 * @brief Called by net_core.c when a network packet is received.
 *
 * @param proto IEEE 802.3 protocol id for the data
 * @param pkt Network packet holding received data
 */
#if defined(CONFIG_NET_SOCKET_RAW)
void net_conn_raw_input(u16_t proto, struct net_pkt *pkt);
#else
static inline void net_conn_raw_input(u16_t proto, struct net_pkt *pkt)
{
	ARG_UNUSED(proto);
	ARG_UNUSED(pkt);
}
#endif /* CONFIG_NET_SOCKET_RAW */

/**
 * @typedef net_conn_raw_foreach_cb_t
 * @brief Callback used while iterating over network connection
 * handlers.
 *
 * @param conn A valid pointer on current network connection handler.
 * @param user_data A valid pointer on some user data or NULL
 */
typedef void (*net_conn_raw_foreach_cb_t)(struct net_conn_raw *conn,
					  void *user_data);

/**
 * @brief Go through all the network connection handlers and call callback
 * for each network connection handler.
 *
 * @param cb User supplied callback function to call.
 * @param user_data User specified data.
 */
void net_conn_raw_foreach(net_conn_raw_foreach_cb_t cb, void *user_data);

#ifdef __cplusplus
}
#endif

#endif /* __CONNECTION_RAW_H */
