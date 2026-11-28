/*
 * Copyright (c) 2026 Zephyr SNMP Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "app_sensor.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <errno.h>

LOG_MODULE_REGISTER(app_sensor, LOG_LEVEL_INF);

static int32_t s_temperature = 25;
static uint32_t s_humidity = 55;
static const char *const s_description = "NUCLEO-H743ZI Sensor Node";

static const char *const s_table_names[] = {"CPU Temp", "Amb Humidity", "Core Voltage"};

static int32_t s_table_static_values[] = {25, 55, 3300};

int app_sensor_init(void)
{
	s_temperature = 25;
	s_humidity = 55;
	LOG_INF("Sensor module initialized (Initial Temp: %d C, Hum: %u%%)", s_temperature,
		s_humidity);
	return 0;
}

int32_t app_sensor_get_temp(void)
{
	return s_temperature;
}

void app_sensor_set_temp(int32_t temp)
{
	s_temperature = temp;
	LOG_INF("Simulated temperature updated to %d C", s_temperature);
}

uint32_t app_sensor_get_humidity(void)
{
	return s_humidity;
}

const char *app_sensor_get_description(void)
{
	return s_description;
}

int app_sensor_get_table_entry(uint32_t index, const char **name, int32_t *value)
{
	if (index < 1 || index > 3) {
		return -ENOENT;
	}

	if (name) {
		*name = s_table_names[index - 1];
	}
	if (value) {
		if (index == 1) {
			*value = s_temperature;
		} else if (index == 2) {
			*value = (int32_t)s_humidity;
		} else {
			*value = s_table_static_values[index - 1];
		}
	}
	return 0;
}

void app_sensor_update_periodic(void)
{
	/* Vary temperature between 22 and 36 C based on uptime */
	uint32_t uptime_sec = (uint32_t)(k_uptime_get() / 1000);
	s_temperature = 22 + (int32_t)(uptime_sec % 15);

	/* Vary humidity between 45% and 65% */
	s_humidity = 45 + (uptime_sec * 3 % 21);
}
