/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_SENSOR_SHELL_H
#define ZEPHYR_DRIVERS_SENSOR_SENSOR_SHELL_H

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/shell/shell.h>

struct sensor_shell_processing_context {
	const struct device *dev;
	const struct shell *sh;
};

extern struct rtio sensor_read_rtio;

int cmd_sensor_stream(const struct shell *shell_ptr, size_t argc, char *argv[]);

void sensor_shell_processing_callback(int result, uint8_t *buf, uint32_t buf_len, void *userdata);

#endif /* ZEPHYR_DRIVERS_SENSOR_SENSOR_SHELL_H */
