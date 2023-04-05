/*
 * Copyright (c) 2022-2023 Intel Corporation.
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

/* number of periodic interrupts */
#define PERIODIC_CYCLES    10
#define MAX_DELAY          UINT32_MAX
#define MAX_CHANNEL        255U

static struct k_sem timer_sem;

void timer_top_handler(const struct device *counter_dev, void *user_data)
{
	ARG_UNUSED(counter_dev);

	k_sem_give(&timer_sem);
}

void timer_alarm_handler(const struct device *counter_dev, uint8_t chan_id,
				uint32_t ticks, void *user_data)
{
	ARG_UNUSED(counter_dev);

	k_sem_give(&timer_sem);
}

static int cmd_timer_free_running(const struct shell *shctx, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	int err = 0;
	const struct device *timer_dev;

	timer_dev = device_get_binding(argv[ARGV_DEV]);
	if (!timer_dev) {
		shell_error(shctx, "Timer: Device %s not found", argv[ARGV_DEV]);
		return -ENODEV;
	}

	/* start timer in free running mode */
	err = counter_start(timer_dev);
	if (err != 0) {
		shell_error(shctx, "%s is not available err:%d", argv[ARGV_DEV], err);
		return err;
	}

	shell_info(shctx, "%s: Timer is freerunning", argv[ARGV_DEV]);

	return  0;
}

static int cmd_timer_stop(const struct shell *shctx, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	uint32_t ticks1 = 0, ticks2 = 0;
	const struct device *timer_dev;

	timer_dev = device_get_binding(argv[ARGV_DEV]);
	if (!timer_dev) {
		shell_error(shctx, "Timer: Device %s not found", argv[ARGV_DEV]);
		return -ENODEV;
	}

	counter_stop(timer_dev);

	counter_get_value(timer_dev, &ticks1);
	counter_get_value(timer_dev, &ticks2);

	if (ticks1 == ticks2) {
		shell_info(shctx, "Timer Stopped");
	} else {
		shell_error(shctx, "Failed to stop timer");
		return -EIO;
	}

	return 0;
}

static int cmd_timer_oneshot(const struct shell *shctx, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	char *endptr;
	int32_t err = 0;
	unsigned long delay = 0;
	unsigned long channel = 0;
	const struct device *timer_dev;
	struct counter_alarm_cfg alarm_cfg;

	k_sem_init(&timer_sem, 0, 1);

	timer_dev = device_get_binding(argv[ARGV_DEV]);
	if (!timer_dev) {
		shell_error(shctx, "Timer: Device %s not found", argv[ARGV_DEV]);
		return -ENODEV;
	}

	errno = 0;
	delay = strtoul(argv[ARGV_ONESHOT_TIME], &endptr, 10);
	if (delay > MAX_DELAY) {
		shell_error(shctx, "delay:%ld greater than max delay:%d", delay, MAX_DELAY);
		return -EINVAL;
	} else if (*endptr || (delay == LONG_MAX && errno)) {
		shell_error(shctx, "delay:%ld out of range, Max Delay: %d", delay, MAX_DELAY);
		return -EINVAL;
	}

	errno = 0;
	channel = strtoul(argv[ARGV_CHN], &endptr, 10);
	if (channel > MAX_CHANNEL) {
		shell_error(shctx, "Channel :%ld greater than max allowed channel:%d",
				channel, MAX_CHANNEL);
		return -EINVAL;
	} else if (*endptr || (channel == LONG_MAX && errno)) {
		shell_error(shctx, "Channel:%ld out of range, Max Channel:%d",
				channel, MAX_CHANNEL);
		return -EINVAL;
	}

	alarm_cfg.flags = 0;
	alarm_cfg.ticks = counter_us_to_ticks(timer_dev, (uint64_t)delay);
	alarm_cfg.callback = timer_alarm_handler;

	/* set an alarm */
	err = counter_set_channel_alarm(timer_dev, (uint8_t)channel, &alarm_cfg);
	if (err != 0) {
		shell_error(shctx, "%s:Failed to set channel alarm, err:%d", argv[ARGV_DEV], err);
		return err;
	}

	k_sem_take(&timer_sem, K_FOREVER);

	shell_info(shctx, "%s: Alarm triggered", argv[ARGV_DEV]);

	return 0;
}

static int cmd_timer_periodic(const struct shell *shctx, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	char *endptr;
	uint32_t count = 0;
	int32_t err = 0;
	unsigned long delay = 0;
	const struct device *timer_dev;
	struct counter_top_cfg top_cfg;

	k_sem_init(&timer_sem, 0, 1);

	timer_dev = device_get_binding(argv[ARGV_DEV]);
	if (!timer_dev) {
		shell_error(shctx, "Timer: Device %s not found", argv[ARGV_DEV]);
		return -ENODEV;
	}

	errno = 0;
	delay = strtoul(argv[ARGV_PERIODIC_TIME], &endptr, 10);
	if (delay > MAX_DELAY) {
		shell_error(shctx, "delay:%ld greater than max delay:%d", delay, MAX_DELAY);
	} else if (*endptr || (delay == LONG_MAX && errno)) {
		shell_error(shctx, "delay:%ld out of range, Max Delay: %d", delay, MAX_DELAY);
		return -EINVAL;
	}

	top_cfg.flags = 0;
	top_cfg.ticks = counter_us_to_ticks(timer_dev, (uint64_t)delay);
	/* interrupt will be triggered periodically */
	top_cfg.callback = timer_top_handler;

	/* set top value */
	err = counter_set_top_value(timer_dev, &top_cfg);
	if (err != 0) {
		shell_error(shctx, "%s: failed to set top value, err: %d", argv[ARGV_DEV], err);
		return err;
	}

	/* Checking periodic interrupt for PERIODIC_CYCLES times and then unblocking shell.
	 * Timer is still running and interrupt is triggered periodically.
	 */
	while (++count < PERIODIC_CYCLES) {
		k_sem_take(&timer_sem, K_FOREVER);
	}

	shell_info(shctx, "%s: periodic timer triggered for %d times", argv[ARGV_DEV], count);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_timer,
			SHELL_CMD_ARG(periodic, NULL,
			"timer periodic <timer_instance_node_id> <time_in_us>",
			cmd_timer_periodic, 3, 0),
			SHELL_CMD_ARG(oneshot, NULL,
			"timer oneshot <timer_instance_node_id> <channel_id> <time_in_us>",
			cmd_timer_oneshot, 4, 0),
			SHELL_CMD_ARG(freerun, NULL,
			"timer freerun <timer_instance_node_id>",
			cmd_timer_free_running, 2, 0),
			SHELL_CMD_ARG(stop, NULL,
			"timer stop <timer_instance_node_id>",
			cmd_timer_stop, 2, 0),
			SHELL_SUBCMD_SET_END /* array terminated. */
			);

SHELL_CMD_REGISTER(timer, &sub_timer, "Timer commands", NULL);
