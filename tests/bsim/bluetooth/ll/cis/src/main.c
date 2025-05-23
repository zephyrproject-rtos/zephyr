/* main.c - Application main entry point */

/*
 * Copyright (c) 2023-2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/gap.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys_clock.h>

#include "bs_types.h"
#include "bs_tracing.h"
#include "time_machine.h"
#include "bstests.h"

#define FAIL(...)					\
	do {						\
		bst_result = Failed;			\
		bs_trace_error_time_line(__VA_ARGS__);	\
	} while (0)

#define PASS(...)					\
	do {						\
		bst_result = Passed;			\
		bs_trace_info_time(1, __VA_ARGS__);	\
	} while (0)

extern enum bst_result_t bst_result;

static struct bt_iso_chan iso_chan[CONFIG_BT_ISO_MAX_CHAN];

static K_SEM_DEFINE(sem_peer_addr, 0, 1);
static K_SEM_DEFINE(sem_peer_conn, 0, 1);
static K_SEM_DEFINE(sem_peer_disc, 0, CONFIG_BT_MAX_CONN);
static K_SEM_DEFINE(sem_iso_conn, 0, 1);
static K_SEM_DEFINE(sem_iso_disc, 0, 1);
static K_SEM_DEFINE(sem_iso_data, CONFIG_BT_ISO_TX_BUF_COUNT,
				   CONFIG_BT_ISO_TX_BUF_COUNT);
static bt_addr_le_t peer_addr;

#define CREATE_CONN_INTERVAL 0x0010
#define CREATE_CONN_WINDOW   0x0010

#define ISO_INTERVAL_US      10000U
#define ISO_LATENCY_MS       DIV_ROUND_UP(ISO_INTERVAL_US, USEC_PER_MSEC)
#define ISO_LATENCY_FT_MS    20U

#if (CONFIG_BT_CTLR_CENTRAL_SPACING == 0)
#define CONN_INTERVAL_MIN_US ISO_INTERVAL_US
#else /* CONFIG_BT_CTLR_CENTRAL_SPACING > 0 */
#define CONN_INTERVAL_MIN_US (ISO_INTERVAL_US * CONFIG_BT_MAX_CONN)
#endif /* CONFIG_BT_CTLR_CENTRAL_SPACING > 0 */
#define CONN_INTERVAL_MAX_US CONN_INTERVAL_MIN_US

#define CONN_INTERVAL_MIN BT_GAP_US_TO_CONN_INTERVAL(CONN_INTERVAL_MIN_US)
#define CONN_INTERVAL_MAX BT_GAP_US_TO_CONN_INTERVAL(CONN_INTERVAL_MAX_US)
#define CONN_TIMEOUT                                                                               \
	MAX(BT_GAP_US_TO_CONN_TIMEOUT(CONN_INTERVAL_MAX_US * 6U), BT_GAP_MS_TO_CONN_TIMEOUT(100U))

#define ADV_INTERVAL_MIN BT_GAP_MS_TO_ADV_INTERVAL(20)
#define ADV_INTERVAL_MAX BT_GAP_MS_TO_ADV_INTERVAL(20)

#define BT_CONN_LE_CREATE_CONN_CUSTOM  \
	BT_CONN_LE_CREATE_PARAM(BT_CONN_LE_OPT_NONE, \
				CREATE_CONN_INTERVAL, \
				CREATE_CONN_WINDOW)

#define BT_LE_CONN_PARAM_CUSTOM \
	BT_LE_CONN_PARAM(CONN_INTERVAL_MIN, CONN_INTERVAL_MAX, 0U, CONN_TIMEOUT)

#if defined(CONFIG_TEST_USE_LEGACY_ADVERTISING)
#define BT_LE_ADV_CONN_CUSTOM                                                                      \
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONN, ADV_INTERVAL_MIN, ADV_INTERVAL_MAX, NULL)
#else /* !CONFIG_TEST_USE_LEGACY_ADVERTISING */
#define BT_LE_ADV_CONN_CUSTOM                                                                      \
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONN | BT_LE_ADV_OPT_EXT_ADV, ADV_INTERVAL_MIN,              \
			ADV_INTERVAL_MAX, NULL)
#endif /* !CONFIG_TEST_USE_LEGACY_ADVERTISING */

#define SEQ_NUM_MAX 1000U

#define NAME_LEN 30

#define BUF_ALLOC_TIMEOUT   (50) /* milliseconds */
NET_BUF_POOL_FIXED_DEFINE(tx_pool, CONFIG_BT_ISO_TX_BUF_COUNT,
			  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static bool data_cb(struct bt_data *data, void *user_data)
{
	char *name = user_data;

	switch (data->type) {
	case BT_DATA_NAME_SHORTENED:
	case BT_DATA_NAME_COMPLETE:
		memcpy(name, data->data, MIN(data->data_len, NAME_LEN - 1));
		return false;
	default:
		return true;
	}
}

static const char *phy2str(uint8_t phy)
{
	switch (phy) {
	case 0: return "No packets";
	case BT_GAP_LE_PHY_1M: return "LE 1M";
	case BT_GAP_LE_PHY_2M: return "LE 2M";
	case BT_GAP_LE_PHY_CODED: return "LE Coded";
	default: return "Unknown";
	}
}

static void scan_recv(const struct bt_le_scan_recv_info *info,
		      struct net_buf_simple *buf)
{
	char le_addr[BT_ADDR_LE_STR_LEN];
	char name[NAME_LEN];

	(void)memset(name, 0, sizeof(name));

	bt_data_parse(buf, data_cb, name);

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
	printk("[DEVICE]: %s, AD evt type %u, Tx Pwr: %i, RSSI %i %s "
	       "C:%u S:%u D:%u SR:%u E:%u Prim: %s, Secn: %s, "
	       "Interval: 0x%04x (%u ms), SID: %u\n",
	       le_addr, info->adv_type, info->tx_power, info->rssi, name,
	       (info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE) != 0,
	       (info->adv_props & BT_GAP_ADV_PROP_SCANNABLE) != 0,
	       (info->adv_props & BT_GAP_ADV_PROP_DIRECTED) != 0,
	       (info->adv_props & BT_GAP_ADV_PROP_SCAN_RESPONSE) != 0,
	       (info->adv_props & BT_GAP_ADV_PROP_EXT_ADV) != 0,
	       phy2str(info->primary_phy), phy2str(info->secondary_phy),
	       info->interval, info->interval * 5 / 4, info->sid);

	bt_addr_le_copy(&peer_addr, info->addr);
	k_sem_give(&sem_peer_addr);
}

static struct bt_le_scan_cb scan_callbacks = {
	.recv = scan_recv,
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		struct bt_conn_info conn_info;

		printk("Failed to connect to %s (%u)\n", addr, err);

		err = bt_conn_get_info(conn, &conn_info);
		if (err) {
			FAIL("Failed to get connection info (%d).\n", err);
			return;
		}

		printk("%s: %s role %u\n", __func__, addr, conn_info.role);

		if (conn_info.role == BT_CONN_ROLE_CENTRAL) {
			bt_conn_unref(conn);
		}

		return;
	}

	printk("Connected: %s\n", addr);

	k_sem_give(&sem_peer_conn);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct bt_conn_info conn_info;
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s (reason 0x%02x)\n", addr, reason);

	err = bt_conn_get_info(conn, &conn_info);
	if (err) {
		FAIL("Failed to get connection info (%d).\n", err);
		return;
	}

	printk("%s: %s role %u\n", __func__, addr, conn_info.role);

	if (conn_info.role == BT_CONN_ROLE_CENTRAL) {
		bt_conn_unref(conn);
	}

	k_sem_give(&sem_peer_disc);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void disconnect(struct bt_conn *conn, void *data)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnecting %s...\n", addr);
	err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err) {
		FAIL("Failed disconnection %s.\n", addr);
		return;
	}
	printk("success.\n");
}

/** Print data as d_0 d_1 d_2 ... d_(n-2) d_(n-1) d_(n) to show the 3 first and 3 last octets
 *
 * Examples:
 * 01
 * 0102
 * 010203
 * 01020304
 * 0102030405
 * 010203040506
 * 010203...050607
 * 010203...060708
 * etc.
 */
static void iso_print_data(uint8_t *data, size_t data_len)
{
	/* Maximum number of octets from each end of the data */
	const uint8_t max_octets = 3;
	char data_str[35];
	size_t str_len;

	str_len = bin2hex(data, MIN(max_octets, data_len), data_str, sizeof(data_str));
	if (data_len > max_octets) {
		if (data_len > (max_octets * 2)) {
			static const char dots[] = "...";

			strcat(&data_str[str_len], dots);
			str_len += strlen(dots);
		}

		str_len += bin2hex(data + (data_len - MIN(max_octets, data_len - max_octets)),
				   MIN(max_octets, data_len - max_octets),
				   data_str + str_len,
				   sizeof(data_str) - str_len);
	}

	printk("\t %s\n", data_str);
}

static uint16_t expected_seq_num[CONFIG_BT_ISO_MAX_CHAN];

static void iso_recv(struct bt_iso_chan *chan, const struct bt_iso_recv_info *info,
		struct net_buf *buf)
{
	uint16_t seq_num;
	uint8_t index;

	index = bt_conn_index(chan->iso);

	printk("Incoming data channel %p (%u) flags 0x%x seq_num %u ts %u len %u:\n",
	       chan, index, info->flags, info->seq_num, info->ts, buf->len);
	iso_print_data(buf->data, buf->len);

	seq_num = sys_get_le32(buf->data);
	if (info->flags & BT_ISO_FLAGS_VALID) {
		if (seq_num != expected_seq_num[index]) {
			if (expected_seq_num[index]) {
				FAIL("ISO data miss match, expected %u actual %u\n",
				     expected_seq_num[index], seq_num);
			}
			expected_seq_num[index] = seq_num;
		}

		expected_seq_num[index] += 1U;

#if defined(CONFIG_TEST_FT_PER_SKIP_SUBEVENTS)
		expected_seq_num[index] += ((CONFIG_TEST_FT_PER_SKIP_EVENTS_COUNT - 1U) * 2U);
#elif defined(CONFIG_TEST_FT_CEN_SKIP_SUBEVENTS)
		expected_seq_num[index] += ((CONFIG_TEST_FT_CEN_SKIP_EVENTS_COUNT - 1U) * 2U);
#endif
	} else if (expected_seq_num[index] &&
		   expected_seq_num[index] < SEQ_NUM_MAX) {
		FAIL("%s: Invalid ISO data after valid ISO data reception.\n"
		     "Expected %u\n", __func__, expected_seq_num[index]);
	}
}

void iso_sent(struct bt_iso_chan *chan)
{
	k_sem_give(&sem_iso_data);
}

static void iso_connected(struct bt_iso_chan *chan)
{
	const struct bt_iso_chan_path hci_path = {
		.pid = BT_ISO_DATA_PATH_HCI,
		.format = BT_HCI_CODING_FORMAT_TRANSPARENT,
	};
	struct bt_iso_info iso_info;
	int err;

	err = bt_iso_chan_get_info(chan, &iso_info);
	if (err != 0) {
		FAIL("Failed to get ISO info: %d\n", err);
		return;
	}

	printk("ISO Channel %p connected\n", chan);

	k_sem_give(&sem_iso_conn);

	if (iso_info.can_recv) {
		err = bt_iso_setup_data_path(chan, BT_HCI_DATAPATH_DIR_CTLR_TO_HOST, &hci_path);
		if (err != 0) {
			FAIL("Failed to setup ISO RX data path: %d\n", err);
		}
	}

	if (iso_info.can_send) {
		err = bt_iso_setup_data_path(chan, BT_HCI_DATAPATH_DIR_HOST_TO_CTLR, &hci_path);
		if (err != 0) {
			FAIL("Failed to setup ISO TX data path: %d\n", err);
		}
	}
}

static void iso_disconnected(struct bt_iso_chan *chan, uint8_t reason)
{
	struct bt_iso_info iso_info;
	int err;

	printk("ISO Channel %p disconnected (reason 0x%02x)\n", chan, reason);

	k_sem_give(&sem_iso_disc);

	err = bt_iso_chan_get_info(chan, &iso_info);
	if (err != 0) {
		FAIL("Failed to get ISO info: %d\n", err);
	} else if (iso_info.type == BT_ISO_CHAN_TYPE_CENTRAL) {
		if (iso_info.can_recv) {
			err = bt_iso_remove_data_path(chan, BT_HCI_DATAPATH_DIR_CTLR_TO_HOST);
			if (err != 0) {
				FAIL("Failed to remove ISO RX data path: %d\n", err);
			}
		}

		if (iso_info.can_send) {
			err = bt_iso_remove_data_path(chan, BT_HCI_DATAPATH_DIR_HOST_TO_CTLR);
			if (err != 0) {
				FAIL("Failed to remove ISO TX data path: %d\n", err);
			}
		}
	}
}

static struct bt_iso_chan_ops iso_ops = {
	.connected    = iso_connected,
	.disconnected = iso_disconnected,
	.recv         = iso_recv,
	.sent         = iso_sent,
};

static void test_cis_central(void)
{
	struct bt_iso_chan_io_qos iso_tx[CONFIG_BT_ISO_MAX_CHAN];
	struct bt_iso_chan_io_qos iso_rx[CONFIG_BT_ISO_MAX_CHAN];
	struct bt_iso_chan_qos iso_qos[CONFIG_BT_ISO_MAX_CHAN];
	struct bt_iso_chan *channels[CONFIG_BT_ISO_MAX_CHAN];
	struct bt_conn *conn_list[CONFIG_BT_MAX_CONN];
	struct bt_iso_cig_param cig_param;
	struct bt_iso_cig *cig;
	int conn_count;
	int err;

	printk("Bluetooth initializing...");
	err = bt_enable(NULL);
	if (err) {
		FAIL("Could not init BT: %d\n", err);
		return;
	}
	printk("success.\n");

	printk("Scan callbacks register...");
	bt_le_scan_cb_register(&scan_callbacks);
	printk("success.\n");

	for (int i = 0; i < CONFIG_BT_ISO_MAX_CHAN; i++) {
		iso_tx[i].sdu = CONFIG_BT_ISO_TX_MTU;
		iso_tx[i].phy = BT_GAP_LE_PHY_2M;
		if (IS_ENABLED(CONFIG_TEST_FT_SKIP_SUBEVENTS)) {
			iso_tx[i].rtn = 2U;
		} else {
			iso_tx[i].rtn = 0U;
		}

		if (!IS_ENABLED(CONFIG_TEST_FT_SKIP_SUBEVENTS) ||
		    IS_ENABLED(CONFIG_TEST_FT_PER_SKIP_SUBEVENTS)) {
			iso_qos[i].tx = &iso_tx[i];
		} else {
			iso_qos[i].tx = NULL;
		}

		iso_rx[i].sdu = CONFIG_BT_ISO_RX_MTU;
		iso_rx[i].phy = BT_GAP_LE_PHY_2M;
		if (IS_ENABLED(CONFIG_TEST_FT_SKIP_SUBEVENTS)) {
			iso_rx[i].rtn = 2U;
		} else {
			iso_rx[i].rtn = 0U;
		}

		if (IS_ENABLED(CONFIG_TEST_FT_CEN_SKIP_SUBEVENTS)) {
			iso_qos[i].rx = &iso_rx[i];
		} else {
			iso_qos[i].rx = NULL;
		}

		iso_chan[i].ops = &iso_ops;
		iso_chan[i].qos = &iso_qos[i];
#if defined(CONFIG_BT_SMP)
		iso_chan[i].required_sec_level = BT_SECURITY_L2,
#endif /* CONFIG_BT_SMP */

		channels[i] = &iso_chan[i];
	}

	cig_param.cis_channels = channels;
	cig_param.num_cis = ARRAY_SIZE(channels);
	cig_param.sca = BT_GAP_SCA_UNKNOWN;
	cig_param.packing = 0U;
	cig_param.framing = 0U;
	cig_param.c_to_p_interval = ISO_INTERVAL_US;
	cig_param.p_to_c_interval = ISO_INTERVAL_US;
	if (IS_ENABLED(CONFIG_TEST_FT_SKIP_SUBEVENTS)) {
		cig_param.c_to_p_latency = ISO_LATENCY_FT_MS;
		cig_param.p_to_c_latency = ISO_LATENCY_FT_MS;
	} else {
		cig_param.c_to_p_latency = ISO_LATENCY_MS;
		cig_param.p_to_c_latency = ISO_LATENCY_MS;
	}

	printk("Create CIG...");
	err = bt_iso_cig_create(&cig_param, &cig);
	if (err) {
		FAIL("Failed to create CIG (%d)\n", err);
		return;
	}
	printk("success.\n");

	conn_count = 0;

#if defined(CONFIG_TEST_FT_CEN_SKIP_SUBEVENTS)
	for (int chan = 0; chan < CONFIG_BT_ISO_MAX_CHAN; chan++) {
		expected_seq_num[chan] = (CONFIG_TEST_FT_CEN_SKIP_EVENTS_COUNT - 1U) * 2U;
	}
#endif

#if !defined(CONFIG_TEST_MULTIPLE_PERIPERAL_CIS)
	for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
#else
		int i = 0;
#endif

		struct bt_conn *conn;
		int conn_index;
		int chan;

		printk("Start scanning (%d)...", i);
		err = bt_le_scan_start(BT_LE_SCAN_PASSIVE_CONTINUOUS, NULL);
		if (err) {
			FAIL("Could not start scan: %d\n", err);
			return;
		}
		printk("success.\n");

		printk("Waiting for advertising report...\n");
		err = k_sem_take(&sem_peer_addr, K_FOREVER);
		if (err) {
			FAIL("failed (err %d)\n", err);
			return;
		}
		printk("Found peer advertising.\n");

		printk("Stop scanning... ");
		err = bt_le_scan_stop();
		if (err) {
			FAIL("Could not stop scan: %d\n", err);
			return;
		}
		printk("success.\n");

		printk("Create connection...");
		err = bt_conn_le_create(&peer_addr, BT_CONN_LE_CREATE_CONN_CUSTOM,
					BT_LE_CONN_PARAM_CUSTOM, &conn);
		if (err) {
			FAIL("Create connection failed (0x%x)\n", err);
			return;
		}
		printk("success.\n");

		printk("Waiting for connection %d...", i);
		err = k_sem_take(&sem_peer_conn, K_FOREVER);
		if (err) {
			FAIL("failed (err %d)\n", err);
			return;
		}
		printk("connected to peer device %d.\n", i);

		conn_list[conn_count] = conn;
		conn_index = chan = conn_count;
		conn_count++;

#if defined(CONFIG_TEST_CONNECT_ACL_FIRST)
	}

	for (int chan = 0, conn_index = 0;
	     (conn_index < conn_count) && (chan < CONFIG_BT_ISO_MAX_CHAN);
	     conn_index++, chan++) {

#elif defined(CONFIG_TEST_MULTIPLE_PERIPERAL_CIS)
	for (int chan = 0, conn_index = 0;
	     (chan < CONFIG_BT_ISO_MAX_CHAN); chan++) {
#endif

		struct bt_iso_connect_param iso_connect_param;

		printk("Connect ISO Channel %d...", chan);
		iso_connect_param.acl = conn_list[conn_index];
		iso_connect_param.iso_chan = &iso_chan[chan];
		err = bt_iso_chan_connect(&iso_connect_param, 1);
		if (err) {
			FAIL("Failed to connect iso (%d)\n", err);
			return;
		}

		printk("Waiting for ISO channel connection %d...", chan);
		err = k_sem_take(&sem_iso_conn, K_FOREVER);
		if (err) {
			FAIL("failed (err %d)\n", err);
			return;
		}
		printk("connected to peer %d ISO channel.\n", chan);
	}

	if (!IS_ENABLED(CONFIG_TEST_FT_SKIP_SUBEVENTS) ||
	    IS_ENABLED(CONFIG_TEST_FT_PER_SKIP_SUBEVENTS)) {
		for (uint16_t seq_num = 0U; seq_num < SEQ_NUM_MAX; seq_num++) {

			for (int chan = 0; chan < CONFIG_BT_ISO_MAX_CHAN; chan++) {
				uint8_t iso_data[CONFIG_BT_ISO_TX_MTU] = { 0, };
				struct net_buf *buf;
				int ret;

				buf = net_buf_alloc(&tx_pool, K_MSEC(BUF_ALLOC_TIMEOUT));
				if (!buf) {
					FAIL("Data buffer allocate timeout on channel %d\n", chan);
					return;
				}

				net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
				sys_put_le16(seq_num, iso_data);
				net_buf_add_mem(buf, iso_data, sizeof(iso_data));

				ret = k_sem_take(&sem_iso_data, K_MSEC(BUF_ALLOC_TIMEOUT));
				if (ret) {
					FAIL("k_sem_take for ISO data sent failed.\n");
					return;
				}

				printk("ISO send: seq_num %u, chan %d\n", seq_num, chan);
				ret = bt_iso_chan_send(&iso_chan[chan], buf, seq_num);
				if (ret < 0) {
					FAIL("Unable to send data on channel %d : %d\n", chan, ret);
					net_buf_unref(buf);
					return;
				}
			}

			if ((seq_num % 100) == 0) {
				printk("Sending value %u\n", seq_num);
			}
		}

		k_sleep(K_MSEC(1000));
	} else {
		k_sleep(K_SECONDS(11));
	}

	for (int chan = 0; chan < CONFIG_BT_ISO_MAX_CHAN; chan++) {
		printk("ISO disconnect channel %d...", chan);
		err = bt_iso_chan_disconnect(&iso_chan[chan]);
		if (err) {
			FAIL("Failed to disconnect channel %d (%d)\n", chan, err);
			return;
		}
		printk("success\n");

		printk("Waiting for ISO channel disconnect %d...", chan);
		err = k_sem_take(&sem_iso_disc, K_FOREVER);
		if (err) {
			FAIL("failed (err %d)\n", err);
			return;
		}
		printk("disconnected to peer %d ISO channel.\n", chan);
	}

	bt_conn_foreach(BT_CONN_TYPE_LE, disconnect, NULL);

#if !defined(CONFIG_TEST_MULTIPLE_PERIPERAL_CIS)
	for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
#endif

		printk("Waiting for disconnection %d...", i);
		err = k_sem_take(&sem_peer_disc, K_FOREVER);
		if (err) {
			FAIL("failed (err %d)\n", err);
			return;
		}
		printk("Disconnected from peer device %d.\n", i);

#if !defined(CONFIG_TEST_MULTIPLE_PERIPERAL_CIS)
	}
#endif

	if (IS_ENABLED(CONFIG_TEST_FT_CEN_SKIP_SUBEVENTS)) {
		for (int chan = 0; chan < CONFIG_BT_ISO_MAX_CHAN; chan++) {
			if (expected_seq_num[chan] < SEQ_NUM_MAX) {
				FAIL("ISO Data reception incomplete %u (%u).\n",
				     expected_seq_num[chan], SEQ_NUM_MAX);
				return;
			}
		}
	}

	PASS("Central ISO tests Passed\n");
}

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
};

static struct bt_iso_chan_io_qos iso_rx_p[CONFIG_BT_ISO_MAX_CHAN];
static struct bt_iso_chan_qos iso_qos_p[CONFIG_BT_ISO_MAX_CHAN];
static struct bt_iso_chan iso_chan_p[CONFIG_BT_ISO_MAX_CHAN];
static uint8_t chan_count;

static int iso_accept(const struct bt_iso_accept_info *info,
		      struct bt_iso_chan **chan)
{
	printk("Incoming request from %p\n", (void *)info->acl);

	if ((chan_count >= CONFIG_BT_ISO_MAX_CHAN) ||
	    iso_chan_p[chan_count].iso) {
		FAIL("No channels available\n");
		return -ENOMEM;
	}

	*chan = &iso_chan_p[chan_count];
	chan_count++;

	printk("Accepted on channel %p\n", *chan);

	return 0;
}

static struct bt_iso_server iso_server = {
#if defined(CONFIG_BT_SMP)
	.sec_level = BT_SECURITY_L1,
#endif /* CONFIG_BT_SMP */
	.accept = iso_accept,
};

static void test_cis_peripheral(void)
{
	struct bt_iso_chan_io_qos iso_tx_p[CONFIG_BT_ISO_MAX_CHAN];
	int err;

	printk("Bluetooth initializing...");
	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}
	printk("success.\n");

	for (int i = 0; i < CONFIG_BT_ISO_MAX_CHAN; i++) {
		iso_tx_p[i].sdu = CONFIG_BT_ISO_TX_MTU;
		iso_tx_p[i].phy = BT_GAP_LE_PHY_2M;
		if (IS_ENABLED(CONFIG_TEST_FT_SKIP_SUBEVENTS)) {
			iso_tx_p[i].rtn = 2U;
		} else {
			iso_tx_p[i].rtn = 0U;
		}

		iso_qos_p[i].tx = &iso_tx_p[i];

		iso_rx_p[i].sdu = CONFIG_BT_ISO_RX_MTU;

		iso_qos_p[i].rx = &iso_rx_p[i];

		iso_chan_p[i].ops = &iso_ops;
		iso_chan_p[i].qos = &iso_qos_p[i];
	}

	printk("ISO Server Register...");
	err = bt_iso_server_register(&iso_server);
	if (err) {
		FAIL("Unable to register ISO server (err %d)\n", err);
		return;
	}
	printk("success.\n");

#if defined(CONFIG_TEST_USE_LEGACY_ADVERTISING)
	printk("Start Advertising...");
	err = bt_le_adv_start(BT_LE_ADV_CONN_CUSTOM, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		FAIL("Advertising failed to start (err %d)\n", err);
		return;
	}
	printk("success.\n");

#else /* !CONFIG_TEST_USE_LEGACY_ADVERTISING */
	struct bt_le_ext_adv *adv;

	printk("Creating connectable extended advertising set...\n");
	err = bt_le_ext_adv_create(BT_LE_ADV_CONN_CUSTOM, NULL, &adv);
	if (err) {
		FAIL("Failed to create advertising set (err %d)\n", err);
		return;
	}
	printk("success.\n");

	/* Set extended advertising data */
	err = bt_le_ext_adv_set_data(adv, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		FAIL("Failed to set advertising data (err %d)\n", err);
		return;
	}

	printk("Start Advertising...");
	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err) {
		FAIL("Failed to start extended advertising (err %d)\n", err);
		return;
	}
	printk("success.\n");
#endif /* !CONFIG_TEST_USE_LEGACY_ADVERTISING */

	printk("Waiting for connection from central...\n");
	err = k_sem_take(&sem_peer_conn, K_FOREVER);
	if (err) {
		FAIL("failed (err %d)\n", err);
		return;
	}
	printk("connected to peer central.\n");

#if defined(CONFIG_TEST_MULTIPLE_PERIPERAL_CIS)
	for (int chan = 0; chan < CONFIG_BT_ISO_MAX_CHAN; chan++) {
#endif

		printk("Waiting for ISO channel connection...");
		err = k_sem_take(&sem_iso_conn, K_FOREVER);
		if (err) {
			FAIL("failed (err %d)\n", err);
			return;
		}
		printk("connected to peer ISO channel.\n");

#if defined(CONFIG_TEST_MULTIPLE_PERIPERAL_CIS)
	}
#endif

	if (IS_ENABLED(CONFIG_TEST_FT_CEN_SKIP_SUBEVENTS)) {
		for (uint16_t seq_num = 0U; seq_num < SEQ_NUM_MAX; seq_num++) {
			for (int chan = 0; chan < CONFIG_BT_ISO_MAX_CHAN; chan++) {
				uint8_t iso_data[CONFIG_BT_ISO_TX_MTU] = { 0, };
				struct net_buf *buf;
				int ret;

				buf = net_buf_alloc(&tx_pool, K_MSEC(BUF_ALLOC_TIMEOUT));
				if (!buf) {
					FAIL("Data buffer allocate timeout on channel %d\n", chan);
					return;
				}

				net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
				sys_put_le16(seq_num, iso_data);
				net_buf_add_mem(buf, iso_data, sizeof(iso_data));

				ret = k_sem_take(&sem_iso_data, K_MSEC(BUF_ALLOC_TIMEOUT));
				if (ret) {
					FAIL("k_sem_take for ISO data sent failed.\n");
					return;
				}

				printk("ISO send: seq_num %u, chan %d\n", seq_num, chan);
				ret = bt_iso_chan_send(&iso_chan_p[chan], buf, seq_num);
				if (ret < 0) {
					FAIL("Unable to send data on channel %d : %d\n", chan, ret);
					net_buf_unref(buf);
					return;
				}
			}

			if ((seq_num % 100) == 0) {
				printk("Sending value %u\n", seq_num);
			}
		}
	}

#if defined(CONFIG_TEST_MULTIPLE_PERIPERAL_CIS)
	for (int chan = 0; chan < CONFIG_BT_ISO_MAX_CHAN; chan++) {
#endif

		printk("Waiting for ISO channel disconnect...");
		err = k_sem_take(&sem_iso_disc, K_FOREVER);
		if (err) {
			FAIL("failed (err %d)\n", err);
			return;
		}
		printk("disconnected to peer ISO channel.\n");

#if defined(CONFIG_TEST_MULTIPLE_PERIPERAL_CIS)
	}
#endif

	printk("Waiting for disconnection...");
	err = k_sem_take(&sem_peer_disc, K_FOREVER);
	if (err) {
		FAIL("failed (err %d)\n", err);
		return;
	}
	printk("disconnected from peer device.\n");

#if !defined(CONFIG_TEST_FT_SKIP_SUBEVENTS) || defined(CONFIG_TEST_FT_PER_SKIP_SUBEVENTS)
#if defined(CONFIG_TEST_MULTIPLE_PERIPERAL_CIS)
	for (int chan = 0; chan < CONFIG_BT_ISO_MAX_CHAN; chan++) {
#else
		int  chan = 0;
#endif
		if (expected_seq_num[chan] < SEQ_NUM_MAX) {
			FAIL("ISO Data reception incomplete %u (%u).\n", expected_seq_num[chan],
			     SEQ_NUM_MAX);
			return;
		}
#if defined(CONFIG_TEST_MULTIPLE_PERIPERAL_CIS)
	}
#endif
#endif

	PASS("Peripheral ISO tests Passed\n");
}

static void test_cis_init(void)
{
	bst_ticker_set_next_tick_absolute(60e6);
	bst_result = In_progress;
}

static void test_cis_tick(bs_time_t HW_device_time)
{
	if (bst_result != Passed) {
		FAIL("test failed (not passed after seconds)\n");
	}
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "central",
		.test_descr = "Central ISO",
		.test_pre_init_f = test_cis_init,
		.test_tick_f = test_cis_tick,
		.test_main_f = test_cis_central,
	},
	{
		.test_id = "peripheral",
		.test_descr = "Peripheral ISO",
		.test_pre_init_f = test_cis_init,
		.test_tick_f = test_cis_tick,
		.test_main_f = test_cis_peripheral,
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_cis_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}

bst_test_install_t test_installers[] = {
	test_cis_install,
	NULL
};

int main(void)
{
	bst_main();
	return 0;
}
