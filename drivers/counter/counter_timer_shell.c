/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/counter.h>
#include <zephyr/shell/shell.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>

#define ARGV_DEV           1
#define ARGV_CHN           2
#define ARGV_PERIODIC_TIME 2
#define ARGV_ONESHOT_TIME  3

#define PERIODIC_CYCLES    10

void timer_top_handler(const struct device *counter_dev, void *user_data)
{
	ARG_UNUSED(counter_dev);
	uint32_t *count = (uint32_t *)user_data;

	*count = *count + 1;
}

void timer_alarm_handler(const struct device *counter_dev, uint8_t chan_id,
						 uint32_t ticks, void *user_data)
{
	ARG_UNUSED(counter_dev);
	bool *flag = (bool *)user_data;

	*flag = true;
}

static int cmd_timer_free_running(const struct shell *shctx,
							 size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	const struct device *timer_dev;
	int err = 0;

	timer_dev = device_get_binding(argv[ARGV_DEV]);
	if (!timer_dev) {
		shell_error(shctx, "Timer: Device %s not found", argv[ARGV_DEV]);
		return -ENODEV;
	}

	err = counter_start(timer_dev);
	if (err != 0) {
		shell_error(shctx, "%s is not available err:%d", argv[ARGV_DEV], err);
		return err;
	}

	shell_info(shctx, "%s: Timer is freerunning", argv[ARGV_DEV]);

	return  0;
}

static int cmd_timer_stop(const struct shell *shctx,
							 size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	const struct device *timer_dev;
	uint32_t ticks1 = 0, ticks2 = 0;

	timer_dev = device_get_binding(argv[ARGV_DEV]);
	if (!timer_dev) {
		shell_error(shctx, "Timer: Device %s not found", argv[ARGV_DEV]);
		return -ENODEV;
	}

	counter_stop(timer_dev);

	counter_get_value(timer_dev, &ticks1);
	counter_get_value(timer_dev, &ticks2);

	if (ticks1 == ticks2)
		shell_info(shctx, "Timer Stopped");

	return 0;
}

static int cmd_timer_oneshot(const struct shell *shctx,
							 size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	const struct device *timer_dev;
	struct counter_alarm_cfg alarm_cfg;
	unsigned long delay = 0;
	unsigned long channel = 0;
	volatile bool alarm_flag = false;
	int32_t err = 0;
	char *endptr;

	timer_dev = device_get_binding(argv[ARGV_DEV]);
	if (!timer_dev) {
		shell_error(shctx, "Timer: Device %s not found", argv[ARGV_DEV]);
		return -ENODEV;
	}

	errno = 0;
	delay = strtoul(argv[ARGV_ONESHOT_TIME], &endptr, 10);
	if (*endptr || (delay == LONG_MAX && errno)) {
		shell_error(shctx, "Timer: invalid delay:%ld", delay);
		return -EINVAL;
	}

	errno = 0;
	channel = strtoul(argv[ARGV_CHN], &endptr, 10);
	if (*endptr || (channel == LONG_MAX && errno)) {
		shell_error(shctx, "Timer: failed to set channel:%ld", channel);
		return -EINVAL;
	}

	alarm_cfg.flags = 0;
	alarm_cfg.ticks = counter_us_to_ticks(timer_dev, (uint64_t)delay);
	alarm_cfg.callback = timer_alarm_handler;
	alarm_cfg.user_data = (void *)&alarm_flag;

	err = counter_set_channel_alarm(timer_dev, (uint8_t)channel, &alarm_cfg);
	if (err != 0) {
		shell_error(shctx, "%s is not available err:%d", argv[ARGV_DEV], err);
		return err;
	}

	/* Wait for alarm interrupt */
	while (!alarm_flag) {
	}

	shell_info(shctx, "%s: Alarm triggered", argv[ARGV_DEV]);
	shell_info(shctx, "Timer stopped");

	return 0;
}

static int cmd_timer_periodic(const struct shell *shctx,
								size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	const struct device *timer_dev;
	struct counter_top_cfg top_cfg;
	unsigned long delay = 0;
	volatile uint32_t count = 0;
	int32_t err = 0;
	char *endptr;

	timer_dev = device_get_binding(argv[ARGV_DEV]);
	if (!timer_dev) {
		shell_error(shctx, "Timer: Device %s not found", argv[ARGV_DEV]);
		return -ENODEV;
	}

	errno = 0;
	delay = strtoul(argv[ARGV_PERIODIC_TIME], &endptr, 10);
	if (*endptr || (delay == LONG_MAX && errno)) {
		shell_error(shctx, "Timer: invalid delay:%ld", delay);
		return -EINVAL;
	}

	top_cfg.flags = 0;
	top_cfg.ticks = counter_us_to_ticks(timer_dev, (uint64_t)delay);
	top_cfg.callback = timer_top_handler;
	top_cfg.user_data = (void *)&count;

	err = counter_set_top_value(timer_dev, &top_cfg);
	if (err != 0) {
		shell_error(shctx, "%s is not available err:%d", argv[ARGV_DEV], err);
		return err;
	}

	/* Wait for PERIODIC_CYCLES number of periodic interrupts */
	while (count <= PERIODIC_CYCLES) {
	}

	counter_stop(timer_dev);

	shell_info(shctx, "%s: periodic timer triggered", argv[ARGV_DEV]);
	shell_info(shctx, "Timer Stopped");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_timer,
			SHELL_CMD_ARG(periodic, NULL,
			"timer periodic <device_label> <time_in_us>",
			cmd_timer_periodic, 3, 0),
			SHELL_CMD_ARG(oneshot, NULL,
			"timer oneshot <device_label> <channel_id> <time_in_us>",
			cmd_timer_oneshot, 4, 0),
			SHELL_CMD_ARG(freerun, NULL,
			"timer freerun <devics_label>",
			cmd_timer_free_running, 2, 0),
			SHELL_CMD_ARG(stop, NULL,
			"timer stop <device_label>",
			cmd_timer_stop, 2, 0),
			SHELL_SUBCMD_SET_END /* Array terminated. */
			);

SHELL_CMD_REGISTER(timer, &sub_timer, "Timer commands", NULL);
