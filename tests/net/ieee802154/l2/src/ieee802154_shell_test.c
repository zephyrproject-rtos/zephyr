/*
 * Copyright (c) 2023 Florian Grandel
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_ieee802154_mgmt_test, LOG_LEVEL_DBG);

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <zephyr/net/ieee802154_mgmt.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/shell/shell.h>

#include <ieee802154_frame.h>

extern struct net_pkt *current_pkt;
extern struct k_sem driver_lock;

static struct net_if *iface;

static struct net_mgmt_event_callback scan_cb;
K_SEM_DEFINE(scan_lock, 0, 1);

#define EXPECTED_COORDINATOR_LQI           15U
#define EXPECTED_COORDINATOR_PAN_LE        0xcd, 0xab
#define EXPECTED_COORDINATOR_PAN_CPU_ORDER 0xabcd
#define EXPECTED_COORDINATOR_ADDR_LE       0xc2, 0xa3, 0x9e, 0x00, 0x00, 0x4b, 0x12, 0x00
#define EXPECTED_COORDINATOR_ADDR_BE       0x00, 0x12, 0x4b, 0x00, 0x00, 0x9e, 0xa3, 0xc2

static void scan_result_cb(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
			   struct net_if *iface)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	struct ieee802154_req_params *scan_ctx = ctx->scan_ctx;
	uint8_t expected_coordinator_address[] = {EXPECTED_COORDINATOR_ADDR_BE};

	/* No need for scan_ctx locking as we should execute exclusively. */

	zassert_not_null(scan_ctx);
	zassert_equal(EXPECTED_COORDINATOR_PAN_CPU_ORDER, scan_ctx->pan_id,
		      "Scan did not receive correct PAN id.");
	zassert_equal(IEEE802154_EXT_ADDR_LENGTH, scan_ctx->len,
		      "Scan did not receive correct co-ordinator address length.");
	zassert_mem_equal(expected_coordinator_address, scan_ctx->addr, IEEE802154_EXT_ADDR_LENGTH);
	zassert_equal(EXPECTED_COORDINATOR_LQI, scan_ctx->lqi,
		      "Scan did not receive correct link quality indicator.");

	k_sem_give(&scan_lock);
}

void test_beacon_request(struct ieee802154_mpdu *mpdu)
{
	struct ieee802154_command *cmd = mpdu->command;

	zassert_equal(1U, mpdu->payload_length, "Beacon request: invalid payload length.");
	zassert_equal(IEEE802154_CFI_BEACON_REQUEST, cmd->cfi, "Not a beacon request.");
	zassert_equal(IEEE802154_ADDR_MODE_SHORT, mpdu->mhr.fs->fc.dst_addr_mode,
		      "Beacon request: invalid destination address mode.");
	zassert_equal(IEEE802154_BROADCAST_ADDRESS, mpdu->mhr.dst_addr->plain.addr.short_addr,
		      "Beacon request: destination address should be broadcast address.");
	zassert_equal(IEEE802154_BROADCAST_PAN_ID, mpdu->mhr.dst_addr->plain.pan_id,
		      "Beacon request: destination PAN should be broadcast PAN.");
}

void test_scan_shell_cmd(void)
{
	struct ieee802154_mpdu mpdu = {0};
	int ret;

	ret = shell_execute_cmd(NULL, "ieee802154 scan active 11 500");
	zassert_equal(0, ret, "Active scan failed: %d", ret);

	/* Expect the beacon to have been received and handled already by the scan command. */
	zassert_equal(0, k_sem_take(&scan_lock, K_NO_WAIT), "Active scan: did not receive beacon.");

	zassert_not_null(current_pkt);

	if (!ieee802154_validate_frame(net_pkt_data(current_pkt), net_pkt_get_len(current_pkt),
				       &mpdu)) {
		NET_ERR("*** Could not parse beacon request.\n");
		ztest_test_fail();
		goto release_frag;
	}

	test_beacon_request(&mpdu);

release_frag:
	net_pkt_frag_unref(current_pkt->frags);
	current_pkt->frags = NULL;
}

ZTEST(ieee802154_l2_shell, test_active_scan)
{
	uint8_t beacon_pkt[] = {
		0x00, 0xd0, /* FCF */
		0x11, /* Sequence Number: 17 */
		EXPECTED_COORDINATOR_PAN_LE, /* Source PAN */
		EXPECTED_COORDINATOR_ADDR_LE, /* Extended Source Address */
		0x00, 0xc0, /* Superframe Specification: PAN coordinator + association permitted */
		0x00, /* GTS */
		0x00, /* Pending Addresses */
		0x00, 0x00 /* Payload */
	};
	struct net_pkt *pkt;

	pkt = net_pkt_rx_alloc_with_buffer(iface, sizeof(beacon_pkt), AF_UNSPEC, 0, K_FOREVER);
	if (!pkt) {
		NET_ERR("*** No buffer to allocate\n");
		ztest_test_fail();
		return;
	}

	net_pkt_set_ieee802154_lqi(pkt, EXPECTED_COORDINATOR_LQI);
	net_buf_add_mem(pkt->buffer, beacon_pkt, sizeof(beacon_pkt));

	if (net_recv_data(iface, pkt) < 0) {
		NET_ERR("Recv data failed");
		net_pkt_unref(pkt);
		ztest_test_fail();
		return;
	}

	net_mgmt_init_event_callback(&scan_cb, scan_result_cb, NET_EVENT_IEEE802154_SCAN_RESULT);
	net_mgmt_add_event_callback(&scan_cb);

	test_scan_shell_cmd();

	net_mgmt_del_event_callback(&scan_cb);
}

static void *test_setup(void)
{
	const struct device *dev = device_get_binding("fake_ieee802154");

	k_sem_reset(&driver_lock);

	if (!dev) {
		NET_ERR("*** Could not get fake device\n");
		return NULL;
	}

	iface = net_if_lookup_by_dev(dev);
	if (!iface) {
		NET_ERR("*** Could not get fake iface\n");
		return NULL;
	}

	NET_INFO("Fake IEEE 802.15.4 network interface ready\n");

	current_pkt = net_pkt_rx_alloc(K_FOREVER);
	if (!current_pkt) {
		NET_ERR("*** No buffer to allocate\n");
		return false;
	}

	return NULL;
}

static void test_teardown(void *test_fixture)
{
	ARG_UNUSED(test_fixture);

	net_pkt_unref(current_pkt);
	current_pkt = NULL;
}

ZTEST_SUITE(ieee802154_l2_shell, NULL, test_setup, NULL, NULL, test_teardown);
