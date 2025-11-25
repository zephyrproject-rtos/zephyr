/*
 * Copyright (c) 2025 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SYSCON shell commands.
 */

#include <zephyr/shell/shell.h>
#include <zephyr/drivers/syscon.h>

static int cmd_base(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	uintptr_t base;
	int ret;

	dev = shell_device_get_binding(argv[1]);
	if (!device_is_ready(dev)) {
		shell_error(sh, "SYSCON device not ready");
		return -ENODEV;
	}

	ret = syscon_get_base(dev, &base);
	if (ret < 0) {
		shell_error(sh, "Failed to get SYSCON base (%d)", ret);
		return ret;
	}

	shell_print(sh, "0x%lx", base);
	return 0;
}

static int cmd_read(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	unsigned long addr;
	uint32_t val;
	int ret;

	dev = shell_device_get_binding(argv[1]);
	if (!device_is_ready(dev)) {
		shell_error(sh, "SYSCON device not ready");
		return -ENODEV;
	}

	addr = shell_strtoul(argv[2], 0, &ret);
	if (ret < 0 || addr > UINT16_MAX) {
		shell_error(sh, "Invalid address %s (%d)", argv[2], ret);
		return -EINVAL;
	}

	ret = syscon_read_reg(dev, addr, &val);
	if (ret < 0) {
		shell_error(sh, "Failed to read (%d)", ret);
		return ret;
	}

	shell_print(sh, "0x%x", val);
	return 0;
}

static int cmd_write(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	unsigned long addr, val;
	int ret;

	dev = shell_device_get_binding(argv[1]);
	if (!device_is_ready(dev)) {
		shell_error(sh, "SYSCON device not ready");
		return -ENODEV;
	}

	addr = shell_strtoul(argv[2], 0, &ret);
	if (ret < 0 || addr > UINT16_MAX) {
		shell_error(sh, "Invalid address %s (%d)", argv[2], ret);
		return -EINVAL;
	}

	val = shell_strtoul(argv[3], 0, &ret);
	if (ret < 0 || val > UINT32_MAX) {
		shell_error(sh, "Invalid value %s (%d)", argv[3], ret);
		return -EINVAL;
	}

	ret = syscon_write_reg(dev, addr, val);
	if (ret < 0) {
		shell_error(sh, "Failed to write (%d)", ret);
		return ret;
	}

	return 0;
}

static int cmd_size(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	size_t size;
	int ret;

	dev = shell_device_get_binding(argv[1]);
	if (!device_is_ready(dev)) {
		shell_error(sh, "SYSCON device not ready");
		return -ENODEV;
	}

	ret = syscon_get_size(dev, &size);
	if (ret < 0) {
		shell_error(sh, "Failed to get SYSCON size (%d)", ret);
		return ret;
	}

	shell_print(sh, "%zu bytes", size);
	return 0;
}

static bool device_is_syscon(const struct device *dev)
{
	return DEVICE_API_IS(syscon, dev);
}

/* Device name autocompletion support */
static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_filter(idx, device_is_syscon);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

SHELL_STATIC_SUBCMD_SET_CREATE(syscon_cmds,
	SHELL_CMD_ARG(base, &dsub_device_name,
		      SHELL_HELP("Get the SYSCON device base address", "<device>"),
		      cmd_base, 2, 0),
	SHELL_CMD_ARG(read, &dsub_device_name,
		      SHELL_HELP("Read from a SYSCON device register", "<device> <address>"),
		      cmd_read, 3, 0),
	SHELL_CMD_ARG(write, &dsub_device_name,
		      SHELL_HELP("Write to a SYSCON device register", "<device> <address> <value>"),
		      cmd_write, 4, 0),
	SHELL_CMD_ARG(size, &dsub_device_name,
		      SHELL_HELP("Print the SYSCON device size in bytes", "<device>"),
		      cmd_size, 2, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(syscon, &syscon_cmds, "SYSCON shell commands", NULL);
