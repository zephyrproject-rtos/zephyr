/*
 * Copyright (c) 2023-2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/bluetooth.h>

#include "cap_initiator.h"
#include "zephyr/fff.h"

/* List of fakes used by this unit tester */
#define FFF_FAKES_LIST(FAKE)                                                                       \
	FAKE(mock_cap_initiator_unicast_discovery_complete_cb)                                     \
	FAKE(mock_cap_initiator_unicast_start_complete_cb)                                         \
	FAKE(mock_cap_initiator_unicast_update_complete_cb)                                        \
	FAKE(mock_cap_initiator_unicast_stop_complete_cb)

DEFINE_FAKE_VOID_FUNC(mock_cap_initiator_unicast_discovery_complete_cb, struct bt_conn *, int,
		      const struct bt_csip_set_coordinator_set_member *,
		      const struct bt_csip_set_coordinator_csis_inst *);

DEFINE_FAKE_VOID_FUNC(mock_cap_initiator_unicast_start_complete_cb, int, struct bt_conn *);
DEFINE_FAKE_VOID_FUNC(mock_cap_initiator_unicast_update_complete_cb, int, struct bt_conn *);
DEFINE_FAKE_VOID_FUNC(mock_cap_initiator_unicast_stop_complete_cb, int, struct bt_conn *);

const struct bt_cap_initiator_cb mock_cap_initiator_cb = {
#if defined(CONFIG_BT_BAP_UNICAST_CLIENT)
	.unicast_discovery_complete = mock_cap_initiator_unicast_discovery_complete_cb,
	.unicast_start_complete = mock_cap_initiator_unicast_start_complete_cb,
	.unicast_update_complete = mock_cap_initiator_unicast_update_complete_cb,
	.unicast_stop_complete = mock_cap_initiator_unicast_stop_complete_cb,
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT */
};

void mock_cap_initiator_init(void)
{
	FFF_FAKES_LIST(RESET_FAKE);
}

void mock_cap_initiator_cleanup(void)
{
}
