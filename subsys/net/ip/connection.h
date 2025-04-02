/** @file
 @brief Generic connection handling.

 This is not to be included by the application.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CONNECTION_H
#define __CONNECTION_H

#include <zephyr/types.h>

#include <zephyr/sys/util.h>

#include <zephyr/net/net_context.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>

#ifdef __cplusplus
extern "C" {
#endif

struct net_conn;

struct net_conn_handle;

/**
 * @brief Function that is called by connection subsystem when a
 * net packet is received which matches local and remote address
 * (and port in case of TCP/UDP packets).
 *
 * The arguments ip_hdr and proto_hdr are NULL in case of non-IP
 * protocols.
 */
typedef enum net_verdict (*net_conn_cb_t)(struct net_conn *conn,
					  struct net_pkt *pkt,
					  union net_ip_header *ip_hdr,
					  union net_proto_header *proto_hdr,
					  void *user_data);

/**
 * @brief Information about a connection in the system.
 *
 * Stores the connection information.
 *
 */
struct net_conn {
	/** Internal slist node */
	sys_snode_t node;

	/** Remote socket address */
	struct sockaddr remote_addr;

	/** Local socket address */
	struct sockaddr local_addr;

	/** Callback to be called when matching net packet is received */
	net_conn_cb_t cb;

	/** A pointer to the net_context corresponding to the connection.
	 *  Can be NULL if no net_context is associated.
	 */
	struct net_context *context;

	/** Possible user to pass to the callback */
	void *user_data;

	/** Connection protocol */
	uint16_t proto;

	/** Connection type */
	enum net_sock_type type;

	/** Protocol family */
	uint8_t family;

	/** Flags for the connection */
	uint8_t flags;

	/** Is v4-mapping-to-v6 enabled for this connection */
	uint8_t v6only : 1;
};

/**
 * @brief Register a callback to be called when a net packet
 * is received corresponding to received packet.
 *
 * @param proto Protocol for the connection (depends on the protocol
 *              family, e.g. UDP/TCP in the case of AF_INET/AF_INET6)
 * @param type Connection type (SOCK_STREAM/DGRAM/RAW)
 * @param family Protocol family (AF_*)
 * @param remote_addr Remote address of the connection end point.
 * @param local_addr Local address of the connection end point.
 * @param remote_port Remote port of the connection end point.
 * @param local_port Local port of the connection end point.
 * @param cb Callback to be called when a matching net pkt is received
 * @param context net_context structure related to the connection.
 * @param user_data User data supplied by caller.
 * @param handle Connection handle that can be used when unregistering
 *
 * @return Return 0 if the registration succeed, <0 otherwise.
 */
#if defined(CONFIG_NET_NATIVE)
int net_conn_register(uint16_t proto, enum net_sock_type type, uint8_t family,
		      const struct sockaddr *remote_addr,
		      const struct sockaddr *local_addr,
		      uint16_t remote_port,
		      uint16_t local_port,
		      struct net_context *context,
		      net_conn_cb_t cb,
		      void *user_data,
		      struct net_conn_handle **handle);
#else
static inline int net_conn_register(uint16_t proto, enum net_sock_type type,
				    uint8_t family,
				    const struct sockaddr *remote_addr,
				    const struct sockaddr *local_addr,
				    uint16_t remote_port,
				    uint16_t local_port,
				    struct net_context *context,
				    net_conn_cb_t cb,
				    void *user_data,
				    struct net_conn_handle **handle)
{
	ARG_UNUSED(proto);
	ARG_UNUSED(family);
	ARG_UNUSED(remote_addr);
	ARG_UNUSED(local_addr);
	ARG_UNUSED(remote_port);
	ARG_UNUSED(local_port);
	ARG_UNUSED(cb);
	ARG_UNUSED(context);
	ARG_UNUSED(user_data);
	ARG_UNUSED(handle);

	return -ENOTSUP;
}
#endif

/**
 * @brief Unregister connection handler.
 *
 * @param handle Handle from registering.
 *
 * @return Return 0 if the unregistration succeed, <0 otherwise.
 */
#if defined(CONFIG_NET_NATIVE)
int net_conn_unregister(struct net_conn_handle *handle);
#else
static inline int net_conn_unregister(struct net_conn_handle *handle)
{
	ARG_UNUSED(handle);

	return -ENOTSUP;
}
#endif

/**
 * @brief Update the callback, user data, remote address, and port
 * for a registered connection handle.
 *
 * @param handle A handle registered with net_conn_register()
 * @param cb Callback to be called
 * @param user_data User data supplied by caller.
 * @param remote_addr Remote address
 * @param remote_port Remote port
 *
 * @return Return 0 if the change succeed, <0 otherwise.
 */
int net_conn_update(struct net_conn_handle *handle,
		    net_conn_cb_t cb,
		    void *user_data,
		    const struct sockaddr *remote_addr,
		    uint16_t remote_port);

/**
 * @brief Called by net_core.c when a network packet is received.
 *
 * @param pkt Network packet holding received data
 * @param proto Protocol for the connection
 *
 * @return NET_OK if the packet was consumed, NET_DROP if
 * the packet parsing failed and the caller should handle
 * the received packet. If corresponding IP protocol support is
 * disabled, the function will always return NET_DROP.
 */
#if defined(CONFIG_NET_IP) || defined(CONFIG_NET_CONNECTION_SOCKETS)
enum net_verdict net_conn_input(struct net_pkt *pkt,
				union net_ip_header *ip_hdr,
				uint8_t proto,
				union net_proto_header *proto_hdr);
#else
static inline enum net_verdict net_conn_input(struct net_pkt *pkt,
					      union net_ip_header *ip_hdr,
					      uint8_t proto,
					      union net_proto_header *proto_hdr)
{
	return NET_DROP;
}
#endif /* CONFIG_NET_IP || CONFIG_NET_CONNECTION_SOCKETS */

/**
 * @typedef net_conn_foreach_cb_t
 * @brief Callback used while iterating over network connection
 * handlers.
 *
 * @param conn A valid pointer on current network connection handler.
 * @param user_data A valid pointer on some user data or NULL
 */
typedef void (*net_conn_foreach_cb_t)(struct net_conn *conn, void *user_data);

/**
 * @brief Go through all the network connection handlers and call callback
 * for each network connection handler.
 *
 * @param cb User supplied callback function to call.
 * @param user_data User specified data.
 */
void net_conn_foreach(net_conn_foreach_cb_t cb, void *user_data);

#if defined(CONFIG_NET_NATIVE)
void net_conn_init(void);
#else
#define net_conn_init(...)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __CONNECTION_H */
