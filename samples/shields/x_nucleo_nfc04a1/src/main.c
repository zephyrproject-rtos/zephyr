/*
 * Copyright (c) 2023 Kickmaker
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/eeprom.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/eeprom/eeprom_st25dv.h>
#include <stdio.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS 1000

#define EEPROM_SAMPLE_OFFSET 0
#define EEPROM_SAMPLE_MAGIC  0xEE9704

#define LED1_NODE DT_ALIAS(shield_green_led1)
#define LED2_NODE DT_ALIAS(shield_blue_led2)
#define LED3_NODE DT_ALIAS(shield_yellow_led3)

struct perisistant_values {
	uint32_t magic;
	uint32_t boot_count;
};

static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(LED2_NODE, gpios);
static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(LED3_NODE, gpios);

/*
 * Get a device structure from a devicetree node with alias eeprom-0
 */
static const struct device *get_eeprom_device(void)
{
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(eeprom_0));

	if (!device_is_ready(dev)) {
		printk("\nError: Device \"%s\" is not ready; "
		       "check the driver initialization logs for errors.\n",
		       dev->name);
		return NULL;
	}

	printk("Found EEPROM device \"%s\"\n", dev->name);
	return dev;
}

int main(void)
{
	const struct device *eeprom = get_eeprom_device();
	struct perisistant_values values;
	int rc;

	if (!gpio_is_ready_dt(&led1) || !gpio_is_ready_dt(&led2) || !gpio_is_ready_dt(&led3)) {
		printk("Device not ready\n");
		return 0;
	}

	rc = gpio_pin_configure_dt(&led1, GPIO_OUTPUT_ACTIVE);
	if (rc < 0) {
		printk("led1 could not be configured\n");
		return 0;
	}
	rc = gpio_pin_configure_dt(&led2, GPIO_OUTPUT_ACTIVE);
	if (rc < 0) {
		printk("led2 could not be configured\n");
		return 0;
	}
	rc = gpio_pin_configure_dt(&led3, GPIO_OUTPUT_ACTIVE);
	if (rc < 0) {
		printk("led3 could not be configured\n");
		return 0;
	}

	if (eeprom == NULL) {
		printk("eeprom device is NULL\n");
		return 0;
	}

#if defined(CONFIG_EEPROM_ST25DV_ENABLE_ADVANCED_FEATURES)
	/*
	 * Use ST25DV specific functions from eeprom_st25dv header file
	 * Read UUID and IC rev
	 */
	size_t eeprom_size;
	uint8_t uuid[8] = {0};
	uint8_t ic_rev = 0;

	eeprom_st25dv_read_uuid(eeprom, uuid);
	printk("UUID = %02X %02X %02X %02X.\n", uuid[0], uuid[1], uuid[2], uuid[3]);
	eeprom_st25dv_read_ic_rev(eeprom, &ic_rev);
	printk("REV_IC = %02X.\n", ic_rev);

	eeprom_size = eeprom_get_size(eeprom);
	printk("Using eeprom with size of: %zu.\n", eeprom_size);
#endif /* CONFIG_EEPROM_ST25DV_ENABLE_ADVANCED_FEATURES */

	rc = eeprom_read(eeprom, EEPROM_SAMPLE_OFFSET, &values, sizeof(values));
	if (rc < 0) {
		printk("Error: Couldn't read eeprom: err: %d.\n", rc);
		return 0;
	}

	if (values.magic != EEPROM_SAMPLE_MAGIC) {
		values.magic = EEPROM_SAMPLE_MAGIC;
		values.boot_count = 0;
	}

	values.boot_count++;
	printk("Device booted %d times.\n", values.boot_count);

	rc = eeprom_write(eeprom, EEPROM_SAMPLE_OFFSET, &values, sizeof(values));
	if (rc < 0) {
		printk("Error: Couldn't write eeprom: err:%d.\n", rc);
		return 0;
	}

	printk("Reset the MCU to see the increasing boot counter.\n\n");

	while (1) {
		rc = gpio_pin_toggle_dt(&led1);
		if (rc < 0) {
			return 0;
		}
		k_msleep(SLEEP_TIME_MS);
		rc = gpio_pin_toggle_dt(&led2);
		if (rc < 0) {
			return 0;
		}
		k_msleep(SLEEP_TIME_MS);
		rc = gpio_pin_toggle_dt(&led3);
		if (rc < 0) {
			return 0;
		}
		k_msleep(SLEEP_TIME_MS);
	}
	return 0;
}
