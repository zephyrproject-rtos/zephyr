/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCKS_CAP_COMMANDER_H_
#define MOCKS_CAP_COMMANDER_H_

#include <zephyr/fff.h>
#include <zephyr/bluetooth/audio/cap.h>

extern struct bt_cap_commander_cb mock_cap_commander_cb;

void mock_cap_commander_init(void);
void mock_cap_commander_cleanup(void);

DECLARE_FAKE_VOID_FUNC(mock_cap_commander_discovery_complete_cb, struct bt_conn *, int,
		       const struct bt_csip_set_coordinator_csis_inst *);

#endif /* MOCKS_CAP_COMMANDER_H_ */
