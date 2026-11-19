/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/random/random.h>
#include <zephyr/ztest.h>

#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>

#define TEST_SESSION_ID 42U
#define EVENT_TIMEOUT   K_MSEC(500)

static const uint8_t peer_mac[WIFI_MAC_ADDR_LEN] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};

struct drv_state {
	struct net_if *iface;
	uint8_t mac_addr[6];
	enum ethernet_if_types eth_if_type;
	wifi_ranging_result_cb_t cb;
	struct wifi_ranging_params last_params;
	uint32_t cancelled;
	bool responder_enabled;
	int start_ret;
	uint32_t assign_id;
};

static struct drv_state drv;

static void wifi_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct drv_state *ctx = dev->data;

	ctx->iface = iface;
	net_if_set_link_addr(iface, ctx->mac_addr, sizeof(ctx->mac_addr), NET_LINK_ETHERNET);
	net_eth_set_if_type_wifi(iface);
	ethernet_init(iface);
}

static int drv_ranging_start(const struct device *dev, struct net_if *iface,
			     struct wifi_ranging_params *params, wifi_ranging_result_cb_t cb)
{
	drv.cb = cb;
	drv.last_params = *params;
	params->session_id = drv.assign_id;

	return drv.start_ret;
}

static int drv_ranging_cancel(const struct device *dev, struct net_if *iface,
			      uint32_t session_id)
{
	drv.cancelled = session_id;

	return 0;
}

static int drv_ranging_responder(const struct device *dev, struct net_if *iface,
				 struct wifi_ranging_responder_params *params)
{
	if (params->oper == WIFI_MGMT_GET) {
		params->enable = drv.responder_enabled;
	} else {
		drv.responder_enabled = params->enable;
	}

	return 0;
}

static int drv_ranging_get_caps(const struct device *dev, struct net_if *iface,
				struct wifi_ranging_caps *caps)
{
	caps->initiator = true;
	caps->responder = true;
	caps->ngp = true;
	caps->max_peers = CONFIG_WIFI_MGMT_RANGING_MAX_PEERS;

	return 0;
}

static int drv_ranging_get_peer_caps(const struct device *dev, struct net_if *iface,
				     struct wifi_ranging_peer_caps *caps)
{
	if (memcmp(caps->mac, peer_mac, sizeof(peer_mac)) != 0) {
		return -ENOENT;
	}

	caps->ftm_responder = true;
	caps->ngp = true;

	return 0;
}

#ifdef CONFIG_WIFI_MGMT_LOCATION
static struct wifi_location_self_params self_loc;

static int drv_location_self(const struct device *dev, struct net_if *iface,
			     struct wifi_location_self_params *params)
{
	if (params->oper == WIFI_MGMT_SET) {
		self_loc = *params;
	} else {
		*params = self_loc;
	}

	return 0;
}
#endif /* CONFIG_WIFI_MGMT_LOCATION */

static struct wifi_mgmt_ops drv_ops = {
	.ranging_start = drv_ranging_start,
	.ranging_cancel = drv_ranging_cancel,
	.ranging_responder = drv_ranging_responder,
	.ranging_get_caps = drv_ranging_get_caps,
	.ranging_get_peer_caps = drv_ranging_get_peer_caps,
#ifdef CONFIG_WIFI_MGMT_LOCATION
	.location_self = drv_location_self,
#endif
};

static const struct net_wifi_mgmt_offload api_funcs = {
	.wifi_iface.iface_api.init = wifi_iface_init,
	.wifi_mgmt_api = &drv_ops,
};

static int wifi_init(const struct device *dev)
{
	struct drv_state *ctx = dev->data;

	ctx->eth_if_type = L2_ETH_IF_TYPE_WIFI;
	/* 00-00-5E-00-53-xx Documentation RFC 7042 */
	ctx->mac_addr[0] = 0x00;
	ctx->mac_addr[1] = 0x00;
	ctx->mac_addr[2] = 0x5E;
	ctx->mac_addr[3] = 0x00;
	ctx->mac_addr[4] = 0x53;
	ctx->mac_addr[5] = sys_rand8_get();

	return 0;
}

ETH_NET_DEVICE_INIT(wlan0, "wifi_ranging_test", wifi_init, NULL, &drv, NULL,
		    CONFIG_ETH_INIT_PRIORITY, &api_funcs, NET_ETH_MTU);

static K_SEM_DEFINE(result_sem, 0, 1);
static K_SEM_DEFINE(done_sem, 0, 1);
static struct wifi_ranging_result rx_result;
static struct wifi_ranging_done rx_done;
static struct net_mgmt_event_callback ev_cb;

static void ev_handler(struct net_mgmt_event_callback *cb, uint64_t event, struct net_if *iface)
{
	if (event == NET_EVENT_WIFI_RANGING_RESULT) {
		memcpy(&rx_result, cb->info, sizeof(rx_result));
		k_sem_give(&result_sem);
	} else if (event == NET_EVENT_WIFI_RANGING_DONE) {
		memcpy(&rx_done, cb->info, sizeof(rx_done));
		k_sem_give(&done_sem);
	}
}

static void *suite_setup(void)
{
	net_mgmt_init_event_callback(&ev_cb, ev_handler,
				     NET_EVENT_WIFI_RANGING_RESULT | NET_EVENT_WIFI_RANGING_DONE);
	net_mgmt_add_event_callback(&ev_cb);

	return NULL;
}

static void before(void *f)
{
	struct net_if *iface = net_if_get_first_wifi();

	zassert_not_null(iface);
	(void)net_if_up(iface);

	drv.cb = NULL;
	drv.cancelled = 0;
	drv.start_ret = 0;
	drv.assign_id = TEST_SESSION_ID;
	drv_ops.ranging_get_peer_caps = drv_ranging_get_peer_caps;
	k_sem_reset(&result_sem);
	k_sem_reset(&done_sem);
}

static void fill_params(struct wifi_ranging_params *params, enum wifi_ranging_mode mode)
{
	memset(params, 0, sizeof(*params));
	params->mode = mode;
	params->num_peers = 1;
	memcpy(params->peers[0].mac, peer_mac, sizeof(peer_mac));
	params->peers[0].band = WIFI_FREQ_BAND_5_GHZ;
	params->peers[0].channel = 36;
}

ZTEST_SUITE(net_wifi_ranging, NULL, suite_setup, before, NULL, NULL);

ZTEST(net_wifi_ranging, test_start_modes)
{
	struct net_if *iface = net_if_get_first_wifi();
	const enum wifi_ranging_mode modes[] = {
		WIFI_RANGING_MODE_AUTO, WIFI_RANGING_MODE_11MC,
		WIFI_RANGING_MODE_11AZ_NTB, WIFI_RANGING_MODE_11AZ_TB,
	};

	for (size_t i = 0; i < ARRAY_SIZE(modes); i++) {
		struct wifi_ranging_params params;

		fill_params(&params, modes[i]);
		/* The caller never picks the id. */
		params.session_id = 0xdead;

		zassert_equal(net_mgmt(NET_REQUEST_WIFI_RANGING_START, iface, &params,
				       sizeof(params)), 0);
		zassert_equal(params.session_id, TEST_SESSION_ID);
		zassert_equal(drv.last_params.mode, modes[i]);
		zassert_equal(drv.last_params.session_id, 0);
	}
}

ZTEST(net_wifi_ranging, test_start_tuning_passthrough)
{
	struct net_if *iface = net_if_get_first_wifi();
	struct wifi_ranging_params params;

	fill_params(&params, WIFI_RANGING_MODE_11MC);
	params.peers[0].tuning.preamble = WIFI_RANGING_PREAMBLE_HE;
	params.peers[0].tuning.ftm.burst_period = 500;
	params.peers[0].tuning.ftm.asap = WIFI_RANGING_ASAP_OFF;

	zassert_equal(net_mgmt(NET_REQUEST_WIFI_RANGING_START, iface, &params, sizeof(params)), 0);
	zassert_equal(drv.last_params.peers[0].tuning.preamble, WIFI_RANGING_PREAMBLE_HE);
	zassert_equal(drv.last_params.peers[0].tuning.ftm.burst_period, 500);
	zassert_equal(drv.last_params.peers[0].tuning.ftm.asap, WIFI_RANGING_ASAP_OFF);
}

ZTEST(net_wifi_ranging, test_start_errors)
{
	struct net_if *iface = net_if_get_first_wifi();
	struct wifi_ranging_params params;

	fill_params(&params, WIFI_RANGING_MODE_11MC);
	params.num_peers = 0;
	zassert_equal(net_mgmt(NET_REQUEST_WIFI_RANGING_START, iface, &params, sizeof(params)),
		      -EINVAL);

	fill_params(&params, WIFI_RANGING_MODE_11MC);
	params.num_peers = CONFIG_WIFI_MGMT_RANGING_MAX_PEERS + 1;
	zassert_equal(net_mgmt(NET_REQUEST_WIFI_RANGING_START, iface, &params, sizeof(params)),
		      -EINVAL);

	fill_params(&params, WIFI_RANGING_MODE_11MC);
	params.mode = (enum wifi_ranging_mode)((int)WIFI_RANGING_MODE_11AZ_TB + 1);
	zassert_equal(net_mgmt(NET_REQUEST_WIFI_RANGING_START, iface, &params, sizeof(params)),
		      -EINVAL);

	params.mode = (enum wifi_ranging_mode)((int)WIFI_RANGING_MODE_AUTO - 1);
	zassert_equal(net_mgmt(NET_REQUEST_WIFI_RANGING_START, iface, &params, sizeof(params)),
		      -EINVAL);

	fill_params(&params, WIFI_RANGING_MODE_11AZ_NTB);
	zassert_equal(net_mgmt(NET_REQUEST_WIFI_RANGING_START, iface, &params,
			       sizeof(params) - 1), -EINVAL);

	/* Driver error is passed through. */
	drv.start_ret = -ENOTSUP;
	zassert_equal(net_mgmt(NET_REQUEST_WIFI_RANGING_START, iface, &params, sizeof(params)),
		      -ENOTSUP);

	/* A missing id is logged, not failed: the session is already running. */
	drv.start_ret = 0;
	drv.assign_id = 0;
	zassert_equal(net_mgmt(NET_REQUEST_WIFI_RANGING_START, iface, &params, sizeof(params)),
		      0);
}

ZTEST(net_wifi_ranging, test_start_location_request)
{
	struct net_if *iface = net_if_get_first_wifi();
	struct wifi_ranging_params params;
	int ret;

	fill_params(&params, WIFI_RANGING_MODE_11MC);
	params.peers[0].request_location = true;

	ret = net_mgmt(NET_REQUEST_WIFI_RANGING_START, iface, &params, sizeof(params));
	if (IS_ENABLED(CONFIG_WIFI_MGMT_LOCATION)) {
		zassert_equal(ret, 0);
	} else {
		zassert_equal(ret, -ENOTSUP);
	}
}

ZTEST(net_wifi_ranging, test_start_iface_down)
{
	struct net_if *iface = net_if_get_first_wifi();
	struct wifi_ranging_params params;

	fill_params(&params, WIFI_RANGING_MODE_11MC);
	zassert_equal(net_if_down(iface), 0);
	zassert_equal(net_mgmt(NET_REQUEST_WIFI_RANGING_START, iface, &params, sizeof(params)),
		      -ENETDOWN);
}

ZTEST(net_wifi_ranging, test_cancel)
{
	struct net_if *iface = net_if_get_first_wifi();
	uint32_t id = TEST_SESSION_ID;

	zassert_equal(net_mgmt(NET_REQUEST_WIFI_RANGING_CANCEL, iface, &id, sizeof(id)), 0);
	zassert_equal(drv.cancelled, TEST_SESSION_ID);

	id = 0;
	zassert_equal(net_mgmt(NET_REQUEST_WIFI_RANGING_CANCEL, iface, &id, sizeof(id)), -EINVAL);
	zassert_equal(net_mgmt(NET_REQUEST_WIFI_RANGING_CANCEL, iface, &id, 1), -EINVAL);
}

ZTEST(net_wifi_ranging, test_events)
{
	struct net_if *iface = net_if_get_first_wifi();
	struct wifi_ranging_params params;
	struct wifi_ranging_result result = {
		.status = WIFI_RANGING_STATUS_SUCCESS,
		.mode = WIFI_RANGING_MODE_11MC,
		.distance_mm = 1500,
	};

	fill_params(&params, WIFI_RANGING_MODE_11MC);
	memcpy(result.mac, peer_mac, sizeof(peer_mac));
	zassert_equal(net_mgmt(NET_REQUEST_WIFI_RANGING_START, iface, &params, sizeof(params)), 0);
	zassert_not_null(drv.cb);

	drv.cb(iface, params.session_id, 0, &result);
	zassert_equal(k_sem_take(&result_sem, EVENT_TIMEOUT), 0);
	zassert_equal(rx_result.session_id, TEST_SESSION_ID);
	zassert_equal(rx_result.distance_mm, 1500);
	zassert_mem_equal(rx_result.mac, peer_mac, sizeof(peer_mac));

	drv.cb(iface, params.session_id, -ETIMEDOUT, NULL);
	zassert_equal(k_sem_take(&done_sem, EVENT_TIMEOUT), 0);
	zassert_equal(rx_done.session_id, TEST_SESSION_ID);
	zassert_equal(rx_done.status, -ETIMEDOUT);
}

ZTEST(net_wifi_ranging, test_responder)
{
	struct net_if *iface = net_if_get_first_wifi();
	struct wifi_ranging_responder_params params = {
		.oper = WIFI_MGMT_SET,
		.enable = true,
		.band = WIFI_FREQ_BAND_5_GHZ,
		.channel = 36,
	};

	zassert_equal(net_mgmt(NET_REQUEST_WIFI_RANGING_RESPONDER, iface, &params,
			       sizeof(params)), 0);
	zassert_true(drv.responder_enabled);

	memset(&params, 0, sizeof(params));
	params.oper = WIFI_MGMT_GET;
	zassert_equal(net_mgmt(NET_REQUEST_WIFI_RANGING_RESPONDER, iface, &params,
			       sizeof(params)), 0);
	zassert_true(params.enable);

	params.oper = (enum wifi_mgmt_op)5;
	zassert_equal(net_mgmt(NET_REQUEST_WIFI_RANGING_RESPONDER, iface, &params,
			       sizeof(params)), -EINVAL);
	zassert_equal(net_mgmt(NET_REQUEST_WIFI_RANGING_RESPONDER, iface, &params, 1), -EINVAL);
}

ZTEST(net_wifi_ranging, test_caps)
{
	struct net_if *iface = net_if_get_first_wifi();
	struct wifi_ranging_caps caps = {0};
	struct wifi_ranging_peer_caps peer = {0};

	zassert_equal(net_mgmt(NET_REQUEST_WIFI_RANGING_CAPS, iface, &caps, sizeof(caps)), 0);
	zassert_true(caps.initiator);
	zassert_true(caps.responder);
	zassert_equal(caps.max_peers, CONFIG_WIFI_MGMT_RANGING_MAX_PEERS);
	zassert_equal(net_mgmt(NET_REQUEST_WIFI_RANGING_CAPS, iface, &caps, 1), -EINVAL);

	memcpy(peer.mac, peer_mac, sizeof(peer_mac));
	zassert_equal(net_mgmt(NET_REQUEST_WIFI_RANGING_PEER_CAPS, iface, &peer, sizeof(peer)), 0);
	zassert_true(peer.ftm_responder);
	zassert_true(peer.ngp);

	/* Unknown peer */
	peer.mac[5] = 0x7f;
	zassert_equal(net_mgmt(NET_REQUEST_WIFI_RANGING_PEER_CAPS, iface, &peer, sizeof(peer)),
		      -ENOENT);

	/* The peer capability op is optional. */
	drv_ops.ranging_get_peer_caps = NULL;
	zassert_equal(net_mgmt(NET_REQUEST_WIFI_RANGING_PEER_CAPS, iface, &peer, sizeof(peer)),
		      -ENOTSUP);
}

#ifdef CONFIG_WIFI_MGMT_LOCATION
ZTEST(net_wifi_ranging, test_location_self)
{
	struct net_if *iface = net_if_get_first_wifi();
	struct wifi_location_self_params params = {
		.oper = WIFI_MGMT_SET,
		.location = {
			.has_lci = true,
			.lci = {.latitude = 1255606845, .altitude = 2688, .altitude_type = 1},
		},
	};

	zassert_equal(net_mgmt(NET_REQUEST_WIFI_LOCATION_SELF, iface, &params, sizeof(params)), 0);

	memset(&params, 0, sizeof(params));
	params.oper = WIFI_MGMT_GET;
	zassert_equal(net_mgmt(NET_REQUEST_WIFI_LOCATION_SELF, iface, &params, sizeof(params)), 0);
	zassert_true(params.location.has_lci);
	zassert_equal(params.location.lci.latitude, 1255606845);

	params.oper = (enum wifi_mgmt_op)5;
	zassert_equal(net_mgmt(NET_REQUEST_WIFI_LOCATION_SELF, iface, &params, sizeof(params)),
		      -EINVAL);
}
#endif /* CONFIG_WIFI_MGMT_LOCATION */
