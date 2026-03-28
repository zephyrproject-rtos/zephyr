/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/dt-bindings/regulator/npm2100.h>

#define UPDATE_TIME_MS 2000

static const struct device *boost = DEVICE_DT_GET(DT_NODELABEL(npm2100_boost));
static const struct device *ldosw = DEVICE_DT_GET(DT_NODELABEL(npm2100_ldosw));
static const struct device *vbat = DEVICE_DT_GET(DT_NODELABEL(npm2100_vbat));

int setup_regulators(void)
{
	int err;

	/* boost to operate in Low Power mode by default, force High Power on GPIO0 active */
	err = regulator_set_mode(boost, NPM2100_REG_OPER_LP | NPM2100_REG_FORCE_HP);
	if (err != 0) {
		return err;
	}

	/* LDOSW to operate as a Load Switch: off by default, on when GPIO1 active */
	err = regulator_set_mode(ldosw,
				 NPM2100_REG_OPER_OFF | NPM2100_REG_FORCE_HP | NPM2100_REG_LDSW_EN);
	if (err != 0) {
		return err;
	}

	return regulator_enable(ldosw);
}

void display_sensor_values(void)
{
	struct sensor_value v_battery;
	struct sensor_value v_out;
	struct sensor_value temp;

	sensor_sample_fetch(vbat);
	sensor_channel_get(vbat, SENSOR_CHAN_GAUGE_VOLTAGE, &v_battery);
	sensor_channel_get(vbat, SENSOR_CHAN_VOLTAGE, &v_out);
	sensor_channel_get(vbat, SENSOR_CHAN_DIE_TEMP, &temp);

	printk("VBat: %d.%03d ", v_battery.val1, v_battery.val2 / 1000);
	printk("VOut: %d.%03d ", v_out.val1, v_out.val2 / 1000);
	printk("T: %s%d.%02d\n", ((temp.val1 < 0) || (temp.val2 < 0)) ? "-" : "", abs(temp.val1),
	       abs(temp.val2) / 10000);
}

int main(void)
{
	int err;

	if (!device_is_ready(ldosw)) {
		printk("Error: LDOSW device is not ready\n");
		return -ENODEV;
	}

	if (!device_is_ready(boost)) {
		printk("Error: BOOST device is not ready\n");
		return -ENODEV;
	}

	if (!device_is_ready(vbat)) {
		printk("Error: vbat device is not ready\n");
		return -ENODEV;
	}

	err = setup_regulators();
	if (err != 0) {
		printk("Error: failed to set regulator modes\n");
		return err;
	}

	printk("PMIC device ok\n");

	while (1) {
		display_sensor_values();
		k_msleep(UPDATE_TIME_MS);
	}
}
