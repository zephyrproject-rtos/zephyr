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
 * @file Implementation of the API 1.0 GPIO compatibility layer
 */

#include <errno.h>

#include <gpio.h>
#include <misc/util.h>

#include "gpio_api_compat.h"

/** These are maintained in a dedicated .gpio_compat section
 * See relevant arch's linker definitions in include/arch/
 */
extern struct gpio_compat_cb __gpio_compat_start[];
extern struct gpio_compat_cb __gpio_compat_end[];

static struct gpio_compat_cb *_gpio_compat_dev_lookup(struct device *dev)
{
	struct gpio_compat_cb *cb;

	for (cb = __gpio_compat_start; cb != __gpio_compat_end; cb++) {
		if (cb->dev == dev) {
			return cb;
		}
	}

	return NULL;
}

static void _gpio_compat_handler(struct device *dev,
				 struct gpio_callback *cb, uint32_t pins)
{
	struct _gpio_compat_data *data;
	int bit;

	data = CONTAINER_OF(cb, struct _gpio_compat_data, cb);

	for (bit = 0; bit < 32; bit++) {
		if (pins & BIT(bit)) {
			data->handler(dev, bit);
		}
	}
}

int gpio_set_callback(struct device *dev, gpio_callback_t callback)
{
	struct gpio_compat_cb *compat = _gpio_compat_dev_lookup(dev);
	int ret;

	if (!compat) {
		return -EIO;
	}

	ret = gpio_remove_callback(dev, &compat->d->cb);
	if (ret != 0) {
		return ret;
	}

	if (!callback) {
		return 0;
	}

	compat->d->handler = callback;
	compat->d->cb.handler = _gpio_compat_handler;

	return gpio_add_callback(dev, &compat->d->cb);
}

void _gpio_enable_callback(struct device *dev, uint32_t pins)
{
	struct gpio_compat_cb *compat = _gpio_compat_dev_lookup(dev);

	if (compat) {
		compat->d->cb.pin_mask |= pins;
	}
}

void _gpio_disable_callback(struct device *dev, uint32_t pins)
{
	struct gpio_compat_cb *compat = _gpio_compat_dev_lookup(dev);

	if (compat) {
		compat->d->cb.pin_mask &= ~(pins);
	}
}
