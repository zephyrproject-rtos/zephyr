/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_tsn_switch_sample, LOG_LEVEL_DBG);

#include "tsn_switch.h"

struct ud g_user_data = {0};

static void bridge_find_cb(struct eth_bridge_iface_context *br, void *user_data)
{
	struct ud *u = user_data;

	if (u->bridge == NULL) {
		u->bridge = br->iface;
		LOG_INF("Find bridge iface %d.", net_if_get_by_iface(br->iface));
	}
}

static void bridge_add_iface_cb(struct net_if *iface, void *user_data)
{
	struct ethernet_context *eth_ctx;
	struct ud *u = user_data;
	int i;
	int ret;

	for (i = 0; i < CONFIG_NET_ETHERNET_BRIDGE_ETH_INTERFACE_COUNT; i++) {
		if (u->iface[i] == NULL) {
			break;
		}
	}

	if (i == CONFIG_NET_ETHERNET_BRIDGE_ETH_INTERFACE_COUNT) {
		return;
	}

	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return;
	}

	eth_ctx = net_if_l2_data(iface);

	if ((net_eth_get_hw_capabilities(iface) & ETHERNET_DSA_USER_PORT) != 0) {
		u->iface[i] = iface;
	} else {
		return;
	}

	LOG_INF("Find iface %d, adding into bridge.", net_if_get_by_iface(iface));

	ret = eth_bridge_iface_add(u->bridge, iface);
	if (ret < 0) {
		LOG_ERR("eth_bridge_iface_add failed: %d", ret);
	}
}

static int qbv_init(struct net_if *iface)
{
	/*
	 * qbv init for demo purpose
	 * - gate0 always on
	 * - gate1 always off
	 * - gate2 has 2ms slot
	 * - gate3 has 8ms slot
	 * - others always on
	 *
	 * time cycle 10ms   gate7 6 5 4 3 2 1 0
	 * -----------------------------------------
	 * slot 1 - 2ms:         1 1 1 1 0 1 0 1
	 * slot 2 - 8ms:         1 1 1 1 1 0 0 1
	 */
	struct ethernet_req_params params_en = {
		.qbv_param.type = ETHERNET_QBV_PARAM_TYPE_STATUS,
		.qbv_param.state = ETHERNET_QBV_STATE_TYPE_ADMIN,
		.qbv_param.enabled = true,
	};
	struct ethernet_req_params params_time = {
		.qbv_param.type = ETHERNET_QBV_PARAM_TYPE_TIME,
		.qbv_param.state = ETHERNET_QBV_STATE_TYPE_ADMIN,
		.qbv_param.base_time.second = 0,
		.qbv_param.base_time.fract_nsecond = 0,
		.qbv_param.cycle_time.second = 0,
		.qbv_param.cycle_time.nanosecond = 10000000,
		.qbv_param.extension_time = 0,
	};
	struct ethernet_req_params params_list_len = {
		.qbv_param.type = ETHERNET_QBV_PARAM_TYPE_GATE_CONTROL_LIST_LEN,
		.qbv_param.gate_control_list_len = 2,
	};
	struct ethernet_req_params params_list = {
		.qbv_param.type = ETHERNET_QBV_PARAM_TYPE_GATE_CONTROL_LIST,
		.qbv_param.state = ETHERNET_QBV_STATE_TYPE_ADMIN,
		.qbv_param.gate_control.time_interval = 0,
		.qbv_param.gate_control.row = 0,
		.qbv_param.gate_control.gate_status = {0},
	};
	uint8_t list1 = 0b11110101;
	uint8_t list2 = 0b11111001;
	struct net_ptp_time tm = {0};
	const struct device *ptp;
	int ret;

	ptp = net_eth_get_ptp_clock(iface);
	if (ptp == NULL) {
		LOG_ERR("No PTP clock device found on iface %d", net_if_get_by_iface(iface));
		return -ENOENT;
	}

	ret = ptp_clock_get(ptp, &tm);
	if (ret < 0) {
		LOG_ERR("Failed to get PTP clock time: %d", ret);
		return -EINVAL;
	}

	/* Enable Qbv */
	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_QBV_PARAM, iface, &params_en, sizeof(params_en));
	if (ret < 0) {
		LOG_ERR("Failed to enable QBV: %d", ret);
		return -EINVAL;
	}

	/* Set base time with a future time which is 3 seconds later */
	params_time.qbv_param.base_time.second = tm.second + 3;
	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_QBV_PARAM, iface, &params_time,
		       sizeof(params_time));
	if (ret < 0) {
		LOG_ERR("Failed to set QBV base time: %d", ret);
		return -EINVAL;
	}

	/* Set gate control list length */
	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_QBV_PARAM, iface, &params_list_len,
		       sizeof(params_list_len));
	if (ret < 0) {
		LOG_ERR("Failed to set QBV gate control list length: %d", ret);
		return -EINVAL;
	}

	/* Set 1st control list */
	params_list.qbv_param.gate_control.time_interval = 2000000;
	params_list.qbv_param.gate_control.row = 0;
	for (int i = 0; i < CONFIG_NET_TC_TX_COUNT; i++) {
		params_list.qbv_param.gate_control.gate_status[i] = ((list1 & BIT(i)) != 0);
	}
	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_QBV_PARAM, iface, &params_list,
		       sizeof(params_list));
	if (ret < 0) {
		LOG_ERR("Failed to set QBV 1st control list: %d", ret);
		return -EINVAL;
	}

	/* Set 2nd control list */
	params_list.qbv_param.gate_control.time_interval = 8000000;
	params_list.qbv_param.gate_control.row = 1;
	for (int i = 0; i < CONFIG_NET_TC_TX_COUNT; i++) {
		params_list.qbv_param.gate_control.gate_status[i] = ((list2 & BIT(i)) != 0);
	}
	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_QBV_PARAM, iface, &params_list,
		       sizeof(params_list));
	if (ret < 0) {
		LOG_ERR("Failed to set QBV 2nd control list: %d", ret);
		return -EINVAL;
	}

	LOG_INF("TSN switch - Qbv enabled on iface %d\r\n"
		"time cycle 10ms   gate 7 6 5 4 3 2 1 0\r\n"
		"--------------------------------------\r\n"
		"slot 1 - 2ms:          1 1 1 1 0 1 0 1\r\n"
		"slot 2 - 8ms:          1 1 1 1 1 0 0 1",
		net_if_get_by_iface(iface));

	return 0;
}

int main(void)
{
	struct ud *u = &g_user_data;

	/* Find the bridge */
	net_eth_bridge_foreach(bridge_find_cb, u);
	if (u->bridge == NULL) {
		LOG_ERR("No bridge found");
		return -ENOENT;
	}

	/* Add interfaces into bridge */
	net_if_foreach(bridge_add_iface_cb, u);
	if (u->iface[0] == NULL) {
		LOG_ERR("No iface found");
		return -ENOENT;
	}

	/* Link up the bridge interface */
	net_if_up(u->bridge);

	/* Config init */
	net_if_set_default(u->bridge);
	(void)net_config_init_app(net_if_get_device(u->bridge), "Initializing network");

	LOG_INF("TSN switch - Bridge created");

	LOG_INF("TSN switch - gPTP enabled");

	return qbv_init(u->iface[0]);
}
