/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/drivers/sfp.h>
#include <zephyr/sff/sff8472.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/util.h>

struct sfp_flag_name {
	uint16_t bit;
	const char *name;
};

static const struct sfp_flag_name sfp_flag_names[] = {
	{SFF8472_FLAG_TEMP_HIGH, "temp-high"},
	{SFF8472_FLAG_TEMP_LOW, "temp-low"},
	{SFF8472_FLAG_VCC_HIGH, "vcc-high"},
	{SFF8472_FLAG_VCC_LOW, "vcc-low"},
	{SFF8472_FLAG_TX_BIAS_HIGH, "tx-bias-high"},
	{SFF8472_FLAG_TX_BIAS_LOW, "tx-bias-low"},
	{SFF8472_FLAG_TX_POWER_HIGH, "tx-power-high"},
	{SFF8472_FLAG_TX_POWER_LOW, "tx-power-low"},
	{SFF8472_FLAG_RX_POWER_HIGH, "rx-power-high"},
	{SFF8472_FLAG_RX_POWER_LOW, "rx-power-low"},
};

static bool device_is_sfp(const struct device *dev)
{
	return DEVICE_API_IS(sfp, dev);
}

static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_filter(idx, device_is_sfp);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_sfp_device, device_name_get);

static const struct device *sfp_shell_device(const struct shell *sh, size_t argc, char **argv)
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

static void sfp_shell_print_flags(const struct shell *sh, const char *label, uint16_t flags)
{
	shell_fprintf(sh, SHELL_NORMAL, "%s: 0x%03x", label, flags);

	for (size_t i = 0U; i < ARRAY_SIZE(sfp_flag_names); i++) {
		if ((flags & sfp_flag_names[i].bit) != 0U) {
			shell_fprintf(sh, SHELL_NORMAL, " %s", sfp_flag_names[i].name);
		}
	}

	shell_fprintf(sh, SHELL_NORMAL, "\n");
}

static int cmd_sfp_info(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = sfp_shell_device(sh, argc, argv);
	struct sff8472_id id;
	int ret;

	if (dev == NULL) {
		return -ENODEV;
	}

	ret = sfp_read_id(dev, &id);
	if (ret != 0) {
		shell_error(sh, "failed to read module ID (%d)", ret);
		return ret;
	}

	shell_print(sh, "Identifier : %s (0x%02x)", sff8472_identifier_name(id.identifier),
		    id.identifier);
	shell_print(sh, "Connector  : %s (0x%02x)", sff8472_connector_name(id.connector),
		    id.connector);
	shell_print(sh, "Vendor     : %s", id.vendor_name);
	shell_print(sh, "Part number: %s", id.vendor_pn);
	shell_print(sh, "Revision   : %s", id.vendor_rev);
	shell_print(sh, "Serial     : %s", id.vendor_sn);
	shell_print(sh, "Date code  : %s", id.date_code);
	shell_print(sh, "Wavelength : %u nm", id.wavelength_nm);
	shell_print(sh, "Bit rate   : %u MBd", id.br_nominal_mbd);
	shell_print(sh, "Diagnostics: %s", id.ddm_supported ? "supported" : "not supported");

	return 0;
}

static int cmd_sfp_ddm(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = sfp_shell_device(sh, argc, argv);
	struct sff8472_diagnostics diag;
	int ret;

	if (dev == NULL) {
		return -ENODEV;
	}

	ret = sfp_read_diagnostics(dev, &diag);
	if (ret != 0) {
		shell_error(sh, "failed to read diagnostics (%d)", ret);
		return ret;
	}

	shell_print(sh, "Temperature: %d.%03d C", diag.temp_mdegc / 1000,
		    abs(diag.temp_mdegc % 1000));
	shell_print(sh, "Vcc        : %u.%03u V", diag.vcc_uv / 1000000U,
		    (diag.vcc_uv / 1000U) % 1000U);
	shell_print(sh, "TX bias    : %u uA", diag.tx_bias_ua);
	shell_print(sh, "TX power   : %u nW", diag.tx_power_nw);
	shell_print(sh, "RX power   : %u nW", diag.rx_power_nw);
	sfp_shell_print_flags(sh, "Alarms   ", diag.alarm_flags);
	sfp_shell_print_flags(sh, "Warnings ", diag.warning_flags);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sfp_cmds,
	SHELL_CMD_ARG(info, &dsub_sfp_device,
		      "Show module identification\nUsage: sfp info <device>", cmd_sfp_info, 2, 0),
	SHELL_CMD_ARG(ddm, &dsub_sfp_device,
		      "Show digital diagnostic monitoring\nUsage: sfp ddm <device>", cmd_sfp_ddm,
		      2, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(sfp, &sfp_cmds, "SFP/SFP+ pluggable module commands", NULL);
