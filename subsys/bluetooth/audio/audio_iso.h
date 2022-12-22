/* @file
 * @brief Internal APIs for Audio ISO handling
 *
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/audio/audio.h>

struct bt_audio_iso_dir {
	struct bt_audio_stream *stream;
	struct bt_audio_ep *ep;
	struct bt_iso_chan_path path;
	struct bt_iso_chan_io_qos qos;
	uint8_t cc[CONFIG_BT_CODEC_MAX_DATA_COUNT * CONFIG_BT_CODEC_MAX_DATA_LEN];
};

struct bt_audio_iso {
	struct bt_iso_chan chan;
	struct bt_iso_chan_qos qos;

	struct bt_audio_iso_dir rx;
	struct bt_audio_iso_dir tx;

	/* Must be at the end so that everything else in the structure can be
	 * memset to zero without affecting the ref.
	 */
	atomic_t ref;
};

typedef bool (*bt_audio_iso_func_t)(struct bt_audio_iso *iso, void *user_data);

struct bt_audio_iso *bt_audio_iso_new(void);
struct bt_audio_iso *bt_audio_iso_ref(struct bt_audio_iso *iso);
void bt_audio_iso_unref(struct bt_audio_iso *iso);
void bt_audio_iso_foreach(bt_audio_iso_func_t func, void *user_data);
struct bt_audio_iso *bt_audio_iso_find(bt_audio_iso_func_t func,
				       void *user_data);
void bt_audio_iso_init(struct bt_audio_iso *iso, struct bt_iso_chan_ops *ops);
void bt_audio_iso_bind_ep(struct bt_audio_iso *iso, struct bt_audio_ep *ep);
void bt_audio_iso_unbind_ep(struct bt_audio_iso *iso, struct bt_audio_ep *ep);
struct bt_audio_ep *bt_audio_iso_get_ep(bool unicast_client,
					struct bt_audio_iso *iso,
					enum bt_audio_dir dir);
/* Unicast client-only functions*/
void bt_audio_iso_bind_stream(struct bt_audio_iso *audio_iso,
			      struct bt_audio_stream *stream);
void bt_audio_iso_unbind_stream(struct bt_audio_iso *audio_iso,
				struct bt_audio_stream *stream);
struct bt_audio_stream *bt_audio_iso_get_stream(struct bt_audio_iso *iso,
						enum bt_audio_dir dir);
