/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/drivers/firmware/scmi/perf.h>
#include <zephyr/drivers/firmware/scmi/protocol.h>
#include <zephyr/shell/shell.h>

extern struct scmi_protocol SCMI_PROTOCOL_NAME(SCMI_PROTOCOL_PERF);
static struct scmi_protocol *_proto = &SCMI_PROTOCOL_NAME(SCMI_PROTOCOL_PERF);

static int cmd_perf_version(const struct shell *sh, size_t argc, char **argv)
{
	uint32_t version;
	int ret;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	ret = scmi_protocol_get_version(_proto, &version);
	if (ret) {
		shell_error(sh, "failed to query protocol version: %d", ret);
		return ret;
	}

	shell_print(sh, "Performance protocol version: 0x%x", version);
	return 0;
}

static int cmd_perf_summary(const struct shell *sh, size_t argc, char **argv)
{
	struct scmi_perf_domain_attrs attrs;
	uint32_t num_domains = 0;
	uint32_t num_opps;
	int ret;

	uint32_t current_khz = 0;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	ret = scmi_perf_num_domains_get(&num_domains);
	if (ret) {
		shell_error(sh, "PROTOCOL_ATTRIBUTES failed: %d", ret);
		return ret;
	}

	shell_print(sh, "Performance domains: %u", num_domains);
	shell_print(sh, "%-4s %-16s %-12s %-12s %-10s",
		    "ID", "Name", "Sustained", "Current", "OPPs");
	shell_print(sh, "%-4s %-16s %-12s %-12s %-10s",
		    "----", "----------------", "------------", "------------", "----------");

	for (uint32_t id = 0; id < num_domains; id++) {
		ret = scmi_perf_domain_attributes_get(id, &attrs);
		if (ret) {
			shell_warn(sh, "domain %u: DOMAIN_ATTRIBUTES failed: %d",
				   id, ret);
			continue;
		}

		num_opps = 0;
		scmi_perf_describe_levels(id, NULL, 0, &num_opps);
		scmi_perf_level_get(id, &current_khz);

		shell_print(sh, "%-4u %-16s %-8u kHz  %-8u kHz  %-10u",
				id, attrs.name, attrs.sustained_freq_khz, current_khz, num_opps);
	}

	return 0;
}

static int cmd_perf_info(const struct shell *sh, size_t argc, char **argv)
{
	struct scmi_perf_domain_attrs attrs;
	struct scmi_perf_opp opps[SCMI_PERF_MAX_OPPS];
	struct scmi_perf_limits limits;
	uint32_t domain_id, current_level, num_opps;
	int ret;

	domain_id = (uint32_t)strtoul(argv[1], NULL, 0);

	ret = scmi_perf_domain_attributes_get(domain_id, &attrs);
	if (ret) {
		shell_error(sh, "DOMAIN_ATTRIBUTES failed: %d", ret);
		return ret;
	}

	ret = scmi_perf_level_get(domain_id, &current_level);
	if (ret) {
		shell_error(sh, "LEVEL_GET failed: %d", ret);
		return ret;
	}

	ret = scmi_perf_describe_levels(domain_id, opps,
					SCMI_PERF_MAX_OPPS, &num_opps);
	if (ret) {
		shell_error(sh, "DESCRIBE_LEVELS failed: %d", ret);
		return ret;
	}

	ret = scmi_perf_limits_get(domain_id, &limits);

	shell_print(sh, "Domain %u: %s", domain_id, attrs.name);
	shell_print(sh, "  Flags:     0x%08x", attrs.flags);
	shell_print(sh, "  Sustained: %u kHz", attrs.sustained_freq_khz);
	shell_print(sh, "  Current:   %u kHz (%u MHz)",
		    current_level, current_level / 1000U);

	if (ret == 0) {
		shell_print(sh, "  Limits:    [%u, %u] kHz",
			    limits.min_level, limits.max_level);
	}

	shell_print(sh, "  OPPs (%u):", num_opps);
	for (uint32_t i = 0; i < num_opps; i++) {
		shell_print(sh, "    [%u] %u kHz (%u MHz)  latency %u us%s",
			    i,
			    opps[i].perf_val_khz,
			    opps[i].perf_val_khz / 1000U,
			    opps[i].transition_latency_us,
			    (opps[i].perf_val_khz == current_level) ?
				"  <-- current" : "");
	}

	return 0;
}

static int cmd_perf_set_level(const struct shell *sh, size_t argc, char **argv)
{
	uint32_t domain_id, perf_level_khz;
	int ret;

	domain_id      = (uint32_t)strtoul(argv[1], NULL, 0);
	perf_level_khz = (uint32_t)strtoul(argv[2], NULL, 0);

	ret = scmi_perf_level_set(domain_id, perf_level_khz);
	if (ret) {
		shell_error(sh,
			    "PERFORMANCE_LEVEL_SET domain=%u level=%u kHz failed: %d",
			    domain_id, perf_level_khz, ret);
		return ret;
	}

	shell_print(sh, "domain %u: set to %u kHz (%u MHz)",
		    domain_id, perf_level_khz, perf_level_khz / 1000U);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(perf_cmds,
	SHELL_CMD(version, NULL,
		  "Get performance protocol version",
		  cmd_perf_version),
	SHELL_CMD(summary, NULL,
		  "List all performance domains",
		  cmd_perf_summary),
	SHELL_CMD_ARG(info, NULL,
		      "Show OPPs and current level\n"
		      "Usage: scmi perf info <domain_id>",
		      cmd_perf_info, 2, 0),
	SHELL_CMD_ARG(set-level, NULL,
		      "Set performance level in kHz\n"
		      "Usage: scmi perf set-level <domain_id> <khz>",
		      cmd_perf_set_level, 3, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((scmi), perf, &perf_cmds,
		 "Performance domain protocol commands", NULL, 0, 0);
