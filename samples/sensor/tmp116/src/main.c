/*
 * Copyright (c) 2019 Centaur Analytics, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/eeprom.h>
#include <zephyr/drivers/sensor/tmp116.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/__assert.h>

#define TMP116_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(ti_tmp116)
#define TMP116_EEPROM_NODE DT_CHILD(TMP116_NODE, ti_tmp116_eeprom_0)

static uint8_t eeprom_content[EEPROM_TMP116_SIZE];

void main(void)
{
	const struct device *dev = DEVICE_DT_GET(TMP116_NODE);
	const struct device *eeprom = DEVICE_DT_GET(TMP116_EEPROM_NODE);
	struct sensor_value temp_value;

	/* offset to be added to the temperature
	 * only supported by TMP117
	 */
	struct sensor_value offset_value;
	int ret;

	__ASSERT(device_is_ready(dev), "TMP116 device not ready");
	__ASSERT(device_is_ready(eeprom), "TMP116 eeprom device not ready");

	printk("Device %s - %p is ready\n", dev->name, dev);

	ret = eeprom_read(eeprom, 0, eeprom_content, sizeof(eeprom_content));
	if (ret == 0) {
		printk("eeprom content %02x%02x%02x%02x%02x%02x%02x%02x\n",
		       eeprom_content[0], eeprom_content[1],
		       eeprom_content[2], eeprom_content[3],
		       eeprom_content[4], eeprom_content[5],
		       eeprom_content[6], eeprom_content[7]);
	} else {
		printk("Failed to get eeprom content\n");
	}

	/*
	 * if an offset of 2.5 oC is to be added,
	 * set val1 = 2 and val2 = 500000.
	 * See struct sensor_value documentation for more details.
	 */
	offset_value.val1 = 0;
	offset_value.val2 = 0;
	ret = sensor_attr_set(dev, SENSOR_CHAN_AMBIENT_TEMP,
			      SENSOR_ATTR_OFFSET, &offset_value);
	if (ret) {
		printk("sensor_attr_set failed ret = %d\n", ret);
		printk("SENSOR_ATTR_OFFSET is only supported by TMP117\n");
	}
	while (1) {
		ret = sensor_sample_fetch(dev);
		if (ret) {
			printk("Failed to fetch measurements (%d)\n", ret);
			return;
		}

		ret = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP,
					 &temp_value);
		if (ret) {
			printk("Failed to get measurements (%d)\n", ret);
			return;
		}

		printk("temp is %d.%d oC\n", temp_value.val1, temp_value.val2);

		k_sleep(K_MSEC(1000));
	}
}
