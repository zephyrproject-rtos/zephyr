/*
 * Copyright (c) 2018 qianfan Zhao
 * Copyright (c) 2018, 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sample_usbd.h>

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/input/input.h>
#include <zephyr/sys/util.h>

#include <zephyr/usb/usbd.h>
#include <zephyr/usb/class/usbd_hid.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const uint8_t hid_report_desc[] = HID_MOUSE_REPORT_DESC(2);

#define MOUSE_BTN_LEFT		0
#define MOUSE_BTN_RIGHT		1

enum mouse_report_idx {
	MOUSE_BTN_REPORT_IDX = 0,
	MOUSE_X_REPORT_IDX = 1,
	MOUSE_Y_REPORT_IDX = 2,
	MOUSE_WHEEL_REPORT_IDX = 3,
	MOUSE_REPORT_COUNT = 4,
};

K_MSGQ_DEFINE(mouse_msgq, MOUSE_REPORT_COUNT, 2, 1);
static bool mouse_ready;

static void input_cb(struct input_event *evt, void *user_data)
{
	static uint8_t tmp[MOUSE_REPORT_COUNT];

	ARG_UNUSED(user_data);

	switch (evt->code) {
	case INPUT_KEY_0:
		WRITE_BIT(tmp[MOUSE_BTN_REPORT_IDX], MOUSE_BTN_LEFT, evt->value);
		break;
	case INPUT_KEY_1:
		WRITE_BIT(tmp[MOUSE_BTN_REPORT_IDX], MOUSE_BTN_RIGHT, evt->value);
		break;
	case INPUT_KEY_2:
		if (evt->value) {
			tmp[MOUSE_X_REPORT_IDX] += 10U;
		}

		break;
	case INPUT_KEY_3:
		if (evt->value) {
			tmp[MOUSE_Y_REPORT_IDX] += 10U;
		}

		break;
	default:
		LOG_INF("Unrecognized input code %u value %d",
			evt->code, evt->value);
		return;
	}

	if (k_msgq_put(&mouse_msgq, tmp, K_NO_WAIT) != 0) {
		LOG_ERR("Failed to put new input event");
	}

	tmp[MOUSE_X_REPORT_IDX] = 0U;
	tmp[MOUSE_Y_REPORT_IDX] = 0U;

}

INPUT_CALLBACK_DEFINE(NULL, input_cb, NULL);

static void mouse_iface_ready(const struct device *dev, const bool ready)
{
	LOG_INF("HID device %s interface is %s",
		dev->name, ready ? "ready" : "not ready");
	mouse_ready = ready;
}

static int mouse_get_report(const struct device *dev,
			 const uint8_t type, const uint8_t id, const uint16_t len,
			 uint8_t *const buf)
{
	LOG_WRN("Get Report not implemented, Type %u ID %u", type, id);

	return 0;
}

struct hid_device_ops mouse_ops = {
	.iface_ready = mouse_iface_ready,
	.get_report = mouse_get_report,
};

int main(void)
{
	struct usbd_context *sample_usbd;
	const struct device *hid_dev;
	int ret;

	if (!gpio_is_ready_dt(&led0)) {
		LOG_ERR("LED device %s is not ready", led0.port->name);
		return 0;
	}

	ret = gpio_pin_configure_dt(&led0, GPIO_OUTPUT);
	if (ret != 0) {
		LOG_ERR("Failed to configure the LED pin, error: %d", ret);
		return 0;
	}

	hid_dev = DEVICE_DT_GET_ONE(zephyr_hid_device);
	if (!device_is_ready(hid_dev)) {
		LOG_ERR("HID Device is not ready");
		return -EIO;
	}

	ret = hid_device_register(hid_dev,
				  hid_report_desc, sizeof(hid_report_desc),
				  &mouse_ops);
	if (ret != 0) {
		LOG_ERR("Failed to register HID Device, %d", ret);
		return ret;
	}

	sample_usbd = sample_usbd_init_device(NULL);
	if (sample_usbd == NULL) {
		LOG_ERR("Failed to initialize USB device");
		return -ENODEV;
	}

	ret = usbd_enable(sample_usbd);
	if (ret != 0) {
		LOG_ERR("Failed to enable device support");
		return ret;
	}

	LOG_DBG("USB device support enabled");

	while (true) {
		UDC_STATIC_BUF_DEFINE(report, MOUSE_REPORT_COUNT);

		k_msgq_get(&mouse_msgq, &report, K_FOREVER);

		if (!mouse_ready) {
			LOG_INF("USB HID device is not ready");
			continue;
		}

		ret = hid_device_submit_report(hid_dev, MOUSE_REPORT_COUNT, report);
		if (ret) {
			LOG_ERR("HID submit report error, %d", ret);
		} else {
			/* Toggle LED on sent report */
			(void)gpio_pin_toggle(led0.port, led0.pin);
		}
	}

	return 0;
}
