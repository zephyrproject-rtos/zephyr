/*
 * Copyright (c) 2026 Leica Geosystems AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_usb_vbus_detect_gpio

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/drivers/usb/usb_vbus_detect.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "../udc/udc_common.h"

LOG_MODULE_REGISTER(usb_vbus_detect_gpio, CONFIG_USB_VBUS_DETECT_LOG_LEVEL);

struct usb_vbus_detect_gpio_config {
	struct gpio_dt_spec vbus_gpio;
	const struct device *const *udc_list;
	size_t udc_count;
	uint32_t debounce_ms;
};

struct usb_vbus_detect_gpio_data {
	const struct device *dev;
	struct gpio_callback gpio_cb;
	struct k_work_delayable debounce_work;
	bool last_state;
	bool initialized;
};

static int usb_vbus_notify_device(const struct device *udc_dev,
				  enum udc_event_type event)
{
	if (!udc_is_initialized(udc_dev)) {
		LOG_DBG("Device %s not initialized, skipping", udc_dev->name);
		return -EPERM;
	}

	return udc_submit_event(udc_dev, event, 0);
}

static void usb_vbus_notify_all(const struct usb_vbus_detect_gpio_config *cfg,
				bool vbus_ready)
{
	enum udc_event_type event = vbus_ready ? UDC_EVT_VBUS_READY
					       : UDC_EVT_VBUS_REMOVED;
	const char *event_name = vbus_ready ? "VBUS_READY" : "VBUS_REMOVED";
	int ret;

	for (size_t i = 0; i < cfg->udc_count; i++) {
		const struct device *udc_dev = cfg->udc_list[i];

		ret = usb_vbus_notify_device(udc_dev, event);
		if (ret < 0 && ret != -EPERM) {
			LOG_ERR("Failed to signal %s to %s: %d",
				event_name, udc_dev->name, ret);
		} else if (ret == 0) {
			LOG_INF("%s sent to %s", event_name, udc_dev->name);
		}
	}
}

static void usb_vbus_detect_gpio_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct usb_vbus_detect_gpio_data *data =
		CONTAINER_OF(dwork, struct usb_vbus_detect_gpio_data,
			     debounce_work);
	const struct device *dev = data->dev;
	const struct usb_vbus_detect_gpio_config *cfg = dev->config;
	int state;

	state = gpio_pin_get_dt(&cfg->vbus_gpio);
	if (state < 0) {
		LOG_ERR("Failed to read VBUS GPIO: %d", state);
		return;
	}

	LOG_DBG("VBUS state: %d (last: %d, initialized: %d)",
		state, data->last_state, data->initialized);

	if (!data->initialized) {
		data->last_state = state;
		data->initialized = true;
		usb_vbus_notify_all(cfg, state != 0);
		return;
	}

	if (state != data->last_state) {
		data->last_state = state;
		usb_vbus_notify_all(cfg, state != 0);
	}
}

static void usb_vbus_detect_gpio_callback(const struct device *port,
					  struct gpio_callback *cb,
					  gpio_port_pins_t pins)
{
	struct usb_vbus_detect_gpio_data *data =
		CONTAINER_OF(cb, struct usb_vbus_detect_gpio_data, gpio_cb);
	const struct device *dev = data->dev;
	const struct usb_vbus_detect_gpio_config *cfg = dev->config;

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	k_work_reschedule(&data->debounce_work, K_MSEC(cfg->debounce_ms));
}

static int usb_vbus_detect_gpio_init(const struct device *dev)
{
	const struct usb_vbus_detect_gpio_config *cfg = dev->config;
	struct usb_vbus_detect_gpio_data *data = dev->data;
	int ret;

	data->dev = dev;
	data->initialized = false;

	if (!gpio_is_ready_dt(&cfg->vbus_gpio)) {
		LOG_ERR("VBUS GPIO device not ready");
		return -ENODEV;
	}

	/* Check all UDC devices are ready and inform them about VBUS detect */
	for (size_t i = 0; i < cfg->udc_count; i++) {
		const struct device *udc_dev = cfg->udc_list[i];

		if (!device_is_ready(udc_dev)) {
			LOG_ERR("UDC device %s not ready", udc_dev->name);
			return -ENODEV;
		}

		/* Inform UDC that external VBUS detection is available */
		udc_set_can_detect_vbus(udc_dev);
		LOG_DBG("Registered VBUS detection for %s", udc_dev->name);
	}

	ret = gpio_pin_configure_dt(&cfg->vbus_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure VBUS GPIO: %d", ret);
		return ret;
	}

	k_work_init_delayable(&data->debounce_work,
			      usb_vbus_detect_gpio_work_handler);

	gpio_init_callback(&data->gpio_cb, usb_vbus_detect_gpio_callback,
			   BIT(cfg->vbus_gpio.pin));

	ret = gpio_add_callback(cfg->vbus_gpio.port, &data->gpio_cb);
	if (ret < 0) {
		LOG_ERR("Failed to add GPIO callback: %d", ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->vbus_gpio,
					      GPIO_INT_EDGE_BOTH);
	if (ret < 0) {
		LOG_ERR("Failed to configure GPIO interrupt: %d", ret);
		return ret;
	}

	/* Schedule initial state check */
	k_work_reschedule(&data->debounce_work, K_MSEC(cfg->debounce_ms));

	LOG_INF("USB VBUS detect initialized with %zu UDC device(s)",
		cfg->udc_count);

	return 0;
}

/* Helper macro to get device pointer for each phandle in udc list */
#define USB_VBUS_UDC_DEVICE_GET(node_id, prop, idx) \
	DEVICE_DT_GET(DT_PHANDLE_BY_IDX(node_id, prop, idx)),

#define USB_VBUS_DETECT_GPIO_DEFINE(inst)				       \
	static struct usb_vbus_detect_gpio_data				       \
		usb_vbus_detect_gpio_data_##inst;			       \
									       \
	static const struct device *const				       \
		usb_vbus_udc_list_##inst[] = {				       \
		DT_INST_FOREACH_PROP_ELEM(inst, udc, USB_VBUS_UDC_DEVICE_GET)  \
	};								       \
									       \
	static const struct usb_vbus_detect_gpio_config			       \
		usb_vbus_detect_gpio_config_##inst = {			       \
		.vbus_gpio = GPIO_DT_SPEC_INST_GET(inst, vbus_gpios),	       \
		.udc_list = usb_vbus_udc_list_##inst,			       \
		.udc_count = ARRAY_SIZE(usb_vbus_udc_list_##inst),	       \
		.debounce_ms = DT_INST_PROP(inst, debounce_ms),		       \
	};								       \
									       \
	DEVICE_DT_INST_DEFINE(inst, usb_vbus_detect_gpio_init, NULL,	       \
			      &usb_vbus_detect_gpio_data_##inst,	       \
			      &usb_vbus_detect_gpio_config_##inst,	       \
			      POST_KERNEL,				       \
			      CONFIG_USB_VBUS_DETECT_INIT_PRIORITY,	       \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(USB_VBUS_DETECT_GPIO_DEFINE)
