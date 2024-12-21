/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/npm1300_charger.h>
#include <zephyr/drivers/led.h>
#include <zephyr/dt-bindings/regulator/npm1300.h>
#include <zephyr/drivers/mfd/npm1300.h>
#include <zephyr/sys/printk.h>
#include <getopt.h>

#define SLEEP_TIME_MS  100
#define UPDATE_TIME_MS 2000

static const struct gpio_dt_spec button1 = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);

static const struct device *regulators = DEVICE_DT_GET(DT_NODELABEL(npm1300_ek_regulators));

static const struct device *charger = DEVICE_DT_GET(DT_NODELABEL(npm1300_ek_charger));

static const struct device *leds = DEVICE_DT_GET(DT_NODELABEL(npm1300_ek_leds));

static const struct device *pmic = DEVICE_DT_GET(DT_NODELABEL(npm1300_ek_pmic));

void configure_ui(void)
{
	int ret;

	if (!gpio_is_ready_dt(&button1)) {
		printk("Error: button device %s is not ready\n", button1.port->name);
		return;
	}

	ret = gpio_pin_configure_dt(&button1, GPIO_INPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n", ret, button1.port->name,
		       button1.pin);
		return;
	}

	printk("Set up button at %s pin %d\n", button1.port->name, button1.pin);

	if (!device_is_ready(leds)) {
		printk("Error: led device is not ready\n");
		return;
	}
}

static void event_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	printk("Event detected\n");
}

void configure_events(void)
{
	if (!device_is_ready(pmic)) {
		printk("Pmic device not ready.\n");
		return;
	}

	/* Setup callback for shiphold button press */
	static struct gpio_callback event_cb;

	gpio_init_callback(&event_cb, event_callback, BIT(NPM1300_EVENT_SHIPHOLD_PRESS));

	mfd_npm1300_add_callback(pmic, &event_cb);
}

void read_sensors(void)
{
	struct sensor_value volt;
	struct sensor_value current;
	struct sensor_value temp;
	struct sensor_value error;
	struct sensor_value status;
	struct sensor_value vbus_present;

	sensor_sample_fetch(charger);
	sensor_channel_get(charger, SENSOR_CHAN_GAUGE_VOLTAGE, &volt);
	sensor_channel_get(charger, SENSOR_CHAN_GAUGE_AVG_CURRENT, &current);
	sensor_channel_get(charger, SENSOR_CHAN_GAUGE_TEMP, &temp);
	sensor_channel_get(charger, (enum sensor_channel)SENSOR_CHAN_NPM1300_CHARGER_STATUS,
			   &status);
	sensor_channel_get(charger, (enum sensor_channel)SENSOR_CHAN_NPM1300_CHARGER_ERROR, &error);
	sensor_attr_get(charger, (enum sensor_channel)SENSOR_CHAN_NPM1300_CHARGER_VBUS_STATUS,
			(enum sensor_attribute)SENSOR_ATTR_NPM1300_CHARGER_VBUS_PRESENT,
			&vbus_present);

	printk("V: %d.%03d ", volt.val1, volt.val2 / 1000);

	printk("I: %s%d.%04d ", ((current.val1 < 0) || (current.val2 < 0)) ? "-" : "",
	       abs(current.val1), abs(current.val2) / 100);

	printk("T: %s%d.%02d\n", ((temp.val1 < 0) || (temp.val2 < 0)) ? "-" : "", abs(temp.val1),
	       abs(temp.val2) / 10000);

	printk("Charger Status: %d, Error: %d, VBUS: %s\n", status.val1, error.val1,
	       vbus_present.val1 ? "connected" : "disconnected");
}

int main(void)
{
	configure_ui();

	configure_events();

	if (!device_is_ready(regulators)) {
		printk("Error: Regulator device is not ready\n");
		return 0;
	}

	if (!device_is_ready(charger)) {
		printk("Charger device not ready.\n");
		return 0;
	}

	while (1) {
		/* Cycle regulator control GPIOs when first button pressed */
		static bool last_button;
		static int dvs_state;
		bool button_state = gpio_pin_get_dt(&button1) == 1;

		if (button_state && !last_button) {
			dvs_state = (dvs_state + 1) % 4;
			regulator_parent_dvs_state_set(regulators, dvs_state);
		}

		/* Update PMIC LED if button state has changed */
		if (button_state != last_button) {
			if (button_state) {
				led_on(leds, 2U);
			} else {
				led_off(leds, 2U);
			}
		}

		/* Read and display charger status */
		static int count;

		if (++count > (UPDATE_TIME_MS / SLEEP_TIME_MS)) {
			read_sensors();
			count = 0;
		}

		last_button = button_state;
		k_msleep(SLEEP_TIME_MS);
	}
}
