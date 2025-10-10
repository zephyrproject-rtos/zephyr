/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ISO_TX_H
#define ISO_TX_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/types.h>

/**
 * @brief Initialize TX
 *
 * This will initialize TX if not already initialized. This creates and starts a thread that
 * will attempt to send data on all streams registered with iso_tx_register().
 */
void iso_tx_init(void);

/**
 * @brief Register a stream for TX
 *
 * This will add it to the list of streams the TX thread will attempt to send on.
 *
 * @param bap_stream The stream to register
 *
 * @retval 0 on success
 * @retval -EINVAL @p iso_chan is NULL
 * @retval -EINVAL @p iso_chan is not configured for TX
 * @retval -ENOMEM if not more streams can be registered
 */
int iso_tx_register(struct bt_iso_chan *iso_chan);

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
int iso_tx_unregister(struct bt_iso_chan *iso_chan);

/**
 * @brief Test if the provided stream has been configured for TX
 *
 * @param bap_stream The stream to test for TX support
 *
 * @retval true if it has been configured for TX, and false if not
 */
bool iso_tx_can_send(const struct bt_iso_chan *iso_chan);

/**
 * @brief Callback to indicate a TX complete
 *
 * @param stream The stream that completed TX
 */
void iso_tx_sent_cb(struct bt_iso_chan *iso_chan);

/**
 * @brief Get the number of sent SDUs for an ISO channel
 *
 * Counter will be unavailable after iso_tx_unregister()
 *
 * @param iso_chan The ISO channel
 *
 * @return The number of sent SDUs
 */
size_t iso_tx_get_sent_cnt(const struct bt_iso_chan *iso_chan);

/* Generate 1 KiB of mock data going 0x00, 0x01, ..., 0xff, 0x00, 0x01, ..., 0xff, etc */
#define ISO_DATA_GEN(_i, _) (uint8_t)_i
static const uint8_t mock_iso_data[] = {
	LISTIFY(1024, ISO_DATA_GEN, (,)),
};
#endif /* ISO_TX_H */
