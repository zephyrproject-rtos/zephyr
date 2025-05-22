/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCKS_CCP_CALL_CONTROL_CLIENT_H_
#define MOCKS_CCP_CALL_CONTROL_CLIENT_H_

#include <stdint.h>

#include <zephyr/bluetooth/audio/ccp.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/fff.h>

extern struct bt_ccp_call_control_client_cb mock_ccp_call_control_client_cb;

void mock_ccp_call_control_client_init(void);
void mock_ccp_call_control_client_cleanup(void);

DECLARE_FAKE_VOID_FUNC(mock_ccp_call_control_client_discover_cb,
		       struct bt_ccp_call_control_client *, int,
		       struct bt_ccp_call_control_client_bearers *);

#endif /* MOCKS_CCP_CALL_CONTROL_CLIENT_H_ */
