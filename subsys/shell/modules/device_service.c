/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Intel Corporation
 * Copyright (C) 2025 Bang & Olufsen A/S, Denmark
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
#include <zephyr/arch/arch_interface.h>

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

	shell_fprintf(sh, SHELL_NORMAL, "devices:\n");

	for (dev = devlist; dev < devlist_end; dev++) {
		char buf[Z_DEVICE_MAX_NAME_LEN];
		const char *name = get_device_name(dev, buf, sizeof(buf));
		const char *state = "READY";
		int usage;

		if (argc == 2
		    && strstr(name, argv[1]) == NULL) {
			continue;
		}

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

		usage = pm_device_runtime_usage(dev);
		if (usage >= 0) {
			shell_fprintf(sh, SHELL_NORMAL, " (%s, usage=%d)\n", state, usage);
		} else {
			shell_fprintf(sh, SHELL_NORMAL, " (%s)\n", state);
		}

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

#ifdef CONFIG_DEVICE_DT_METADATA
		const struct device_dt_nodelabels *nl = device_get_dt_nodelabels(dev);

		if (nl != NULL && nl->num_nodelabels > 0) {
			shell_fprintf(sh, SHELL_NORMAL, "  DT node labels:");
			for (size_t j = 0; j < nl->num_nodelabels; j++) {
				const char *nodelabel = nl->nodelabels[j];

				shell_fprintf(sh, SHELL_NORMAL, " %s", nodelabel);
			}
			shell_fprintf(sh, SHELL_NORMAL, "\n");
		}
#endif /* CONFIG_DEVICE_DT_METADATAa */
	}

	return 0;
}

static void device_name_lookup(size_t idx,
			       struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_lookup_all(idx, NULL);

	entry->syntax = dev != NULL ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = "device";
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name_lookup, device_name_lookup);


#ifdef CONFIG_DEVICE_SHELL_INIT_CMD
#ifdef CONFIG_DEVICE_DEPS
static int cmd_device_check_deps(const struct device *dev,
				 void *context)
{
	const struct cmd_device_list_visitor_context *ctx = context;

	if (!device_is_ready(dev)) {
		shell_error(ctx->sh, "Device %s is required, but not ready",
			    get_device_name(dev, ctx->buf, ctx->buf_size));
		return -EINVAL;
	}

	return 0;
}
#endif /* CONFIG_DEVICE_DEPS */

static int cmd_device_init(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int ret;

	dev = shell_device_get_binding_all(argv[1]);
	if (dev == NULL) {
		shell_error(sh, "Device unknown (%s)", argv[1]);
		return -ENODEV;
	}

	if (device_is_ready(dev)) {
		shell_info(sh, "Device %s is already initialized", argv[1]);
		return 0;
	}

#ifdef CONFIG_DEVICE_DEPS
	if (!k_is_user_context()) {
		char buf[Z_DEVICE_MAX_NAME_LEN];

		struct cmd_device_list_visitor_context ctx = {
			.sh = sh,
			.buf = buf,
			.buf_size = sizeof(buf),
		};

		ret = device_required_foreach(dev, cmd_device_check_deps, &ctx);
		if (ret < 0) {
			return ret;
		}
	}
#endif /* CONFIG_DEVICE_DEPS */

	ret = device_init(dev);
	if (ret != 0) {
		shell_error(sh, "Device %s initialization failed with err=%d",
			    argv[1], ret);
	} else {
		shell_info(sh, "Device %s initialized successfully", argv[1]);
	}

	return ret;
}

static void device_name_get_non_ready(size_t idx,
				      struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_lookup_non_ready(idx, NULL);

	entry->syntax = dev != NULL ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = "device";
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name_non_ready, device_name_get_non_ready);

#define DEVICE_INIT_CMD SHELL_CMD_ARG(init, &dsub_device_name_non_ready, \
				      "Manually initialize a device",	\
				      cmd_device_init, 2, 0),
#else
#define DEVICE_INIT_CMD
#endif /* CONFIG_DEVICE_SHELL_INIT_CMD */

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


#define LIST_CMD_HELP SHELL_HELP("List configured devices, with an optional filter", \
				 "[<device filter>]")

SHELL_STATIC_SUBCMD_SET_CREATE(sub_device,
	SHELL_CMD_ARG(list, &dsub_device_name_lookup,
		      LIST_CMD_HELP, cmd_device_list, 1, 1),
	DEVICE_INIT_CMD
	PM_SHELL_CMD
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(device, &sub_device, "Device commands", NULL);
