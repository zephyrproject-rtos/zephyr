/* main.c - Host long advertising receive */

/*
 * Copyright (c) 2021-2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <string.h>

#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <errno.h>
#include <zephyr/tc_util.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest_assert.h>

#include <fake_hci.h>

#define LOG_LEVEL CONFIG_BT_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(host_test_app);

DEFINE_FFF_GLOBALS;

struct test_adv_report {
	uint8_t data[CONFIG_BT_EXT_SCAN_BUF_SIZE];
	uint8_t length;
	uint16_t evt_prop;
	bt_addr_le_t addr;
};

#define COMPLETE BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_COMPLETE << 5
#define MORE_TO_COME BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL << 5
#define TRUNCATED BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_INCOMPLETE << 5

static void adv_info_create(struct bt_hci_evt_le_ext_advertising_info *evt, uint16_t evt_type,
			    const bt_addr_le_t *const addr, uint8_t length)
{
	evt->evt_type = evt_type;
	bt_addr_le_copy(&evt->addr, addr);
	evt->prim_phy = 0;
	evt->sec_phy = 0;
	evt->sid = 0;
	evt->tx_power = 0;
	evt->rssi = 0;
	evt->interval = 0;
	bt_addr_le_copy(&evt->direct_addr, BT_ADDR_LE_NONE);
	evt->length = length;
}

/* The Controller answers LE Periodic Advertising Create Sync with a Command Status */
static bool cmd_cb(uint16_t opcode, struct net_buf *buf)
{
	ARG_UNUSED(buf);

	if (opcode == BT_HCI_OP_LE_PER_ADV_CREATE_SYNC) {
		fake_hci_cmd_status(opcode, BT_HCI_ERR_SUCCESS);
		return true;
	}

	return false;
}

/* Semaphore to test if the prop callback was called. */
static K_SEM_DEFINE(prop_cb_sem, 0, 1);

/* Send an extended advertising report with the given data. */
static void send_adv_report(const struct test_adv_report *report)
{
	struct bt_hci_evt_le_ext_advertising_info *evt;
	struct net_buf *buf;

	LOG_DBG("Sending adv report");

	buf = fake_hci_le_meta_evt(BT_HCI_EVT_LE_EXT_ADVERTISING_REPORT,
				   1 + sizeof(*evt) + report->length);
	(void)net_buf_add_u8(buf, 1); /* Number of reports */
	evt = net_buf_add(buf, sizeof(*evt));
	adv_info_create(evt, report->evt_prop, &report->addr, report->length);
	(void)net_buf_add_mem(buf, &report->data, report->length);

	fake_hci_recv(buf);
}

FAKE_VALUE_FUNC(struct test_adv_report, get_expected_report);

static void scan_recv_cb(const struct bt_le_scan_recv_info *info, struct net_buf_simple *buf)
{
	ARG_UNUSED(info);

	LOG_DBG("Received event with length %u", buf->len);

	const struct test_adv_report expected = get_expected_report();

	zassert_equal(buf->len, expected.length, "Lengths should be equal");
	zassert_mem_equal(buf->data, expected.data, buf->len, "Data should be equal");
}

static void scan_timeout_cb(void)
{
	zassert_unreachable("Timeout should not happen");
}

#define SYNC_HANDLE 0x0001

/* Send a periodic advertising sync established event for the pending sync. With PAwR sync
 * support, the Host takes only the v2 event, which appends the PAwR parameters to the v1 one.
 */
static void send_per_adv_sync_established(const bt_addr_le_t *addr, uint8_t sid)
{
	const bool is_v2 = IS_ENABLED(CONFIG_BT_PER_ADV_SYNC_RSP);
	struct bt_hci_evt_le_per_adv_sync_established_v2 *evt;
	struct net_buf *buf;
	size_t len;

	len = is_v2 ? sizeof(*evt) : sizeof(struct bt_hci_evt_le_per_adv_sync_established);

	buf = fake_hci_le_meta_evt(is_v2 ? BT_HCI_EVT_LE_PER_ADV_SYNC_ESTABLISHED_V2
					 : BT_HCI_EVT_LE_PER_ADV_SYNC_ESTABLISHED,
				   len);
	evt = net_buf_add(buf, len);
	evt->status = BT_HCI_ERR_SUCCESS;
	evt->handle = sys_cpu_to_le16(SYNC_HANDLE);
	evt->sid = sid;
	bt_addr_le_copy(&evt->adv_addr, addr);
	evt->phy = BT_HCI_LE_PHY_1M;
	evt->interval = sys_cpu_to_le16(0x0100);
	evt->clock_accuracy = 0;

	if (is_v2) {
		evt->num_subevents = 1U;
		evt->subevent_interval = 0U;
		evt->response_slot_delay = 0U;
		evt->response_slot_spacing = 0U;
	}

	fake_hci_recv(buf);
}

/* send_per_adv_report() with the v2 event, the only one the Host takes with PAwR sync support */
static void send_per_adv_report_v2(uint8_t data_status, const uint8_t *data, uint8_t data_len,
				   uint8_t claimed_len)
{
	struct bt_hci_evt_le_per_advertising_report_v2 *evt;
	struct net_buf *buf;

	buf = fake_hci_le_meta_evt(BT_HCI_EVT_LE_PER_ADVERTISING_REPORT_V2,
				   sizeof(*evt) + data_len);
	evt = net_buf_add(buf, sizeof(*evt));
	evt->handle = sys_cpu_to_le16(SYNC_HANDLE);
	evt->tx_power = 0;
	evt->rssi = 0;
	evt->cte_type = BT_HCI_LE_NO_CTE;
	evt->periodic_event_counter = 0U;
	evt->subevent = 0U;
	evt->data_status = data_status;
	evt->length = claimed_len;
	(void)net_buf_add_mem(buf, data, data_len);

	fake_hci_recv(buf);
}

/* Send a periodic advertising report carrying data_len bytes of data, whose length
 * field claims claimed_len bytes.
 */
static void send_per_adv_report(uint8_t data_status, const uint8_t *data, uint8_t data_len,
				uint8_t claimed_len)
{
	struct bt_hci_evt_le_per_advertising_report *evt;
	struct net_buf *buf;

	if (IS_ENABLED(CONFIG_BT_PER_ADV_SYNC_RSP)) {
		send_per_adv_report_v2(data_status, data, data_len, claimed_len);
		return;
	}

	buf = fake_hci_le_meta_evt(BT_HCI_EVT_LE_PER_ADVERTISING_REPORT, sizeof(*evt) + data_len);
	evt = net_buf_add(buf, sizeof(*evt));
	evt->handle = sys_cpu_to_le16(SYNC_HANDLE);
	evt->tx_power = 0;
	evt->rssi = 0;
	evt->cte_type = BT_HCI_LE_NO_CTE;
	evt->data_status = data_status;
	evt->length = claimed_len;
	net_buf_add_mem(buf, data, data_len);

	fake_hci_recv(buf);
}

static K_SEM_DEFINE(per_adv_synced_sem, 0, 1);

FAKE_VOID_FUNC(subevent_rx_failed);

static void per_adv_sync_synced_cb(struct bt_le_per_adv_sync *sync,
				   struct bt_le_per_adv_sync_synced_info *info)
{
	k_sem_give(&per_adv_synced_sem);
}

static void per_adv_sync_term_cb(struct bt_le_per_adv_sync *sync,
				 const struct bt_le_per_adv_sync_term_info *info)
{
	zassert_equal(info->reason, BT_HCI_ERR_LOCALHOST_TERM_CONN,
		      "Sync should only be deleted by the test (reason 0x%02x)", info->reason);
}

static void per_adv_sync_recv_cb(struct bt_le_per_adv_sync *sync,
				 const struct bt_le_per_adv_sync_recv_info *info,
				 struct net_buf_simple *buf)
{
	struct test_adv_report expected;

	if (buf == NULL) {
		subevent_rx_failed();
		return;
	}

	expected = get_expected_report();

	LOG_DBG("Received periodic adv report with length %u", buf->len);

	zassert_equal(buf->len, expected.length, "Lengths should be equal");
	zassert_mem_equal(buf->data, expected.data, buf->len, "Data should be equal");
}

static void generate_sequence(uint8_t *dest, uint16_t len, uint8_t range_start, uint8_t range_end)
{
	uint16_t written = 0;
	uint8_t value = range_start;

	while (written < len) {
		*dest++ = value++;
		written++;
		if (value > range_end) {
			value = range_start;
		}
	}
}

static void *suite_setup(void)
{
	static struct bt_le_per_adv_sync_cb sync_callbacks = {
		.synced = per_adv_sync_synced_cb,
		.term = per_adv_sync_term_cb,
		.recv = per_adv_sync_recv_cb,
	};
	static const struct fake_hci_cb cb = {
		.cmd = cmd_cb,
	};
	int err;

	/* Bogus Controller features */
	(void)memset(fake_hci.features, 0xFF, sizeof(fake_hci.features));
	(void)memset(fake_hci.le_features, 0xFF, sizeof(fake_hci.le_features));
	fake_hci_cb_set(&cb);

	zassert_true((bt_enable(NULL) == 0), "bt_enable failed");

	err = bt_le_per_adv_sync_cb_register(&sync_callbacks);
	zassert_equal(err, 0, "bt_le_per_adv_sync_cb_register failed (err %d)", err);

	return NULL;
}

static struct bt_le_per_adv_sync *per_adv_sync;

/* Runs after failed tests too, so that the next test gets a fresh sync */
static void test_after(void *fixture)
{
	int err;

	ARG_UNUSED(fixture);

	if (per_adv_sync == NULL) {
		return;
	}

	err = bt_le_per_adv_sync_delete(per_adv_sync);
	per_adv_sync = NULL;
	zassert_equal(err, 0, "bt_le_per_adv_sync_delete failed (err %d)", err);
}

ZTEST_SUITE(long_adv_rx_tests, NULL, suite_setup, NULL, test_after, NULL);

ZTEST(long_adv_rx_tests, test_host_long_adv_recv)
{
	struct test_adv_report expected_reports[2];

	static struct bt_le_scan_cb scan_callbacks = { .recv = scan_recv_cb,
						       .timeout = scan_timeout_cb };
	bt_le_scan_cb_register(&scan_callbacks);
	zassert_true((bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL) == 0), "bt_le_scan_start failed");

	bt_addr_le_t addr_a;
	bt_addr_le_t addr_b;
	bt_addr_le_t addr_c;
	bt_addr_le_t addr_d;

	bt_addr_le_create_static(&addr_a);
	bt_addr_le_create_static(&addr_b);
	bt_addr_le_create_static(&addr_c);
	bt_addr_le_create_static(&addr_d);

	struct test_adv_report report_a_1 = { .length = 30, .evt_prop = MORE_TO_COME };
	struct test_adv_report report_a_2 = { .length = 30, .evt_prop = COMPLETE };

	bt_addr_le_copy(&report_a_1.addr, &addr_a);
	bt_addr_le_copy(&report_a_2.addr, &addr_a);

	struct test_adv_report report_b_1 = { .length = 30, .evt_prop = MORE_TO_COME };
	struct test_adv_report report_b_2 = { .length = 30, .evt_prop = COMPLETE };

	bt_addr_le_copy(&report_b_1.addr, &addr_b);
	bt_addr_le_copy(&report_b_2.addr, &addr_b);

	struct test_adv_report report_c = { .length = 30,
					    .evt_prop = COMPLETE | BT_HCI_LE_ADV_EVT_TYPE_LEGACY };

	bt_addr_le_copy(&report_c.addr, &addr_c);

	struct test_adv_report report_d = { .length = 30, .evt_prop = TRUNCATED };

	bt_addr_le_copy(&report_c.addr, &addr_c);

	struct test_adv_report report_a_combined = { .length = report_a_1.length +
							       report_a_2.length };

	struct test_adv_report report_a_1_repeated = { .length = CONFIG_BT_EXT_SCAN_BUF_SIZE };

	struct test_adv_report report_b_combined = { .length = report_b_1.length +
							       report_b_2.length };

	generate_sequence(report_a_combined.data, report_a_combined.length, 'A', 'Z');
	generate_sequence(report_b_combined.data, report_b_combined.length, 'a', 'z');
	generate_sequence(report_c.data, report_c.length, '0', '9');

	(void)memcpy(report_a_1.data, report_a_combined.data, report_a_1.length);
	(void)memcpy(report_a_2.data, &report_a_combined.data[report_a_1.length],
		     report_a_2.length);

	for (int i = 0; i < report_a_1_repeated.length; i += report_a_1.length) {
		memcpy(&report_a_1_repeated.data[i], report_a_1.data,
		       MIN(report_a_1.length, (report_a_1_repeated.length - i)));
	}

	(void)memcpy(report_b_1.data, report_b_combined.data, report_b_1.length);
	(void)memcpy(report_b_2.data, &report_b_combined.data[report_b_1.length],
		     report_b_2.length);

	/* Check that non-interleaved fragmented adv reports work */
	expected_reports[0] = report_a_combined;
	expected_reports[1] = report_b_combined;
	SET_RETURN_SEQ(get_expected_report, expected_reports, 2);
	send_adv_report(&report_a_1);
	send_adv_report(&report_a_2);
	send_adv_report(&report_b_1);
	send_adv_report(&report_b_2);
	zassert_equal(2, get_expected_report_fake.call_count);
	RESET_FAKE(get_expected_report);
	FFF_RESET_HISTORY();

	/* Check that legacy adv reports interleaved with fragmented adv reports work */
	expected_reports[0] = report_c;
	expected_reports[1] = report_a_combined;
	SET_RETURN_SEQ(get_expected_report, expected_reports, 2);
	send_adv_report(&report_a_1);
	send_adv_report(&report_c); /* Interleaved legacy adv report */
	send_adv_report(&report_a_2);
	zassert_equal(2, get_expected_report_fake.call_count);
	RESET_FAKE(get_expected_report);
	FFF_RESET_HISTORY();

	/* Check that complete adv reports interleaved with fragmented adv reports work */
	expected_reports[0] = report_b_2;
	expected_reports[1] = report_a_combined;
	SET_RETURN_SEQ(get_expected_report, expected_reports, 2);
	send_adv_report(&report_a_1);
	send_adv_report(&report_b_2); /* Interleaved short extended adv report */
	send_adv_report(&report_a_2);
	zassert_equal(2, get_expected_report_fake.call_count);
	RESET_FAKE(get_expected_report);
	FFF_RESET_HISTORY();

	/* Check that fragmented adv reports from one peer are received,
	 * and that interleaved fragmented adv reports from other peers are discarded
	 */
	expected_reports[0] = report_a_combined;
	expected_reports[1] = report_b_2;
	SET_RETURN_SEQ(get_expected_report, expected_reports, 2);
	send_adv_report(&report_a_1);
	send_adv_report(&report_b_1); /* Interleaved fragmented adv report, NOT SUPPORTED */
	send_adv_report(&report_a_2);
	send_adv_report(&report_b_2);
	zassert_equal(2, get_expected_report_fake.call_count);
	RESET_FAKE(get_expected_report);
	FFF_RESET_HISTORY();

	/* Check that host discards the data if the controller keeps sending
	 * incomplete packets.
	 */
	expected_reports[0] = report_b_combined;
	SET_RETURN_SEQ(get_expected_report, expected_reports, 1);
	for (int i = 0; i < (2 + (CONFIG_BT_EXT_SCAN_BUF_SIZE / report_a_1.length)); i++) {
		send_adv_report(&report_a_1);
	}
	send_adv_report(&report_a_2);

	/* Check that controller truncated reports do not generate events */
	send_adv_report(&report_d);

	/* Check that reports from a different advertiser works after truncation */
	send_adv_report(&report_b_1);
	send_adv_report(&report_b_2);
	zassert_equal(1, get_expected_report_fake.call_count);
}

static void per_adv_sync_establish(void)
{
	struct bt_le_per_adv_sync_param param = {
		.sid = 1,
		.timeout = BT_GAP_PER_ADV_MIN_TIMEOUT,
	};
	int err;

	bt_addr_le_create_static(&param.addr);

	err = bt_le_per_adv_sync_create(&param, &per_adv_sync);
	zassert_equal(err, 0, "bt_le_per_adv_sync_create failed (err %d)", err);

	send_per_adv_sync_established(&param.addr, param.sid);
	err = k_sem_take(&per_adv_synced_sem, K_SECONDS(1));
	zassert_equal(err, 0, "Sync not established (err %d)", err);
}

ZTEST(long_adv_rx_tests, test_host_per_adv_report_bad_len)
{
	struct test_adv_report report = { .length = 20 };
	uint8_t fragment[10];

	/* Without a reassembly buffer, the Host drops every report in fragments
	 * and checks no fragment lengths
	 */
	if (CONFIG_BT_PER_ADV_SYNC_BUF_SIZE == 0) {
		ztest_test_skip();
	}

	per_adv_sync_establish();

	RESET_FAKE(get_expected_report);
	FFF_RESET_HISTORY();

	generate_sequence(fragment, sizeof(fragment), 'A', 'Z');
	generate_sequence(report.data, report.length, 'a', 'z');

	/* A partial fragment claiming more data than the event carries. The report is
	 * discarded up to and including the next complete fragment, and the one after
	 * that is delivered intact.
	 */
	SET_RETURN_SEQ(get_expected_report, &report, 1);
	send_per_adv_report(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL, fragment, sizeof(fragment),
			    sizeof(fragment));
	send_per_adv_report(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL, fragment, sizeof(fragment),
			    4 * sizeof(fragment));
	send_per_adv_report(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_COMPLETE, report.data, report.length,
			    report.length);
	send_per_adv_report(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_COMPLETE, report.data, report.length,
			    report.length);
	zassert_equal(1, get_expected_report_fake.call_count);
	RESET_FAKE(get_expected_report);
	FFF_RESET_HISTORY();

	/* Same with the complete fragment of a reassembled report being the bad one */
	SET_RETURN_SEQ(get_expected_report, &report, 1);
	send_per_adv_report(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL, fragment, sizeof(fragment),
			    sizeof(fragment));
	send_per_adv_report(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_COMPLETE, fragment, sizeof(fragment),
			    4 * sizeof(fragment));
	send_per_adv_report(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_COMPLETE, report.data, report.length,
			    report.length);
	send_per_adv_report(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_COMPLETE, report.data, report.length,
			    report.length);
	zassert_equal(1, get_expected_report_fake.call_count);
}

static ZTEST(long_adv_rx_tests, test_host_per_adv_report_incomplete)
{
	struct test_adv_report report = { .length = 20 };
	uint8_t fragment[10];

	per_adv_sync_establish();

	RESET_FAKE(get_expected_report);
	FFF_RESET_HISTORY();

	generate_sequence(fragment, sizeof(fragment), 'A', 'Z');
	generate_sequence(report.data, report.length, 'a', 'z');

	/* A report that the Controller failed to receive in full ends with the
	 * Incomplete data status. It is dropped, and the next report is delivered.
	 */
	SET_RETURN_SEQ(get_expected_report, &report, 1);
	send_per_adv_report(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL, fragment, sizeof(fragment),
			    sizeof(fragment));
	send_per_adv_report(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_INCOMPLETE, fragment, 0U, 0U);
	send_per_adv_report(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_COMPLETE, report.data, report.length,
			    report.length);
	zassert_equal(1, get_expected_report_fake.call_count);
	RESET_FAKE(get_expected_report);
	FFF_RESET_HISTORY();

	/* Same with the Incomplete report as the only one of its report */
	SET_RETURN_SEQ(get_expected_report, &report, 1);
	send_per_adv_report(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_INCOMPLETE, fragment, 0U, 0U);
	send_per_adv_report(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_COMPLETE, report.data, report.length,
			    report.length);
	zassert_equal(1, get_expected_report_fake.call_count);
	RESET_FAKE(get_expected_report);
	FFF_RESET_HISTORY();

	/* Same with a report that the Host already drops, here for a fragment
	 * claiming more data than the event carries
	 */
	SET_RETURN_SEQ(get_expected_report, &report, 1);
	send_per_adv_report(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL, fragment, sizeof(fragment),
			    4 * sizeof(fragment));
	send_per_adv_report(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_INCOMPLETE, fragment, 0U, 0U);
	send_per_adv_report(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_COMPLETE, report.data, report.length,
			    report.length);
	zassert_equal(1, get_expected_report_fake.call_count);
}

static ZTEST(long_adv_rx_tests, test_host_per_adv_report_rx_failed)
{
	struct test_adv_report report = { .length = 20 };

	if (!IS_ENABLED(CONFIG_BT_PER_ADV_SYNC_RSP)) {
		ztest_test_skip();
	}

	per_adv_sync_establish();

	RESET_FAKE(get_expected_report);
	RESET_FAKE(subevent_rx_failed);
	FFF_RESET_HISTORY();

	generate_sequence(report.data, report.length, 'a', 'z');

	/* A subevent that the Controller failed to receive is reported without data, and the
	 * next report is delivered
	 */
	SET_RETURN_SEQ(get_expected_report, &report, 1);
	send_per_adv_report(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_RX_FAILED, report.data, 0U, 0U);
	zassert_equal(1, subevent_rx_failed_fake.call_count);
	send_per_adv_report(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_COMPLETE, report.data, report.length,
			    report.length);
	zassert_equal(1, get_expected_report_fake.call_count);
}
