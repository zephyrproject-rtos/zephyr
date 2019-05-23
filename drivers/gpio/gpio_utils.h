/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Header where utility code can be found for GPIO drivers
 */

#ifndef ZEPHYR_DRIVERS_GPIO_GPIO_UTILS_H_
#define ZEPHYR_DRIVERS_GPIO_GPIO_UTILS_H_


/**
 * @brief Generic function to insert or remove a callback from a callback list
 *
 * @param callbacks A pointer to the original list of callbacks (can be NULL)
 * @param callback A pointer of the callback to insert or remove from the list
 * @param set A boolean indicating insertion or removal of the callback
 *
 * @return 0 on success, negative errno otherwise.
 */
static inline int gpio_manage_callback(sys_slist_t *callbacks,
					struct gpio_callback *callback,
					bool set)
{
	__ASSERT(callback, "No callback!");
	__ASSERT(callback->handler, "No callback handler!");

	if (!sys_slist_is_empty(callbacks)) {
		if (!sys_slist_find_and_remove(callbacks, &callback->node)) {
			if (!set) {
				return -EINVAL;
			}
		}
	}

	if (set) {
		sys_slist_prepend(callbacks, &callback->node);
	}

	return 0;
}

/**
 * @brief Generic function to go through and fire callback from a callback list
 *
 * @param list A pointer on the gpio callback list
 * @param port A pointer on the gpio driver instance
 * @param pins The actual pin mask that triggered the interrupt
 */
static inline void gpio_fire_callbacks(sys_slist_t *list,
					struct device *port,
					u32_t pins)
{
	struct gpio_callback *cb, *tmp;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(list, cb, tmp, node) {
		if (cb->pin_mask & pins) {
			__ASSERT(cb->handler, "No callback handler!");
			cb->handler(port, cb, pins);
		}
	}
}

#endif /* ZEPHYR_DRIVERS_GPIO_GPIO_UTILS_H_ */
