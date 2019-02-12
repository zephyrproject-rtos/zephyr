/*
 * Copyright (c) 2018 qianfan Zhao
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <device.h>
#include <gpio.h>

#include <usb/usb_device.h>
#include <usb/class/usb_hid.h>

#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(main);

/* change this to use another GPIO port */
#ifdef SW0_GPIO_CONTROLLER
#define PORT0 SW0_GPIO_CONTROLLER
#else
#error SW0_GPIO_CONTROLLER needs to be set
#endif

/* change this to use another GPIO pin */
#ifdef SW0_GPIO_PIN
#define PIN0     SW0_GPIO_PIN
#else
#error SW0_GPIO_PIN needs to be set
#endif

/* The switch pin pull-up/down flags */
#ifdef SW0_GPIO_FLAGS
#define PIN0_FLAGS SW0_GPIO_FLAGS
#else
#error SW0_GPIO_FLAGS needs to be set
#endif

/* If second button exists, use it as right-click. */
#ifdef SW1_GPIO_PIN
#define PIN1	SW1_GPIO_PIN
#endif

#ifdef SW1_GPIO_CONTROLLER
#define PORT1	SW1_GPIO_CONTROLLER
#endif

#ifdef SW1_GPIO_FLAGS
#define PIN1_FLAGS SW1_GPIO_FLAGS
#endif

/* If third button exists, use it as X axis movement. */
#ifdef SW2_GPIO_PIN
#define PIN2	SW2_GPIO_PIN
#endif

#ifdef SW2_GPIO_CONTROLLER
#define PORT2	SW2_GPIO_CONTROLLER
#endif

#ifdef SW2_GPIO_FLAGS
#define PIN2_FLAGS SW2_GPIO_FLAGS
#endif

/* If fourth button exists, use it as Y axis movement. */
#ifdef SW3_GPIO_PIN
#define PIN3	SW3_GPIO_PIN
#endif

#ifdef SW3_GPIO_CONTROLLER
#define PORT3	SW3_GPIO_CONTROLLER
#endif

#ifdef SW3_GPIO_FLAGS
#define PIN3_FLAGS SW3_GPIO_FLAGS
#endif

#define LED_PORT	LED0_GPIO_CONTROLLER
#define LED		LED0_GPIO_PIN

static const u8_t hid_report_desc[] = HID_MOUSE_REPORT_DESC(2);

static u32_t def_val[4];
static volatile u8_t status[4];
static K_SEM_DEFINE(sem, 0, 1);	/* starts off "not available" */
static struct gpio_callback callback[4];
static enum usb_dc_status_code usb_status;

#define MOUSE_BTN_REPORT_POS	0
#define MOUSE_X_REPORT_POS	1
#define MOUSE_Y_REPORT_POS	2

#define MOUSE_BTN_LEFT		BIT(0)
#define MOUSE_BTN_RIGHT		BIT(1)
#define MOUSE_BTN_MIDDLE	BIT(2)



static void status_cb(enum usb_dc_status_code status, const u8_t *param)
{
	usb_status = status;
}

static void left_button(struct device *gpio, struct gpio_callback *cb,
			u32_t pins)
{
	u32_t cur_val;
	u8_t state = status[MOUSE_BTN_REPORT_POS];

	if (IS_ENABLED(CONFIG_USB_DEVICE_REMOTE_WAKEUP)) {
		if (usb_status == USB_DC_SUSPEND) {
			usb_wakeup_request();
			return;
		}
	}

	gpio_pin_read(gpio, PIN0, &cur_val);
	if (def_val[0] != cur_val) {
		state |= MOUSE_BTN_LEFT;
	} else {
		state &= ~MOUSE_BTN_LEFT;
	}

	if (status[MOUSE_BTN_REPORT_POS] != state) {
		status[MOUSE_BTN_REPORT_POS] = state;
		k_sem_give(&sem);
	}
}

#ifdef SW1_GPIO_PIN
static void right_button(struct device *gpio, struct gpio_callback *cb,
			 u32_t pins)
{
	u32_t cur_val;
	u8_t state = status[MOUSE_BTN_REPORT_POS];

	if (IS_ENABLED(CONFIG_USB_DEVICE_REMOTE_WAKEUP)) {
		if (usb_status == USB_DC_SUSPEND) {
			usb_wakeup_request();
			return;
		}
	}

	gpio_pin_read(gpio, PIN1, &cur_val);
	if (def_val[0] != cur_val) {
		state |= MOUSE_BTN_RIGHT;
	} else {
		state &= ~MOUSE_BTN_RIGHT;
	}

	if (status[MOUSE_BTN_REPORT_POS] != state) {
		status[MOUSE_BTN_REPORT_POS] = state;
		k_sem_give(&sem);
	}
}
#endif

#ifdef SW2_GPIO_PIN
static void x_move(struct device *gpio, struct gpio_callback *cb, u32_t pins)
{
	u32_t cur_val;
	u8_t state = status[MOUSE_X_REPORT_POS];

	gpio_pin_read(gpio, PIN2, &cur_val);

	if (def_val[2] != cur_val) {
		state += 10U;
	}

	if (status[MOUSE_X_REPORT_POS] != state) {
		status[MOUSE_X_REPORT_POS] = state;
		k_sem_give(&sem);
	}
}
#endif

#ifdef SW3_GPIO_PIN
static void y_move(struct device *gpio, struct gpio_callback *cb, u32_t pins)
{
	u32_t cur_val;
	u8_t state = status[MOUSE_Y_REPORT_POS];

	gpio_pin_read(gpio, PIN3, &cur_val);

	if (def_val[3] != cur_val) {
		state += 10U;
	}

	if (status[MOUSE_Y_REPORT_POS] != state) {
		status[MOUSE_Y_REPORT_POS] = state;
		k_sem_give(&sem);
	}
}
#endif

int callbacks_configure(struct device *gpio, u32_t pin, int flags,
			void (*handler)(struct device*, struct gpio_callback*,
			u32_t), struct gpio_callback *callback, u32_t *val)
{
	if (!gpio) {
		LOG_ERR("Could not find PORT");
		return -ENXIO;
	}
	gpio_pin_configure(gpio, pin,
			   GPIO_DIR_IN | GPIO_INT |
			   GPIO_INT_EDGE | GPIO_INT_DOUBLE_EDGE |
			   flags);
	gpio_pin_read(gpio, pin, val);
	gpio_init_callback(callback, handler, BIT(pin));
	gpio_add_callback(gpio, callback);
	gpio_pin_enable_callback(gpio, pin);
	return 0;
}

void main(void)
{
	u8_t report[4] = { 0x00 };
	u8_t toggle = 0U;
	struct device *led_dev, *hid_dev;

	led_dev = device_get_binding(LED_PORT);
	if (led_dev == NULL) {
		LOG_ERR("Cannot get LED");
		return;
	}

	hid_dev = device_get_binding("HID_0");
	if (hid_dev == NULL) {
		LOG_ERR("Cannot get USB HID Device");
		return;
	}

	gpio_pin_configure(led_dev, LED, GPIO_DIR_OUT);

	if (callbacks_configure(device_get_binding(PORT0), PIN0, PIN0_FLAGS,
				&left_button, &callback[0], &def_val[0])) {
		LOG_ERR("Failed configuring left button callback.");
		return;
	}

#ifdef SW1_GPIO_PIN
	if (callbacks_configure(device_get_binding(PORT1), PIN1, PIN1_FLAGS,
				&right_button, &callback[1], &def_val[1])) {
		LOG_ERR("Failed configuring right button callback.");
		return;
	}
#endif

#ifdef SW2_GPIO_PIN
	if (callbacks_configure(device_get_binding(PORT2), PIN2, PIN2_FLAGS,
				&x_move, &callback[2], &def_val[2])) {
		LOG_ERR("Failed configuring X axis movement callback.");
		return;
	}
#endif

#ifdef SW3_GPIO_PIN
	if (callbacks_configure(device_get_binding(PORT3), PIN3, PIN3_FLAGS,
				&y_move, &callback[3], &def_val[3])) {
		LOG_ERR("Failed configuring Y axis movement callback.");
		return;
	}
#endif

	static const struct hid_ops ops = {
			.status_cb = status_cb
	};
	usb_hid_register_device(hid_dev,
				hid_report_desc, sizeof(hid_report_desc),
				&ops);
	usb_hid_init(hid_dev);

	while (true) {
		k_sem_take(&sem, K_FOREVER);

		report[MOUSE_BTN_REPORT_POS] = status[MOUSE_BTN_REPORT_POS];
		report[MOUSE_X_REPORT_POS] = status[MOUSE_X_REPORT_POS];
		status[MOUSE_X_REPORT_POS] = 0U;
		report[MOUSE_Y_REPORT_POS] = status[MOUSE_Y_REPORT_POS];
		status[MOUSE_Y_REPORT_POS] = 0U;
		hid_int_ep_write(hid_dev, report, sizeof(report), NULL);

		/* Toggle LED on sent report */
		gpio_pin_write(led_dev, LED, toggle);
		toggle = !toggle;
	}
}
