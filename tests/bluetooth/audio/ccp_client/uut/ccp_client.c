/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>

#include <zephyr/bluetooth/audio/ccp.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/fff.h>

#include "ccp_client.h"

/* List of fakes used by this unit tester */
#define FFF_FAKES_LIST(FAKE) FAKE(mock_ccp_client_discover_cb)

DEFINE_FAKE_VOID_FUNC(mock_ccp_client_discover_cb, struct bt_ccp_client *, int,
		      struct bt_ccp_client_bearers *);
#if defined(CONFIG_BT_TBS_CLIENT_BEARER_PROVIDER_NAME)
DEFINE_FAKE_VOID_FUNC(mock_ccp_client_bearer_provider_name_cb, struct bt_ccp_client_bearer *, int,
		      const char *);
#endif /* CONFIG_BT_TBS_CLIENT_BEARER_PROVIDER_NAME */
#if defined(CONFIG_BT_TBS_CLIENT_BEARER_PROVIDER_NAME)
DEFINE_FAKE_VOID_FUNC(mock_ccp_client_bearer_uci_cb, struct bt_ccp_client_bearer *, int,
		      const char *);
#endif /* CONFIG_BT_TBS_CLIENT_BEARER_PROVIDER_NAME */

struct bt_ccp_client_cb mock_ccp_client_cb = {
	.discover = mock_ccp_client_discover_cb,
#if defined(CONFIG_BT_TBS_CLIENT_BEARER_PROVIDER_NAME)
	.bearer_provider_name = mock_ccp_client_bearer_provider_name_cb,
#endif /* CONFIG_BT_TBS_CLIENT_BEARER_PROVIDER_NAME */
#if defined(CONFIG_BT_TBS_CLIENT_BEARER_UCI)
	.bearer_uci = mock_ccp_client_bearer_uci_cb,
#endif /* CONFIG_BT_TBS_CLIENT_BEARER_UCI */
};

void mock_ccp_client_init(void)
{
	FFF_FAKES_LIST(RESET_FAKE);
}

void mock_ccp_client_cleanup(void)
{
}
