/** @file
 *  @brief Bluetooth audio shell functions
 *
 *  This is not to be included by the application.
 */

/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef AUDIO_SHELL_AUDIO_H
#define AUDIO_SHELL_AUDIO_H

#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/atomic_types.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys_clock.h>

#include "common/bt_shell_private.h"

#define SHELL_PRINT_INDENT_LEVEL_SIZE 2
#define MAX_CODEC_FRAMES_PER_SDU      4U

extern struct bt_csip_set_member_svc_inst *svc_inst;

size_t audio_ad_data_add(struct bt_data *data, const size_t data_size, const bool discoverable,
			 const bool connectable);
size_t audio_pa_data_add(struct bt_data *data_array, const size_t data_array_size);
size_t csis_ad_data_add(struct bt_data *data, const size_t data_size, const bool discoverable);
size_t cap_acceptor_ad_data_add(struct bt_data data[], size_t data_size, bool discoverable);
size_t bap_scan_delegator_ad_data_add(struct bt_data data[], size_t data_size);
size_t gmap_ad_data_add(struct bt_data data[], size_t data_size);
size_t pbp_ad_data_add(struct bt_data data[], size_t data_size);
size_t cap_initiator_pa_data_add(struct bt_data *data_array, const size_t data_array_size);

#if defined(CONFIG_BT_AUDIO)
/* Must guard before including audio.h as audio.h uses Kconfigs guarded by
 * CONFIG_BT_AUDIO
 */
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/bluetooth/audio/cap.h>

unsigned long bap_get_stats_interval(void);

#if defined(CONFIG_LIBLC3)
#include "lc3.h"

#define USB_SAMPLE_RATE            48000U
#define LC3_MAX_SAMPLE_RATE        48000U
#define LC3_MAX_FRAME_DURATION_US  10000U
#define LC3_MAX_NUM_SAMPLES_MONO   ((LC3_MAX_FRAME_DURATION_US * LC3_MAX_SAMPLE_RATE) /            \
				    USEC_PER_SEC)
#define LC3_MAX_NUM_SAMPLES_STEREO (LC3_MAX_NUM_SAMPLES_MONO * 2U)
#endif /* CONFIG_LIBLC3 */

#define LOCATION BT_AUDIO_LOCATION_FRONT_LEFT
#define CONTEXT                                                                                    \
	(BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED | BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL |                \
	 BT_AUDIO_CONTEXT_TYPE_MEDIA |                                                             \
	 COND_CODE_1(IS_ENABLED(CONFIG_BT_GMAP), (BT_AUDIO_CONTEXT_TYPE_GAME), (0)))

const struct named_lc3_preset *gmap_get_named_preset(bool is_unicast, enum bt_audio_dir dir,
						     const char *preset_arg);

struct named_lc3_preset {
	const char *name;
	struct bt_bap_lc3_preset preset;
};

struct shell_stream {
	struct bt_cap_stream stream;
	struct bt_audio_codec_cfg codec_cfg;
	struct bt_bap_qos_cfg qos;
	bool is_tx;
	bool is_rx;

#if defined(CONFIG_LIBLC3)
	uint32_t lc3_freq_hz;
	uint32_t lc3_frame_duration_us;
	uint16_t lc3_octets_per_frame;
	uint8_t lc3_frame_blocks_per_sdu;
	enum bt_audio_location lc3_chan_allocation;
	uint8_t lc3_chan_cnt;
#endif /* CONFIG_LIBLC3 */

	union {
#if defined(CONFIG_BT_AUDIO_TX)
		struct {
			/* The uptime tick measured when stream was connected */
			int64_t connected_at_ticks;
			uint16_t seq_num;
#if defined(CONFIG_LIBLC3)
			atomic_t lc3_enqueue_cnt;
			bool active;
			size_t encoded_cnt;
			size_t lc3_sdu_cnt;
			lc3_encoder_mem_48k_t lc3_encoder_mem;
			lc3_encoder_t lc3_encoder;
#if defined(CONFIG_USBD_AUDIO2_CLASS)
			/* Indicates where to read left USB data in the ring buffer */
			size_t left_read_idx;
			/* Indicates where to read right USB data in the ring buffer */
			size_t right_read_idx;
			size_t right_ring_buf_fail_cnt;
#endif /* CONFIG_USBD_AUDIO2_CLASS */
#endif /* CONFIG_LIBLC3 */
		} tx;
#endif /* CONFIG_BT_AUDIO_TX */

#if defined(CONFIG_BT_AUDIO_RX)
		struct {
			struct bt_iso_recv_info last_info;
			size_t empty_sdu_pkts;
			size_t valid_sdu_pkts;
			size_t lost_pkts;
			size_t err_pkts;
			size_t dup_psn;
			size_t rx_cnt;
			size_t dup_ts;
#if defined(CONFIG_LIBLC3)
			lc3_decoder_mem_48k_t lc3_decoder_mem;
			lc3_decoder_t lc3_decoder;
			size_t decoded_cnt;
#endif /* CONFIG_LIBLC3 */
		} rx;
#endif /* CONFIG_BT_AUDIO_RX */
	};
};

const struct named_lc3_preset *bap_get_named_preset(bool is_unicast, enum bt_audio_dir dir,
						    const char *preset_arg);

size_t bap_get_rx_streaming_cnt(void);
size_t bap_get_tx_streaming_cnt(void);
void bap_foreach_stream(void (*func)(struct shell_stream *sh_stream, void *data), void *data);

int bap_usb_init(void);

int bap_usb_add_frame_to_usb(enum bt_audio_location lc3_chan_allocation, const int16_t *frame,
			     size_t frame_size, uint32_t ts);
void bap_usb_clear_frames_to_usb(void);
uint16_t get_next_seq_num(struct bt_bap_stream *bap_stream);
struct shell_stream *shell_stream_from_bap_stream(struct bt_bap_stream *bap_stream);
struct bt_bap_stream *bap_stream_from_shell_stream(struct shell_stream *sh_stream);
bool bap_usb_can_get_full_sdu(struct shell_stream *sh_stream);
void bap_usb_get_frame(struct shell_stream *sh_stream, enum bt_audio_location chan_alloc,
		       int16_t buffer[]);
size_t bap_usb_get_frame_size(const struct shell_stream *sh_stream);

struct broadcast_source {
	bool is_cap;
	union {
		struct bt_bap_broadcast_source *bap_source;
		struct bt_cap_broadcast_source *cap_source;
	};
	struct bt_audio_codec_cfg codec_cfg;
	struct bt_bap_qos_cfg qos;
	uint32_t broadcast_id;
};

struct broadcast_sink {
	struct bt_bap_broadcast_sink *bap_sink;
	struct bt_le_per_adv_sync *pa_sync;
	uint8_t received_base[UINT8_MAX];
	uint8_t base_size;
	uint32_t broadcast_id;
	size_t stream_cnt;
	bool syncable;
};

#define BAP_UNICAST_AC_MAX_CONN   2U
#define BAP_UNICAST_AC_MAX_SNK    (2U * BAP_UNICAST_AC_MAX_CONN)
#define BAP_UNICAST_AC_MAX_SRC    (2U * BAP_UNICAST_AC_MAX_CONN)
#define BAP_UNICAST_AC_MAX_PAIR   MAX(BAP_UNICAST_AC_MAX_SNK, BAP_UNICAST_AC_MAX_SRC)
#define BAP_UNICAST_AC_MAX_STREAM (BAP_UNICAST_AC_MAX_SNK + BAP_UNICAST_AC_MAX_SRC)

#if defined(CONFIG_BT_BAP_UNICAST)

#define UNICAST_SERVER_STREAM_COUNT                                                                \
	COND_CODE_1(CONFIG_BT_ASCS_ASE_SRC, (CONFIG_BT_ASCS_MAX_ASE_SRC_COUNT), (0)) +             \
	COND_CODE_1(CONFIG_BT_ASCS_ASE_SNK, (CONFIG_BT_ASCS_MAX_ASE_SNK_COUNT), (0))

#define UNICAST_CLIENT_STREAM_COUNT                                                                \
	COND_CODE_1(CONFIG_BT_BAP_UNICAST_CLIENT,                                                  \
		    (CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT +                                  \
		     CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT),                                  \
		    (0))

extern struct shell_stream unicast_streams[CONFIG_BT_MAX_CONN * MAX(UNICAST_SERVER_STREAM_COUNT,
								    UNICAST_CLIENT_STREAM_COUNT)];

#if defined(CONFIG_BT_BAP_UNICAST_CLIENT)

struct bap_unicast_ac_param {
	char *name;
	size_t conn_cnt;
	size_t snk_cnt[BAP_UNICAST_AC_MAX_CONN];
	size_t src_cnt[BAP_UNICAST_AC_MAX_CONN];
	size_t snk_chan_cnt;
	size_t src_chan_cnt;
};

extern struct bt_bap_unicast_group *default_unicast_group;
extern struct bt_bap_ep *snks[CONFIG_BT_MAX_CONN][CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT];
extern struct bt_bap_ep *srcs[CONFIG_BT_MAX_CONN][CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT];
extern struct named_lc3_preset default_sink_preset;
extern struct named_lc3_preset default_source_preset;

int bap_ac_create_unicast_group(const struct bap_unicast_ac_param *param,
				struct shell_stream *snk_uni_streams[], size_t snk_cnt,
				struct shell_stream *src_uni_streams[], size_t src_cnt);

int cap_ac_unicast(const struct shell *sh, const struct bap_unicast_ac_param *param);
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT */
#endif /* CONFIG_BT_BAP_UNICAST */

static inline void print_qos(const struct bt_bap_qos_cfg *qos)
{
#if defined(CONFIG_BT_BAP_BROADCAST_SOURCE) || defined(CONFIG_BT_BAP_UNICAST)
	bt_shell_print("QoS: interval %u framing 0x%02x phy 0x%02x sdu %u rtn %u latency %u pd %u",
		       qos->interval, qos->framing, qos->phy, qos->sdu, qos->rtn, qos->latency,
		       qos->pd);
#else
	bt_shell_print("QoS: interval %u framing 0x%02x phy 0x%02x sdu %u rtn %u pd %u",
		       qos->interval, qos->framing, qos->phy, qos->sdu, qos->rtn, qos->pd);
#endif /* CONFIG_BT_BAP_BROADCAST_SOURCE || CONFIG_BT_BAP_UNICAST */
}

struct print_ltv_info {
	size_t indent;
	size_t cnt;
};

static bool print_ltv_elem(struct bt_data *data, void *user_data)
{
	struct print_ltv_info *ltv_info = user_data;
	const size_t elem_indent = ltv_info->indent += SHELL_PRINT_INDENT_LEVEL_SIZE;

	bt_shell_print("%*s#%zu: type 0x%02x value_len %u", ltv_info->indent, "",
		       ltv_info->cnt, data->type, data->data_len);

	bt_shell_fprintf_print("%*s", elem_indent, "");

	for (uint8_t i = 0U; i < data->data_len; i++) {
		bt_shell_fprintf_print("%02X", data->data[i]);
	}

	bt_shell_fprintf_print("\n");

	ltv_info->cnt++;

	return true;
}

static void print_ltv_array(size_t indent, const uint8_t *ltv_data, size_t ltv_data_len)
{
	struct print_ltv_info ltv_info = {
		.cnt = 0U,
		.indent = indent,
	};
	int err;

	err = bt_audio_data_parse(ltv_data, ltv_data_len, print_ltv_elem, &ltv_info);
	if (err != 0 && err != -ECANCELED) {
		bt_shell_error("%*sInvalid LTV data: %d", indent, "", err);
	}
}

static inline void print_codec_meta_pref_context(size_t indent, enum bt_audio_context context)
{
	bt_shell_print("%*sPreferred audio contexts:", indent, "");

	indent += SHELL_PRINT_INDENT_LEVEL_SIZE;

	/* There can be up to 16 bits set in the field */
	for (size_t i = 0U; i < 16; i++) {
		const uint16_t bit_val = BIT(i);

		if (context & bit_val) {
			bt_shell_print("%*s%s (0x%04X)", indent, "",
				       bt_audio_context_bit_to_str(bit_val), bit_val);
		}
	}
}

static inline void print_codec_meta_stream_context(size_t indent, enum bt_audio_context context)
{
	bt_shell_print("%*sStreaming audio contexts:", indent, "");

	indent += SHELL_PRINT_INDENT_LEVEL_SIZE;

	/* There can be up to 16 bits set in the field */
	for (size_t i = 0U; i < 16; i++) {
		const uint16_t bit_val = BIT(i);

		if (context & bit_val) {
			bt_shell_print("%*s%s (0x%04X)", indent, "",
				       bt_audio_context_bit_to_str(bit_val), bit_val);
		}
	}
}

static inline void print_codec_meta_program_info(size_t indent, const uint8_t *program_info,
						 uint8_t program_info_len)
{
	bt_shell_print("%*sProgram info:", indent, "");

	indent += SHELL_PRINT_INDENT_LEVEL_SIZE;

	bt_shell_fprintf_print("%*s", indent, "");
	for (uint8_t i = 0U; i < program_info_len; i++) {
		bt_shell_fprintf_print("%c", (char)program_info[i]);
	}

	bt_shell_fprintf_print("\n");
}

static inline void print_codec_meta_language(size_t indent,
					     const uint8_t lang[BT_AUDIO_LANG_SIZE])
{
	bt_shell_print("%*sLanguage: %c%c%c", indent, "", (char)lang[0], (char)lang[1],
		       (char)lang[2]);
}

static inline void print_codec_meta_ccid_list(size_t indent, const uint8_t *ccid_list,
					      uint8_t ccid_list_len)
{
	bt_shell_print("%*sCCID list:", indent, "");

	indent += SHELL_PRINT_INDENT_LEVEL_SIZE;

	/* There can be up to 16 bits set in the field */
	for (uint8_t i = 0U; i < ccid_list_len; i++) {
		bt_shell_print("%*s0x%02X ", indent, "", ccid_list[i]);
	}
}

static inline void print_codec_meta_parental_rating(size_t indent,
						    enum bt_audio_parental_rating parental_rating)
{
	bt_shell_print("%*sRating: %s (0x%02X)", indent, "",
		       bt_audio_parental_rating_to_str(parental_rating), (uint8_t)parental_rating);
}

static inline void print_codec_meta_program_info_uri(size_t indent,
						     const uint8_t *program_info_uri,
						     uint8_t program_info_uri_len)
{
	bt_shell_print("%*sProgram info URI:", indent, "");

	indent += SHELL_PRINT_INDENT_LEVEL_SIZE;

	bt_shell_fprintf_print("%*s", indent, "");

	for (uint8_t i = 0U; i < program_info_uri_len; i++) {
		bt_shell_fprintf_print("%c", (char)program_info_uri[i]);
	}

	bt_shell_fprintf_print("\n");
}

static inline void print_codec_meta_audio_active_state(size_t indent,
						       enum bt_audio_active_state state)
{
	bt_shell_print("%*sAudio active state: %s (0x%02X)", indent, "",
		       bt_audio_active_state_to_str(state), (uint8_t)state);
}

static inline void print_codec_meta_bcast_audio_immediate_rend_flag(size_t indent)
{
	bt_shell_print("%*sBroadcast audio immediate rendering flag set", indent, "");
}

static inline void print_codec_meta_extended(size_t indent, const uint8_t *extended_meta,
					     size_t extended_meta_len)
{
	bt_shell_print("%*sExtended metadata:", indent, "");

	indent += SHELL_PRINT_INDENT_LEVEL_SIZE;

	bt_shell_fprintf_print("%*s", indent, "");

	for (uint8_t i = 0U; i < extended_meta_len; i++) {
		bt_shell_fprintf_print("%u", (uint8_t)extended_meta[i]);
	}

	bt_shell_fprintf_print("\n");
}

static inline void print_codec_meta_vendor(size_t indent, const uint8_t *vendor_meta,
					   size_t vender_meta_len)
{
	bt_shell_print("%*sVendor metadata:", indent, "");

	indent += SHELL_PRINT_INDENT_LEVEL_SIZE;

	bt_shell_fprintf_print("%*s", indent, "");

	for (uint8_t i = 0U; i < vender_meta_len; i++) {
		bt_shell_fprintf_print("%u", (uint8_t)vendor_meta[i]);
	}

	bt_shell_fprintf_print("\n");
}

static inline void print_codec_cap_freq(size_t indent, enum bt_audio_codec_cap_freq freq)
{
	bt_shell_print("%*sSupported sampling frequencies:", indent, "");

	indent += SHELL_PRINT_INDENT_LEVEL_SIZE;
	/* There can be up to 16 bits set in the field */
	for (size_t i = 0; i < 16; i++) {
		const uint16_t bit_val = BIT(i);

		if (freq & bit_val) {
			bt_shell_print("%*s%s (0x%04X)", indent, "",
				       bt_audio_codec_cap_freq_bit_to_str(bit_val), bit_val);
		}
	}
}

static inline void print_codec_cap_frame_dur(size_t indent,
					     enum bt_audio_codec_cap_frame_dur frame_dur)
{
	bt_shell_print("%*sSupported frame durations:", indent, "");

	indent += SHELL_PRINT_INDENT_LEVEL_SIZE;
	/* There can be up to 8 bits set in the field */
	for (size_t i = 0; i < 8; i++) {
		const uint8_t bit_val = BIT(i);

		if (frame_dur & bit_val) {
			bt_shell_print("%*s%s (0x%02X)", indent, "",
				       bt_audio_codec_cap_frame_dur_bit_to_str(bit_val), bit_val);
		}
	}
}

static inline void print_codec_cap_chan_count(size_t indent,
					      enum bt_audio_codec_cap_chan_count chan_count)
{
	bt_shell_print("%*sSupported channel counts:", indent, "");

	indent += SHELL_PRINT_INDENT_LEVEL_SIZE;
	/* There can be up to 8 bits set in the field */
	for (size_t i = 0; i < 8; i++) {
		const uint8_t bit_val = BIT(i);

		if (chan_count & bit_val) {
			bt_shell_print("%*s%s (0x%02X)", indent, "",
				       bt_audio_codec_cap_chan_count_bit_to_str(bit_val), bit_val);
		}
	}
}

static inline void print_codec_cap_octets_per_codec_frame(
	size_t indent, const struct bt_audio_codec_octets_per_codec_frame *codec_frame)
{
	bt_shell_print("%*sSupported octets per codec frame counts:", indent, "");

	indent += SHELL_PRINT_INDENT_LEVEL_SIZE;
	bt_shell_print("%*sMin: %u", indent, "", codec_frame->min);
	bt_shell_print("%*sMax: %u", indent, "", codec_frame->max);
}

static inline void print_codec_cap_max_codec_frames_per_sdu(size_t indent,
							    uint8_t codec_frames_per_sdu)
{
	bt_shell_print("%*sSupported max codec frames per SDU: %u", indent, "",
		       codec_frames_per_sdu);
}

static inline void print_codec_cap(size_t indent, const struct bt_audio_codec_cap *codec_cap)
{
	bt_shell_print("%*scodec cap id 0x%02x cid 0x%04x vid 0x%04x", indent, "", codec_cap->id,
		       codec_cap->cid, codec_cap->vid);

	indent += SHELL_PRINT_INDENT_LEVEL_SIZE;

#if CONFIG_BT_AUDIO_CODEC_CAP_MAX_DATA_SIZE > 0
	bt_shell_print("%*sCodec specific capabilities:", indent, "");

	indent += SHELL_PRINT_INDENT_LEVEL_SIZE;
	if (codec_cap->data_len == 0U) {
		bt_shell_print("%*sNone", indent, "");
	} else if (codec_cap->id == BT_HCI_CODING_FORMAT_LC3) {
		struct bt_audio_codec_octets_per_codec_frame codec_frame;
		int ret;

		ret = bt_audio_codec_cap_get_freq(codec_cap);
		if (ret >= 0) {
			print_codec_cap_freq(indent, (enum bt_audio_codec_cap_freq)ret);
		}

		ret = bt_audio_codec_cap_get_frame_dur(codec_cap);
		if (ret >= 0) {
			print_codec_cap_frame_dur(indent,
						  (enum bt_audio_codec_cap_frame_dur)ret);
		}

		ret = bt_audio_codec_cap_get_supported_audio_chan_counts(codec_cap, true);
		if (ret >= 0) {
			print_codec_cap_chan_count(indent,
						   (enum bt_audio_codec_cap_chan_count)ret);
		}

		ret = bt_audio_codec_cap_get_octets_per_frame(codec_cap, &codec_frame);
		if (ret >= 0) {
			print_codec_cap_octets_per_codec_frame(indent, &codec_frame);
		}

		ret = bt_audio_codec_cap_get_max_codec_frames_per_sdu(codec_cap, true);
		if (ret >= 0) {
			print_codec_cap_max_codec_frames_per_sdu(indent, (uint8_t)ret);
		}
	} else { /* If not LC3, we cannot assume it's LTV */
		bt_shell_fprintf_print("%*s", indent, "");

		for (uint8_t i = 0U; i < codec_cap->data_len; i++) {
			bt_shell_fprintf_print("%*s%02X", indent, "", codec_cap->data[i]);
		}

		bt_shell_fprintf_print("\n");
	}

	/* Reduce for metadata*/
	indent -= SHELL_PRINT_INDENT_LEVEL_SIZE;
#endif /* CONFIG_BT_AUDIO_CODEC_CAP_MAX_DATA_SIZE > 0 */

#if CONFIG_BT_AUDIO_CODEC_CAP_MAX_METADATA_SIZE > 0
	bt_shell_print("%*sCodec capabilities metadata:", indent, "");

	indent += SHELL_PRINT_INDENT_LEVEL_SIZE;
	if (codec_cap->meta_len == 0U) {
		bt_shell_print("%*sNone", indent, "");
	} else {
		const uint8_t *data;
		int ret;

		ret = bt_audio_codec_cap_meta_get_pref_context(codec_cap);
		if (ret >= 0) {
			print_codec_meta_pref_context(indent, (enum bt_audio_context)ret);
		}

		ret = bt_audio_codec_cap_meta_get_stream_context(codec_cap);
		if (ret >= 0) {
			print_codec_meta_stream_context(indent, (enum bt_audio_context)ret);
		}

		ret = bt_audio_codec_cap_meta_get_program_info(codec_cap, &data);
		if (ret >= 0) {
			print_codec_meta_program_info(indent, data, (uint8_t)ret);
		}

		ret = bt_audio_codec_cap_meta_get_lang(codec_cap, &data);
		if (ret >= 0) {
			print_codec_meta_language(indent, data);
		}

		ret = bt_audio_codec_cap_meta_get_ccid_list(codec_cap, &data);
		if (ret >= 0) {
			print_codec_meta_ccid_list(indent, data, (uint8_t)ret);
		}

		ret = bt_audio_codec_cap_meta_get_parental_rating(codec_cap);
		if (ret >= 0) {
			print_codec_meta_parental_rating(indent,
							 (enum bt_audio_parental_rating)ret);
		}

		ret = bt_audio_codec_cap_meta_get_audio_active_state(codec_cap);
		if (ret >= 0) {
			print_codec_meta_audio_active_state(indent,
							    (enum bt_audio_active_state)ret);
		}

		ret = bt_audio_codec_cap_meta_get_bcast_audio_immediate_rend_flag(codec_cap);
		if (ret >= 0) {
			print_codec_meta_bcast_audio_immediate_rend_flag(indent);
		}

		ret = bt_audio_codec_cap_meta_get_extended(codec_cap, &data);
		if (ret >= 0) {
			print_codec_meta_extended(indent, data, (uint8_t)ret);
		}

		ret = bt_audio_codec_cap_meta_get_vendor(codec_cap, &data);
		if (ret >= 0) {
			print_codec_meta_vendor(indent, data, (uint8_t)ret);
		}
	}
#endif /* CONFIG_BT_AUDIO_CODEC_CAP_MAX_METADATA_SIZE > 0 */
}

static inline void print_codec_cfg_freq(size_t indent, enum bt_audio_codec_cfg_freq freq)
{
	bt_shell_print("%*sSampling frequency: %u Hz (0x%04X)", indent, "",
		       bt_audio_codec_cfg_freq_to_freq_hz(freq), (uint16_t)freq);
}

static inline void print_codec_cfg_frame_dur(size_t indent,
					     enum bt_audio_codec_cfg_frame_dur frame_dur)
{
	bt_shell_print("%*sFrame duration: %u us (0x%02X)", indent, "",
		       bt_audio_codec_cfg_frame_dur_to_frame_dur_us(frame_dur), (uint8_t)frame_dur);
}

static inline void print_codec_cfg_chan_allocation(size_t indent,
						   enum bt_audio_location chan_allocation)
{
	bt_shell_print("%*sChannel allocation:", indent, "");

	indent += SHELL_PRINT_INDENT_LEVEL_SIZE;

	if (chan_allocation == BT_AUDIO_LOCATION_MONO_AUDIO) {
		bt_shell_print("%*s Mono", indent, "");
	} else {
		/* There can be up to 32 bits set in the field */
		for (size_t i = 0; i < 32; i++) {
			const uint8_t bit_val = BIT(i);

			if (chan_allocation & bit_val) {
				bt_shell_print("%*s%s (0x%08X)", indent, "",
					       bt_audio_location_bit_to_str(bit_val), bit_val);
			}
		}
	}
}

static inline void print_codec_cfg_octets_per_frame(size_t indent, uint16_t octets_per_frame)
{
	bt_shell_print("%*sOctets per codec frame: %u", indent, "", octets_per_frame);
}

static inline void print_codec_cfg_frame_blocks_per_sdu(size_t indent,
							uint8_t frame_blocks_per_sdu)
{
	bt_shell_print("%*sCodec frame blocks per SDU: %u", indent, "", frame_blocks_per_sdu);
}

static inline void print_codec_cfg(size_t indent, const struct bt_audio_codec_cfg *codec_cfg)
{
	bt_shell_print("%*scodec cfg id 0x%02x cid 0x%04x vid 0x%04x count %u", indent, "",
		       codec_cfg->id, codec_cfg->cid, codec_cfg->vid, codec_cfg->data_len);

	indent += SHELL_PRINT_INDENT_LEVEL_SIZE;

#if CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE > 0
	bt_shell_print("%*sCodec specific configuration:", indent, "");

	indent += SHELL_PRINT_INDENT_LEVEL_SIZE;
	if (codec_cfg->data_len == 0U) {
		bt_shell_print("%*sNone", indent, "");
	} else if (codec_cfg->id == BT_HCI_CODING_FORMAT_LC3) {
		enum bt_audio_location chan_allocation;
		int ret;

		ret = bt_audio_codec_cfg_get_freq(codec_cfg);
		if (ret >= 0) {
			print_codec_cfg_freq(indent, (enum bt_audio_codec_cfg_freq)ret);
		}

		ret = bt_audio_codec_cfg_get_frame_dur(codec_cfg);
		if (ret >= 0) {
			print_codec_cfg_frame_dur(indent,
						  (enum bt_audio_codec_cfg_frame_dur)ret);
		}

		ret = bt_audio_codec_cfg_get_chan_allocation(codec_cfg, &chan_allocation, false);
		if (ret == 0) {
			print_codec_cfg_chan_allocation(indent, chan_allocation);
		}

		ret = bt_audio_codec_cfg_get_octets_per_frame(codec_cfg);
		if (ret >= 0) {
			print_codec_cfg_octets_per_frame(indent, (uint16_t)ret);
		}

		ret = bt_audio_codec_cfg_get_frame_blocks_per_sdu(codec_cfg, false);
		if (ret >= 0) {
			print_codec_cfg_frame_blocks_per_sdu(indent, (uint8_t)ret);
		}
	} else { /* If not LC3, we cannot assume it's LTV */
		bt_shell_fprintf_print("%*s", indent, "");

		for (uint8_t i = 0U; i < codec_cfg->data_len; i++) {
			bt_shell_fprintf_print("%*s%02X", indent, "", codec_cfg->data[i]);
		}

		bt_shell_fprintf_print("\n");
	}

	/* Reduce for metadata*/
	indent -= SHELL_PRINT_INDENT_LEVEL_SIZE;
#endif /* CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE > 0 */

#if CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE > 0
	bt_shell_print("%*sCodec specific metadata:", indent, "");

	indent += SHELL_PRINT_INDENT_LEVEL_SIZE;
	if (codec_cfg->meta_len == 0U) {
		bt_shell_print("%*sNone", indent, "");
	} else {
		const uint8_t *data;
		int ret;

		ret = bt_audio_codec_cfg_meta_get_pref_context(codec_cfg, true);
		if (ret >= 0) {
			print_codec_meta_pref_context(indent, (enum bt_audio_context)ret);
		}

		ret = bt_audio_codec_cfg_meta_get_stream_context(codec_cfg);
		if (ret >= 0) {
			print_codec_meta_stream_context(indent, (enum bt_audio_context)ret);
		}

		ret = bt_audio_codec_cfg_meta_get_program_info(codec_cfg, &data);
		if (ret >= 0) {
			print_codec_meta_program_info(indent, data, (uint8_t)ret);
		}

		ret = bt_audio_codec_cfg_meta_get_lang(codec_cfg, &data);
		if (ret >= 0) {
			print_codec_meta_language(indent, data);
		}

		ret = bt_audio_codec_cfg_meta_get_ccid_list(codec_cfg, &data);
		if (ret >= 0) {
			print_codec_meta_ccid_list(indent, data, (uint8_t)ret);
		}

		ret = bt_audio_codec_cfg_meta_get_parental_rating(codec_cfg);
		if (ret >= 0) {
			print_codec_meta_parental_rating(indent,
							 (enum bt_audio_parental_rating)ret);
		}

		ret = bt_audio_codec_cfg_meta_get_audio_active_state(codec_cfg);
		if (ret >= 0) {
			print_codec_meta_audio_active_state(indent,
							    (enum bt_audio_active_state)ret);
		}

		ret = bt_audio_codec_cfg_meta_get_bcast_audio_immediate_rend_flag(codec_cfg);
		if (ret >= 0) {
			print_codec_meta_bcast_audio_immediate_rend_flag(indent);
		}

		ret = bt_audio_codec_cfg_meta_get_extended(codec_cfg, &data);
		if (ret >= 0) {
			print_codec_meta_extended(indent, data, (uint8_t)ret);
		}

		ret = bt_audio_codec_cfg_meta_get_vendor(codec_cfg, &data);
		if (ret >= 0) {
			print_codec_meta_vendor(indent, data, (uint8_t)ret);
		}
	}
#endif /* CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE > 0 */
}

#if defined(CONFIG_BT_BAP_BROADCAST_SOURCE)
struct bap_broadcast_ac_param {
	char *name;
	size_t stream_cnt;
	size_t chan_cnt;
};

int cap_ac_broadcast(const struct shell *sh, size_t argc, char **argv,
		     const struct bap_broadcast_ac_param *param);

extern struct shell_stream broadcast_source_streams[CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT];
extern struct broadcast_source default_source;
extern struct named_lc3_preset default_broadcast_source_preset;
#endif /* CONFIG_BT_BAP_BROADCAST_SOURCE */

static inline bool print_base_subgroup_bis_cb(const struct bt_bap_base_subgroup_bis *bis,
					      void *user_data)
{
	size_t indent = 2 * SHELL_PRINT_INDENT_LEVEL_SIZE;
	struct bt_bap_base_codec_id *codec_id = user_data;

	bt_shell_print("%*sBIS index: 0x%02X", indent, "", bis->index);

	indent += SHELL_PRINT_INDENT_LEVEL_SIZE;

	/* Print CC data */
	if (codec_id->id == BT_HCI_CODING_FORMAT_LC3) {
		struct bt_audio_codec_cfg codec_cfg = {
			.id = codec_id->id,
			.cid = codec_id->cid,
			.vid = codec_id->vid,
		};
		int err;

		err = bt_bap_base_subgroup_bis_codec_to_codec_cfg(bis, &codec_cfg);
		if (err == 0) {
			print_codec_cfg(indent, &codec_cfg);
		} else {
			bt_shell_print("%*sCodec specific configuration:", indent, "");
			print_ltv_array(indent, bis->data, bis->data_len);
		}
	} else { /* If not LC3, we cannot assume it's LTV */
		bt_shell_print("%*sCodec specific configuration:", indent, "");

		indent += SHELL_PRINT_INDENT_LEVEL_SIZE;
		bt_shell_fprintf_print("%*s", indent, "");

		for (uint8_t i = 0U; i < bis->data_len; i++) {
			bt_shell_fprintf_print("%02X", bis->data[i]);
		}

		bt_shell_fprintf_print("\n");
	}

	return true;
}

static inline bool print_base_subgroup_cb(const struct bt_bap_base_subgroup *subgroup,
					  void *user_data)
{
	size_t indent = 1 * SHELL_PRINT_INDENT_LEVEL_SIZE;
	struct bt_bap_base_codec_id codec_id;
	struct bt_audio_codec_cfg codec_cfg;
	uint8_t *data;
	int ret;

	bt_shell_print("Subgroup %p:", subgroup);

	ret = bt_bap_base_get_subgroup_codec_id(subgroup, &codec_id);
	if (ret < 0) {
		return false;
	}

	bt_shell_print("%*sCodec Format: 0x%02X", indent, "", codec_id.id);
	bt_shell_print("%*sCompany ID  : 0x%04X", indent, "", codec_id.cid);
	bt_shell_print("%*sVendor ID   : 0x%04X", indent, "", codec_id.vid);

	ret = bt_bap_base_subgroup_codec_to_codec_cfg(subgroup, &codec_cfg);
	if (ret == 0) {
		print_codec_cfg(indent, &codec_cfg);
	} else {
		/* If we cannot store it in a codec_cfg, then we cannot easily print it as such */
		ret = bt_bap_base_get_subgroup_codec_data(subgroup, &data);
		if (ret < 0) {
			return false;
		}

		bt_shell_print("%*sCodec specific configuration:", indent, "");
		indent += SHELL_PRINT_INDENT_LEVEL_SIZE;

		/* Print CC data */
		if (codec_id.id == BT_HCI_CODING_FORMAT_LC3) {
			print_ltv_array(indent, data, (uint8_t)ret);
		} else { /* If not LC3, we cannot assume it's LTV */
			bt_shell_fprintf_print("%*s", indent, "");

			for (uint8_t i = 0U; i < (uint8_t)ret; i++) {
				bt_shell_fprintf_print("%c", data[i]);
			}

			bt_shell_fprintf_print("\n");
		}

		ret = bt_bap_base_get_subgroup_codec_meta(subgroup, &data);
		if (ret < 0) {
			return false;
		}

		bt_shell_print(
			"%*sCodec specific metadata:", indent - SHELL_PRINT_INDENT_LEVEL_SIZE,
			"");

		/* Print metadata */
		if (codec_id.id == BT_HCI_CODING_FORMAT_LC3) {
			print_ltv_array(indent, data, (uint8_t)ret);
		} else { /* If not LC3, we cannot assume it's LTV */
			bt_shell_fprintf_print("%*s", indent, "");

			for (uint8_t i = 0U; i < (uint8_t)ret; i++) {
				bt_shell_fprintf_print("%c", data[i]);
			}

			bt_shell_fprintf_print("\n");
		}
	}

	ret = bt_bap_base_subgroup_foreach_bis(subgroup, print_base_subgroup_bis_cb, &codec_id);
	if (ret < 0) {
		return false;
	}

	return true;
}

static inline void print_base(const struct bt_bap_base *base)
{
	int err;

	bt_shell_print("Presentation delay: %d", bt_bap_base_get_pres_delay(base));
	bt_shell_print("Subgroup count: %d", bt_bap_base_get_subgroup_count(base));

	err = bt_bap_base_foreach_subgroup(base, print_base_subgroup_cb, NULL);
	if (err < 0) {
		bt_shell_info("Invalid BASE: %d", err);
	}
}

static inline void copy_unicast_stream_preset(struct shell_stream *stream,
					      const struct named_lc3_preset *named_preset)
{
	memcpy(&stream->qos, &named_preset->preset.qos, sizeof(stream->qos));
	memcpy(&stream->codec_cfg, &named_preset->preset.codec_cfg, sizeof(stream->codec_cfg));
}

static inline void copy_broadcast_source_preset(struct broadcast_source *source,
						const struct named_lc3_preset *named_preset)
{
	memcpy(&source->qos, &named_preset->preset.qos, sizeof(source->qos));
	memcpy(&source->codec_cfg, &named_preset->preset.codec_cfg, sizeof(source->codec_cfg));
}
#endif /* CONFIG_BT_AUDIO */

#endif /* AUDIO_SHELL_AUDIO_H */
