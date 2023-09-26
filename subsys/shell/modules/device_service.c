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
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/arch_interface.h>

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

#ifdef CONFIG_DEVICE_DEPS
struct cmd_device_list_visitor_context {
	const struct shell *sh;
	char *buf;
	size_t buf_size;
};

static int cmd_device_list_visitor(const struct device *dev,
				   void *context)
{
	const struct cmd_device_list_visitor_context *ctx = context;

	shell_fprintf(ctx->sh, SHELL_NORMAL, "  requires: %s\n",
		      get_device_name(dev, ctx->buf, ctx->buf_size));

	return 0;
}
#endif /* CONFIG_DEVICE_DEPS */

static int cmd_device_list(const struct shell *sh,
			   size_t argc, char **argv)
{
	const struct device *devlist;
	size_t devcnt = z_device_get_all_static(&devlist);
	const struct device *devlist_end = devlist + devcnt;
	const struct device *dev;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_fprintf(sh, SHELL_NORMAL, "devices:\n");

	for (dev = devlist; dev < devlist_end; dev++) {
		char buf[20];
		const char *name = get_device_name(dev, buf, sizeof(buf));
		const char *state = "READY";

		shell_fprintf(sh, SHELL_NORMAL, "- %s", name);
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

		shell_fprintf(sh, SHELL_NORMAL, " (%s)\n", state);
#ifdef CONFIG_DEVICE_DEPS
		if (!k_is_user_context()) {
			struct cmd_device_list_visitor_context ctx = {
				.sh = sh,
				.buf = buf,
				.buf_size = sizeof(buf),
			};

			(void)device_required_foreach(dev, cmd_device_list_visitor, &ctx);
		}
#endif /* CONFIG_DEVICE_DEPS */
	}

	return 0;
}

#ifdef CONFIG_PM_DEVICE_RUNTIME
static int cmd_device_pm_toggle(const struct shell *sh,
			 size_t argc, char **argv)
{
	const struct device *dev;
	enum pm_device_state pm_state;

	dev = device_get_binding(argv[1]);
	if (dev == NULL) {
		shell_error(sh, "Device unknown (%s)", argv[1]);
		return -ENODEV;
	}

	if (!pm_device_runtime_is_enabled(dev)) {
		shell_error(sh, "Device (%s) does not have runtime power management",
			    argv[1]);
		return -ENOTSUP;
	}

	(void)pm_device_state_get(dev, &pm_state);

	if (pm_state == PM_DEVICE_STATE_ACTIVE) {
		shell_fprintf(sh, SHELL_NORMAL, "pm_device_runtime_put(%s)\n",
			      argv[1]);
		pm_device_runtime_put(dev);
	} else {
		shell_fprintf(sh, SHELL_NORMAL, "pm_device_runtime_get(%s)\n",
			      argv[1]);
		pm_device_runtime_get(dev);
	}

	return 0;
}
#define PM_SHELL_CMD SHELL_CMD(pm_toggle, NULL, "Toggle device power (pm get/put)",\
			       cmd_device_pm_toggle),
#else
#define PM_SHELL_CMD
#endif /* CONFIG_PM_DEVICE_RUNTIME  */



SHELL_STATIC_SUBCMD_SET_CREATE(sub_device,
	SHELL_CMD(list, NULL, "List configured devices", cmd_device_list),
	PM_SHELL_CMD
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(device, &sub_device, "Device commands", NULL);
