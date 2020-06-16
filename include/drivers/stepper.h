/**
 * @file
 *
 * @brief Public APIs for the stepper controller drivers.
 */

/*
 * Copyright (c) 2020 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_STEPPER_H_
#define ZEPHYR_DRIVERS_STEPPER_H_

#include <zephyr/types.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @cond INTERNAL_HIDDEN
 *
 * These are for internal use only, so skip these in
 * public documentation.
 */

struct stepper_driver_api {
	int (*set_enabled)(struct device *dev, uint32_t channel, bool enabled);
	int (*set_speed)(struct device *dev, uint32_t channel, int32_t speed);
	int (*move)(struct device *dev, uint32_t channel, int32_t steps);
#ifdef CONFIG_STEPPER_ASYNC
	int (*move_async)(struct device *dev, uint32_t channel, int32_t steps,
			  struct k_poll_signal *async);
#endif
	int (*stop)(struct device *dev, uint32_t channel);
};

/**
 * @endcond
 */

/**
 * @brief Enable or disable a stepper channel.
 * @param dev	  Stepper controller device.
 * @param channel Stepper channel.
 * @param enabled Enabled flag.
 */
__syscall int stepper_set_enabled(struct device *dev, uint32_t channel,
				  bool enabled);

static inline int z_impl_stepper_set_enabled(struct device *dev,
					     uint32_t channel, bool enabled)
{
	const struct stepper_driver_api *api =
		(const struct stepper_driver_api *)dev->driver_api;
	return api->set_enabled(dev, channel, enabled);
}

/**
 * @brief Configure stepper speed.
 *
 * @param dev	  Stepper controller device.
 * @param channel Stepper channel.
 * @param speed   Speed (steps/s).
 */
__syscall void stepper_set_speed(struct device *dev, uint32_t channel,
				 int32_t speed);

static inline void z_impl_stepper_set_speed(struct device *dev,
					    uint32_t channel, int32_t speed)
{
	const struct stepper_driver_api *api =
		(const struct stepper_driver_api *)dev->driver_api;
	api->set_speed(dev, channel, speed);
}

/**
 * @brief Move stepper N steps.
 *
 * @param dev	  Stepper controller device.
 * @param channel Stepper channel.
 * @param steps   Number of steps.
 */
__syscall int stepper_move(struct device *dev, uint32_t channel, int32_t steps);

static inline int z_impl_stepper_move(struct device *dev, uint32_t channel,
				      int32_t steps)
{
	const struct stepper_driver_api *api =
		(const struct stepper_driver_api *)dev->driver_api;
	return api->move(dev, channel, steps);
}

#ifdef CONFIG_STEPPER_ASYNC
/**
 * @brief Move stepper N steps (asynchronous API).
 *
 * @param dev	  Stepper controller device.
 * @param channel Stepper channel.
 * @param steps   Number of steps.
 * @param async   Poll signal.
 */
__syscall int stepper_move_async(struct device *dev, uint32_t channel,
				 int32_t steps, struct k_poll_signal *async);

static inline int z_impl_stepper_move_async(struct device *dev,
					    uint32_t channel, int32_t steps,
					    struct k_poll_signal *async)
{
	const struct stepper_driver_api *api =
		(const struct stepper_driver_api *)dev->driver_api;
	return api->move_async(dev, channel, steps, async);
}
#endif

/**
 * @brief Stop stepper movement.
 *
 * @param dev	  Stepper controller device.
 * @param channel Channel.
 */
__syscall int stepper_stop(struct device *dev, uint32_t channel);

static inline int z_impl_stepper_stop(struct device *dev, uint32_t channel)
{
	const struct stepper_driver_api *api =
		(const struct stepper_driver_api *)dev->driver_api;
	return api->stop(dev, channel);
}

#ifdef __cplusplus
}
#endif

#include <syscalls/stepper.h>

#endif /* ZEPHYR_DRIVERS_STEPPER_H_ */
