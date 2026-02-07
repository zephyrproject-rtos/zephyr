/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCKS_CAP_HANDOVER_H_
#define MOCKS_CAP_HANDOVER_H_

#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/fff.h>

extern const struct bt_cap_handover_cb mock_cap_handover_cb;

void mock_cap_handover_init(void);

DECLARE_FAKE_VOID_FUNC(mock_unicast_to_broadcast_complete_cb, int, struct bt_conn *,
		       struct bt_cap_unicast_group *, struct bt_cap_broadcast_source *);

#endif /* MOCKS_CAP_HANDOVER_H_ */
