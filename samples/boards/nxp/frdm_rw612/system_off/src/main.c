/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/wuc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/sys/printk.h>

int main(void)
{
	const struct device *rtc = DEVICE_DT_GET(DT_NODELABEL(rtc));
	const struct wuc_dt_spec wakeup = WUC_DT_SPEC_GET(DT_NODELABEL(rtc));
	const struct counter_alarm_cfg alarm = {
		.ticks = CONFIG_SAMPLE_RW612_SYSTEM_OFF_WAKEUP_SECONDS,
	};
	uint32_t reset_cause;
	int ret;

	ret = hwinfo_get_reset_cause(&reset_cause);
	if (ret != 0) {
		printk("Reset cause unavailable: %d\n", ret);
		return ret;
	}
	printk("RW612 boot: reset cause 0x%08x\n", reset_cause);
	if ((reset_cause & RESET_LOW_POWER_WAKE) != 0U) {
		printk("PM4 wake reset: fresh boot; SYSTEM OFF TEST PASSED\n");
		return 0;
	}

	if (!device_is_ready(wakeup.dev)) {
		printk("Wakeup controller is not ready\n");
		return -ENODEV;
	}
	ret = wuc_disable_wakeup_source_dt(&wakeup);
	if (ret == 0) {
		ret = wuc_clear_wakeup_source_triggered_dt(&wakeup);
	}
	if (ret != 0) {
		printk("Wakeup controller setup failed: %d\n", ret);
		return ret;
	}

	if (!device_is_ready(rtc)) {
		printk("RTC is not ready\n");
		return -ENODEV;
	}
	printk("PM4 system off in 10 s; RTC wake after %u s\n",
	       CONFIG_SAMPLE_RW612_SYSTEM_OFF_WAKEUP_SECONDS);
	k_sleep(K_SECONDS(10));

	/* Print before arming the alarm so console output cannot consume its interval. */
	printk("Entering PM4 via sys_poweroff()\n");
	ret = counter_start(rtc);
	if (ret == 0) {
		ret = counter_set_channel_alarm(rtc, 0U, &alarm);
	}
	if (ret != 0) {
		printk("RTC setup failed: %d\n", ret);
		return ret;
	}
	ret = wuc_clear_wakeup_source_triggered_dt(&wakeup);
	if (ret == 0) {
		ret = wuc_enable_wakeup_source_dt(&wakeup);
	}
	if (ret != 0) {
		printk("RTC wakeup routing failed: %d\n", ret);
		return ret;
	}
	sys_poweroff();
}
