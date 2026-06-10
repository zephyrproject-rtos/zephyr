/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for GPIO utility functions
 * @ingroup gpio_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_UTILS_H_
#define ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_UTILS_H_

/**
 * @addtogroup gpio_interface
 * @{
 */

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

/**
 * @brief Makes a bitmask of allowed GPIOs from a number of GPIOs.
 *
 * @param ngpios number of GPIOs
 * @return the bitmask of allowed gpios
 */
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

/** @cond INTERNAL_HIDDEN */

/* Static initializer for a struct gpio_hog_dt_spec */
#define GPIO_HOG_DT_SPEC_GET_BY_IDX(idx, node_id)                                                  \
	{                                                                                          \
		.pin = DT_GPIO_HOG_PIN_BY_IDX(node_id, idx),                                       \
		.flags = DT_GPIO_HOG_FLAGS_BY_IDX(node_id, idx) |                                  \
			 COND_CODE_1(DT_PROP(node_id, input), (GPIO_INPUT),			   \
				     (COND_CODE_1(DT_PROP(node_id, output_low),			   \
						 (GPIO_OUTPUT_INACTIVE),			   \
						 (COND_CODE_1(DT_PROP(node_id, output_high),	   \
							     (GPIO_OUTPUT_ACTIVE), (0)))))),       \
	}

/* Expands to 1 if node_id is a GPIO hog, empty otherwise */
#define GPIO_HOGS_NODE_IS_GPIO_HOG(node_id) IF_ENABLED(DT_PROP_OR(node_id, gpio_hog, 0), 1)

/* Expands to 1 if GPIO controller node_id has GPIO hog children, 0 otherwise */
#define GPIO_HOGS_GPIO_CTLR_HAS_HOGS(node_id)                                                      \
	COND_CODE_0(IS_EMPTY(DT_FOREACH_CHILD_STATUS_OKAY(node_id, GPIO_HOGS_NODE_IS_GPIO_HOG)),   \
	(1), (0))

/* Called for GPIO hog dts nodes */
#define GPIO_HOGS_INIT_GPIO_HOGS(node_id)                                                          \
	LISTIFY(DT_NUM_GPIO_HOGS(node_id), GPIO_HOG_DT_SPEC_GET_BY_IDX, (,), node_id),

/* Called for GPIO controller dts node children */
#define GPIO_HOGS_COND_INIT_GPIO_HOGS(node_id)                                                     \
	IF_ENABLED(DT_PROP_OR(node_id, gpio_hog, 0), (GPIO_HOGS_INIT_GPIO_HOGS(node_id)))

/* Called for each GPIO controller dts node which has GPIO hog children */
#define GPIO_HOGS_INIT_GPIO_CTLR(node_id)                                                          \
	.gpio_hogs = {                                                                             \
		.specs = (const struct gpio_hog_dt_spec[]){DT_FOREACH_CHILD_STATUS_OKAY(           \
			node_id, GPIO_HOGS_COND_INIT_GPIO_HOGS)},                                  \
		.num_specs = DT_FOREACH_CHILD_STATUS_OKAY_SEP(node_id, DT_NUM_GPIO_HOGS, (+)),     \
	},

#if defined(CONFIG_GPIO_HOGS) || defined(__DOXYGEN__)
/**
 * @brief Make a struct gpio_hogs initializer from a node identifier
 *
 * @param node_id GPIO controller node identifier.
 * @return a struct gpio_hogs initializer
 */
#define GPIO_HOGS_COND_INIT_GPIO_CTLR(node_id)			\
	IF_ENABLED(GPIO_HOGS_GPIO_CTLR_HAS_HOGS(node_id),	\
		   (GPIO_HOGS_INIT_GPIO_CTLR(node_id)))
#else
#define GPIO_HOGS_COND_INIT_GPIO_CTLR(node_id)
#endif /* CONFIG_GPIO_HOGS */

/** @endcond */

/**
 * @brief Make a struct gpio_driver_config initializer from a node identifier
 *
 * @param node_id GPIO controller node identifier.
 * @return a struct gpio_driver_config initializer
 */
#define GPIO_COMMON_CONFIG_FROM_DT_NODE(node_id)                           \
	{                                                                  \
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_NODE(node_id), \
		GPIO_HOGS_COND_INIT_GPIO_CTLR(node_id)                     \
	}

/**
 * @brief Make a struct gpio_driver_config initializer from a DT_DRV_COMPAT instance number
 *
 * @param inst DT_DRV_COMPAT instance number
 * @return a struct gpio_driver_config initializer
 */
#define GPIO_COMMON_CONFIG_FROM_DT_INST(inst)                   \
	GPIO_COMMON_CONFIG_FROM_DT_NODE(DT_DRV_INST(inst))

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

/**
 * @brief Initialize GPIO hogs for a given device
 *
 * This function is called by GPIO drivers to initialize GPIO hogs defined in
 * devicetree for the device. It is expected to be called from gpio_common_init().
 *
 * @kconfig_dep{CONFIG_GPIO_HOGS}
 *
 * @param dev GPIO device for which to initialize hogs
 * @return 0 if successful, or a negative error code if initialization failed
 */
int gpio_hogs_init(const struct device *dev);

/**
 * @brief Common GPIO initialization function
 *
 * This function is intended to be called by GPIO drivers at the end of their
 * initialization function. It performs common initialization tasks, such as
 * initializing GPIO hogs if enabled.
 *
 * @param dev Pointer to the GPIO device being initialized.
 * @return 0 on success, negative errno otherwise.
 */
static inline int gpio_common_init(const struct device *dev)
{
#ifdef CONFIG_GPIO_HOGS
	return gpio_hogs_init(dev);
#else
	ARG_UNUSED(dev);
	return 0;
#endif
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_UTILS_H_ */
