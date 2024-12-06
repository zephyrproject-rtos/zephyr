/* btp_bap_audio_stream.h - Bluetooth BAP Tester */

/*
 * Copyright (c) 2023 Codecoup
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BTP_BAP_AUDIO_STREAM_H
#define BTP_BAP_AUDIO_STREAM_H

#include <stdint.h>

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/types.h>

struct btp_bap_audio_stream {
	struct bt_cap_stream cap_stream;
};

/**
 * @brief Initialize TX
 *
 * This will initialize TX if not already initialized. This creates and starts a thread that
 * will attempt to send data on all streams registered with btp_bap_audio_stream_tx_register().
 */
void btp_bap_audio_stream_tx_init(void);

/**
 * @brief Register a stream for TX
 *
 * This will add it to the list of streams the TX thread will attempt to send on.
 *
 * @param bap_stream The stream to register
 *
 * @retval 0 on success
 * @retval -EINVAL @p bap_stream is NULL
 * @retval -EINVAL @p bap_stream is not configured for TX
 * @retval -EINVAL @p bap_stream.codec_cfg contains invalid values
 * @retval -ENOMEM if not more streams can be registered
 */
int btp_bap_audio_stream_tx_register(struct btp_bap_audio_stream *stream);

/**
 * @brief Unregister a stream for TX
 *
 * This will remove it to the list of streams the TX thread will attempt to send on.
 *
 * @param bap_stream The stream to unregister
 *
 * @retval 0 on success
 * @retval -EINVAL @p bap_stream is NULL
 * @retval -EINVAL @p bap_stream is not configured for TX
 * @retval -EALREADY @p bap_stream is currently not registered
 */
int btp_bap_audio_stream_tx_unregister(struct btp_bap_audio_stream *stream);

/**
 * @brief Test if the provided stream has been configured for TX
 *
 * @param bap_stream The stream to test for TX support
 *
 * @retval true if it has been configured for TX, and false if not
 */
bool btp_bap_audio_stream_can_send(struct btp_bap_audio_stream *stream);

/**
 * @brief Callback to indicate a TX complete
 *
 * @param stream The stream that completed TX
 */
void btp_bap_audio_stream_sent_cb(struct bt_bap_stream *stream);

/* Generate 310 octets of mock data going 0x00, 0x01, ..., 0xff, 0x00, 0x01, ..., 0xff, etc
 * For 2 x 155 = 310 octets is used as the maximum number of channels per stream defined by BAP is 2
 * and the maximum octets per codec frame is 155 for the 48_6 configs.
 * If we ever want to send multiple frames per SDU, we can simply multiply this value.
 */
#define BTP_BAP_AUDIO_STREAM_ISO_DATA_GEN(_i, _) (uint8_t) _i
static const uint8_t btp_bap_audio_stream_mock_data[] = {
	LISTIFY(310, BTP_BAP_AUDIO_STREAM_ISO_DATA_GEN, (,)),
};
#endif /* BTP_BAP_AUDIO_STREAM_H */
