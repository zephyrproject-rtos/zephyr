/*
 * Copyright (c) 2023-2025 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/clock.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#include "babblekit/testcase.h"
#include "babblekit/flags.h"
#include "bstests.h"
#include "common.h"
#include "iso_tx.h"

#define EXPECTED_TX_CNT 100U

extern enum bst_result_t bst_result;
static struct bt_iso_chan iso_chans[CONFIG_BT_ISO_MAX_CHAN];
static struct bt_iso_chan *default_chan = &iso_chans[0];
static struct bt_iso_cig_param cig_param;
static struct bt_iso_cig *cig;
static const uint32_t latency_ms = 10U;                  /* 10ms */
static const uint32_t interval_us = 10U * USEC_PER_MSEC; /* 10 ms */
static uint8_t disconnect_reason;

BUILD_ASSERT(CONFIG_BT_ISO_MAX_CHAN > 1, "CONFIG_BT_ISO_MAX_CHAN shall be at least 2");

DEFINE_FLAG_STATIC(flag_iso_connected);

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	int err;

	err = bt_le_scan_stop();
	if (err) {
		TEST_FAIL("Failed to stop scanning (err %d)", err);

		return;
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT,
				&default_conn);
	if (err) {
		TEST_FAIL("Failed to create connection (err %d)", err);

		return;
	}
}

static void iso_connected(struct bt_iso_chan *chan)
{
	const struct bt_iso_chan_path hci_path = {
		.pid = BT_ISO_DATA_PATH_HCI,
		.format = BT_HCI_CODING_FORMAT_TRANSPARENT,
	};
	struct bt_iso_info info;
	int err;

	printk("ISO Channel %p connected\n", chan);

	err = bt_iso_setup_data_path(chan, BT_HCI_DATAPATH_DIR_HOST_TO_CTLR, &hci_path);
	TEST_ASSERT(err == 0, "Failed to set ISO data path: %d", err);

	err = bt_iso_chan_get_info(chan, &info);
	TEST_ASSERT(err == 0, "Failed to get CIS info: %d", err);

	TEST_ASSERT(info.type == BT_ISO_CHAN_TYPE_CENTRAL);
	TEST_ASSERT(info.can_send || info.can_recv);
	TEST_ASSERT(IN_RANGE(info.iso_interval, BT_ISO_ISO_INTERVAL_MIN, BT_ISO_ISO_INTERVAL_MAX),
		    "Invalid ISO interval 0x%04x", info.iso_interval);
	TEST_ASSERT(IN_RANGE(info.max_subevent, BT_ISO_NSE_MIN, BT_ISO_NSE_MAX),
		    "Invalid subevent number 0x%02x", info.max_subevent);
	TEST_ASSERT(IN_RANGE(info.unicast.cig_sync_delay, BT_HCI_LE_CIG_SYNC_DELAY_MIN,
			     BT_HCI_LE_CIG_SYNC_DELAY_MAX),
		    "Invalid CIG sync delay 0x%06x", info.unicast.cig_sync_delay);
	TEST_ASSERT(IN_RANGE(info.unicast.cis_sync_delay, BT_HCI_LE_CIS_SYNC_DELAY_MIN,
			     BT_HCI_LE_CIS_SYNC_DELAY_MAX),
		    "Invalid CIS sync delay 0x%06x", info.unicast.cis_sync_delay);

	if (info.can_send) {
		const struct bt_iso_unicast_tx_info *central = &info.unicast.central;

		TEST_ASSERT((BT_GAP_ISO_INTERVAL_TO_US(info.iso_interval) %
			     cig_param.c_to_p_interval) == 0U,
			    "ISO interval %u shall be a multiple of the SDU interval %u",
			    BT_GAP_ISO_INTERVAL_TO_US(info.iso_interval),
			    cig_param.c_to_p_interval);
		TEST_ASSERT(IN_RANGE(central->latency, BT_HCI_LE_TRANSPORT_LATENCY_C_TO_P_MIN,
				     BT_HCI_LE_TRANSPORT_LATENCY_C_TO_P_MAX),
			    "Invalid transport latency 0x%06x", central->latency);
		TEST_ASSERT((central->flush_timeout % info.iso_interval) == 0U,
			    "Flush timeout in %u shall be a multiple of the ISO interval %u",
			    central->flush_timeout, info.iso_interval);
		TEST_ASSERT(IN_RANGE(central->max_pdu, BT_ISO_CONNECTED_PDU_MIN, BT_ISO_PDU_MAX),
			    "Invalid max PDU 0x%04x", central->max_pdu);
		TEST_ASSERT((central->phy == BT_GAP_LE_PHY_1M || central->phy == BT_GAP_LE_PHY_2M ||
			     central->phy == BT_GAP_LE_PHY_CODED),
			    "Invalid PHY 0x%02x", central->phy);
		TEST_ASSERT(IN_RANGE(central->bn, BT_ISO_BN_MIN, BT_ISO_BN_MAX),
			    "Invalid BN 0x%02x", central->bn);
	}
	if (info.can_recv) {
		const struct bt_iso_unicast_tx_info *peripheral = &info.unicast.peripheral;

		TEST_ASSERT((BT_GAP_ISO_INTERVAL_TO_US(info.iso_interval) %
			     cig_param.p_to_c_interval) == 0U,
			    "ISO interval %u shall be a multiple of the SDU interval %u",
			    BT_GAP_ISO_INTERVAL_TO_US(info.iso_interval),
			    cig_param.p_to_c_interval);
		TEST_ASSERT(IN_RANGE(peripheral->latency, BT_HCI_LE_TRANSPORT_LATENCY_P_TO_C_MIN,
				     BT_HCI_LE_TRANSPORT_LATENCY_P_TO_C_MAX),
			    "Invalid transport latency 0x%06x", peripheral->latency);
		TEST_ASSERT((peripheral->flush_timeout % info.iso_interval) == 0U,
			    "Flush timeout in %u shall be a multiple of the ISO interval %u",
			    peripheral->flush_timeout, info.iso_interval);
		TEST_ASSERT(IN_RANGE(peripheral->max_pdu, BT_ISO_CONNECTED_PDU_MIN, BT_ISO_PDU_MAX),
			    "Invalid max PDU 0x%04x", peripheral->max_pdu);
		TEST_ASSERT((peripheral->phy == BT_GAP_LE_PHY_1M ||
			     peripheral->phy == BT_GAP_LE_PHY_2M ||
			     peripheral->phy == BT_GAP_LE_PHY_CODED),
			    "Invalid PHY 0x%02x", peripheral->phy);
		TEST_ASSERT(IN_RANGE(peripheral->bn, BT_ISO_BN_MIN, BT_ISO_BN_MAX),
			    "Invalid BN 0x%02x", peripheral->bn);
	}

	if (chan == default_chan) {
		/* Register for TX to start sending */
		err = iso_tx_register(chan);
		TEST_ASSERT(err == 0, "Failed to register chan for TX: %d", err);

		SET_FLAG(flag_iso_connected);
	}
}

static void iso_disconnected(struct bt_iso_chan *chan, uint8_t reason)
{
	int err;

	printk("ISO Channel %p disconnected (reason 0x%02x)\n", chan, reason);

	if (chan == default_chan) {
		err = iso_tx_unregister(chan);
		TEST_ASSERT(err == 0, "Failed to unregister chan for TX: %d", err);

		disconnect_reason = reason;

		UNSET_FLAG(flag_iso_connected);
	}

	err = bt_iso_remove_data_path(chan, BT_HCI_DATAPATH_DIR_HOST_TO_CTLR);
	TEST_ASSERT(err == 0, "Failed to remove ISO data path: %d", err);
}

static void init(void)
{
	static struct bt_iso_chan_ops iso_ops = {
		.connected = iso_connected,
		.disconnected = iso_disconnected,
		.sent = iso_tx_sent_cb,
	};
	static struct bt_iso_chan_io_qos iso_tx = {
		.sdu = CONFIG_BT_ISO_TX_MTU,
		.phy = BT_GAP_LE_PHY_2M,
		.rtn = 1,
	};
	static struct bt_iso_chan_qos iso_qos = {
		.tx = &iso_tx,
		.rx = NULL,
	};
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		TEST_FAIL("Bluetooth enable failed (err %d)", err);

		return;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(iso_chans); i++) {
		iso_chans[i].ops = &iso_ops;
		iso_chans[i].qos = &iso_qos;
	}

	iso_tx_init();
}

static void set_cig_defaults(void)
{
	cig_param.cis_channels = &default_chan;
	cig_param.num_cis = 1U;
	cig_param.sca = BT_GAP_SCA_UNKNOWN;
	cig_param.packing = BT_ISO_PACKING_SEQUENTIAL;
	cig_param.framing = BT_ISO_FRAMING_UNFRAMED;
	cig_param.c_to_p_latency = latency_ms;   /* ms */
	cig_param.p_to_c_latency = latency_ms;   /* ms */
	cig_param.c_to_p_interval = interval_us; /* us */
	cig_param.p_to_c_interval = interval_us; /* us */
}

static void create_cig(size_t iso_channels)
{
	struct bt_iso_chan *channels[ARRAY_SIZE(iso_chans)];
	int err;

	for (size_t i = 0U; i < iso_channels; i++) {
		channels[i] = &iso_chans[i];
	}

	set_cig_defaults();
	cig_param.num_cis = iso_channels;
	cig_param.cis_channels = channels;

	err = bt_iso_cig_create(&cig_param, &cig);
	if (err != 0) {
		TEST_FAIL("Failed to create CIG (%d)", err);

		return;
	}
}

static int reconfigure_cig_interval(void)
{
	int err;

	/* Test modifying CIG parameter without any CIS */
	cig_param.num_cis = 0U;
	cig_param.c_to_p_interval = 7500; /* us */
	cig_param.p_to_c_interval = cig_param.c_to_p_interval;
	err = bt_iso_cig_reconfigure(cig, &cig_param);
	if (err != 0) {
		TEST_FAIL("Failed to reconfigure CIG to new interval (%d)", err);

		return err;
	}

	err = bt_iso_cig_reconfigure(cig, &cig_param);
	if (err != 0) {
		TEST_FAIL("Failed to reconfigure CIG to same interval (%d)", err);

		return err;
	}

	/* Test modifying to different values for both intervals */
	cig_param.c_to_p_interval = 5000; /* us */
	cig_param.p_to_c_interval = 2500; /* us */
	err = bt_iso_cig_reconfigure(cig, &cig_param);
	if (err != 0) {
		TEST_FAIL("Failed to reconfigure CIG to new interval (%d)", err);

		return err;
	}

	return 0;
}

static int reconfigure_cig_latency(void)
{
	int err;

	/* Test modifying CIG latency without any CIS */
	cig_param.num_cis = 0U;
	cig_param.c_to_p_latency = 20; /* ms */
	cig_param.p_to_c_latency = cig_param.c_to_p_latency;
	err = bt_iso_cig_reconfigure(cig, &cig_param);
	if (err != 0) {
		TEST_FAIL("Failed to reconfigure CIG latency (%d)", err);

		return err;
	}

	cig_param.c_to_p_latency = 30; /* ms */
	cig_param.p_to_c_latency = 40; /* ms */
	err = bt_iso_cig_reconfigure(cig, &cig_param);
	if (err != 0) {
		TEST_FAIL("Failed to reconfigure CIG for different latencies (%d)", err);

		return err;
	}

	return 0;
}

static void reconfigure_cig(void)
{
	struct bt_iso_chan *channels[2];
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(channels); i++) {
		channels[i] = &iso_chans[i];
	}

	set_cig_defaults();

	/* Test modifying existing CIS */
	default_chan->qos->tx->rtn++;

	err = bt_iso_cig_reconfigure(cig, &cig_param);
	if (err != 0) {
		TEST_FAIL("Failed to reconfigure CIS to new RTN (%d)", err);

		return;
	}

	/* Test modifying interval parameter */
	err = reconfigure_cig_interval();
	if (err != 0) {
		return;
	}

	/* Test modifying latency parameter */
	err = reconfigure_cig_latency();
	if (err != 0) {
		return;
	}

	/* Add CIS to the CIG and restore all other parameters */
	set_cig_defaults();
	cig_param.cis_channels = &channels[1];

	err = bt_iso_cig_reconfigure(cig, &cig_param);
	if (err != 0) {
		TEST_FAIL("Failed to reconfigure CIG with new CIS and original parameters (%d)",
			  err);

		return;
	}
}

static void connect_acl(void)
{
	int err;

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err != 0) {
		TEST_FAIL("Scanning failed to start (err %d)", err);

		return;
	}

	WAIT_FOR_FLAG(flag_connected);
}

static void connect_cis(void)
{
	const struct bt_iso_connect_param connect_param = {
		.acl = default_conn,
		.iso_chan = default_chan,
	};
	int err;

	err = bt_iso_chan_connect(&connect_param, 1);
	if (err) {
		TEST_FAIL("Failed to connect ISO (%d)", err);

		return;
	}

	WAIT_FOR_FLAG(flag_iso_connected);
}

static void disconnect_cis(void)
{
	int err;

	err = bt_iso_chan_disconnect(default_chan);
	if (err) {
		TEST_FAIL("Failed to disconnect ISO (err %d)", err);

		return;
	}

	WAIT_FOR_FLAG_UNSET(flag_iso_connected);
}

static void disconnect_acl(void)
{
	int err;

	err = bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err) {
		TEST_FAIL("Failed to disconnect ACL (err %d)", err);

		return;
	}

	WAIT_FOR_FLAG_UNSET(flag_connected);
}

static void terminate_cig(void)
{
	int err;

	err = bt_iso_cig_terminate(cig);
	if (err != 0) {
		TEST_FAIL("Failed to terminate CIG (%d)", err);

		return;
	}

	cig = NULL;
}

static void reset_bluetooth(void)
{
	int err;

	printk("Resetting Bluetooth\n");

	err = bt_disable();
	if (err != 0) {
		TEST_FAIL("Failed to disable (%d)", err);

		return;
	}

	/* After a disable, all CIGs and BIGs are removed */
	cig = NULL;
	disconnect_reason = 0U;

	err = bt_enable(NULL);
	if (err != 0) {
		TEST_FAIL("Failed to re-enable (%d)", err);

		return;
	}
}

static void wait_tx_complete(void)
{
	size_t tx_cnt;

	do {
		tx_cnt = iso_tx_get_sent_cnt(default_chan);
		k_sleep(K_USEC(interval_us));

		if (!IS_FLAG_SET(flag_iso_connected)) {
			/* We don't expect all TX to be complete in the test where the peripheral
			 * actively disconnects
			 */
			if (disconnect_reason != BT_HCI_ERR_REMOTE_USER_TERM_CONN) {
				TEST_FAIL("Did not send expected amount before disconnection");
			}

			return;
		}
	} while (tx_cnt < EXPECTED_TX_CNT);
}

static void test_main(void)
{
	init();
	create_cig(1);
	reconfigure_cig();
	connect_acl();
	connect_cis();
	wait_tx_complete();
	if (IS_FLAG_SET(flag_iso_connected)) {
		disconnect_cis();
	}
	if (IS_FLAG_SET(flag_connected)) {
		disconnect_acl();
	}
	terminate_cig();

	TEST_PASS("Test passed");
}

static void test_main_disable(void)
{
	init();

	/* Setup and connect before disabling */
	create_cig(ARRAY_SIZE(iso_chans));
	connect_acl();
	connect_cis();

	/* Reset BT to see if we can set it up again */
	reset_bluetooth();

	/* Set everything up again to see if everything still works as expected */
	create_cig(ARRAY_SIZE(iso_chans));
	connect_acl();
	connect_cis();
	wait_tx_complete();
	if (IS_FLAG_SET(flag_iso_connected)) {
		disconnect_cis();
	}
	if (IS_FLAG_SET(flag_connected)) {
		disconnect_acl();
	}
	terminate_cig();

	TEST_PASS("Disable test passed");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "central",
		.test_descr = "Central",
		.test_main_f = test_main,
	},
	{
		.test_id = "central_disable",
		.test_descr = "CIS central that tests bt_disable for ISO",
		.test_main_f = test_main_disable,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_main_cis_central_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}
