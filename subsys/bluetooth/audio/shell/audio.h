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

#ifndef __AUDIO_H
#define __AUDIO_H

#include <stdio.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>

#include "shell/bt.h"

#define SHELL_PRINT_INDENT_LEVEL_SIZE 2

extern struct bt_csip_set_member_svc_inst *svc_inst;

ssize_t audio_ad_data_add(struct bt_data *data, const size_t data_size, const bool discoverable,
			  const bool connectable);
ssize_t audio_pa_data_add(struct bt_data *data_array, const size_t data_array_size);
ssize_t csis_ad_data_add(struct bt_data *data, const size_t data_size, const bool discoverable);
size_t cap_acceptor_ad_data_add(struct bt_data data[], size_t data_size, bool discoverable);
size_t gmap_ad_data_add(struct bt_data data[], size_t data_size);
size_t pbp_ad_data_add(struct bt_data data[], size_t data_size);
ssize_t cap_initiator_ad_data_add(struct bt_data *data_array, const size_t data_array_size,
				  const bool discoverable, const bool connectable);
ssize_t cap_initiator_pa_data_add(struct bt_data *data_array, const size_t data_array_size);

#if defined(CONFIG_BT_AUDIO)
/* Must guard before including audio.h as audio.h uses Kconfigs guarded by
 * CONFIG_BT_AUDIO
 */
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/bluetooth/audio/cap.h>

#define LOCATION BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT
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

const struct named_lc3_preset *bap_get_named_preset(bool is_unicast, enum bt_audio_dir dir,
						    const char *preset_arg);

struct shell_stream {
	struct bt_cap_stream stream;
	struct bt_audio_codec_cfg codec_cfg;
	struct bt_audio_codec_qos qos;
#if defined(CONFIG_LIBLC3)
	uint32_t lc3_freq_hz;
	uint32_t lc3_frame_duration_us;
	uint16_t lc3_octets_per_frame;
	uint8_t lc3_frames_per_sdu;
#endif /* CONFIG_LIBLC3 */
#if defined(CONFIG_BT_AUDIO_TX)
	int64_t connected_at_ticks; /* The uptime tick measured when stream was connected */
	uint16_t seq_num;
	struct k_work_delayable audio_send_work;
	bool tx_active;
#if defined(CONFIG_LIBLC3)
	atomic_t lc3_enqueue_cnt;
	size_t lc3_sdu_cnt;
#endif /* CONFIG_LIBLC3 */
#endif /* CONFIG_BT_AUDIO_TX */
#if defined(CONFIG_BT_AUDIO_RX)
	struct bt_iso_recv_info last_info;
	size_t empty_sdu_pkts;
	size_t lost_pkts;
	size_t err_pkts;
	size_t dup_psn;
	size_t rx_cnt;
	size_t dup_ts;
#endif /* CONFIG_BT_AUDIO_RX */
};

struct broadcast_source {
	bool is_cap;
	union {
		struct bt_bap_broadcast_source *bap_source;
		struct bt_cap_broadcast_source *cap_source;
	};
	struct bt_audio_codec_cfg codec_cfg;
	struct bt_audio_codec_qos qos;
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
	COND_CODE_1(CONFIG_BT_ASCS, (CONFIG_BT_ASCS_ASE_SNK_COUNT + CONFIG_BT_ASCS_ASE_SRC_COUNT), \
		    (0))
#define UNICAST_CLIENT_STREAM_COUNT                                                                \
	COND_CODE_1(CONFIG_BT_BAP_UNICAST_CLIENT,                                                  \
		    (CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT +                                  \
		     CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT),                                  \
		    (0))

extern struct shell_stream unicast_streams[CONFIG_BT_MAX_CONN * (UNICAST_SERVER_STREAM_COUNT +
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

static inline void print_qos(const struct shell *sh, const struct bt_audio_codec_qos *qos)
{
#if defined(CONFIG_BT_BAP_BROADCAST_SOURCE) || defined(CONFIG_BT_BAP_UNICAST)
	shell_print(sh,
		    "QoS: interval %u framing 0x%02x phy 0x%02x sdu %u rtn %u latency %u pd %u",
		    qos->interval, qos->framing, qos->phy, qos->sdu, qos->rtn, qos->latency,
		    qos->pd);
#else
	shell_print(sh, "QoS: interval %u framing 0x%02x phy 0x%02x sdu %u rtn %u pd %u",
		    qos->interval, qos->framing, qos->phy, qos->sdu, qos->rtn, qos->pd);
#endif /* CONFIG_BT_BAP_BROADCAST_SOURCE || CONFIG_BT_BAP_UNICAST */
}

struct print_ltv_info {
	const struct shell *sh;
	size_t indent;
	size_t cnt;
};

static bool print_ltv_elem(struct bt_data *data, void *user_data)
{
	struct print_ltv_info *ltv_info = user_data;
	const size_t elem_indent = ltv_info->indent += SHELL_PRINT_INDENT_LEVEL_SIZE;

	shell_print(ltv_info->sh, "%*s#%zu: type 0x%02x value_len %u", ltv_info->indent, "",
		    ltv_info->cnt, data->type, data->data_len);

	shell_fprintf(ltv_info->sh, SHELL_NORMAL, "%*s", elem_indent, "");

	for (uint8_t i = 0U; i < data->data_len; i++) {
		shell_fprintf(ltv_info->sh, SHELL_NORMAL, "%02X", data->data[i]);
	}

	shell_fprintf(ltv_info->sh, SHELL_NORMAL, "\n");

	ltv_info->cnt++;

	return true;
}

static void print_ltv_array(const struct shell *sh, size_t indent, const uint8_t *ltv_data,
			    size_t ltv_data_len)
{
	struct print_ltv_info ltv_info = {
		.sh = sh,
		.cnt = 0U,
		.indent = indent,
	};

	bt_audio_data_parse(ltv_data, ltv_data_len, print_ltv_elem, &ltv_info);
}

static inline char *context_bit_to_str(enum bt_audio_context context)
{
	switch (context) {
	case BT_AUDIO_CONTEXT_TYPE_PROHIBITED:
		return "Prohibited";
	case BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED:
		return "Unspecified";
	case BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL:
		return "Converstation";
	case BT_AUDIO_CONTEXT_TYPE_MEDIA:
		return "Media";
	case BT_AUDIO_CONTEXT_TYPE_GAME:
		return "Game";
	case BT_AUDIO_CONTEXT_TYPE_INSTRUCTIONAL:
		return "Instructional";
	case BT_AUDIO_CONTEXT_TYPE_VOICE_ASSISTANTS:
		return "Voice assistant";
	case BT_AUDIO_CONTEXT_TYPE_LIVE:
		return "Live";
	case BT_AUDIO_CONTEXT_TYPE_SOUND_EFFECTS:
		return "Sound effects";
	case BT_AUDIO_CONTEXT_TYPE_NOTIFICATIONS:
		return "Notifications";
	case BT_AUDIO_CONTEXT_TYPE_RINGTONE:
		return "Ringtone";
	case BT_AUDIO_CONTEXT_TYPE_ALERTS:
		return "Alerts";
	case BT_AUDIO_CONTEXT_TYPE_EMERGENCY_ALARM:
		return "Emergency alarm";
	default:
		return "Unknown context";
	}
}

static inline void print_codec_meta_pref_context(const struct shell *sh, size_t indent,
						 enum bt_audio_context context)
{
	shell_print(sh, "%*sPreferred audio contexts:", indent, "");

	indent += SHELL_PRINT_INDENT_LEVEL_SIZE;

	/* There can be up to 16 bits set in the field */
	for (size_t i = 0U; i < 16; i++) {
		const uint16_t bit_val = BIT(i);

		if (context & bit_val) {
			shell_print(sh, "%*s%s (0x%04X)", indent, "", context_bit_to_str(bit_val),
				    bit_val);
		}
	}
}

static inline void print_codec_meta_stream_context(const struct shell *sh, size_t indent,
						   enum bt_audio_context context)
{
	shell_print(sh, "%*sStreaming audio contexts:", indent, "");

	indent += SHELL_PRINT_INDENT_LEVEL_SIZE;

	/* There can be up to 16 bits set in the field */
	for (size_t i = 0U; i < 16; i++) {
		const uint16_t bit_val = BIT(i);

		if (context & bit_val) {
			shell_print(sh, "%*s%s (0x%04X)", indent, "", context_bit_to_str(bit_val),
				    bit_val);
		}
	}
}

static inline void print_codec_meta_program_info(const struct shell *sh, size_t indent,
						 const uint8_t *program_info,
						 uint8_t program_info_len)
{
	shell_print(sh, "%*sProgram info:", indent, "");

	indent += SHELL_PRINT_INDENT_LEVEL_SIZE;

	shell_fprintf(sh, SHELL_NORMAL, "%*s", indent, "");
	for (uint8_t i = 0U; i < program_info_len; i++) {
		shell_fprintf(sh, SHELL_NORMAL, "%c", (char)program_info[i]);
	}

	shell_fprintf(sh, SHELL_NORMAL, "\n");
}

static inline void print_codec_meta_language(const struct shell *sh, size_t indent,
					     uint32_t stream_lang)
{
	uint8_t lang_array[3];

	sys_put_be24(stream_lang, lang_array);

	shell_print(sh, "%*sLanguage: %c%c%c", indent, "", (char)lang_array[0], (char)lang_array[1],
		    (char)lang_array[2]);
}

static inline void print_codec_meta_ccid_list(const struct shell *sh, size_t indent,
					      const uint8_t *ccid_list, uint8_t ccid_list_len)
{
	shell_print(sh, "%*sCCID list:", indent, "");

	indent += SHELL_PRINT_INDENT_LEVEL_SIZE;

	/* There can be up to 16 bits set in the field */
	for (uint8_t i = 0U; i < ccid_list_len; i++) {
		shell_print(sh, "%*s0x%02X ", indent, "", ccid_list[i]);
	}
}

static inline char *parental_rating_to_str(enum bt_audio_parental_rating parental_rating)
{
	switch (parental_rating) {
	case BT_AUDIO_PARENTAL_RATING_NO_RATING:
		return "No rating";
	case BT_AUDIO_PARENTAL_RATING_AGE_ANY:
		return "Any";
	case BT_AUDIO_PARENTAL_RATING_AGE_5_OR_ABOVE:
		return "Age 5 or above";
	case BT_AUDIO_PARENTAL_RATING_AGE_6_OR_ABOVE:
		return "Age 6 or above";
	case BT_AUDIO_PARENTAL_RATING_AGE_7_OR_ABOVE:
		return "Age 7 or above";
	case BT_AUDIO_PARENTAL_RATING_AGE_8_OR_ABOVE:
		return "Age 8 or above";
	case BT_AUDIO_PARENTAL_RATING_AGE_9_OR_ABOVE:
		return "Age 9 or above";
	case BT_AUDIO_PARENTAL_RATING_AGE_10_OR_ABOVE:
		return "Age 10 or above";
	case BT_AUDIO_PARENTAL_RATING_AGE_11_OR_ABOVE:
		return "Age 11 or above";
	case BT_AUDIO_PARENTAL_RATING_AGE_12_OR_ABOVE:
		return "Age 12 or above";
	case BT_AUDIO_PARENTAL_RATING_AGE_13_OR_ABOVE:
		return "Age 13 or above";
	case BT_AUDIO_PARENTAL_RATING_AGE_14_OR_ABOVE:
		return "Age 14 or above";
	case BT_AUDIO_PARENTAL_RATING_AGE_15_OR_ABOVE:
		return "Age 15 or above";
	case BT_AUDIO_PARENTAL_RATING_AGE_16_OR_ABOVE:
		return "Age 16 or above";
	case BT_AUDIO_PARENTAL_RATING_AGE_17_OR_ABOVE:
		return "Age 17 or above";
	case BT_AUDIO_PARENTAL_RATING_AGE_18_OR_ABOVE:
		return "Age 18 or above";
	default:
		return "Unknown rating";
	}
}

static inline void print_codec_meta_parental_rating(const struct shell *sh, size_t indent,
						    enum bt_audio_parental_rating parental_rating)
{
	shell_print(sh, "%*sRating: %s (0x%02X)", indent, "",
		    parental_rating_to_str(parental_rating), (uint8_t)parental_rating);
}

static inline void print_codec_meta_program_info_uri(const struct shell *sh, size_t indent,
						     const uint8_t *program_info_uri,
						     uint8_t program_info_uri_len)
{
	shell_print(sh, "%*sProgram info URI:", indent, "");

	indent += SHELL_PRINT_INDENT_LEVEL_SIZE;

	shell_fprintf(sh, SHELL_NORMAL, "%*s", indent, "");

	for (uint8_t i = 0U; i < program_info_uri_len; i++) {
		shell_fprintf(sh, SHELL_NORMAL, "%c", (char)program_info_uri[i]);
	}

	shell_fprintf(sh, SHELL_NORMAL, "\n");
}

static inline void print_codec_meta_audio_active_state(const struct shell *sh, size_t indent,
						       enum bt_audio_active_state state)
{
	shell_print(sh, "%*sAudio active state: %s (0x%02X)", indent, "",
		    state == BT_AUDIO_ACTIVE_STATE_ENABLED ? "enabled" : "disabled",
		    (uint8_t)state);
}

static inline void print_codec_meta_bcast_audio_immediate_rend_flag(const struct shell *sh,
								    size_t indent)
{
	shell_print(sh, "%*sBroadcast audio immediate rendering flag set", indent, "");
}

static inline void print_codec_meta_extended(const struct shell *sh, size_t indent,
					     const uint8_t *extended_meta, size_t extended_meta_len)
{
	shell_print(sh, "%*sExtended metadata:", indent, "");

	indent += SHELL_PRINT_INDENT_LEVEL_SIZE;

	shell_fprintf(sh, SHELL_NORMAL, "%*s", indent, "");

	for (uint8_t i = 0U; i < extended_meta_len; i++) {
		shell_fprintf(sh, SHELL_NORMAL, "%u", (uint8_t)extended_meta[i]);
	}

	shell_fprintf(sh, SHELL_NORMAL, "\n");
}

static inline void print_codec_meta_vendor(const struct shell *sh, size_t indent,
					   const uint8_t *vendor_meta, size_t vender_meta_len)
{
	shell_print(sh, "%*sVendor metadata:", indent, "");

	indent += SHELL_PRINT_INDENT_LEVEL_SIZE;

	shell_fprintf(sh, SHELL_NORMAL, "%*s", indent, "");

	for (uint8_t i = 0U; i < vender_meta_len; i++) {
		shell_fprintf(sh, SHELL_NORMAL, "%u", (uint8_t)vendor_meta[i]);
	}

	shell_fprintf(sh, SHELL_NORMAL, "\n");
}

static inline char *codec_cap_freq_bit_to_str(enum bt_audio_codec_cap_freq freq)
{
	switch (freq) {
	case BT_AUDIO_CODEC_CAP_FREQ_8KHZ:
		return "8000 Hz";
	case BT_AUDIO_CODEC_CAP_FREQ_11KHZ:
		return "11025 Hz";
	case BT_AUDIO_CODEC_CAP_FREQ_16KHZ:
		return "16000 Hz";
	case BT_AUDIO_CODEC_CAP_FREQ_22KHZ:
		return "22050 Hz";
	case BT_AUDIO_CODEC_CAP_FREQ_24KHZ:
		return "24000 Hz";
	case BT_AUDIO_CODEC_CAP_FREQ_32KHZ:
		return "32000 Hz";
	case BT_AUDIO_CODEC_CAP_FREQ_44KHZ:
		return "44100 Hz";
	case BT_AUDIO_CODEC_CAP_FREQ_48KHZ:
		return "48000 Hz";
	case BT_AUDIO_CODEC_CAP_FREQ_88KHZ:
		return "88200 Hz";
	case BT_AUDIO_CODEC_CAP_FREQ_96KHZ:
		return "96000 Hz";
	case BT_AUDIO_CODEC_CAP_FREQ_176KHZ:
		return "176400 Hz";
	case BT_AUDIO_CODEC_CAP_FREQ_192KHZ:
		return "192000 Hz";
	case BT_AUDIO_CODEC_CAP_FREQ_384KHZ:
		return "384000 Hz";
	default:
		return "Unknown supported frequency";
	}
}

static inline void print_codec_cap_freq(const struct shell *sh, size_t indent,
					enum bt_audio_codec_cap_freq freq)
{
	shell_print(sh, "%*sSupported sampling frequencies:", indent, "");

	indent += SHELL_PRINT_INDENT_LEVEL_SIZE;
	/* There can be up to 16 bits set in the field */
	for (size_t i = 0; i < 16; i++) {
		const uint16_t bit_val = BIT(i);

		if (freq & bit_val) {
			shell_print(sh, "%*s%s (0x%04X)", indent, "",
				    codec_cap_freq_bit_to_str(bit_val), bit_val);
		}
	}
}

static inline char *codec_cap_frame_dur_bit_to_str(enum bt_audio_codec_cap_frame_dur frame_dur)
{
	switch (frame_dur) {
	case BT_AUDIO_CODEC_CAP_DURATION_7_5:
		return "7.5 ms";
	case BT_AUDIO_CODEC_CAP_DURATION_10:
		return "10 ms";
	case BT_AUDIO_CODEC_CAP_DURATION_PREFER_7_5:
		return "7.5 ms preferred";
	case BT_AUDIO_CODEC_CAP_DURATION_PREFER_10:
		return "10 ms preferred";
	default:
		return "Unknown frame duration";
	}
}

static inline void print_codec_cap_frame_dur(const struct shell *sh, size_t indent,
					     enum bt_audio_codec_cap_frame_dur frame_dur)
{
	shell_print(sh, "%*sSupported frame durations:", indent, "");

	indent += SHELL_PRINT_INDENT_LEVEL_SIZE;
	/* There can be up to 8 bits set in the field */
	for (size_t i = 0; i < 8; i++) {
		const uint8_t bit_val = BIT(i);

		if (frame_dur & bit_val) {
			shell_print(sh, "%*s%s (0x%02X)", indent, "",
				    codec_cap_frame_dur_bit_to_str(bit_val), bit_val);
		}
	}
}

static inline char *codec_cap_chan_count_bit_to_str(enum bt_audio_codec_cap_chan_count chan_count)
{
	switch (chan_count) {
	case BT_AUDIO_CODEC_CAP_CHAN_COUNT_1:
		return "1 channel";
	case BT_AUDIO_CODEC_CAP_CHAN_COUNT_2:
		return "2 channels";
	case BT_AUDIO_CODEC_CAP_CHAN_COUNT_3:
		return "3 channels";
	case BT_AUDIO_CODEC_CAP_CHAN_COUNT_4:
		return "4 channels";
	case BT_AUDIO_CODEC_CAP_CHAN_COUNT_5:
		return "5 channels";
	case BT_AUDIO_CODEC_CAP_CHAN_COUNT_6:
		return "6 channels";
	case BT_AUDIO_CODEC_CAP_CHAN_COUNT_7:
		return "7 channels";
	case BT_AUDIO_CODEC_CAP_CHAN_COUNT_8:
		return "8 channels";
	default:
		return "Unknown channel count";
	}
}

static inline void print_codec_cap_chan_count(const struct shell *sh, size_t indent,
					      enum bt_audio_codec_cap_chan_count chan_count)
{
	shell_print(sh, "%*sSupported channel counts:", indent, "");

	indent += SHELL_PRINT_INDENT_LEVEL_SIZE;
	/* There can be up to 8 bits set in the field */
	for (size_t i = 0; i < 8; i++) {
		const uint8_t bit_val = BIT(i);

		if (chan_count & bit_val) {
			shell_print(sh, "%*s%s (0x%02X)", indent, "",
				    codec_cap_chan_count_bit_to_str(bit_val), bit_val);
		}
	}
}

static inline void print_codec_cap_octets_per_codec_frame(
	const struct shell *sh, size_t indent,
	const struct bt_audio_codec_octets_per_codec_frame *codec_frame)
{
	shell_print(sh, "%*sSupported octets per codec frame counts:", indent, "");

	indent += SHELL_PRINT_INDENT_LEVEL_SIZE;
	shell_print(sh, "%*sMin: %u", indent, "", codec_frame->min);
	shell_print(sh, "%*sMax: %u", indent, "", codec_frame->max);
}

static inline void print_codec_cap_max_codec_frames_per_sdu(const struct shell *sh, size_t indent,
							    uint8_t codec_frames_per_sdu)
{
	shell_print(sh, "%*sSupported max codec frames per SDU: %u", indent, "",
		    codec_frames_per_sdu);
}

static inline void print_codec_cap(const struct shell *sh, size_t indent,
				   const struct bt_audio_codec_cap *codec_cap)
{
	shell_print(sh, "%*scodec cap id 0x%02x cid 0x%04x vid 0x%04x", indent, "", codec_cap->id,
		    codec_cap->cid, codec_cap->vid);

	indent += SHELL_PRINT_INDENT_LEVEL_SIZE;

#if CONFIG_BT_AUDIO_CODEC_CAP_MAX_DATA_SIZE > 0
	shell_print(sh, "%*sCodec specific capabilities:", indent, "");

	indent += SHELL_PRINT_INDENT_LEVEL_SIZE;
	if (codec_cap->data_len == 0U) {
		shell_print(sh, "%*sNone", indent, "");
	} else if (codec_cap->id == BT_HCI_CODING_FORMAT_LC3) {
		struct bt_audio_codec_octets_per_codec_frame codec_frame;
		int ret;

		ret = bt_audio_codec_cap_get_freq(codec_cap);
		if (ret >= 0) {
			print_codec_cap_freq(sh, indent, (enum bt_audio_codec_cap_freq)ret);
		}

		ret = bt_audio_codec_cap_get_frame_dur(codec_cap);
		if (ret >= 0) {
			print_codec_cap_frame_dur(sh, indent,
						  (enum bt_audio_codec_cap_frame_dur)ret);
		}

		ret = bt_audio_codec_cap_get_supported_audio_chan_counts(codec_cap);
		if (ret >= 0) {
			print_codec_cap_chan_count(sh, indent,
						   (enum bt_audio_codec_cap_chan_count)ret);
		}

		ret = bt_audio_codec_cap_get_octets_per_frame(codec_cap, &codec_frame);
		if (ret >= 0) {
			print_codec_cap_octets_per_codec_frame(sh, indent, &codec_frame);
		}

		ret = bt_audio_codec_cap_get_max_codec_frames_per_sdu(codec_cap);
		if (ret >= 0) {
			print_codec_cap_max_codec_frames_per_sdu(sh, indent, (uint8_t)ret);
		}
	} else { /* If not LC3, we cannot assume it's LTV */
		shell_fprintf(sh, SHELL_NORMAL, "%*s", indent, "");

		for (uint8_t i = 0U; i < codec_cap->data_len; i++) {
			shell_fprintf(sh, SHELL_NORMAL, "%*s%02X", indent, "", codec_cap->data[i]);
		}

		shell_fprintf(sh, SHELL_NORMAL, "\n");
	}

	/* Reduce for metadata*/
	indent -= SHELL_PRINT_INDENT_LEVEL_SIZE;
#endif /* CONFIG_BT_AUDIO_CODEC_CAP_MAX_DATA_SIZE > 0 */

#if CONFIG_BT_AUDIO_CODEC_CAP_MAX_METADATA_SIZE > 0
	shell_print(sh, "%*sCodec capabilities metadata:", indent, "");

	indent += SHELL_PRINT_INDENT_LEVEL_SIZE;
	if (codec_cap->meta_len == 0U) {
		shell_print(sh, "%*sNone", indent, "");
	} else {
		const uint8_t *data;
		int ret;

		ret = bt_audio_codec_cap_meta_get_pref_context(codec_cap);
		if (ret >= 0) {
			print_codec_meta_pref_context(sh, indent, (enum bt_audio_context)ret);
		}

		ret = bt_audio_codec_cap_meta_get_stream_context(codec_cap);
		if (ret >= 0) {
			print_codec_meta_stream_context(sh, indent, (enum bt_audio_context)ret);
		}

		ret = bt_audio_codec_cap_meta_get_program_info(codec_cap, &data);
		if (ret >= 0) {
			print_codec_meta_program_info(sh, indent, data, (uint8_t)ret);
		}

		ret = bt_audio_codec_cap_meta_get_stream_lang(codec_cap);
		if (ret >= 0) {
			print_codec_meta_language(sh, indent, (uint32_t)ret);
		}

		ret = bt_audio_codec_cap_meta_get_ccid_list(codec_cap, &data);
		if (ret >= 0) {
			print_codec_meta_ccid_list(sh, indent, data, (uint8_t)ret);
		}

		ret = bt_audio_codec_cap_meta_get_parental_rating(codec_cap);
		if (ret >= 0) {
			print_codec_meta_parental_rating(sh, indent,
							 (enum bt_audio_parental_rating)ret);
		}

		ret = bt_audio_codec_cap_meta_get_audio_active_state(codec_cap);
		if (ret >= 0) {
			print_codec_meta_audio_active_state(sh, indent,
							    (enum bt_audio_active_state)ret);
		}

		ret = bt_audio_codec_cap_meta_get_bcast_audio_immediate_rend_flag(codec_cap);
		if (ret >= 0) {
			print_codec_meta_bcast_audio_immediate_rend_flag(sh, indent);
		}

		ret = bt_audio_codec_cap_meta_get_extended(codec_cap, &data);
		if (ret >= 0) {
			print_codec_meta_extended(sh, indent, data, (uint8_t)ret);
		}

		ret = bt_audio_codec_cap_meta_get_vendor(codec_cap, &data);
		if (ret >= 0) {
			print_codec_meta_vendor(sh, indent, data, (uint8_t)ret);
		}
	}
#endif /* CONFIG_BT_AUDIO_CODEC_CAP_MAX_METADATA_SIZE > 0 */
}

static inline void print_codec_cfg_freq(const struct shell *sh, size_t indent,
					enum bt_audio_codec_cfg_freq freq)
{
	shell_print(sh, "%*sSampling frequency: %u Hz (0x%04X)", indent, "",
		    bt_audio_codec_cfg_freq_to_freq_hz(freq), (uint16_t)freq);
}

static inline void print_codec_cfg_frame_dur(const struct shell *sh, size_t indent,
					     enum bt_audio_codec_cfg_frame_dur frame_dur)
{
	shell_print(sh, "%*sFrame duration: %u us (0x%02X)", indent, "",
		    bt_audio_codec_cfg_frame_dur_to_frame_dur_us(frame_dur), (uint8_t)frame_dur);
}

static inline char *chan_location_bit_to_str(enum bt_audio_location chan_allocation)
{
	switch (chan_allocation) {
	case BT_AUDIO_LOCATION_MONO_AUDIO:
		return "Mono";
	case BT_AUDIO_LOCATION_FRONT_LEFT:
		return "Front left";
	case BT_AUDIO_LOCATION_FRONT_RIGHT:
		return "Front right";
	case BT_AUDIO_LOCATION_FRONT_CENTER:
		return "Front center";
	case BT_AUDIO_LOCATION_LOW_FREQ_EFFECTS_1:
		return "Low frequency effects 1";
	case BT_AUDIO_LOCATION_BACK_LEFT:
		return "Back left";
	case BT_AUDIO_LOCATION_BACK_RIGHT:
		return "Back right";
	case BT_AUDIO_LOCATION_FRONT_LEFT_OF_CENTER:
		return "Front left of center";
	case BT_AUDIO_LOCATION_FRONT_RIGHT_OF_CENTER:
		return "Front right of center";
	case BT_AUDIO_LOCATION_BACK_CENTER:
		return "Back center";
	case BT_AUDIO_LOCATION_LOW_FREQ_EFFECTS_2:
		return "Low frequency effects 2";
	case BT_AUDIO_LOCATION_SIDE_LEFT:
		return "Side left";
	case BT_AUDIO_LOCATION_SIDE_RIGHT:
		return "Side right";
	case BT_AUDIO_LOCATION_TOP_FRONT_LEFT:
		return "Top front left";
	case BT_AUDIO_LOCATION_TOP_FRONT_RIGHT:
		return "Top front right";
	case BT_AUDIO_LOCATION_TOP_FRONT_CENTER:
		return "Top front center";
	case BT_AUDIO_LOCATION_TOP_CENTER:
		return "Top center";
	case BT_AUDIO_LOCATION_TOP_BACK_LEFT:
		return "Top back left";
	case BT_AUDIO_LOCATION_TOP_BACK_RIGHT:
		return "Top back right";
	case BT_AUDIO_LOCATION_TOP_SIDE_LEFT:
		return "Top side left";
	case BT_AUDIO_LOCATION_TOP_SIDE_RIGHT:
		return "Top side right";
	case BT_AUDIO_LOCATION_TOP_BACK_CENTER:
		return "Top back center";
	case BT_AUDIO_LOCATION_BOTTOM_FRONT_CENTER:
		return "Bottom front center";
	case BT_AUDIO_LOCATION_BOTTOM_FRONT_LEFT:
		return "Bottom front left";
	case BT_AUDIO_LOCATION_BOTTOM_FRONT_RIGHT:
		return "Bottom front right";
	case BT_AUDIO_LOCATION_FRONT_LEFT_WIDE:
		return "Front left wide";
	case BT_AUDIO_LOCATION_FRONT_RIGHT_WIDE:
		return "Front right wde";
	case BT_AUDIO_LOCATION_LEFT_SURROUND:
		return "Left surround";
	case BT_AUDIO_LOCATION_RIGHT_SURROUND:
		return "Right surround";
	default:
		return "Unknown location";
	}
}

static inline void print_codec_cfg_chan_allocation(const struct shell *sh, size_t indent,
						   enum bt_audio_location chan_allocation)
{
	shell_print(sh, "%*sChannel allocation:", indent, "");

	indent += SHELL_PRINT_INDENT_LEVEL_SIZE;

	if (chan_allocation == BT_AUDIO_LOCATION_MONO_AUDIO) {
		shell_print(sh, "%*s Mono", indent, "");
	} else {
		/* There can be up to 32 bits set in the field */
		for (size_t i = 0; i < 32; i++) {
			const uint8_t bit_val = BIT(i);

			if (chan_allocation & bit_val) {
				shell_print(sh, "%*s%s (0x%08X)", indent, "",
					    chan_location_bit_to_str(bit_val), bit_val);
			}
		}
	}
}

static inline void print_codec_cfg_octets_per_frame(const struct shell *sh, size_t indent,
						    uint16_t octets_per_frame)
{
	shell_print(sh, "%*sOctets per codec frame: %u", indent, "", octets_per_frame);
}

static inline void print_codec_cfg_frame_blocks_per_sdu(const struct shell *sh, size_t indent,
							uint8_t frame_blocks_per_sdu)
{
	shell_print(sh, "%*sCodec frame blocks per SDU: %u", indent, "", frame_blocks_per_sdu);
}

static inline void print_codec_cfg(const struct shell *sh, size_t indent,
				   const struct bt_audio_codec_cfg *codec_cfg)
{
	shell_print(sh, "%*scodec cfg id 0x%02x cid 0x%04x vid 0x%04x count %u", indent, "",
		    codec_cfg->id, codec_cfg->cid, codec_cfg->vid, codec_cfg->data_len);

	indent += SHELL_PRINT_INDENT_LEVEL_SIZE;

#if CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE > 0
	shell_print(sh, "%*sCodec specific configuration:", indent, "");

	indent += SHELL_PRINT_INDENT_LEVEL_SIZE;
	if (codec_cfg->data_len == 0U) {
		shell_print(sh, "%*sNone", indent, "");
	} else if (codec_cfg->id == BT_HCI_CODING_FORMAT_LC3) {
		enum bt_audio_location chan_allocation;
		int ret;

		ret = bt_audio_codec_cfg_get_freq(codec_cfg);
		if (ret >= 0) {
			print_codec_cfg_freq(sh, indent, (enum bt_audio_codec_cfg_freq)ret);
		}

		ret = bt_audio_codec_cfg_get_frame_dur(codec_cfg);
		if (ret >= 0) {
			print_codec_cfg_frame_dur(sh, indent,
						  (enum bt_audio_codec_cfg_frame_dur)ret);
		}

		ret = bt_audio_codec_cfg_get_chan_allocation(codec_cfg, &chan_allocation);
		if (ret >= 0) {
			print_codec_cfg_chan_allocation(sh, indent, chan_allocation);
		}

		ret = bt_audio_codec_cfg_get_octets_per_frame(codec_cfg);
		if (ret >= 0) {
			print_codec_cfg_octets_per_frame(sh, indent, (uint16_t)ret);
		}

		ret = bt_audio_codec_cfg_get_frame_blocks_per_sdu(codec_cfg, false);
		if (ret >= 0) {
			print_codec_cfg_frame_blocks_per_sdu(sh, indent, (uint8_t)ret);
		}
	} else { /* If not LC3, we cannot assume it's LTV */
		shell_fprintf(sh, SHELL_NORMAL, "%*s", indent, "");

		for (uint8_t i = 0U; i < codec_cfg->data_len; i++) {
			shell_fprintf(sh, SHELL_NORMAL, "%*s%02X", indent, "", codec_cfg->data[i]);
		}

		shell_fprintf(sh, SHELL_NORMAL, "\n");
	}

	/* Reduce for metadata*/
	indent -= SHELL_PRINT_INDENT_LEVEL_SIZE;
#endif /* CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE > 0 */

#if CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE > 0
	shell_print(sh, "%*sCodec specific metadata:", indent, "");

	indent += SHELL_PRINT_INDENT_LEVEL_SIZE;
	if (codec_cfg->meta_len == 0U) {
		shell_print(sh, "%*sNone", indent, "");
	} else {
		const uint8_t *data;
		int ret;

		ret = bt_audio_codec_cfg_meta_get_pref_context(codec_cfg);
		if (ret >= 0) {
			print_codec_meta_pref_context(sh, indent, (enum bt_audio_context)ret);
		}

		ret = bt_audio_codec_cfg_meta_get_stream_context(codec_cfg);
		if (ret >= 0) {
			print_codec_meta_stream_context(sh, indent, (enum bt_audio_context)ret);
		}

		ret = bt_audio_codec_cfg_meta_get_program_info(codec_cfg, &data);
		if (ret >= 0) {
			print_codec_meta_program_info(sh, indent, data, (uint8_t)ret);
		}

		ret = bt_audio_codec_cfg_meta_get_stream_lang(codec_cfg);
		if (ret >= 0) {
			print_codec_meta_language(sh, indent, (uint32_t)ret);
		}

		ret = bt_audio_codec_cfg_meta_get_ccid_list(codec_cfg, &data);
		if (ret >= 0) {
			print_codec_meta_ccid_list(sh, indent, data, (uint8_t)ret);
		}

		ret = bt_audio_codec_cfg_meta_get_parental_rating(codec_cfg);
		if (ret >= 0) {
			print_codec_meta_parental_rating(sh, indent,
							 (enum bt_audio_parental_rating)ret);
		}

		ret = bt_audio_codec_cfg_meta_get_audio_active_state(codec_cfg);
		if (ret >= 0) {
			print_codec_meta_audio_active_state(sh, indent,
							    (enum bt_audio_active_state)ret);
		}

		ret = bt_audio_codec_cfg_meta_get_bcast_audio_immediate_rend_flag(codec_cfg);
		if (ret >= 0) {
			print_codec_meta_bcast_audio_immediate_rend_flag(sh, indent);
		}

		ret = bt_audio_codec_cfg_meta_get_extended(codec_cfg, &data);
		if (ret >= 0) {
			print_codec_meta_extended(sh, indent, data, (uint8_t)ret);
		}

		ret = bt_audio_codec_cfg_meta_get_vendor(codec_cfg, &data);
		if (ret >= 0) {
			print_codec_meta_vendor(sh, indent, data, (uint8_t)ret);
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

	shell_print(ctx_shell, "%*sBIS index: 0x%02X", indent, "", bis->index);

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
			print_codec_cfg(ctx_shell, indent, &codec_cfg);
		} else {
			shell_print(ctx_shell, "%*sCodec specific configuration:", indent, "");
			print_ltv_array(ctx_shell, indent, bis->data, bis->data_len);
		}
	} else { /* If not LC3, we cannot assume it's LTV */
		shell_print(ctx_shell, "%*sCodec specific configuration:", indent, "");

		indent += SHELL_PRINT_INDENT_LEVEL_SIZE;
		shell_fprintf(ctx_shell, SHELL_NORMAL, "%*s", indent, "");

		for (uint8_t i = 0U; i < bis->data_len; i++) {
			shell_fprintf(ctx_shell, SHELL_NORMAL, "%02X", bis->data[i]);
		}

		shell_fprintf(ctx_shell, SHELL_NORMAL, "\n");
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

	shell_print(ctx_shell, "Subgroup %p:", subgroup);

	ret = bt_bap_base_get_subgroup_codec_id(subgroup, &codec_id);
	if (ret < 0) {
		return false;
	}

	shell_print(ctx_shell, "%*sCodec Format: 0x%02X", indent, "", codec_id.id);
	shell_print(ctx_shell, "%*sCompany ID  : 0x%04X", indent, "", codec_id.cid);
	shell_print(ctx_shell, "%*sVendor ID   : 0x%04X", indent, "", codec_id.vid);

	ret = bt_bap_base_subgroup_codec_to_codec_cfg(subgroup, &codec_cfg);
	if (ret == 0) {
		print_codec_cfg(ctx_shell, indent, &codec_cfg);
	} else {
		/* If we cannot store it in a codec_cfg, then we cannot easily print it as such */
		ret = bt_bap_base_get_subgroup_codec_data(subgroup, &data);
		if (ret < 0) {
			return false;
		}

		shell_print(ctx_shell, "%*sCodec specific configuration:", indent, "");
		indent += SHELL_PRINT_INDENT_LEVEL_SIZE;

		/* Print CC data */
		if (codec_id.id == BT_HCI_CODING_FORMAT_LC3) {
			print_ltv_array(ctx_shell, indent, data, (uint8_t)ret);
		} else { /* If not LC3, we cannot assume it's LTV */
			shell_fprintf(ctx_shell, SHELL_NORMAL, "%*s", indent, "");

			for (uint8_t i = 0U; i < (uint8_t)ret; i++) {
				shell_fprintf(ctx_shell, SHELL_NORMAL, "%c", data[i]);
			}

			shell_fprintf(ctx_shell, SHELL_NORMAL, "\n");
		}

		ret = bt_bap_base_get_subgroup_codec_meta(subgroup, &data);
		if (ret < 0) {
			return false;
		}

		shell_print(ctx_shell,
			    "%*sCodec specific metadata:", indent - SHELL_PRINT_INDENT_LEVEL_SIZE,
			    "");

		/* Print metadata */
		if (codec_id.id == BT_HCI_CODING_FORMAT_LC3) {
			print_ltv_array(ctx_shell, indent, data, (uint8_t)ret);
		} else { /* If not LC3, we cannot assume it's LTV */
			shell_fprintf(ctx_shell, SHELL_NORMAL, "%*s", indent, "");

			for (uint8_t i = 0U; i < (uint8_t)ret; i++) {
				shell_fprintf(ctx_shell, SHELL_NORMAL, "%c", data[i]);
			}

			shell_fprintf(ctx_shell, SHELL_NORMAL, "\n");
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

	shell_print(ctx_shell, "Presentation delay: %d", bt_bap_base_get_pres_delay(base));
	shell_print(ctx_shell, "Subgroup count: %d", bt_bap_base_get_subgroup_count(base));

	err = bt_bap_base_foreach_subgroup(base, print_base_subgroup_cb, NULL);
	if (err < 0) {
		shell_info(ctx_shell, "Invalid BASE: %d", err);
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

#endif /* __AUDIO_H */
