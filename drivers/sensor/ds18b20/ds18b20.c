/*
 * Copyright (c) 2018 Roman Tataurov <diytronic@yandex.ru>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <w1.h>
#include <init.h>
#include <misc/__assert.h>
#include <misc/byteorder.h>
#include <sensor.h>
#include <string.h>

#include "ds18b20.h"

#define DS18B20_COMMAND_WRITE_SCRATCHPAD  0x4E
#define DS18B20_COMMAND_READ_SCRATCHPAD   0xBE
#define DS18B20_COMMAND_COPY_SCRATCHPAD   0x48
#define DS18B20_COMMAND_CONVERT_T         0x44
#define DS18B20_COMMAND_RECALL_EEPROM     0xB8
#define DS18B20_COMMAND_READ_POWER_SOURCE 0xB4

#define DS18B20_CONFIG_BIT_R0 5
#define DS18B20_CONFIG_BIT_R1 6

#ifdef CONFIG_DS18B20_RESOLUTION_9BIT
	#define DS18B20_RESOLUTION_BITS 0x00
	#define DS18B20_MAX_CONVERSION_TIME 93750
#endif

#ifdef CONFIG_DS18B20_RESOLUTION_10BIT
	#define DS18B20_RESOLUTION_BITS (1 << DS18B20_CONFIG_BIT_R0)
	#define DS18B20_MAX_CONVERSION_TIME 187500
#endif

#ifdef CONFIG_DS18B20_RESOLUTION_11BIT
	#define DS18B20_RESOLUTION_BITS (1 << DS18B20_CONFIG_BIT_R1)
	#define DS18B20_MAX_CONVERSION_TIME 375000
#endif

#ifdef CONFIG_DS18B20_RESOLUTION_12BIT
	#define DS18B20_RESOLUTION_BITS \
		((1 << DS18B20_CONFIG_BIT_R0) | (1 << DS18B20_CONFIG_BIT_R1))
	#define DS18B20_MAX_CONVERSION_TIME 750000
#endif

#define DS18B20_T_HIGH(t) ((t & 0x0FFF) >> 4)
#define DS18B20_T_LOW(t) ((t & 0x000F) * 10000)

static int ds18b20_write_scratchpad(struct device *dev,
	ds18b20_scratchpad_t scratchpad)
{

	struct ds18b20_data *drv_data = dev->driver_data;
	struct device *bus = drv_data->bus;

	u8_t data[3] = {
		scratchpad.alarm_temperature_high,
		scratchpad.alarm_temperature_low,
		scratchpad.configuration
	};

	w1_skip_rom(bus);
	w1_send_command(bus, DS18B20_COMMAND_WRITE_SCRATCHPAD);
	w1_write_block(bus, &data, sizeof(data));

	return 0;
}

static int ds18b20_read_scratchpad(struct device *dev,
	ds18b20_scratchpad_t *scratchpad)
{
	struct ds18b20_data *drv_data = dev->driver_data;
	struct device *bus = drv_data->bus;

	w1_skip_rom(bus);
	w1_send_command(bus, DS18B20_COMMAND_READ_SCRATCHPAD);
	w1_read_block(bus, (char *)scratchpad, 9);

	return 0;
}

static void ds18b20_convert(struct device *dev)
{
	struct ds18b20_data *drv_data = dev->driver_data;
	struct device *bus = drv_data->bus;

	w1_skip_rom(bus);
	w1_send_command(drv_data->bus, DS18B20_COMMAND_CONVERT_T);
	w1_wait_for(drv_data->bus, 1, DS18B20_MAX_CONVERSION_TIME);
}

static int ds18b20_read_power_source(struct device *dev)
{
	struct ds18b20_data *drv_data = dev->driver_data;
	struct device *bus = drv_data->bus;

	w1_skip_rom(bus);
	w1_send_command(bus, DS18B20_COMMAND_READ_POWER_SOURCE);
	drv_data->power_source = w1_read_bit(bus);

	return 0;
}

static int ds18b20_set_resolution(struct device *dev)
{
	struct ds18b20_data *drv_data = dev->driver_data;

	drv_data->scratchpad.configuration &= \
		~((1 << DS18B20_CONFIG_BIT_R0) | (1 << DS18B20_CONFIG_BIT_R1));
	drv_data->scratchpad.configuration |= DS18B20_RESOLUTION_BITS;

	return 0;
}

static int ds18b20_set_alarm_trigger(struct device *dev, u8_t low, u8_t high)
{
	struct ds18b20_data *drv_data = dev->driver_data;

	drv_data->scratchpad.alarm_temperature_high = high;
	drv_data->scratchpad.alarm_temperature_low = low;

	return 0;
}

static int ds18b20_channel_get(struct device *dev,
			enum sensor_channel chan,
			struct sensor_value *val)
{
	struct ds18b20_data *drv_data = dev->driver_data;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_AMBIENT_TEMP);

	ds18b20_read_scratchpad(dev, &drv_data->scratchpad);

	if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		val->val1 = DS18B20_T_HIGH(drv_data->scratchpad.temperature);
		val->val2 = DS18B20_T_LOW(drv_data->scratchpad.temperature);
	}

	return 0;
}

static int ds18b20_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	ds18b20_convert(dev);

	return 0;
}

static const struct ds18b20_driver_api ds18b20_api = {
	.sample_fetch = ds18b20_sample_fetch,
	.channel_get = ds18b20_channel_get,
	.convert = ds18b20_convert,
};

int ds18b20_init(struct device *dev)
{
	struct ds18b20_data *drv_data = dev->driver_data;

	drv_data->bus = device_get_binding(CONFIG_DS18B20_BUS_NAME);
	if (drv_data->bus == NULL) {
		SYS_LOG_ERR("Could not get pointer to 1-Wire bus %s device.",
				 CONFIG_DS18B20_BUS_NAME);
		return -EINVAL;
	}

	if (!w1_reset_bus(drv_data->bus)) {
		SYS_LOG_ERR("No 1-Wire devices found");
		return -EINVAL;
	}

	w1_reg_num reg_num;

	w1_read_rom(drv_data->bus, &reg_num);
	if (reg_num.family != DS18B20_FAMILY_CODE) {
		SYS_LOG_ERR("Found 1-Wire device is not a DS18B20");
		return -EINVAL;
	}

	/* read power source */
	ds18b20_read_power_source(dev);

	/* write defaults */
	ds18b20_set_resolution(dev);
	/* ds18b20_set_alarm_trigger(dev, 7, 100); */
	ds18b20_write_scratchpad(dev, drv_data->scratchpad);

	return 0;
}

struct ds18b20_data ds18b20_driver;

DEVICE_AND_API_INIT(ds18b20, CONFIG_DS18B20_NAME, ds18b20_init, &ds18b20_driver,
	NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
	&ds18b20_api);
