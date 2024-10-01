/* main_l2cap_ecred.c - Application main entry point */

/*
 * Copyright (c) 2022 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common.h"

#define LOG_MODULE_NAME main_l2cap_ecred
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_DBG);

extern enum bst_result_t bst_result;

static struct bt_conn *default_conn;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
};

#define DATA_MTU CONFIG_BT_L2CAP_TX_MTU
#define DATA_MPS 65
#define DATA_BUF_SIZE BT_L2CAP_SDU_BUF_SIZE(DATA_MTU)
#define L2CAP_CHANNELS 2
#define SERVERS 1
#define SDU_SEND_COUNT 200
#define ECRED_CHAN_MAX 5
#define LONG_MSG (DATA_MTU - 500)
#define SHORT_MSG (DATA_MPS - 2)
#define LONG_MSG_CHAN_IDX 0
#define SHORT_MSG_CHAN_IDX 1

NET_BUF_POOL_FIXED_DEFINE(rx_data_pool, L2CAP_CHANNELS, BT_L2CAP_BUF_SIZE(DATA_BUF_SIZE), 8, NULL);
NET_BUF_POOL_FIXED_DEFINE(tx_data_pool_0, 1, BT_L2CAP_SDU_BUF_SIZE(DATA_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);
NET_BUF_POOL_FIXED_DEFINE(tx_data_pool_1, 1, BT_L2CAP_SDU_BUF_SIZE(DATA_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static struct bt_l2cap_server servers[SERVERS];
void send_sdu_chan_worker(struct k_work *item);
struct channel {
	uint8_t chan_id; /* Internal number that identifies L2CAP channel. */
	struct bt_l2cap_le_chan le;
	bool in_use;
	size_t sdus_received;
	size_t bytes_to_send;
	uint8_t iteration;
	struct net_buf *buf;
	struct k_work work;
	struct k_work_q work_queue;
	uint8_t payload[DATA_MTU];
};
static struct channel channels[L2CAP_CHANNELS];

CREATE_FLAG(is_connected);
CREATE_FLAG(unsequenced_data);

#define T_STACK_SIZE 512
#define T_PRIORITY 5

static K_THREAD_STACK_ARRAY_DEFINE(stack_area, L2CAP_CHANNELS, T_STACK_SIZE);
static K_SEM_DEFINE(chan_conn_sem, 0, L2CAP_CHANNELS);
static K_SEM_DEFINE(all_chan_conn_sem, 0, 1);
static K_SEM_DEFINE(all_chan_disconn_sem, 0, 1);
static K_SEM_DEFINE(sent_sem, 0, L2CAP_CHANNELS);

static void init_workqs(void)
{
	for (int i = 0; i < L2CAP_CHANNELS; i++) {
		k_work_queue_init(&channels[i].work_queue);
		k_work_queue_start(&channels[i].work_queue, stack_area[i],
		K_THREAD_STACK_SIZEOF(*stack_area), T_PRIORITY, NULL);
	}
}

static struct net_buf *chan_alloc_buf_cb(struct bt_l2cap_chan *chan)
{
	LOG_DBG("Allocated on chan %p", chan);
	return net_buf_alloc(&rx_data_pool, K_FOREVER);
}

static int chan_recv_cb(struct bt_l2cap_chan *l2cap_chan, struct net_buf *buf)
{
	struct bt_l2cap_le_chan *l2cap_le_chan = CONTAINER_OF(
			l2cap_chan, struct bt_l2cap_le_chan, chan);
	struct channel *chan = CONTAINER_OF(l2cap_le_chan, struct channel, le);
	const uint32_t received_iterration = net_buf_pull_le32(buf);

	LOG_DBG("received_iterration %i sdus_received %i, chan_id: %d, data_length: %d",
		received_iterration, chan->sdus_received, chan->chan_id, buf->len);
	if (!TEST_FLAG(unsequenced_data) && received_iterration != chan->sdus_received) {
		FAIL("Received out of sequence data.");
	}

	const int retval = memcmp(buf->data + sizeof(received_iterration),
			    chan->payload + sizeof(received_iterration),
			    buf->len - sizeof(received_iterration));
	if (retval) {
		FAIL("Payload received didn't match expected value memcmp returned %i", retval);
	}

	/*By the time we rx on long msg channel we should have already rx on short msg channel*/
	if (chan->chan_id == 0) {
		if (channels[SHORT_MSG_CHAN_IDX].sdus_received !=
			(channels[LONG_MSG_CHAN_IDX].sdus_received + 1)) {
			FAIL("Didn't receive on short msg channel first");
		}
	}

	chan->sdus_received++;

	return 0;
}

static void chan_sent_cb(struct bt_l2cap_chan *l2cap_chan)
{
	struct bt_l2cap_le_chan *l2cap_le_chan = CONTAINER_OF(
			l2cap_chan, struct bt_l2cap_le_chan, chan);
	struct channel *chan = CONTAINER_OF(l2cap_le_chan, struct channel, le);

	chan->buf = 0;
	k_sem_give(&sent_sem);

	LOG_DBG("chan_id: %d", chan->chan_id);
}

static void chan_connected_cb(struct bt_l2cap_chan *l2cap_chan)
{
	struct bt_l2cap_le_chan *l2cap_le_chan = CONTAINER_OF(
			l2cap_chan, struct bt_l2cap_le_chan, chan);
	struct channel *chan = CONTAINER_OF(l2cap_le_chan, struct channel, le);

	LOG_DBG("chan_id: %d", chan->chan_id);

	LOG_DBG("tx.mtu %d, tx.mps: %d, rx.mtu: %d, rx.mps %d", sys_cpu_to_le16(chan->le.tx.mtu),
		sys_cpu_to_le16(chan->le.tx.mps), sys_cpu_to_le16(chan->le.rx.mtu),
		sys_cpu_to_le16(chan->le.rx.mps));

	k_sem_give(&chan_conn_sem);

	if (k_sem_count_get(&chan_conn_sem) == L2CAP_CHANNELS) {
		k_sem_give(&all_chan_conn_sem);
		k_sem_reset(&all_chan_disconn_sem);
	}
}

static void chan_disconnected_cb(struct bt_l2cap_chan *l2cap_chan)
{
	struct bt_l2cap_le_chan *l2cap_le_chan = CONTAINER_OF(
			l2cap_chan, struct bt_l2cap_le_chan, chan);
	struct channel *chan = CONTAINER_OF(l2cap_le_chan, struct channel, le);

	LOG_DBG("chan_id: %d", chan->chan_id);

	chan->in_use = false;
	k_sem_take(&chan_conn_sem, K_FOREVER);

	if (k_sem_count_get(&chan_conn_sem) == 0) {
		k_sem_give(&all_chan_disconn_sem);
		k_sem_reset(&all_chan_conn_sem);
	}
}

static void chan_status_cb(struct bt_l2cap_chan *l2cap_chan, atomic_t *status)
{
	struct bt_l2cap_le_chan *l2cap_le_chan = CONTAINER_OF(
			l2cap_chan, struct bt_l2cap_le_chan, chan);
	struct channel *chan = CONTAINER_OF(l2cap_le_chan, struct channel, le);

	LOG_DBG("chan_id: %d, status: %ld", chan->chan_id, *status);
}

static void chan_released_cb(struct bt_l2cap_chan *l2cap_chan)
{
	struct bt_l2cap_le_chan *l2cap_le_chan = CONTAINER_OF(
			l2cap_chan, struct bt_l2cap_le_chan, chan);
	struct channel *chan = CONTAINER_OF(l2cap_le_chan, struct channel, le);

	LOG_DBG("chan_id: %d", chan->chan_id);
}

static void chan_reconfigured_cb(struct bt_l2cap_chan *l2cap_chan)
{
	struct bt_l2cap_le_chan *l2cap_le_chan = CONTAINER_OF(
			l2cap_chan, struct bt_l2cap_le_chan, chan);
	struct channel *chan = CONTAINER_OF(l2cap_le_chan, struct channel, le);

	LOG_DBG("chan_id: %d", chan->chan_id);
}

static const struct bt_l2cap_chan_ops l2cap_ops = {
	.alloc_buf = chan_alloc_buf_cb,
	.recv = chan_recv_cb,
	.sent = chan_sent_cb,
	.connected = chan_connected_cb,
	.disconnected = chan_disconnected_cb,
	.status = chan_status_cb,
	.released = chan_released_cb,
	.reconfigured = chan_reconfigured_cb,
};

static struct channel *get_free_channel(void)
{
	for (int idx = 0; idx < L2CAP_CHANNELS; idx++) {
		struct channel *chan = &channels[idx];

		if (chan->in_use) {
			continue;
		}

		chan->chan_id = idx;
		channels[idx].in_use = true;
		(void)memset(chan->payload, idx, sizeof(chan->payload));
		k_work_init(&chan->work, send_sdu_chan_worker);
		chan->le.chan.ops = &l2cap_ops;
		chan->le.rx.mtu = DATA_MTU;
		chan->le.rx.mps = DATA_MPS;

		return chan;
	}

	return NULL;
}

static void connect_num_channels(uint8_t num_l2cap_channels)
{
	struct bt_l2cap_chan *allocated_channels[ECRED_CHAN_MAX] = { NULL };

	for (int i = 0; i < num_l2cap_channels; i++) {
		struct channel *chan = get_free_channel();

		if (!chan) {
			FAIL("failed, chan not free");
			return;
		}

		allocated_channels[i] = &chan->le.chan;
	}

	const int err = bt_l2cap_ecred_chan_connect(default_conn, allocated_channels,
						     servers[0].psm);

	if (err) {
		FAIL("can't connect ecred %d ", err);
	}
}

static void disconnect_all_channels(void)
{
	for (int i = 0; i < ARRAY_SIZE(channels); i++) {
		if (channels[i].in_use) {
			LOG_DBG("Disconnecting channel: %d)", channels[i].chan_id);
			const int err = bt_l2cap_chan_disconnect(&channels[i].le.chan);

			if (err) {
				LOG_DBG("can't disconnect channel (err: %d)", err);
			}

			channels[i].in_use = false;
		}
	}
}

static int accept(struct bt_conn *conn, struct bt_l2cap_server *server,
		  struct bt_l2cap_chan **l2cap_chan)
{
	struct channel *chan;

	chan = get_free_channel();
	if (!chan) {
		return -ENOMEM;
	}

	*l2cap_chan = &chan->le.chan;

	return 0;
}

static struct bt_l2cap_server *get_free_server(void)
{
	for (int i = 0; i < SERVERS; i++) {
		if (servers[i].psm) {
			continue;
		}

		return &servers[i];
	}

	return NULL;
}

static void register_l2cap_server(void)
{
	struct bt_l2cap_server *server;

	server = get_free_server();
	if (!server) {
		FAIL("Failed to get free server");
		return;
	}

	server->accept = accept;
	server->psm = 0;

	if (bt_l2cap_server_register(server) < 0) {
		FAIL("Failed to get free server");
		return;
	}

	LOG_DBG("L2CAP server registered, PSM:0x%X", server->psm);
}

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		FAIL("Failed to connect to %s (%u)", addr, conn_err);
		bt_conn_unref(default_conn);
		default_conn = NULL;
		return;
	}

	default_conn = bt_conn_ref(conn);
	LOG_DBG("%s", addr);

	SET_FLAG(is_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_DBG("%s (reason 0x%02x)", addr, reason);

	if (default_conn != conn) {
		FAIL("Conn mismatch disconnect %s %s)", default_conn, conn);
		return;
	}

	bt_conn_unref(default_conn);
	default_conn = NULL;
	UNSET_FLAG(is_connected);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void send_sdu(int iteration, int chan_idx, int bytes)
{
	struct bt_l2cap_chan *chan = &channels[chan_idx].le.chan;
	struct net_buf *buf;

	/* First 4 bytes in sent payload is iteration count */
	sys_put_le32(iteration, channels[chan_idx].payload);

	if (channels[chan_idx].buf != 0) {
		FAIL("Buf should have been deallocated by now");
		return;
	}

	if (chan_idx == 0) {
		buf = net_buf_alloc(&tx_data_pool_0, K_NO_WAIT);
	} else {
		buf = net_buf_alloc(&tx_data_pool_1, K_NO_WAIT);
	}

	if (buf == NULL) {
		FAIL("Failed to get buff on ch %i, iteration %i should never happen", chan_idx,
		     chan_idx);
	}

	channels[chan_idx].buf = buf;
	net_buf_reserve(buf, BT_L2CAP_SDU_CHAN_SEND_RESERVE);
	net_buf_add_mem(buf, channels[chan_idx].payload, bytes);

	LOG_DBG("bt_l2cap_chan_sending ch: %i bytes: %i iteration: %i", chan_idx, bytes, iteration);
	const int ret = bt_l2cap_chan_send(chan, buf);

	LOG_DBG("bt_l2cap_chan_send returned: %i", ret);

	if (ret < 0) {
		FAIL("Error: send failed error: %i", ret);
		channels[chan_idx].buf = 0;
		net_buf_unref(buf);
	}
}

void send_sdu_chan_worker(struct k_work *item)
{
	const struct channel *ch = CONTAINER_OF(item, struct channel, work);

	send_sdu(ch->iteration, ch->chan_id, ch->bytes_to_send);
}

static void send_sdu_concurrently(void)
{
	for (int i = 0; i < SDU_SEND_COUNT; i++) {
		for (int k = 0; k < L2CAP_CHANNELS; k++) {
			channels[k].iteration = i;
			/* Assign the right msg to the right channel */
			channels[k].bytes_to_send = (k == LONG_MSG_CHAN_IDX) ? LONG_MSG : SHORT_MSG;
			const int err = k_work_submit_to_queue(&channels[k].work_queue,
								&channels[k].work);

			if (err < 0) {
				FAIL("Failed to submit work to the queue, error: %d", err);
			}
		}

		/* Wait until messages on all of the channels has been sent */
		for (int l = 0; l < L2CAP_CHANNELS; l++) {
			k_sem_take(&sent_sem, K_FOREVER);
		}
	}
}

static int change_mtu_on_channels(int num_channels, int new_mtu)
{
	struct bt_l2cap_chan *reconf_channels[ECRED_CHAN_MAX] = { NULL };

	for (int i = 0; i < num_channels; i++) {
		reconf_channels[i] = &(&channels[i])->le.chan;
	}

	return bt_l2cap_ecred_chan_reconfigure(reconf_channels, new_mtu);
}

static void test_peripheral_main(void)
{
	device_sync_init(PERIPHERAL_ID);
	LOG_DBG("*L2CAP ECRED Peripheral started*");
	init_workqs();
	int err;

	err = bt_enable(NULL);
	if (err) {
		FAIL("Can't enable Bluetooth (err %d)", err);
		return;
	}

	LOG_DBG("Peripheral Bluetooth initialized.");
	LOG_DBG("Connectable advertising...");
	err = bt_le_adv_start(BT_LE_ADV_CONN_ONE_TIME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		FAIL("Advertising failed to start (err %d)", err);
		return;
	}

	LOG_DBG("Advertising started.");
	LOG_DBG("Peripheral waiting for connection...");
	WAIT_FOR_FLAG_SET(is_connected);
	LOG_DBG("Peripheral Connected.");
	register_l2cap_server();
	connect_num_channels(L2CAP_CHANNELS);
	k_sem_take(&all_chan_conn_sem, K_FOREVER);

	/* Disconnect and reconnect channels *****************************************************/
	LOG_DBG("############# Disconnect and reconnect channels");
	disconnect_all_channels();
	k_sem_take(&all_chan_disconn_sem, K_FOREVER);

	connect_num_channels(L2CAP_CHANNELS);
	k_sem_take(&all_chan_conn_sem, K_FOREVER);

	LOG_DBG("Send sync after reconnection");
	device_sync_send();

	/* Send bytes on both channels and expect ch 1 to receive all of them before ch 0 *********/
	LOG_DBG("############# Send bytes on both channels concurrently");
	send_sdu_concurrently();

	/* Change mtu size on all connected channels *********************************************/
	LOG_DBG("############# Change MTU of the channels");
	err = change_mtu_on_channels(L2CAP_CHANNELS, CONFIG_BT_L2CAP_TX_MTU + 10);

	if (err) {
		FAIL("MTU change failed (err %d)\n", err);
	}

	/* Read from both devices (Central and Peripheral) at the same time **********************/
	LOG_DBG("############# Read from both devices (Central and Peripheral) at the same time");
	LOG_DBG("Wait for sync before sending the msg");
	device_sync_wait();
	LOG_DBG("Received sync");
	send_sdu(0, 1, 10);

	k_sem_take(&sent_sem, K_FOREVER);
	disconnect_all_channels();
	WAIT_FOR_FLAG_UNSET(is_connected);
	PASS("L2CAP ECRED Peripheral tests Passed");
	bs_trace_silent_exit(0);
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	struct bt_le_conn_param *param;
	int err;

	err = bt_le_scan_stop();
	if (err) {
		FAIL("Stop LE scan failed (err %d)", err);
		return;
	}

	param = BT_LE_CONN_PARAM_DEFAULT;
	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, param, &default_conn);
	if (err) {
		FAIL("Create conn failed (err %d)", err);
		return;
	}
}

static void test_central_main(void)
{
	struct bt_le_scan_param scan_param = {
		.type = BT_LE_SCAN_TYPE_ACTIVE,
		.options = BT_LE_SCAN_OPT_NONE,
		.interval = BT_GAP_SCAN_FAST_INTERVAL,
		.window = BT_GAP_SCAN_FAST_WINDOW,
	};

	device_sync_init(CENTRAL_ID);

	LOG_DBG("*L2CAP ECRED Central started*");
	int err;

	err = bt_enable(NULL);
	if (err) {
		FAIL("Can't enable Bluetooth (err %d)\n", err);
		return;
	}
	LOG_DBG("Central Bluetooth initialized.\n");

	err = bt_le_scan_start(&scan_param, device_found);
	if (err) {
		FAIL("Scanning failed to start (err %d)\n", err);
		return;
	}

	LOG_DBG("Scanning successfully started\n");

	LOG_DBG("Central waiting for connection...\n");
	WAIT_FOR_FLAG_SET(is_connected);
	LOG_DBG("Central Connected.\n");
	register_l2cap_server();

	LOG_DBG("Wait for sync after reconnection");
	device_sync_wait();
	LOG_DBG("Received sync");

	/* Read from both devices (Central and Peripheral) at the same time **********************/
	LOG_DBG("############# Read from both devices (Central and Peripheral) at the same time");
	LOG_DBG("Send sync for SDU send");
	SET_FLAG(unsequenced_data);
	device_sync_send();
	send_sdu(0, 1, 10);

	/* Wait until all of the channels are disconnected */
	k_sem_take(&all_chan_disconn_sem, K_FOREVER);

	LOG_DBG("Both l2cap channels disconnected, test over\n");

	UNSET_FLAG(unsequenced_data);
	LOG_DBG("received PDUs on long msg channel %i and short msg channel %i",
		channels[LONG_MSG_CHAN_IDX].sdus_received,
		channels[SHORT_MSG_CHAN_IDX].sdus_received);

	if (channels[LONG_MSG_CHAN_IDX].sdus_received < SDU_SEND_COUNT ||
	    channels[SHORT_MSG_CHAN_IDX].sdus_received < SDU_SEND_COUNT) {
		FAIL("received less than %i", SDU_SEND_COUNT);
	}

	/* Disconnect */
	LOG_DBG("Central Disconnecting....");
	err = bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	bt_conn_unref(default_conn);
	LOG_DBG("Central tried to disconnect");

	if (err) {
		FAIL("Disconnection failed (err %d)", err);
		return;
	}

	LOG_DBG("Central Disconnected.");

	PASS("L2CAP ECRED Central tests Passed\n");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "peripheral",
		.test_descr = "Peripheral L2CAP ECRED",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_peripheral_main
	},
	{
		.test_id = "central",
		.test_descr = "Central L2CAP ECRED",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_central_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_main_l2cap_ecred_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}
