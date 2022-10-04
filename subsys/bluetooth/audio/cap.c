/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bluetooth/gatt.h>
#include <bluetooth/audio/tbs.h>
#include <bluetooth/audio/csis.h>
#include "cap_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_CAP)
#define LOG_MODULE_NAME bt_cap
#include "common/log.h"

#if defined(CONFIG_BT_CAP_ACCEPTOR)
#if defined(CONFIG_BT_CAP_ACCEPTOR_SET_MEMBER)

static struct bt_gatt_attr svc_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_CAS),
	BT_GATT_INCLUDE_SERVICE(NULL) /* To be overwritten */
};

int bt_cap_acceptor_register(const struct bt_csis_register_param *param,
			     struct bt_csis **csis)
{
	static struct bt_gatt_service cas;
	int err;

	err = bt_csis_register(param, csis);
	if (err != 0) {
		BT_DBG("Failed to register CSIS");
		return err;
	}

	cas = (struct bt_gatt_service)BT_GATT_SERVICE(svc_attrs);

	/* Overwrite the include definition with the CSIS */
	cas.attrs[1].user_data = bt_csis_svc_decl_get(*csis);

	err = bt_gatt_service_register(&cas);
	if (err) {
		BT_DBG("Failed to register CAS");
		return err;
	}

	return 0;
}

#else

BT_GATT_SERVICE_DEFINE(cas_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_CAS)
	/* TODO: Add support for included CSIS */
);

#endif /* CONFIG_BT_CAP_ACCEPTOR_SET_MEMBER */

bool bt_cap_acceptor_ccid_exist(const struct bt_conn *conn, uint8_t ccid)
{
	if (IS_ENABLED(CONFIG_BT_TBS_CLIENT_CCID) &&
	    bt_tbs_client_get_by_ccid(conn, ccid) != NULL) {
		return true;
	}

	/* TODO: check mcs */

	return false;
}

#endif /* BT_CAP_ACCEPTOR */
