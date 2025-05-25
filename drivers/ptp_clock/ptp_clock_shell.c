/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell.h>
#include <stdlib.h>
#include <zephyr/drivers/ptp_clock.h>
#include <string.h>
#include <zephyr/sys/util.h>
#include <zephyr/devicetree.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ptp_clock_shell, CONFIG_LOG_DEFAULT_LEVEL);

static bool device_is_ptp_clock(const struct device *dev)
{
	return DEVICE_API_IS(ptp_clock, dev);
}

static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_filter(idx, device_is_ptp_clock);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

static int parse_device_arg(const struct shell *sh, char **argv, const struct device **dev)
{
	*dev = shell_device_get_binding(argv[1]);
	if (!*dev) {
		shell_error(sh, "device %s not found", argv[1]);
		return -ENODEV;
	}
	return 0;
}

/* ptp_clock get <device> */
static int cmd_ptp_clock_get(const struct shell *sh, size_t argc, char **argv)
{
	struct net_ptp_time tm = {0};
	const struct device *dev;
	int ret;

	ret = parse_device_arg(sh, argv, &dev);
	if (ret < 0) {
		return ret;
	}

	ret = ptp_clock_get(dev, &tm);
	if (ret < 0) {
		return ret;
	}

	shell_print(sh, "%"PRIu64".%09u", tm.second, tm.nanosecond);

	return 0;
}

/* ptp_clock set <device> <seconds> */
static int cmd_ptp_clock_set(const struct shell *sh, size_t argc, char **argv)
{
	struct net_ptp_time tm = {0};
	const struct device *dev;
	int ret;

	ret = parse_device_arg(sh, argv, &dev);
	if (ret < 0) {
		return ret;
	}

	tm.second = shell_strtoull(argv[2], 10, &ret);
	if (ret < 0) {
		return ret;
	}

	ret = ptp_clock_set(dev, &tm);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

/* ptp_clock adj <device> <seconds> */
static int cmd_ptp_clock_adj(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int adj;
	int ret;

	ret = parse_device_arg(sh, argv, &dev);
	if (ret < 0) {
		return ret;
	}

	adj = shell_strtol(argv[2], 10, &ret);
	if (ret < 0) {
		return ret;
	}

	ret = ptp_clock_adjust(dev, adj);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

/* ptp_clock freq <device> <ppb> */
static int cmd_ptp_clock_freq(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int ppb;
	int ret;

	ret = parse_device_arg(sh, argv, &dev);
	if (ret < 0) {
		return ret;
	}

	ppb = shell_strtol(argv[2], 10, &ret);
	if (ret < 0) {
		return ret;
	}

	ret = ptp_clock_rate_adjust(dev, 1.0 + ((double)ppb / 1000000000.0));
	if (ret < 0) {
		return ret;
	}

	return 0;
}

/* ptp_clock selftest <device> <time> <freq> <delay> <adj> */
static int cmd_ptp_clock_selftest(const struct shell *sh, size_t argc, char **argv)
{
	struct net_ptp_time tm = {0};
	const struct device *dev;
	int freq, delay, adj, ret;
	uint64_t seconds;

	ret = parse_device_arg(sh, argv, &dev);
	if (ret < 0) {
		return ret;
	}

	seconds = shell_strtoull(argv[2], 10, &ret);
	if (ret < 0) {
		return ret;
	}

	freq = shell_strtol(argv[3], 10, &ret);
	if (ret < 0) {
		return ret;
	}

	delay = shell_strtol(argv[4], 10, &ret);
	if (ret < 0) {
		return ret;
	}

	adj = shell_strtol(argv[5], 10, &ret);
	if (ret < 0) {
		return ret;
	}

	/* set 'time' seconds and read back to verify clock setting/getting */
	tm.second = seconds;
	ret = ptp_clock_set(dev, &tm);
	if (ret < 0) {
		shell_print(sh, "failed to set time");
		return ret;
	}
	shell_print(sh, "test1: set time %"PRIu64".%09u", tm.second, tm.nanosecond);

	ret = ptp_clock_get(dev, &tm);
	if (ret < 0) {
		shell_print(sh, "failed to get time");
		return ret;
	}
	shell_print(sh, "  result: read back time %"PRIu64".%09u", tm.second, tm.nanosecond);

	/* set 'freq' with ppb value, sleep 'delay' seconds, and read back time */
	ret = ptp_clock_rate_adjust(dev, 1.0 + ((double)freq / 1000000000.0));
	if (ret < 0) {
		shell_print(sh, "failed to adjust rate");
		return ret;
	}
	shell_print(sh, "test2: adjust frequency %d ppb (ratio %f), delay %d seconds...",
		    freq, 1.0 + ((double)freq / 1000000000.0), delay);

	k_sleep(K_SECONDS(delay));

	ret = ptp_clock_get(dev, &tm);
	if (ret < 0) {
		shell_print(sh, "failed to get time");
		return ret;
	}
	shell_print(sh, "  result: read back time %"PRIu64".%09u", tm.second, tm.nanosecond);

	/* set 'adj' seconds and read back time to verify time adjustment */
	ret = ptp_clock_adjust(dev, adj);
	if (ret < 0) {
		shell_print(sh, "failed to adjtime");
		return ret;
	}
	shell_print(sh, "test3: adjust time %d seconds", adj);

	ret = ptp_clock_get(dev, &tm);
	if (ret < 0) {
		shell_print(sh, "failed to get time");
		return ret;
	}
	shell_print(sh, "  result: read back time %"PRIu64".%09u", tm.second, tm.nanosecond);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_ptp_clock_cmds,
	SHELL_CMD_ARG(get, &dsub_device_name,
		"Get time: get <device>",
		cmd_ptp_clock_get, 2, 0),
	SHELL_CMD_ARG(set, &dsub_device_name,
		"Set time: set <device> <seconds>",
		cmd_ptp_clock_set, 3, 0),
	SHELL_CMD_ARG(adj, &dsub_device_name,
		"Adjust time: adj <device> <seconds>",
		cmd_ptp_clock_adj, 3, 0),
	SHELL_CMD_ARG(freq, &dsub_device_name,
		"Adjust frequency: freq <device> <ppb>",
		cmd_ptp_clock_freq, 3, 0),
	SHELL_CMD_ARG(selftest, &dsub_device_name,
		"selftest <device> <time> <freq> <delay> <adj>\n"
		"The selftest will do following steps:\n"
		"1. set 'time' with seconds and read back to\n"
		"   verify clock setting/getting.\n"
		"2. set 'freq' with ppb value, sleep 'delay' seconds,\n"
		"    and read back time to verify rate adjustment.\n"
		"3. set 'adj' seconds and read back time to\n"
		"   verify time adjustment.\n"
		"Example:\n"
		"   ptp_clock selftest ptp_clock 1000 100000000 10 10",
		cmd_ptp_clock_selftest, 6, 0),
	SHELL_SUBCMD_SET_END     /* Array terminated. */
);

SHELL_CMD_REGISTER(ptp_clock, &sub_ptp_clock_cmds, "PTP clock commands", NULL);
