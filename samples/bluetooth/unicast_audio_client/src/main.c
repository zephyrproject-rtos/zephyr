/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr.h>
#include <sys/printk.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/audio.h>
#include <sys/byteorder.h>

static void start_scan(void);

static struct bt_conn *default_conn;
static struct k_work_delayable audio_send_work;
static struct bt_audio_chan audio_channel;
static struct bt_audio_unicast_group *unicast_group;
static struct bt_audio_capability *remote_capabilities[CONFIG_BT_BAP_PAC_COUNT];
static struct bt_audio_ep *sinks[CONFIG_BT_BAP_ASE_SNK_COUNT];
NET_BUF_POOL_FIXED_DEFINE(tx_pool, 1, CONFIG_BT_ISO_TX_MTU + BT_ISO_CHAN_SEND_RESERVE, NULL);

/* Mandatory support preset by both client and server */
static struct bt_audio_lc3_preset preset_16_2_1 = BT_AUDIO_LC3_UNICAST_PRESET_16_2_1;

static K_SEM_DEFINE(sem_connected, 0, 1);
static K_SEM_DEFINE(sem_mtu_exchanged, 0, 1);
static K_SEM_DEFINE(sem_sink_discovered, 0, 1);
static K_SEM_DEFINE(sem_chan_configured, 0, 1);
static K_SEM_DEFINE(sem_chan_qos, 0, 1);
static K_SEM_DEFINE(sem_chan_enabled, 0, 1);

void print_hex(const uint8_t *ptr, size_t len)
{
	while (len-- != 0) {
		printk("%02x", *ptr++);
	}
}

static void print_codec(const struct bt_codec *codec)
{
	printk("codec 0x%02x cid 0x%04x vid 0x%04x count %u\n",
	       codec->id, codec->cid, codec->vid, codec->data_count);

	for (size_t i = 0; i < codec->data_count; i++) {
		printk("data #%zu: type 0x%02x len %u\n",
		       i, codec->data[i].data.type,
		       codec->data[i].data.data_len);
		print_hex(codec->data[i].data.data,
			  codec->data[i].data.data_len -
				sizeof(codec->data[i].data.type));
		printk("\n");
	}

	for (size_t i = 0; i < codec->meta_count; i++) {
		printk("meta #%zu: type 0x%02x len %u\n",
		       i, codec->meta[i].data.type,
		       codec->meta[i].data.data_len);
		print_hex(codec->meta[i].data.data,
			  codec->meta[i].data.data_len -
				sizeof(codec->meta[i].data.type));
		printk("\n");
	}
}

static void print_qos(struct bt_codec_qos *qos)
{
	printk("QoS: dir 0x%02x interval %u framing 0x%02x phy 0x%02x sdu %u "
	       "rtn %u latency %u pd %u\n",
	       qos->dir, qos->interval, qos->framing, qos->phy, qos->sdu,
	       qos->rtn, qos->latency, qos->pd);
}

/**
 * @brief Send audio data on timeout
 *
 * This will send an increasing amount of audio data, starting from 1 octet.
 * The data is just mock data, and does actually represent any audio.
 *
 * First iteration : 0x00
 * Second iteration: 0x00 0x01
 * Third iteration : 0x00 0x01 0x02
 *
 * And so on, until it wraps around the configured MTU (CONFIG_BT_ISO_TX_MTU)
 *
 * @param work Pointer to the work structure
 */
static void audio_timer_timeout(struct k_work *work)
{
	int ret;
	static uint8_t buf_data[CONFIG_BT_ISO_TX_MTU];
	static bool data_initialized;
	struct net_buf *buf;
	static size_t len_to_send = 1;

	if (!data_initialized) {
		/* TODO: Actually encode some audio data */
		for (int i = 0; i < ARRAY_SIZE(buf_data); i++) {
			buf_data[i] = (uint8_t)i;
		}

		data_initialized = true;
	}

	buf = net_buf_alloc(&tx_pool, K_FOREVER);
	net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);

	net_buf_add_mem(buf, buf_data, len_to_send);

	ret = bt_audio_chan_send(&audio_channel, buf);
	if (ret < 0) {
		printk("Failed to send audio data (%d)\n", ret);
		net_buf_unref(buf);
	} else {
		printk("Sending mock data with len %zu\n", len_to_send);
	}

	k_work_schedule(&audio_send_work, K_MSEC(1000));

	len_to_send++;
	if (len_to_send > ARRAY_SIZE(buf_data)) {
		len_to_send = 1;
	}
}

static struct bt_audio_chan *lc3_config(struct bt_conn *conn,
					struct bt_audio_ep *ep,
					struct bt_audio_capability *cap,
					struct bt_codec *codec)
{
	printk("ASE Codec Config: conn %p ep %p cap %p\n", conn, ep, cap);

	print_codec(codec);

	if (audio_channel.conn == NULL) {
		return &audio_channel;
	}

	printk("No channels available\n");

	return NULL;
}

static int lc3_reconfig(struct bt_audio_chan *chan,
			struct bt_audio_capability *cap,
			struct bt_codec *codec)
{
	printk("ASE Codec Reconfig: chan %p cap %p\n", chan, cap);

	print_codec(codec);

	k_sem_give(&sem_chan_configured);

	return 0;
}

static int lc3_qos(struct bt_audio_chan *chan, struct bt_codec_qos *qos)
{
	printk("QoS: chan %p qos %p\n", chan, qos);

	print_qos(qos);

	k_sem_give(&sem_chan_qos);

	return 0;
}

static int lc3_enable(struct bt_audio_chan *chan, uint8_t meta_count,
		      struct bt_codec_data *meta)
{
	printk("Enable: chan %p meta_count %u\n", chan, meta_count);

	k_sem_give(&sem_chan_enabled);

	return 0;
}

static int lc3_start(struct bt_audio_chan *chan)
{
	printk("Start: chan %p\n", chan);

	return 0;
}

static int lc3_metadata(struct bt_audio_chan *chan, uint8_t meta_count,
			struct bt_codec_data *meta)
{
	printk("Metadata: chan %p meta_count %u\n", chan, meta_count);

	return 0;
}

static int lc3_disable(struct bt_audio_chan *chan)
{
	printk("Disable: chan %p\n", chan);

	return 0;
}

static int lc3_stop(struct bt_audio_chan *chan)
{
	printk("Stop: chan %p\n", chan);

	return 0;
}

static int lc3_release(struct bt_audio_chan *chan)
{
	printk("Release: chan %p\n", chan);

	return 0;
}

static struct bt_audio_capability_ops lc3_ops = {
	.config = lc3_config,
	.reconfig = lc3_reconfig,
	.qos = lc3_qos,
	.enable = lc3_enable,
	.start = lc3_start,
	.metadata = lc3_metadata,
	.disable = lc3_disable,
	.stop = lc3_stop,
	.release = lc3_release,
};

static struct bt_audio_capability caps[] = {
	{
		.type = BT_AUDIO_SOURCE,
		.pref = BT_AUDIO_CAPABILITY_PREF(
				BT_AUDIO_CAPABILITY_UNFRAMED_SUPPORTED,
				BT_GAP_LE_PHY_2M, 0x02, 10, 40000, 40000,
				40000, 40000),
		.codec = &preset_16_2_1.codec,
		.ops = &lc3_ops,
	}
};

static bool check_audio_support(struct bt_data *data, void *user_data)
{
	bt_addr_le_t *addr = user_data;
	int i;

	printk("[AD]: %u data_len %u\n", data->type, data->data_len);

	switch (data->type) {
	case BT_DATA_UUID16_SOME:
	case BT_DATA_UUID16_ALL:
		if (data->data_len % sizeof(uint16_t) != 0U) {
			printk("AD malformed\n");
			return true; /* Continue */
		}

		for (i = 0; i < data->data_len; i += sizeof(uint16_t)) {
			struct bt_uuid *uuid;
			uint16_t uuid_val;
			int err;

			memcpy(&uuid_val, &data->data[i], sizeof(uuid_val));
			uuid = BT_UUID_DECLARE_16(sys_le16_to_cpu(uuid_val));
			if (bt_uuid_cmp(uuid, BT_UUID_ASCS) != 0) {
				continue;
			}

			err = bt_le_scan_stop();
			if (err != 0) {
				printk("Failed to stop scan: %d\n", err);
				return false;
			}

			printk("Audio server found; connecting\n");

			err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
						BT_LE_CONN_PARAM_DEFAULT,
						&default_conn);
			if (err != 0) {
				printk("Create conn to failed (%u)\n", err);
				start_scan();
			}

			return false; /* Stop parsing */
		}
	}

	return true;
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];

	if (default_conn != NULL) {
		/* Already connected */
		return;
	}

	/* We're only interested in connectable events */
	if (type != BT_GAP_ADV_TYPE_ADV_IND && type != BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
		return;
	}

	(void)bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	printk("Device found: %s (RSSI %d)\n", addr_str, rssi);

	/* connect only to devices in close proximity */
	if (rssi < -70) {
		return;
	}

	bt_data_parse(ad, check_audio_support, (void *)addr);
}

static void start_scan(void)
{
	int err;

	/* This demo doesn't require active scan */
	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err != 0) {
		printk("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");
}

static void chan_connected(struct bt_audio_chan *chan)
{
	printk("Audio Channel %p connected, start sending\n", chan);

	/* Start send timer */
	k_work_schedule(&audio_send_work, K_MSEC(0));
}

static void chan_disconnected(struct bt_audio_chan *chan, uint8_t reason)
{
	printk("Audio Channel %p disconnected (reason 0x%02x)\n", chan, reason);
	k_work_cancel_delayable(&audio_send_work);
}

static struct bt_audio_chan_ops chan_ops = {
	.connected	= chan_connected,
	.disconnected	= chan_disconnected,
};

static void add_remote_sink(struct bt_audio_ep *ep, uint8_t index)
{
	printk("Sink #%u: ep %p\n", index, ep);

	sinks[index] = ep;
}

static void add_remote_capability(struct bt_audio_capability *cap, int index)
{
	printk("#%u: cap %p type 0x%02x\n", index, cap, cap->type);

	print_codec(cap->codec);

	if (cap->type != BT_AUDIO_SINK && cap->type != BT_AUDIO_SOURCE) {
		return;
	}

	if (index < CONFIG_BT_BAP_PAC_COUNT) {
		remote_capabilities[index] = cap;
	}
}

static void discover_sink_cb(struct bt_conn *conn,
			    struct bt_audio_capability *cap,
			    struct bt_audio_ep *ep,
			    struct bt_audio_discover_params *params)
{
	if (params->err != 0) {
		printk("Discovery failed: %d\n", params->err);
		return;
	}

	if (cap != NULL) {
		add_remote_capability(cap, params->num_caps);
		return;
	}

	if (ep != NULL) {
		if (params->type == BT_AUDIO_SINK) {
			add_remote_sink(ep, params->num_eps);
		} else {
			printk("Invalid param type: %u\n", params->type);
		}

		return;
	}

	printk("Discover complete: err %d\n", params->err);

	(void)memset(params, 0, sizeof(*params));

	k_sem_give(&sem_sink_discovered);
}

static void gatt_mtu_cb(struct bt_conn *conn, uint8_t err,
		   struct bt_gatt_exchange_params *params)
{
	if (err != 0) {
		printk("Failed to exchange MTU (%u)\n", err);
		return;
	}

	k_sem_give(&sem_mtu_exchanged);
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	(void)bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err != 0) {
		printk("Failed to connect to %s (%u)\n", addr, err);

		bt_conn_unref(default_conn);
		default_conn = NULL;

		start_scan();
		return;
	}

	if (conn != default_conn) {
		return;
	}

	printk("Connected: %s\n", addr);
	k_sem_give(&sem_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (conn != default_conn) {
		return;
	}

	(void)bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s (reason 0x%02x)\n", addr, reason);

	bt_conn_unref(default_conn);
	default_conn = NULL;

	start_scan();
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static int init(void)
{
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		printk("Bluetooth enable failed (err %d)\n", err);
		return err;
	}

	for (size_t i = 0; i < ARRAY_SIZE(caps); i++) {
		err = bt_audio_capability_register(&caps[i]);
		if (err != 0) {
			printk("Register capabilities[%zu] failed: %d", i, err);
			return err;
		}
	}

	audio_channel.ops = &chan_ops;

	k_work_init_delayable(&audio_send_work, audio_timer_timeout);

	return 0;
}

static int scan_and_connect(void)
{
	int err;

	start_scan();

	err = k_sem_take(&sem_connected, K_FOREVER);
	if (err != 0) {
		printk("failed to take sem_connected (err %d)\n", err);
		return err;
	}

	return 0;
}

static int exchange_mtu(void)
{
	struct bt_gatt_exchange_params mtu_params = {
		.func = gatt_mtu_cb
	};
	int err;

	err = bt_gatt_exchange_mtu(default_conn, &mtu_params);
	if (err != 0) {
		printk("Failed to exchange MTU %d\n", err);
		return err;
	}

	err = k_sem_take(&sem_mtu_exchanged, K_FOREVER);
	if (err != 0) {
		printk("failed to take sem_mtu_exchanged (err %d)\n", err);
		return err;
	}

	return 0;
}

static int discover_sink(void)
{
	static struct bt_audio_discover_params params;
	int err;

	params.func = discover_sink_cb;
	params.type = BT_AUDIO_SINK;

	err = bt_audio_discover(default_conn, &params);
	if (err != 0) {
		printk("Failed to discover sink: %d\n", err);
		return err;
	}

	err = k_sem_take(&sem_sink_discovered, K_FOREVER);
	if (err != 0) {
		printk("failed to take sem_sink_discovered (err %d)\n", err);
		return err;
	}

	return 0;
}

static int configure_chan(struct bt_audio_chan **chan)
{
	int err;

	*chan = bt_audio_chan_config(default_conn, sinks[0],
				     remote_capabilities[0],
				     &preset_16_2_1.codec);
	if (*chan == NULL) {
		printk("Could not configure channel\n");
		return -ENOENT;
	}

	err = k_sem_take(&sem_chan_configured, K_FOREVER);
	if (err != 0) {
		printk("failed to take sem_chan_configured (err %d)\n", err);
		return err;
	}

	return 0;
}

static int create_group(struct bt_audio_chan *chan)
{
	int err;

	err = bt_audio_unicast_group_create(chan, 1, &unicast_group);
	if (err != 0) {
		printk("Could not create unicast group (err %d)\n", err);
		return err;
	}

	return 0;
}

static int set_chan_qos(void)
{
	int err;

	err = bt_audio_chan_qos(default_conn, unicast_group,
				&preset_16_2_1.qos);
	if (err != 0) {
		printk("Unable to setup QoS: %d", err);
		return err;
	}

	err = k_sem_take(&sem_chan_qos, K_FOREVER);
	if (err != 0) {
		printk("failed to take sem_chan_qos (err %d)\n", err);
		return err;
	}

	return 0;
}

static int enable_chan(struct bt_audio_chan *chan)
{
	int err;

	err = bt_audio_chan_enable(chan, preset_16_2_1.codec.meta_count,
				   preset_16_2_1.codec.meta);
	if (err != 0) {
		printk("Unable to enable channel: %d", err);
		return err;
	}

	err = k_sem_take(&sem_chan_enabled, K_FOREVER);
	if (err != 0) {
		printk("failed to take sem_chan_enabled (err %d)\n", err);
		return err;
	}

	return 0;
}

void main(void)
{
	struct bt_audio_chan *chan;
	int err;

	printk("Initializing\n");
	err = init();
	if (err != 0) {
		return;
	}
	printk("Initialized\n");

	printk("Waiting for connection\n");
	err = scan_and_connect();
	if (err != 0) {
		return;
	}
	printk("Connected\n");

	printk("Initiating MTU exchange\n");
	err = exchange_mtu();
	if (err != 0) {
		return;
	}
	printk("MTU exchanged\n");

	printk("Discovering sink\n");
	err = discover_sink();
	if (err != 0) {
		return;
	}
	printk("Sink discovered\n");

	printk("Configuring channel\n");
	err = configure_chan(&chan);
	if (err != 0) {
		return;
	}
	printk("Channel configured\n");

	printk("Creating unicast group\n");
	err = create_group(chan);
	if (err != 0) {
		return;
	}
	printk("Unicast group created\n");

	printk("Setting channel QoS\n");
	err = set_chan_qos();
	if (err != 0) {
		return;
	}
	printk("Channel QoS Set\n");

	printk("Enabling channel\n");
	err = enable_chan(chan);
	if (err != 0) {
		return;
	}
	printk("Channel enabled\n");
}
