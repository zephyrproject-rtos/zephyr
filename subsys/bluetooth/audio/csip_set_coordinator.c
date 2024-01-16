/* Bluetooth Coordinated Set Identification Client
 *
 * Copyright (c) 2020 Bose Corporation
 * Copyright (c) 2021-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * csip_set_coordinator should be used in the following way
 *  1) Find and connect to a set device
 *  2) Do discovery
 *  3) read values (always SIRK, size, lock and rank if possible)
 *  4) Discover other set members if applicable
 *  5) Connect and bond with each set member
 *  6) Do discovery of each member
 *  7) Read rank for each set member
 *  8) Lock all members based on rank if possible
 *  9) Do whatever is needed during lock
 * 10) Unlock all members
 */

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/check.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/audio/csip.h>
#include "csip_crypto.h"
#include "csip_internal.h"
#include "../host/conn_internal.h"
#include "../host/keys.h"
#include "common/bt_str.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_csip_set_coordinator, CONFIG_BT_CSIP_SET_COORDINATOR_LOG_LEVEL);

static uint8_t gatt_write_buf[1];
static struct bt_gatt_write_params write_params;
static struct bt_gatt_read_params read_params;
static struct bt_gatt_discover_params discover_params;
static struct bt_csip_set_coordinator_svc_inst *cur_inst;
static bool busy;

struct bt_csip_set_coordinator_svc_inst {
	uint8_t set_lock;

	uint16_t start_handle;
	uint16_t end_handle;
	uint16_t set_sirk_handle;
	uint16_t set_size_handle;
	uint16_t set_lock_handle;
	uint16_t rank_handle;

	uint8_t idx;
	struct bt_gatt_subscribe_params sirk_sub_params;
	struct bt_gatt_discover_params sirk_sub_disc_params;
	struct bt_gatt_subscribe_params size_sub_params;
	struct bt_gatt_discover_params size_sub_disc_params;
	struct bt_gatt_subscribe_params lock_sub_params;
	struct bt_gatt_discover_params lock_sub_disc_params;

	struct bt_conn *conn;
	struct bt_csip_set_coordinator_set_info *set_info;
};

static struct active_members {
	struct bt_csip_set_coordinator_set_member *members[CONFIG_BT_MAX_CONN];
	const struct bt_csip_set_coordinator_set_info *info;
	uint8_t members_count;
	uint8_t members_handled;
	uint8_t members_restored;

	bt_csip_set_coordinator_ordered_access_t oap_cb;
} active;

struct bt_csip_set_coordinator_inst {
	uint8_t inst_count;
	struct bt_csip_set_coordinator_svc_inst svc_insts
		[CONFIG_BT_CSIP_SET_COORDINATOR_MAX_CSIS_INSTANCES];
	struct bt_csip_set_coordinator_set_member set_member;
	struct bt_conn *conn;
};

static struct bt_uuid_16 uuid = BT_UUID_INIT_16(0);

static sys_slist_t csip_set_coordinator_cbs = SYS_SLIST_STATIC_INIT(&csip_set_coordinator_cbs);
static struct bt_csip_set_coordinator_inst client_insts[CONFIG_BT_MAX_CONN];

static int read_set_sirk(struct bt_csip_set_coordinator_svc_inst *svc_inst);
static int csip_set_coordinator_read_set_size(struct bt_conn *conn,
					      uint8_t inst_idx,
					      bt_gatt_read_func_t cb);
static int csip_set_coordinator_read_set_lock(struct bt_csip_set_coordinator_svc_inst *svc_inst);

static uint8_t csip_set_coordinator_discover_insts_read_set_sirk_cb(
	struct bt_conn *conn, uint8_t err, struct bt_gatt_read_params *params,
	const void *data, uint16_t length);
static void discover_insts_resume(struct bt_conn *conn, uint16_t sirk_handle,
				 uint16_t size_handle, uint16_t rank_handle);

static void active_members_reset(void)
{
	(void)memset(&active, 0, sizeof(active));
}

static struct bt_csip_set_coordinator_svc_inst *lookup_instance_by_handle(struct bt_conn *conn,
						 uint16_t handle)
{
	uint8_t conn_index;
	struct bt_csip_set_coordinator_inst *client;

	__ASSERT(conn, "NULL conn");
	__ASSERT(handle > 0, "Handle cannot be 0");

	conn_index = bt_conn_index(conn);
	client = &client_insts[conn_index];

	for (int i = 0; i < ARRAY_SIZE(client->svc_insts); i++) {
		if (client->svc_insts[i].start_handle <= handle &&
		    client->svc_insts[i].end_handle >= handle) {
			return &client->svc_insts[i];
		}
	}

	return NULL;
}

static struct bt_csip_set_coordinator_svc_inst *lookup_instance_by_index(const struct bt_conn *conn,
						uint8_t idx)
{
	uint8_t conn_index;
	struct bt_csip_set_coordinator_inst *client;

	__ASSERT(conn, "NULL conn");
	__ASSERT(idx < CONFIG_BT_CSIP_SET_COORDINATOR_MAX_CSIS_INSTANCES,
		 "Index shall be less than maximum number of instances %u (was %u)",
		 CONFIG_BT_CSIP_SET_COORDINATOR_MAX_CSIS_INSTANCES, idx);

	conn_index = bt_conn_index(conn);
	client = &client_insts[conn_index];
	return &client->svc_insts[idx];
}

static struct bt_csip_set_coordinator_svc_inst *lookup_instance_by_set_info(
	const struct bt_csip_set_coordinator_set_member *member,
	const struct bt_csip_set_coordinator_set_info *set_info)
{
	struct bt_csip_set_coordinator_inst *inst =
		CONTAINER_OF(member, struct bt_csip_set_coordinator_inst, set_member);

	for (int i = 0; i < ARRAY_SIZE(member->insts); i++) {
		const struct bt_csip_set_coordinator_set_info *member_set_info;

		member_set_info = &member->insts[i].info;
		if (member_set_info->set_size == set_info->set_size &&
		    memcmp(&member_set_info->set_sirk,
			   &set_info->set_sirk,
			   sizeof(set_info->set_sirk)) == 0) {
			return lookup_instance_by_index(inst->conn, i);
		}
	}

	return NULL;
}

static struct bt_csip_set_coordinator_svc_inst *get_next_active_instance(void)
{
	struct bt_csip_set_coordinator_set_member *member;
	struct bt_csip_set_coordinator_svc_inst *svc_inst;

	member = active.members[active.members_handled];

	svc_inst =  lookup_instance_by_set_info(member, active.info);
	if (svc_inst == NULL) {
		LOG_DBG("Failed to lookup instance by set_info %p", active.info);
	}

	return svc_inst;
}

static int member_rank_compare_asc(const void *m1, const void *m2)
{
	const struct bt_csip_set_coordinator_set_member *member_1 =
		*(const struct bt_csip_set_coordinator_set_member **)m1;
	const struct bt_csip_set_coordinator_set_member *member_2 =
		*(const struct bt_csip_set_coordinator_set_member **)m2;
	struct bt_csip_set_coordinator_svc_inst *svc_inst_1;
	struct bt_csip_set_coordinator_svc_inst *svc_inst_2;

	svc_inst_1 = lookup_instance_by_set_info(member_1, active.info);
	svc_inst_2 = lookup_instance_by_set_info(member_2, active.info);

	if (svc_inst_1 == NULL) {
		LOG_ERR("svc_inst_1 was NULL for member %p", member_1);
		return 0;
	}

	if (svc_inst_2 == NULL) {
		LOG_ERR("svc_inst_2 was NULL for member %p", member_2);
		return 0;
	}

	return svc_inst_1->set_info->rank - svc_inst_2->set_info->rank;
}

static int member_rank_compare_desc(const void *m1, const void *m2)
{
	/* If we call the "compare ascending" function with the members
	 * reversed, it will work as a descending comparison
	 */
	return member_rank_compare_asc(m2, m1);
}

static void active_members_store_ordered(const struct bt_csip_set_coordinator_set_member *members[],
					 size_t count,
					 const struct bt_csip_set_coordinator_set_info *info,
					 bool ascending)
{
	(void)memcpy(active.members, members, count * sizeof(members[0U]));
	active.members_count = count;
	active.info = info;

	if (count > 1U && CONFIG_BT_MAX_CONN > 1) {
		qsort(active.members, count, sizeof(members[0U]),
		ascending ? member_rank_compare_asc : member_rank_compare_desc);

#if defined(CONFIG_ASSERT)
		for (size_t i = 1U; i < count; i++) {
			const struct bt_csip_set_coordinator_svc_inst *svc_inst_1 =
				lookup_instance_by_set_info(active.members[i - 1U], info);
			const struct bt_csip_set_coordinator_svc_inst *svc_inst_2 =
				lookup_instance_by_set_info(active.members[i], info);
			const uint8_t rank_1 = svc_inst_1->set_info->rank;
			const uint8_t rank_2 = svc_inst_2->set_info->rank;

			if (ascending) {
				__ASSERT(rank_1 <= rank_2,
					 "Members not sorted by ascending rank %u - %u", rank_1,
					 rank_2);
			} else {
				__ASSERT(rank_1 >= rank_2,
					 "Members not sorted by descending rank %u - %u", rank_1,
					 rank_2);
			}
		}
#endif /* CONFIG_ASSERT */
	}
}

static int sirk_decrypt(struct bt_conn *conn,
			const uint8_t *enc_sirk,
			uint8_t *out_sirk)
{
	int err;
	uint8_t *k;

	if (IS_ENABLED(CONFIG_BT_CSIP_SET_COORDINATOR_TEST_SAMPLE_DATA)) {
		/* test_k is from the sample data from A.2 in the CSIS spec */
		static uint8_t test_k[] = {0x67, 0x6e, 0x1b, 0x9b,
					   0xd4, 0x48, 0x69, 0x6f,
					   0x06, 0x1e, 0xc6, 0x22,
					   0x3c, 0xe5, 0xce, 0xd9};
		static bool swapped;

		LOG_DBG("Decrypting with sample data K");

		if (!swapped && IS_ENABLED(CONFIG_LITTLE_ENDIAN)) {
			/* Swap test_k to little endian */
			sys_mem_swap(test_k, 16);
			swapped = true;
		}
		k = test_k;
	} else {
		k = conn->le.keys->ltk.val;
	}

	err = bt_csip_sdf(k, enc_sirk, out_sirk);

	return err;
}

static void lock_changed(struct bt_csip_set_coordinator_csis_inst *inst, bool locked)
{
	struct bt_csip_set_coordinator_cb *listener;

	active_members_reset();

	SYS_SLIST_FOR_EACH_CONTAINER(&csip_set_coordinator_cbs, listener, _node) {
		if (listener->lock_changed) {
			listener->lock_changed(inst, locked);
		}
	}
}

static void release_set_complete(int err)
{
	struct bt_csip_set_coordinator_cb *listener;

	active_members_reset();

	SYS_SLIST_FOR_EACH_CONTAINER(&csip_set_coordinator_cbs, listener, _node) {
		if (listener->release_set) {
			listener->release_set(err);
		}
	}
}

static void lock_set_complete(int err)
{
	struct bt_csip_set_coordinator_cb *listener;

	active_members_reset();

	SYS_SLIST_FOR_EACH_CONTAINER(&csip_set_coordinator_cbs, listener, _node) {
		if (listener->lock_set) {
			listener->lock_set(err);
		}
	}
}

static void ordered_access_complete(const struct bt_csip_set_coordinator_set_info *set_info,
				    int err, bool locked,
				    struct bt_csip_set_coordinator_set_member *member)
{

	struct bt_csip_set_coordinator_cb *listener;

	active_members_reset();

	SYS_SLIST_FOR_EACH_CONTAINER(&csip_set_coordinator_cbs, listener, _node) {
		if (listener->ordered_access) {
			listener->ordered_access(set_info, err, locked, member);
		}
	}
}

static void discover_complete(struct bt_csip_set_coordinator_inst *client,
			      int err)
{
	struct bt_csip_set_coordinator_cb *listener;

	cur_inst = NULL;
	busy = false;

	SYS_SLIST_FOR_EACH_CONTAINER(&csip_set_coordinator_cbs, listener, _node) {
		if (listener->discover) {
			if (err == 0) {
				listener->discover(client->conn,
						   &client->set_member,
						   err, client->inst_count);
			} else {
				listener->discover(client->conn, NULL, err, 0U);
			}
		}
	}
}

static uint8_t sirk_notify_func(struct bt_conn *conn,
				struct bt_gatt_subscribe_params *params,
				const void *data, uint16_t length)
{
	uint16_t handle = params->value_handle;
	struct bt_csip_set_coordinator_svc_inst *svc_inst;

	if (data == NULL) {
		LOG_DBG("[UNSUBSCRIBED] %u", params->value_handle);
		params->value_handle = 0U;

		return BT_GATT_ITER_STOP;
	}

	if (conn == NULL) {
		return BT_GATT_ITER_CONTINUE;
	}

	svc_inst = lookup_instance_by_handle(conn, handle);

	if (svc_inst != NULL) {
		LOG_DBG("Instance %u", svc_inst->idx);
		if (length == sizeof(struct bt_csip_set_sirk)) {
			struct bt_csip_set_sirk *sirk =
				(struct bt_csip_set_sirk *)data;
			struct bt_csip_set_coordinator_inst *client;
			uint8_t *dst_sirk;

			client = &client_insts[bt_conn_index(conn)];
			dst_sirk = client->set_member.insts[svc_inst->idx].info.set_sirk;

			LOG_DBG("Set SIRK %sencrypted",
				sirk->type == BT_CSIP_SIRK_TYPE_PLAIN ? "not " : "");

			/* Assuming not connected to other set devices */
			if (sirk->type == BT_CSIP_SIRK_TYPE_ENCRYPTED) {
				if (IS_ENABLED(CONFIG_BT_CSIP_SET_COORDINATOR_ENC_SIRK_SUPPORT)) {
					int err;

					LOG_HEXDUMP_DBG(sirk->value, sizeof(*sirk),
							"Encrypted Set SIRK");
					err = sirk_decrypt(conn, sirk->value,
							   dst_sirk);
					if (err != 0) {
						LOG_ERR("Could not decrypt "
							"SIRK %d",
							err);
					}
				} else {
					LOG_DBG("Encrypted SIRK not supported");
					return BT_GATT_ITER_CONTINUE;
				}
			} else {
				(void)memcpy(dst_sirk, sirk->value, sizeof(sirk->value));
			}

			LOG_HEXDUMP_DBG(dst_sirk, BT_CSIP_SET_SIRK_SIZE,
					"Set SIRK");

			/* TODO: Notify app */
		} else {
			LOG_DBG("Invalid length %u", length);
		}
	} else {
		LOG_DBG("Notification/Indication on unknown service inst");
	}

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t size_notify_func(struct bt_conn *conn,
				struct bt_gatt_subscribe_params *params,
				const void *data, uint16_t length)
{
	uint8_t set_size;
	uint16_t handle = params->value_handle;
	struct bt_csip_set_coordinator_svc_inst *svc_inst;

	if (data == NULL) {
		LOG_DBG("[UNSUBSCRIBED] %u", params->value_handle);
		params->value_handle = 0U;

		return BT_GATT_ITER_STOP;
	}

	if (conn == NULL) {
		return BT_GATT_ITER_CONTINUE;
	}

	svc_inst = lookup_instance_by_handle(conn, handle);

	if (svc_inst != NULL) {
		if (length == sizeof(set_size)) {
			struct bt_csip_set_coordinator_inst *client;
			struct bt_csip_set_coordinator_set_info *set_info;

			client = &client_insts[bt_conn_index(conn)];
			set_info = &client->set_member.insts[svc_inst->idx].info;

			(void)memcpy(&set_size, data, length);
			LOG_DBG("Set size updated from %u to %u", set_info->set_size, set_size);

			set_info->set_size = set_size;
			/* TODO: Notify app */
		} else {
			LOG_DBG("Invalid length %u", length);
		}

	} else {
		LOG_DBG("Notification/Indication on unknown service inst");
	}
	LOG_HEXDUMP_DBG(data, length, "Value");

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t lock_notify_func(struct bt_conn *conn,
				struct bt_gatt_subscribe_params *params,
				const void *data, uint16_t length)
{
	uint8_t value;
	uint16_t handle = params->value_handle;
	struct bt_csip_set_coordinator_svc_inst *svc_inst;

	if (data == NULL) {
		LOG_DBG("[UNSUBSCRIBED] %u", params->value_handle);
		params->value_handle = 0U;

		return BT_GATT_ITER_STOP;
	}

	if (conn == NULL) {
		return BT_GATT_ITER_CONTINUE;
	}

	svc_inst = lookup_instance_by_handle(conn, handle);

	if (svc_inst != NULL) {
		if (length == sizeof(svc_inst->set_lock)) {
			struct bt_csip_set_coordinator_inst *client;
			struct bt_csip_set_coordinator_csis_inst *inst;
			bool locked;

			(void)memcpy(&value, data, length);
			if (value != BT_CSIP_RELEASE_VALUE &&
			    value != BT_CSIP_LOCK_VALUE) {
				LOG_DBG("Invalid value %u", value);
				return BT_GATT_ITER_STOP;
			}

			(void)memcpy(&svc_inst->set_lock, data, length);

			locked = svc_inst->set_lock == BT_CSIP_LOCK_VALUE;
			LOG_DBG("Instance %u lock was %s", svc_inst->idx,
				locked ? "locked" : "released");

			client = &client_insts[bt_conn_index(conn)];
			inst = &client->set_member.insts[svc_inst->idx];

			lock_changed(inst, locked);
		} else {
			LOG_DBG("Invalid length %u", length);
		}
	} else {
		LOG_DBG("Notification/Indication on unknown service inst");
	}
	LOG_HEXDUMP_DBG(data, length, "Value");

	return BT_GATT_ITER_CONTINUE;
}

static int csip_set_coordinator_write_set_lock(struct bt_csip_set_coordinator_svc_inst *inst,
					       bool lock,
					       bt_gatt_write_func_t cb)
{
	if (inst->set_lock_handle == 0) {
		LOG_DBG("Handle not set");
		cur_inst = NULL;
		return -EINVAL;
	}

	/* Write to call control point */
	gatt_write_buf[0] = lock ? BT_CSIP_LOCK_VALUE : BT_CSIP_RELEASE_VALUE;
	write_params.data = gatt_write_buf;
	write_params.length = sizeof(lock);
	write_params.func = cb;
	write_params.handle = inst->set_lock_handle;

	return bt_gatt_write(inst->conn, &write_params);
}

static int read_set_sirk(struct bt_csip_set_coordinator_svc_inst *svc_inst)
{
	if (cur_inst != NULL) {
		if (cur_inst != svc_inst) {
			return -EBUSY;
		}
	} else {
		cur_inst = svc_inst;
	}

	if (svc_inst->set_sirk_handle == 0) {
		LOG_DBG("Handle not set");
		return -EINVAL;
	}

	read_params.func = csip_set_coordinator_discover_insts_read_set_sirk_cb;
	read_params.handle_count = 1;
	read_params.single.handle = svc_inst->set_sirk_handle;
	read_params.single.offset = 0U;

	return bt_gatt_read(svc_inst->conn, &read_params);
}

static int csip_set_coordinator_read_set_size(struct bt_conn *conn,
					      uint8_t inst_idx,
					      bt_gatt_read_func_t cb)
{
	if (inst_idx >= CONFIG_BT_CSIP_SET_COORDINATOR_MAX_CSIS_INSTANCES) {
		return -EINVAL;
	} else if (cur_inst != NULL) {
		if (cur_inst != lookup_instance_by_index(conn, inst_idx)) {
			return -EBUSY;
		}
	} else {
		cur_inst = lookup_instance_by_index(conn, inst_idx);
		if (cur_inst == NULL) {
			LOG_DBG("Inst not found");
			return -EINVAL;
		}
	}

	if (cur_inst->set_size_handle == 0) {
		LOG_DBG("Handle not set");
		cur_inst = NULL;
		return -EINVAL;
	}

	read_params.func = cb;
	read_params.handle_count = 1;
	read_params.single.handle = cur_inst->set_size_handle;
	read_params.single.offset = 0U;

	return bt_gatt_read(conn, &read_params);
}

static int csip_set_coordinator_read_rank(struct bt_conn *conn,
					  uint8_t inst_idx,
					  bt_gatt_read_func_t cb)
{
	if (inst_idx >= CONFIG_BT_CSIP_SET_COORDINATOR_MAX_CSIS_INSTANCES) {
		return -EINVAL;
	} else if (cur_inst != NULL) {
		if (cur_inst != lookup_instance_by_index(conn, inst_idx)) {
			return -EBUSY;
		}
	} else {
		cur_inst = lookup_instance_by_index(conn, inst_idx);
		if (cur_inst == NULL) {
			LOG_DBG("Inst not found");
			return -EINVAL;
		}
	}

	if (cur_inst->rank_handle == 0) {
		LOG_DBG("Handle not set");
		cur_inst = NULL;
		return -EINVAL;
	}

	read_params.func = cb;
	read_params.handle_count = 1;
	read_params.single.handle = cur_inst->rank_handle;
	read_params.single.offset = 0U;

	return bt_gatt_read(conn, &read_params);
}

static int csip_set_coordinator_discover_sets(struct bt_csip_set_coordinator_set_member *member)
{
	int err;

	/* Start reading values and call CB when done */
	err = read_set_sirk((struct bt_csip_set_coordinator_svc_inst *)member->insts[0].svc_inst);
	if (err == 0) {
		busy = true;
	}

	return err;
}

static uint8_t discover_func(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	struct bt_gatt_chrc *chrc;
	struct bt_csip_set_coordinator_inst *client = &client_insts[bt_conn_index(conn)];
	struct bt_gatt_subscribe_params *sub_params = NULL;
	void *notify_handler = NULL;

	if (attr == NULL) {
		LOG_DBG("Setup complete for %u / %u", cur_inst->idx + 1, client->inst_count);
		(void)memset(params, 0, sizeof(*params));

		if (CONFIG_BT_CSIP_SET_COORDINATOR_MAX_CSIS_INSTANCES > 1 &&
		    (cur_inst->idx + 1) < client->inst_count) {
			int err;

			cur_inst = &client->svc_insts[cur_inst->idx + 1];
			discover_params.uuid = NULL;
			discover_params.start_handle = cur_inst->start_handle;
			discover_params.end_handle = cur_inst->end_handle;
			discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
			discover_params.func = discover_func;

			err = bt_gatt_discover(conn, &discover_params);
			if (err != 0) {
				LOG_DBG("Discover failed (err %d)", err);
				discover_complete(client, err);
			}

		} else {
			int err;

			cur_inst = NULL;
			busy = false;
			err = csip_set_coordinator_discover_sets(&client->set_member);
			if (err != 0) {
				LOG_DBG("Discover sets failed (err %d)", err);
				discover_complete(client, err);
			}
		}
		return BT_GATT_ITER_STOP;
	}

	LOG_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC &&
	    client->inst_count != 0) {
		chrc = (struct bt_gatt_chrc *)attr->user_data;
		if (bt_uuid_cmp(chrc->uuid, BT_UUID_CSIS_SET_SIRK) == 0) {
			LOG_DBG("Set SIRK");
			cur_inst->set_sirk_handle = chrc->value_handle;
			sub_params = &cur_inst->sirk_sub_params;
			sub_params->disc_params = &cur_inst->sirk_sub_disc_params;
			notify_handler = sirk_notify_func;
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_CSIS_SET_SIZE) == 0) {
			LOG_DBG("Set size");
			cur_inst->set_size_handle = chrc->value_handle;
			sub_params = &cur_inst->size_sub_params;
			sub_params->disc_params = &cur_inst->size_sub_disc_params;
			notify_handler = size_notify_func;
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_CSIS_SET_LOCK) == 0) {
			struct bt_csip_set_coordinator_set_info *set_info;

			LOG_DBG("Set lock");
			cur_inst->set_lock_handle = chrc->value_handle;
			sub_params = &cur_inst->lock_sub_params;
			sub_params->disc_params = &cur_inst->lock_sub_disc_params;
			notify_handler = lock_notify_func;

			set_info = &client->set_member.insts[cur_inst->idx].info;
			set_info->lockable = true;
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_CSIS_RANK) == 0) {
			LOG_DBG("Set rank");
			cur_inst->rank_handle = chrc->value_handle;
		}

		if (sub_params != NULL && notify_handler != NULL) {
			sub_params->value = 0;
			if ((chrc->properties & BT_GATT_CHRC_NOTIFY) != 0) {
				sub_params->value = BT_GATT_CCC_NOTIFY;
			} else if ((chrc->properties & BT_GATT_CHRC_INDICATE) != 0) {
				sub_params->value = BT_GATT_CCC_INDICATE;
			}

			if (sub_params->value != 0) {
				int err;

				/* With ccc_handle == 0 it will use auto discovery */
				sub_params->ccc_handle = 0;
				sub_params->end_handle = cur_inst->end_handle;
				sub_params->value_handle = chrc->value_handle;
				sub_params->notify = notify_handler;
				atomic_set_bit(sub_params->flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

				err = bt_gatt_subscribe(conn, sub_params);
				if (err != 0 && err != -EALREADY) {
					LOG_DBG("Failed to subscribe (err %d)", err);
					discover_complete(client, err);

					return BT_GATT_ITER_STOP;
				}
			}
		}
	}

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t primary_discover_func(struct bt_conn *conn,
				     const struct bt_gatt_attr *attr,
				     struct bt_gatt_discover_params *params)
{
	struct bt_gatt_service_val *prim_service;
	struct bt_csip_set_coordinator_inst *client = &client_insts[bt_conn_index(conn)];

	if (attr == NULL ||
	    client->inst_count == CONFIG_BT_CSIP_SET_COORDINATOR_MAX_CSIS_INSTANCES) {
		LOG_DBG("Discover complete, found %u instances", client->inst_count);
		(void)memset(params, 0, sizeof(*params));

		if (client->inst_count != 0) {
			int err;

			cur_inst = &client->svc_insts[0];
			discover_params.uuid = NULL;
			discover_params.start_handle = cur_inst->start_handle;
			discover_params.end_handle = cur_inst->end_handle;
			discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
			discover_params.func = discover_func;

			err = bt_gatt_discover(conn, &discover_params);
			if (err != 0) {
				LOG_DBG("Discover failed (err %d)", err);
				discover_complete(client, err);
			}
		} else {
			discover_complete(client, 0);
		}

		return BT_GATT_ITER_STOP;
	}

	LOG_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_PRIMARY) {
		prim_service = (struct bt_gatt_service_val *)attr->user_data;
		discover_params.start_handle = attr->handle + 1;

		cur_inst = &client->svc_insts[client->inst_count];
		cur_inst->idx = client->inst_count;
		cur_inst->start_handle = attr->handle;
		cur_inst->end_handle = prim_service->end_handle;
		cur_inst->conn = bt_conn_ref(conn);
		cur_inst->set_info = &client->set_member.insts[cur_inst->idx].info;
		client->inst_count++;
	}

	return BT_GATT_ITER_CONTINUE;
}

bool bt_csip_set_coordinator_is_set_member(const uint8_t set_sirk[BT_CSIP_SET_SIRK_SIZE],
				  struct bt_data *data)
{
	if (data->type == BT_DATA_CSIS_RSI &&
	    data->data_len == BT_CSIP_RSI_SIZE) {
		uint8_t err;
		uint8_t hash[BT_CSIP_CRYPTO_HASH_SIZE];
		uint8_t prand[BT_CSIP_CRYPTO_PRAND_SIZE];
		uint8_t calculated_hash[BT_CSIP_CRYPTO_HASH_SIZE];

		memcpy(hash, data->data, BT_CSIP_CRYPTO_HASH_SIZE);
		memcpy(prand, data->data + BT_CSIP_CRYPTO_HASH_SIZE, BT_CSIP_CRYPTO_PRAND_SIZE);

		LOG_DBG("hash: %s", bt_hex(hash, BT_CSIP_CRYPTO_HASH_SIZE));
		LOG_DBG("prand %s", bt_hex(prand, BT_CSIP_CRYPTO_PRAND_SIZE));
		err = bt_csip_sih(set_sirk, prand, calculated_hash);
		if (err != 0) {
			return false;
		}

		LOG_DBG("calculated_hash: %s", bt_hex(calculated_hash, BT_CSIP_CRYPTO_HASH_SIZE));
		LOG_DBG("hash: %s", bt_hex(hash, BT_CSIP_CRYPTO_HASH_SIZE));

		return memcmp(calculated_hash, hash, BT_CSIP_CRYPTO_HASH_SIZE) == 0;
	}

	return false;
}

static uint8_t csip_set_coordinator_discover_insts_read_rank_cb(struct bt_conn *conn,
								uint8_t err,
								struct bt_gatt_read_params *params,
								const void *data,
								uint16_t length)
{
	struct bt_csip_set_coordinator_inst *client = &client_insts[bt_conn_index(conn)];

	__ASSERT(cur_inst != NULL, "cur_inst must not be NULL");

	busy = false;

	if (err != 0) {
		LOG_DBG("err: 0x%02X", err);

		discover_complete(client, err);
	} else if (data != NULL) {
		struct bt_csip_set_coordinator_set_info *set_info;

		LOG_HEXDUMP_DBG(data, length, "Data read");

		set_info = &client->set_member.insts[cur_inst->idx].info;

		if (length == sizeof(set_info->rank)) {
			(void)memcpy(&set_info->rank, data, length);
			LOG_DBG("%u", set_info->rank);
		} else {
			LOG_DBG("Invalid length, continuing to next member");
		}

		discover_insts_resume(conn, 0, 0, 0);
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t csip_set_coordinator_discover_insts_read_set_size_cb(
	struct bt_conn *conn, uint8_t err, struct bt_gatt_read_params *params,
	const void *data, uint16_t length)
{
	struct bt_csip_set_coordinator_inst *client = &client_insts[bt_conn_index(conn)];

	__ASSERT(cur_inst != NULL, "cur_inst must not be NULL");

	busy = false;

	if (err != 0) {
		LOG_DBG("err: 0x%02X", err);

		discover_complete(client, err);
	} else if (data != NULL) {
		struct bt_csip_set_coordinator_set_info *set_info;

		LOG_HEXDUMP_DBG(data, length, "Data read");

		set_info = &client->set_member.insts[cur_inst->idx].info;

		if (length == sizeof(set_info->set_size)) {
			(void)memcpy(&set_info->set_size, data, length);
			LOG_DBG("%u", set_info->set_size);
		} else {
			LOG_DBG("Invalid length");
		}

		discover_insts_resume(conn, 0, 0, cur_inst->rank_handle);
	}

	return BT_GATT_ITER_STOP;
}

static int parse_sirk(struct bt_csip_set_coordinator_inst *client,
		      const void *data, uint16_t length)
{
	uint8_t *set_sirk;

	set_sirk = client->set_member.insts[cur_inst->idx].info.set_sirk;

	if (length == sizeof(struct bt_csip_set_sirk)) {
		struct bt_csip_set_sirk *sirk =
			(struct bt_csip_set_sirk *)data;

		LOG_DBG("Set SIRK %sencrypted",
			sirk->type == BT_CSIP_SIRK_TYPE_PLAIN ? "not " : "");
		/* Assuming not connected to other set devices */
		if (sirk->type == BT_CSIP_SIRK_TYPE_ENCRYPTED) {
			if (IS_ENABLED(CONFIG_BT_CSIP_SET_COORDINATOR_ENC_SIRK_SUPPORT)) {
				int err;

				LOG_HEXDUMP_DBG(sirk->value, sizeof(sirk->value),
						"Encrypted Set SIRK");
				err = sirk_decrypt(client->conn, sirk->value,
						   set_sirk);
				if (err != 0) {
					LOG_ERR("Could not decrypt "
						"SIRK %d",
						err);
					return err;
				}
			} else {
				LOG_WRN("Encrypted SIRK not supported");
				set_sirk = NULL;
				return BT_ATT_ERR_INSUFFICIENT_ENCRYPTION;
			}
		} else {
			(void)memcpy(set_sirk, sirk->value, sizeof(sirk->value));
		}

		if (set_sirk != NULL) {
			LOG_HEXDUMP_DBG(set_sirk, BT_CSIP_SET_SIRK_SIZE,
					"Set SIRK");
		}
	} else {
		LOG_DBG("Invalid length");
		return BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
	}

	return 0;
}

static uint8_t csip_set_coordinator_discover_insts_read_set_sirk_cb(
	struct bt_conn *conn, uint8_t err, struct bt_gatt_read_params *params,
	const void *data, uint16_t length)
{
	struct bt_csip_set_coordinator_inst *client = &client_insts[bt_conn_index(conn)];
	int cb_err = err;
	__ASSERT(cur_inst != NULL, "cur_inst must not be NULL");

	busy = false;

	if (err != 0) {
		LOG_DBG("err: 0x%02X", err);

		discover_complete(client, err);
	} else if (data != NULL) {
		LOG_HEXDUMP_DBG(data, length, "Data read");

		cb_err = parse_sirk(client, data, length);

		if (cb_err != 0) {
			LOG_DBG("Could not parse SIRK: %d", cb_err);
		} else {
			discover_insts_resume(conn, 0,
					     cur_inst->set_size_handle,
					     cur_inst->rank_handle);
		}
	}

	return BT_GATT_ITER_STOP;
}

/**
 * @brief Reads the (next) characteristics for the set discovery procedure
 *
 * It skips all handles that are 0.
 *
 * @param conn        Connection to a CSIP set member device.
 * @param sirk_handle 0, or the handle for the SIRK characteristic.
 * @param size_handle 0, or the handle for the size characteristic.
 * @param rank_handle 0, or the handle for the rank characteristic.
 */
static void discover_insts_resume(struct bt_conn *conn, uint16_t sirk_handle,
				 uint16_t size_handle, uint16_t rank_handle)
{
	int cb_err = 0;
	struct bt_csip_set_coordinator_inst *client = &client_insts[bt_conn_index(conn)];

	if (size_handle != 0) {
		cb_err = csip_set_coordinator_read_set_size(
				conn, cur_inst->idx,
				csip_set_coordinator_discover_insts_read_set_size_cb);
		if (cb_err != 0) {
			LOG_DBG("Could not read set size: %d", cb_err);
		}
	} else if (rank_handle != 0) {
		cb_err = csip_set_coordinator_read_rank(
				conn, cur_inst->idx,
				csip_set_coordinator_discover_insts_read_rank_cb);
		if (cb_err != 0) {
			LOG_DBG("Could not read set rank: %d", cb_err);
		}
	} else {
		uint8_t next_idx = cur_inst->idx + 1;

		cur_inst = NULL;
		if (next_idx < client->inst_count) {
			cur_inst = lookup_instance_by_index(conn, next_idx);

			/* Read next */
			cb_err = read_set_sirk(cur_inst);
		} else {
			discover_complete(client, 0);

			return;
		}
	}

	if (cb_err != 0) {
		discover_complete(client, cb_err);
	} else {
		busy = true;
	}
}

static void csip_set_coordinator_write_restore_cb(struct bt_conn *conn,
						  uint8_t err,
						  struct bt_gatt_write_params *params)
{
	busy = false;

	if (err != 0) {
		LOG_WRN("Could not restore (%d)", err);
		release_set_complete(err);

		return;
	}

	active.members_restored++;
	LOG_DBG("Restored %u/%u members", active.members_restored, active.members_handled);

	if (active.members_restored < active.members_handled &&
	    CONFIG_BT_MAX_CONN > 1) {
		struct bt_csip_set_coordinator_set_member *member;
		int csip_err;

		member = active.members[active.members_handled - active.members_restored - 1];
		cur_inst = lookup_instance_by_set_info(member, active.info);
		if (cur_inst == NULL) {
			release_set_complete(-ENOENT);

			return;
		}

		csip_err = csip_set_coordinator_write_set_lock(
				cur_inst, false,
				csip_set_coordinator_write_restore_cb);
		if (csip_err == 0) {
			busy = true;
		} else {
			LOG_DBG("Failed to release next member[%u]: %d", active.members_handled,
				csip_err);

			release_set_complete(csip_err);
		}
	} else {
		release_set_complete(0);
	}
}

static void csip_set_coordinator_write_lock_cb(struct bt_conn *conn,
					       uint8_t err,
					       struct bt_gatt_write_params *params)
{
	busy = false;

	if (err != 0) {
		LOG_DBG("Could not lock (0x%X)", err);
		if (active.members_handled > 0 && CONFIG_BT_MAX_CONN > 1) {
			struct bt_csip_set_coordinator_set_member *member;
			int csip_err;

			active.members_restored = 0;

			member = active.members[active.members_handled - active.members_restored];
			cur_inst = lookup_instance_by_set_info(member,
							       active.info);
			if (cur_inst == NULL) {
				LOG_DBG("Failed to lookup instance by set_info %p", active.info);

				lock_set_complete(-ENOENT);
				return;
			}

			csip_err = csip_set_coordinator_write_set_lock(
					cur_inst, false,
					csip_set_coordinator_write_restore_cb);
			if (csip_err == 0) {
				busy = true;
			} else {
				LOG_WRN("Could not release lock of previous locked member: %d",
					csip_err);
				active_members_reset();
				return;
			}
		}

		lock_set_complete(err);

		return;
	}

	active.members_handled++;
	LOG_DBG("Locked %u/%u members", active.members_handled, active.members_count);

	if (active.members_handled < active.members_count) {
		struct bt_csip_set_coordinator_svc_inst *prev_inst = cur_inst;
		int csip_err;

		cur_inst = get_next_active_instance();
		if (cur_inst == NULL) {
			lock_set_complete(-ENOENT);

			return;
		}

		csip_err = csip_set_coordinator_write_set_lock(
				cur_inst, true,
				csip_set_coordinator_write_lock_cb);
		if (csip_err == 0) {
			busy = true;
		} else {
			LOG_DBG("Failed to lock next member[%u]: %d", active.members_handled,
				csip_err);

			active.members_restored = 0;

			csip_err = csip_set_coordinator_write_set_lock(
					prev_inst, false,
					csip_set_coordinator_write_restore_cb);
			if (csip_err == 0) {
				busy = true;
			} else {
				LOG_WRN("Could not release lock of previous locked member: %d",
					csip_err);
				active_members_reset();
				return;
			}
		}
	} else {
		lock_set_complete(0);
	}
}

static void csip_set_coordinator_write_release_cb(struct bt_conn *conn, uint8_t err,
						  struct bt_gatt_write_params *params)
{
	busy = false;

	if (err != 0) {
		LOG_DBG("Could not release lock (%d)", err);
		release_set_complete(err);

		return;
	}

	active.members_handled++;
	LOG_DBG("Released %u/%u members", active.members_handled, active.members_count);

	if (active.members_handled < active.members_count) {
		int csip_err;

		cur_inst = get_next_active_instance();
		if (cur_inst == NULL) {
			release_set_complete(-ENOENT);

			return;
		}

		csip_err = csip_set_coordinator_write_set_lock(
				cur_inst, false,
				csip_set_coordinator_write_release_cb);
		if (csip_err == 0) {
			busy = true;
		} else {
			LOG_DBG("Failed to release next member[%u]: %d", active.members_handled,
				csip_err);

			release_set_complete(csip_err);
		}
	} else {
		release_set_complete(0);
	}
}

static void csip_set_coordinator_lock_state_read_cb(int err, bool locked)
{
	const struct bt_csip_set_coordinator_set_info *info = active.info;
	struct bt_csip_set_coordinator_set_member *cur_member = NULL;

	if (err || locked) {
		cur_member = active.members[active.members_handled];
	} else if (active.oap_cb == NULL || !active.oap_cb(info, active.members,
		   active.members_count)) {
		err = -ECANCELED;
	}

	ordered_access_complete(info, err, locked, cur_member);
}

static uint8_t csip_set_coordinator_read_lock_cb(struct bt_conn *conn,
						 uint8_t err,
						 struct bt_gatt_read_params *params,
						 const void *data,
						 uint16_t length)
{
	uint8_t value = 0;

	busy = false;

	if (err != 0) {
		LOG_DBG("Could not read lock value (0x%X)", err);

		csip_set_coordinator_lock_state_read_cb(err, false);

		return BT_GATT_ITER_STOP;
	}

	active.members_handled++;
	LOG_DBG("Read lock state on %u/%u members", active.members_handled, active.members_count);

	if (data == NULL || length != sizeof(cur_inst->set_lock)) {
		LOG_DBG("Invalid data %p or length %u", data, length);

		csip_set_coordinator_lock_state_read_cb(err, false);

		return BT_GATT_ITER_STOP;
	}

	value = ((uint8_t *)data)[0];
	if (value != BT_CSIP_RELEASE_VALUE && value != BT_CSIP_LOCK_VALUE) {
		LOG_DBG("Invalid value %u read", value);
		err = BT_ATT_ERR_UNLIKELY;

		csip_set_coordinator_lock_state_read_cb(err, false);

		return BT_GATT_ITER_STOP;
	}

	cur_inst->set_lock = value;

	if (value != BT_CSIP_RELEASE_VALUE) {
		LOG_DBG("Set member not unlocked");

		csip_set_coordinator_lock_state_read_cb(0, true);

		return BT_GATT_ITER_STOP;
	}

	if (active.members_handled < active.members_count) {
		int csip_err;

		cur_inst = get_next_active_instance();
		if (cur_inst == NULL) {
			csip_set_coordinator_lock_state_read_cb(-ENOENT, false);

			return BT_GATT_ITER_STOP;
		}

		csip_err = csip_set_coordinator_read_set_lock(cur_inst);
		if (csip_err == 0) {
			busy = true;
		} else {
			LOG_DBG("Failed to read next member[%u]: %d", active.members_handled,
				csip_err);

			csip_set_coordinator_lock_state_read_cb(err, false);
		}
	} else {
		csip_set_coordinator_lock_state_read_cb(0, false);
	}

	return BT_GATT_ITER_STOP;
}

static int csip_set_coordinator_read_set_lock(struct bt_csip_set_coordinator_svc_inst *inst)
{
	if (inst->set_lock_handle == 0) {
		LOG_DBG("Handle not set");
		cur_inst = NULL;
		return -EINVAL;
	}

	read_params.func = csip_set_coordinator_read_lock_cb;
	read_params.handle_count = 1;
	read_params.single.handle = inst->set_lock_handle;
	read_params.single.offset = 0;

	return bt_gatt_read(inst->conn, &read_params);
}

static void csip_set_coordinator_reset(struct bt_csip_set_coordinator_inst *inst)
{
	inst->inst_count = 0U;
	memset(&inst->set_member, 0, sizeof(inst->set_member));

	for (size_t i = 0; i < ARRAY_SIZE(inst->svc_insts); i++) {
		struct bt_csip_set_coordinator_svc_inst *svc_inst = &inst->svc_insts[i];

		svc_inst->idx = 0;
		svc_inst->set_lock = 0;
		svc_inst->start_handle = 0;
		svc_inst->end_handle = 0;
		svc_inst->set_sirk_handle = 0;
		svc_inst->set_size_handle = 0;
		svc_inst->set_lock_handle = 0;
		svc_inst->rank_handle = 0;

		if (svc_inst->conn != NULL) {
			struct bt_conn *conn = svc_inst->conn;

			bt_conn_unref(conn);
			svc_inst->conn = NULL;
		}

		if (svc_inst->set_info != NULL) {
			memset(svc_inst->set_info, 0, sizeof(*svc_inst->set_info));
			svc_inst->set_info = NULL;
		}
	}

	if (inst->conn) {
		bt_conn_unref(inst->conn);
		inst->conn = NULL;
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct bt_csip_set_coordinator_inst *inst = &client_insts[bt_conn_index(conn)];

	if (inst->conn == conn) {
		csip_set_coordinator_reset(inst);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.disconnected = disconnected,
};

struct bt_csip_set_coordinator_csis_inst *bt_csip_set_coordinator_csis_inst_by_handle(
	struct bt_conn *conn, uint16_t start_handle)
{
	const struct bt_csip_set_coordinator_svc_inst *svc_inst;

	CHECKIF(conn == NULL) {
		LOG_DBG("conn is NULL");

		return NULL;
	}

	CHECKIF(start_handle == 0) {
		LOG_DBG("start_handle is 0");

		return NULL;
	}

	svc_inst = lookup_instance_by_handle(conn, start_handle);

	if (svc_inst != NULL) {
		struct bt_csip_set_coordinator_inst *client;

		client = &client_insts[bt_conn_index(conn)];

		return &client->set_member.insts[svc_inst->idx];
	}

	return NULL;
}

/*************************** PUBLIC FUNCTIONS ***************************/
int bt_csip_set_coordinator_register_cb(struct bt_csip_set_coordinator_cb *cb)
{
	CHECKIF(cb == NULL) {
		LOG_DBG("cb is NULL");

		return -EINVAL;
	}

	sys_slist_append(&csip_set_coordinator_cbs, &cb->_node);

	return 0;
}

int bt_csip_set_coordinator_discover(struct bt_conn *conn)
{
	int err;
	struct bt_csip_set_coordinator_inst *client;

	CHECKIF(conn == NULL) {
		LOG_DBG("NULL conn");
		return -EINVAL;
	}

	if (busy) {
		return -EBUSY;
	}

	client = &client_insts[bt_conn_index(conn)];

	csip_set_coordinator_reset(client);

	/* Discover CSIS on peer, setup handles and notify */
	(void)memset(&discover_params, 0, sizeof(discover_params));
	(void)memcpy(&uuid, BT_UUID_CSIS, sizeof(uuid));
	discover_params.func = primary_discover_func;
	discover_params.uuid = &uuid.uuid;
	discover_params.type = BT_GATT_DISCOVER_PRIMARY;
	discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;

	err = bt_gatt_discover(conn, &discover_params);
	if (err == 0) {
		for (size_t i = 0; i < ARRAY_SIZE(client->set_member.insts); i++) {
			client->set_member.insts[i].svc_inst = (void *)&client->svc_insts[i];
		}
		busy = true;
		client->conn = bt_conn_ref(conn);
	}

	return err;
}

static int verify_members(const struct bt_csip_set_coordinator_set_member **members,
			  uint8_t count,
			  const struct bt_csip_set_coordinator_set_info *set_info)
{
	bool zero_rank;
	uint8_t ranks[CONFIG_BT_MAX_CONN];

	if (count > CONFIG_BT_MAX_CONN) {
		LOG_DBG("count (%u) larger than maximum support servers (%d)", count,
			CONFIG_BT_MAX_CONN);
		return -EINVAL;
	}

	zero_rank = false;
	for (int i = 0; i < count; i++) {
		const struct bt_csip_set_coordinator_set_member *member = members[i];
		struct bt_csip_set_coordinator_inst *client_inst =
			CONTAINER_OF(member, struct bt_csip_set_coordinator_inst, set_member);
		struct bt_csip_set_coordinator_svc_inst *svc_inst;
		struct bt_conn *conn;

		CHECKIF(member == NULL) {
			LOG_DBG("Invalid member[%d] was NULL", i);
			return -EINVAL;
		}

		conn = client_inst->conn;

		CHECKIF(conn == NULL) {
			LOG_DBG("Member[%d] conn was NULL", i);
			return -EINVAL;
		}

		if (conn->state != BT_CONN_CONNECTED) {
			LOG_DBG("Member[%d] was not connected", i);
			return -ENOTCONN;
		}

		svc_inst = lookup_instance_by_set_info(member, set_info);
		if (svc_inst == NULL) {
			LOG_DBG("Member[%d] could not find matching instance for the set_info", i);
			return -EINVAL;
		}

		ranks[i] = svc_inst->set_info->rank;
		if (ranks[i] == 0U && !zero_rank) {
			zero_rank = true;
		} else if (ranks[i] != 0 && zero_rank) {
			/* all members in a set shall either use rank, or not use rank */
			LOG_DBG("Found mix of 0 and non-0 ranks");
			return -EINVAL;
		}
	}

	if (CONFIG_BT_MAX_CONN > 1 && !zero_rank && count > 1U) {
		/* Search for duplicate ranks */
		for (uint8_t i = 0U; i < count - 1; i++) {
			for (uint8_t j = i + 1; j < count; j++) {
				if (ranks[j] == ranks[i]) {
					/* duplicate rank */
					LOG_DBG("Duplicate rank (%u) for members[%zu] "
						"and members[%zu]",
						ranks[i], i, j);
					return -EINVAL;
				}
			}
		}
	}

	return 0;
}

static int bt_csip_set_coordinator_get_lock_state(
	const struct bt_csip_set_coordinator_set_member **members,
	uint8_t count,
	const struct bt_csip_set_coordinator_set_info *set_info)
{
	int err;

	if (busy) {
		LOG_DBG("csip_set_coordinator busy");
		return -EBUSY;
	}

	err = verify_members(members, count, set_info);
	if (err != 0) {
		LOG_DBG("Could not verify members: %d", err);
		return err;
	}

	active_members_store_ordered(members, count, set_info, true);

	for (uint8_t i = 0U; i < count; i++) {
		cur_inst = lookup_instance_by_set_info(active.members[i], active.info);
		if (cur_inst == NULL) {
			LOG_DBG("Failed to lookup instance by set_info %p", active.info);

			active_members_reset();
			return -ENOENT;
		}

		if (cur_inst->set_info->lockable) {
			err = csip_set_coordinator_read_set_lock(cur_inst);
			if (err == 0) {
				busy = true;
			} else {
				cur_inst = NULL;
			}

			break;
		}

		active.members_handled++;
	}

	if (!busy && err == 0) {
		/* We are not reading any lock states (because they are not on the remote devices),
		 * so we can just initiate the ordered access procedure (oap) callback directly
		 * here.
		 */
		if (active.oap_cb == NULL ||
		    !active.oap_cb(active.info, active.members, active.members_count)) {
			err = -ECANCELED;
		}

		ordered_access_complete(active.info, err, false, NULL);
	}

	return err;
}

int bt_csip_set_coordinator_ordered_access(
	const struct bt_csip_set_coordinator_set_member *members[],
	uint8_t count,
	const struct bt_csip_set_coordinator_set_info *set_info,
	bt_csip_set_coordinator_ordered_access_t cb)
{
	int err;

	/* wait for the get_lock_state to finish and then call the callback */
	active.oap_cb = cb;

	err = bt_csip_set_coordinator_get_lock_state(members, count, set_info);
	if (err != 0) {
		active.oap_cb = NULL;

		return err;
	}

	return 0;
}

int bt_csip_set_coordinator_lock(
	const struct bt_csip_set_coordinator_set_member **members,
	uint8_t count,
	const struct bt_csip_set_coordinator_set_info *set_info)
{
	int err;

	CHECKIF(busy) {
		LOG_DBG("csip_set_coordinator busy");
		return -EBUSY;
	}

	err = verify_members(members, count, set_info);
	if (err != 0) {
		LOG_DBG("Could not verify members: %d", err);
		return err;
	}

	active_members_store_ordered(members, count, set_info, true);

	cur_inst = lookup_instance_by_set_info(active.members[0], active.info);
	if (cur_inst == NULL) {
		LOG_DBG("Failed to lookup instance by set_info %p", active.info);

		active_members_reset();
		return -ENOENT;
	}

	err = csip_set_coordinator_write_set_lock(cur_inst, true,
						  csip_set_coordinator_write_lock_cb);
	if (err == 0) {
		busy = true;
	}

	return err;
}

int bt_csip_set_coordinator_release(const struct bt_csip_set_coordinator_set_member **members,
				    uint8_t count,
				    const struct bt_csip_set_coordinator_set_info *set_info)
{
	int err;

	CHECKIF(busy) {
		LOG_DBG("csip_set_coordinator busy");
		return -EBUSY;
	}

	err = verify_members(members, count, set_info);
	if (err != 0) {
		LOG_DBG("Could not verify members: %d", err);
		return err;
	}

	active_members_store_ordered(members, count, set_info, false);

	cur_inst = lookup_instance_by_set_info(active.members[0], active.info);
	if (cur_inst == NULL) {
		LOG_DBG("Failed to lookup instance by set_info %p", active.info);

		active_members_reset();
		return -ENOENT;
	}

	err = csip_set_coordinator_write_set_lock(cur_inst, false,
						  csip_set_coordinator_write_release_cb);
	if (err == 0) {
		busy = true;
	}

	return err;
}
