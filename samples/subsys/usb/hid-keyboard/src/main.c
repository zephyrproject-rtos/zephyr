/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
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

static const uint8_t __aligned(32) hid_report_desc[] = HID_KEYBOARD_REPORT_DESC();

enum kb_leds_idx {
	KB_LED_NUMLOCK = 0,
	KB_LED_CAPSLOCK,
	KB_LED_SCROLLLOCK,
	KB_LED_COUNT,
};

static const struct gpio_dt_spec kb_leds[KB_LED_COUNT] = {
	GPIO_DT_SPEC_GET_OR(DT_ALIAS(led0), gpios, {0}),
	GPIO_DT_SPEC_GET_OR(DT_ALIAS(led1), gpios, {0}),
	GPIO_DT_SPEC_GET_OR(DT_ALIAS(led2), gpios, {0}),
};

enum kb_report_idx {
	KB_MOD_KEY = 0,
	KB_RESERVED,
	KB_KEY_CODE1,
	KB_KEY_CODE2,
	KB_KEY_CODE3,
	KB_KEY_CODE4,
	KB_KEY_CODE5,
	KB_KEY_CODE6,
	KB_REPORT_COUNT,
};

static uint8_t report[KB_REPORT_COUNT];
static K_SEM_DEFINE(report_sem, 0, 1);
static uint32_t kb_duration;
static bool prankme;

static void input_cb(struct input_event *evt)
{
	uint8_t tmp[KB_REPORT_COUNT];

	(void)memcpy(tmp, report, sizeof(tmp));

	switch (evt->code) {
	case INPUT_KEY_0:
		tmp[KB_KEY_CODE1] = 0;
		if (evt->value) {
			tmp[KB_KEY_CODE1] = HID_KEY_NUMLOCK;
		}

		break;
	case INPUT_KEY_1:
		tmp[KB_KEY_CODE1] = 0;
		if (evt->value) {
			tmp[KB_KEY_CODE1] = HID_KEY_CAPSLOCK;
		}

		break;
	case INPUT_KEY_2:
		tmp[KB_KEY_CODE1] = 0;
		if (evt->value) {
			tmp[KB_KEY_CODE1] = HID_KEY_SCROLLLOCK;
		}

		break;
	case INPUT_KEY_3:
		tmp[KB_KEY_CODE1] = 0;
		if (evt->value) {
			tmp[KB_KEY_CODE1] = HID_KEY_ENTER;
			prankme = !prankme;
		}

		break;
	default:
		LOG_INF("Unrecognized input code %u value %d",
			evt->code, evt->value);
		return;
	}

	if (memcmp(tmp, report, sizeof(tmp))) {
		memcpy(report, tmp, sizeof(report));
		k_sem_give(&report_sem);
	}
}

INPUT_CALLBACK_DEFINE(NULL, input_cb);

static void do_prankme(void)
{
	static uint32_t cnt;

	switch (cnt & 3) {
	case 0:
		report[KB_KEY_CODE1] = HID_KEY_NUMLOCK;
		break;
	case 1:
		report[KB_KEY_CODE1] = HID_KEY_CAPSLOCK;
		break;
	case 2:
		report[KB_KEY_CODE1] = HID_KEY_SCROLLLOCK;
		break;
	case 3:
		report[KB_KEY_CODE1] = 0;
		break;
	}

	cnt ++;
}

static void kb_iface_ready(const struct device *dev, const bool ready)
{
	LOG_INF("HID device %p interface is %s", dev, ready ? "ready" : "not ready");
}

static int kb_get_report(const struct device *dev,
			 const uint8_t type, const uint8_t id, const uint16_t len,
			 uint8_t *const buf)
{
	return 0;
}

static int kb_set_report(const struct device *dev,
			 const uint8_t type, const uint8_t id, const uint16_t len,
			 const uint8_t *const buf)
{
	for (unsigned int i = 0; i < ARRAY_SIZE(kb_leds); i++) {
		if (kb_leds[i].port == NULL) {
			continue;
		}

		(void)gpio_pin_set_dt(&kb_leds[i], buf[0] & BIT(i));
	}

	return 0;
}

/* Idle duration is stored but not used to calculate idle reports. */
static void kb_set_idle(const struct device *dev,
			const uint8_t id, const uint32_t duration)
{
	LOG_INF("Set Idle %u to %u", id, duration);
	kb_duration = duration;
}

static uint32_t kb_get_idle(const struct device *dev, const uint8_t id)
{
	LOG_INF("Get Idle %u to %u", id, kb_duration);
	return kb_duration;
}

static void kb_set_protocol(const struct device *dev, const uint8_t proto)
{
	LOG_INF("Protocol changed to %s",
		proto == 0U ? "Boot Protocol" : "Report Protocol");
}

static void kb_output_report(const struct device *dev, const uint16_t len,
			     const uint8_t *const buf)
{
	LOG_HEXDUMP_DBG(buf, len, "o.r.");
	kb_set_report(dev, 2U, 0U, len, buf);
}

struct hid_device_ops kb_ops = {
	.iface_ready = kb_iface_ready,
	.get_report = kb_get_report,
	.set_report = kb_set_report,
	.set_idle = kb_set_idle,
	.get_idle = kb_get_idle,
	.set_protocol = kb_set_protocol,
	.output_report = kb_output_report,
};

int main(void)
{
	struct usbd_contex *sample_usbd;
	const struct device *hid_dev;
	int ret;

	for (unsigned int i = 0; i < ARRAY_SIZE(kb_leds); i++) {
		if (kb_leds[i].port == NULL) {
			continue;
		}

		if (!gpio_is_ready_dt(&kb_leds[i])) {
			LOG_ERR("LED device %s is not ready", kb_leds[i].port->name);
			return -EIO;
		}

		ret = gpio_pin_configure_dt(&kb_leds[i], GPIO_OUTPUT_INACTIVE);
		if (ret != 0) {
			LOG_ERR("Failed to configure the LED pin, %d", ret);
			return -EIO;
		}
	}

	hid_dev = DEVICE_DT_GET_ONE(zephyr_hid_device);
	if (!device_is_ready(hid_dev)) {
		LOG_ERR("HID Device is not ready");
		return -EIO;
	}

	ret = hid_device_register(hid_dev,
				  hid_report_desc, sizeof(hid_report_desc),
				  &kb_ops);
	if (ret != 0) {
		LOG_ERR("Failed to register HID Device, %d", ret);
		return ret;
	}

	sample_usbd = sample_usbd_init_device();
	if (sample_usbd == NULL) {
		LOG_ERR("Failed to initialize USB device");
		return -ENODEV;
	}

	ret = usbd_enable(sample_usbd);
	if (ret) {
		LOG_ERR("Failed to enable device support");
		return ret;
	}

	while (true) {
		if (prankme) {
			k_msleep(10);
			do_prankme();
		} else {
			k_sem_take(&report_sem, K_FOREVER);
		}

		ret = hid_device_submit_report(hid_dev, sizeof(report), report, true);
		if (ret) {
			LOG_ERR("HID submit report error, %d", ret);
		}
	}

	return 0;
}
