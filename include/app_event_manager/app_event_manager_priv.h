/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Application Event Manager private header.
 *
 * Although these defines are globally visible they must not be used directly.
 */

#ifndef ZEPHYR_INCLUDE_APP_EVENT_MANAGER_EVENT_MANAGER_PRIV_H_
#define ZEPHYR_INCLUDE_APP_EVENT_MANAGER_EVENT_MANAGER_PRIV_H_

#include <zephyr.h>
#include <zephyr/types.h>
#include <sys/__assert.h>
#include <logging/log.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Determine if _Generic is supported.
 * In general it's a C11 feature but it was added also in:
 * - GCC 4.9.0 https://gcc.gnu.org/gcc-4.9/changes.html
 * - Clang 3.0 https://releases.llvm.org/3.0/docs/ClangReleaseNotes.html
 *
 * @note Z_C_GENERIC is also set for C++ where functionality is implemented
 * using overloading and templates.
 */
#ifndef Z_C_GENERIC
#if defined(__cplusplus) || (((__STDC_VERSION__ >= 201112L) || \
	((__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__) >= 40900) || \
	((__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__) >= 30000)))
#define Z_C_GENERIC 1
#else
#define Z_C_GENERIC 0
#endif
#endif

/* Macros related to sorting of elements in the event subscribers section. */

/* Markers used for ordering elements in subscribers array for each event type.
 * To ensure ordering markers should go in alphabetical order.
 */

#define _EM_MARKER_ARRAY_START   _a
#define _EM_MARKER_FIRST_ELEMENT _b
#define _EM_MARKER_PRIO_ELEMENTS _p
#define _EM_MARKER_FINAL_ELEMENT _y
#define _EM_MARKER_ARRAY_END     _z

/* Macro expanding ordering level into string.
 * The level must be between 00 and 99. Leading zero is required to ensure
 * proper sorting.
 */
#define _EM_SUBS_PRIO_ID(level) _CONCAT(_CONCAT(_EM_MARKER_PRIO_ELEMENTS, level), _)

/* There are 2 default ordering levels of event subscribers. */
#define _EM_SUBS_PRIO_EARLY  05
#define _EM_SUBS_PRIO_NORMAL 10

/* Convenience macros generating section names. */

#define _APP_EVENT_SUBSCRIBERS_SECTION_PREFIX(ename, marker) \
	_CONCAT(_CONCAT(event_subscribers_, ename), marker)

#define _APP_EVENT_SUBSCRIBERS_SECTION_NAME(ename, marker) \
	STRINGIFY(_APP_EVENT_SUBSCRIBERS_SECTION_PREFIX(ename, marker))


/* Macros related to subscriber array tags
 * Each tag is a zero-length element that which is placed by linker at the start
 * and the end of the subscriber array respectively.
 */

/* Convenience macro generating tag name. */
#define _EM_TAG_NAME(prefix) _CONCAT(prefix, _tag)

/* Zero-length subscriber to be used as a tag. */
#define _APP_EVENT_SUBSCRIBERS_TAG(ename, marker)					\
	const struct {} _EM_TAG_NAME(_APP_EVENT_SUBSCRIBERS_SECTION_PREFIX		\
		(ename, marker))							\
	__used __aligned(__alignof(struct event_subscriber))				\
	__attribute__((__section__(_APP_EVENT_SUBSCRIBERS_SECTION_NAME			\
		(ename, marker)))) = {};

/* Macro defining subscriber array boundary tags. */
#define _APP_EVENT_SUBSCRIBERS_ARRAY_TAGS(ename)			\
	_APP_EVENT_SUBSCRIBERS_TAG(ename, _EM_MARKER_ARRAY_START)	\
	_APP_EVENT_SUBSCRIBERS_TAG(ename, _EM_MARKER_ARRAY_END)

/* Pointer to the first element of subscriber array for a given event type. */
#define _APP_EVENT_SUBSCRIBERS_START_TAG(ename)						\
	((const struct event_subscriber *)						\
	&_EM_TAG_NAME(_APP_EVENT_SUBSCRIBERS_SECTION_PREFIX				\
		(ename, _EM_MARKER_ARRAY_START))					\
	)

/* Pointer to the element past the last element of subscriber array for a given event type. */
#define _APP_EVENT_SUBSCRIBERS_END_TAG(ename)						\
	((const struct event_subscriber *)						\
	 &_EM_TAG_NAME(_APP_EVENT_SUBSCRIBERS_SECTION_PREFIX(ename, _EM_MARKER_ARRAY_END))\
	 )

/* Subscribe a listener to an event. */
#define _APP_EVENT_SUBSCRIBE(lname, ename, prio)					\
	const struct event_subscriber _CONCAT(_CONCAT(__event_subscriber_, ename), lname)\
	__used __aligned(__alignof(struct event_subscriber))				\
	__attribute__((__section__(_APP_EVENT_SUBSCRIBERS_SECTION_NAME(ename, prio)))) = {\
		.listener = &_CONCAT(__event_listener_, lname),				\
	}

/* Pointer to event type definition is used as event type identifier. */
#define _EVENT_ID(ename) (&_CONCAT(__event_type_, ename))

/* Macro generates a function of name new_ename where ename is provided as
 * an argument. Allocator function is used to create an event of the given
 * ename type.
 */
#define _EVENT_ALLOCATOR_FN(ename)						\
	static inline struct ename *_CONCAT(new_, ename)(void)			\
	{									\
		struct ename *event =						\
			(struct ename *)app_event_manager_alloc(sizeof(*event));	\
		BUILD_ASSERT(offsetof(struct ename, header) == 0,		\
				 "");						\
		if (event != NULL) {						\
			event->header.type_id = _EVENT_ID(ename);		\
		}								\
		return event;							\
	}

/* Macro generates a function of name new_ename where ename is provided as
 * an argument. Allocator function is used to create an event of the given
 * ename type.
 */
#define _EVENT_ALLOCATOR_DYNDATA_FN(ename)						\
	static inline struct ename *_CONCAT(new_, ename)(size_t size)			\
	{										\
		struct ename *event =							\
			(struct ename *)app_event_manager_alloc(sizeof(*event) + size);	\
		BUILD_ASSERT((offsetof(struct ename, dyndata) +				\
				  sizeof(event->dyndata.size)) ==			\
				 sizeof(*event), "");					\
		BUILD_ASSERT(offsetof(struct ename, header) == 0,			\
				 "");							\
		if (event != NULL) {							\
			event->header.type_id = _EVENT_ID(ename);			\
			event->dyndata.size = size;					\
		}									\
		return event;								\
	}

/* Macro generates a function of name cast_ename where ename is provided as
 * an argument. Casting function is used to convert app_event_header pointer
 * into pointer to event matching the given ename type.
 */
#define _EVENT_CASTER_FN(ename)									\
	static inline struct ename *_CONCAT(cast_, ename)(const struct app_event_header *aeh)	\
	{											\
		struct ename *event = NULL;							\
		if (aeh->type_id == _EVENT_ID(ename)) {						\
			event = CONTAINER_OF(aeh, struct ename, header);			\
		}										\
		return event;									\
	}


/* Macro generates a function of name is_ename where ename is provided as
 * an argument. Typecheck function is used to check if pointer to app_event_header
 * belongs to the event matching the given ename type.
 */
#define _EVENT_TYPECHECK_FN(ename) \
	static inline bool _CONCAT(is_, ename)(const struct app_event_header *aeh)	\
	{									\
		return (aeh->type_id == _EVENT_ID(ename));			\
	}

/* Declarations and definitions - for more details refer to public API. */
#define _APP_EVENT_LISTENER(lname, notification_fn)					\
	STRUCT_SECTION_ITERABLE(event_listener, _CONCAT(__event_listener_, lname)) = {	\
		.name = STRINGIFY(lname),						\
		.notification = (notification_fn),					\
	}

#define _APP_EVENT_TYPE_DECLARE_COMMON(ename)						\
	extern Z_DECL_ALIGN(struct event_type) _CONCAT(__event_type_, ename);		\
	_EVENT_CASTER_FN(ename);							\
	_EVENT_TYPECHECK_FN(ename)


#define _APP_EVENT_TYPE_DECLARE(ename)					\
	enum {_CONCAT(ename, _HAS_DYNDATA) = 0};			\
	_APP_EVENT_TYPE_DECLARE_COMMON(ename);				\
	_EVENT_ALLOCATOR_FN(ename)

#define _APP_EVENT_TYPE_DYNDATA_DECLARE(ename)				\
	enum {_CONCAT(ename, _HAS_DYNDATA) = 1};			\
	_APP_EVENT_TYPE_DECLARE_COMMON(ename);				\
	_EVENT_ALLOCATOR_DYNDATA_FN(ename)

#if IS_ENABLED(CONFIG_APP_EVENT_MANAGER_PROVIDE_EVENT_SIZE)
#define _APP_EVENT_TYPE_DEFINE_SIZES(ename)             \
	.struct_size = sizeof(struct ename),
#else
#define _APP_EVENT_TYPE_DEFINE_SIZES(ename)
#endif

/** @brief Event header.
 *
 * When defining an event structure, the application event header
 * must be placed as the first field.
 */
struct app_event_header {
	/** Linked list node used to chain events. */
	sys_snode_t node;

	/** Pointer to the event type object. */
	const struct event_type *type_id;
};

/** Function to log data from this event. */
typedef void (*log_event_data)(const struct app_event_header *aeh);

/** Deprecated function to log data from this event. */
typedef	int (*log_event_data_dep)(const struct app_event_header *aeh, char *buf, size_t buf_len);

#if IS_ENABLED(CONFIG_APP_EVENT_MANAGER_USE_DEPRECATED_LOG_FUN)
#define _APP_EVENT_TYPE_DEFINE_LOG_FUN(log_fn)						\
	.log_event_func_dep = ((IS_ENABLED(CONFIG_LOG) && Z_C_GENERIC) ?		\
					((log_event_data_dep)_Generic((log_fn),		\
						log_event_data : NULL,			\
						log_event_data_dep : log_fn,		\
						default : NULL))			\
					: (NULL)),					\
	.log_event_func     = (log_event_data) (IS_ENABLED(CONFIG_LOG) ?		\
					(Z_C_GENERIC ?					\
						(_Generic((log_fn),			\
							log_event_data : log_fn,	\
							log_event_data_dep : NULL,	\
							default : NULL))		\
						: (log_fn))				\
					: (NULL)),
#else
#define _APP_EVENT_TYPE_DEFINE_LOG_FUN(log_fun) .log_event_func = log_fun,
#endif

/** @brief Event type.
 */

struct event_type {
	/** Event name. */
	const char			*name;

	/** Pointer to the array of subscribers. */
	const struct event_subscriber	*subs_start;

	/** Pointer to the element directly after the array of subscribers. */
	const struct event_subscriber	*subs_stop;

	/** Function to log data from this event. */
	log_event_data log_event_func;

#if IS_ENABLED(CONFIG_APP_EVENT_MANAGER_USE_DEPRECATED_LOG_FUN)
	/** Deprecated function to log data from this event. */
	log_event_data_dep log_event_func_dep;
#endif

	/** Custom data related to tracking. */
	const void *trace_data;

	/** Array of flags dedicated to event type. */
	const uint8_t flags;

#if IS_ENABLED(CONFIG_APP_EVENT_MANAGER_PROVIDE_EVENT_SIZE)
	/** The size of the event structure */
	uint16_t struct_size;
#endif
};

extern struct event_type _event_type_list_start[];
extern struct event_type _event_type_list_end[];

#define _APP_EVENT_TYPE_DEFINE(ename, log_fn, trace_data_pointer, et_flags)		\
	BUILD_ASSERT(((et_flags) & ((BIT_MASK(APP_EVENT_TYPE_FLAGS_USER_SETTABLE_START-	\
		APP_EVENT_TYPE_FLAGS_SYSTEM_START))<<					\
		APP_EVENT_TYPE_FLAGS_SYSTEM_START)) == 0);				\
	_APP_EVENT_SUBSCRIBERS_ARRAY_TAGS(ename);					\
	STRUCT_SECTION_ITERABLE(event_type, _CONCAT(__event_type_, ename)) = {			\
		.name            = STRINGIFY(ename),						\
		.subs_start      = _APP_EVENT_SUBSCRIBERS_START_TAG(ename),		\
		.subs_stop       = _APP_EVENT_SUBSCRIBERS_END_TAG(ename),		\
		_APP_EVENT_TYPE_DEFINE_LOG_FUN(log_fn) /* No comma here intentionally */\
		.trace_data      = (IS_ENABLED(CONFIG_APP_EVENT_MANAGER_TRACE_EVENT_DATA) ?\
					(trace_data_pointer) : (NULL)),				\
		.flags = ((_CONCAT(ename, _HAS_DYNDATA)) ?					\
				((et_flags) | BIT(APP_EVENT_TYPE_FLAGS_HAS_DYNDATA)) :	\
				((et_flags) & (~BIT(APP_EVENT_TYPE_FLAGS_HAS_DYNDATA)))),\
		_APP_EVENT_TYPE_DEFINE_SIZES(ename) /* No comma here intentionally */	\
	}

/**
 * @brief Bitmask indicating event is displayed.
 */
struct app_event_manager_event_display_bm {
	ATOMIC_DEFINE(flags, CONFIG_APP_EVENT_MANAGER_MAX_EVENT_CNT);
};

extern struct app_event_manager_event_display_bm _app_event_manager_event_display_bm;


/* Event hooks subscribers */
#define _APP_EVENT_HOOK_REGISTER(section, hook_fn, prio)           \
	BUILD_ASSERT((hook_fn) != NULL, "Registered hook cannot be NULL"); \
	STRUCT_SECTION_ITERABLE(section, _CONCAT(prio, hook_fn)) = {       \
		.hook = (hook_fn)                                          \
	}

#define _APP_EVENT_MANAGER_HOOK_POSTINIT_REGISTER(hook_fn, prio)             \
	BUILD_ASSERT(IS_ENABLED(CONFIG_APP_EVENT_MANAGER_POSTINIT_HOOK),     \
		     "Enable APP_EVENT_MANAGER_POSTINIT_HOOK before usage"); \
	_APP_EVENT_HOOK_REGISTER(app_event_manager_postinit_hook, hook_fn, prio)

#define _APP_EVENT_HOOK_ON_SUBMIT_REGISTER(hook_fn, prio)                   \
	BUILD_ASSERT(IS_ENABLED(CONFIG_APP_EVENT_MANAGER_SUBMIT_HOOKS),     \
		     "Enable APP_EVENT_MANAGER_SUBMIT_HOOKS before usage"); \
	_APP_EVENT_HOOK_REGISTER(event_submit_hook, hook_fn, prio)

#define _APP_EVENT_HOOK_PREPROCESS_REGISTER(hook_fn, prio)                      \
	BUILD_ASSERT(IS_ENABLED(CONFIG_APP_EVENT_MANAGER_PREPROCESS_HOOKS),     \
		     "Enable APP_EVENT_MANAGER_PREPROCESS_HOOKS before usage"); \
	_APP_EVENT_HOOK_REGISTER(event_preprocess_hook, hook_fn, prio)

#define _APP_EVENT_HOOK_POSTPROCESS_REGISTER(hook_fn, prio)                      \
	BUILD_ASSERT(IS_ENABLED(CONFIG_APP_EVENT_MANAGER_POSTPROCESS_HOOKS),     \
		     "Enable APP_EVENT_MANAGER_POSTPROCESS_HOOKS before usage"); \
	_APP_EVENT_HOOK_REGISTER(event_postprocess_hook, hook_fn, prio)

/**
 * @brief Joining together event type flags.
 */
#define _EVENT_FLAGS_JOIN(flag) BIT(flag)


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
 * All event listeners must be defined using @ref APP_EVENT_LISTENER.
 */
struct event_listener {
	/** Name of this listener. */
	const char *name;

	/** Pointer to the function that is called when an event is handled.
	 * The function should return true to consume the event, which means that the event is
	 * not propagated to further listeners, or false, otherwise.
	 */
	bool (*notification)(const struct app_event_header *aeh);
};

/** @brief Event subscriber.
 */
struct event_subscriber {
	/** Pointer to the listener. */
	const struct event_listener *listener;
};


/** @brief Structure used to register Application Event Manager initialization hook
 */
struct app_event_manager_postinit_hook {
	/** @brief Hook function */
	int (*hook)(void);
};
/** @brief Structure used to register event submit hook
 */
struct event_submit_hook {
	/** @brief Hook function */
	void (*hook)(const struct app_event_header *aeh);
};

/** @brief Structure used to register event preprocess hook
 */
struct event_preprocess_hook {
	/** @brief Hook function */
	void (*hook)(const struct app_event_header *aeh);
};

/** @brief Structure used to register event postprocess hook
 */
struct event_postprocess_hook {
	/** @brief Hook function */
	void (*hook)(const struct app_event_header *aeh);
};


/** @brief Submit an event to the Application Event Manager.
 *
 * @param aeh  Pointer to the application event header element in the event object.
 */
void _event_submit(struct app_event_header *aeh);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_APP_EVENT_MANAGER_EVENT_MANAGER_PRIV_H_ */
