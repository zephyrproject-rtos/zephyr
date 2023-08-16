/** @file
 *  @brief Bluetooth Basic Audio Profile (BAP) Unicast Server role.
 *
 *  Copyright (c) 2021-2023 Nordic Semiconductor ASA
 *  Copyright (c) 2022 Codecoup
 *  Copyright (c) 2023 NXP
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/pacs.h>
#include <zephyr/bluetooth/audio/tbs.h>

#define AVAILABLE_SINK_CONTEXT CONFIG_BT_PACS_SNK_CONTEXT
#define AVAILABLE_SOURCE_CONTEXT CONFIG_BT_PACS_SRC_CONTEXT

static const struct bt_audio_codec_cap lc3_codec_cap =
	BT_AUDIO_CODEC_CAP_LC3(BT_AUDIO_CODEC_LC3_FREQ_16KHZ | BT_AUDIO_CODEC_LC3_FREQ_32KHZ |
				       BT_AUDIO_CODEC_LC3_FREQ_48KHZ,
			       BT_AUDIO_CODEC_LC3_DURATION_7_5 | BT_AUDIO_CODEC_LC3_DURATION_10,
			       BT_AUDIO_CODEC_LC3_CHAN_COUNT_SUPPORT(2), 30, 155u, 1u,
			       (CONFIG_BT_PACS_SNK_CONTEXT | CONFIG_BT_PACS_SRC_CONTEXT));

static struct bt_conn *default_conn;
static struct bt_bap_stream streams[CONFIG_BT_ASCS_ASE_SNK_COUNT + CONFIG_BT_ASCS_ASE_SRC_COUNT];
static struct audio_source {
	struct bt_bap_stream *stream;
	uint16_t seq_num;
} source_streams[CONFIG_BT_ASCS_ASE_SRC_COUNT];
static size_t configured_source_stream_count;

static const struct bt_audio_codec_qos_pref qos_pref =
	BT_AUDIO_CODEC_QOS_PREF(true, BT_GAP_LE_PHY_2M, 0x02, 10, 20000, 40000, 20000, 40000);

static void print_hex(const uint8_t *ptr, size_t len)
{
	while (len-- != 0) {
		printk("%02x", *ptr++);
	}
}

static void print_codec_cfg(const struct bt_audio_codec_cfg *codec_cfg)
{
	printk("codec_cfg 0x%02x cid 0x%04x vid 0x%04x count %u\n", codec_cfg->id, codec_cfg->cid,
	       codec_cfg->vid, codec_cfg->data_count);

	for (size_t i = 0; i < codec_cfg->data_count; i++) {
		printk("data #%zu: type 0x%02x len %u\n", i, codec_cfg->data[i].data.type,
		       codec_cfg->data[i].data.data_len);
		print_hex(codec_cfg->data[i].data.data,
			  codec_cfg->data[i].data.data_len - sizeof(codec_cfg->data[i].data.type));
		printk("\n");
	}

	if (codec_cfg->id == BT_AUDIO_CODEC_LC3_ID) {
		/* LC3 uses the generic LTV format - other codecs might do as well */

		uint32_t chan_allocation;

		printk("  Frequency: %d Hz\n", bt_audio_codec_cfg_get_freq(codec_cfg));
		printk("  Frame Duration: %d us\n",
		       bt_audio_codec_cfg_get_frame_duration_us(codec_cfg));
		if (bt_audio_codec_cfg_get_chan_allocation_val(codec_cfg, &chan_allocation) == 0) {
			printk("  Channel allocation: 0x%x\n", chan_allocation);
		}

		printk("  Octets per frame: %d (negative means value not pressent)\n",
		       bt_audio_codec_cfg_get_octets_per_frame(codec_cfg));
		printk("  Frames per SDU: %d\n",
		       bt_audio_codec_cfg_get_frame_blocks_per_sdu(codec_cfg, true));
	}

	for (size_t i = 0; i < codec_cfg->meta_count; i++) {
		printk("meta #%zu: type 0x%02x len %u\n", i, codec_cfg->meta[i].data.type,
		       codec_cfg->meta[i].data.data_len);
		print_hex(codec_cfg->meta[i].data.data,
			  codec_cfg->meta[i].data.data_len - sizeof(codec_cfg->meta[i].data.type));
		printk("\n");
	}
}

static void print_qos(const struct bt_audio_codec_qos *qos)
{
	printk("QoS: interval %u framing 0x%02x phy 0x%02x sdu %u "
	       "rtn %u latency %u pd %u\n",
	       qos->interval, qos->framing, qos->phy, qos->sdu,
	       qos->rtn, qos->latency, qos->pd);
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
		      const struct bt_audio_codec_cfg *codec_cfg, struct bt_bap_stream **stream,
		      struct bt_audio_codec_qos_pref *const pref, struct bt_bap_ascs_rsp *rsp)
{

	printk("ASE Codec Config: conn %p ep %p dir %u\n", conn, ep, dir);

	print_codec_cfg(codec_cfg);

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
			const struct bt_audio_codec_cfg *codec_cfg,
			struct bt_audio_codec_qos_pref *const pref, struct bt_bap_ascs_rsp *rsp)
{
	printk("ASE Codec Reconfig: stream %p\n", stream);
	print_codec_cfg(codec_cfg);
	*pref = qos_pref;

	return 0;
}

static int lc3_qos(struct bt_bap_stream *stream, const struct bt_audio_codec_qos *qos,
		   struct bt_bap_ascs_rsp *rsp)
{
	printk("QoS: stream %p qos %p\n", stream, qos);

	print_qos(qos);

	return 0;
}

static int lc3_enable(struct bt_bap_stream *stream, const struct bt_audio_codec_data *meta,
		      size_t meta_count, struct bt_bap_ascs_rsp *rsp)
{
	printk("Enable: stream %p meta_count %u\n", stream, meta_count);

	return 0;
}

static int lc3_start(struct bt_bap_stream *stream, struct bt_bap_ascs_rsp *rsp)
{
	printk("Start: stream %p\n", stream);

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
	case BT_AUDIO_METADATA_TYPE_EXTENDED: /* 2 - 255 octets */
	case BT_AUDIO_METADATA_TYPE_VENDOR: /* 2 - 255 octets */
		/* At least Extended Metadata Type / Company_ID should be there */
		if (len < 2) {
			return false;
		}

		return true;
	case BT_AUDIO_METADATA_TYPE_CCID_LIST:
	case BT_AUDIO_METADATA_TYPE_PROGRAM_INFO: /* 0 - 255 octets */
	case BT_AUDIO_METADATA_TYPE_PROGRAM_INFO_URI: /* 0 - 255 octets */
		return true;
	default:
		return false;
	}
}

static int lc3_metadata(struct bt_bap_stream *stream, const struct bt_audio_codec_data *meta,
			size_t meta_count, struct bt_bap_ascs_rsp *rsp)
{
	printk("Metadata: stream %p meta_count %u\n", stream, meta_count);
	bool stream_context_present = false;

	for (size_t i = 0; i < meta_count; i++) {
		const struct bt_audio_codec_data *data = &meta[i];

		if (!valid_metadata_type(data->data.type, data->data.data_len)) {
			printk("Invalid metadata type %u or length %u\n",
			       data->data.type, data->data.data_len);
			*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_METADATA_REJECTED,
					       data->data.type);

			return -EINVAL;
		}

		if (data->data.type == BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT) {
			stream_context_present = true;
		}

		if (data->data.type == BT_AUDIO_METADATA_TYPE_CCID_LIST) {
			for (uint8_t i = 0; i < data->data.data_len; i++) {
				const uint8_t ccid = data->data.data[i];

				if (!(IS_ENABLED(CONFIG_BT_TBS_CLIENT_CCID) &&
					bt_tbs_client_get_by_ccid(default_conn, ccid) != NULL)) {
					printk("CCID %u is unknown", ccid);
					*rsp = BT_BAP_ASCS_RSP(
						BT_BAP_ASCS_RSP_CODE_METADATA_REJECTED,
						BT_BAP_ASCS_REASON_NONE);

					return -EINVAL;
				}
			}
		}
	}

	if (stream_context_present == false) {
		printk("Stream audio context not present on peer!");
		*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_METADATA_REJECTED,
				       BT_BAP_ASCS_REASON_NONE);

		return -EINVAL;
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
	if (buf->len != 0) {
		printk("Incoming audio on stream %p len %u\n", stream, buf->len);
	}
	/* TODO: decode data (if applicable) */
}

static void stream_enabled(struct bt_bap_stream *stream)
{
	const int err = bt_bap_stream_start(stream);

	if (err != 0) {
		printk("Failed to start stream %p: %d", stream, err);
	}
}

static struct bt_bap_stream_ops stream_ops = {
	.recv = stream_recv,
	.enabled = stream_enabled
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	default_conn = bt_conn_ref(conn);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	if (conn != default_conn) {
		return;
	}

	bt_conn_unref(default_conn);
	default_conn = NULL;

	if (IS_ENABLED(CONFIG_BT_ASCS_ASE_SRC)) {
		/* reset data */
		(void)memset(source_streams, 0, sizeof(source_streams));
		configured_source_stream_count = 0U;
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static struct bt_pacs_cap cap = {
	.codec_cap = &lc3_codec_cap,
};

int bap_unicast_sr_init(void)
{
	bt_bap_unicast_server_register_cb(&unicast_server_cb);

	if (IS_ENABLED(CONFIG_BT_PAC_SNK_LOC)) {
		/* Register CT required capabilities */
		bt_pacs_cap_register(BT_AUDIO_DIR_SINK, &cap);

		if (IS_ENABLED(CONFIG_TMAP_PERIPHERAL_LEFT)) {
			bt_pacs_set_location(BT_AUDIO_DIR_SINK, BT_AUDIO_LOCATION_FRONT_LEFT);
		} else {
			bt_pacs_set_location(BT_AUDIO_DIR_SINK, BT_AUDIO_LOCATION_FRONT_RIGHT);
		}

		bt_pacs_set_supported_contexts(BT_AUDIO_DIR_SINK,
					       AVAILABLE_SINK_CONTEXT);
		bt_pacs_set_available_contexts(BT_AUDIO_DIR_SINK,
					       AVAILABLE_SINK_CONTEXT);
	}

	if (IS_ENABLED(CONFIG_BT_ASCS_ASE_SRC)) {
		/* Register CT required capabilities */
		bt_pacs_cap_register(BT_AUDIO_DIR_SOURCE, &cap);

		if (IS_ENABLED(CONFIG_TMAP_PERIPHERAL_LEFT)) {
			bt_pacs_set_location(BT_AUDIO_DIR_SOURCE, BT_AUDIO_LOCATION_FRONT_LEFT);
		} else {
			bt_pacs_set_location(BT_AUDIO_DIR_SOURCE, BT_AUDIO_LOCATION_FRONT_RIGHT);
		}

		bt_pacs_set_supported_contexts(BT_AUDIO_DIR_SOURCE,
					       AVAILABLE_SOURCE_CONTEXT);
		bt_pacs_set_available_contexts(BT_AUDIO_DIR_SOURCE,
					       AVAILABLE_SOURCE_CONTEXT);
	}

	for (size_t i = 0; i < ARRAY_SIZE(streams); i++) {
		bt_bap_stream_cb_register(&streams[i], &stream_ops);
	}

	return 0;
}
