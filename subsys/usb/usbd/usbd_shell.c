/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/util.h>
#include <shell/shell.h>
#include <usb/usbd.h>
#include <usbd_internal.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(usbd_shell, CONFIG_USBD_LOG_LEVEL);

#define USBD_DEVICE_PREFIX "USBD_CLASS_"

static int cmd_register(const struct shell *shell,
			size_t argc, char **argv)
{
	struct device *dev;

	dev = device_get_binding(argv[1]);

	if (!dev) {
		shell_error(shell, "USBD Class %s not found", argv[1]);
		return -ENODEV;
	}

	if (usbd_cctx_register(dev)) {
		shell_error(shell, "Failed to register USBD Class %s", argv[1]);
		return -ENODEV;
	}

	shell_print(shell, "USBD Class %s registered", argv[1]);

	return 0;
}

static int cmd_unregister(const struct shell *shell,
			  size_t argc, char **argv)
{
	struct device *dev;

	dev = device_get_binding(argv[1]);

	if (!dev) {
		shell_error(shell, "USBD Class %s not found", argv[1]);
		return -ENODEV;
	}

	if (usbd_cctx_unregister(dev)) {
		shell_error(shell, "Failed to unregister USBD Class %s", argv[1]);
		return -ENODEV;
	}

	shell_print(shell, "USBD Class %s unregistered", argv[1]);

	return 0;
}

static int cmd_usbd_enable(const struct shell *shell,
			   size_t argc, char **argv)
{
	int err;

	err = usbd_enable(NULL);

	if (err == -EALREADY) {
		shell_error(shell, "USB already enabled");
	} else if (err) {
		shell_error(shell, "Failed to enable USB, error %d", err);
	} else {
		shell_print(shell, "USB enabled");
	}

	return err;
}

static int cmd_usbd_disable(const struct shell *shell,
			    size_t argc, char **argv)
{
	int err;

	err = usbd_disable();

	if (err) {
		shell_error(shell, "Failed to disable USB");
		return err;
	}

	shell_print(shell, "USB disabled");

	return 0;
}

static void device_name_get(size_t idx, struct shell_static_entry *entry);

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	struct device *dev = shell_device_lookup(idx, USBD_DEVICE_PREFIX);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_usbd_cmds,
			       SHELL_CMD(register, &dsub_device_name,
					 "Register USBD Class", cmd_register),
			       SHELL_CMD(unregister, &dsub_device_name,
					 "Unregister USBD Class", cmd_unregister),
			       SHELL_CMD(enable, NULL,
					 "Enable USB device stack",
					 cmd_usbd_enable),
			       SHELL_CMD(disable, NULL,
					 "Disable USB device stack",
					 cmd_usbd_disable),
			       SHELL_SUBCMD_SET_END
			       );

SHELL_CMD_REGISTER(usbd, &sub_usbd_cmds, "USBD commands", NULL);
