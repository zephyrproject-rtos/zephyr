/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/sys/util.h>

#include <inc/hw_rtc.h>
#include <inc/hw_memmap.h>
#include <inc/hw_systim.h>

#include <string.h>

const struct device *const rtc = DEVICE_DT_GET(DT_ALIAS(rtc));

struct k_sem alarm_semaphore;
struct k_sem update_semaphore;

#if defined(CONFIG_RTC_ALARM)

int alarm_count;

static void alarm_callback(const struct device *dev, uint16_t id, void *user_data)
{
	int *counter = (int *)user_data;

	(*counter)++;

	k_sem_give(&alarm_semaphore);

	printk("Alarm count: %d\r\n", *counter);
}

#endif /* CONFIG_RTC_ALARM */

#if defined(CONFIG_RTC_UPDATE)

int update_count;

static void update_callback(const struct device *dev, void *user_data)
{
	int *counter = (int *)user_data;

	(*counter)++;

	k_sem_give(&update_semaphore);

	printk("Update count: %d\r\n", *counter);
}

#endif /* CONFIG_RTC_UPDATE */

#define TIME(year, month, mday, hour, minute, second)                                              \
	{                                                                                          \
		.tm_year = year - 1900,                                                            \
		.tm_mon = month - 1,                                                               \
		.tm_mday = mday,                                                                   \
		.tm_hour = hour,                                                                   \
		.tm_min = minute,                                                                  \
		.tm_sec = second,                                                                  \
	}

#define PRINT_TIME(x)                                                                              \
	printf("RTC date and time: %04d-%02d-%02d %02d:%02d:%02d\r\n", x.tm_year + 1900,           \
	       x.tm_mon + 1, x.tm_mday, x.tm_hour, x.tm_min, x.tm_sec);

static __attribute__((used)) void test_set_get_time(const struct device *rtc)
{
	int ret = 0;
	int expected_seconds = 0;

	struct rtc_time read_time;

	struct rtc_time test_time[] = {
		TIME(2024, 11, 17, 1, 15, 0),
		TIME(2024, 11, 17, 15, 30, 15),
	};

	for (int idx = 0; idx < ARRAY_SIZE(test_time); idx++) {
		printf("[%d/%d] Set: ", idx + 1, (int)ARRAY_SIZE(test_time));
		PRINT_TIME(test_time[idx]);

		ret = rtc_set_time(rtc, &test_time[idx]);

		if (ret < 0) {
			printf("  FAIL: cannot set RTC time (reason: %d)\r\n", ret);
			continue;
		}

		ret = rtc_get_time(rtc, &read_time);

		if (ret < 0) {
			printf("  FAIL: cannot get RTC time (reason: %d)\r\n", ret);
			continue;
		}

		if ((read_time.tm_sec != test_time[idx].tm_sec) ||
		    (read_time.tm_min != test_time[idx].tm_min) ||
		    (read_time.tm_hour != test_time[idx].tm_hour) ||
		    (read_time.tm_mday != test_time[idx].tm_mday) ||
		    (read_time.tm_mon != test_time[idx].tm_mon) ||
		    (read_time.tm_year != test_time[idx].tm_year)) {
			printf("  FAIL: set time and read time do not match\r\n");
		}

		expected_seconds = test_time[idx].tm_sec;

		for (int itr = 0; itr < 5; itr++) {
			expected_seconds++;
			expected_seconds %= 60;

			k_msleep(1000);

			ret = rtc_get_time(rtc, &read_time);

			if (ret != 0) {
				printf("  FAIL: cannot get RTC time (reason: %d)\r\n", ret);
				continue;
			}

			printf("  Read (after %d s): ", itr + 1);
			PRINT_TIME(read_time);

			if (read_time.tm_sec != expected_seconds) {
				printf("  FAIL: seconds mismatch (expected %d, got %d)\r\n",
				       expected_seconds, read_time.tm_sec);
			} else {
				printf("  PASS: seconds match expected value\r\n");
			}
		}
	}
}

#if CONFIG_RTC_ALARM

static __attribute__((used)) void test_rtc_alarm(const struct device *rtc)
{
	int ret = 0;

	struct rtc_time start_time[] = {
		TIME(2024, 11, 17, 10, 20, 0),
		TIME(2024, 11, 17, 8, 45, 0),
		TIME(2024, 11, 17, 23, 59, 59),
	};

	struct rtc_time alarm_time[] = {
		TIME(2024, 11, 17, 10, 20, 15),
		TIME(2024, 11, 17, 8, 45, 15),
		TIME(2024, 11, 18, 0, 0, 14),
	};

	uint32_t masks[] = {(RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE),
			    (RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE),
			    (RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE)};

	struct rtc_time current_time;

	printf("Starting RTC alarm test.\r\n");

	ret = rtc_alarm_set_callback(rtc, 0, alarm_callback, &alarm_count);

	if (ret < 0) {
		printf("FAIL: cannot set alarm callback (reason: %d)\r\n", ret);
		return;
	}

	for (int idx = 0; idx < ARRAY_SIZE(start_time); idx++) {
		printf("[%d/%d] Start: ", idx + 1, (int)ARRAY_SIZE(start_time));
		PRINT_TIME(start_time[idx]);
		printf("       Alarm: ");
		PRINT_TIME(alarm_time[idx]);

		ret = rtc_set_time(rtc, &start_time[idx]);

		if (ret < 0) {
			printf("  FAIL: cannot set start time (reason: %d)\r\n", ret);
			continue;
		}

		ret = rtc_alarm_set_time(rtc, 0, masks[idx], &alarm_time[idx]);

		if (ret < 0) {
			printf("  FAIL: cannot set alarm (reason: %d)\r\n", ret);
			continue;
		}

		k_sem_reset(&alarm_semaphore);

		k_sem_take(&alarm_semaphore, K_MSEC(15100));

		rtc_get_time(rtc, &current_time);
		printf("  Time at wakeup: ");
		PRINT_TIME(current_time);

		ret = rtc_alarm_is_pending(rtc, 0);

		if (ret > 0) {
			printf("  Alarm pending flag set as expected\r\n");
		} else {
			printf("  Alarm pending flag not set (return: %d)\r\n", ret);
		}

		if (alarm_count == (idx + 1)) {
			printf("  PASS: alarm fired as expected\r\n");
		} else {
			printf("  FAIL: alarm count mismatch (expected %d, got %d)\r\n", idx + 1,
			       alarm_count);
		}
	}

	ret = rtc_alarm_set_time(rtc, 0, 0, NULL);

	if (ret < 0) {
		printf("FAIL: cannot disarm alarm (reason: %d)\r\n", ret);
	}
}

static __attribute__((used)) void test_rtc_alarm_masked(const struct device *rtc)
{
	int ret;
	int prev_alarm_count;
	struct rtc_time current_time;

	struct alarm_test_case {
		const char *name;
		struct rtc_time start;
		struct rtc_time alarm;
		uint16_t mask;
		uint32_t timeout_ms;
	};

	/* 2024-11-17 is a Sunday (tm_wday = 0, zero-initialised by the TIME
	 * macro for fields not explicitly set).
	 *
	 * Each test case is constructed so the alarm fires within its timeout:
	 *   - SECOND / SECOND|MINUTE / SECOND|MINUTE|HOUR / SECOND|MONTHDAY /
	 *     SECOND|WEEKDAY  -- start :30, alarm :45  -> fires in ~15 s
	 *   - MINUTE                                   -> fires in ~60 s
	 *
	 * HOUR-only, MONTHDAY-only, and WEEKDAY-only masks require up to 1 hour,
	 * 1 day, and 7 days to fire and are excluded from the live testbench.
	 */
	static const struct alarm_test_case cases[] = {
		{
			.name = "SECOND",
			.start = TIME(2024, 11, 17, 10, 20, 30),
			.alarm = TIME(2024, 11, 17, 10, 20, 45),
			.mask = RTC_ALARM_TIME_MASK_SECOND,
			.timeout_ms = 16000,
		},
		{
			.name = "SECOND | MINUTE",
			.start = TIME(2024, 11, 17, 10, 20, 30),
			.alarm = TIME(2024, 11, 17, 10, 20, 45),
			.mask = RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE,
			.timeout_ms = 16000,
		},
		{
			.name = "SECOND | MINUTE | HOUR",
			.start = TIME(2024, 11, 17, 10, 20, 30),
			.alarm = TIME(2024, 11, 17, 10, 20, 45),
			.mask = RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE |
				RTC_ALARM_TIME_MASK_HOUR,
			.timeout_ms = 16000,
		},
		{
			.name = "MINUTE",
			.start = TIME(2024, 11, 17, 10, 20, 00),
			.alarm = TIME(2024, 11, 17, 10, 21, 00),
			.mask = RTC_ALARM_TIME_MASK_MINUTE,
			.timeout_ms = 62000,
		},
		{
			/* 2024-11-17 is Sunday; alarm.tm_wday = 0 (zero-init) matches */
			.name = "SECOND | WEEKDAY",
			.start = TIME(2024, 11, 17, 10, 20, 30),
			.alarm = TIME(2024, 11, 17, 10, 20, 45),
			.mask = RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_WEEKDAY,
			.timeout_ms = 16000,
		},
		{
			.name = "SECOND | MONTHDAY",
			.start = TIME(2024, 11, 17, 10, 20, 30),
			.alarm = TIME(2024, 11, 17, 10, 20, 45),
			.mask = RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MONTHDAY,
			.timeout_ms = 16000,
		},
	};

	printf("Starting RTC masked alarm test.\r\n");

	ret = rtc_alarm_set_callback(rtc, 0, alarm_callback, &alarm_count);

	if (ret < 0) {
		printf("Unable to set alarm callback (reason: %d).\r\n", ret);
		return;
	}

	for (int idx = 0; idx < ARRAY_SIZE(cases); idx++) {
		const struct alarm_test_case *tc = &cases[idx];

		printf("[%d/%d] Mask: %s\r\n", idx + 1, (int)ARRAY_SIZE(cases), tc->name);
		printf("  Start: ");
		PRINT_TIME(tc->start);
		printf("  Alarm: ");
		PRINT_TIME(tc->alarm);

		/* Drain any semaphore count left from a previous iteration */
		k_sem_reset(&alarm_semaphore);

		prev_alarm_count = alarm_count;

		ret = rtc_set_time(rtc, &tc->start);

		if (ret < 0) {
			printf("  FAIL: cannot set start time (reason: %d)\r\n", ret);
			continue;
		}

		ret = rtc_alarm_set_time(rtc, 0, tc->mask, &tc->alarm);

		if (ret < 0) {
			printf("  FAIL: cannot set alarm (reason: %d)\r\n", ret);
			continue;
		}

		ret = k_sem_take(&alarm_semaphore, K_MSEC(tc->timeout_ms));

		rtc_get_time(rtc, &current_time);
		printf("  Time at wakeup: ");
		PRINT_TIME(current_time);

		if (ret != 0) {
			printf("  FAIL: alarm did not fire within %u ms\r\n", tc->timeout_ms);
			continue;
		}

		if (alarm_count != prev_alarm_count + 1) {
			printf("  FAIL: alarm count mismatch (expected %d, got %d)\r\n",
			       prev_alarm_count + 1, alarm_count);
		} else {
			printf("  PASS: alarm fired as expected\r\n");
		}

		ret = rtc_alarm_is_pending(rtc, 0);

		if (ret > 0) {
			printf("  Alarm pending flag set as expected\r\n");
		} else {
			printf("  Alarm pending flag not set (return: %d)\r\n", ret);
		}
	}

	ret = rtc_alarm_set_time(rtc, 0, 0, NULL);

	if (ret < 0) {
		printf("Unable to disarm alarm (reason: %d).\r\n", ret);
	}
}

#endif /* CONFIG_RTC_ALARM */

#if CONFIG_RTC_UPDATE

static __attribute__((used)) void test_rtc_update(const struct device *rtc)
{
	int ret;

	struct rtc_time current_time;

	printf("Starting RTC update callback test.\r\n");

	ret = rtc_update_set_callback(rtc, update_callback, &update_count);

	if (ret < 0) {
		printf("FAIL: cannot set update callback (reason: %d)\r\n", ret);
		return;
	}

	for (int i = 0; i < 5; i++) {
		ret = k_sem_take(&update_semaphore, K_MSEC(1100));

		rtc_get_time(rtc, &current_time);
		printf("[%d/5] Time at wakeup: ", i + 1);
		PRINT_TIME(current_time);

		if (ret != 0) {
			printf("  FAIL: update callback did not fire within 1100 ms\r\n");
			continue;
		}

		if (update_count == (i + 1)) {
			printf("  PASS: update count incremented as expected\r\n");
		} else {
			printf("  FAIL: update count mismatch (expected %d, got %d)\r\n", i + 1,
			       update_count);
		}
	}

	ret = rtc_update_set_callback(rtc, NULL, NULL);

	if (ret < 0) {
		printf("FAIL: cannot disable update callback (reason: %d)\r\n", ret);
		return;
	}
}

#endif /* CONFIG_RTC_UPDATE */

#if CONFIG_RTC_UPDATE && CONFIG_RTC_ALARM

static void test_rtc_alarm_and_update(const struct device *rtc)
{
	int ret;
	int key;

	struct rtc_time start_time = TIME(2024, 11, 17, 10, 20, 0);
	struct rtc_time alarm_time = TIME(2024, 11, 17, 10, 20, 15);
	struct rtc_time current_time;

	key = irq_lock();

	alarm_count = 0;
	update_count = 0;

	irq_unlock(key);

	ret = rtc_set_time(rtc, &start_time);

	if (ret < 0) {
		printf("FAIL: cannot set start time (reason: %d)\r\n", ret);
		return;
	}

	printf("Starting RTC alarm & update test.\r\n");
	printf("  Start: ");
	PRINT_TIME(start_time);
	printf("  Alarm: ");
	PRINT_TIME(alarm_time);

	ret = rtc_alarm_set_callback(rtc, 0, alarm_callback, &alarm_count);

	if (ret < 0) {
		printf("FAIL: cannot set alarm callback (reason: %d)\r\n", ret);
		return;
	}

	ret = rtc_alarm_set_time(rtc, 0, RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE,
				 &alarm_time);

	if (ret < 0) {
		printf("FAIL: cannot set alarm (reason: %d)\r\n", ret);
		return;
	}

	k_msleep(3100);

	ret = rtc_update_set_callback(rtc, update_callback, &update_count);

	if (ret < 0) {
		printf("FAIL: cannot set update callback (reason: %d)\r\n", ret);
		return;
	}

	for (int i = 0; i < 5; i++) {
		ret = k_sem_take(&update_semaphore, K_MSEC(1100));

		rtc_get_time(rtc, &current_time);
		printf("[%d/5] Time at wakeup: ", i + 1);
		PRINT_TIME(current_time);

		if (ret != 0) {
			printf("  FAIL: update callback did not fire within 1100 ms\r\n");
			continue;
		}

		if (update_count != (i + 1)) {
			printf("  FAIL: update count mismatch (expected %d, got %d)\r\n", i + 1,
			       update_count);
		} else {
			printf("  PASS: update count incremented as expected\r\n");
		}
	}

	ret = rtc_update_set_callback(rtc, NULL, NULL);

	if (ret < 0) {
		printf("FAIL: cannot disable update callback (reason: %d)\r\n", ret);
		return;
	}

	ret = k_sem_take(&alarm_semaphore, K_MSEC(7100));

	rtc_get_time(rtc, &current_time);
	printf("  Time at alarm wakeup: ");
	PRINT_TIME(current_time);

	if (ret != 0) {
		printf("FAIL: alarm did not fire within 7000 ms\r\n");
		return;
	}

	if (alarm_count != 1) {
		printf("FAIL: alarm count mismatch (expected 1, got %d)\r\n", alarm_count);
	} else {
		printf("PASS: alarm fired as expected\r\n");
	}

	if (update_count != 5) {
		printf("FAIL: update count changed after callback was disabled (got %d)\r\n",
		       update_count);
		return;
	}

	ret = rtc_update_set_callback(rtc, NULL, NULL);

	if (ret < 0) {
		printf("FAIL: cannot disable update callback (reason: %d)\r\n", ret);
		return;
	}

	ret = rtc_alarm_set_time(rtc, 0, 0, NULL);

	if (ret < 0) {
		printf("FAIL: cannot disarm alarm (reason: %d)\r\n", ret);
	}
}

#endif /* CONFIG_RTC_ALARM */

int main(void)
{
	/* Check if the RTC is ready */
	if (!device_is_ready(rtc)) {
		printk("Device is not ready\n");
		return 0;
	}

	k_sem_init(&alarm_semaphore, 0, 1);
	k_sem_init(&update_semaphore, 0, 1);

	while (1) {

		test_set_get_time(rtc);

#if defined(CONFIG_RTC_ALARM)
		test_rtc_alarm(rtc);
		test_rtc_alarm_masked(rtc);
#endif /* CONFIG_RTC_ALARM */

#if defined(CONFIG_RTC_UPDATE)
		test_rtc_update(rtc);
#endif /* CONFIG_RTC_ALARM  */

#if defined(CONFIG_RTC_ALARM) && defined(CONFIG_RTC_UPDATE)
		test_rtc_alarm_and_update(rtc);
#endif /* CONFIG_RTC_ALARM */

		k_sleep(K_FOREVER);
	};

	return 0;
}
