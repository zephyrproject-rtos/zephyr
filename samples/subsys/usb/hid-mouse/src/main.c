/*
 * Copyright (c) 2018 qianfan Zhao
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <device.h>
#include <drivers/gpio.h>

#include <usb/usb_device.h>
#include <usb/class/usb_hid.h>

#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(main);

/* change this to use another GPIO port */
#ifdef DT_ALIAS_SW0_GPIOS_CONTROLLER
#define PORT0 DT_ALIAS_SW0_GPIOS_CONTROLLER
#else
#error DT_ALIAS_SW0_GPIOS_CONTROLLER needs to be set
#endif

/* change this to use another GPIO pin */
#ifdef DT_ALIAS_SW0_GPIOS_PIN
#define PIN0     DT_ALIAS_SW0_GPIOS_PIN
#else
#error DT_ALIAS_SW0_GPIOS_PIN needs to be set
#endif

/* The switch pin pull-up/down flags */
#ifdef DT_ALIAS_SW0_GPIOS_FLAGS
#define PIN0_FLAGS DT_ALIAS_SW0_GPIOS_FLAGS
#else
#error DT_ALIAS_SW0_GPIOS_FLAGS needs to be set
#endif

/* If second button exists, use it as right-click. */
#ifdef DT_ALIAS_SW1_GPIOS_PIN
#define PIN1	DT_ALIAS_SW1_GPIOS_PIN
#endif

#ifdef DT_ALIAS_SW1_GPIOS_CONTROLLER
#define PORT1	DT_ALIAS_SW1_GPIOS_CONTROLLER
#endif

#ifdef DT_ALIAS_SW1_GPIOS_FLAGS
#define PIN1_FLAGS DT_ALIAS_SW1_GPIOS_FLAGS
#endif

/* If third button exists, use it as X axis movement. */
#ifdef DT_ALIAS_SW2_GPIOS_PIN
#define PIN2	DT_ALIAS_SW2_GPIOS_PIN
#endif

#ifdef DT_ALIAS_SW2_GPIOS_CONTROLLER
#define PORT2	DT_ALIAS_SW2_GPIOS_CONTROLLER
#endif

#ifdef DT_ALIAS_SW2_GPIOS_FLAGS
#define PIN2_FLAGS DT_ALIAS_SW2_GPIOS_FLAGS
#endif

/* If fourth button exists, use it as Y axis movement. */
#ifdef DT_ALIAS_SW3_GPIOS_PIN
#define PIN3	DT_ALIAS_SW3_GPIOS_PIN
#endif

#ifdef DT_ALIAS_SW3_GPIOS_CONTROLLER
#define PORT3	DT_ALIAS_SW3_GPIOS_CONTROLLER
#endif

#ifdef DT_ALIAS_SW3_GPIOS_FLAGS
#define PIN3_FLAGS DT_ALIAS_SW3_GPIOS_FLAGS
#endif

#define LED_PORT	DT_ALIAS_LED0_GPIOS_CONTROLLER
#define LED		DT_ALIAS_LED0_GPIOS_PIN
#define LED_FLAGS	DT_ALIAS_LED0_GPIOS_FLAGS

static const u8_t hid_report_desc[] = HID_MOUSE_REPORT_DESC(2);

static u8_t def_val[4];
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
	int ret;
	u8_t state = status[MOUSE_BTN_REPORT_POS];

	if (IS_ENABLED(CONFIG_USB_DEVICE_REMOTE_WAKEUP)) {
		if (usb_status == USB_DC_SUSPEND) {
			usb_wakeup_request();
			return;
		}
	}

	ret = gpio_pin_get(gpio, PIN0);
	if (ret < 0) {
		LOG_ERR("Failed to get the state of pin %u, error: %d",
			PIN0, ret);
		return;
	}

	if (def_val[0] != (u8_t)ret) {
		state |= MOUSE_BTN_LEFT;
	} else {
		state &= ~MOUSE_BTN_LEFT;
	}

	if (status[MOUSE_BTN_REPORT_POS] != state) {
		status[MOUSE_BTN_REPORT_POS] = state;
		k_sem_give(&sem);
	}
}

#ifdef DT_ALIAS_SW1_GPIOS_PIN
static void right_button(struct device *gpio, struct gpio_callback *cb,
			 u32_t pins)
{
	int ret;
	u8_t state = status[MOUSE_BTN_REPORT_POS];

	if (IS_ENABLED(CONFIG_USB_DEVICE_REMOTE_WAKEUP)) {
		if (usb_status == USB_DC_SUSPEND) {
			usb_wakeup_request();
			return;
		}
	}

	ret = gpio_pin_get(gpio, PIN1);
	if (ret < 0) {
		LOG_ERR("Failed to get the state of pin %u, error: %d",
			PIN1, ret);
		return;
	}

	if (def_val[1] != (u8_t)ret) {
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

#ifdef DT_ALIAS_SW2_GPIOS_PIN
static void x_move(struct device *gpio, struct gpio_callback *cb, u32_t pins)
{
	int ret;
	u8_t state = status[MOUSE_X_REPORT_POS];

	ret = gpio_pin_get(gpio, PIN2);
	if (ret < 0) {
		LOG_ERR("Failed to get the state of pin %u, error: %d",
			PIN2, ret);
		return;
	}

	if (def_val[2] != (u8_t)ret) {
		state += 10U;
	}

	if (status[MOUSE_X_REPORT_POS] != state) {
		status[MOUSE_X_REPORT_POS] = state;
		k_sem_give(&sem);
	}
}
#endif

#ifdef DT_ALIAS_SW3_GPIOS_PIN
static void y_move(struct device *gpio, struct gpio_callback *cb, u32_t pins)
{
	int ret;
	u8_t state = status[MOUSE_Y_REPORT_POS];

	ret = gpio_pin_get(gpio, PIN3);
	if (ret < 0) {
		LOG_ERR("Failed to get the state of pin %u, error: %d",
			PIN3, ret);
		return;
	}

	if (def_val[3] != (u8_t)ret) {
		state += 10U;
	}

	if (status[MOUSE_Y_REPORT_POS] != state) {
		status[MOUSE_Y_REPORT_POS] = state;
		k_sem_give(&sem);
	}
}
#endif

int callbacks_configure(struct device *gpio, u32_t pin, int flags,
			gpio_callback_handler_t handler,
			struct gpio_callback *callback, u8_t *val)
{
	int ret;

	if (!gpio) {
		LOG_ERR("Could not find PORT");
		return -ENXIO;
	}

	ret = gpio_pin_configure(gpio, pin, GPIO_INPUT | flags);
	if (ret < 0) {
		LOG_ERR("Failed to configure pin %u, error: %d",
			pin, ret);
		return ret;
	}

	ret = gpio_pin_get(gpio, pin);
	if (ret < 0) {
		LOG_ERR("Failed to get the state of pin %u, error: %d",
			pin, ret);
		return ret;
	}

	*val = (u8_t)ret;

	gpio_init_callback(callback, handler, BIT(pin));
	ret = gpio_add_callback(gpio, callback);
	if (ret < 0) {
		LOG_ERR("Failed to add the callback for pin %u, error: %d",
			pin, ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure(gpio, pin, GPIO_INT_EDGE_BOTH);
	if (ret < 0) {
		LOG_ERR("Failed to configure interrupt for pin %u, error: %d",
			pin, ret);
		return ret;
	}

	return 0;
}

void main(void)
{
	int ret;
	u8_t report[4] = { 0x00 };
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

	ret = gpio_pin_configure(led_dev, LED, GPIO_OUTPUT | LED_FLAGS);
	if (ret < 0) {
		LOG_ERR("Failed to configure the LED pin, error: %d", ret);
		return;
	}

	if (callbacks_configure(device_get_binding(PORT0), PIN0, PIN0_FLAGS,
				&left_button, &callback[0], &def_val[0])) {
		LOG_ERR("Failed configuring left button callback.");
		return;
	}

#ifdef DT_ALIAS_SW1_GPIOS_PIN
	if (callbacks_configure(device_get_binding(PORT1), PIN1, PIN1_FLAGS,
				&right_button, &callback[1], &def_val[1])) {
		LOG_ERR("Failed configuring right button callback.");
		return;
	}
#endif

#ifdef DT_ALIAS_SW2_GPIOS_PIN
	if (callbacks_configure(device_get_binding(PORT2), PIN2, PIN2_FLAGS,
				&x_move, &callback[2], &def_val[2])) {
		LOG_ERR("Failed configuring X axis movement callback.");
		return;
	}
#endif

#ifdef DT_ALIAS_SW3_GPIOS_PIN
	if (callbacks_configure(device_get_binding(PORT3), PIN3, PIN3_FLAGS,
				&y_move, &callback[3], &def_val[3])) {
		LOG_ERR("Failed configuring Y axis movement callback.");
		return;
	}
#endif

	usb_hid_register_device(hid_dev,
				hid_report_desc, sizeof(hid_report_desc),
				NULL);

	usb_hid_init(hid_dev);

	ret = usb_enable(status_cb);
	if (ret != 0) {
		LOG_ERR("Failed to enable USB");
		return;
	}

	while (true) {
		k_sem_take(&sem, K_FOREVER);

		report[MOUSE_BTN_REPORT_POS] = status[MOUSE_BTN_REPORT_POS];
		report[MOUSE_X_REPORT_POS] = status[MOUSE_X_REPORT_POS];
		status[MOUSE_X_REPORT_POS] = 0U;
		report[MOUSE_Y_REPORT_POS] = status[MOUSE_Y_REPORT_POS];
		status[MOUSE_Y_REPORT_POS] = 0U;
		hid_int_ep_write(hid_dev, report, sizeof(report), NULL);

		/* Toggle LED on sent report */
		ret = gpio_pin_toggle(led_dev, LED);
		if (ret < 0) {
			LOG_ERR("Failed to toggle the LED pin, error: %d", ret);
		}
	}
}
