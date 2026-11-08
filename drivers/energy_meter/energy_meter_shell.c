/*
 * Copyright (c) 2025 Pedro Ramos
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>

#include <zephyr/drivers/energy_meter.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/util.h>

#define VALUE_LEN 24
#define LABEL_LEN 32

struct quantity_desc {
	const char *name;
	const char *unit;
};

static const struct quantity_desc quantities[] = {
	[ENERGY_METER_VOLTAGE] = {"voltage", "V"},
	[ENERGY_METER_VOLTAGE_LL] = {"voltage_ll", "V"},
	[ENERGY_METER_CURRENT] = {"current", "A"},
	[ENERGY_METER_ACTIVE_POWER] = {"active_power", "W"},
	[ENERGY_METER_REACTIVE_POWER] = {"reactive_power", "var"},
	[ENERGY_METER_APPARENT_POWER] = {"apparent_power", "VA"},
	[ENERGY_METER_POWER_FACTOR] = {"power_factor", ""},
	[ENERGY_METER_PHASE_ANGLE] = {"phase_angle", "deg"},
	[ENERGY_METER_VOLTAGE_ANGLE] = {"voltage_angle", "deg"},
	[ENERGY_METER_FREQUENCY] = {"frequency", "Hz"},
	[ENERGY_METER_ACTIVE_ENERGY_IMPORT] = {"active_energy_import", "Wh"},
	[ENERGY_METER_ACTIVE_ENERGY_EXPORT] = {"active_energy_export", "Wh"},
	[ENERGY_METER_REACTIVE_ENERGY_IMPORT] = {"reactive_energy_import", "varh"},
	[ENERGY_METER_REACTIVE_ENERGY_EXPORT] = {"reactive_energy_export", "varh"},
	[ENERGY_METER_APPARENT_ENERGY] = {"apparent_energy", "VAh"},
	[ENERGY_METER_TEMPERATURE] = {"temperature", "C"},
};

BUILD_ASSERT(ARRAY_SIZE(quantities) == ENERGY_METER_QUANTITY_COMMON_COUNT,
	     "every common quantity needs a name");

static const char *const line_names[ENERGY_METER_LINE_COUNT] = {
	[ENERGY_METER_LINE_A] = "a",         [ENERGY_METER_LINE_B] = "b",
	[ENERGY_METER_LINE_C] = "c",         [ENERGY_METER_LINE_N] = "n",
	[ENERGY_METER_LINE_TOTAL] = "total",
};

static bool device_is_energy_meter(const struct device *dev)
{
	return DEVICE_API_IS(energy_meter, dev);
}

static const struct device *get_meter(const struct shell *sh, const char *name)
{
	const struct device *dev = shell_device_get_binding(name);

	if ((dev == NULL) || !device_is_energy_meter(dev)) {
		shell_error(sh, "%s is not an energy meter", name);
		return NULL;
	}

	if (!device_is_ready(dev)) {
		shell_error(sh, "%s is not ready", name);
		return NULL;
	}

	return dev;
}

/* Format a value in millionths of its unit with three decimals */
static const char *format_micro(char *buf, int64_t value)
{
	uint64_t magnitude = (value < 0) ? (uint64_t)(-(value + 1)) + 1U : (uint64_t)value;

	snprintk(buf, VALUE_LEN, "%s%llu.%03llu", (value < 0) ? "-" : "",
		 (unsigned long long)(magnitude / 1000000U),
		 (unsigned long long)((magnitude % 1000000U) / 1000U));

	return buf;
}

static int parse_line(const struct shell *sh, const char *arg, uint8_t *line)
{
	for (uint8_t i = 0; i < ENERGY_METER_LINE_COUNT; i++) {
		if (strcmp(arg, line_names[i]) == 0) {
			*line = i;
			return 0;
		}
	}

	shell_error(sh, "Unknown line %s; use a, b, c, n or total", arg);

	return -EINVAL;
}

static int parse_quantity(const struct shell *sh, const char *arg, uint8_t *quantity)
{
	char *end;
	unsigned long num;

	for (uint8_t i = 0; i < ARRAY_SIZE(quantities); i++) {
		if (strcmp(arg, quantities[i].name) == 0) {
			*quantity = i;
			return 0;
		}
	}

	/* Device-specific quantities are given by number */
	num = strtoul(arg, &end, 0);
	if ((*end != '\0') || (num > ENERGY_METER_QUANTITY_MAX)) {
		shell_error(sh, "Unknown quantity %s", arg);
		return -EINVAL;
	}

	*quantity = (uint8_t)num;

	return 0;
}

static int cmd_info(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = get_meter(sh, argv[1]);
	struct energy_meter_info info;
	int ret;

	ARG_UNUSED(argc);

	if (dev == NULL) {
		return -ENODEV;
	}

	ret = energy_meter_get_info(dev, &info);
	if (ret < 0) {
		shell_error(sh, "Failed to get info (%d)", ret);
		return ret;
	}

	shell_print(sh, "phases: %u", info.phases);

	for (uint8_t line = 0; line < ENERGY_METER_LINE_COUNT; line++) {
		if (info.quantities[line] == 0U) {
			continue;
		}

		shell_fprintf_normal(sh, "%-6s", line_names[line]);
		for (uint8_t q = 0; q < ARRAY_SIZE(quantities); q++) {
			if ((info.quantities[line] & BIT(q)) != 0U) {
				shell_fprintf_normal(sh, " %s", quantities[q].name);
			}
		}
		shell_fprintf_normal(sh, "\n");
	}

	return 0;
}

/* Kept off the shell thread stack; shared by every shell instance */
static struct energy_meter_snapshot snap;
static K_MUTEX_DEFINE(snap_lock);

static int print_snapshot_locked(const struct shell *sh, const struct device *dev)
{
	char label[LABEL_LEN];
	char buf[VALUE_LEN];
	int ret;

	ret = energy_meter_get_snapshot(dev, &snap);
	if (ret < 0) {
		shell_error(sh, "Failed to read %s (%d)", dev->name, ret);
		return ret;
	}

	shell_fprintf_normal(sh, "%-*s", LABEL_LEN, "");
	for (uint8_t line = 0; line < ENERGY_METER_LINE_COUNT; line++) {
		if (snap.valid[line] != 0U) {
			shell_fprintf_normal(sh, " %14s", line_names[line]);
		}
	}
	shell_fprintf_normal(sh, "\n");

	for (uint8_t q = 0; q < ARRAY_SIZE(quantities); q++) {
		bool any = false;

		for (uint8_t line = 0; line < ENERGY_METER_LINE_COUNT; line++) {
			any = any || ((snap.valid[line] & BIT(q)) != 0U);
		}

		if (!any) {
			continue;
		}

		if (quantities[q].unit[0] != '\0') {
			snprintk(label, sizeof(label), "%s [%s]", quantities[q].name,
				 quantities[q].unit);
		} else {
			snprintk(label, sizeof(label), "%s", quantities[q].name);
		}
		shell_fprintf_normal(sh, "%-*s", LABEL_LEN, label);

		for (uint8_t line = 0; line < ENERGY_METER_LINE_COUNT; line++) {
			if (snap.valid[line] == 0U) {
				continue;
			}

			if ((snap.valid[line] & BIT(q)) == 0U) {
				shell_fprintf_normal(sh, " %14s", "-");
				continue;
			}

			shell_fprintf_normal(
				sh, " %14s",
				format_micro(buf, energy_meter_line_value(&snap.line[line], q)));
		}
		shell_fprintf_normal(sh, "\n");
	}

	return 0;
}

static int print_snapshot(const struct shell *sh, const struct device *dev)
{
	int ret;

	(void)k_mutex_lock(&snap_lock, K_FOREVER);
	ret = print_snapshot_locked(sh, dev);
	(void)k_mutex_unlock(&snap_lock);

	return ret;
}

static int cmd_get(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = get_meter(sh, argv[1]);

	ARG_UNUSED(argc);

	if (dev == NULL) {
		return -ENODEV;
	}

	return print_snapshot(sh, dev);
}

static int cmd_watch(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = get_meter(sh, argv[1]);
	unsigned long period_ms = 1000;
	unsigned long count = 10;
	int ret = 0;

	if (dev == NULL) {
		return -ENODEV;
	}

	if (argc > 2) {
		period_ms = shell_strtoul(argv[2], 0, &ret);
	}

	if ((ret == 0) && (argc > 3)) {
		count = shell_strtoul(argv[3], 0, &ret);
	}

	if ((ret < 0) || (period_ms == 0U) || (period_ms > INT32_MAX)) {
		shell_error(sh, "Invalid period or count");
		return -EINVAL;
	}

	for (unsigned long i = 0; i < count; i++) {
		if (i > 0U) {
			k_msleep(period_ms);
			shell_print(sh, "");
		}

		ret = print_snapshot(sh, dev);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int cmd_read(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = get_meter(sh, argv[1]);
	uint8_t line = ENERGY_METER_LINE_TOTAL;
	char buf[VALUE_LEN];
	uint8_t quantity;
	int64_t value;
	int ret;

	if (dev == NULL) {
		return -ENODEV;
	}

	ret = parse_quantity(sh, argv[2], &quantity);
	if (ret < 0) {
		return ret;
	}

	if (argc > 3) {
		ret = parse_line(sh, argv[3], &line);
		if (ret < 0) {
			return ret;
		}
	}

	ret = energy_meter_get_one(dev, quantity, line, &value);
	if (ret < 0) {
		shell_error(sh, "Failed to read quantity %u on line %s (%d)", quantity,
			    line_names[line], ret);
		return ret;
	}

	if (quantity < ARRAY_SIZE(quantities)) {
		shell_print(sh, "%s %s", format_micro(buf, value), quantities[quantity].unit);
	} else {
		shell_print(sh, "%lld (0x%llx)", (long long)value, (unsigned long long)value);
	}

	return 0;
}

static int cmd_reset_energy(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = get_meter(sh, argv[1]);
	int ret;

	ARG_UNUSED(argc);

	if (dev == NULL) {
		return -ENODEV;
	}

	ret = energy_meter_reset_energy(dev);
	if (ret < 0) {
		shell_error(sh, "Failed to reset energy (%d)", ret);
	}

	return ret;
}

static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_filter(idx, device_is_energy_meter);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

SHELL_STATIC_SUBCMD_SET_CREATE(
	energy_meter_cmds,
	SHELL_CMD_ARG(info, &dsub_device_name, "List the measured values\nUsage: info <device>",
		      cmd_info, 2, 0),
	SHELL_CMD_ARG(get, &dsub_device_name, "Read all values\nUsage: get <device>", cmd_get, 2,
		      0),
	SHELL_CMD_ARG(watch, &dsub_device_name,
		      "Read all values periodically\n"
		      "Usage: watch <device> [period_ms (1000)] [count (10)]",
		      cmd_watch, 2, 2),
	SHELL_CMD_ARG(read, &dsub_device_name,
		      "Read one value; device-specific quantities are given by number\n"
		      "Usage: read <device> <quantity> [a|b|c|n|total (total)]",
		      cmd_read, 3, 1),
	SHELL_CMD_ARG(reset_energy, &dsub_device_name,
		      "Reset the accumulated energy\nUsage: reset_energy <device>",
		      cmd_reset_energy, 2, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(energy_meter, &energy_meter_cmds, "Energy meter commands", NULL);
