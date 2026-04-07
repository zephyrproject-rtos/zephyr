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

#define RESET_BORDER "+------------------------------------------------------------------------+"

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

static int scmi_reset_get_num_domains(struct scmi_protocol *proto, uint16_t *num_domains)
{
	int ret;
	uint32_t attributes;

	ret = scmi_protocol_attributes_get(proto, &attributes);
	if (ret != 0) {
		return ret;
	}

	*num_domains = SCMI_RESET_ATTR_GET_NUM_DOMAINS(attributes);

	return 0;
}

static int scmi_shell_reset_version(const struct shell *sh, size_t argc, char **argv)
{
	struct scmi_protocol_version ver;
	struct scmi_protocol *proto;
	int ret;

	proto = reset_dev->data;

	ret = scmi_protocol_get_version(proto, &ver.raw);
	if (ret != 0) {
		shell_error(sh, "reset get version failed (%d)", ret);
		return ret;
	}

	shell_print(sh, "Reset protocol version 0x%04x.%04x", ver.major, ver.minor);

	return 0;
}

static int scmi_shell_num_domains(const struct shell *sh, size_t argc, char **argv)
{
	struct scmi_protocol *proto;
	uint16_t num_domains;
	int ret;

	proto = reset_dev->data;

	ret = scmi_reset_get_num_domains(proto, &num_domains);
	if (ret != 0) {
		shell_error(sh, "reset get attributes failed (%d)", ret);
		return ret;
	}

	shell_print(sh, "Reset protocol total number of domains: %u", num_domains);

	return 0;
}

static int scmi_shell_reset_summary(const struct shell *sh, size_t argc, char **argv)
{
	struct scmi_reset_domain_attr dom_attr;
	struct scmi_protocol *proto;
	uint16_t num_domains;
	uint16_t i;
	int ret;

	proto = reset_dev->data;

	ret = scmi_reset_get_num_domains(proto, &num_domains);
	if (ret != 0) {
		shell_error(sh, "reset get attributes failed (%d)", ret);
		return ret;
	}

	shell_print(sh, RESET_BORDER);
	shell_print(sh,
		    "|  ID |       Name       |  Latency   |            Attribures            |");
	shell_print(sh, RESET_BORDER);

	for (i = 0; i < num_domains; i++) {
		ret = scmi_reset_domain_get_attr(proto, i, &dom_attr);
		if (ret != 0) {
			shell_error(sh, "reset domain:%u get attributes failed (%d)", ret, i);
			return ret;
		}

		shell_print(sh, "| %3u | %16s | 0x%08x | async=%s, notify=%s, latency=%s |",
			    i, dom_attr.name,
			    dom_attr.latency, dom_attr.is_async_sup ? "y" : "n",
			    dom_attr.is_notifications_sup ? "y" : "n",
			    dom_attr.is_latency_valid ? "valid" : "inval");
	}

	shell_print(sh, RESET_BORDER);

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

	ret = scmi_reset_get_num_domains(proto, &num_domains);
	if (ret != 0) {
		shell_error(sh, "reset get attributes failed (%d)", ret);
		return ret;
	}

	ret = get_id(argv[1], &domain_id);
	if (ret != 0) {
		shell_error(sh, "invalid reset domain index input %s\n", argv[1]);
		return ret;
	}
	if (domain_id >= num_domains) {
		shell_error(sh, "invalid reset domain index %s\n", argv[1]);
		return -ENOENT;
	}

	ret = scmi_reset_domain_get_attr(proto, domain_id, &dom_attr);
	if (ret != 0) {
		shell_error(sh, "reset domain get attributes failed (%d)", ret);
		return ret;
	}

	shell_print(sh, "ARM SCMI reset domain: %u", domain_id);
	shell_print(sh, " name\t\t: %s", dom_attr.name);
	if (dom_attr.is_latency_valid) {
		shell_print(sh, " latency\t: 0x%08x", dom_attr.latency);
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

	ret = get_id(argv[1], &domain_id);
	if (ret != 0) {
		shell_error(sh, "invalid reset domain index input %s\n", argv[1]);
		return ret;
	}

	ret = reset_line_assert(reset_dev, domain_id);
	if (ret != 0) {
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

	ret = get_id(argv[1], &domain_id);
	if (ret != 0) {
		shell_error(sh, "invalid reset domain index input %s\n", argv[1]);
		return ret;
	}

	ret = reset_line_deassert(reset_dev, domain_id);
	if (ret != 0) {
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

	ret = get_id(argv[1], &domain_id);
	if (ret != 0) {
		shell_error(sh, "invalid reset domain index input %s\n", argv[1]);
		return ret;
	}

	ret = reset_line_toggle(reset_dev, domain_id);
	if (ret != 0) {
		shell_error(sh, "reset domain:%u toggle failed (%d)", domain_id, ret);
	} else {
		shell_info(sh, "reset domain:%u toggle done", domain_id);
	}

	return ret;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_scmi_reset_cmds,
	SHELL_CMD(version, NULL,
		  SHELL_HELP("get protocol version", ""),
		  scmi_shell_reset_version),
	SHELL_CMD(num-domains, NULL,
		  SHELL_HELP("get total number of reset domains", ""),
		  scmi_shell_num_domains),
	SHELL_CMD(summary, NULL,
		  SHELL_HELP("get summary for all reset domains", ""),
		  scmi_shell_reset_summary),
	SHELL_CMD_ARG(info, NULL,
		      SHELL_HELP("show domain info", "<domain_id>"),
		      scmi_shell_reset_info, 2, 0),
	SHELL_CMD_ARG(assert, NULL,
		      SHELL_HELP("domain assert", "<domain_id>"),
		      scmi_shell_reset_assert, 2, 0),
	SHELL_CMD_ARG(deassert, NULL,
		      SHELL_HELP("domain de-assert", "<domain_id>"),
		      scmi_shell_reset_deassert, 2, 0),
	SHELL_CMD_ARG(autoreset, NULL,
		      SHELL_HELP("domain Autonomous reset", "<domain_id>"),
		      scmi_shell_reset_toggle, 2, 0),
	SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((scmi), reset, &sub_scmi_reset_cmds, "Reset protocol commands", NULL, 0, 0);
