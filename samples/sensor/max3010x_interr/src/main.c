/*
 * SPDX-FileCopyrightText Copyright (c) 2026 Ashirwad Paswan
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#define I2C_NODE DT_NODELABEL(mysensor)
LOG_MODULE_REGISTER(prj_max30102, LOG_LEVEL_DBG);

void trigger_handler(const struct device *dev, const struct sensor_trigger *trig)
{

	struct sensor_value red, ir;
	/* struct sensor_value green;*/
	int ret;

	ret = sensor_sample_fetch(dev);

	if (ret < 0) {
		LOG_ERR("Failed to fetch sample");
	}
	ret = sensor_channel_get(dev, SENSOR_CHAN_RED, &red);

	if (ret < 0) {
		LOG_ERR("Failed to get red value");
	}
	ret = sensor_channel_get(dev, SENSOR_CHAN_IR, &ir);

	if (ret < 0) {
		LOG_ERR("Failed to get ir value");
	}

	/*
	 * ret = sensor_channel_get(dev, SENSOR_CHAN_GREEN,&green);
	 *
	 * if (ret < 0) {
	 *	LOG_ERR("Failed to get ir value");
	 *
	 *	}
	 */
	printk("Interrupt! Red: %d, IR: %d\n", red.val1, ir.val1);
	/* printk("Interrupt! Red: %d, IR: %d, GREEN: %d\n" red.val1, ir.val1, green.val1);*/
}

int main(void)
{
	static const struct device *max = DEVICE_DT_GET(I2C_NODE);

	if (!device_is_ready(max)) {
		LOG_WRN("I2C bus is not ready!\n\r");
		return -1;
	}
	printk("MAX30102 initialized successfully\n");
	struct sensor_trigger trig = {

		.type = SENSOR_TRIG_FIFO_WATERMARK,
		.chan = SENSOR_CHAN_RED,
	};

	if (sensor_trigger_set(max, &trig, trigger_handler) < 0) {
		LOG_ERR("Could not set trigger");
		return 0;
	}

	LOG_INF("Trigger set, waiting for interrupts...");

	while (1) {
		k_sleep(K_FOREVER);
	}
	return 0;
}
