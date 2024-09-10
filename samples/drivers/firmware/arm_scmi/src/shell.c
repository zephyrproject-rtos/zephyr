/*
 * System Control and Management Interface (SCMI) Shell interface
 *
 * Copyright (c) 2024 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/firmware/scmi/protocol.h>
#include <zephyr/drivers/firmware/scmi/base.h>
#include <zephyr/shell/shell.h>

static int scmi_shell_base_revision(const struct shell *sh, size_t argc, char **argv)
{
	struct scmi_revision_info rev;
	int ret;

	ret = scmi_base_get_revision_info(&rev);
	if (ret) {
		shell_error(sh, "base proto get revision failed (%d)", ret);
		return ret;
	}

	shell_print(sh, "ARM SCMI base protocol v%04x.%04x", rev.major_ver, rev.minor_ver);

#if defined(CONFIG_ARM_SCMI_BASE_EXT_REV)
	shell_print(sh, "  vendor\t:%s", rev.vendor_id);
	shell_print(sh, "  subvendor\t:%s", rev.sub_vendor_id);
	shell_print(sh, "  fw version\t:0x%x", rev.impl_ver);
	shell_print(sh, "  protocols\t:%d", rev.num_protocols);
	shell_print(sh, "  num_agents\t:%d", rev.num_agents);
#endif /* CONFIG_ARM_SCMI_BASE_EXT_REV */

	return 0;
}

#if defined(CONFIG_ARM_SCMI_BASE_AGENT_HELPERS)
static int scmi_shell_base_discover_agent(const struct shell *sh, size_t argc, char **argv)
{
	struct scmi_revision_info rev;
	struct scmi_agent_info agent_info;
	uint32_t agent_id = SCMI_BASE_AGENT_ID_OWN;
	int ret;

	if (argc > 2) {
		return -EINVAL;
	}

	if (argc == 2) {
		agent_id = atoi(argv[1]);
	}

	ret = scmi_base_get_revision_info(&rev);
	if (ret) {
		shell_error(sh, "base proto get revision failed (%d)", ret);
		return ret;
	}

	if (agent_id != SCMI_BASE_AGENT_ID_OWN && agent_id >= rev.num_agents) {
		shell_error(sh, "base proto invalid agent id (%u)", agent_id);
		return -EINVAL;
	}

	ret = scmi_base_discover_agent(agent_id, &agent_info);
	if (ret) {
		shell_error(sh, "base proto discover agent failed (%d)", ret);
		return ret;
	}

	shell_print(sh, "  agent id\t:%u", agent_info.agent_id);
	shell_print(sh, "  agent name\t:%s", agent_info.name);
	return 0;
}

static int scmi_shell_base_device_permission(const struct shell *sh, size_t argc, char **argv)
{
	uint32_t agent_id, device_id;
	bool allow;
	int ret;

	if (argc != 4) {
		return -EINVAL;
	}

	agent_id = atoi(argv[1]);
	device_id = atoi(argv[2]);
	allow = !!atoi(argv[3]);

	ret = scmi_base_device_permission(agent_id, device_id, allow);
	if (ret) {
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

	if (argc != 3) {
		return -EINVAL;
	}

	agent_id = atoi(argv[1]);
	reset_perm = !!atoi(argv[2]);

	ret = scmi_base_reset_agent_cfg(agent_id, reset_perm);
	if (ret) {
		shell_error(sh, "base proto reset agent cfg failed (%d)", ret);
		return ret;
	}

	shell_print(sh, "agent:%u reset cfg reset_perm:%s", agent_id,
		    reset_perm ? "reset" : "none");

	return 0;
}
#endif /* CONFIG_ARM_SCMI_BASE_AGENT_HELPERS */

SHELL_STATIC_SUBCMD_SET_CREATE(sub_scmi_base_cmds,
	SHELL_CMD(revision, NULL,
		  "SCMI Base get revision info\n"
		  "Usage: arm_scmi base revision\n",
		  scmi_shell_base_revision),
#if defined(CONFIG_ARM_SCMI_BASE_AGENT_HELPERS)
	SHELL_CMD_ARG(discover_agent, NULL,
		  "SCMI Base discover an agent\n"
		  "Usage: arm_scmi base discover_agent [agent_id]\n",
		  scmi_shell_base_discover_agent, 1, 2),
	SHELL_CMD_ARG(device_permission, NULL,
		  "SCMI Base set an agent permissions to access device\n"
		  "Usage: arm_scmi base discover_agent <agent_id> <device_id> <allow := 0|1>\n",
		  scmi_shell_base_device_permission, 4, 0),
	SHELL_CMD_ARG(reset_agent_cfg, NULL,
		  "SCMI Base reset an agent configuration\n"
		  "Usage: arm_scmi base reset_agent_cfg <agent_id> <reset_perm := 0|1>\n",
		  scmi_shell_base_reset_agent_cfg, 3, 0),
#endif /* CONFIG_ARM_SCMI_BASE_AGENT_HELPERS */
	SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((arm_scmi), base, &sub_scmi_base_cmds, "SCMI Base proto commands.",
		 NULL, 1, 0);

SHELL_SUBCMD_SET_CREATE(subcmd_arm_scmi, (arm_scmi));

SHELL_CMD_REGISTER(arm_scmi, &subcmd_arm_scmi, "ARM SCMI commands", NULL);
