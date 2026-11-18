/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>

#include "util/util.h"
#include "util/memq.h"
#include "util/dbuf.h"

#include "hal/ccm.h"

#include "pdu_df.h"
#include "lll/pdu_vendor.h"
#include "pdu.h"

#include "lll.h"
#include "lll/lll_df_types.h"
#include "lll_conn.h"
#include "lll_conn_iso.h"

#include "ull_tx_queue.h"

#include "isoal.h"
#include "ull_iso_types.h"
#include "ull_conn_iso_types.h"
#include "ull_conn_types.h"
#include "ull_conn_internal.h"

#include "bs_types.h"
#include "bs_tracing.h"
#include "bstests.h"

#include "babblekit/flags.h"
#include "babblekit/testcase.h"

#define FSU_PHYS          (BT_HCI_LE_FRAME_SPACE_UPDATE_PHY_1M_MASK | \
			   BT_HCI_LE_FRAME_SPACE_UPDATE_PHY_2M_MASK)
#define FSU_SPACING_TYPES (BT_HCI_LE_FRAME_SPACE_UPDATE_SPACING_TYPE_IFS_ACL_CP_MASK | \
			   BT_HCI_LE_FRAME_SPACE_UPDATE_SPACING_TYPE_IFS_ACL_PC_MASK)

/* Smallest frame space supported by the radio, selected by the responder */
#define FSU_RADIO_MIN_US  CONFIG_BT_CTLR_FRAME_SPACE_MIN_US

/* Frame space requested by the peripheral, larger than the default 150 us */
#define FSU_PERIPH_MIN_US 300U
#define FSU_PERIPH_MAX_US 1000U

/* Connection interval after the connection update, in 1.25 ms units */
#define CONN_INTERVAL_UPD 80U

#define WAIT_TIME_S       60U
#define POLL_TIME_MS      10U
#define POLL_TIMEOUT_MS   5000U

/* Write Commands sent back-to-back, so that the connection events have more data and both
 * Central to Peripheral and Peripheral to Central frame spaces are on air.
 */
#define TRAFFIC_TIME_MS   500U
#define TRAFFIC_PERIOD_MS 50U
#define TRAFFIC_BURST     4U
#define TRAFFIC_HANDLE    0x0001U

extern enum bst_result_t bst_result;

static struct bt_conn *default_conn;

DEFINE_FLAG_STATIC(flag_connected);
DEFINE_FLAG_STATIC(flag_disconnected);
DEFINE_FLAG_STATIC(flag_phy_updated);
DEFINE_FLAG_STATIC(flag_param_updated);

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err != 0U) {
		TEST_FAIL("Connection failed (err 0x%02x)", err);
		return;
	}

	if (default_conn == NULL) {
		default_conn = bt_conn_ref(conn);
	}

	TEST_PRINT("Connected");
	SET_FLAG(flag_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	TEST_PRINT("Disconnected (reason 0x%02x)", reason);

	if (default_conn == conn) {
		bt_conn_unref(default_conn);
		default_conn = NULL;
	}

	SET_FLAG(flag_disconnected);
}

static void le_param_updated(struct bt_conn *conn, uint16_t interval, uint16_t latency,
			     uint16_t timeout)
{
	TEST_PRINT("Connection parameters updated: interval %u", interval);

	if (interval == CONN_INTERVAL_UPD) {
		SET_FLAG(flag_param_updated);
	}
}

static void le_phy_updated(struct bt_conn *conn, struct bt_conn_le_phy_info *param)
{
	TEST_PRINT("PHY updated: tx 0x%02x rx 0x%02x", param->tx_phy, param->rx_phy);

	if ((param->tx_phy == BT_GAP_LE_PHY_2M) && (param->rx_phy == BT_GAP_LE_PHY_2M)) {
		SET_FLAG(flag_phy_updated);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.le_param_updated = le_param_updated,
	.le_phy_updated = le_phy_updated,
};

static uint16_t conn_handle_get(struct bt_conn *conn)
{
	uint16_t handle;
	int err;

	err = bt_hci_get_conn_handle(conn, &handle);
	if (err != 0) {
		TEST_FAIL("Failed to get connection handle (err %d)", err);
	}

	return handle;
}

static void frame_space_update(struct bt_conn *conn, uint16_t frame_space_min,
			       uint16_t frame_space_max)
{
	struct bt_hci_cp_le_frame_space_update *cp;
	struct net_buf *buf;
	int err;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_FRAME_SPACE_UPDATE, sizeof(*cp));
	if (buf == NULL) {
		TEST_FAIL("Failed to allocate HCI command");
		return;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(conn_handle_get(conn));
	cp->frame_space_min = sys_cpu_to_le16(frame_space_min);
	cp->frame_space_max = sys_cpu_to_le16(frame_space_max);
	cp->phys = FSU_PHYS;
	cp->spacing_types = sys_cpu_to_le16(FSU_SPACING_TYPES);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_FRAME_SPACE_UPDATE, buf, NULL);
	if (err != 0) {
		TEST_FAIL("LE Frame Space Update failed (err %d)", err);
		return;
	}

	TEST_PRINT("LE Frame Space Update: min %u max %u us", frame_space_min, frame_space_max);
}

/* The Host is not notified of the procedure completion, the LE Frame Space Update Complete
 * event is enabled only with the extended feature set. Poll the frame spaces in use by the
 * Controller instead.
 */
static void frame_space_wait(struct bt_conn *conn, uint16_t frame_space_us)
{
	struct ll_conn *ll_conn;
	uint32_t elapsed_ms;

	ll_conn = ll_connected_get(conn_handle_get(conn));
	if (ll_conn == NULL) {
		TEST_FAIL("Connection context not found");
		return;
	}

	elapsed_ms = 0U;
	while ((ll_conn->lll.tifs_tx_us != frame_space_us) ||
	       (ll_conn->lll.tifs_rx_us != frame_space_us) ||
	       (ll_conn->lll.tifs_hcto_us != frame_space_us)) {
		if (elapsed_ms >= POLL_TIMEOUT_MS) {
			TEST_FAIL("Frame space tx %u rx %u hcto %u us, expected %u us",
				  ll_conn->lll.tifs_tx_us, ll_conn->lll.tifs_rx_us,
				  ll_conn->lll.tifs_hcto_us, frame_space_us);
			return;
		}

		k_sleep(K_MSEC(POLL_TIME_MS));
		elapsed_ms += POLL_TIME_MS;
	}

	TEST_PRINT("Frame space %u us in use", frame_space_us);
}

static void traffic(struct bt_conn *conn)
{
	static const uint8_t data[20];

	for (uint32_t elapsed_ms = 0U; elapsed_ms < TRAFFIC_TIME_MS;
	     elapsed_ms += TRAFFIC_PERIOD_MS) {
		for (uint8_t i = 0U; i < TRAFFIC_BURST; i++) {
			int err;

			/* Peer ignores a Write Command to a non-writable attribute */
			err = bt_gatt_write_without_response(conn, TRAFFIC_HANDLE, data,
							     sizeof(data), false);
			if (err != 0) {
				TEST_FAIL("Write Command failed (err %d)", err);
				return;
			}
		}

		k_sleep(K_MSEC(TRAFFIC_PERIOD_MS));
	}
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	int err;

	if (type != BT_GAP_ADV_TYPE_ADV_IND) {
		return;
	}

	err = bt_le_scan_stop();
	if (err != 0) {
		TEST_FAIL("Stop LE scan failed (err %d)", err);
		return;
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT,
				&default_conn);
	if (err != 0) {
		TEST_FAIL("Create connection failed (err %d)", err);
	}
}

/* Central: requests the lowest frame space, updates to 2M PHY, waits for the frame space
 * requested by the peripheral, updates the connection interval and disconnects.
 */
static void test_central_main(void)
{
	struct bt_le_conn_param param = {
		.interval_min = CONN_INTERVAL_UPD,
		.interval_max = CONN_INTERVAL_UPD,
		.latency = 0U,
		.timeout = 400U,
	};
	int err;

	TEST_START("central");

	err = bt_enable(NULL);
	if (err != 0) {
		TEST_FAIL("Bluetooth init failed (err %d)", err);
		return;
	}

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err != 0) {
		TEST_FAIL("Scanning failed to start (err %d)", err);
		return;
	}

	WAIT_FOR_FLAG(flag_connected);

	/* Any frame space, the peripheral selects the smallest supported by its radio */
	frame_space_update(default_conn, 0U, BT_CONN_LE_FRAME_SPACE_MAX);
	frame_space_wait(default_conn, FSU_RADIO_MIN_US);
	traffic(default_conn);

	/* Frame space on 2M PHY was updated by the same procedure */
	err = bt_conn_le_phy_update(default_conn, BT_CONN_LE_PHY_PARAM_2M);
	if (err != 0) {
		TEST_FAIL("PHY update failed (err %d)", err);
		return;
	}

	WAIT_FOR_FLAG(flag_phy_updated);
	frame_space_wait(default_conn, FSU_RADIO_MIN_US);
	traffic(default_conn);

	/* Peripheral initiated frame space update */
	frame_space_wait(default_conn, FSU_PERIPH_MIN_US);
	traffic(default_conn);

	/* Negotiated frame space is kept across connection update */
	err = bt_conn_le_param_update(default_conn, &param);
	if (err != 0) {
		TEST_FAIL("Connection update failed (err %d)", err);
		return;
	}

	WAIT_FOR_FLAG(flag_param_updated);
	frame_space_wait(default_conn, FSU_PERIPH_MIN_US);
	traffic(default_conn);

	err = bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err != 0) {
		TEST_FAIL("Disconnect failed (err %d)", err);
		return;
	}

	WAIT_FOR_FLAG(flag_disconnected);

	TEST_PASS("central");
}

/* Peripheral: responds to the central request, then requests a larger frame space on 2M
 * PHY.
 */
static void test_peripheral_main(void)
{
	int err;

	TEST_START("peripheral");

	err = bt_enable(NULL);
	if (err != 0) {
		TEST_FAIL("Bluetooth init failed (err %d)", err);
		return;
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, NULL, 0U, NULL, 0U);
	if (err != 0) {
		TEST_FAIL("Advertising failed to start (err %d)", err);
		return;
	}

	WAIT_FOR_FLAG(flag_connected);

	frame_space_wait(default_conn, FSU_RADIO_MIN_US);

	WAIT_FOR_FLAG(flag_phy_updated);
	frame_space_wait(default_conn, FSU_RADIO_MIN_US);

	frame_space_update(default_conn, FSU_PERIPH_MIN_US, FSU_PERIPH_MAX_US);
	frame_space_wait(default_conn, FSU_PERIPH_MIN_US);

	WAIT_FOR_FLAG(flag_param_updated);
	frame_space_wait(default_conn, FSU_PERIPH_MIN_US);

	WAIT_FOR_FLAG(flag_disconnected);

	TEST_PASS("peripheral");
}

static void test_init(void)
{
	bst_ticker_set_next_tick_absolute(WAIT_TIME_S * USEC_PER_SEC);
	bst_result = In_progress;
}

static void test_tick(bs_time_t HW_device_time)
{
	if (bst_result != Passed) {
		TEST_FAIL("Test not passed after %u seconds", WAIT_TIME_S);
	}
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "central",
		.test_descr = "Central initiates Frame Space Update",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_central_main,
	},
	{
		.test_id = "peripheral",
		.test_descr = "Peripheral initiates Frame Space Update",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_peripheral_main,
	},
	BSTEST_END_MARKER,
};

static struct bst_test_list *test_frame_space_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}

bst_test_install_t test_installers[] = {
	test_frame_space_install,
	NULL,
};

int main(void)
{
	bst_main();

	return 0;
}
