/*
 * System Control and Management Interface (SCMI) Shell interface
 *
 * Copyright (c) 2026 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/drivers/firmware/scmi/base.h>
#include <zephyr/drivers/firmware/scmi/protocol.h>
#include <zephyr/shell/shell.h>

#define SCMI_PROTOCOL_DECLARE(id) extern struct scmi_protocol SCMI_PROTOCOL_NAME(id);

SCMI_PROTOCOL_DECLARE(SCMI_PROTOCOL_BASE);
static struct scmi_protocol *_proto = &SCMI_PROTOCOL_NAME(SCMI_PROTOCOL_BASE);

static int get_id(char *str, uint32_t *id)
{
	char *endptr;
	long parsed_id;

	errno = 0;
	parsed_id = strtol(str, &endptr, 10);

	if (str == endptr || *endptr != '\0' || errno == ERANGE ||
	    parsed_id < 0 || parsed_id > UINT32_MAX) {
		return -EINVAL;
	}

	*id = (uint32_t)parsed_id;
	return 0;
}

static int get_bool(char *str, bool *value)
{
	char *endptr;
	long parsed_value;

	errno = 0;
	parsed_value = strtol(str, &endptr, 10);

	if (str == endptr || *endptr != '\0' || errno == ERANGE ||
	    (parsed_value != 0 && parsed_value != 1)) {
		return -EINVAL;
	}

	*value = !!parsed_value;
	return 0;
}

static int get_agent_id(const struct shell *sh, char *str, uint32_t *agent_id)
{
	uint32_t parsed_id;
	uint8_t agent_num;
	int ret;

	ret = get_id(str, &parsed_id);
	if (ret != 0) {
		shell_error(sh, "invalid agent id (%s)", str);
		return ret;
	}

	ret = scmi_base_get_num_agents(&agent_num);
	if (ret != 0) {
		shell_error(sh, "base proto get attributes failed (%d)", ret);
		return ret;
	}

	if (parsed_id >= agent_num) {
		shell_error(sh, "base proto invalid agent id (%u)", parsed_id);
		return -EINVAL;
	}

	*agent_id = parsed_id;
	return 0;
}

static int scmi_shell_base_version(const struct shell *sh, size_t argc, char **argv)
{
	struct scmi_protocol_version version = {0};

	version.raw = _proto->version;

	shell_print(sh, "Base protocol version: %04x.%04x", version.major, version.minor);

	return 0;
}

static int scmi_shell_base_platform_info(const struct shell *sh, size_t argc, char **argv)
{
	struct scmi_platform_info rev = {0};
	int ret;

	ret = scmi_base_get_platform_info(&rev);
	if (ret != 0) {
		shell_error(sh, "base proto get revision failed (%d)", ret);
		return ret;
	}

	shell_print(sh, "  vendor\t: %s", rev.vendor_id);
	shell_print(sh, "  subvendor\t: %s", rev.sub_vendor_id);
	shell_print(sh, "  fw version\t: 0x%x", rev.impl_ver);
	shell_print(sh, "  protocols\t: %d", rev.num_protocols);
	shell_print(sh, "  num_agents\t: %d", rev.num_agents);

	return 0;
}

static int scmi_shell_base_discover_agent(const struct shell *sh, size_t argc, char **argv)
{
	struct scmi_agent_info agent_info;
	uint32_t agent_id = SCMI_BASE_AGENT_ID_OWN;
	int ret;

	if (argc == 2) {
		ret = get_agent_id(sh, argv[1], &agent_id);
		if (ret != 0) {
			return ret;
		}
	}

	ret = scmi_base_discover_agent(agent_id, &agent_info);
	if (ret != 0) {
		shell_error(sh, "base proto discover agent failed (%d)", ret);
		return ret;
	}

	shell_print(sh, "  agent id\t: %u", agent_info.agent_id);
	shell_print(sh, "  agent name\t: %s", agent_info.name);
	return 0;
}

static int scmi_shell_base_device_permission(const struct shell *sh, size_t argc, char **argv)
{
	uint32_t agent_id, device_id;
	bool allow;
	int ret;

	ret = get_agent_id(sh, argv[1], &agent_id);
	if (ret != 0) {
		return ret;
	}

	ret = get_id(argv[2], &device_id);
	if (ret != 0) {
		shell_error(sh, "invalid device id (%s)", argv[2]);
		return ret;
	}

	ret = get_bool(argv[3], &allow);
	if (ret != 0) {
		shell_error(sh, "allow should be either 0 or 1 (%s)", argv[3]);
		return ret;
	}

	ret = scmi_base_device_permission(agent_id, device_id, allow);
	if (ret != 0) {
		shell_error(sh, "base proto device permission failed (%d)", ret);
		return ret;
	}

	shell_print(sh, "agent:%u device:%u permission set to %s", agent_id, device_id,
		    allow ? "allow" : "deny");

	return 0;
}

static int scmi_shell_base_reset_agent_cfg(const struct shell *sh, size_t argc, char **argv)
{
	uint32_t agent_id;
	bool reset_perm;
	int ret;

	ret = get_agent_id(sh, argv[1], &agent_id);
	if (ret != 0) {
		return ret;
	}

	ret = get_bool(argv[2], &reset_perm);
	if (ret != 0) {
		shell_error(sh, "reset_perm should be either 0 or 1 (%s)", argv[2]);
		return ret;
	}

	ret = scmi_base_reset_agent_cfg(agent_id, reset_perm);
	if (ret != 0) {
		shell_error(sh, "base proto reset agent cfg failed (%d)", ret);
		return ret;
	}

	shell_print(sh, "agent:%u reset cfg reset_perm:%s", agent_id,
		    reset_perm ? "reset" : "none");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_scmi_base_cmds,
	SHELL_CMD(version, NULL,
		  SHELL_HELP("get protocol version", ""),
		  scmi_shell_base_version),
	SHELL_CMD(get-platform-info, NULL,
		  SHELL_HELP("get platform info", ""),
		  scmi_shell_base_platform_info),
	SHELL_CMD_ARG(discover-agent, NULL,
		      SHELL_HELP("discover an agent", "[agent_id]"),
		      scmi_shell_base_discover_agent, 1, 1),
	SHELL_CMD_ARG(device-permission, NULL,
		      SHELL_HELP("set an agent permissions to access device",
				 "<agent_id> <device_id> <allow := 0|1>"),
		      scmi_shell_base_device_permission, 4, 0),
	SHELL_CMD_ARG(reset-agent-cfg, NULL,
		      SHELL_HELP("reset an agent configuration", "<agent_id> <reset_perm := 0|1>"),
		      scmi_shell_base_reset_agent_cfg, 3, 0),
	SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((scmi), base, &sub_scmi_base_cmds, "Base protocol commands", NULL, 0, 0);
