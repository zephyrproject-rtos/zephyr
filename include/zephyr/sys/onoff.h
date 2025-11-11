/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_ONOFF_H_
#define ZEPHYR_INCLUDE_SYS_ONOFF_H_

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/sys/notify.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup resource_mgmt_onoff_apis On-Off Service APIs
 * @ingroup os_services
 * @{
 */

/**
 * @brief Flag indicating an error state.
 *
 * Error states are cleared using onoff_reset().
 */
#define ONOFF_FLAG_ERROR BIT(0)

/** @cond INTERNAL_HIDDEN */
#define ONOFF_FLAG_ONOFF BIT(1)
#define ONOFF_FLAG_TRANSITION BIT(2)
/** @endcond */

/**
 * @brief Mask used to isolate bits defining the service state.
 *
 * Mask a value with this then test for ONOFF_FLAG_ERROR to determine
 * whether the machine has an unfixed error, or compare against
 * ONOFF_STATE_ON, ONOFF_STATE_OFF, ONOFF_STATE_TO_ON,
 * ONOFF_STATE_TO_OFF, or ONOFF_STATE_RESETTING.
 */
#define ONOFF_STATE_MASK (ONOFF_FLAG_ERROR   \
			  | ONOFF_FLAG_ONOFF \
			  | ONOFF_FLAG_TRANSITION)

/**
 * @brief Value exposed by ONOFF_STATE_MASK when service is off.
 */
#define ONOFF_STATE_OFF 0U

/**
 * @brief Value exposed by ONOFF_STATE_MASK when service is on.
 */
#define ONOFF_STATE_ON ONOFF_FLAG_ONOFF

/**
 * @brief Value exposed by ONOFF_STATE_MASK when the service is in an
 * error state (and not in the process of resetting its state).
 */
#define ONOFF_STATE_ERROR ONOFF_FLAG_ERROR

/**
 * @brief Value exposed by ONOFF_STATE_MASK when service is
 * transitioning to on.
 */
#define ONOFF_STATE_TO_ON (ONOFF_FLAG_TRANSITION | ONOFF_STATE_ON)

/**
 * @brief Value exposed by ONOFF_STATE_MASK when service is
 * transitioning to off.
 */
#define ONOFF_STATE_TO_OFF (ONOFF_FLAG_TRANSITION | ONOFF_STATE_OFF)

/**
 * @brief Value exposed by ONOFF_STATE_MASK when service is in the
 * process of resetting.
 */
#define ONOFF_STATE_RESETTING (ONOFF_FLAG_TRANSITION | ONOFF_STATE_ERROR)

/* Forward declarations */
struct onoff_manager;
struct onoff_monitor;

/**
 * @brief Signature used to notify an on-off manager that a transition
 * has completed.
 *
 * Functions of this type are passed to service-specific transition
 * functions to be used to report the completion of the operation.
 * The functions may be invoked from any context.
 *
 * @param mgr the manager for which transition was requested.
 *
 * @param res the result of the transition.  This shall be
 * non-negative on success, or a negative error code.  If an error is
 * indicated the service shall enter an error state.
 */
typedef void (*onoff_notify_fn)(struct onoff_manager *mgr,
				int res);

/**
 * @brief Signature used by service implementations to effect a
 * transition.
 *
 * Service definitions use two required function pointers of this type
 * to be notified that a transition is required, and a third optional
 * one to reset the service when it is in an error state.
 *
 * The start function will be called only from the off state.
 *
 * The stop function will be called only from the on state.
 *
 * The reset function (where supported) will be called only when
 * onoff_has_error() returns true.
 *
 * @note All transitions functions must be isr-ok.
 *
 * @param mgr the manager for which transition was requested.
 *
 * @param notify the function to be invoked when the transition has
 * completed.  If the transition is synchronous, notify shall be
 * invoked by the implementation before the transition function
 * returns.  Otherwise the implementation shall capture this parameter
 * and invoke it when the transition completes.
 */
typedef void (*onoff_transition_fn)(struct onoff_manager *mgr,
				    onoff_notify_fn notify);

/** @brief On-off service transition functions. */
struct onoff_transitions {
	/** Function to invoke to transition the service to on. */
	onoff_transition_fn start;

	/** Function to invoke to transition the service to off. */
	onoff_transition_fn stop;

	/** Function to force the service state to reset, where
	 * supported.
	 */
	onoff_transition_fn reset;
};

/**
 * @brief State associated with an on-off manager.
 *
 * No fields in this structure are intended for use by service
 * providers or clients.  The state is to be initialized once, using
 * onoff_manager_init(), when the service provider is initialized.  In
 * case of error it may be reset through the onoff_reset() API.
 */
struct onoff_manager {
	/** List of clients waiting for request or reset completion
	 * notifications.
	 */
	sys_slist_t clients;

	/** List of monitors to be notified of state changes including
	 * errors and transition completion.
	 */
	sys_slist_t monitors;

	/** Transition functions. */
	const struct onoff_transitions *transitions;

	/** Mutex protection for other fields. */
	struct k_spinlock lock;

	/** The result of the last transition. */
	int last_res;

	/** Flags identifying the service state. */
	uint16_t flags;

	/** Number of active clients for the service. */
	uint16_t refs;
};

/** @brief Initializer for a onoff_transitions object.
 *
 * @param _start a function used to transition from off to on state.
 *
 * @param _stop a function used to transition from on to off state.
 *
 * @param _reset a function used to clear errors and force the service
 * to an off state. Can be null.
 */
#define ONOFF_TRANSITIONS_INITIALIZER(_start, _stop, _reset) { \
		.start = (_start),			       \
		.stop = (_stop),			       \
		.reset = (_reset),			       \
}

/** @cond INTERNAL_HIDDEN */
#define ONOFF_MANAGER_INITIALIZER(_transitions) { \
		.transitions = (_transitions),	  \
}
/** @endcond */

/**
 * @brief Initialize an on-off service to off state.
 *
 * This function must be invoked exactly once per service instance, by
 * the infrastructure that provides the service, and before any other
 * on-off service API is invoked on the service.
 *
 * This function should never be invoked by clients of an on-off
 * service.
 *
 * @param mgr the manager definition object to be initialized.
 *
 * @param transitions pointer to a structure providing transition
 * functions.  The referenced object must persist as long as the
 * manager can be referenced.
 *
 * @retval 0 on success
 * @retval -EINVAL if start, stop, or flags are invalid
 */
int onoff_manager_init(struct onoff_manager *mgr,
		       const struct onoff_transitions *transitions);

/* Forward declaration */
struct onoff_client;

/**
 * @brief Signature used to notify an on-off service client of the
 * completion of an operation.
 *
 * These functions may be invoked from any context including
 * pre-kernel, ISR, or cooperative or pre-emptible threads.
 * Compatible functions must be isr-ok and not sleep.
 *
 * @param mgr the manager for which the operation was initiated.  This may be
 * null if the on-off service uses synchronous transitions.
 *
 * @param cli the client structure passed to the function that
 * initiated the operation.
 *
 * @param state the state of the machine at the time of completion,
 * restricted by ONOFF_STATE_MASK.  ONOFF_FLAG_ERROR must be checked
 * independently of whether res is negative as a machine error may
 * indicate that all future operations except onoff_reset() will fail.
 *
 * @param res the result of the operation.  Expected values are
 * service-specific, but the value shall be non-negative if the
 * operation succeeded, and negative if the operation failed.  If res
 * is negative ONOFF_FLAG_ERROR will be set in state, but if res is
 * non-negative ONOFF_FLAG_ERROR may still be set in state.
 */
typedef void (*onoff_client_callback)(struct onoff_manager *mgr,
				      struct onoff_client *cli,
				      uint32_t state,
				      int res);

/**
 * @brief State associated with a client of an on-off service.
 *
 * Objects of this type are allocated by a client, which is
 * responsible for zero-initializing the node field and invoking the
 * appropriate sys_notify init function to configure notification.
 *
 * Control of the object content transfers to the service provider
 * when a pointer to the object is passed to any on-off manager
 * function.  While the service provider controls the object the
 * client must not change any object fields.  Control reverts to the
 * client concurrent with release of the owned sys_notify structure,
 * or when indicated by an onoff_cancel() return value.
 *
 * After control has reverted to the client the notify field must be
 * reinitialized for the next operation.
 */
struct onoff_client {
	/** @cond INTERNAL_HIDDEN
	 *
	 * Links the client into the set of waiting service users.
	 * Applications must ensure this field is zero-initialized
	 * before use.
	 */
	sys_snode_t node;
	/** @endcond */

	/** @brief Notification configuration. */
	struct sys_notify notify;
};

/**
 * @brief Identify region of sys_notify flags available for
 * containing services.
 *
 * Bits of the flags field of the sys_notify structure contained
 * within the queued_operation structure at and above this position
 * may be used by extensions to the onoff_client structure.
 *
 * These bits are intended for use by containing service
 * implementations to record client-specific information and are
 * subject to other conditions of use specified on the sys_notify API.
 */
#define ONOFF_CLIENT_EXTENSION_POS SYS_NOTIFY_EXTENSION_POS

/**
 * @brief Test whether an on-off service has recorded an error.
 *
 * This function can be used to determine whether the service has
 * recorded an error.  Errors may be cleared by invoking
 * onoff_reset().
 *
 * This is an unlocked convenience function suitable for use only when
 * it is known that no other process might invoke an operation that
 * transitions the service between an error and non-error state.
 *
 * @return true if and only if the service has an uncleared error.
 */
static inline bool onoff_has_error(const struct onoff_manager *mgr)
{
	return (mgr->flags & ONOFF_FLAG_ERROR) != 0;
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
 * @param mgr the manager that will be used.
 *
 * @param cli a non-null pointer to client state providing
 * instructions on synchronous expectations and how to notify the
 * client when the request completes.  Behavior is undefined if client
 * passes a pointer object associated with an incomplete service
 * operation.
 *
 * @retval non-negative the observed state of the machine at the time
 * the request was processed, if successful.
 * @retval -EIO if service has recorded an error.
 * @retval -EINVAL if the parameters are invalid.
 * @retval -EAGAIN if the reference count would overflow.
 */
int onoff_request(struct onoff_manager *mgr,
		  struct onoff_client *cli);

/**
 * @brief Release a reserved use of an on-off service.
 *
 * This synchronously releases the caller's previous request.  If the
 * last request is released the manager will initiate a transition to
 * off, which can be observed by registering an onoff_monitor.
 *
 * @note Behavior is undefined if this is not paired with a preceding
 * onoff_request() call that completed successfully.
 *
 * @param mgr the manager for which a request was successful.
 *
 * @retval non-negative the observed state (ONOFF_STATE_ON) of the
 * machine at the time of the release, if the release succeeds.
 * @retval -EIO if service has recorded an error.
 * @retval -ENOTSUP if the machine is not in a state that permits
 * release.
 */
int onoff_release(struct onoff_manager *mgr);

/**
 * @brief Attempt to cancel an in-progress client operation.
 *
 * It may be that a client has initiated an operation but needs to
 * shut down before the operation has completed.  For example, when a
 * request was made and the need is no longer present.
 *
 * Cancelling is supported only for onoff_request() and onoff_reset()
 * operations, and is a synchronous operation.  Be aware that any
 * transition that was initiated on behalf of the client will continue
 * to progress to completion: it is only notification of transition
 * completion that may be eliminated.  If there are no active requests
 * when a transition to on completes the manager will initiate a
 * transition to off.
 *
 * Client notification does not occur for cancelled operations.
 *
 * @param mgr the manager for which an operation is to be cancelled.
 *
 * @param cli a pointer to the same client state that was provided
 * when the operation to be cancelled was issued.
 *
 * @retval non-negative the observed state of the machine at the time
 * of the cancellation, if the cancellation succeeds.  On successful
 * cancellation ownership of @c *cli reverts to the client.
 * @retval -EINVAL if the parameters are invalid.
 * @retval -EALREADY if cli was not a record of an uncompleted
 * notification at the time the cancellation was processed.  This
 * likely indicates that the operation and client notification had
 * already completed.
 */
int onoff_cancel(struct onoff_manager *mgr,
		 struct onoff_client *cli);

/**
 * @brief Helper function to safely cancel a request.
 *
 * Some applications may want to issue requests on an asynchronous
 * event (such as connection to a USB bus) and to release on a paired
 * event (such as loss of connection to a USB bus).  Applications
 * cannot precisely determine that an in-progress request is still
 * pending without using onoff_monitor and carefully avoiding race
 * conditions.
 *
 * This function is a helper that attempts to cancel the operation and
 * issues a release if cancellation fails because the request was
 * completed.  This synchronously ensures that ownership of the client
 * data reverts to the client so is available for a future request.
 *
 * @param mgr the manager for which an operation is to be cancelled.
 *
 * @param cli a pointer to the same client state that was provided
 * when onoff_request() was invoked.  Behavior is undefined if this is
 * a pointer to client data associated with an onoff_reset() request.
 *
 * @retval ONOFF_STATE_TO_ON if the cancellation occurred before the
 * transition completed.
 *
 * @retval ONOFF_STATE_ON if the cancellation occurred after the
 * transition completed.
 *
 * @retval -EINVAL if the parameters are invalid.
 *
 * @retval negative other errors produced by onoff_release().
 */
static inline int onoff_cancel_or_release(struct onoff_manager *mgr,
					  struct onoff_client *cli)
{
	int rv = onoff_cancel(mgr, cli);

	if (rv == -EALREADY) {
		rv = onoff_release(mgr);
	}
	return rv;
}

/**
 * @brief Clear errors on an on-off service and reset it to its off
 * state.
 *
 * A service can only be reset when it is in an error state as
 * indicated by onoff_has_error().
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
 * @note Due to the conditions on state transition all incomplete
 * asynchronous operations will have been informed of the error when
 * it occurred.  There need be no concern about dangling requests left
 * after a reset completes.
 *
 * @param mgr the manager to be reset.
 *
 * @param cli pointer to client state, including instructions on how
 * to notify the client when reset completes.  Behavior is undefined
 * if cli references an object associated with an incomplete service
 * operation.
 *
 * @retval non-negative the observed state of the machine at the time
 * of the reset, if the reset succeeds.
 * @retval -ENOTSUP if reset is not supported by the service.
 * @retval -EINVAL if the parameters are invalid.
 * @retval -EALREADY if the service does not have a recorded error.
 */
int onoff_reset(struct onoff_manager *mgr,
		struct onoff_client *cli);

/**
 * @brief Signature used to notify a monitor of an onoff service of
 * errors or completion of a state transition.
 *
 * This is similar to onoff_client_callback but provides information
 * about all transitions, not just ones associated with a specific
 * client.  Monitor callbacks are invoked before any completion
 * notifications associated with the state change are made.
 *
 * These functions may be invoked from any context including
 * pre-kernel, ISR, or cooperative or pre-emptible threads.
 * Compatible functions must be isr-ok and not sleep.
 *
 * The callback is permitted to unregister itself from the manager,
 * but must not register or unregister any other monitors.
 *
 * @param mgr the manager for which a transition has completed.
 *
 * @param mon the monitor instance through which this notification
 * arrived.
 *
 * @param state the state of the machine at the time of completion,
 * restricted by ONOFF_STATE_MASK.  All valid states may be observed.
 *
 * @param res the result of the operation.  Expected values are
 * service- and state-specific, but the value shall be non-negative if
 * the operation succeeded, and negative if the operation failed.
 */
typedef void (*onoff_monitor_callback)(struct onoff_manager *mgr,
				       struct onoff_monitor *mon,
				       uint32_t state,
				       int res);

/**
 * @brief Registration state for notifications of onoff service
 * transitions.
 *
 * Any given onoff_monitor structure can be associated with at most
 * one onoff_manager instance.
 */
struct onoff_monitor {
	/** Links the client into the set of waiting service users.
	 *
	 * This must be zero-initialized.
	 */
	sys_snode_t node;

	/** Callback to be invoked on state change.
	 *
	 * This must not be null.
	 */
	onoff_monitor_callback callback;
};

/**
 * @brief Add a monitor of state changes for a manager.
 *
 * @param mgr the manager for which a state changes are to be monitored.
 *
 * @param mon a linkable node providing a non-null callback to be
 * invoked on state changes.
 *
 * @return non-negative on successful addition, or a negative error
 * code.
 */
int onoff_monitor_register(struct onoff_manager *mgr,
			   struct onoff_monitor *mon);

/**
 * @brief Remove a monitor of state changes from a manager.
 *
 * @param mgr the manager for which a state changes are to be monitored.
 *
 * @param mon a linkable node providing the callback to be invoked on
 * state changes.
 *
 * @return non-negative on successful removal, or a negative error
 * code.
 */
int onoff_monitor_unregister(struct onoff_manager *mgr,
			     struct onoff_monitor *mon);

/**
 * @brief State used when a driver uses the on-off service API for synchronous
 * operations.
 *
 * This is useful when a subsystem API uses the on-off API to support
 * asynchronous operations but the transitions required by a
 * particular driver are isr-ok and not sleep.  It serves as a
 * substitute for #onoff_manager, with locking and persisted state
 * updates supported by onoff_sync_lock() and onoff_sync_finalize().
 */
struct onoff_sync_service {
	/** Mutex protection for other fields. */
	struct k_spinlock lock;

	/** Negative is error, non-negative is reference count. */
	int32_t count;
};

/**
 * @brief Lock a synchronous onoff service and provide its state.
 *
 * @note If an error state is returned it is the caller's responsibility to
 * decide whether to preserve it (finalize with the same error state) or clear
 * the error (finalize with a non-error result).
 *
 * @param srv pointer to the synchronous service state.
 *
 * @param keyp pointer to where the lock key should be stored
 *
 * @return negative if the service is in an error state, otherwise the
 * number of active requests at the time the lock was taken.  The lock
 * is held on return regardless of whether a negative state is
 * returned.
 */
int onoff_sync_lock(struct onoff_sync_service *srv,
		    k_spinlock_key_t *keyp);

/**
 * @brief Process the completion of a transition in a synchronous
 * service and release lock.
 *
 * This function updates the service state on the @p res and @p on parameters
 * then releases the lock.  If @p cli is not null it finalizes the client
 * notification using @p res.
 *
 * If the service was in an error state when locked, and @p res is non-negative
 * when finalized, the count is reset to zero before completing finalization.
 *
 * @param srv pointer to the synchronous service state
 *
 * @param key the key returned by the preceding invocation of onoff_sync_lock().
 *
 * @param cli pointer to the onoff client through which completion
 * information is returned.  If a null pointer is passed only the
 * state of the service is updated.  For compatibility with the
 * behavior of callbacks used with the manager API @p cli must be null
 * when @p on is false (the manager does not support callbacks when
 * turning off devices).
 *
 * @param res the result of the transition.  A negative value places the service
 * into an error state.  A non-negative value increments or decrements the
 * reference count as specified by @p on.
 *
 * @param on Only when @p res is non-negative, the service reference count will
 * be incremented if@p on is @c true, and decremented if @p on is @c false.
 *
 * @return negative if the service is left or put into an error state, otherwise
 * the number of active requests at the time the lock was released.
 */
int onoff_sync_finalize(struct onoff_sync_service *srv,
			 k_spinlock_key_t key,
			 struct onoff_client *cli,
			 int res,
			 bool on);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_ONOFF_H_ */
