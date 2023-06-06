/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>

#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

#include "sensor_shell.h"

/* Create a single common config for streaming */
static struct sensor_stream_trigger iodev_sensor_shell_trigger;
static struct sensor_read_config iodev_sensor_shell_stream_config = {
	.sensor = NULL,
	.is_streaming = true,
	.triggers = &iodev_sensor_shell_trigger,
	.count = 0,
	.max = 1,
};
RTIO_IODEV_DEFINE(iodev_sensor_shell_stream, &__sensor_iodev_api,
		  &iodev_sensor_shell_stream_config);

static void sensor_shell_processing_entry_point(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	while (true) {
		sensor_processing_with_callback(&sensor_read_rtio,
						sensor_shell_processing_callback);
	}
}
K_THREAD_DEFINE(sensor_shell_processing_tid, CONFIG_SENSOR_SHELL_THREAD_STACK_SIZE,
		sensor_shell_processing_entry_point, NULL, NULL, NULL, 0, 0, 0);

int cmd_sensor_stream(const struct shell *shell_ptr, size_t argc, char *argv[])
{
	static struct rtio_sqe *current_streaming_handle;
	static struct sensor_shell_processing_context ctx;
	const struct device *dev = device_get_binding(argv[1]);

	if (argc != 5 && argc != 3) {
		shell_error(shell_ptr, "Wrong number of arguments (%zu)", argc);
		return -EINVAL;
	}

	if (dev == NULL) {
		shell_error(shell_ptr, "Device unknown (%s)", argv[1]);
		return -ENODEV;
	}

	if (current_streaming_handle != NULL) {
		shell_info(shell_ptr, "Disabling existing stream");
		rtio_sqe_cancel(current_streaming_handle);
	}

	if (strcmp("off", argv[2]) == 0) {
		return 0;
	}

	if (strcmp("on", argv[2]) != 0) {
		shell_error(shell_ptr, "Unknown streaming operation (%s)", argv[2]);
		return -EINVAL;
	}

	if (strcmp("double_tap", argv[3]) == 0) {
		iodev_sensor_shell_trigger.trigger = SENSOR_TRIG_DOUBLE_TAP;
	} else if (strcmp("data_ready", argv[3]) == 0) {
		iodev_sensor_shell_trigger.trigger = SENSOR_TRIG_DATA_READY;
	} else if (strcmp("delta", argv[3]) == 0) {
		iodev_sensor_shell_trigger.trigger = SENSOR_TRIG_DELTA;
	} else if (strcmp("freefall", argv[3]) == 0) {
		iodev_sensor_shell_trigger.trigger = SENSOR_TRIG_FREEFALL;
	} else if (strcmp("motion", argv[3]) == 0) {
		iodev_sensor_shell_trigger.trigger = SENSOR_TRIG_MOTION;
	} else if (strcmp("near_far", argv[3]) == 0) {
		iodev_sensor_shell_trigger.trigger = SENSOR_TRIG_NEAR_FAR;
	} else if (strcmp("stationary", argv[3]) == 0) {
		iodev_sensor_shell_trigger.trigger = SENSOR_TRIG_STATIONARY;
	} else if (strcmp("threshold", argv[3]) == 0) {
		iodev_sensor_shell_trigger.trigger = SENSOR_TRIG_THRESHOLD;
	} else if (strcmp("fifo_wm", argv[3]) == 0) {
		iodev_sensor_shell_trigger.trigger = SENSOR_TRIG_FIFO_WATERMARK;
	} else if (strcmp("fifo_full", argv[3]) == 0) {
		iodev_sensor_shell_trigger.trigger = SENSOR_TRIG_FIFO_FULL;
	} else if (strcmp("tap", argv[3]) == 0) {
		iodev_sensor_shell_trigger.trigger = SENSOR_TRIG_TAP;
	} else {
		shell_error(shell_ptr, "Invalid trigger (%s)", argv[3]);
		return -EINVAL;
	}

	if (strcmp("incl", argv[4]) == 0) {
		iodev_sensor_shell_trigger.opt = SENSOR_STREAM_DATA_INCLUDE;
	} else if (strcmp("drop", argv[4]) == 0) {
		iodev_sensor_shell_trigger.opt = SENSOR_STREAM_DATA_DROP;
	} else if (strcmp("nop", argv[4]) == 0) {
		iodev_sensor_shell_trigger.opt = SENSOR_STREAM_DATA_NOP;
	} else {
		shell_error(shell_ptr, "Unknown trigger op (%s)", argv[4]);
		return -EINVAL;
	}

	shell_print(shell_ptr, "Enabling stream...");
	iodev_sensor_shell_stream_config.sensor = dev;

	iodev_sensor_shell_stream_config.count = 1;

	ctx.dev = dev;
	ctx.sh = shell_ptr;

	int rc = sensor_stream(&iodev_sensor_shell_stream, &sensor_read_rtio, &ctx,
			       &current_streaming_handle);

	if (rc != 0) {
		shell_error(shell_ptr, "Failed to start stream");
	}
	return rc;
}
