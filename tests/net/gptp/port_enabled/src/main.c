/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/ptp_clock.h>
#include <zephyr/kernel.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/gptp.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/ptp_time.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "gptp_messages.h"
#include "gptp_data_set.h"
#include "gptp_mi.h"
#include "gptp_private.h"

/* The gPTP thread runs this port, on the only Ethernet interface that has a
 * PTP clock.
 */
#define THREAD_PORT GPTP_PORT_START

/* No interface is registered for this port, so the gPTP thread leaves it alone
 * and the tests can step its state machines themselves.
 */
#define MANUAL_PORT (GPTP_PORT_START + 1)

#define ROLE_TIMEOUT_MS  5000
#define ANNOUNCE_TIMEOUT K_SECONDS(5)

struct peer_frame {
	struct net_eth_hdr eth;
	struct gptp_hdr hdr;
	/* Pdelay_Resp and Pdelay_Resp_Follow_Up have the same layout. */
	struct gptp_pdelay_resp body;
} __packed;

static K_SEM_DEFINE(announce_sent, 0, K_SEM_MAX_LIMIT);

static void test_time_get(struct net_ptp_time *tm)
{
	uint64_t now = k_ticks_to_ns_floor64(k_uptime_ticks());

	tm->second = now / NSEC_PER_SEC;
	tm->nanosecond = now % NSEC_PER_SEC;
}

static int test_clock_set(const struct device *dev, struct net_ptp_time *tm)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(tm);

	return 0;
}

static int test_clock_get(const struct device *dev, struct net_ptp_time *tm)
{
	ARG_UNUSED(dev);

	test_time_get(tm);

	return 0;
}

static int test_clock_adjust(const struct device *dev, int increment)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(increment);

	return 0;
}

static int test_clock_adjust_rate(const struct device *dev, int64_t scaled_ppm)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(scaled_ppm);

	return 0;
}

static DEVICE_API(ptp_clock, test_clock_api) = {
	.set = test_clock_set,
	.get = test_clock_get,
	.adjust = test_clock_adjust,
	.adjust_rate = test_clock_adjust_rate,
};

DEVICE_DEFINE(gptp_test_clock, "gptp_test_clock", NULL, NULL, NULL, NULL, POST_KERNEL,
	      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &test_clock_api);

static void test_eth_iface_init(struct net_if *iface)
{
	/* 00-00-5E-00-53-xx, documentation range of RFC 7042. */
	static uint8_t mac[] = {0x00, 0x00, 0x5e, 0x00, 0x53, 0x01};

	net_if_set_link_addr(iface, mac, sizeof(mac), NET_LINK_ETHERNET);
	ethernet_init(iface);
}

static const struct device *test_eth_get_ptp_clock(const struct device *dev, struct net_if *iface)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(iface);

	return DEVICE_GET(gptp_test_clock);
}

/* Answer a Pdelay_Req as the link partner would. All timestamps of the
 * exchange are the same instant, which makes the propagation delay zero.
 */
static void test_peer_send(struct net_if *iface, uint8_t type, const struct gptp_hdr *req,
			   struct net_ptp_time *tm)
{
	struct peer_frame frame = {
		.eth = {
			.dst = gptp_multicast_eth_addr,
			.src.addr = {0x00, 0x00, 0x5e, 0x00, 0x53, 0x02},
			.type = net_htons(NET_ETH_PTYPE_PTP),
		},
		.hdr = {
			.message_type = type,
			.transport_specific = GPTP_TRANSPORT_802_1_AS,
			.ptp_version = GPTP_VERSION,
			.message_length = net_htons(GPTP_PDELAY_RESP_LEN),
			.port_id = {
				.clk_id = {0x00, 0x00, 0x5e, 0xff, 0xfe, 0x00, 0x53, 0x02},
				.port_number = net_htons(1),
			},
			.sequence_id = req->sequence_id,
			.control = GPTP_OTHER_CONTROL_VALUE,
			.log_msg_interval = GPTP_RESP_LOG_MSG_ITV,
		},
		.body = {
			.req_receipt_ts_secs_high = net_htons(tm->_sec.high),
			.req_receipt_ts_secs_low = net_htonl(tm->_sec.low),
			.req_receipt_ts_nsecs = net_htonl(tm->nanosecond),
			.requesting_port_id = req->port_id,
		},
	};
	struct net_pkt *pkt;

	pkt = net_pkt_rx_alloc_with_buffer(iface, sizeof(frame), NET_AF_UNSPEC, 0, K_NO_WAIT);
	zassert_not_null(pkt);
	zassert_ok(net_pkt_write(pkt, &frame, sizeof(frame)));
	net_pkt_set_timestamp(pkt, tm);
	net_pkt_set_rx_timestamping(pkt, true);

	if (net_recv_data(iface, pkt) < 0) {
		net_pkt_unref(pkt);
	}
}

static int test_eth_send(const struct device *dev, struct net_pkt *pkt)
{
	struct gptp_hdr *hdr = GPTP_HDR(pkt);
	struct net_ptp_time now;

	ARG_UNUSED(dev);

	test_time_get(&now);
	net_pkt_set_timestamp(pkt, &now);

	switch (hdr->message_type) {
	case GPTP_PATH_DELAY_REQ_MESSAGE:
		test_peer_send(net_pkt_iface(pkt), GPTP_PATH_DELAY_RESP_MESSAGE, hdr, &now);
		test_peer_send(net_pkt_iface(pkt), GPTP_PATH_DELAY_FOLLOWUP_MESSAGE, hdr, &now);
		break;
	case GPTP_ANNOUNCE_MESSAGE:
		k_sem_give(&announce_sent);
		break;
	default:
		break;
	}

	return 0;
}

static DEVICE_API(ethernet, test_eth_api) = {
	.iface_api.init = test_eth_iface_init,
	.get_ptp_clock = test_eth_get_ptp_clock,
	.send = test_eth_send,
};

ETH_NET_DEVICE_INIT(gptp_test_eth, "gptp_test_eth", NULL, NULL, NULL, NULL,
		    CONFIG_ETH_INIT_PRIORITY, &test_eth_api, NET_ETH_MTU);

static void test_dummy_iface_init(struct net_if *iface)
{
	ARG_UNUSED(iface);
}

static int test_dummy_send(const struct device *dev, struct net_pkt *pkt)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pkt);

	return 0;
}

static DEVICE_API(dummy, test_dummy_api) = {
	.iface_api.init = test_dummy_iface_init,
	.send = test_dummy_send,
};

NET_DEVICE_INIT(gptp_test_dummy, "gptp_test_dummy", NULL, NULL, NULL, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &test_dummy_api,
		DUMMY_L2, NET_L2_GET_CTX_TYPE(DUMMY_L2), 127);

static bool wait_for_role(int port, enum gptp_port_state role)
{
	return WAIT_FOR(GPTP_GLOBAL_DS()->selected_role[port] == role,
			ROLE_TIMEOUT_MS * USEC_PER_MSEC, k_msleep(1));
}

static void before(void *fixture)
{
	struct gptp_port_ds *port_ds = GPTP_PORT_DS(MANUAL_PORT);
	struct gptp_port_bmca_data *bmca_data = GPTP_PORT_BMCA_DATA(MANUAL_PORT);
	struct net_if *iface = net_if_lookup_by_dev(DEVICE_GET(gptp_test_dummy));

	ARG_UNUSED(fixture);

	zassert_not_null(iface);
	zassert_true(net_if_is_up(iface));
	GPTP_PORT_IFACE(MANUAL_PORT) = iface;

	port_ds->ptt_port_enabled = true;
	port_ds->as_capable = true;

	/* A port that has taken its own information, as the only grandmaster
	 * on a link does.
	 */
	bmca_data->info_is = GPTP_INFO_IS_MINE;
	bmca_data->rcvd_msg = false;
	bmca_data->updt_info = false;
	GPTP_PORT_STATE(MANUAL_PORT)->pa_info.state = GPTP_PA_INFO_CURRENT;
	GPTP_PORT_STATE(MANUAL_PORT)->pa_rcv.state = GPTP_PA_RCV_RECEIVE;
	GPTP_PORT_STATE(MANUAL_PORT)->pa_rcv.rcvd_announce = false;
}

static void after(void *fixture)
{
	struct gptp_port_bmca_data *bmca_data = GPTP_PORT_BMCA_DATA(MANUAL_PORT);
	struct net_if *iface = GPTP_PORT_IFACE(MANUAL_PORT);

	ARG_UNUSED(fixture);

	if (bmca_data->rcvd_announce_ptr != NULL) {
		net_pkt_unref(bmca_data->rcvd_announce_ptr);
		bmca_data->rcvd_announce_ptr = NULL;
	}

	if (!net_if_is_up(iface)) {
		zassert_ok(net_if_up(iface));
	}

	GPTP_PORT_IFACE(MANUAL_PORT) = NULL;
}

ZTEST(gptp_port_enabled, test_interface_up_keeps_announce_information)
{
	struct gptp_port_bmca_data *bmca_data = GPTP_PORT_BMCA_DATA(MANUAL_PORT);

	gptp_mi_port_bmca_state_machines(MANUAL_PORT);

	zexpect_equal(bmca_data->info_is, GPTP_INFO_IS_MINE);
	zexpect_equal(GPTP_PORT_STATE(MANUAL_PORT)->pa_info.state, GPTP_PA_INFO_CURRENT);
}

ZTEST(gptp_port_enabled, test_interface_down_disables_announce_information)
{
	struct gptp_port_bmca_data *bmca_data = GPTP_PORT_BMCA_DATA(MANUAL_PORT);

	zassert_ok(net_if_down(GPTP_PORT_IFACE(MANUAL_PORT)));
	gptp_mi_port_bmca_state_machines(MANUAL_PORT);

	zexpect_equal(bmca_data->info_is, GPTP_INFO_IS_DISABLED);
	zexpect_equal(GPTP_PORT_STATE(MANUAL_PORT)->pa_info.state, GPTP_PA_INFO_POST_DISABLED);

	/* The port stays disabled for as long as the interface is down. */
	gptp_mi_port_bmca_state_machines(MANUAL_PORT);

	zexpect_equal(bmca_data->info_is, GPTP_INFO_IS_DISABLED);
	zexpect_equal(GPTP_PORT_STATE(MANUAL_PORT)->pa_info.state, GPTP_PA_INFO_POST_DISABLED);
}

ZTEST(gptp_port_enabled, test_interface_up_again_asks_for_role_selection)
{
	struct gptp_port_bmca_data *bmca_data = GPTP_PORT_BMCA_DATA(MANUAL_PORT);

	zassert_ok(net_if_down(GPTP_PORT_IFACE(MANUAL_PORT)));
	gptp_mi_port_bmca_state_machines(MANUAL_PORT);

	zassert_ok(net_if_up(GPTP_PORT_IFACE(MANUAL_PORT)));
	/* POST_DISABLED to AGED, then AGED asks for a new role selection. */
	gptp_mi_port_bmca_state_machines(MANUAL_PORT);
	gptp_mi_port_bmca_state_machines(MANUAL_PORT);

	zexpect_equal(bmca_data->info_is, GPTP_INFO_IS_AGED);
	zexpect_equal(GPTP_PORT_STATE(MANUAL_PORT)->pa_info.state, GPTP_PA_INFO_UPDATE);
}

ZTEST(gptp_port_enabled, test_interface_down_releases_held_announce)
{
	struct gptp_port_bmca_data *bmca_data = GPTP_PORT_BMCA_DATA(MANUAL_PORT);
	struct net_pkt *announce;

	announce = net_pkt_alloc(K_NO_WAIT);
	zassert_not_null(announce);

	/* A qualified Announce that the information state machine has not
	 * taken yet. Keep an observer reference to check that it is released.
	 */
	bmca_data->rcvd_announce_ptr = net_pkt_ref(announce);
	bmca_data->rcvd_msg = true;

	zassert_ok(net_if_down(GPTP_PORT_IFACE(MANUAL_PORT)));
	gptp_mi_port_bmca_state_machines(MANUAL_PORT);

	zexpect_is_null(bmca_data->rcvd_announce_ptr);
	zexpect_false(bmca_data->rcvd_msg);
	zexpect_equal(atomic_get(&announce->atomic_ref), 1);

	net_pkt_unref(announce);
}

ZTEST(gptp_port_enabled, test_port_is_master_again_after_interface_bounce)
{
	struct net_if *iface = net_if_lookup_by_dev(DEVICE_GET(gptp_test_eth));

	zassert_not_null(iface);
	zassert_equal(gptp_get_port_number(iface), THREAD_PORT);

	/* With no Announce from the link partner, the port becomes master. */
	zassert_true(wait_for_role(THREAD_PORT, GPTP_PORT_MASTER));
	k_sem_reset(&announce_sent);
	zassert_ok(k_sem_take(&announce_sent, ANNOUNCE_TIMEOUT));

	zassert_ok(net_if_down(iface));
	zassert_true(wait_for_role(THREAD_PORT, GPTP_PORT_DISABLED));

	zassert_ok(net_if_up(iface));
	zassert_true(wait_for_role(THREAD_PORT, GPTP_PORT_MASTER));
	k_sem_reset(&announce_sent);
	zexpect_ok(k_sem_take(&announce_sent, ANNOUNCE_TIMEOUT));
}

ZTEST_SUITE(gptp_port_enabled, NULL, NULL, before, after, NULL);
