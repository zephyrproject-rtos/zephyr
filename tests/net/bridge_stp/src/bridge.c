/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * RSTP on real bridge interfaces. Three bridges are cabled in a ring through
 * fake Ethernet ports, and bridge 0 has a third port for a host:
 *
 *   bridge0 port f0 --- f1 bridge1
 *   bridge0 port f2 --- f3 bridge2
 *   bridge1 port f4 --- f5 bridge2
 *   bridge0 port f6     (host, no peer)
 *
 * A frame sent on a port is copied and received by the peer port from a work
 * item, not from the sending context.
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/ethernet_bridge.h>
#include <zephyr/net/ethernet_bridge_stp.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/virtual.h>

#include "bridge/bridge_internal.h"
#include "rstp_internal.h"

#define NUM_FAKE    7
#define NUM_BRIDGES 3
#define HOST        6
#define TEST_PTYPE  0x88b5U
#define MAX_FRAME   128
#define WIRE_LEN    32

/* A bridge that restarts learns the root from the next periodic BPDU of its
 * neighbours, as they do not answer the inferior information it sends first
 * (IEEE Std 802.1Q-2014 Figure 13-20, INFERIOR_DESIGNATED).
 */
#define RECONVERGE_TIME K_SECONDS(ETH_RSTP_HELLO_TIME + 1)

/* Which fake port each fake port is cabled to, -1 for none */
static const int peer_of[NUM_FAKE] = {1, 0, 3, 2, 5, 4, -1};
/* Which bridge each fake port belongs to */
static const int bridge_of[NUM_FAKE] = {0, 1, 0, 2, 1, 2, 0};

static const uint8_t bpdu_dst[] = {0x01, 0x80, 0xc2, 0x00, 0x00, 0x00};

struct fake_port {
	struct net_if *iface;
	uint8_t mac[NET_ETH_ADDR_LEN];
	atomic_t bpdu_tx;
	atomic_t data_tx;
	uint8_t last_bpdu[MAX_FRAME];
	size_t last_bpdu_len;
};

struct wire_frame {
	int to;
	size_t len;
	uint8_t data[MAX_FRAME];
};

static struct fake_port fake[NUM_FAKE];
static struct net_if *bridges[NUM_BRIDGES];
static struct wire_frame wire[WIRE_LEN];
static size_t wire_head;
static size_t wire_count;
static uint32_t wire_overflows;
static struct k_spinlock wire_lock;

static void wire_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	while (true) {
		struct wire_frame frame;
		struct net_pkt *pkt;
		k_spinlock_key_t key = k_spin_lock(&wire_lock);

		if (wire_count == 0U) {
			k_spin_unlock(&wire_lock, key);
			return;
		}

		frame = wire[wire_head];
		wire_head = (wire_head + 1U) % WIRE_LEN;
		wire_count--;
		k_spin_unlock(&wire_lock, key);

		pkt = net_pkt_rx_alloc_with_buffer(fake[frame.to].iface, frame.len, NET_AF_UNSPEC,
						   0, K_NO_WAIT);
		if (pkt == NULL) {
			continue;
		}

		if (net_pkt_write(pkt, frame.data, frame.len) < 0 ||
		    net_recv_data(fake[frame.to].iface, pkt) < 0) {
			net_pkt_unref(pkt);
		}
	}
}

static K_WORK_DEFINE(wire_work, wire_handler);

static struct fake_port *fake_of(const struct device *dev)
{
	return dev->data;
}

static void fake_iface_init(struct net_if *iface)
{
	struct fake_port *fp = fake_of(net_if_get_device(iface));
	int index = fp - fake;

	fp->iface = iface;
	fp->mac[0] = 0x02;
	fp->mac[4] = 0x01;
	fp->mac[5] = (uint8_t)index;

	net_if_set_link_addr(iface, fp->mac, sizeof(fp->mac), NET_LINK_ETHERNET);
	ethernet_init(iface);
}

static int fake_send(const struct device *dev, struct net_pkt *pkt)
{
	struct fake_port *fp = fake_of(dev);
	int peer = peer_of[fp - fake];
	uint8_t buf[MAX_FRAME];
	size_t len = net_pkt_get_len(pkt);
	k_spinlock_key_t key;
	uint16_t type;

	if (len > sizeof(buf) || len < sizeof(struct net_eth_hdr)) {
		return 0;
	}

	net_pkt_cursor_init(pkt);
	if (net_pkt_read(pkt, buf, len) < 0) {
		return -EIO;
	}

	type = sys_get_be16(&buf[12]);

	if (memcmp(buf, bpdu_dst, sizeof(bpdu_dst)) == 0) {
		memcpy(fp->last_bpdu, buf, len);
		fp->last_bpdu_len = len;
		atomic_inc(&fp->bpdu_tx);
	} else if (type == TEST_PTYPE) {
		atomic_inc(&fp->data_tx);
	} else {
		return 0;
	}

	if (peer < 0 || !net_if_is_up(fp->iface) || !net_if_is_up(fake[peer].iface)) {
		return 0;
	}

	key = k_spin_lock(&wire_lock);

	if (wire_count == WIRE_LEN) {
		wire_overflows++;
	} else {
		struct wire_frame *frame = &wire[(wire_head + wire_count) % WIRE_LEN];

		frame->to = peer;
		frame->len = len;
		memcpy(frame->data, buf, len);
		wire_count++;
	}

	k_spin_unlock(&wire_lock, key);

	k_work_submit(&wire_work);

	return 0;
}

static enum ethernet_hw_caps fake_get_capabilities(const struct device *dev, struct net_if *iface)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(iface);

	return ETHERNET_PROMISC_MODE | ETHERNET_LINK_100BASE;
}

static int fake_set_config(const struct device *dev, struct net_if *iface,
			   enum ethernet_config_type type, const struct ethernet_config *config)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(iface);
	ARG_UNUSED(config);

	return type == ETHERNET_CONFIG_TYPE_PROMISC_MODE ? 0 : -ENOTSUP;
}

static const struct ethernet_api fake_api = {
	.iface_api.init = fake_iface_init,
	.get_capabilities = fake_get_capabilities,
	.set_config = fake_set_config,
	.send = fake_send,
};

#define FAKE_PORT(n, _)                                                                            \
	ETH_NET_DEVICE_INIT(fake_port##n, "fake_port" #n, NULL, NULL, &fake[n], NULL,              \
			    CONFIG_ETH_INIT_PRIORITY, &fake_api, NET_ETH_MTU)

LISTIFY(NUM_FAKE, FAKE_PORT, (;));

static void find_bridges(struct net_if *iface, void *user_data)
{
	ARG_UNUSED(user_data);

	if (net_if_l2(iface) == &NET_L2_GET_NAME(VIRTUAL) &&
	    (net_virtual_get_iface_capabilities(iface) & VIRTUAL_INTERFACE_BRIDGE) != 0) {
		struct eth_bridge_iface_context *ctx = net_if_get_device(iface)->data;

		bridges[ctx->id] = iface;
	}
}

static void clear_counters(void)
{
	ARRAY_FOR_EACH(fake, i) {
		atomic_clear(&fake[i].bpdu_tx);
		atomic_clear(&fake[i].data_tx);
	}
}

static uint32_t data_frames(void)
{
	uint32_t count = 0U;

	ARRAY_FOR_EACH(fake, i) {
		count += (uint32_t)atomic_get(&fake[i].data_tx);
	}

	return count;
}

static struct eth_bridge_stp_port_status port_status(int f)
{
	struct eth_bridge_stp_port_status status;

	zassert_ok(eth_bridge_stp_port_get_status(bridges[bridge_of[f]], fake[f].iface, &status));

	return status;
}

static struct eth_bridge_stp_status bridge_status(int b)
{
	struct eth_bridge_stp_status status;

	zassert_ok(eth_bridge_stp_get_status(bridges[b], &status));

	return status;
}

/* Receive a frame on the host port as if a host had sent it */
static void host_send(const uint8_t *frame, size_t len)
{
	struct net_pkt *pkt;

	pkt = net_pkt_rx_alloc_with_buffer(fake[HOST].iface, len, NET_AF_UNSPEC, 0, K_FOREVER);
	zassert_not_null(pkt);
	zassert_ok(net_pkt_write(pkt, frame, len));
	zassert_ok(net_recv_data(fake[HOST].iface, pkt));
}

static void host_broadcast(void)
{
	uint8_t frame[60] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
			     0x02, 0x00, 0x00, 0x00, 0x09, 0x99};

	sys_put_be16(TEST_PTYPE, &frame[12]);
	host_send(frame, sizeof(frame));
}

/* The ring as the protocol leaves it: bridge0 is the root, and bridge2
 * blocks its port towards bridge1.
 */
static void expect_ring_converged(void)
{
	int discarding = 0;

	for (int f = 0; f < HOST; f++) {
		if (!port_status(f).forwarding) {
			discarding++;
		}
	}

	zexpect_equal(discarding, 1);
	zexpect_false(port_status(5).forwarding);
	zexpect_equal(port_status(5).role, ETH_BRIDGE_STP_ROLE_ALTERNATE);
	zexpect_equal(eth_bridge_port_get_state(fake[5].iface), ETH_BRIDGE_PORT_STATE_DISCARDING);
	zexpect_equal(bridge_status(2).root_port, fake[3].iface);
	zexpect_equal(bridge_status(1).root_port, fake[1].iface);
	zexpect_is_null(bridge_status(0).root_port);
}

static void *setup(void)
{
	net_if_foreach(find_bridges, NULL);

	ARRAY_FOR_EACH(fake, f) {
		zassert_not_null(fake[f].iface);
		zassert_ok(eth_bridge_iface_add(bridges[bridge_of[f]], fake[f].iface));
		net_if_up(fake[f].iface);
	}

	ARRAY_FOR_EACH(bridges, b) {
		zassert_not_null(bridges[b]);
		zassert_ok(net_if_up(bridges[b]));
		zassert_ok(eth_bridge_stp_enable(bridges[b], true));
	}

	k_sleep(RECONVERGE_TIME);

	return NULL;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	clear_counters();
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);

	zexpect_equal(wire_overflows, 0U);
}

ZTEST(rstp_bridge, test_ring_converges)
{
	struct eth_bridge_stp_status status = bridge_status(2);

	zexpect_true(status.enabled);
	zexpect_equal(status.root_id, bridge_status(0).bridge_id);
	zexpect_equal(status.root_path_cost, ETH_RSTP_DEFAULT_PATH_COST, "100 Mb/s links");

	expect_ring_converged();
}

ZTEST(rstp_bridge, test_bpdu_frame)
{
	struct eth_rstp_bpdu bpdu;
	uint8_t *frame = fake[0].last_bpdu;

	/* Designated Ports send a BPDU every Hello Time */
	k_sleep(K_SECONDS(ETH_RSTP_HELLO_TIME + 1));
	zassert_true(atomic_get(&fake[0].bpdu_tx) > 0);

	zexpect_mem_equal(&frame[6], fake[0].mac, NET_ETH_ADDR_LEN, "source is the port MAC");
	zexpect_equal(sys_get_be16(&frame[12]), 3U + ETH_RSTP_BPDU_RST_LEN, "802.3 length");
	zexpect_equal(frame[14], 0x42);
	zexpect_equal(frame[15], 0x42);
	zexpect_equal(frame[16], 0x03);
	zexpect_true(fake[0].last_bpdu_len >= 17U + ETH_RSTP_BPDU_RST_LEN);

	zassert_ok(eth_rstp_bpdu_decode(&frame[17], ETH_RSTP_BPDU_RST_LEN, &bpdu));
	zexpect_equal(bpdu.type, ETH_RSTP_BPDU_RST);
	zexpect_equal(bpdu.bridge_id, bridge_status(0).bridge_id);
	zexpect_equal((bpdu.flags & ETH_RSTP_FLAG_ROLE_MASK) >> ETH_RSTP_FLAG_ROLE_SHIFT,
		      ETH_RSTP_BPDU_ROLE_DESIGNATED);

	/* No BPDU leaves the bridges as a forwarded frame */
	zexpect_equal(data_frames(), 0U);
}

/* A broadcast from the host crosses each link of the spanning tree once */
ZTEST(rstp_bridge, test_broadcast_not_looped)
{
	host_broadcast();
	k_sleep(K_MSEC(200));

	zexpect_equal(atomic_get(&fake[0].data_tx), 1);
	zexpect_equal(atomic_get(&fake[2].data_tx), 1);
	zexpect_equal(atomic_get(&fake[4].data_tx), 1);
	zexpect_equal(data_frames(), 3U);
}

/* When the link to its Root Port fails, bridge2 forwards through its
 * Alternate Port at once, and returns to the old tree once the link is back.
 */
ZTEST(rstp_bridge, test_link_down)
{
	net_if_carrier_off(fake[2].iface);
	net_if_carrier_off(fake[3].iface);
	k_sleep(K_MSEC(100));

	zexpect_equal(bridge_status(2).root_port, fake[5].iface);
	zexpect_true(port_status(5).forwarding);
	zexpect_equal(port_status(3).role, ETH_BRIDGE_STP_ROLE_DISABLED);
	zexpect_equal(bridge_status(2).root_path_cost, 2U * ETH_RSTP_DEFAULT_PATH_COST);

	host_broadcast();
	k_sleep(K_MSEC(200));
	zexpect_equal(atomic_get(&fake[0].data_tx), 1);
	zexpect_equal(atomic_get(&fake[4].data_tx), 1);
	zexpect_equal(data_frames(), 2U);

	net_if_carrier_on(fake[2].iface);
	net_if_carrier_on(fake[3].iface);
	k_sleep(RECONVERGE_TIME);

	expect_ring_converged();
}

/* Stopping the protocol makes every port forward, starting it again blocks
 * the loop again.
 */
ZTEST(rstp_bridge, test_stop_and_start)
{
	zassert_ok(eth_bridge_stp_enable(bridges[2], false));
	zexpect_equal(eth_bridge_stp_enable(bridges[2], false), -EALREADY);
	zexpect_false(bridge_status(2).enabled);
	zexpect_equal(eth_bridge_port_get_state(fake[5].iface), ETH_BRIDGE_PORT_STATE_FORWARDING);
	zexpect_equal(port_status(5).role, ETH_BRIDGE_STP_ROLE_DISABLED);

	zassert_ok(eth_bridge_stp_enable(bridges[2], true));
	k_sleep(RECONVERGE_TIME);

	expect_ring_converged();
}

/* The host port falls back to STP BPDUs when it hears an STP bridge, and
 * sends RST BPDUs again after mcheck.
 */
ZTEST(rstp_bridge, test_stp_bridge_on_host_port)
{
	uint8_t frame[60] = {0x01, 0x80, 0xc2, 0x00, 0x00, 0x00,
			     0x02, 0x00, 0x00, 0x00, 0x07, 0x77};
	const struct eth_rstp_bpdu config = {
		.type = ETH_RSTP_BPDU_CONFIG,
		.root_id = 0xf000020000000777ULL,
		.bridge_id = 0xf000020000000777ULL,
		.port_id = 0x8001U,
		.times = {.max_age = 20U, .hello_time = 2U, .forward_delay = 15U},
	};

	sys_put_be16(3U + ETH_RSTP_BPDU_CONFIG_LEN, &frame[12]);
	frame[14] = 0x42;
	frame[15] = 0x42;
	frame[16] = 0x03;
	zassert_equal(eth_rstp_bpdu_encode(&config, &frame[17], sizeof(frame) - 17U),
		      ETH_RSTP_BPDU_CONFIG_LEN);

	/* A BPDU in the first MigrateTime after the port started is ignored */
	k_sleep(K_SECONDS(ETH_RSTP_MIGRATE_TIME));

	host_send(frame, sizeof(frame));
	k_sleep(K_MSEC(100));
	zexpect_false(port_status(HOST).send_rstp);
	zexpect_false(port_status(HOST).oper_edge);

	k_sleep(K_SECONDS(ETH_RSTP_HELLO_TIME + 1));
	zexpect_equal(sys_get_be16(&fake[HOST].last_bpdu[12]), 3U + ETH_RSTP_BPDU_CONFIG_LEN);
	zexpect_equal(fake[HOST].last_bpdu[19], 0U, "Protocol Version Identifier");

	zassert_ok(eth_bridge_stp_port_mcheck(bridges[0], fake[HOST].iface));
	zexpect_true(port_status(HOST).send_rstp);

	k_sleep(K_SECONDS(ETH_RSTP_HELLO_TIME + 1));
	zexpect_equal(sys_get_be16(&fake[HOST].last_bpdu[12]), 3U + ETH_RSTP_BPDU_RST_LEN);
	zexpect_true(port_status(HOST).forwarding);

	/* The STP BPDU was not forwarded */
	zexpect_equal(atomic_get(&fake[0].data_tx), 0);
}

ZTEST(rstp_bridge, test_api_errors)
{
	struct eth_bridge_stp_port_status status;

	zexpect_equal(eth_bridge_stp_enable(fake[0].iface, true), -EINVAL);
	zexpect_equal(eth_bridge_stp_port_get_status(bridges[1], fake[0].iface, &status), -ENOENT);
	zexpect_equal(eth_bridge_stp_set_bridge_priority(bridges[1], 1U), -EINVAL);
	zexpect_equal(eth_bridge_stp_set_times(bridges[1], 30U, 10U), -EINVAL);
	zexpect_equal(eth_bridge_stp_set_tx_hold_count(bridges[1], 0U), -EINVAL);
	zexpect_equal(eth_bridge_stp_set_version(bridges[1], 1), -EINVAL);
	zexpect_equal(eth_bridge_stp_port_set_priority(bridges[1], fake[1].iface, 7U), -EINVAL);
	zexpect_equal(eth_bridge_stp_port_set_path_cost(bridges[1], fake[1].iface,
							ETH_RSTP_MAX_PATH_COST + 1U),
		      -EINVAL);
	zexpect_equal(eth_bridge_stp_port_set_p2p(bridges[1], fake[1].iface, 7), -EINVAL);
}

ZTEST_SUITE(rstp_bridge, NULL, setup, before, after, NULL);
