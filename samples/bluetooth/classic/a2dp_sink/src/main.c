/*
 * Copyright (c) 2025 SiFli Technologies(Nanjing) Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/classic/a2dp.h>
#include <zephyr/bluetooth/classic/avrcp.h>
#include <zephyr/bluetooth/classic/a2dp_codec_sbc.h>
#include <zephyr/bluetooth/classic/classic.h>
#include <zephyr/bluetooth/sbc.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/audio/codec.h>

LOG_MODULE_REGISTER(a2dp_sink_codec, LOG_LEVEL_INF);

/* Audio Configuration */
#define CODEC_BLOCK_SIZE    1600
#define DECODE_OUT_BUF_SIZE 4096
#define DECODE_IN_BUF_SIZE  1024
#define DECODE_THRESHOLD    167

#define SPEAKER_VOL   15
#define RING_BUF_SIZE (32000)

static uint32_t a2dp_samplerate;
static uint32_t a2dp_channels;
static uint8_t is_codec_opened;

/* Audio Buffers */
static uint8_t ring_buffer[RING_BUF_SIZE];
static struct ring_buf rb;

static uint32_t decode_in_offset;
static uint8_t decode_in_buf[DECODE_IN_BUF_SIZE];
static int16_t decode_out_buf[DECODE_OUT_BUF_SIZE / sizeof(int16_t)];
static volatile bool is_playing;

/* A2DP Variables */
static struct bt_a2dp_stream a2dp_stream;
static struct bt_a2dp *active_a2dp;
static struct bt_conn *active_bt_conn;

/* SBC decoder*/
static struct sbc_decoder decoder;

static inline void stereo2mono(int16_t *stereo, uint32_t samples, int16_t *mono)
{
	size_t mono_samples = samples / 2;

	for (size_t i = 0; i < mono_samples; i++) {
		int32_t left = *stereo++;
		int32_t right = *stereo++;
		*mono++ = (int16_t)((left + right) / 2);
	}
}

#if DT_NODE_HAS_STATUS_OKAY(DT_ALIAS(codec0))
static uint8_t block_data[CODEC_BLOCK_SIZE];
static const struct device *codec_dev;

static void tx_done(const struct device *dev, void *user_data)
{
	if (ring_buf_size_get(&rb) < CODEC_BLOCK_SIZE) {
		LOG_WRN("buffer empty\n");
	}
	ring_buf_get(&rb, block_data, CODEC_BLOCK_SIZE);
	audio_codec_write(dev, block_data, CODEC_BLOCK_SIZE);
}

static void codec_open(uint32_t samplerate)
{
	audio_property_value_t val;
	struct audio_codec_cfg cfg = {
		.dai_type = AUDIO_DAI_TYPE_PCM,
		.dai_cfg.pcm.dir = AUDIO_DAI_DIR_TX,
		.dai_cfg.pcm.pcm_width = AUDIO_PCM_WIDTH_16_BITS,
		.dai_cfg.pcm.channels = 1,
		.dai_cfg.pcm.block_size = CODEC_BLOCK_SIZE,
		.dai_cfg.pcm.samplerate = a2dp_samplerate,
	};

	LOG_INF("codec: samplerate=%d block_size=%d\n", a2dp_samplerate, CODEC_BLOCK_SIZE);

	audio_codec_register_done_callback(codec_dev, tx_done, NULL, NULL, NULL);
	audio_codec_configure(codec_dev, &cfg);
	audio_codec_start(codec_dev, AUDIO_DAI_DIR_TX);
	val.vol = SPEAKER_VOL;
	audio_codec_set_property(codec_dev, AUDIO_PROPERTY_OUTPUT_VOLUME, 0, val);
}

static void codec_close(void)
{
	if (is_codec_opened) {
		audio_codec_stop(codec_dev, AUDIO_DAI_DIR_TX);
		is_codec_opened = 0;
	}
}
#else
#define codec_open(samplerate)
#define codec_close()
#endif

/*  -------------------------- A2DP Stream Operations -------------------------- */

static uint32_t get_sample_rate(uint8_t codec_ie0)
{
	uint32_t samplerate = 44100;

	if (0U != (codec_ie0 & A2DP_SBC_SAMP_FREQ_16000)) {
		LOG_INF("	16000 ");
		samplerate = 16000;
	} else if (0U != (codec_ie0 & A2DP_SBC_SAMP_FREQ_32000)) {
		LOG_INF("	32000 ");
		samplerate = 32000;
	} else if (0U != (codec_ie0 & A2DP_SBC_SAMP_FREQ_44100)) {
		LOG_INF("	44100 ");
		samplerate = 44100;
	} else if (0U != (codec_ie0 & A2DP_SBC_SAMP_FREQ_48000)) {
		LOG_INF("	48000");
		samplerate = 48000;
	} else {
		LOG_WRN("   unknown,default to 44.1k");
	}
	return samplerate;
}

static uint32_t get_channels(uint8_t codec_ie0)
{
	uint32_t channels = 2;

	if (0U != (codec_ie0 & A2DP_SBC_CH_MODE_MONO)) {
		LOG_INF("	Mono ");
		channels = 1;
	} else if (0U != (codec_ie0 & A2DP_SBC_CH_MODE_DUAL)) {
		LOG_INF("	Dual ");
	} else if (0U != (codec_ie0 & A2DP_SBC_CH_MODE_STEREO)) {
		LOG_INF("	Stereo ");
	} else if (0U != (codec_ie0 & A2DP_SBC_CH_MODE_JOINT)) {
		LOG_INF("	Joint-Stereo");
	} else {
		LOG_ERR("	Unknown");
	}
	return channels;
}

static void a2dp_stream_configured(struct bt_a2dp_stream *stream)
{
	if (BT_A2DP_SBC == stream->local_ep->codec_type) {
		uint8_t *codec_ie = stream->codec_config.codec_ie;
		uint16_t codec_ie_len = stream->codec_config.len;

		LOG_INF("  codec type: SBC");
		if (BT_A2DP_SBC_IE_LENGTH != codec_ie_len) {
			LOG_ERR("  wrong sbc codec ie");
			return;
		}

		LOG_INF("  sample frequency:");
		a2dp_samplerate = get_sample_rate(codec_ie[0U]);

		LOG_INF("  channel mode:");
		a2dp_channels = get_channels(codec_ie[0U]);

		/* Decode Support for Block Length */
		LOG_INF("  Block Length:");
		if (0U != (codec_ie[1U] & A2DP_SBC_BLK_LEN_4)) {
			LOG_INF("	4 ");
		} else if (0U != (codec_ie[1U] & A2DP_SBC_BLK_LEN_8)) {
			LOG_INF("	8 ");
		} else if (0U != (codec_ie[1U] & A2DP_SBC_BLK_LEN_12)) {
			LOG_INF("	12 ");
		} else if (0U != (codec_ie[1U] & A2DP_SBC_BLK_LEN_16)) {
			LOG_INF("	16");
		} else {
			LOG_ERR("	Unknown");
		}

		/* Decode Support for Subbands */
		LOG_INF("  Subbands:");
		if (0U != (codec_ie[1U] & A2DP_SBC_SUBBAND_4)) {
			LOG_INF("	4 ");
		} else if (0U != (codec_ie[1U] & A2DP_SBC_SUBBAND_8)) {
			LOG_INF("	8");
		} else {
			LOG_ERR("	Unknown");
		}

		/* Decode Support for Allocation Method */
		LOG_INF("  Allocation Method:");
		if (0U != (codec_ie[1U] & A2DP_SBC_ALLOC_MTHD_SNR)) {
			LOG_INF("	SNR ");
		} else if (0U != (codec_ie[1U] & A2DP_SBC_ALLOC_MTHD_LOUDNESS)) {
			LOG_INF("	Loudness");
		} else {
			LOG_ERR("	Unknown");
		}

		LOG_INF("  Bitpool Range: %d - %d", codec_ie[2U], codec_ie[3U]);
		sbc_setup_decoder(&decoder);
	} else {
		LOG_ERR("  not SBC codecs");
	}
}

static void a2dp_stream_established(struct bt_a2dp_stream *stream)
{
	LOG_INF("A2DP stream established");
}

static void a2dp_stream_released(struct bt_a2dp_stream *stream)
{
	LOG_INF("A2DP stream released");
	is_playing = false;

	active_a2dp = NULL;
	active_bt_conn = NULL;
}

static void a2dp_stream_started(struct bt_a2dp_stream *stream)
{
	LOG_INF("A2DP stream started");
	is_playing = true;
}

static void a2dp_stream_suspended(struct bt_a2dp_stream *stream)
{
	LOG_INF("A2DP stream suspended");
	is_playing = false;
	codec_close();
}

static void a2dp_stream_recv(struct bt_a2dp_stream *stream, struct net_buf *buf, uint16_t seq_num,
			     uint32_t ts)
{
	ARG_UNUSED(stream);
	ARG_UNUSED(seq_num);
	ARG_UNUSED(ts);

	int ret;

	/* Only copy data if the target buffer is empty */
	if (ring_buf_space_get(&rb) < DECODE_OUT_BUF_SIZE) {
		LOG_WRN("decode no space, drop %zu bytes\n", buf->len);
		return;
	}
	if (buf->len + decode_in_offset > DECODE_IN_BUF_SIZE) {
		LOG_WRN("decode in buf too small, need big DECODE_IN_BUF_SIZE big\n");
		return;
	}
	memcpy(&decode_in_buf[decode_in_offset], buf->data, buf->len);
	decode_in_offset += buf->len;

	/* Prepare output parameters */
	uint32_t out_size = DECODE_OUT_BUF_SIZE;

	while (decode_in_offset >= DECODE_THRESHOLD) {
		const void *in_data = decode_in_buf; /* Pointer to start of SBC frame */
		uint32_t in_size = decode_in_offset; /* Size of SBC frame in bytes */

		/* Decode the SBC frame using your custom decoder */
		ret = sbc_decode(&decoder, &in_data, &in_size, decode_out_buf, &out_size);
		LOG_DBG("ret=%d decode_in_offset=%d in_size=%d, out_size = %d\n", ret,
			decode_in_offset, in_size, out_size);

		if (ret != 0) {
			LOG_ERR("SBC decode failed: %d", ret);
			break;
		}
		memcpy(decode_in_buf, &decode_in_buf[decode_in_offset - in_size], in_size);
		decode_in_offset = in_size;

#if DT_NODE_HAS_STATUS_OKAY(DT_ALIAS(codec0))
		uint32_t putted;
		/* Decoding succeeded: process PCM data (e.g., send to DAC) */
		if (a2dp_channels == 2) {
			/*Convert to mono channel and play*/
			static int16_t
				mono_buf[DECODE_OUT_BUF_SIZE / sizeof(DECODE_OUT_BUF_SIZE) / 2];
			stereo2mono(decode_out_buf, out_size / 2, mono_buf);
			putted = ring_buf_put(&rb, (const uint8_t *)&mono_buf[0], out_size / 2);
			if (putted != out_size / 2) {
				LOG_WRN("out overflow\n");
			}

		} else {
			putted = ring_buf_put(&rb, (const uint8_t *)&decode_out_buf[0], out_size);
			if (putted != out_size) {
				LOG_WRN("out overflow\n");
			}
		}
#endif

		if (is_codec_opened == 0 && ring_buf_size_get(&rb) >= RING_BUF_SIZE / 2) {
			codec_open(a2dp_samplerate);
			is_codec_opened = 1;
		}
	}
}

static struct bt_a2dp_stream_ops a2dp_stream_ops = {
	.configured = a2dp_stream_configured,
	.established = a2dp_stream_established,
	.released = a2dp_stream_released,
	.started = a2dp_stream_started,
	.suspended = a2dp_stream_suspended,
	.recv = a2dp_stream_recv,
};

/*-------------------------- A2DP Callbacks --------------------------*/

static int a2dp_config_req(struct bt_a2dp *a2dp, struct bt_a2dp_ep *ep,
			   struct bt_a2dp_codec_cfg *codec_cfg, struct bt_a2dp_stream **stream,
			   uint8_t *rsp_err_code)
{
	LOG_INF("A2DP config request");

	*stream = &a2dp_stream;
	bt_a2dp_stream_cb_register(*stream, &a2dp_stream_ops);

	return 0;
}

static int a2dp_start_req(struct bt_a2dp_stream *stream, uint8_t *rsp_err_code)
{
	LOG_INF("A2DP start request");
	return 0;
}

static int a2dp_suspend_req(struct bt_a2dp_stream *stream, uint8_t *rsp_err_code)
{
	LOG_INF("A2DP suspend request");
	return 0;
}

static int a2dp_release_req(struct bt_a2dp_stream *stream, uint8_t *rsp_err_code)
{
	LOG_INF("A2DP release request");
	return 0;
}

static void a2dp_connected(struct bt_a2dp *a2dp, int err)
{
	if (err != 0) {
		LOG_ERR("A2DP connection failed: %d", err);
		return;
	}

	LOG_INF("A2DP connected");
	active_a2dp = a2dp;
	active_bt_conn = bt_a2dp_get_conn(a2dp);
}

static void a2dp_disconnected(struct bt_a2dp *a2dp)
{
	LOG_INF("A2DP disconnected");
	active_a2dp = NULL;
	active_bt_conn = NULL;
	is_playing = false;
}

static struct bt_a2dp_cb a2dp_cb = {
	.connected = a2dp_connected,
	.disconnected = a2dp_disconnected,
	.config_req = a2dp_config_req,
	.start_req = a2dp_start_req,
	.suspend_req = a2dp_suspend_req,
	.release_req = a2dp_release_req,
};

/* -------------------------- AVRCP TG Callbacks -------------------------- */
static void avrcp_tg_connected(struct bt_conn *conn, struct bt_avrcp_tg *tg)
{
	LOG_INF("AVRCP TG connected");
}

static void avrcp_tg_disconnected(struct bt_avrcp_tg *tg)
{
	LOG_INF("AVRCP TG disconnected");
}

static void avrcp_tg_passthrough_req(struct bt_avrcp_tg *tg, uint8_t tid, struct net_buf *buf)
{
	/* Parse pass-through command */
	uint8_t *data = buf->data;
	uint8_t opid = data[0] & 0x7F;
	uint8_t state = (data[0] >> 7) & 0x01;

	LOG_INF("AVRCP pass-through: opid=0x%02x, state=%d", opid, state);

	if (state == 0) { /* Button pressed */
		switch (opid) {
		case 0x44: /* Play */
			LOG_INF("Play command");
			is_playing = true;
			break;

		case 0x46: /* Pause */
			LOG_INF("Pause command");
			is_playing = false;
			break;

		case 0x45: /* Stop */
			LOG_INF("Stop command");
			is_playing = false;
			break;

		default:
			LOG_DBG("Unhandled AVRCP opid: 0x%02x", opid);
			break;
		}
	}

	/* Send response */
	struct net_buf *rsp = bt_avrcp_create_pdu(NULL);

	if (rsp) {
		net_buf_add_u8(rsp, data[0]); /* Echo back the same data */
		bt_avrcp_tg_send_passthrough_rsp(tg, tid, BT_AVRCP_RSP_ACCEPTED, rsp);
		net_buf_unref(rsp);
	}

	net_buf_unref(buf);
}

static struct bt_avrcp_tg_cb avrcp_tg_cb = {
	.connected = avrcp_tg_connected,
	.disconnected = avrcp_tg_disconnected,
	.passthrough_req = avrcp_tg_passthrough_req,
};

/*-------------------------- A2DP Endpoint Definition -------------------------- */
BT_A2DP_SBC_SINK_EP_DEFAULT(a2dp_sink_ep);

static struct bt_sdp_attribute a2dp_sink_attrs[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(BT_SDP_ATTR_SVCLASS_ID_LIST, BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3), /* 35 03 */
			BT_SDP_DATA_ELEM_LIST(
				{
					BT_SDP_TYPE_SIZE(BT_SDP_UUID16),           /* 19 */
					BT_SDP_ARRAY_16(BT_SDP_AUDIO_SINK_SVCLASS) /* 11 0B */
				},)),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROTO_DESC_LIST, BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 16), /* 35 10 */
		BT_SDP_DATA_ELEM_LIST(
			{BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6), /* 35 06 */
			 BT_SDP_DATA_ELEM_LIST(
				{
					BT_SDP_TYPE_SIZE(BT_SDP_UUID16),    /* 19 */
					BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP) /* 01 00 */
				},
				{
					BT_SDP_TYPE_SIZE(BT_SDP_UINT16),   /* 09 */
					BT_SDP_ARRAY_16(BT_UUID_AVDTP_VAL) /* 00 19 */
				},)},
			{BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6), /* 35 06 */
			BT_SDP_DATA_ELEM_LIST(
				{
					BT_SDP_TYPE_SIZE(BT_SDP_UUID16),   /* 19 */
					BT_SDP_ARRAY_16(BT_UUID_AVDTP_VAL) /* 00 19 */
				},
				{
					BT_SDP_TYPE_SIZE(BT_SDP_UINT16), /* 09 */
					BT_SDP_ARRAY_16(AVDTP_VERSION)   /* AVDTP version: 01 03 */
				},)},)),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROFILE_DESC_LIST, BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8), /* 35 08 */
		BT_SDP_DATA_ELEM_LIST(
			{BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6), /* 35 06 */
			BT_SDP_DATA_ELEM_LIST(
				{
					BT_SDP_TYPE_SIZE(BT_SDP_UUID16),               /* 19 */
					BT_SDP_ARRAY_16(BT_SDP_ADVANCED_AUDIO_SVCLASS) /* 11 0d */
				},
				{
					BT_SDP_TYPE_SIZE(BT_SDP_UINT16), /* 09 */
					BT_SDP_ARRAY_16(0x0103U)         /* 01 03 */
				},)},)),
	BT_SDP_SERVICE_NAME("A2DPSink"),
	BT_SDP_SUPPORTED_FEATURES(0x0001U),
};

static struct bt_sdp_record a2dp_sink_rec = BT_SDP_RECORD(a2dp_sink_attrs);

/* -------------------------- Bluetooth Initialization -------------------------- */
static int bluetooth_init(void)
{
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		LOG_ERR("BT enable failed: %d", err);
		return err;
	}

	err = bt_sdp_register_service(&a2dp_sink_rec);
	if (err != 0) {
		LOG_ERR("A2DP sink SDP register failed: %d", err);
		return err;
	}

	/* Register A2DP endpoint */
	err = bt_a2dp_register_ep(&a2dp_sink_ep, BT_AVDTP_AUDIO, BT_AVDTP_SINK);
	if (err != 0) {
		LOG_ERR("A2DP endpoint register failed: %d", err);
		return err;
	}

	/* Register A2DP callbacks */
	err = bt_a2dp_register_cb(&a2dp_cb);
	if (err != 0) {
		LOG_ERR("A2DP callback register failed: %d", err);
		return err;
	}

	/* Register AVRCP TG callbacks */
	err = bt_avrcp_tg_register_cb(&avrcp_tg_cb);
	if (err != 0) {
		LOG_ERR("AVRCP TG callback register failed: %d", err);
		return err;
	}

	err = bt_set_name("ljq-525");
	if (err != 0) {
		LOG_ERR("Set BT name failed: %d", err);
		return err;
	}

	err = bt_br_set_connectable(true);
	if (err != 0) {
		LOG_ERR("Enable connectable failed: %d", err);
		return err;
	}

	err = bt_br_set_discoverable(true, false);
	if (err != 0) {
		LOG_ERR("Enable discoverable failed: %d", err);
		return err;
	}

	LOG_INF("BT initialized: discoverable as 'Zephyr A2DP DAC'");
	return 0;
}

int main(void)
{
	int err;

#if DT_NODE_HAS_STATUS_OKAY(DT_ALIAS(codec0))
	codec_dev = DEVICE_DT_GET(DT_ALIAS(codec0));
#endif

	LOG_INF("Starting A2DP DAC Sink...");

	decode_in_offset = 0;
	ring_buf_init(&rb, RING_BUF_SIZE, ring_buffer);

	err = bluetooth_init();
	if (err != 0) {
		LOG_ERR("BT init failed: %d", err);
		return err;
	}

	LOG_INF("A2DP DAC Sink ready. Waiting for connection...");

	while (1) {
		k_sleep(K_SECONDS(5));

		/* Log status periodically */
		if (active_bt_conn != NULL) {
			LOG_INF("Status - Connected: yes, Playing: %s", is_playing ? "yes" : "no");
		} else {
			LOG_INF("Status - Waiting for connection...");
		}
	}

	return 0;
}
