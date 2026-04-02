/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/drivers/firmware/scmi/clk.h>
#include <zephyr/drivers/firmware/scmi/protocol.h>
#include <zephyr/shell/shell.h>

#define SCMI_PROTOCOL_DECLARE(id) extern struct scmi_protocol SCMI_PROTOCOL_NAME(id);

struct clk_info {
	bool enabled;
	uint32_t rate;
	uint32_t parent_id;
	char name[SCMI_CLK_NAME_LEN];
	char parent_name[SCMI_CLK_NAME_LEN];
};

SCMI_PROTOCOL_DECLARE(SCMI_PROTOCOL_CLOCK);
static struct scmi_protocol *_proto = &SCMI_PROTOCOL_NAME(SCMI_PROTOCOL_CLOCK);

static int clk_get_info(const struct shell *sh, uint32_t clk_id, struct clk_info *info)
{
	struct scmi_clock_attributes attributes;
	int ret;

	memset(info, 0, sizeof(*info));

	ret = scmi_clock_attributes(_proto, clk_id, &attributes);
	if (ret) {
		return ret;
	}

	memcpy(info->name, attributes.clock_name, SCMI_CLK_NAME_LEN);
	info->enabled = SCMI_CLK_ENABLED(attributes.attributes);

	ret = scmi_clock_rate_get(_proto, clk_id, &info->rate);
	if (ret) {
		shell_error(sh, "failed to query clk %d rate: %d", clk_id, ret);
		return ret;
	}

	/* parent information is optional, thus the error suppression */
	ret = scmi_clock_parent_get(_proto, clk_id, &info->parent_id);
	if (ret) {
		return 0;
	}

	/* no error suppression - parent clock ID should be a valid one */
	ret = scmi_clock_attributes(_proto, info->parent_id, &attributes);
	if (ret) {
		shell_error(sh,
			    "failed to query parent %d attributes: %d",
			    info->parent_id, ret);
		return ret;
	}

	memcpy(info->parent_name, attributes.clock_name, SCMI_CLK_NAME_LEN);

	return 0;
}

static int get_clk_id(char *str, uint32_t *clk_id)
{
	uint32_t attributes;
	char *endptr;
	int ret;

	*clk_id = strtoul(str, &endptr, 0);
	if (*endptr != '\0') {
		return -EINVAL;
	}

	ret = scmi_clock_protocol_attributes(_proto, &attributes);
	if (ret) {
		return ret;
	}

	if (*clk_id >= SCMI_CLK_ATTRIBUTES_CLK_NUM(attributes)) {
		return -ERANGE;
	}

	return 0;
}

static int cmd_clk_version(const struct shell *sh, size_t argc, char **argv)
{
	uint32_t version;
	int ret;

	ret = scmi_protocol_get_version(_proto, &version);
	if (ret) {
		shell_error(sh, "failed to query protocol version: %d", ret);
		return ret;
	}

	shell_print(sh, "Clock protocol version: 0x%x", version);

	return 0;
}

static int cmd_clk_summary(const struct shell *sh, size_t argc, char **argv)
{
	uint32_t clk_num, attributes;
	struct clk_info info;
	int ret, i;

	ret = scmi_clock_protocol_attributes(_proto, &attributes);
	if (ret) {
		shell_error(sh, "Failed to query protocol attributes: %d", ret);
		return ret;
	}

	clk_num = SCMI_CLK_ATTRIBUTES_CLK_NUM(attributes);

	shell_print(sh,
		    "+----------------------------------------------------------------------+");
	shell_print(sh,
		    "| ID |        Name        | Enabled |   Rate(Hz)  |        Parent      |");
	shell_print(sh,
		    "+----------------------------------------------------------------------+");

	for (i = 0; i < clk_num; i++) {
		ret = clk_get_info(sh, i, &info);
		if (ret) {
			continue;
		}

		shell_print(sh, "|%3d |  %16s  |    %c    |%12u |  %16s  |",
			    i, info.name,
			    info.enabled ? 'Y' : 'N',
			    info.rate, info.parent_name[0] ? info.parent_name : "N/A");

		shell_print(sh,
			    "+----------------------------------------------------------------------+");
	}

	return 0;
}

static int cmd_clk_info(const struct shell *sh, size_t argc, char **argv)
{
	struct clk_info info;
	int clk_id, ret;

	ret = get_clk_id(argv[1], &clk_id);
	if (ret) {
		shell_error(sh, "Failed to fetch clock ID: %d", ret);
		return ret;
	}

	ret = clk_get_info(sh, clk_id, &info);
	if (ret) {
		shell_error(sh, "Failed to query clk %d info: %d", clk_id, ret);
		return ret;
	}

	shell_print(sh, "Name: %s", info.name);
	shell_print(sh, "Enabled status: %c", info.enabled ? 'Y' : 'N');
	shell_print(sh, "Rate (Hz): %u", info.rate);

	/* TODO: this needs to be replaced with the list of all possible parents */
	if (info.parent_name[0]) {
		shell_print(sh, "Parent: %s [%d]", info.parent_name, info.parent_id);
	} else {
		shell_print(sh, "Parent: N/A");
	}

	return 0;
}

static int cmd_clk_set_enabled(const struct shell *sh, size_t argc, char **argv)
{
	struct scmi_clock_config cfg = { 0 };
	uint32_t clk_id;
	bool enable;
	int ret;

	if (!strcmp(argv[2], "on")) {
		enable = true;
	} else if (!strcmp(argv[2], "off")) {
		enable = false;
	} else {
		shell_error(sh, "Second parameter should be either \"on\" or \"off\"");
		return -EINVAL;
	}

	ret = get_clk_id(argv[1], &clk_id);
	if (ret) {
		shell_error(sh, "Failed to fetch clock ID: %d", ret);
		return  ret;
	}

	cfg.attributes = SCMI_CLK_CONFIG_ENABLE_DISABLE(enable);
	cfg.clk_id = clk_id;

	/* negative return code may be normal here (e.g. clock is not gateable) */
	ret = scmi_clock_config_set(_proto, &cfg);
	if (ret) {
		shell_error(sh, "Unable to enable/disable clock %d", clk_id);
		return ret;
	}

	return 0;
}

static int cmd_clk_set_rate(const struct shell *sh, size_t argc, char **argv)
{
	struct scmi_clock_rate_config cfg = { 0 };
	uint32_t clk_id, rate;
	char *endptr;
	int ret;

	ret = get_clk_id(argv[1], &clk_id);
	if (ret) {
		shell_error(sh, "Failed to parse clock ID: %d", ret);
		return ret;
	}

	rate = strtoul(argv[2], &endptr, 0);
	if (*endptr != '\0') {
		shell_error(sh, "Failed to parse rate");
		return -EINVAL;
	}

	cfg.clk_id = clk_id;
	cfg.rate[0] = rate;

	/* negative return code may be normal (e.g. no support for rate change) */
	ret = scmi_clock_rate_set(_proto, &cfg);
	if (ret) {
		shell_error(sh, "Unable to change rate for clock %d", clk_id);
		return ret;
	}

	return 0;
}

static int cmd_clk_set_parent(const struct shell *sh, size_t argc, char **argv)
{
	uint32_t clk_id, parent_id;
	int ret;

	ret = get_clk_id(argv[1], &clk_id);
	if (ret) {
		shell_error(sh, "Failed to parse clock ID: %d", ret);
		return ret;
	}

	ret = get_clk_id(argv[2], &parent_id);
	if (ret) {
		shell_error(sh, "Failed to parse parent clock ID: %d", ret);
		return ret;
	}

	ret = scmi_clock_parent_set(_proto, clk_id, parent_id);
	if (ret) {
		shell_error(sh, "Unable to set clock %d parent to %d", clk_id, parent_id);
		return ret;
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(clk_cmds,
	SHELL_CMD(version, NULL,
		  SHELL_HELP("get protocol version", ""),
		  cmd_clk_version),
	SHELL_CMD(summary, NULL,
		  SHELL_HELP("get clock tree summary", ""),
		  cmd_clk_summary),
	SHELL_CMD_ARG(info, NULL,
		      SHELL_HELP("get detailed clock information", "<clock_id>"),
		      cmd_clk_info, 2, 0),
	SHELL_CMD_ARG(set-enabled, NULL,
		      SHELL_HELP("enable/disable a clock", "<clock_id> on|off"),
		      cmd_clk_set_enabled, 3, 0),
	SHELL_CMD_ARG(set-rate, NULL,
		      SHELL_HELP("set a clock's rate (in Hz)", "<clock_id> <rate>"),
		      cmd_clk_set_rate, 3, 0),
	SHELL_CMD_ARG(set-parent, NULL,
		      SHELL_HELP("set a clock's parent", "<clock_id> <parent_id>"),
		      cmd_clk_set_parent, 3, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((scmi), clk, &clk_cmds, "Clock protocol commands", NULL, 0, 0);
