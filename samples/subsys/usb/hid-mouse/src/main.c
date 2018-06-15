/*
 * Copyright (c) 2018 qianfan Zhao
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <board.h>
#include <device.h>
#include <gpio.h>

#include <usb/usb_device.h>
#include <usb/class/usb_hid.h>

/* change this to use another GPIO port */
#ifndef SW0_GPIO_CONTROLLER
#ifdef SW0_GPIO_NAME
#define SW0_GPIO_CONTROLLER SW0_GPIO_NAME
#else
#error SW0_GPIO_NAME or SW0_GPIO_CONTROLLER needs to be set in board.h
#endif
#endif
#define PORT SW0_GPIO_CONTROLLER

/* change this to use another GPIO pin */
#ifdef SW0_GPIO_PIN
#define PIN     SW0_GPIO_PIN
#else
#error SW0_GPIO_PIN needs to be set in board.h
#endif

static const u8_t hid_report_desc[] = {
	HID_GI_USAGE_PAGE, USAGE_GEN_DESKTOP,
	/* USAGE_PAGE (Generic Desktop)				05 01 */
	HID_LI_USAGE, USAGE_GEN_DESKTOP_MOUSE,
	/* USAGE (Mouse)					09 02 */
		HID_MI_COLLECTION, COLLECTION_APPLICATION,
		/* COLLECTION (Application)			A1 01 */
		HID_LI_USAGE, USAGE_GEN_DESKTOP_POINTER,
		/* 	USAGE (Pointer)				09 01 */
		HID_MI_COLLECTION, COLLECTION_PHYSICAL,
		/* 	COLLECTION (Physical)			A1 00 */
				HID_GI_USAGE_PAGE, USAGE_GEN_BUTTON,
		/* 		USAGE_PAGE (Button)		05 09 */
				HID_LI_USAGE_MIN(1), 0x01,
		/* 		USAGE_MINIMUM (Button 1)	19 01 */
				HID_LI_USAGE_MAX(1), 0x03,
		/* 		USAGE_MAXIMUM (Button 3)	29 03 */
				HID_GI_LOGICAL_MIN(1), 0,
		/* 		LOGICAL_MINIMUM (0)		15 00 */
				HID_GI_LOGICAL_MAX(1), 1,
		/* 		LOGICAL_MAXIMUM (1)		25 01 */
				HID_GI_REPORT_COUNT, 3,
		/* 		REPORT_COUNT (3)		95 03 */
				HID_GI_REPORT_SIZE, 1,
		/* 		REPORT_SIZE (1)			75 01 */
				HID_MI_INPUT, 0x02,
		/* 		INPUT (Data,Var,Abs)		81 02 */
				HID_GI_REPORT_COUNT, 1,
		/* 		REPORT_COUNT (1)		95 01 */
				HID_GI_REPORT_SIZE, 5,
		/* 		REPORT_SIZE (5)			75 05 */
				HID_MI_INPUT, 0x01,
		/* 		INPUT (Cnst,Ary,Abs)		81 01 */
				HID_GI_USAGE_PAGE, USAGE_GEN_DESKTOP,
		/* 		USAGE_PAGE (Generic Desktop)	05 01 */
				HID_LI_USAGE, USAGE_GEN_DESKTOP_X,
		/* 		USAGE (X)			09 30 */
				HID_LI_USAGE, USAGE_GEN_DESKTOP_Y,
		/* 		USAGE (Y)			09 31 */
				HID_LI_USAGE, USAGE_GEN_DESKTOP_WHEEL,
		/* 		USAGE (Wheel)			09 38 */
				HID_GI_LOGICAL_MIN(1), -127,
		/* 		LOGICAL_MINIMUM (-127)		15 81 */
				HID_GI_LOGICAL_MAX(1), 127,
		/* 		LOGICAL_MAXIMUM (127)		25 7F */
				HID_GI_REPORT_SIZE, 8,
		/* 		REPORT_SIZE (8)			75 08 */
				HID_GI_REPORT_COUNT, 3,
		/* 		REPORT_COUNT (3)		95 03 */
				HID_MI_INPUT, 0x06,
		/* 		INPUT (Data,Var,Rel)		81 06 */
				HID_MI_COLLECTION_END,
		/* 		END_COLLECTION			C0    */
			HID_MI_COLLECTION_END,
		/* 	END_COLLECTION				C0    */
};

static int get_report_cb(struct usb_setup_packet *setup, s32_t *len,
			 u8_t **data)
{
	*len = sizeof(hid_report_desc);
	*data = (u8_t *)hid_report_desc;

	return 0;
}

static struct hid_ops ops = {
	.get_report = get_report_cb,
};

static int def_val;
static volatile int status;
static K_SEM_DEFINE(sem, 0, 1);	/* starts off "not available" */

#define MOUSE_BTN_LEFT			BIT(0)
#define MOUSE_BTN_RIGHT			BIT(1)
#define MOUSE_BTN_MIDDLE		BIT(2)

void button_pressed(struct device *gpio, struct gpio_callback *cb,
		    u32_t pins)
{
	int cur_val, state = status;

	gpio_pin_read(gpio, PIN, &cur_val);
	if (def_val != cur_val) {
		state |= MOUSE_BTN_LEFT;
	} else {
		state &= ~MOUSE_BTN_LEFT;
	}

	if (state != status) {
		status = state;
		k_sem_give(&sem);
	}
}

void main(void)
{
	struct device *gpio;
	struct gpio_callback gpio_cb;
	u8_t report[4] = { 0x00 };

	gpio = device_get_binding(PORT);
	if (!gpio) {
		printk("error\n");
		return;
	}

	gpio_pin_configure(gpio, PIN,
			   GPIO_DIR_IN | GPIO_INT |
			   GPIO_INT_EDGE | GPIO_INT_DOUBLE_EDGE);

	gpio_pin_read(gpio, PIN, &def_val);
	gpio_init_callback(&gpio_cb, button_pressed, BIT(PIN));

	gpio_add_callback(gpio, &gpio_cb);
	gpio_pin_enable_callback(gpio, PIN);

	usb_hid_register_device(hid_report_desc, sizeof(hid_report_desc), &ops);
	usb_hid_init();

	while (true) {
		k_sem_take(&sem, K_FOREVER);

		report[0] = status;
		hid_int_ep_write(report, sizeof(report), NULL);
	}
}
