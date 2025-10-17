/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#include <zephyr/shell/shell.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/sys/util.h>
#include <zephyr/devicetree.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/ethernet_mgmt.h>

#include "net_shell_private.h"

#if defined(CONFIG_NET_QBV) && defined(CONFIG_NET_L2_ETHERNET_MGMT)
static struct net_if *get_iface_from_shell(const struct shell *sh, size_t argc, char **argv)
{
	int idx;
	struct net_if *iface;

	idx = get_iface_idx(sh, argv[1]);
	if (idx < 0) {
		return NULL;
	}

	iface = net_if_get_by_index(idx);
	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		PR_WARNING("No such interface in index %d\n", idx);
		return NULL;
	}

	return iface;
}
#endif

/* qbv enable <iface_index> <value(off, on)> */
static int cmd_net_qbv(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if defined(CONFIG_NET_QBV) && defined(CONFIG_NET_L2_ETHERNET_MGMT)
	shell_print(sh, "To set Qbv config:");
	shell_print(sh, "  1. Run enable to on");
	shell_print(sh, "  2. Run set_config to set base_time/cycle_time/cycle_time_ext/list_len");
	shell_print(sh, "  3. Run set_gc to set gate control");
	shell_print(sh, "For example:");
	shell_print(sh, "  1. net qbv enable 1 on");
	shell_print(sh, "  2. net qbv set_config 1 200 0 0 10000000 0 2");
	shell_print(sh, "  3. qbv set_gc 1 0 0x1 5000000");
	shell_print(sh, "  4. qbv set_gc 1 0 0x2 5000000");
#else
	shell_print(sh, "Set %s to enable %s support.\n", "CONFIG_NET_QBV", "qbv");
	shell_print(sh, "Set %s to enable %s support.\n", "CONFIG_NET_L2_ETHERNET_MGMT",
		    "Ethernet network management interface");
#endif

	return 0;
}

/* qbv enable <iface_index> <value(off, on)> */
static int cmd_qbv_enable(const struct shell *sh, size_t argc, char **argv)
{
#if defined(CONFIG_NET_QBV) && defined(CONFIG_NET_L2_ETHERNET_MGMT)
	struct net_if *iface;
	struct ethernet_req_params params;
	int ret;
	bool enable;

	iface = get_iface_from_shell(sh, argc, argv);
	if (!iface) {
		return -ENOEXEC;
	}

	enable = shell_strtobool(argv[2], 0, &ret);
	if (ret < 0) {
		return ret;
	}

	params.qbv_param.type = ETHERNET_QBV_PARAM_TYPE_STATUS;
	params.qbv_param.state = ETHERNET_QBV_STATE_TYPE_ADMIN;
	params.qbv_param.enabled = enable;

	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_QBV_PARAM,
		       iface,
		       &params, sizeof(struct ethernet_req_params));
	if (ret < 0) {
		shell_error(sh, "failed to set %s", argv[1]);
		return ret;
	}
#else
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	shell_print(sh, "Set %s to enable %s support.\n", "CONFIG_NET_QBV", "qbv");
	shell_print(sh, "Set %s to enable %s support.\n", "CONFIG_NET_L2_ETHERNET_MGMT",
		    "Ethernet network management interface");
#endif

	return 0;
}

/*
 * qbv set_config <iface_index> <base_time(s)> <base_time(2*(-16)ns)>
 * <cycle_time(s)> <cycle_time(ns)> <cycle_time_ext> <list_len>
 */
static int cmd_qbv_set_config(const struct shell *sh, size_t argc, char **argv)
{
#if defined(CONFIG_NET_QBV) && defined(CONFIG_NET_L2_ETHERNET_MGMT)
	struct net_if *iface;
	struct ethernet_req_params params;
	uint32_t list_len;
	int ret;

	iface = get_iface_from_shell(sh, argc, argv);
	if (!iface) {
		return -ENOEXEC;
	}

	params.qbv_param.type = ETHERNET_QBV_PARAM_TYPE_TIME;
	params.qbv_param.state = ETHERNET_QBV_STATE_TYPE_ADMIN;

	params.qbv_param.base_time.second = shell_strtoull(argv[2], 10, &ret);
	if (ret < 0) {
		return ret;
	}

	params.qbv_param.base_time.fract_nsecond = shell_strtoull(argv[3], 10, &ret);
	if (ret < 0) {
		return ret;
	}

	params.qbv_param.cycle_time.second = shell_strtoull(argv[4], 10, &ret);
	if (ret < 0) {
		return ret;
	}

	params.qbv_param.cycle_time.nanosecond = shell_strtoul(argv[5], 10, &ret);
	if (ret < 0) {
		return ret;
	}

	params.qbv_param.extension_time = shell_strtoul(argv[6], 10, &ret);
	if (ret < 0) {
		return ret;
	}

	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_QBV_PARAM,
		       iface,
		       &params, sizeof(struct ethernet_req_params));

	list_len = shell_strtol(argv[7], 10, &ret);
	if (ret < 0) {
		shell_print(sh, "failed to set times");
		return ret;
	}

	params.qbv_param.type =
		ETHERNET_QBV_PARAM_TYPE_GATE_CONTROL_LIST_LEN;
	params.qbv_param.gate_control_list_len = list_len;
	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_QBV_PARAM,
		       iface,
		       &params, sizeof(struct ethernet_req_params));
	if (ret < 0) {
		shell_print(sh, "failed to set list length");
		return ret;
	}
#else
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	shell_print(sh, "Set %s to enable %s support.\n", "CONFIG_NET_QBV", "qbv");
	shell_print(sh, "Set %s to enable %s support.\n", "CONFIG_NET_L2_ETHERNET_MGMT",
		    "Ethernet network management interface");
#endif

	return 0;
}

/* qbv set_config <iface_index> <row> <gate_control> <interval> */
static int cmd_qbv_set_gc(const struct shell *sh, size_t argc, char **argv)
{
#if defined(CONFIG_NET_QBV) && defined(CONFIG_NET_L2_ETHERNET_MGMT)
	struct net_if *iface;
	struct ethernet_req_params params;
	uint32_t row;
	uint32_t interval;
	uint32_t gc;
	int ret;

	iface = get_iface_from_shell(sh, argc, argv);
	if (!iface) {
		return -ENOEXEC;
	}

	row = shell_strtoul(argv[2], 10, &ret);
	if (ret < 0) {
		return ret;
	}

	gc = shell_strtoul(argv[3], 16, &ret);
	if (ret < 0) {
		return ret;
	}

	interval = shell_strtoul(argv[4], 10, &ret);
	if (ret < 0) {
		return ret;
	}
	params.qbv_param.type = ETHERNET_QBV_PARAM_TYPE_GATE_CONTROL_LIST;
	params.qbv_param.state = ETHERNET_QBV_STATE_TYPE_ADMIN;

	params.qbv_param.gate_control.time_interval = interval;
	params.qbv_param.gate_control.row = row;
	for (int i = 0; i < CONFIG_NET_TC_TX_COUNT; i++) {
		params.qbv_param.gate_control.gate_status[i] = ((gc & BIT(i)) != 0);
	}

	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_QBV_PARAM,
		       iface,
		       &params, sizeof(struct ethernet_req_params));
	if (ret < 0) {
		return ret;
	}
#else
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	shell_print(sh, "Set %s to enable %s support.\n", "CONFIG_NET_QBV", "qbv");
	shell_print(sh, "Set %s to enable %s support.\n", "CONFIG_NET_L2_ETHERNET_MGMT",
		    "Ethernet network management interface");
#endif

	return 0;
}

/* qbv get_info <iface_index> */
static int cmd_qbv_get_info(const struct shell *sh, size_t argc, char **argv)
{
#if defined(CONFIG_NET_QBV) && defined(CONFIG_NET_L2_ETHERNET_MGMT)
	struct net_if *iface;
	struct ethernet_req_params params;
	int ret;
	uint32_t list_len;
	uint32_t gate_status;

	iface = get_iface_from_shell(sh, argc, argv);
	if (!iface) {
		return -ENOEXEC;
	}

	params.qbv_param.type = ETHERNET_QBV_PARAM_TYPE_STATUS;
	params.qbv_param.state = ETHERNET_QBV_STATE_TYPE_ADMIN;

	ret = net_mgmt(NET_REQUEST_ETHERNET_GET_QBV_PARAM,
		       iface,
		       &params, sizeof(struct ethernet_req_params));
	if (ret < 0) {
		shell_error(sh, "failed to get %s status", argv[1]);
		return ret;
	}
	shell_print(sh, "status: %s", (params.qbv_param.enabled ? "on" : "off"));

	params.qbv_param.type = ETHERNET_QBV_PARAM_TYPE_TIME;
	ret = net_mgmt(NET_REQUEST_ETHERNET_GET_QBV_PARAM,
		       iface,
		       &params, sizeof(struct ethernet_req_params));
	if (ret < 0) {
		shell_error(sh, "failed to get %s time", argv[1]);
		return ret;
	}
	shell_print(sh, "base_time(s): %"PRIu64, params.qbv_param.base_time.second);
	shell_print(sh, "base_time(fract_ns): %"PRIu64, params.qbv_param.base_time.fract_nsecond);
	shell_print(sh, "cycle_time(s): %"PRIu64, params.qbv_param.cycle_time.second);
	shell_print(sh, "cycle_time(ns): %"PRIu32, params.qbv_param.cycle_time.nanosecond);
	shell_print(sh, "extension_time(ns): %"PRIu32, params.qbv_param.extension_time);

	params.qbv_param.type =	ETHERNET_QBV_PARAM_TYPE_GATE_CONTROL_LIST_LEN;
	ret = net_mgmt(NET_REQUEST_ETHERNET_GET_QBV_PARAM,
		       iface,
		       &params, sizeof(struct ethernet_req_params));
	if (ret < 0) {
		shell_error(sh, "failed to get %s list length", argv[1]);
		return ret;
	}
	shell_print(sh, "list len: %"PRIu32, params.qbv_param.gate_control_list_len);

	list_len = params.qbv_param.gate_control_list_len;
	params.qbv_param.type = ETHERNET_QBV_PARAM_TYPE_GATE_CONTROL_LIST;
	for (uint16_t i = 0; i < list_len; i++) {
		params.qbv_param.gate_control.row = i;
		ret = net_mgmt(NET_REQUEST_ETHERNET_GET_QBV_PARAM,
			       iface,
			       &params, sizeof(struct ethernet_req_params));
		if (ret < 0) {
			shell_error(sh, "failed to get %s gate control", argv[1]);
			return ret;
		}
		gate_status = 0;
		for (int j = 0; j < CONFIG_NET_TC_TX_COUNT; j++) {
			gate_status |= params.qbv_param.gate_control.gate_status[j] << j;
		}
		shell_print(sh, "row: %"PRIu16" interval: %"PRIu32" gate_status: 0x%x",
			    i, params.qbv_param.gate_control.time_interval, gate_status);
	}
#else
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	shell_print(sh, "Set %s to enable %s support.\n", "CONFIG_NET_QBV", "qbv");
	shell_print(sh, "Set %s to enable %s support.\n", "CONFIG_NET_L2_ETHERNET_MGMT",
		    "Ethernet network management interface");
#endif

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_qbv,
	SHELL_CMD_ARG(enable, NULL,
		"Enable: enable <iface_index> <value(off, on)>",
		cmd_qbv_enable, 3, 0),
	SHELL_CMD_ARG(set_config, NULL,
		"Set config: set <iface_index> <base_time(s)> <base_time(2*(-16)ns)> <cycle_time(s)> <cycle_time(ns)> <cycle_time_ext(ns)> <list_len>",
		cmd_qbv_set_config, 8, 0),
	SHELL_CMD_ARG(set_gc, NULL,
		"Set gate control: set <iface_index> <row> <gate_control> <interval>",
		cmd_qbv_set_gc, 5, 0),
	SHELL_CMD_ARG(get_info, NULL,
		"Get info: get_info <iface_index>",
		cmd_qbv_get_info, 2, 0),
	SHELL_SUBCMD_SET_END     /* Array terminated. */
);

SHELL_SUBCMD_ADD((net), qbv, &net_cmd_qbv,
		 "Qbv commands",
		 cmd_net_qbv, 1, 0);
