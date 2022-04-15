/*  Bluetooth Audio Unicast Server */

/*
 * Copyright (c) 2021-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/audio/audio.h>

extern const struct bt_audio_unicast_server_cb *unicast_server_cb;

int bt_unicast_server_release(struct bt_audio_stream *stream, bool cache);
