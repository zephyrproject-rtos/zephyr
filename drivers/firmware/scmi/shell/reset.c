/*
 * SCMI Reset domain management protocol Shell interface
 *
 * Copyright (c) 2026 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/firmware/scmi/reset.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/shell/shell.h>

#if defined(CONFIG_DT_HAS_ARM_SCMI_RESET_ENABLED)
static const struct device *reset_dev = DEVICE_DT_GET_ANY(arm_scmi_reset);
#else
BUILD_ASSERT(1, "unsupported scmi reset interface");
#endif

static int scmi_shell_reset_revision(const struct shell *sh, size_t argc, char **argv)
{
	struct scmi_protocol_version ver;
	struct scmi_protocol *proto;
	uint16_t num_domains;
	int ret;

	proto = reset_dev->data;

	ret = scmi_protocol_get_version(proto, &ver.raw);
	if (ret) {
		shell_error(sh, "reset get version failed (%d)", ret);
		return ret;
	}

	ret = scmi_reset_get_attr(proto, &num_domains);
	if (ret) {
		shell_error(sh, "reset get attributes failed (%d)", ret);
		return ret;
	}

	shell_print(sh, "ARM SCMI Reset protocol version 0x%04x.%04x num_domains:%u", ver.major,
		    ver.minor, num_domains);

	return 0;
}

static int scmi_shell_reset_dom_list(const struct shell *sh, size_t argc, char **argv)
{
	struct scmi_reset_domain_attr dom_attr;
	struct scmi_protocol *proto;
	uint16_t num_domains;
	uint16_t i;
	int ret;

	proto = reset_dev->data;

	ret = scmi_reset_get_attr(proto, &num_domains);
	if (ret) {
		shell_error(sh, "reset get attributes failed (%d)", ret);
		return ret;
	}

	shell_print(sh, "domain_id\tname\tlatency\t\tattributes");

	for (i = 0; i < num_domains; i++) {
		ret = scmi_reset_domain_get_attr(proto, i, &dom_attr);
		if (ret) {
			shell_error(sh, "reset domain:%u get attributes failed (%d)", ret, i);
			return ret;
		}

		shell_print(sh, "%u\t%s\t0x%08x\tasync=%s,notify=%s,latency=%s", i, dom_attr.name,
			    dom_attr.latency, dom_attr.is_async_sup ? "yes" : "no",
			    dom_attr.is_notifications_sup ? "yes" : "no",
			    dom_attr.is_latency_valid ? "valid" : "invalid");
	}

	return 0;
}

static int scmi_shell_reset_info(const struct shell *sh, size_t argc, char **argv)
{
	struct scmi_reset_domain_attr dom_attr;
	struct scmi_protocol *proto;
	uint16_t num_domains;
	uint32_t domain_id;
	int ret;

	proto = reset_dev->data;

	ret = scmi_reset_get_attr(proto, &num_domains);
	if (ret) {
		shell_error(sh, "reset get attributes failed (%d)", ret);
		return ret;
	}

	domain_id = atoi(argv[1]);
	if (domain_id >= num_domains) {
		shell_error(sh, "invalid reset domain index %s\n", argv[1]);
		return -ENOENT;
	}

	ret = scmi_reset_domain_get_attr(proto, domain_id, &dom_attr);
	if (ret) {
		shell_error(sh, "reset domain get attributes failed (%d)", ret);
		return ret;
	}

	shell_print(sh, "ARM SCMI reset domain: %u", domain_id);
	shell_print(sh, " name\t\t: %s", dom_attr.name);
	if (dom_attr.is_latency_valid) {
		shell_print(sh, " latency\t: %u", dom_attr.latency);
	} else {
		shell_print(sh, " latency\t: invalid");
	}
	shell_print(sh, " async\t\t: %s", dom_attr.is_async_sup ? "supported" : "not supported");
	shell_print(sh, " notifications\t: %s",
		    dom_attr.is_notifications_sup ? "supported" : "not supported");

	return 0;
}

static int scmi_shell_reset_assert(const struct shell *sh, size_t argc, char **argv)
{
	uint32_t domain_id;
	int ret;

	domain_id = atoi(argv[1]);

	ret = reset_line_assert(reset_dev, domain_id);
	if (ret) {
		shell_error(sh, "reset domain:%u assert failed (%d)", domain_id, ret);
	} else {
		shell_info(sh, "reset domain:%u assert done", domain_id);
	}

	return ret;
}

static int scmi_shell_reset_deassert(const struct shell *sh, size_t argc, char **argv)
{
	uint32_t domain_id;
	int ret;

	domain_id = atoi(argv[1]);

	ret = reset_line_deassert(reset_dev, domain_id);
	if (ret) {
		shell_error(sh, "reset domain:%u deassert failed (%d)", domain_id, ret);
	} else {
		shell_info(sh, "reset domain:%u deassert done", domain_id);
	}

	return ret;
}

static int scmi_shell_reset_toggle(const struct shell *sh, size_t argc, char **argv)
{
	uint32_t domain_id;
	int ret;

	domain_id = atoi(argv[1]);

	ret = reset_line_toggle(reset_dev, domain_id);
	if (ret) {
		shell_error(sh, "reset domain:%u toggle failed (%d)", domain_id, ret);
	} else {
		shell_info(sh, "reset domain:%u toggle done", domain_id);
	}

	return ret;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_scmi_reset_cmds,
	SHELL_CMD_ARG(revision, NULL,
		      SHELL_HELP("SCMI Reset proto show revision information", ""),
		      scmi_shell_reset_revision, 1, 0),
	SHELL_CMD_ARG(list, NULL,
		      SHELL_HELP("SCMI Reset domains list", ""),
		      scmi_shell_reset_dom_list, 1, 0),
	SHELL_CMD_ARG(info, NULL,
		      SHELL_HELP("SCMI Reset domain show info", "<domain_id>"),
		      scmi_shell_reset_info, 2, 0),
	SHELL_CMD_ARG(assert, NULL,
		      SHELL_HELP("SCMI Reset domain assert", "<domain_id>"),
		      scmi_shell_reset_assert, 2, 0),
	SHELL_CMD_ARG(deassert, NULL,
		      SHELL_HELP("SCMI Reset domain de-assert", "<domain_id>"),
		      scmi_shell_reset_deassert, 2, 0),
	SHELL_CMD_ARG(autoreset, NULL,
		      SHELL_HELP("SCMI Reset domain Autonomous reset", "<domain_id>"),
		      scmi_shell_reset_toggle, 2, 0),
	SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((scmi), reset, &sub_scmi_reset_cmds,
		 "SCMI Reset proto commands.",
		 NULL, 1, 1);
