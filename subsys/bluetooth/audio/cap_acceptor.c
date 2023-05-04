/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/check.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/audio/tbs.h>
#include <zephyr/bluetooth/audio/csip.h>
#include "cap_internal.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_cap_acceptor, CONFIG_BT_CAP_ACCEPTOR_LOG_LEVEL);

#if defined(CONFIG_BT_CAP_ACCEPTOR_SET_MEMBER)

static struct bt_gatt_attr svc_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_CAS),
	BT_GATT_INCLUDE_SERVICE(NULL) /* To be overwritten */
};

int bt_cap_acceptor_register(const struct bt_csip_set_member_register_param *param,
			     struct bt_csip_set_member_svc_inst **svc_inst)
{
	static struct bt_gatt_service cas;
	int err;

	err = bt_csip_set_member_register(param, svc_inst);
	if (err != 0) {
		LOG_DBG("Failed to register CSIP");
		return err;
	}

	cas = (struct bt_gatt_service)BT_GATT_SERVICE(svc_attrs);

	/* Overwrite the include definition with the CSIP */
	cas.attrs[1].user_data = bt_csip_set_member_svc_decl_get(*svc_inst);

	err = bt_gatt_service_register(&cas);
	if (err) {
		LOG_DBG("Failed to register CAS");
		return err;
	}

	return 0;
}

#else /* CONFIG_BT_CAP_ACCEPTOR_SET_MEMBER */

BT_GATT_SERVICE_DEFINE(cas_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_CAS)
	/* TODO: Add support for included CSIP */
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
