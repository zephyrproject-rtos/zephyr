/* @file
 * @brief Internal APIs for BAP ISO handling
 *
 * Copyright (c) 2022 Codecoup
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>

struct bt_bap_iso_dir {
	struct bt_bap_stream *stream;
	struct bt_bap_ep *ep;
	struct bt_iso_chan_path path;
	struct bt_iso_chan_io_qos qos;
	uint8_t cc[CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE];
};

struct bt_bap_iso {
	struct bt_iso_chan chan;
	struct bt_iso_chan_qos qos;

	struct bt_bap_iso_dir rx;
	struct bt_bap_iso_dir tx;

	/* Must be at the end so that everything else in the structure can be
	 * memset to zero without affecting the ref.
	 */
	atomic_t ref;
};

typedef bool (*bt_bap_iso_func_t)(struct bt_bap_iso *iso, void *user_data);

struct bt_bap_iso *bt_bap_iso_new(void);
struct bt_bap_iso *bt_bap_iso_ref(struct bt_bap_iso *iso);
void bt_bap_iso_unref(struct bt_bap_iso *iso);
void bt_bap_iso_foreach(bt_bap_iso_func_t func, void *user_data);
struct bt_bap_iso *bt_bap_iso_find(bt_bap_iso_func_t func, void *user_data);
void bt_bap_iso_init(struct bt_bap_iso *iso, struct bt_iso_chan_ops *ops);
void bt_bap_iso_bind_ep(struct bt_bap_iso *iso, struct bt_bap_ep *ep);
void bt_bap_iso_configure_data_path(struct bt_bap_ep *ep, struct bt_audio_codec_cfg *codec_cfg);
void bt_bap_iso_unbind_ep(struct bt_bap_iso *iso, struct bt_bap_ep *ep);
struct bt_bap_ep *bt_bap_iso_get_ep(bool unicast_client, struct bt_bap_iso *iso,
				    enum bt_audio_dir dir);

struct bt_bap_ep *bt_bap_iso_get_paired_ep(const struct bt_bap_ep *ep);
/* Unicast client-only functions*/
void bt_bap_iso_bind_stream(struct bt_bap_iso *bap_iso, struct bt_bap_stream *stream,
			    enum bt_audio_dir dir);
void bt_bap_iso_unbind_stream(struct bt_bap_iso *bap_iso, struct bt_bap_stream *stream,
			      enum bt_audio_dir dir);
struct bt_bap_stream *bt_bap_iso_get_stream(struct bt_bap_iso *iso, enum bt_audio_dir dir);
