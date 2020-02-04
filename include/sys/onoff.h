/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_ONOFF_H_
#define ZEPHYR_INCLUDE_SYS_ONOFF_H_

#include <kernel.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup resource_mgmt_apis Resource Management APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Flag fields used to specify on-off service behavior.
 */
enum onoff_service_flags {
	/**
	 * @brief Flag used in struct onoff_service_transitions.
	 *
	 * When provided this indicates the start transition function
	 * may cause the calling thread to wait.  This blocks attempts
	 * to initiate a transition from a non-thread context.
	 */
	ONOFF_SERVICE_START_SLEEPS      = BIT(0),

	/**
	 * @brief Flag used in struct onoff_service_transitions.
	 *
	 * As with @ref ONOFF_SERVICE_START_SLEEPS but describing the
	 * stop transition function.
	 */
	ONOFF_SERVICE_STOP_SLEEPS       = BIT(1),

	/**
	 * @brief Flag used in struct onoff_service_transitions.
	 *
	 * As with @ref ONOFF_SERVICE_START_SLEEPS but describing the
	 * reset transition function.
	 */
	ONOFF_SERVICE_RESET_SLEEPS      = BIT(2),

	/* Internal use. */
	ONOFF_SERVICE_HAS_ERROR         = BIT(3),

	/* This and higher bits reserved for internal use. */
	ONOFF_SERVICE_INTERNAL_BASE     = BIT(4),
};

/* Forward declaration */
struct onoff_service;

/**
 * @brief Signature used to notify an on-off service that a transition
 * has completed.
 *
 * Functions of this type are passed to service-specific transition
 * functions to be used to report the completion of the operation.
 * The functions may be invoked from any context.
 *
 * @param srv the service for which transition was requested.
 *
 * @param res the result of the transition.  This shall be
 * non-negative on success, or a negative error code.  If an error is
 * indicated the service shall enter an error state.
 */
typedef void (*onoff_service_notify_fn)(struct onoff_service *srv,
					int res);

/**
 * @brief Signature used by service implementations to effect a
 * transition.
 *
 * Service definitions use two function pointers of this type to be
 * notified that a transition is required, and a third optional one to
 * reset service state.
 *
 * The start function will be called only from the off state.
 *
 * The stop function will be called only from the on state.
 *
 * The reset function may be called only when
 * onoff_service_has_error() returns true.
 *
 * @param srv the service for which transition was requested.
 *
 * @param notify the function to be invoked when the transition has
 * completed.  The callee shall capture this parameter to notify on
 * completion of asynchronous transitions.  If the transition is not
 * asynchronous, notify shall be invoked before the transition
 * function returns.
 */
typedef void (*onoff_service_transition_fn)(struct onoff_service *srv,
					    onoff_service_notify_fn notify);

/** @brief On-off service transition functions. */
struct onoff_service_transitions {
	/* Function to invoke to transition the service to on. */
	onoff_service_transition_fn start;

	/* Function to invoke to transition the service to off. */
	onoff_service_transition_fn stop;

	/* Function to force the service state to reset, where supported. */
	onoff_service_transition_fn reset;

	/* Flags identifying transition function capabilities. */
	u8_t flags;
};

/**
 * @brief State associated with an on-off service.
 *
 * No fields in this structure are intended for use by service
 * providers or clients.  The state is to be initialized once, using
 * onoff_service_init(), when the service provider is initialized.
 * In case of error it may be reset through the
 * onoff_service_reset() API.
 */
struct onoff_service {
	/* List of clients waiting for completion of reset or
	 * transition to on.
	 */
	sys_slist_t clients;

	/* Transition functions. */
	const struct onoff_service_transitions *transitions;

	/* Mutex protection for flags, clients, releaser, and refs. */
	struct k_spinlock lock;

	/* Client to be informed when transition to off completes. */
	struct onoff_client *releaser;

	/* Flags identifying the service state. */
	u16_t flags;

	/* Number of active clients for the service. */
	u16_t refs;
};

/** @brief Initializer of transitions structure.
 *
 * @param _start a function used to transition from off to on state.
 *
 * @param _stop a function used to transition from on to off state.
 *
 * @param _reset a function used to clear errors and force the service to an off
 * state. Can be null.
 *
 * @param _flags any or all of the flags from enum onoff_service_flags.
 */
#define ONOFF_SERVICE_TRANSITIONS_INITIALIZER(_start, _stop, _reset, _flags) { \
		.start = _start,					       \
		.stop = _stop,						       \
		.reset = _reset,					       \
		.flags = _flags,					       \
}

/** @internal */
#define ONOFF_SERVICE_INITIALIZER(_transitions) { \
		.transitions = _transitions,	  \
		.flags = (_transitions)->flags,	  \
}

/**
 * @brief Initialize an on-off service to off state.
 *
 * This function must be invoked exactly once per service instance, by
 * the infrastructure that provides the service, and before any other
 * on-off service API is invoked on the service.
 *
 * This function should never be invoked by clients of an on-off service.
 *
 * @param srv the service definition object to be initialized.
 *
 * @param transitions A structure with transition functions. Structure must be
 * persistent as it is used by the service.
 *
 * @retval 0 on success
 * @retval -EINVAL if start, stop, or flags are invalid
 */
int onoff_service_init(struct onoff_service *srv,
		       const struct onoff_service_transitions *transitions);

/** @internal
 *
 * Flag fields used to specify on-off client behavior.
 *
 * These flags control whether calls to onoff_service_request() and
 * onoff_service_release() are synchronous or asynchronous, and for
 * asynchronous operations how the operation result is communicated to
 * the client.
 */
enum onoff_client_flags {
	/* Known-invalid field, used in validation */
	ONOFF_CLIENT_NOTIFY_INVALID     = 0,

	/*
	 * Indicates that no notification will be provided.
	 *
	 * Callers must check for completions using
	 * onoff_client_fetch_result().
	 *
	 * See onoff_client_init_spinwait().
	 */
	ONOFF_CLIENT_NOTIFY_SPINWAIT    = 1,

	/*
	 * Select notification through @ref k_poll signal
	 *
	 * See onoff_client_init_signal().
	 */
	ONOFF_CLIENT_NOTIFY_SIGNAL      = 2,

	/**
	 * Select notification through a user-provided callback.
	 *
	 * See onoff_client_init_callback().
	 */
	ONOFF_CLIENT_NOTIFY_CALLBACK    = 3,
};

/* Forward declaration */
struct onoff_client;

/**
 * @brief Signature used to notify an on-off service client of the
 * completion of an operation.
 *
 * These functions may be invoked from any context including
 * pre-kernel, ISR, or cooperative or pre-emptible threads.
 * Compatible functions must be isr-callable and non-suspendable.
 *
 * @param srv the service for which the operation was initiated.
 *
 * @param cli the client structure passed to the function that
 * initiated the operation.
 *
 * @param user_data user data provided when the client structure was
 * initialized with onoff_client_init_callback().
 *
 * @param res the result of the operation.  Expected values are
 * service-specific, but the value shall be non-negative if the
 * operation succeeded, and negative if the operation failed.
 */
typedef void (*onoff_client_callback)(struct onoff_service *srv,
				      struct onoff_client *cli,
				      void *user_data,
				      int res);

/**
 * @brief State associated with a client of an on-off service.
 *
 * Objects of this type are allocated by a client, which must use an
 * initialization function (e.g. onoff_client_init_signal()) to
 * configure them.
 *
 * Control of the object content transfers to the service provider
 * when a pointer to the object is passed to any on-off service
 * function.  While the service provider controls the object the
 * client must not change any object fields.  Control reverts to the
 * client:
 * * if the call to the service API returns an error;
 * * if the call to the service API succeeds for a no-wait operation;
 * * when operation completion is posted (signalled or callback
 *   invoked).
 *
 * After control has reverted to the client the state object must be
 * reinitialized for the next operation.
 *
 * The content of this structure is not public API: all configuration
 * and inspection should be done with functions like
 * onoff_client_init_callback() and onoff_client_fetch_result().
 */
struct onoff_client {
	/* Links the client into the set of waiting service users. */
	sys_snode_t node;

	union async {
		/* Pointer to signal used to notify client.
		 *
		 * The signal value corresponds to the res parameter
		 * of onoff_client_callback.
		 */
		struct k_poll_signal *signal;

		/* Handler and argument for callback notification. */
		struct callback {
			onoff_client_callback handler;
			void *user_data;
		} callback;
	} async;

	/*
	 * The result of the operation.
	 *
	 * This is the value that was (or would be) passed to the
	 * async infrastructure.  This field is the sole record of
	 * success or failure for no-wait synchronous operations.
	 */
	int volatile result;

	/* Flags recording client state. */
	u32_t volatile flags;
};

/**
 * @brief Check for and read the result of an asynchronous operation.
 *
 * @param op pointer to the object used to specify asynchronous
 * function behavior and store completion information.
 *
 * @param result pointer to storage for the result of the operation.
 * The result is stored only if the operation has completed.
 *
 * @retval 0 if the operation has completed.
 * @retval -EAGAIN if the operation has not completed.
 */
static inline int onoff_client_fetch_result(const struct onoff_client *op,
					    int *result)
{
	__ASSERT_NO_MSG(op != NULL);
	__ASSERT_NO_MSG(result != NULL);

	int rv = -EAGAIN;

	if (op->flags == 0U) {
		rv = 0;
		*result = op->result;
	}
	return rv;
}

/**
 * @brief Initialize an on-off client to be used for a spin-wait
 * operation notification.
 *
 * Clients that use this initialization receive no asynchronous
 * notification, and instead must periodically check for completion
 * using onoff_client_fetch_result().
 *
 * On completion of the operation the client object must be
 * reinitialized before it can be re-used.
 *
 * @param cli pointer to the client state object.
 */
static inline void onoff_client_init_spinwait(struct onoff_client *cli)
{
	__ASSERT_NO_MSG(cli != NULL);

	*cli = (struct onoff_client){
		.flags = ONOFF_CLIENT_NOTIFY_SPINWAIT,
	};
}

/**
 * @brief Initialize an on-off client to be used for a signal
 * operation notification.
 *
 * Clients that use this initialization will be notified of the
 * completion of operations submitted through onoff_request() and
 * onoff_release() through the provided signal.
 *
 * On completion of the operation the client object must be
 * reinitialized before it can be re-used.
 *
 * @note
 *   @rst
 *   This capability is available only when :option:`CONFIG_POLL` is
 *   selected.
 *   @endrst
 *
 * @param cli pointer to the client state object.
 *
 * @param sigp pointer to the signal to use for notification.  The
 * value must not be null.  The signal must be reset before the client
 * object is passed to the on-off service API.
 */
static inline void onoff_client_init_signal(struct onoff_client *cli,
					    struct k_poll_signal *sigp)
{
	__ASSERT_NO_MSG(cli != NULL);
	__ASSERT_NO_MSG(sigp != NULL);

	*cli = (struct onoff_client){
#ifdef CONFIG_POLL
		.async = {
			.signal = sigp,
		},
#endif /* CONFIG_POLL */
		.flags = ONOFF_CLIENT_NOTIFY_SIGNAL,
	};
}

/**
 * @brief Initialize an on-off client to be used for a callback
 * operation notification.
 *
 * Clients that use this initialization will be notified of the
 * completion of operations submitted through on-off service API
 * through the provided callback.  Note that callbacks may be invoked
 * from various contexts depending on the specific service; see
 * @ref onoff_client_callback.
 *
 * On completion of the operation the client object must be
 * reinitialized before it can be re-used.
 *
 * @param cli pointer to the client state object.
 *
 * @param handler a function pointer to use for notification.
 *
 * @param user_data an opaque pointer passed to the handler to provide
 * additional context.
 */
static inline void onoff_client_init_callback(struct onoff_client *cli,
					      onoff_client_callback handler,
					      void *user_data)
{
	__ASSERT_NO_MSG(cli != NULL);
	__ASSERT_NO_MSG(handler != NULL);

	*cli = (struct onoff_client){
		.async = {
			.callback = {
				.handler = handler,
				.user_data = user_data,
			},
		},
		.flags = ONOFF_CLIENT_NOTIFY_CALLBACK,
	};
}

/**
 * @brief Request a reservation to use an on-off service.
 *
 * The return value indicates the success or failure of an attempt to
 * initiate an operation to request the resource be made available.
 * If initiation of the operation succeeds the result of the request
 * operation is provided through the configured client notification
 * method, possibly before this call returns.
 *
 * Note that the call to this function may succeed in a case where the
 * actual request fails.  Always check the operation completion
 * result.
 *
 * As a specific example: A call to this function may succeed at a
 * point while the service is still transitioning to off due to a
 * previous call to onoff_release().  When the transition completes
 * the service would normally start a transition to on.  However, if
 * the transition to off completed in a non-thread context, and the
 * transition to on can sleep, the transition cannot be started and
 * the request will fail with `-EWOULDBLOCK`.
 *
 * @param srv the service that will be used.
 *
 * @param cli a non-null pointer to client state providing
 * instructions on synchronous expectations and how to notify the
 * client when the request completes.  Behavior is undefined if client
 * passes a pointer object associated with an incomplete service
 * operation.
 *
 * @retval Non-negative on successful (initiation of) request
 * @retval -EIO if service has recorded an an error
 * @retval -EINVAL if the parameters are invalid
 * @retval -EAGAIN if the reference count would overflow
 * @retval -EWOULDBLOCK if the function was invoked from non-thread
 * context and successful initiation could result in an attempt to
 * make the calling thread sleep.
 */
int onoff_request(struct onoff_service *srv,
		  struct onoff_client *cli);

/**
 * @brief Release a reserved use of an on-off service.
 *
 * The return value indicates the success or failure of an attempt to
 * initiate an operation to release the resource.  If initiation of
 * the operation succeeds the result of the release operation itself
 * is provided through the configured client notification method,
 * possibly before this call returns.
 *
 * Note that the call to this function may succeed in a case where the
 * actual release fails.  Always check the operation completion
 * result.
 *
 * @param srv the service that will be used.
 *
 * @param cli a non-null pointer to client state providing
 * instructions on how to notify the client when release completes.
 * Behavior is undefined if cli references an object associated with
 * an incomplete service operation.
 *
 * @retval Non-negative on successful (initiation of) release
 * @retval -EINVAL if the parameters are invalid
 * @retval -EIO if service has recorded an an error
 * @retval -EWOULDBLOCK if a non-blocking request was made and
 *         could not be satisfied without potentially blocking.
 * @retval -EALREADY if the service is already off or transitioning
 *         to off
 * @retval -EBUSY if the service is transitioning to on
 */
int onoff_release(struct onoff_service *srv,
		  struct onoff_client *cli);

/**
 * @brief Test whether an on-off service has recorded an error.
 *
 * This function can be used to determine whether the service has
 * recorded an error.  Errors may be cleared by invoking
 * onoff_service_reset().
 *
 * @return true if and only if the service has an uncleared error.
 */
static inline bool onoff_service_has_error(const struct onoff_service *srv)
{
	return (srv->flags & ONOFF_SERVICE_HAS_ERROR) != 0;
}

/**
 * @brief Clear errors on an on-off service and reset it to its off
 * state.
 *
 * A service can only be reset when it is in an error state as
 * indicated by onoff_service_has_error().
 *
 * The return value indicates the success or failure of an attempt to
 * initiate an operation to reset the resource.  If initiation of the
 * operation succeeds the result of the reset operation itself is
 * provided through the configured client notification method,
 * possibly before this call returns.  Multiple clients may request a
 * reset; all are notified when it is complete.
 *
 * Note that the call to this function may succeed in a case where the
 * actual reset fails.  Always check the operation completion result.
 *
 * This function is blocking if the reset transition is blocking,
 * unless client notification specifies no-wait.
 *
 * @note Due to the conditions on state transition all incomplete
 * asynchronous operations will have been informed of the error when
 * it occurred.  There need be no concern about dangling requests left
 * after a reset completes.
 *
 * @param srv the service to be reset.
 *
 * @param cli pointer to client state, including instructions on how
 * to notify the client when reset completes.  Behavior is undefined
 * if cli references an object associated with an incomplete service
 * operation.
 *
 * @retval 0 on success
 * @retval -ENOTSUP if reset is not supported
 * @retval -EINVAL if the parameters are invalid, or if the service
 * @retval -EALREADY if the service does not have a recorded error
 */
int onoff_service_reset(struct onoff_service *srv,
			struct onoff_client *cli);

/**
 * @brief Attempt to cancel an in-progress client operation.
 *
 * It may be that a client has initiated an operation but needs to
 * shut down before the operation has completed.  For example, when a
 * request was made and the need is no longer present.
 *
 * There is limited support for cancelling an in-progress operation:
 * * If a start or reset is in progress, all but one clients
 *   requesting the start can cancel their request.
 * * If a stop is in progress, all clients requesting a restart can
 *   cancel their request;
 * * A client requesting a release cannot cancel the release.
 *
 * Be aware that any transition that was initiated on behalf of the
 * client will continue to progress to completion.  The restricted
 * support for cancellation ensures that for any in-progress
 * transition there will always be at least one client that will be
 * notified when the operation completes.
 *
 * If the cancellation fails the service retains control of the client
 * object, and the client must wait for operation completion.
 *
 * @param srv the service for which an operation is to be cancelled.
 *
 * @param cli a pointer to the same client state that was provided
 * when the operation to be cancelled was issued.  If the cancellation
 * is successful the client will be notified of operation completion
 * with a result of `-ECANCELED`.
 *
 * @retval 0 if the cancellation was completed before the client could
 * be notified.  The client will be notified through cli with an
 * operation completion of `-ECANCELED`.
 * @retval -EINVAL if the parameters are invalid.
 * @retval -EWOULDBLOCK if cancellation was rejected because the
 * client is the only waiter for an in-progress transition.  The
 * service retains control of the client structure.
 * @retval -EALREADY if cli was not a record of an uncompleted
 * notification at the time the cancellation was processed.  This
 * likely indicates that the operation and client notification had
 * already completed.
 */
int onoff_cancel(struct onoff_service *srv,
		 struct onoff_client *cli);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_ONOFF_H_ */
