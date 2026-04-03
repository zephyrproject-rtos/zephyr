/*
 * Copyright (c) 2023 Bjarki Arge Andreasen
 * Copyright (c) 2024 Andrew Featherstone
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/drivers/rtc.h>

#include "rtc_utils.h"

bool rtc_utils_validate_rtc_time(const struct rtc_time *timeptr, uint16_t mask)
{
	if ((mask & RTC_ALARM_TIME_MASK_SECOND) && (timeptr->tm_sec < 0 || timeptr->tm_sec > 59)) {
		return false;
	}

	if ((mask & RTC_ALARM_TIME_MASK_MINUTE) && (timeptr->tm_min < 0 || timeptr->tm_min > 59)) {
		return false;
	}

	if ((mask & RTC_ALARM_TIME_MASK_HOUR) && (timeptr->tm_hour < 0 || timeptr->tm_hour > 23)) {
		return false;
	}

	if ((mask & RTC_ALARM_TIME_MASK_MONTH) && (timeptr->tm_mon < 0 || timeptr->tm_mon > 11)) {
		return false;
	}

	if ((mask & RTC_ALARM_TIME_MASK_MONTHDAY) &&
	    (timeptr->tm_mday < 1 || timeptr->tm_mday > 31)) {
		return false;
	}

	if ((mask & RTC_ALARM_TIME_MASK_YEAR) && (timeptr->tm_year < 0 || timeptr->tm_year > 199)) {
		return false;
	}

	if ((mask & RTC_ALARM_TIME_MASK_WEEKDAY) &&
	    (timeptr->tm_wday < 0 || timeptr->tm_wday > 6)) {
		return false;
	}

	if ((mask & RTC_ALARM_TIME_MASK_YEARDAY) &&
	    (timeptr->tm_yday < 0 || timeptr->tm_yday > 365)) {
		return false;
	}

	if ((mask & RTC_ALARM_TIME_MASK_NSEC) &&
	    (timeptr->tm_nsec < 0 || timeptr->tm_nsec > 999999999)) {
		return false;
	}

	return true;
}
