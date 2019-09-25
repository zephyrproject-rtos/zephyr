/* clock_control.h - public clock controller driver API */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_H_

#include <zephyr/types.h>
#include <stddef.h>
#include <device.h>
#include <sys/__assert.h>
#include <sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Clock control API */

/* Used to select all subsystem of a clock controller */
#define CLOCK_CONTROL_SUBSYS_ALL	NULL

/**
 * @brief Current clock status.
 */
enum clock_control_status {
	CLOCK_CONTROL_STATUS_STARTING,
	CLOCK_CONTROL_STATUS_OFF,
	CLOCK_CONTROL_STATUS_ON,
	CLOCK_CONTROL_STATUS_UNKNOWN
};

typedef void (*clock_control_cb_t)(struct device *dev, void *user_data);

/**
 * @cond INTERNAL_HIDDEN
 */
#define Z_CLOCK_CONTROL_ASYNC_DATA_INITIALIZER(_cb, _user_data) \
	{ \
		.cb = cb, \
		.user_data = _user_data \
	}
/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * Define and initialize clock_control async data.
 *
 * @param name		Name of the data.
 * @param cb		Callback.
 * @param user_data	User data
 */
#define CLOCK_CONTROL_ASYNC_DATA_DEFINE(name, cb, user_data) \
	struct clock_control_async_data name = \
		Z_CLOCK_CONTROL_ASYNC_DATA_INITIALIZER(cb, user_data)

/**
 * @brief Clock control data used for asynchronous clock enabling.
 *
 * @param node		Used internally for linking asynchronous requests.
 * @param cb		Callback called when clock is started.
 * @param user_data	User data passed as an argument in the callback.
 */
struct clock_control_async_data {
	sys_snode_t node;
	clock_control_cb_t cb;
	void *user_data;
};

/**
 * clock_control_subsys_t is a type to identify a clock controller sub-system.
 * Such data pointed is opaque and relevant only to the clock controller
 * driver instance being used.
 */
typedef void *clock_control_subsys_t;

typedef int (*clock_control)(struct device *dev, clock_control_subsys_t sys);

typedef int (*clock_control_get)(struct device *dev,
				 clock_control_subsys_t sys,
				 u32_t *rate);

typedef int (*clock_control_async_on_fn)(struct device *dev,
				   clock_control_subsys_t sys,
				   struct clock_control_async_data *data);

typedef enum clock_control_status (*clock_control_get_status_fn)(
						struct device *dev,
						clock_control_subsys_t sys);

struct clock_control_driver_api {
	clock_control			on;
	clock_control			off;
	clock_control_async_on_fn	async_on;
	clock_control_get		get_rate;
	clock_control_get_status_fn	get_status;
};

/**
 * @brief Enable the clock of a sub-system controlled by the device
 * @param dev Pointer to the device structure for the clock controller driver
 * 	instance
 * @param sys A pointer to an opaque data representing the sub-system
 */
static inline int clock_control_on(struct device *dev,
				   clock_control_subsys_t sys)
{
	const struct clock_control_driver_api *api =
		(const struct clock_control_driver_api *)dev->driver_api;

	return api->on(dev, sys);
}

/**
 * @brief Disable the clock of a sub-system controlled by the device
 * @param dev Pointer to the device structure for the clock controller driver
 * 	instance
 * @param sys A pointer to an opaque data representing the sub-system
 */
static inline int clock_control_off(struct device *dev,
				    clock_control_subsys_t sys)
{
	const struct clock_control_driver_api *api =
		(const struct clock_control_driver_api *)dev->driver_api;

	return api->off(dev, sys);
}

/**
 * @brief Request clock to start with notification when clock has been started.
 *
 * User can request delayed start by providing exact information when clock
 * should be ready. Driver ensures that clock is ready before requested time.
 * It is the driver responsibility to take into account clock startup time.
 * When clock is already running user callback will be called from the context
 * of the function call else it is called from other context (e.g. clock
 * interrupt).
 *
 * @param dev 	   Device.
 * @param sys	   A pointer to an opaque data representing the sub-system.
 * @param data	   Data structure containing a callback that is called when
 *		   action is performed. Structure content must be valid until
 *		   clock is started and user callback is called. Can be NULL.
 *
 * @retval 0 if clock is started or already running.
 * @retval -EBUSY if same request already scheduled and not yet completed.
 * @retval -ENOTSUP if not supported.
 */
static inline int clock_control_async_on(struct device *dev,
					 clock_control_subsys_t sys,
					 struct clock_control_async_data *data)
{
	const struct clock_control_driver_api *api =
		(const struct clock_control_driver_api *)dev->driver_api;

	if (!api->async_on) {
		return -ENOTSUP;
	}

	return api->async_on(dev, sys, data);
}

/**
 * @brief Get clock status.
 *
 * @param dev Device.
 * @param sys A pointer to an opaque data representing the sub-system.
 *
 * @return Status.
 */
static inline enum clock_control_status clock_control_get_status(
						struct device *dev,
						clock_control_subsys_t sys)
{
	const struct clock_control_driver_api *api =
		(const struct clock_control_driver_api *)dev->driver_api;

	if (!api->get_status) {
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}

	return api->get_status(dev, sys);
}

/**
 * @brief Obtain the clock rate of given sub-system
 * @param dev Pointer to the device structure for the clock controller driver
 *        instance
 * @param sys A pointer to an opaque data representing the sub-system
 * @param[out] rate Subsystem clock rate
 */
static inline int clock_control_get_rate(struct device *dev,
					 clock_control_subsys_t sys,
					 u32_t *rate)
{
	const struct clock_control_driver_api *api =
		(const struct clock_control_driver_api *)dev->driver_api;

	__ASSERT(api->get_rate != NULL, "%s not implemented for device %s",
		__func__, dev->config->name);

	return api->get_rate(dev, sys, rate);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_H_ */
