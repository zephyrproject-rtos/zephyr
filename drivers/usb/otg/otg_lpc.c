/*
 * Private Porting
 * by David Hor - Xtooltech 2025
 * david.hor@xtooltech.com
 */

/*
 * Copyright (c) 2024 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/usb_otg.h>
#include <zephyr/logging/log.h>
#include <zephyr/init.h>

LOG_MODULE_REGISTER(otg_lpc, CONFIG_USB_DRIVER_LOG_LEVEL);

struct otg_lpc_config {
	const struct gpio_dt_spec id_gpio;
	const struct gpio_dt_spec vbus_gpio;
	const struct device *host_dev;
	const struct device *device_dev;
};

struct otg_lpc_data {
	enum usb_otg_role current_role;
	usb_otg_event_cb_t event_cb;
	struct gpio_callback id_cb;
	struct gpio_callback vbus_cb;
	struct k_work role_work;
	const struct device *dev;
};

static void otg_lpc_role_work(struct k_work *work)
{
	struct otg_lpc_data *data = CONTAINER_OF(work, struct otg_lpc_data, role_work);
	const struct device *dev = data->dev;
	const struct otg_lpc_config *config = dev->config;
	enum usb_otg_role new_role = USB_OTG_ROLE_NONE;
	enum usb_otg_event event = USB_OTG_EVENT_NONE;

	/* Read ID pin to determine role */
	if (gpio_pin_get_dt(&config->id_gpio) == 0) {
		/* ID pin low - host mode */
		new_role = USB_OTG_ROLE_HOST;
		event = USB_OTG_EVENT_ID_LOW;
	} else {
		/* ID pin high - device mode */
		new_role = USB_OTG_ROLE_DEVICE;
		event = USB_OTG_EVENT_ID_HIGH;
	}

	if (new_role != data->current_role) {
		LOG_INF("USB OTG role change: %s -> %s",
			data->current_role == USB_OTG_ROLE_HOST ? "host" : "device",
			new_role == USB_OTG_ROLE_HOST ? "host" : "device");

		data->current_role = new_role;

		/* Enable VBUS for host mode */
		if (new_role == USB_OTG_ROLE_HOST && config->vbus_gpio.port) {
			gpio_pin_set_dt(&config->vbus_gpio, 1);
		} else if (config->vbus_gpio.port) {
			gpio_pin_set_dt(&config->vbus_gpio, 0);
		}

		/* Notify application of role change */
		if (data->event_cb) {
			data->event_cb(dev, event, new_role);
		}
	}
}

static void otg_lpc_id_gpio_cb(const struct device *port,
			       struct gpio_callback *cb,
			       gpio_port_pins_t pins)
{
	struct otg_lpc_data *data = CONTAINER_OF(cb, struct otg_lpc_data, id_cb);

	k_work_submit(&data->role_work);
}

static void otg_lpc_vbus_gpio_cb(const struct device *port,
				 struct gpio_callback *cb,
				 gpio_port_pins_t pins)
{
	struct otg_lpc_data *data = CONTAINER_OF(cb, struct otg_lpc_data, vbus_cb);
	const struct device *dev = data->dev;
	const struct otg_lpc_config *config = dev->config;
	enum usb_otg_event event;

	if (gpio_pin_get_dt(&config->vbus_gpio)) {
		event = USB_OTG_EVENT_VBUS_VALID;
	} else {
		event = USB_OTG_EVENT_VBUS_INVALID;
	}

	/* Notify application of VBUS change */
	if (data->event_cb) {
		data->event_cb(dev, event, data->current_role);
	}
}

static int otg_lpc_init(const struct device *dev)
{
	const struct otg_lpc_config *config = dev->config;
	struct otg_lpc_data *data = dev->data;
	int ret;

	data->dev = dev;
	k_work_init(&data->role_work, otg_lpc_role_work);

	/* Configure ID pin */
	if (config->id_gpio.port) {
		ret = gpio_pin_configure_dt(&config->id_gpio, GPIO_INPUT);
		if (ret < 0) {
			LOG_ERR("Failed to configure ID GPIO: %d", ret);
			return ret;
		}

		ret = gpio_pin_interrupt_configure_dt(&config->id_gpio,
						     GPIO_INT_EDGE_BOTH);
		if (ret < 0) {
			LOG_ERR("Failed to configure ID GPIO interrupt: %d", ret);
			return ret;
		}

		gpio_init_callback(&data->id_cb, otg_lpc_id_gpio_cb,
				  BIT(config->id_gpio.pin));
		ret = gpio_add_callback(config->id_gpio.port, &data->id_cb);
		if (ret < 0) {
			LOG_ERR("Failed to add ID GPIO callback: %d", ret);
			return ret;
		}
	}

	/* Configure VBUS control pin */
	if (config->vbus_gpio.port) {
		ret = gpio_pin_configure_dt(&config->vbus_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure VBUS GPIO: %d", ret);
			return ret;
		}
	}

	/* Determine initial role */
	k_work_submit(&data->role_work);

	return 0;
}

static int otg_lpc_deinit(const struct device *dev)
{
	const struct otg_lpc_config *config = dev->config;
	struct otg_lpc_data *data = dev->data;

	if (config->id_gpio.port) {
		gpio_remove_callback(config->id_gpio.port, &data->id_cb);
	}

	if (config->vbus_gpio.port) {
		gpio_pin_set_dt(&config->vbus_gpio, 0);
	}

	return 0;
}

static int otg_lpc_set_role(const struct device *dev, enum usb_otg_role role)
{
	struct otg_lpc_data *data = dev->data;
	const struct otg_lpc_config *config = dev->config;

	if (role == data->current_role) {
		return 0;
	}

	LOG_INF("Setting USB OTG role to %s",
		role == USB_OTG_ROLE_HOST ? "host" : "device");

	data->current_role = role;

	/* Control VBUS based on role */
	if (role == USB_OTG_ROLE_HOST && config->vbus_gpio.port) {
		gpio_pin_set_dt(&config->vbus_gpio, 1);
	} else if (config->vbus_gpio.port) {
		gpio_pin_set_dt(&config->vbus_gpio, 0);
	}

	/* Notify application */
	if (data->event_cb) {
		data->event_cb(dev, USB_OTG_EVENT_ROLE_CHANGED, role);
	}

	return 0;
}

static enum usb_otg_role otg_lpc_get_role(const struct device *dev)
{
	struct otg_lpc_data *data = dev->data;

	return data->current_role;
}

static int otg_lpc_register_callback(const struct device *dev,
				    usb_otg_event_cb_t cb)
{
	struct otg_lpc_data *data = dev->data;

	data->event_cb = cb;
	return 0;
}

static int otg_lpc_enable(const struct device *dev)
{
	/* Role detection is already active after init */
	return 0;
}

static int otg_lpc_disable(const struct device *dev)
{
	return otg_lpc_deinit(dev);
}

static const struct usb_otg_api otg_lpc_api = {
	.init = otg_lpc_init,
	.deinit = otg_lpc_deinit,
	.set_role = otg_lpc_set_role,
	.get_role = otg_lpc_get_role,
	.register_callback = otg_lpc_register_callback,
	.enable = otg_lpc_enable,
	.disable = otg_lpc_disable,
};

#define OTG_LPC_INIT(n)							\
	static const struct otg_lpc_config otg_lpc_config_##n = {	\
		.id_gpio = GPIO_DT_SPEC_INST_GET_OR(n, id_gpios, {0}),	\
		.vbus_gpio = GPIO_DT_SPEC_INST_GET_OR(n, vbus_gpios, {0}),\
		.host_dev = DEVICE_DT_GET_OR_NULL(DT_INST_PHANDLE(n, host_controller)),\
		.device_dev = DEVICE_DT_GET_OR_NULL(DT_INST_PHANDLE(n, device_controller)),\
	};								\
									\
	static struct otg_lpc_data otg_lpc_data_##n;			\
									\
	DEVICE_DT_INST_DEFINE(n, otg_lpc_init, NULL,			\
			     &otg_lpc_data_##n, &otg_lpc_config_##n,	\
			     POST_KERNEL, CONFIG_USB_OTG_INIT_PRIORITY,\
			     &otg_lpc_api);

DT_INST_FOREACH_STATUS_OKAY(OTG_LPC_INIT)