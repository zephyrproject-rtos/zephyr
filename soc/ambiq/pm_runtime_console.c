/*
 * Copyright (c) 2025 Ambiq Micro Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/util.h>

#include <errno.h>

#if DT_HAS_CHOSEN(zephyr_console)

static int ambiq_console_pm_opt_out(void)
{
	const struct device *console = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	int ret;

	if (!device_is_ready(console)) {
		return -ENODEV;
	}

	ret = pm_device_runtime_get(console);
	if ((ret < 0) && (ret != -EALREADY)) {
		return 0;
	}

	(void)pm_device_runtime_disable(console);

	return 0;
}

SYS_INIT(ambiq_console_pm_opt_out, PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif /* DT_HAS_CHOSEN(zephyr_console) */
