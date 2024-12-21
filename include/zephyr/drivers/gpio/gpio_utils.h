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
#include <zephyr/tracing/tracing.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GPIO_PORT_PIN_MASK_FROM_NGPIOS(ngpios)			\
	((gpio_port_pins_t)(((uint64_t)1 << (ngpios)) - 1U))

/**
 * @brief Makes a bitmask of allowed GPIOs from the @p "gpio-reserved-ranges"
 *        and @p "ngpios" DT properties values
 *
 * @param node_id GPIO controller node identifier.
 * @return the bitmask of allowed gpios
 * @see GPIO_DT_PORT_PIN_MASK_NGPIOS_EXC()
 */
#define GPIO_PORT_PIN_MASK_FROM_DT_NODE(node_id)		\
	GPIO_DT_PORT_PIN_MASK_NGPIOS_EXC(node_id, DT_PROP(node_id, ngpios))

/**
 * @brief Make a bitmask of allowed GPIOs from a DT_DRV_COMPAT instance's GPIO
 *        @p "gpio-reserved-ranges" and @p "ngpios" DT properties values
 *
 * @param inst DT_DRV_COMPAT instance number
 * @return the bitmask of allowed gpios
 * @see GPIO_DT_PORT_PIN_MASK_NGPIOS_EXC()
 */
#define GPIO_PORT_PIN_MASK_FROM_DT_INST(inst)			\
	GPIO_PORT_PIN_MASK_FROM_DT_NODE(DT_DRV_INST(inst))

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
static inline void gpio_fire_callbacks(sys_slist_t *list,
					const struct device *port,
					uint32_t pins)
{
	struct gpio_callback *cb, *tmp;

	sys_port_trace_gpio_fire_callbacks_enter(list, port, pins);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(list, cb, tmp, node) {
		if (cb->pin_mask & pins) {
			__ASSERT(cb->handler, "No callback handler!");

			cb->handler(port, cb, cb->pin_mask & pins);
			sys_port_trace_gpio_fire_callback(port, cb);
		}
	}
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_UTILS_H_ */
