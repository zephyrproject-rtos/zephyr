/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <shell/shell.h>
#include <init.h>
#include <string.h>
#include <device.h>

extern struct device __device_init_start[];
extern struct device __device_PRE_KERNEL_1_start[];
extern struct device __device_PRE_KERNEL_2_start[];
extern struct device __device_POST_KERNEL_start[];
extern struct device __device_APPLICATION_start[];
extern struct device __device_init_end[];

static struct device *config_levels[] = {
	__device_PRE_KERNEL_1_start,
	__device_PRE_KERNEL_2_start,
	__device_POST_KERNEL_start,
	__device_APPLICATION_start,
	/* End marker */
	__device_init_end,
};

static bool device_get_config_level(const struct shell *shell, int level)
{
	struct device *info;
	bool devices = false;

	for (info = config_levels[level]; info < config_levels[level+1];
								info++) {
		if (info->driver_api != NULL) {
			devices = true;
			shell_fprintf(shell, SHELL_NORMAL, "- %s\n",
					info->config->name);
		}
	}
	return devices;
}

static int cmd_device_levels(const struct shell *shell,
			      size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	bool ret;

	shell_fprintf(shell, SHELL_NORMAL, "POST_KERNEL:\n");
	ret = device_get_config_level(shell, _SYS_INIT_LEVEL_POST_KERNEL);
	if (ret == false) {
		shell_fprintf(shell, SHELL_NORMAL, "- None\n");
	}

	shell_fprintf(shell, SHELL_NORMAL, "APPLICATION:\n");
	ret = device_get_config_level(shell, _SYS_INIT_LEVEL_APPLICATION);
	if (ret == false) {
		shell_fprintf(shell, SHELL_NORMAL, "- None\n");
	}

	shell_fprintf(shell, SHELL_NORMAL, "PRE KERNEL 1:\n");
	ret = device_get_config_level(shell, _SYS_INIT_LEVEL_PRE_KERNEL_1);
	if (ret == false) {
		shell_fprintf(shell, SHELL_NORMAL, "- None\n");
	}

	shell_fprintf(shell, SHELL_NORMAL, "PRE KERNEL 2:\n");
	ret = device_get_config_level(shell, _SYS_INIT_LEVEL_PRE_KERNEL_2);
	if (ret == false) {
		shell_fprintf(shell, SHELL_NORMAL, "- None\n");
	}

	return 0;
}

static int cmd_device_list(const struct shell *shell,
			      size_t argc, char **argv)
{
	struct device *info;
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_fprintf(shell, SHELL_NORMAL, "devices:\n");
	for (info = __device_init_start; info != __device_init_end; info++) {
		if (info->driver_api != NULL) {
			shell_fprintf(shell, SHELL_NORMAL, "- %s\n",
					info->config->name);
		}
	}

	return 0;
}


SHELL_STATIC_SUBCMD_SET_CREATE(sub_device,
	SHELL_CMD(levels, NULL, "List configured devices by levels", cmd_device_levels),
	SHELL_CMD(list, NULL, "List configured devices", cmd_device_list),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(device, &sub_device, "Device commands", NULL);
