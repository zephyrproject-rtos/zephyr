/*
 * Copyright (c) 2023, Prevas A/S <kim.bondergaard@prevas.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/drivers/rtc.h>
#include <time.h>
#include <stdlib.h>

/* Formats accepted when setting date and/or time */
static const char format_iso8601[] = "%FT%T";
static const char format_time[] = "%T";  /* hh:mm:ss */
static const char format_date[] = " %F"; /* yyyy-mm-dd */

#if !defined CONFIG_BOARD_NATIVE_POSIX

static const char *consume_chars(const char *s, char *dest, unsigned int cnt)
{
	if (strlen(s) < cnt) {
		return NULL;
	}

	memcpy(dest, s, cnt);
	dest[cnt] = '\0';

	return s + cnt;
}

static const char *consume_char(const char *s, char ch)
{
	if (*s != ch) {
		return NULL;
	}
	return ++s;
}

static const char *consume_date(const char *s, struct tm *tm_time)
{
	char year[4 + 1];
	char month[2 + 1];
	char day[2 + 1];

	s = consume_chars(s, year, 4);
	if (!s) {
		return NULL;
	}

	s = consume_char(s, '-');
	if (!s) {
		return NULL;
	}

	s = consume_chars(s, month, 2);
	if (!s) {
		return NULL;
	}

	s = consume_char(s, '-');
	if (!s) {
		return NULL;
	}

	s = consume_chars(s, day, 2);
	if (!s) {
		return NULL;
	}

	tm_time->tm_year = atoi(year) - 1900;
	tm_time->tm_mon = atoi(month) - 1;
	tm_time->tm_mday = atoi(day);

	return s;
}

static const char *consume_time(const char *s, struct tm *tm_time)
{
	char hour[2 + 1];
	char minute[2 + 1];
	char second[2 + 1];

	s = consume_chars(s, hour, 2);
	if (!s) {
		return NULL;
	}

	s = consume_char(s, ':');
	if (!s) {
		return NULL;
	}

	s = consume_chars(s, minute, 2);
	if (!s) {
		return NULL;
	}

	s = consume_char(s, ':');
	if (!s) {
		return NULL;
	}

	s = consume_chars(s, second, 2);
	if (!s) {
		return NULL;
	}

	tm_time->tm_hour = atoi(hour);
	tm_time->tm_min = atoi(minute);
	tm_time->tm_sec = atoi(second);

	return s;
}

static char *strptime(const char *s, const char *format, struct tm *tm_time)
{
	/* Reduced implementation of strptime -
	 * accepting only the 3 different format strings
	 */
	if (!strcmp(format, format_iso8601)) {
		s = consume_date(s, tm_time);
		if (!s) {
			return NULL;
		}

		s = consume_char(s, 'T');
		if (!s) {
			return NULL;
		}

		s = consume_time(s, tm_time);
		if (!s) {
			return NULL;
		}

		return (char *)s;

	} else if (!strcmp(format, format_time)) {
		return (char *)consume_time(s, tm_time);

	} else if (!strcmp(format, format_date)) {
		return (char *)consume_date(s, tm_time);

	} else {
		return NULL;
	}
}

#endif

static int cmd_set(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = device_get_binding(argv[1]);

	if (!device_is_ready(dev)) {
		shell_error(sh, "device %s not ready", argv[1]);
		return -ENODEV;
	}

	argc--;
	argv++;

	struct rtc_time rtctime = {0};
	struct tm *tm_time = rtc_time_to_tm(&rtctime);

	(void)rtc_get_time(dev, &rtctime);

	const char *format;

	if (strchr(argv[1], 'T')) {
		format = format_iso8601;
	} else if (strchr(argv[1], '-')) {
		format = format_date;
	} else {
		format = format_time;
	}

	char *parseRes = strptime(argv[1], format, tm_time);

	if (!parseRes || *parseRes != '\0') {
		shell_error(sh, "Error in argument format");
		return -EINVAL;
	}

	int res = rtc_set_time(dev, &rtctime);

	if (-EINVAL == res) {
		shell_error(sh, "error in time");
		return -EINVAL;
	}
	return res;
}

static int cmd_get(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = device_get_binding(argv[1]);

	if (!device_is_ready(dev)) {
		shell_error(sh, "device %s not ready", argv[1]);
		return -ENODEV;
	}

	struct rtc_time rtctime;

	int res = rtc_get_time(dev, &rtctime);

	if (-ENODATA == res) {
		shell_print(sh, "RTC not set");
		return 0;
	}
	if (res < 0) {
		return res;
	}

	shell_print(sh, "%04d-%02d-%02dT%02d:%02d:%02d:%06d", rtctime.tm_year + 1900,
		    rtctime.tm_mon + 1, rtctime.tm_mday, rtctime.tm_hour, rtctime.tm_min,
		    rtctime.tm_sec, rtctime.tm_nsec / 1000000);

	return 0;
}

#define RTC_GET_HELP                                                                               \
	("Get current time (UTC)\n"                                                                \
	 "Usage: rtc get <device>")

#define RTC_SET_HELP                                                                               \
	("Set UTC time\n"                                                                          \
	 "Usage: rtc set <device> <YYYY-MM-DDThh:mm:ss> | <YYYY-MM-DD> | <hh:mm:ss>")

SHELL_STATIC_SUBCMD_SET_CREATE(sub_rtc,
			       /* Alphabetically sorted */
			       SHELL_CMD_ARG(set, NULL, RTC_SET_HELP, cmd_set, 3, 0),
			       SHELL_CMD_ARG(get, NULL, RTC_GET_HELP, cmd_get, 2, 0),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(rtc, &sub_rtc, "RTC commands", NULL);
