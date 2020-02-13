/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_QUEUED_OPERATION_H_
#define ZEPHYR_INCLUDE_SYS_QUEUED_OPERATION_H_

#include <kernel.h>
#include <zephyr/types.h>
#include <sys/notify.h>
#include <sys/onoff.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration */
struct queued_operation;
struct queued_operation_manager;

/**
 * @defgroup resource_mgmt_queued_operation_apis Queued Operation APIs
 * @ingroup kernel_apis
 * @{
 */

/** @internal */
#define QUEUED_OPERATION_PRIORITY_POS SYS_NOTIFY_EXTENSION_POS
/** @internal */
#define QUEUED_OPERATION_PRIORITY_BITS 8U
/** @internal */
#define QUEUED_OPERATION_PRIORITY_MASK BIT_MASK(QUEUED_OPERATION_PRIORITY_BITS)

/**
 * @brief Special priority value to indicate operation should be
 * placed last in current queue.
 *
 * This is like providing the lowest priority but uses a constant-time
 * insertion and is FIFO.
 */
#define QUEUED_OPERATION_PRIORITY_APPEND \
	((int)QUEUED_OPERATION_PRIORITY_MASK + 1)

/**
 * @brief Special priority value to indicate operation should be
 * placed first in the current queue.
 *
 * This is like providing the highest priority but uses a
 * constant-time insertion and is LIFO.
 */
#define QUEUED_OPERATION_PRIORITY_PREPEND \
	((int)QUEUED_OPERATION_PRIORITY_MASK + 2)

/**
 * @brief Identify the region of sys_notify flags available for
 * containing services.
 *
 * Bits of the flags field of the sys_notify structure contained
 * within the queued_operation structure at and above this position
 * may be used by extensions to the sys_notify structure.
 *
 * These bits are intended for use by containing service
 * implementations to record client-specific information.  The bits
 * are cleared by sys_notify_validate().  Use of these does not
 * imply that the flags field becomes public API.
 */
#define QUEUED_OPERATION_EXTENSION_POS \
	(QUEUED_OPERATION_PRIORITY_POS + QUEUED_OPERATION_PRIORITY_BITS)

/**
 * @brief Base object providing state for an operation.
 *
 * Instances of this should be members of a service-specific structure
 * that provides the operation parameters.
 */
struct queued_operation {
	/** @internal
	 *
	 * Links the operation into the operation queue.
	 */
	sys_snode_t node;

	/**
	 * @brief Notification configuration.
	 *
	 * This must be initialized using sys_notify_init_callback()
	 * or its sibling functions before an operation can be passed
	 * to queued_operation_submit().
	 *
	 * The queued operation manager provides specific error codes
	 * for failures identified at the manager level:
	 * * -ENODEV indicates a failure in an onoff service.
	 */
	struct sys_notify notify;
};

/**
 * @ brief Table of functions used by a queued operation manager.
 */
struct queued_operation_functions {
	/**
	 * @brief Function used to verify an operation is well-defined.
	 *
	 * When provided this function is invoked by
	 * queued_operation_submit() to verify that the operation
	 * definition meets the expectations of the service.  The
	 * operation is acceptable only if a non-negative value is
	 * returned.
	 *
	 * If not provided queued_operation_submit() will assume
	 * service-specific expectations are trivially satisfied, and
	 * will reject the operation only if sys_notify_validate()
	 * fails.  Because that validation is limited services should
	 * at a minimum verify that the extension bits have the
	 * expected value (zero, when none are being used).
	 *
	 * @note The validate function must be isr-ok.
	 *
	 * @param mgr the service that supports queued operations.
	 *
	 * @param op the operation being considered for suitability.
	 *
	 * @return the value to be returned from queued_operation_submit().
	 */
	int (*validate)(struct queued_operation_manager *mgr,
			struct queued_operation *op);

	/**
	 * @brief Function to transform a generic notification
	 * callback to its service-specific form.
	 *
	 * The implementation should cast cb to the proper signature
	 * for the service, and invoke the cast pointer with the
	 * appropriate arguments.
	 *
	 * @note The callback function must be isr-ok.
	 *
	 * @param mgr the service that supports queued operations.
	 *
	 * @param op the operation that has been completed.
	 *
	 * @param cb the generic callback to invoke.
	 */
	void (*callback)(struct queued_operation_manager *mgr,
			 struct queued_operation *op,
			 sys_notify_generic_callback cb);

	/**
	 * @brief Function used to inform the manager of a new operation.
	 *
	 * This function can be called as a side effect of
	 * queued_operation_submit() or queued_operation_finalize() to
	 * tell the service that a new operation needs to be
	 * processed.
	 *
	 * Be aware that if processing is entirely
	 * synchronous--meaning queued_operation_finalize() can be
	 * invoked during process()--then the process() function will
	 * be invoked recursively, possibly with another operation.
	 * This can cause unbounded stack growth, and requires that
	 * process() be re-entrant.  Generally the process() function
	 * should itself be async, with finalization done after
	 * process() returns.
	 *
	 * @note The process function must be isr-ok.
	 *
	 * @param mgr the service that supports queued operations.
	 *
	 * @param op the operation that should be initiated.  A null
	 * pointer is passed if there are no pending operations.
	 */
	void (*process)(struct queued_operation_manager *mgr,
			struct queued_operation *op);
};

/**
 * @brief State associated with a manager instance.
 */
struct queued_operation_manager {
	/* Links the operation into the operation queue. */
	sys_slist_t operations;

	/* Pointer to the functions that support the manager. */
	const struct queued_operation_functions *vtable;

	/* Pointer to an on-off service supporting this service.  NULL
	 * if service is always available.
	 */
	struct onoff_manager *onoff;

	/* The state of on-off service requests. */
	struct onoff_client onoff_client;

	/* Lock controlling access to other fields. */
	struct k_spinlock lock;

	/* The operation that is being processed. */
	struct queued_operation *current;

	/* Information about the internal state of the manager. */
	uint32_t volatile state;
};

#define QUEUED_OPERATION_MANAGER_INITIALIZER(_vtable, _onoff) {	\
		.vtable = _vtable,				\
		.onoff = _onoff,				\
}

/**
 * @brief Submit an operation to be processed when the service is
 * available.
 *
 * The service process function will be invoked during this call if
 * the service is available.
 *
 * @param mgr a generic pointer to the service instance
 *
 * @param op a generic pointer to an operation to be performed.  The
 * notify field in the provided operation must have been initialized
 * before being submitted, even if the operation description is being
 * re-used.  This may be done directly with sys_notify API or by
 * wrapping it in a service-specific operation init function.
 *
 * @param priority the priority of the operation relative to other
 * operations.  Numerically lower values are higher priority.  Values
 * outside the range of a signed 8-bit integer will be rejected,
 * except for named priorities like QUEUED_OPERATION_PRIORITY_APPEND.
 *
 * @retval -ENOTSUP if callback notification is requested and the
 * service does not provide a callback translation.  This may also be
 * returned due to service-specific validation.
 *
 * @retval -EINVAL if the passed priority is out of the range of
 * supported priorities.  This may also be returned due to
 * service-specific validation.
 *
 * @return A negative value if the operation was rejected by service
 * validation or due to other configuration errors.  A non-negative
 * value indicates the operation has been accepted for processing and
 * completion notification will be provided.
 */
int queued_operation_submit(struct queued_operation_manager *mgr,
			    struct queued_operation *op,
			    int priority);

int queued_operation_sync_submit(struct queued_operation_manager *qop_mgr,
				 struct queued_operation *qop, int priority);
/**
 * @brief Helper to extract the result from a queued operation.
 *
 * This forwards to sys_notify_fetch_result().
 */
static inline int queued_operation_fetch_result(const struct queued_operation *op,
						int *result)
{
	return sys_notify_fetch_result(&op->notify, result);
}

/**
 * @brief Attempt to cancel a queued operation.
 *
 * Successful cancellation issues a completion notification with
 * result -ECANCELED for the submitted operation before this function
 * returns.
 *
 * @retval 0 if successfully cancelled.
 * @retval -EINPROGRESS if op is currently being executed, so cannot
 * be cancelled.
 * @retval -EINVAL if op is neither being executed nor in the queue of
 * pending operations
 */
int queued_operation_cancel(struct queued_operation_manager *mgr,
			    struct queued_operation *op);

/**
 * @brief Send the completion notification for a queued operation.
 *
 * This function must be invoked by services that support queued
 * operations when the operation provided to them through the process
 * function have been completed.  It is not intended to be invoked by
 * users of a service.
 *
 * @param mgr a generic pointer to the service instance
 * @param res the result of the operation, as with
 * sys_notify_finalize().
 */
void queued_operation_finalize(struct queued_operation_manager *mgr,
			       int res);

/**
 * @brief Test whether the queued operation service is in an error
 * state.
 *
 * This function can be used to determine whether the service has
 * recorded an error.  These errors occur only when the manager's
 * attempt to turn an underlying service on or off has itself produced
 * an error, leaving the service in an undefined state.
 *
 * queued_operation_reset() may be invoked to attempt to clear an
 * error.
 *
 * This is an convenience function that due to race conditions may
 * incorrectly represent the state of the service at the moment
 * control returns to the caller.
 *
 * @return true if and only if the service has an error.
 */
bool queued_operation_has_error(struct queued_operation_manager *mgr);

/**
 * @brief Attempt to reset the manager when it is in an error state.
 *
 * A queued operation service can only be reset when it is in an error
 * state as indicated by queued_operation_has_error().
 *
 * @note If a non-null @p cli is provided in a path where the manager
 * successfully initiates a reset the return value from this function
 * will reflect the success or failure of registering the independent
 * notification through @p cli.
 *
 * @param mgr a generic pointer to the service instance.
 *
 * @param cli an optional client structure that will be passed to
 * onoff_reset() on behalf of the service client to provide
 * independent notification when a succesfully initiated reset has
 * completed.
 *
 * @retval nonnegative on successful initiation of a reset (when
 *         @p cli is null)
 * @retval -EALREADY if the service has not recorded an error
 * @retval other success and failure values from onoff_reset()
 */
int queued_operation_reset(struct queued_operation_manager *mgr,
			   struct onoff_client *cli);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_ASYNCNOTIFY_H_ */
