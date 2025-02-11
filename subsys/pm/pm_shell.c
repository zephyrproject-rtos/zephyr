/*
 * Copyright 2024 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/shell/shell.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

static bool pm_device_filter(const struct device *dev)
{
	return dev->pm != NULL;
}

static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_filter(idx, pm_device_filter);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

static int pm_cmd_suspend(const struct shell *sh, size_t argc, char *argv[])
{
	const struct device *dev;
	int ret;

	dev = device_get_binding(argv[1]);
	if (dev == NULL) {
		shell_error(sh, "Invalid device: %s", argv[1]);
		return -ENODEV;
	}

	if (pm_device_runtime_is_enabled(dev)) {
		shell_error(sh, "Device %s uses runtime PM, use the runtime functions instead",
			    dev->name);
		return -EINVAL;
	}

	ret = pm_device_action_run(dev, PM_DEVICE_ACTION_SUSPEND);
	if (ret < 0) {
		shell_error(sh, "Device %s error: %d", "suspend", ret);
		return ret;
	}

	return 0;
}

static int pm_cmd_resume(const struct shell *sh, size_t argc, char *argv[])
{
	const struct device *dev;
	int ret;

	dev = device_get_binding(argv[1]);
	if (dev == NULL) {
		shell_error(sh, "Invalid device: %s", argv[1]);
		return -ENODEV;
	}

	if (pm_device_runtime_is_enabled(dev)) {
		shell_error(sh, "Device %s uses runtime PM, use the runtime functions instead",
			    dev->name);
		return -EINVAL;
	}

	ret = pm_device_action_run(dev, PM_DEVICE_ACTION_RESUME);
	if (ret < 0) {
		shell_error(sh, "Device %s error: %d", "resume", ret);
		return ret;
	}

	return 0;
}

#if defined(CONFIG_PM_DEVICE_RUNTIME)
static int pm_cmd_runtime_get(const struct shell *sh, size_t argc, char *argv[])
{
	const struct device *dev;
	int ret;

	dev = device_get_binding(argv[1]);
	if (dev == NULL) {
		shell_error(sh, "Invalid device: %s", argv[1]);
		return -ENODEV;
	}

	if (!pm_device_runtime_is_enabled(dev)) {
		shell_error(sh, "Device %s is not using runtime PM", dev->name);
		return -EINVAL;
	}

	ret = pm_device_runtime_get(dev);
	if (ret < 0) {
		shell_error(sh, "Device %s error: %d", "runtime get", ret);
		return ret;
	}

	return 0;
}

static int pm_cmd_runtime_put(const struct shell *sh, size_t argc, char *argv[])
{
	const struct device *dev;
	int ret;

	dev = device_get_binding(argv[1]);
	if (dev == NULL) {
		shell_error(sh, "Invalid device: %s", argv[1]);
		return -ENODEV;
	}

	if (!pm_device_runtime_is_enabled(dev)) {
		shell_error(sh, "Device %s is not using runtime PM", dev->name);
		return -EINVAL;
	}

	ret = pm_device_runtime_put(dev);
	if (ret < 0) {
		shell_error(sh, "Device %s error: %d", "runtime put", ret);
		return ret;
	}

	return 0;
}

static int pm_cmd_runtime_put_async(const struct shell *sh, size_t argc, char *argv[])
{
	const struct device *dev;
	int ret;

	dev = device_get_binding(argv[1]);
	if (dev == NULL) {
		shell_error(sh, "Invalid device: %s", argv[1]);
		return -ENODEV;
	}

	if (!pm_device_runtime_is_enabled(dev)) {
		shell_error(sh, "Device %s is not using runtime PM", dev->name);
		return -EINVAL;
	}

	ret = pm_device_runtime_put_async(dev, K_NO_WAIT);
	if (ret < 0) {
		shell_error(sh, "Device %s error: %d", "runtime put async", ret);
		return ret;
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE_RUNTIME */

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_pm_cmds,
	SHELL_CMD_ARG(suspend, &dsub_device_name,
		      "Call the PM suspend action on a device",
		      pm_cmd_suspend, 2, 0),
	SHELL_CMD_ARG(resume, &dsub_device_name,
		      "Call the PM resume action on a device",
		      pm_cmd_resume, 2, 0),
#if defined(CONFIG_PM_DEVICE_RUNTIME)
	SHELL_CMD_ARG(runtime-get, &dsub_device_name,
		      "Call the PM runtime get on a device",
		      pm_cmd_runtime_get, 2, 0),
	SHELL_CMD_ARG(runtime-put, &dsub_device_name,
		      "Call the PM runtime put on a device",
		      pm_cmd_runtime_put, 2, 0),
	SHELL_CMD_ARG(runtime-put-async, &dsub_device_name,
		      "Call the PM runtime put async on a device",
		      pm_cmd_runtime_put_async, 2, 0),
#endif /* CONFIG_PM_DEVICE_RUNTIME */
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(pm, &sub_pm_cmds, "PM commands", NULL);
