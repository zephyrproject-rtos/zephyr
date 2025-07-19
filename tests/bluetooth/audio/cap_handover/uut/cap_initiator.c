/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>

#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/bluetooth.h>

#include "cap_initiator.h"
#include "zephyr/fff.h"

/* List of fakes used by this unit tester */
#define FFF_FAKES_LIST(FAKE)                                                                       \
	FAKE(mock_unicast_start_complete_cb)                                                       \
	FAKE(mock_unicast_to_broadcast_complete_cb)

DEFINE_FAKE_VOID_FUNC(mock_unicast_start_complete_cb, int, struct bt_conn *);
DEFINE_FAKE_VOID_FUNC(mock_unicast_to_broadcast_complete_cb, int, struct bt_conn *,
		      struct bt_cap_unicast_group *, struct bt_cap_broadcast_source *);

const struct bt_cap_initiator_cb mock_cap_initiator_cb = {
	.unicast_start_complete = mock_unicast_start_complete_cb,
	.unicast_to_broadcast_complete = mock_unicast_to_broadcast_complete_cb,
};

void mock_cap_initiator_init(void)
{
	FFF_FAKES_LIST(RESET_FAKE);
}
