/*
 * Copyright (c) 2026 Adrien RICCIARDI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sample_usbd.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/class/usbd_xbox360_controller.h>
#include <zephyr/usb/usbd.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define GAMEPAD_DT_NODE DT_NODELABEL(gamepad)

K_SEM_DEFINE(ready_sem, 0, 1);

static void gamepad_iface_ready(const struct device *dev, bool ready)
{
	LOG_DBG("Gamepad device %s interface is %s.", dev->name, ready ? "ready" : "not ready");
	k_sem_give(&ready_sem);
}

static struct xbox360_controller_device_ops gamepad_ops = {
	.iface_ready = gamepad_iface_ready
};

static void gamepad_button_sequence(void *report_buffer)
{
	const struct device *gamepad_dev = DEVICE_DT_GET(GAMEPAD_DT_NODE);
	int ret;

	/* Press the button */
	ret = xbox360_controller_submit_report(gamepad_dev, report_buffer);
	if (ret != 0) {
		LOG_ERR("Gamepad device submit report error (%d).", ret);
	}
	k_msleep(2000);

	/* Release the button */
	memset(report_buffer, 0, sizeof(struct xbox360_controller_control_surface));
	ret = xbox360_controller_submit_report(gamepad_dev, report_buffer);
	if (ret != 0) {
		LOG_ERR("Gamepad device submit report error (%d).", ret);
	}
	k_msleep(2000);
}

static void gamepad_joystick_axis_sequence(void *report_buffer, void *joystick_axis)
{
	const struct device *gamepad_dev = DEVICE_DT_GET(GAMEPAD_DT_NODE);
	const int VARIATION_AMOUNT = 16;
	int i, ret;
	/* Use this trick to avoid the -Waddress-of-packed-member warning */
	int16_t *axis = joystick_axis;

	for (i = 0; i >= INT16_MIN; i -= VARIATION_AMOUNT) {
		*axis = (int16_t)i;
		ret = xbox360_controller_submit_report(gamepad_dev, report_buffer);
		if (ret != 0) {
			LOG_ERR("Gamepad device submit report error (%d).", ret);
		}
		k_msleep(4);
	}

	for (i = INT16_MIN; i <= 0; i += VARIATION_AMOUNT) {
		*axis = (int16_t)i;
		ret = xbox360_controller_submit_report(gamepad_dev, report_buffer);
		if (ret != 0) {
			LOG_ERR("Gamepad device submit report error (%d).", ret);
		}
		k_msleep(4);
	}

	for (; i < INT16_MAX; i += VARIATION_AMOUNT) {
		*axis = (int16_t)i;
		ret = xbox360_controller_submit_report(gamepad_dev, report_buffer);
		if (ret != 0) {
			LOG_ERR("Gamepad device submit report error (%d).", ret);
		}
		k_msleep(4);
	}

	for (i = INT16_MAX; i >= 0; i -= VARIATION_AMOUNT) {
		*axis = (int16_t)i;
		ret = xbox360_controller_submit_report(gamepad_dev, report_buffer);
		if (ret != 0) {
			LOG_ERR("Gamepad device submit report error (%d).", ret);
		}
		k_msleep(4);
	}
}

static void gamepad_trigger_sequence(void *report_buffer, uint8_t *trigger)
{
	const struct device *gamepad_dev = DEVICE_DT_GET(GAMEPAD_DT_NODE);
	int i, ret;

	for (i = 0; i < UINT8_MAX; i++) {
		*trigger = (uint8_t)i;
		ret = xbox360_controller_submit_report(gamepad_dev, report_buffer);
		if (ret != 0) {
			LOG_ERR("Gamepad device submit report error (%d).", ret);
		}

		k_msleep(10);
	}

	for (; i >= 0; i--) {
		*trigger = (uint8_t)i;
		ret = xbox360_controller_submit_report(gamepad_dev, report_buffer);
		if (ret != 0) {
			LOG_ERR("Gamepad device submit report error (%d).", ret);
		}

		k_msleep(10);
	}
}

static void gamepad_sequence(void)
{
	UDC_STATIC_BUF_DEFINE(gamepad_report, sizeof(struct xbox360_controller_control_surface));
	struct xbox360_controller_control_surface *ptr_gamepad_report =
		(struct xbox360_controller_control_surface *)gamepad_report;

	memset(gamepad_report, 0, sizeof(struct xbox360_controller_control_surface));

	LOG_INF("Up button...");
	ptr_gamepad_report->button_directional_pad_up = 1;
	gamepad_button_sequence(gamepad_report);

	LOG_INF("Down button...");
	ptr_gamepad_report->button_directional_pad_down = 1;
	gamepad_button_sequence(gamepad_report);

	LOG_INF("Left button...");
	ptr_gamepad_report->button_directional_pad_left = 1;
	gamepad_button_sequence(gamepad_report);

	LOG_INF("Right button...");
	ptr_gamepad_report->button_directional_pad_right = 1;
	gamepad_button_sequence(gamepad_report);

	LOG_INF("Start button...");
	ptr_gamepad_report->button_start = 1;
	gamepad_button_sequence(gamepad_report);

	LOG_INF("Back button...");
	ptr_gamepad_report->button_back = 1;
	gamepad_button_sequence(gamepad_report);

	LOG_INF("L3 button...");
	ptr_gamepad_report->button_l3 = 1;
	gamepad_button_sequence(gamepad_report);

	LOG_INF("R3 button...");
	ptr_gamepad_report->button_r3 = 1;
	gamepad_button_sequence(gamepad_report);

	LOG_INF("Left bottom button...");
	ptr_gamepad_report->button_lb = 1;
	gamepad_button_sequence(gamepad_report);

	LOG_INF("Right bottom button...");
	ptr_gamepad_report->button_rb = 1;
	gamepad_button_sequence(gamepad_report);

	LOG_INF("Xbox button...");
	ptr_gamepad_report->button_xbox = 1;
	gamepad_button_sequence(gamepad_report);

	LOG_INF("'A' button...");
	ptr_gamepad_report->button_a = 1;
	gamepad_button_sequence(gamepad_report);

	LOG_INF("'B' button...");
	ptr_gamepad_report->button_b = 1;
	gamepad_button_sequence(gamepad_report);

	LOG_INF("'X' button...");
	ptr_gamepad_report->button_x = 1;
	gamepad_button_sequence(gamepad_report);

	LOG_INF("'Y' button...");
	ptr_gamepad_report->button_y = 1;
	gamepad_button_sequence(gamepad_report);

	LOG_INF("Left trigger...");
	gamepad_trigger_sequence(gamepad_report, &ptr_gamepad_report->trigger_lt);
	LOG_INF("Right trigger...");
	gamepad_trigger_sequence(gamepad_report, &ptr_gamepad_report->trigger_rt);
	k_msleep(2000);

	LOG_INF("Left joystick X axis...");
	gamepad_joystick_axis_sequence(gamepad_report, &ptr_gamepad_report->left_joystick_x);
	LOG_INF("Left joystick Y axis...");
	gamepad_joystick_axis_sequence(gamepad_report, &ptr_gamepad_report->left_joystick_y);
	LOG_INF("Right joystick X axis...");
	gamepad_joystick_axis_sequence(gamepad_report, &ptr_gamepad_report->right_joystick_x);
	LOG_INF("Right joystick Y axis...");
	gamepad_joystick_axis_sequence(gamepad_report, &ptr_gamepad_report->right_joystick_y);
}

int main(void)
{
	struct usbd_context *sample_usbd;
	const struct device *gamepad_dev = DEVICE_DT_GET(GAMEPAD_DT_NODE);
	int ret;

	if (!device_is_ready(gamepad_dev)) {
		LOG_ERR("The gamepad device is not ready.");
		return -EIO;
	}

	ret = xbox360_controller_device_register(gamepad_dev, &gamepad_ops);
	if (ret != 0) {
		LOG_ERR("Failed to register the gamepad device (%d).", ret);
		return ret;
	}

	sample_usbd = sample_usbd_init_device(NULL);
	if (sample_usbd == NULL) {
		LOG_ERR("Failed to initialize the USB device.");
		return -ENODEV;
	}

	ret = usbd_enable(sample_usbd);
	if (ret != 0) {
		LOG_ERR("Failed to enable the device support (%d).", ret);
		return ret;
	}

	LOG_INF("Waiting for the gamepad interface to become ready...");
	k_sem_take(&ready_sem, K_FOREVER);
	LOG_INF("The gamepad interface is ready.");

	LOG_INF("Waiting 5 seconds before starting the sequence. "
		"Make sure that your tool that displays the gamepad keys is ready.");
	k_msleep(5000);

	while (1) {
		gamepad_sequence();
		k_msleep(2000);
	}

	return 0;
}
