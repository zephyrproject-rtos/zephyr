/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/sys/check.h>

#include "cap_internal.h"
#include "csip_internal.h"

LOG_MODULE_REGISTER(bt_cap_common, CONFIG_BT_CAP_COMMON_LOG_LEVEL);

#include "common/bt_str.h"

static struct bt_cap_common_client bt_cap_common_clients[CONFIG_BT_MAX_CONN];
static const struct bt_uuid *cas_uuid = BT_UUID_CAS;
static struct bt_cap_common_proc active_proc;

struct bt_cap_common_proc *bt_cap_common_get_active_proc(void)
{
	return &active_proc;
}

void bt_cap_common_clear_active_proc(void)
{
	(void)memset(&active_proc, 0, sizeof(active_proc));
}

void bt_cap_common_start_proc(enum bt_cap_common_proc_type proc_type, size_t proc_cnt)
{
	atomic_set_bit(active_proc.proc_state_flags, BT_CAP_COMMON_PROC_STATE_ACTIVE);
	active_proc.proc_cnt = proc_cnt;
	active_proc.proc_type = proc_type;
	active_proc.proc_done_cnt = 0U;
	active_proc.proc_initiated_cnt = 0U;
}

#if defined(CONFIG_BT_CAP_INITIATOR_UNICAST)
void bt_cap_common_set_subproc(enum bt_cap_common_subproc_type subproc_type)
{
	active_proc.proc_done_cnt = 0U;
	active_proc.proc_initiated_cnt = 0U;
	active_proc.subproc_type = subproc_type;
}

bool bt_cap_common_subproc_is_type(enum bt_cap_common_subproc_type subproc_type)
{
	return active_proc.subproc_type == subproc_type;
}
#endif /* CONFIG_BT_CAP_INITIATOR_UNICAST */

struct bt_conn *bt_cap_common_get_member_conn(enum bt_cap_set_type type,
					      const union bt_cap_set_member *member)
{
	if (type == BT_CAP_SET_TYPE_CSIP) {
		struct bt_cap_common_client *client;

		/* We have verified that `client` won't be NULL in
		 * `valid_change_volume_param`.
		 */
		client = bt_cap_common_get_client_by_csis(member->csip);
		if (client != NULL) {
			return client->conn;
		}
	}

	return member->member;
}

bool bt_cap_common_proc_is_active(void)
{
	return atomic_test_bit(active_proc.proc_state_flags, BT_CAP_COMMON_PROC_STATE_ACTIVE);
}

bool bt_cap_common_proc_is_aborted(void)
{
	return atomic_test_bit(active_proc.proc_state_flags, BT_CAP_COMMON_PROC_STATE_ABORTED);
}

bool bt_cap_common_proc_all_handled(void)
{
	return active_proc.proc_done_cnt == active_proc.proc_initiated_cnt;
}

bool bt_cap_common_proc_is_done(void)
{
	return active_proc.proc_done_cnt == active_proc.proc_cnt;
}

void bt_cap_common_abort_proc(struct bt_conn *conn, int err)
{
	if (bt_cap_common_proc_is_aborted()) {
		/* no-op */
		return;
	}

	active_proc.err = err;
	active_proc.failed_conn = conn;
	atomic_set_bit(active_proc.proc_state_flags, BT_CAP_COMMON_PROC_STATE_ABORTED);
}

#if defined(CONFIG_BT_CAP_INITIATOR_UNICAST)
static bool active_proc_is_initiator(void)
{
	switch (active_proc.proc_type) {
	case BT_CAP_COMMON_PROC_TYPE_START:
	case BT_CAP_COMMON_PROC_TYPE_UPDATE:
	case BT_CAP_COMMON_PROC_TYPE_STOP:
		return true;
	default:
		return false;
	}
}
#endif /* CONFIG_BT_CAP_INITIATOR_UNICAST */

#if defined(CONFIG_BT_CAP_COMMANDER)
static bool active_proc_is_commander(void)
{
	switch (active_proc.proc_type) {
	case BT_CAP_COMMON_PROC_TYPE_VOLUME_CHANGE:
	case BT_CAP_COMMON_PROC_TYPE_VOLUME_OFFSET_CHANGE:
	case BT_CAP_COMMON_PROC_TYPE_VOLUME_MUTE_CHANGE:
	case BT_CAP_COMMON_PROC_TYPE_MICROPHONE_GAIN_CHANGE:
	case BT_CAP_COMMON_PROC_TYPE_MICROPHONE_MUTE_CHANGE:
		return true;
	default:
		return false;
	}
}
#endif /* CONFIG_BT_CAP_INITIATOR_UNICAST */

bool bt_cap_common_conn_in_active_proc(const struct bt_conn *conn)
{
	if (!bt_cap_common_proc_is_active()) {
		return false;
	}

	for (size_t i = 0U; i < active_proc.proc_initiated_cnt; i++) {
#if defined(CONFIG_BT_CAP_INITIATOR_UNICAST)
		if (active_proc_is_initiator()) {
			if (active_proc.proc_param.initiator[i].stream->bap_stream.conn == conn) {
				return true;
			}
		}
#endif /* CONFIG_BT_CAP_INITIATOR_UNICAST */
#if defined(CONFIG_BT_CAP_COMMANDER)
		if (active_proc_is_commander()) {
			if (active_proc.proc_param.commander[i].conn == conn) {
				return true;
			}
		}
#endif /* CONFIG_BT_CAP_INITIATOR_UNICAST */
	}

	return false;
}

bool bt_cap_common_stream_in_active_proc(const struct bt_cap_stream *cap_stream)
{
	if (!bt_cap_common_proc_is_active()) {
		return false;
	}

#if defined(CONFIG_BT_CAP_INITIATOR_UNICAST)
	if (active_proc_is_initiator()) {
		for (size_t i = 0U; i < active_proc.proc_cnt; i++) {
			if (active_proc.proc_param.initiator[i].stream == cap_stream) {
				return true;
			}
		}
	}
#endif /* CONFIG_BT_CAP_INITIATOR_UNICAST */

	return false;
}

void bt_cap_common_disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct bt_cap_common_client *client = bt_cap_common_get_client_by_acl(conn);

	if (client->conn != NULL) {
		bt_conn_unref(client->conn);
	}
	(void)memset(client, 0, sizeof(*client));

	if (bt_cap_common_conn_in_active_proc(conn)) {
		bt_cap_common_abort_proc(conn, -ENOTCONN);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.disconnected = bt_cap_common_disconnected,
};

struct bt_cap_common_client *bt_cap_common_get_client_by_acl(const struct bt_conn *acl)
{
	if (acl == NULL) {
		return NULL;
	}

	return &bt_cap_common_clients[bt_conn_index(acl)];
}

struct bt_cap_common_client *
bt_cap_common_get_client_by_csis(const struct bt_csip_set_coordinator_csis_inst *csis_inst)
{
	if (csis_inst == NULL) {
		return NULL;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(bt_cap_common_clients); i++) {
		struct bt_cap_common_client *client = &bt_cap_common_clients[i];

		if (client->csis_inst == csis_inst) {
			return client;
		}
	}

	return NULL;
}

struct bt_cap_common_client *bt_cap_common_get_client(enum bt_cap_set_type type,
						      const union bt_cap_set_member *member)
{
	struct bt_cap_common_client *client = NULL;

	if (type == BT_CAP_SET_TYPE_AD_HOC) {
		CHECKIF(member->member == NULL) {
			LOG_DBG("member->member is NULL");
			return NULL;
		}

		client = bt_cap_common_get_client_by_acl(member->member);
	} else if (type == BT_CAP_SET_TYPE_CSIP) {
		CHECKIF(member->csip == NULL) {
			LOG_DBG("member->csip is NULL");
			return NULL;
		}

		client = bt_cap_common_get_client_by_csis(member->csip);
		if (client == NULL) {
			LOG_DBG("CSIS was not found for member");
			return NULL;
		}
	}

	if (client == NULL || !client->cas_found) {
		LOG_DBG("CAS was not found for member %p", member);
		return NULL;
	}

	return client;
}

static void cap_common_discover_complete(struct bt_conn *conn, int err,
					 const struct bt_csip_set_coordinator_csis_inst *csis_inst)
{
	struct bt_cap_common_client *client;

	client = bt_cap_common_get_client_by_acl(conn);
	if (client != NULL && client->discover_cb_func != NULL) {
		const bt_cap_common_discover_func_t cb_func = client->discover_cb_func;

		client->discover_cb_func = NULL;
		cb_func(conn, err, csis_inst);
	}
}

static void csis_client_discover_cb(struct bt_conn *conn,
				    const struct bt_csip_set_coordinator_set_member *member,
				    int err, size_t set_count)
{
	struct bt_cap_common_client *client;

	if (err != 0) {
		LOG_DBG("CSIS client discover failed: %d", err);

		cap_common_discover_complete(conn, err, NULL);

		return;
	}

	client = bt_cap_common_get_client_by_acl(conn);
	client->csis_inst =
		bt_csip_set_coordinator_csis_inst_by_handle(conn, client->csis_start_handle);

	if (member == NULL || set_count == 0 || client->csis_inst == NULL) {
		LOG_ERR("Unable to find CSIS for CAS");

		cap_common_discover_complete(conn, -ENODATA, NULL);
	} else {
		LOG_DBG("Found CAS with CSIS");
		cap_common_discover_complete(conn, 0, client->csis_inst);
	}
}

static uint8_t bt_cap_common_discover_included_cb(struct bt_conn *conn,
						  const struct bt_gatt_attr *attr,
						  struct bt_gatt_discover_params *params)
{
	if (attr == NULL) {
		LOG_DBG("CAS CSIS include not found");

		cap_common_discover_complete(conn, 0, NULL);
	} else {
		const struct bt_gatt_include *included_service = attr->user_data;
		struct bt_cap_common_client *client =
			CONTAINER_OF(params, struct bt_cap_common_client, param);

		/* If the remote CAS includes CSIS, we first check if we
		 * have already discovered it, and if so we can just retrieve it
		 * and forward it to the application. If not, then we start
		 * CSIS discovery
		 */
		client->csis_start_handle = included_service->start_handle;
		client->csis_inst = bt_csip_set_coordinator_csis_inst_by_handle(
			conn, client->csis_start_handle);
		if (client->csis_inst == NULL) {
			static struct bt_csip_set_coordinator_cb csis_client_cb = {
				.discover = csis_client_discover_cb,
			};
			static bool csis_cbs_registered;
			int err;

			LOG_DBG("CAS CSIS not known, discovering");

			if (!csis_cbs_registered) {
				bt_csip_set_coordinator_register_cb(&csis_client_cb);
				csis_cbs_registered = true;
			}

			err = bt_csip_set_coordinator_discover(conn);
			if (err != 0) {
				LOG_DBG("Discover failed (err %d)", err);
				cap_common_discover_complete(conn, err, NULL);
			}
		} else {
			LOG_DBG("Found CAS with CSIS");
			cap_common_discover_complete(conn, 0, client->csis_inst);
		}
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t bt_cap_common_discover_cas_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
					     struct bt_gatt_discover_params *params)
{
	if (attr == NULL) {
		cap_common_discover_complete(conn, -ENODATA, NULL);
	} else {
		const struct bt_gatt_service_val *prim_service = attr->user_data;
		struct bt_cap_common_client *client =
			CONTAINER_OF(params, struct bt_cap_common_client, param);
		int err;

		client->cas_found = true;
		client->conn = bt_conn_ref(conn);

		if (attr->handle == prim_service->end_handle) {
			LOG_DBG("Found CAS without CSIS");
			cap_common_discover_complete(conn, 0, NULL);

			return BT_GATT_ITER_STOP;
		}

		LOG_DBG("Found CAS, discovering included CSIS");

		params->uuid = NULL;
		params->start_handle = attr->handle + 1;
		params->end_handle = prim_service->end_handle;
		params->type = BT_GATT_DISCOVER_INCLUDE;
		params->func = bt_cap_common_discover_included_cb;

		err = bt_gatt_discover(conn, params);
		if (err != 0) {
			LOG_DBG("Discover failed (err %d)", err);

			cap_common_discover_complete(conn, err, NULL);
		}
	}

	return BT_GATT_ITER_STOP;
}

int bt_cap_common_discover(struct bt_conn *conn, bt_cap_common_discover_func_t func)
{
	struct bt_gatt_discover_params *param;
	struct bt_cap_common_client *client;
	int err;

	client = bt_cap_common_get_client_by_acl(conn);
	if (client->discover_cb_func != NULL) {
		return -EBUSY;
	}

	param = &client->param;
	param->func = bt_cap_common_discover_cas_cb;
	param->uuid = cas_uuid;
	param->type = BT_GATT_DISCOVER_PRIMARY;
	param->start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	param->end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;

	client->discover_cb_func = func;

	err = bt_gatt_discover(conn, param);
	if (err != 0) {
		client->discover_cb_func = NULL;

		/* Report expected possible errors */
		if (err == -ENOTCONN || err == -ENOMEM) {
			return err;
		}

		LOG_DBG("Unexpected err %d from bt_gatt_discover", err);
		return -ENOEXEC;
	}

	return 0;
}
