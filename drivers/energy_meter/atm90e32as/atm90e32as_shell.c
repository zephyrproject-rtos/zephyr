/*
 * Copyright (c) 2025 Pedro Ramos
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell.h>

#include "atm90e32as.h"

#define ATM90E32AS_REG_MAX 0xFFU

static const struct device *get_device(const struct shell *sh, const char *name)
{
	const struct device *dev = shell_device_get_binding(name);

	if ((dev == NULL) || !atm90e32as_is_device(dev) || !device_is_ready(dev)) {
		shell_error(sh, "%s is not a ready ATM90E32AS", name);
		return NULL;
	}

	return dev;
}

static int parse_u16(const struct shell *sh, const char *arg, unsigned long max, unsigned long *val)
{
	int ret = 0;

	*val = shell_strtoul(arg, 0, &ret);
	if ((ret < 0) || (*val > max)) {
		shell_error(sh, "Invalid value %s", arg);
		return -EINVAL;
	}

	return 0;
}

static int cmd_read(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = get_device(sh, argv[1]);
	unsigned long addr;
	unsigned long count = 1;
	uint16_t val;
	int ret;

	if (dev == NULL) {
		return -ENODEV;
	}

	ret = parse_u16(sh, argv[2], ATM90E32AS_REG_MAX, &addr);
	if ((ret == 0) && (argc > 3)) {
		ret = parse_u16(sh, argv[3], ATM90E32AS_REG_MAX + 1U - addr, &count);
	}

	if (ret < 0) {
		return ret;
	}

	for (unsigned long i = 0; i < count; i++) {
		ret = atm90e32as_debug_read(dev, (uint8_t)(addr + i), &val);
		if (ret < 0) {
			shell_error(sh, "Read of 0x%02lx failed (%d)", addr + i, ret);
			return ret;
		}

		shell_print(sh, "0x%02lx: 0x%04x", addr + i, val);
	}

	return 0;
}

static int cmd_write(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = get_device(sh, argv[1]);
	unsigned long addr;
	unsigned long val;
	int ret;

	ARG_UNUSED(argc);

	if (dev == NULL) {
		return -ENODEV;
	}

	ret = parse_u16(sh, argv[2], ATM90E32AS_REG_MAX, &addr);
	if (ret == 0) {
		ret = parse_u16(sh, argv[3], UINT16_MAX, &val);
	}

	if (ret < 0) {
		return ret;
	}

	ret = atm90e32as_debug_write(dev, (uint8_t)addr, (uint16_t)val);
	if (ret < 0) {
		shell_error(sh, "Write of 0x%02lx failed (%d)", addr, ret);
	}

	return ret;
}

static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_filter(idx, atm90e32as_is_device);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

SHELL_STATIC_SUBCMD_SET_CREATE(
	atm90e32as_cmds,
	SHELL_CMD_ARG(read, &dsub_device_name,
		      "Read registers; energy registers are added to the driver totals\n"
		      "Usage: read <device> <address> [count]",
		      cmd_read, 3, 1),
	SHELL_CMD_ARG(write, &dsub_device_name,
		      "Write a register until the next reset or resume\n"
		      "Usage: write <device> <address> <value>",
		      cmd_write, 4, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(atm90e32as, &atm90e32as_cmds, "ATM90E32AS register access", NULL);
