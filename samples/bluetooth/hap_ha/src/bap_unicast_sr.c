/** @file
 *  @brief Bluetooth Basic Audio Profile (BAP) Unicast Server role.
 *
 *  Copyright (c) 2021-2023 Nordic Semiconductor ASA
 *  Copyright (c) 2022 Codecoup
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/pacs.h>

NET_BUF_POOL_FIXED_DEFINE(tx_pool, CONFIG_BT_ASCS_ASE_SRC_COUNT,
			  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static struct bt_codec lc3_codec =
	BT_CODEC_LC3(BT_CODEC_LC3_FREQ_16KHZ | BT_CODEC_LC3_FREQ_24KHZ,
		     BT_CODEC_LC3_DURATION_10,
		     BT_CODEC_LC3_CHAN_COUNT_SUPPORT(1), 40u, 60u, 1u,
		     (BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BT_AUDIO_CONTEXT_TYPE_MEDIA));

static struct bt_conn *default_conn;
static struct k_work_delayable audio_send_work;
static struct bt_bap_stream streams[CONFIG_BT_ASCS_ASE_SNK_COUNT + CONFIG_BT_ASCS_ASE_SRC_COUNT];
static struct audio_source {
	struct bt_bap_stream *stream;
	uint16_t seq_num;
} source_streams[CONFIG_BT_ASCS_ASE_SRC_COUNT];
static size_t configured_source_stream_count;

static const struct bt_codec_qos_pref qos_pref = BT_CODEC_QOS_PREF(true, BT_GAP_LE_PHY_2M, 0x02,
								   10, 20000, 40000, 20000, 40000);

static uint16_t get_and_incr_seq_num(const struct bt_bap_stream *stream)
{
	for (size_t i = 0U; i < configured_source_stream_count; i++) {
		if (stream == source_streams[i].stream) {
			return source_streams[i].seq_num++;
		}
	}

	printk("Could not find endpoint from stream %p\n", stream);

	return 0;
}

static void print_hex(const uint8_t *ptr, size_t len)
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

	if (codec->id == BT_CODEC_LC3_ID) {
		/* LC3 uses the generic LTV format - other codecs might do as well */

		enum bt_audio_location chan_allocation;

		printk("  Frequency: %d Hz\n", bt_codec_cfg_get_freq(codec));
		printk("  Frame Duration: %d us\n", bt_codec_cfg_get_frame_duration_us(codec));
		if (bt_codec_cfg_get_chan_allocation_val(codec, &chan_allocation) == 0) {
			printk("  Channel allocation: 0x%x\n", chan_allocation);
		}

		printk("  Octets per frame: %d (negative means value not pressent)\n",
		       bt_codec_cfg_get_octets_per_frame(codec));
		printk("  Frames per SDU: %d\n",
		       bt_codec_cfg_get_frame_blocks_per_sdu(codec, true));
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

static void print_qos(const struct bt_codec_qos *qos)
{
	printk("QoS: interval %u framing 0x%02x phy 0x%02x sdu %u "
	       "rtn %u latency %u pd %u\n",
	       qos->interval, qos->framing, qos->phy, qos->sdu,
	       qos->rtn, qos->latency, qos->pd);
}

/**
 * @brief Send audio data on timeout
 *
 * This will send an increasing amount of audio data, starting from 1 octet.
 * The data is just mock data, and does not actually represent any audio.
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
		for (size_t i = 0U; i < ARRAY_SIZE(buf_data); i++) {
			buf_data[i] = (uint8_t)i;
		}

		data_initialized = true;
	}

	/* We configured the sink streams to be first in `streams`, so that
	 * we can use `stream[i]` to select sink streams (i.e. streams with
	 * data going to the server)
	 */
	for (size_t i = 0; i < configured_source_stream_count; i++) {
		struct bt_bap_stream *stream = source_streams[i].stream;

		buf = net_buf_alloc(&tx_pool, K_FOREVER);
		net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);

		net_buf_add_mem(buf, buf_data, len_to_send);

		ret = bt_bap_stream_send(stream, buf,
					   get_and_incr_seq_num(stream),
					   BT_ISO_TIMESTAMP_NONE);
		if (ret < 0) {
			printk("Failed to send audio data on streams[%zu] (%p): (%d)\n",
			       i, stream, ret);
			net_buf_unref(buf);
		} else {
			printk("Sending mock data with len %zu on streams[%zu] (%p)\n",
			       len_to_send, i, stream);
		}
	}

	k_work_schedule(&audio_send_work, K_MSEC(1000U));

	len_to_send++;
	if (len_to_send > ARRAY_SIZE(buf_data)) {
		len_to_send = 1;
	}
}

static struct bt_bap_stream *stream_alloc(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(streams); i++) {
		struct bt_bap_stream *stream = &streams[i];

		if (!stream->conn) {
			return stream;
		}
	}

	return NULL;
}

static int lc3_config(struct bt_conn *conn, const struct bt_bap_ep *ep, enum bt_audio_dir dir,
		      const struct bt_codec *codec, struct bt_bap_stream **stream,
		      struct bt_codec_qos_pref *const pref, struct bt_bap_ascs_rsp *rsp)
{
	printk("ASE Codec Config: conn %p ep %p dir %u\n", conn, ep, dir);

	print_codec(codec);

	*stream = stream_alloc();
	if (*stream == NULL) {
		printk("No streams available\n");
		*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_NO_MEM, BT_BAP_ASCS_REASON_NONE);
		return -ENOMEM;
	}

	printk("ASE Codec Config stream %p\n", *stream);

	if (dir == BT_AUDIO_DIR_SOURCE) {
		source_streams[configured_source_stream_count++].stream = *stream;
	}

	*pref = qos_pref;

	return 0;
}

static int lc3_reconfig(struct bt_bap_stream *stream, enum bt_audio_dir dir,
			const struct bt_codec *codec, struct bt_codec_qos_pref *const pref,
			struct bt_bap_ascs_rsp *rsp)
{
	printk("ASE Codec Reconfig: stream %p\n", stream);

	print_codec(codec);

	*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_CONF_UNSUPPORTED, BT_BAP_ASCS_REASON_NONE);

	/* We only support one QoS at the moment, reject changes */
	return -ENOEXEC;
}

static int lc3_qos(struct bt_bap_stream *stream, const struct bt_codec_qos *qos,
		   struct bt_bap_ascs_rsp *rsp)
{
	printk("QoS: stream %p qos %p\n", stream, qos);

	print_qos(qos);

	return 0;
}

static int lc3_enable(struct bt_bap_stream *stream, const struct bt_codec_data *meta,
		      size_t meta_count, struct bt_bap_ascs_rsp *rsp)
{
	printk("Enable: stream %p meta_count %u\n", stream, meta_count);

	return 0;
}

static int lc3_start(struct bt_bap_stream *stream, struct bt_bap_ascs_rsp *rsp)
{
	printk("Start: stream %p\n", stream);

	if (IS_ENABLED(CONFIG_BT_ASCS_ASE_SRC)) {
		for (size_t i = 0U; i < configured_source_stream_count; i++) {
			if (source_streams[i].stream == stream) {
				source_streams[i].seq_num = 0U;
				break;
			}
		}

		if (configured_source_stream_count > 0 &&
		!k_work_delayable_is_pending(&audio_send_work)) {

			/* Start send timer */
			k_work_schedule(&audio_send_work, K_MSEC(0));
		}
	}

	return 0;
}

static bool valid_metadata_type(uint8_t type, uint8_t len)
{
	switch (type) {
	case BT_AUDIO_METADATA_TYPE_PREF_CONTEXT:
	case BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT:
		if (len != 2) {
			return false;
		}

		return true;
	case BT_AUDIO_METADATA_TYPE_STREAM_LANG:
		if (len != 3) {
			return false;
		}

		return true;
	case BT_AUDIO_METADATA_TYPE_PARENTAL_RATING:
		if (len != 1) {
			return false;
		}

		return true;
	case BT_AUDIO_METADATA_TYPE_EXTENDED: /* 1 - 255 octets */
	case BT_AUDIO_METADATA_TYPE_VENDOR: /* 1 - 255 octets */
		if (len < 1) {
			return false;
		}

		return true;
	case BT_AUDIO_METADATA_TYPE_CCID_LIST: /* 2 - 254 octets */
		if (len < 2) {
			return false;
		}

		return true;
	case BT_AUDIO_METADATA_TYPE_PROGRAM_INFO: /* 0 - 255 octets */
	case BT_AUDIO_METADATA_TYPE_PROGRAM_INFO_URI: /* 0 - 255 octets */
		return true;
	default:
		return false;
	}
}

static int lc3_metadata(struct bt_bap_stream *stream, const struct bt_codec_data *meta,
			size_t meta_count, struct bt_bap_ascs_rsp *rsp)
{
	printk("Metadata: stream %p meta_count %u\n", stream, meta_count);

	for (size_t i = 0; i < meta_count; i++) {
		const struct bt_codec_data *data = &meta[i];

		if (!valid_metadata_type(data->data.type, data->data.data_len)) {
			printk("Invalid metadata type %u or length %u\n",
			       data->data.type, data->data.data_len);
			*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_METADATA_REJECTED,
					       data->data.type);

			return -EINVAL;
		}
	}

	return 0;
}

static int lc3_disable(struct bt_bap_stream *stream, struct bt_bap_ascs_rsp *rsp)
{
	printk("Disable: stream %p\n", stream);

	return 0;
}

static int lc3_stop(struct bt_bap_stream *stream, struct bt_bap_ascs_rsp *rsp)
{
	printk("Stop: stream %p\n", stream);

	return 0;
}

static int lc3_release(struct bt_bap_stream *stream, struct bt_bap_ascs_rsp *rsp)
{
	printk("Release: stream %p\n", stream);
	return 0;
}

static const struct bt_bap_unicast_server_cb unicast_server_cb = {
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

static void stream_recv(struct bt_bap_stream *stream, const struct bt_iso_recv_info *info,
			struct net_buf *buf)
{
	printk("Incoming audio on stream %p len %u\n", stream, buf->len);
}

static struct bt_bap_stream_ops stream_ops = {
	.recv = stream_recv
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err != 0) {
		printk("Failed to connect to %s (%u)\n", addr, err);

		default_conn = NULL;
		return;
	}

	printk("Connected: %s\n", addr);
	default_conn = bt_conn_ref(conn);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];
	struct k_work_sync sync;

	if (conn != default_conn) {
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s (reason 0x%02x)\n", addr, reason);

	bt_conn_unref(default_conn);
	default_conn = NULL;

	if (IS_ENABLED(CONFIG_BT_ASCS_ASE_SRC)) {
		/* reset data */
		(void)memset(source_streams, 0, sizeof(source_streams));
		configured_source_stream_count = 0U;
		k_work_cancel_delayable_sync(&audio_send_work, &sync);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static struct bt_pacs_cap cap_sink = {
	.codec = &lc3_codec,
};

static struct bt_pacs_cap cap_source = {
	.codec = &lc3_codec,
};

int bap_unicast_sr_init(void)
{
	bt_bap_unicast_server_register_cb(&unicast_server_cb);

	bt_pacs_cap_register(BT_AUDIO_DIR_SINK, &cap_sink);

	if (IS_ENABLED(CONFIG_BT_PAC_SNK_LOC)) {
		if (IS_ENABLED(CONFIG_HAP_HA_HEARING_AID_BANDED)) {
			/* HAP_d1.0r00; 3.7 BAP Unicast Server role requirements
			 * A Banded Hearing Aid in the HA role shall set the
			 * Front Left and the Front Right bits to a value of 0b1
			 * in the Sink Audio Locations characteristic value.
			 */
			bt_pacs_set_location(BT_AUDIO_DIR_SINK,
					     (BT_AUDIO_LOCATION_FRONT_LEFT |
					      BT_AUDIO_LOCATION_FRONT_RIGHT));
		} else if (IS_ENABLED(CONFIG_HAP_HA_HEARING_AID_LEFT)) {
			bt_pacs_set_location(BT_AUDIO_DIR_SINK,
					     BT_AUDIO_LOCATION_FRONT_LEFT);
		} else {
			bt_pacs_set_location(BT_AUDIO_DIR_SINK,
					     BT_AUDIO_LOCATION_FRONT_RIGHT);
		}
	}

	if (IS_ENABLED(CONFIG_BT_ASCS_ASE_SRC)) {
		bt_pacs_cap_register(BT_AUDIO_DIR_SOURCE, &cap_source);
	}

	for (size_t i = 0; i < ARRAY_SIZE(streams); i++) {
		bt_bap_stream_cb_register(&streams[i], &stream_ops);
	}

	if (IS_ENABLED(CONFIG_BT_ASCS_ASE_SRC)) {
		k_work_init_delayable(&audio_send_work, audio_timer_timeout);
	}

	return 0;
}
