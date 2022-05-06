/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell.h>
#include <zephyr/init.h>
#include <string.h>
#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/arch_interface.h>

extern const struct device __device_PRE_KERNEL_1_start[];
extern const struct device __device_PRE_KERNEL_2_start[];
extern const struct device __device_POST_KERNEL_start[];
extern const struct device __device_APPLICATION_start[];
extern const struct device __device_end[];

#ifdef CONFIG_SMP
extern const struct device __device_SMP_start[];
#endif

static const struct device *levels[] = {
	__device_PRE_KERNEL_1_start,
	__device_PRE_KERNEL_2_start,
	__device_POST_KERNEL_start,
	__device_APPLICATION_start,
#ifdef CONFIG_SMP
	__device_SMP_start,
#endif
	/* End marker */
	__device_end,
};

static const char *get_device_name(const struct device *dev,
				   char *buf,
				   size_t len)
{
	const char *name = dev->name;

	if ((name == NULL) || (name[0] == 0)) {
		snprintf(buf, len, "[%p]", dev);
		name = buf;
	}

	return name;
}

static bool device_get_config_level(const struct shell *shell, int level)
{
	const struct device *dev;
	bool devices = false;
	char buf[20];

	for (dev = levels[level]; dev < levels[level+1]; dev++) {
		if (device_is_ready(dev)) {
			devices = true;

			shell_fprintf(shell, SHELL_NORMAL, "- %s\n",
				      get_device_name(dev, buf, sizeof(buf)));
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

#ifdef CONFIG_SMP
	shell_fprintf(shell, SHELL_NORMAL, "SMP:\n");
	ret = device_get_config_level(shell, _SYS_INIT_LEVEL_SMP);
	if (ret == false) {
		shell_fprintf(shell, SHELL_NORMAL, "- None\n");
	}
#endif /* CONFIG_SMP */

	return 0;
}

struct cmd_device_list_visitor_context {
	const struct shell *shell;
	char *buf;
	size_t buf_size;
};

static int cmd_device_list_visitor(const struct device *dev,
				   void *context)
{
	const struct cmd_device_list_visitor_context *ctx = context;

	shell_fprintf(ctx->shell, SHELL_NORMAL, "  requires: %s\n",
		      get_device_name(dev, ctx->buf, ctx->buf_size));

	return 0;
}

static int cmd_device_list(const struct shell *shell,
			   size_t argc, char **argv)
{
	const struct device *devlist;
	size_t devcnt = z_device_get_all_static(&devlist);
	const struct device *devlist_end = devlist + devcnt;
	const struct device *dev;
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_fprintf(shell, SHELL_NORMAL, "devices:\n");

	for (dev = devlist; dev < devlist_end; dev++) {
		char buf[20];
		const char *name = get_device_name(dev, buf, sizeof(buf));
		const char *state = "READY";

		shell_fprintf(shell, SHELL_NORMAL, "- %s", name);
		if (!device_is_ready(dev)) {
			state = "DISABLED";
		} else {
#ifdef CONFIG_PM_DEVICE
			enum pm_device_state st = PM_DEVICE_STATE_ACTIVE;
			int err = pm_device_state_get(dev, &st);

			if (!err) {
				state = pm_device_state_str(st);
			}
#endif /* CONFIG_PM_DEVICE */
		}

		shell_fprintf(shell, SHELL_NORMAL, " (%s)\n", state);
		if (!k_is_user_context()) {
			struct cmd_device_list_visitor_context ctx = {
				.shell = shell,
				.buf = buf,
				.buf_size = sizeof(buf),
			};

			(void)device_required_foreach(dev, cmd_device_list_visitor, &ctx);
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
