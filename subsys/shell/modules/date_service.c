/*
 * Copyright (c) 2020 Nick Ward
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <zephyr/init.h>
#include <string.h>

#include <zephyr/sys/timeutil.h>

#if defined(CONFIG_ARCH_POSIX) && defined(CONFIG_EXTERNAL_LIBC)
#include <time.h>
#else
#include <zephyr/posix/time.h>
#endif

#define HELP_NONE      "[none]"
#define HELP_DATE_SET  "[Y-m-d] <H:M:S>"

static void date_print(const struct shell *sh, struct tm *t)
{
	shell_print(sh,
		    "%d-%02u-%02u "
		    "%02u:%02u:%02u UTC",
		    t->tm_year + 1900,
		    t->tm_mon + 1,
		    t->tm_mday,
		    t->tm_hour,
		    t->tm_min,
		    t->tm_sec);
}

static int get_y_m_d(const struct shell *sh, struct tm *t, char *date_str)
{
	int year;
	int month;
	int day;
	char *endptr;

	endptr = NULL;
	year = strtol(date_str, &endptr, 10);
	if ((endptr == date_str) || (*endptr != '-')) {
		return -EINVAL;
	}

	date_str = endptr + 1;

	endptr = NULL;
	month = strtol(date_str, &endptr, 10);
	if ((endptr == date_str) || (*endptr != '-')) {
		return -EINVAL;
	}

	if ((month < 1) || (month > 12)) {
		shell_error(sh, "Invalid month");
		return -EINVAL;
	}

	date_str = endptr + 1;

	endptr = NULL;
	day = strtol(date_str, &endptr, 10);
	if ((endptr == date_str) || (*endptr != '\0')) {
		return -EINVAL;
	}

	/* Check day against maximum month length */
	if ((day < 1) || (day > 31)) {
		shell_error(sh, "Invalid day");
		return -EINVAL;
	}

	t->tm_year = year - 1900;
	t->tm_mon = month - 1;
	t->tm_mday = day;

	return 0;
}

/*
 * For user convenience of small adjustments to time the time argument will
 * accept H:M:S, :M:S or ::S where the missing field(s) will be filled in by
 * the previous time state.
 */
static int get_h_m_s(const struct shell *sh, struct tm *t, char *time_str)
{
	char *endptr;

	if (*time_str == ':') {
		time_str++;
	} else {
		endptr = NULL;
		t->tm_hour = strtol(time_str, &endptr, 10);
		if (endptr == time_str) {
			return -EINVAL;
		} else if (*endptr == ':') {
			if ((t->tm_hour < 0) || (t->tm_hour > 23)) {
				shell_error(sh, "Invalid hour");
				return -EINVAL;
			}

			time_str = endptr + 1;
		} else {
			return -EINVAL;
		}
	}

	if (*time_str == ':') {
		time_str++;
	} else {
		endptr = NULL;
		t->tm_min = strtol(time_str, &endptr, 10);
		if (endptr == time_str) {
			return -EINVAL;
		} else if (*endptr == ':') {
			if ((t->tm_min < 0) || (t->tm_min > 59)) {
				shell_error(sh, "Invalid minute");
				return -EINVAL;
			}

			time_str = endptr + 1;
		} else {
			return -EINVAL;
		}
	}

	endptr = NULL;
	t->tm_sec = strtol(time_str, &endptr, 10);
	if ((endptr == time_str) || (*endptr != '\0')) {
		return -EINVAL;
	}

	/* Note range allows for a leap second */
	if ((t->tm_sec < 0) || (t->tm_sec > 60)) {
		shell_error(sh, "Invalid second");
		return -EINVAL;
	}

	return 0;
}

static int cmd_date_set(const struct shell *sh, size_t argc, char **argv)
{
	struct timespec tp;
	struct tm tm;
	int ret;

	clock_gettime(CLOCK_REALTIME, &tp);

	gmtime_r(&tp.tv_sec, &tm);

	if (argc == 3) {
		ret = get_y_m_d(sh, &tm, argv[1]);
		if (ret != 0) {
			shell_help(sh);
			return -EINVAL;
		}
		ret = get_h_m_s(sh, &tm, argv[2]);
		if (ret != 0) {
			shell_help(sh);
			return -EINVAL;
		}
	} else if (argc == 2) {
		ret = get_h_m_s(sh, &tm, argv[1]);
		if (ret != 0) {
			shell_help(sh);
			return -EINVAL;
		}
	} else {
		shell_help(sh);
		return -EINVAL;
	}

	tp.tv_sec = timeutil_timegm(&tm);
	if (tp.tv_sec == -1) {
		shell_error(sh, "Failed to calculate seconds since Epoch");
		return -EINVAL;
	}
	tp.tv_nsec = 0;

	ret = clock_settime(CLOCK_REALTIME, &tp);
	if (ret != 0) {
		shell_error(sh, "Could not set date %d", ret);
		return -EINVAL;
	}

	date_print(sh, &tm);

	return 0;
}

static int cmd_date_get(const struct shell *sh, size_t argc, char **argv)
{
	struct timespec tp;
	struct tm tm;

	clock_gettime(CLOCK_REALTIME, &tp);

	gmtime_r(&tp.tv_sec, &tm);

	date_print(sh, &tm);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_date,
	SHELL_CMD(set, NULL, HELP_DATE_SET, cmd_date_set),
	SHELL_CMD(get, NULL, HELP_NONE, cmd_date_get),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(date, &sub_date, "Date commands", NULL);
