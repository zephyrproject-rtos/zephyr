/*
 * Copyright (c) 2018 qianfan Zhao
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/class/usb_hid.h>

#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(main);

/*
 * Devicetree node identifiers for the buttons and LED this sample
 * supports.
 */
#define SW0_NODE DT_ALIAS(sw0)
#define SW1_NODE DT_ALIAS(sw1)
#define SW2_NODE DT_ALIAS(sw2)
#define SW3_NODE DT_ALIAS(sw3)
#define LED0_NODE DT_ALIAS(led0)

/*
 * Button sw0 and LED led0 are required.
 */
#if !DT_NODE_EXISTS(SW0_NODE)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif

#if !DT_NODE_EXISTS(LED0_NODE)
#error "Unsupported board: led0 devicetree alias is not defined"
#endif

/*
 * Helper macro for initializing a gpio_dt_spec from the devicetree
 * with fallback values when the nodes are missing.
 */
#define GPIO_SPEC(node_id) GPIO_DT_SPEC_GET_OR(node_id, gpios, {0})

/*
 * Create gpio_dt_spec structures from the devicetree.
 */
static const struct gpio_dt_spec sw0 = GPIO_SPEC(SW0_NODE),
	sw1 = GPIO_SPEC(SW1_NODE),
	sw2 = GPIO_SPEC(SW2_NODE),
	sw3 = GPIO_SPEC(SW3_NODE),
	led0 = GPIO_SPEC(LED0_NODE);

static const uint8_t hid_report_desc[] = HID_MOUSE_REPORT_DESC(2);

static uint8_t def_val[4];
static volatile uint8_t status[4];
static K_SEM_DEFINE(sem, 0, 1);	/* starts off "not available" */
static struct gpio_callback callback[4];
static enum usb_dc_status_code usb_status;

#define MOUSE_BTN_REPORT_POS	0
#define MOUSE_X_REPORT_POS	1
#define MOUSE_Y_REPORT_POS	2

#define MOUSE_BTN_LEFT		BIT(0)
#define MOUSE_BTN_RIGHT		BIT(1)
#define MOUSE_BTN_MIDDLE	BIT(2)

static void status_cb(enum usb_dc_status_code status, const uint8_t *param)
{
	usb_status = status;
}

static void left_button(const struct device *gpio, struct gpio_callback *cb,
			uint32_t pins)
{
	int ret;
	uint8_t state = status[MOUSE_BTN_REPORT_POS];

	if (IS_ENABLED(CONFIG_USB_DEVICE_REMOTE_WAKEUP)) {
		if (usb_status == USB_DC_SUSPEND) {
			usb_wakeup_request();
			return;
		}
	}

	ret = gpio_pin_get(gpio, sw0.pin);
	if (ret < 0) {
		LOG_ERR("Failed to get the state of port %s pin %u, error: %d",
			gpio->name, sw0.pin, ret);
		return;
	}

	if (def_val[0] != (uint8_t)ret) {
		state |= MOUSE_BTN_LEFT;
	} else {
		state &= ~MOUSE_BTN_LEFT;
	}

	if (status[MOUSE_BTN_REPORT_POS] != state) {
		status[MOUSE_BTN_REPORT_POS] = state;
		k_sem_give(&sem);
	}
}

static void right_button(const struct device *gpio, struct gpio_callback *cb,
			 uint32_t pins)
{
	int ret;
	uint8_t state = status[MOUSE_BTN_REPORT_POS];

	if (IS_ENABLED(CONFIG_USB_DEVICE_REMOTE_WAKEUP)) {
		if (usb_status == USB_DC_SUSPEND) {
			usb_wakeup_request();
			return;
		}
	}

	ret = gpio_pin_get(gpio, sw1.pin);
	if (ret < 0) {
		LOG_ERR("Failed to get the state of port %s pin %u, error: %d",
			gpio->name, sw1.pin, ret);
		return;
	}

	if (def_val[1] != (uint8_t)ret) {
		state |= MOUSE_BTN_RIGHT;
	} else {
		state &= ~MOUSE_BTN_RIGHT;
	}

	if (status[MOUSE_BTN_REPORT_POS] != state) {
		status[MOUSE_BTN_REPORT_POS] = state;
		k_sem_give(&sem);
	}
}

static void x_move(const struct device *gpio, struct gpio_callback *cb,
		   uint32_t pins)
{
	int ret;
	uint8_t state = status[MOUSE_X_REPORT_POS];

	ret = gpio_pin_get(gpio, sw2.pin);
	if (ret < 0) {
		LOG_ERR("Failed to get the state of port %s pin %u, error: %d",
			gpio->name, sw2.pin, ret);
		return;
	}

	if (def_val[2] != (uint8_t)ret) {
		state += 10U;
	}

	if (status[MOUSE_X_REPORT_POS] != state) {
		status[MOUSE_X_REPORT_POS] = state;
		k_sem_give(&sem);
	}
}

static void y_move(const struct device *gpio, struct gpio_callback *cb,
		   uint32_t pins)
{
	int ret;
	uint8_t state = status[MOUSE_Y_REPORT_POS];

	ret = gpio_pin_get(gpio, sw3.pin);
	if (ret < 0) {
		LOG_ERR("Failed to get the state of port %s pin %u, error: %d",
			gpio->name, sw3.pin, ret);
		return;
	}

	if (def_val[3] != (uint8_t)ret) {
		state += 10U;
	}

	if (status[MOUSE_Y_REPORT_POS] != state) {
		status[MOUSE_Y_REPORT_POS] = state;
		k_sem_give(&sem);
	}
}

int callbacks_configure(const struct gpio_dt_spec *spec,
			gpio_callback_handler_t handler,
			struct gpio_callback *callback, uint8_t *val)
{
	const struct device *gpio = spec->port;
	gpio_pin_t pin = spec->pin;
	int ret;

	if (gpio == NULL) {
		/* Optional GPIO is missing. */
		return 0;
	}

	if (!device_is_ready(gpio)) {
		LOG_ERR("GPIO port %s is not ready", gpio->name);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(spec, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure port %s pin %u, error: %d",
			gpio->name, pin, ret);
		return ret;
	}

	ret = gpio_pin_get(gpio, pin);
	if (ret < 0) {
		LOG_ERR("Failed to get the state of port %s pin %u, error: %d",
			gpio->name, pin, ret);
		return ret;
	}

	*val = (uint8_t)ret;

	gpio_init_callback(callback, handler, BIT(pin));
	ret = gpio_add_callback(gpio, callback);
	if (ret < 0) {
		LOG_ERR("Failed to add the callback for port %s pin %u, "
			"error: %d",
			gpio->name, pin, ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(spec, GPIO_INT_EDGE_BOTH);
	if (ret < 0) {
		LOG_ERR("Failed to configure interrupt for port %s pin %u, "
			"error: %d",
			gpio->name, pin, ret);
		return ret;
	}

	return 0;
}

int main(void)
{
	int ret;
	uint8_t report[4] = { 0x00 };
	const struct device *hid_dev;

	if (!device_is_ready(led0.port)) {
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

	if (callbacks_configure(&sw0, &left_button, &callback[0],
				&def_val[0])) {
		LOG_ERR("Failed configuring left button callback.");
		return 0;
	}

	if (callbacks_configure(&sw1, &right_button, &callback[1],
				&def_val[1])) {
		LOG_ERR("Failed configuring right button callback.");
		return 0;
	}

	if (callbacks_configure(&sw2, &x_move, &callback[2], &def_val[2])) {
		LOG_ERR("Failed configuring X axis movement callback.");
		return 0;
	}

	if (callbacks_configure(&sw3, &y_move, &callback[3], &def_val[3])) {
		LOG_ERR("Failed configuring Y axis movement callback.");
		return 0;
	}

	usb_hid_register_device(hid_dev,
				hid_report_desc, sizeof(hid_report_desc),
				NULL);

	usb_hid_init(hid_dev);

	ret = usb_enable(status_cb);
	if (ret != 0) {
		LOG_ERR("Failed to enable USB");
		return 0;
	}

	while (true) {
		k_sem_take(&sem, K_FOREVER);

		report[MOUSE_BTN_REPORT_POS] = status[MOUSE_BTN_REPORT_POS];
		report[MOUSE_X_REPORT_POS] = status[MOUSE_X_REPORT_POS];
		status[MOUSE_X_REPORT_POS] = 0U;
		report[MOUSE_Y_REPORT_POS] = status[MOUSE_Y_REPORT_POS];
		status[MOUSE_Y_REPORT_POS] = 0U;
		ret = hid_int_ep_write(hid_dev, report, sizeof(report), NULL);
		if (ret) {
			LOG_ERR("HID write error, %d", ret);
		}

		/* Toggle LED on sent report */
		ret = gpio_pin_toggle(led0.port, led0.pin);
		if (ret < 0) {
			LOG_ERR("Failed to toggle the LED pin, error: %d", ret);
		}
	}
	return 0;
}
