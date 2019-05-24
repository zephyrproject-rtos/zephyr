/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for counter and timer drivers
 */

#ifndef ZEPHYR_INCLUDE_COUNTER_H_
#define ZEPHYR_INCLUDE_COUNTER_H_

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

/** @brief Alarm callback
 *
 * @param dev       Pointer to the device structure for the driver instance.
 * @param chan_id   Channel ID.
 * @param ticks     Counter value that triggered the callback.
 * @param user_data User data.
 */
typedef void (*counter_alarm_callback_t)(struct device *dev,
					 u8_t chan_id, u32_t ticks,
					 void *user_data);

/** @brief Alarm callback structure.
 *
 * @param callback Callback called on alarm (cannot be NULL).
 * @param ticks Ticks that triggers the alarm. In case of absolute flag is set,
 *		maximal value that can be set equals top value
 *		(@ref counter_get_top_value). Otherwise
 *		@ref counter_get_max_relative_alarm() returns maximal value that
 *		can be set. If counter is clock driven then ticks can be
 *		converted to microseconds (see @ref counter_ticks_to_us).
 *		Alternatively, counter implementation may count asynchronous
 *		events.
 * @param user_data User data returned in callback.
 * @param absolute Ticks relation to counter value. If true ticks are treated as
 *		absolute value, else it is relative to the counter reading
 *		performed during the call.
 */
struct counter_alarm_cfg {
	counter_alarm_callback_t callback;
	u32_t ticks;
	void *user_data;
	bool absolute;
};

/** @brief Callback called when counter turns around.
 *
 * @param dev       Pointer to the device structure for the driver instance.
 * @param user_data User data provided in @ref counter_set_top_value.
 */
typedef void (*counter_top_callback_t)(struct device *dev, void *user_data);

/** @brief Structure with generic counter features.
 *
 * @param max_top_value Maximal (default) top value on which counter is reset
 *			(cleared or reloaded).
 * @param freq		Frequency of the source clock if synchronous events are
 *			counted.
 * @param count_up	Flag indicating direction of the counter. If true
 *			counter is counting up else counting down.
 * @param channels	Number of channels that can be used for setting alarm,
 *			see @ref counter_set_alarm.
 */
struct counter_config_info {
	u32_t max_top_value;
	u32_t freq;
	bool count_up;
	u8_t channels;
};

typedef int (*counter_api_start)(struct device *dev);
typedef int (*counter_api_stop)(struct device *dev);
typedef u32_t (*counter_api_read)(struct device *dev);
typedef int (*counter_api_set_alarm)(struct device *dev, u8_t chan_id,
				const struct counter_alarm_cfg *alarm_cfg);
typedef int (*counter_api_cancel_alarm)(struct device *dev, u8_t chan_id);
typedef int (*counter_api_set_top_value)(struct device *dev, u32_t ticks,
				    counter_top_callback_t callback,
				    void *user_data);
typedef u32_t (*counter_api_get_pending_int)(struct device *dev);
typedef u32_t (*counter_api_get_top_value)(struct device *dev);
typedef u32_t (*counter_api_get_max_relative_alarm)(struct device *dev);
typedef void *(*counter_api_get_user_data)(struct device *dev);

struct counter_driver_api {
	counter_api_start start;
	counter_api_stop stop;
	counter_api_read read;
	counter_api_set_alarm set_alarm;
	counter_api_cancel_alarm cancel_alarm;
	counter_api_set_top_value set_top_value;
	counter_api_get_pending_int get_pending_int;
	counter_api_get_top_value get_top_value;
	counter_api_get_max_relative_alarm get_max_relative_alarm;
	counter_api_get_user_data get_user_data;
};


/**
 * @brief Function to check if counter is counting up.
 *
 * @param[in]  dev    Pointer to the device structure for the driver instance.
 *
 * @retval true if counter is counting up.
 * @retval false if counter is counting down.
 */
static inline bool counter_is_counting_up(const struct device *dev)
{
	const struct counter_config_info *config =
			(struct counter_config_info *)dev->config->config_info;

	return config->count_up;
}

/**
 * @brief Function to get number of alarm channels.
 *
 * @param[in]  dev    Pointer to the device structure for the driver instance.
 *
 * @return Number of alarm channels.
 */
static inline u8_t counter_get_num_of_channels(const struct device *dev)
{
	const struct counter_config_info *config =
			(struct counter_config_info *)dev->config->config_info;

	return config->channels;
}

/**
 * @brief Function to get counter frequency.
 *
 * @param[in]  dev    Pointer to the device structure for the driver instance.
 *
 * @return Frequency of the counter in Hz, or zero if the counter does
 * not have a fixed frequency.
 */
static inline u32_t counter_get_frequency(const struct device *dev)
{
	const struct counter_config_info *config =
			(struct counter_config_info *)dev->config->config_info;

	return config->freq;
}

/**
 * @brief Function to convert microseconds to ticks.
 *
 * @param[in]  dev    Pointer to the device structure for the driver instance.
 * @param[in]  us     Microseconds.
 *
 * @return Converted ticks. Ticks will be saturated if exceed 32 bits.
 */
static inline u32_t counter_us_to_ticks(const struct device *dev, u64_t us)
{
	const struct counter_config_info *config =
			(struct counter_config_info *)dev->config->config_info;
	u64_t ticks = (us * config->freq) / USEC_PER_SEC;

	return (ticks > (u64_t)UINT32_MAX) ? UINT32_MAX : ticks;
}

/**
 * @brief Function to convert ticks to microseconds.
 *
 * @param[in]  dev    Pointer to the device structure for the driver instance.
 * @param[in]  ticks  Ticks.
 *
 * @return Converted microseconds.
 */
static inline u64_t counter_ticks_to_us(const struct device *dev, u32_t ticks)
{
	const struct counter_config_info *config =
			(struct counter_config_info *)dev->config->config_info;

	return ((u64_t)ticks * USEC_PER_SEC) / config->freq;
}

/**
 * @brief Function to retrieve maximum top value that can be set.
 *
 * @param[in]  dev    Pointer to the device structure for the driver instance.
 *
 * @return Max top value.
 */
static inline u32_t counter_get_max_top_value(const struct device *dev)
{
	const struct counter_config_info *config =
			(struct counter_config_info *)dev->config->config_info;

	return config->max_top_value;
}

/**
 * @brief Start counter device in free running mode.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int counter_start(struct device *dev);

static inline int z_impl_counter_start(struct device *dev)
{
	const struct counter_driver_api *api =
				(struct counter_driver_api *)dev->driver_api;

	return api->start(dev);
}

/**
 * @brief Stop counter device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP if the device doesn't support stopping the
 *                        counter.
 */
__syscall int counter_stop(struct device *dev);

static inline int z_impl_counter_stop(struct device *dev)
{
	const struct counter_driver_api *api =
				(struct counter_driver_api *)dev->driver_api;

	return api->stop(dev);
}

/**
 * @brief Read current counter value.
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return  32-bit value
 */
__syscall u32_t counter_read(struct device *dev);

static inline u32_t z_impl_counter_read(struct device *dev)
{
	const struct counter_driver_api *api =
				(struct counter_driver_api *)dev->driver_api;

	return api->read(dev);
}

/**
 * @brief Set a single shot alarm on a channel.
 *
 * After expiration alarm can be set again, disabling is not needed. When alarm
 * expiration handler is called, channel is considered available and can be
 * set again in that context.
 *
 * @note API is not thread safe.
 *
 * @param dev		Pointer to the device structure for the driver instance.
 * @param chan_id	Channel ID.
 * @param alarm_cfg	Alarm configuration.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP if request is not supported (device does not support
 *		    interrupts or requested channel).
 * @retval -EINVAL if alarm settings are invalid.
 */
static inline int counter_set_channel_alarm(struct device *dev, u8_t chan_id,
				      const struct counter_alarm_cfg *alarm_cfg)
{
	const struct counter_driver_api *api =
				(struct counter_driver_api *)dev->driver_api;

	if (chan_id >= counter_get_num_of_channels(dev)) {
		return -ENOTSUP;
	}

	return api->set_alarm(dev, chan_id, alarm_cfg);
}

/**
 * @brief Cancel an alarm on a channel.
 *
 * @note API is not thread safe.
 *
 * @param dev		Pointer to the device structure for the driver instance.
 * @param chan_id	Channel ID.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP if request is not supported or the counter was not started
 *		    yet.
 */
static inline int counter_cancel_channel_alarm(struct device *dev,
						u8_t chan_id)
{
	const struct counter_driver_api *api =
				(struct counter_driver_api *)dev->driver_api;

	if (chan_id >= counter_get_num_of_channels(dev)) {
		return -ENOTSUP;
	}

	return api->cancel_alarm(dev, chan_id);
}

/**
 * @brief Set counter top value.
 *
 * Function sets top value and resets the counter to 0 or top value
 * depending on counter direction. On turnaround, counter is reset and optional
 * callback is periodically called. Top value can only be changed when there
 * is no active channel alarm.
 *
 * @param dev		Pointer to the device structure for the driver instance.
 * @param ticks		Top value.
 * @param callback	Callback function. Can be NULL.
 * @param user_data	User data passed to callback function. Not valid if
 *			callback is NULL.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP if request is not supported (e.g. top value cannot be
 *		    changed).
 * @retval -EBUSY if any alarm is active.
 */
static inline int counter_set_top_value(struct device *dev, u32_t ticks,
					counter_top_callback_t callback,
					void *user_data)
{
	const struct counter_driver_api *api =
				(struct counter_driver_api *)dev->driver_api;

	if (ticks > counter_get_max_top_value(dev)) {
		return -EINVAL;
	}

	return api->set_top_value(dev, ticks, callback, user_data);
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
 * @retval 1 if any counter interrupt is pending.
 * @retval 0 if no counter interrupt is pending.
 */
__syscall int counter_get_pending_int(struct device *dev);

static inline int z_impl_counter_get_pending_int(struct device *dev)
{
	const struct counter_driver_api *api =
				(struct counter_driver_api *)dev->driver_api;

	return api->get_pending_int(dev);
}

/**
 * @brief Function to retrieve current top value.
 *
 * @param[in]  dev    Pointer to the device structure for the driver instance.
 *
 * @return Top value.
 */
__syscall u32_t counter_get_top_value(struct device *dev);

static inline u32_t z_impl_counter_get_top_value(struct device *dev)
{
	const struct counter_driver_api *api =
				(struct counter_driver_api *)dev->driver_api;

	return api->get_top_value(dev);
}

/**
 * @brief Function to retrieve maximum relative value that can be set by
 *        counter_set_alarm.
 *
 * @param[in]  dev    Pointer to the device structure for the driver instance.
 *
 * @return Max alarm value.
 */
__syscall u32_t counter_get_max_relative_alarm(struct device *dev);

static inline u32_t z_impl_counter_get_max_relative_alarm(struct device *dev)
{
	const struct counter_driver_api *api =
				(struct counter_driver_api *)dev->driver_api;

	return api->get_max_relative_alarm(dev);
}

/* Deprecated counter callback. */
typedef void (*counter_callback_t)(struct device *dev, void *user_data);

/**
 * @brief Deprecated function.
 */
__deprecated static inline int counter_set_alarm(struct device *dev,
						 counter_callback_t callback,
						 u32_t count, void *user_data)
{
	return counter_set_top_value(dev, count, callback, user_data);
}

/**
 * @brief Get user data set for top alarm.
 *
 * @note Function intended to be used only by deprecated RTC driver API to
 * provide backward compatibility.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return User data.
 */
__deprecated static inline void *counter_get_user_data(struct device *dev)
{
	const struct counter_driver_api *api =
				(struct counter_driver_api *)dev->driver_api;

	if (api->get_user_data) {
		return api->get_user_data(dev);
	} else {
		return NULL;
	}
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/counter.h>

#endif /* ZEPHYR_INCLUDE_COUNTER_H_ */
