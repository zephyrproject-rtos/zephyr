/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/audio/cap.h>

#include "cap_commander.h"

/* List of fakes used by this unit tester */
#define FFF_FAKES_LIST(FAKE)                                                                       \
	FAKE(mock_cap_commander_discovery_complete_cb)                                             \
	FAKE(mock_cap_commander_volume_changed_cb)                                                 \
	FAKE(mock_cap_commander_volume_mute_changed_cb)                                            \
	FAKE(mock_cap_commander_volume_offset_changed_cb)                                          \
	FAKE(mock_cap_commander_microphone_mute_changed_cb)                                        \
	FAKE(mock_cap_commander_microphone_gain_changed_cb)                                        \
	FAKE(mock_cap_commander_broadcast_reception_start_cb)                                      \
	FAKE(mock_cap_commander_broadcast_reception_stop_cb)                                       \
	FAKE(mock_cap_commander_distribute_broadcast_code_cb)

DEFINE_FAKE_VOID_FUNC(mock_cap_commander_discovery_complete_cb, struct bt_conn *, int,
		      const struct bt_csip_set_coordinator_set_member *,
		      const struct bt_csip_set_coordinator_csis_inst *);

DEFINE_FAKE_VOID_FUNC(mock_cap_commander_volume_changed_cb, struct bt_conn *, int);
DEFINE_FAKE_VOID_FUNC(mock_cap_commander_volume_mute_changed_cb, struct bt_conn *, int);
DEFINE_FAKE_VOID_FUNC(mock_cap_commander_volume_offset_changed_cb, struct bt_conn *, int);
DEFINE_FAKE_VOID_FUNC(mock_cap_commander_microphone_mute_changed_cb, struct bt_conn *, int);
DEFINE_FAKE_VOID_FUNC(mock_cap_commander_microphone_gain_changed_cb, struct bt_conn *, int);
DEFINE_FAKE_VOID_FUNC(mock_cap_commander_broadcast_reception_start_cb, struct bt_conn *, int);
DEFINE_FAKE_VOID_FUNC(mock_cap_commander_broadcast_reception_stop_cb, struct bt_conn *, int);
DEFINE_FAKE_VOID_FUNC(mock_cap_commander_distribute_broadcast_code_cb, struct bt_conn *, int);

const struct bt_cap_commander_cb mock_cap_commander_cb = {
	.discovery_complete = mock_cap_commander_discovery_complete_cb,
#if defined(CONFIG_BT_VCP_VOL_CTLR)
	.volume_changed = mock_cap_commander_volume_changed_cb,
	.volume_mute_changed = mock_cap_commander_volume_mute_changed_cb,
#if defined(CONFIG_BT_VCP_VOL_CTLR_VOCS)
	.volume_offset_changed = mock_cap_commander_volume_offset_changed_cb,
#endif /* CONFIG_BT_VCP_VOL_CTLR */
#endif /* CONFIG_BT_VCP_VOL_CTLR */
#if defined(CONFIG_BT_MICP_MIC_CTLR)
	.microphone_mute_changed = mock_cap_commander_microphone_mute_changed_cb,
#if defined(CONFIG_BT_MICP_MIC_CTLR_AICS)
	.microphone_gain_changed = mock_cap_commander_microphone_gain_changed_cb,
#endif /* CONFIG_BT_MICP_MIC_CTLR_AICS */
#endif /* CONFIG_BT_MICP_MIC_CTLR */
#if defined(CONFIG_BT_BAP_BROADCAST_ASSISTANT)
	.broadcast_reception_start = mock_cap_commander_broadcast_reception_start_cb,
	.broadcast_reception_stop = mock_cap_commander_broadcast_reception_stop_cb,
	.distribute_broadcast_code = mock_cap_commander_distribute_broadcast_code_cb,
#endif /* CONFIG_BT_BAP_BROADCAST_ASSISTANT */
};

void mock_cap_commander_init(void)
{
	FFF_FAKES_LIST(RESET_FAKE);
}

void mock_cap_commander_cleanup(void)
{
}
