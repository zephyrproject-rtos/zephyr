/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdlib.h>

#include <zephyr/cmis/cmis.h>
#include <zephyr/device.h>
#include <zephyr/drivers/cmis.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/util.h>

struct cmis_flag_name {
	uint8_t bit;
	const char *name;
};

static const struct cmis_flag_name cmis_flag_names[] = {
	{CMIS_FLAG_TEMP_HIGH_ALARM, "temp-high-alarm"},
	{CMIS_FLAG_TEMP_LOW_ALARM, "temp-low-alarm"},
	{CMIS_FLAG_TEMP_HIGH_WARNING, "temp-high-warning"},
	{CMIS_FLAG_TEMP_LOW_WARNING, "temp-low-warning"},
	{CMIS_FLAG_VCC_HIGH_ALARM, "vcc-high-alarm"},
	{CMIS_FLAG_VCC_LOW_ALARM, "vcc-low-alarm"},
	{CMIS_FLAG_VCC_HIGH_WARNING, "vcc-high-warning"},
	{CMIS_FLAG_VCC_LOW_WARNING, "vcc-low-warning"},
};

static bool device_is_cmis(const struct device *dev)
{
	return DEVICE_API_IS(cmis, dev);
}

static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_filter(idx, device_is_cmis);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_cmis_device, device_name_get);

static const struct device *cmis_shell_device(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;

	if (argc < 2) {
		shell_error(sh, "missing device argument");
		return NULL;
	}

	dev = shell_device_get_binding(argv[1]);
	if (dev == NULL) {
		shell_error(sh, "device %s not found", argv[1]);
	}

	return dev;
}

static int cmd_cmis_info(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = cmis_shell_device(sh, argc, argv);
	struct cmis_id id;
	int ret;

	if (dev == NULL) {
		return -ENODEV;
	}

	ret = cmis_read_id(dev, &id);
	if (ret != 0) {
		shell_error(sh, "failed to read module ID (%d)", ret);
		return ret;
	}

	shell_print(sh, "Identifier : %s (0x%02x)", cmis_identifier_name(id.identifier),
		    id.identifier);
	shell_print(sh, "CMIS rev   : %u.%u", id.cmis_major, id.cmis_minor);
	shell_print(sh, "State      : %s (0x%02x)", cmis_module_state_name(id.module_state),
		    id.module_state);
	shell_print(sh, "Media      : %s (0x%02x)", cmis_media_type_name(id.media_type),
		    id.media_type);
	shell_print(sh, "Vendor     : %s", id.vendor_name);
	shell_print(sh, "Part number: %s", id.vendor_pn);
	shell_print(sh, "Revision   : %s", id.vendor_rev);
	shell_print(sh, "Serial     : %s", id.vendor_sn);
	shell_print(sh, "Date code  : %s", id.date_code);
	shell_print(sh, "Power class: %u", id.power_class);
	shell_print(sh, "Max power  : %u mW", id.max_power_mw);

	return 0;
}

static int cmd_cmis_mon(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = cmis_shell_device(sh, argc, argv);
	struct cmis_monitors mon;
	int ret;

	if (dev == NULL) {
		return -ENODEV;
	}

	ret = cmis_read_monitors(dev, &mon);
	if (ret != 0) {
		shell_error(sh, "failed to read monitors (%d)", ret);
		return ret;
	}

	shell_print(sh, "Temperature: %d.%03d C", mon.temp_mdegc / 1000,
		    abs(mon.temp_mdegc % 1000));
	shell_print(sh, "Vcc        : %u.%03u V", mon.vcc_uv / 1000000U,
		    (mon.vcc_uv / 1000U) % 1000U);

	shell_fprintf(sh, SHELL_NORMAL, "Flags      : 0x%02x", mon.flags);
	for (size_t i = 0U; i < ARRAY_SIZE(cmis_flag_names); i++) {
		if ((mon.flags & cmis_flag_names[i].bit) != 0U) {
			shell_fprintf(sh, SHELL_NORMAL, " %s", cmis_flag_names[i].name);
		}
	}
	shell_fprintf(sh, SHELL_NORMAL, "\n");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(cmis_cmds,
	SHELL_CMD_ARG(info, &dsub_cmis_device,
		      "Show module identification\nUsage: cmis info <device>", cmd_cmis_info, 2, 0),
	SHELL_CMD_ARG(mon, &dsub_cmis_device,
		      "Show module-level monitors\nUsage: cmis mon <device>", cmd_cmis_mon, 2, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(cmis, &cmis_cmds, "CMIS pluggable module commands", NULL);
