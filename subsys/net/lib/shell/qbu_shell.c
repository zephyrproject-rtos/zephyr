/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#include <inttypes.h>
#include <string.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/util.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/ethernet_mgmt.h>

#include "net_shell_private.h"

#if defined(CONFIG_NET_QBU)

/* Index in argv of the first TC value: "set_preempt <iface> <tc0> ..." */
#define QBU_PREEMPT_TC_ARGV_IDX 2

static int qbu_set(struct net_if *iface, struct ethernet_req_params *params,
		   enum ethernet_qbu_param_type type)
{
	params->qbu_param.type = type;

	return net_mgmt(NET_REQUEST_ETHERNET_SET_QBU_PARAM, iface, params, sizeof(*params));
}

static int qbu_get(struct net_if *iface, struct ethernet_req_params *params,
		   enum ethernet_qbu_param_type type)
{
	memset(params, 0, sizeof(*params));
	params->qbu_param.type = type;

	return net_mgmt(NET_REQUEST_ETHERNET_GET_QBU_PARAM, iface, params, sizeof(*params));
}

/* net qbu */
static int cmd_net_qbu(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "Available Qbu commands:");
	shell_print(sh, "  net qbu enable <iface_index> <on|off>");
	shell_print(sh, "  net qbu set_preempt <iface_index> <tc0> ... <tc%d>",
		    NET_TC_TX_COUNT - 1);
	shell_print(sh, "      tc: 0=preemptable, 1=express, unlisted TCs become express");
	shell_print(sh, "  net qbu set_frag_size <iface_index> <0-3>");
	shell_print(sh, "      0=64B, 1=128B, 2=192B, 3=256B minimum non-final fragment");
	shell_print(sh, "  net qbu get_info <iface_index>");

	return 0;
}

/* net qbu enable <iface_index> <on|off> */
static int cmd_qbu_enable(const struct shell *sh, size_t argc, char **argv)
{
	struct ethernet_req_params params = {0};
	struct net_if *iface;
	bool enable;
	int ret = 0;

	iface = get_iface_from_shell(sh, argc, argv);
	if (iface == NULL) {
		return -ENOEXEC;
	}

	enable = shell_strtobool(argv[2], 0, &ret);
	if (ret < 0) {
		shell_error(sh, "invalid value \"%s\", expected on|off", argv[2]);
		return ret;
	}

	params.qbu_param.enabled = enable;

	ret = qbu_set(iface, &params, ETHERNET_QBU_PARAM_TYPE_STATUS);
	if (ret < 0) {
		shell_error(sh, "failed to set qbu status on %s, ret=%d", argv[1], ret);
		return ret;
	}

	shell_print(sh, "qbu preemption %s on interface %s",
		    enable ? "enabled" : "disabled", argv[1]);

	return 0;
}

/* net qbu set_preempt <iface_index> <tc0> ... <tcN-1> */
static int cmd_qbu_set_preempt(const struct shell *sh, size_t argc, char **argv)
{
	struct ethernet_req_params params = {0};
	struct net_if *iface;
	size_t tc_args;
	int ret = 0;
	int i;

	iface = get_iface_from_shell(sh, argc, argv);
	if (iface == NULL) {
		return -ENOEXEC;
	}

	tc_args = MIN(argc - QBU_PREEMPT_TC_ARGV_IDX, (size_t)NET_TC_TX_COUNT);

	for (i = 0; i < NET_TC_TX_COUNT; i++) {
		unsigned long val = 1UL;

		if ((size_t)i < tc_args) {
			val = shell_strtoul(argv[QBU_PREEMPT_TC_ARGV_IDX + i], 10, &ret);
			if (ret < 0 || val > 1UL) {
				shell_error(sh, "invalid status for tc%d \"%s\", expected 0 or 1",
					    i, argv[QBU_PREEMPT_TC_ARGV_IDX + i]);
				return -EINVAL;
			}
		}

		params.qbu_param.frame_preempt_statuses[i] =
			(val != 0UL) ? ETHERNET_QBU_STATUS_EXPRESS
				     : ETHERNET_QBU_STATUS_PREEMPTABLE;
	}

	ret = qbu_set(iface, &params, ETHERNET_QBU_PARAM_TYPE_PREEMPTION_STATUS_TABLE);
	if (ret < 0) {
		shell_error(sh, "failed to set qbu preemption table on %s, ret=%d", argv[1], ret);
		return ret;
	}

	shell_print(sh, "qbu preemption status table updated on interface %s", argv[1]);

	return 0;
}

/* net qbu set_frag_size <iface_index> <0-3> */
static int cmd_qbu_set_frag_size(const struct shell *sh, size_t argc, char **argv)
{
	struct ethernet_req_params params = {0};
	unsigned long frag_size;
	struct net_if *iface;
	int ret = 0;

	iface = get_iface_from_shell(sh, argc, argv);
	if (iface == NULL) {
		return -ENOEXEC;
	}

	frag_size = shell_strtoul(argv[2], 10, &ret);
	if (ret < 0 || frag_size > 3UL) {
		shell_error(sh, "additional_fragment_size must be in [0, 3]");
		return -EINVAL;
	}

	params.qbu_param.additional_fragment_size = (uint8_t)frag_size;

	ret = qbu_set(iface, &params, ETHERNET_QBR_PARAM_TYPE_ADDITIONAL_FRAGMENT_SIZE);
	if (ret < 0) {
		shell_error(sh, "failed to set qbu fragment size on %s, ret=%d", argv[1], ret);
		return ret;
	}

	shell_print(sh, "qbu additional_fragment_size set to %lu (min fragment = %lu bytes)",
		    frag_size, (frag_size + 1UL) * 64UL);

	return 0;
}

/* net qbu get_info <iface_index> */
static int cmd_qbu_get_info(const struct shell *sh, size_t argc, char **argv)
{
	struct ethernet_req_params params;
	struct net_if *iface;
	int ret;
	int i;

	iface = get_iface_from_shell(sh, argc, argv);
	if (iface == NULL) {
		return -ENOEXEC;
	}

	shell_print(sh, "Qbu/Qbr info for interface %s:", argv[1]);

	/*
	 * Drivers may only implement a subset of the parameters, so a failure
	 * on one of them is reported and the remaining ones are still queried.
	 */
	ret = qbu_get(iface, &params, ETHERNET_QBU_PARAM_TYPE_STATUS);
	if (ret < 0) {
		shell_warn(sh, "preemption enabled: n/a (ret=%d)", ret);
	} else {
		shell_print(sh, "preemption enabled: %s",
			    params.qbu_param.enabled ? "on" : "off");
	}

	ret = qbu_get(iface, &params, ETHERNET_QBU_PARAM_TYPE_HOLD_ADVANCE);
	if (ret < 0) {
		shell_warn(sh, "hold_advance(ns): n/a (ret=%d)", ret);
	} else {
		shell_print(sh, "hold_advance(ns): %" PRIu32, params.qbu_param.hold_advance);
	}

	ret = qbu_get(iface, &params, ETHERNET_QBU_PARAM_TYPE_RELEASE_ADVANCE);
	if (ret < 0) {
		shell_warn(sh, "release_advance(ns): n/a (ret=%d)", ret);
	} else {
		shell_print(sh, "release_advance(ns): %" PRIu32,
			    params.qbu_param.release_advance);
	}

	ret = qbu_get(iface, &params, ETHERNET_QBR_PARAM_TYPE_LINK_PARTNER_STATUS);
	if (ret < 0) {
		shell_warn(sh, "link_partner_status: n/a (ret=%d)", ret);
	} else {
		shell_print(sh, "link_partner_status: %s",
			    params.qbu_param.link_partner_status ? "on" : "off");
	}

	ret = qbu_get(iface, &params, ETHERNET_QBR_PARAM_TYPE_ADDITIONAL_FRAGMENT_SIZE);
	if (ret < 0) {
		shell_warn(sh, "additional_fragment_size: n/a (ret=%d)", ret);
	} else {
		shell_print(sh, "additional_fragment_size: %u (min fragment = %u bytes)",
			    params.qbu_param.additional_fragment_size,
			    (params.qbu_param.additional_fragment_size + 1U) * 64U);
	}

	ret = qbu_get(iface, &params, ETHERNET_QBU_PARAM_TYPE_PREEMPTION_STATUS_TABLE);
	if (ret < 0) {
		shell_warn(sh, "TC preemption status: n/a (ret=%d)", ret);
	} else {
		shell_print(sh, "TC preemption status:");
		for (i = 0; i < NET_TC_TX_COUNT; i++) {
			shell_print(sh, "  tc%d: %s", i,
				    (params.qbu_param.frame_preempt_statuses[i] ==
				     ETHERNET_QBU_STATUS_EXPRESS) ? "express" : "preemptable");
		}
	}

	return 0;
}

#else /* CONFIG_NET_QBU */

static int cmd_qbu_not_supported(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "Set %s to enable %s support.", "CONFIG_NET_QBU", "qbu");

	return 0;
}

#define cmd_net_qbu           cmd_qbu_not_supported
#define cmd_qbu_enable        cmd_qbu_not_supported
#define cmd_qbu_set_preempt   cmd_qbu_not_supported
#define cmd_qbu_set_frag_size cmd_qbu_not_supported
#define cmd_qbu_get_info      cmd_qbu_not_supported

#endif /* CONFIG_NET_QBU */

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_qbu,
	SHELL_CMD_ARG(enable, NULL,
		      SHELL_HELP("Enable or disable Qbu frame preemption",
				 "<iface_index> <on|off>"),
		      cmd_qbu_enable, 3, 0),
	SHELL_CMD_ARG(set_preempt, NULL,
		      SHELL_HELP("Set per-TC preemption status table",
				 "<iface_index> <tc0> ... <tcN-1>\n"
				 "    N is NET_TC_TX_COUNT, tc: 0=preemptable, 1=express\n"
				 "    unlisted TCs become express"),
		      cmd_qbu_set_preempt, 3, NET_TC_TX_COUNT - 1),
	SHELL_CMD_ARG(set_frag_size, NULL,
		      SHELL_HELP("Set additional fragment size (Qbr)",
				 "<iface_index> <0-3>\n"
				 "    minimum non-final fragment: "
				 "0=64B, 1=128B, 2=192B, 3=256B"),
		      cmd_qbu_set_frag_size, 3, 0),
	SHELL_CMD_ARG(get_info, NULL,
		      SHELL_HELP("Get Qbu/Qbr information", "<iface_index>"),
		      cmd_qbu_get_info, 2, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((net), qbu, &net_cmd_qbu,
		 "Qbu (frame preemption) commands",
		 cmd_net_qbu, 1, 0);
