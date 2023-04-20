/*
 * Copyright (c) 2022 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>

#if CONFIG_SOC_ESP32
#error "Temperature sensor is not supported on ESP32 soc"
#endif /* CONFIG_SOC_ESP32 */

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET_ONE(espressif_esp32_temp);
	struct sensor_value val;
	int rc;

	if (!device_is_ready(dev)) {
		printk("Temperature sensor is not ready\n");
		return 0;
	}

	printk("ESP32 Die temperature sensor test\n");

	while (1) {
		k_sleep(K_MSEC(300));

		/* fetch sensor samples */
		rc = sensor_sample_fetch(dev);
		if (rc) {
			printk("Failed to fetch sample (%d)\n", rc);
			return 0;
		}

		rc = sensor_channel_get(dev, SENSOR_CHAN_DIE_TEMP, &val);
		if (rc) {
			printk("Failed to get data (%d)\n", rc);
			return 0;
		}

		printk("Current temperature: %.1f °C\n", sensor_value_to_double(&val));
	}
	return 0;
}
