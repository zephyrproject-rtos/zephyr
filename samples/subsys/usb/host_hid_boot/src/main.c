/*
 * Copyright (c) 2026 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/input/input.h>
#include <zephyr/shell/shell_uart.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/usbh.h>

#include "keys.h"

#define INT32_DECIMAL_STRING_SIZE 12

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

USBH_CONTROLLER_DEFINE(uhs_ctx, DEVICE_DT_GET(DT_NODELABEL(zephyr_uhc0)));

static const struct device *usbh_hid_dev;
static const struct shell *sh;

#ifdef CONFIG_SAMPLE_COLORED_OUTPUT
static const enum shell_vt100_color highlight_color = SHELL_VT100_COLOR_GREEN;
#else
static const enum shell_vt100_color highlight_color = SHELL_VT100_COLOR_DEFAULT;
#endif

static void print_highlighted(const char *s1, const char *s2_highlight, const char *s3,
			      const char *s4_highlight)
{
	shell_fprintf(sh, SHELL_NORMAL, "%s", s1);
	shell_fprintf(sh, highlight_color, "%s", s2_highlight);
	shell_fprintf(sh, SHELL_NORMAL, "%s", s3);
	shell_fprintf(sh, highlight_color, "%s\n", s4_highlight);
}

static const char *input_to_name(uint16_t key_code)
{
	if (key_code >= ARRAY_SIZE(input_to_name_map)) {
		return 0;
	}
	return input_to_name_map[key_code];
}

static void mouse_evt_handler(struct input_event *evt)
{
	const char *btn_name, *axis;
	char delta[INT32_DECIMAL_STRING_SIZE];

	switch (evt->code) {
	case INPUT_BTN_LEFT:
	case INPUT_BTN_MIDDLE:
	case INPUT_BTN_RIGHT:
		btn_name = input_to_name(evt->code);

		print_highlighted("", btn_name, " button ", evt->value ? "pressed" : "let go");
		break;
	case INPUT_REL_X:
	case INPUT_REL_Y: {
		axis = evt->code == INPUT_REL_X ? "X" : "Y";
		snprintk(delta, sizeof(delta), "%d", evt->value);

		print_highlighted("Mouse moved in ", axis, " axis by ", delta);
		break;
	}
	}
}

static void kbd_evt_handler(struct input_event *evt)
{
	const char *name = input_to_name(evt->code);

	if (name) {
		print_highlighted("Key ", name, " was ", evt->value ? "pressed" : "released");
	}
}

static void input_cb(struct input_event *evt, void *user_data)
{
	ARG_UNUSED(user_data);

	/* Check if the event is from the desired device */
	if (evt->dev != usbh_hid_dev) {
		return;
	}

	if (evt->type == INPUT_EV_REL || evt->code == INPUT_BTN_LEFT ||
	    evt->code == INPUT_BTN_MIDDLE || evt->code == INPUT_BTN_RIGHT) {
		mouse_evt_handler(evt);
	} else {
		kbd_evt_handler(evt);
	}
}
INPUT_CALLBACK_DEFINE(NULL, input_cb, NULL);

static void shell_bypass_cb(const struct shell *sh, uint8_t *data, size_t len, void *user_data)
{
	ARG_UNUSED(sh);
	ARG_UNUSED(data);
	ARG_UNUSED(len);
	ARG_UNUSED(user_data);
}

int main(void)
{
	int err;

	sh = shell_backend_uart_get_ptr();
	shell_set_bypass(sh, shell_bypass_cb, NULL);

	/*
	 * HID Boot devices are named:
	 * usbh_hid_boot_0,
	 * usbh_hid_boot_1,
	 * ...,
	 * usbh_hid_boot_(CONFIG_USBH_HID_BOOT_INSTANCES_COUNT-1)
	 */
	usbh_hid_dev = device_get_binding("usbh_hid_boot_0");

	err = usbh_init(&uhs_ctx);
	if (err == -EALREADY) {
		LOG_ERR("host: USB host already initialized");
	} else if (err) {
		LOG_ERR("host: Failed to initialize %d", err);
	} else {
		LOG_INF("host: USB host initialized");
	}

	return 0;
}
