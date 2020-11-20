/* Bluetooth CSIS - Coordinated Set Identification Client
 *
 * Copyright (c) 2020 Bose Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * CSIP should be used in the following way
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

#include <zephyr.h>
#include <zephyr/types.h>

#include <device.h>
#include <init.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include "csip.h"
#include "sih.h"
#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_CSIP)
#define LOG_MODULE_NAME bt_csip
#include "common/log.h"

#define FIRST_HANDLE                    0x0001
#define LAST_HANDLE                     0xFFFF
#define CSIP_DISCOVER_TIMER_VALUE       K_SECONDS(10)

struct csis_instance_t {
	uint8_t rank;
	uint8_t set_lock;

	uint16_t start_handle;
	uint16_t end_handle;
	uint16_t set_sirk_handle;
	uint16_t set_size_handle;
	uint16_t set_lock_handle;
	uint16_t rank_handle;

	uint8_t idx;
	struct bt_gatt_subscribe_params sirk_sub_params;
	struct bt_gatt_subscribe_params size_sub_params;
	struct bt_gatt_subscribe_params lock_sub_params;
};

struct set_member_t {
	bool locked_by_us;
	uint8_t member_id;
	uint8_t inst_count;
	struct bt_conn *conn;
	struct bt_csip_set_t sets[CONFIG_BT_CSIP_MAX_CSIS_INSTANCES];
	struct csis_instance_t csis_insts[CONFIG_BT_CSIP_MAX_CSIS_INSTANCES];
};

static uint8_t gatt_write_buf[1];
static struct bt_gatt_write_params write_params;
static struct bt_gatt_read_params read_params;
static bool subscribe_all;
static struct bt_gatt_discover_params discover_params;
static struct csis_instance_t *cur_inst;
static struct bt_csip_set_t cur_set;
static bool busy;
static struct k_work_delayable discover_members_timer;

static struct bt_uuid_16 uuid = BT_UUID_INIT_16(0);

static struct bt_csip_cb_t *csip_cbs;
static bt_csip_discover_cb_t init_cb;
static bt_csip_discover_sets_cb_t discover_sets_cb;
static uint8_t members_found;
static struct set_member_t set_members[CONFIG_BT_MAX_CONN];
static struct bt_csip_set_member_t set_member_addrs[CONFIG_BT_MAX_CONN];

static int read_set_sirk(struct bt_conn *conn, uint8_t inst_idx);
static int csip_read_set_size(struct bt_conn *conn, uint8_t inst_idx,
			      bt_gatt_read_func_t cb);

static uint8_t csip_discover_sets_read_set_sirk_cb(
	struct bt_conn *conn, uint8_t err, struct bt_gatt_read_params *params,
	const void *data, uint16_t length);
static void csip_lock_set_write_lock_cb(struct bt_conn *conn, uint8_t err,
	struct bt_gatt_write_params *params);
static void csip_release_set_write_lock_cb(struct bt_conn *conn, uint8_t err,
	struct bt_gatt_write_params *params);
static void csip_lock_set_init_cb(struct bt_conn *conn, int err,
				  uint8_t set_count);

static struct set_member_t *lookup_member_by_conn(struct bt_conn *conn)
{
	for (int i = 0; i < ARRAY_SIZE(set_members); i++) {
		if (set_members[i].conn == conn) {
			return &set_members[i];
		}
	}
	return NULL;
}

static struct csis_instance_t *lookup_instance_by_handle(struct bt_conn *conn,
							 uint16_t handle)
{
	struct set_member_t *member;
	struct csis_instance_t *inst;

	member = lookup_member_by_conn(conn);

	if (member) {
		for (int i = 0; i < CONFIG_BT_CSIP_MAX_CSIS_INSTANCES; i++) {
			inst = &member->csis_insts[i];
			if (handle <= inst->end_handle &&
			    handle >= inst->start_handle) {
				return inst;
			}
		}
	}
	return NULL;
}

static struct csis_instance_t *lookup_instance_by_index(struct bt_conn *conn,
							uint8_t idx)
{
	struct set_member_t *member;
	struct csis_instance_t *inst;

	member = lookup_member_by_conn(conn);

	if (member) {
		for (int i = 0; i < CONFIG_BT_CSIP_MAX_CSIS_INSTANCES; i++) {
			inst = &member->csis_insts[i];
			if (idx == inst->idx) {
				return inst;
			}
		}
	}
	return NULL;
}


static int8_t lookup_index_by_sirk(struct bt_conn *conn, uint8_t *sirk)
{
	struct set_member_t *member;
	uint8_t *set_sirk;

	member = lookup_member_by_conn(conn);

	if (member) {
		for (int i = 0; i < CONFIG_BT_CSIP_MAX_CSIS_INSTANCES; i++) {
			set_sirk = member->sets[i].set_sirk;
			if (!memcmp(sirk, set_sirk, BT_CSIP_SET_SIRK_SIZE)) {
				return i;
			}
		}
	}
	return -1;
}

static uint8_t sirk_notify_func(struct bt_conn *conn,
				struct bt_gatt_subscribe_params *params,
				const void *data, uint16_t length)
{
	uint16_t handle = params->value_handle;
	struct csis_instance_t *csis_inst;

	if (!data) {
		BT_DBG("[UNSUBSCRIBED] %u", params->value_handle);
		params->value_handle = 0U;

		return BT_GATT_ITER_STOP;
	}

	csis_inst = lookup_instance_by_handle(conn, handle);

	if (csis_inst) {
		BT_DBG("Instance %u", csis_inst->idx);
		if (length == sizeof(cur_set.set_sirk)) {
			BT_HEXDUMP_DBG(data, length, "Set SIRK changed");
			/* Assuming not connected to other set devices */
			memcpy(cur_set.set_sirk, data, length);
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
	uint8_t size;
	uint16_t handle = params->value_handle;
	struct csis_instance_t *csis_inst;

	if (!data) {
		BT_DBG("[UNSUBSCRIBED] %u", params->value_handle);
		params->value_handle = 0U;

		return BT_GATT_ITER_STOP;
	}

	csis_inst = lookup_instance_by_handle(conn, handle);

	if (csis_inst) {
		if (length == sizeof(cur_set.set_size)) {
			memcpy(&size, data, length);
			BT_DBG("Set size updated from %u to %u",
			       cur_set.set_size, size);
			cur_set.set_size = size;
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
	struct csis_instance_t *csis_inst;

	if (!data) {
		BT_DBG("[UNSUBSCRIBED] %u", params->value_handle);
		params->value_handle = 0U;

		return BT_GATT_ITER_STOP;
	}

	csis_inst = lookup_instance_by_handle(conn, handle);

	if (csis_inst) {
		if (length == sizeof(csis_inst->set_lock)) {
			bool locked;

			memcpy(&value, data, length);
			if (value != BT_CSIP_RELEASE_VALUE &&
			    value != BT_CSIP_LOCK_VALUE) {
				BT_DBG("Invalid value %u", value);
				return BT_GATT_ITER_STOP;
			}

			memcpy(&csis_inst->set_lock, data, length);

			locked = csis_inst->set_lock == BT_CSIP_LOCK_VALUE;
			BT_DBG("Instance %u lock was %s",
			       csis_inst->idx, locked ? "locked" : "released");
			if (csip_cbs && csip_cbs->locked) {
				csip_cbs->locked(conn, &cur_set, locked);
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

static int csip_write_set_lock(struct bt_conn *conn, uint8_t inst_idx,
			       bool lock, bt_gatt_write_func_t cb)
{

	if (inst_idx >= CONFIG_BT_CSIP_MAX_CSIS_INSTANCES) {
		return -EINVAL;
	} else if (cur_inst) {
		if (cur_inst != lookup_instance_by_index(conn, inst_idx)) {
			return -EBUSY;
		}
	} else {
		cur_inst = lookup_instance_by_index(conn, inst_idx);
		if (!cur_inst) {
			BT_DBG("Inst not found");
			return -EINVAL;
		}
	}

	if (!cur_inst->rank_handle) {
		BT_DBG("Handle not set");
		cur_inst = NULL;
		return -EINVAL;
	}

	/* Write to call control point */
	gatt_write_buf[0] = lock ? BT_CSIP_LOCK_VALUE : BT_CSIP_RELEASE_VALUE;
	write_params.data = gatt_write_buf;
	write_params.length = sizeof(lock);
	write_params.func = cb;
	write_params.handle = cur_inst->set_lock_handle;

	return bt_gatt_write(conn, &write_params);
}

static int read_set_sirk(struct bt_conn *conn, uint8_t inst_idx)
{
	if (inst_idx >= CONFIG_BT_CSIP_MAX_CSIS_INSTANCES) {
		return -EINVAL;
	} else if (cur_inst) {
		if (cur_inst != lookup_instance_by_index(conn, inst_idx)) {
			return -EBUSY;
		}
	} else {
		cur_inst = lookup_instance_by_index(conn, inst_idx);
		if (!cur_inst) {
			BT_DBG("Inst not found");
			return -EINVAL;
		}
	}

	if (!cur_inst->set_sirk_handle) {
		BT_DBG("Handle not set");
		cur_inst = NULL;
		return -EINVAL;
	}

	read_params.func = csip_discover_sets_read_set_sirk_cb;
	read_params.handle_count = 1;
	read_params.single.handle = cur_inst->set_sirk_handle;
	read_params.single.offset = 0U;

	return bt_gatt_read(conn, &read_params);
}

static int csip_read_set_size(struct bt_conn *conn, uint8_t inst_idx,
			      bt_gatt_read_func_t cb)
{
	if (inst_idx >= CONFIG_BT_CSIP_MAX_CSIS_INSTANCES) {
		return -EINVAL;
	} else if (cur_inst) {
		if (cur_inst != lookup_instance_by_index(conn, inst_idx)) {
			return -EBUSY;
		}
	} else {
		cur_inst = lookup_instance_by_index(conn, inst_idx);
		if (!cur_inst) {
			BT_DBG("Inst not found");
			return -EINVAL;
		}
	}

	if (!cur_inst->set_size_handle) {
		BT_DBG("Handle not set");
		cur_inst = NULL;
		return -EINVAL;
	}

	read_params.func = cb;
	read_params.handle_count = 1;
	read_params.single.handle = cur_inst->set_size_handle;
	read_params.single.offset = 0U;

	return bt_gatt_read(conn, &read_params);
}

static int csip_read_set_lock(struct bt_conn *conn, uint8_t inst_idx,
			      bt_gatt_read_func_t cb)
{
	if (inst_idx >= CONFIG_BT_CSIP_MAX_CSIS_INSTANCES) {
		return -EINVAL;
	} else if (cur_inst) {
		if (cur_inst != lookup_instance_by_index(conn, inst_idx)) {
			return -EBUSY;
		}
	} else {
		cur_inst = lookup_instance_by_index(conn, inst_idx);
		if (!cur_inst) {
			BT_DBG("Inst not found");
			return -EINVAL;
		}
	}

	if (!cur_inst->set_lock_handle) {
		BT_DBG("Handle not set");
		cur_inst = NULL;
		return -EINVAL;
	}

	read_params.func = cb;
	read_params.handle_count = 1;
	read_params.single.handle = cur_inst->set_lock_handle;
	read_params.single.offset = 0U;

	return bt_gatt_read(conn, &read_params);
}

static int csip_read_rank(struct bt_conn *conn, uint8_t inst_idx,
			  bt_gatt_read_func_t cb)
{
	if (inst_idx >= CONFIG_BT_CSIP_MAX_CSIS_INSTANCES) {
		return -EINVAL;
	} else if (cur_inst) {
		if (cur_inst != lookup_instance_by_index(conn, inst_idx)) {
			return -EBUSY;
		}
	} else {
		cur_inst = lookup_instance_by_index(conn, inst_idx);
		if (!cur_inst) {
			BT_DBG("Inst not found");
			return -EINVAL;
		}
	}

	if (!cur_inst->rank_handle) {
		BT_DBG("Handle not set");
		cur_inst = NULL;
		return -EINVAL;
	}

	read_params.func = cb;
	read_params.handle_count = 1;
	read_params.single.handle = cur_inst->rank_handle;
	read_params.single.offset = 0U;

	return bt_gatt_read(conn, &read_params);
}

static uint8_t discover_func(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	int err;
	struct bt_gatt_chrc *chrc;
	struct set_member_t *member = lookup_member_by_conn(conn);
	struct bt_gatt_subscribe_params *sub_params = NULL;
	void *notify_handler = NULL;

	if (!member) {
		busy = false;
		cur_inst = NULL;
		if (init_cb) {
			init_cb(conn, -ENOTCONN, 0);
		}
		return BT_GATT_ITER_STOP;
	}

	if (!attr) {
		BT_DBG("Setup complete for %u / %u",
		       cur_inst->idx + 1, member->inst_count);
		(void)memset(params, 0, sizeof(*params));

		if ((cur_inst->idx + 1) < member->inst_count) {
			cur_inst = &member->csis_insts[cur_inst->idx + 1];
			discover_params.uuid = NULL;
			discover_params.start_handle =
				cur_inst->start_handle;
			discover_params.end_handle =
				cur_inst->end_handle;
			discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
			discover_params.func = discover_func;

			err = bt_gatt_discover(conn, &discover_params);
			if (err) {
				BT_DBG("Discover failed (err %d)", err);
				cur_inst = NULL;
				busy = false;
				if (init_cb) {
					init_cb(conn, err, member->inst_count);
				}
			}

		} else {
			cur_inst = NULL;
			busy = false;
			if (init_cb) {
				init_cb(conn, 0, member->inst_count);
			}
		}
		return BT_GATT_ITER_STOP;
	}

	BT_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC &&
	    member->inst_count) {
		chrc = (struct bt_gatt_chrc *)attr->user_data;
		if (!bt_uuid_cmp(chrc->uuid, BT_UUID_CSIS_SET_SIRK)) {
			BT_DBG("Set SIRK");
			cur_inst->set_sirk_handle = chrc->value_handle;
			sub_params = &cur_inst->sirk_sub_params;
			notify_handler = sirk_notify_func;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_CSIS_SET_SIZE)) {
			BT_DBG("Set size");
			cur_inst->set_size_handle = chrc->value_handle;
			sub_params = &cur_inst->size_sub_params;
			notify_handler = size_notify_func;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_CSIS_SET_LOCK)) {
			BT_DBG("Set lock");
			cur_inst->set_lock_handle = chrc->value_handle;
			sub_params = &cur_inst->lock_sub_params;
			notify_handler = lock_notify_func;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_CSIS_RANK)) {
			BT_DBG("Set rank");
			cur_inst->rank_handle = chrc->value_handle;
		}

		if (subscribe_all && sub_params && notify_handler) {
			sub_params->value = 0;
			if (chrc->properties & BT_GATT_CHRC_NOTIFY) {
				sub_params->value = BT_GATT_CCC_NOTIFY;
			} else if (chrc->properties & BT_GATT_CHRC_INDICATE) {
				sub_params->value = BT_GATT_CCC_INDICATE;
			}

			if (sub_params->value) {
				/*
				 * TODO: Don't assume that CCC is at handle + 2;
				 * do proper discovery;
				 */
				sub_params->ccc_handle = attr->handle + 2;
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
	int err;
	struct bt_gatt_service_val *prim_service;
	struct set_member_t *member = lookup_member_by_conn(conn);

	if (!member) {
		BT_DBG("Conn is not a member");
		busy = false;
		cur_inst = NULL;
		if (init_cb) {
			init_cb(conn, -ENOTCONN, 0);
		}

		return BT_GATT_ITER_STOP;
	}

	if (!attr || member->inst_count == CONFIG_BT_CSIP_MAX_CSIS_INSTANCES) {
		BT_DBG("Discover complete, found %u instances",
		       member->inst_count);
		(void)memset(params, 0, sizeof(*params));

		if (member->inst_count) {
			cur_inst = &member->csis_insts[0];
			discover_params.uuid = NULL;
			discover_params.start_handle =
				cur_inst->start_handle;
			discover_params.end_handle = cur_inst->end_handle;
			discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
			discover_params.func = discover_func;

			err = bt_gatt_discover(conn, &discover_params);
			if (err) {
				BT_DBG("Discover failed (err %d)", err);
				busy = false;
				cur_inst = NULL;
				if (init_cb) {
					init_cb(conn, err, member->inst_count);
				}
			}
		} else {
			busy = false;
			cur_inst = NULL;
			if (init_cb) {
				init_cb(conn, 0, 0);
			}
		}
		return BT_GATT_ITER_STOP;
	}

	BT_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_PRIMARY) {
		prim_service = (struct bt_gatt_service_val *)attr->user_data;
		discover_params.start_handle = attr->handle + 1;

		cur_inst = &member->csis_insts[member->inst_count];
		cur_inst->idx = member->inst_count;
		cur_inst->start_handle = attr->handle + 1;
		cur_inst->end_handle = prim_service->end_handle;
		member->inst_count++;
	}

	return BT_GATT_ITER_CONTINUE;
}

static bool is_set_member(struct bt_data *data)
{
	uint8_t err;
	uint32_t hash = ((uint32_t *)data->data)[0] & 0xffffff;
	uint32_t prand = ((uint32_t *)(data->data + 3))[0] & 0xffffff;
	uint32_t calculated_hash;

	BT_DBG("hash: 0x%06x, prand 0x%06x", hash, prand);
	err = sih(cur_set.set_sirk, prand, &calculated_hash);
	if (err) {
		return false;
	}

	calculated_hash &= 0xffffff;

	return calculated_hash == hash;
}

static bool is_discovered(const bt_addr_le_t *addr)
{
	for (int i = 0; i < members_found; i++) {
		if (!bt_addr_le_cmp(addr, &set_member_addrs[i].addr)) {
			return true;
		}
	}
	return false;
}

static bool csis_found(struct bt_data *data, void *user_data)
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	bt_addr_le_t *addr = user_data;

	if (data->type == BT_CSIS_AD_TYPE &&
	    data->data_len == BT_CSIS_PSRI_SIZE) {
		bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
		BT_DBG("Found CSIS advertiser with address %s",
		       log_strdup(addr_str));
		if (is_set_member(data)) {
			if (is_discovered(addr)) {
				BT_DBG("Set member already found");
				return true;
			}
			memcpy(&set_member_addrs[members_found].addr,
			       addr,
			       sizeof(bt_addr_le_t));

			members_found++;
			BT_DBG("Found member (%u / %u)",
			       members_found, cur_set.set_size);

			if (members_found == cur_set.set_size) {
				(void)k_work_cancel_delayable(&discover_members_timer);
				bt_le_scan_stop();
				busy = false;
				if (csip_cbs && csip_cbs->members) {
					csip_cbs->members(0, cur_set.set_size,
							  members_found);
				}

			}
		} else {
			BT_DBG("Not member of current set");
		}
	}
	return true;
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	/* We're only interested in connectable events */
	if (type != BT_GAP_ADV_TYPE_ADV_IND &&
	    type != BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
		return;
	}

	bt_data_parse(ad, csis_found, (void *)addr);
}

static int init_discovery(struct set_member_t *member, bool subscribe,
			  bt_csip_discover_cb_t cb)
{
	init_cb = cb;
	member->inst_count = 0;
	/* Discover CSIS on peer, setup handles and notify */
	subscribe_all = subscribe;
	(void)memset(&discover_params, 0, sizeof(discover_params));
	memcpy(&uuid, BT_UUID_CSIS, sizeof(uuid));
	discover_params.func = primary_discover_func;
	discover_params.uuid = &uuid.uuid;
	discover_params.type = BT_GATT_DISCOVER_PRIMARY;
	discover_params.start_handle = FIRST_HANDLE;
	discover_params.end_handle = LAST_HANDLE;

	return bt_gatt_discover(member->conn, &discover_params);
}

static uint8_t csip_discover_sets_read_rank_cb(
	struct bt_conn *conn, uint8_t err, struct bt_gatt_read_params *params,
	const void *data, uint16_t length)
{
	int error;
	uint8_t next_idx;
	struct set_member_t *member;

	if (err) {
		BT_DBG("err: 0x%02X", err);
	} else if (!cur_inst) {
		BT_DBG("cur_inst is NULL");
	} else if (data) {
		BT_HEXDUMP_DBG(data, length, "Data read");
		member = lookup_member_by_conn(conn);
		if (!member) {
			BT_DBG("member is NULL");
			return BT_GATT_ITER_STOP;
		}

		if (length == 1) {
			memcpy(&member->csis_insts[cur_inst->idx].rank,
			       data, length);
			BT_DBG("%u", member->csis_insts[cur_inst->idx].rank);
		} else {
			BT_DBG("Invalid length, continuing to next member");
		}

		next_idx = cur_inst->idx + 1;
		cur_inst = NULL;
		if (next_idx < member->inst_count) {
			/* Read next */
			error = read_set_sirk(conn, next_idx);
			if (error && discover_sets_cb) {
				busy = false;
				discover_sets_cb(conn, error,
						 member->inst_count,
						 member->sets);
			}
		} else if (discover_sets_cb) {
			busy = false;
			discover_sets_cb(conn, 0, member->inst_count,
						member->sets);
		}
	}
	return BT_GATT_ITER_STOP;
}

static uint8_t csip_discover_sets_read_set_lock_cb(
	struct bt_conn *conn, uint8_t err, struct bt_gatt_read_params *params,
	const void *data, uint16_t length)
{
	int error;
	uint8_t next_idx;
	struct set_member_t *member;
	uint8_t value;

	if (err) {
		BT_DBG("err: 0x%02X", err);
	} else if (!cur_inst) {
		BT_DBG("cur_inst is NULL");
	} else if (data) {
		BT_HEXDUMP_DBG(data, length, "Data read");
		member = lookup_member_by_conn(conn);
		if (!member) {
			BT_DBG("member is NULL");
			busy = false;
			discover_sets_cb(conn, -ENOTCONN, 0, NULL);
			return BT_GATT_ITER_STOP;
		}

		if (length == 1) {
			memcpy(&value, data, length);
			if (value != BT_CSIP_RELEASE_VALUE &&
			    value != BT_CSIP_LOCK_VALUE) {
				BT_DBG("Invalid value %u", value);
				busy = false;
				discover_sets_cb(conn, -ENOTCONN, 0, NULL);
				return BT_GATT_ITER_STOP;
			}
			memcpy(&member->csis_insts[cur_inst->idx].set_lock,
			       data, length);
			BT_DBG("%u",
			       member->csis_insts[cur_inst->idx].set_lock);
		} else {
			BT_DBG("Invalid length, continuing");
		}

		next_idx = cur_inst->idx;
		if (cur_inst->rank_handle) {
			csip_read_rank(conn, next_idx,
				       csip_discover_sets_read_rank_cb);
		} else {
			BT_DBG("Next instance");
			next_idx++;
			cur_inst = NULL;
			if (next_idx < member->inst_count) {
				/* Read next */
				error = read_set_sirk(conn, next_idx);
				if (error && discover_sets_cb) {
					busy = false;
					discover_sets_cb(conn, error,
							 member->inst_count,
							 member->sets);
				}
			} else if (discover_sets_cb) {
				busy = false;
				discover_sets_cb(conn, 0, member->inst_count,
						 member->sets);
			}
		}
	}
	return BT_GATT_ITER_STOP;
}

static uint8_t csip_discover_sets_read_set_size_cb(
	struct bt_conn *conn, uint8_t err, struct bt_gatt_read_params *params,
	const void *data, uint16_t length)
{
	int error;
	uint8_t next_idx;
	struct set_member_t *member;

	if (err) {
		BT_DBG("err: 0x%02X", err);
	} else if (!cur_inst) {
		BT_DBG("cur_inst is NULL");
	} else if (data) {
		BT_HEXDUMP_DBG(data, length, "Data read");
		member = lookup_member_by_conn(conn);
		if (!member) {
			BT_DBG("member is NULL");
			return BT_GATT_ITER_STOP;
		}

		if (length == 1) {
			memcpy(&member->sets[cur_inst->idx].set_size,
			       data, length);
			BT_DBG("%u", member->sets[cur_inst->idx].set_size);
		} else {
			BT_DBG("Invalid length");
		}

		next_idx = cur_inst->idx;
		if (cur_inst->set_lock_handle) {
			csip_read_set_lock(conn, next_idx,
					   csip_discover_sets_read_set_lock_cb);
		} else if (cur_inst->rank_handle) {
			csip_read_rank(conn, next_idx,
				       csip_discover_sets_read_rank_cb);
		} else {
			next_idx++;
			cur_inst = NULL;
			if (next_idx < member->inst_count) {
				/* Read next */
				error = read_set_sirk(conn, next_idx);
				if (error && discover_sets_cb) {
					busy = false;
					discover_sets_cb(conn, error,
							 member->inst_count,
							 member->sets);
				}
			} else if (discover_sets_cb) {
				busy = false;
				discover_sets_cb(conn, 0, member->inst_count,
						 member->sets);
			}
		}
	}
	return BT_GATT_ITER_STOP;
}

static uint8_t csip_discover_sets_read_set_sirk_cb(
	struct bt_conn *conn, uint8_t err, struct bt_gatt_read_params *params,
	const void *data, uint16_t length)
{
	int error;
	uint8_t next_idx;
	struct set_member_t *member;

	if (err) {
		BT_DBG("err: 0x%02X", err);
	} else if (!cur_inst) {
		BT_DBG("cur_inst is NULL");
	} else if (data) {
		BT_HEXDUMP_DBG(data, length, "Data read");
		member = lookup_member_by_conn(conn);
		if (!member) {
			BT_DBG("member is NULL");
			return BT_GATT_ITER_STOP;
		}

		if (length == BT_CSIP_SET_SIRK_SIZE) {
			memcpy(&member->sets[cur_inst->idx].set_sirk,
			       data, length);
			BT_HEXDUMP_DBG(member->sets[cur_inst->idx].set_sirk,
				       length, "Set SIRK");
		} else {
			BT_DBG("Invalid length");
		}

		next_idx = cur_inst->idx;
		if (cur_inst->set_size_handle) {
			csip_read_set_size(conn, next_idx,
					   csip_discover_sets_read_set_size_cb);
		} else if (cur_inst->set_lock_handle) {
			csip_read_set_lock(conn, next_idx,
					   csip_discover_sets_read_set_lock_cb);
		} else if (cur_inst->rank_handle) {
			csip_read_rank(conn, next_idx,
				       csip_discover_sets_read_rank_cb);
		} else {
			next_idx++;
			cur_inst = NULL;
			if (next_idx < member->inst_count) {
				/* Read next */
				error = read_set_sirk(conn, next_idx);
				if (error && discover_sets_cb) {
					busy = false;
					discover_sets_cb(conn, error,
							 member->inst_count,
							 member->sets);
				}
			} else if (discover_sets_cb) {
				busy = false;
				discover_sets_cb(conn, 0, member->inst_count,
						 member->sets);
			}
		}
	}
	return BT_GATT_ITER_STOP;
}


static void csip_connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];
	struct set_member_t *member = lookup_member_by_conn(conn);

	if (member) {
		bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

		if (err) {
			BT_DBG("Failed to connect to %s (%u)", log_strdup(addr),
			       err);
			return;
		}

		BT_DBG("Connected to %s", log_strdup(addr));

		err = init_discovery(member, true, csip_lock_set_init_cb);

		if (err) {
			if (csip_cbs && csip_cbs->lock_set) {
				csip_cbs->lock_set(err);
			}
		}

		bt_conn_ref(conn);
	}
}

static void csip_disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];
	struct set_member_t *member = lookup_member_by_conn(conn);

	if (member) {
		bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

		BT_DBG("Disconnected: %s (reason %u)", log_strdup(addr),
		       reason);

		bt_conn_unref(conn);
	}
}

static struct bt_conn_cb csip_conn_callbacks = {
	.connected = csip_connected,
	.disconnected = csip_disconnected,
};

static void csip_write_lowest_rank(void)
{
	int8_t cur_idx;
	uint8_t min_rank = 0xFF;
	uint8_t min_idx;
	struct bt_conn *min_conn = NULL;

	cur_inst = NULL;

	for (int i = 0; i < cur_set.set_size; i++) {
		cur_idx = lookup_index_by_sirk(set_members[i].conn,
					       cur_set.set_sirk);
		if (cur_idx < 0) {
			continue;
		}
		if (set_members[i].csis_insts[cur_idx].rank < min_rank &&
		    !set_members[i].locked_by_us) {
			min_rank = set_members[i].csis_insts[cur_idx].rank;
			min_idx = cur_idx;
			min_conn = set_members[i].conn;
		}
	}

	if (min_conn) {
		int err;

		BT_DBG("Locking member with rank %u", min_rank);
		err = csip_write_set_lock(min_conn, min_idx, true,
					  csip_lock_set_write_lock_cb);
		if (err) {
			if (csip_cbs && csip_cbs->lock_set) {
				csip_cbs->lock_set(err);
			}
		}
	} else {
		busy = false;
		if (csip_cbs && csip_cbs->lock_set) {
			csip_cbs->lock_set(0);
		}
	}
}

static void csip_release_highest_rank(void)
{
	int8_t cur_idx;
	uint8_t max_rank = 0;
	uint8_t max_idx;
	struct bt_conn *max_conn = NULL;

	cur_inst = NULL;

	for (int i = 0; i < cur_set.set_size; i++) {
		cur_idx = lookup_index_by_sirk(set_members[i].conn,
					       cur_set.set_sirk);
		if (cur_idx < 0) {
			continue;
		}
		if (set_members[i].csis_insts[cur_idx].rank > max_rank &&
		    set_members[i].locked_by_us) {
			max_rank = set_members[i].csis_insts[cur_idx].rank;
			max_idx = cur_idx;
			max_conn = set_members[i].conn;
		}
	}

	if (max_conn) {
		BT_DBG("Releasing member with rank %u", max_rank);
		csip_write_set_lock(max_conn, max_idx, false,
				    csip_release_set_write_lock_cb);
	} else {
		busy = false;
		if (csip_cbs && csip_cbs->release_set) {
			csip_cbs->release_set(0);
		}
	}
}

static void csip_release_set_write_lock_cb(struct bt_conn *conn, uint8_t err,
					   struct bt_gatt_write_params *params)
{
	struct set_member_t *member;

	if (err) {
		busy = false;
		if (csip_cbs && csip_cbs->release_set) {
			csip_cbs->release_set(err);
		}
		return;
	}
	member = lookup_member_by_conn(conn);

	if (!member) {
		busy = false;
		if (csip_cbs && csip_cbs->release_set) {
			csip_cbs->release_set(-ENOTCONN);
		}
		return;
	}

	member->locked_by_us = false;

	csip_release_highest_rank();
}

static void csip_lock_set_write_lock_cb(struct bt_conn *conn, uint8_t err,
					struct bt_gatt_write_params *params)
{
	struct set_member_t *member;

	if (err) {
		busy = false;
		BT_WARN("Could not lock set (%d)", err);
		if (csip_cbs && csip_cbs->lock_set) {
			csip_cbs->lock_set(err);
		}
		return;
	}
	member = lookup_member_by_conn(conn);

	if (!member) {
		busy = false;
		if (csip_cbs && csip_cbs->lock_set) {
			csip_cbs->lock_set(-ENOTCONN);
		}
		return;
	}

	member->locked_by_us = true;

	csip_write_lowest_rank();
}

static bool csip_set_is_locked(void)
{
	int8_t cur_idx;

	for (int i = 0; i < cur_set.set_size; i++) {
		cur_idx = lookup_index_by_sirk(set_members[i].conn,
					       cur_set.set_sirk);
		if (set_members[i].csis_insts[cur_idx].set_lock ==
			BT_CSIP_LOCK_VALUE) {
			return true;
		}
	}
	return false;
}

static void csip_lock_set_discover_sets_cb(struct bt_conn *conn, int err,
					   uint8_t set_count,
					   struct bt_csip_set_t *sets)
{
	char addr[BT_ADDR_LE_STR_LEN];

	for (int i = 0; i < cur_set.set_size; i++) {
		if (!set_members[i].conn) {
			int err;

			bt_addr_le_to_str(&set_member_addrs[i].addr, addr,
					  sizeof(addr));
			BT_DBG("[%d]: Connecting to %s", i, log_strdup(addr));

			err = bt_conn_le_create(&set_member_addrs[i].addr,
						BT_CONN_LE_CREATE_CONN,
						BT_LE_CONN_PARAM_DEFAULT,
						&set_members[i].conn);

			if (err) {
				if (csip_cbs && csip_cbs->lock_set) {
					csip_cbs->lock_set(err);
				}
			}
			return;
		}
	}

	/* Else everything is connected, start locking sets by rank */

	/* Make sure that the set is not locked */
	if (csip_set_is_locked()) {
		busy = false;
		if (csip_cbs && csip_cbs->lock_set) {
			csip_cbs->lock_set(-EBUSY);
		}
		return;
	}

	csip_write_lowest_rank();
}

static void csip_lock_set_init_cb(struct bt_conn *conn, int err,
				  uint8_t set_count)
{
	BT_DBG("");
	if (err || !conn) {
		BT_DBG("Could not connect");
	} else {
		discover_sets_cb = csip_lock_set_discover_sets_cb;
		/* Start reading values and call CB when done */
		err = read_set_sirk(conn, 0);
		if (err) {
			if (csip_cbs && csip_cbs->lock_set) {
				csip_cbs->lock_set(err);
			}
		}
	}
}

static void discover_members_timer_handler(struct k_work *work)
{
	BT_DBG("Could not find all members");
	bt_le_scan_stop();
	busy = false;
	if (csip_cbs && csip_cbs->members) {
		csip_cbs->members(-ETIMEDOUT, cur_set.set_size, members_found);
	}
}

/*************************** PUBLIC FUNCTIONS ***************************/
void bt_csip_register_cb(struct bt_csip_cb_t *cb)
{
	csip_cbs = cb;
}

int bt_csip_discover(struct bt_conn *conn, bool subscribe)
{
	int err;

	if (!conn) {
		return -ENOTCONN;
	} else if (busy) {
		return -EBUSY;
	}

	bt_conn_cb_register(&csip_conn_callbacks);
	memset(&set_members, 0, sizeof(set_members));

	k_work_init_delayable(&discover_members_timer,
			      discover_members_timer_handler);

	set_members[0].conn = conn;
	memcpy(&set_member_addrs[0].addr,
	       bt_conn_get_dst(conn),
	       sizeof(bt_addr_le_t));
	if (csip_cbs && csip_cbs->discover) {
		err = init_discovery(&set_members[0], subscribe,
				     csip_cbs->discover);
	} else {
		err = init_discovery(&set_members[0], subscribe, NULL);
	}
	if (!err) {
		busy = true;
	}
	return err;
}

int bt_csip_discover_sets(struct bt_conn *conn)
{
	struct set_member_t *member;
	int err;

	if (!conn) {
		BT_DBG("Not connected");
		return -ENOTCONN;
	} else if (busy) {
		return -EBUSY;
	}

	member = lookup_member_by_conn(conn);
	if (!member) {
		return -EINVAL;
	}

	if (csip_cbs && csip_cbs->sets) {
		discover_sets_cb = csip_cbs->sets;
	}
	/* Start reading values and call CB when done */
	err = read_set_sirk(conn, 0);
	if (!err) {
		busy = true;
	}
	return err;
}

int bt_csip_discover_members(struct bt_csip_set_t *set)
{
	int err;

	if (busy) {
		return -EBUSY;
	} else if (!set) {
		return -EINVAL;
	} else if (set->set_size && set->set_size > ARRAY_SIZE(set_members)) {
		/*
		 * TODO Handle case where set size is larger than
		 * number of possible connections
		 */
		BT_DBG("Set size (%u) larger than max connections (%u)",
		       set->set_size, (uint32_t)ARRAY_SIZE(set_members));
		return -EINVAL;
	}

	memcpy(&cur_set, set, sizeof(cur_set));
	members_found = 0;

	err = k_work_reschedule(&discover_members_timer,
				CSIP_DISCOVER_TIMER_VALUE);
	if (err < 0) { /* Can return 0, 1 and 2 for success */
		BT_DBG("Could not schedule discover_members_timer %d", err);
		return err;
	}


	/*
	 * First member may be the currently connected device, and
	 * we are unlikely to see advertisements from that device
	 */
	for (int i = 0; i < ARRAY_SIZE(set_members); i++) {
		if (set_members[i].conn) {
			members_found++;
		}
	}

	if (members_found == cur_set.set_size) {
		if (csip_cbs && csip_cbs->members) {
			csip_cbs->members(0, cur_set.set_size, members_found);
		}

		return 0;
	}

	/* TODO: Add timeout on scan if not all devices could be found */
	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, device_found);
	if (!err) {
		busy = true;
	}
	return err;
}

int bt_csip_lock_set(void)
{
	char addr[BT_ADDR_LE_STR_LEN];
	bool fully_connected = true;

	/* TODO: Check ranks of devices in sets for duplicates */

	if (busy) {
		return -EBUSY;
	}

	busy = true;

	for (int i = 0; i < cur_set.set_size; i++) {
		if (!set_members[i].conn) {
			int err;

			bt_addr_le_to_str(&set_member_addrs[i].addr, addr,
					  sizeof(addr));
			BT_DBG("[%d]: Connecting to %s", i, log_strdup(addr));


			err = bt_conn_le_create(&set_member_addrs[i].addr,
						BT_CONN_LE_CREATE_CONN,
						BT_LE_CONN_PARAM_DEFAULT,
						&set_members[i].conn);
			if (err) {
				return err;
			}

			fully_connected = false;
			break;
		}
	}

	if (fully_connected) {
		/* Make sure that the set is not locked */
		if (csip_set_is_locked()) {
			busy = false;
			return -EAGAIN;
		}

		csip_write_lowest_rank();
	}

	return 0;
}

int bt_csip_release_set(void)
{
	csip_release_highest_rank();
	return 0;
}

int bt_csip_disconnect(void)
{
	int err = 0;
	char addr[BT_ADDR_LE_STR_LEN];

	for (int i = 0; i < cur_set.set_size; i++) {
		BT_DBG("member %d", i);
		if (set_members[i].conn) {
			bt_addr_le_to_str(&set_member_addrs[i].addr, addr,
					  sizeof(addr));
			BT_DBG("Disconnecting %s", log_strdup(addr));
			err = bt_conn_disconnect(
				set_members[i].conn,
				BT_HCI_ERR_REMOTE_USER_TERM_CONN);
			set_members[i].conn = NULL;
			if (err) {
				return err;
			}
		}
	}
	return 0;
}
