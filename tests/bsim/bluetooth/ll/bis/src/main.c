/* main.c - Application main entry point */

/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>

#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/iso.h>

#include "subsys/bluetooth/host/hci_core.h"
#include "subsys/bluetooth/controller/include/ll.h"
#include "subsys/bluetooth/controller/util/memq.h"
#include "subsys/bluetooth/controller/ll_sw/lll.h"

/* For VS data path */
#include "subsys/bluetooth/controller/ll_sw/isoal.h"
#include "subsys/bluetooth/controller/ll_sw/ull_iso_types.h"

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

static uint8_t mfg_data1[] = { 0xff, 0xff, 0x01, 0x02, 0x03, 0x04 };
static uint8_t mfg_data2[] = { 0xff, 0xff, 0x05 };

static const struct bt_data per_ad_data1[] = {
	BT_DATA(BT_DATA_MANUFACTURER_DATA, mfg_data1, 6),
};

static const struct bt_data per_ad_data2[] = {
	BT_DATA(BT_DATA_MANUFACTURER_DATA, mfg_data2, 3),
};

static uint8_t chan_map[] = { 0x1F, 0XF1, 0x1F, 0xF1, 0x1F };

static bool volatile is_iso_connected;
static bool volatile deleting_pa_sync;
static void iso_connected(struct bt_iso_chan *chan);
static void iso_disconnected(struct bt_iso_chan *chan, uint8_t reason);
static void iso_recv(struct bt_iso_chan *chan,
		     const struct bt_iso_recv_info *info, struct net_buf *buf);

static struct bt_iso_chan_ops iso_ops = {
	.connected	= iso_connected,
	.disconnected	= iso_disconnected,
	.recv		= iso_recv,
};

static struct bt_iso_chan_path iso_path_rx = {
	.pid = BT_HCI_DATAPATH_ID_HCI
};

static struct bt_iso_chan_qos bis_iso_qos;
static struct bt_iso_chan_io_qos iso_tx_qos;
static struct bt_iso_chan_io_qos iso_rx_qos = {
	.path = &iso_path_rx
};

static struct bt_iso_chan bis_iso_chan = {
	.ops = &iso_ops,
	.qos = &bis_iso_qos,
};

#define BIS_ISO_CHAN_COUNT 1
static struct bt_iso_chan *bis_channels[BIS_ISO_CHAN_COUNT] = { &bis_iso_chan };
static uint16_t seq_num;

NET_BUF_POOL_FIXED_DEFINE(bis_tx_pool, BIS_ISO_CHAN_COUNT,
			  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

#if defined(CONFIG_BT_CTLR_ISO_VENDOR_DATA_PATH)
static uint8_t test_rx_buffer[CONFIG_BT_CTLR_SYNC_ISO_PDU_LEN_MAX];
static bool is_iso_vs_emitted;

static isoal_status_t test_sink_sdu_alloc(const struct isoal_sink    *sink_ctx,
					  const struct isoal_pdu_rx  *valid_pdu,
					  struct isoal_sdu_buffer    *sdu_buffer)
{
	sdu_buffer->dbuf = test_rx_buffer;
	sdu_buffer->size = sizeof(test_rx_buffer);

	return ISOAL_STATUS_OK;
}


static isoal_status_t test_sink_sdu_emit(const struct isoal_sink             *sink_ctx,
					 const struct isoal_emitted_sdu_frag *sdu_frag,
					 const struct isoal_emitted_sdu      *sdu)
{
	printk("Vendor sink SDU fragment size %u / %u, seq_num %u, ts %u\n",
		sdu_frag->sdu_frag_size, sdu->total_sdu_size, sdu_frag->sdu.sn,
		sdu_frag->sdu.timestamp);
	is_iso_vs_emitted = true;

	return ISOAL_STATUS_OK;
}

static isoal_status_t test_sink_sdu_write(void *dbuf,
					  const uint8_t *pdu_payload,
					  const size_t consume_len)
{
	memcpy(dbuf, pdu_payload, consume_len);

	return ISOAL_STATUS_OK;
}


bool ll_data_path_sink_create(uint16_t handle, struct ll_iso_datapath *datapath,
			      isoal_sink_sdu_alloc_cb *sdu_alloc,
			      isoal_sink_sdu_emit_cb *sdu_emit,
			      isoal_sink_sdu_write_cb *sdu_write)
{
	ARG_UNUSED(handle);
	ARG_UNUSED(datapath);

	*sdu_alloc = test_sink_sdu_alloc;
	*sdu_emit  = test_sink_sdu_emit;
	*sdu_write = test_sink_sdu_write;

	printk("VS data path sink created\n");
	return true;
}
#endif /* CONFIG_BT_CTLR_ISO_VENDOR_DATA_PATH */

static void test_iso_main(void)
{
	struct bt_le_ext_adv *adv;
	int err;

	printk("\n*ISO broadcast test*\n");

	printk("Bluetooth initializing...");
	err = bt_enable(NULL);
	if (err) {
		FAIL("Could not init BT: %d\n", err);
		return;
	}
	printk("success.\n");

	printk("Create advertising set...");
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN_NAME, NULL, &adv);
	if (err) {
		FAIL("Failed to create advertising set (err %d)\n", err);
		return;
	}
	printk("success.\n");

	printk("Setting Periodic Advertising parameters...");
	err = bt_le_per_adv_set_param(adv, BT_LE_PER_ADV_DEFAULT);
	if (err) {
		FAIL("Failed to set periodic advertising parameters (err %d)\n",
		     err);
		return;
	}
	printk("success.\n");

	printk("Enable Periodic Advertising...");
	err = bt_le_per_adv_start(adv);
	if (err) {
		FAIL("Failed to enable periodic advertising (err %d)\n", err);
		return;
	}
	printk("success.\n");

	printk("Start extended advertising...");
	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err) {
		printk("Failed to start extended advertising (err %d)\n", err);
		return;
	}
	printk("success.\n");


#if TEST_LL_INTERFACE
	printk("Creating BIG...");
	uint16_t max_sdu = CONFIG_BT_CTLR_ADV_ISO_PDU_LEN_MAX;
	uint8_t bcode[BT_ISO_BROADCAST_CODE_SIZE] = { 0 };
	uint32_t sdu_interval = 10000; /* us */
	uint16_t max_latency = 10; /* ms */
	uint8_t encryption = 0;
	uint8_t big_handle = 0;
	uint8_t bis_count = 1; /* TODO: Add support for multiple BIS per BIG */
	uint8_t phy = BIT(1);
	uint8_t packing = 0;
	uint8_t framing = 0;
	uint8_t adv_handle;
	uint8_t rtn = 0;

	/* Assume that index == handle */
	adv_handle = bt_le_ext_adv_get_index(adv);

	err = ll_big_create(big_handle, adv_handle, bis_count, sdu_interval,
			    max_sdu, max_latency, rtn, phy, packing, framing,
			    encryption, bcode);
	if (err) {
		FAIL("Could not create BIG: %d\n", err);
		return;
	}
	printk("success.\n");
#endif

	printk("Creating BIG...\n");
	struct bt_iso_big_create_param big_create_param;
	struct bt_iso_big *big;

	big_create_param.bis_channels = bis_channels;
	big_create_param.num_bis = BIS_ISO_CHAN_COUNT;
	big_create_param.encryption = false;
	big_create_param.interval = 10000; /* us */
	big_create_param.latency = 10; /* milliseconds */
	big_create_param.packing = 0; /* 0 - sequential; 1 - interleaved */
	big_create_param.framing = 0; /* 0 - unframed; 1 - framed */
	iso_tx_qos.sdu = 502; /* bytes */
	iso_tx_qos.rtn = 2;
	iso_tx_qos.phy = BT_GAP_LE_PHY_2M;
	bis_iso_qos.tx = &iso_tx_qos;
	bis_iso_qos.rx = NULL;
	err = bt_iso_big_create(adv, &big_create_param, &big);
	if (err) {
		FAIL("Could not create BIG: %d\n", err);
		return;
	}
	printk("success.\n");

	printk("Wait for ISO connected callback...");
	while (!is_iso_connected) {
		k_sleep(K_MSEC(100));
	}
	printk("ISO connected\n");

	uint32_t iso_send_count = 0;
	uint8_t iso_data[sizeof(iso_send_count)] = { 0 };
	struct net_buf *buf;

	buf = net_buf_alloc(&bis_tx_pool, K_FOREVER);
	net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
	sys_put_le32(++iso_send_count, iso_data);
	net_buf_add_mem(buf, iso_data, sizeof(iso_data));
	err = bt_iso_chan_send(&bis_iso_chan, buf, seq_num++, BT_ISO_TIMESTAMP_NONE);
	if (err < 0) {
		net_buf_unref(buf);
		FAIL("Unable to broadcast data (%d)", err);
		return;
	}
	printk("Sending value %u\n", iso_send_count);

	k_sleep(K_MSEC(5000));

	printk("Update periodic advertising data 1...");
	err = bt_le_per_adv_set_data(adv, per_ad_data1,
				     ARRAY_SIZE(per_ad_data1));
	if (err) {
		FAIL("Failed to update periodic advertising data 1 (%d).\n",
		     err);
	}
	printk("success.\n");

	k_sleep(K_MSEC(2500));

	printk("Periodic Advertising and ISO Channel Map Update...");
	err = ll_chm_update(chan_map);
	if (err) {
		FAIL("Channel Map Update failed.\n");
	}
	printk("success.\n");

	k_sleep(K_MSEC(2500));

	printk("Update periodic advertising data 2...");
	err = bt_le_per_adv_set_data(adv, per_ad_data2,
				     ARRAY_SIZE(per_ad_data2));
	if (err) {
		FAIL("Failed to update periodic advertising data 2 (%d).\n",
		     err);
	}
	printk("success.\n");

	k_sleep(K_MSEC(5000));

#if TEST_LL_INTERFACE
	printk("Terminating BIG...");
	err = ll_big_terminate(big_handle, BT_HCI_ERR_LOCALHOST_TERM_CONN);
	if (err) {
		FAIL("Could not terminate BIG: %d\n", err);
		return;
	}
	printk("success.\n");
#endif

	printk("Terminating BIG...\n");
	err = bt_iso_big_terminate(big);
	if (err) {
		FAIL("Could not terminate BIG: %d\n", err);
		return;
	}
	printk("success.\n");

	k_sleep(K_MSEC(10000));

	printk("Stop Periodic Advertising...");
	err = bt_le_per_adv_stop(adv);
	if (err) {
		FAIL("Failed to stop periodic advertising (err %d)\n", err);
		return;
	}
	printk("success.\n");


	PASS("ISO tests Passed\n");

	return;
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

static void iso_recv(struct bt_iso_chan *chan,
		     const struct bt_iso_recv_info *info, struct net_buf *buf)
{
	printk("Incoming data channel %p len %u, flags %u, seq_num %u, ts %u\n",
		chan, buf->len, info->flags, info->seq_num, info->ts);
}

static void iso_connected(struct bt_iso_chan *chan)
{
	printk("ISO Channel %p connected\n", chan);

	seq_num = 0U;
	is_iso_connected = true;
}

static uint8_t volatile is_iso_disconnected;

static void iso_disconnected(struct bt_iso_chan *chan, uint8_t reason)
{
	printk("ISO Channel %p disconnected with reason 0x%02x\n", chan, reason);

	is_iso_disconnected = reason;
}

static bool volatile is_sync;

static void pa_sync_cb(struct bt_le_per_adv_sync *sync,
		     struct bt_le_per_adv_sync_synced_info *info)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	printk("PER_ADV_SYNC[%u]: [DEVICE]: %s synced, "
	       "Interval 0x%04x (%u ms), PHY %s\n",
	       bt_le_per_adv_sync_get_index(sync), le_addr,
	       info->interval, info->interval * 5 / 4, phy2str(info->phy));

	is_sync = true;
}

static void pa_terminated_cb(struct bt_le_per_adv_sync *sync,
			     const struct bt_le_per_adv_sync_term_info *info)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	printk("PER_ADV_SYNC[%u]: [DEVICE]: %s sync terminated\n",
	       bt_le_per_adv_sync_get_index(sync), le_addr);

	if (!deleting_pa_sync) {
		FAIL("PA terminated unexpectedly\n");
	} else {
		deleting_pa_sync = false;
	}
}

static bool volatile is_sync_recv;

static void pa_recv_cb(struct bt_le_per_adv_sync *sync,
		       const struct bt_le_per_adv_sync_recv_info *info,
		       struct net_buf_simple *buf)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	printk("PER_ADV_SYNC[%u]: [DEVICE]: %s, tx_power %i, "
	       "RSSI %i, CTE %u, data length %u\n",
	       bt_le_per_adv_sync_get_index(sync), le_addr, info->tx_power,
	       info->rssi, info->cte_type, buf->len);

	is_sync_recv = true;
}

static void
pa_state_changed_cb(struct bt_le_per_adv_sync *sync,
		    const struct bt_le_per_adv_sync_state_info *info)
{
	printk("PER_ADV_SYNC[%u]: state changed, receive %s.\n",
	       bt_le_per_adv_sync_get_index(sync),
	       info->recv_enabled ? "enabled" : "disabled");
}

static bool volatile is_big_info;

static void pa_biginfo_cb(struct bt_le_per_adv_sync *sync,
			  const struct bt_iso_biginfo *biginfo)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(biginfo->addr, le_addr, sizeof(le_addr));
	printk("BIG INFO[%u]: [DEVICE]: %s, sid 0x%02x, "
	       "num_bis %u, nse %u, interval 0x%04x (%u ms), "
	       "bn %u, pto %u, irc %u, max_pdu %u, "
	       "sdu_interval %u us, max_sdu %u, phy %s, "
	       "%s framing, %sencrypted\n",
	       bt_le_per_adv_sync_get_index(sync), le_addr, biginfo->sid,
	       biginfo->num_bis, biginfo->sub_evt_count,
	       biginfo->iso_interval,
	       (biginfo->iso_interval * 5 / 4),
	       biginfo->burst_number, biginfo->offset,
	       biginfo->rep_count, biginfo->max_pdu, biginfo->sdu_interval,
	       biginfo->max_sdu, phy2str(biginfo->phy),
	       biginfo->framing ? "with" : "without",
	       biginfo->encryption ? "" : "not ");

	if (!is_big_info) {
		is_big_info = true;
	}
}

static struct bt_le_per_adv_sync_cb sync_cb = {
	.synced = pa_sync_cb,
	.term = pa_terminated_cb,
	.recv = pa_recv_cb,
	.state_changed = pa_state_changed_cb,
	.biginfo = pa_biginfo_cb,
};

#define NAME_LEN 30

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

static bool volatile is_periodic;
static bt_addr_le_t per_addr;
static uint8_t per_sid;

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

	if (info->interval) {
		if (!is_periodic) {
			is_periodic = true;
			per_sid = info->sid;
			bt_addr_le_copy(&per_addr, info->addr);
		}
	}
}

static struct bt_le_scan_cb scan_callbacks = {
	.recv = scan_recv,
};

static void test_iso_recv_main(void)
{
	struct bt_le_scan_param scan_param = {
		.type       = BT_HCI_LE_SCAN_ACTIVE,
		.options    = BT_LE_SCAN_OPT_NONE,
		.interval   = 0x0004,
		.window     = 0x0004,
	};
	struct bt_le_per_adv_sync_param sync_create_param;
	struct bt_le_per_adv_sync *sync = NULL;
	int err;

	printk("\n*ISO broadcast test*\n");

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

	printk("Periodic Advertising callbacks register...");
	bt_le_per_adv_sync_cb_register(&sync_cb);
	printk("Success.\n");

	printk("Start scanning...");
	is_periodic = false;
	err = bt_le_scan_start(&scan_param, NULL);
	if (err) {
		FAIL("Could not start scan: %d\n", err);
		return;
	}
	printk("success.\n");

	while (!is_periodic) {
		k_sleep(K_MSEC(100));
	}
	printk("Periodic Advertising found (SID: %u)\n", per_sid);

	printk("Creating Periodic Advertising Sync...");
	is_sync = false;
	bt_addr_le_copy(&sync_create_param.addr, &per_addr);
	sync_create_param.options =
		BT_LE_PER_ADV_SYNC_OPT_REPORTING_INITIALLY_DISABLED;
	sync_create_param.sid = per_sid;
	sync_create_param.skip = 0;
	sync_create_param.timeout = 0xa;
	err = bt_le_per_adv_sync_create(&sync_create_param, &sync);
	if (err) {
		FAIL("Could not create sync: %d\n", err);
		return;
	}
	printk("success.\n");

	/* TODO: Enable when advertiser is added */
	printk("Waiting for sync...");
	while (!is_sync) {
		k_sleep(K_MSEC(100));
	}

	printk("Stop scanning...");
	err = bt_le_scan_stop();
	if (err) {
		FAIL("Could not stop scan: %d\n", err);
		return;
	}
	printk("success.\n");

	printk("Wait for BIG Info Advertising Report...");
	is_big_info = false;
	while (!is_big_info) {
		k_sleep(K_MSEC(100));
	}
	printk("success.\n");

#if TEST_LL_INTERFACE
	printk("Creating BIG Sync...");
	uint8_t bcode[BT_ISO_BROADCAST_CODE_SIZE] = { 0 };
	uint16_t sync_timeout = 10;
	uint8_t big_handle = 0;
	uint8_t bis_handle = 0;
	uint8_t encryption = 0;
	uint8_t bis_count = 1; /* TODO: Add support for multiple BIS per BIG */
	uint8_t mse = 0;

	err = ll_big_sync_create(big_handle, sync->handle, encryption, bcode,
				 mse, sync_timeout, bis_count, &bis_handle);
	if (err) {
		FAIL("Could not create BIG sync: %d\n", err);
		return;
	}
	printk("success.\n");

	k_sleep(K_MSEC(5000));

	printk("Deleting Periodic Advertising Sync...");
	deleting_pa_sync = true;
	err = bt_le_per_adv_sync_delete(sync);
	if (err) {
		FAIL("Failed to delete periodic advertising sync (err %d)\n",
		     err);
		return;
	}
	printk("success.\n");

	printk("Terminating BIG Sync...");
	struct node_rx_hdr *node_rx = NULL;
	err = ll_big_sync_terminate(big_handle, (void **)&node_rx);
	if (err) {
		FAIL("Could not terminate BIG sync: %d\n", err);
		return;
	}
	printk("success.\n");

	if (node_rx) {
		FAIL("Generated Node Rx for synchronized BIG.\n");
	}

	k_sleep(K_MSEC(5000));

	printk("Creating BIG Sync after terminate...");
	err = ll_big_sync_create(big_handle, sync->handle, encryption, bcode,
				 mse, sync_timeout, bis_count, &bis_handle);
	if (err) {
		FAIL("Could not create BIG sync: %d\n", err);
		return;
	}
	printk("success.\n");

	printk("Terminating BIG Sync...");
	node_rx = NULL;
	err = ll_big_sync_terminate(big_handle, (void **)&node_rx);
	if (err) {
		FAIL("Could not terminate BIG sync: %d\n", err);
		return;
	}
	printk("success.\n");

	if (node_rx) {
		node_rx->next = NULL;
		ll_rx_mem_release((void **)&node_rx);
	}
#else
	struct bt_iso_big_sync_param big_param = { 0, };
	struct bt_iso_big *big;

	printk("ISO BIG create sync...");
	is_iso_connected = false;
	bis_iso_qos.tx = NULL;
	bis_iso_qos.rx = &iso_rx_qos;
	big_param.bis_channels = bis_channels;
	big_param.num_bis = BIS_ISO_CHAN_COUNT;
	big_param.bis_bitfield = BIT(1); /* BIS 1 selected */
	big_param.mse = 1;
	big_param.sync_timeout = 100; /* 1000 ms */
	big_param.encryption = false;
	iso_path_rx.pid = BT_HCI_DATAPATH_ID_HCI;
	memset(big_param.bcode, 0, sizeof(big_param.bcode));
	err = bt_iso_big_sync(sync, &big_param, &big);
	if (err) {
		FAIL("Could not create BIG sync: %d\n", err);
		return;
	}
	printk("success.\n");

	printk("Wait for ISO connected callback...");
	while (!is_iso_connected) {
		k_sleep(K_MSEC(100));
	}

	printk("ISO terminate BIG...");
	is_iso_disconnected = 0U;
	err = bt_iso_big_terminate(big);
	if (err) {
		FAIL("Could not terminate BIG sync: %d\n", err);
		return;
	}
	printk("success.\n");

	printk("Waiting for ISO disconnected callback...\n");
	while (!is_iso_disconnected) {
		k_sleep(K_MSEC(100));
	}
	printk("disconnected.\n");

	if (is_iso_disconnected != BT_HCI_ERR_LOCALHOST_TERM_CONN) {
		FAIL("Local Host Terminate Failed.\n");
	}

	printk("ISO BIG create sync (test remote disconnect)...");
	is_iso_connected = false;
	is_iso_disconnected = 0U;
	err = bt_iso_big_sync(sync, &big_param, &big);
	if (err) {
		FAIL("Could not create BIG sync: %d\n", err);
		return;
	}
	printk("success.\n");

	printk("Wait for ISO connected callback...");
	while (!is_iso_connected) {
		k_sleep(K_MSEC(100));
	}
	printk("connected.\n");

	printk("Waiting for ISO disconnected callback...\n");
	while (!is_iso_disconnected) {
		k_sleep(K_MSEC(100));
	}
	printk("disconnected.\n");

	if (is_iso_disconnected != BT_HCI_ERR_REMOTE_USER_TERM_CONN) {
		FAIL("Remote Host Terminate Failed.\n");
	}

	printk("Periodic sync receive enable...\n");
	err = bt_le_per_adv_sync_recv_enable(sync);
	if (err) {
		printk("failed (err %d)\n", err);
		return;
	}
	printk("receive enabled.\n");

	uint8_t check_countdown = 3;

	printk("Waiting for remote BIG terminate by checking for missing "
	       "%u BIG Info report...\n", check_countdown);
	do {
		is_sync_recv = false;
		is_big_info = false;
		while (!is_sync_recv) {
			k_sleep(K_MSEC(100));
		}

		k_sleep(K_MSEC(100));

		if (!is_big_info) {
			if (!--check_countdown) {
				break;
			}
		}
	} while (1);
	printk("success.\n");

	printk("Deleting Periodic Advertising Sync...");
	deleting_pa_sync = true;
	err = bt_le_per_adv_sync_delete(sync);
	if (err) {
		FAIL("Failed to delete periodic advertising sync (err %d)\n",
		     err);
		return;
	}
	printk("success.\n");
#endif

	PASS("ISO recv test Passed\n");

	return;
}

#if defined(CONFIG_BT_CTLR_ISO_VENDOR_DATA_PATH)
static void test_iso_recv_vs_dp_main(void)
{
	struct bt_le_scan_param scan_param = {
		.type       = BT_HCI_LE_SCAN_ACTIVE,
		.options    = BT_LE_SCAN_OPT_NONE,
		.interval   = 0x0004,
		.window     = 0x0004,
	};
	struct bt_le_per_adv_sync_param sync_create_param;
	struct bt_le_per_adv_sync *sync = NULL;
	int err;

	printk("Bluetooth initializing... ");
	err = bt_enable(NULL);
	if (err) {
		FAIL("Could not init BT: %d\n", err);
		return;
	}
	printk("success.\n");

	printk("Scan callbacks register... ");
	bt_le_scan_cb_register(&scan_callbacks);
	printk("success.\n");

	printk("Periodic Advertising callbacks register... ");
	bt_le_per_adv_sync_cb_register(&sync_cb);
	printk("success.\n");

	printk("Configure vendor data path... ");
	err = bt_configure_data_path(BT_HCI_DATAPATH_DIR_CTLR_TO_HOST,
				     BT_HCI_DATAPATH_ID_VS, 0U, NULL);
	if (err) {
		FAIL("Failed (err %d)\n", err);
		return;
	}

	printk("success.\n");
	printk("Start scanning... ");
	is_periodic = false;
	err = bt_le_scan_start(&scan_param, NULL);
	if (err) {
		FAIL("Could not start scan: %d\n", err);
		return;
	}
	printk("success.\n");

	while (!is_periodic) {
		k_sleep(K_MSEC(100));
	}
	printk("Periodic Advertising found (SID: %u)\n", per_sid);

	printk("Creating Periodic Advertising Sync... ");
	is_sync = false;

	bt_addr_le_copy(&sync_create_param.addr, &per_addr);
	sync_create_param.options =
		BT_LE_PER_ADV_SYNC_OPT_REPORTING_INITIALLY_DISABLED;
	sync_create_param.sid = per_sid;
	sync_create_param.skip = 0;
	sync_create_param.timeout = 0xa;

	err = bt_le_per_adv_sync_create(&sync_create_param, &sync);
	if (err) {
		FAIL("Could not create sync: %d\n", err);
		return;
	}
	printk("success.\n");

	/* TODO: Enable when advertiser is added */
	printk("Waiting for sync...\n");
	while (!is_sync) {
		k_sleep(K_MSEC(100));
	}
	printk("success.\n");

	printk("Stop scanning... ");
	err = bt_le_scan_stop();
	if (err) {
		FAIL("Could not stop scan: %d\n", err);
		return;
	}
	printk("success.\n");

	printk("Wait for BIG Info Advertising Report...\n");
	is_big_info = false;
	while (!is_big_info) {
		k_sleep(K_MSEC(100));
	}
	printk("success.\n");

	struct bt_iso_big_sync_param big_param = { 0, };
	struct bt_iso_big *big;

	printk("ISO BIG create sync... ");
	is_iso_connected = false;
	bis_iso_qos.tx = NULL;
	bis_iso_qos.rx = &iso_rx_qos;
	big_param.bis_channels = bis_channels;
	big_param.num_bis = BIS_ISO_CHAN_COUNT;
	big_param.bis_bitfield = BIT(1); /* BIS 1 selected */
	big_param.mse = 1;
	big_param.sync_timeout = 100; /* 1000 ms */
	big_param.encryption = false;
	memset(big_param.bcode, 0, sizeof(big_param.bcode));

	is_iso_connected = false;
	is_iso_disconnected = 0U;
	is_iso_vs_emitted = false;
	iso_path_rx.pid = BT_HCI_DATAPATH_ID_VS;

	err = bt_iso_big_sync(sync, &big_param, &big);
	if (err) {
		FAIL("Could not create BIG sync: %d\n", err);
		return;
	}
	printk("success.\n");

	printk("Wait for ISO connected callback... ");
	while (!is_iso_connected) {
		k_sleep(K_MSEC(100));
	}

	/* Allow some SDUs to be received */
	k_sleep(K_MSEC(100));

	printk("ISO terminate BIG... ");
	is_iso_disconnected = 0U;
	err = bt_iso_big_terminate(big);
	if (err) {
		FAIL("Could not terminate BIG sync: %d\n", err);
		return;
	}
	printk("success.\n");

	printk("Waiting for ISO disconnected callback...\n");
	while (!is_iso_disconnected) {
		k_sleep(K_MSEC(100));
	}
	printk("disconnected.\n");

	if (is_iso_disconnected != BT_HCI_ERR_LOCALHOST_TERM_CONN) {
		FAIL("Local Host Terminate Failed.\n");
	}

	if (!is_iso_vs_emitted) {
		FAIL("Emitting of VS SDUs failed.\n");
	}

	printk("success.\n");

	printk("Deleting Periodic Advertising Sync... ");
	deleting_pa_sync = true;
	err = bt_le_per_adv_sync_delete(sync);
	if (err) {
		FAIL("Failed to delete periodic advertising sync (err %d)\n",
		     err);
		return;
	}
	printk("success.\n");

	PASS("ISO recv VS test Passed\n");
}
#endif /* CONFIG_BT_CTLR_ISO_VENDOR_DATA_PATH */

static void test_iso_init(void)
{
	bst_ticker_set_next_tick_absolute(60e6);
	bst_result = In_progress;
}

static void test_iso_tick(bs_time_t HW_device_time)
{
	if (bst_result != Passed) {
		FAIL("test failed (not passed after seconds)\n");
	}
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "broadcast",
		.test_descr = "ISO broadcast",
		.test_post_init_f = test_iso_init,
		.test_tick_f = test_iso_tick,
		.test_main_f = test_iso_main
	},
	{
		.test_id = "receive",
		.test_descr = "ISO receive",
		.test_post_init_f = test_iso_init,
		.test_tick_f = test_iso_tick,
		.test_main_f = test_iso_recv_main
	},
#if defined(CONFIG_BT_CTLR_ISO_VENDOR_DATA_PATH)
	{
		.test_id = "receive_vs_dp",
		.test_descr = "ISO receive VS",
		.test_post_init_f = test_iso_init,
		.test_tick_f = test_iso_tick,
		.test_main_f = test_iso_recv_vs_dp_main
	},
#endif /* CONFIG_BT_CTLR_ISO_VENDOR_DATA_PATH */
	BSTEST_END_MARKER
};

struct bst_test_list *test_iso_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}

bst_test_install_t test_installers[] = {
	test_iso_install,
	NULL
};

int main(void)
{
	bst_main();
	return 0;
}
