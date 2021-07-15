/** @file
 * @brief Advance Audio Distribution Profile.
 */

/*
 * Copyright (c) 2021 NXP
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>
#include <errno.h>
#include <sys/atomic.h>
#include <sys/byteorder.h>
#include <sys/util.h>
#include <sys/printk.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/avdtp.h>
#include <bluetooth/a2dp_codec_sbc.h>
#include <bluetooth/a2dp.h>

#define A2DP_AVDTP(_avdtp) CONTAINER_OF(_avdtp, struct bt_a2dp, session)
#define DISCOVER_REQ(_req) CONTAINER_OF(_req, struct bt_avdtp_discover_params, req)
#define DISCOVER_PARAM(_discover_param) CONTAINER_OF(_discover_param, struct bt_a2dp,\
						discover_param)
#define GET_CAP_REQ(_req) CONTAINER_OF(_req, struct bt_avdtp_get_capabilities_params, req)
#define GET_CAP_PARAM(_get_cap_param) CONTAINER_OF(_get_cap_param, struct bt_a2dp,\
						get_capabilities_param)
#define SET_CONF_REQ(_req) CONTAINER_OF(_req, struct bt_avdtp_set_configuration_params, req)
#define SET_CONF_PARAM(_set_conf_param) CONTAINER_OF(_set_conf_param, struct bt_a2dp,\
						set_configuration_param)
#define OPEN_REQ(_req) CONTAINER_OF(_req, struct bt_avdtp_open_params, req)
#define OPEN_PARAM(_open_param) CONTAINER_OF(_open_param, struct bt_a2dp, open_param)
#define START_REQ(_req) CONTAINER_OF(_req, struct bt_avdtp_start_params, req)
#define START_PARAM(_start_param) CONTAINER_OF(_start_param, struct bt_a2dp, start_param)

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_A2DP)
#define LOG_MODULE_NAME bt_a2dp
#include "common/log.h"

#include "hci_core.h"
#include "conn_internal.h"
#include "l2cap_internal.h"
#include "avdtp_internal.h"
#include "a2dp_internal.h"

#define A2DP_NO_SPACE (-1)
#define A2DP_MAX_IE_LENGTH (32U)
#define A2DP_MSG_MAX_COUNT (10)
#define A2DP_SBC_BLOCK_MAX (512U)

struct a2dp_task_msg {
	void *parameter;
	uint8_t event;
};

enum a2dp_task_event {
	A2DP_EVENT_INVALID = 0,
	A2DP_EVENT_SEND_PCM_DATA,
	A2DP_EVENT_RECEIVE_SBC_DATA,
};

enum bt_a2dp_internal_state {
	INTERNAL_STATE_IDLE = 0,
	INTERNAL_STATE_AVDTP_CONNECTED,
};

struct bt_a2dp_codec_ie_internal {
	uint8_t len;
	uint8_t codec_ie[A2DP_MAX_IE_LENGTH];
};

#if defined(CONFIG_BT_A2DP_SOURCE)
struct bt_a2dp_encoder_sbc {
	struct bt_a2dp_codec_sbc_encoder sbc_encoder;
	struct net_buf *send_buf;
	uint8_t *send_buf_header;
	uint32_t a2dp_pcm_sbc_framelen;
	uint32_t send_samples_count;
	uint16_t send_count;
	uint32_t a2dp_pcm_buffer_w;
	uint32_t a2dp_pcm_buffer_r;
	uint8_t a2dp_pcm_buffer[CONFIG_BT_A2DP_SBC_ENCODER_PCM_BUFFER_SIZE];
	uint16_t a2dp_pcm_buffer_frame[A2DP_SBC_BLOCK_MAX >> 1];
	uint8_t a2dp_sbc_encode_buffer_frame[A2DP_SBC_BLOCK_MAX];
	uint8_t send_buf_sbc_frame_count;
};
#endif

#if defined(CONFIG_BT_A2DP_SINK)
#define A2DP_SBC_ONE_FRAME_MAX_SIZE (2 * 2 * 16 * 8)
struct bt_a2dp_decoder_sbc {
	struct bt_a2dp_codec_sbc_decoder sbc_decoder;
	struct k_fifo sbc_data_fifo;
	uint8_t *pcm_buffer;
	uint16_t pcm_decoder_buffer[A2DP_SBC_ONE_FRAME_MAX_SIZE / 2];
	uint32_t pcm_buffer_length;
	/* the samples count in one frame */
	uint32_t frame_samples;
	/* the bytes count in one frame */
	uint32_t frame_size;
	uint32_t pcm_data_ind_length;
	volatile uint32_t pcm_r;
	volatile uint32_t pcm_w;
	uint16_t sbc_current_sn;
	uint16_t sbc_expected_sn;
	uint32_t sbc_current_ts;
	uint32_t sbc_expected_ts;
	uint8_t sample_size;
	uint8_t channel_num;
	uint8_t sbc_first_data:1;
	uint8_t pcm_first_data:1;
};
#endif

struct bt_a2dp_encoder_decoder_state {
#if defined(CONFIG_BT_A2DP_SOURCE)
	struct bt_a2dp_encoder_sbc encoder_sbc;
#endif
#if defined(CONFIG_BT_A2DP_SINK)
	struct bt_a2dp_decoder_sbc decoder_sbc;
#endif
};

struct bt_a2dp_endpoint_internal {
	/* the a2dp connection */
	struct bt_a2dp *a2dp;
	/* the corresponding endpoint */
	struct bt_a2dp_endpoint *endpoint;
	/* remote SEP Code Type */
	uint8_t remote_codec_type;
	/* the config result */
	struct bt_a2dp_codec_ie_internal config_internal;
	/* remote SEP info */
	struct bt_avdtp_seid_info remote_ep_info;
	/* remote SEP capability */
	struct bt_a2dp_codec_ie_internal remote_ep_ie;
	struct bt_a2dp_encoder_decoder_state *encoder_decoder;
};

struct bt_a2dp {
	struct bt_avdtp session;
	struct bt_avdtp_discover_params discover_param;
	struct bt_avdtp_get_capabilities_params get_capabilities_param;
	struct bt_avdtp_set_configuration_params set_configuration_param;
	struct bt_avdtp_open_params open_param;
	struct bt_avdtp_start_params start_param;
	void (*configure_cb)(int err);
	bt_a2dp_discover_peer_endpoint_cb_t discover_endpoint_cb;
	struct bt_avdtp_seid_info peer_seids[CONFIG_BT_A2DP_MAX_ENDPOINT_COUNT];
	uint8_t auto_select_endpoint_index;
	uint8_t get_capabilities_index;
	enum bt_a2dp_internal_state a2dp_state;
	uint8_t peer_seids_count;
	uint8_t auto_configure_enabled : 1;
};

static void a2dp_thread_entry(void);
K_THREAD_DEFINE(a2dp_thread_id, CONFIG_BT_A2DP_TASK_STACK_SIZE, a2dp_thread_entry, NULL, NULL,
		NULL, CONFIG_BT_A2DP_TASK_PRIORITY, 0, 0);
struct bt_a2dp_connect_cb connect_cb;
static struct bt_a2dp_endpoint_internal endpoint_internals[CONFIG_BT_A2DP_MAX_ENDPOINT_COUNT];
K_MSGQ_DEFINE(a2dp_msgq, sizeof(struct a2dp_task_msg), A2DP_MSG_MAX_COUNT, 4);
K_MUTEX_DEFINE(a2dp_mutex);

#define A2DP_LOCK() k_mutex_lock(&a2dp_mutex, K_FOREVER)
#define A2DP_UNLOCK() k_mutex_unlock(&a2dp_mutex)

/* Connections */
static struct bt_a2dp connection[CONFIG_BT_MAX_CONN];

static int bt_a2dp_get_seid_caps(struct bt_a2dp *a2dp);
static void a2dp_set_task_msg(void *parameter, uint8_t event);

static struct bt_a2dp_endpoint_internal *bt_a2dp_get_endpoint_state(
	struct bt_a2dp_endpoint *endpoint)
{
	for (uint8_t index = 0; index < CONFIG_BT_A2DP_MAX_ENDPOINT_COUNT; ++index) {
		if (endpoint_internals[index].endpoint == endpoint) {
			return &endpoint_internals[index];
		}
	}
	return NULL;
}

static uint8_t bt_a2dp_get_endpoints_count(void)
{
	uint8_t index;

	for (index = 0; index < CONFIG_BT_A2DP_MAX_ENDPOINT_COUNT; index++) {
		if (endpoint_internals[index].endpoint == NULL) {
			break;
		}
	}
	return index;
}

static void a2d_reset(struct bt_a2dp *a2dp)
{
	(void)memset(a2dp, 0, sizeof(struct bt_a2dp));
}

static struct bt_a2dp *get_new_connection(struct bt_conn *conn)
{
	int8_t i, free;

	free = A2DP_NO_SPACE;

	if (!conn) {
		BT_ERR("Invalid Input (err: %d)", -EINVAL);
		return NULL;
	}

	/* Find a space */
	for (i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		if (connection[i].session.br_chan.chan.conn == conn) {
			BT_DBG("Conn already exists");
			return &connection[i];
		}

		if (!connection[i].session.br_chan.chan.conn &&
			free == A2DP_NO_SPACE) {
			free = i;
		}
	}

	if (free == A2DP_NO_SPACE) {
		BT_DBG("More connection cannot be supported");
		return NULL;
	}

	/* Clean the memory area before returning */
	a2d_reset(&connection[free]);

	return &connection[free];
}

void avdtp_connected(struct bt_avdtp *session)
{
	struct bt_a2dp *a2dp = A2DP_AVDTP(session);

	a2dp->a2dp_state = INTERNAL_STATE_AVDTP_CONNECTED;
	/* notify a2dp app the connection. */
	connect_cb.connected(a2dp, 0);
}

void avdtp_disconnected(struct bt_avdtp *session)
{
	struct bt_a2dp *a2dp = A2DP_AVDTP(session);

	/* notify a2dp app the disconnection. */
	connect_cb.disconnected(a2dp);
}

static struct net_buf *avdtp_alloc_buf(struct bt_avdtp *session)
{
	return NULL;
}

int a2dp_discovery_ind(struct bt_avdtp *session, uint8_t *errcode)
{
	*errcode = 0;
	return 0;
}

int a2dp_get_capabilities_ind(struct bt_avdtp *session, struct bt_avdtp_seid_lsep *lsep,
		struct net_buf *rsp_buf, uint8_t *errcode)
{
	struct bt_a2dp_endpoint *endpoint;

	*errcode = 0;
	/* Service Category: Media Transport */
	net_buf_add_u8(rsp_buf, BT_AVDTP_SERVICE_MEDIA_TRANSPORT);
	net_buf_add_u8(rsp_buf, 0);
	/* Service Category: Media Codec */
	net_buf_add_u8(rsp_buf, BT_AVDTP_SERVICE_MEDIA_CODEC);
	endpoint = CONTAINER_OF(lsep, struct bt_a2dp_endpoint, info);
	/* Length Of Service Capability */
	net_buf_add_u8(rsp_buf, endpoint->capabilities->len + 2u);
	/* Media Type */
	net_buf_add_u8(rsp_buf, lsep->sep.media_type << 4u);
	/* Media Codec Type */
	net_buf_add_u8(rsp_buf, endpoint->codec_id);
	/* Codec Info Element */
	net_buf_add_mem(rsp_buf, &endpoint->capabilities->codec_ie[0], endpoint->capabilities->len);
	return 0;
}

int a2dp_set_configuration_ind(struct bt_avdtp *session, struct bt_avdtp_seid_lsep *lsep,
		struct net_buf *buf, uint8_t *errcode)
{
	struct bt_a2dp *a2dp = A2DP_AVDTP(session);
	int err;
	uint8_t codec_type;
	uint8_t *codec_info_element;
	uint16_t codec_info_element_len;
	struct bt_a2dp_endpoint *endpoint;
	struct bt_a2dp_codec_sbc_params *sbc_set;
	struct bt_a2dp_codec_sbc_params *sbc;
	struct bt_a2dp_endpoint_internal *ep_internal;

	*errcode = 0;
	/* parse the configuration */
	codec_info_element_len = 4U;
	err = bt_avdtp_parse_capability_codec(buf,
				&codec_type, &codec_info_element,
				&codec_info_element_len);
	endpoint = CONTAINER_OF(lsep, struct bt_a2dp_endpoint, info);
	if (endpoint->codec_id != codec_type) {
		*errcode = BAD_ACP_SEID;
		return 0;
	}

	ep_internal = bt_a2dp_get_endpoint_state(endpoint);
	if (ep_internal == NULL) {
		*errcode = BAD_ACP_SEID;
		return 0;
	}
	ep_internal->a2dp = a2dp;

	/* only support SBC now */
	if (codec_type == BT_A2DP_SBC) {
		struct bt_a2dp_endpoint_configure_result result;

		if (codec_info_element_len != 4U) {
			*errcode = BAD_ACP_SEID;
			return 0;
		}

		sbc_set = (struct bt_a2dp_codec_sbc_params *)codec_info_element;
		sbc = (struct bt_a2dp_codec_sbc_params *)&endpoint->capabilities->codec_ie[0];
		if (((BT_A2DP_SBC_SAMP_FREQ(sbc_set) & BT_A2DP_SBC_SAMP_FREQ(sbc)) == 0) ||
			((BT_A2DP_SBC_CHAN_MODE(sbc_set) & BT_A2DP_SBC_CHAN_MODE(sbc)) == 0) ||
			((BT_A2DP_SBC_BLK_LEN(sbc_set) & BT_A2DP_SBC_BLK_LEN(sbc)) == 0) ||
			((BT_A2DP_SBC_SUB_BAND(sbc_set) & BT_A2DP_SBC_SUB_BAND(sbc)) == 0) ||
			((BT_A2DP_SBC_ALLOC_MTHD(sbc_set) & BT_A2DP_SBC_ALLOC_MTHD(sbc)) == 0)) {
			*errcode = BAD_ACP_SEID;
			return 0;
		}

		/* callback to app */
		ep_internal->config_internal.len = codec_info_element_len;
		memcpy(&ep_internal->config_internal.codec_ie[0], codec_info_element,
			(codec_info_element_len > 4U ? 4U : codec_info_element_len));

		result.config.media_config =
			(struct bt_a2dp_codec_ie *)&ep_internal->config_internal;
		result.err = 0;
		if (endpoint->control_cbs.configured != NULL) {
			endpoint->control_cbs.configured(&result);
		}
	} else {
		*errcode = BAD_ACP_SEID;
		return 0;
	}

	return 0;
}

#if defined(CONFIG_BT_A2DP_SINK)
static void a2dp_sink_sbc_get_pcm_data(struct bt_a2dp_decoder_sbc *decoder,
			uint8_t **data, uint32_t *length)
{
	uint32_t data_space;

	if (decoder->pcm_first_data == 0) {
		decoder->pcm_first_data = 1;
		data_space = 0U;
	} else {
		if (decoder->pcm_w > decoder->pcm_r) {
			data_space = decoder->pcm_w - decoder->pcm_r;
		} else {
			data_space = decoder->pcm_buffer_length -
				(decoder->pcm_r - decoder->pcm_w);
		}
	}

	if (data_space < decoder->pcm_data_ind_length) {
		*data = NULL;
		*length = 0;
	} else {
		*data = &decoder->pcm_buffer[decoder->pcm_r];
		*length = decoder->pcm_data_ind_length;
		decoder->pcm_r += *length;
		if (decoder->pcm_r >= decoder->pcm_buffer_length) {
			decoder->pcm_r = 0;
		}
	}
}

static uint32_t a2dp_sink_pcm_buffer_free(struct bt_a2dp_decoder_sbc *decoder)
{
	if (decoder->pcm_first_data == 0) {
		decoder->pcm_first_data = 1;
		return decoder->pcm_buffer_length;
	}

	if (decoder->pcm_w > decoder->pcm_r) {
		return decoder->pcm_buffer_length - (decoder->pcm_w - decoder->pcm_r);
	} else {
		return decoder->pcm_r - decoder->pcm_w;
	}
}

static void a2dp_sink_sbc_add_pcm_data(struct bt_a2dp_decoder_sbc *decoder, uint8_t *data,
		uint32_t length)
{
	uint32_t free_space;

	free_space = a2dp_sink_pcm_buffer_free(decoder);

	if (free_space < length) {
		length = free_space;
	}

	/* copy data to buffer */
	if ((decoder->pcm_w + length) <= decoder->pcm_buffer_length) {
		if (data != NULL) {
			memcpy(&decoder->pcm_buffer[decoder->pcm_w], data, length);
		} else {
			memset(&decoder->pcm_buffer[decoder->pcm_w], 0, length);
		}
		decoder->pcm_w += length;
	} else {
		if (data != NULL) {
			memcpy(&decoder->pcm_buffer[decoder->pcm_w], data,
				(decoder->pcm_buffer_length - decoder->pcm_w));
			memcpy(&decoder->pcm_buffer[0],
				&data[(decoder->pcm_buffer_length - decoder->pcm_w)],
				length - (decoder->pcm_buffer_length - decoder->pcm_w));
		} else {
			memset(&decoder->pcm_buffer[decoder->pcm_w], 0,
				(decoder->pcm_buffer_length - decoder->pcm_w));
			memset(&decoder->pcm_buffer[0], 0,
				length - (decoder->pcm_buffer_length - decoder->pcm_w));
		}
		decoder->pcm_w = length - (decoder->pcm_buffer_length - decoder->pcm_w);
	}

	if (decoder->pcm_w == decoder->pcm_buffer_length) {
		decoder->pcm_w = 0;
	}
}

static void a2dp_sink_process_received_sbc_buf(struct bt_a2dp_endpoint_internal *ep_internal)
{
	struct bt_a2dp_decoder_sbc *decoder;
	struct net_buf *buf;
	/* process the sbc encoded data */
	struct bt_avdtp_media_hdr *media_hdr;
	struct bt_a2dp_codec_sbc_media_packet_hdr *sbc_hdr;
	uint32_t samples_lost = 0;
	uint32_t num_frames = 0;
	uint32_t pcm_data_len;
	uint32_t pre_len;
	int err;

	decoder = &ep_internal->encoder_decoder->decoder_sbc;
	while (1) {
		buf = net_buf_get(&decoder->sbc_data_fifo, K_NO_WAIT);
		if (buf == NULL) {
			return;
		}

		if (buf->len <= (sizeof(*media_hdr) + sizeof(*sbc_hdr))) {
			net_buf_unref(buf);
			return;
		}
		media_hdr = net_buf_pull_mem(buf, sizeof(*media_hdr));
		sbc_hdr = net_buf_pull_mem(buf, sizeof(*sbc_hdr));
		num_frames = sbc_hdr->number_of_sbc_frames;

		if (decoder->sbc_first_data == 0) {
			decoder->sbc_first_data = 1;
			decoder->sbc_current_ts =
				sys_get_be32((uint8_t *)&media_hdr->time_stamp);
			decoder->sbc_expected_ts = decoder->sbc_current_ts;
			decoder->sbc_current_sn =
				sys_get_be16((uint8_t *)&media_hdr->sequence_number);
			decoder->sbc_expected_sn = decoder->sbc_current_sn;
		} else {
			if (decoder->sbc_current_sn != decoder->sbc_expected_sn) {
				uint32_t current_ts =
					sys_get_be32((uint8_t *)&media_hdr->time_stamp);

				if (decoder->sbc_expected_ts < current_ts) {
					samples_lost = current_ts - decoder->sbc_expected_ts;
				}
			}
		}

		decoder->sbc_current_sn = sys_get_be16((uint8_t *)&media_hdr->sequence_number);
		decoder->sbc_expected_sn = decoder->sbc_current_sn + 1;
		decoder->sbc_current_ts = sys_get_be32((uint8_t *)&media_hdr->time_stamp);
		decoder->sbc_expected_ts = decoder->sbc_current_ts +
				num_frames * decoder->frame_samples;

		if (samples_lost != 0) {
			samples_lost = samples_lost * 2U * decoder->channel_num;
			a2dp_sink_sbc_add_pcm_data(decoder, NULL, samples_lost);
		}

		pre_len = 0;
		while ((buf->len > 3) && (pre_len != buf->len)) {
			pre_len = buf->len;
			pcm_data_len = sizeof(decoder->pcm_decoder_buffer);
			err = bt_a2dp_sbc_decode(&decoder->sbc_decoder,
				&buf->data, (uint32_t *)&buf->len,
				decoder->pcm_decoder_buffer,
				&pcm_data_len);
			if (!err) {
				a2dp_sink_sbc_add_pcm_data(decoder,
						(uint8_t *)decoder->pcm_decoder_buffer,
						pcm_data_len);
			} else {
				BT_DBG("decode err");
			}
		}
		if (buf->len > 0) {
			BT_DBG("err:%d", buf->len);
		}
		net_buf_unref(buf);
	}
}

static void a2dp_init_sbc_decoder_state(struct bt_avdtp_seid_lsep *lsep)
{
	struct bt_a2dp_endpoint *endpoint;
	struct bt_a2dp_endpoint_internal *ep_internal;
	struct bt_a2dp_decoder_sbc *decoder;
	struct bt_a2dp_codec_sbc_params *sbc;

	endpoint = CONTAINER_OF(lsep, struct bt_a2dp_endpoint, info);
	ep_internal = bt_a2dp_get_endpoint_state(endpoint);
	if (ep_internal == NULL) {
		return;
	}
	decoder = &ep_internal->encoder_decoder->decoder_sbc;
	memset(decoder, 0, sizeof(*decoder));
	sbc = (struct bt_a2dp_codec_sbc_params *)
				&ep_internal->config_internal.codec_ie[0];
	decoder->sbc_decoder.sbc = sbc;
	bt_a2dp_sbc_decoder_init(&decoder->sbc_decoder);

	/* initialize sbc decoder states */
	decoder->channel_num = bt_a2dp_sbc_get_channel_num(sbc);
	decoder->frame_samples =
			bt_a2dp_sbc_get_subband_num(sbc) * bt_a2dp_sbc_get_block_length(sbc);
	decoder->frame_size =
			decoder->frame_samples * 2U * decoder->channel_num;
	decoder->pcm_buffer_length = CONFIG_BT_A2DP_SBC_DECODER_PCM_BUFFER_SIZE;
	decoder->pcm_data_ind_length = CONFIG_BT_A2DP_SBC_DATA_IND_SAMPLES_COUNT *
						2U * decoder->channel_num;
	decoder->pcm_buffer_length = decoder->pcm_buffer_length -
			(decoder->pcm_buffer_length % decoder->pcm_data_ind_length);
	k_fifo_init(&decoder->sbc_data_fifo);
}

static void bt_a2dp_media_data_callback(
		struct bt_avdtp_seid_lsep *lsep,
		struct net_buf *buf)
{
	struct bt_a2dp_endpoint *endpoint;
	struct bt_a2dp_endpoint_internal *ep_internal;
	struct bt_a2dp_decoder_sbc *decoder;

	endpoint = CONTAINER_OF(lsep, struct bt_a2dp_endpoint, info);
	ep_internal = bt_a2dp_get_endpoint_state(endpoint);
	if (ep_internal == NULL) {
		return;
	}

	decoder = &ep_internal->encoder_decoder->decoder_sbc;
	net_buf_ref(buf);
	net_buf_put(&decoder->sbc_data_fifo, buf);
	a2dp_set_task_msg(ep_internal, A2DP_EVENT_RECEIVE_SBC_DATA);
}
#endif

int a2dp_open_ind(struct bt_avdtp *session, struct bt_avdtp_seid_lsep *lsep, uint8_t *errcode)
{
#if defined(CONFIG_BT_A2DP_SINK)
	if (lsep->sep.tsep == BT_A2DP_SINK) {
		lsep->media_data_cb = bt_a2dp_media_data_callback;
	} else {
		lsep->media_data_cb = NULL;
	}

	a2dp_init_sbc_decoder_state(lsep);
#else
	lsep->media_data_cb = NULL;
#endif
	*errcode = 0;
	return 0;
}

int a2dp_start_ind(struct bt_avdtp *session, struct bt_avdtp_seid_lsep *lsep, uint8_t *errcode)
{
	struct bt_a2dp_endpoint *endpoint;

	*errcode = 0;
	endpoint = CONTAINER_OF(lsep, struct bt_a2dp_endpoint, info);

	/* callback to app */
	if (endpoint->control_cbs.start_play != NULL) {
		endpoint->control_cbs.start_play(0);
	}

#if defined(CONFIG_BT_A2DP_SINK)
	if (lsep->sep.tsep == BT_A2DP_SINK) {
		for (uint8_t index = 0; index < CONFIG_BT_A2DP_SBC_DATA_IND_COUNT; index++) {
			endpoint->control_cbs.sink_streamer_data(NULL, 0);
		}
	}
#endif

	return 0;
}

int a2dp_close_ind(struct bt_avdtp *session, struct bt_avdtp_seid_lsep *lsep, uint8_t *errcode)
{
	struct bt_a2dp_endpoint *endpoint;

	*errcode = 0;
	endpoint = CONTAINER_OF(lsep, struct bt_a2dp_endpoint, info);

	/* callback to app */
	if (endpoint->control_cbs.stop_play != NULL) {
		endpoint->control_cbs.stop_play(0);
	}
	return 0;
}

int a2dp_suspend_ind(struct bt_avdtp *session, struct bt_avdtp_seid_lsep *lsep, uint8_t *errcode)
{
	struct bt_a2dp_endpoint *endpoint;

	*errcode = 0;
	endpoint = CONTAINER_OF(lsep, struct bt_a2dp_endpoint, info);

	/* callback to app */
	if (endpoint->control_cbs.stop_play != NULL) {
		endpoint->control_cbs.stop_play(0);
	}
	return 0;
}

static void bt_a2dp_auto_configure_callback(struct bt_a2dp *a2dp, uint8_t err)
{
	struct bt_a2dp_endpoint_configure_result config_result;
	struct bt_a2dp_endpoint_internal *ep_internal;

	ep_internal = &endpoint_internals[a2dp->auto_select_endpoint_index];

	/* success */
	if (!err) {
		config_result.err = 0;
		config_result.config.media_config =
			(struct bt_a2dp_codec_ie *)&ep_internal->config_internal;
		ep_internal->a2dp = a2dp;
		a2dp->configure_cb(0);
		ep_internal->endpoint->control_cbs.configured(&config_result);
	} else {
		ep_internal->a2dp = NULL;
		a2dp->configure_cb(err);
	}
}

static int bt_a2dp_open_cb(struct bt_avdtp_req *req)
{
	struct bt_a2dp *a2dp = OPEN_PARAM(OPEN_REQ(req));

	BT_DBG("OPEN result:%d", a2dp->open_param.status);
	if (!a2dp->open_param.status) {
		if (a2dp->auto_configure_enabled) {
			bt_a2dp_auto_configure_callback(a2dp, 0);
		} else {
			/* todo */
		}
	} else {
		if (a2dp->auto_configure_enabled) {
			bt_a2dp_auto_configure_callback(a2dp, -1);
		} else {
			/* todo */
		}
	}
	return 0;
}

static int bt_a2dp_set_configuration_cb(struct bt_avdtp_req *req)
{
	struct bt_a2dp *a2dp = SET_CONF_PARAM(SET_CONF_REQ(req));
	struct bt_a2dp_endpoint *endpoint;
	struct bt_a2dp_endpoint_internal *ep_internal;

	endpoint = CONTAINER_OF(a2dp->set_configuration_param.lsep, struct bt_a2dp_endpoint, info);
	ep_internal = bt_a2dp_get_endpoint_state(endpoint);

	if (ep_internal == NULL) {
		BT_DBG("error");
		return -EIO;
	}

	BT_DBG("SET CONFIGURATION result:%d", a2dp->set_configuration_param.status);
	if (!a2dp->set_configuration_param.status) {
		if (a2dp->auto_configure_enabled) {
			a2dp->open_param.req.func = bt_a2dp_open_cb;
			a2dp->open_param.acp_stream_endpoint_id = ep_internal->remote_ep_info.id;
			a2dp->open_param.lsep = a2dp->set_configuration_param.lsep;
			ep_internal->config_internal.len =
				a2dp->set_configuration_param.codec_ie->len;
			memcpy(&ep_internal->config_internal.codec_ie[0],
				&a2dp->set_configuration_param.codec_ie->codec_ie[0],
				ep_internal->config_internal.len);
			if (bt_avdtp_open(&a2dp->session, &a2dp->open_param) != 0) {
				bt_a2dp_auto_configure_callback(a2dp, -1);
			}
		} else {
			/* todo:*/
		}

	} else {
		if (a2dp->auto_configure_enabled) {
			bt_a2dp_auto_configure_callback(a2dp, -1);
		} else {
			/* todo: */
		}
	}

	return 0;
}

static void bt_a2dp_select_and_configure(struct bt_a2dp *a2dp)
{
	struct bt_a2dp_endpoint *self_endpoint;

	if ((a2dp->auto_select_endpoint_index == 0xFFu) ||
			(a2dp->auto_select_endpoint_index >= CONFIG_BT_A2DP_MAX_ENDPOINT_COUNT)) {
		return;
	}

	self_endpoint =
		endpoint_internals[a2dp->auto_select_endpoint_index].endpoint;
	if (self_endpoint->codec_id == BT_A2DP_SBC) {
		struct bt_a2dp_endpoint_internal *ep_internal;

		if (self_endpoint->config == NULL) {
			bt_a2dp_auto_configure_callback(a2dp, -1);
			return;
		}

		ep_internal = &endpoint_internals[a2dp->auto_select_endpoint_index];

		a2dp->set_configuration_param.req.func = bt_a2dp_set_configuration_cb;
		a2dp->set_configuration_param.acp_stream_endpoint_id =
			ep_internal->remote_ep_info.id;
		a2dp->set_configuration_param.int_stream_endpoint_id =
			self_endpoint->info.sep.id;
		a2dp->set_configuration_param.media_type = BT_A2DP_AUDIO;
		a2dp->set_configuration_param.media_codec_type = BT_A2DP_SBC;
		a2dp->set_configuration_param.codec_ie = self_endpoint->config;
		a2dp->set_configuration_param.lsep = &self_endpoint->info;

		ep_internal = bt_a2dp_get_endpoint_state(self_endpoint);
		if (ep_internal != NULL) {
#if defined(CONFIG_BT_A2DP_SOURCE)
			struct bt_a2dp_codec_sbc_params *sbc =
			(struct bt_a2dp_codec_sbc_params *)&(self_endpoint->config->codec_ie[0]);
			if (self_endpoint->info.sep.tsep == BT_A2DP_SOURCE) {
				ep_internal->encoder_decoder->encoder_sbc.
					a2dp_pcm_sbc_framelen =
					2 * bt_a2dp_sbc_get_subband_num(sbc) *
					bt_a2dp_sbc_get_block_length(sbc) *
					bt_a2dp_sbc_get_channel_num(sbc);
				ep_internal->encoder_decoder->encoder_sbc.
					sbc_encoder.sbc = sbc;
				if (bt_a2dp_sbc_encoder_init(
						&ep_internal->encoder_decoder->
						encoder_sbc.sbc_encoder) != 0) {
					BT_DBG("sbc encoder init fail");
				}
			}
#endif
#if defined(CONFIG_BT_A2DP_SINK)
			if (self_endpoint->info.sep.tsep == BT_A2DP_SINK) {
				/* todo: */
			}
#endif
		}

		if (bt_avdtp_set_configuration(&a2dp->session,
				&a2dp->set_configuration_param) != 0) {
			bt_a2dp_auto_configure_callback(a2dp, -1);
		}
	} else {
		/* todo: the follow codecs are not supported yet.
			* BT_A2DP_MPEG1
			* BT_A2DP_MPEG2
			* BT_A2DP_ATRAC
			* BT_A2DP_VENDOR
			*/
		bt_a2dp_auto_configure_callback(a2dp, -1);
	}
}

static int bt_a2dp_get_capabilities_cb(struct bt_avdtp_req *req)
{
	int err;
	uint8_t *codec_info_element;
	struct bt_a2dp *a2dp = GET_CAP_PARAM(GET_CAP_REQ(req));
	uint16_t codec_info_element_len;
	uint8_t codec_type;
	uint8_t index;

	BT_DBG("GET CAPABILITIES result:%d", a2dp->get_capabilities_param.status);
	if (a2dp->get_capabilities_param.status) {
		if (a2dp->auto_configure_enabled) {
			bt_a2dp_select_and_configure(a2dp);
		} else {
			a2dp->discover_endpoint_cb(a2dp, NULL, 0);
			a2dp->discover_endpoint_cb = NULL;
		}
		return 0;
	}

	err = bt_avdtp_parse_capability_codec(a2dp->get_capabilities_param.buf,
			&codec_type, &codec_info_element, &codec_info_element_len);
	if (err) {
		BT_DBG("codec capability parsing fail");
		return 0;
	}

	if (codec_info_element_len > A2DP_MAX_IE_LENGTH) {
		codec_info_element_len = A2DP_MAX_IE_LENGTH;
	}

	if (a2dp->auto_configure_enabled) {
		for (index = 0; index < bt_a2dp_get_endpoints_count(); index++) {
			if (endpoint_internals[index].endpoint->codec_id ==
					codec_type) {
				if (index < a2dp->auto_select_endpoint_index) {
					struct bt_a2dp_endpoint_internal *ep_internal =
						&endpoint_internals[index];
					a2dp->auto_select_endpoint_index = index;
					ep_internal->remote_ep_info =
						a2dp->peer_seids[a2dp->get_capabilities_index];
					ep_internal->remote_codec_type = codec_type;

					memcpy(ep_internal->remote_ep_ie.codec_ie,
						codec_info_element, codec_info_element_len);
					ep_internal->remote_ep_ie.len = codec_info_element_len;
				}
			}
		}
		if (bt_a2dp_get_seid_caps(a2dp) != 0) {
			bt_a2dp_select_and_configure(a2dp);
		}
	} else if (a2dp->discover_endpoint_cb != NULL) {
		struct bt_a2dp_endpoint peer_endpoint;
		struct bt_a2dp_codec_ie_internal peer_endpoint_internal_cap;

		peer_endpoint.codec_id = codec_type;
		peer_endpoint.info.sep =
			a2dp->peer_seids[a2dp->get_capabilities_index];
		peer_endpoint.info.next = NULL;
		peer_endpoint.config = NULL;
		memcpy(peer_endpoint_internal_cap.codec_ie,
			codec_info_element, codec_info_element_len);
		peer_endpoint_internal_cap.len = codec_info_element_len;
		peer_endpoint.capabilities =
			(struct bt_a2dp_codec_ie *)&peer_endpoint_internal_cap;
		if (a2dp->discover_endpoint_cb(a2dp, &peer_endpoint, 0) ==
			BT_A2DP_DISCOVER_ENDPOINT_CONTINUE) {
			if (bt_a2dp_get_seid_caps(a2dp) != 0) {
				a2dp->discover_endpoint_cb(a2dp, NULL, 0);
				a2dp->discover_endpoint_cb = NULL;
			}
		}
	}

	return 0;
}

static int bt_a2dp_get_seid_caps(struct bt_a2dp *a2dp)
{
	int err;

	if (a2dp->get_capabilities_index == 0xFFu) {
		a2dp->get_capabilities_index = 0U;
	} else {
		a2dp->get_capabilities_index++;
	}

	for (; a2dp->get_capabilities_index < a2dp->peer_seids_count;
			a2dp->get_capabilities_index++) {
		if ((a2dp->peer_seids[a2dp->get_capabilities_index].inuse == 0) &&
			(a2dp->peer_seids[a2dp->get_capabilities_index].media_type ==
			BT_A2DP_AUDIO)) {
			a2dp->get_capabilities_param.req.func = bt_a2dp_get_capabilities_cb;
			a2dp->get_capabilities_param.buf = NULL;
			a2dp->get_capabilities_param.stream_endpoint_id =
				a2dp->peer_seids[a2dp->get_capabilities_index].id;
			err = bt_avdtp_get_capabilities(&a2dp->session,
				&a2dp->get_capabilities_param);
			if (err) {
				BT_DBG("AVDTP get capabilities failed");
			}
			return 0;
		}
	}
	return -1;
}

static int bt_a2dp_discover_cb(struct bt_avdtp_req *req)
{
	struct bt_a2dp *a2dp = DISCOVER_PARAM(DISCOVER_REQ(req));
	struct bt_avdtp_seid_info *seid;

	BT_DBG("DISCOVER result:%d", DISCOVER_REQ(req)->status);
	a2dp->peer_seids_count = 0U;
	if (!(DISCOVER_REQ(req)->status)) {
		seid = bt_avdtp_parse_seids(DISCOVER_REQ(req)->buf);
		while ((seid != NULL) &&
				(a2dp->peer_seids_count < CONFIG_BT_A2DP_MAX_ENDPOINT_COUNT)) {
			BT_DBG("id:%d, inuse:%d, media_type:%d, tsep:%d, ",
				seid->id,
				seid->inuse,
				seid->media_type,
				seid->tsep);
			a2dp->peer_seids[a2dp->peer_seids_count] = *seid;
			a2dp->peer_seids_count++;
			seid = bt_avdtp_parse_seids(DISCOVER_REQ(req)->buf);
		}

		/* trigger the getting capability */
		a2dp->get_capabilities_index = 0xFFu;
		if (bt_a2dp_get_seid_caps(a2dp) != 0) {
			/* the peer_seids' caps is done.*/
			if (a2dp->auto_configure_enabled) {
				bt_a2dp_select_and_configure(a2dp);
			} else if (a2dp->discover_endpoint_cb != NULL) {
				a2dp->discover_endpoint_cb(a2dp, NULL, 0);
			}
		}
	} else {
		if (a2dp->auto_configure_enabled) {
			bt_a2dp_select_and_configure(a2dp);
		} else if (a2dp->discover_endpoint_cb != NULL) {
			a2dp->discover_endpoint_cb(a2dp, NULL, -1);
		}
	}

	return 0;
}

int bt_a2dp_configure(struct bt_a2dp *a2dp, void (*result_cb)(int err))
{
	int err;

	if (a2dp == NULL) {
		return -EINVAL;
	}

	if (a2dp->a2dp_state != INTERNAL_STATE_AVDTP_CONNECTED) {
		return -EIO;
	}

	a2dp->auto_configure_enabled = 1U;
	a2dp->configure_cb = result_cb;

	a2dp->discover_param.req.func = bt_a2dp_discover_cb;
	a2dp->discover_param.buf = NULL;
	err = bt_avdtp_discover(&a2dp->session, &a2dp->discover_param);
	if (err) {
		BT_DBG("AVDTP discover failed");
	}
	return 0;
}

int bt_a2dp_discover_peer_endpoints(struct bt_a2dp *a2dp, bt_a2dp_discover_peer_endpoint_cb_t cb)
{
	int err;

	if (a2dp == NULL) {
		return -EINVAL;
	}

	a2dp->auto_configure_enabled = 0U;
	a2dp->discover_endpoint_cb = cb;
	a2dp->discover_param.req.func = bt_a2dp_discover_cb;
	a2dp->discover_param.buf = NULL;
	err = bt_avdtp_discover(&a2dp->session, &a2dp->discover_param);
	if (err) {
		BT_DBG("AVDTP discover failed");
	}

	return err;
}

int bt_a2dp_configure_endpoint(struct bt_a2dp *a2dp, struct bt_a2dp_endpoint *endpoint,
		struct bt_a2dp_endpoint *peer_endpoint,
		struct bt_a2dp_endpoint_config *config)
{
	/* todo */
	return 0;
}

int bt_a2dp_deconfigure(struct bt_a2dp_endpoint *endpoint)
{
	/* todo */
	return 0;
}

static int bt_a2dp_start_cb(struct bt_avdtp_req *req)
{
	struct bt_a2dp *a2dp = START_PARAM(START_REQ(req));
	struct bt_a2dp_endpoint *endpoint;
	struct bt_a2dp_endpoint_internal *ep_internal;

	endpoint = CONTAINER_OF(a2dp->start_param.lsep, struct bt_a2dp_endpoint, info);
	ep_internal = bt_a2dp_get_endpoint_state(endpoint);
	if (ep_internal == NULL) {
		return -EPERM;
	}

	BT_DBG("START result:%d", a2dp->start_param.status);
	if (!a2dp->start_param.status) {
		if (endpoint->control_cbs.start_play != NULL) {
			endpoint->control_cbs.start_play(0);
		}
#if defined(CONFIG_BT_A2DP_SINK)
		if (a2dp->start_param.lsep->sep.tsep == BT_A2DP_SINK) {
			for (uint8_t index = 0; index < CONFIG_BT_A2DP_SBC_DATA_IND_COUNT;
				index++) {
				endpoint->control_cbs.sink_streamer_data(NULL, 0);
			}
		}
#endif
	} else {
		if (endpoint->control_cbs.start_play != NULL) {
			endpoint->control_cbs.start_play(a2dp->start_param.status);
		}
	}
	return 0;
}

int bt_a2dp_start(struct bt_a2dp_endpoint *endpoint)
{
	struct bt_a2dp *a2dp;
	struct bt_a2dp_endpoint_internal *ep_internal;

	if (endpoint == NULL) {
		return -EPERM;
	}

	ep_internal = bt_a2dp_get_endpoint_state(endpoint);
	if (ep_internal == NULL) {
		return -EPERM;
	}

	a2dp = ep_internal->a2dp;
	if (a2dp == NULL) {
		return -EPERM;
	}

	a2dp->start_param.req.func = bt_a2dp_start_cb;
	a2dp->start_param.acp_stream_endpoint_id = ep_internal->remote_ep_info.id;
	a2dp->start_param.lsep = &endpoint->info;
	if (bt_avdtp_start(&a2dp->session, &a2dp->start_param) != 0) {
		if (endpoint->control_cbs.start_play != NULL) {
			endpoint->control_cbs.start_play(-1);
		}
		return -EIO;
	}
	return 0;
}

int bt_a2dp_reconfigure(struct bt_a2dp_endpoint *endpoint,
		struct bt_a2dp_endpoint_config *config)
{
	/* todo: */
	return 0;
}

static void a2dp_set_task_msg(void *parameter, uint8_t event)
{
	struct a2dp_task_msg task_msg;

	task_msg.parameter = parameter;
	task_msg.event = event;
	k_msgq_put(&a2dp_msgq, &task_msg, K_NO_WAIT);
}

#if defined(CONFIG_BT_A2DP_SOURCE)
int bt_a2dp_src_media_write(struct bt_a2dp_endpoint *endpoint,
							uint8_t *data, uint16_t datalen)
{
	uint32_t free_space;
	struct bt_a2dp_endpoint_internal *ep_internal;

	if (endpoint == NULL) {
		return -EPERM;
	}

	ep_internal = bt_a2dp_get_endpoint_state(endpoint);
	if (ep_internal == NULL) {
		return -EIO;
	}

	if (endpoint->codec_id == BT_A2DP_SBC) {
		struct bt_a2dp_encoder_sbc *encoder_sbc =
						&ep_internal->encoder_decoder->encoder_sbc;
		A2DP_LOCK();
		if (encoder_sbc->a2dp_pcm_buffer_w >= encoder_sbc->a2dp_pcm_buffer_r) {
			free_space = CONFIG_BT_A2DP_SBC_ENCODER_PCM_BUFFER_SIZE +
					encoder_sbc->a2dp_pcm_buffer_r -
					encoder_sbc->a2dp_pcm_buffer_w;
		} else {
			free_space = encoder_sbc->a2dp_pcm_buffer_r -
				encoder_sbc->a2dp_pcm_buffer_w;
		}

		if ((free_space < 1) || (free_space < datalen)) {
			a2dp_set_task_msg(ep_internal, A2DP_EVENT_SEND_PCM_DATA);
			A2DP_UNLOCK();
			/* printk("no free\r\n"); */
			return -EIO;
		}

		/* copy PCM data to enqueued buffer */
		if (encoder_sbc->a2dp_pcm_buffer_w + datalen <=
				CONFIG_BT_A2DP_SBC_ENCODER_PCM_BUFFER_SIZE) {
			memcpy(&encoder_sbc->a2dp_pcm_buffer[encoder_sbc->a2dp_pcm_buffer_w],
				data, datalen);
			encoder_sbc->a2dp_pcm_buffer_w += datalen;
		} else {
			memcpy(&encoder_sbc->a2dp_pcm_buffer[encoder_sbc->a2dp_pcm_buffer_w],
					data, (CONFIG_BT_A2DP_SBC_ENCODER_PCM_BUFFER_SIZE -
					encoder_sbc->a2dp_pcm_buffer_w));
			memcpy(&encoder_sbc->a2dp_pcm_buffer[0],
					data + (CONFIG_BT_A2DP_SBC_ENCODER_PCM_BUFFER_SIZE -
					encoder_sbc->a2dp_pcm_buffer_w),
					datalen - (CONFIG_BT_A2DP_SBC_ENCODER_PCM_BUFFER_SIZE -
					encoder_sbc->a2dp_pcm_buffer_w));
			encoder_sbc->a2dp_pcm_buffer_w =
					datalen - (CONFIG_BT_A2DP_SBC_ENCODER_PCM_BUFFER_SIZE -
					encoder_sbc->a2dp_pcm_buffer_w);
		}

		if (encoder_sbc->a2dp_pcm_buffer_w == CONFIG_BT_A2DP_SBC_ENCODER_PCM_BUFFER_SIZE) {
			encoder_sbc->a2dp_pcm_buffer_w = 0U;
		}
		A2DP_UNLOCK();

		a2dp_set_task_msg(ep_internal, A2DP_EVENT_SEND_PCM_DATA);
	} else {
		/* todo: other codec is not supported yet */
	}

	return 0;
}

static void a2dp_source_net_buf_alloc(struct bt_a2dp_encoder_sbc *encoder_sbc)
{
	if (encoder_sbc->send_buf == NULL) {
		encoder_sbc->send_buf = bt_l2cap_create_pdu(NULL, 0);
		if (encoder_sbc->send_buf == NULL) {
			BT_DBG("bt_l2cap_create_pdu fail");
			return;
		}

		encoder_sbc->send_buf_header =
				net_buf_add(encoder_sbc->send_buf,
				sizeof(struct bt_a2dp_codec_sbc_media_packet_hdr) +
				sizeof(struct bt_avdtp_media_hdr));
		memset(encoder_sbc->send_buf_header, 0,
				sizeof(struct bt_a2dp_codec_sbc_media_packet_hdr) +
				sizeof(struct bt_avdtp_media_hdr));
		((struct bt_avdtp_media_hdr *)(encoder_sbc->send_buf_header))->playload_type =
				0x60U;
		((struct bt_avdtp_media_hdr *)(encoder_sbc->send_buf_header))->
				synchronization_source = 0U;
	}
}

static void a2dp_source_net_buf_send(struct bt_a2dp_endpoint_internal *ep_internal,
					uint32_t samples_count)
{
	int err;
	struct bt_a2dp_encoder_sbc *encoder_sbc =
						&ep_internal->encoder_decoder->encoder_sbc;
	/* update time_stamp in the buf */
	sys_put_be32(encoder_sbc->send_samples_count,
		(uint8_t *)&((struct bt_avdtp_media_hdr *)
		(encoder_sbc->send_buf_header))->time_stamp);
	/* update sequence_number in the buf */
	sys_put_be16(encoder_sbc->send_count,
		(uint8_t *)&((struct bt_avdtp_media_hdr *)
		(encoder_sbc->send_buf_header))->sequence_number);
	/* send the buf */
	err = bt_avdtp_send_media_data(&ep_internal->endpoint->info, encoder_sbc->send_buf);
	if (err < 0) {
		BT_DBG("avdtp media fail\r\n");
		net_buf_unref(encoder_sbc->send_buf);
	}
	/* update recorder */
	encoder_sbc->send_samples_count += samples_count;
	encoder_sbc->send_count++;
	encoder_sbc->send_buf = NULL;
}

static int a2dp_source_sbc_encode_and_send(struct bt_a2dp_endpoint_internal *ep_internal)
{
	uint32_t remain_data;
	uint32_t send_bytes;
	uint32_t encoder_len;
	int err;
	uint8_t number_of_frames;
	struct bt_a2dp_codec_sbc_params *sbc;
	uint8_t blocks;
	uint8_t subbands;
	uint16_t count;

	sbc = (struct bt_a2dp_codec_sbc_params *)&(ep_internal->config_internal.codec_ie[0]);
	blocks = bt_a2dp_sbc_get_block_length(sbc);
	subbands = bt_a2dp_sbc_get_subband_num(sbc);
	struct bt_a2dp_encoder_sbc *encoder_sbc =
						&ep_internal->encoder_decoder->encoder_sbc;

	for (;;) {
		A2DP_LOCK();
		if (encoder_sbc->a2dp_pcm_buffer_w >= encoder_sbc->a2dp_pcm_buffer_r) {
			remain_data = encoder_sbc->a2dp_pcm_buffer_w -
				encoder_sbc->a2dp_pcm_buffer_r;
		} else {
			remain_data = CONFIG_BT_A2DP_SBC_ENCODER_PCM_BUFFER_SIZE -
				(encoder_sbc->a2dp_pcm_buffer_r -
				encoder_sbc->a2dp_pcm_buffer_w);
		}

		if (remain_data < encoder_sbc->a2dp_pcm_sbc_framelen) {
			A2DP_UNLOCK();
			break;
		}
		A2DP_UNLOCK();

		send_bytes = (remain_data > encoder_sbc->a2dp_pcm_sbc_framelen) ?
			encoder_sbc->a2dp_pcm_sbc_framelen : remain_data;

		for (count = 0; count < (send_bytes >> 1); count++) {
			encoder_sbc->a2dp_pcm_buffer_frame[count] =
			(uint16_t)(((uint16_t)
			encoder_sbc->a2dp_pcm_buffer[encoder_sbc->a2dp_pcm_buffer_r + 1]
			<< 8) | ((uint16_t)encoder_sbc->a2dp_pcm_buffer
			[encoder_sbc->a2dp_pcm_buffer_r]));
			encoder_sbc->a2dp_pcm_buffer_r += 2;
			if (encoder_sbc->a2dp_pcm_buffer_r >=
					CONFIG_BT_A2DP_SBC_ENCODER_PCM_BUFFER_SIZE) {
				encoder_sbc->a2dp_pcm_buffer_r = 0U;
			}
		}

		encoder_len = encoder_sbc->a2dp_pcm_sbc_framelen;
		err = bt_a2dp_sbc_encode(&encoder_sbc->sbc_encoder,
			(uint8_t *)&encoder_sbc->a2dp_pcm_buffer_frame[0u],
			&encoder_sbc->a2dp_sbc_encode_buffer_frame[0], &encoder_len);
		if (err) {
			BT_DBG("sbc encode fail");
			continue;
		}

		a2dp_source_net_buf_alloc(encoder_sbc);
		if (encoder_sbc->send_buf == NULL) {
			continue;
		}

		if ((encoder_sbc->send_buf->len + encoder_len >
			bt_avdtp_get_media_mtu(&ep_internal->endpoint->info)) ||
			(net_buf_tailroom(encoder_sbc->send_buf) < encoder_len)) {
			number_of_frames = ((struct bt_a2dp_codec_sbc_media_packet_hdr *)
				(encoder_sbc->send_buf_header +
				sizeof(struct bt_avdtp_media_hdr)))->number_of_sbc_frames;
			a2dp_source_net_buf_send(ep_internal, number_of_frames * subbands * blocks);
		}

		a2dp_source_net_buf_alloc(encoder_sbc);
		if (encoder_sbc->send_buf == NULL) {
			continue;
		}

		/* update number_of_sbc_frames in the buf */
		number_of_frames = ++((struct bt_a2dp_codec_sbc_media_packet_hdr *)
			(encoder_sbc->send_buf_header +
			sizeof(struct bt_avdtp_media_hdr)))->
			number_of_sbc_frames;
		memcpy(net_buf_add(encoder_sbc->send_buf, encoder_len),
			&encoder_sbc->a2dp_sbc_encode_buffer_frame[0], encoder_len);
		/* the frames is full or the mtu */
		if (number_of_frames == 0x0Fu) {
			a2dp_source_net_buf_send(ep_internal, number_of_frames * subbands * blocks);
		}
	}
	return 0;
}

static int a2dp_source_encode_and_send(struct bt_a2dp_endpoint_internal *ep_internal)
{
	for (;;) {
		if (ep_internal->endpoint->codec_id == BT_A2DP_SBC) {
#if defined(CONFIG_BT_A2DP_SOURCE)
			return a2dp_source_sbc_encode_and_send(ep_internal);
#endif
		} else {
			/* not supported yet */
		}
	}
	return -EIO;
}
#endif

static void a2dp_thread_entry(void)
{
	struct bt_a2dp_endpoint_internal *ep_internal;
	struct a2dp_task_msg msg_data;

	while (1U) {
		if (k_msgq_get(&a2dp_msgq, &msg_data, K_FOREVER)) {
			continue;
		}

		if (msg_data.parameter == NULL) {
			continue;
		}

		switch (msg_data.event) {
#if defined(CONFIG_BT_A2DP_SOURCE)
		case A2DP_EVENT_SEND_PCM_DATA:
			ep_internal = (struct bt_a2dp_endpoint_internal *)msg_data.parameter;
			a2dp_source_encode_and_send(ep_internal);
			break;
#endif
#if defined(CONFIG_BT_A2DP_SINK)
		case A2DP_EVENT_RECEIVE_SBC_DATA:
			ep_internal = (struct bt_a2dp_endpoint_internal *)msg_data.parameter;
			a2dp_sink_process_received_sbc_buf(ep_internal);
			break;
#endif
		default:
			break;
		}
	}
}

static const struct bt_avdtp_ops_cb signaling_avdtp_ops = {
	.connected = avdtp_connected,
	.disconnected = avdtp_disconnected,
	.alloc_buf = avdtp_alloc_buf,
	.discovery_ind = a2dp_discovery_ind,
	.get_capabilities_ind = a2dp_get_capabilities_ind,
	.set_configuration_ind = a2dp_set_configuration_ind,
	.open_ind = a2dp_open_ind,
	.start_ind = a2dp_start_ind,
	.close_ind = a2dp_close_ind,
	.suspend_ind = a2dp_suspend_ind,
};

int a2dp_accept(struct bt_conn *conn, struct bt_avdtp **session)
{
	struct bt_a2dp *a2dp;

	a2dp = get_new_connection(conn);
	if (!a2dp) {
		return -ENOMEM;
	}

	*session = &(a2dp->session);
	a2dp->session.ops = &signaling_avdtp_ops;
	BT_DBG("session: %p", &(a2dp->session));

	return 0;
}

/* The above callback structures need to be packed and passed to AVDTP */
static struct bt_avdtp_event_cb avdtp_cb = {
	.accept = a2dp_accept,
};

int bt_a2dp_init(void)
{
	int err;

	/* Register event handlers with AVDTP */
	err = bt_avdtp_register(&avdtp_cb);
	if (err < 0) {
		BT_ERR("A2DP registration failed");
		return err;
	}

	for (uint8_t i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		memset(&connection[i], 0, sizeof(struct bt_a2dp));
	}

	/* start a2dp task. */
	k_thread_start(a2dp_thread_id);

	memset(endpoint_internals, 0, sizeof(endpoint_internals));
	BT_DBG("A2DP Initialized successfully.");
	return 0;
}

struct bt_a2dp *bt_a2dp_connect(struct bt_conn *conn)
{
	struct bt_a2dp *a2dp;
	int err;

	a2dp = get_new_connection(conn);
	if (!a2dp) {
		BT_ERR("Cannot allocate memory");
		return NULL;
	}

	a2dp->auto_select_endpoint_index = 0xFFu;
	a2dp->a2dp_state = INTERNAL_STATE_IDLE;

	a2dp->session.ops = &signaling_avdtp_ops;
	err = bt_avdtp_connect(conn, &(a2dp->session));
	if (err < 0) {
		/* If error occurs, undo the saving and return the error */
		a2d_reset(a2dp);
		BT_DBG("AVDTP Connect failed");
		return NULL;
	}

	BT_DBG("Connect request sent");
	return a2dp;
}

int bt_a2dp_disconnect(struct bt_a2dp *a2dp)
{
	/* todo: */
	return 0;
}

int bt_a2dp_register_endpoint(struct bt_a2dp_endpoint *endpoint,
				  uint8_t media_type, uint8_t role)
{
	int err;
	uint8_t index;

	BT_ASSERT(endpoint);

	err = bt_avdtp_register_sep(media_type, role, &(endpoint->info));
	if (err < 0) {
		return err;
	}

	for (index = 0; index < CONFIG_BT_A2DP_MAX_ENDPOINT_COUNT; index++) {
		if (endpoint_internals[index].endpoint == NULL) {
			endpoint_internals[index].endpoint = endpoint;
			endpoint_internals[index].encoder_decoder =
				(struct bt_a2dp_encoder_decoder_state *)&endpoint->codec_buffer[0];
#if ((defined(CONFIG_BT_A2DP_SINK)) && (CONFIG_BT_A2DP_SINK > 0U))
			if ((endpoint->codec_id == BT_A2DP_SBC) && (role == BT_A2DP_SINK)) {
				endpoint_internals[index].encoder_decoder->decoder_sbc.pcm_buffer =
					(uint8_t *)&endpoint->codec_buffer_nocached[0];
			}
#endif
			break;
		}
	}
	return 0;
}

int bt_a2dp_register_connect_callback(struct bt_a2dp_connect_cb *cb)
{
	connect_cb = *cb;
	return 0;
}

#if defined(CONFIG_BT_A2DP_SINK)
int bt_a2dp_snk_media_sync(struct bt_a2dp_endpoint *endpoint,
							uint8_t *data, uint16_t datalen)
{
	uint8_t *get_data;
	uint32_t length;
	struct bt_a2dp *a2dp;
	struct bt_a2dp_endpoint_internal *ep_internal = bt_a2dp_get_endpoint_state(endpoint);

	if (ep_internal == NULL) {
		return -EIO;
	}
	a2dp = ep_internal->a2dp;
	if (a2dp == NULL) {
		return -EIO;
	}

	if (endpoint->codec_id == BT_A2DP_SBC) {
		/* give more data */
		a2dp_sink_sbc_get_pcm_data(&ep_internal->encoder_decoder->decoder_sbc,
							&get_data, &length);
		endpoint->control_cbs.sink_streamer_data(get_data, length);
		a2dp_set_task_msg(ep_internal, A2DP_EVENT_RECEIVE_SBC_DATA);
	} else {
		/* other codes are not implemented yet */
	}
	return 0;
}
#endif
