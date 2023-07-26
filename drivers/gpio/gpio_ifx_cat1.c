/*
 * Copyright (c) 2022 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief GPIO driver for Infineon CAT1 MCU family.
 *
 * Note:
 * - Trigger detection on pin rising or falling edge (GPIO_INT_TRIG_BOTH)
 *   is not supported in current version of GPIO CAT1 driver.
 */

#define DT_DRV_COMPAT  infineon_cat1_gpio

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>

#include <cyhal_gpio.h>
#include <cyhal_hwmgr.h>
#include <cy_gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_cat1, CONFIG_GPIO_LOG_LEVEL);

/* Device config structure */
struct gpio_cat1_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	cyhal_gpio_callback_data_t *cb_data_ptr;
	GPIO_PRT_Type              *regs;
	uint8_t ngpios;
	uint8_t intr_priority;
};

/* Data structure */
struct gpio_cat1_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;

	/* device's owner of this data */
	const struct device *dev;

	/* callbacks list */
	sys_slist_t callbacks;
};

/* Get port number by calculation difference from current port address minus
 * GPIO base address divided by GPIO structure size.
 */
#define GET_PORT_NUM(dev)				     \
	(((uint32_t) ((const struct gpio_cat1_config *const) \
		      (dev)->config)->regs - CY_GPIO_BASE) / GPIO_PRT_SECTION_SIZE)

#define GET_DEV_OBJ_FROM_LIST(i, _) \
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpio_prt##i))

/* Map port number to device object */
static const struct device *const port_dev_obj[IOSS_GPIO_GPIO_PORT_NR] = {
	LISTIFY(15, GET_DEV_OBJ_FROM_LIST, (,))
};

static int gpio_cat1_configure(const struct device *dev,
			       gpio_pin_t pin, gpio_flags_t flags)
{
	cy_rslt_t status;
	cyhal_gpio_t gpio_pin = CYHAL_GET_GPIO(GET_PORT_NUM(dev), pin);
	cyhal_gpio_drive_mode_t gpio_mode = CYHAL_GPIO_DRIVE_NONE;
	cyhal_gpio_direction_t gpio_dir = CYHAL_GPIO_DIR_INPUT;
	bool pin_val = false;

	switch (flags & (GPIO_INPUT | GPIO_OUTPUT)) {
	case GPIO_INPUT:
		gpio_dir = CYHAL_GPIO_DIR_INPUT;

		if ((flags & GPIO_PULL_UP) && (flags & GPIO_PULL_DOWN)) {
			gpio_mode = CYHAL_GPIO_DRIVE_PULLUPDOWN;
		} else if (flags & GPIO_PULL_UP) {
			gpio_mode = CYHAL_GPIO_DRIVE_PULLUP;
			pin_val = true;
		} else if (flags & GPIO_PULL_DOWN) {
			gpio_mode = CYHAL_GPIO_DRIVE_PULLDOWN;
		} else {
			gpio_mode = CYHAL_GPIO_DRIVE_NONE;
		}
		break;

	case GPIO_OUTPUT:
		gpio_dir = CYHAL_GPIO_DIR_OUTPUT;
		if (flags & GPIO_SINGLE_ENDED) {
			if (flags & GPIO_LINE_OPEN_DRAIN) {
				gpio_mode = CYHAL_GPIO_DRIVE_OPENDRAINDRIVESLOW;
				pin_val = true;
			} else {
				gpio_mode = CYHAL_GPIO_DRIVE_OPENDRAINDRIVESHIGH;
				pin_val = false;
			}
		} else {
			gpio_mode = CYHAL_GPIO_DRIVE_STRONG;
			pin_val = (flags & GPIO_OUTPUT_INIT_HIGH) ? true : false;
		}
		break;

	case GPIO_DISCONNECTED:
		/* Handle this after calling cyhal_gpio_init(), otherwise it will cause an assert
		 * from HAL for freeing an uninitialized pin
		 */
		break;

	default:
		return -ENOTSUP;
	}

	status = cyhal_gpio_init(gpio_pin, gpio_dir, gpio_mode, pin_val);

	/* If the gpio requested resource is already in use, try to free and
	 * initialize again
	 */
	if (status == CYHAL_HWMGR_RSLT_ERR_INUSE) {
		cyhal_gpio_free(gpio_pin);
		status = cyhal_gpio_init(gpio_pin, gpio_dir, gpio_mode, pin_val);
	}

	if (flags & GPIO_DISCONNECTED) {
		cyhal_gpio_free(gpio_pin);
	}

	return (status == CY_RSLT_SUCCESS) ? 0 : -EIO;

}

static int gpio_cat1_port_get_raw(const struct device *dev,
				  uint32_t *value)
{
	const struct gpio_cat1_config *const cfg = dev->config;
	GPIO_PRT_Type *const base = cfg->regs;

	*value = GPIO_PRT_IN(base);

	return 0;
}

static int gpio_cat1_port_set_masked_raw(const struct device *dev,
					 uint32_t mask, uint32_t value)
{
	const struct gpio_cat1_config *const cfg = dev->config;
	GPIO_PRT_Type *const base = cfg->regs;

	GPIO_PRT_OUT(base) = (GPIO_PRT_OUT(base) & ~mask) | (mask & value);

	return 0;
}

static int gpio_cat1_port_set_bits_raw(const struct device *dev,
				       uint32_t mask)
{
	const struct gpio_cat1_config *const cfg = dev->config;
	GPIO_PRT_Type *const base = cfg->regs;

	GPIO_PRT_OUT_SET(base) = mask;

	return 0;
}

static int gpio_cat1_port_clear_bits_raw(const struct device *dev,
					 uint32_t mask)
{
	const struct gpio_cat1_config *const cfg = dev->config;
	GPIO_PRT_Type *const base = cfg->regs;

	GPIO_PRT_OUT_CLR(base) = mask;

	return 0;
}

static int gpio_cat1_port_toggle_bits(const struct device *dev,
				      uint32_t mask)
{
	const struct gpio_cat1_config *const cfg = dev->config;
	GPIO_PRT_Type *const base = cfg->regs;

	GPIO_PRT_OUT_INV(base) = mask;

	return 0;
}

static uint32_t gpio_cat1_get_pending_int(const struct device *dev)
{
	const struct gpio_cat1_config *const cfg = dev->config;
	GPIO_PRT_Type *const base = cfg->regs;

	return GPIO_PRT_INTR_MASKED(base);
}

static void gpio_event_callback(void *callback_arg, cyhal_gpio_event_t event)
{
	ARG_UNUSED(event);
	uint32_t port_num = CYHAL_GET_PORT((uint32_t) callback_arg);
	uint32_t pin_num = CYHAL_GET_PIN((uint32_t) callback_arg);
	const struct device *dev = port_dev_obj[port_num];

	/* Goes through and fires callback from a callback list */
	if (dev) {
		gpio_fire_callbacks(&((struct gpio_cat1_data *const)(dev)->data)->callbacks,
				    dev, 1 << pin_num);
	}
	/* NOTE: cyhal gpio handles cleaning of interrupts */
}

static int gpio_cat1_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					     enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct gpio_cat1_config *const cfg = dev->config;
	cyhal_gpio_callback_data_t *cb_data_ptr = cfg->cb_data_ptr;
	cyhal_gpio_event_t event = CYHAL_GPIO_IRQ_NONE;

	/* Level interrupts (GPIO_INT_MODE_LEVEL) is not supported */
	if (mode == GPIO_INT_MODE_LEVEL) {
		return -ENOTSUP;
	}

	switch (trig) {
	case GPIO_INT_TRIG_LOW:
		event = CYHAL_GPIO_IRQ_FALL;
		break;

	case GPIO_INT_TRIG_HIGH:
		event = CYHAL_GPIO_IRQ_RISE;
		break;

	case GPIO_INT_TRIG_BOTH:
		/* Trigger detection on pin rising or falling edge (GPIO_INT_TRIG_BOTH)
		 * is not supported. Refer to SWINTEGRATION-696
		 */
		/* event = CYHAL_GPIO_IRQ_BOTH; */
		return -ENOTSUP;

	default:
		return -ENOTSUP;
	}

	cyhal_gpio_t gpio_pin = CYHAL_GET_GPIO(GET_PORT_NUM(dev), pin);

	/* Find index of free callback data structure */
	uint32_t index;

	for (index = 0u; index < cfg->ngpios; index++) {
		if ((cb_data_ptr[index].callback == NULL) || (cb_data_ptr[index].pin == gpio_pin)) {
			break;
		}
	}

	if (index != cfg->ngpios) {
		/* Store callback data: gpio callback and gpio device driver handle */
		cb_data_ptr[index].callback = &gpio_event_callback;
		cb_data_ptr[index].callback_arg = (void *)(gpio_pin);

		/* Register/clear a callback handler for pin events */
		cyhal_gpio_register_callback(gpio_pin, (mode == GPIO_INT_MODE_DISABLED) ?
					     NULL : &cb_data_ptr[index]);

		/* Enable/disable the specified GPIO event */
		cyhal_gpio_enable_event(gpio_pin, event, cfg->intr_priority,
					(mode == GPIO_INT_MODE_DISABLED) ? false : true);
		return 0;
	} else {
		return -EINVAL;
	}
}

static int gpio_cat1_manage_callback(const struct device *port,
				     struct gpio_callback *callback,
				     bool set)
{
	return gpio_manage_callback(&((struct gpio_cat1_data *const)(port)->data)->callbacks,
				    callback, set);
}

static const struct gpio_driver_api gpio_cat1_api = {
	.pin_configure = gpio_cat1_configure,
	.port_get_raw = gpio_cat1_port_get_raw,
	.port_set_masked_raw = gpio_cat1_port_set_masked_raw,
	.port_set_bits_raw = gpio_cat1_port_set_bits_raw,
	.port_clear_bits_raw = gpio_cat1_port_clear_bits_raw,
	.port_toggle_bits = gpio_cat1_port_toggle_bits,
	.pin_interrupt_configure = gpio_cat1_pin_interrupt_configure,
	.manage_callback = gpio_cat1_manage_callback,
	.get_pending_int = gpio_cat1_get_pending_int,
};

#define GPIO_CAT1_INIT(n)							     \
										     \
	cyhal_gpio_callback_data_t						     \
	_cat1_gpio##n##_cb_data[DT_INST_PROP(n, ngpios)];			     \
										     \
	static const struct gpio_cat1_config _cat1_gpio##n##_config = {		     \
		.common = {							     \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n), \
			},							     \
		.intr_priority = DT_INST_IRQ_BY_IDX(n, 0, priority),		     \
		.cb_data_ptr = _cat1_gpio##n##_cb_data,				     \
		.ngpios = DT_INST_PROP(n, ngpios),				     \
		.regs = (GPIO_PRT_Type *)DT_INST_REG_ADDR(n),			     \
	};									     \
										     \
	static struct gpio_cat1_data _cat1_gpio##n##_data;			     \
										     \
	DEVICE_DT_INST_DEFINE(n, NULL, NULL, &_cat1_gpio##n##_data,		     \
			      &_cat1_gpio##n##_config, POST_KERNEL,		     \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		     \
			      &gpio_cat1_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_CAT1_INIT)
