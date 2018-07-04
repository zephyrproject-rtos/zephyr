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
	u8_t channel_id; /*!< Channel ID. Number of channels is driver
			  * dependent.
			  */

	bool absolute; /*!< If true ticks are treated as absolute value, else
			*   it is relative to the counter reading performed
			*   during the call.
			*/

	u32_t ticks; /*!< In case of absolute flag is set, maximal value that
		      *   can be set equals wrap value (counter_get_wrap).
		      *   Otherwise counter_get_max_relative_alarm() returns
		      *   maximal value that can be set. If counter is clock
		      *   driven then ticks can be converted to microseconds
		      *   (see @ref counter_ticks_to_us). Alternatively,
		      *   counter implementation may count asynchronous events.
		      */

	u32_t period; /*!< If non-zero alarm is repeated. If counter is clock
		       *   driven then period can be converted to microseconds
		       *   (see @ref counter_ticks_to_us). Alternatively,
		       *   counter implementation may count asynchronous events.
		       */

	counter_alarm_callback_handler_t handler; /*!< handler called on alarm
						   *   (cannot be NULL).
						   */
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
	u32_t ticks; /*!< If counter is clock driven then period can be
		      *   converted to microseconds (see
		      *   @ref counter_ticks_to_us). Alternatively, counter
		      *   implementation may count asynchronous events.
		      */
	counter_wrap_callback_handler_t handler; /*!< handler called on wrapping
						  *   (can be NULL).
						  */
};

/** @brief Structure with generic counter features. */
struct counter_config_info {
	u32_t max_wrap; /*!< Maximal (default) wrap value on which counter is
			 *   reset (cleared or reloaded).
			 */

	u32_t freq; /*!< Frequency of the source clock if synchronous events
		     *   are counted.
		     */

	bool count_up; /*!< Flag indicating direction of the counter. If true
			*   counter is counting up else counting down.
			*/

	u8_t channels; /*!< Number of channels that can be used for setting
			*   alarm, see @ref counter_set_alarm.
			*/

	const void *config_info; /*!< Pointer to implementation specific
				  *   configuration.
				  */
};

typedef int (*counter_api_start)(struct device *dev);
typedef int (*counter_api_stop)(struct device *dev);
typedef u32_t (*counter_api_read)(struct device *dev);
typedef int (*counter_api_set_alarm)(struct device *dev,
				     struct counter_alarm_callback *cb);
typedef int (*counter_api_disable_alarm)(struct device *dev,
				     struct counter_alarm_callback *cb);
typedef int (*counter_api_set_wrap_alarm)(struct device *dev,
					  struct counter_wrap_callback *cb);
typedef u32_t (*counter_api_get_pending_int)(struct device *dev);
typedef u32_t (*counter_api_get_wrap)(struct device *dev);
typedef u32_t (*counter_api_get_max_relative_alarm)(struct device *dev);

struct counter_driver_api {
	counter_api_start start;
	counter_api_stop stop;
	counter_api_read read;
	counter_api_set_alarm set_alarm;
	counter_api_disable_alarm disable_alarm;
	counter_api_set_wrap_alarm set_wrap_alarm;
	counter_api_get_pending_int get_pending_int;
	counter_api_get_wrap get_wrap;
	counter_api_get_max_relative_alarm get_max_relative_alarm;
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
 * @param cb  Callback structure. If NULL counter wrapping is reset to initial
 *	      state.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP if the counter was not started yet.
 * @retval -ENODEV if the device doesn't support interrupt (e.g. free
 *                        running counters).
 * @retval -EBUSY if any alarm is active.
 * @retval Negative errno code if failure.
 */
static inline int counter_set_wrap_alarm(struct device *dev,
					 struct counter_wrap_callback *cb)
{
	const struct counter_driver_api *api = dev->driver_api;

	return api->set_wrap_alarm(dev, cb);
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
__syscall u32_t counter_us_to_ticks(struct device *dev, u64_t us);

static inline u32_t _impl_counter_us_to_ticks(struct device *dev, u64_t us)
{
	const struct counter_config_info *config = dev->config->config_info;
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
__syscall u64_t counter_ticks_to_us(struct device *dev, u32_t ticks);

static inline u64_t _impl_counter_ticks_to_us(struct device *dev, u32_t ticks)
{
	const struct counter_config_info *config = dev->config->config_info;

	return ((u64_t)ticks * USEC_PER_SEC) / config->freq;
}

/**
 * @brief Function to retrieve maximum wrap value that can be set.
 *
 * @param[in]  dev    Pointer to the device structure for the driver instance.
 *
 * @return Max wrap value.
 */
__syscall u32_t counter_get_max_wrap(struct device *dev);

static inline u32_t _impl_counter_get_max_wrap(struct device *dev)
{
	const struct counter_config_info *config = dev->config->config_info;

	return config->max_wrap;
}

/**
 * @brief Function to retrieve current wrap value.
 *
 * @param[in]  dev    Pointer to the device structure for the driver instance.
 *
 * @return Wrap value.
 */
__syscall u32_t counter_get_wrap(struct device *dev);

static inline u32_t _impl_counter_get_wrap(struct device *dev)
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
__syscall u32_t counter_get_max_relative_alarm(struct device *dev);

static inline u32_t _impl_counter_get_max_relative_alarm(struct device *dev)
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
__syscall bool counter_count_up(struct device *dev);

static inline bool _impl_counter_count_up(struct device *dev)
{
	const struct counter_config_info *config = dev->config->config_info;

	return config->count_up;
}

/**
 * @brief Function to get number of alarm channels.
 *
 * @param[in]  dev    Pointer to the device structure for the driver instance.
 *
 * @return Number of alarm channels.
 */
static inline u8_t counter_get_num_of_channels(struct device *dev)
{
	const struct counter_config_info *config = dev->config->config_info;

	return config->channels;
}

/**
 * @internal
 *
 * @brief Function to get implementation specific configuration structure.
 *
 * @note Function for internal use only.
 *
 * @param[in]  dev    Pointer to the device structure for the driver instance.
 *
 * @return Pointer to the configuration structure.
 */
static inline const struct counter_config_info *_counter_get_config(
							struct device *dev)
{
	return dev->config->config_info;
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/counter.h>

#endif /* __COUNTER_H__ */
