/*
 * Copyright (c) 2016-2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for counter and timer drivers
 */

#ifndef __COUNTER_H__
#define __COUNTER_H__

/**
 * @brief Counter Interface
 * @defgroup counter_interface Counter Interface
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <device.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct counter_alarm_callback;

/** @brief Alarm callback
 *
 * @param dev     Pointer to the device structure for the driver instance.
 * @param cb      Original structure owning this handler.
 * @param counter Counter value that triggered the callback.
 */
typedef void (*counter_alarm_callback_handler_t)(struct device *dev,
					 struct counter_alarm_callback *cb,
					 u32_t counter);

/** @brief Alarm callback structure.
 *
 * Used to set an alarm. *
 * Beware such structure should not be allocated on stack.
 */
struct counter_alarm_callback {
	/** Channel ID. Number of channels is driver dependent.*/
	u8_t channel_id;
	/** If true ticks are treated as absolute value, else it is relative to
	 * the counter reading performed during the call.
	 */
	bool abs; 
	/* In case of absolute flag is set, maximal value that can be set equals
	 * wrap value (counter_get_wrap). Otherwise
	 * counter_get_max_relative_alarm() returns maximal value that can be
	 * set.
	 */
	u32_t ticks;
	/** If non-zero alarm is repeated. */
	u32_t period;
	counter_alarm_callback_handler_t handler;
};

struct counter_wrap_callback;

typedef void (*counter_wrap_callback_handler_t)(struct device *dev,
					struct counter_wrap_callback *cb);

/** @brief Wrap callback structure.
 *
 * Used to set an overflow. *
 * Beware such structure should not be allocated on stack.
 */
struct counter_wrap_callback {
	u32_t ticks;
	counter_wrap_callback_handler_t handler;
};

typedef int (*counter_api_start)(struct device *dev);
typedef int (*counter_api_stop)(struct device *dev);
typedef u32_t (*counter_api_read)(struct device *dev);
typedef int (*counter_api_set_alarm)(struct device *dev,
				     struct counter_alarm_callback *cb);
typedef int (*counter_api_disable_alarm)(struct device *dev,
				     struct counter_alarm_callback *cb);

typedef int (*counter_api_set_wrap)(struct device *dev,
				    struct counter_wrap_callback *cb);

typedef u32_t (*counter_api_get_pending_int)(struct device *dev);
typedef u32_t (*counter_api_us_to_ticks)(struct device *dev, u64_t us);
typedef u64_t (*counter_api_ticks_to_us)(struct device *dev, u32_t ticks);
typedef u32_t (*counter_api_get_max_wrap)(struct device *dev);
typedef u32_t (*counter_api_get_wrap)(struct device *dev);
typedef u32_t (*counter_api_get_max_relative_alarm)(struct device *dev);
typedef bool (*counter_api_count_up)(struct device *dev);

struct counter_driver_api {
	counter_api_start start;
	counter_api_stop stop;
	counter_api_read read;
	counter_api_set_alarm set_alarm;
	counter_api_disable_alarm disable_alarm;
	counter_api_set_wrap set_wrap;
	counter_api_get_pending_int get_pending_int;
	counter_api_us_to_ticks us_to_ticks;
	counter_api_ticks_to_us ticks_to_us;
	counter_api_get_max_wrap get_max_wrap;
	counter_api_get_wrap get_wrap;
	counter_api_get_max_relative_alarm get_max_relative_alarm;
	counter_api_count_up count_up;
};

/**
 * @brief Start counter device in free running mode.
 *
 * Start the counter device. The counter initial value is set to zero. Counter
 * wraps at 0xffffffff.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int counter_start(struct device *dev);

static inline int _impl_counter_start(struct device *dev)
{
	const struct counter_driver_api *api = dev->driver_api;

	return api->start(dev);
}

/**
 * @brief Stop counter device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 If successful.
 * @retval -ENODEV if the device doesn't support stopping the
 *                        counter.
 */
__syscall int counter_stop(struct device *dev);

static inline int _impl_counter_stop(struct device *dev)
{
	const struct counter_driver_api *api = dev->driver_api;

	return api->stop(dev);
}

/**
 * @brief Read current counter value.
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return  32-bit value
 */
__syscall u32_t counter_read(struct device *dev);

static inline u32_t _impl_counter_read(struct device *dev)
{
	const struct counter_driver_api *api = dev->driver_api;

	return api->read(dev);
}

/**
 * @brief Set an alarm.
 *
 * In case of absolute request, maximal value that can be set is equal to
 * wrap value set by counter_set_wrap call or default, maximal one. in case of
 * relative request, Maximal value can be retrieved using
 * counter_get_max_relative_alarm.
 *
 * @param dev	Pointer to the device structure for the driver instance.
 * @param cb	Alarm callback.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP if the counter was not started yet.
 * @retval -ENODEV if the device doesn't support interrupt (e.g. free
 *                        running counters).
 * @retval -ENOTSUP if request is not supported.
 * @retval -EINVAL if alarm settings are invalid.
 * @retval Negative errno code if failure.
 */
static inline int counter_set_alarm(struct device *dev,
				    struct counter_alarm_callback *cb)
{
	const struct counter_driver_api *api = dev->driver_api;

	return api->set_alarm(dev, cb);
}

/**
 * @brief Disable an alarm.
 *
 * @param dev	Pointer to the device structure for the driver instance.
 * @param cb	Alarm callback.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP if the counter was not started yet.
 * @retval -ENOTSUP if request is not supported.
 * @retval Negative errno code if failure.
 */
static inline int counter_disable_alarm(struct device *dev,
					struct counter_alarm_callback *cb)
{
	const struct counter_driver_api *api = dev->driver_api;

	return api->disable_alarm(dev, cb);
}


/**
 * @brief Set alarm on counter wrap.
 *
 * Resets counter to 0 if counter counts up or to wrap value if counter counts
 * down. On wrap callback is periodically called and counter is reset.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param cb  Callback structure.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP if the counter was not started yet.
 * @retval -ENODEV if the device doesn't support interrupt (e.g. free
 *                        running counters).
 * @retval -EBUSY if any alarm is active.
 * @retval Negative errno code if failure.
 */
static inline int counter_set_wrap(struct device *dev,
				   struct counter_wrap_callback *cb)
{
	const struct counter_driver_api *api = dev->driver_api;

	return api->set_wrap(dev, cb);
}

/**
 * @brief Function to get pending interrupts
 *
 * The purpose of this function is to return the interrupt
 * status register for the device.
 * This is especially useful when waking up from
 * low power states to check the wake up source.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 1 if the counter any interrupt is pending.
 * @retval 0 if no counter interrupt is pending.
 */
__syscall int counter_get_pending_int(struct device *dev);

static inline int _impl_counter_get_pending_int(struct device *dev)
{
	const struct counter_driver_api *api = dev->driver_api;

	return api->get_pending_int(dev);
}

/**
 * @brief Function to convert microseconds to ticks.
 *
 * @param[in]  dev    Pointer to the device structure for the driver instance.
 * @param[in]  us     Microseconds.
 *
 * @return Converted ticks. Ticks will be saturated if exceed 32 bits.
 */
static inline u32_t counter_us_to_ticks(struct device *dev, u64_t us)
{
	const struct counter_driver_api *api = dev->driver_api;

	return api->us_to_ticks(dev, us);
}

/**
 * @brief Function to convert ticks to microseconds.
 *
 * @param[in]  dev    Pointer to the device structure for the driver instance.
 * @param[in]  ticks  Ticks.
 *
 * @return Converted microseconds.
 */
static inline u64_t counter_ticks_to_us(struct device *dev, u32_t ticks)
{
	const struct counter_driver_api *api = dev->driver_api;

	return api->ticks_to_us(dev, ticks);
}

/**
 * @brief Function to retrieve maximum wrap value that can be set.
 *
 * @param[in]  dev    Pointer to the device structure for the driver instance.
 *
 * @return Max wrap value.
 */
static inline u32_t counter_get_max_wrap(struct device *dev)
{
	const struct counter_driver_api *api = dev->driver_api;

	return api->get_max_wrap(dev);
}

/**
 * @brief Function to retrieve current wrap value.
 *
 * @param[in]  dev    Pointer to the device structure for the driver instance.
 *
 * @return Wrap value.
 */
static inline u32_t counter_get_wrap(struct device *dev)
{
	const struct counter_driver_api *api = dev->driver_api;

	return api->get_wrap(dev);
}

/**
 * @brief Function to retrieve maximum relative value that can be set by
 *        counter_set_alarm.
 *
 * @param[in]  dev    Pointer to the device structure for the driver instance.
 *
 * @return Max alarm value.
 */
static inline u32_t counter_get_max_relative_alarm(struct device *dev)
{
	const struct counter_driver_api *api = dev->driver_api;

	return api->get_max_relative_alarm(dev);
}

/**
 * @brief Function to check if counter is counting up.
 *
 * @param[in]  dev    Pointer to the device structure for the driver instance.
 *
 * @retval true if counter is counting up.
 * @retval false if counter is counting down.
 */
static inline bool counter_count_up(struct device *dev)
{
	const struct counter_driver_api *api = dev->driver_api;

	return api->count_up(dev);
}
#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/counter.h>

#endif /* __COUNTER_H__ */
