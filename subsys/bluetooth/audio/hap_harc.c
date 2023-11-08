/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/audio/hap.h>
#include <zephyr/bluetooth/audio/has.h>
#include <zephyr/net/buf.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/check.h>

#include "../bluetooth/host/hci_core.h"
#include "csip_internal.h"
#include "has_internal.h"

#include "common/bt_str.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_hap_harc, CONFIG_BT_HAP_LOG_LEVEL);

enum harc_flags {
	FLAG_CONNECTED,
	FLAG_PENDING_PROC,
	FLAG_PENDING_CSIP_DISCOVERY,

	FLAG_NUM,
};

enum harc_proc_flag {
	PROC_FLAG_BUSY,
	PROC_FLAG_PENDING_LOCK,
	PROC_FLAG_PENDING_RELEASE,
	PROC_FLAG_PENDING_OAP,
	PROC_FLAG_LOCKED,
	PROC_FLAG_NUM,
};

typedef int (*harc_proc_func_t)(struct bt_hap_harc *harc, void *user_data);

struct harc_proc {
	/** Procedure opcode */
	uint8_t opcode;

	/** Internal flags */
	ATOMIC_DEFINE(flags, PROC_FLAG_NUM);

	/** Array of controller instances to perform procedure on. */
	struct bt_hap_harc *harcs[BT_HAP_HARC_SET_SIZE_MAX];

	/** Instance count */
	uint8_t count;

	/** Set information deep-copy (in case of CSIP pointer is removed) */
	struct bt_csip_set_coordinator_set_info set_info;

	/** Pending procedure user data */
	void *user_data;
};

static struct csip_set_member {
	/** Set member object pointer */
	const struct bt_csip_set_coordinator_set_member *member;

	/** CSIS instance pointer. May be NULL */
	const struct bt_csip_set_coordinator_csis_inst *csis;

	/** The number of sets the object is member of */
	size_t set_count;
} set_members[CONFIG_BT_MAX_CONN];

static struct bt_hap_harc {
	/** HAS client instance */
	struct bt_has_client *client;

	/** Set identifier */
	uint8_t set_id;

	/** Set member pointer. Valid for BT_HAS_HEARING_AID_TYPE_BINAURAL */
	struct csip_set_member *set_member;

	/** Internal flags */
	ATOMIC_DEFINE(flags, FLAG_NUM);
} harc_insts[CONFIG_BT_MAX_CONN];

static struct harc_proc active_proc;
static sys_slist_t harc_cbs = SYS_SLIST_STATIC_INIT(&harc_cbs);
static const struct bt_hap_harc_preset_cb *preset_cb;

static void notify_proc_complete(int err)
{
	switch (active_proc.opcode) {
	case BT_HAS_OP_READ_PRESET_REQ: {
		struct bt_hap_harc_preset_read_params *params = active_proc.user_data;

		if (params->complete != NULL) {
			params->complete(err, params);
		}

		break;
	}
	case BT_HAS_OP_WRITE_PRESET_NAME: {
		struct bt_hap_harc_preset_write_params *params = active_proc.user_data;

		if (params->complete != NULL) {
			params->complete(err, params);
		}

		break;
	}
	case BT_HAS_OP_SET_ACTIVE_PRESET:
	case BT_HAS_OP_SET_NEXT_PRESET:
	case BT_HAS_OP_SET_PREV_PRESET:
	case BT_HAS_OP_SET_ACTIVE_PRESET_SYNC:
	case BT_HAS_OP_SET_NEXT_PRESET_SYNC:
	case BT_HAS_OP_SET_PREV_PRESET_SYNC: {
		struct bt_hap_harc_preset_set_params *params = active_proc.user_data;

		if (params->complete != NULL) {
			params->complete(err, params);
		}

		break;
	}
	default:
		__ASSERT_PRINT("invalid opcode 0x%02x", active_proc.opcode);
		break;
	};
}

static void harc_proc_complete(int err)
{
	if (atomic_test_bit(active_proc.flags, PROC_FLAG_LOCKED)) {
		const struct bt_csip_set_coordinator_set_member *members[BT_HAP_HARC_SET_SIZE_MAX];
		int ret;

		for (size_t i = 0; i < active_proc.count; i++) {
			struct csip_set_member *set_member;
			struct bt_hap_harc *harc;

			harc = active_proc.harcs[i];
			__ASSERT_NO_MSG(harc != NULL);

			set_member = harc->set_member;
			__ASSERT_NO_MSG(harc != NULL);

			members[i] = set_member->member;
			__ASSERT_NO_MSG(members[i] != NULL);
		}

		atomic_set_bit(active_proc.flags, PROC_FLAG_PENDING_RELEASE);
		ret = bt_csip_set_coordinator_release(members, active_proc.count,
						      &active_proc.set_info);
		if (ret != 0) {
			LOG_ERR("Failed to release the locks (err %d)", ret);
		} else {
			return;
		}
	}

	atomic_clear(active_proc.flags);

	notify_proc_complete(err);
}

static void notify_proc_status(struct bt_hap_harc *harc, int err)
{
	switch (active_proc.opcode) {
	case BT_HAS_OP_READ_PRESET_REQ:
		break;
	case BT_HAS_OP_WRITE_PRESET_NAME: {
		struct bt_hap_harc_preset_write_params *params = active_proc.user_data;

		if (params->status != NULL) {
			params->status(harc, err, params);
		}

		break;
	}
	case BT_HAS_OP_SET_ACTIVE_PRESET:
	case BT_HAS_OP_SET_NEXT_PRESET:
	case BT_HAS_OP_SET_PREV_PRESET:
	case BT_HAS_OP_SET_ACTIVE_PRESET_SYNC:
	case BT_HAS_OP_SET_NEXT_PRESET_SYNC:
	case BT_HAS_OP_SET_PREV_PRESET_SYNC: {
		struct bt_hap_harc_preset_set_params *params = active_proc.user_data;

		if (params->status != NULL) {
			params->status(harc, err, params);
		}

		break;
	}
	default:
		__ASSERT_PRINT("invalid opcode 0x%02x", active_proc.opcode);
		break;
	};
}

static int harc_proc_handler(void);

static void harc_proc_status(struct bt_hap_harc *harc, int err)
{
	__ASSERT_NO_MSG(atomic_get(active_proc.flags) > 0);

	LOG_DBG("%p %s (err %d)", harc, err != 0 ? "failed" : "", err);

	atomic_clear_bit(harc->flags, FLAG_PENDING_PROC);
	notify_proc_status(harc, err);
	harc_proc_handler();
}

static struct bt_hap_harc *harc_proc_get_next_inst(void)
{
	__ASSERT_NO_MSG(atomic_get(active_proc.flags) > 0);

	for (size_t i = 0; i < active_proc.count; i++) {
		struct bt_hap_harc *harc;

		harc = active_proc.harcs[i];
		__ASSERT_NO_MSG(harc != NULL);

		if (atomic_test_bit(harc->flags, FLAG_PENDING_PROC)) {
			return harc;
		}
	}

	return NULL;
}

static int harc_proc_handler(void)
{
	struct bt_hap_harc *harc;
	bool oob_sync = false;
	int err;

	harc = harc_proc_get_next_inst();
	if (harc == NULL) {
		harc_proc_complete(0);

		return 0;
	}

	switch (active_proc.opcode) {
	case BT_HAS_OP_READ_PRESET_REQ: {
		struct bt_hap_harc_preset_read_params *params = active_proc.user_data;

		__ASSERT_NO_MSG(params != NULL);

		err = bt_has_client_cmd_presets_read(harc->client, params->start_index,
						     params->max_count);
		break;
	}
	case BT_HAS_OP_WRITE_PRESET_NAME: {
		struct bt_hap_harc_preset_write_params *params = active_proc.user_data;

		__ASSERT_NO_MSG(params != NULL);

		err = bt_has_client_cmd_preset_write(harc->client, params->index, params->name);
		break;
	}
	case BT_HAS_OP_SET_ACTIVE_PRESET_SYNC:
		oob_sync = true;
		/* fallthrough */
	case BT_HAS_OP_SET_ACTIVE_PRESET: {
		struct bt_hap_harc_preset_set_params *params = active_proc.user_data;
		uint8_t preset_index;

		__ASSERT_NO_MSG(params != NULL);

		preset_index = POINTER_TO_UINT(params->reserved);

		err = bt_has_client_cmd_preset_set(harc->client, preset_index, oob_sync);
		break;
	}
	case BT_HAS_OP_SET_NEXT_PRESET_SYNC:
		oob_sync = true;
		/* fallthrough */
	case BT_HAS_OP_SET_NEXT_PRESET: {
		err = bt_has_client_cmd_preset_next(harc->client, oob_sync);
		break;
	}
	case BT_HAS_OP_SET_PREV_PRESET_SYNC:
		oob_sync = true;
		/* fallthrough */
	case BT_HAS_OP_SET_PREV_PRESET: {
		err = bt_has_client_cmd_preset_prev(harc->client, oob_sync);
		break;
	}
	default:
		__ASSERT_PRINT("invalid opcode 0x%02x", active_proc.opcode);
		harc_proc_complete(-EINVAL);
		return -EINVAL;
	}

	if (err != 0) {
		LOG_ERR("HARC %p failed to execute procedure (err %d)", harc, err);
		harc_proc_status(harc, err);

		return err;
	}

	return 0;
}

static struct bt_conn *harc_conn_get(const struct bt_hap_harc *harc)
{
	struct bt_conn *conn;
	int err;

	if (harc->client == NULL) {
		return NULL;
	}

	err = bt_has_client_conn_get(harc->client, &conn);
	if (err != 0) {
		return NULL;
	}

	return conn;
}

static struct bt_hap_harc *harc_find_by_conn(const struct bt_conn *conn)
{
	__ASSERT_NO_MSG(conn != NULL);

	for (size_t i = 0; i < ARRAY_SIZE(harc_insts); i++) {
		struct bt_hap_harc *harc = &harc_insts[i];

		if (harc_conn_get(harc) == conn) {
			return harc;
		}
	}

	return NULL;
}

static struct bt_hap_harc *harc_find_by_client(const struct bt_has_client *client)
{
	for (size_t i = 0; i < ARRAY_SIZE(harc_insts); i++) {
		struct bt_hap_harc *harc = &harc_insts[i];

		if (harc->client == client) {
			return harc;
		}
	}

	return NULL;
}

static struct bt_hap_harc *harc_find_by_member(
	const struct bt_csip_set_coordinator_set_member *member)
{
	for (size_t i = 0; i < ARRAY_SIZE(harc_insts); i++) {
		struct bt_hap_harc *harc = &harc_insts[i];

		if (harc->set_member != NULL && harc->set_member->member == member) {
			return harc;
		}
	}

	return NULL;
}

static bool sirk_eq(const uint8_t *sirk_0, const uint8_t *sirk_1)
{
	__ASSERT_NO_MSG(sirk_0 != NULL);
	__ASSERT_NO_MSG(sirk_1 != NULL);

	return memcmp(sirk_0, sirk_1, BT_CSIP_SET_SIRK_SIZE) == 0;
}

static const struct bt_csip_set_coordinator_csis_inst *csis_lookup_sirk(
	struct csip_set_member *set_member, const uint8_t *sirk)
{
	for (size_t i = 0; i < set_member->set_count; i++) {
		const struct bt_csip_set_coordinator_csis_inst *inst;

		inst = &set_member->member->insts[i];
		if (inst->info.set_size != BT_HAP_HARC_SET_SIZE_MAX) {
			/* This can be some other group */
			continue;
		}

		if (sirk_eq(inst->info.set_sirk, sirk)) {
			return inst;
		}
	}

	return NULL;
}

static void set_member_set_csis(struct csip_set_member *set_member,
				const struct bt_csip_set_coordinator_csis_inst *csis)
{
	__ASSERT_NO_MSG(set_member != NULL);
	__ASSERT_NO_MSG(set_member->csis == NULL || set_member->csis == csis);

	set_member->csis = csis;
}

static int harc_set_add(struct bt_hap_harc *harc_0, struct bt_hap_harc *harc_1,
			const struct bt_csip_set_coordinator_csis_inst *csis_0,
			const struct bt_csip_set_coordinator_csis_inst *csis_1)
{
	__ASSERT_NO_MSG(harc_0 != NULL);
	__ASSERT_NO_MSG(harc_1 != NULL);

	LOG_DBG("harc %p harc %p", harc_0, harc_1);

	set_member_set_csis(harc_0->set_member, csis_0);
	set_member_set_csis(harc_1->set_member, csis_1);

	harc_1->set_id = harc_0->set_id;

	LOG_HEXDUMP_DBG(csis_0->info.set_sirk, BT_CSIP_SET_SIRK_SIZE, "Set SIRK");

	return 0;
}

static void harc_set_member_init(struct bt_hap_harc *harc)
{
	/* FIXME: As the discovery result does not provide GATT Service context the
	 *        set is used, we pair devices in set if there's any SIRK match.
	 */
	for (size_t i = 0; i < harc->set_member->set_count; i++) {
		const struct bt_csip_set_coordinator_csis_inst *csis;
		const uint8_t *set_sirk;

		csis = &harc->set_member->member->insts[i];
		if (csis->info.set_size != BT_HAP_HARC_SET_SIZE_MAX) {
			/* This can be some other group */
			continue;
		}

		set_sirk = csis->info.set_sirk;

		for (size_t j = 0; j < ARRAY_SIZE(harc_insts); j++) {
			const struct bt_csip_set_coordinator_csis_inst *pair_csis = NULL;
			struct bt_hap_harc *pair = &harc_insts[j];
			int err;

			if (pair == harc) {
				/* self */
				continue;
			}

			if (pair->client == NULL || pair->set_member == NULL) {
				continue;
			}

			if (pair->set_member->csis == NULL) {
				pair_csis = csis_lookup_sirk(pair->set_member, set_sirk);
			} else if (sirk_eq(pair->set_member->csis->info.set_sirk, set_sirk)) {
				pair_csis = pair->set_member->csis;
			}

			if (pair_csis == NULL) {
				continue;
			}

			err = harc_set_add(pair, harc, pair_csis, csis);
			if (err == 0) {
				return;
			}
		}
	}
}

static int set_coordinator_discover(struct bt_hap_harc *harc)
{
	struct bt_conn *conn;
	int err;

	atomic_set_bit(harc->flags, FLAG_PENDING_CSIP_DISCOVERY);

	conn = harc_conn_get(harc);
	__ASSERT_NO_MSG(conn != NULL);

	err = bt_csip_set_coordinator_discover(conn);
	if (err != 0) {
		LOG_ERR("bt_csip_set_coordinator_discover (err %d)", err);
		atomic_clear_bit(harc->flags, FLAG_PENDING_CSIP_DISCOVERY);
	}

	return err;
}

static void notify_connected(struct bt_hap_harc *harc, int err)
{
	struct bt_hap_harc_cb *cb;

	if (err == 0) {
		atomic_set_bit(harc->flags, FLAG_CONNECTED);
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&harc_cbs, cb, _node) {
		if (cb->connected != NULL) {
			cb->connected(harc, err);
		}
	}
}

static void notify_disconnected(struct bt_hap_harc *harc)
{
	if (atomic_test_and_clear_bit(harc->flags, FLAG_CONNECTED)) {
		struct bt_hap_harc_cb *cb;

		SYS_SLIST_FOR_EACH_CONTAINER(&harc_cbs, cb, _node) {
			if (cb->disconnected != NULL) {
				cb->disconnected(harc);
			}
		}
	} else {
		notify_connected(harc, -ECANCELED);
	}
}

static void harc_free(struct bt_hap_harc *harc)
{
	if (harc->client != NULL) {
		(void)bt_has_client_unbind(harc->client);
		harc->client = NULL;
	}
}

static void client_connected_cb(struct bt_has_client *client, int err)
{
	struct bt_has_client_info info = { 0 };
	struct bt_hap_harc *harc;
	struct bt_conn *conn;

	LOG_DBG("client %p err %d", (void *)client, err);

	harc = harc_find_by_client(client);
	__ASSERT_NO_MSG(harc != NULL);

	if (err != 0) {
		LOG_ERR("Connection failed (err %d)", err);
		harc_free(harc);

		notify_connected(harc, err);

		return;
	}

	err = bt_has_client_conn_get(harc->client, &conn);
	__ASSERT_NO_MSG(err == 0);

	err = bt_has_client_info_get(harc->client, &info);
	__ASSERT_NO_MSG(err == 0);

	/* HAP_v1.0; Connection establishment procedures
	 *
	 * HARC shall use the discovery procedures defined in CSIP for the Set Coordinator
	 * role to discover Binaural Hearing Aid Set members.
	 */
	if (info.type == BT_HAS_HEARING_AID_TYPE_BINAURAL) {
		struct csip_set_member *set_member;

		set_member = &set_members[bt_conn_index(conn)];
		if (set_member->member == NULL) {
			err = set_coordinator_discover(harc);
			if (err == 0) {
				/* Wait for CSIP discovery results */
				return;
			} else if (err == -ENOMEM) {
				/* Try again */
				err = -EAGAIN;
			}
		} else {
			harc->set_member = set_member;
			harc_set_member_init(harc);
		}
	}

	notify_connected(harc, err);
}

static void client_disconnected_cb(struct bt_has_client *client)
{
	struct bt_hap_harc *harc;

	LOG_DBG("client %p", (void *)client);

	harc = harc_find_by_client(client);
	if (harc == NULL) {
		return;
	}

	notify_disconnected(harc);
}

static void client_unbound_cb(struct bt_has_client *client, int err)
{
	struct bt_hap_harc *harc;

	LOG_DBG("client %p err %d", (void *)client, err);

	harc = harc_find_by_client(client);
	if (harc == NULL) {
		return;
	}

	harc_free(harc);
}

static void client_preset_switch_cb(struct bt_has_client *client, uint8_t index)
{
	struct bt_hap_harc *harc;

	LOG_DBG("client %p index 0x%02x", (void *)client, index);

	if (preset_cb == NULL) {
		return;
	}

	harc = harc_find_by_client(client);
	if (harc == NULL) {
		return;
	}

	if (preset_cb->active) {
		preset_cb->active(harc, index);
	}
}

static void client_preset_read_rsp_cb(struct bt_has_client *client,
				      const struct bt_has_preset_record *record, bool is_last)
{
	struct bt_hap_harc *harc;

	LOG_DBG("client %p index 0x%02x is_last %s",
		(void *)client, record->index, is_last ? "yes" : "no");

	harc = harc_find_by_client(client);
	if (harc == NULL) {
		return;
	}

	if (preset_cb != NULL && preset_cb->store != NULL) {
		preset_cb->store(harc, record);
	}

	if (!is_last) {
		return;
	}

	if (preset_cb != NULL && preset_cb->commit != NULL) {
		preset_cb->commit(harc);
	}
}

static void client_preset_update_cb(struct bt_has_client *client, uint8_t index_prev,
				    const struct bt_has_preset_record *record, bool is_last)
{
	struct bt_hap_harc *harc;
	uint8_t offset;

	LOG_DBG("client %p index_prev 0x%02x index 0x%02x is_last %s",
		(void *)client, index_prev, record->index, is_last ? "yes" : "no");

	if (preset_cb == NULL) {
		return;
	}

	harc = harc_find_by_client(client);
	if (harc == NULL) {
		return;
	}

	offset = index_prev - record->index;
	if (offset > 1U && preset_cb->remove != NULL) {
		preset_cb->remove(harc, index_prev + 1, record->index - 1);
	}

	if (preset_cb->store != NULL) {
		preset_cb->store(harc, record);
	}

	if (is_last && preset_cb->commit) {
		preset_cb->commit(harc);
	}
}

static void client_preset_available_cb(struct bt_has_client *client, uint8_t index,
					  bool available, bool is_last)
{
	struct bt_hap_harc *harc;

	LOG_DBG("client %p index 0x%02x available %s is_last %s",
		(void *)client, index, available ? "yes" : "no", is_last ? "yes" : "no");

	if (preset_cb == NULL) {
		return;
	}

	harc = harc_find_by_client(client);
	if (harc == NULL) {
		return;
	}

	if (preset_cb->available) {
		preset_cb->available(harc, index, available);
	}

	if (is_last && preset_cb->commit) {
		preset_cb->commit(harc);
	}
}

static void client_preset_deleted_cb(struct bt_has_client *client, uint8_t index, bool is_last)
{
	struct bt_hap_harc *harc;

	LOG_DBG("client %p index 0x%02x is_last %s", (void *)client, index, is_last ? "yes" : "no");

	if (preset_cb == NULL) {
		return;
	}

	harc = harc_find_by_client(client);
	if (harc == NULL) {
		return;
	}

	if (preset_cb->remove) {
		preset_cb->remove(harc, index, index);
	}

	if (is_last && preset_cb->commit) {
		preset_cb->commit(harc);
	}
}

static void client_features_cb(struct bt_has_client *client, enum bt_has_features features)
{
	struct bt_hap_harc_info info = { 0 };
	struct bt_hap_harc_cb *cb;
	struct bt_hap_harc *harc;
	int err;

	LOG_DBG("client %p features 0x%02x", (void *)client, features);

	harc = harc_find_by_client(client);
	__ASSERT_NO_MSG(harc != NULL);

	err = bt_hap_harc_info_get(harc, &info);
	__ASSERT_NO_MSG(err == 0);

	SYS_SLIST_FOR_EACH_CONTAINER(&harc_cbs, cb, _node) {
		if (cb->info != NULL) {
			cb->info(harc, &info);
		}
	}
}

static void client_status_cb(struct bt_has_client *client, uint8_t err)
{
	struct bt_hap_harc *harc;

	LOG_DBG("client %p err 0x%02x", (void *)client, err);

	harc = harc_find_by_client(client);
	__ASSERT_NO_MSG(harc != NULL);

	if (atomic_test_bit(active_proc.flags, PROC_FLAG_BUSY)) {
		harc_proc_status(harc, err);
	}
}

static const struct bt_has_client_cb client_cb = {
	.connected = client_connected_cb,
	.disconnected = client_disconnected_cb,
	.unbound = client_unbound_cb,
	.preset_switch = client_preset_switch_cb,
	.preset_read_rsp = client_preset_read_rsp_cb,
	.preset_update = client_preset_update_cb,
	.preset_deleted = client_preset_deleted_cb,
	.preset_availability = client_preset_available_cb,
	.features = client_features_cb,
	.cmd_status = client_status_cb,
};

static void set_coordinator_discover_cb(struct bt_conn *conn,
					const struct bt_csip_set_coordinator_set_member *member,
					int err, size_t set_count)
{
	struct csip_set_member *set_member = &set_members[bt_conn_index(conn)];
	struct bt_hap_harc *harc;

	LOG_DBG("conn %p member %p err %d set_count %zu", (void *)conn, member, err, set_count);

	if (err == 0) {
		__ASSERT(set_member->member == NULL || set_member->member == member,
			 "Unexpected set member pointer change %p -> %p",
			 set_member->member, member);

		set_member->set_count = set_count;
		set_member->member = set_member->set_count > 0 ? member : NULL;
		if (set_member->member == NULL) {
			/* It's a error case for us, as we expect the device to be in set */
			err = -EINVAL;
		}
	}

	harc = harc_find_by_conn(conn);
	if (harc != NULL && atomic_test_and_clear_bit(harc->flags, FLAG_PENDING_CSIP_DISCOVERY)) {
		if (err != 0) {
			LOG_ERR("Set coordinator discovery failed (err %d)", err);
			harc_free(harc);

			if (err == -ENOMEM) {
				err = -EAGAIN;
			}
		} else {
			harc->set_member = set_member;
			harc_set_member_init(harc);
		}

		notify_connected(harc, err);
	}
}

static void set_coordinator_set_locked_cb(int err)
{
	const struct bt_csip_set_coordinator_set_member *members[BT_HAP_HARC_SET_SIZE_MAX];

	LOG_DBG("err %d", err);

	if (!atomic_test_and_clear_bit(active_proc.flags, PROC_FLAG_PENDING_LOCK)) {
		/* Not initated by us */
		return;
	}

	if (err != 0) {
		LOG_ERR("Lock procedure failed (err %d)", err);
		harc_proc_complete(err);

		return;
	}

	atomic_set_bit(active_proc.flags, PROC_FLAG_LOCKED);

	for (size_t i = 0; i < active_proc.count; i++) {
		struct csip_set_member *set_member;
		struct bt_hap_harc *harc;

		harc = active_proc.harcs[i];
		__ASSERT_NO_MSG(harc != NULL);

		set_member = harc->set_member;
		__ASSERT_NO_MSG(harc != NULL);

		members[i] = set_member->member;
		__ASSERT_NO_MSG(members[i] != NULL);
	}

	bt_csip_set_coordinator_set_members_sort_by_rank(members, active_proc.count,
							 &active_proc.set_info, true);

	for (size_t i = 0; i < active_proc.count; i++) {
		active_proc.harcs[i] = harc_find_by_member(members[i]);
		__ASSERT_NO_MSG(active_proc.harcs[i] != NULL);
	}

	err = harc_proc_handler();
	if (err != 0) {
		LOG_ERR("Group procedure failed (err %d)", err);
	}
}

static void set_coordinator_set_released_cb(int err)
{
	LOG_DBG("err %d", err);

	if (!atomic_test_and_clear_bit(active_proc.flags, PROC_FLAG_PENDING_RELEASE)) {
		/* Not initated by us */
		return;
	}

	if (err == 0) {
		atomic_clear_bit(active_proc.flags, PROC_FLAG_LOCKED);
	}

	harc_proc_complete(err);
}

static void set_coordinator_lock_changed_cb(struct bt_csip_set_coordinator_csis_inst *inst,
					    bool locked)
{
	LOG_DBG("csis %p locked %d", inst, locked);
}

static void set_coordinator_ordered_access_cb(
	const struct bt_csip_set_coordinator_set_info *set_info, int err, bool locked,
	struct bt_csip_set_coordinator_set_member *member)
{
	LOG_DBG("member %p err %d locked %d", member, err, locked);
}

static struct bt_csip_set_coordinator_cb set_coordinator_cb = {
	.discover = set_coordinator_discover_cb,
	.lock_set = set_coordinator_set_locked_cb,
	.release_set = set_coordinator_set_released_cb,
	.lock_changed = set_coordinator_lock_changed_cb,
	.ordered_access = set_coordinator_ordered_access_cb,
};

static void conn_disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	struct csip_set_member *set_member;

	set_member = &set_members[bt_conn_index(conn)];
	memset(set_member, 0, sizeof(*set_member));
}

BT_CONN_CB_DEFINE(conn_cb) = {
	.disconnected = conn_disconnected_cb,
};

static int bt_hap_harc_init(void)
{
	int err;

	err = bt_has_client_init(&client_cb);
	if (err != 0 && err != -EALREADY) {
		LOG_ERR("Failed to initialize HAS Client (err %d)", err);
		return err;
	}

	err = bt_csip_set_coordinator_register_cb(&set_coordinator_cb);
	if (err != 0) {
		LOG_ERR("Failed to register CSIP Set Coordinator callbacks (err %d)", err);
		return err;
	}

	return 0;
}

int bt_hap_harc_cb_register(struct bt_hap_harc_cb *cb)
{
	static bool initialized;

	CHECKIF(cb == NULL) {
		LOG_ERR("cb is NULL");

		return -EINVAL;
	}

	sys_slist_append(&harc_cbs, &cb->_node);

	if (initialized) {
		return 0;
	}

	return bt_hap_harc_init();
}

int bt_hap_harc_cb_unregister(struct bt_hap_harc_cb *cb)
{
	CHECKIF(cb == NULL) {
		LOG_ERR("cb is NULL");

		return -EINVAL;
	}

	sys_slist_find_and_remove(&harc_cbs, &cb->_node);

	return 0;
}

int bt_hap_harc_bind(struct bt_conn *conn, struct bt_hap_harc **harc_out)
{
	struct bt_hap_harc *harc;
	uint8_t conn_index;
	int err;

	CHECKIF(conn == NULL) {
		LOG_DBG("NULL conn");
		return -EINVAL;
	}

	conn_index = bt_conn_index(conn);

	harc = &harc_insts[conn_index];
	if (harc->client != NULL) {
		*harc_out = harc;

		return -EALREADY;
	}

	memset(harc, 0, sizeof(*harc));
	harc->set_id = conn_index;

	err = bt_has_client_bind(conn, &harc->client);
	if (err == 0 || err == -EALREADY) {
		*harc_out = harc;
		err = 0;
	}

	return err;
}

int bt_hap_harc_unbind(struct bt_hap_harc *harc)
{
	CHECKIF(harc == NULL) {
		LOG_ERR("NULL harc");
		return -EINVAL;
	}

	if (harc->client == NULL) {
		return -ENODEV;
	}

	return bt_has_client_unbind(harc->client);
}

static struct bt_hap_harc *harc_find_pair(const struct bt_hap_harc *harc)
{
	for (size_t i = 0; i < ARRAY_SIZE(harc_insts); i++) {
		struct bt_hap_harc *pair = &harc_insts[i];

		if (pair != harc && pair->client != NULL && pair->set_id == harc->set_id) {
			return pair;
		}
	}

	return NULL;
}

int bt_hap_harc_info_get(const struct bt_hap_harc *harc, struct bt_hap_harc_info *out_info)
{
	struct bt_has_client_info info;
	int err;

	CHECKIF(harc == NULL) {
		LOG_DBG("NULL harc");
		return -EINVAL;
	}

	CHECKIF(out_info == NULL) {
		LOG_DBG("NULL info");
		return -EINVAL;
	}

	if (harc->client == NULL) {
		return -ENODEV;
	}

	err = bt_has_client_info_get(harc->client, &info);
	if (err != 0) {
		LOG_ERR("Failed to get info (err %d)", err);
		return err;
	}

	out_info->type = info.type;
	out_info->features = info.features;
	out_info->caps = info.caps;

	if (info.type == BT_HAS_HEARING_AID_TYPE_BINAURAL) {
		out_info->binaural.csis = harc->set_member->csis;
		out_info->binaural.member = harc->set_member->member;
		out_info->binaural.pair = harc_find_pair(harc);
	} else {
		out_info->binaural.csis = NULL;
		out_info->binaural.member = NULL;
		out_info->binaural.pair = NULL;
	}

	return 0;
}

int bt_hap_harc_conn_get(const struct bt_hap_harc *harc, struct bt_conn **conn)
{
	CHECKIF(harc == NULL) {
		LOG_DBG("NULL harc");
		return -EINVAL;
	}

	CHECKIF(conn == NULL) {
		LOG_DBG("NULL conn");
		return -EINVAL;
	}

	*conn = harc_conn_get(harc);
	if (*conn == NULL) {
		return -ENOTCONN;
	}

	return 0;
}

struct procedure_param {
	/* Hearing Aid Remote Controller instances */
	struct bt_hap_harc *harc[BT_HAP_HARC_SET_SIZE_MAX];
	/* Number of instances */
	uint8_t count;
	/* Procedure parametrs provided by user */
	void *params;
	/* Hearing Aid Type */
	enum bt_has_hearing_aid_type type;
	/* Common features */
	enum bt_has_features features;
	/* Set member that supports OOB preset synchronization */
	struct bt_hap_harc *syncable;
	/* true if all instances support lock */
	bool is_lockable;
	/* true if all instances are bonded */
	bool is_bonded;
	/* true if instances are in set */
	bool is_set;
};

static int procedure_prepare(struct procedure_param *param, struct bt_hap_harc **harcs,
			     uint8_t count)
{
	uint8_t set_id = count > 0;
	int err;

	CHECKIF(count == 0 || count > ARRAY_SIZE(param->harc)) {
		LOG_ERR("count out of bounds <%d, %zu>", 1, ARRAY_SIZE(param->harc));

		return -EINVAL;
	}

	param->features = 0xFF;
	param->is_lockable = true;
	param->is_bonded = true;
	param->is_set = true;
	param->count = 0;

	for (uint8_t i = 0; i < count; i++) {
		struct bt_has_client_info info = { 0 };
		struct bt_hap_harc *harc = harcs[i];

		CHECKIF(harc == NULL) {
			LOG_DBG("NULL harc");

			return -EINVAL;
		}

		if (harc->client == NULL) {
			return -ENODEV;
		}

		err = bt_has_client_info_get(harc->client, &info);
		__ASSERT_NO_MSG(err == 0);

		if (i > 0) {
			if (info.type != param->type) {
				LOG_ERR("Type mismatch");
				return -EINVAL;
			}

			param->is_set &= (harc->set_member != NULL && harc->set_id == set_id);
		} else {
			param->type = info.type;
			param->is_set = harc->set_member != NULL;

			set_id = harc->set_id;
		}

		if (harc->set_member != NULL && harc->set_member->csis != NULL) {
			param->is_lockable &= harc->set_member->csis->info.lockable;
		}

		param->features &= info.features;

		if (param->syncable == NULL &&
		    bt_has_features_check_preset_sync_supp(info.features) &&
		    !bt_has_features_check_preset_list_independent(info.features)) {
			param->syncable = harc;
		}

		param->is_bonded &= bt_addr_le_is_bonded(info.id, info.addr);
		param->harc[i] = harc;
		param->count++;
	}

	return 0;
}

static bool ordered_access_func(const struct bt_csip_set_coordinator_set_info *set_info,
				struct bt_csip_set_coordinator_set_member *members[], size_t count)
{
	int err;

	LOG_DBG("set_info %p members %p count %zu", set_info, members, count);

	if (!atomic_test_and_clear_bit(active_proc.flags, PROC_FLAG_PENDING_OAP)) {
		return false;
	}

	__ASSERT_NO_MSG(count == active_proc.count);

	/* Update the members order, as the list provided here is sorted by rank */
	for (size_t i = 0; i < count; i++) {
		active_proc.harcs[i] = harc_find_by_member(members[i]);
		__ASSERT_NO_MSG(active_proc.harcs[i] != NULL);
	}

	err = harc_proc_handler();
	if (err != 0) {
		LOG_ERR("Group procedure failed (err %d)", err);
		return false;
	}

	return true;
}

static int procedure_start(struct procedure_param *param, uint8_t opcode, void *user_data)
{
	const struct bt_csip_set_coordinator_set_member *members[BT_HAP_HARC_SET_SIZE_MAX];
	const struct bt_csip_set_coordinator_set_info *set_info = NULL;
	struct bt_hap_harc *harc;
	int err;

	if (atomic_get(active_proc.flags) != 0) {
		LOG_ERR("Another operation in progress");

		return -EBUSY;
	};

	active_proc.opcode = opcode;
	active_proc.user_data = user_data;

	for (uint8_t i = 0; i < param->count; i++) {
		harc = param->harc[i];

		if (param->is_set) {
			members[i] = harc->set_member->member;

			if (set_info == NULL) {
				set_info = &harc->set_member->csis->info;
			}
		}

		active_proc.harcs[i] = harc;

		atomic_set_bit(harc->flags, FLAG_PENDING_PROC);
	}

	active_proc.count = param->count;
	atomic_set_bit(active_proc.flags, PROC_FLAG_BUSY);

	if (param->is_set) {
		__ASSERT_NO_MSG(set_info != NULL);
		memcpy(&active_proc.set_info, set_info, sizeof(*set_info));

		if (param->is_bonded && param->is_lockable) {
			atomic_set_bit(active_proc.flags, PROC_FLAG_PENDING_LOCK);
			err = bt_csip_set_coordinator_lock(members, active_proc.count, set_info);
		} else {
			atomic_set_bit(active_proc.flags, PROC_FLAG_PENDING_OAP);
			err = bt_csip_set_coordinator_ordered_access(members, active_proc.count,
								     set_info, ordered_access_func);
		}
	} else {
		err = harc_proc_handler();
	}

	if (err != 0) {
		for (uint8_t i = 0; i < param->count; i++) {
			harc = param->harc[i];

			atomic_clear_bit(harc->flags, FLAG_PENDING_PROC);
		}

		atomic_clear(active_proc.flags);
	}

	return err;
}

int bt_hap_harc_preset_read(struct bt_hap_harc *harc,
			    struct bt_hap_harc_preset_read_params *read_params)
{
	struct procedure_param param = { 0 };
	int err;

	CHECKIF(harc == NULL) {
		LOG_ERR("NULL harc");
		return -EINVAL;
	}

	CHECKIF(read_params == NULL) {
		LOG_ERR("NULL params");
		return -EINVAL;
	}

	CHECKIF(preset_cb == NULL || preset_cb->store == NULL) {
		LOG_ERR("NULL callback");
		return -EINVAL;
	}

	err = procedure_prepare(&param, &harc, 1);
	if (err != 0) {
		LOG_ERR("Failed to prepare parameters (err %d)", err);

		return err;
	}

	return procedure_start(&param, BT_HAS_OP_READ_PRESET_REQ, read_params);
}

int bt_hap_harc_preset_get_active(struct bt_hap_harc *harc, uint8_t *index)
{
	struct bt_has_client_info info = { 0 };
	int err;

	CHECKIF(harc == NULL) {
		LOG_ERR("NULL harc");
		return -EINVAL;
	}

	CHECKIF(index == NULL) {
		LOG_ERR("NULL index");
		return -EINVAL;
	}

	if (harc->client == NULL) {
		return -ENODEV;
	}

	err = bt_has_client_info_get(harc->client, &info);
	__ASSERT_NO_MSG(err == 0);

	if ((info.caps & BT_HAS_PRESET_SUPPORT) == 0) {
		return -ENOTSUP;
	}

	*index = info.active_index;

	return 0;
}

static int harc_preset_set_common(struct bt_hap_harc **harcs, uint8_t count,
				  struct bt_hap_harc_preset_set_params *preset_set_params,
				  uint8_t opcode, uint8_t opcode_sync)
{
	struct procedure_param param = { 0 };
	int err;

	CHECKIF(preset_set_params == NULL) {
		LOG_ERR("NULL params");

		return -EINVAL;
	}

	err = procedure_prepare(&param, harcs, count);
	if (err != 0) {
		LOG_ERR("Failed to prepare parameters (err %d)", err);

		return err;
	}

	if (param.type == BT_HAS_HEARING_AID_TYPE_BINAURAL) {
		bool are_both_connected = count > 0;

		if (!param.is_set) {
			LOG_ERR("instances are not in set");
			return -EINVAL;
		}

		for (uint8_t i = 0; i < count; i++) {
			struct bt_hap_harc *harc = harcs[i];

			are_both_connected &= harc_conn_get(harc) != NULL;
		}

		/* HAP_v1.0 5.5.3 Write Preset Name procedure
		 *
		 * The HARC shall execute this procedure with both HAs that are part of a
		 * Binaural Hearing Aid Set if the Independent Presets flag is set to 0b0,
		 * and the HARC shall use the same parameter values during both executions.
		 */
		if (!bt_has_features_check_preset_list_independent(param.features) &&
		    !are_both_connected) {
			LOG_ERR("Procedure cannot be performed on single member");
			return -EACCES;
		}
	}

	/* Use OOB preset synchronization */
	if (param.syncable != NULL) {
		param.harc[0] = param.syncable;
		param.count = 1;

		return procedure_start(&param, opcode_sync, preset_set_params);
	}

	return procedure_start(&param, opcode, preset_set_params);
}

int bt_hap_harc_preset_set(struct bt_hap_harc **harcs, uint8_t count, uint8_t index,
			   struct bt_hap_harc_preset_set_params *params)
{
	params->reserved = UINT_TO_POINTER(index);

	if (preset_cb == NULL || preset_cb->get == NULL) {
		return -ECANCELED;
	}

	for (size_t i = 0; i < count; i++) {
		struct bt_hap_harc *harc = harcs[i];
		struct bt_has_preset_record record;
		int err;

		err = preset_cb->get(harc, index, &record);
		if (err != 0) {
			return err;
		}

		if ((record.properties & BT_HAS_PROP_AVAILABLE) == 0) {
			LOG_ERR("Preset is not available");
			return -EACCES;
		}
	}

	return harc_preset_set_common(harcs, count, params, BT_HAS_OP_SET_ACTIVE_PRESET,
				      BT_HAS_OP_SET_ACTIVE_PRESET_SYNC);
}

int bt_hap_harc_preset_set_next(struct bt_hap_harc **harcs, uint8_t count,
				struct bt_hap_harc_preset_set_params *params)
{
	return harc_preset_set_common(harcs, count, params, BT_HAS_OP_SET_NEXT_PRESET,
				      BT_HAS_OP_SET_NEXT_PRESET_SYNC);
}

int bt_hap_harc_preset_set_prev(struct bt_hap_harc **harcs, uint8_t count,
				struct bt_hap_harc_preset_set_params *params)
{
	return harc_preset_set_common(harcs, count, params, BT_HAS_OP_SET_PREV_PRESET,
				      BT_HAS_OP_SET_PREV_PRESET_SYNC);
}

int bt_hap_harc_preset_write(struct bt_hap_harc **harcs, uint8_t count,
			     struct bt_hap_harc_preset_write_params *write_params)
{
	struct procedure_param param = { 0 };
	int err;

	CHECKIF(write_params == NULL) {
		LOG_ERR("NULL params");

		return -EINVAL;
	}

	if (preset_cb == NULL || preset_cb->get == NULL) {
		return -ECANCELED;
	}

	for (size_t i = 0; i < count; i++) {
		struct bt_hap_harc *harc = harcs[i];
		struct bt_has_preset_record record;

		err = preset_cb->get(harc, write_params->index, &record);
		if (err != 0) {
			return err;
		}

		if ((record.properties & BT_HAS_PROP_WRITABLE) == 0) {
			LOG_ERR("Preset is not writable");
			return -EACCES;
		}
	}

	err = procedure_prepare(&param, harcs, count);
	if (err != 0) {
		LOG_ERR("Failed to prepare parameters (err %d)", err);

		return err;
	}

	if (param.type == BT_HAS_HEARING_AID_TYPE_BINAURAL) {
		bool are_both_connected = count > 0;

		if (!param.is_set) {
			LOG_ERR("instances are not in set");
			return -EINVAL;
		}

		for (uint8_t i = 0; i < count; i++) {
			struct bt_hap_harc *harc = harcs[i];

			are_both_connected &= harc_conn_get(harc) != NULL;
		}

		/* HAP_v1.0 5.5.3 Write Preset Name procedure
		 *
		 * The HARC shall execute this procedure with both HAs that are part of a
		 * Binaural Hearing Aid Set if the Independent Presets flag is set to 0b0,
		 * and the HARC shall use the same parameter values during both executions.
		 */
		if (!bt_has_features_check_preset_list_independent(param.features) &&
		    !are_both_connected) {
			LOG_ERR("Procedure cannot be performed on single member");
			return -EACCES;
		}
	}

	if (!bt_has_features_check_preset_write_supported(param.features)) {
		LOG_ERR("Non-writable presets");
		return -EROFS;
	}

	return procedure_start(&param, BT_HAS_OP_WRITE_PRESET_NAME, write_params);
}

int bt_hap_harc_preset_cb_register(const struct bt_hap_harc_preset_cb *cb)
{
	CHECKIF(cb == NULL) {
		LOG_DBG("NULL cb");

		return -EINVAL;
	}

	if (preset_cb != NULL) {
		return -EALREADY;
	}

	preset_cb = cb;

	return 0;
}
