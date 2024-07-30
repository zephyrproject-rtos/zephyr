/** @file
 *  @brief DHCPv4 Client Handler
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_DHCPV4_H_
#define ZEPHYR_INCLUDE_NET_DHCPV4_H_

#include <zephyr/sys/slist.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief DHCPv4
 * @defgroup dhcpv4 DHCPv4
 * @since 1.7
 * @version 0.8.0
 * @ingroup networking
 * @{
 */

/** @cond INTERNAL_HIDDEN */

/** Current state of DHCPv4 client address negotiation.
 *
 * Additions removals and reorders in this definition must be
 * reflected within corresponding changes to net_dhcpv4_state_name.
 */
enum net_dhcpv4_state {
	NET_DHCPV4_DISABLED,
	NET_DHCPV4_INIT,
	NET_DHCPV4_SELECTING,
	NET_DHCPV4_REQUESTING,
	NET_DHCPV4_RENEWING,
	NET_DHCPV4_REBINDING,
	NET_DHCPV4_BOUND,
	NET_DHCPV4_DECLINE,
} __packed;

/** @endcond */

/**
 * @brief DHCPv4 message types
 *
 * These enumerations represent RFC2131 defined msy type codes, hence
 * they should not be renumbered.
 *
 * Additions, removald and reorders in this definition must be reflected
 * within corresponding changes to net_dhcpv4_msg_type_name.
 */
enum net_dhcpv4_msg_type {
	NET_DHCPV4_MSG_TYPE_DISCOVER	= 1, /**< Discover message */
	NET_DHCPV4_MSG_TYPE_OFFER	= 2, /**< Offer message */
	NET_DHCPV4_MSG_TYPE_REQUEST	= 3, /**< Request message */
	NET_DHCPV4_MSG_TYPE_DECLINE	= 4, /**< Decline message */
	NET_DHCPV4_MSG_TYPE_ACK		= 5, /**< Acknowledge message */
	NET_DHCPV4_MSG_TYPE_NAK		= 6, /**< Negative acknowledge message */
	NET_DHCPV4_MSG_TYPE_RELEASE	= 7, /**< Release message */
	NET_DHCPV4_MSG_TYPE_INFORM	= 8, /**< Inform message */
};

struct net_dhcpv4_option_callback;

/**
 * @typedef net_dhcpv4_option_callback_handler_t
 * @brief Define the application callback handler function signature
 *
 * @param cb Original struct net_dhcpv4_option_callback owning this handler
 * @param length The length of data returned by the server. If this is
 *		 greater than cb->max_length, only cb->max_length bytes
 *		 will be available in cb->data
 * @param msg_type Type of DHCP message that triggered the callback
 * @param iface The interface on which the DHCP message was received
 *
 * Note: cb pointer can be used to retrieve private data through
 * CONTAINER_OF() if original struct net_dhcpv4_option_callback is stored in
 * another private structure.
 */
typedef void (*net_dhcpv4_option_callback_handler_t)(struct net_dhcpv4_option_callback *cb,
						     size_t length,
						     enum net_dhcpv4_msg_type msg_type,
						     struct net_if *iface);

/** @cond INTERNAL_HIDDEN */

/**
 * @brief DHCP option callback structure
 *
 * Used to register a callback in the DHCPv4 client callback list.
 * As many callbacks as needed can be added as long as each of them
 * are unique pointers of struct net_dhcpv4_option_callback.
 * Beware such structure should not be allocated on stack.
 *
 * Note: To help setting it, see net_dhcpv4_init_option_callback() below
 */
struct net_dhcpv4_option_callback {
	/** This is meant to be used internally and the user should not
	 * mess with it.
	 */
	sys_snode_t node;

	/** Actual callback function being called when relevant. */
	net_dhcpv4_option_callback_handler_t handler;

	/** The DHCP option this callback is attached to. */
	uint8_t option;

	/** Maximum length of data buffer. */
	size_t max_length;

	/** Pointer to a buffer of size max_length that is used to store the
	 * option data.
	 */
	void *data;
};

/** @endcond */

/**
 * @brief Helper to initialize a struct net_dhcpv4_option_callback properly
 * @param callback A valid Application's callback structure pointer.
 * @param handler A valid handler function pointer.
 * @param option The DHCP option the callback responds to.
 * @param data A pointer to a buffer for max_length bytes.
 * @param max_length The maximum length of the data returned.
 */
static inline void net_dhcpv4_init_option_callback(struct net_dhcpv4_option_callback *callback,
						   net_dhcpv4_option_callback_handler_t handler,
						   uint8_t option,
						   void *data,
						   size_t max_length)
{
	__ASSERT(callback, "Callback pointer should not be NULL");
	__ASSERT(handler, "Callback handler pointer should not be NULL");
	__ASSERT(data, "Data pointer should not be NULL");

	callback->handler = handler;
	callback->option = option;
	callback->data = data;
	callback->max_length = max_length;
}

/**
 * @brief Add an application callback.
 * @param cb A valid application's callback structure pointer.
 * @return 0 if successful, negative errno code on failure.
 */
int net_dhcpv4_add_option_callback(struct net_dhcpv4_option_callback *cb);

/**
 * @brief Remove an application callback.
 * @param cb A valid application's callback structure pointer.
 * @return 0 if successful, negative errno code on failure.
 */
int net_dhcpv4_remove_option_callback(struct net_dhcpv4_option_callback *cb);

/**
 * @brief Helper to initialize a struct net_dhcpv4_option_callback for encapsulated vendor-specific
 * options properly
 * @param callback A valid Application's callback structure pointer.
 * @param handler A valid handler function pointer.
 * @param option The DHCP encapsulated vendor-specific option the callback responds to.
 * @param data A pointer to a buffer for max_length bytes.
 * @param max_length The maximum length of the data returned.
 */
static inline void
net_dhcpv4_init_option_vendor_callback(struct net_dhcpv4_option_callback *callback,
				       net_dhcpv4_option_callback_handler_t handler, uint8_t option,
				       void *data, size_t max_length)
{
	__ASSERT(callback, "Callback pointer should not be NULL");
	__ASSERT(handler, "Callback handler pointer should not be NULL");
	__ASSERT(data, "Data pointer should not be NULL");

	callback->handler = handler;
	callback->option = option;
	callback->data = data;
	callback->max_length = max_length;
}

/**
 * @brief Add an application callback for encapsulated vendor-specific options.
 * @param cb A valid application's callback structure pointer.
 * @return 0 if successful, negative errno code on failure.
 */
int net_dhcpv4_add_option_vendor_callback(struct net_dhcpv4_option_callback *cb);

/**
 * @brief Remove an application callback for encapsulated vendor-specific options.
 * @param cb A valid application's callback structure pointer.
 * @return 0 if successful, negative errno code on failure.
 */
int net_dhcpv4_remove_option_vendor_callback(struct net_dhcpv4_option_callback *cb);

/**
 *  @brief Start DHCPv4 client on an iface
 *
 *  @details Start DHCPv4 client on a given interface. DHCPv4 client
 *  will start negotiation for IPv4 address. Once the negotiation is
 *  success IPv4 address details will be added to interface.
 *
 *  @param iface A valid pointer on an interface
 */
void net_dhcpv4_start(struct net_if *iface);

/**
 *  @brief Stop DHCPv4 client on an iface
 *
 *  @details Stop DHCPv4 client on a given interface. DHCPv4 client
 *  will remove all configuration obtained from a DHCP server from the
 *  interface and stop any further negotiation with the server.
 *
 *  @param iface A valid pointer on an interface
 */
void net_dhcpv4_stop(struct net_if *iface);

/**
 *  @brief Restart DHCPv4 client on an iface
 *
 *  @details Restart DHCPv4 client on a given interface. DHCPv4 client
 *  will restart the state machine without any of the initial delays
 *  used in start.
 *
 *  @param iface A valid pointer on an interface
 */
void net_dhcpv4_restart(struct net_if *iface);

/** @cond INTERNAL_HIDDEN */

/**
 *  @brief DHCPv4 state name
 *
 *  @internal
 */
const char *net_dhcpv4_state_name(enum net_dhcpv4_state state);

/** @endcond */

/**
 * @brief Return a text representation of the msg_type
 *
 * @param msg_type The msg_type to be converted to text
 * @return A text representation of msg_type
 */
const char *net_dhcpv4_msg_type_name(enum net_dhcpv4_msg_type msg_type);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_DHCPV4_H_ */
