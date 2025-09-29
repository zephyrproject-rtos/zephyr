/*
 * Copyright (c) 2023 Nordic Semiconductor
 * Copyright (c) 2025 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Based on the credit_seg_recv test with the following changes:
 *  1. tests seg_send callback
 *  2. tests both small message (len < l2cap tx.mtu) and
 *     large messages (len > l2cap tx.mtu). for the large
 *     messages, only one net_buf is allocated for the message
 *     and it is reused repeatedly in the seg_send callback
 *     until it has been completely sent
 *  3. demonstrates a new pdu length heuristic when the SDU
 *     len is larger than the l2cap tx.mps. when the tx.mps
 *     is the same as the acl mtu, it's sometimes more
 *     efficient to choose a pdu data len that is smaller
 *     than the tx.mps because of the PDU header.
 *  4. doesn't do any ecred testing since it's already tested
 *     by the credit_seg_recv test and the seg_send feature
 *     shouldn't change how ecred works.
 */

#include <stddef.h>

#include <zephyr/types.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/l2cap.h>

#include "babblekit/testcase.h"
#include "babblekit/flags.h"

#define LOG_MODULE_NAME main
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_DBG);

DEFINE_FLAG_STATIC(is_connected);
DEFINE_FLAG_STATIC(flag_l2cap_connected);

#define L2CAP_MPS         CONFIG_BT_L2CAP_TX_MTU
#define NUM_MESSAGES      3
/* For first tx test, we send smaller SDUs than the channel can fit */
#define SMALL_MESSAGE_LEN (2 * L2CAP_MPS)
/* For second tx test, we have a message with more data than the L2CAP_MTU.
 * We test using the seg_send callback to allocate one net_buf for this
 * message but split the message into multiple SDUs.
 */
#define LARGE_MESSAGE_LEN (2 * L2CAP_MTU)

#define L2CAP_MTU         (2 * SMALL_MESSAGE_LEN)

/* Only one message transmitted or received at a time */
NET_BUF_POOL_DEFINE(message_pool, 1, BT_L2CAP_SDU_BUF_SIZE(LARGE_MESSAGE_LEN),
		    CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static uint8_t tx_data[LARGE_MESSAGE_LEN];
static uint16_t rx_cnt;
static size_t rx_offset;
static uint16_t pdu_data_len_preferred;

struct test_ctx {
	struct bt_l2cap_le_chan le_chan;
	size_t tx_left;
	size_t msg_len;
	size_t sdu_len;
	size_t msg_bytes_remaining;
	size_t sdu_bytes_remaining;
	size_t last_segment_size;
	struct net_buf *pdu;
} test_ctx;

static void compute_preferred_pdu_data_len(struct bt_l2cap_le_chan *lechan)
{
	struct bt_le_local_features features;
	int err;

	err = bt_le_get_local_features(&features);
	TEST_ASSERT(!err, "Error calling bt_le_get_local_features (err %d)", err);

	/* A common scenario is when the lechan->tx.mps is the same as the ACL mtu
	 * (for example with data length extension enabled, both can be 251).
	 *
	 * If we simply use the lechan->tx.mps as the PDU data len, once the
	 * 4-byte PDU header is added, then the PDU will become fragmented at
	 * the ACL/link layer and the transmission will be very inefficient.
	 *
	 * For example, when lechan->tx.mps and ACL mtu are both 251, then using
	 * tx.mps for the pdu_len results in a PDU buf length of 255 after the
	 * 4-byte PDU header is added. When the PDU is sent over to the
	 * controller, it becomes two ACL/LL fragments since the ACL mtu
	 * is 251. The first fragment would be 251 bytes and the second
	 * fragment would be 4 bytes.
	 *
	 * Transmission can be much more efficient (over 2x due to LL overhead,
	 * and possibly spilling to next connection interval) if we reduce the
	 * pdu_len to less than the tx.mps.
	 *
	 * A simple choice of pdu_len would be to have the total pdu length with
	 * header be a multiple of the ACL mtu so we avoid the small last fragment.
	 * However, reducing the pdu length from the maximum could cause an extra
	 * ACL/LL fragment to be created, so it's not always better to use max
	 * sized fragments.
	 *
	 * The heuristic we use is to compare the size of the last fragment and if
	 * it is less than half the ACL mtu, then set the pdu size to be a multiple
	 * of the ACL mtu. Otherwise use the full ACL mtu to avoid creating an
	 * extra fragment, which would reduce overall transmit efficiency.
	 *
	 * Note this heuristic method isn't always optimal because it doesn't account
	 * for the sdu size and there are some cases where because of the sdu size,
	 * using only max sized fragments results in no additional fragment, but does
	 * cause one additional PDU to be created. The extra PDU incurs an extra 4-byte
	 * for the PDU header, and the receiving side may have to have an extra
	 * buffer to receive the additional PDU. However this heuristic is simple
	 * and doesn't require* the sdu size, which an optimal algorithm would need.
	 * We can compute the preferred pdu len once, when we know just the acl_mtu
	 * and tx.mps, instead of on every sdu send.
	 */
	size_t acl_mtu = features.acl_mtu;
	size_t max_pdu_with_header_len = lechan->tx.mps + BT_L2CAP_HDR_SIZE;

	LOG_DBG("Set pdu_data_len_preferred to tx.mps %u", lechan->tx.mps);
	pdu_data_len_preferred = lechan->tx.mps;
	if (acl_mtu == 0) {
		LOG_WRN("acl_mtu is 0, not ready for computing pdu_data_len_preferred");
		return;
	}
	if (max_pdu_with_header_len > acl_mtu) {
		size_t last_fragment_size = max_pdu_with_header_len % acl_mtu;

		if (last_fragment_size < (acl_mtu / 2)) {
			/*
			 * The preferred pdu_data_len is one that sends only
			 * full acl fragments without any small last
			 * fragment.
			 *
			 * Then we subtract the l2cap header size since
			 * that will be added when the pdu is created.
			 */
			pdu_data_len_preferred =
				(ROUND_DOWN(max_pdu_with_header_len, acl_mtu) - BT_L2CAP_HDR_SIZE);
			LOG_DBG("Reducing pdu_data_len_preferred to %u", pdu_data_len_preferred);
		}
	}
}

static void recv_cb(struct bt_l2cap_chan *l2cap_chan, size_t sdu_len, off_t seg_offset,
		    struct net_buf_simple *seg)
{
	LOG_DBG("sdu len %u seg offset %u seg len %u", sdu_len, seg_offset, seg->len);

	TEST_ASSERT(sdu_len == test_ctx.sdu_len, "Recv SDU length %u does not match send length %u",
		    sdu_len, test_ctx.sdu_len);

	/* Verify SDU data matches TX'd data. */
	TEST_ASSERT(memcmp(seg->data, &tx_data[rx_offset], seg->len) == 0,
		    "RX data doesn't match TX");
	rx_offset += seg->len;

	if (rx_offset == test_ctx.msg_len) {
		rx_cnt++;
		rx_offset = 0;
	}

	/* Give more credits to complete SDU. */
	bt_l2cap_chan_give_credits(&test_ctx.le_chan.chan, 1);
}

static uint16_t get_pdu_len(struct bt_l2cap_le_chan *lechan, size_t bytes_remaining)
{
	/* If bytes_remaiing fits in a single PDU, use the
	 * full PDU size available as defined by lechan->tx.mps.
	 * Otherwise, use the previously computed pdu_len
	 * that reduces fragmentation without causing additional
	 * PDUs to be created.
	 */
	if (bytes_remaining <= lechan->tx.mps) {
		return bytes_remaining;
	}
	return pdu_data_len_preferred;
}

static struct net_buf *send_cb(struct bt_l2cap_chan *l2cap_chan)
{
	if (test_ctx.last_segment_size > 0) {
		/* we're being called after last segment was sent, update
		 * bytes left to send in the SDU
		 */
		LOG_DBG("done sending %u byte seg", test_ctx.last_segment_size);
		LOG_DBG("sdu_bytes_remaining %u -> %u, msg_bytes_remaining %u -> %u",
			test_ctx.sdu_bytes_remaining,
			test_ctx.sdu_bytes_remaining - test_ctx.last_segment_size,
			test_ctx.msg_bytes_remaining,
			test_ctx.msg_bytes_remaining - test_ctx.last_segment_size);
		test_ctx.sdu_bytes_remaining -= test_ctx.last_segment_size;
		test_ctx.msg_bytes_remaining -= test_ctx.last_segment_size;
		test_ctx.last_segment_size = 0;
	}
	if (test_ctx.sdu_bytes_remaining == 0) {
		struct net_buf *buf = test_ctx.pdu;

		if (test_ctx.msg_bytes_remaining == 0) {
			if (buf) {
				LOG_DBG("unref completed message net_buf %p", buf);
				net_buf_unref(buf);
				test_ctx.pdu = NULL;
				test_ctx.tx_left--;
			}

			if (test_ctx.tx_left == 0) {
				LOG_DBG("tx_left == 0, nothing more to send");
				return NULL;
			}

			/* allocate a new message net buf to send */
			LOG_DBG("Allocating net_buf for new message with data len %u",
				test_ctx.msg_len);
			buf = net_buf_alloc(&message_pool, K_NO_WAIT);
			if (buf == NULL) {
				TEST_FAIL("No more memory");
				return NULL;
			}

			net_buf_reserve(buf, BT_L2CAP_SDU_CHAN_SEND_RESERVE);
			TEST_ASSERT(test_ctx.msg_len <= sizeof(tx_data));
			net_buf_add_mem(buf, tx_data, test_ctx.msg_len);
			test_ctx.msg_bytes_remaining = test_ctx.msg_len;

			test_ctx.pdu = buf;
		} else {
			/* buf->len was set for PDU len earlier, and after
			 * sending should be 0. reset to remaining data len
			 */
			buf->len = test_ctx.msg_bytes_remaining;
		}

		/* prepare SDU */

		/* prepend SDU length */
		size_t sdu_len = MIN(test_ctx.msg_bytes_remaining, test_ctx.sdu_len);

		LOG_DBG("Adding SDU header, sdu_len %u", sdu_len);
		net_buf_push_le16(buf, sdu_len);

		test_ctx.sdu_bytes_remaining = sdu_len + BT_L2CAP_SDU_HDR_SIZE;
		test_ctx.msg_bytes_remaining += BT_L2CAP_SDU_HDR_SIZE;
		LOG_DBG("After SDU header added, buf->len %u", buf->len);
	}

	/* prepare PDU */
	struct net_buf *pdu = test_ctx.pdu;
	struct bt_l2cap_le_chan *chan = CONTAINER_OF(l2cap_chan, struct bt_l2cap_le_chan, chan);

	/* Adjust the buffer so it's one segment/PDU */
	uint16_t pdu_data_len = get_pdu_len(chan, test_ctx.sdu_bytes_remaining);

	/* Change the buf size to the pdu_len. We track
	 * the number of bytes remaining in test_ctx.
	 */
	pdu->len = pdu_data_len;

	/* Increase reference so we can reuse the buf
	 * for next PDU. Stack will decrement reference
	 * once the PDU has been sent.
	 */
	pdu = net_buf_ref(pdu);

	LOG_DBG("sending seg with data len %u", pdu_data_len);

	/* The pdu we're returning will be sent immediately since
	 * we're called only if a credit is available.
	 * TBD: what happens if the pdu we return fails to be sent?
	 */
	test_ctx.last_segment_size = pdu->len;
	return pdu;
}

static void l2cap_chan_connected_cb(struct bt_l2cap_chan *l2cap_chan)
{
	struct bt_l2cap_le_chan *chan = CONTAINER_OF(l2cap_chan, struct bt_l2cap_le_chan, chan);

	/* TODO: check that actual MPS < expected MPS */

	SET_FLAG(flag_l2cap_connected);
	LOG_DBG("%x (tx mtu %d mps %d) (rx mtu %d mps %d)", l2cap_chan, chan->tx.mtu, chan->tx.mps,
		chan->rx.mtu, chan->rx.mps);

	compute_preferred_pdu_data_len(chan);
}

static void l2cap_chan_disconnected_cb(struct bt_l2cap_chan *chan)
{
	UNSET_FLAG(flag_l2cap_connected);
	LOG_DBG("%p", chan);
}

static struct bt_l2cap_chan_ops ops = {
	.connected = l2cap_chan_connected_cb,
	.disconnected = l2cap_chan_disconnected_cb,
	.seg_recv = recv_cb,
	.seg_send = send_cb,
};

static int server_accept_cb(struct bt_conn *conn, struct bt_l2cap_server *server,
			    struct bt_l2cap_chan **chan)
{
	struct bt_l2cap_le_chan *le_chan = &test_ctx.le_chan;

	*chan = &le_chan->chan;

	/* Always default initialize the chan. */
	memset(le_chan, 0, sizeof(*le_chan));
	le_chan->chan.ops = &ops;

	le_chan->rx.mtu = L2CAP_MTU;
	le_chan->rx.mps = BT_L2CAP_RX_MTU;

	/* Credits can be given before returning from this
	 * accept-handler and after the 'connected' event. Credits given
	 * before completing the accept are sent in the 'initial
	 * credits' field of the connection response PDU.
	 */
	bt_l2cap_chan_give_credits(*chan, 1);

	return 0;
}

static struct bt_l2cap_server test_l2cap_server = {.accept = server_accept_cb};

static int l2cap_server_register(bt_security_t sec_level)
{
	int err;

	test_l2cap_server.psm = 0;
	test_l2cap_server.sec_level = sec_level;

	err = bt_l2cap_server_register(&test_l2cap_server);

	TEST_ASSERT(err == 0, "Failed to register l2cap server.");

	return test_l2cap_server.psm;
}

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		TEST_FAIL("Failed to connect to %s (%u)", addr, conn_err);
		return;
	}

	LOG_DBG("%s", addr);

	SET_FLAG(is_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_DBG("%p %s (reason 0x%02x)", conn, addr, reason);

	UNSET_FLAG(is_connected);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void disconnect_device(struct bt_conn *conn, void *data)
{
	int err;

	SET_FLAG(is_connected);

	err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	TEST_ASSERT(!err, "Failed to initate disconnect (err %d)", err);

	LOG_DBG("Waiting for disconnection...");
	WAIT_FOR_FLAG_UNSET(is_connected);
}

static void test_peripheral_main(void)
{
	int err;
	int psm;

	LOG_DBG("*L2CAP CREDITS Peripheral started*");

	/* Prepare tx_data */
	for (size_t i = 0; i < sizeof(tx_data); i++) {
		tx_data[i] = (uint8_t)i;
	}

	err = bt_enable(NULL);
	if (err) {
		TEST_FAIL("Can't enable Bluetooth (err %d)", err);
		return;
	}

	LOG_DBG("Bluetooth initialized.");
	LOG_DBG("Connectable advertising...");
	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, NULL, 0, NULL, 0);
	if (err) {
		TEST_FAIL("Advertising failed to start (err %d)", err);
		return;
	}

	LOG_DBG("Advertising started.");
	LOG_DBG("Waiting for connection...");
	WAIT_FOR_FLAG(is_connected);
	LOG_DBG("Connected.");

	/* Set to verify received messages */
	test_ctx.msg_len = SMALL_MESSAGE_LEN;
	test_ctx.sdu_len = SMALL_MESSAGE_LEN;

	psm = l2cap_server_register(BT_SECURITY_L1);

	LOG_DBG("Registered server PSM %x", psm);

	LOG_DBG("Waiting for transfer completion of %u messages of %u bytes each", NUM_MESSAGES,
		test_ctx.msg_len);
	while (rx_cnt < NUM_MESSAGES) {
		/* Sleep enough so the peer has time to attempt sending another
		 * message. If it still has credits, it's in its right to do so. If
		 * it does so before we release the ref below, then allocation
		 * will fail and the channel will be disconnected.
		 */
		k_msleep(100);
	}
	LOG_INF("Total messages of len %u received: %d", test_ctx.msg_len, rx_cnt);

	/* Set to verify received messages */
	rx_cnt = 0;
	rx_offset = 0;
	test_ctx.msg_len = LARGE_MESSAGE_LEN;
	test_ctx.sdu_len = L2CAP_MTU;

	LOG_DBG("Waiting for transfer completion of %u messages of %u bytes each", NUM_MESSAGES,
		test_ctx.msg_len);
	while (rx_cnt < NUM_MESSAGES) {
		/* Sleep enough so the peer has time to attempt sending another
		 * message. If it still has credits, it's in its right to do so. If
		 * it does so before we release the ref below, then allocation
		 * will fail and the channel will be disconnected.
		 */
		k_sleep(K_SECONDS(5));
	}

	bt_conn_foreach(BT_CONN_TYPE_LE, disconnect_device, NULL);
	LOG_INF("Total messages of len %u received: %d", test_ctx.msg_len, rx_cnt);

	TEST_PASS("L2CAP CREDITS Peripheral passed");
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char str[BT_ADDR_LE_STR_LEN];
	int err;
	struct bt_conn *conn;
	struct bt_le_conn_param *param;

	err = bt_le_scan_stop();
	if (err) {
		TEST_FAIL("Stop LE scan failed (err %d)", err);
		return;
	}

	bt_addr_le_to_str(addr, str, sizeof(str));

	LOG_DBG("Connecting to %s", str);

	param = BT_LE_CONN_PARAM_DEFAULT;
	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, param, &conn);
	if (err) {
		TEST_FAIL("Create conn failed (err %d)", err);
		return;
	}
}

static void connect_peripheral(void)
{
	struct bt_le_scan_param scan_param = {
		.type = BT_LE_SCAN_TYPE_ACTIVE,
		.options = BT_LE_SCAN_OPT_NONE,
		.interval = BT_GAP_SCAN_FAST_INTERVAL,
		.window = BT_GAP_SCAN_FAST_WINDOW,
	};
	int err;

	UNSET_FLAG(is_connected);

	err = bt_le_scan_start(&scan_param, device_found);

	TEST_ASSERT(!err, "Scanning failed to start (err %d)", err);

	LOG_DBG("Central initiating connection...");
	WAIT_FOR_FLAG(is_connected);
}

static void connect_l2cap_channel(struct bt_conn *conn, void *data)
{
	int err;
	struct bt_l2cap_le_chan *le_chan = &test_ctx.le_chan;

	*le_chan = (struct bt_l2cap_le_chan){
		.chan.ops = &ops,
		.rx.mtu = L2CAP_MTU,
		.rx.mps = BT_L2CAP_RX_MTU,
	};

	UNSET_FLAG(flag_l2cap_connected);

	/* Credits can be given before requesting the connection and
	 * after the 'connected' event. Credits given before connecting
	 * are sent in the 'initial credits' field of the connection
	 * request PDU.
	 */
	bt_l2cap_chan_give_credits(&le_chan->chan, 1);

	err = bt_l2cap_chan_connect(conn, &le_chan->chan, 0x0080);
	TEST_ASSERT(!err, "Error connecting l2cap channel (err %d)", err);

	WAIT_FOR_FLAG(flag_l2cap_connected);
}

static void connect_l2cap_ecred_channel(struct bt_conn *conn, void *data)
{
	int err;
	struct bt_l2cap_le_chan *le_chan = &test_ctx.le_chan;
	struct bt_l2cap_chan *chan_list[2] = {&le_chan->chan, 0};

	*le_chan = (struct bt_l2cap_le_chan){
		.chan.ops = &ops,
		.rx.mtu = L2CAP_MTU,
		.rx.mps = BT_L2CAP_RX_MTU,
	};

	UNSET_FLAG(flag_l2cap_connected);

	/* Credits can be given before requesting the connection and
	 * after the 'connected' event. Credits given before connecting
	 * are sent in the 'initial credits' field of the connection
	 * request PDU.
	 */
	bt_l2cap_chan_give_credits(&le_chan->chan, 1);

	err = bt_l2cap_ecred_chan_connect(conn, chan_list, 0x0080);
	TEST_ASSERT(!err, "Error connecting l2cap channel (err %d)", err);

	WAIT_FOR_FLAG(flag_l2cap_connected);
}

static void test_central_main(void)
{
	int err;

	LOG_DBG("*L2CAP CREDITS Central started*");

	/* Prepare tx_data */
	for (size_t i = 0; i < sizeof(tx_data); i++) {
		tx_data[i] = (uint8_t)i;
	}

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Can't enable Bluetooth (err %d)", err);
	LOG_DBG("Central Bluetooth initialized.");

	connect_peripheral();

	/* Connect L2CAP channels */
	LOG_DBG("Connect L2CAP channels");
	if (IS_ENABLED(CONFIG_BT_L2CAP_ECRED)) {
		bt_conn_foreach(BT_CONN_TYPE_LE, connect_l2cap_ecred_channel, NULL);
	} else {
		bt_conn_foreach(BT_CONN_TYPE_LE, connect_l2cap_channel, NULL);
	}

	/* Send NUM_MESSAGES to each peripheral. Each SDU is of size SMALL_MESSAGE_LEN */
	test_ctx.tx_left = NUM_MESSAGES;
	test_ctx.msg_len = SMALL_MESSAGE_LEN;
	test_ctx.sdu_len = SMALL_MESSAGE_LEN;
	LOG_DBG("Initiating sending %u messages of len %u over l2cap", NUM_MESSAGES,
		SMALL_MESSAGE_LEN);
	err = bt_l2cap_chan_send_ready(&test_ctx.le_chan.chan);
	TEST_ASSERT(err >= 0, "Failed to initiate send: err %d", err);

	LOG_DBG("Wait until all transfers are completed.");
	while (test_ctx.tx_left) {
		k_msleep(100);
	}

	/* Send NUM_MESSAGES to each peripheral. Each message is of size LARGE_MESSAGE_LEN */
	k_sleep(K_SECONDS(1));
	test_ctx.tx_left = NUM_MESSAGES;
	test_ctx.msg_len = LARGE_MESSAGE_LEN;
	test_ctx.sdu_len = L2CAP_MTU; /* SDU len set to maximum L2CAP_MTU */
	LOG_DBG("Initiating sending %u messages of len %U over l2cap", NUM_MESSAGES,
		LARGE_MESSAGE_LEN);
	err = bt_l2cap_chan_send_ready(&test_ctx.le_chan.chan);
	TEST_ASSERT(err >= 0, "Failed to initiate send: err %d", err);

	LOG_DBG("Wait until all transfers are completed.");
	while (test_ctx.tx_left) {
		k_msleep(100);
	}

	WAIT_FOR_FLAG_UNSET(is_connected);
	LOG_DBG("Peripheral disconnected.");
	TEST_PASS("L2CAP CREDITS Central passed");
}

static const struct bst_test_instance test_def[] = {
	{.test_id = "peripheral",
	 .test_descr = "Peripheral L2CAP CREDITS",
	 .test_main_f = test_peripheral_main},
	{.test_id = "central",
	 .test_descr = "Central L2CAP CREDITS",
	 .test_main_f = test_central_main},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_main_l2cap_credits_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}

extern struct bst_test_list *test_main_l2cap_credits_install(struct bst_test_list *tests);

bst_test_install_t test_installers[] = {test_main_l2cap_credits_install, NULL};

int main(void)
{
	bst_main();

	return 0;
}
