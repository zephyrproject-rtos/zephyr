/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/pm/pm.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/device.h>
#include <zephyr/drivers/wuc.h>

#if !DT_NODE_HAS_PROP(DT_PATH(zephyr_user), wakeup_ctrls)
#error "No wakeup-ctrls defined under /zephyr,user"
#endif

static const struct wuc_dt_spec wuc = WUC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);

static void report_wakeup_source(void)
{
	int ret = wuc_check_wakeup_source_triggered_dt(&wuc);

	if (ret > 0) {
		printk("Wakeup source triggered\n");
		wuc_clear_wakeup_source_triggered_dt(&wuc);
	} else if (ret < 0) {
		printk("Failed to check wakeup source: %d\n", ret);
	}
}

static int enable_wakeup_source(void)
{
	int ret = wuc_enable_wakeup_source_dt(&wuc);

	if (ret != 0) {
		printk("Failed to enable wakeup source: %d\n", ret);
	}

	return ret;
}

int main(void)
{
	if (!device_is_ready(wuc.dev)) {
		printk("WUC device not ready: %s\n", wuc.dev->name);
		return 0;
	}

	report_wakeup_source();

	if (enable_wakeup_source() != 0) {
		return 0;
	}

	printk("Entering Deep Software Standby mode via %s\n", IS_ENABLED(CONFIG_POWEROFF) ?
								"sys_poweroff" :
								"pm_state_force");
#if defined(CONFIG_POWEROFF)
	sys_poweroff();
#else
	pm_state_force(0u, &(struct pm_state_info){ PM_STATE_SOFT_OFF, 0, 0 });
#endif
	k_sleep(K_FOREVER);

	return 0;
}
