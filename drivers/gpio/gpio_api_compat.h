/*
 * Copyright (c) 2016 Intel Corporation.
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
 * @file Header for the API 1.0 GPIO compatibility code
 */

#ifndef __GPIO_API_COMPAT_H__
#define __GPIO_API_COMPAT_H__

struct _gpio_compat_data {
	struct gpio_callback cb;
	gpio_callback_t handler;
};

struct gpio_compat_cb {
	struct device *dev;
	struct _gpio_compat_data *d;
};

/** This macro is mandatory to be used in order to enable the API 1.0
 * support on GPIO drivers.
 */
#define GPIO_SETUP_COMPAT_DEV(dev_name)					\
	static struct _gpio_compat_data (__gcd_##dev_name) = {};	\
									\
	static struct gpio_compat_cb (__gpio_compat_##dev_name) __used	\
	__attribute__((__section__(".gpio_compat.init"))) = {		\
		.dev = &(__device_##dev_name),				\
		.d = &(__gcd_##dev_name)				\
	}

/**
 * @brief Enable the API v1.0 callback on given pins
 *
 * @param port device driver instance pointer to affect
 * @param pins mask of pins to enable
 */
void _gpio_enable_callback(struct device *port, uint32_t pins);

/**
 * @brief Disable the API v1.0 callback on given pins
 *
 * @param port device driver instance pointer to affect
 * @param pins mask of pins to disable
 */
void _gpio_disable_callback(struct device *port, uint32_t pins);

#endif /* __GPIO_API_COMPAT_H__ */
