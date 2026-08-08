/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/fff.h>

#include "cap_handover.h"

/* List of fakes used by this unit tester */
#define FFF_FAKES_LIST(FAKE)                                                                       \
	FAKE(mock_unicast_to_broadcast_complete_cb)                                                \
	FAKE(mock_broadcast_to_unicast_complete_cb)

DEFINE_FAKE_VOID_FUNC(mock_unicast_to_broadcast_complete_cb, int, struct bt_conn *,
		      struct bt_cap_unicast_group *, struct bt_cap_broadcast_source *);
DEFINE_FAKE_VOID_FUNC(mock_broadcast_to_unicast_complete_cb, int, struct bt_conn *,
		      struct bt_cap_broadcast_source *, struct bt_cap_unicast_group *);

const struct bt_cap_handover_cb mock_cap_handover_cb = {
	.unicast_to_broadcast_complete = mock_unicast_to_broadcast_complete_cb,
	.broadcast_to_unicast_complete = mock_broadcast_to_unicast_complete_cb,
};

void mock_cap_handover_init(void)
{
	FFF_FAKES_LIST(RESET_FAKE);
}
