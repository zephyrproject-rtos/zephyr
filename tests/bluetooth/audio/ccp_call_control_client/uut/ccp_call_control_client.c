/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>

#include <zephyr/bluetooth/audio/ccp.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/fff.h>

#include "ccp_call_control_client.h"

/* List of fakes used by this unit tester */
#define FFF_FAKES_LIST(FAKE) FAKE(mock_ccp_call_control_client_discover_cb)

DEFINE_FAKE_VOID_FUNC(mock_ccp_call_control_client_discover_cb, struct bt_ccp_call_control_client *,
		      int, struct bt_ccp_call_control_client_bearers *);

struct bt_ccp_call_control_client_cb mock_ccp_call_control_client_cb = {
	.discover = mock_ccp_call_control_client_discover_cb,
};

void mock_ccp_call_control_client_init(void)
{
	FFF_FAKES_LIST(RESET_FAKE);
}

void mock_ccp_call_control_client_cleanup(void)
{
}
