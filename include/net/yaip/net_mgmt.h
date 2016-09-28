/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 * @brief Network Management API public header
 */

#ifndef __NET_MGMT_H__
#define __NET_MGMT_H__

#include <net/net_if.h>

/**
 * @brief NET MGMT event mask basics, normalizing parts of bit fields
 */
#define NET_MGMT_EVENT_MASK		0x80000000
#define NET_MGMT_ON_IFACE_MASK		0x40000000
#define NET_MGMT_LAYER_MASK		0x30000000
#define NET_MGMT_LAYER_CODE_MASK	0x0FFF0000
#define NET_MGMT_COMMAND_MASK		0x0000FFFF

#define NET_MGMT_EVENT(mgmt_request)		\
	(mgmt_request & NET_MGMT_EVENT_MASK)

#define NET_MGMT_ON_IFACE(mgmt_request)		\
	(mgmt_request & NET_MGMT_ON_IFACE_MASK)

#define NET_MGMT_GET_LAYER(mgmt_request)	\
	(mgmt_request & NET_MGMT_LAYER_MASK)

#define NET_MGMT_GET_LAYER_CODE(mgmt_request)	\
	(mgmt_request & NET_MGMT_LAYER_CODE_MASK)

#define NET_MGMT_GET_COMMAND(mgmt_request)	\
	(mgmt_request & NET_MGMT_COMMAND_MASK)

/**
 * @brief Signature which all Net MGMT request handler need to follow
 * @param mgmt_request The exact request value the handler is being called
 *        through
 * @param iface A valid pointer on struct net_if if the request is meant
 *        to be tight to a network interface. NULL otherwise.
 * @param data A valid pointer on a data understood by the handler.
 *        NULL otherwise.
 * @param len Length in byte of the memory pointed by data.
 */
typedef int (*net_mgmt_request_handler_t)(uint32_t mgmt_request,
					  struct net_if *iface,
					  void *data, size_t len);

#define net_mgmt(_mgmt_request, _iface, _data, _len)			\
	net_mgmt_##_mgmt_request(_mgmt_request, _iface, _data, _len)

#define NET_MGMT_REGISTER_REQUEST_HANDLER(_mgmt_request, _func)	\
	FUNC_ALIAS(_func, net_mgmt_##_mgmt_request, int)

struct net_mgmt_event_callback;

/**
 * @typedef net_mgmt_event_handler_t
 * @brief Define the user's callback handler function signature
 * @param "struct net_mgmt_event_callback *cb"
 *        Original struct net_mgmt_event_callback owning this handler.
 * @param "uint32_t mgmt_event" The network event being notified.
 * @param "struct net_if *iface" A pointer on a struct net_if to which the
 *        the event belongs to, if it's an event on an iface. NULL otherwise.
 */
typedef void (*net_mgmt_event_handler_t)(struct net_mgmt_event_callback *cb,
					 uint32_t mgmt_event,
					 struct net_if *iface);

/**
 * @brief Network Management event callback structure
 * Used to register a callback into the network management event part, in order
 * to let the owner of this struct to get network event notification based on
 * given event mask.
 */
struct net_mgmt_event_callback {
	/** Meant to be used internally, to insert the callback into a list.
	 * So nobody should mess with it.
	 */
	sys_snode_t node;

	/** Actual callback function being used to notify the owner
	 */
	net_mgmt_event_handler_t handler;

	/** A mask of network events on which the above handler should be
	 * called in case those events come. Such mask can be modified
	 * whenever necessary by the owner, and thus will affect the handler
	 * being called or not.
	 */
	uint32_t event_mask;
};

/**
 * @brief Helper to initialize a struct net_mgmt_event_callback properly
 * @param cb A valid application's callback structure pointer.
 * @param handler A valid handler function pointer.
 * @param mgmt_event_mask A mask of relevant events for the handler
 */
static inline
void net_mgmt_init_event_callback(struct net_mgmt_event_callback *cb,
				  net_mgmt_event_handler_t handler,
				  uint32_t mgmt_event_mask)
{
	__ASSERT(cb, "Callback pointer should not be NULL");
	__ASSERT(handler, "Handler pointer should not be NULL");

	cb->handler = handler;
	cb->event_mask = mgmt_event_mask;
};

/**
 * @brief Add a user callback
 * @param cb A valid pointer on user's callback to add.
 */
void net_mgmt_add_event_callback(struct net_mgmt_event_callback *cb);

/**
 * @brief Delete a user callback
 * @param cb A valid pointer on user's callback to delete.
 */
void net_mgmt_del_event_callback(struct net_mgmt_event_callback *cb);

#ifdef CONFIG_NET_MGMT_EVENT
/**
 * @brief Used by the system to notify an event.
 * @param mgmt_event The actual network event code to notify
 * @param iface a valid pointer on a struct net_if if only the event is
 *        based on an iface. NULL otherwise.
 */
void net_mgmt_event_notify(uint32_t mgmt_event, struct net_if *iface);

/**
 * @brief Used by the core of the network stack to initialize the network
 *        event processing.
 */
void net_mgmt_event_init(void);
#else
#define net_mgmt_event_notify(...)
#define net_mgmt_event_init(...)
#endif /* CONFIG_NET_MGMT_EVENT */

#endif /* __NET_MGMT_H__ */
