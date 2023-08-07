/*
 * Copyright (c) 2023 FTP Technologies
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/pm/pm.h>
#include <zephyr/shell/shell.h>

#include <stdlib.h>

#define PM_DEVICE_RUNTIME_AUTO_ENABLE_HELP                                                         \
	"Auto enable device runtime based on devicetree properties for 1 "                         \
	"or more devices. Syntax:\n <device 0> [device 1] .. [device N]"

#define PM_DEVICE_RUNTIME_ENABLE_HELP                                                              \
	"Enable device runtime PM on 1 or more devices. "                                          \
	"Syntax:\n <device 0> [device 1] .. [device N]"

#define PM_DEVICE_RUNTIME_DISABLE_HELP                                                             \
	"Disable device runtime PM on 1 or more devices. "                                         \
	"Syntax:\n <device 0> [device 1] .. [device N]"

#define PM_DEVICE_RUNTIME_GET_HELP                                                                 \
	"Resume 1 or more devices based on usage count. "                                          \
	"Syntax:\n <device 0> [device 1] .. [device N]"

#define PM_DEVICE_RUNTIME_PUT_HELP                                                                 \
	"Suspend 1 or more devices based on usage count. "                                         \
	"Syntax:\n <device 0> [device 1] .. [device N]"

#define PM_DEVICE_RUNTIME_IS_ENABLED_HELP                                                          \
	"Check if device runtime is enabled for 1 or more devices. "                               \
	"Syntax:\n <device 0> [device 1] .. [device N]"

static int cmd_runtime(const struct shell *sh, size_t argc, char *argv[],
		       int (api)(const struct device *), char *action)
{
	const struct device *dev;
	int i, ret;

	__ASSERT_NO_MSG(api != NULL);
	__ASSERT_NO_MSG(action != NULL);

	for (i = 1; i < argc; i++) {
		dev = device_get_binding(argv[i]);
		if (dev == NULL) {
			shell_error(sh, "device %s unknown", argv[i]);
			continue;
		}
		if (!device_is_ready(dev)) {
			shell_error(sh, "device %s not ready", argv[1]);
			continue;
		}
		ret = api(dev);
		if (ret != 0) {
			shell_error(sh, "failed to %s %s: %d", action, argv[i], ret);
		}
	}

	return 0;
}

static int cmd_runtime_auto_enable(const struct shell *sh, size_t argc, char *argv[])
{
	return cmd_runtime(sh, argc, argv, pm_device_runtime_auto_enable, "auto enable");
}

static int cmd_runtime_enable(const struct shell *sh, size_t argc, char *argv[])
{
	return cmd_runtime(sh, argc, argv, pm_device_runtime_enable, "enable");
}

static int cmd_runtime_disable(const struct shell *sh, size_t argc, char *argv[])
{
	return cmd_runtime(sh, argc, argv, pm_device_runtime_disable, "disable");
}

static int cmd_runtime_get_devices(const struct shell *sh, size_t argc, char *argv[])
{
	return cmd_runtime(sh, argc, argv, pm_device_runtime_get, "get");
}

static int cmd_runtime_put_devices(const struct shell *sh, size_t argc, char *argv[])
{
	return cmd_runtime(sh, argc, argv, pm_device_runtime_put, "put");
}

static int cmd_runtime_is_enabled(const struct shell *sh, size_t argc, char *argv[])
{
	const struct device *dev;
	bool is_enabled;
	int i;

	for (i = 1; i < argc; i++) {
		dev = device_get_binding(argv[i]);
		if (dev == NULL) {
			shell_error(sh, "device %s unknown", argv[i]);
			continue;
		}
		if (!device_is_ready(dev)) {
			shell_error(sh, "device %s not ready", argv[1]);
			continue;
		}
		is_enabled = pm_device_runtime_is_enabled(dev);
		shell_print(sh, "%s", is_enabled ? "true" : "false");
	}

	return 0;
}

/* Device name autocompletion support */
static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_lookup(idx, NULL);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

/* clang-format off */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_device_runtime,
	SHELL_CMD_ARG(auto_enable, &dsub_device_name, PM_DEVICE_RUNTIME_AUTO_ENABLE_HELP,
		      cmd_runtime_auto_enable, 2, 255),
	SHELL_CMD_ARG(enable, &dsub_device_name, PM_DEVICE_RUNTIME_ENABLE_HELP,
		      cmd_runtime_enable, 2, 255),
	SHELL_CMD_ARG(disable, &dsub_device_name, PM_DEVICE_RUNTIME_DISABLE_HELP,
		      cmd_runtime_disable, 2, 255),
	SHELL_CMD_ARG(get, &dsub_device_name, PM_DEVICE_RUNTIME_GET_HELP,
		      cmd_runtime_get_devices, 2, 255),
	SHELL_CMD_ARG(put, &dsub_device_name, PM_DEVICE_RUNTIME_PUT_HELP,
		      cmd_runtime_put_devices, 2, 255),
	SHELL_CMD_ARG(is_enabled, &dsub_device_name, PM_DEVICE_RUNTIME_IS_ENABLED_HELP,
		      cmd_runtime_is_enabled, 2, 255),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_pm,
	SHELL_COND_CMD_ARG(CONFIG_PM_DEVICE_RUNTIME, device_runtime,
			   &sub_device_runtime, NULL, NULL, 2, 255),
	SHELL_SUBCMD_SET_END
);
/* clang-format on */

SHELL_CMD_REGISTER(pm, &sub_pm, "Power Management commands", NULL);
