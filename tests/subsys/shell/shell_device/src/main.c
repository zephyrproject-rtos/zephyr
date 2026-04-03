/*
 * Copyright 2024 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>

#ifdef CONFIG_PM_DEVICE
static int dummy_device_pm_action(const struct device *dev,
				  enum pm_device_action action)
{
	return 0;
}

PM_DEVICE_DEFINE(dummy_pm_driver_1, dummy_device_pm_action);
PM_DEVICE_DEFINE(dummy_pm_driver_2, dummy_device_pm_action);
PM_DEVICE_DEFINE(dummy_pm_driver_3, dummy_device_pm_action);
PM_DEVICE_DEFINE(dummy_pm_driver_4, dummy_device_pm_action);
#endif

DEVICE_DEFINE(device_0, "device@0", NULL, NULL,
	      NULL, NULL,
	      POST_KERNEL, 0, NULL);

DEVICE_DEFINE(device_1, "device@1", NULL, PM_DEVICE_GET(dummy_pm_driver_1),
	      NULL, NULL,
	      POST_KERNEL, 1, NULL);

DEVICE_DEFINE(device_2, "device@2", NULL, PM_DEVICE_GET(dummy_pm_driver_2),
	      NULL, NULL,
	      POST_KERNEL, 2, NULL);

DEVICE_DEFINE(device_3, "device@3", NULL, PM_DEVICE_GET(dummy_pm_driver_3),
	      NULL, NULL,
	      POST_KERNEL, 3, NULL);

DEVICE_DEFINE(device_4, "device@4", NULL, PM_DEVICE_GET(dummy_pm_driver_4),
	      NULL, NULL,
	      POST_KERNEL, 4, NULL);

#ifdef CONFIG_PM_DEVICE
static const struct device *d2 = &DEVICE_NAME_GET(device_2);
#endif
static const struct device *d3 = &DEVICE_NAME_GET(device_3);
static const struct device *d4 = &DEVICE_NAME_GET(device_4);

int main(void)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	const char *buf;
	int err;
	size_t size;

	/* Let the shell backend initialize. */
	k_usleep(10);

#ifdef CONFIG_PM_DEVICE
	pm_device_action_run(d2, PM_DEVICE_ACTION_SUSPEND);
#endif

	pm_device_runtime_enable(d3);
	pm_device_runtime_enable(d4);
	pm_device_runtime_get(d4);

	shell_backend_dummy_clear_output(sh);

	err = shell_execute_cmd(sh, "device list");
	if (err) {
		printf("Failed to execute the shell command: %d.\n",
		       err);
	}

	buf = shell_backend_dummy_get_output(sh, &size);
	printf("%s\n", buf);

	return 0;
}
