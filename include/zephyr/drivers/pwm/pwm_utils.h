/*
 * Copyright (c) 2025 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PWM_PWM_UTILS_H_
#define ZEPHYR_INCLUDE_DRIVERS_PWM_PWM_UTILS_H_

#include <stdbool.h>
#include <stdint.h>
#include <errno.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Generic function to insert or remove a callback from a callback list.
 *
 * @param callbacks A pointer to the original list of callbacks (can be NULL).
 * @param callback A pointer of the callback to insert or remove from the list.
 * @param set A boolean indicating insertion or removal of the callback.
 *
 * @retval 0 on success.
 * @retval negate errno on failure.
 */
static inline int pwm_manage_event_callback(sys_slist_t *callbacks,
					    struct pwm_event_callback *callback, bool set)
{
	__ASSERT_NO_MSG(callback != NULL);
	__ASSERT_NO_MSG(callback->handler != NULL);

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
 * @brief Generic function to go through and fire callback from a callback lists.
 *
 * @param list A pointer on the pwm event callback list.
 * @param dev A pointer on the pwm driver instance.
 * @param channel The channel that fired the interrupt.
 * @param events The event that fired the interrupt.
 */
static inline void pwm_fire_event_callbacks(sys_slist_t *list, const struct device *dev,
					    uint32_t channel, pwm_flags_t events)
{
	struct pwm_event_callback *cb, *tmp;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(list, cb, tmp, node) {
		if (cb->channel == channel) {
			__ASSERT_NO_MSG(cb->handler != NULL);

			cb->handler(dev, cb, channel, events);
		}
	}
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_PWM_PWM_UTILS_H_ */
