/*
 * Copyright (c) 2023-2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>

#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/bluetooth.h>

#include "cap_initiator.h"
#include <zephyr/fff.h>

/* List of fakes used by this unit tester */
#define FFF_FAKES_LIST(FAKE)                                                                       \
	FAKE(mock_cap_initiator_unicast_discovery_complete_cb)                                     \
	FAKE(mock_cap_initiator_unicast_start_complete_cb)                                         \
	FAKE(mock_cap_initiator_unicast_start_codec_configured_cb)                                 \
	FAKE(mock_cap_initiator_unicast_start_qos_configured_cb)                                   \
	FAKE(mock_cap_initiator_unicast_start_enabled_cb)                                          \
	FAKE(mock_cap_initiator_unicast_start_connected_cb)                                        \
	FAKE(mock_cap_initiator_unicast_start_started_cb)                                          \
	FAKE(mock_cap_initiator_unicast_update_complete_cb)                                        \
	FAKE(mock_cap_initiator_unicast_stop_complete_cb)                                          \
	FAKE(mock_cap_initiator_unicast_stop_disabled_cb)                                          \
	FAKE(mock_cap_initiator_unicast_stop_stopped_cb)                                           \
	FAKE(mock_cap_initiator_unicast_stop_released_cb)

DEFINE_FAKE_VOID_FUNC(mock_cap_initiator_unicast_discovery_complete_cb, struct bt_conn *, int,
		      const struct bt_csip_set_coordinator_set_member *,
		      const struct bt_csip_set_coordinator_csis_inst *);

DEFINE_FAKE_VOID_FUNC(mock_cap_initiator_unicast_start_complete_cb, int, struct bt_conn *);
DEFINE_FAKE_VOID_FUNC(mock_cap_initiator_unicast_start_codec_configured_cb);
DEFINE_FAKE_VOID_FUNC(mock_cap_initiator_unicast_start_qos_configured_cb);
DEFINE_FAKE_VOID_FUNC(mock_cap_initiator_unicast_start_enabled_cb);
DEFINE_FAKE_VOID_FUNC(mock_cap_initiator_unicast_start_connected_cb);
DEFINE_FAKE_VOID_FUNC(mock_cap_initiator_unicast_start_started_cb);
DEFINE_FAKE_VOID_FUNC(mock_cap_initiator_unicast_update_complete_cb, int, struct bt_conn *);
DEFINE_FAKE_VOID_FUNC(mock_cap_initiator_unicast_stop_complete_cb, int, struct bt_conn *);
DEFINE_FAKE_VOID_FUNC(mock_cap_initiator_unicast_stop_disabled_cb);
DEFINE_FAKE_VOID_FUNC(mock_cap_initiator_unicast_stop_stopped_cb);
DEFINE_FAKE_VOID_FUNC(mock_cap_initiator_unicast_stop_released_cb);
DEFINE_FAKE_VOID_FUNC(mock_cap_initiator_broadcast_started_cb, struct bt_cap_broadcast_source *);
DEFINE_FAKE_VOID_FUNC(mock_cap_initiator_broadcast_stopped_cb, struct bt_cap_broadcast_source *,
		      uint8_t);

const struct bt_cap_initiator_cb mock_cap_initiator_cb = {
#if defined(CONFIG_BT_BAP_UNICAST_CLIENT)
	.unicast_discovery_complete = mock_cap_initiator_unicast_discovery_complete_cb,
	.unicast_start_complete = mock_cap_initiator_unicast_start_complete_cb,
	.unicast_start_codec_configured = mock_cap_initiator_unicast_start_codec_configured_cb,
	.unicast_start_qos_configured = mock_cap_initiator_unicast_start_qos_configured_cb,
	.unicast_start_enabled = mock_cap_initiator_unicast_start_enabled_cb,
	.unicast_start_connected = mock_cap_initiator_unicast_start_connected_cb,
	.unicast_start_started = mock_cap_initiator_unicast_start_started_cb,
	.unicast_update_complete = mock_cap_initiator_unicast_update_complete_cb,
	.unicast_stop_complete = mock_cap_initiator_unicast_stop_complete_cb,
	.unicast_stop_disabled = mock_cap_initiator_unicast_stop_disabled_cb,
	.unicast_stop_stopped = mock_cap_initiator_unicast_stop_stopped_cb,
	.unicast_stop_released = mock_cap_initiator_unicast_stop_released_cb,
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT */
#if defined(CONFIG_BT_BAP_BROADCAST_SOURCE)
	.broadcast_started = mock_cap_initiator_broadcast_started_cb,
	.broadcast_stopped = mock_cap_initiator_broadcast_stopped_cb,
#endif /* CONFIG_BT_BAP_BROADCAST_SOURCE */
};

void mock_cap_initiator_init(void)
{
	FFF_FAKES_LIST(RESET_FAKE);
}

void mock_cap_initiator_cleanup(void)
{
}
