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

extern struct bt_csip_set_member_svc_inst *svc_inst;

ssize_t audio_ad_data_add(struct bt_data *data, const size_t data_size, const bool discoverable,
			  const bool connectable);
ssize_t audio_pa_data_add(struct bt_data *data_array, const size_t data_array_size);
ssize_t csis_ad_data_add(struct bt_data *data, const size_t data_size, const bool discoverable);
size_t cap_acceptor_ad_data_add(struct bt_data data[], size_t data_size, bool discoverable);
size_t gmap_ad_data_add(struct bt_data data[], size_t data_size);
size_t pbp_ad_data_add(struct bt_data data[], size_t data_size);

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

#if defined(CONFIG_BT_BAP_UNICAST)

#define UNICAST_SERVER_STREAM_COUNT                                                                \
	COND_CODE_1(CONFIG_BT_ASCS, (CONFIG_BT_ASCS_ASE_SNK_COUNT + CONFIG_BT_ASCS_ASE_SRC_COUNT), \
		    (0))
#define UNICAST_CLIENT_STREAM_COUNT                                                                \
	COND_CODE_1(CONFIG_BT_BAP_UNICAST_CLIENT,                                                  \
		    (CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT +                                  \
		     CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT),                                  \
		    (0))

#define BAP_UNICAST_AC_MAX_CONN   2U
#define BAP_UNICAST_AC_MAX_SNK    (2U * BAP_UNICAST_AC_MAX_CONN)
#define BAP_UNICAST_AC_MAX_SRC    (2U * BAP_UNICAST_AC_MAX_CONN)
#define BAP_UNICAST_AC_MAX_PAIR   MAX(BAP_UNICAST_AC_MAX_SNK, BAP_UNICAST_AC_MAX_SRC)
#define BAP_UNICAST_AC_MAX_STREAM (BAP_UNICAST_AC_MAX_SNK + BAP_UNICAST_AC_MAX_SRC)

struct shell_stream {
	struct bt_cap_stream stream;
	struct bt_audio_codec_cfg codec_cfg;
	struct bt_audio_codec_qos qos;
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
	size_t lost_pkts;
	size_t err_pkts;
	size_t dup_psn;
	size_t rx_cnt;
	size_t dup_ts;
#endif /* CONFIG_BT_AUDIO_RX */
};

struct broadcast_source {
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
extern const struct named_lc3_preset *default_sink_preset;
extern const struct named_lc3_preset *default_source_preset;

int bap_ac_create_unicast_group(const struct bap_unicast_ac_param *param,
				struct shell_stream *snk_uni_streams[], size_t snk_cnt,
				struct shell_stream *src_uni_streams[], size_t src_cnt);

int cap_ac_unicast(const struct shell *sh, size_t argc, char **argv,
		   const struct bap_unicast_ac_param *param);
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
	const char *str;
	size_t cnt;
};

static bool print_ltv_elem(struct bt_data *data, void *user_data)
{
	struct print_ltv_info *ltv_info = user_data;

	shell_print(ltv_info->sh, "%s #%zu: type 0x%02x value_len %u", ltv_info->str, ltv_info->cnt,
		    data->type, data->data_len);
	shell_hexdump(ltv_info->sh, data->data, data->data_len);

	ltv_info->cnt++;

	return true;
}

static void print_ltv_array(const struct shell *sh, const char *str, const uint8_t *ltv_data,
			    size_t ltv_data_len)
{
	struct print_ltv_info ltv_info = {
		.sh = sh,
		.str = str,
		.cnt = 0U,
	};

	bt_audio_data_parse(ltv_data, ltv_data_len, print_ltv_elem, &ltv_info);
}

static inline void print_codec_cap(const struct shell *sh,
				   const struct bt_audio_codec_cap *codec_cap)
{
	shell_print(sh, "codec cap id 0x%02x cid 0x%04x vid 0x%04x count %u", codec_cap->id,
		    codec_cap->cid, codec_cap->vid, codec_cap->data_len);

#if CONFIG_BT_AUDIO_CODEC_CAP_MAX_DATA_SIZE > 0
	if (codec_cap->id == BT_HCI_CODING_FORMAT_LC3) {
		print_ltv_array(sh, "data", codec_cap->data, codec_cap->data_len);
	} else { /* If not LC3, we cannot assume it's LTV */
		shell_hexdump(sh, codec_cap->data, codec_cap->data_len);
	}
#endif /* CONFIG_BT_AUDIO_CODEC_CAP_MAX_DATA_SIZE > 0 */

#if CONFIG_BT_AUDIO_CODEC_CAP_MAX_METADATA_SIZE > 0
	print_ltv_array(sh, "meta", codec_cap->meta, codec_cap->meta_len);
#endif /* CONFIG_BT_AUDIO_CODEC_CAP_MAX_METADATA_SIZE > 0 */
}

static inline void print_codec_cfg(const struct shell *sh,
				   const struct bt_audio_codec_cfg *codec_cfg)
{
	shell_print(sh, "codec cfg id 0x%02x cid 0x%04x vid 0x%04x count %u", codec_cfg->id,
		    codec_cfg->cid, codec_cfg->vid, codec_cfg->data_len);

#if CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE > 0
	if (codec_cfg->id == BT_HCI_CODING_FORMAT_LC3) {
		print_ltv_array(sh, "data", codec_cfg->data, codec_cfg->data_len);
	} else { /* If not LC3, we cannot assume it's LTV */
		shell_hexdump(sh, codec_cfg->data, codec_cfg->data_len);
	}
#endif /* CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE > 0 */

#if CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE > 0
	print_ltv_array(sh, "meta", codec_cfg->meta, codec_cfg->meta_len);
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
#endif /* CONFIG_BT_BAP_BROADCAST_SOURCE */

static inline bool print_base_subgroup_bis_cb(const struct bt_bap_base_subgroup_bis *bis,
					      void *user_data)
{
	struct bt_bap_base_codec_id *codec_id = user_data;

	shell_print(ctx_shell, "\t\tBIS index: 0x%02X", bis->index);
	/* Print CC data */
	if (codec_id->id == BT_HCI_CODING_FORMAT_LC3) {
		print_ltv_array(ctx_shell, "\t\tdata", bis->data, bis->data_len);
	} else { /* If not LC3, we cannot assume it's LTV */
		shell_hexdump(ctx_shell, bis->data, bis->data_len);
	}

	return true;
}

static inline bool print_base_subgroup_cb(const struct bt_bap_base_subgroup *subgroup,
					  void *user_data)
{
	struct bt_bap_base_codec_id codec_id;
	uint8_t *data;
	int ret;

	shell_print(ctx_shell, "Subgroup %p:", subgroup);

	ret = bt_bap_base_get_subgroup_codec_id(subgroup, &codec_id);
	if (ret < 0) {
		return false;
	}

	shell_print(ctx_shell, "\tCodec Format: 0x%02X", codec_id.id);
	shell_print(ctx_shell, "\tCompany ID  : 0x%04X", codec_id.cid);
	shell_print(ctx_shell, "\tVendor ID   : 0x%04X", codec_id.vid);

	ret = bt_bap_base_get_subgroup_codec_data(subgroup, &data);
	if (ret < 0) {
		return false;
	}

	/* Print CC data */
	if (codec_id.id == BT_HCI_CODING_FORMAT_LC3) {
		print_ltv_array(ctx_shell, "\tdata", data, (uint8_t)ret);
	} else { /* If not LC3, we cannot assume it's LTV */
		shell_hexdump(ctx_shell, data, (uint8_t)ret);
	}

	ret = bt_bap_base_get_subgroup_codec_meta(subgroup, &data);
	if (ret < 0) {
		return false;
	}

	/* Print metadata */
	if (codec_id.id == BT_HCI_CODING_FORMAT_LC3) {
		print_ltv_array(ctx_shell, "\tdata", data, (uint8_t)ret);
	} else { /* If not LC3, we cannot assume it's LTV */
		shell_hexdump(ctx_shell, data, (uint8_t)ret);
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
