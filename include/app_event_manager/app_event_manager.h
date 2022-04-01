/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Application Event Manager header.
 */

#ifndef ZEPHYR_INCLUDE_APP_EVENT_MANAGER_EVENT_MANAGER_H_
#define ZEPHYR_INCLUDE_APP_EVENT_MANAGER_EVENT_MANAGER_H_

/**
 * @defgroup app_event_manager Application Event Manager
 * @brief Application Event Manager
 *
 * @{
 */

#include <zephyr.h>
#include <zephyr/types.h>
#include <sys/__assert.h>
#include <logging/log.h>

#include <app_event_manager/app_event_manager_priv.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Pointer to the event handler function.
 *
 * @param aeh  Pointer to the application event header of the event that is
 * processed by app_event_manager.
 * @retval    True if event was consumed and should not be propagated to other listeners,
 *            false otherwise.
 */
typedef bool (*cb_fn)(const struct app_event_header *aeh);
/**
 * @brief List of bits in event type flags.
 */
enum app_event_type_flags {
	/* Flags set internally by Application Event Manager. */
	APP_EVENT_TYPE_FLAGS_SYSTEM_START,
	APP_EVENT_TYPE_FLAGS_HAS_DYNDATA = APP_EVENT_TYPE_FLAGS_SYSTEM_START,

	/* Flags available for user. */
	APP_EVENT_TYPE_FLAGS_USER_SETTABLE_START,
	APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE =
		APP_EVENT_TYPE_FLAGS_USER_SETTABLE_START,

	/* Number of predefined flags. */
	APP_EVENT_TYPE_FLAGS_COUNT,

	/* Value greater or equal are user-specific. */
	APP_EVENT_TYPE_FLAGS_USER_DEFINED_START = APP_EVENT_TYPE_FLAGS_COUNT,
};

/** @brief Get event type flag's value.
 *
 * @param flag Selected event type flag.
 * @param et   Pointer to the event type.
 * @retval Boolean value of requested flag.
 */
static inline bool app_event_get_type_flag(const struct event_type *et,
					   enum app_event_type_flags flag)
{
	return (et->flags & BIT(flag)) != 0;
}

/** @brief Create an event listener object.
 *
 * @param lname   Listener name.
 * @param cb_fn  Pointer to the event handler function.
 */
#define APP_EVENT_LISTENER(lname, cb_fn) Z_APP_EVENT_LISTENER(lname, cb_fn)

/** @brief Subscribe a listener to an event type as first module that is
 *  being notified.
 *
 * @param lname  Name of the listener.
 * @param ename  Name of the event.
 */
#define APP_EVENT_SUBSCRIBE_FIRST(lname, ename)						\
	Z_APP_EVENT_SUBSCRIBE(lname, ename, Z_APP_EM_MARKER_FIRST_ELEMENT);		\
	const struct {} _CONCAT(_CONCAT(__event_subscriber_, ename), first_sub_redefined) = {}

/** @brief Subscribe a listener to the early notification list for an
 *  event type.
 *
 * @param lname  Name of the listener.
 * @param ename  Name of the event.
 */
#define APP_EVENT_SUBSCRIBE_EARLY(lname, ename) \
	Z_APP_EVENT_SUBSCRIBE(lname, ename, Z_APP_EM_SUBS_PRIO_ID(Z_APP_EM_SUBS_PRIO_EARLY))

/** @brief Subscribe a listener to the normal notification list for an event
 *  type.
 *
 * @param lname  Name of the listener.
 * @param ename  Name of the event.
 */
#define APP_EVENT_SUBSCRIBE(lname, ename) \
	Z_APP_EVENT_SUBSCRIBE(lname, ename, Z_APP_EM_SUBS_PRIO_ID(Z_APP_EM_SUBS_PRIO_NORMAL))

/** @brief Subscribe a listener to an event type as final module that is
 *  being notified.
 *
 * @param lname  Name of the listener.
 * @param ename  Name of the event.
 */
#define APP_EVENT_SUBSCRIBE_FINAL(lname, ename)						\
	Z_APP_EVENT_SUBSCRIBE(lname, ename, Z_APP_EM_MARKER_FINAL_ELEMENT);			\
	const struct {} _CONCAT(_CONCAT(__event_subscriber_, ename), final_sub_redefined) = {}

/** @brief Declare an event type.
 *
 * This macro provides declarations required for an event to be used
 * by other modules.
 *
 * @param ename  Name of the event.
 */
#define APP_EVENT_TYPE_DECLARE(ename) Z_APP_EVENT_TYPE_DECLARE(ename)

/** @brief Declare an event type with dynamic data size.
 *
 * This macro provides declarations required for an event to be used
 * by other modules.
 * Declared event will use dynamic data.
 *
 * @param ename  Name of the event.
 */
#define APP_EVENT_TYPE_DYNDATA_DECLARE(ename) Z_APP_EVENT_TYPE_DYNDATA_DECLARE(ename)

/** @brief Define an event type.
 *
 * This macro defines an event type. In addition, it defines functions
 * specific to the event type and the event type structure.
 *
 * For every defined event, the following functions are created, where
 * <i>%event_type</i> is replaced with the given event type name @p ename
 * (for example, button_event):
 * - new_<i>%event_type</i>  - Allocates an event of a given type.
 * - is_<i>%event_type</i>   - Checks if the application event header that is provided
 *                            as argument represents the given event type.
 * - cast_<i>%event_type</i> - Casts the application event header that is provided
 *                            as argument to an event of the given type.
 *
 * @param ename     	   Name of the event.
 * @param log_fn  	   Function to stringify an event of this type.
 * @param ev_info_struct   Data structure describing the event type.
 * @param app_event_type_flags Event type flags.
 *                         You should use APP_EVENT_FLAGS_CREATE to define them.
 */
#define APP_EVENT_TYPE_DEFINE(ename, log_fn, ev_info_struct, app_event_type_flags) \
	Z_APP_EVENT_TYPE_DEFINE(ename, log_fn, ev_info_struct, app_event_type_flags)

/** @brief Verify if an event ID is valid.
 *
 * The pointer to an event type structure is used as its ID. This macro
 * validates that the provided pointer is within the range where event
 * type structures are defined.
 *
 * @param id  ID.
 */
#define APP_EVENT_ASSERT_ID(id) \
	__ASSERT_NO_MSG((id >= _event_type_list_start) && (id < _event_type_list_end))

/** @brief Submit an event.
 *
 * This helper macro simplifies the event submission.
 *
 * @param event  Pointer to the event object.
 */
#define APP_EVENT_SUBMIT(event) _event_submit(&event->header)

/**
 * @brief Register event hook after the Application Event Manager is initialized.
 *
 * The event hook called after the app_event_manager is initialized to provide some additional
 * initialization of the modules that depends on it.
 * The hook function should have a form `int hook(void)`.
 * If the initialization hook returns non-zero value the initialization process is interrupted.
 *
 * @param hook_fn Hook function.
 */
#define APP_EVENT_MANAGER_HOOK_POSTINIT_REGISTER(hook_fn)	\
	Z_APP_EVENT_MANAGER_HOOK_POSTINIT_REGISTER(hook_fn,	\
	Z_APP_EM_SUBS_PRIO_ID(Z_APP_EM_SUBS_PRIO_NORMAL))

/**
 * @brief Get the event size
 *
 * Function that calculates the event size using its header.
 * @note
 * For this function to be available the
 * @kconfig{CONFIG_APP_EVENT_MANAGER_PROVIDE_EVENT_SIZE} option needs to be enabled.
 *
 * @param aeh Pointer to the application event header.
 *
 * @return Event size in bytes.
 */
static inline size_t app_event_manager_event_size(const struct app_event_header *aeh)
{
#if IS_ENABLED(CONFIG_APP_EVENT_MANAGER_PROVIDE_EVENT_SIZE)
	size_t size = aeh->type_id->struct_size;

	if (app_event_get_type_flag(aeh->type_id, APP_EVENT_TYPE_FLAGS_HAS_DYNDATA)) {
		size += ((const struct event_dyndata *)
			 (((const uint8_t *)aeh) + size - sizeof(struct event_dyndata)))->size;
	}
	return size;
#else
	__ASSERT_NO_MSG(false);
	return 0;
#endif
}


/**
 * @brief Register hook called on event submission. The hook would be called first.
 *
 * The event hook called when the event is submitted.
 * The hook function should have a form `void hook(const struct app_event_header *aeh)`.
 * The macro makes sure that the hook provided here is called first.
 * Only one hook can be registered with this macro.
 *
 * @note
 * The registered hook may be called from many contexts.
 * To ensure that order of events in the queue matches the order of the registered callbacks calls,
 * the callbacks are called under the same spinlock as adding events to the queue.
 *
 * @param hook_fn Hook function.
 */
#define APP_EVENT_HOOK_ON_SUBMIT_REGISTER_FIRST(hook_fn)                      \
	const struct {} __event_hook_on_submit_first_sub_redefined = {};  \
	Z_APP_EVENT_HOOK_ON_SUBMIT_REGISTER(hook_fn, Z_APP_EM_MARKER_FIRST_ELEMENT)

/**
 * @brief Register event hook on submission.
 *
 * The event hook called when the event is submitted.
 * The hook function should have a form `void hook(const struct app_event_header *aeh)`.
 *
 * @note
 * The registered hook may be called from many contexts.
 * To ensure that order of events in the queue matches the order of the registered callbacks calls,
 * the callbacks are called under the same spinlock as adding events to the queue.
 *
 * @param hook_fn Hook function.
 */
#define APP_EVENT_HOOK_ON_SUBMIT_REGISTER(hook_fn) \
	Z_APP_EVENT_HOOK_ON_SUBMIT_REGISTER(hook_fn, Z_APP_EM_SUBS_PRIO_ID(Z_APP_EM_SUBS_PRIO_NORMAL))

/**
 * @brief Register event hook on submission. The hook would be called last.
 *
 * The event hook called when the event is submitted.
 * The hook function should have a form `void hook(const struct app_event_header *aeh)`.
 * The macro makes sure that the hook provided here is called last.
 * Only one hook can be registered with this macro.
 *
 * @note
 * The registered hook may be called from many contexts.
 * To ensure that order of events in the queue matches the order of the registered callbacks calls,
 * the callbacks are called under the same spinlock as adding events to the queue.
 *
 * @param hook_fn Hook function.
 */
#define APP_EVENT_HOOK_ON_SUBMIT_REGISTER_LAST(hook_fn)                      \
	const struct {} __event_hook_on_submit_last_sub_redefined = {};  \
	Z_APP_EVENT_HOOK_ON_SUBMIT_REGISTER(hook_fn, Z_APP_EM_MARKER_FINAL_ELEMENT)

/**
 * @brief Register event hook on the start of event processing. The hook would be called first.
 *
 * The hook function should have a form `void hook(const struct app_event_header *aeh)`.
 * The macro makes sure that the hook provided here is called first.
 * Only one hook can be registered with this macro.
 *
 * @param hook_fn Hook function.
 */
#define APP_EVENT_HOOK_PREPROCESS_REGISTER_FIRST(hook_fn)                      \
	const struct {} __event_hook_preprocess_first_sub_redefined = {};  \
	Z_APP_EVENT_HOOK_PREPROCESS_REGISTER(hook_fn, Z_APP_EM_MARKER_FIRST_ELEMENT)

/**
 * @brief Register event hook on the start of event processing.
 *
 * The event hook called when the event is being processed.
 * The hook function should have a form `void hook(const struct app_event_header *aeh)`.
 *
 * @param hook_fn Hook function.
 */
#define APP_EVENT_HOOK_PREPROCESS_REGISTER(hook_fn) \
	Z_APP_EVENT_HOOK_PREPROCESS_REGISTER(hook_fn, Z_APP_EM_SUBS_PRIO_ID(Z_APP_EM_SUBS_PRIO_NORMAL))

/**
 * @brief Register event hook on the start of event processing. The hook would be called last.
 *
 * The event hook called when the event is being processed.
 * The hook function should have a form `void hook(const struct app_event_header *aeh)`.
 * The macro makes sure that the hook provided here is called last.
 * Only one hook can be registered with this macro.
 *
 * @param hook_fn Hook function.
 */
#define APP_EVENT_HOOK_PREPROCESS_REGISTER_LAST(hook_fn)                      \
	const struct {} __event_hook_preprocess_last_sub_redefined = {};  \
	Z_APP_EVENT_HOOK_PREPROCESS_REGISTER(hook_fn, Z_APP_EM_MARKER_FINAL_ELEMENT)

/**
 * @brief Register event hook on the end of event processing. The hook would be called first.
 *
 * The event hook called after the event is processed.
 * The hook function should have a form `void hook(const struct app_event_header *aeh)`.
 * The macro makes sure that the hook provided here is called first.
 * Only one hook can be registered with this macro.
 *
 * @param hook_fn Hook function.
 */
#define APP_EVENT_HOOK_POSTPROCESS_REGISTER_FIRST(hook_fn)                      \
	const struct {} __event_hook_postprocess_first_sub_redefined = {};  \
	Z_APP_EVENT_HOOK_POSTPROCESS_REGISTER(hook_fn, Z_APP_EM_MARKER_FIRST_ELEMENT)

/**
 * @brief Register event hook on the end of event processing.
 *
 * The event hook called after the event is processed.
 * The hook function should have a form `void hook(const struct app_event_header *aeh)`.
 *
 * @param hook_fn Hook function.
 */
#define APP_EVENT_HOOK_POSTPROCESS_REGISTER(hook_fn)	\
	Z_APP_EVENT_HOOK_POSTPROCESS_REGISTER(hook_fn,	\
	Z_APP_EM_SUBS_PRIO_ID(Z_APP_EM_SUBS_PRIO_NORMAL))

/**
 * @brief Register event hook on the end of event processing. The hook would be called last.
 *
 * The event hook called after the event is processed.
 * The hook function should have a form `void hook(const struct app_event_header *aeh)`.
 * The macro makes sure that the hook provided here is called last.
 * Only one hook can be registered with this macro.
 *
 * @param hook_fn Hook function.
 */
#define APP_EVENT_HOOK_POSTPROCESS_REGISTER_LAST(hook_fn)			\
	const struct {} __event_hook_postprocess_last_sub_redefined = {};	\
	Z_APP_EVENT_HOOK_POSTPROCESS_REGISTER(hook_fn, Z_APP_EM_MARKER_FINAL_ELEMENT)


/** @brief Initialize the Application Event Manager.
 *
 * @retval 0 If the operation was successful. Error values can be added by the hooks registered
 *         by @ref APP_EVENT_MANAGER_HOOK_POSTINIT_REGISTER macro.
 */
int app_event_manager_init(void);

/** @brief Allocate event.
 *
 * The behavior of this function depends on the actual implementation.
 * The default implementation of this function is same as k_malloc.
 * It is annotated as weak and can be overridden by user.
 *
 * @param size  Amount of memory requested (in bytes).
 * @retval Address of the allocated memory if successful, otherwise NULL.
 **/
void *app_event_manager_alloc(size_t size);


/** @brief Free memory occupied by the event.
 *
 * The behavior of this function depends on the actual implementation.
 * The default implementation of this function is same as k_free.
 * It is annotated as weak and can be overridden by user.
 *
 * @param addr  Pointer to previously allocated memory.
 **/
void app_event_manager_free(void *addr);

/** @brief Log event.
 *
 * This helper macro simplifies event logging.
 *
 * @param aeh  Pointer to the application event header of the event that is
 *             processed by app_event_manager.
 * @param ... `printf`- like format string and variadic list of arguments corresponding to
 *            the format string.
 */
#define APP_EVENT_MANAGER_LOG(aeh, ...) do {							\
	LOG_MODULE_DECLARE(app_event_manager, CONFIG_APP_EVENT_MANAGER_LOG_LEVEL);		\
	if (IS_ENABLED(CONFIG_APP_EVENT_MANAGER_LOG_EVENT_TYPE)) {				\
		LOG_INF("e:%s " GET_ARG_N(1, __VA_ARGS__), aeh->type_id->name			\
			COND_CODE_0(NUM_VA_ARGS_LESS_1(__VA_ARGS__),				\
			    (),									\
			    (, GET_ARGS_LESS_N(1, __VA_ARGS__))					\
			));									\
	} else {										\
		LOG_INF(__VA_ARGS__);								\
	}											\
} while (0)


/**
 * @brief Define flags for event type.
 *
 * @param ... Comma-separated list of flags which should be set.
 *            In case no flags should be set leave it empty.
 */
 #define APP_EVENT_FLAGS_CREATE(...)				\
	(FOR_EACH_NONEMPTY_TERM(Z_APP_EVENT_FLAGS_JOIN, (|), __VA_ARGS__) 0)


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_APP_EVENT_MANAGER_EVENT_MANAGER_H_ */
