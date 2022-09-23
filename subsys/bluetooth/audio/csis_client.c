/* Bluetooth Coordinated Set Identification Client
 *
 * Copyright (c) 2020 Bose Corporation
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * csis_client should be used in the following way
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
#include <zephyr/bluetooth/audio/csis.h>
#include "csis_crypto.h"
#include "csis_internal.h"
#include "../host/conn_internal.h"
#include "../host/keys.h"
#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_CSIS_CLIENT)
#define LOG_MODULE_NAME bt_csis_client
#include "common/log.h"

static uint8_t gatt_write_buf[1];
static struct bt_gatt_write_params write_params;
static struct bt_gatt_read_params read_params;
static struct bt_gatt_discover_params discover_params;
static struct bt_csis *cur_inst;
static bool busy;

static struct active_members {
	struct bt_csis_client_set_member *members[CONFIG_BT_MAX_CONN];
	const struct bt_csis_client_set_info *info;
	uint8_t members_count;
	uint8_t members_handled;
	uint8_t members_restored;

	bt_csis_client_ordered_access_t oap_cb;
} active;

struct bt_csis_client_inst {
	uint8_t inst_count;
	struct bt_csis csis_insts[CONFIG_BT_CSIS_CLIENT_MAX_CSIS_INSTANCES];
	struct bt_csis_client_set_member *set_member;
};

static struct bt_uuid_16 uuid = BT_UUID_INIT_16(0);

static struct bt_csis_client_cb *csis_client_cbs;
static struct bt_csis_client_inst client_insts[CONFIG_BT_MAX_CONN];

static int read_set_sirk(struct bt_csis *csis);
static int csis_client_read_set_size(struct bt_conn *conn, uint8_t inst_idx,
				     bt_gatt_read_func_t cb);
static int csis_client_read_set_lock(struct bt_csis *inst);

static uint8_t csis_client_discover_insts_read_set_sirk_cb(
	struct bt_conn *conn, uint8_t err, struct bt_gatt_read_params *params,
	const void *data, uint16_t length);
static void discover_insts_resume(struct bt_conn *conn, uint16_t sirk_handle,
				 uint16_t size_handle, uint16_t rank_handle);

static void active_members_reset(void)
{
	(void)memset(&active, 0, sizeof(active));
}

static struct bt_csis *lookup_instance_by_handle(struct bt_conn *conn,
						 uint16_t handle)
{
	uint8_t conn_index;
	struct bt_csis_client_inst *client;

	__ASSERT(conn, "NULL conn");
	__ASSERT(handle > 0, "Handle cannot be 0");

	conn_index = bt_conn_index(conn);
	client = &client_insts[conn_index];

	for (int i = 0; i < ARRAY_SIZE(client->csis_insts); i++) {
		if (client->csis_insts[i].cli.start_handle <= handle &&
		    client->csis_insts[i].cli.end_handle >= handle) {
			return &client->csis_insts[i];
		}
	}

	return NULL;
}

static struct bt_csis *lookup_instance_by_index(const struct bt_conn *conn,
						uint8_t idx)
{
	uint8_t conn_index;
	struct bt_csis_client_inst *client;

	__ASSERT(conn, "NULL conn");
	__ASSERT(idx < CONFIG_BT_CSIS_CLIENT_MAX_CSIS_INSTANCES,
		 "Index shall be less than maximum number of instances %u (was %u)",
		 CONFIG_BT_CSIS_CLIENT_MAX_CSIS_INSTANCES, idx);

	conn_index = bt_conn_index(conn);
	client = &client_insts[conn_index];
	return &client->csis_insts[idx];
}

static struct bt_csis *lookup_instance_by_set_info(const struct bt_csis_client_set_member *member,
						   const struct bt_csis_client_set_info *set_info)
{
	for (int i = 0; i < ARRAY_SIZE(member->insts); i++) {
		const struct bt_csis_client_set_info *member_set_info;

		member_set_info = &member->insts[i].info;
		if (member_set_info->set_size == set_info->set_size &&
		    memcmp(&member_set_info->set_sirk,
			   &set_info->set_sirk,
			   sizeof(set_info->set_sirk)) == 0) {
			return lookup_instance_by_index(member->conn, i);
		}
	}

	return NULL;
}

static struct bt_csis *get_next_active_instance(void)
{
	struct bt_csis_client_set_member *member;
	struct bt_csis *inst;

	member = active.members[active.members_handled];

	inst =  lookup_instance_by_set_info(member, active.info);
	if (inst == NULL) {
		BT_DBG("Failed to lookup instance by set_info %p", active.info);
	}

	return inst;
}

static int member_rank_compare_asc(const void *m1, const void *m2)
{
	const struct bt_csis_client_set_member *member_1 =
		*(const struct bt_csis_client_set_member **)m1;
	const struct bt_csis_client_set_member *member_2 =
		*(const struct bt_csis_client_set_member **)m2;
	struct bt_csis *inst_1;
	struct bt_csis *inst_2;

	inst_1 = lookup_instance_by_set_info(member_1, active.info);
	inst_2 = lookup_instance_by_set_info(member_2, active.info);

	if (inst_1 == NULL) {
		BT_ERR("inst_1 was NULL for member %p", member_1);
		return 0;
	}

	if (inst_2 == NULL) {
		BT_ERR("inst_2 was NULL for member %p", member_2);
		return 0;
	}

	return inst_1->cli.rank - inst_2->cli.rank;
}

static int member_rank_compare_desc(const void *m1, const void *m2)
{
	/* If we call the "compare ascending" function with the members
	 * reversed, it will work as a descending comparison
	 */
	return member_rank_compare_asc(m2, m1);
}

static void active_members_store_ordered(struct bt_csis_client_set_member *members[],
					 size_t count,
					 const struct bt_csis_client_set_info *info,
					 bool ascending)
{
	(void)memcpy(active.members, members, count * sizeof(members[0U]));
	active.members_count = count;
	active.info = info;

	if (count > 1U && CONFIG_BT_MAX_CONN > 1) {
		qsort(active.members, count, sizeof(members[0U]),
		ascending ? member_rank_compare_asc : member_rank_compare_desc);

		if (IS_ENABLED(CONFIG_ASSERT)) {
			for (size_t i = 1U; i < count; i++) {
				const struct bt_csis *inst_1 =
					lookup_instance_by_set_info(active.members[i - 1U], info);
				const struct bt_csis *inst_2 =
					lookup_instance_by_set_info(active.members[i], info);
				const uint8_t rank_1 = inst_1->cli.rank;
				const uint8_t rank_2 = inst_2->cli.rank;

				if (ascending) {
					__ASSERT(rank_1 <= rank_2,
						"Members not sorted by ascending rank %u - %u",
						rank_1, rank_2);
				} else {
					__ASSERT(rank_1 >= rank_2,
						"Members not sorted by descending rank %u - %u",
						rank_1, rank_2);
				}
			}
		}
	}
}

static int sirk_decrypt(struct bt_conn *conn,
			const uint8_t *enc_sirk,
			uint8_t *out_sirk)
{
	int err;
	uint8_t *k;

	if (IS_ENABLED(CONFIG_BT_CSIS_CLIENT_TEST_SAMPLE_DATA)) {
		/* test_k is from the sample data from A.2 in the CSIS spec */
		static uint8_t test_k[] = {0x67, 0x6e, 0x1b, 0x9b,
					   0xd4, 0x48, 0x69, 0x6f,
					   0x06, 0x1e, 0xc6, 0x22,
					   0x3c, 0xe5, 0xce, 0xd9};
		static bool swapped;

		BT_DBG("Decrypting with sample data K");

		if (!swapped && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) {
			/* Swap test_k to little endian */
			sys_mem_swap(test_k, 16);
			swapped = true;
		}
		k = test_k;
	} else {
		k = conn->le.keys->ltk.val;
	}

	err = bt_csis_sdf(k, enc_sirk, out_sirk);

	return err;
}

static uint8_t sirk_notify_func(struct bt_conn *conn,
				struct bt_gatt_subscribe_params *params,
				const void *data, uint16_t length)
{
	uint16_t handle = params->value_handle;
	struct bt_csis *csis_inst;

	if (data == NULL) {
		BT_DBG("[UNSUBSCRIBED] %u", params->value_handle);
		params->value_handle = 0U;

		return BT_GATT_ITER_STOP;
	}

	if (conn == NULL) {
		return BT_GATT_ITER_CONTINUE;
	}

	csis_inst = lookup_instance_by_handle(conn, handle);

	if (csis_inst != NULL) {
		BT_DBG("Instance %u", csis_inst->cli.idx);
		if (length == sizeof(struct bt_csis_set_sirk)) {
			struct bt_csis_set_sirk *sirk =
				(struct bt_csis_set_sirk *)data;
			struct bt_csis_client_inst *client;
			uint8_t *dst_sirk;

			client = &client_insts[bt_conn_index(conn)];
			dst_sirk = client->set_member->insts[csis_inst->cli.idx].info.set_sirk;

			BT_DBG("Set SIRK %sencrypted",
			       sirk->type == BT_CSIS_SIRK_TYPE_PLAIN
				? "not " : "");

			/* Assuming not connected to other set devices */
			if (sirk->type == BT_CSIS_SIRK_TYPE_ENCRYPTED) {
				if (IS_ENABLED(CONFIG_BT_CSIS_CLIENT_ENC_SIRK_SUPPORT)) {
					int err;

					BT_HEXDUMP_DBG(sirk->value,
						       sizeof(*sirk),
						       "Encrypted Set SIRK");
					err = sirk_decrypt(conn, sirk->value,
							   dst_sirk);
					if (err != 0) {
						BT_ERR("Could not decrypt "
						       "SIRK %d", err);
					}
				} else {
					BT_DBG("Encrypted SIRK not supported");
					return BT_GATT_ITER_CONTINUE;
				}
			} else {
				(void)memcpy(dst_sirk, sirk->value, sizeof(sirk->value));
			}

			BT_HEXDUMP_DBG(dst_sirk, BT_CSIS_SET_SIRK_SIZE,
				       "Set SIRK");

			/* TODO: Notify app */
		} else {
			BT_DBG("Invalid length %u", length);
		}
	} else {
		BT_DBG("Notification/Indication on unknown CSIS inst");
	}

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t size_notify_func(struct bt_conn *conn,
				struct bt_gatt_subscribe_params *params,
				const void *data, uint16_t length)
{
	uint8_t set_size;
	uint16_t handle = params->value_handle;
	struct bt_csis *csis_inst;

	if (data == NULL) {
		BT_DBG("[UNSUBSCRIBED] %u", params->value_handle);
		params->value_handle = 0U;

		return BT_GATT_ITER_STOP;
	}

	if (conn == NULL) {
		return BT_GATT_ITER_CONTINUE;
	}

	csis_inst = lookup_instance_by_handle(conn, handle);

	if (csis_inst != NULL) {
		if (length == sizeof(set_size)) {
			struct bt_csis_client_inst *client;
			struct bt_csis_client_set_info *set_info;

			client = &client_insts[bt_conn_index(conn)];
			set_info = &client->set_member->insts[csis_inst->cli.idx].info;

			(void)memcpy(&set_size, data, length);
			BT_DBG("Set size updated from %u to %u",
			       set_info->set_size, set_size);

			set_info->set_size = set_size;
			/* TODO: Notify app */
		} else {
			BT_DBG("Invalid length %u", length);
		}

	} else {
		BT_DBG("Notification/Indication on unknown CSIS inst");
	}
	BT_HEXDUMP_DBG(data, length, "Value");

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t lock_notify_func(struct bt_conn *conn,
				struct bt_gatt_subscribe_params *params,
				const void *data, uint16_t length)
{
	uint8_t value;
	uint16_t handle = params->value_handle;
	struct bt_csis *csis_inst;

	if (data == NULL) {
		BT_DBG("[UNSUBSCRIBED] %u", params->value_handle);
		params->value_handle = 0U;

		return BT_GATT_ITER_STOP;
	}

	if (conn == NULL) {
		return BT_GATT_ITER_CONTINUE;
	}

	csis_inst = lookup_instance_by_handle(conn, handle);

	if (csis_inst != NULL) {
		if (length == sizeof(csis_inst->cli.set_lock)) {
			bool locked;

			(void)memcpy(&value, data, length);
			if (value != BT_CSIS_RELEASE_VALUE &&
			    value != BT_CSIS_LOCK_VALUE) {
				BT_DBG("Invalid value %u", value);
				return BT_GATT_ITER_STOP;
			}

			(void)memcpy(&csis_inst->cli.set_lock, data, length);

			locked = csis_inst->cli.set_lock == BT_CSIS_LOCK_VALUE;
			BT_DBG("Instance %u lock was %s",
			       csis_inst->cli.idx,
			       locked ? "locked" : "released");
			if (csis_client_cbs != NULL &&
			    csis_client_cbs->lock_changed != NULL) {
				struct bt_csis_client_inst *client;
				struct bt_csis_client_csis_inst *inst;

				client = &client_insts[bt_conn_index(conn)];
				inst = &client->set_member->insts[csis_inst->cli.idx];

				csis_client_cbs->lock_changed(inst, locked);
			}
		} else {
			BT_DBG("Invalid length %u", length);
		}
	} else {
		BT_DBG("Notification/Indication on unknown CSIS inst");
	}
	BT_HEXDUMP_DBG(data, length, "Value");

	return BT_GATT_ITER_CONTINUE;
}

static int csis_client_write_set_lock(struct bt_csis *inst,
				      bool lock, bt_gatt_write_func_t cb)
{
	if (inst->cli.set_lock_handle == 0) {
		BT_DBG("Handle not set");
		cur_inst = NULL;
		return -EINVAL;
	}

	/* Write to call control point */
	gatt_write_buf[0] = lock ? BT_CSIS_LOCK_VALUE : BT_CSIS_RELEASE_VALUE;
	write_params.data = gatt_write_buf;
	write_params.length = sizeof(lock);
	write_params.func = cb;
	write_params.handle = inst->cli.set_lock_handle;

	return bt_gatt_write(inst->cli.conn, &write_params);
}

static int read_set_sirk(struct bt_csis *csis)
{
	if (cur_inst != NULL) {
		if (cur_inst != csis) {
			return -EBUSY;
		}
	} else {
		cur_inst = csis;
	}

	if (csis->cli.set_sirk_handle == 0) {
		BT_DBG("Handle not set");
		return -EINVAL;
	}

	read_params.func = csis_client_discover_insts_read_set_sirk_cb;
	read_params.handle_count = 1;
	read_params.single.handle = csis->cli.set_sirk_handle;
	read_params.single.offset = 0U;

	return bt_gatt_read(csis->cli.conn, &read_params);
}

static int csis_client_read_set_size(struct bt_conn *conn, uint8_t inst_idx,
				     bt_gatt_read_func_t cb)
{
	if (inst_idx >= CONFIG_BT_CSIS_CLIENT_MAX_CSIS_INSTANCES) {
		return -EINVAL;
	} else if (cur_inst != NULL) {
		if (cur_inst != lookup_instance_by_index(conn, inst_idx)) {
			return -EBUSY;
		}
	} else {
		cur_inst = lookup_instance_by_index(conn, inst_idx);
		if (cur_inst == NULL) {
			BT_DBG("Inst not found");
			return -EINVAL;
		}
	}

	if (cur_inst->cli.set_size_handle == 0) {
		BT_DBG("Handle not set");
		cur_inst = NULL;
		return -EINVAL;
	}

	read_params.func = cb;
	read_params.handle_count = 1;
	read_params.single.handle = cur_inst->cli.set_size_handle;
	read_params.single.offset = 0U;

	return bt_gatt_read(conn, &read_params);
}

static int csis_client_read_rank(struct bt_conn *conn, uint8_t inst_idx,
				 bt_gatt_read_func_t cb)
{
	if (inst_idx >= CONFIG_BT_CSIS_CLIENT_MAX_CSIS_INSTANCES) {
		return -EINVAL;
	} else if (cur_inst != NULL) {
		if (cur_inst != lookup_instance_by_index(conn, inst_idx)) {
			return -EBUSY;
		}
	} else {
		cur_inst = lookup_instance_by_index(conn, inst_idx);
		if (cur_inst == NULL) {
			BT_DBG("Inst not found");
			return -EINVAL;
		}
	}

	if (cur_inst->cli.rank_handle == 0) {
		BT_DBG("Handle not set");
		cur_inst = NULL;
		return -EINVAL;
	}

	read_params.func = cb;
	read_params.handle_count = 1;
	read_params.single.handle = cur_inst->cli.rank_handle;
	read_params.single.offset = 0U;

	return bt_gatt_read(conn, &read_params);
}

static int csis_client_discover_sets(struct bt_csis_client_set_member *member)
{
	int err;

	if (member->conn == NULL) {
		BT_DBG("member->conn is NULL");
		return -EINVAL;
	} else if (busy) {
		return -EBUSY;
	}

	/* Start reading values and call CB when done */
	err = read_set_sirk(member->insts[0].csis);
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
	struct bt_csis_client_inst *client = &client_insts[bt_conn_index(conn)];
	struct bt_gatt_subscribe_params *sub_params = NULL;
	void *notify_handler = NULL;

	if (attr == NULL) {
		BT_DBG("Setup complete for %u / %u",
		       cur_inst->cli.idx + 1, client->inst_count);
		(void)memset(params, 0, sizeof(*params));

		if ((cur_inst->cli.idx + 1) < client->inst_count) {
			int err;

			cur_inst = &client->csis_insts[cur_inst->cli.idx + 1];
			discover_params.uuid = NULL;
			discover_params.start_handle = cur_inst->cli.start_handle;
			discover_params.end_handle = cur_inst->cli.end_handle;
			discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
			discover_params.func = discover_func;

			err = bt_gatt_discover(conn, &discover_params);
			if (err != 0) {
				BT_DBG("Discover failed (err %d)", err);
				cur_inst = NULL;
				busy = false;
				if (csis_client_cbs != NULL &&
				    csis_client_cbs->discover != NULL) {
					csis_client_cbs->discover(client->set_member, err,
								  client->inst_count);
				}
			}

		} else {
			int err;

			cur_inst = NULL;
			busy = false;
			err = csis_client_discover_sets(client->set_member);
			if (err != 0) {
				BT_DBG("Discover sets failed (err %d)", err);
				cur_inst = NULL;
				busy = false;
				if (csis_client_cbs != NULL &&
				    csis_client_cbs->discover != NULL) {
					csis_client_cbs->discover(client->set_member, err,
								  client->inst_count);
				}
			}
		}
		return BT_GATT_ITER_STOP;
	}

	BT_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC &&
	    client->inst_count != 0) {
		chrc = (struct bt_gatt_chrc *)attr->user_data;
		if (bt_uuid_cmp(chrc->uuid, BT_UUID_CSIS_SET_SIRK) == 0) {
			BT_DBG("Set SIRK");
			cur_inst->cli.set_sirk_handle = chrc->value_handle;
			sub_params = &cur_inst->cli.sirk_sub_params;
			sub_params->disc_params = &cur_inst->cli.sirk_sub_disc_params;
			notify_handler = sirk_notify_func;
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_CSIS_SET_SIZE) == 0) {
			BT_DBG("Set size");
			cur_inst->cli.set_size_handle = chrc->value_handle;
			sub_params = &cur_inst->cli.size_sub_params;
			sub_params->disc_params = &cur_inst->cli.size_sub_disc_params;
			notify_handler = size_notify_func;
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_CSIS_SET_LOCK) == 0) {
			BT_DBG("Set lock");
			cur_inst->cli.set_lock_handle = chrc->value_handle;
			sub_params = &cur_inst->cli.lock_sub_params;
			sub_params->disc_params = &cur_inst->cli.lock_sub_disc_params;
			notify_handler = lock_notify_func;
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_CSIS_RANK) == 0) {
			BT_DBG("Set rank");
			cur_inst->cli.rank_handle = chrc->value_handle;
		}

		if (sub_params != NULL && notify_handler != NULL) {
			sub_params->value = 0;
			if ((chrc->properties & BT_GATT_CHRC_NOTIFY) != 0) {
				sub_params->value = BT_GATT_CCC_NOTIFY;
			} else if ((chrc->properties & BT_GATT_CHRC_INDICATE) != 0) {
				sub_params->value = BT_GATT_CCC_INDICATE;
			}

			if (sub_params->value != 0) {
				/* With ccc_handle == 0 it will use auto discovery */
				sub_params->ccc_handle = 0;
				sub_params->end_handle = cur_inst->cli.end_handle;
				sub_params->value_handle = chrc->value_handle;
				sub_params->notify = notify_handler;
				bt_gatt_subscribe(conn, sub_params);
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
	struct bt_csis_client_inst *client = &client_insts[bt_conn_index(conn)];

	if (attr == NULL ||
	    client->inst_count == CONFIG_BT_CSIS_CLIENT_MAX_CSIS_INSTANCES) {
		BT_DBG("Discover complete, found %u instances",
		       client->inst_count);
		(void)memset(params, 0, sizeof(*params));

		if (client->inst_count != 0) {
			int err;

			cur_inst = &client->csis_insts[0];
			discover_params.uuid = NULL;
			discover_params.start_handle = cur_inst->cli.start_handle;
			discover_params.end_handle = cur_inst->cli.end_handle;
			discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
			discover_params.func = discover_func;

			err = bt_gatt_discover(conn, &discover_params);
			if (err != 0) {
				BT_DBG("Discover failed (err %d)", err);
				busy = false;
				cur_inst = NULL;
				if (csis_client_cbs != NULL &&
				    csis_client_cbs->discover != 0) {
					csis_client_cbs->discover(client->set_member,
								  err,
								  client->inst_count);
				}
			}
		} else {
			busy = false;
			cur_inst = NULL;
			if (csis_client_cbs != NULL &&
			    csis_client_cbs->discover != NULL) {
				csis_client_cbs->discover(client->set_member, 0, 0);
			}
		}

		return BT_GATT_ITER_STOP;
	}

	BT_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_PRIMARY) {
		prim_service = (struct bt_gatt_service_val *)attr->user_data;
		discover_params.start_handle = attr->handle + 1;

		cur_inst = &client->csis_insts[client->inst_count];
		cur_inst->cli.idx = client->inst_count;
		cur_inst->cli.start_handle = attr->handle + 1;
		cur_inst->cli.end_handle = prim_service->end_handle;
		cur_inst->cli.conn = bt_conn_ref(conn);
		client->inst_count++;
	}

	return BT_GATT_ITER_CONTINUE;
}

bool bt_csis_client_is_set_member(uint8_t set_sirk[BT_CSIS_SET_SIRK_SIZE],
				  struct bt_data *data)
{
	if (data->type == BT_DATA_CSIS_RSI &&
	    data->data_len == BT_CSIS_RSI_SIZE) {
		uint8_t err;

		uint32_t hash = sys_get_le24(data->data);
		uint32_t prand = sys_get_le24(data->data + 3);
		uint32_t calculated_hash;

		BT_DBG("hash: 0x%06x, prand 0x%06x", hash, prand);
		err = bt_csis_sih(set_sirk, prand, &calculated_hash);
		if (err != 0) {
			return false;
		}

		calculated_hash &= 0xffffff;

		BT_DBG("calculated_hash: 0x%06x, hash 0x%06x",
		       calculated_hash, hash);

		return calculated_hash == hash;
	}

	return false;
}

static uint8_t csis_client_discover_insts_read_rank_cb(struct bt_conn *conn,
						      uint8_t err,
						      struct bt_gatt_read_params *params,
						      const void *data,
						      uint16_t length)
{
	struct bt_csis_client_inst *client = &client_insts[bt_conn_index(conn)];
	int cb_err = err;

	__ASSERT(cur_inst != NULL, "cur_inst must not be NULL");

	busy = false;

	if (err != 0) {
		BT_DBG("err: 0x%02X", err);
	} else if (data != NULL) {
		BT_HEXDUMP_DBG(data, length, "Data read");

		if (length == 1) {
			(void)memcpy(&client->csis_insts[cur_inst->cli.idx].cli.rank,
				     data, length);
			BT_DBG("%u",
			       client->csis_insts[cur_inst->cli.idx].cli.rank);
		} else {
			BT_DBG("Invalid length, continuing to next member");
		}

		discover_insts_resume(conn, 0, 0, 0);
	}

	if (cb_err != 0) {
		if (csis_client_cbs != NULL &&
		    csis_client_cbs->discover != NULL) {
			csis_client_cbs->discover(client->set_member, cb_err,
						  client->inst_count);
		}
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t csis_client_discover_insts_read_set_size_cb(struct bt_conn *conn,
							  uint8_t err,
							  struct bt_gatt_read_params *params,
							  const void *data,
							  uint16_t length)
{
	struct bt_csis_client_inst *client = &client_insts[bt_conn_index(conn)];
	int cb_err = err;

	__ASSERT(cur_inst != NULL, "cur_inst must not be NULL");

	busy = false;

	if (err != 0) {
		BT_DBG("err: 0x%02X", err);
	} else if (data != NULL) {
		struct bt_csis_client_set_info *set_info;

		BT_HEXDUMP_DBG(data, length, "Data read");

		set_info = &client->set_member->insts[cur_inst->cli.idx].info;

		if (length == sizeof(set_info->set_size)) {
			(void)memcpy(&set_info->set_size, data, length);
			BT_DBG("%u", set_info->set_size);
		} else {
			BT_DBG("Invalid length");
		}

		discover_insts_resume(conn, 0, 0, cur_inst->cli.rank_handle);
	}

	if (cb_err != 0) {
		if (csis_client_cbs != NULL &&
		    csis_client_cbs->discover != NULL) {
			csis_client_cbs->discover(client->set_member, cb_err,
						  client->inst_count);
		}
	}

	return BT_GATT_ITER_STOP;
}

static int parse_sirk(struct bt_csis_client_set_member *member,
		      const void *data, uint16_t length)
{
	uint8_t *set_sirk;

	set_sirk = member->insts[cur_inst->cli.idx].info.set_sirk;

	if (length == sizeof(struct bt_csis_set_sirk)) {
		struct bt_csis_set_sirk *sirk =
			(struct bt_csis_set_sirk *)data;

		BT_DBG("Set SIRK %sencrypted",
		       sirk->type == BT_CSIS_SIRK_TYPE_PLAIN ? "not " : "");
		/* Assuming not connected to other set devices */
		if (sirk->type == BT_CSIS_SIRK_TYPE_ENCRYPTED) {
			if (IS_ENABLED(CONFIG_BT_CSIS_CLIENT_ENC_SIRK_SUPPORT)) {
				int err;

				BT_HEXDUMP_DBG(sirk->value, sizeof(sirk->value),
					       "Encrypted Set SIRK");
				err = sirk_decrypt(member->conn, sirk->value,
						   set_sirk);
				if (err != 0) {
					BT_ERR("Could not decrypt "
						"SIRK %d", err);
					return err;
				}
			} else {
				BT_WARN("Encrypted SIRK not supported");
				set_sirk = NULL;
				return BT_ATT_ERR_INSUFFICIENT_ENCRYPTION;
			}
		} else {
			(void)memcpy(set_sirk, sirk->value, sizeof(sirk->value));
		}

		if (set_sirk != NULL) {
			BT_HEXDUMP_DBG(set_sirk, BT_CSIS_SET_SIRK_SIZE,
				       "Set SIRK");
		}
	} else {
		BT_DBG("Invalid length");
		return BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
	}

	return 0;
}

static uint8_t csis_client_discover_insts_read_set_sirk_cb(struct bt_conn *conn,
							  uint8_t err,
							  struct bt_gatt_read_params *params,
							  const void *data,
							  uint16_t length)
{
	struct bt_csis_client_inst *client = &client_insts[bt_conn_index(conn)];
	int cb_err = err;

	__ASSERT(cur_inst != NULL, "cur_inst must not be NULL");

	busy = false;

	if (err != 0) {
		BT_DBG("err: 0x%02X", err);
	} else if (data != NULL) {
		BT_HEXDUMP_DBG(data, length, "Data read");

		cb_err = parse_sirk(client->set_member, data, length);

		if (cb_err != 0) {
			BT_DBG("Could not parse SIRK: %d", cb_err);
		} else {
			discover_insts_resume(conn, 0,
					     cur_inst->cli.set_size_handle,
					     cur_inst->cli.rank_handle);
		}
	}

	if (cb_err != 0) {
		if (csis_client_cbs != NULL &&
		    csis_client_cbs->discover != NULL) {
			csis_client_cbs->discover(client->set_member, cb_err,
						  client->inst_count);
		}
	}

	return BT_GATT_ITER_STOP;
}

/**
 * @brief Reads the (next) characteristics for the set discovery procedure
 *
 * It skips all handles that are 0.
 *
 * @param conn        Connection to a CSIS device.
 * @param sirk_handle 0, or the handle for the SIRK characteristic.
 * @param size_handle 0, or the handle for the size characteristic.
 * @param rank_handle 0, or the handle for the rank characteristic.
 */
static void discover_insts_resume(struct bt_conn *conn, uint16_t sirk_handle,
				 uint16_t size_handle, uint16_t rank_handle)
{
	int cb_err = 0;
	struct bt_csis_client_inst *client = &client_insts[bt_conn_index(conn)];

	if (size_handle != 0) {
		cb_err = csis_client_read_set_size(
				conn, cur_inst->cli.idx,
				csis_client_discover_insts_read_set_size_cb);
		if (cb_err != 0) {
			BT_DBG("Could not read set size: %d", cb_err);
		}
	} else if (rank_handle != 0) {
		cb_err = csis_client_read_rank(
				conn, cur_inst->cli.idx,
				csis_client_discover_insts_read_rank_cb);
		if (cb_err != 0) {
			BT_DBG("Could not read set rank: %d", cb_err);
		}
	} else {
		uint8_t next_idx = cur_inst->cli.idx + 1;

		cur_inst = NULL;
		if (next_idx < client->inst_count) {
			cur_inst = lookup_instance_by_index(conn, next_idx);

			/* Read next */
			cb_err = read_set_sirk(cur_inst);
		} else if (csis_client_cbs != NULL &&
			   csis_client_cbs->discover != NULL) {
			csis_client_cbs->discover(client->set_member, 0,
						  client->inst_count);
		}

		return;
	}

	if (cb_err != 0) {
		if (csis_client_cbs != NULL &&
		    csis_client_cbs->discover != NULL) {
			csis_client_cbs->discover(client->set_member, cb_err,
						  client->inst_count);
		}
	} else {
		busy = true;
	}
}

static void csis_client_write_restore_cb(struct bt_conn *conn, uint8_t err,
					 struct bt_gatt_write_params *params)
{
	busy = false;

	if (err != 0) {
		BT_WARN("Could not restore (%d)", err);
		active_members_reset();
		if (csis_client_cbs != NULL &&
		    csis_client_cbs->release_set != NULL) {
			csis_client_cbs->release_set(err);
		}
		return;
	}

	active.members_restored++;
	BT_DBG("Restored %u/%u members",
	       active.members_restored, active.members_handled);

	if (active.members_restored < active.members_handled &&
	    CONFIG_BT_MAX_CONN > 1) {
		struct bt_csis_client_set_member *member;
		int csis_client_err;

		member = active.members[active.members_handled - active.members_restored - 1];
		cur_inst = lookup_instance_by_set_info(member, active.info);
		if (cur_inst == NULL) {
			active_members_reset();
			if (csis_client_cbs != NULL &&
			    csis_client_cbs->release_set != NULL) {
				csis_client_cbs->release_set(-ENOENT);
			}

			return;
		}

		csis_client_err = csis_client_write_set_lock(cur_inst, false,
							     csis_client_write_restore_cb);
		if (csis_client_err == 0) {
			busy = true;
		} else {
			BT_DBG("Failed to release next member[%u]: %d",
			       active.members_handled, csis_client_err);

			active_members_reset();
			if (csis_client_cbs != NULL &&
			    csis_client_cbs->release_set != NULL) {
				csis_client_cbs->release_set(csis_client_err);
			}
		}
	} else {
		active_members_reset();
		if (csis_client_cbs != NULL &&
		    csis_client_cbs->release_set != NULL) {
			csis_client_cbs->release_set(0);
		}
	}
}

static void csis_client_write_lock_cb(struct bt_conn *conn, uint8_t err,
				      struct bt_gatt_write_params *params)
{
	busy = false;

	if (err != 0) {
		BT_DBG("Could not lock (0x%X)", err);
		if (active.members_handled > 0 && CONFIG_BT_MAX_CONN > 1) {
			struct bt_csis_client_set_member *member;
			int csis_client_err;

			active.members_restored = 0;

			member = active.members[active.members_handled - active.members_restored];
			cur_inst = lookup_instance_by_set_info(member,
							       active.info);
			if (cur_inst == NULL) {
				BT_DBG("Failed to lookup instance by set_info %p",
				       active.info);

				active_members_reset();
				if (csis_client_cbs != NULL &&
				csis_client_cbs->lock_set != NULL) {
					csis_client_cbs->lock_set(-ENOENT);
				}
			}

			csis_client_err = csis_client_write_set_lock(cur_inst,
								     false,
								     csis_client_write_restore_cb);
			if (csis_client_err == 0) {
				busy = true;
			} else {
				BT_WARN("Could not release lock of previous locked member: %d",
					csis_client_err);
				active_members_reset();
				return;
			}
		}

		active_members_reset();
		if (csis_client_cbs != NULL &&
		    csis_client_cbs->lock_set != NULL) {
			csis_client_cbs->lock_set(err);
		}
		return;
	}

	active.members_handled++;
	BT_DBG("Locked %u/%u members",
	       active.members_handled, active.members_count);

	if (active.members_handled < active.members_count) {
		struct bt_csis *prev_inst = cur_inst;
		int csis_client_err;

		cur_inst = get_next_active_instance();
		if (cur_inst == NULL) {
			active_members_reset();
			if (csis_client_cbs != NULL &&
			    csis_client_cbs->lock_set != NULL) {
				csis_client_cbs->lock_set(-ENOENT);
			}

			return;
		}

		csis_client_err = csis_client_write_set_lock(cur_inst, true,
							     csis_client_write_lock_cb);
		if (csis_client_err == 0) {
			busy = true;
		} else {
			BT_DBG("Failed to lock next member[%u]: %d",
			       active.members_handled, csis_client_err);

			active.members_restored = 0;

			csis_client_err = csis_client_write_set_lock(prev_inst,
								     false,
								     csis_client_write_restore_cb);
			if (csis_client_err == 0) {
				busy = true;
			} else {
				BT_WARN("Could not release lock of previous locked member: %d",
					csis_client_err);
				active_members_reset();
				return;
			}
		}
	} else {
		active_members_reset();
		if (csis_client_cbs != NULL &&
		    csis_client_cbs->lock_set != NULL) {
			csis_client_cbs->lock_set(0);
		}
	}
}

static void csis_client_write_release_cb(struct bt_conn *conn, uint8_t err,
					 struct bt_gatt_write_params *params)
{
	busy = false;

	if (err != 0) {
		BT_DBG("Could not release lock (%d)", err);
		active_members_reset();
		if (csis_client_cbs != NULL &&
		    csis_client_cbs->release_set != NULL) {
			csis_client_cbs->release_set(err);
		}
		return;
	}

	active.members_handled++;
	BT_DBG("Released %u/%u members",
	       active.members_handled, active.members_count);

	if (active.members_handled < active.members_count) {
		int csis_client_err;

		cur_inst = get_next_active_instance();
		if (cur_inst == NULL) {
			active_members_reset();
			if (csis_client_cbs != NULL &&
			    csis_client_cbs->release_set != NULL) {
				csis_client_cbs->release_set(-ENOENT);
			}

			return;
		}

		csis_client_err = csis_client_write_set_lock(cur_inst, false,
							     csis_client_write_release_cb);
		if (csis_client_err == 0) {
			busy = true;
		} else {
			BT_DBG("Failed to release next member[%u]: %d",
			       active.members_handled, csis_client_err);

			active_members_reset();
			if (csis_client_cbs != NULL &&
			    csis_client_cbs->release_set != NULL) {
				csis_client_cbs->release_set(csis_client_err);
			}
		}
	} else {
		active_members_reset();
		if (csis_client_cbs != NULL &&
		    csis_client_cbs->release_set != NULL) {
			csis_client_cbs->release_set(0);
		}
	}
}

static void csis_client_lock_state_read_cb(int err, bool locked)
{
	const struct bt_csis_client_set_info *info = active.info;
	struct bt_csis_client_set_member *cur_member = NULL;

	if (err || locked) {
		cur_member = active.members[active.members_handled];
	} else if (!active.oap_cb(info, active.members, active.members_count)) {
		err = -ECANCELED;
	}

	active_members_reset();

	if (csis_client_cbs != NULL &&
		csis_client_cbs->ordered_access != NULL) {
		csis_client_cbs->ordered_access(info, err, locked, cur_member);
	}
}

static uint8_t csis_client_read_lock_cb(struct bt_conn *conn, uint8_t err,
					struct bt_gatt_read_params *params,
					const void *data, uint16_t length)
{
	uint8_t value = 0;

	busy = false;

	if (err != 0) {
		BT_DBG("Could not read lock value (0x%X)", err);

		csis_client_lock_state_read_cb(err, false);

		return BT_GATT_ITER_STOP;
	}

	active.members_handled++;
	BT_DBG("Read lock state on %u/%u members",
	       active.members_handled, active.members_count);

	if (data == NULL || length != sizeof(cur_inst->cli.set_lock)) {
		BT_DBG("Invalid data %p or length %u", data, length);

		csis_client_lock_state_read_cb(err, false);

		return BT_GATT_ITER_STOP;
	}

	value = ((uint8_t *)data)[0];
	if (value != BT_CSIS_RELEASE_VALUE && value != BT_CSIS_LOCK_VALUE) {
		BT_DBG("Invalid value %u read", value);
		err = BT_ATT_ERR_UNLIKELY;

		csis_client_lock_state_read_cb(err, false);

		return BT_GATT_ITER_STOP;
	}

	cur_inst->cli.set_lock = value;

	if (value != BT_CSIS_RELEASE_VALUE) {
		BT_DBG("Set member not unlocked");

		csis_client_lock_state_read_cb(0, true);

		return BT_GATT_ITER_STOP;
	}

	if (active.members_handled < active.members_count) {
		int csis_client_err;

		cur_inst = get_next_active_instance();
		if (cur_inst == NULL) {
			csis_client_lock_state_read_cb(-ENOENT, false);

			return BT_GATT_ITER_STOP;
		}

		csis_client_err = csis_client_read_set_lock(cur_inst);
		if (csis_client_err == 0) {
			busy = true;
		} else {
			BT_DBG("Failed to read next member[%u]: %d",
			       active.members_handled, csis_client_err);

			csis_client_lock_state_read_cb(err, false);
		}
	} else {
		csis_client_lock_state_read_cb(0, false);
	}

	return BT_GATT_ITER_STOP;
}

static int csis_client_read_set_lock(struct bt_csis *inst)
{
	if (inst->cli.set_lock_handle == 0) {
		BT_DBG("Handle not set");
		cur_inst = NULL;
		return -EINVAL;
	}

	read_params.func = csis_client_read_lock_cb;
	read_params.handle_count = 1;
	read_params.single.handle = inst->cli.set_lock_handle;
	read_params.single.offset = 0;

	return bt_gatt_read(inst->cli.conn, &read_params);
}

static void csis_client_reset(struct bt_csis_client_inst *inst)
{
	for (size_t i = 0; i < ARRAY_SIZE(inst->csis_insts); i++) {
		struct bt_csis_client_svc_inst *cli = &inst->csis_insts[i].cli;

		cli->idx = 0;
		cli->rank = 0;
		cli->set_lock = 0;
		cli->start_handle = 0;
		cli->end_handle = 0;
		cli->set_sirk_handle = 0;
		cli->set_size_handle = 0;
		cli->set_lock_handle = 0;
		cli->rank_handle = 0;

		if (cli->conn != NULL) {
			struct bt_conn *conn = cli->conn;

			/* It's okay if these fail. In case of disconnect,
			 * we can't unsubscribe and they will just fail.
			 * In case that we reset due to another call of the
			 * discover function, we will unsubscribe (regardless of
			 * bonding state) to accommodate the new discovery
			 * values.
			 */
			(void)bt_gatt_unsubscribe(conn, &cli->sirk_sub_params);
			(void)bt_gatt_unsubscribe(conn, &cli->size_sub_params);
			(void)bt_gatt_unsubscribe(conn, &cli->lock_sub_params);

			bt_conn_unref(conn);
			cli->conn = NULL;
		}
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct bt_csis_client_inst *inst = &client_insts[bt_conn_index(conn)];

	/* All csis_insts share the same conn pointer */
	if (inst->csis_insts[0].cli.conn == conn) {
		csis_client_reset(inst);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.disconnected = disconnected,
};

/*************************** PUBLIC FUNCTIONS ***************************/
void bt_csis_client_register_cb(struct bt_csis_client_cb *cb)
{
	csis_client_cbs = cb;
}

int bt_csis_client_discover(struct bt_csis_client_set_member *member)
{
	int err;
	struct bt_csis_client_inst *client;

	CHECKIF(member == NULL) {
		BT_DBG("NULL member");
		return -EINVAL;
	}

	CHECKIF(member->conn == NULL) {
		BT_DBG("NULL conn");
		return -EINVAL;
	}

	if (busy) {
		return -EBUSY;
	}

	client = &client_insts[bt_conn_index(member->conn)];

	(void)memset(client, 0, sizeof(*client));

	client->set_member = member;
	for (size_t i = 0U; i < ARRAY_SIZE(client->csis_insts); i++) {
		client->csis_insts[i].cli.member = member;
	}

	/* Discover CSIS on peer, setup handles and notify */
	(void)memset(&discover_params, 0, sizeof(discover_params));
	(void)memcpy(&uuid, BT_UUID_CSIS, sizeof(uuid));
	discover_params.func = primary_discover_func;
	discover_params.uuid = &uuid.uuid;
	discover_params.type = BT_GATT_DISCOVER_PRIMARY;
	discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;

	err = bt_gatt_discover(member->conn, &discover_params);
	if (err == 0) {
		for (size_t i = 0; i < ARRAY_SIZE(member->insts); i++) {
			member->insts[i].csis = &client->csis_insts[i];
		}
		busy = true;
	}

	return err;
}

static int verify_members(struct bt_csis_client_set_member **members,
			  uint8_t count,
			  const struct bt_csis_client_set_info *set_info)
{
	bool zero_rank;
	uint8_t ranks[CONFIG_BT_MAX_CONN];

	if (count > CONFIG_BT_MAX_CONN) {
		BT_DBG("count (%u) larger than maximum support servers (%d)",
		       count, CONFIG_BT_MAX_CONN);
		return -EINVAL;
	}

	zero_rank = false;
	for (int i = 0; i < count; i++) {
		struct bt_csis_client_set_member *member = members[i];
		struct bt_csis *inst;
		struct bt_conn *conn;

		CHECKIF(member == NULL) {
			BT_DBG("Invalid member[%d] was NULL", i);
			return -EINVAL;
		}

		conn = member->conn;

		CHECKIF(conn == NULL) {
			BT_DBG("Member[%d] conn was NULL", i);
			return -EINVAL;
		}

		if (conn->state != BT_CONN_CONNECTED) {
			BT_DBG("Member[%d] was not connected", i);
			return -ENOTCONN;
		}

		inst = lookup_instance_by_set_info(member, set_info);
		if (inst == NULL) {
			BT_DBG("Member[%d] could not find matching instance for the set_info",
			       i);
			return -EINVAL;
		}

		ranks[i] = inst->cli.rank;
		if (ranks[i] == 0U && !zero_rank) {
			zero_rank = true;
		} else if (ranks[i] != 0 && zero_rank) {
			/* all members in a set shall either use rank, or not use rank */
			BT_DBG("Found mix of 0 and non-0 ranks");
			return -EINVAL;
		}

		if (!zero_rank) {
			for (size_t j = 0U; j < i; j++) {
				if (ranks[j] == ranks[i]) {
					/* duplicate rank */
					BT_DBG("Duplicate rank (%u) for members[%zu] "
					       "and members[%zu]",
					       ranks[i], i, j);
					return -EINVAL;
				}
			}
		}
	}

	return 0;
}

static int bt_csis_client_get_lock_state(struct bt_csis_client_set_member **members,
					 uint8_t count,
					 const struct bt_csis_client_set_info *set_info)
{
	int err;

	if (busy) {
		BT_DBG("csis_client busy");
		return -EBUSY;
	}

	err = verify_members(members, count, set_info);
	if (err != 0) {
		BT_DBG("Could not verify members: %d", err);
		return err;
	}

	active_members_store_ordered(members, count, set_info, true);

	cur_inst = lookup_instance_by_set_info(active.members[0], active.info);
	if (cur_inst == NULL) {
		BT_DBG("Failed to lookup instance by set_info %p", active.info);

		active_members_reset();
		return -ENOENT;
	}

	err = csis_client_read_set_lock(cur_inst);
	if (err == 0) {
		busy = true;
	} else {
		cur_inst = NULL;
	}

	return err;
}

int bt_csis_client_ordered_access(struct bt_csis_client_set_member *members[],
				  uint8_t count,
				  const struct bt_csis_client_set_info *set_info,
				  bt_csis_client_ordered_access_t cb)
{
	int err;

	err = bt_csis_client_get_lock_state(members, count, set_info);
	if (err != 0) {
		return err;
	}

	/* wait for the get_lock_state to finish and then call the callback */
	active.oap_cb = cb;

	return 0;
}

int bt_csis_client_lock(struct bt_csis_client_set_member **members,
			uint8_t count,
			const struct bt_csis_client_set_info *set_info)
{
	int err;

	CHECKIF(busy) {
		BT_DBG("csis_client busy");
		return -EBUSY;
	}

	err = verify_members(members, count, set_info);
	if (err != 0) {
		BT_DBG("Could not verify members: %d", err);
		return err;
	}

	active_members_store_ordered(members, count, set_info, true);

	cur_inst = lookup_instance_by_set_info(active.members[0], active.info);
	if (cur_inst == NULL) {
		BT_DBG("Failed to lookup instance by set_info %p", active.info);

		active_members_reset();
		return -ENOENT;
	}

	err = csis_client_write_set_lock(cur_inst, true,
					 csis_client_write_lock_cb);
	if (err == 0) {
		busy = true;
	}

	return err;
}

int bt_csis_client_release(struct bt_csis_client_set_member **members,
			   uint8_t count,
			   const struct bt_csis_client_set_info *set_info)
{
	int err;

	CHECKIF(busy) {
		BT_DBG("csis_client busy");
		return -EBUSY;
	}

	err = verify_members(members, count, set_info);
	if (err != 0) {
		BT_DBG("Could not verify members: %d", err);
		return err;
	}

	active_members_store_ordered(members, count, set_info, false);

	cur_inst = lookup_instance_by_set_info(active.members[0], active.info);
	if (cur_inst == NULL) {
		BT_DBG("Failed to lookup instance by set_info %p", active.info);

		active_members_reset();
		return -ENOENT;
	}

	err = csis_client_write_set_lock(cur_inst, false,
					 csis_client_write_release_cb);
	if (err == 0) {
		busy = true;
	}

	return err;
}
