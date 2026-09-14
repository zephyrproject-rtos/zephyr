/*
 * Copyright (c) 2026 Siemens AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/relay/relay.h>

#include <zephyr/device.h>
#include <zephyr/shell/shell.h>

#include <errno.h>

static bool device_is_relay(const struct device *dev)
{
	return DEVICE_API_IS(relay, dev);
}

static int relay_set(const struct shell *sh, char **argv, enum relay_state state)
{
	const struct device *dev = shell_device_get_binding(argv[1]);
	int err;

	if (dev == NULL || !device_is_relay(dev)) {
		shell_error(sh, "Relay device %s not available", argv[1]);
		return -ENODEV;
	}

	err = relay_set_state(dev, state);
	if (err != 0) {
		shell_error(sh, "Failed to set relay %s (err %d)", dev->name, err);
	}

	return err;
}

static int cmd_on(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	return relay_set(sh, argv, RELAY_STATE_ON);
}

static int cmd_off(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	return relay_set(sh, argv, RELAY_STATE_OFF);
}

static int cmd_get(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = shell_device_get_binding(argv[1]);
	enum relay_state state;
	int err;

	ARG_UNUSED(argc);

	if (dev == NULL || !device_is_relay(dev)) {
		shell_error(sh, "Relay device %s not available", argv[1]);
		return -ENODEV;
	}

	err = relay_get_state(dev, &state);
	if (err != 0) {
		shell_error(sh, "Failed to read relay %s (err %d)", dev->name, err);
		return err;
	}

	shell_print(sh, "%s", state == RELAY_STATE_ON ? "on" : "off");
	return 0;
}

static int cmd_list(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	size_t idx = 0;
	bool found = false;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	while ((dev = shell_device_filter(idx++, device_is_relay)) != NULL) {
		shell_print(sh, "%s", dev->name);
		found = true;
	}

	if (!found) {
		shell_print(sh, "No relay devices found");
	}

	return 0;
}

static void relay_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_filter(idx, device_is_relay);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_relay_name, relay_name_get);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_relay,
	SHELL_CMD_ARG(list, NULL, SHELL_HELP("List available relay devices", NULL), cmd_list, 1, 0),
	SHELL_CMD_ARG(on, &dsub_relay_name, SHELL_HELP("Turn a relay on", "<device>"), cmd_on, 2,
		      0),
	SHELL_CMD_ARG(off, &dsub_relay_name, SHELL_HELP("Turn a relay off", "<device>"), cmd_off, 2,
		      0),
	SHELL_CMD_ARG(get, &dsub_relay_name, SHELL_HELP("Read back a relay state", "<device>"),
		      cmd_get, 2, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(relay, &sub_relay, "Relay control commands", NULL);
