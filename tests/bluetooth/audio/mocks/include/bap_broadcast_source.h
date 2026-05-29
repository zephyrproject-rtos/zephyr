/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCKS_BAP_BROADCAST_SOURCE_H_
#define MOCKS_BAP_BROADCAST_SOURCE_H_

#include <stdint.h>

#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/fff.h>

extern struct bt_bap_broadcast_source_cb mock_bap_broadcast_source_cb;

void mock_bap_broadcast_source_init(void);

DECLARE_FAKE_VOID_FUNC(mock_bap_broadcast_source_started_cb, struct bt_bap_broadcast_source *);
DECLARE_FAKE_VOID_FUNC(mock_bap_broadcast_source_stopped_cb, struct bt_bap_broadcast_source *,
		       uint8_t);

#endif /* MOCKS_BAP_BROADCAST_SOURCE_H_ */
