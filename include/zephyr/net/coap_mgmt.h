/*
 * Copyright (c) 2023 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief CoAP Events code public header
 */

#ifndef ZEPHYR_INCLUDE_NET_COAP_MGMT_H_
#define ZEPHYR_INCLUDE_NET_COAP_MGMT_H_

#include <zephyr/net/net_mgmt.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CoAP Manager Events
 * @defgroup coap_mgmt CoAP Manager Events
 * @since 3.6
 * @version 0.1.0
 * @ingroup networking
 * @{
 */

/** @cond INTERNAL_HIDDEN */

/* CoAP events */
#define _NET_COAP_LAYER		NET_MGMT_LAYER_L4
#define _NET_COAP_CODE		0x1c0
#define _NET_COAP_IF_BASE	(NET_MGMT_EVENT_BIT |			\
				 NET_MGMT_LAYER(_NET_COAP_LAYER) |	\
				 NET_MGMT_LAYER_CODE(_NET_COAP_CODE))

struct coap_service;
struct coap_resource;
struct coap_observer;

enum net_event_coap_cmd {
	/* Service events */
	NET_EVENT_COAP_CMD_SERVICE_STARTED = 1,
	NET_EVENT_COAP_CMD_SERVICE_STOPPED,
	/* Observer events */
	NET_EVENT_COAP_CMD_OBSERVER_ADDED,
	NET_EVENT_COAP_CMD_OBSERVER_REMOVED,
};

/** @endcond */

/**
 * @brief coap_mgmt event raised when a service has started
 */
#define NET_EVENT_COAP_SERVICE_STARTED			\
	(_NET_COAP_IF_BASE | NET_EVENT_COAP_CMD_SERVICE_STARTED)

/**
 * @brief coap_mgmt event raised when a service has stopped
 */
#define NET_EVENT_COAP_SERVICE_STOPPED			\
	(_NET_COAP_IF_BASE | NET_EVENT_COAP_CMD_SERVICE_STOPPED)

/**
 * @brief coap_mgmt event raised when an observer has been added to a resource
 */
#define NET_EVENT_COAP_OBSERVER_ADDED			\
	(_NET_COAP_IF_BASE | NET_EVENT_COAP_CMD_OBSERVER_ADDED)

/**
 * @brief coap_mgmt event raised when an observer has been removed from a resource
 */
#define NET_EVENT_COAP_OBSERVER_REMOVED			\
	(_NET_COAP_IF_BASE | NET_EVENT_COAP_CMD_OBSERVER_REMOVED)

/**
 * @brief CoAP Service event structure.
 */
struct net_event_coap_service {
	/** The CoAP service for which the event is emitted */
	const struct coap_service *service;
};

/**
 * @brief CoAP Observer event structure.
 */
struct net_event_coap_observer {
	/** The CoAP resource for which the event is emitted */
	struct coap_resource *resource;
	/** The observer that is added/removed */
	struct coap_observer *observer;
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_COAP_MGMT_H_ */
