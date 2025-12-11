/*
 * Copyright (c) 2023-2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCKS_CAP_INITIATOR_H_
#define MOCKS_CAP_INITIATOR_H_
#include <stdint.h>

#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/fff.h>

extern const struct bt_cap_initiator_cb mock_cap_initiator_cb;

void mock_cap_initiator_init(void);
void mock_cap_initiator_cleanup(void);

DECLARE_FAKE_VOID_FUNC(mock_cap_initiator_unicast_discovery_complete_cb, struct bt_conn *, int,
		       const struct bt_csip_set_coordinator_set_member *,
		       const struct bt_csip_set_coordinator_csis_inst *);
DECLARE_FAKE_VOID_FUNC(mock_cap_initiator_unicast_start_complete_cb, int, struct bt_conn *);
DECLARE_FAKE_VOID_FUNC(mock_cap_initiator_unicast_update_complete_cb, int, struct bt_conn *);
DECLARE_FAKE_VOID_FUNC(mock_cap_initiator_unicast_stop_complete_cb, int, struct bt_conn *);
DECLARE_FAKE_VOID_FUNC(mock_cap_initiator_broadcast_started_cb, struct bt_cap_broadcast_source *);
DECLARE_FAKE_VOID_FUNC(mock_cap_initiator_broadcast_stopped_cb, struct bt_cap_broadcast_source *,
		       uint8_t);

#endif /* MOCKS_CAP_INITIATOR_H_ */
