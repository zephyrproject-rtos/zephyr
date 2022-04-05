/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bluetooth/gatt.h>
#include <bluetooth/audio/tbs.h>
#include "cap_internal.h"

#if defined(CONFIG_BT_CAP_ACCEPTOR)

BT_GATT_SERVICE_DEFINE(cas_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_CAS)
	/* TODO: Add support for included CSIS */
);

bool bt_cap_acceptor_ccid_exist(const struct bt_conn *conn, uint8_t ccid)
{
	if (IS_ENABLED(CONFIG_BT_TBS_CLIENT) &&
	    bt_tbs_client_get_by_ccid(conn, ccid) != NULL) {
		return true;
	}

	/* TODO: check mcs */

	return false;
}

#endif /* BT_CAP_ACCEPTOR */
