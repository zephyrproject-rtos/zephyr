/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define NET_LOG_LEVEL CONFIG_NET_L2_ETHERNET_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, NET_LOG_LEVEL);

#include <zephyr/kernel.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/ethernet_mgmt.h>

#include <zephyr/ztest.h>

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

static struct net_if *default_iface;

static const uint8_t mac_addr_init[6] = { 0x01, 0x02, 0x03,
				       0x04,  0x05,  0x06 };

static const uint8_t mac_addr_change[6] = { 0x01, 0x02, 0x03,
					 0x04,  0x05,  0x07 };

struct eth_fake_context {
	struct net_if *iface;
	uint8_t mac_address[6];

	bool auto_negotiation;
	bool full_duplex;
	bool link_10bt;
	bool link_100bt;
	bool promisc_mode;
	struct {
		bool qav_enabled;
		int idle_slope;
		int delta_bandwidth;
	} priority_queues[2];

	struct {
		/* Qbv parameters */
		struct {
			bool gate_status[NET_TC_TX_COUNT];
			enum ethernet_gate_state_operation operation;
			uint32_t time_interval;
			uint16_t row;
		} gate_control;
		uint32_t gate_control_list_len;
		bool qbv_enabled;
		struct net_ptp_extended_time base_time;
		struct net_ptp_time cycle_time;
		uint32_t extension_time;

		/* Qbu parameters */
		uint32_t hold_advance;
		uint32_t release_advance;
		enum ethernet_qbu_preempt_status
				frame_preempt_statuses[NET_TC_TX_COUNT];
		bool qbu_enabled;
		bool link_partner_status;
		uint8_t additional_fragment_size : 2;
	} ports[2];

	/* TXTIME parameters */
	bool txtime_statuses[NET_TC_TX_COUNT];
};

static struct eth_fake_context eth_fake_data;

static void eth_fake_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct eth_fake_context *ctx = dev->data;

	ctx->iface = iface;

	net_if_set_link_addr(iface, ctx->mac_address,
			     sizeof(ctx->mac_address),
			     NET_LINK_ETHERNET);

	ethernet_init(iface);
}

static int eth_fake_send(const struct device *dev,
			 struct net_pkt *pkt)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pkt);

	return 0;
}

static enum ethernet_hw_caps eth_fake_get_capabilities(const struct device *dev)
{
	return ETHERNET_AUTO_NEGOTIATION_SET | ETHERNET_LINK_10BASE_T |
		ETHERNET_LINK_100BASE_T | ETHERNET_DUPLEX_SET | ETHERNET_QAV |
		ETHERNET_PROMISC_MODE | ETHERNET_PRIORITY_QUEUES |
		ETHERNET_QBV | ETHERNET_QBU | ETHERNET_TXTIME;
}

static int eth_fake_get_total_bandwidth(struct eth_fake_context *ctx)
{
	if (ctx->link_100bt) {
		return 100 * 1000 * 1000 / 8;
	}

	if (ctx->link_10bt) {
		return 10 * 1000 * 1000 / 8;
	}

	/* No link */
	return 0;
}

static void eth_fake_recalc_qav_delta_bandwidth(struct eth_fake_context *ctx)
{
	int bw;
	int i;

	bw = eth_fake_get_total_bandwidth(ctx);

	for (i = 0; i < ARRAY_SIZE(ctx->priority_queues); ++i) {
		if (bw == 0) {
			ctx->priority_queues[i].delta_bandwidth = 0;
		} else {
			ctx->priority_queues[i].delta_bandwidth =
				(ctx->priority_queues[i].idle_slope * 100);

			ctx->priority_queues[i].delta_bandwidth /= bw;
		}
	}
}

static void eth_fake_recalc_qav_idle_slopes(struct eth_fake_context *ctx)
{
	int bw;
	int i;

	bw = eth_fake_get_total_bandwidth(ctx);

	for (i = 0; i < ARRAY_SIZE(ctx->priority_queues); ++i) {
		ctx->priority_queues[i].idle_slope =
			(ctx->priority_queues[i].delta_bandwidth * bw) / 100;
	}
}

static int eth_fake_set_config(const struct device *dev,
			       enum ethernet_config_type type,
			       const struct ethernet_config *config)
{
	struct eth_fake_context *ctx = dev->data;
	int priority_queues_num = ARRAY_SIZE(ctx->priority_queues);
	int ports_num = ARRAY_SIZE(ctx->ports);
	enum ethernet_qav_param_type qav_param_type;
	enum ethernet_qbv_param_type qbv_param_type;
	enum ethernet_qbu_param_type qbu_param_type;
	enum ethernet_txtime_param_type txtime_param_type;
	int queue_id, port_id;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_AUTO_NEG:
		if (config->auto_negotiation == ctx->auto_negotiation) {
			return -EALREADY;
		}

		ctx->auto_negotiation = config->auto_negotiation;

		break;
	case ETHERNET_CONFIG_TYPE_LINK:
		if ((config->l.link_10bt && ctx->link_10bt) ||
		    (config->l.link_100bt && ctx->link_100bt)) {
			return -EALREADY;
		}

		if (config->l.link_10bt) {
			ctx->link_10bt = true;
			ctx->link_100bt = false;
		} else {
			ctx->link_10bt = false;
			ctx->link_100bt = true;
		}

		eth_fake_recalc_qav_idle_slopes(ctx);

		break;
	case ETHERNET_CONFIG_TYPE_DUPLEX:
		if (config->full_duplex == ctx->full_duplex) {
			return -EALREADY;
		}

		ctx->full_duplex = config->full_duplex;

		break;
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		memcpy(ctx->mac_address, config->mac_address.addr, 6);

		net_if_set_link_addr(ctx->iface, ctx->mac_address,
				     sizeof(ctx->mac_address),
				     NET_LINK_ETHERNET);
		break;
	case ETHERNET_CONFIG_TYPE_QAV_PARAM:
		queue_id = config->qav_param.queue_id;
		qav_param_type = config->qav_param.type;

		if (queue_id < 0 || queue_id >= priority_queues_num) {
			return -EINVAL;
		}

		switch (qav_param_type) {
		case ETHERNET_QAV_PARAM_TYPE_STATUS:
			ctx->priority_queues[queue_id].qav_enabled =
				config->qav_param.enabled;
			break;
		case ETHERNET_QAV_PARAM_TYPE_IDLE_SLOPE:
			ctx->priority_queues[queue_id].idle_slope =
				config->qav_param.idle_slope;

			eth_fake_recalc_qav_delta_bandwidth(ctx);
			break;
		case ETHERNET_QAV_PARAM_TYPE_DELTA_BANDWIDTH:
			ctx->priority_queues[queue_id].delta_bandwidth =
				config->qav_param.delta_bandwidth;

			eth_fake_recalc_qav_idle_slopes(ctx);
			break;
		default:
			return -ENOTSUP;
		}

		break;
	case ETHERNET_CONFIG_TYPE_QBV_PARAM:
		port_id = config->qbv_param.port_id;
		qbv_param_type = config->qbv_param.type;

		if (port_id < 0 || port_id >= ports_num) {
			return -EINVAL;
		}

		switch (qbv_param_type) {
		case ETHERNET_QBV_PARAM_TYPE_STATUS:
			ctx->ports[port_id].qbv_enabled =
				config->qbv_param.enabled;
			break;
		case ETHERNET_QBV_PARAM_TYPE_TIME:
			memcpy(&ctx->ports[port_id].cycle_time,
			       &config->qbv_param.cycle_time,
			       sizeof(ctx->ports[port_id].cycle_time));
			ctx->ports[port_id].extension_time =
				config->qbv_param.extension_time;
			memcpy(&ctx->ports[port_id].base_time,
			       &config->qbv_param.base_time,
			       sizeof(ctx->ports[port_id].base_time));
			break;
		case ETHERNET_QBV_PARAM_TYPE_GATE_CONTROL_LIST:
			ctx->ports[port_id].gate_control.gate_status[0] =
			    config->qbv_param.gate_control.gate_status[0];
			ctx->ports[port_id].gate_control.gate_status[1] =
			    config->qbv_param.gate_control.gate_status[1];
			break;
		case ETHERNET_QBV_PARAM_TYPE_GATE_CONTROL_LIST_LEN:
			ctx->ports[port_id].gate_control_list_len =
			    config->qbv_param.gate_control_list_len;
			break;
		default:
			return -ENOTSUP;
		}

		break;
	case ETHERNET_CONFIG_TYPE_QBU_PARAM:
		port_id = config->qbu_param.port_id;
		qbu_param_type = config->qbu_param.type;

		if (port_id < 0 || port_id >= ports_num) {
			return -EINVAL;
		}

		switch (qbu_param_type) {
		case ETHERNET_QBU_PARAM_TYPE_STATUS:
			ctx->ports[port_id].qbu_enabled =
				config->qbu_param.enabled;
			break;
		case ETHERNET_QBU_PARAM_TYPE_RELEASE_ADVANCE:
			ctx->ports[port_id].release_advance =
				config->qbu_param.release_advance;
			break;
		case ETHERNET_QBU_PARAM_TYPE_HOLD_ADVANCE:
			ctx->ports[port_id].hold_advance =
				config->qbu_param.hold_advance;
			break;
		case ETHERNET_QBR_PARAM_TYPE_LINK_PARTNER_STATUS:
			ctx->ports[port_id].link_partner_status =
				config->qbu_param.link_partner_status;
			break;
		case ETHERNET_QBR_PARAM_TYPE_ADDITIONAL_FRAGMENT_SIZE:
			ctx->ports[port_id].additional_fragment_size =
				config->qbu_param.additional_fragment_size;
			break;
		case ETHERNET_QBU_PARAM_TYPE_PREEMPTION_STATUS_TABLE:
			memcpy(&ctx->ports[port_id].frame_preempt_statuses,
			   &config->qbu_param.frame_preempt_statuses,
			   sizeof(ctx->ports[port_id].frame_preempt_statuses));
			break;
		default:
			return -ENOTSUP;
		}

		break;
	case ETHERNET_CONFIG_TYPE_TXTIME_PARAM:
		queue_id = config->txtime_param.queue_id;
		txtime_param_type = config->txtime_param.type;

		if (queue_id < 0 || queue_id >= priority_queues_num) {
			return -EINVAL;
		}

		switch (txtime_param_type) {
		case ETHERNET_TXTIME_PARAM_TYPE_ENABLE_QUEUES:
			ctx->txtime_statuses[queue_id] =
				config->txtime_param.enable_txtime;
			break;
		default:
			return -ENOTSUP;
		}

		break;
	case ETHERNET_CONFIG_TYPE_PROMISC_MODE:
		if (config->promisc_mode == ctx->promisc_mode) {
			return -EALREADY;
		}

		ctx->promisc_mode = config->promisc_mode;

		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int eth_fake_get_config(const struct device *dev,
			       enum ethernet_config_type type,
			       struct ethernet_config *config)
{
	struct eth_fake_context *ctx = dev->data;
	int priority_queues_num = ARRAY_SIZE(ctx->priority_queues);
	int ports_num = ARRAY_SIZE(ctx->ports);
	enum ethernet_qav_param_type qav_param_type;
	enum ethernet_qbv_param_type qbv_param_type;
	enum ethernet_qbu_param_type qbu_param_type;
	enum ethernet_txtime_param_type txtime_param_type;
	int queue_id, port_id;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_PRIORITY_QUEUES_NUM:
		config->priority_queues_num = ARRAY_SIZE(ctx->priority_queues);
		break;
	case ETHERNET_CONFIG_TYPE_PORTS_NUM:
		config->ports_num = ARRAY_SIZE(ctx->ports);
		break;
	case ETHERNET_CONFIG_TYPE_QAV_PARAM:
		queue_id = config->qav_param.queue_id;
		qav_param_type = config->qav_param.type;

		if (queue_id < 0 || queue_id >= priority_queues_num) {
			return -EINVAL;
		}

		switch (qav_param_type) {
		case ETHERNET_QAV_PARAM_TYPE_STATUS:
			config->qav_param.enabled =
				ctx->priority_queues[queue_id].qav_enabled;
			break;
		case ETHERNET_QAV_PARAM_TYPE_IDLE_SLOPE:
		case ETHERNET_QAV_PARAM_TYPE_OPER_IDLE_SLOPE:
			/* No distinction between idle slopes for fake eth */
			config->qav_param.idle_slope =
				ctx->priority_queues[queue_id].idle_slope;
			break;
		case ETHERNET_QAV_PARAM_TYPE_DELTA_BANDWIDTH:
			config->qav_param.delta_bandwidth =
				ctx->priority_queues[queue_id].delta_bandwidth;
			break;
		case ETHERNET_QAV_PARAM_TYPE_TRAFFIC_CLASS:
			/* Default TC for BE - it doesn't really matter here */
			config->qav_param.traffic_class =
				net_tx_priority2tc(NET_PRIORITY_BE);
			break;
		default:
			return -ENOTSUP;
		}

		break;
	case ETHERNET_CONFIG_TYPE_QBV_PARAM:
		port_id = config->qbv_param.port_id;
		qbv_param_type = config->qbv_param.type;

		if (port_id < 0 || port_id >= ports_num) {
			return -EINVAL;
		}

		switch (qbv_param_type) {
		case ETHERNET_QBV_PARAM_TYPE_STATUS:
			config->qbv_param.enabled =
				ctx->ports[port_id].qbv_enabled;
			break;
		case ETHERNET_QBV_PARAM_TYPE_TIME:
			memcpy(&config->qbv_param.base_time,
			       &ctx->ports[port_id].base_time,
			       sizeof(config->qbv_param.base_time));
			memcpy(&config->qbv_param.cycle_time,
			       &ctx->ports[port_id].cycle_time,
			       sizeof(config->qbv_param.cycle_time));
			config->qbv_param.extension_time =
				ctx->ports[port_id].extension_time;
			break;
		case ETHERNET_QBV_PARAM_TYPE_GATE_CONTROL_LIST_LEN:
			config->qbv_param.gate_control_list_len =
				ctx->ports[port_id].gate_control_list_len;
			break;
		case ETHERNET_QBV_PARAM_TYPE_GATE_CONTROL_LIST:
			memcpy(&config->qbv_param.gate_control,
			       &ctx->ports[port_id].gate_control,
			       sizeof(config->qbv_param.gate_control));
			break;
		default:
			return -ENOTSUP;
		}

		break;
	case ETHERNET_CONFIG_TYPE_QBU_PARAM:
		port_id = config->qbu_param.port_id;
		qbu_param_type = config->qbu_param.type;

		if (port_id < 0 || port_id >= ports_num) {
			return -EINVAL;
		}

		switch (qbu_param_type) {
		case ETHERNET_QBU_PARAM_TYPE_STATUS:
			config->qbu_param.enabled =
				ctx->ports[port_id].qbu_enabled;
			break;
		case ETHERNET_QBU_PARAM_TYPE_RELEASE_ADVANCE:
			config->qbu_param.release_advance =
				ctx->ports[port_id].release_advance;
			break;
		case ETHERNET_QBU_PARAM_TYPE_HOLD_ADVANCE:
			config->qbu_param.hold_advance =
				ctx->ports[port_id].hold_advance;
			break;
		case ETHERNET_QBR_PARAM_TYPE_LINK_PARTNER_STATUS:
			config->qbu_param.link_partner_status =
				ctx->ports[port_id].link_partner_status;
			break;
		case ETHERNET_QBR_PARAM_TYPE_ADDITIONAL_FRAGMENT_SIZE:
			config->qbu_param.additional_fragment_size =
				ctx->ports[port_id].additional_fragment_size;
			break;
		case ETHERNET_QBU_PARAM_TYPE_PREEMPTION_STATUS_TABLE:
			memcpy(&config->qbu_param.frame_preempt_statuses,
			     &ctx->ports[port_id].frame_preempt_statuses,
			     sizeof(config->qbu_param.frame_preempt_statuses));
			break;
		default:
			return -ENOTSUP;
		}

		break;
	case ETHERNET_CONFIG_TYPE_TXTIME_PARAM:
		queue_id = config->txtime_param.queue_id;
		txtime_param_type = config->txtime_param.type;

		if (queue_id < 0 || queue_id >= priority_queues_num) {
			return -EINVAL;
		}

		switch (txtime_param_type) {
		case ETHERNET_TXTIME_PARAM_TYPE_ENABLE_QUEUES:
			config->txtime_param.enable_txtime =
				ctx->txtime_statuses[queue_id];
			break;
		default:
			return -ENOTSUP;
		}

		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static struct ethernet_api eth_fake_api_funcs = {
	.iface_api.init = eth_fake_iface_init,

	.get_capabilities = eth_fake_get_capabilities,
	.set_config = eth_fake_set_config,
	.get_config = eth_fake_get_config,
	.send = eth_fake_send,
};

static int eth_fake_init(const struct device *dev)
{
	struct eth_fake_context *ctx = dev->data;
	int i;

	ctx->auto_negotiation = true;
	ctx->full_duplex = true;
	ctx->link_10bt = true;
	ctx->link_100bt = false;

	memcpy(ctx->mac_address, mac_addr_init, 6);

	/* Initialize priority queues */
	for (i = 0; i < ARRAY_SIZE(ctx->priority_queues); ++i) {
		ctx->priority_queues[i].qav_enabled = true;
		if (i + 1 == ARRAY_SIZE(ctx->priority_queues)) {
			/* 75% for the last priority queue */
			ctx->priority_queues[i].delta_bandwidth = 75;
		} else {
			/* 0% for the rest */
			ctx->priority_queues[i].delta_bandwidth = 0;
		}
	}

	eth_fake_recalc_qav_idle_slopes(ctx);

	return 0;
}

ETH_NET_DEVICE_INIT(eth_fake, "eth_fake", eth_fake_init, NULL,
		    &eth_fake_data, NULL, CONFIG_ETH_INIT_PRIORITY,
		    &eth_fake_api_funcs, NET_ETH_MTU);

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
static const char *iface2str(struct net_if *iface)
{
#ifdef CONFIG_NET_L2_ETHERNET
	if (net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) {
		return "Ethernet";
	}
#endif

#ifdef CONFIG_NET_L2_DUMMY
	if (net_if_l2(iface) == &NET_L2_GET_NAME(DUMMY)) {
		return "Dummy";
	}
#endif

	return "<unknown type>";
}
#endif

static void iface_cb(struct net_if *iface, void *user_data)
{
	struct net_if **my_iface = user_data;

	DBG("Interface %p (%s) [%d]\n", iface, iface2str(iface),
	    net_if_get_by_iface(iface));

	if (net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) {
		if (PART_OF_ARRAY(NET_IF_GET_NAME(eth_fake, 0), iface)) {
			*my_iface = iface;
		}
	}
}

static void *ethernet_mgmt_setup(void)
{
	net_if_foreach(iface_cb, &default_iface);

	zassert_not_null(default_iface, "Cannot find test interface");

	return NULL;
}

static void change_mac_when_up(void)
{
	struct net_if *iface = default_iface;
	struct ethernet_req_params params;
	int ret;

	memcpy(params.mac_address.addr, mac_addr_change, 6);

	net_if_up(iface);

	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_MAC_ADDRESS, iface,
		       &params, sizeof(struct ethernet_req_params));

	zassert_not_equal(ret, 0,
			  "mac address change should not be possible");
}

static void change_mac_when_down(void)
{
	struct net_if *iface = default_iface;
	struct ethernet_req_params params;
	int ret;

	memcpy(params.mac_address.addr, mac_addr_change, 6);

	net_if_down(iface);

	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_MAC_ADDRESS, iface,
		       &params, sizeof(struct ethernet_req_params));

	zassert_equal(ret, 0, "unable to change mac address");

	ret = memcmp(net_if_get_link_addr(iface)->addr, mac_addr_change,
		     sizeof(mac_addr_change));

	zassert_equal(ret, 0, "invalid mac address change");

	net_if_up(iface);
}

ZTEST(net_ethernet_mgmt, test_change_mac)
{
	change_mac_when_up();
	change_mac_when_down();
}

static void change_auto_neg(bool is_auto_neg)
{
	struct net_if *iface = default_iface;
	struct ethernet_req_params params;
	int ret;

	params.auto_negotiation = is_auto_neg;

	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_AUTO_NEGOTIATION, iface,
		       &params, sizeof(struct ethernet_req_params));

	zassert_equal(ret, 0, "invalid auto negotiation change");
}

static void change_to_same_auto_neg(bool is_auto_neg)
{
	struct net_if *iface = default_iface;
	struct ethernet_req_params params;
	int ret;

	params.auto_negotiation = is_auto_neg;

	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_AUTO_NEGOTIATION, iface,
		       &params, sizeof(struct ethernet_req_params));

	zassert_not_equal(ret, 0,
			  "invalid change to already auto negotiation");
}

ZTEST(net_ethernet_mgmt, test_change_auto_neg)
{
	change_auto_neg(false);
	change_to_same_auto_neg(false);
	change_auto_neg(true);
}

static void change_link_10bt(void)
{
	struct net_if *iface = default_iface;
	struct ethernet_req_params params = { 0 };
	int ret;

	params.l.link_10bt = true;

	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_LINK, iface,
		       &params, sizeof(struct ethernet_req_params));

	zassert_equal(ret, 0, "invalid link change");
}

static void change_link_100bt(void)
{
	struct net_if *iface = default_iface;
	struct ethernet_req_params params = { 0 };
	int ret;

	params.l.link_100bt = true;

	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_LINK, iface,
		       &params, sizeof(struct ethernet_req_params));

	zassert_equal(ret, 0, "invalid link change");
}

static void change_same_link_100bt(void)
{
	struct net_if *iface = default_iface;
	struct ethernet_req_params params = { 0 };
	int ret;

	params.l.link_100bt = true;

	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_LINK, iface,
		       &params, sizeof(struct ethernet_req_params));

	zassert_not_equal(ret, 0, "invalid same link change");
}

static void change_unsupported_link_1000bt(void)
{
	struct net_if *iface = default_iface;
	struct ethernet_req_params params = { 0 };
	int ret;

	params.l.link_1000bt = true;

	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_LINK, iface,
		       &params, sizeof(struct ethernet_req_params));

	zassert_not_equal(ret, 0, "invalid change to unsupported link");
}

ZTEST(net_ethernet_mgmt, test_change_link)
{
	change_link_100bt();
	change_same_link_100bt();
	change_unsupported_link_1000bt();
	change_link_10bt();
}

static void change_duplex(bool is_full_duplex)
{
	struct net_if *iface = default_iface;
	struct ethernet_req_params params;
	int ret;

	params.full_duplex = is_full_duplex;

	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_DUPLEX, iface,
		       &params, sizeof(struct ethernet_req_params));

	zassert_equal(ret, 0, "invalid duplex change");
}

static void change_same_duplex(bool is_full_duplex)
{
	struct net_if *iface = default_iface;
	struct ethernet_req_params params;
	int ret;

	params.full_duplex = is_full_duplex;

	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_DUPLEX, iface,
		       &params, sizeof(struct ethernet_req_params));

	zassert_not_equal(ret, 0, "invalid change to already set duplex");
}

ZTEST(net_ethernet_mgmt, test_change_duplex)
{
	change_duplex(false);
	change_same_duplex(false);
	change_duplex(true);
}

ZTEST(net_ethernet_mgmt, test_change_qav_params)
{
	struct net_if *iface = default_iface;
	const struct device *dev = net_if_get_device(iface);
	struct eth_fake_context *ctx = dev->data;
	struct ethernet_req_params params;
	int available_priority_queues;
	int i;
	int ret;

	/* Try to get the number of the priority queues */
	ret = net_mgmt(NET_REQUEST_ETHERNET_GET_PRIORITY_QUEUES_NUM,
		       iface,
		       &params, sizeof(struct ethernet_req_params));

	zassert_equal(ret, 0, "could not get the number of priority queues");

	available_priority_queues = params.priority_queues_num;

	zassert_not_equal(available_priority_queues, 0,
			  "returned no priority queues");
	zassert_equal(available_priority_queues,
		      ARRAY_SIZE(ctx->priority_queues),
		      "an invalid number of priority queues returned");

	for (i = 0; i < available_priority_queues; ++i) {
		/* Try to set correct params to a correct queue id */
		params.qav_param.queue_id = i;

		/* Disable Qav for queue */
		params.qav_param.type = ETHERNET_QAV_PARAM_TYPE_STATUS;
		params.qav_param.enabled = false;
		ret = net_mgmt(NET_REQUEST_ETHERNET_SET_QAV_PARAM,
			       iface,
			       &params, sizeof(struct ethernet_req_params));

		zassert_equal(ret, 0, "could not disable qav");

		/* Invert it to make sure the read-back value is proper */
		params.qav_param.enabled = true;

		ret = net_mgmt(NET_REQUEST_ETHERNET_GET_QAV_PARAM,
			       iface,
			       &params, sizeof(struct ethernet_req_params));

		zassert_equal(ret, 0, "could not read qav status");

		zassert_equal(false, params.qav_param.enabled,
			      "qav should be disabled");

		/* Re-enable Qav for queue */
		params.qav_param.enabled = true;
		ret = net_mgmt(NET_REQUEST_ETHERNET_SET_QAV_PARAM,
			       iface,
			       &params, sizeof(struct ethernet_req_params));

		zassert_equal(ret, 0, "could not enable qav");

		/* Invert it to make sure the read-back value is proper */
		params.qav_param.enabled = false;

		ret = net_mgmt(NET_REQUEST_ETHERNET_GET_QAV_PARAM,
			       iface,
			       &params, sizeof(struct ethernet_req_params));

		zassert_equal(ret, 0, "could not read qav status");

		zassert_equal(true, params.qav_param.enabled,
			      "qav should be enabled");

		/* Starting with delta bandwidth */
		params.qav_param.type =
			ETHERNET_QAV_PARAM_TYPE_DELTA_BANDWIDTH;
		params.qav_param.delta_bandwidth = 10U;
		ret = net_mgmt(NET_REQUEST_ETHERNET_SET_QAV_PARAM,
			       iface,
			       &params, sizeof(struct ethernet_req_params));

		zassert_equal(ret, 0, "could not set delta bandwidth");

		/* Reset local value - read-back and verify it */
		params.qav_param.delta_bandwidth = 0U;
		ret = net_mgmt(NET_REQUEST_ETHERNET_GET_QAV_PARAM,
			       iface,
			       &params, sizeof(struct ethernet_req_params));

		zassert_equal(ret, 0, "could not read delta bandwidth");
		zassert_equal(params.qav_param.delta_bandwidth, 10,
			      "delta bandwidth did not change");

		/* And them the idle slope */
		params.qav_param.type = ETHERNET_QAV_PARAM_TYPE_IDLE_SLOPE;
		params.qav_param.idle_slope = 10U;
		ret = net_mgmt(NET_REQUEST_ETHERNET_SET_QAV_PARAM,
			       iface,
			       &params, sizeof(struct ethernet_req_params));

		zassert_equal(ret, 0, "could not set idle slope");

		/* Reset local value - read-back and verify it */
		params.qav_param.idle_slope = 0U;
		ret = net_mgmt(NET_REQUEST_ETHERNET_GET_QAV_PARAM,
			       iface,
			       &params, sizeof(struct ethernet_req_params));

		zassert_equal(ret, 0, "could not read idle slope");
		zassert_equal(params.qav_param.idle_slope, 10,
			      "idle slope did not change");

		/* Oper idle slope should also be the same */
		params.qav_param.type =
			ETHERNET_QAV_PARAM_TYPE_OPER_IDLE_SLOPE;
		ret = net_mgmt(NET_REQUEST_ETHERNET_GET_QAV_PARAM,
			       iface,
			       &params, sizeof(struct ethernet_req_params));

		zassert_equal(ret, 0, "could not read oper idle slope");
		zassert_equal(params.qav_param.oper_idle_slope, 10,
			      "oper idle slope should equal admin idle slope");

		/* Now try to set incorrect params to a correct queue */
		params.qav_param.type =
			ETHERNET_QAV_PARAM_TYPE_DELTA_BANDWIDTH;
		params.qav_param.delta_bandwidth = -10;
		ret = net_mgmt(NET_REQUEST_ETHERNET_SET_QAV_PARAM,
			       iface,
			       &params, sizeof(struct ethernet_req_params));

		zassert_not_equal(ret, 0,
				  "allowed to set invalid delta bandwidth");

		params.qav_param.delta_bandwidth = 101U;
		ret = net_mgmt(NET_REQUEST_ETHERNET_SET_QAV_PARAM,
			       iface,
			       &params, sizeof(struct ethernet_req_params));

		zassert_not_equal(ret, 0,
				  "allowed to set invalid delta bandwidth");
	}

	/* Try to set read-only parameters */
	params.qav_param.queue_id = 0;
	params.qav_param.type = ETHERNET_QAV_PARAM_TYPE_OPER_IDLE_SLOPE;
	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_QAV_PARAM,
		       iface,
		       &params, sizeof(struct ethernet_req_params));

	zassert_not_equal(ret, 0, "should not be able to set oper idle slope");

	params.qav_param.queue_id = 0;
	params.qav_param.type = ETHERNET_QAV_PARAM_TYPE_TRAFFIC_CLASS;
	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_QAV_PARAM,
		       iface,
		       &params, sizeof(struct ethernet_req_params));

	zassert_not_equal(ret, 0, "should not be able to set traffic class");

	/* Now try to set valid parameters to an invalid queue id */
	params.qav_param.type = ETHERNET_QAV_PARAM_TYPE_DELTA_BANDWIDTH;
	params.qav_param.queue_id = available_priority_queues;
	params.qav_param.delta_bandwidth = 10U;
	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_QAV_PARAM,
		       iface,
		       &params, sizeof(struct ethernet_req_params));

	zassert_not_equal(ret, 0, "should not be able to set delta bandwidth");

	params.qav_param.type = ETHERNET_QAV_PARAM_TYPE_IDLE_SLOPE;
	params.qav_param.idle_slope = 10U;
	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_QAV_PARAM,
		       iface,
		       &params, sizeof(struct ethernet_req_params));

	zassert_not_equal(ret, 0, "should not be able to set idle slope");
}

ZTEST(net_ethernet_mgmt, test_change_qbv_params)
{
	struct net_if *iface = default_iface;
	const struct device *dev = net_if_get_device(iface);
	struct eth_fake_context *ctx = dev->data;
	struct ethernet_req_params params;
	struct net_ptp_time cycle_time;
	int available_ports;
	int i;
	int ret;

	/* Try to get the number of the ports */
	ret = net_mgmt(NET_REQUEST_ETHERNET_GET_PORTS_NUM,
		       iface,
		       &params, sizeof(struct ethernet_req_params));

	zassert_equal(ret, 0, "could not get the number of ports (%d)", ret);

	available_ports = params.ports_num;

	zassert_not_equal(available_ports, 0, "returned no priority queues");
	zassert_equal(available_ports,
		      ARRAY_SIZE(ctx->ports),
		      "an invalid number of ports returned");

	for (i = 0; i < available_ports; ++i) {
		/* Try to set correct params to a correct queue id */
		params.qbv_param.port_id = i;

		/* Disable Qbv for port */
		params.qbv_param.type = ETHERNET_QBV_PARAM_TYPE_STATUS;
		params.qbv_param.state = ETHERNET_QBV_STATE_TYPE_ADMIN;
		params.qbv_param.enabled = false;
		ret = net_mgmt(NET_REQUEST_ETHERNET_SET_QBV_PARAM,
			       iface,
			       &params, sizeof(struct ethernet_req_params));

		zassert_equal(ret, 0, "could not disable qbv for port %d", i);

		/* Invert it to make sure the read-back value is proper */
		params.qbv_param.enabled = true;

		ret = net_mgmt(NET_REQUEST_ETHERNET_GET_QBV_PARAM,
			       iface,
			       &params, sizeof(struct ethernet_req_params));

		zassert_equal(ret, 0, "could not read qbv status (%d)", ret);

		zassert_equal(false, params.qbv_param.enabled,
			      "qbv should be disabled");

		/* Re-enable Qbv for queue */
		params.qbv_param.enabled = true;
		ret = net_mgmt(NET_REQUEST_ETHERNET_SET_QBV_PARAM,
			       iface,
			       &params, sizeof(struct ethernet_req_params));

		zassert_equal(ret, 0, "could not enable qbv (%d)", ret);

		/* Invert it to make sure the read-back value is proper */
		params.qbv_param.enabled = false;

		ret = net_mgmt(NET_REQUEST_ETHERNET_GET_QBV_PARAM,
			       iface,
			       &params, sizeof(struct ethernet_req_params));

		zassert_equal(ret, 0, "could not read qbv status (%d)", ret);

		zassert_equal(true, params.qbv_param.enabled,
			      "qbv should be enabled");

		/* Then the Qbv parameter checks */

		params.qbv_param.type = ETHERNET_QBV_PARAM_TYPE_TIME;

		params.qbv_param.base_time.second = 10ULL;
		params.qbv_param.base_time.fract_nsecond = 20ULL;
		params.qbv_param.cycle_time.second = 30ULL;
		params.qbv_param.cycle_time.nanosecond = 20;
		params.qbv_param.extension_time = 40;

		ret = net_mgmt(NET_REQUEST_ETHERNET_SET_QBV_PARAM,
			       iface,
			       &params, sizeof(struct ethernet_req_params));

		zassert_equal(ret, 0, "could not set base time (%d)", ret);

		/* Reset local value - read-back and verify it */
		params.qbv_param.base_time.second = 0ULL;
		params.qbv_param.base_time.fract_nsecond = 0ULL;
		params.qbv_param.cycle_time.second = 0ULL;
		params.qbv_param.cycle_time.nanosecond = 0;
		params.qbv_param.extension_time = 0;

		ret = net_mgmt(NET_REQUEST_ETHERNET_GET_QBV_PARAM,
			       iface,
			       &params, sizeof(struct ethernet_req_params));

		zassert_equal(ret, 0, "could not read times (%d)", ret);
		zassert_equal(params.qbv_param.base_time.second, 10ULL,
			      "base_time.second did not change");
		zassert_equal(params.qbv_param.base_time.fract_nsecond, 20ULL,
			      "base_time.fract_nsecond did not change");

		cycle_time.second = 30ULL;
		cycle_time.nanosecond = 20;
		zassert_true(params.qbv_param.cycle_time.second == cycle_time.second &&
			     params.qbv_param.cycle_time.nanosecond == cycle_time.nanosecond,
			     "cycle time did not change");

		zassert_equal(params.qbv_param.extension_time, 40,
			      "extension time did not change");

		params.qbv_param.type =
			ETHERNET_QBV_PARAM_TYPE_GATE_CONTROL_LIST;
		params.qbv_param.gate_control.gate_status[0] = true;
		params.qbv_param.gate_control.gate_status[1] = false;
		ret = net_mgmt(NET_REQUEST_ETHERNET_SET_QBV_PARAM,
			       iface,
			       &params, sizeof(struct ethernet_req_params));

		zassert_equal(ret, 0, "could not set gate control list (%d)",
			      ret);

		/* Reset local value - read-back and verify it */
		ret = net_mgmt(NET_REQUEST_ETHERNET_GET_QBV_PARAM,
			       iface,
			       &params, sizeof(struct ethernet_req_params));

		zassert_equal(ret, 0, "could not read gate control (%d)", ret);

		params.qbv_param.type =
			ETHERNET_QBV_PARAM_TYPE_GATE_CONTROL_LIST_LEN;
		params.qbv_param.gate_control_list_len = 1;
		ret = net_mgmt(NET_REQUEST_ETHERNET_SET_QBV_PARAM,
			       iface,
			       &params, sizeof(struct ethernet_req_params));

		zassert_equal(ret, 0,
			      "could not set gate control list len (%d)", ret);

		/* Reset local value - read-back and verify it */
		params.qbv_param.gate_control_list_len = 0;
		ret = net_mgmt(NET_REQUEST_ETHERNET_GET_QBV_PARAM,
			       iface,
			       &params, sizeof(struct ethernet_req_params));

		zassert_equal(ret, 0,
			      "could not read gate control list len (%d)",
			      ret);
		zassert_equal(params.qbv_param.gate_control_list_len, 1,
			      "gate control list len did not change");
	}

	/* Try to set read-only parameters */
	params.qbv_param.state = ETHERNET_QBV_STATE_TYPE_OPER;
	params.qbv_param.type = ETHERNET_QBV_PARAM_TYPE_TIME;
	params.qbv_param.extension_time = 50;
	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_QBV_PARAM,
		       iface,
		       &params, sizeof(struct ethernet_req_params));

	zassert_not_equal(ret, 0, "allowed to set oper status parameter (%d)",
			  ret);

	params.qbv_param.state = ETHERNET_QBV_STATE_TYPE_ADMIN;
	params.qbv_param.type = ETHERNET_QBV_PARAM_TYPE_TIME;
	params.qbv_param.base_time.fract_nsecond = 1000000000;
	params.qbv_param.cycle_time.nanosecond = 1000000000;
	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_QBV_PARAM,
		       iface,
		       &params, sizeof(struct ethernet_req_params));

	zassert_not_equal(ret, 0, "allowed to set base_time parameter (%d)",
			  ret);
}

ZTEST(net_ethernet_mgmt, test_change_qbu_params)
{
	struct net_if *iface = default_iface;
	const struct device *dev = net_if_get_device(iface);
	struct eth_fake_context *ctx = dev->data;
	struct ethernet_req_params params;
	int available_ports;
	int i, j;
	int ret;

	/* Try to get the number of the ports */
	ret = net_mgmt(NET_REQUEST_ETHERNET_GET_PORTS_NUM,
		       iface,
		       &params, sizeof(struct ethernet_req_params));

	zassert_equal(ret, 0, "could not get the number of ports (%d)", ret);

	available_ports = params.ports_num;

	zassert_not_equal(available_ports, 0, "returned no priority queues");
	zassert_equal(available_ports,
		      ARRAY_SIZE(ctx->ports),
		      "an invalid number of ports returned");

	for (i = 0; i < available_ports; ++i) {
		/* Try to set correct params to a correct queue id */
		params.qbu_param.port_id = i;

		/* Disable Qbu for port */
		params.qbu_param.type = ETHERNET_QBU_PARAM_TYPE_STATUS;
		params.qbu_param.enabled = false;
		ret = net_mgmt(NET_REQUEST_ETHERNET_SET_QBU_PARAM,
			       iface,
			       &params, sizeof(struct ethernet_req_params));

		zassert_equal(ret, 0, "could not disable qbu for port %d (%d)",
			      i, ret);

		/* Invert it to make sure the read-back value is proper */
		params.qbu_param.enabled = true;

		ret = net_mgmt(NET_REQUEST_ETHERNET_GET_QBU_PARAM,
			       iface,
			       &params, sizeof(struct ethernet_req_params));

		zassert_equal(ret, 0, "could not read qbu status (%d)", ret);

		zassert_equal(false, params.qbu_param.enabled,
			      "qbu should be disabled");

		/* Re-enable Qbu for queue */
		params.qbu_param.enabled = true;
		ret = net_mgmt(NET_REQUEST_ETHERNET_SET_QBU_PARAM,
			       iface,
			       &params, sizeof(struct ethernet_req_params));

		zassert_equal(ret, 0, "could not enable qbu (%d)", ret);

		/* Invert it to make sure the read-back value is proper */
		params.qbu_param.enabled = false;

		ret = net_mgmt(NET_REQUEST_ETHERNET_GET_QBU_PARAM,
			       iface,
			       &params, sizeof(struct ethernet_req_params));

		zassert_equal(ret, 0, "could not read qbu status (%d)", ret);

		zassert_equal(true, params.qbu_param.enabled,
			      "qbu should be enabled");

		/* Then the Qbu parameter checks */

		params.qbu_param.type = ETHERNET_QBU_PARAM_TYPE_RELEASE_ADVANCE;
		params.qbu_param.release_advance = 10;
		ret = net_mgmt(NET_REQUEST_ETHERNET_SET_QBU_PARAM,
			       iface,
			       &params, sizeof(struct ethernet_req_params));

		zassert_equal(ret, 0, "could not set release advance (%d)",
			      ret);

		/* Reset local value - read-back and verify it */
		params.qbu_param.release_advance = 0;
		ret = net_mgmt(NET_REQUEST_ETHERNET_GET_QBU_PARAM,
			       iface,
			       &params, sizeof(struct ethernet_req_params));

		zassert_equal(ret, 0, "could not read release advance (%d)",
			      ret);
		zassert_equal(params.qbu_param.release_advance, 10,
			      "release_advance did not change");

		/* And them the hold advance */
		params.qbu_param.type = ETHERNET_QBU_PARAM_TYPE_HOLD_ADVANCE;
		params.qbu_param.hold_advance = 20;
		ret = net_mgmt(NET_REQUEST_ETHERNET_SET_QBU_PARAM,
			       iface,
			       &params, sizeof(struct ethernet_req_params));

		zassert_equal(ret, 0, "could not set hold advance (%d)", ret);

		/* Reset local value - read-back and verify it */
		params.qbu_param.hold_advance = 0;
		ret = net_mgmt(NET_REQUEST_ETHERNET_GET_QBU_PARAM,
			       iface,
			       &params, sizeof(struct ethernet_req_params));

		zassert_equal(ret, 0, "could not read hold advance (%d)", ret);
		zassert_equal(params.qbu_param.hold_advance, 20,
			      "hold advance did not change");

		params.qbu_param.type =
			ETHERNET_QBR_PARAM_TYPE_LINK_PARTNER_STATUS;
		params.qbu_param.link_partner_status = true;
		ret = net_mgmt(NET_REQUEST_ETHERNET_SET_QBU_PARAM,
			       iface,
			       &params, sizeof(struct ethernet_req_params));

		zassert_equal(ret, -EINVAL,
			      "could set link partner status (%d)", ret);

		/* Reset local value - read-back and verify it */
		params.qbu_param.link_partner_status = false;
		ret = net_mgmt(NET_REQUEST_ETHERNET_GET_QBU_PARAM,
			       iface,
			       &params, sizeof(struct ethernet_req_params));

		zassert_equal(ret, 0,
			      "could not read link partner status (%d)", ret);
		zassert_equal(params.qbu_param.link_partner_status, false,
			      "link partner status changed");

		params.qbu_param.type =
			ETHERNET_QBR_PARAM_TYPE_ADDITIONAL_FRAGMENT_SIZE;
		params.qbu_param.additional_fragment_size = 2;
		ret = net_mgmt(NET_REQUEST_ETHERNET_SET_QBU_PARAM,
			       iface,
			       &params, sizeof(struct ethernet_req_params));

		zassert_equal(ret, 0,
			      "could not set additional frag size (%d)", ret);

		/* Reset local value - read-back and verify it */
		params.qbu_param.additional_fragment_size = 1;
		ret = net_mgmt(NET_REQUEST_ETHERNET_GET_QBU_PARAM,
			       iface,
			       &params, sizeof(struct ethernet_req_params));

		zassert_equal(ret, 0,
			      "could not read additional frag size (%d)", ret);
		zassert_equal(params.qbu_param.additional_fragment_size, 2,
			      "additional fragment size did not change");

		params.qbu_param.type =
			ETHERNET_QBU_PARAM_TYPE_PREEMPTION_STATUS_TABLE;

		for (j = 0;
		     j < ARRAY_SIZE(params.qbu_param.frame_preempt_statuses);
		     j++) {
			/* Set the preempt status for different priorities.
			 */
			params.qbu_param.frame_preempt_statuses[j] =
				j % 2 ? ETHERNET_QBU_STATUS_EXPRESS :
					ETHERNET_QBU_STATUS_PREEMPTABLE;
		}

		ret = net_mgmt(NET_REQUEST_ETHERNET_SET_QBU_PARAM,
			       iface,
			       &params, sizeof(struct ethernet_req_params));

		zassert_equal(ret, 0,
			      "could not set frame preempt status (%d)", ret);

		/* Reset local value - read-back and verify it */
		for (j = 0;
		     j < ARRAY_SIZE(params.qbu_param.frame_preempt_statuses);
		     j++) {
			params.qbu_param.frame_preempt_statuses[j] =
				j % 2 ? ETHERNET_QBU_STATUS_PREEMPTABLE :
					ETHERNET_QBU_STATUS_EXPRESS;
		}

		ret = net_mgmt(NET_REQUEST_ETHERNET_GET_QBU_PARAM,
			       iface,
			       &params, sizeof(struct ethernet_req_params));

		zassert_equal(ret, 0,
			      "could not read frame preempt status (%d)", ret);

		for (j = 0;
		     j < ARRAY_SIZE(params.qbu_param.frame_preempt_statuses);
		     j++) {
			zassert_equal(
				params.qbu_param.frame_preempt_statuses[j],
				j % 2 ? ETHERNET_QBU_STATUS_EXPRESS :
					ETHERNET_QBU_STATUS_PREEMPTABLE,
			      "frame preempt status did not change");
		}
	}
}

ZTEST(net_ethernet_mgmt, test_change_txtime_params)
{
	struct net_if *iface = default_iface;
	const struct device *dev = net_if_get_device(iface);
	struct eth_fake_context *ctx = dev->data;
	struct ethernet_req_params params;
	int available_priority_queues;
	int ret;
	int i;

	/* Try to get the number of the priority queues */
	ret = net_mgmt(NET_REQUEST_ETHERNET_GET_PRIORITY_QUEUES_NUM,
		       iface,
		       &params, sizeof(struct ethernet_req_params));

	zassert_equal(ret, 0, "could not get the number of priority queues");

	available_priority_queues = params.priority_queues_num;

	zassert_not_equal(available_priority_queues, 0,
			  "returned no priority queues");
	zassert_equal(available_priority_queues,
		      ARRAY_SIZE(ctx->priority_queues),
		      "an invalid number of priority queues returned");

	net_if_up(iface);

	/* Make sure we cannot enable txtime if the interface is up */
	params.txtime_param.queue_id = 0;
	params.txtime_param.type = ETHERNET_TXTIME_PARAM_TYPE_ENABLE_QUEUES;
	params.txtime_param.enable_txtime = false;
	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_TXTIME_PARAM, iface,
		       &params, sizeof(struct ethernet_req_params));

	zassert_equal(ret, -EACCES, "could disable TXTIME for queue 0 (%d)",
		      ret);

	net_if_down(iface);

	for (i = 0; i < available_priority_queues; ++i) {
		/* Try to set correct params to a correct queue id */
		params.txtime_param.queue_id = i;

		/* Disable TXTIME for queue */
		params.txtime_param.type = ETHERNET_TXTIME_PARAM_TYPE_ENABLE_QUEUES;
		params.txtime_param.enable_txtime = false;
		ret = net_mgmt(NET_REQUEST_ETHERNET_SET_TXTIME_PARAM,
			       iface,
			       &params, sizeof(struct ethernet_req_params));

		zassert_equal(ret, 0, "could not disable TXTIME for queue %d (%d)",
			      i, ret);

		/* Invert it to make sure the read-back value is proper */
		params.txtime_param.enable_txtime = true;

		ret = net_mgmt(NET_REQUEST_ETHERNET_GET_TXTIME_PARAM, iface,
			       &params, sizeof(struct ethernet_req_params));

		zassert_equal(ret, 0, "could not read txtime status (%d)", ret);

		zassert_equal(false, params.txtime_param.enable_txtime,
			      "txtime should be disabled");

		/* Re-enable TXTIME for queue */
		params.txtime_param.enable_txtime = true;
		ret = net_mgmt(NET_REQUEST_ETHERNET_SET_TXTIME_PARAM, iface,
			       &params, sizeof(struct ethernet_req_params));

		zassert_equal(ret, 0, "could not enable txtime (%d)", ret);

		/* Invert it to make sure the read-back value is proper */
		params.txtime_param.enable_txtime = false;

		ret = net_mgmt(NET_REQUEST_ETHERNET_GET_TXTIME_PARAM, iface,
			       &params, sizeof(struct ethernet_req_params));

		zassert_equal(ret, 0, "could not read txtime status (%d)", ret);

		zassert_equal(true, params.txtime_param.enable_txtime,
			      "txtime should be enabled");
	}
}

static void change_promisc_mode(bool mode)
{
	struct net_if *iface = default_iface;
	struct ethernet_req_params params;
	int ret;

	params.promisc_mode = mode;

	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_PROMISC_MODE, iface,
		       &params, sizeof(struct ethernet_req_params));

	zassert_equal(ret, 0, "invalid promisc mode change");
}

static void change_promisc_mode_on(void)
{
	change_promisc_mode(true);
}

static void change_promisc_mode_off(void)
{
	change_promisc_mode(false);
}

static void change_to_same_promisc_mode(void)
{
	struct net_if *iface = default_iface;
	struct ethernet_req_params params;
	int ret;

	params.promisc_mode = true;

	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_PROMISC_MODE, iface,
		       &params, sizeof(struct ethernet_req_params));

	zassert_equal(ret, -EALREADY,
		      "invalid change to already set promisc mode");
}

ZTEST(net_ethernet_mgmt, test_change_to_promisc_mode)
{
	change_promisc_mode_on();
	change_to_same_promisc_mode();
	change_promisc_mode_off();
}
ZTEST_SUITE(net_ethernet_mgmt, NULL, ethernet_mgmt_setup, NULL, NULL, NULL);
