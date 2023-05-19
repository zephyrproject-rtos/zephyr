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

#include <zephyr/net/ieee802154.h>
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
#define EXPECTED_COORDINATOR_PAN_STR       "43981"

#define EXPECTED_COORDINATOR_ADDR_LE       0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
#define EXPECTED_COORDINATOR_ADDR_BE       0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08
#define EXPECTED_COORDINATOR_ADDR_STR      "0f:0e:0d:0c:0b:0a:09:08"

#define EXPECTED_ENDDEVICE_SHORT_ADDR      0xaaaa

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

static void test_beacon_request(struct ieee802154_mpdu *mpdu)
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

static void test_association_request(struct ieee802154_mpdu *mpdu)
{
	struct ieee802154_command *cmd = mpdu->command;

	zassert_equal(
		mpdu->mhr.fs->fc.frame_version, IEEE802154_VERSION_802154_2006,
		"Association Request: currently only IEEE 802.15.4 2006 frame version supported.");
	zassert_equal(mpdu->mhr.fs->fc.frame_type, IEEE802154_FRAME_TYPE_MAC_COMMAND,
		      "Association Request: should be a MAC command.");
	zassert_equal(mpdu->mhr.fs->fc.ar, true, "Association Request: must request ACK.");
	zassert_equal(mpdu->payload_length, 1U + IEEE802154_CMD_ASSOC_REQ_LENGTH);

	zassert_equal(cmd->cfi, IEEE802154_CFI_ASSOCIATION_REQUEST,
		      "Association Request: unexpected CFI.");
	zassert_equal(cmd->assoc_req.ci.alloc_addr, true,
		      "Association Request: should allocate short address.");
	zassert_equal(cmd->assoc_req.ci.association_type, false,
		      "Association Request: fast association is not supported.");
}

static void test_scan_shell_cmd(void)
{
	struct ieee802154_mpdu mpdu = {0};
	int ret;

	/* The beacon placed into the RX queue will be received and handled as
	 * soon as this command yields waiting for beacons.
	 */
	ret = shell_execute_cmd(NULL, "ieee802154 scan active 11 500");
	zassert_equal(0, ret, "Active scan failed: %d", ret);

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

static void test_associate_shell_cmd(struct ieee802154_context *ctx)
{
	uint8_t expected_coord_addr_le[] = {EXPECTED_COORDINATOR_ADDR_LE};
	struct ieee802154_mpdu mpdu = {0};
	struct net_buf *assoc_req;
	int ret;

	/* The association response placed into the RX queue will be received and
	 * handled as soon as this command yields waiting for a response.
	 */
	ret = shell_execute_cmd(NULL, "ieee802154 associate " EXPECTED_COORDINATOR_PAN_STR
				      " " EXPECTED_COORDINATOR_ADDR_STR);
	zassert_equal(0, ret, "Association failed: %d", ret);

	/* Test that we were associated. */
	zassert_equal(ctx->pan_id, EXPECTED_COORDINATOR_PAN_CPU_ORDER,
		      "Association: did not get associated to the expected PAN.");
	zassert_equal(ctx->short_addr, EXPECTED_ENDDEVICE_SHORT_ADDR,
		      "Association: did not get the expected short address asigned.");
	zassert_equal(ctx->coord_short_addr, IEEE802154_NO_SHORT_ADDRESS_ASSIGNED,
		      "Association: co-ordinator should not use short address.");
	zassert_mem_equal(
		ctx->coord_ext_addr, expected_coord_addr_le, sizeof(ctx->coord_ext_addr),
		"Association: did not get associated co-ordinator by the expected coordinator.");

	/* Test the association request that should have been sent out. */
	zassert_not_null(current_pkt);
	assoc_req = current_pkt->frags;
	zassert_not_null(assoc_req);

	if (!ieee802154_validate_frame(assoc_req->data, assoc_req->len, &mpdu)) {
		NET_ERR("*** Could not parse association request.\n");
		ztest_test_fail();
		goto release_frag;
	}

	test_association_request(&mpdu);

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
		goto fail;
	}

	net_pkt_set_ieee802154_lqi(pkt, EXPECTED_COORDINATOR_LQI);
	net_buf_add_mem(pkt->buffer, beacon_pkt, sizeof(beacon_pkt));

	/* The packet will be placed in the RX queue but not yet handled. */
	if (net_recv_data(iface, pkt) < 0) {
		NET_ERR("Recv data failed");
		net_pkt_unref(pkt);
		goto fail;
	}

	net_mgmt_init_event_callback(&scan_cb, scan_result_cb, NET_EVENT_IEEE802154_SCAN_RESULT);
	net_mgmt_add_event_callback(&scan_cb);

	test_scan_shell_cmd();

	net_mgmt_del_event_callback(&scan_cb);
	return;

fail:
	ztest_test_fail();
}

ZTEST(ieee802154_l2_shell, test_associate)
{
	uint8_t coord_addr_le[] = {EXPECTED_COORDINATOR_ADDR_LE};
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	struct ieee802154_frame_params params = {
		.dst = {
			.len = IEEE802154_EXT_ADDR_LENGTH,
			.pan_id = EXPECTED_COORDINATOR_PAN_CPU_ORDER,
		}};
	struct ieee802154_command *cmd;
	struct net_pkt *pkt;

	sys_memcpy_swap(params.dst.ext_addr, ctx->ext_addr, sizeof(params.dst.ext_addr));

	/* Simulate a packet from the coordinator. */
	memcpy(ctx->ext_addr, coord_addr_le, sizeof(ctx->ext_addr));

	pkt = ieee802154_create_mac_cmd_frame(iface, IEEE802154_CFI_ASSOCIATION_RESPONSE, &params);
	if (!pkt) {
		NET_ERR("*** Could not create association response\n");
		goto fail;
	}

	cmd = ieee802154_get_mac_command(pkt);
	cmd->assoc_res.short_addr = sys_cpu_to_le16(EXPECTED_ENDDEVICE_SHORT_ADDR);
	cmd->assoc_res.status = IEEE802154_ASF_SUCCESSFUL;
	ieee802154_mac_cmd_finalize(pkt, IEEE802154_CFI_ASSOCIATION_RESPONSE);

	/* The packet will be placed in the RX queue but not yet handled. */
	if (net_recv_data(iface, pkt) < 0) {
		NET_ERR("Recv assoc resp pkt failed");
		net_pkt_unref(pkt);
		goto fail;
	}

	/* Restore the end device's extended address. */
	sys_memcpy_swap(ctx->ext_addr, params.dst.ext_addr, sizeof(ctx->ext_addr));

	test_associate_shell_cmd(ctx);
	return;

fail:
	sys_memcpy_swap(ctx->ext_addr, params.dst.ext_addr, sizeof(ctx->ext_addr));
	ztest_test_fail();
}

static void reset_fake_driver(void *test_fixture)
{
	struct ieee802154_context *ctx;

	ARG_UNUSED(test_fixture);

	__ASSERT_NO_MSG(iface);

	/* Set initial conditions. */
	ctx = net_if_l2_data(iface);
	ctx->pan_id = IEEE802154_PAN_ID_NOT_ASSOCIATED;
	ctx->short_addr = IEEE802154_SHORT_ADDRESS_NOT_ASSOCIATED;
	ctx->coord_short_addr = IEEE802154_SHORT_ADDRESS_NOT_ASSOCIATED;
	memset(ctx->coord_ext_addr, 0, sizeof(ctx->coord_ext_addr));
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

ZTEST_SUITE(ieee802154_l2_shell, NULL, test_setup, reset_fake_driver, reset_fake_driver,
	    test_teardown);
