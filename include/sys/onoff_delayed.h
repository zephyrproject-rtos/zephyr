/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_ONOFF_DELAYED_H_
#define ZEPHYR_INCLUDE_SYS_ONOFF_DELAYED_H_

#include <sys/onoff.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup resource_mgmt_onoff_delayed_apis Delayed On-Off Service APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief State associated with a delayed on-off manager.
 */
struct onoff_delayed_manager {
	/* On-off manager. */
	struct onoff_manager mgr;

	/* Startup time + maximum interrupt latency given in system ticks.
	 * Service start must be scheduled earlier to be ready by the scheduled
	 * time.
	 */
	uint32_t startup_time;
	uint32_t stop_time;

	/* Timeout instance used for scheduling. */
	struct _timeout timer;

	/* List of scheduled clients. */
	sys_slist_t future_clients;

	struct onoff_monitor monitor;
};

/**
 * @brief State associated with a delayed client of an on-off service.
 */
struct onoff_delayed_client {
	/** @brief On-off client.
	 *
	 * Notify structure within the client must be initialize before use.
	 */
	struct onoff_client cli;

	/* Internally used to store absolute timeout when start must be
	 * initiated to be ready at expected time.
	 */
	k_timeout_t timeout;
};

/**
 * @brief Initialize a delayed on-off manager to off state.
 *
 * This function must be invoked exactly once per service instance, by
 * the infrastructure that provides the service, and before any other
 * on-off service API is invoked on the service.
 *
 * This function should never be invoked by clients of an on-off
 * service.
 *
 * @param mgr the manager to be initialized.
 *
 * @param transitions pointer to a structure providing transition
 * functions.  The referenced object must persist as long as the
 * manager can be referenced.
 *
 * @param startup_time Service startup time + maximum interrupt latency given in
 * system ticks. Start request is scheduled %p startup_time earlier to be
 * ready no later than at requested time.
 *
 * @param stop_time Service stop time in system ticks. Stop time is used to
 * ensure that service is not stopped if there is pending request that will
 * restart the service. It may exceed actual service stop procedure time if
 * there is additional cost of restart (e.g. higher current consumption during
 * initialization).
 *
 * @retval 0 on success.
 * @retval -EINVAL if parameters are invalid.
 */
int onoff_delayed_manager_init(struct onoff_delayed_manager *mgr,
			       const struct onoff_transitions *transitions,
			       uint32_t startup_time, uint32_t stop_time);

/**
 * @brief Request on-off service to be in on state at specified time.
 *
 * Note that K_NO_WAIT or absolute timeout which is in the past results in
 * immediate request and it is equivalent of calling on-off manager directly.
 *
 * @param mgr the manager that will be used.
 *
 * @param cli a non-null pointer to client state providing
 * instructions on synchronous expectations and how to notify the
 * client when the request completes.  Behavior is undefined if client
 * passes a pointer object associated with an incomplete service
 * operation.
 *
 * @param delay relative delay or absolute time at which service must be in on
 * state.
 *
 * @retval 0 if successfully scheduled.
 * @retval non-negative if delay is K_NO_WAIT or absolute time in the past and
 * on-off manager was requested directly. See @ref onoff_request for details.
 * @retval -EINVAL if the parameters are invalid.
 * @retval negative on error returned by @ref onoff_request. See
 * @ref onoff_request for details.
 */
int onoff_delayed_request(struct onoff_delayed_manager *mgr,
			  struct onoff_delayed_client *cli,
			  k_timeout_t delay);

/**
 * @brief Release a reserved use of an on-off service.
 *
 * See @ref onoff_release for more details.
 *
 * @param mgr the manager for which a request was successful.
 *
 * @retval non-negative the observed state (ONOFF_STATE_ON) of the
 * machine at the time of the release, if the release succeeds.
 * @retval negative on error returned by @ref onoff_release. See
 * @ref onoff_release for details.
 */
static inline int onoff_delayed_release(struct onoff_delayed_manager *mgr)
{
	return onoff_release(&mgr->mgr);
}

/**
 * @brief Attempt to cancel an in-progress client operation.
 *
 * On successful cancellation ownership of @c *cli reverts to the client.
 * See @ref onoff_cancel for details.
 *
 * @param mgr the manager for which an operation is to be cancelled.
 *
 * @param cli a pointer to the same client state that was provided
 * when the operation to be cancelled was issued.
 *
 * @retval 0 if successfully cancelled before request delay has passed.
 * @retval non-negative the observed state of the machine at the time
 * of the cancellation, if the cancellation succeeds.
 * @retval negative on error returned by @ref onoff_cancel. See
 * @ref onoff_cancel for details).
 */
int onoff_delayed_cancel(struct onoff_delayed_manager *mgr,
			 struct onoff_delayed_client *cli);

/**
 * @brief Helper function to safely cancel a request.
 *
 * See @ref onoff_cancel_or_release for details.
 *
 * @param mgr the manager for which an operation is to be cancelled.
 *
 * @param cli a pointer to the same client state that was provided
 * when onoff_request() was invoked.  Behavior is undefined if this is
 * a pointer to client data associated with an onoff_reset() request.
 *
 * @retval non-negative on successful canceling or releasing.
 * @retval negative on error return. See @ref onoff_cancel and
 * @ref onoff_release.
 */
static inline int onoff_delayed_cancel_or_release(
					  struct onoff_delayed_manager *mgr,
					  struct onoff_delayed_client *cli)
{
	int rv = onoff_delayed_cancel(mgr, cli);

	if (rv == -EALREADY) {
		rv = onoff_delayed_release(mgr);
	}
	return rv;
}

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_ONOFF_DELAYED_H_ */
