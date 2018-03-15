/*
 * Copyright (c) 2018 Roman Tataurov <diytronic@yandex.ru>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SENSOR_DS18B20_H__
#define __SENSOR_DS18B20_H__

#include <device.h>
#include <misc/util.h>
#include <zephyr/types.h>
#include <gpio.h>

#define SYS_LOG_DOMAIN "DS18B20"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SENSOR_LEVEL
#include <logging/sys_log.h>

#define DS18B20_DEVICE_DEFAULT -1

#define DS18B20_FAMILY_CODE 0x28

#define DS18B20_POWER_SOURCE_EXTERNAL 0x1 /* powered with external source */
#define DS18B20_POWER_SOURCE_PARASITE 0x0 /* used "parasite" power */

/**
 * DS18B20 registers
 *
 * More details here https://cdn-shop.adafruit.com/datasheets/DS18B20.pdf
 */
typedef struct {
	s16_t temperature;
	u8_t  alarm_temperature_high;
	u8_t  alarm_temperature_low;
	u8_t  configuration;
	u8_t  reserved1;
	u8_t  reserved2;
	u8_t  reserved3;
	u8_t  crc;
} ds18b20_scratchpad_t;

/**
 * @cond INTERNAL_HIDDEN
 *
 * These are for internal use only, so skip these in
 * public documentation.
 */
typedef void (*ds18b20_set_alarm_trigger_t)(struct device *, u8_t, u8_t);
typedef void (*ds18b20_convert_t)(struct device *);
typedef void (*ds18b20_write_scratchpad_t)(struct device *,
	const ds18b20_scratchpad_t);
typedef void (*ds18b20_read_scratchpad_t)(struct device *,
	ds18b20_scratchpad_t *);

struct ds18b20_driver_api {
	sensor_attr_set_t attr_set;
	sensor_trigger_set_t trigger_set;
	sensor_sample_fetch_t sample_fetch;
	sensor_channel_get_t channel_get;
	ds18b20_set_alarm_trigger_t set_alarm_trigger;
	ds18b20_convert_t convert;
	ds18b20_write_scratchpad_t write_scratchpad;
	ds18b20_read_scratchpad_t read_scratchpad;
};

/**
 * @endcond
 */

/**
 * Driver instance data
 */
struct ds18b20_data {
	struct device *bus;
	ds18b20_scratchpad_t scratchpad;
	u8_t power_source : 1;
};

#endif /* __SENSOR_DS18B20__ */
