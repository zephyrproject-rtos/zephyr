/*
 * Copyright (c) 2018 qianfan Zhao
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2019 Intel Corp
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

#define LED_PORT	DT_ALIAS_LED0_GPIOS_CONTROLLER
#define LED		DT_ALIAS_LED0_GPIOS_PIN

#ifdef CONFIG_FXOS8700
#include <drivers/sensor.h>
#define SENSOR_ACCEL_NAME DT_INST_0_NXP_FXOS8700_LABEL
#endif

static const u8_t hid_report_desc[] = HID_MOUSE_REPORT_DESC(2);

static u32_t def_val[4];
static volatile u8_t status[4];
static K_SEM_DEFINE(sem, 0, 1);	/* starts off "not available" */
static struct gpio_callback callback[4];

#define MOUSE_BTN_REPORT_POS	0
#define MOUSE_X_REPORT_POS	1
#define MOUSE_Y_REPORT_POS	2

#define MOUSE_BTN_LEFT		BIT(0)
#define MOUSE_BTN_RIGHT		BIT(1)
#define MOUSE_BTN_MIDDLE	BIT(2)


static void left_button(struct device *gpio, struct gpio_callback *cb,
			u32_t pins)
{
	u32_t cur_val;
	u8_t state = status[MOUSE_BTN_REPORT_POS];

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

#ifdef DT_ALIAS_SW1_GPIOS_PIN
static void right_button(struct device *gpio, struct gpio_callback *cb,
			 u32_t pins)
{
	u32_t cur_val;
	u8_t state = status[MOUSE_BTN_REPORT_POS];

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

static bool read_accel(struct device *dev)
{
	struct sensor_value val[3];
	int ret;

	ret = sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, val);
	if (ret < 0) {
		LOG_ERR("Cannot read sensor channels");
		return false;
	}

	/* For printing double we need to use printf with
	 * printf("%10.6f\n", sensor_value_to_double(x));
	 */
	LOG_DBG("int parts: X %d Y %d", val[0].val1, val[1].val1);

	/* TODO: Add proper calculations */

	if (val[0].val1 != 0) {
		status[MOUSE_X_REPORT_POS] = val[0].val1 * 4;
	}

	if (val[1].val1 != 0) {
		status[MOUSE_Y_REPORT_POS] = val[1].val1 * 4;
	}

	if (val[0].val1 != 0 || val[1].val1 != 0) {
		return true;
	} else {
		return false;
	}
}

static void trigger_handler(struct device *dev, struct sensor_trigger *tr)
{
	ARG_UNUSED(tr);

	/* Always fetch the sample to clear the data ready interrupt in the
	 * sensor.
	 */
	if (sensor_sample_fetch(dev)) {
		LOG_ERR("sensor_sample_fetch failed");
		return;
	}

	if (read_accel(dev)) {
		k_sem_give(&sem);
	}
}

void main(void)
{
	u8_t report[4] = { 0x00 };
	u8_t toggle = 0U;
	struct device *led_dev, *accel_dev, *hid_dev;

	led_dev = device_get_binding(LED_PORT);
	if (led_dev == NULL) {
		LOG_ERR("Could not get %s device", LED_PORT);
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

#ifdef DT_ALIAS_SW1_GPIOS_PIN
	if (callbacks_configure(device_get_binding(PORT1), PIN1, PIN1_FLAGS,
				&right_button, &callback[1], &def_val[1])) {
		LOG_ERR("Failed configuring right button callback.");
		return;
	}
#endif

	accel_dev = device_get_binding(SENSOR_ACCEL_NAME);
	if (accel_dev == NULL) {
		LOG_ERR("Could not get %s device", SENSOR_ACCEL_NAME);
		return;
	}

	struct sensor_value attr = {
		.val1 = 6,
		.val2 = 250000,
	};

	if (sensor_attr_set(accel_dev, SENSOR_CHAN_ALL,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &attr)) {
		LOG_ERR("Could not set sampling frequency");
		return;
	}

	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ACCEL_XYZ,
	};

	if (sensor_trigger_set(accel_dev, &trig, trigger_handler)) {
		LOG_ERR("Could not set trigger");
		return;
	}

	usb_hid_register_device(hid_dev, hid_report_desc,
				sizeof(hid_report_desc), NULL);
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
