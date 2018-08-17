/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <net/net_if.h>
#include <net/ethernet.h>
#include <net/ethernet_mgmt.h>

#include <ztest.h>

static const u8_t mac_addr_init[6] = { 0x01, 0x02, 0x03,
				       0x04,  0x05,  0x06 };

static const u8_t mac_addr_change[6] = { 0x01, 0x02, 0x03,
					 0x04,  0x05,  0x07 };

struct eth_fake_context {
	struct net_if *iface;
	u8_t mac_address[6];

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
};

static struct eth_fake_context eth_fake_data;

static void eth_fake_iface_init(struct net_if *iface)
{
	struct device *dev = net_if_get_device(iface);
	struct eth_fake_context *ctx = dev->driver_data;

	ctx->iface = iface;

	net_if_set_link_addr(iface, ctx->mac_address,
			     sizeof(ctx->mac_address),
			     NET_LINK_ETHERNET);

	ethernet_init(iface);
}

static int eth_fake_send(struct net_if *iface,
			 struct net_pkt *pkt)
{
	ARG_UNUSED(iface);

	net_pkt_unref(pkt);

	return 0;
}

static enum ethernet_hw_caps eth_fake_get_capabilities(struct device *dev)
{
	return ETHERNET_AUTO_NEGOTIATION_SET | ETHERNET_LINK_10BASE_T |
		ETHERNET_LINK_100BASE_T | ETHERNET_DUPLEX_SET | ETHERNET_QAV |
		ETHERNET_PROMISC_MODE | ETHERNET_PRIORITY_QUEUES;
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

static int eth_fake_set_config(struct device *dev,
			       enum ethernet_config_type type,
			       const struct ethernet_config *config)
{
	struct eth_fake_context *ctx = dev->driver_data;
	int priority_queues_num = ARRAY_SIZE(ctx->priority_queues);
	enum ethernet_qav_param_type qav_param_type;
	int queue_id;

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

static int eth_fake_get_config(struct device *dev,
			       enum ethernet_config_type type,
			       struct ethernet_config *config)
{
	struct eth_fake_context *ctx = dev->driver_data;
	int priority_queues_num = ARRAY_SIZE(ctx->priority_queues);
	enum ethernet_qav_param_type qav_param_type;
	int queue_id;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_PRIORITY_QUEUES_NUM:
		config->priority_queues_num = ARRAY_SIZE(ctx->priority_queues);
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
	default:
		return -ENOTSUP;
	}

	return 0;
}

static struct ethernet_api eth_fake_api_funcs = {
	.iface_api.init = eth_fake_iface_init,
	.iface_api.send = eth_fake_send,

	.get_capabilities = eth_fake_get_capabilities,
	.set_config = eth_fake_set_config,
	.get_config = eth_fake_get_config,
};

static int eth_fake_init(struct device *dev)
{
	struct eth_fake_context *ctx = dev->driver_data;
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

ETH_NET_DEVICE_INIT(eth_fake, "eth_fake", eth_fake_init, &eth_fake_data,
		    NULL, CONFIG_ETH_INIT_PRIORITY, &eth_fake_api_funcs, 1500);

static void test_change_mac_when_up(void)
{
	struct net_if *iface = net_if_get_default();
	struct ethernet_req_params params;
	int ret;

	memcpy(params.mac_address.addr, mac_addr_change, 6);

	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_MAC_ADDRESS, iface,
		       &params, sizeof(struct ethernet_req_params));

	zassert_not_equal(ret, 0,
			  "mac address change should not be possible");
}

static void test_change_mac_when_down(void)
{
	struct net_if *iface = net_if_get_default();
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

static void test_change_auto_neg(void)
{
	struct net_if *iface = net_if_get_default();
	struct ethernet_req_params params;
	int ret;

	params.auto_negotiation = false;

	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_AUTO_NEGOTIATION, iface,
		       &params, sizeof(struct ethernet_req_params));

	zassert_equal(ret, 0, "invalid auto negotiation change");
}

static void test_change_to_same_auto_neg(void)
{
	struct net_if *iface = net_if_get_default();
	struct ethernet_req_params params;
	int ret;

	params.auto_negotiation = false;

	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_AUTO_NEGOTIATION, iface,
		       &params, sizeof(struct ethernet_req_params));

	zassert_not_equal(ret, 0,
			  "invalid change to already auto negotiation");
}

static void test_change_link(void)
{
	struct net_if *iface = net_if_get_default();
	struct ethernet_req_params params = { 0 };
	int ret;

	params.l.link_100bt = true;

	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_LINK, iface,
		       &params, sizeof(struct ethernet_req_params));

	zassert_equal(ret, 0, "invalid link change");
}

static void test_change_same_link(void)
{
	struct net_if *iface = net_if_get_default();
	struct ethernet_req_params params = { 0 };
	int ret;

	params.l.link_100bt = true;

	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_LINK, iface,
		       &params, sizeof(struct ethernet_req_params));

	zassert_not_equal(ret, 0, "invalid same link change");
}

static void test_change_unsupported_link(void)
{
	struct net_if *iface = net_if_get_default();
	struct ethernet_req_params params = { 0 };
	int ret;

	params.l.link_1000bt = true;

	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_LINK, iface,
		       &params, sizeof(struct ethernet_req_params));

	zassert_not_equal(ret, 0, "invalid change to unsupported link");
}

static void test_change_duplex(void)
{
	struct net_if *iface = net_if_get_default();
	struct ethernet_req_params params;
	int ret;

	params.full_duplex = false;

	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_DUPLEX, iface,
		       &params, sizeof(struct ethernet_req_params));

	zassert_equal(ret, 0, "invalid duplex change");
}

static void test_change_same_duplex(void)
{
	struct net_if *iface = net_if_get_default();
	struct ethernet_req_params params;
	int ret;

	params.full_duplex = false;

	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_DUPLEX, iface,
		       &params, sizeof(struct ethernet_req_params));

	zassert_not_equal(ret, 0, "invalid change to already set duplex");
}

static void test_change_qav_params(void)
{
	struct net_if *iface = net_if_get_default();
	struct device *dev = net_if_get_device(iface);
	struct eth_fake_context *ctx = dev->driver_data;
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
		params.qav_param.delta_bandwidth = 10;
		ret = net_mgmt(NET_REQUEST_ETHERNET_SET_QAV_PARAM,
			       iface,
			       &params, sizeof(struct ethernet_req_params));

		zassert_equal(ret, 0, "could not set delta bandwidth");

		/* Reset local value - read-back and verify it */
		params.qav_param.delta_bandwidth = 0;
		ret = net_mgmt(NET_REQUEST_ETHERNET_GET_QAV_PARAM,
			       iface,
			       &params, sizeof(struct ethernet_req_params));

		zassert_equal(ret, 0, "could not read delta bandwidth");
		zassert_equal(params.qav_param.delta_bandwidth, 10,
			      "delta bandwidth did not change");

		/* And them the idle slope */
		params.qav_param.type = ETHERNET_QAV_PARAM_TYPE_IDLE_SLOPE;
		params.qav_param.idle_slope = 10;
		ret = net_mgmt(NET_REQUEST_ETHERNET_SET_QAV_PARAM,
			       iface,
			       &params, sizeof(struct ethernet_req_params));

		zassert_equal(ret, 0, "could not set idle slope");

		/* Reset local value - read-back and verify it */
		params.qav_param.idle_slope = 0;
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

		params.qav_param.delta_bandwidth = 101;
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
	params.qav_param.delta_bandwidth = 10;
	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_QAV_PARAM,
		       iface,
		       &params, sizeof(struct ethernet_req_params));

	zassert_not_equal(ret, 0, "should not be able to set delta bandwidth");

	params.qav_param.type = ETHERNET_QAV_PARAM_TYPE_IDLE_SLOPE;
	params.qav_param.idle_slope = 10;
	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_QAV_PARAM,
		       iface,
		       &params, sizeof(struct ethernet_req_params));

	zassert_not_equal(ret, 0, "should not be able to set idle slope");
}

static void test_change_promisc_mode(bool mode)
{
	struct net_if *iface = net_if_get_default();
	struct ethernet_req_params params;
	int ret;

	params.promisc_mode = mode;

	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_PROMISC_MODE, iface,
		       &params, sizeof(struct ethernet_req_params));

	zassert_equal(ret, 0, "invalid promisc mode change");
}

static void test_change_promisc_mode_on(void)
{
	test_change_promisc_mode(true);
}

static void test_change_promisc_mode_off(void)
{
	test_change_promisc_mode(false);
}

static void test_change_to_same_promisc_mode(void)
{
	struct net_if *iface = net_if_get_default();
	struct ethernet_req_params params;
	int ret;

	params.promisc_mode = true;

	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_PROMISC_MODE, iface,
		       &params, sizeof(struct ethernet_req_params));

	zassert_equal(ret, -EALREADY,
		      "invalid change to already set promisc mode");
}

void test_main(void)
{
	ztest_test_suite(ethernet_mgmt_test,
			 ztest_unit_test(test_change_mac_when_up),
			 ztest_unit_test(test_change_mac_when_down),
			 ztest_unit_test(test_change_auto_neg),
			 ztest_unit_test(test_change_to_same_auto_neg),
			 ztest_unit_test(test_change_link),
			 ztest_unit_test(test_change_same_link),
			 ztest_unit_test(test_change_unsupported_link),
			 ztest_unit_test(test_change_duplex),
			 ztest_unit_test(test_change_same_duplex),
			 ztest_unit_test(test_change_qav_params),
			 ztest_unit_test(test_change_promisc_mode_on),
			 ztest_unit_test(test_change_to_same_promisc_mode),
			 ztest_unit_test(test_change_promisc_mode_off));

	ztest_run_test_suite(ethernet_mgmt_test);
}
