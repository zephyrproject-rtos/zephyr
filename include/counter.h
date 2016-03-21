/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 * @brief Public API for counter and timer drivers
 */

#ifndef __COUNTER_H__
#define __COUNTER_H__

/**
 * @brief COUNTER Interface
 * @defgroup counter_interface COUNTER Interface
 * @{
 */

#include <stdint.h>
#include <stddef.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*counter_callback_t)(struct device *dev, void *user_data);

typedef int (*counter_api_start)(struct device *dev);
typedef int (*counter_api_stop)(struct device *dev);
typedef uint32_t (*counter_api_read)(void);
typedef int (*counter_api_set_alarm)(struct device *dev,
				     counter_callback_t callback,
				     uint32_t count, void *user_data);

struct counter_driver_api {
	counter_api_start start;
	counter_api_stop stop;
	counter_api_read read;
	counter_api_set_alarm set_alarm;
};

/**
 * @brief Start counter device in free running mode.
 *
 * Start the counter device. If the device is a 'countup' counter, the
 * counter initial value is set to zero. If it is a 'countdown' counter,
 * the initial value is set to the maximum value supported by the device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int counter_start(struct device *dev)
{
	struct counter_driver_api *api;

	api = (struct counter_driver_api *)dev->driver_api;

	return api->start(dev);
}

/**
 * @brief Stop counter device.
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 If successful.
 * @retval -ENODEV if the device doesn't support stopping the
 *                        counter.
 */
static inline int counter_stop(struct device *dev)
{
	struct counter_driver_api *api;

	api = (struct counter_driver_api *)dev->driver_api;

	return api->stop(dev);
}

/**
 * @brief Read current counter value.
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return  32-bit value
 */
static inline uint32_t counter_read(struct device *dev)
{
	struct counter_driver_api *api;

	api = (struct counter_driver_api *)dev->driver_api;

	return api->read();
}

/**
 * @brief Set an alarm.
 * @param dev Pointer to the device structure for the driver instance.
 * @param callback Pointer to the callback function. if this is NULL,
 *                 this function unsets the alarm.
 * @param count Number of counter ticks.
 * @param user_data Pointer to user data.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP if the counter was not started yet.
 * @retval -ENODEV if the device doesn't support interrupt (e.g. free
 *                        running counters).
 * @retval Negative errno code if failure.
 */
static inline int counter_set_alarm(struct device *dev,
				    counter_callback_t callback,
				    uint32_t count, void *user_data)
{
	struct counter_driver_api *api;

	api = (struct counter_driver_api *)dev->driver_api;

	return api->set_alarm(dev, callback, count, user_data);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __COUNTER_H__ */
