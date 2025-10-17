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
#include <zephyr/kernel.h>

#define OTBR_VENDOR_NAME		CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_VENDOR_NAME "#0000"
#define OTBR_BASE_SERVICE_INSTANCE_NAME CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_BASE_SERVICE_NAME
#define OTBR_MODEL_NAME			CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_MODEL_NAME

#define OTBR_MESSAGE_SIZE 1500

extern char otbr_base_service_instance_name[], otbr_vendor_name[], otbr_model_name[];

extern struct k_work openthread_border_router_work;

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
	uint8_t buffer[OTBR_MESSAGE_SIZE];

	/** Actual length of incoming data */
	uint16_t length;

	/** A pointer to user data */
	void *user_data;
	union {
		/** Generic OpenThread message info structure */
		otMessageInfo message_info;

		/** OpenThread mDNS address info structure */
		otPlatMdnsAddressInfo addr_info;

		/** Generic OpenThread IPv6 address structure */
		otIp6Address addr;

		/** OpenThread IPv6 socket address structure */
		otSockAddr sock_addr;
	};
};

/**
 * @brief Allocates a message data structure on border router's message memory slab area.
 *
 * @param msg Pointer to block address area of border router's message memory slab.
 */
int openthread_border_router_allocate_message(void **msg);

/**
 * @brief Deallocates a message which was posted to border router's message memory slab.
 *
 * @param msg Pointer to block address area of border router's message memory slab.
 */
void openthread_border_router_deallocate_message(void *msg);

/**
 * @brief Posts a message received on backbone interface to be parsed in OT context.
 *
 * @param msg_context Pointer to contained message.
 */
void openthread_border_router_post_message(struct otbr_msg_ctx *msg_context);

/**
 * @brief Verifies if a packet respects the imposed OpenThread forwarding rules
 * between Backbone and Thread interface.
 *
 * @param pkt Pointer to packet that will be checked.
 */
bool openthread_border_router_check_packet_forwarding_rules(struct net_pkt *pkt);

#ifdef __cplusplus
}
#endif

#endif /* __OPENTHREAD_BORDER_ROUTER_H__ */
