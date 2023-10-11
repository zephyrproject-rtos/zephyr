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

#ifndef ZEPHYR_INCLUDE_DRIVERS_COUNTER_H_
#define ZEPHYR_INCLUDE_DRIVERS_COUNTER_H_

/**
 * @brief Counter Interface
 * @defgroup counter_interface Counter Interface
 * @ingroup io_interfaces
 * @{
 */

#include <errno.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/device.h>
#include <zephyr/sys_clock.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @anchor COUNTER_FLAGS
 * @name Counter device capabilities
 * @{
 */

/**
 * @brief Counter count up flag.
 */
#define COUNTER_CONFIG_INFO_COUNT_UP BIT(0)

/**@} */

/**
 * @anchor COUNTER_TOP_FLAGS
 * @name Flags used by counter_top_cfg.
 * @{
 */

/**
 * @brief Flag preventing counter reset when top value is changed.
 *
 * If flags is set then counter is free running while top value is updated,
 * otherwise counter is reset (see @ref counter_set_top_value()).
 */
#define COUNTER_TOP_CFG_DONT_RESET BIT(0)

/**
 * @brief Flag instructing counter to reset itself if changing top value
 *	  results in counter going out of new top value bound.
 *
 * See @ref COUNTER_TOP_CFG_DONT_RESET.
 */
#define COUNTER_TOP_CFG_RESET_WHEN_LATE BIT(1)

/**@} */

/**
 * @anchor COUNTER_ALARM_FLAGS
 * @name Alarm configuration flags
 *
 * @brief Used in alarm configuration structure (@ref counter_alarm_cfg).
 * @{ */

/**
 * @brief Counter alarm absolute value flag.
 *
 * Ticks relation to counter value. If set ticks are treated as absolute value,
 * else it is relative to the counter reading performed during the call.
 */
#define COUNTER_ALARM_CFG_ABSOLUTE BIT(0)

/**
 * @brief Alarm flag enabling immediate expiration when driver detects that
 *	  absolute alarm was set too late.
 *
 * Alarm callback must be called from the same context as if it was set on time.
 */
#define COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE  BIT(1)

/**@} */

/**
 * @anchor COUNTER_GUARD_PERIOD_FLAGS
 * @name Counter guard period flags
 *
 * @brief Used by @ref counter_set_guard_period and
 *	  @ref counter_get_guard_period.
 * @{ */

/**
 * @brief Identifies guard period needed for detection of late setting of
 *	  absolute alarm (see @ref counter_set_channel_alarm).
 */
#define COUNTER_GUARD_PERIOD_LATE_TO_SET BIT(0)

/**@} */

/** @brief Alarm callback
 *
 * @param dev       Pointer to the device structure for the driver instance.
 * @param chan_id   Channel ID.
 * @param ticks     Counter value that triggered the alarm.
 * @param user_data User data.
 */
typedef void (*counter_alarm_callback_t)(const struct device *dev,
					 uint8_t chan_id, uint32_t ticks,
					 void *user_data);

/** @brief Alarm callback structure.
 */
struct counter_alarm_cfg {
	/**
	 * Callback called on alarm (cannot be NULL).
	 */
	counter_alarm_callback_t callback;
	/**
	 * Number of ticks that triggers the alarm.
	 *
	 * It can be relative (to now) or an absolute value (see @ref
	 * COUNTER_ALARM_CFG_ABSOLUTE). Both, relative and absolute, alarm
	 * values can be any value between zero and the current top value (see
	 * @ref counter_get_top_value). When setting an absolute alarm value
	 * close to the current counter value there is a risk that the counter
	 * will have counted past the given absolute value before the driver
	 * manages to activate the alarm. Therefore a guard period can be
	 * defined that lets the driver decide unambiguously whether it is late
	 * or not (see @ref counter_set_guard_period). If the counter is clock
	 * driven then ticks can be converted to microseconds (see @ref
	 * counter_ticks_to_us). Alternatively, the counter implementation may
	 * count asynchronous events.
	 */
	uint32_t ticks;
	/**
	 * User data returned in callback.
	 */
	void *user_data;
	/**
	 * Alarm flags (see @ref COUNTER_ALARM_FLAGS).
	 */
	uint32_t flags;
};

/** @brief Callback called when counter turns around.
 *
 * @param dev       Pointer to the device structure for the driver instance.
 * @param user_data User data provided in @ref counter_set_top_value.
 */
typedef void (*counter_top_callback_t)(const struct device *dev,
				       void *user_data);

/** @brief Top value configuration structure.
 */
struct counter_top_cfg {
	/**
	 * Top value.
	 */
	uint32_t ticks;
	/**
	 * Callback function (can be NULL).
	 */
	counter_top_callback_t callback;
	/**
	 * User data passed to callback function (not valid if callback is NULL).
	 */
	void *user_data;
	/**
	 * Flags (see @ref COUNTER_TOP_FLAGS).
	 */
	uint32_t flags;
};

/** @brief Structure with generic counter features.
 */
struct counter_config_info {
	/**
	 * Maximal (default) top value on which counter is reset (cleared or reloaded).
	 */
	uint32_t max_top_value;
	/**
	 * Frequency of the source clock if synchronous events are counted.
	 */
	uint32_t freq;
	/**
	 * Flags (see @ref COUNTER_FLAGS).
	 */
	uint8_t flags;
	/**
	 * Number of channels that can be used for setting alarm.
	 *
	 * @see counter_set_channel_alarm
	 */
	uint8_t channels;
};

typedef int (*counter_api_start)(const struct device *dev);
typedef int (*counter_api_stop)(const struct device *dev);
typedef int (*counter_api_get_value)(const struct device *dev,
				     uint32_t *ticks);
typedef int (*counter_api_get_value_64)(const struct device *dev,
			uint64_t *ticks);
typedef int (*counter_api_set_alarm)(const struct device *dev,
				     uint8_t chan_id,
				     const struct counter_alarm_cfg *alarm_cfg);
typedef int (*counter_api_cancel_alarm)(const struct device *dev,
					uint8_t chan_id);
typedef int (*counter_api_set_top_value)(const struct device *dev,
					 const struct counter_top_cfg *cfg);
typedef uint32_t (*counter_api_get_pending_int)(const struct device *dev);
typedef uint32_t (*counter_api_get_top_value)(const struct device *dev);
typedef uint32_t (*counter_api_get_guard_period)(const struct device *dev,
						 uint32_t flags);
typedef int (*counter_api_set_guard_period)(const struct device *dev,
						uint32_t ticks,
						uint32_t flags);
typedef uint32_t (*counter_api_get_freq)(const struct device *dev);

__subsystem struct counter_driver_api {
	counter_api_start start;
	counter_api_stop stop;
	counter_api_get_value get_value;
	counter_api_get_value_64 get_value_64;
	counter_api_set_alarm set_alarm;
	counter_api_cancel_alarm cancel_alarm;
	counter_api_set_top_value set_top_value;
	counter_api_get_pending_int get_pending_int;
	counter_api_get_top_value get_top_value;
	counter_api_get_guard_period get_guard_period;
	counter_api_set_guard_period set_guard_period;
	counter_api_get_freq get_freq;
};

/**
 * @brief Function to check if counter is counting up.
 *
 * @param[in]  dev    Pointer to the device structure for the driver instance.
 *
 * @retval true if counter is counting up.
 * @retval false if counter is counting down.
 */
__syscall bool counter_is_counting_up(const struct device *dev);

static inline bool z_impl_counter_is_counting_up(const struct device *dev)
{
	const struct counter_config_info *config =
			(const struct counter_config_info *)dev->config;

	return config->flags & COUNTER_CONFIG_INFO_COUNT_UP;
}

/**
 * @brief Function to get number of alarm channels.
 *
 * @param[in]  dev    Pointer to the device structure for the driver instance.
 *
 * @return Number of alarm channels.
 */
__syscall uint8_t counter_get_num_of_channels(const struct device *dev);

static inline uint8_t z_impl_counter_get_num_of_channels(const struct device *dev)
{
	const struct counter_config_info *config =
			(const struct counter_config_info *)dev->config;

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
__syscall uint32_t counter_get_frequency(const struct device *dev);

static inline uint32_t z_impl_counter_get_frequency(const struct device *dev)
{
	const struct counter_config_info *config =
			(const struct counter_config_info *)dev->config;
	const struct counter_driver_api *api =
				(struct counter_driver_api *)dev->api;

	return api->get_freq ? api->get_freq(dev) : config->freq;
}

/**
 * @brief Function to convert microseconds to ticks.
 *
 * @param[in]  dev    Pointer to the device structure for the driver instance.
 * @param[in]  us     Microseconds.
 *
 * @return Converted ticks. Ticks will be saturated if exceed 32 bits.
 */
__syscall uint32_t counter_us_to_ticks(const struct device *dev, uint64_t us);

static inline uint32_t z_impl_counter_us_to_ticks(const struct device *dev,
					       uint64_t us)
{
	uint64_t ticks = (us * z_impl_counter_get_frequency(dev)) / USEC_PER_SEC;

	return (ticks > (uint64_t)UINT32_MAX) ? UINT32_MAX : ticks;
}

/**
 * @brief Function to convert ticks to microseconds.
 *
 * @param[in]  dev    Pointer to the device structure for the driver instance.
 * @param[in]  ticks  Ticks.
 *
 * @return Converted microseconds.
 */
__syscall uint64_t counter_ticks_to_us(const struct device *dev, uint32_t ticks);

static inline uint64_t z_impl_counter_ticks_to_us(const struct device *dev,
					       uint32_t ticks)
{
	return ((uint64_t)ticks * USEC_PER_SEC) / z_impl_counter_get_frequency(dev);
}

/**
 * @brief Function to retrieve maximum top value that can be set.
 *
 * @param[in]  dev    Pointer to the device structure for the driver instance.
 *
 * @return Max top value.
 */
__syscall uint32_t counter_get_max_top_value(const struct device *dev);

static inline uint32_t z_impl_counter_get_max_top_value(const struct device *dev)
{
	const struct counter_config_info *config =
			(const struct counter_config_info *)dev->config;

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
__syscall int counter_start(const struct device *dev);

static inline int z_impl_counter_start(const struct device *dev)
{
	const struct counter_driver_api *api =
				(struct counter_driver_api *)dev->api;

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
__syscall int counter_stop(const struct device *dev);

static inline int z_impl_counter_stop(const struct device *dev)
{
	const struct counter_driver_api *api =
				(struct counter_driver_api *)dev->api;

	return api->stop(dev);
}

/**
 * @brief Get current counter value.
 * @param dev Pointer to the device structure for the driver instance.
 * @param ticks Pointer to where to store the current counter value
 *
 * @retval 0 If successful.
 * @retval Negative error code on failure getting the counter value
 */
__syscall int counter_get_value(const struct device *dev, uint32_t *ticks);

static inline int z_impl_counter_get_value(const struct device *dev,
					   uint32_t *ticks)
{
	const struct counter_driver_api *api =
				(struct counter_driver_api *)dev->api;

	return api->get_value(dev, ticks);
}

/**
 * @brief Get current counter 64-bit value.
 * @param dev Pointer to the device structure for the driver instance.
 * @param ticks Pointer to where to store the current counter value
 *
 * @retval 0 If successful.
 * @retval Negative error code on failure getting the counter value
 */
__syscall int counter_get_value_64(const struct device *dev, uint64_t *ticks);

static inline int z_impl_counter_get_value_64(const struct device *dev,
					   uint64_t *ticks)
{
	const struct counter_driver_api *api =
				(struct counter_driver_api *)dev->api;

	if (!api->get_value_64) {
		return -ENOTSUP;
	}

	return api->get_value_64(dev, ticks);
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
 * @retval -ETIME  if absolute alarm was set too late.
 * @retval -EBUSY  if alarm is already active.
 */
__syscall int counter_set_channel_alarm(const struct device *dev,
					uint8_t chan_id,
					const struct counter_alarm_cfg *alarm_cfg);

static inline int z_impl_counter_set_channel_alarm(const struct device *dev,
						   uint8_t chan_id,
						   const struct counter_alarm_cfg *alarm_cfg)
{
	const struct counter_driver_api *api =
				(struct counter_driver_api *)dev->api;

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
__syscall int counter_cancel_channel_alarm(const struct device *dev,
					   uint8_t chan_id);

static inline int z_impl_counter_cancel_channel_alarm(const struct device *dev,
						      uint8_t chan_id)
{
	const struct counter_driver_api *api =
				(struct counter_driver_api *)dev->api;

	if (chan_id >= counter_get_num_of_channels(dev)) {
		return -ENOTSUP;
	}

	return api->cancel_alarm(dev, chan_id);
}

/**
 * @brief Set counter top value.
 *
 * Function sets top value and optionally resets the counter to 0 or top value
 * depending on counter direction. On turnaround, counter can be reset and
 * optional callback is periodically called. Top value can only be changed when
 * there is no active channel alarm.
 *
 * @ref COUNTER_TOP_CFG_DONT_RESET prevents counter reset. When counter is
 * running while top value is updated, it is possible that counter progresses
 * outside the new top value. In that case, error is returned and optionally
 * driver can reset the counter (see @ref COUNTER_TOP_CFG_RESET_WHEN_LATE).
 *
 * @param dev		Pointer to the device structure for the driver instance.
 * @param cfg		Configuration. Cannot be NULL.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP if request is not supported (e.g. top value cannot be
 *		    changed or counter cannot/must be reset during top value
		    update).
 * @retval -EBUSY if any alarm is active.
 * @retval -ETIME if @ref COUNTER_TOP_CFG_DONT_RESET was set and new top value
 *		  is smaller than current counter value (counter counting up).
 */
__syscall int counter_set_top_value(const struct device *dev,
				    const struct counter_top_cfg *cfg);

static inline int z_impl_counter_set_top_value(const struct device *dev,
					       const struct counter_top_cfg
					       *cfg)
{
	const struct counter_driver_api *api =
				(struct counter_driver_api *)dev->api;

	if (cfg->ticks > counter_get_max_top_value(dev)) {
		return -EINVAL;
	}

	return api->set_top_value(dev, cfg);
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
__syscall int counter_get_pending_int(const struct device *dev);

static inline int z_impl_counter_get_pending_int(const struct device *dev)
{
	const struct counter_driver_api *api =
				(struct counter_driver_api *)dev->api;

	return api->get_pending_int(dev);
}

/**
 * @brief Function to retrieve current top value.
 *
 * @param[in]  dev    Pointer to the device structure for the driver instance.
 *
 * @return Top value.
 */
__syscall uint32_t counter_get_top_value(const struct device *dev);

static inline uint32_t z_impl_counter_get_top_value(const struct device *dev)
{
	const struct counter_driver_api *api =
				(struct counter_driver_api *)dev->api;

	return api->get_top_value(dev);
}

/**
 * @brief Set guard period in counter ticks.
 *
 * When setting an absolute alarm value close to the current counter value there
 * is a risk that the counter will have counted past the given absolute value
 * before the driver manages to activate the alarm. If this would go unnoticed
 * then the alarm would only expire after the timer has wrapped and reached the
 * given absolute value again after a full timer period. This could take a long
 * time in case of a 32 bit timer. Setting a sufficiently large guard period will
 * help the driver detect unambiguously whether it is late or not.
 *
 * The guard period should be as many counter ticks as the driver will need at
 * most to actually activate the alarm after the driver API has been called. If
 * the driver finds that the counter has just passed beyond the given absolute
 * tick value but is still close enough to fall within the guard period, it will
 * assume that it is "late", i.e. that the intended expiry time has already passed.
 * Depending on the @ref COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE flag the driver will
 * either ignore the alarm or expire it immediately in such a case.
 *
 * If, however, the counter is past the given absolute tick value but outside
 * the guard period, then the driver will assume that this is intentional and
 * let the counter wrap around to/from zero before it expires.
 *
 * More precisely:
 *
 * - When counting upwards (see @ref COUNTER_CONFIG_INFO_COUNT_UP) the given
 *   absolute tick value must be above (now + guard_period) % top_value to be
 *   accepted by the driver.
 * - When counting downwards, the given absolute tick value must be less than
 *   (now + top_value - guard_period) % top_value to be accepted.
 *
 * Examples:
 *
 * - counting upwards, now = 4950, top value = 5000, guard period = 100:
 *      absolute tick value >= (4950 + 100) % 5000 = 50
 * - counting downwards, now = 50, top value = 5000, guard period = 100:
 *      absolute tick value <= (50 + 5000 - * 100) % 5000 = 4950
 *
 * If you need only short alarm periods, you can set the guard period very high
 * (e.g. half of the counter top value) which will make it highly unlikely that
 * the counter will ever unintentionally wrap.
 *
 * The guard period is set to 0 on initialization (no protection).
 *
 * @param dev		Pointer to the device structure for the driver instance.
 * @param ticks		Guard period in counter ticks.
 * @param flags		See @ref COUNTER_GUARD_PERIOD_FLAGS.
 *
 * @retval 0 if successful.
 * @retval -ENOTSUP if function or flags are not supported.
 * @retval -EINVAL if ticks value is invalid.
 */
__syscall int counter_set_guard_period(const struct device *dev,
					uint32_t ticks,
					uint32_t flags);

static inline int z_impl_counter_set_guard_period(const struct device *dev,
						   uint32_t ticks, uint32_t flags)
{
	const struct counter_driver_api *api =
				(struct counter_driver_api *)dev->api;

	if (!api->set_guard_period) {
		return -ENOTSUP;
	}

	return api->set_guard_period(dev, ticks, flags);
}

/**
 * @brief Return guard period.
 *
 * @see counter_set_guard_period.
 *
 * @param dev	Pointer to the device structure for the driver instance.
 * @param flags	See @ref COUNTER_GUARD_PERIOD_FLAGS.
 *
 * @return Guard period given in counter ticks or 0 if function or flags are
 *	   not supported.
 */
__syscall uint32_t counter_get_guard_period(const struct device *dev,
					    uint32_t flags);

static inline uint32_t z_impl_counter_get_guard_period(const struct device *dev,
							uint32_t flags)
{
	const struct counter_driver_api *api =
				(struct counter_driver_api *)dev->api;

	return (api->get_guard_period) ? api->get_guard_period(dev, flags) : 0;
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/counter.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_COUNTER_H_ */
