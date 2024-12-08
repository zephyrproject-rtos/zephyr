/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel_shell.h"

#include <zephyr/kernel.h>

#define MINUTES_FACTOR (MSEC_PER_SEC * SEC_PER_MIN)
#define HOURS_FACTOR   (MINUTES_FACTOR * MIN_PER_HOUR)
#define DAYS_FACTOR    (HOURS_FACTOR * HOUR_PER_DAY)

static int cmd_kernel_uptime(const struct shell *sh, size_t argc, char **argv)
{
	int64_t milliseconds = k_uptime_get();
	int64_t days;
	int64_t hours;
	int64_t minutes;
	int64_t seconds;

	if (argc == 1) {
		shell_print(sh, "Uptime: %llu ms", milliseconds);
		return 0;
	}

	/* No need to enable the getopt and getopt_long for just one option. */
	if (strcmp("-p", argv[1]) && strcmp("--pretty", argv[1]) != 0) {
		shell_error(sh, "Unsupported option: %s", argv[1]);
		return -EIO;
	}

	days = milliseconds / DAYS_FACTOR;
	milliseconds %= DAYS_FACTOR;
	hours = milliseconds / HOURS_FACTOR;
	milliseconds %= HOURS_FACTOR;
	minutes = milliseconds / MINUTES_FACTOR;
	milliseconds %= MINUTES_FACTOR;
	seconds = milliseconds / MSEC_PER_SEC;
	milliseconds = milliseconds % MSEC_PER_SEC;

	shell_print(sh,
		    "uptime: %llu days, %llu hours, %llu minutes, %llu seconds, %llu milliseconds",
		    days, hours, minutes, seconds, milliseconds);

	return 0;
}

KERNEL_CMD_ARG_ADD(uptime, NULL, "Kernel uptime. Can be called with the -p or --pretty options",
		   cmd_kernel_uptime, 1, 1);
