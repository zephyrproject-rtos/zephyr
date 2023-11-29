/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCKS_CAP_COMMANDER_H_
#define MOCKS_CAP_COMMANDER_H_

#include <zephyr/fff.h>
#include <zephyr/bluetooth/audio/cap.h>

extern const struct bt_cap_commander_cb mock_cap_commander_cb;

void mock_cap_commander_init(void);
void mock_cap_commander_cleanup(void);

DECLARE_FAKE_VOID_FUNC(mock_cap_commander_discovery_complete_cb, struct bt_conn *, int,
		       const struct bt_csip_set_coordinator_csis_inst *);
DECLARE_FAKE_VOID_FUNC(mock_cap_commander_volume_changed_cb, struct bt_conn *, int);
DECLARE_FAKE_VOID_FUNC(mock_cap_commander_volume_offset_changed_cb, struct bt_conn *, int);

#endif /* MOCKS_CAP_COMMANDER_H_ */
