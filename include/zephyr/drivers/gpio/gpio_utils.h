/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_UTILS_H_
#define ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_UTILS_H_

#include <stdbool.h>
#include <stdint.h>
#include <errno.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/slist.h>

#define GPIO_PORT_PIN_MASK_FROM_NGPIOS(ngpios)			\
	((gpio_port_pins_t)(((uint64_t)1 << (ngpios)) - 1U))

#define GPIO_PORT_PIN_MASK_FROM_DT_NODE(node_id)		\
	GPIO_PORT_PIN_MASK_FROM_NGPIOS(DT_PROP(node_id, ngpios))

#define GPIO_PORT_PIN_MASK_FROM_DT_INST(inst)			\
	GPIO_PORT_PIN_MASK_FROM_NGPIOS(DT_INST_PROP(inst, ngpios))

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
 * @param list A pointer on the gpio callback list
 * @param port A pointer on the gpio driver instance
 * @param pins The actual pin mask that triggered the interrupt
 */
static inline void gpio_fire_callbacks_general(sys_slist_t *list,
					const struct device *port,
					uint32_t pins)
{
	struct gpio_callback *cb, *tmp;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(list, cb, tmp, node) {
		if (cb->pin_mask & pins) {
			__ASSERT(cb->handler, "No callback handler!");
			cb->handler(port, cb, cb->pin_mask & pins);
		}
	}
}

#if CONFIG_GPIO_INTERNAL_INTERRUPT

/**
 * @brief Generic function to go through and fire callback from a callback list(but prefer internal callback)
 *
 * @param list A pointer on the gpio callback list
 * @param port A pointer on the gpio driver instance
 * @param pins The actual pin mask that triggered the interrupt
 *
 * @note if has internal callback, only fire internal callback
 */
static inline void gpio_fire_callbacks_prefer_internal(sys_slist_t *list,
					const struct device *port,
					uint32_t pins)
{
	struct gpio_callback *cb, *tmp;
	bool has_internal = false;

	// check if has any internal callback for this pins
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(list, cb, tmp, node) {
		if (cb->pin_mask & pins && cb->is_internal) {
			has_internal = true;
			break;
		}
	}

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(list, cb, tmp, node) {
		if (cb->pin_mask & pins && (!has_internal || cb->is_internal)) {
			__ASSERT(cb->handler, "No callback handler!");
			cb->handler(port, cb, cb->pin_mask & pins);
		}
	}
}

/**
 * @brief Generic function for fire external callbacks
 *
 * @param list A pointer on the gpio callback list
 * @param port A pointer on the gpio driver instance
 * @param pins The actual pin mask that triggered the interrupt
 *
 * @note this function will called in the internal callback when want to fire external callback
 */
static inline void gpio_fire_callbacks_force_external(sys_slist_t *list,
					const struct device *port,
					uint32_t pins)
{
	struct gpio_callback *cb, *tmp;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(list, cb, tmp, node) {
		if (cb->pin_mask & pins && !cb->is_internal) {
			__ASSERT(cb->handler, "No callback handler!");
			cb->handler(port, cb, cb->pin_mask & pins);
		}
	}
}
#endif

/**
 * @brief Generic function to go through and fire callback from a callback list
 *
 * @param list A pointer on the gpio callback list
 * @param port A pointer on the gpio driver instance
 * @param pins The actual pin mask that triggered the interrupt
 */
static inline void gpio_fire_callbacks(sys_slist_t *list,
					const struct device *port,
					uint32_t pins)
{
#if CONFIG_GPIO_INTERNAL_INTERRUPT
	gpio_fire_callbacks_prefer_internal(list, port, pins);
#else
	gpio_fire_callbacks_general(list, port, pins);
#endif
}

#endif /* ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_UTILS_H_ */
