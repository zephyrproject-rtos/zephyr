/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */


/** @file
 * @brief Event manager header.
 */

#ifndef ZEPHYR_INCLUDE_EVENT_MANAGER_EVENT_MANAGER_H_
#define ZEPHYR_INCLUDE_EVENT_MANAGER_EVENT_MANAGER_H_


/**
 * @defgroup event_manager Event Manager
 * @brief Event Manager
 *
 * @{
 */

#include <zephyr.h>
#include <zephyr/types.h>
#include <sys/reboot.h>
#include <sys/__assert.h>
#include <logging/log_ctrl.h>

#include <event_manager/event_manager_priv.h>

#ifdef __cplusplus
extern "C" {
#endif


/** @def SUBS_PRIO_MIN
 *
 * @brief Index of the highest subscriber priority level.
 */
#define SUBS_PRIO_MIN    _SUBS_PRIO_FIRST


/** @def SUBS_PRIO_MAX
 *
 * @brief Index of the lowest subscriber priority level.
 */
#define SUBS_PRIO_MAX    _SUBS_PRIO_FINAL


/** @def SUBS_PRIO_COUNT
 *
 * @brief Number of subscriber priority levels.
 */
#define SUBS_PRIO_COUNT (SUBS_PRIO_MAX - SUBS_PRIO_MIN + 1)


/** @brief Event header.
 *
 * When defining an event structure, the event header
 * must be placed as the first field.
 */
struct event_header {
	/** Linked list node used to chain events. */
	sys_snode_t node;

	/** Pointer to the event type object. */
	const struct event_type *type_id;
};


/** @brief Dynamic event data.
 *
 * When defining an event structure, the dynamic event data
 * must be placed as the last field.
 */
struct event_dyndata {
	/** Size of the dynamic data. */
	size_t size;

	/** Dynamic data. */
	uint8_t data[0];
};


/** @brief Event listener.
 *
 * All event listeners must be defined using @ref EVENT_LISTENER.
 */
struct event_listener {
	/** Name of this listener. */
	const char *name;

	/** Pointer to the function that is called when an event
	 *  is handled.
	 */
	bool (*notification)(const struct event_header *eh);
};


/** @brief Event subscriber.
 */
struct event_subscriber {
	/** Pointer to the listener. */
	const struct event_listener *listener;
};


/** @brief Event type.
 */
struct event_type {
	/** Event name. */
	const char			*name;

	/** Array of pointers to the array of subscribers. */
	const struct event_subscriber	*subs_start[SUBS_PRIO_COUNT];

	/** Array of pointers to the element directly after the array of
	 * subscribers.
	 */
	const struct event_subscriber	*subs_stop[SUBS_PRIO_COUNT];

	/** Bool indicating if the event is logged by default. */
	bool init_log_enable;

	/** Function to log data from this event. */
	int (*log_event)(const struct event_header *eh, char *buf,
			      size_t buf_len);

	/** Custom data related to tracking. */
	const void *trace_data;
};


extern const struct event_listener __start_event_listeners[];
extern const struct event_listener __stop_event_listeners[];

extern const struct event_type __start_event_types[];
extern const struct event_type __stop_event_types[];


/** Create an event listener object.
 *
 * @param lname   Module name.
 * @param cb_fn  Pointer to the event handler function.
 */
#define EVENT_LISTENER(lname, cb_fn) _EVENT_LISTENER(lname, cb_fn)

/** Subscribe a listener to an event type as first module that is
 *  being notified.
 *
 * @param lname  Name of the listener.
 * @param ename  Name of the event.
 */
#define EVENT_SUBSCRIBE_FIRST(lname, ename)							\
	_EVENT_SUBSCRIBE(lname, ename, _EM_MARKER_FIRST_ELEMENT);				\
	const struct {} _CONCAT(_CONCAT(__event_subscriber_, ename), first_sub_redefined) = {}



/** Subscribe a listener to the early notification list for an
 *  event type.
 *
 * @param lname  Name of the listener.
 * @param ename  Name of the event.
 */
#define EVENT_SUBSCRIBE_EARLY(lname, ename) \
	_EVENT_SUBSCRIBE(lname, ename, _SUBS_PRIO_ID(_SUBS_PRIO_FIRST))


/** Subscribe a listener to the normal notification list for an event
 *  type.
 *
 * @param lname  Name of the listener.
 * @param ename  Name of the event.
 */
#define EVENT_SUBSCRIBE(lname, ename) \
	_EVENT_SUBSCRIBE(lname, ename, _SUBS_PRIO_ID(_SUBS_PRIO_NORMAL))


/** Subscribe a listener to an event type as final module that is
 *  being notified.
 *
 * @param lname  Name of the listener.
 * @param ename  Name of the event.
 */
#define EVENT_SUBSCRIBE_FINAL(lname, ename)							\
	_EVENT_SUBSCRIBE(lname, ename, _SUBS_PRIO_ID(_SUBS_PRIO_FINAL));			\
	const struct {} _CONCAT(_CONCAT(__event_subscriber_, ename), final_sub_redefined) = {}


/** Declare an event type.
 *
 * This macro provides declarations required for an event to be used
 * by other modules.
 *
 * @param ename  Name of the event.
 */
#define EVENT_TYPE_DECLARE(ename) _EVENT_TYPE_DECLARE(ename)


/** Declare an event type with dynamic data size.
 *
 * This macro provides declarations required for an event to be used
 * by other modules.
 * Declared event will use dynamic data.
 *
 * @param ename  Name of the event.
 */
#define EVENT_TYPE_DYNDATA_DECLARE(ename) _EVENT_TYPE_DYNDATA_DECLARE(ename)


/** Define an event type.
 *
 * This macro defines an event type. In addition, it defines functions
 * specific to the event type and the event type structure.
 *
 * For every defined event, the following functions are created, where
 * <i>%event_type</i> is replaced with the given event type name @p ename
 * (for example, button_event):
 * - new_<i>%event_type</i>  - Allocates an event of a given type.
 * - is_<i>%event_type</i>   - Checks if the event header that is provided
 *                            as argument represents the given event type.
 * - cast_<i>%event_type</i> - Casts the event header that is provided
 *                            as argument to an event of the given type.
 *
 * @param ename		   Name of the event.
 * @param init_log_en	   Bool indicating if the event is logged
 *                         by default.
 * @param log_fn	   Function to stringify an event of this type.
 * @param ev_info_struct   Data structure describing the event type.
 */
#define EVENT_TYPE_DEFINE(ename, init_log_en, log_fn, ev_info_struct) \
	_EVENT_TYPE_DEFINE(ename, init_log_en, log_fn, ev_info_struct)


/** Verify if an event ID is valid.
 *
 * The pointer to an event type structure is used as its ID. This macro
 * validates that the provided pointer is within the range where event
 * type structures are defined.
 *
 * @param id  ID.
 */
#define ASSERT_EVENT_ID(id) \
	__ASSERT_NO_MSG((id >= __start_event_types) && (id < __stop_event_types))


/** Submit an event to the Event Manager.
 *
 * @param eh  Pointer to the event header element in the event object.
 */
void _event_submit(struct event_header *eh);


/** Submit an event.
 *
 * This helper macro simplifies the event submission.
 *
 * @param event  Pointer to the event object.
 */
#define EVENT_SUBMIT(event) _event_submit(&event->header)


/** Initialize the Event Manager.
 *
 * @retval 0 If the operation was successful.
 */
int event_manager_init(void);


/** Trace event execution.
 * The behavior of this function depends on the actual implementation.
 * The default implementation of this function is no-operation.
 * It is annotated as weak and is meant to be overridden by layer
 * adding support for profiling mechanism.
 **/
void event_manager_trace_event_execution(const struct event_header *eh,
				  bool is_start);


/** Trace event submission.
 * The behavior of this function depends on the actual implementation.
 * The default implementation of this function is no-operation.
 * It is annotated as weak and is meant to be overridden by layer
 * adding support for profiling mechanism.
 **/
void event_manager_trace_event_submission(const struct event_header *eh,
				const void *trace_info);


/** Initialize tracing in the Event Manager.
 * The behavior of this function depends on the actual implementation.
 * The default implementation of this function is no-operation.
 * It is annotated as weak and is meant to be overridden by layer
 * adding support for profiling mechanism.
 **/
int event_manager_trace_event_init(void);

/** Allocate event.
 * The behavior of this function depends on the actual implementation.
 * The default implementation of this function is same as k_malloc.
 * It is annotated as weak and can be overridden by user.
 *
 * @param size  Amount of memory requested (in bytes).
 * @retval Address of the allocated memory if successful, otherwise NULL.
 **/
void *event_manager_alloc(size_t size);


/** Free memory occupied by the event.
 * The behavior of this function depends on the actual implementation.
 * The default implementation of this function is same as k_free.
 * It is annotated as weak and can be overridden by user.
 *
 * @param addr  Pointer to previously allocated memory.
 **/
void event_manager_free(void *addr);


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_EVENT_MANAGER_EVENT_MANAGER_H_ */
