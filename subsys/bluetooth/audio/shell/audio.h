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
#include <zephyr/sys/printk.h>

#include "shell/bt.h"

extern struct bt_csip_set_member_svc_inst *svc_inst;

ssize_t audio_ad_data_add(struct bt_data *data, const size_t data_size, const bool discoverable,
			  const bool connectable);
ssize_t audio_pa_data_add(struct bt_data *data_array, const size_t data_array_size);
ssize_t csis_ad_data_add(struct bt_data *data, const size_t data_size, const bool discoverable);
size_t cap_acceptor_ad_data_add(struct bt_data data[], size_t data_size, bool discoverable);

#if defined(CONFIG_BT_AUDIO)
/* Must guard before including audio.h as audio.h uses Kconfigs guarded by
 * CONFIG_BT_AUDIO
 */
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/bluetooth/audio/cap.h>

struct named_lc3_preset {
	const char *name;
	struct bt_bap_lc3_preset preset;
};

#if defined(CONFIG_BT_BAP_UNICAST)

#define UNICAST_SERVER_STREAM_COUNT                                                                \
	COND_CODE_1(CONFIG_BT_ASCS, (CONFIG_BT_ASCS_ASE_SNK_COUNT + CONFIG_BT_ASCS_ASE_SRC_COUNT), \
		    (0))
#define UNICAST_CLIENT_STREAM_COUNT                                                                \
	COND_CODE_1(CONFIG_BT_BAP_UNICAST_CLIENT,                                                  \
		    (CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT +                                  \
		     CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT),                                  \
		    (0))

struct shell_stream {
	struct bt_cap_stream stream;
	struct bt_audio_codec_cfg codec_cfg;
	struct bt_audio_codec_qos qos;
#if defined(CONFIG_BT_AUDIO_TX)
	int64_t connected_at_ticks;      /* The uptime tick measured when stream was connected */
	uint16_t last_allocated_seq_num; /* The last packet sequence number allocated */
#endif /* CONFIG_BT_AUDIO_TX */
};

struct broadcast_source {
	union {
		struct bt_bap_broadcast_source *bap_source;
		struct bt_cap_broadcast_source *cap_source;
	};
	struct bt_audio_codec_cfg codec_cfg;
	struct bt_audio_codec_qos qos;
};

extern struct shell_stream unicast_streams[CONFIG_BT_MAX_CONN * (UNICAST_SERVER_STREAM_COUNT +
								 UNICAST_CLIENT_STREAM_COUNT)];

#if defined(CONFIG_BT_BAP_UNICAST_CLIENT)

extern struct bt_bap_unicast_group *default_unicast_group;
extern struct bt_bap_ep *snks[CONFIG_BT_MAX_CONN][CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT];
extern struct bt_bap_ep *srcs[CONFIG_BT_MAX_CONN][CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT];
extern const struct named_lc3_preset *default_sink_preset;
extern const struct named_lc3_preset *default_source_preset;
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

static inline void print_codec_cap(const struct shell *sh,
				   const struct bt_audio_codec_cap *codec_cap)
{
	shell_print(sh, "codec cap id 0x%02x cid 0x%04x vid 0x%04x", codec_cap->id, codec_cap->cid,
		    codec_cap->vid);

#if CONFIG_BT_AUDIO_CODEC_CAP_MAX_DATA_COUNT > 0
	shell_print(sh, "data_count %u", codec_cap->data_count);
	for (size_t i = 0U; i < codec_cap->data_count; i++) {
		shell_print(sh, "data #%u: type 0x%02x len %u", i, codec_cap->data[i].data.type,
			    codec_cap->data[i].data.data_len);
		shell_hexdump(sh, codec_cap->data[i].data.data,
			      codec_cap->data[i].data.data_len -
				      sizeof(codec_cap->data[i].data.type));
	}
#endif /* CONFIG_BT_AUDIO_CODEC_CAP_MAX_DATA_COUNT > 0 */

#if CONFIG_BT_AUDIO_CODEC_CAP_MAX_METADATA_COUNT > 0
	shell_print(sh, "meta_count %u", codec_cap->data_count);
	for (size_t i = 0U; i < codec_cap->meta_count; i++) {
		shell_print(sh, "meta #%u: type 0x%02x len %u", i, codec_cap->meta[i].data.type,
			    codec_cap->meta[i].data.data_len);
		shell_hexdump(sh, codec_cap->meta[i].data.data,
			      codec_cap->meta[i].data.data_len -
				      sizeof(codec_cap->meta[i].data.type));
	}
#endif /* CONFIG_BT_AUDIO_CODEC_CAP_MAX_METADATA_COUNT > 0 */
}

static inline void print_codec_cfg(const struct shell *sh,
				   const struct bt_audio_codec_cfg *codec_cfg)
{
	shell_print(sh, "codec cfg id 0x%02x cid 0x%04x vid 0x%04x", codec_cfg->id, codec_cfg->cid,
		    codec_cfg->vid);

#if CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_COUNT > 0
	shell_print(sh, "data_count %u", codec_cfg->data_count);
	for (size_t i = 0U; i < codec_cfg->data_count; i++) {
		shell_print(sh, "data #%u: type 0x%02x len %u", i, codec_cfg->data[i].data.type,
			    codec_cfg->data[i].data.data_len);
		shell_hexdump(sh, codec_cfg->data[i].data.data,
			      codec_cfg->data[i].data.data_len -
				      sizeof(codec_cfg->data[i].data.type));
	}
#endif /* CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_COUNT > 0 */

#if CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_COUNT > 0
	shell_print(sh, "meta_count %u", codec_cfg->data_count);
	for (size_t i = 0U; i < codec_cfg->meta_count; i++) {
		shell_print(sh, "meta #%u: type 0x%02x len %u", i, codec_cfg->meta[i].data.type,
			    codec_cfg->meta[i].data.data_len);
		shell_hexdump(sh, codec_cfg->meta[i].data.data,
			      codec_cfg->meta[i].data.data_len -
				      sizeof(codec_cfg->meta[i].data.type));
	}
#endif /* CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_COUNT > 0 */
}

#if defined(CONFIG_BT_BAP_BROADCAST_SOURCE)
extern struct shell_stream broadcast_source_streams[CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT];
extern struct broadcast_source default_source;
#endif /* CONFIG_BT_BAP_BROADCAST_SOURCE */

#if BROADCAST_SNK_SUBGROUP_CNT > 0
static inline void print_base(const struct shell *sh, const struct bt_bap_base *base)
{
	uint8_t bis_indexes[BT_ISO_MAX_GROUP_ISO_COUNT] = {0};
	/* "0xXX " requires 5 characters */
	char bis_indexes_str[5 * ARRAY_SIZE(bis_indexes) + 1];
	size_t index_count = 0;

	for (size_t i = 0U; i < base->subgroup_count; i++) {
		const struct bt_bap_base_subgroup *subgroup;

		subgroup = &base->subgroups[i];

		shell_print(sh, "Subgroup[%d]:", i);
		print_codec_cfg(sh, &subgroup->codec_cfg);

		for (size_t j = 0U; j < subgroup->bis_count; j++) {
			const struct bt_bap_base_bis_data *bis_data;

			bis_data = &subgroup->bis_data[j];

			shell_print(sh, "BIS[%d] index 0x%02x", j, bis_data->index);
			bis_indexes[index_count++] = bis_data->index;

#if CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_COUNT > 0
			for (size_t k = 0U; k < bis_data->data_count; k++) {
				const struct bt_audio_codec_data *codec_data;

				codec_data = &bis_data->data[k];

				shell_print(sh, "data #%u: type 0x%02x len %u", k,
					    codec_data->data.type, codec_data->data.data_len);
				shell_hexdump(sh, codec_data->data.data,
					      codec_data->data.data_len -
						      sizeof(codec_data->data.type));
			}
#endif /* CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_COUNT > 0 */
		}
	}

	(void)memset(bis_indexes_str, 0, sizeof(bis_indexes_str));

	/* Create space separated list of indexes as hex values */
	for (size_t i = 0U; i < index_count; i++) {
		char bis_index_str[6];

		sprintf(bis_index_str, "0x%02x ", bis_indexes[i]);

		strcat(bis_indexes_str, bis_index_str);
		shell_print(sh, "[%d]: %s", i, bis_index_str);
	}

	shell_print(sh, "Possible indexes: %s", bis_indexes_str);
}
#endif /* BROADCAST_SNK_SUBGROUP_CNT > 0 */

static inline void copy_unicast_stream_preset(struct shell_stream *stream,
					      const struct named_lc3_preset *named_preset)
{
	memcpy(&stream->qos, &named_preset->preset.qos, sizeof(stream->qos));
	memcpy(&stream->codec_cfg, &named_preset->preset.codec_cfg, sizeof(stream->codec_cfg));

#if CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_COUNT > 0 && CONFIG_BT_AUDIO_CODEC_MAX_DATA_LEN > 0
	/* Need to update the `bt_data.data` pointer to the new value after copying the codec */
	for (size_t i = 0U; i < ARRAY_SIZE(stream->codec_cfg.data); i++) {
		const struct bt_audio_codec_data *preset_data =
			&named_preset->preset.codec_cfg.data[i];
		struct bt_audio_codec_data *data = &stream->codec_cfg.data[i];
		const uint8_t data_len = preset_data->data.data_len;

		data->data.data = data->value;
		data->data.data_len = data_len;
		memcpy(data->value, preset_data->data.data, data_len);
	}
#endif /* CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_COUNT > 0 &&                                          \
	* CONFIG_BT_AUDIO_CODEC_MAX_DATA_LEN > 0                                                   \
	*/

#if CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_COUNT > 0 && CONFIG_BT_AUDIO_CODEC_MAX_DATA_LEN > 0
	for (size_t i = 0U; i < ARRAY_SIZE(stream->codec_cfg.meta); i++) {
		const struct bt_audio_codec_data *preset_data =
			&named_preset->preset.codec_cfg.meta[i];
		struct bt_audio_codec_data *data = &stream->codec_cfg.meta[i];
		const uint8_t data_len = preset_data->data.data_len;

		data->data.data = data->value;
		data->data.data_len = data_len;
		memcpy(data->value, preset_data->data.data, data_len);
	}
#endif /* CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_COUNT > 0 &&                                      \
	* CONFIG_BT_AUDIO_CODEC_MAX_DATA_LEN > 0                                                   \
	*/
}

static inline void copy_broadcast_source_preset(struct broadcast_source *source,
						const struct named_lc3_preset *named_preset)
{
	memcpy(&source->qos, &named_preset->preset.qos, sizeof(source->qos));
	memcpy(&source->codec_cfg, &named_preset->preset.codec_cfg, sizeof(source->codec_cfg));

#if CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_COUNT > 0 && CONFIG_BT_AUDIO_CODEC_MAX_DATA_LEN > 0
	/* Need to update the `bt_data.data` pointer to the new value after copying the codec */
	for (size_t i = 0U; i < ARRAY_SIZE(source->codec_cfg.data); i++) {
		const struct bt_audio_codec_data *preset_data =
			&named_preset->preset.codec_cfg.data[i];
		struct bt_audio_codec_data *data = &source->codec_cfg.data[i];
		const uint8_t data_len = preset_data->data.data_len;

		data->data.data = data->value;
		data->data.data_len = data_len;
		memcpy(data->value, preset_data->data.data, data_len);
	}
#endif /* CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_COUNT > 0 && CONFIG_BT_AUDIO_CODEC_MAX_DATA_LEN > \
	* 0                                                                                        \
	*/

#if CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_COUNT > 0 && CONFIG_BT_AUDIO_CODEC_MAX_DATA_LEN > 0
	for (size_t i = 0U; i < ARRAY_SIZE(source->codec_cfg.meta); i++) {
		const struct bt_audio_codec_data *preset_data =
			&named_preset->preset.codec_cfg.meta[i];
		struct bt_audio_codec_data *data = &source->codec_cfg.meta[i];
		const uint8_t data_len = preset_data->data.data_len;

		data->data.data = data->value;
		data->data.data_len = data_len;
		memcpy(data->value, preset_data->data.data, data_len);
	}
#endif /* CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_COUNT > 0 && CONFIG_BT_AUDIO_CODEC_MAX_DATA_LEN > \
	* 0                                                                                        \
	*/
}

#endif /* CONFIG_BT_AUDIO */

#endif /* __AUDIO_H */
