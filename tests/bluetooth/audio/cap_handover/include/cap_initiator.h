/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCKS_CAP_INITIATOR_H_
#define MOCKS_CAP_INITIATOR_H_

#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/fff.h>

extern const struct bt_cap_initiator_cb mock_cap_initiator_cb;

void mock_cap_initiator_init(void);

DECLARE_FAKE_VOID_FUNC(mock_unicast_start_complete_cb, int, struct bt_conn *);
DECLARE_FAKE_VOID_FUNC(mock_broadcast_start_cb, struct bt_cap_broadcast_source *);

#endif /* MOCKS_CAP_INITIATOR_H_ */
