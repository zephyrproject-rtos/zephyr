/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file icmp.h
 * @brief Header file for ICMP protocol support.
 * @ingroup icmp
 *
 * @defgroup icmp ICMP
 * @brief Send and receive IPv4 or IPv6 ICMP (Internet Control Message Protocol)
 *        Echo Request messages.
 * @since 3.5
 * @version 0.8.0
 * @ingroup networking
 * @{
 */

#ifndef ZEPHYR_INCLUDE_NET_ICMP_H_
#define ZEPHYR_INCLUDE_NET_ICMP_H_

#include <stddef.h>

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NET_ICMPV4_ECHO_REQUEST 8    /**< ICMPv4 Echo-Request */
#define NET_ICMPV4_ECHO_REPLY   0    /**< ICMPv4 Echo-Reply */
#define NET_ICMPV6_ECHO_REQUEST 128  /**< ICMPv6 Echo-Request */
#define NET_ICMPV6_ECHO_REPLY   129  /**< ICMPv6 Echo-Reply */

struct net_icmp_ctx;
struct net_icmp_ip_hdr;
struct net_icmp_ping_params;

/**
 * @typedef net_icmp_handler_t
 * @brief Handler function that is called when ICMP response is received.
 *
 * @param ctx ICMP context to use.
 * @param pkt Received ICMP response network packet.
 * @param ip_hdr IP header of the packet.
 * @param icmp_hdr ICMP header of the packet.
 * @param user_data A valid pointer to user data or NULL
 */
typedef int (*net_icmp_handler_t)(struct net_icmp_ctx *ctx,
				  struct net_pkt *pkt,
				  struct net_icmp_ip_hdr *ip_hdr,
				  struct net_icmp_hdr *icmp_hdr,
				  void *user_data);

/**
 * @typedef net_icmp_offload_ping_handler_t
 * @brief Handler function that is called when an Echo-Request is sent
 *        to offloaded device. This handler is typically setup by the
 *        device driver so that it can catch the ping request and send
 *        it to the offloaded device.
 *
 * @param ctx ICMP context used in this request.
 * @param iface Network interface, can be set to NULL in which case the
 *        interface is selected according to destination address.
 * @param dst IP address of the target host.
 * @param params Echo-Request specific parameters. May be NULL in which case
 *        suitable default parameters are used.
 * @param user_data User supplied opaque data passed to the handler. May be NULL.
 *
 */
typedef int (*net_icmp_offload_ping_handler_t)(struct net_icmp_ctx *ctx,
					       struct net_if *iface,
					       struct sockaddr *dst,
					       struct net_icmp_ping_params *params,
					       void *user_data);

/**
 * @brief ICMP context structure.
 */
struct net_icmp_ctx {
	/** List node */
	sys_snode_t node;

	/** ICMP response handler */
	net_icmp_handler_t handler;

	/** Network interface where the ICMP request was sent */
	struct net_if *iface;

	/** Opaque user supplied data */
	void *user_data;

	/** ICMP type of the response we are waiting */
	uint8_t type;

	/** ICMP code of the response type we are waiting */
	uint8_t code;
};

/**
 * @brief Struct presents either IPv4 or IPv6 header in ICMP response message.
 */
struct net_icmp_ip_hdr {
	union {
		/** IPv4 header in response message. */
		struct net_ipv4_hdr *ipv4;

		/** IPv6 header in response message. */
		struct net_ipv6_hdr *ipv6;
	};

	/** Is the header IPv4 or IPv6 one. Value of either AF_INET or AF_INET6 */
	sa_family_t family;
};

/**
 * @brief Struct presents parameters that are needed when sending
 *        Echo-Request (ping) messages.
 */
struct net_icmp_ping_params {
	/** An identifier to aid in matching Echo Replies to this Echo Request.
	 * May be zero.
	 */
	uint16_t identifier;

	/** A sequence number to aid in matching Echo Replies to this
	 * Echo Request. May be zero.
	 */
	uint16_t sequence;

	/** Can be either IPv4 Type-of-service field value, or IPv6 Traffic
	 * Class field value. Represents combined DSCP and ECN values.
	 */
	uint8_t tc_tos;

	/** Network packet priority. */
	int priority;

	/** Arbitrary payload data that will be included in the Echo Reply
	 * verbatim. May be NULL.
	 */
	const void *data;

	/** Size of the Payload Data in bytes. May be zero. In case data
	 * pointer is NULL, the function will generate the payload up to
	 * the requested size.
	 */
	size_t data_size;
};

/**
 * @brief Initialize the ICMP context structure. Must be called before
 *        ICMP messages can be sent. This will register handler to the
 *        system.
 *
 * @param ctx ICMP context used in this request.
 * @param type Type of ICMP message we are handling.
 * @param code Code of ICMP message we are handling.
 * @param handler Callback function that is called when a response is received.
 */
int net_icmp_init_ctx(struct net_icmp_ctx *ctx, uint8_t type, uint8_t code,
		      net_icmp_handler_t handler);

/**
 * @brief Cleanup the ICMP context structure. This will unregister the ICMP handler
 *        from the system.
 *
 * @param ctx ICMP context used in this request.
 */
int net_icmp_cleanup_ctx(struct net_icmp_ctx *ctx);

/**
 * @brief Send ICMP echo request message.
 *
 * @param ctx ICMP context used in this request.
 * @param iface Network interface, can be set to NULL in which case the
 *        interface is selected according to destination address.
 * @param dst IP address of the target host.
 * @param params Echo-Request specific parameters. May be NULL in which case
 *        suitable default parameters are used.
 * @param user_data User supplied opaque data passed to the handler. May be NULL.
 *
 * @return Return 0 if the sending succeed, <0 otherwise.
 */
int net_icmp_send_echo_request(struct net_icmp_ctx *ctx,
			       struct net_if *iface,
			       struct sockaddr *dst,
			       struct net_icmp_ping_params *params,
			       void *user_data);

/**
 * @brief Send ICMP echo request message without waiting during send.
 *
 * @details This function can be used to send ICMP Echo-Request from a system
 *          workqueue handler which should not have any sleeps or waits.
 *          This variant will do the net_buf allocations with K_NO_WAIT.
 *          This will avoid a warning message in the log about the timeout.
 *
 * @param ctx ICMP context used in this request.
 * @param iface Network interface, can be set to NULL in which case the
 *        interface is selected according to destination address.
 * @param dst IP address of the target host.
 * @param params Echo-Request specific parameters. May be NULL in which case
 *        suitable default parameters are used.
 * @param user_data User supplied opaque data passed to the handler. May be NULL.
 *
 * @return Return 0 if the sending succeed, <0 otherwise.
 */
int net_icmp_send_echo_request_no_wait(struct net_icmp_ctx *ctx,
				       struct net_if *iface,
				       struct sockaddr *dst,
				       struct net_icmp_ping_params *params,
				       void *user_data);

/**
 * @brief ICMP offload context structure.
 */
struct net_icmp_offload {
	/** List node */
	sys_snode_t node;

	/**
	 * ICMP response handler. Currently there is only one handler.
	 * This means that one offloaded ping request/response can be going
	 * on at the same time.
	 */
	net_icmp_handler_t handler;

	/** ICMP offloaded ping handler */
	net_icmp_offload_ping_handler_t ping_handler;

	/** Offloaded network interface */
	struct net_if *iface;
};

/**
 * @brief Register a handler function that is called when an Echo-Request
 *        is sent to the offloaded device. This function is typically
 *        called by a device driver so that it can do the actual offloaded
 *        ping call.
 *
 * @param ctx ICMP offload context used for this interface.
 * @param iface Network interface of the offloaded device.
 * @param ping_handler Function to be called when offloaded ping request is done.
 *
 * @return Return 0 if the register succeed, <0 otherwise.
 */
int net_icmp_register_offload_ping(struct net_icmp_offload *ctx,
				   struct net_if *iface,
				   net_icmp_offload_ping_handler_t ping_handler);

/**
 * @brief Unregister the offload handler.
 *
 * @param ctx ICMP offload context used for this interface.
 *
 * @return Return 0 if the call succeed, <0 otherwise.
 */
int net_icmp_unregister_offload_ping(struct net_icmp_offload *ctx);

/**
 * @brief Get a ICMP response handler function for an offloaded device.
 *        When a ping response is received by the driver, it should call
 *        the handler function with proper parameters so that the ICMP response
 *        is received by the net stack.
 *
 * @param ctx ICMP offload context used in this request.
 * @param resp_handler Function to be called when offloaded ping response
 *        is received by the offloaded driver. The ICMP response handler
 *        function is returned and the caller should call it when appropriate.
 *
 * @return Return 0 if the call succeed, <0 otherwise.
 */
int net_icmp_get_offload_rsp_handler(struct net_icmp_offload *ctx,
				     net_icmp_handler_t *resp_handler);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_ICMP_H */

/**@}  */
