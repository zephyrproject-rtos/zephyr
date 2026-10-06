/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2026 Silicon Laboratories Inc.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/__assert.h>
#include <string.h>

#include <zephyr/pm/pm.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>
#include <zephyr/pm/state.h>
#include <zephyr/sys/poweroff.h>

#include <siwx91x_poweroff.h>
#include <zephyr/devicetree.h>

/* size of stack area used by each thread */
#define STACKSIZE 1024

/* scheduling priority used by each thread */
#define PRIORITY 7

#define LED0_NODE              DT_ALIAS(led0)
#define LED1_NODE              DT_ALIAS(led1)
#define RTC_WAKE_DELAY_SECONDS 10U

#if !DT_NODE_HAS_STATUS_OKAY(LED0_NODE)
#error "Unsupported board: led0 devicetree alias is not defined"
#endif

#if !DT_NODE_HAS_STATUS_OKAY(LED1_NODE)
#error "Unsupported board: led1 devicetree alias is not defined"
#endif

#if !DT_NODE_HAS_STATUS_OKAY(DT_ALIAS(rtc))
#error "Unsupported board: rtc devicetree alias is not defined"
#endif

K_FIFO_DEFINE(printk_fifo);
struct printk_data_t {
	void *fifo_reserved; /* 1st word reserved for use by fifo */
	uint32_t led;
	uint32_t cnt;
};

struct led_dev {
	struct gpio_dt_spec spec;
	uint8_t num;
};

static const struct gpio_dt_spec gpio_uulp_wakeup_soruce = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);

static const struct device *const calendar_rtc = DEVICE_DT_GET(DT_ALIAS(rtc));

static const struct led_dev led0 = {
	.spec = GPIO_DT_SPEC_GET_OR(LED0_NODE, gpios, {0}),
	.num = 0,
};

static const struct led_dev led1 = {
	.spec = GPIO_DT_SPEC_GET_OR(LED1_NODE, gpios, {0}),
	.num = 1,
};
/* arm gpio UULP pin as wakeup source */
static int arm_gpio_uulp_wakeup_soruce(void)
{
	int ret;

	if (!gpio_is_ready_dt(&gpio_uulp_wakeup_soruce)) {
		return -ENODEV;
	}
	/* Required by the current UULP driver to select wakeup mux mode. */
	ret = gpio_pin_configure_dt(&gpio_uulp_wakeup_soruce, GPIO_INPUT | GPIO_INT_WAKEUP);
	if (ret) {
		return ret;
	}
	/* UULP PS0 wake supports level only; button is active-low. */
	return gpio_pin_interrupt_configure_dt(&gpio_uulp_wakeup_soruce,
					       GPIO_INT_LEVEL_ACTIVE | GPIO_INT_WAKEUP);
}

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

static int arm_rtc_wakeup(void)
{
	static const uint16_t alarm_mask = RTC_ALARM_TIME_MASK_YEAR | RTC_ALARM_TIME_MASK_MONTH |
					   RTC_ALARM_TIME_MASK_MONTHDAY | RTC_ALARM_TIME_MASK_HOUR |
					   RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_SECOND;
	struct rtc_time now = {};
	struct rtc_time alarm;
	int ret;

	if (!device_is_ready(calendar_rtc)) {
		return -ENODEV;
	}

	ret = rtc_get_time(calendar_rtc, &now);
	if ((ret != 0) || !rtc_time_is_valid(&now)) {
		/*
		 * A cold boot has no guaranteed calendar value. Use a valid test
		 * epoch so the relative wake interval remains deterministic.
		 */
		now = (struct rtc_time){
			.tm_year = 126, /* 2026 */
			.tm_mon = 0,
			.tm_mday = 1,
			.tm_wday = 4, /* Thursday */
		};
		ret = rtc_set_time(calendar_rtc, &now);
		if (ret != 0) {
			return ret;
		}
	}

	alarm = now;
	ret = rtc_add_seconds(&alarm, RTC_WAKE_DELAY_SECONDS);
	if (ret) {
		return ret;
	}

	if (!pm_device_wakeup_enable(calendar_rtc, true)) {
		return -EIO;
	}

	ret = rtc_alarm_set_time(calendar_rtc, 0U, alarm_mask, &alarm);
	if (ret != 0) {
		(void)pm_device_wakeup_enable(calendar_rtc, false);
		return ret;
	}

	printk("RTC wake alarm armed for %u seconds.\n", RTC_WAKE_DELAY_SECONDS);
	return 0;
}

static int report_rtc_wakeup(void)
{
	int ret = rtc_alarm_is_pending(calendar_rtc, 0U);

	if (ret < 0) {
		return ret;
	}

	if (ret > 0) {
		printk("Booted from RTC alarm wake.\n");
	}

	return 0;
}

SYS_INIT(report_rtc_wakeup, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

/* Function to call poweroff(softoff) */
static void poweroff(void)
{
	int ret;
	/*
	 * NWP profile changes are blocking operations and must be performed from
	 * thread context, not interrupt context.
	 */
	ret = siwx91x_nwp_prepare_poweroff();
	if (ret) {
		printk("NWP-poweroff request failed (%d)\n", ret);
		return;
	}
	/* trigger zephyr poweroff system call */
	sys_poweroff();
}

/*
 * Poll the same button that is armed as the UULP wake source. This runs in
 * thread context so poweroff() can issue the blocking NWP profile requests.
 */
void button_monitor(void)
{
	int ret;
	int64_t press_start_ms = 0;
	bool long_press_handled = false;

	if (!gpio_is_ready_dt(&gpio_uulp_wakeup_soruce)) {
		return;
	}

	ret = gpio_pin_configure_dt(&gpio_uulp_wakeup_soruce, GPIO_INPUT);
	if (ret) {
		return;
	}

	while (true) {
		ret = gpio_pin_get_dt(&gpio_uulp_wakeup_soruce);
		if (ret > 0) {
			if (press_start_ms == 0) {
				press_start_ms = k_uptime_get();
			}

			if (!long_press_handled && (k_uptime_get() - press_start_ms >= 1000)) {
				long_press_handled = true;
				printk("Power-off button held for 1 second; powering off.\n");
				ret = arm_rtc_wakeup();
				if (ret) {
					printk("RTC wake setup failed (%d)\n", ret);
				}

				ret = arm_gpio_uulp_wakeup_soruce();

				if (!ret) {
					printk("GPIO is set as wakeup source, click to wake!\n");

				} else {
					printk("GPIO wake setup failed (%d)\n", ret);
				}

				poweroff();
			}
		} else {
			/* A release cancels a short press and allows another attempt. */
			press_start_ms = 0;
			long_press_handled = false;
		}

		k_msleep(20);
	}
}

void blink(const struct led_dev *led, uint32_t sleep_ms, uint32_t id)
{
	const struct gpio_dt_spec *spec = &led->spec;
	int cnt = 0;
	int ret;

	if (!device_is_ready(spec->port)) {
		printk("Error: %s device is not ready\n", spec->port->name);
		return;
	}

	ret = gpio_pin_configure_dt(spec, GPIO_OUTPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure pin %d (LED '%d')\n", ret, spec->pin,
		       led->num);
		return;
	}

	while (1) {
		gpio_pin_set(spec->port, spec->pin, cnt % 2);

		struct printk_data_t tx_data = {.led = id, .cnt = cnt};

		size_t size = sizeof(struct printk_data_t);
		char *mem_ptr = k_malloc(size);

		__ASSERT_NO_MSG(mem_ptr != 0);

		memcpy(mem_ptr, &tx_data, size);

		k_fifo_put(&printk_fifo, mem_ptr);

		k_msleep(sleep_ms);
		cnt++;
	}
}

void blink0(void)
{
	blink(&led0, 100, 0);
}

void blink1(void)
{
	blink(&led1, 1000, 1);
}

void uart_out(void)
{
	while (1) {
		struct printk_data_t *rx_data = k_fifo_get(&printk_fifo, K_FOREVER);
		printk("Toggled led%d; counter=%d\n", rx_data->led, rx_data->cnt);
		k_free(rx_data);
	}
}

K_THREAD_DEFINE(blink0_id, STACKSIZE * 2, blink0, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(blink1_id, STACKSIZE, blink1, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(uart_out_id, STACKSIZE, uart_out, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(button_monitor_id, STACKSIZE, button_monitor, NULL, NULL, NULL, PRIORITY, 0, 0);
