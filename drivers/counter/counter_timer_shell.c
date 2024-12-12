/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/counter.h>
#include <zephyr/shell/shell.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#define ARGV_DEV           1
#define ARGV_CHN           2
#define ARGV_PERIODIC_TIME 2
#define ARGV_ONESHOT_TIME  3

/* number of periodic interrupts */
#define PERIODIC_CYCLES    10
#define MAX_DELAY          UINT32_MAX
#define MAX_CHANNEL        255U

static struct k_sem timer_sem;

static void timer_top_handler(const struct device *counter_dev, void *user_data)
{
	ARG_UNUSED(counter_dev);

	k_sem_give(&timer_sem);
}

static void timer_alarm_handler(const struct device *counter_dev, uint8_t chan_id,
				uint32_t ticks, void *user_data)
{
	ARG_UNUSED(counter_dev);

	k_sem_give(&timer_sem);
}

static inline int parse_device(const struct shell *shctx, size_t argc, char *argv[],
			       const struct device **timer_dev)
{
	ARG_UNUSED(argc);

	*timer_dev = shell_device_get_binding(argv[ARGV_DEV]);
	if (*timer_dev == NULL) {
		shell_error(shctx, "Timer: Device %s not found", argv[ARGV_DEV]);
		return -ENODEV;
	}

	return 0;
}

static int cmd_timer_free_running(const struct shell *shctx, size_t argc, char **argv)
{
	const struct device *timer_dev;
	int err = parse_device(shctx, argc, argv, &timer_dev);

	if (err != 0) {
		return err;
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
	uint32_t ticks1 = 0, ticks2 = 0;
	const struct device *timer_dev;
	int err = parse_device(shctx, argc, argv, &timer_dev);

	if (err != 0) {
		return err;
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
	unsigned long delay = 0;
	unsigned long channel = 0;
	const struct device *timer_dev;
	int err = parse_device(shctx, argc, argv, &timer_dev);
	struct counter_alarm_cfg alarm_cfg;

	k_sem_init(&timer_sem, 0, 1);

	if (err != 0) {
		return err;
	}

	delay = shell_strtoul(argv[ARGV_ONESHOT_TIME], 10, &err);
	if (err != 0) {
		shell_error(shctx, "invalid delay parameter");
		return err;
	} else if (delay > MAX_DELAY) {
		shell_error(shctx, "delay out of range");
		return -ERANGE;
	}

	channel = shell_strtoul(argv[ARGV_CHN], 10, &err);
	if (err != 0) {
		shell_error(shctx, "invalid channel parameter");
		return err;
	} else if (channel > MAX_CHANNEL) {
		shell_error(shctx, "channel out of range");
		return -ERANGE;
	}

	alarm_cfg.flags = 0;
	alarm_cfg.ticks = counter_us_to_ticks(timer_dev, (uint64_t)delay);
	alarm_cfg.callback = timer_alarm_handler;
	alarm_cfg.user_data = NULL;

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
	uint32_t count = 0;
	unsigned long delay = 0;
	const struct device *timer_dev;
	int err = parse_device(shctx, argc, argv, &timer_dev);
	struct counter_top_cfg top_cfg;

	k_sem_init(&timer_sem, 0, 1);

	if (err != 0) {
		return err;
	}

	delay = shell_strtoul(argv[ARGV_PERIODIC_TIME], 10, &err);
	if (err != 0) {
		shell_error(shctx, "invalid delay parameter");
		return err;
	} else if (delay > MAX_DELAY) {
		shell_error(shctx, "delay out of range");
		return -ERANGE;
	}

	top_cfg.flags = 0;
	top_cfg.ticks = counter_us_to_ticks(timer_dev, (uint64_t)delay);
	/* interrupt will be triggered periodically */
	top_cfg.callback = timer_top_handler;
	top_cfg.user_data = NULL;

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

/* Device name autocompletion support */
static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_lookup(idx, "timer");

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_timer,
			SHELL_CMD_ARG(periodic, &dsub_device_name,
			"timer periodic <timer_instance_node_id> <time_in_us>",
			cmd_timer_periodic, 3, 0),
			SHELL_CMD_ARG(oneshot, &dsub_device_name,
			"timer oneshot <timer_instance_node_id> <channel_id> <time_in_us>",
			cmd_timer_oneshot, 4, 0),
			SHELL_CMD_ARG(freerun, &dsub_device_name,
			"timer freerun <timer_instance_node_id>",
			cmd_timer_free_running, 2, 0),
			SHELL_CMD_ARG(stop, &dsub_device_name,
			"timer stop <timer_instance_node_id>",
			cmd_timer_stop, 2, 0),
			SHELL_SUBCMD_SET_END /* array terminated. */
			);

SHELL_CMD_REGISTER(timer, &sub_timer, "Timer commands", NULL);
