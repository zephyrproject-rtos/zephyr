/* clock_control.h - public clock controller driver API */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public Clock Control APIs
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_H_

/**
 * @brief Clock Control Interface
 * @defgroup clock_control_interface Clock Control Interface
 * @since 1.0
 * @version 1.0.0
 * @ingroup io_interfaces
 * @{
 */

#include <errno.h>
#include <stddef.h>

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/slist.h>

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

/**
 * clock_control_subsys_t is a type to identify a clock controller sub-system.
 * Such data pointed is opaque and relevant only to the clock controller
 * driver instance being used.
 */
typedef void *clock_control_subsys_t;

/**
 * clock_control_subsys_rate_t is a type to identify a clock
 * controller sub-system rate.  Such data pointed is opaque and
 * relevant only to set the clock controller rate of the driver
 * instance being used.
 */
typedef void *clock_control_subsys_rate_t;

/** @brief Callback called on clock started.
 *
 * @param dev		Device structure whose driver controls the clock.
 * @param subsys	Opaque data representing the clock.
 * @param user_data	User data.
 */
typedef void (*clock_control_cb_t)(const struct device *dev,
				   clock_control_subsys_t subsys,
				   void *user_data);

typedef int (*clock_control)(const struct device *dev,
			     clock_control_subsys_t sys);

typedef int (*clock_control_get)(const struct device *dev,
				 clock_control_subsys_t sys,
				 uint32_t *rate);

typedef int (*clock_control_async_on_fn)(const struct device *dev,
					 clock_control_subsys_t sys,
					 clock_control_cb_t cb,
					 void *user_data);

typedef enum clock_control_status (*clock_control_get_status_fn)(
						    const struct device *dev,
						    clock_control_subsys_t sys);

typedef int (*clock_control_set)(const struct device *dev,
				 clock_control_subsys_t sys,
				 clock_control_subsys_rate_t rate);

typedef int (*clock_control_configure_fn)(const struct device *dev,
					  clock_control_subsys_t sys,
					  void *data);

struct clock_control_driver_api {
	clock_control			on;
	clock_control			off;
	clock_control_async_on_fn	async_on;
	clock_control_get		get_rate;
	clock_control_get_status_fn	get_status;
	clock_control_set		set_rate;
	clock_control_configure_fn	configure;
};

/**
 * @brief Enable a clock controlled by the device
 *
 * On success, the clock is enabled and ready when this function
 * returns. This function may sleep, and thus can only be called from
 * thread context.
 *
 * Use @ref clock_control_async_on() for non-blocking operation.
 *
 * @param dev Device structure whose driver controls the clock.
 * @param sys Opaque data representing the clock.
 * @return 0 on success, negative errno on failure.
 */
static inline int clock_control_on(const struct device *dev,
				   clock_control_subsys_t sys)
{
	const struct clock_control_driver_api *api =
		(const struct clock_control_driver_api *)dev->api;

	return api->on(dev, sys);
}

/**
 * @brief Disable a clock controlled by the device
 *
 * This function is non-blocking and can be called from any context.
 * On success, the clock is disabled when this function returns.
 *
 * @param dev Device structure whose driver controls the clock
 * @param sys Opaque data representing the clock
 * @return 0 on success, negative errno on failure.
 */
static inline int clock_control_off(const struct device *dev,
				    clock_control_subsys_t sys)
{
	const struct clock_control_driver_api *api =
		(const struct clock_control_driver_api *)dev->api;

	return api->off(dev, sys);
}

/**
 * @brief Request clock to start with notification when clock has been started.
 *
 * Function is non-blocking and can be called from any context. User callback is
 * called when clock is started.
 *
 * @param dev	    Device.
 * @param sys	    A pointer to an opaque data representing the sub-system.
 * @param cb	    Callback.
 * @param user_data User context passed to the callback.
 *
 * @retval 0 if start is successfully initiated.
 * @retval -EALREADY if clock was already started and is starting or running.
 * @retval -ENOTSUP If the requested mode of operation is not supported.
 * @retval -ENOSYS if the interface is not implemented.
 * @retval other negative errno on vendor specific error.
 */
static inline int clock_control_async_on(const struct device *dev,
					 clock_control_subsys_t sys,
					 clock_control_cb_t cb,
					 void *user_data)
{
	const struct clock_control_driver_api *api =
		(const struct clock_control_driver_api *)dev->api;

	if (api->async_on == NULL) {
		return -ENOSYS;
	}

	return api->async_on(dev, sys, cb, user_data);
}

/**
 * @brief Get clock status.
 *
 * @param dev Device.
 * @param sys A pointer to an opaque data representing the sub-system.
 *
 * @return Status.
 */
static inline enum clock_control_status clock_control_get_status(const struct device *dev,
								 clock_control_subsys_t sys)
{
	const struct clock_control_driver_api *api =
		(const struct clock_control_driver_api *)dev->api;

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
 * @retval 0 on successful rate reading.
 * @retval -EAGAIN if rate cannot be read. Some drivers do not support returning the rate when the
 *         clock is off.
 * @retval -ENOTSUP if reading the clock rate is not supported for the given sub-system.
 * @retval -ENOSYS if the interface is not implemented.
 */
static inline int clock_control_get_rate(const struct device *dev,
					 clock_control_subsys_t sys,
					 uint32_t *rate)
{
	const struct clock_control_driver_api *api =
		(const struct clock_control_driver_api *)dev->api;

	if (api->get_rate == NULL) {
		return -ENOSYS;
	}

	return api->get_rate(dev, sys, rate);
}

/**
 * @brief Set the rate of the clock controlled by the device.
 *
 * On success, the new clock rate is set and ready when this function
 * returns. This function may sleep, and thus can only be called from
 * thread context.
 *
 * @param dev Device structure whose driver controls the clock.
 * @param sys Opaque data representing the clock.
 * @param rate Opaque data representing the clock rate to be used.
 *
 * @retval -EALREADY if clock was already in the given rate.
 * @retval -ENOTSUP If the requested mode of operation is not supported.
 * @retval -ENOSYS if the interface is not implemented.
 * @retval other negative errno on vendor specific error.
 */
static inline int clock_control_set_rate(const struct device *dev,
		clock_control_subsys_t sys,
		clock_control_subsys_rate_t rate)
{
	const struct clock_control_driver_api *api =
		(const struct clock_control_driver_api *)dev->api;

	if (api->set_rate == NULL) {
		return -ENOSYS;
	}

	return api->set_rate(dev, sys, rate);
}

/**
 * @brief Configure a source clock
 *
 * This function is non-blocking and can be called from any context.
 * On success, the selected clock is configured as per caller's request.
 *
 * It is caller's responsibility to ensure that subsequent calls to the API
 * provide the right information to allows clock_control driver to perform
 * the right action (such as using the right clock source on clock_control_get_rate
 * call).
 *
 * @p data is implementation specific and could be used to convey
 * supplementary information required for expected clock configuration.
 *
 * @param dev Device structure whose driver controls the clock
 * @param sys Opaque data representing the clock
 * @param data Opaque data providing additional input for clock configuration
 *
 * @retval 0 On success
 * @retval -ENOSYS If the device driver does not implement this call
 * @retval -errno Other negative errno on failure.
 */
static inline int clock_control_configure(const struct device *dev,
					  clock_control_subsys_t sys,
					  void *data)
{
	const struct clock_control_driver_api *api =
		(const struct clock_control_driver_api *)dev->api;

	if (api->configure == NULL) {
		return -ENOSYS;
	}

	return api->configure(dev, sys, data);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_H_ */
