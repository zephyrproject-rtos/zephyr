/*
 * Copyright (c) 2023, Prevas A/S <kim.bondergaard@prevas.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

/* _POSIX_C_SOURCE is required to expose gmtime_r declaration in glibc headers */
#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/sys/timeutil.h>
#include <time.h>
#include <stdlib.h>

/* Formats accepted when setting date and/or time */
static const char format_iso8601[] = "%FT%T";
static const char format_time[] = "%T";  /* hh:mm:ss */
static const char format_date[] = " %F"; /* yyyy-mm-dd */

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

static const char *get_rtc_format(const char *str)
{
	if (strchr(str, 'T')) {
		return format_iso8601;
	} else if (strchr(str, '-')) {
		return format_date;
	} else {
		return format_time;
	}
}

static int parse_rtc_time(const char *str, struct tm *tm_time)
{
	const char *format = get_rtc_format(str);

	char *parse_res = strptime(str, format, tm_time);

	if (!parse_res || *parse_res != '\0') {
		return -EINVAL;
	}

	return 0;
}

static int derive_wday(const char *str, struct tm *tm_time)
{
	const char *format = get_rtc_format(str);

	if (format != format_iso8601 && format != format_date) {
		return 0;
	}

	struct tm *volatile local_tm = tm_time;
	time_t t = timeutil_timegm(local_tm);

	if (t == (time_t)-1) {
		return -EINVAL;
	}

	struct tm tmp;

	gmtime_r(&t, &tmp);
	local_tm->tm_wday = tmp.tm_wday;

	return 0;
}

#ifdef CONFIG_RTC_ALARM
static int parse_uint16(const struct shell *sh, const char *str, uint16_t *out)
{
	char *endptr;
	unsigned long val = strtoul(str, &endptr, 0);

	if (endptr == str || *endptr != '\0') {
		shell_error(sh, "Invalid value '%s'", str);
		return -EINVAL;
	}

	if (val > UINT16_MAX) {
		shell_error(sh, "Value '%s' out of range", str);
		return -EINVAL;
	}
	*out = (uint16_t)val;

	return 0;
}

static int cmd_set_alarm(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = shell_device_get_binding(argv[1]);
	int res;
	uint16_t id, mask, mask_supported;

	if (!device_is_ready(dev)) {
		shell_error(sh, "Device %s not ready", argv[1]);
		return -ENODEV;
	}

	struct rtc_time rtctime = {0};
	struct tm *tm_time = rtc_time_to_tm(&rtctime);

	if (parse_uint16(sh, argv[2], &id) != 0 || parse_uint16(sh, argv[3], &mask) != 0) {
		return -EINVAL;
	}

	if (parse_rtc_time(argv[4], tm_time) != 0) {
		shell_error(sh, "Error in argument format");
		return -EINVAL;
	}

	res = rtc_alarm_get_supported_fields(dev, id, &mask_supported);
	if (res < 0) {
		switch (res) {
		case -EINVAL:
			shell_error(sh, "Invalid alarm id");
			break;
		case -ENOTSUP:
			shell_error(sh, "Alarm not supported by hardware");
			break;
		default:
			shell_error(sh, "Failed to get supported fields: %d", res);
			break;
		}
		return res;
	}

	if (mask & ~mask_supported) {
		shell_error(sh, "Unsupported alarm mask: 0x%04x (supported: 0x%04x)", mask,
			    mask_supported);
		return -EINVAL;
	}

	if (mask & RTC_ALARM_TIME_MASK_WEEKDAY) {
		/* Derive weekday from parsed datetime */
		if (derive_wday(argv[4], tm_time) != 0) {
			shell_error(sh, "Error in time");
			return -EINVAL;
		}
	}

	res = rtc_alarm_set_time(dev, id, mask, &rtctime);
	if (res < 0) {
		switch (res) {
		case -EINVAL:
			shell_error(sh, "Invalid alarm id or time is invalid");
			break;
		case -ENOTSUP:
			shell_error(sh, "Alarm not supported by hardware");
			break;
		default:
			shell_error(sh, "Failed to set alarm: %d", res);
			break;
		}
		return res;
	}

	return res;
}

static int cmd_get_alarm(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = shell_device_get_binding(argv[1]);
	int res;
	uint16_t id, mask;

	if (!device_is_ready(dev)) {
		shell_error(sh, "device %s not ready", argv[1]);
		return -ENODEV;
	}

	if (parse_uint16(sh, argv[2], &id) != 0) {
		return -EINVAL;
	}

	struct rtc_time rtctime = {0};

	res = rtc_alarm_get_time(dev, id, &mask, &rtctime);
	if (res < 0) {
		switch (res) {
		case -EINVAL:
			shell_error(sh, "Invalid alarm id");
			break;
		case -ENOTSUP:
			shell_error(sh, "Alarm not supported by hardware");
			break;
		default:
			shell_error(sh, "Failed to get alarm: %d", res);
			break;
		}
		return res;
	}

	struct tm *tm_time = rtc_time_to_tm(&rtctime);

	shell_print(sh, "Alarm %d: %04d-%02d-%02dT%02d:%02d:%02d (mask=0x%x)", id,
		    tm_time->tm_year + 1900, tm_time->tm_mon + 1, tm_time->tm_mday,
		    tm_time->tm_hour, tm_time->tm_min, tm_time->tm_sec, mask);

	return 0;
}

static int cmd_get_alarm_mask(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = shell_device_get_binding(argv[1]);
	int res;
	uint16_t id, mask;

	if (!device_is_ready(dev)) {
		shell_error(sh, "device %s not ready", argv[1]);
		return -ENODEV;
	}

	if (parse_uint16(sh, argv[2], &id) != 0) {
		return -EINVAL;
	}

	res = rtc_alarm_get_supported_fields(dev, id, &mask);
	if (res < 0) {
		switch (res) {
		case -EINVAL:
			shell_error(sh, "Invalid alarm id");
			break;
		case -ENOTSUP:
			shell_error(sh, "Alarm not supported by hardware");
			break;
		default:
			shell_error(sh, "Failed to get supported fields: %d", res);
			break;
		}
		return res;
	}

	shell_print(sh, "Supported mask: 0x%04x", mask);

	if (mask & RTC_ALARM_TIME_MASK_SECOND) {
		shell_print(sh, "  SECOND");
	}
	if (mask & RTC_ALARM_TIME_MASK_MINUTE) {
		shell_print(sh, "  MINUTE");
	}
	if (mask & RTC_ALARM_TIME_MASK_HOUR) {
		shell_print(sh, "  HOUR");
	}
	if (mask & RTC_ALARM_TIME_MASK_MONTHDAY) {
		shell_print(sh, "  MONTHDAY");
	}
	if (mask & RTC_ALARM_TIME_MASK_MONTH) {
		shell_print(sh, "  MONTH");
	}
	if (mask & RTC_ALARM_TIME_MASK_YEAR) {
		shell_print(sh, "  YEAR");
	}
	if (mask & RTC_ALARM_TIME_MASK_WEEKDAY) {
		shell_print(sh, "  WEEKDAY");
	}
	if (mask & RTC_ALARM_TIME_MASK_YEARDAY) {
		shell_print(sh, "  YEARDAY");
	}
	if (mask & RTC_ALARM_TIME_MASK_NSEC) {
		shell_print(sh, "  NSEC");
	}

	return 0;
}
#endif /* CONFIG_RTC_ALARM */

static int cmd_set(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = shell_device_get_binding(argv[1]);

	if (!device_is_ready(dev)) {
		shell_error(sh, "device %s not ready", argv[1]);
		return -ENODEV;
	}

	argc--;
	argv++;

	struct rtc_time rtctime = {0};
	struct tm *tm_time = rtc_time_to_tm(&rtctime);

	(void)rtc_get_time(dev, &rtctime);

	if (parse_rtc_time(argv[1], tm_time) != 0) {
		shell_error(sh, "Error in argument format");
		return -EINVAL;
	}

	/* Derive weekday from parsed datetime */
	if (derive_wday(argv[1], tm_time) != 0) {
		shell_error(sh, "Error in time");
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
	const struct device *dev = shell_device_get_binding(argv[1]);

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

	shell_print(sh, "%04d-%02d-%02dT%02d:%02d:%02d.%03d", rtctime.tm_year + 1900,
		    rtctime.tm_mon + 1, rtctime.tm_mday, rtctime.tm_hour, rtctime.tm_min,
		    rtctime.tm_sec, rtctime.tm_nsec / 1000000);

	return 0;
}

#ifdef CONFIG_RTC_CALIBRATION
static int cmd_get_calibration(const struct shell *sh, size_t argc, char **argv)
{
	int res;
	int32_t calibration_ppb;
	const struct device *dev = shell_device_get_binding(argv[1]);

	if (!device_is_ready(dev)) {
		shell_error(sh, "device %s not ready", argv[1]);
		return -ENODEV;
	}

	res = rtc_get_calibration(dev, &calibration_ppb);
	if (-ENOTSUP == res) {
		shell_error(sh, "Calibration not supported");
		return 0;
	}
	if (res < 0) {
		shell_error(sh, "Error getting calibration: %d", res);
		return res;
	}

	shell_print(sh, "%dppb", calibration_ppb);
	return 0;
}

static int cmd_set_calibration(const struct shell *sh, size_t argc, char **argv)
{
	int res;
	char *endptr;
	int32_t calibration_ppb;
	const struct device *dev = shell_device_get_binding(argv[1]);

	if (!device_is_ready(dev)) {
		shell_error(sh, "device %s not ready", argv[1]);
		return -ENODEV;
	}

	calibration_ppb = strtol(argv[2], &endptr, 10);
	if (*endptr != '\0') {
		return -EINVAL;
	}
	if (calibration_ppb > 1000000 || calibration_ppb < -1000000) {
		return -EINVAL;
	}

	res = rtc_set_calibration(dev, calibration_ppb);
	if (-ENOTSUP == res) {
		shell_error(sh, "Calibration not supported");
		return 0;
	}
	if (res < 0) {
		shell_error(sh, "Error setting calibration: %d", res);
	}

	return res;
}
#endif /* CONFIG_RTC_CALIBRATION */

static bool device_is_rtc(const struct device *dev)
{
	return DEVICE_API_IS(rtc, dev);
}

static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_filter(idx, device_is_rtc);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

#define RTC_GET_HELP                                                                               \
	SHELL_HELP("Get current time (UTC)",                                                       \
		   "<device>")

#define RTC_SET_HELP                                                                               \
	SHELL_HELP("Set UTC time",                                                                 \
		   "<device> <YYYY-MM-DDThh:mm:ss> | <YYYY-MM-DD> | <hh:mm:ss>")

#ifdef CONFIG_RTC_ALARM
#define RTC_SET_ALARM_HELP                                                                         \
	SHELL_HELP("Set RTC alarm",                                                                \
		   "<device> <id> <mask> <YYYY-MM-DDThh:mm:ss> | <YYYY-MM-DD> | <hh:mm:ss>")
#define RTC_GET_ALARM_HELP      SHELL_HELP("Get RTC alarm", "<device> <id>")
#define RTC_GET_ALARM_MASK_HELP SHELL_HELP("Get supported alarm fields", "<device> <id>")
#endif /* CONFIG_RTC_ALARM */

#define RTC_GET_CALIBRATION_HELP SHELL_HELP("Get calibration", "<device>")
#define RTC_SET_CALIBRATION_HELP SHELL_HELP("Set calibration", "<device> <ppb>")

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_rtc, SHELL_CMD_ARG(set, &dsub_device_name, RTC_SET_HELP, cmd_set, 3, 0),
	SHELL_CMD_ARG(get, &dsub_device_name, RTC_GET_HELP, cmd_get, 2, 0),
#ifdef CONFIG_RTC_ALARM
	SHELL_CMD_ARG(set_alarm, &dsub_device_name, RTC_SET_ALARM_HELP, cmd_set_alarm, 5, 0),
	SHELL_CMD_ARG(get_alarm, &dsub_device_name, RTC_GET_ALARM_HELP, cmd_get_alarm, 3, 0),
	SHELL_CMD_ARG(get_alarm_mask, &dsub_device_name, RTC_GET_ALARM_MASK_HELP,
		      cmd_get_alarm_mask, 3, 0),
#endif /* CONFIG_RTC_ALARM */
#ifdef CONFIG_RTC_CALIBRATION
	SHELL_CMD_ARG(get_calibration, &dsub_device_name, RTC_GET_CALIBRATION_HELP,
		      cmd_get_calibration, 2, 0),
	SHELL_CMD_ARG(set_calibration, &dsub_device_name, RTC_SET_CALIBRATION_HELP,
		      cmd_set_calibration, 3, 0),
#endif /* CONFIG_RTC_CALIBRATION */
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(rtc, &sub_rtc, "RTC commands", NULL);
