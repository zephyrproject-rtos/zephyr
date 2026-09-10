/*
 * Copyright (c) 2018 qianfan Zhao
 * Copyright (c) 2018, 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/input/input.h>
#include <zephyr/sys/util.h>

#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/class/usb_hid.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const uint8_t hid_report_desc[] = HID_MOUSE_REPORT_DESC(2);
static enum usb_dc_status_code usb_status;

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
static K_SEM_DEFINE(ep_write_sem, 0, 1);

static inline void status_cb(enum usb_dc_status_code status, const uint8_t *param)
{
	usb_status = status;
}

static ALWAYS_INLINE void rwup_if_suspended(void)
{
	if (IS_ENABLED(CONFIG_USB_DEVICE_REMOTE_WAKEUP)) {
		if (usb_status == USB_DC_SUSPEND) {
			usb_wakeup_request();
			return;
		}
	}
}

static void input_cb(struct input_event *evt, void *user_data)
{
	static uint8_t tmp[MOUSE_REPORT_COUNT];

	ARG_UNUSED(user_data);

	switch (evt->code) {
	case INPUT_KEY_0:
		rwup_if_suspended();
		WRITE_BIT(tmp[MOUSE_BTN_REPORT_IDX], MOUSE_BTN_LEFT, evt->value);
		break;
	case INPUT_KEY_1:
		rwup_if_suspended();
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

static void int_in_ready_cb(const struct device *dev)
{
	ARG_UNUSED(dev);
	k_sem_give(&ep_write_sem);
}

static const struct hid_ops ops = {
	.int_in_ready = int_in_ready_cb,
};

int main(void)
{
	const struct device *hid_dev;
	int ret;

	if (!gpio_is_ready_dt(&led0)) {
		LOG_ERR("LED device %s is not ready", led0.port->name);
		return 0;
	}

	hid_dev = device_get_binding("HID_0");
	if (hid_dev == NULL) {
		LOG_ERR("Cannot get USB HID Device");
		return 0;
	}

	ret = gpio_pin_configure_dt(&led0, GPIO_OUTPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure the LED pin, error: %d", ret);
		return 0;
	}

	usb_hid_register_device(hid_dev,
				hid_report_desc, sizeof(hid_report_desc),
				&ops);

	usb_hid_init(hid_dev);

	ret = usb_enable(status_cb);
	if (ret != 0) {
		LOG_ERR("Failed to enable USB");
		return 0;
	}

	while (true) {
		uint8_t __aligned(sizeof(void *)) report[MOUSE_REPORT_COUNT];

		k_msgq_get(&mouse_msgq, &report, K_FOREVER);

		ret = hid_int_ep_write(hid_dev, report, MOUSE_REPORT_COUNT, NULL);
		if (ret) {
			LOG_ERR("HID write error, %d", ret);
		} else {
			k_sem_take(&ep_write_sem, K_FOREVER);
			/* Toggle LED on sent report */
			(void)gpio_pin_toggle(led0.port, led0.pin);
		}
	}
	return 0;
}
