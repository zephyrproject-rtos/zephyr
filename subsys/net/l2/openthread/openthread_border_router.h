/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <openthread/backbone_router_ftd.h>
#include <openthread/ip6.h>

/**
 * @brief The callback type for notifying a subscription to an IPv6 multicast address
 * This callback is called when backbone router notifies a subscription/de-registration.
 *
 * @param context The context to pass to the callback.
 * @param event The multicast listener event.
 * @param address The IPv6 address of the multicast listener.
 */
typedef void (*openthread_bbr_multicast_listener_cb)(void *context,
						     otBackboneRouterMulticastListenerEvent event,
						     const otIp6Address *address);

/**
 * @brief Set the additional callback for OpenThread multicast listener subscription.
 *
 * @details This callback is called once a subscription to an IPv6 multicast address is performed.
 * Setting this callback is optional.
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
