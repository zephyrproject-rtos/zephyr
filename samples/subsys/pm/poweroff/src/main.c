/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2026 Silicon Laboratories Inc.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/drivers/wuc.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/poweroff.h>

#define RTC_WAKE_DELAY_SECONDS 10U
#define LONG_PRESS_MS          1000

#if !DT_NODE_HAS_STATUS_OKAY(DT_ALIAS(button))
#error "Unsupported board: button devicetree alias is not defined"
#endif

#if !DT_NODE_HAS_STATUS_OKAY(DT_ALIAS(rtc))
#error "Unsupported board: rtc devicetree alias is not defined"
#endif

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(button), gpios);
static const struct device *const rtc = DEVICE_DT_GET(DT_ALIAS(rtc));

#if defined(CONFIG_WUC) && DT_NODE_HAS_PROP(DT_ALIAS(rtc), wakeup_ctrls)
#define RTC_WAKE_USES_WUC 1
static const struct wuc_dt_spec rtc_wakeup = WUC_DT_SPEC_GET(DT_ALIAS(rtc));
#else
#define RTC_WAKE_USES_WUC 0
#endif

static uint8_t rtc_days_in_month(int year, int month)
{
	static const uint8_t days_per_month[] = {
		31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
	};

	if ((month == 1) && ((year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0)))) {
		return 29U;
	}

	return days_per_month[month];
}

static bool rtc_time_is_valid(const struct rtc_time *tm)
{
	int year = tm->tm_year + 1900;

	if ((year < 2000) || (year > 2399) || (tm->tm_mon < 0) || (tm->tm_mon > 11) ||
	    (tm->tm_mday < 1) || (tm->tm_mday > rtc_days_in_month(year, tm->tm_mon)) ||
	    (tm->tm_hour < 0) || (tm->tm_hour > 23) || (tm->tm_min < 0) || (tm->tm_min > 59) ||
	    (tm->tm_sec < 0) || (tm->tm_sec > 59) || (tm->tm_nsec < 0) ||
	    (tm->tm_nsec >= NSEC_PER_SEC)) {
		return false;
	}

	return true;
}

static int rtc_add_seconds(struct rtc_time *curr_time, uint32_t seconds)
{
	uint32_t value = curr_time->tm_sec + seconds;
	uint32_t day_increment;
	int year;

	curr_time->tm_sec = value % 60U;
	value /= 60U;
	value += curr_time->tm_min;
	curr_time->tm_min = value % 60U;
	value /= 60U;
	value += curr_time->tm_hour;
	curr_time->tm_hour = value % 24U;
	day_increment = value / 24U;

	while (day_increment > 0U) {
		year = curr_time->tm_year + 1900;
		curr_time->tm_mday++;
		curr_time->tm_wday = (curr_time->tm_wday + 1) % 7;
		if (curr_time->tm_mday > rtc_days_in_month(year, curr_time->tm_mon)) {
			curr_time->tm_mday = 1;
			curr_time->tm_mon++;
			if (curr_time->tm_mon > 11) {
				curr_time->tm_mon = 0;
				curr_time->tm_year++;
				if ((curr_time->tm_year + 1900) > 2399) {
					return -ERANGE;
				}
			}
		}
		day_increment--;
	}

	return 0;
}

static int setup_rtc_wakeup(uint32_t wakeup_delay)
{
	static const uint16_t alarm_mask = RTC_ALARM_TIME_MASK_YEAR | RTC_ALARM_TIME_MASK_MONTH |
					   RTC_ALARM_TIME_MASK_MONTHDAY | RTC_ALARM_TIME_MASK_HOUR |
					   RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_SECOND;
	struct rtc_time now = {};
	struct rtc_time alarm;
	int ret;

	if (!device_is_ready(rtc)) {
		return -ENODEV;
	}

	ret = rtc_get_time(rtc, &now);
	if ((ret != 0) || !rtc_time_is_valid(&now)) {
		now = (struct rtc_time){
			.tm_year = 126,
			.tm_mon = 0,
			.tm_mday = 1,
			.tm_wday = 4,
		};
		ret = rtc_set_time(rtc, &now);
		if (ret != 0) {
			return ret;
		}
	}

	alarm = now;
	ret = rtc_add_seconds(&alarm, wakeup_delay);
	if (ret != 0) {
		return ret;
	}

	ret = rtc_alarm_set_time(rtc, 0U, alarm_mask, &alarm);
	if (ret != 0) {
		return ret;
	}

#if RTC_WAKE_USES_WUC
	ret = wuc_enable_wakeup_source_dt(&rtc_wakeup);
#endif

	return ret;
}

static int setup_button_wakeup(void)
{
	int ret;

	if (!gpio_is_ready_dt(&button)) {
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT | GPIO_INT_WAKEUP);
	if (ret != 0) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_LEVEL_ACTIVE | GPIO_INT_WAKEUP);
	return ret;
}

static bool setup_wakeup_sources(void)
{
	int rtc_ret = setup_rtc_wakeup(RTC_WAKE_DELAY_SECONDS);
	int gpio_ret = setup_button_wakeup();

	if (rtc_ret == 0) {
		printk("RTC wake alarm armed for %u seconds.\n", RTC_WAKE_DELAY_SECONDS);
	} else {
		printk("RTC wake setup failed (%d)\n", rtc_ret);
	}

	if (gpio_ret == 0) {
		printk("GPIO is set as wakeup source, click to wake!\n");
	} else {
		printk("GPIO wake setup failed (%d)\n", gpio_ret);
	}

	return (rtc_ret == 0) || (gpio_ret == 0);
}

int main(void)
{
	int ret;
	int64_t press_start_ms = 0;
	bool long_press_handled = false;

	printk("Poweroff example with GPIO and RTC wakeup sources\n");
	if (!gpio_is_ready_dt(&button)) {
		printk("Button GPIO not ready\n");
		return 0;
	}

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret != 0) {
		printk("Button configure failed (%d)\n", ret);
		return 0;
	}

	printk("Hold button 1 s to arm wake sources and power off.\n");

	while (true) {
		ret = gpio_pin_get_dt(&button);
		if (ret > 0) {
			if (press_start_ms == 0) {
				press_start_ms = k_uptime_get();
			}

			if (!long_press_handled &&
			    (k_uptime_get() - press_start_ms >= LONG_PRESS_MS)) {
				long_press_handled = true;
				printk("Power-off button held for 1 second; powering off.\n");

				if (setup_wakeup_sources()) {
					sys_poweroff();
					printk("ERROR: System off failed\n");
				} else {
					printk("Poweroff rejected: wake source setup failed.\n");
					(void)gpio_pin_interrupt_configure_dt(&button,
									      GPIO_INT_DISABLE);
					printk("Release button, then hold 1 s again to retry.\n");
				}
			}
		} else {
			press_start_ms = 0;
			long_press_handled = false;
		}

		k_msleep(20);
	}

	return 0;
}
