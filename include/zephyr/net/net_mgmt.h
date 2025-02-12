/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Network Management API public header
 */

#ifndef ZEPHYR_INCLUDE_NET_NET_MGMT_H_
#define ZEPHYR_INCLUDE_NET_NET_MGMT_H_

#include <zephyr/sys/__assert.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_event.h>
#include <zephyr/sys/iterable_sections.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Network Management
 * @defgroup net_mgmt Network Management
 * @ingroup networking
 * @{
 */

struct net_if;

/** @cond INTERNAL_HIDDEN */
/**
 * @brief NET MGMT event mask basics, normalizing parts of bit fields
 */
#define NET_MGMT_EVENT_MASK		0x80000000
#define NET_MGMT_ON_IFACE_MASK		0x40000000
#define NET_MGMT_LAYER_MASK		0x30000000
#define NET_MGMT_SYNC_EVENT_MASK	0x08000000
#define NET_MGMT_LAYER_CODE_MASK	0x07FF0000
#define NET_MGMT_COMMAND_MASK		0x0000FFFF

#define NET_MGMT_EVENT_BIT		BIT(31)
#define NET_MGMT_IFACE_BIT		BIT(30)
#define NET_MGMT_SYNC_EVENT_BIT		BIT(27)

#define NET_MGMT_LAYER(_layer)		(_layer << 28)
#define NET_MGMT_LAYER_CODE(_code)	(_code << 16)

#define NET_MGMT_EVENT(mgmt_request)		\
	(mgmt_request & NET_MGMT_EVENT_MASK)

#define NET_MGMT_ON_IFACE(mgmt_request)		\
	(mgmt_request & NET_MGMT_ON_IFACE_MASK)

#define NET_MGMT_EVENT_SYNCHRONOUS(mgmt_request)	\
	(mgmt_request & NET_MGMT_SYNC_EVENT_MASK)

#define NET_MGMT_GET_LAYER(mgmt_request)	\
	((mgmt_request & NET_MGMT_LAYER_MASK) >> 28)

#define NET_MGMT_GET_LAYER_CODE(mgmt_request)	\
	((mgmt_request & NET_MGMT_LAYER_CODE_MASK) >> 16)

#define NET_MGMT_GET_COMMAND(mgmt_request)	\
	(mgmt_request & NET_MGMT_COMMAND_MASK)


/* Useful generic definitions */
#define NET_MGMT_LAYER_L2		1
#define NET_MGMT_LAYER_L3		2
#define NET_MGMT_LAYER_L4		3

/** @endcond */


/**
 * @typedef net_mgmt_request_handler_t
 * @brief Signature which all Net MGMT request handler need to follow
 * @param mgmt_request The exact request value the handler is being called
 *        through
 * @param iface A valid pointer on struct net_if if the request is meant
 *        to be tied to a network interface. NULL otherwise.
 * @param data A valid pointer on a data understood by the handler.
 *        NULL otherwise.
 * @param len Length in byte of the memory pointed by data.
 */
typedef int (*net_mgmt_request_handler_t)(uint32_t mgmt_request,
					  struct net_if *iface,
					  void *data, size_t len);

/**
 * @brief Generate a network management event.
 *
 * @param _mgmt_request Management event identifier
 * @param _iface Network interface
 * @param _data Any additional data for the event
 * @param _len Length of the additional data.
 */
#define net_mgmt(_mgmt_request, _iface, _data, _len)			\
	net_mgmt_##_mgmt_request(_mgmt_request, _iface, _data, _len)

/**
 * @brief Declare a request handler function for the given network event.
 *
 * @param _mgmt_request Management event identifier
 */
#define NET_MGMT_DEFINE_REQUEST_HANDLER(_mgmt_request)			\
	extern int net_mgmt_##_mgmt_request(uint32_t mgmt_request,	\
					    struct net_if *iface,	\
					    void *data, size_t len)

/**
 * @brief Create a request handler function for the given network event.
 *
 * @param _mgmt_request Management event identifier
 * @param _func Function for handling this event
 */
#define NET_MGMT_REGISTER_REQUEST_HANDLER(_mgmt_request, _func)	\
	FUNC_ALIAS(_func, net_mgmt_##_mgmt_request, int)

struct net_mgmt_event_callback;

/**
 * @typedef net_mgmt_event_handler_t
 * @brief Define the user's callback handler function signature
 * @param cb Original struct net_mgmt_event_callback owning this handler.
 * @param mgmt_event The network event being notified.
 * @param iface A pointer on a struct net_if to which the event belongs to,
 *        if it's an event on an iface. NULL otherwise.
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

	union {
		/** Actual callback function being used to notify the owner
		 */
		net_mgmt_event_handler_t handler;
		/** Semaphore meant to be used internally for the synchronous
		 * net_mgmt_event_wait() function.
		 */
		struct k_sem *sync_call;
	};

#ifdef CONFIG_NET_MGMT_EVENT_INFO
	const void *info;
	size_t info_length;
#endif

	/** A mask of network events on which the above handler should be
	 * called in case those events come. Such mask can be modified
	 * whenever necessary by the owner, and thus will affect the handler
	 * being called or not.
	 */
	union {
		/** A mask of network events on which the above handler should
		 * be called in case those events come.
		 * Note that only the command part is treated as a mask,
		 * matching one to several commands. Layer and layer code will
		 * be made of an exact match. This means that in order to
		 * receive events from multiple layers, one must have multiple
		 * listeners registered, one for each layer being listened.
		 */
		uint32_t event_mask;
		/** Internal place holder when a synchronous event wait is
		 * successfully unlocked on a event.
		 */
		uint32_t raised_event;
	};
};

/**
 * @typedef net_mgmt_event_static_handler_t
 * @brief Define the user's callback handler function signature
 * @param mgmt_event The network event being notified.
 * @param iface A pointer on a struct net_if to which the event belongs to,
 *        if it's an event on an iface. NULL otherwise.
 * @param info A valid pointer on a data understood by the handler.
 *        NULL otherwise.
 * @param info_length Length in bytes of the memory pointed by @p info.
 * @param user_data Data provided by the user to the handler.
 */
typedef void (*net_mgmt_event_static_handler_t)(uint32_t mgmt_event,
						struct net_if *iface,
						void *info, size_t info_length,
						void *user_data);

/** @cond INTERNAL_HIDDEN */

/* Structure for event handler registered at compile time */
struct net_mgmt_event_static_handler {
	uint32_t event_mask;
	net_mgmt_event_static_handler_t handler;
	void *user_data;
};

/** @endcond */

/**
 * @brief Define a static network event handler.
 * @param _name Name of the event handler.
 * @param _event_mask A mask of network events on which the passed handler should
 *        be called in case those events come.
 *        Note that only the command part is treated as a mask,
 *        matching one to several commands. Layer and layer code will
 *        be made of an exact match. This means that in order to
 *        receive events from multiple layers, one must have multiple
 *        listeners registered, one for each layer being listened.
 * @param _func The function to be called upon network events being emitted.
 * @param _user_data User data passed to the handler being called on network events.
 */
#define NET_MGMT_REGISTER_EVENT_HANDLER(_name, _event_mask, _func, _user_data)	\
	const STRUCT_SECTION_ITERABLE(net_mgmt_event_static_handler, _name) = {	\
		.event_mask = _event_mask,					\
		.handler = _func,						\
		.user_data = (void *)_user_data,				\
	}

/**
 * @brief Helper to initialize a struct net_mgmt_event_callback properly
 * @param cb A valid application's callback structure pointer.
 * @param handler A valid handler function pointer.
 * @param mgmt_event_mask A mask of relevant events for the handler
 */
#ifdef CONFIG_NET_MGMT_EVENT
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
#else
#define net_mgmt_init_event_callback(...)
#endif

/**
 * @brief Add a user callback
 * @param cb A valid pointer on user's callback to add.
 */
#ifdef CONFIG_NET_MGMT_EVENT
void net_mgmt_add_event_callback(struct net_mgmt_event_callback *cb);
#else
#define net_mgmt_add_event_callback(...)
#endif

/**
 * @brief Delete a user callback
 * @param cb A valid pointer on user's callback to delete.
 */
#ifdef CONFIG_NET_MGMT_EVENT
void net_mgmt_del_event_callback(struct net_mgmt_event_callback *cb);
#else
#define net_mgmt_del_event_callback(...)
#endif

/**
 * @brief Used by the system to notify an event.
 * @param mgmt_event The actual network event code to notify
 * @param iface a valid pointer on a struct net_if if only the event is
 *        based on an iface. NULL otherwise.
 * @param info A valid pointer on the information you want to pass along
 *        with the event. NULL otherwise. Note the data pointed there is
 *        normalized by the related event.
 * @param length size of the data pointed by info pointer.
 *
 * Note: info and length are disabled if CONFIG_NET_MGMT_EVENT_INFO
 *       is not defined.
 */
#if defined(CONFIG_NET_MGMT_EVENT)
void net_mgmt_event_notify_with_info(uint32_t mgmt_event, struct net_if *iface,
				     const void *info, size_t length);
#else
#define net_mgmt_event_notify_with_info(...)
#endif

/**
 * @brief Used by the system to notify an event without any additional information.
 * @param mgmt_event The actual network event code to notify
 * @param iface A valid pointer on a struct net_if if only the event is
 *        based on an iface. NULL otherwise.
 */
#if defined(CONFIG_NET_MGMT_EVENT)
static inline void net_mgmt_event_notify(uint32_t mgmt_event,
					 struct net_if *iface)
{
	net_mgmt_event_notify_with_info(mgmt_event, iface, NULL, 0);
}
#else
#define net_mgmt_event_notify(...)
#endif

/**
 * @brief Used to wait synchronously on an event mask
 * @param mgmt_event_mask A mask of relevant events to wait on.
 * @param raised_event a pointer on a uint32_t to get which event from
 *        the mask generated the event. Can be NULL if the caller is not
 *        interested in that information.
 * @param iface a pointer on a place holder for the iface on which the
 *        event has originated from. This is valid if only the event mask
 *        has bit NET_MGMT_IFACE_BIT set relevantly, depending on events
 *        the caller wants to listen to.
 * @param info a valid pointer if user wants to get the information the
 *        event might bring along. NULL otherwise.
 * @param info_length tells how long the info memory area is. Only valid if
 *        the info is not NULL.
 * @param timeout A timeout delay. K_FOREVER can be used to wait indefinitely.
 *
 * @return 0 on success, a negative error code otherwise. -ETIMEDOUT will
 *         be specifically returned if the timeout kick-in instead of an
 *         actual event.
 */
#ifdef CONFIG_NET_MGMT_EVENT
int net_mgmt_event_wait(uint32_t mgmt_event_mask,
			uint32_t *raised_event,
			struct net_if **iface,
			const void **info,
			size_t *info_length,
			k_timeout_t timeout);
#else
static inline int net_mgmt_event_wait(uint32_t mgmt_event_mask,
				      uint32_t *raised_event,
				      struct net_if **iface,
				      const void **info,
				      size_t *info_length,
				      k_timeout_t timeout)
{
	return 0;
}
#endif

/**
 * @brief Used to wait synchronously on an event mask for a specific iface
 * @param iface a pointer on a valid network interface to listen event to
 * @param mgmt_event_mask A mask of relevant events to wait on. Listened
 *        to events should be relevant to iface events and thus have the bit
 *        NET_MGMT_IFACE_BIT set.
 * @param raised_event a pointer on a uint32_t to get which event from
 *        the mask generated the event. Can be NULL if the caller is not
 *        interested in that information.
 * @param info a valid pointer if user wants to get the information the
 *        event might bring along. NULL otherwise.
 * @param info_length tells how long the info memory area is. Only valid if
 *        the info is not NULL.
 * @param timeout A timeout delay. K_FOREVER can be used to wait indefinitely.
 *
 * @return 0 on success, a negative error code otherwise. -ETIMEDOUT will
 *         be specifically returned if the timeout kick-in instead of an
 *         actual event.
 */
#ifdef CONFIG_NET_MGMT_EVENT
int net_mgmt_event_wait_on_iface(struct net_if *iface,
				 uint32_t mgmt_event_mask,
				 uint32_t *raised_event,
				 const void **info,
				 size_t *info_length,
				 k_timeout_t timeout);
#else
static inline int net_mgmt_event_wait_on_iface(struct net_if *iface,
					       uint32_t mgmt_event_mask,
					       uint32_t *raised_event,
					       const void **info,
					       size_t *info_length,
					       k_timeout_t timeout)
{
	return 0;
}
#endif

/**
 * @brief Used by the core of the network stack to initialize the network
 *        event processing.
 */
#ifdef CONFIG_NET_MGMT_EVENT
void net_mgmt_event_init(void);
#else
#define net_mgmt_event_init(...)
#endif /* CONFIG_NET_MGMT_EVENT */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_NET_MGMT_H_ */
