/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __OPENTHREAD_BORDER_ROUTER_H__
#define __OPENTHREAD_BORDER_ROUTER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <openthread/backbone_router_ftd.h>
#include <openthread/ip6.h>
#include <openthread/platform/mdns_socket.h>
#include <openthread/udp.h>
#include <zephyr/net/openthread.h>

#define VENDOR_NAME		   CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_VENDOR_NAME "#0000"
#define BASE_SERVICE_INSTANCE_NAME CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_BASE_SERVICE_NAME "#0000"

extern char host_name[], base_service_instance_name[];

/**
 * @brief The callback type for notifying a subscription to an IPv6 multicast
 * address This callback is called when backbone router notifies a
 * subscription/de-registration.
 *
 * @param context The context to pass to the callback.
 * @param event The multicast listener event.
 * @param address The IPv6 address of the multicast listener.
 */
typedef void (*openthread_bbr_multicast_listener_cb)(void *context,
						     otBackboneRouterMulticastListenerEvent event,
						     const otIp6Address *address);

/**
 * @brief Set the additional callback for OpenThread multicast listener
 * subscription.
 *
 * @details This callback is called once a subscription to an IPv6 multicast
 * address is performed. Setting this callback is optional.
 *
 * @param cb Callback to set.
 * @param context Context to pass to the callback.
 */
void openthread_set_bbr_multicast_listener_cb(openthread_bbr_multicast_listener_cb cb,
					      void *context);

/**
 * @brief Initializes the Border Router application.
 *
 * @details This function registers required AIL and OpenThread callbacks.
 *
 * @param ot_ctx Pointer to OpenThread context.
 */
void openthread_border_router_init(struct openthread_context *ot_ctx);

/**
 * @brief Returns OpenThread's IPv6 slaac address.
 *
 * @param ot_ctx Pointer to OpenThread instance.
 */
const otIp6Address *get_ot_slaac_address(otInstance *instance);

/**
 * @brief Generic structure which holds a message to be parsed on OpenThread context.
 *
 */

struct otbr_msg_ctx;
typedef void (*br_msg_callback)(struct otbr_msg_ctx *msg_ctx_ptr);

struct otbr_msg_ctx {
	/** Used for fifo word boundary alignment */
	void *unused;

	/** Callback to function to be executed in OT context */
	br_msg_callback cb;

	/** OpenThread stack socket */
	otUdpSocket *socket;

	/** Buffer which holds incoming data */
	uint8_t buffer[1500];

	/** Actual length of incoming data */
	uint16_t length;

	union {
		/** Generic OpenThread message info structure */
		otMessageInfo message_info;

		/** OpenThread mDNS address info structure */
		otPlatMdnsAddressInfo addr_info;

		/** Generic OpenThread IPv6 address structure */
		otIp6Address addr;
	};
};

/**
 * @brief Posts a message received on backbone interface to be parsed in OT context.
 *
 * @param msg_context Pointer to contained message.
 */
void openthread_border_router_post_message(struct otbr_msg_ctx *msg_context);

/**
 * @brief Creates base name for mDNS host or for a service associated with this host.
 *
 * @param ot_instance Pointer to OpenThread instance.
 * @param base_name Pointer to provided base name from which final name will be created.
 * @param is_for_service Specifies is name creation is desired for host or service.
 */
const char *create_base_name(otInstance *ot_instance, char *base_name, bool is_for_service);

/**
 * @brief Creates an alternative base name in case of conflict.
 *
 * @param base_name Pointer to name which has created conflict.
 */
const char *create_alternative_base_name(char *base_name);

#ifdef __cplusplus
}
#endif

#endif /* __OPENTHREAD_BORDER_ROUTER_H__ */
