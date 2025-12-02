/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/rtc.h>

#define RTC_ALARM_FIELDS                                                                           \
	(RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR)

#if CONFIG_SOC_FAMILY_AMBIQ
static const struct device *rtc = DEVICE_DT_GET(DT_NODELABEL(rtc0));
#else
static const struct device *rtc = DEVICE_DT_GET(DT_NODELABEL(rtc));
#endif

static k_tid_t my_tid;

#define MY_STACK_SIZE 500
#define MY_PRIORITY   5

struct k_thread my_thread_data;
K_THREAD_STACK_DEFINE(my_stack_area, MY_STACK_SIZE);

void my_entry_point(void *, void *, void *)
{
	printk("Going sleep.\n");
	k_msleep(3000);
}

/* Fri Jan 01 2021 13:29:50 GMT+0000 */
static const struct rtc_time rtc_time = {
	.tm_sec = 50,
	.tm_min = 29,
	.tm_hour = 13,
	.tm_mday = 1,
	.tm_mon = 0,
	.tm_year = 121,
	.tm_wday = 5,
	.tm_yday = 1,
	.tm_isdst = -1,
	.tm_nsec = 0,
};

/* Fri Jan 01 2021 13:29:51 GMT+0000 */
static const struct rtc_time alarm_time = {
	.tm_sec = 51,
	.tm_min = 29,
	.tm_hour = 13,
	.tm_mday = 1,
	.tm_mon = 0,
	.tm_year = 121,
	.tm_wday = 5,
	.tm_yday = 1,
	.tm_isdst = -1,
	.tm_nsec = 0,
};

#if CONFIG_SOC_FAMILY_AMBIQ
void normalize_rtc_time(struct rtc_time *t)
{
	if (t->tm_sec >= 60) {
		t->tm_min += t->tm_sec / 60;
		t->tm_sec %= 60;
	}
	if (t->tm_min >= 60) {
		t->tm_hour += t->tm_min / 60;
		t->tm_min %= 60;
	}
	if (t->tm_hour >= 24) {
		t->tm_mday += t->tm_hour / 24;
		t->tm_hour %= 24;
	}
}
#endif

static void wakeup_cb(const struct device *dev, uint16_t id, void *user_data)
{
	printk("Wake up by alarm.\n");
#if CONFIG_SOC_FAMILY_AMBIQ
	struct rtc_time next_time;
	uint16_t mask = RTC_ALARM_FIELDS;

	rtc_get_time(dev, &next_time);
	next_time.tm_sec += 5;
	normalize_rtc_time(&next_time);
	rtc_alarm_set_time(dev, id, mask, &next_time);
#else
	k_thread_abort(my_tid);
#endif
}

int main(void)
{
	int ret;
	uint16_t mask = RTC_ALARM_FIELDS;

	ret = rtc_set_time(rtc, &rtc_time);
	if (ret < 0) {
		return 0;
	}

	ret = rtc_alarm_set_time(rtc, 0, mask, &alarm_time);
	if (ret < 0) {
		return 0;
	}

	ret = rtc_alarm_set_callback(rtc, 0, wakeup_cb, NULL);
	if (ret < 0) {
		return 0;
	}

	printk("Created the thread.\n");
	my_tid = k_thread_create(&my_thread_data, my_stack_area,
				 K_THREAD_STACK_SIZEOF(my_stack_area), my_entry_point, NULL, NULL,
				 NULL, MY_PRIORITY, 0, K_NO_WAIT);

	k_thread_join(my_tid, K_FOREVER);

#if CONFIG_SOC_FAMILY_AMBIQ
	k_sleep(K_FOREVER);
#else
	while (1) {
		arch_nop();
	}
#endif
	return 0;
}
