/*
 * Copyright (c) 2024 Vogl Electronic GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief hawkBit event header file
 */

/**
 * @brief hawkBit event API.
 * @defgroup hawkbit_event hawkBit event API
 * @ingroup hawkbit
 * @{
 */

#ifndef ZEPHYR_INCLUDE_MGMT_HAWKBIT_EVENT_H_
#define ZEPHYR_INCLUDE_MGMT_HAWKBIT_EVENT_H_

#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>

/**
 * @brief hawkBit event type.
 *
 * @details These events are used to register the callback functions.
 *
 */
enum hawkbit_event_type {
	/** Event triggered when there was an error */
	HAWKBIT_EVENT_ERROR,
	/** Event triggered when there was a networking error */
	HAWKBIT_EVENT_ERROR_NETWORKING,
	/** Event triggered when there was a permission error */
	HAWKBIT_EVENT_ERROR_PERMISSION,
	/** Event triggered when there was a metadata error */
	HAWKBIT_EVENT_ERROR_METADATA,
	/** Event triggered when there was a download error */
	HAWKBIT_EVENT_ERROR_DOWNLOAD,
	/** Event triggered when there was an allocation error */
	HAWKBIT_EVENT_ERROR_ALLOC,
	/** Event triggered when a new update was downloaded */
	HAWKBIT_EVENT_UPDATE_DOWNLOADED,
	/** Event triggered when there is no update available */
	HAWKBIT_EVENT_NO_UPDATE,
	/** Event triggered when the update was canceled by the server */
	HAWKBIT_EVENT_CANCEL_UPDATE,
	/** Event triggered before the download starts */
	HAWKBIT_EVENT_START_DOWNLOAD,
	/** Event triggered after the download ends */
	HAWKBIT_EVENT_END_DOWNLOAD,
	/** Event triggered before the hawkBit run starts */
	HAWKBIT_EVENT_START_RUN,
	/** Event triggered after the hawkBit run ends */
	HAWKBIT_EVENT_END_RUN,
	/** Event triggered before hawkBit does a reboot */
	HAWKBIT_EVENT_BEFORE_REBOOT,
};

struct hawkbit_event_callback;

/**
 * @typedef hawkbit_event_callback_handler_t
 * @brief Define the application callback handler function signature
 *
 * @param cb Original struct hawkbit_event_callback owning this handler
 * @param event The event that triggered the callback
 *
 * Note: cb pointer can be used to retrieve private data through
 * CONTAINER_OF() if original struct hawkbit_event_callback is stored in
 * another private structure.
 */
typedef void (*hawkbit_event_callback_handler_t)(struct hawkbit_event_callback *cb,
						 enum hawkbit_event_type event);

/** @cond INTERNAL_HIDDEN */

/**
 * @brief hawkBit callback structure
 *
 * Used to register a callback in the hawkBit callback list.
 * As many callbacks as needed can be added as long as each of them
 * are unique pointers of struct hawkbit_event_callback.
 * Beware such structure should not be allocated on stack.
 *
 * Note: To help setting it, see hawkbit_event_init_callback() below
 */
struct hawkbit_event_callback {
	/** This is meant to be used internally and the user should not mess with it. */
	sys_snode_t node;

	/** Actual callback function being called when relevant. */
	hawkbit_event_callback_handler_t handler;

	/** The event type this callback is attached to. */
	enum hawkbit_event_type event;
};

/** @endcond */


/**
 * @brief Macro to create and initialize a struct hawkbit_event_callback properly.
 *
 * @details This macro can be used instead of hawkbit_event_init_callback().
 *
 * @param _callback Name of the  callback structure
 * @param _handler  A valid handler function pointer.
 * @param _event    The event of ::hawkbit_event_type that will trigger the callback
 */
#define HAWKBIT_EVENT_CREATE_CALLBACK(_callback, _handler, _event)                                 \
	struct hawkbit_event_callback _callback = {                                                \
		.handler = _handler,                                                               \
		.event = _event,                                                                   \
	}

/**
 * @brief Helper to initialize a struct hawkbit_event_callback properly
 *
 * @param callback A valid Application's callback structure pointer.
 * @param handler A valid handler function pointer.
 * @param event The event of ::hawkbit_event_type that will trigger the callback.
 */
static inline void hawkbit_event_init_callback(struct hawkbit_event_callback *callback,
					       hawkbit_event_callback_handler_t handler,
					       enum hawkbit_event_type event)
{
	__ASSERT(callback, "Callback pointer should not be NULL");
	__ASSERT(handler, "Callback handler pointer should not be NULL");

	callback->handler = handler;
	callback->event = event;
}

/**
 * @brief Add an application callback.
 *
 * @param cb A valid application's callback structure pointer.
 *
 * @return 0 if successful, negative errno code on failure.
 */
int hawkbit_event_add_callback(struct hawkbit_event_callback *cb);

/**
 * @brief Remove an application callback.
 *
 * @param cb A valid application's callback structure pointer.
 *
 * @return 0 if successful, negative errno code on failure.
 */
int hawkbit_event_remove_callback(struct hawkbit_event_callback *cb);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_MGMT_HAWKBIT_EVENT_H_ */
