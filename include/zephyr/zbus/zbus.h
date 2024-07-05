/*
 * Copyright (c) 2022 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZBUS_H_
#define ZEPHYR_INCLUDE_ZBUS_H_

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/iterable_sections.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Zbus API
 * @defgroup zbus_apis Zbus APIs
 * @ingroup os_services
 * @{
 */

/**
 * @brief Type used to represent a channel mutable data.
 *
 * Every channel has a zbus_channel_data structure associated.
 */
struct zbus_channel_data {
	/** Static channel observer list start index. Considering the ITERABLE SECTIONS allocation
	 * order.
	 */
	int16_t observers_start_idx;

	/** Static channel observer list end index. Considering the ITERABLE SECTIONS allocation
	 * order.
	 */
	int16_t observers_end_idx;

	/** Access control semaphore. Points to the semaphore used to avoid race conditions
	 * for accessing the channel.
	 */
	struct k_sem sem;

#if defined(CONFIG_ZBUS_PRIORITY_BOOST)
	/** Highest observer priority. Indicates the priority that the VDED will use to boost the
	 * notification process avoiding preemptions.
	 */
	int highest_observer_priority;
#endif /* CONFIG_ZBUS_PRIORITY_BOOST */

#if defined(CONFIG_ZBUS_RUNTIME_OBSERVERS) || defined(__DOXYGEN__)
	/** Channel observer list. Represents the channel's observers list, it can be empty
	 * or have listeners and subscribers mixed in any sequence. It can be changed in runtime.
	 */
	sys_slist_t observers;
#endif /* CONFIG_ZBUS_RUNTIME_OBSERVERS */

#if defined(CONFIG_ZBUS_MSG_SUBSCRIBER_NET_BUF_POOL_ISOLATION) || defined(__DOXYGEN__)
	/** Net buf pool for message subscribers. It can be either the global or a separated one.
	 */
	struct net_buf_pool *msg_subscriber_pool;
#endif /* ZBUS_MSG_SUBSCRIBER_NET_BUF_POOL_ISOLATION */
};

/**
 * @brief Type used to represent a channel.
 *
 * Every channel has a zbus_channel structure associated used to control the channel
 * access and usage.
 */
struct zbus_channel {
#if defined(CONFIG_ZBUS_CHANNEL_NAME) || defined(__DOXYGEN__)
	/** Channel name. */
	const char *const name;
#endif
	/** Message reference. Represents the message's reference that points to the actual
	 * shared memory region.
	 */
	void *const message;

	/** Message size. Represents the channel's message size. */
	const size_t message_size;

	/** User data available to extend zbus features. The channel must be claimed before
	 * using this field.
	 */
	void *const user_data;

	/** Message validator. Stores the reference to the function to check the message
	 * validity before actually performing the publishing. No invalid messages can be
	 * published. Every message is valid when this field is empty.
	 */
	bool (*const validator)(const void *msg, size_t msg_size);

	/** Mutable channel data struct. */
	struct zbus_channel_data *const data;
};

/**
 * @brief Type used to represent an observer type.
 *
 * A observer can be a listener or a subscriber.
 */
enum __packed zbus_observer_type {
	ZBUS_OBSERVER_LISTENER_TYPE,
	ZBUS_OBSERVER_SUBSCRIBER_TYPE,
	ZBUS_OBSERVER_MSG_SUBSCRIBER_TYPE,
};

struct zbus_observer_data {
	/** Enabled flag. Indicates if observer is receiving notification. */
	bool enabled;

#if defined(CONFIG_ZBUS_PRIORITY_BOOST)
	/** Subscriber attached thread priority. */
	int priority;
#endif /* CONFIG_ZBUS_PRIORITY_BOOST */
};

/**
 * @brief Type used to represent an observer.
 *
 * Every observer has an representation structure containing the relevant information.
 * An observer is a code portion interested in some channel. The observer can be notified
 * synchronously or asynchronously and it is called listener and subscriber respectively.
 * The observer can be enabled or disabled during runtime by change the enabled boolean
 * field of the structure. The listeners have a callback function that is executed by the
 * bus with the index of the changed channel as argument when the notification is sent.
 * The subscribers have a message queue where the bus enqueues the index of the changed
 * channel when a notification is sent.
 *
 * @see zbus_obs_set_enable function to properly change the observer's enabled field.
 *
 */
struct zbus_observer {
#if defined(CONFIG_ZBUS_OBSERVER_NAME) || defined(__DOXYGEN__)
	/** Observer name. */
	const char *const name;
#endif
	/** Type indication. */
	enum zbus_observer_type type;

	/** Mutable observer data struct. */
	struct zbus_observer_data *const data;

	union {
		/** Observer message queue. It turns the observer into a subscriber. */
		struct k_msgq *const queue;

		/** Observer callback function. It turns the observer into a listener. */
		void (*const callback)(const struct zbus_channel *chan);

#if defined(CONFIG_ZBUS_MSG_SUBSCRIBER) || defined(__DOXYGEN__)
		/** Observer message FIFO. It turns the observer into a message subscriber. It only
		 * exists if the @kconfig{CONFIG_ZBUS_MSG_SUBSCRIBER} is enabled.
		 */
		struct k_fifo *const message_fifo;
#endif /* CONFIG_ZBUS_MSG_SUBSCRIBER */
	};
};

/** @cond INTERNAL_HIDDEN */
struct zbus_channel_observation_mask {
	bool enabled;
};

struct zbus_channel_observation {
	const struct zbus_channel *const chan;
	const struct zbus_observer *const obs;
};

#ifdef __cplusplus
#define _ZBUS_CPP_EXTERN extern
#else
#define _ZBUS_CPP_EXTERN
#endif /* __cplusplus */

#define ZBUS_MIN_THREAD_PRIORITY (CONFIG_NUM_PREEMPT_PRIORITIES - 1)

#if defined(CONFIG_ZBUS_ASSERT_MOCK)
#define _ZBUS_ASSERT(_cond, _fmt, ...)                                                             \
	do {                                                                                       \
		if (!(_cond)) {                                                                    \
			printk("ZBUS ASSERT: ");                                                   \
			printk(_fmt, ##__VA_ARGS__);                                               \
			printk("\n");                                                              \
			return -EFAULT;                                                            \
		}                                                                                  \
	} while (0)
#else
#define _ZBUS_ASSERT(_cond, _fmt, ...) __ASSERT(_cond, _fmt, ##__VA_ARGS__)
#endif

#if defined(CONFIG_ZBUS_CHANNEL_NAME)
#define ZBUS_CHANNEL_NAME_INIT(_name) .name = #_name,
#define _ZBUS_CHAN_NAME(_chan)        (_chan)->name
#else
#define ZBUS_CHANNEL_NAME_INIT(_name)
#define _ZBUS_CHAN_NAME(_chan) ""
#endif

#if defined(CONFIG_ZBUS_OBSERVER_NAME)
#define ZBUS_OBSERVER_NAME_INIT(_name) .name = #_name,
#define _ZBUS_OBS_NAME(_obs)           (_obs)->name
#else
#define ZBUS_OBSERVER_NAME_INIT(_name)
#define _ZBUS_OBS_NAME(_obs) ""
#endif

#if defined(CONFIG_ZBUS_RUNTIME_OBSERVERS)
#define ZBUS_RUNTIME_OBSERVERS_LIST_DECL(_slist_name) static sys_slist_t _slist_name
#define ZBUS_RUNTIME_OBSERVERS_LIST_INIT(_slist_name) .runtime_observers = &_slist_name,
#else
#define ZBUS_RUNTIME_OBSERVERS_LIST_DECL(_slist_name)
#define ZBUS_RUNTIME_OBSERVERS_LIST_INIT(_slist_name) /* No runtime observers */
#endif

#define _ZBUS_OBS_EXTERN(_name) extern struct zbus_observer _name

#define _ZBUS_CHAN_EXTERN(_name) extern const struct zbus_channel _name

#define ZBUS_REF(_value) &(_value)

#define FOR_EACH_FIXED_ARG_NONEMPTY_TERM(F, sep, fixed_arg, ...)                                   \
	COND_CODE_0(/* are there zero non-empty arguments ? */                                     \
		    NUM_VA_ARGS_LESS_1(                                                            \
			    LIST_DROP_EMPTY(__VA_ARGS__, _)), /* if so, expand to nothing */       \
		    (),                                       /* otherwise, expand to: */          \
		    (FOR_EACH_IDX_FIXED_ARG(                                                       \
			    F, sep, fixed_arg,                                                     \
			    LIST_DROP_EMPTY(__VA_ARGS__)) /* plus a final terminator */            \
		     __DEBRACKET sep))

#define _ZBUS_OBSERVATION_PREFIX(_idx)                                                             \
	GET_ARG_N(_idx, 00, 01, 02, 03, 04, 05, 06, 07, 08, 09, 10, 11, 12, 13, 14, 15, 16, 17,    \
		  18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37,  \
		  38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57,  \
		  58, 59, 60, 61, 62, 63)

#define _ZBUS_CHAN_OBSERVATION(_idx, _obs, _chan)                                                  \
	const STRUCT_SECTION_ITERABLE(                                                             \
		zbus_channel_observation,                                                          \
		_CONCAT(_chan, _ZBUS_OBSERVATION_PREFIX(UTIL_INC(_idx)))) = {.chan = &_chan,       \
									     .obs = &_obs};        \
	STRUCT_SECTION_ITERABLE(zbus_channel_observation_mask,                                     \
				_CONCAT(_CONCAT(_chan, _ZBUS_OBSERVATION_PREFIX(UTIL_INC(_idx))),  \
					_mask)) = {.enabled = false};

#if defined(CONFIG_ZBUS_RUNTIME_OBSERVERS) || defined(__DOXYGEN__)
#define _ZBUS_RUNTIME_OBSERVERS(_name)      .observers = &(_CONCAT(_observers_, _name)),
#define _ZBUS_RUNTIME_OBSERVERS_DECL(_name) static sys_slist_t _CONCAT(_observers_, _name);
#else
#define _ZBUS_RUNTIME_OBSERVERS(_name)
#define _ZBUS_RUNTIME_OBSERVERS_DECL(_name)
#endif /* CONFIG_ZBUS_RUNTIME_OBSERVERS */

/** @endcond */

/* clang-format off */
/**
 * @brief Add a static channel observervation.
 *
 * This macro initializes a channel observation by receiving the
 * channel and the observer.
 *
 * @param _chan Channel instance.
 * @param _obs Observer instance.
 * @param _masked Observation state.
 * @param _prio Observer notification sequence priority.
 */
#define ZBUS_CHAN_ADD_OBS_WITH_MASK(_chan, _obs, _masked, _prio)                                   \
	const STRUCT_SECTION_ITERABLE(zbus_channel_observation,                                    \
		_CONCAT(_CONCAT(_chan, zz), _CONCAT(_prio, _obs))) = {                             \
			.chan = &_chan,                                                            \
			.obs = &_obs,                                                              \
	};                                                                                         \
	STRUCT_SECTION_ITERABLE(zbus_channel_observation_mask,                                     \
				_CONCAT(_CONCAT(_CONCAT(_chan, zz), _CONCAT(_prio, _obs)),         \
					_mask)) = {.enabled = _masked}
/* clang-format on */

/**
 * @brief Add a static channel observervation.
 *
 * This macro initializes a channel observation by receiving the
 * channel and the observer.
 *
 * @param _chan Channel instance.
 * @param _obs Observer instance.
 * @param _prio Observer notification sequence priority.
 */
#define ZBUS_CHAN_ADD_OBS(_chan, _obs, _prio) ZBUS_CHAN_ADD_OBS_WITH_MASK(_chan, _obs, false, _prio)

/**
 * @def ZBUS_OBS_DECLARE
 * This macro list the observers to be used in a file. Internally, it declares the observers with
 * the extern statement. Note it is only necessary when the observers are declared outside the file.
 */
#define ZBUS_OBS_DECLARE(...) FOR_EACH_NONEMPTY_TERM(_ZBUS_OBS_EXTERN, (;), __VA_ARGS__)

/**
 * @def ZBUS_CHAN_DECLARE
 * This macro list the channels to be used in a file. Internally, it declares the channels with the
 * extern statement. Note it is only necessary when the channels are declared outside the file.
 */
#define ZBUS_CHAN_DECLARE(...) FOR_EACH(_ZBUS_CHAN_EXTERN, (;), __VA_ARGS__)

/**
 * @def ZBUS_OBSERVERS_EMPTY
 * This macro indicates the channel has no observers.
 */
#define ZBUS_OBSERVERS_EMPTY

/**
 * @def ZBUS_OBSERVERS
 * This macro indicates the channel has listed observers. Note the sequence of observer notification
 * will follow the same as listed.
 */
#define ZBUS_OBSERVERS(...) __VA_ARGS__

/* clang-format off */
/**
 * @brief Zbus channel definition.
 *
 * This macro defines a channel.
 *
 * @param _name The channel's name.
 * @param _type The Message type. It must be a struct or union.
 * @param _validator The validator function.
 * @param _user_data A pointer to the user data.
 *
 * @see struct zbus_channel
 * @param _observers The observers list. The sequence indicates the priority of the observer. The
 * first the highest priority.
 * @param _init_val The message initialization.
 */
#define ZBUS_CHAN_DEFINE(_name, _type, _validator, _user_data, _observers, _init_val)     \
	static _type _CONCAT(_zbus_message_, _name) = _init_val;                          \
	static struct zbus_channel_data _CONCAT(_zbus_chan_data_, _name) = {              \
		.observers_start_idx = -1,                                                \
		.observers_end_idx = -1,                                                  \
		.sem = Z_SEM_INITIALIZER(_CONCAT(_zbus_chan_data_, _name).sem, 1, 1),     \
		IF_ENABLED(CONFIG_ZBUS_RUNTIME_OBSERVERS, (                               \
			.observers = SYS_SLIST_STATIC_INIT(                               \
				&_CONCAT(_zbus_chan_data_, _name).observers),             \
		))                                                                        \
		IF_ENABLED(CONFIG_ZBUS_PRIORITY_BOOST, (                                  \
			.highest_observer_priority = ZBUS_MIN_THREAD_PRIORITY,            \
		))                                                                        \
	};                                                                                \
	static K_MUTEX_DEFINE(_CONCAT(_zbus_mutex_, _name));                              \
	_ZBUS_CPP_EXTERN const STRUCT_SECTION_ITERABLE(zbus_channel, _name) = {           \
		ZBUS_CHANNEL_NAME_INIT(_name) /* Maybe removed */                         \
		.message = &_CONCAT(_zbus_message_, _name),                               \
		.message_size = sizeof(_type),                                            \
		.user_data = _user_data,                                                  \
		.validator = _validator,                                                  \
		.data = &_CONCAT(_zbus_chan_data_, _name),                                \
		IF_ENABLED(ZBUS_MSG_SUBSCRIBER_NET_BUF_POOL_ISOLATION, (                   \
			.msg_subscriber_pool = &_zbus_msg_subscribers_pool,               \
		))                                                                        \
	};                                                                                \
	/* Extern declaration of observers */                                             \
	ZBUS_OBS_DECLARE(_observers);                                                     \
	/* Create all channel observations from observers list */                         \
	FOR_EACH_FIXED_ARG_NONEMPTY_TERM(_ZBUS_CHAN_OBSERVATION, (;), _name, _observers)
/* clang-format on */

/**
 * @brief Initialize a message.
 *
 * This macro initializes a message by passing the values to initialize the message struct
 * or union.
 *
 * @param[in] _val Variadic with the initial values. ``ZBUS_INIT(0)`` means ``{0}``, as
 * ZBUS_INIT(.a=10, .b=30) means ``{.a=10, .b=30}``.
 */
#define ZBUS_MSG_INIT(_val, ...)                                                                   \
	{                                                                                          \
		_val, ##__VA_ARGS__                                                                \
	}

/* clang-format off */
/**
 * @brief Define and initialize a subscriber.
 *
 * This macro defines an observer of subscriber type. It defines a message queue where the
 * subscriber will receive the notification asynchronously, and initialize the ``struct
 * zbus_observer`` defining the subscriber.
 *
 * @param[in] _name The subscriber's name.
 * @param[in] _queue_size The notification queue's size.
 * @param[in] _enable The subscriber initial enable state.
 */
#define ZBUS_SUBSCRIBER_DEFINE_WITH_ENABLE(_name, _queue_size, _enable)       \
	K_MSGQ_DEFINE(_zbus_observer_queue_##_name,                           \
		      sizeof(const struct zbus_channel *),                    \
		      _queue_size, sizeof(const struct zbus_channel *)        \
	);                                                                    \
	static struct zbus_observer_data _CONCAT(_zbus_obs_data_, _name) = {  \
		.enabled = _enable,                                           \
		IF_ENABLED(CONFIG_ZBUS_PRIORITY_BOOST, (                      \
			.priority = ZBUS_MIN_THREAD_PRIORITY,                 \
		))                                                            \
	};                                                                    \
	STRUCT_SECTION_ITERABLE(zbus_observer, _name) = {                     \
		ZBUS_OBSERVER_NAME_INIT(_name) /* Name field */               \
		.type = ZBUS_OBSERVER_SUBSCRIBER_TYPE,                        \
		.data = &_CONCAT(_zbus_obs_data_, _name),                     \
		.queue = &_zbus_observer_queue_##_name,                       \
	}
/* clang-format on */

/**
 * @brief Define and initialize a subscriber.
 *
 * This macro defines an observer of subscriber type. It defines a message queue where the
 * subscriber will receive the notification asynchronously, and initialize the ``struct
 * zbus_observer`` defining the subscriber. The subscribers are defined in the enabled
 * state with this macro.
 *
 * @param[in] _name The subscriber's name.
 * @param[in] _queue_size The notification queue's size.
 */
#define ZBUS_SUBSCRIBER_DEFINE(_name, _queue_size)                                                 \
	ZBUS_SUBSCRIBER_DEFINE_WITH_ENABLE(_name, _queue_size, true)

/* clang-format off */
/**
 * @brief Define and initialize a listener.
 *
 * This macro defines an observer of listener type. This macro establishes the callback where the
 * listener will be notified synchronously, and initialize the ``struct zbus_observer`` defining the
 * listener.
 *
 * @param[in] _name The listener's name.
 * @param[in] _cb The callback function.
 * @param[in] _enable The listener initial enable state.
 */
#define ZBUS_LISTENER_DEFINE_WITH_ENABLE(_name, _cb, _enable)                                      \
	static struct zbus_observer_data _CONCAT(_zbus_obs_data_, _name) = {                       \
		.enabled = _enable,                                                                \
		IF_ENABLED(CONFIG_ZBUS_PRIORITY_BOOST, (                                           \
			.priority = ZBUS_MIN_THREAD_PRIORITY,                                      \
		))                                                                                 \
	};                                                                                         \
	STRUCT_SECTION_ITERABLE(zbus_observer, _name) = {                                          \
		ZBUS_OBSERVER_NAME_INIT(_name) /* Name field */                                    \
		.type = ZBUS_OBSERVER_LISTENER_TYPE,                                               \
		.data = &_CONCAT(_zbus_obs_data_, _name),                                          \
		.callback = (_cb)                                                                  \
	}
/* clang-format on */

/**
 * @brief Define and initialize a listener.
 *
 * This macro defines an observer of listener type. This macro establishes the callback where the
 * listener will be notified synchronously and initialize the ``struct zbus_observer`` defining the
 * listener. The listeners are defined in the enabled state with this macro.
 *
 * @param[in] _name The listener's name.
 * @param[in] _cb The callback function.
 */
#define ZBUS_LISTENER_DEFINE(_name, _cb) ZBUS_LISTENER_DEFINE_WITH_ENABLE(_name, _cb, true)

/* clang-format off */
/**
 * @brief Define and initialize a message subscriber.
 *
 * This macro defines an observer of @ref ZBUS_OBSERVER_SUBSCRIBER_TYPE type. It defines a FIFO
 * where the subscriber will receive the message asynchronously and initialize the @ref
 * zbus_observer defining the subscriber.
 *
 * @param[in] _name The subscriber's name.
 * @param[in] _enable The subscriber's initial state.
 */
#define ZBUS_MSG_SUBSCRIBER_DEFINE_WITH_ENABLE(_name, _enable)                \
	static K_FIFO_DEFINE(_zbus_observer_fifo_##_name);                    \
	static struct zbus_observer_data _CONCAT(_zbus_obs_data_, _name) = {  \
		.enabled = _enable,                                           \
		IF_ENABLED(CONFIG_ZBUS_PRIORITY_BOOST, (                      \
			.priority = ZBUS_MIN_THREAD_PRIORITY,                 \
		))                                                            \
	};                                                                    \
	STRUCT_SECTION_ITERABLE(zbus_observer, _name) = {                     \
		ZBUS_OBSERVER_NAME_INIT(_name) /* Name field */               \
		.type = ZBUS_OBSERVER_MSG_SUBSCRIBER_TYPE,                    \
		.data = &_CONCAT(_zbus_obs_data_, _name),                     \
		.message_fifo = &_zbus_observer_fifo_##_name,                 \
	}
/* clang-format on */

/**
 * @brief Define and initialize an enabled message subscriber.
 *
 * This macro defines an observer of message subscriber type. It defines a FIFO where the
 * subscriber will receive the message asynchronously and initialize the @ref
 * zbus_observer defining the subscriber. The message subscribers are defined in the enabled state
 * with this macro.

 *
 * @param[in] _name The subscriber's name.
 */
#define ZBUS_MSG_SUBSCRIBER_DEFINE(_name) ZBUS_MSG_SUBSCRIBER_DEFINE_WITH_ENABLE(_name, true)
/**
 *
 * @brief Publish to a channel
 *
 * This routine publishes a message to a channel.
 *
 * @param chan The channel's reference.
 * @param msg Reference to the message where the publish function copies the channel's
 * message data from.
 * @param timeout Waiting period to publish the channel,
 *                or one of the special values K_NO_WAIT and K_FOREVER.
 *
 * @retval 0 Channel published.
 * @retval -ENOMSG The message is invalid based on the validator function or some of the
 * observers could not receive the notification.
 * @retval -EBUSY The channel is busy.
 * @retval -EAGAIN Waiting period timed out.
 * @retval -EFAULT A parameter is incorrect, the notification could not be sent to one or more
 * observer, or the function context is invalid (inside an ISR). The function only returns this
 * value when the @kconfig{CONFIG_ZBUS_ASSERT_MOCK} is enabled.
 */
int zbus_chan_pub(const struct zbus_channel *chan, const void *msg, k_timeout_t timeout);

/**
 * @brief Read a channel
 *
 * This routine reads a message from a channel.
 *
 * @param[in] chan The channel's reference.
 * @param[out] msg Reference to the message where the read function copies the channel's
 * message data to.
 * @param[in] timeout Waiting period to read the channel,
 *                or one of the special values K_NO_WAIT and K_FOREVER.
 *
 * @retval 0 Channel read.
 * @retval -EBUSY The channel is busy.
 * @retval -EAGAIN Waiting period timed out.
 * @retval -EFAULT A parameter is incorrect, or the function context is invalid (inside an ISR). The
 * function only returns this value when the @kconfig{CONFIG_ZBUS_ASSERT_MOCK} is enabled.
 */
int zbus_chan_read(const struct zbus_channel *chan, void *msg, k_timeout_t timeout);

/**
 * @brief Claim a channel
 *
 * This routine claims a channel. During the claiming period the channel is blocked for publishing,
 * reading, notifying or claiming again. Finishing is the only available action.
 *
 * @warning After calling this routine, the channel cannot be used by other
 * thread until the zbus_chan_finish routine is performed.
 *
 * @warning This routine should only be called once before a zbus_chan_finish.
 *
 * @param[in] chan The channel's reference.
 * @param[in] timeout Waiting period to claim the channel,
 *                or one of the special values K_NO_WAIT and K_FOREVER.
 *
 * @retval 0 Channel claimed.
 * @retval -EBUSY The channel is busy.
 * @retval -EAGAIN Waiting period timed out.
 * @retval -EFAULT A parameter is incorrect, or the function context is invalid (inside an ISR). The
 * function only returns this value when the @kconfig{CONFIG_ZBUS_ASSERT_MOCK} is enabled.
 */
int zbus_chan_claim(const struct zbus_channel *chan, k_timeout_t timeout);

/**
 * @brief Finish a channel claim.
 *
 * This routine finishes a channel claim. After calling this routine with success, the channel will
 * be able to be used by other thread.
 *
 * @warning This routine must only be used after a zbus_chan_claim.
 *
 * @param chan The channel's reference.
 *
 * @retval 0 Channel finished.
 * @retval -EFAULT A parameter is incorrect, or the function context is invalid (inside an ISR). The
 * function only returns this value when the @kconfig{CONFIG_ZBUS_ASSERT_MOCK} is enabled.
 */
int zbus_chan_finish(const struct zbus_channel *chan);

/**
 * @brief Force a channel notification.
 *
 * This routine forces the event dispatcher to notify the channel's observers even if the message
 * has no changes. Note this function could be useful after claiming/finishing actions.
 *
 * @param chan The channel's reference.
 * @param timeout Waiting period to notify the channel,
 *                or one of the special values K_NO_WAIT and K_FOREVER.
 *
 * @retval 0 Channel notified.
 * @retval -EBUSY The channel's semaphore returned without waiting.
 * @retval -EAGAIN Timeout to take the channel's semaphore.
 * @retval -ENOMEM There is not more buffer on the messgage buffers pool.
 * @retval -EFAULT A parameter is incorrect, the notification could not be sent to one or more
 * observer, or the function context is invalid (inside an ISR). The function only returns this
 * value when the @kconfig{CONFIG_ZBUS_ASSERT_MOCK} is enabled.
 */
int zbus_chan_notify(const struct zbus_channel *chan, k_timeout_t timeout);

#if defined(CONFIG_ZBUS_CHANNEL_NAME) || defined(__DOXYGEN__)

/**
 * @brief Get the channel's name.
 *
 * This routine returns the channel's name reference.
 *
 * @param chan The channel's reference.
 *
 * @return Channel's name reference.
 */
static inline const char *zbus_chan_name(const struct zbus_channel *chan)
{
	__ASSERT(chan != NULL, "chan is required");

	return chan->name;
}

#endif

/**
 * @brief Get the reference for a channel message directly.
 *
 * This routine returns the reference of a channel message.
 *
 * @warning This function must only be used directly for already locked channels. This
 * can be done inside a listener for the receiving channel or after claim a channel.
 *
 * @param chan The channel's reference.
 *
 * @return Channel's message reference.
 */
static inline void *zbus_chan_msg(const struct zbus_channel *chan)
{
	__ASSERT(chan != NULL, "chan is required");

	return chan->message;
}

/**
 * @brief Get a constant reference for a channel message directly.
 *
 * This routine returns a constant reference of a channel message. This should be used
 * inside listeners to access the message directly. In this way zbus prevents the listener of
 * changing the notifying channel's message during the notification process.
 *
 * @warning This function must only be used directly for already locked channels. This
 * can be done inside a listener for the receiving channel or after claim a channel.
 *
 * @param chan The channel's constant reference.
 *
 * @return A constant channel's message reference.
 */
static inline const void *zbus_chan_const_msg(const struct zbus_channel *chan)
{
	__ASSERT(chan != NULL, "chan is required");

	return chan->message;
}

/**
 * @brief Get the channel's message size.
 *
 * This routine returns the channel's message size.
 *
 * @param chan The channel's reference.
 *
 * @return Channel's message size.
 */
static inline uint16_t zbus_chan_msg_size(const struct zbus_channel *chan)
{
	__ASSERT(chan != NULL, "chan is required");

	return chan->message_size;
}

/**
 * @brief Get the channel's user data.
 *
 * This routine returns the channel's user data.
 *
 * @param chan The channel's reference.
 *
 * @return Channel's user data.
 */
static inline void *zbus_chan_user_data(const struct zbus_channel *chan)
{
	__ASSERT(chan != NULL, "chan is required");

	return chan->user_data;
}

#if defined(CONFIG_ZBUS_MSG_SUBSCRIBER_NET_BUF_POOL_ISOLATION) || defined(__DOXYGEN__)

/**
 * @brief Set the channel's msg subscriber `net_buf` pool.
 *
 * @param chan The channel's reference.
 * @param pool The reference to the `net_buf` memory pool.
 */
static inline void zbus_chan_set_msg_sub_pool(const struct zbus_channel *chan,
					      struct net_buf_pool *pool)
{
	__ASSERT(chan != NULL, "chan is required");
	__ASSERT(pool != NULL, "pool is required");

	chan->data->msg_subscriber_pool = pool;
}

#endif /* ZBUS_MSG_SUBSCRIBER_NET_BUF_POOL_ISOLATION */

#if defined(CONFIG_ZBUS_RUNTIME_OBSERVERS) || defined(__DOXYGEN__)

/**
 * @brief Add an observer to a channel.
 *
 * This routine adds an observer to the channel.
 *
 * @param chan The channel's reference.
 * @param obs The observer's reference to be added.
 * @param timeout Waiting period to add an observer,
 *                or one of the special values K_NO_WAIT and K_FOREVER.
 *
 * @retval 0 Observer added to the channel.
 * @retval -EALREADY The observer is already present in the channel's runtime observers list.
 * @retval -ENOMEM Returned without waiting.
 * @retval -EAGAIN Waiting period timed out.
 * @retval -EINVAL Some parameter is invalid.
 */
int zbus_chan_add_obs(const struct zbus_channel *chan, const struct zbus_observer *obs,
		      k_timeout_t timeout);

/**
 * @brief Remove an observer from a channel.
 *
 * This routine removes an observer to the channel.
 *
 * @param chan The channel's reference.
 * @param obs The observer's reference to be removed.
 * @param timeout Waiting period to remove an observer,
 *                or one of the special values K_NO_WAIT and K_FOREVER.
 *
 * @retval 0 Observer removed to the channel.
 * @retval -EINVAL Invalid data supplied.
 * @retval -EBUSY Returned without waiting.
 * @retval -EAGAIN Waiting period timed out.
 * @retval -ENODATA no observer found in channel's runtime observer list.
 * @retval -ENOMEM Returned without waiting.
 */
int zbus_chan_rm_obs(const struct zbus_channel *chan, const struct zbus_observer *obs,
		     k_timeout_t timeout);

/** @cond INTERNAL_HIDDEN */

struct zbus_observer_node {
	sys_snode_t node;
	const struct zbus_observer *obs;
};

/** @endcond */

#endif /* CONFIG_ZBUS_RUNTIME_OBSERVERS */

/**
 * @brief Change the observer state.
 *
 * This routine changes the observer state. A channel when disabled will not receive
 * notifications from the event dispatcher.
 *
 * @param[in] obs The observer's reference.
 * @param[in] enabled State to be. When false the observer stops to receive notifications.
 *
 * @retval 0 Observer set enable.
 * @retval -EFAULT A parameter is incorrect, or the function context is invalid (inside an ISR). The
 * function only returns this value when the @kconfig{CONFIG_ZBUS_ASSERT_MOCK} is enabled.
 */
int zbus_obs_set_enable(struct zbus_observer *obs, bool enabled);

/**
 * @brief Get the observer state.
 *
 * This routine retrieves the observer state.
 *
 * @param[in] obs The observer's reference.
 * @param[out] enable The boolean output's reference.
 *
 * @return Observer state.
 */
static inline int zbus_obs_is_enabled(struct zbus_observer *obs, bool *enable)
{
	_ZBUS_ASSERT(obs != NULL, "obs is required");
	_ZBUS_ASSERT(enable != NULL, "enable is required");

	*enable = obs->data->enabled;

	return 0;
}

/**
 * @brief Mask notifications from a channel to an observer.
 *
 * The observer can mask notifications from a specific observing channel by calling this function.
 *
 * @param obs The observer's reference to be added.
 * @param chan The channel's reference.
 * @param masked The mask state. When the mask is true, the observer will not receive notifications
 * from the channel.
 *
 * @retval 0 Channel notifications masked to the observer.
 * @retval -ESRCH No observation found for the related pair chan/obs.
 * @retval -EINVAL Some parameter is invalid.
 */
int zbus_obs_set_chan_notification_mask(const struct zbus_observer *obs,
					const struct zbus_channel *chan, bool masked);

/**
 * @brief Get the notifications masking state from a channel to an observer.
 *
 * @param obs The observer's reference to be added.
 * @param chan The channel's reference.
 * @param[out] masked The mask state. When the mask is true, the observer will not receive
 * notifications from the channel.
 *
 * @retval 0 Retrieved the masked state.
 * @retval -ESRCH No observation found for the related pair chan/obs.
 * @retval -EINVAL Some parameter is invalid.
 */
int zbus_obs_is_chan_notification_masked(const struct zbus_observer *obs,
					 const struct zbus_channel *chan, bool *masked);

#if defined(CONFIG_ZBUS_OBSERVER_NAME) || defined(__DOXYGEN__)

/**
 * @brief Get the observer's name.
 *
 * This routine returns the observer's name reference.
 *
 * @param obs The observer's reference.
 *
 * @return The observer's name reference.
 */
static inline const char *zbus_obs_name(const struct zbus_observer *obs)
{
	__ASSERT(obs != NULL, "obs is required");

	return obs->name;
}

#endif

#if defined(CONFIG_ZBUS_PRIORITY_BOOST) || defined(__DOXYGEN__)

/**
 * @brief Set the observer thread priority by attaching it to a thread.
 *
 * @param[in] obs The observer's reference.
 *
 * @retval 0 Observer detached from the thread.
 * @retval -EFAULT A parameter is incorrect, or the function context is invalid (inside an ISR). The
 * function only returns this value when the @kconfig{CONFIG_ZBUS_ASSERT_MOCK} is enabled.
 */
int zbus_obs_attach_to_thread(const struct zbus_observer *obs);

/**
 * @brief Clear the observer thread priority by detaching it from a thread.
 *
 * @param[in] obs The observer's reference.
 *
 * @retval 0 Observer detached from the thread.
 * @retval -EFAULT A parameter is incorrect, or the function context is invalid (inside an ISR). The
 * function only returns this value when the @kconfig{CONFIG_ZBUS_ASSERT_MOCK} is enabled.
 */
int zbus_obs_detach_from_thread(const struct zbus_observer *obs);

#endif /* CONFIG_ZBUS_PRIORITY_BOOST */

/**
 * @brief Wait for a channel notification.
 *
 * This routine makes the subscriber to wait a notification. The notification comes as a channel
 * reference.
 *
 * @param[in] sub The subscriber's reference.
 * @param[out] chan The notification channel's reference.
 * @param[in] timeout Waiting period for a notification arrival,
 *                or one of the special values K_NO_WAIT and K_FOREVER.
 *
 * @retval 0 Notification received.
 * @retval -ENOMSG Returned without waiting.
 * @retval -EAGAIN Waiting period timed out.
 * @retval -EINVAL The observer is not a subscriber.
 * @retval -EFAULT A parameter is incorrect, or the function context is invalid (inside an ISR). The
 * function only returns this value when the @kconfig{CONFIG_ZBUS_ASSERT_MOCK} is enabled.
 */
int zbus_sub_wait(const struct zbus_observer *sub, const struct zbus_channel **chan,
		  k_timeout_t timeout);

#if defined(CONFIG_ZBUS_MSG_SUBSCRIBER) || defined(__DOXYGEN__)

/**
 * @brief Wait for a channel message.
 *
 * This routine makes the subscriber wait for the new message in case of channel publication.
 *
 * @param[in] sub The subscriber's reference.
 * @param[out] chan The notification channel's reference.
 * @param[out] msg A reference to a copy of the published message.
 * @param[in] timeout Waiting period for a notification arrival,
 *                or one of the special values, K_NO_WAIT and K_FOREVER.
 *
 * @retval 0 Message received.
 * @retval -EINVAL The observer is not a subscriber.
 * @retval -ENOMSG Could not retrieve the net_buf from the subscriber FIFO.
 * @retval -EILSEQ Received an invalid channel reference.
 * @retval -EFAULT A parameter is incorrect, or the function context is invalid (inside an ISR). The
 * function only returns this value when the @kconfig{CONFIG_ZBUS_ASSERT_MOCK} is enabled.
 */
int zbus_sub_wait_msg(const struct zbus_observer *sub, const struct zbus_channel **chan, void *msg,
		      k_timeout_t timeout);

#endif /* CONFIG_ZBUS_MSG_SUBSCRIBER */

/**
 *
 * @brief Iterate over channels.
 *
 * Enables the developer to iterate over the channels giving to this function an
 * iterator_func which is called for each channel. If the iterator_func returns false all
 * the iteration stops.
 *
 * @param[in] iterator_func The function that will be execute on each iteration.
 *
 * @retval true Iterator executed for all channels.
 * @retval false Iterator could not be executed. Some iterate returned false.
 */
bool zbus_iterate_over_channels(bool (*iterator_func)(const struct zbus_channel *chan));
/**
 *
 * @brief Iterate over channels with user data.
 *
 * Enables the developer to iterate over the channels giving to this function an
 * iterator_func which is called for each channel. If the iterator_func returns false all
 * the iteration stops.
 *
 * @param[in] iterator_func The function that will be execute on each iteration.
 * @param[in] user_data The user data that can be passed in the function.
 *
 * @retval true Iterator executed for all channels.
 * @retval false Iterator could not be executed. Some iterate returned false.
 */
bool zbus_iterate_over_channels_with_user_data(
	bool (*iterator_func)(const struct zbus_channel *chan, void *user_data), void *user_data);

/**
 *
 * @brief Iterate over observers.
 *
 * Enables the developer to iterate over the observers giving to this function an
 * iterator_func which is called for each observer. If the iterator_func returns false all
 * the iteration stops.
 *
 * @param[in] iterator_func The function that will be execute on each iteration.
 *
 * @retval true Iterator executed for all channels.
 * @retval false Iterator could not be executed. Some iterate returned false.
 */
bool zbus_iterate_over_observers(bool (*iterator_func)(const struct zbus_observer *obs));
/**
 *
 * @brief Iterate over observers with user data.
 *
 * Enables the developer to iterate over the observers giving to this function an
 * iterator_func which is called for each observer. If the iterator_func returns false all
 * the iteration stops.
 *
 * @param[in] iterator_func The function that will be execute on each iteration.
 * @param[in] user_data The user data that can be passed in the function.
 *
 * @retval true Iterator executed for all channels.
 * @retval false Iterator could not be executed. Some iterate returned false.
 */
bool zbus_iterate_over_observers_with_user_data(
	bool (*iterator_func)(const struct zbus_observer *obs, void *user_data), void *user_data);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZBUS_H_ */
