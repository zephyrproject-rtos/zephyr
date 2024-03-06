/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CLOCK_CONTROL_UTILS_H
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CLOCK_CONTROL_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/slist.h>

/**
 * @brief Generic function to invert or remove a clock callback from a callback list
 *
 * @param callbacks A pointer to the original list of callbacks (can be NULL)
 * @param callback A pointer of the callback to insert or remove from the list
 * @param set A boolean indicating insertion or removal of the callback
 *
 * @return 0 on success, negative errno otherwise.
 */
static inline int clock_control_manage_callback(sys_slist_t *callbacks,
						struct clock_control_callback *callback,
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
	} else if (!set) {
		return -EINVAL;
	}

	if (set) {
		sys_slist_prepend(callbacks, &callback->node);
	}

	return 0;
}


/**
 * @brief Generic function to go through and fire callback from a callback list
 *
 * @param list A pointer on the clock_control callback list
 * @param dev A pointer to the clock control driver instance
 */
static inline void clock_control_fire_callbacks(sys_slist_t *list,
						const struct device *dev)
{
	struct clock_control_callback *cb, *tmp;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(list, cb, tmp, node) {
		__ASSERT(cb->handler, "No callback handler!");
		cb->handler(dev, cb->user_data);
	}
}

#ifdef __cplusplus
}
#endif

#endif
