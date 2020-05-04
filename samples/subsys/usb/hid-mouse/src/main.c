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

#define FLAGS_OR_ZERO(node)						\
	COND_CODE_1(DT_PHA_HAS_CELL(node, gpios, flags),		\
		    (DT_GPIO_FLAGS(node, gpios)),			\
		    (0))

#define SW0_NODE DT_ALIAS(sw0)

#if DT_NODE_HAS_STATUS(SW0_NODE, okay)
#define PORT0		DT_GPIO_LABEL(SW0_NODE, gpios)
#define PIN0		DT_GPIO_PIN(SW0_NODE, gpios)
#define PIN0_FLAGS	FLAGS_OR_ZERO(SW0_NODE)
#else
#error "Unsupported board: sw0 devicetree alias is not defined"
#define PORT0		""
#define PIN0		0
#define PIN0_FLAGS	0
#endif

#define SW1_NODE DT_ALIAS(sw1)

#if DT_NODE_HAS_STATUS(SW1_NODE, okay)
#define PORT1		DT_GPIO_LABEL(SW1_NODE, gpios)
#define PIN1		DT_GPIO_PIN(SW1_NODE, gpios)
#define PIN1_FLAGS	FLAGS_OR_ZERO(SW1_NODE)
#endif

#define SW2_NODE DT_ALIAS(sw2)

#if DT_NODE_HAS_STATUS(SW2_NODE, okay)
#define PORT2		DT_GPIO_LABEL(SW2_NODE, gpios)
#define PIN2		DT_GPIO_PIN(SW2_NODE, gpios)
#define PIN2_FLAGS	FLAGS_OR_ZERO(SW2_NODE)
#endif

#define SW3_NODE DT_ALIAS(sw3)

#if DT_NODE_HAS_STATUS(SW3_NODE, okay)
#define PORT3		DT_GPIO_LABEL(SW3_NODE, gpios)
#define PIN3		DT_GPIO_PIN(SW3_NODE, gpios)
#define PIN3_FLAGS	FLAGS_OR_ZERO(SW3_NODE)
#endif

#define LED0_NODE DT_ALIAS(led0)

#if DT_NODE_HAS_STATUS(LED0_NODE, okay)
#define LED_PORT	DT_GPIO_LABEL(LED0_NODE, gpios)
#define LED		DT_GPIO_PIN(LED0_NODE, gpios)
#define LED_FLAGS	DT_GPIO_FLAGS(LED0_NODE, gpios)
#else
#error "Unsupported board: led0 devicetree alias is not defined"
#define LED_PORT	""
#define LED		0
#define LED_FLAGS	0
#endif

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

#if DT_NODE_HAS_STATUS(SW1_NODE, okay)
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

#if DT_NODE_HAS_STATUS(SW2_NODE, okay)
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

#if DT_NODE_HAS_STATUS(SW3_NODE, okay)
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

#if DT_NODE_HAS_STATUS(SW1_NODE, okay)
	if (callbacks_configure(device_get_binding(PORT1), PIN1, PIN1_FLAGS,
				&right_button, &callback[1], &def_val[1])) {
		LOG_ERR("Failed configuring right button callback.");
		return;
	}
#endif

#if DT_NODE_HAS_STATUS(SW2_NODE, okay)
	if (callbacks_configure(device_get_binding(PORT2), PIN2, PIN2_FLAGS,
				&x_move, &callback[2], &def_val[2])) {
		LOG_ERR("Failed configuring X axis movement callback.");
		return;
	}
#endif

#if DT_NODE_HAS_STATUS(SW3_NODE, okay)
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
