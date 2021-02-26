/*  Bluetooth TBS - Telephone Bearer Service
 *
 * Copyright (c) 2020 Bose Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <zephyr/types.h>

#include <device.h>
#include <init.h>
#include <stdlib.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include "ccp.h"

#include "tbs_internal.h"
#include "ccid_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_TBS)
#define LOG_MODULE_NAME bt_tbs
#include "common/log.h"

#define BT_TBS_VALID_STATUS_FLAGS(val)         ((val) <= (BIT(0) | BIT(1)))

struct tbs_service_inst_t {
	/* Attribute values */
	char provider_name[CONFIG_BT_TBS_MAX_PROVIDER_NAME_LENGTH];
	char uci[BT_TBS_MAX_UCI_SIZE];
	char uri_scheme_list[CONFIG_BT_TBS_MAX_SCHEME_LIST_LENGTH];
	uint8_t technology;
	uint8_t signal_strength;
	uint8_t signal_strength_interval;
	uint8_t ccid;
	uint16_t optional_opcodes;
	uint16_t status_flags;
	struct bt_tbs_in_uri_t incoming_uri;
	struct bt_tbs_terminate_reason_t terminate_reason;
	struct bt_tbs_in_uri_t friendly_name;
	struct bt_tbs_in_uri_t in_call;

	/* Instance values */
	uint8_t index;
	struct bt_tbs_call_t calls[CONFIG_BT_TBS_MAX_CALLS];
	bool notify_current_calls;
	bool notify_call_states;
	bool pending_signal_strength_notification;
	struct k_work_delayable reporting_interval_work;

	/* TODO: The TBS (service) and the Telephone Bearers should be separated
	 * into two different instances. This is due to the addition of GTBS,
	 * where we now are in a state where this isn't a 1-to-1 correlation
	 * between TBS and the Telephone Bearers
	 */
	const struct bt_gatt_service_static *service_p;
};

struct gtbs_service_inst_t {
	/* Attribute values */
	char provider_name[CONFIG_BT_TBS_MAX_PROVIDER_NAME_LENGTH];
	char uci[BT_TBS_MAX_UCI_SIZE];
	uint8_t technology;
	uint8_t signal_strength;
	uint8_t signal_strength_interval;
	uint8_t ccid;
	uint16_t optional_opcodes;
	uint16_t status_flags;
	struct bt_tbs_in_uri_t incoming_uri;
	struct bt_tbs_in_uri_t friendly_name;
	struct bt_tbs_in_uri_t in_call;

	/* Instance values */
	bool notify_current_calls;
	bool notify_call_states;
	bool pending_signal_strength_notification;
	struct k_work_delayable reporting_interval_work;

	/* TODO: The TBS (service) and the Telephone Bearers should be separated
	 * into two different instances. This is due to the addition of GTBS,
	 * where we now are in a state where this isn't a 1-to-1 correlation
	 * between TBS and the Telephone Bearers
	 */
	const struct bt_gatt_service_static *service_p;
};

#if IS_ENABLED(CONFIG_BT_GTBS)
#define READ_BUF_SIZE	(CONFIG_BT_TBS_MAX_CALLS * \
				sizeof(struct bt_tbs_current_call_item_t) * \
				CONFIG_BT_TBS_BEARER_COUNT)
#else
#define READ_BUF_SIZE	(CONFIG_BT_TBS_MAX_CALLS * \
				sizeof(struct bt_tbs_current_call_item_t))
#endif /* IS_ENABLED(CONFIG_BT_GTBS) */
NET_BUF_SIMPLE_DEFINE_STATIC(read_buf, READ_BUF_SIZE);

static struct tbs_service_inst_t svc_insts[CONFIG_BT_TBS_BEARER_COUNT];
static struct gtbs_service_inst_t gtbs_inst;

/* Used to notify app with held calls in case of join */
static struct bt_tbs_call_t *held_calls[CONFIG_BT_TBS_MAX_CALLS];
static uint8_t held_calls_cnt;

static struct bt_tbs_cb_t *tbs_cbs;

static struct bt_tbs_call_t *lookup_call_in_instance(
	struct tbs_service_inst_t *inst, uint8_t call_index)
{
	if (call_index == BT_TBS_FREE_CALL_INDEX) {
		return NULL;
	}

	for (int i = 0; i < ARRAY_SIZE(svc_insts[i].calls); i++) {
		if (inst->calls[i].index == call_index) {
			return &inst->calls[i];
		}
	}
	return NULL;
}

/**
 * @brief Finds an returns a call
 *
 * @param call_index The ID of the call
 * @return struct bt_tbs_call_t* Pointer to the call. NULL if not found
 */
static struct bt_tbs_call_t *lookup_call(uint8_t call_index)
{
	struct bt_tbs_call_t *call;

	if (call_index == BT_TBS_FREE_CALL_INDEX) {
		return NULL;
	}

	for (int i = 0; i < ARRAY_SIZE(svc_insts); i++) {
		call = lookup_call_in_instance(&svc_insts[i], call_index);
		if (call) {
			return call;
		}
	}
	return NULL;
}

static struct tbs_service_inst_t *lookup_instance_by_ccc(
	const struct bt_gatt_attr *ccc)
{
	struct tbs_service_inst_t *inst;

	if (!ccc) {
		return NULL;
	}

	for (int i = 0; i < ARRAY_SIZE(svc_insts); i++) {
		inst = &svc_insts[i];
		if (!inst->service_p) {
			continue;
		}

		for (int j = 0; j < inst->service_p->attr_count; j++) {

			if (inst->service_p->attrs[j].user_data ==
				ccc->user_data) {
				return inst;
			}
		}
	}
	return NULL;
}

static struct tbs_service_inst_t *lookup_instance_by_call_index(
	uint8_t call_index)
{
	if (call_index == BT_TBS_FREE_CALL_INDEX) {
		return NULL;
	}

	for (int i = 0; i < ARRAY_SIZE(svc_insts); i++) {
		if (lookup_call_in_instance(&svc_insts[i], call_index)) {
			return &svc_insts[i];
		}
	}
	return NULL;
}

static bool is_authorized(struct bt_conn *conn)
{
	if (IS_ENABLED(CONFIG_BT_TBS_AUTHORIZATION)) {
		if (tbs_cbs && tbs_cbs->authorize) {
			return tbs_cbs->authorize(conn);
		} else {
			return false;
		}
	}
	return true;
}

static bool uri_scheme_in_list(const char *uri_scheme,
			       const char *uri_scheme_list)
{
	uint8_t scheme_len = strlen(uri_scheme);
	uint8_t scheme_list_len = strlen(uri_scheme_list);
	const char *uri_scheme_cand = uri_scheme_list;
	uint8_t uri_scheme_cand_len = 0;
	int start_idx = 0;

	for (int i = 0; i < scheme_list_len; i++) {
		if (uri_scheme_list[i] == ',') {
			uri_scheme_cand_len = i - start_idx;
			if (uri_scheme_cand_len != scheme_len) {
				continue;
			}

			if (!memcmp(uri_scheme, uri_scheme_cand, scheme_len)) {
				return true;
			}

			if (i + 1 < scheme_list_len) {
				uri_scheme_cand = &uri_scheme_list[i + 1];
			}
		}
	}
	return false;
}

static struct tbs_service_inst_t *find_tbs_by_uri_scheme(const char *uri,
							 uint8_t uri_len)
{
	char uri_scheme[CONFIG_BT_TBS_MAX_URI_LENGTH] = { 0 };

	/* Look for ':' between the first and last char */
	for (int i = 1; i < uri_len - 1; i++) {
		if (uri[i] == ':') {
			memcpy(uri_scheme, uri, i);
		}
	}

	if (strlen(uri_scheme) == 0) {
		/* No URI scheme found */
		return NULL;
	}

	for (int i = 0; i < ARRAY_SIZE(svc_insts); i++) {
		for (int j = 0; j < ARRAY_SIZE(svc_insts[i].calls); j++) {
			if (uri_scheme_in_list(uri_scheme,
					       svc_insts[i].uri_scheme_list)) {
				return &svc_insts[i];
			}
		}
	}
	return NULL;
}

static struct tbs_service_inst_t *lookup_instance_by_work(struct k_work *work)
{
	if (!work) {
		return NULL;
	}

	for (int i = 0; i < ARRAY_SIZE(svc_insts); i++) {
		if (&svc_insts[i].reporting_interval_work.work == work) {
			return &svc_insts[i];
		}
	}
	return NULL;
}

static void tbs_set_terminate_reason(struct tbs_service_inst_t *inst,
				     uint8_t call_index, uint8_t reason)
{
	inst->terminate_reason.call_index = call_index;
	inst->terminate_reason.reason = reason;
	BT_DBG("Index %u: call index 0x%02x, reason %s",
		inst->index, call_index, tbs_term_reason_str(reason));

	bt_gatt_notify_uuid(NULL, BT_UUID_TBS_TERMINATE_REASON,
			    inst->service_p->attrs,
			    (void *)&inst->terminate_reason,
			    sizeof(inst->terminate_reason));

	if (IS_ENABLED(CONFIG_BT_GTBS)) {
		bt_gatt_notify_uuid(NULL, BT_UUID_TBS_TERMINATE_REASON,
				    gtbs_inst.service_p->attrs,
				    (void *)&inst->terminate_reason,
				    sizeof(inst->terminate_reason));
	}
}

/**
 * @brief Gets the next free call_index
 *
 * For each new call, the call index should be incremented and wrap at 255.
 * However, the index = 0 is reserved for outgoing calls
 *
 * @return uint8_t The next free call index
 */
static uint8_t next_free_call_index(void)
{
	static uint8_t next_call_index = 1;
	struct bt_tbs_call_t *call;

	for (int i = 0; i < CONFIG_BT_TBS_MAX_CALLS; i++) {
		call = lookup_call(next_call_index);
		if (call == NULL) {
			return next_call_index++;
		}

		next_call_index++;
		if (next_call_index == UINT8_MAX) {
			/* call_index = 0 reserved for outgoing calls */
			next_call_index = 1;
		}
	}
	BT_DBG("No more free call spots");
	return BT_TBS_FREE_CALL_INDEX;
}


static void net_buf_put_call_state(void *inst_p)
{
	struct bt_tbs_call_t *call;
	struct bt_tbs_call_t *calls;
	int call_count;

	net_buf_simple_reset(&read_buf);

	if (!inst_p) {
		return;
	}

	if (IS_ENABLED(CONFIG_BT_GTBS) && inst_p == &gtbs_inst) {
		for (int i = 0; i < ARRAY_SIZE(svc_insts); i++) {
			calls = svc_insts[i].calls;
			call_count = ARRAY_SIZE(svc_insts[i].calls);

			for (int i = 0; i < call_count; i++) {
				call = &calls[i];
				if (call->index == BT_TBS_FREE_CALL_INDEX) {
					continue;
				}
				net_buf_simple_add_u8(&read_buf, call->index);
				net_buf_simple_add_u8(&read_buf, call->state);
				net_buf_simple_add_u8(&read_buf, call->flags);
			}

		}
	} else {
		struct tbs_service_inst_t *inst =
			(struct tbs_service_inst_t *)inst_p;
		calls = inst->calls;
		call_count = ARRAY_SIZE(inst->calls);

		for (int i = 0; i < call_count; i++) {
			call = &calls[i];
			if (call->index == BT_TBS_FREE_CALL_INDEX) {
				continue;
			}
			net_buf_simple_add_u8(&read_buf, call->index);
			net_buf_simple_add_u8(&read_buf, call->state);
			net_buf_simple_add_u8(&read_buf, call->flags);
		}
	}
}

static void net_buf_put_current_calls(void *inst_p)
{
	struct bt_tbs_call_t *call;
	struct bt_tbs_call_t *calls;
	int call_count;
	uint8_t uri_length;
	uint8_t item_len;

	net_buf_simple_reset(&read_buf);

	if (!inst_p) {
		return;
	}

	if (IS_ENABLED(CONFIG_BT_GTBS) && inst_p == &gtbs_inst) {
		for (int i = 0; i < ARRAY_SIZE(svc_insts); i++) {
			calls = svc_insts[i].calls;
			call_count = ARRAY_SIZE(svc_insts[i].calls);

			for (int i = 0; i < call_count; i++) {
				call = &calls[i];
				if (call->index == BT_TBS_FREE_CALL_INDEX) {
					continue;
				}
				uri_length = strlen(call->remote_uri);
				item_len = sizeof(call->index) +
						sizeof(call->state) +
						sizeof(call->flags) +
						uri_length;
				net_buf_simple_add_u8(&read_buf, item_len);
				net_buf_simple_add_u8(&read_buf, call->index);
				net_buf_simple_add_u8(&read_buf, call->state);
				net_buf_simple_add_u8(&read_buf, call->flags);
				net_buf_simple_add_mem(&read_buf,
						       call->remote_uri,
						       uri_length);
			}

		}
	} else {
		struct tbs_service_inst_t *inst =
			(struct tbs_service_inst_t *)inst_p;
		calls = inst->calls;
		call_count = ARRAY_SIZE(inst->calls);

		for (int i = 0; i < call_count; i++) {
			call = &calls[i];
			if (call->index == BT_TBS_FREE_CALL_INDEX) {
				continue;
			}
			uri_length = strlen(call->remote_uri);
			item_len = sizeof(call->index) +
					sizeof(call->state) +
					sizeof(call->flags) + uri_length;
			net_buf_simple_add_u8(&read_buf, item_len);
			net_buf_simple_add_u8(&read_buf, call->index);
			net_buf_simple_add_u8(&read_buf, call->state);
			net_buf_simple_add_u8(&read_buf, call->flags);
			net_buf_simple_add_mem(&read_buf, call->remote_uri,
					       uri_length);
		}
	}
}

static int notify_calls(struct tbs_service_inst_t *inst)
{
	int err = 0;

	if (!inst) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_GTBS)) {
		if (gtbs_inst.notify_call_states) {
			net_buf_put_call_state(&gtbs_inst);

			err = bt_gatt_notify_uuid(NULL, BT_UUID_TBS_CALL_STATE,
						  gtbs_inst.service_p->attrs,
						  read_buf.data, read_buf.len);
			if (err) {
				return err;
			}
		}

		if (gtbs_inst.notify_current_calls) {
			net_buf_put_current_calls(&gtbs_inst);

			err = bt_gatt_notify_uuid(
				NULL, BT_UUID_TBS_LIST_CURRENT_CALLS,
				gtbs_inst.service_p->attrs,
				read_buf.data, read_buf.len);
			if (err) {
				return err;
			}
		}
	}

	if (inst->notify_call_states) {
		net_buf_put_call_state(inst);

		err = bt_gatt_notify_uuid(NULL, BT_UUID_TBS_CALL_STATE,
					  inst->service_p->attrs,
					  read_buf.data, read_buf.len);
		if (err) {
			return err;
		}
	}
	if (inst->notify_current_calls) {
		net_buf_put_current_calls(inst);

		err = bt_gatt_notify_uuid(NULL, BT_UUID_TBS_LIST_CURRENT_CALLS,
					  inst->service_p->attrs,
					  read_buf.data, read_buf.len);
		if (err) {
			return err;
		}
	}

	return err;
}

static ssize_t read_provider_name(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr,
				  void *buf, uint16_t len, uint16_t offset)
{
	char *provider_name;

	if (IS_ENABLED(CONFIG_BT_GTBS) && attr->user_data == &gtbs_inst) {
		provider_name = gtbs_inst.provider_name;
		BT_DBG("GTBS: Provider name %s", log_strdup(provider_name));
	} else {
		struct tbs_service_inst_t *inst =
			(struct tbs_service_inst_t *)attr->user_data;
		provider_name = inst->provider_name;
		BT_DBG("Index %u, Provider name %s",
		       inst->index, log_strdup(provider_name));
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 provider_name, strlen(provider_name));
}

static void provider_name_cfg_changed(const struct bt_gatt_attr *attr,
				      uint16_t value)
{
	struct tbs_service_inst_t *inst = lookup_instance_by_ccc(attr);

	if (inst) {
		BT_DBG("Index %u: value 0x%04x", inst->index, value);
	} else if (IS_ENABLED(CONFIG_BT_GTBS)) {
		BT_DBG("GTBS: value 0x%04x", value);
	}
}

static ssize_t read_uci(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, uint16_t len, uint16_t offset)
{
	char *uci;

	if (IS_ENABLED(CONFIG_BT_GTBS) && attr->user_data == &gtbs_inst) {
		uci = gtbs_inst.uci;
		BT_DBG("GTBS: UCI %s", log_strdup(uci));
	} else {
		struct tbs_service_inst_t *inst =
			(struct tbs_service_inst_t *)attr->user_data;
		uci = inst->uci;
		BT_DBG("Index %u: UCI %s", inst->index, log_strdup(uci));
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 uci, strlen(uci));
}

static ssize_t read_technology(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr,
			       void *buf, uint16_t len, uint16_t offset)
{
	uint8_t technology;

	if (IS_ENABLED(CONFIG_BT_GTBS) && attr->user_data == &gtbs_inst) {
		technology = gtbs_inst.technology;
		BT_DBG("GTBS: Technology 0x%02X", technology);
	} else {
		struct tbs_service_inst_t *inst =
			(struct tbs_service_inst_t *)attr->user_data;
		technology = inst->technology;
		BT_DBG("Index %u: Technology 0x%02X", inst->index, technology);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &technology, sizeof(technology));
}

static void technology_cfg_changed(const struct bt_gatt_attr *attr,
				   uint16_t value)
{
	struct tbs_service_inst_t *inst = lookup_instance_by_ccc(attr);

	if (inst) {
		BT_DBG("Index %u: value 0x%04x", inst->index, value);
	} else if (IS_ENABLED(CONFIG_BT_GTBS)) {
		BT_DBG("GTBS: value 0x%04x", value);
	}
}

static ssize_t read_uri_scheme_list(struct bt_conn *conn,
				    const struct bt_gatt_attr *attr,
				    void *buf, uint16_t len, uint16_t offset)
{
	net_buf_simple_reset(&read_buf);

	if (IS_ENABLED(CONFIG_BT_GTBS) && attr->user_data == &gtbs_inst) {
		/* TODO: Make uri schemes unique */
		for (int i = 0; i < ARRAY_SIZE(svc_insts); i++) {
			size_t len = strlen(svc_insts[i].uri_scheme_list);

			if (read_buf.len + len >= read_buf.size) {
				BT_WARN("Cannot fit all TBS instances in GTBS "
					"URI scheme list");
				break;
			}

			net_buf_simple_add_mem(
				&read_buf, svc_insts[i].uri_scheme_list, len);
		}
		/* Add null terminator for printing */
		read_buf.data[read_buf.len] = '\0';
		BT_DBG("GTBS: URI scheme %s", log_strdup(read_buf.data));
	} else {
		struct tbs_service_inst_t *inst =
			(struct tbs_service_inst_t *)attr->user_data;
		net_buf_simple_add_mem(&read_buf, inst->uri_scheme_list,
				       strlen(inst->uri_scheme_list));
		/* Add null terminator for printing */
		read_buf.data[read_buf.len] = '\0';
		BT_DBG("Index %u: URI scheme %s", inst->index,
		       log_strdup(read_buf.data));
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 read_buf.data, read_buf.len);
}

static void uri_scheme_list_cfg_changed(const struct bt_gatt_attr *attr,
				   uint16_t value)
{
	struct tbs_service_inst_t *inst = lookup_instance_by_ccc(attr);

	if (inst) {
		BT_DBG("Index %u: value 0x%04x", inst->index, value);
	} else if (IS_ENABLED(CONFIG_BT_GTBS)) {
		BT_DBG("GTBS: value 0x%04x", value);
	}
}

static ssize_t read_signal_strength(struct bt_conn *conn,
				    const struct bt_gatt_attr *attr,
				    void *buf, uint16_t len, uint16_t offset)
{
	uint8_t signal_strength;

	if (IS_ENABLED(CONFIG_BT_GTBS) && attr->user_data == &gtbs_inst) {
		signal_strength = gtbs_inst.signal_strength;
		BT_DBG("GTBS: Signal strength 0x%02x", signal_strength);
	} else {
		struct tbs_service_inst_t *inst =
			(struct tbs_service_inst_t *)attr->user_data;
		signal_strength = inst->signal_strength;
		BT_DBG("Index %u: Signal strength 0x%02x",
			inst->index, signal_strength);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &signal_strength, sizeof(signal_strength));
}

static void signal_strength_cfg_changed(const struct bt_gatt_attr *attr,
					uint16_t value)
{
	struct tbs_service_inst_t *inst = lookup_instance_by_ccc(attr);

	if (inst) {
		BT_DBG("Index %u: value 0x%04x", inst->index, value);
	} else if (IS_ENABLED(CONFIG_BT_GTBS)) {
		BT_DBG("GTBS: value 0x%04x", value);
	}
}

static ssize_t read_signal_strength_interval(struct bt_conn *conn,
					     const struct bt_gatt_attr *attr,
					     void *buf, uint16_t len,
					     uint16_t offset)
{
	uint8_t signal_strength_interval;

	if (!is_authorized(conn)) {
		return BT_GATT_ERR(BT_ATT_ERR_AUTHORIZATION);
	}

	if (IS_ENABLED(CONFIG_BT_GTBS) && attr->user_data == &gtbs_inst) {
		signal_strength_interval = gtbs_inst.signal_strength_interval;
		BT_DBG("GTBS: Signal strength interval 0x%02x",
		       signal_strength_interval);
	} else {
		struct tbs_service_inst_t *inst =
			(struct tbs_service_inst_t *)attr->user_data;
		signal_strength_interval = inst->signal_strength_interval;
		BT_DBG("Index %u: Signal strength interval 0x%02x",
		       inst->index, signal_strength_interval);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &signal_strength_interval,
				 sizeof(signal_strength_interval));
}

static ssize_t write_signal_strength_interval(struct bt_conn *conn,
					      const struct bt_gatt_attr *attr,
					      const void *buf, uint16_t len,
					      uint16_t offset, uint8_t flags)
{
	struct net_buf_simple net_buf;
	uint8_t signal_strength_interval;

	if (!is_authorized(conn)) {
		return BT_GATT_ERR(BT_ATT_ERR_AUTHORIZATION);
	}

	if (offset) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != sizeof(signal_strength_interval)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	net_buf_simple_init_with_data(&net_buf, (void *)buf, len);
	signal_strength_interval = net_buf_simple_pull_u8(&net_buf);

	if (IS_ENABLED(CONFIG_BT_GTBS) && attr->user_data == &gtbs_inst) {
		gtbs_inst.signal_strength_interval = signal_strength_interval;
		BT_DBG("GTBS: 0x%02x", signal_strength_interval);
	} else {
		struct tbs_service_inst_t *inst =
			(struct tbs_service_inst_t *)attr->user_data;
		inst->signal_strength_interval = signal_strength_interval;
		BT_DBG("Index %u: 0x%02x",
		       inst->index, signal_strength_interval);
	}

	return len;
}

static void current_calls_cfg_changed(const struct bt_gatt_attr *attr,
				      uint16_t value)
{
	struct tbs_service_inst_t *inst = lookup_instance_by_ccc(attr);

	if (inst) {
		BT_DBG("Index %u: value 0x%04x", inst->index, value);
		inst->notify_current_calls = (value == BT_GATT_CCC_NOTIFY);
	} else if (IS_ENABLED(CONFIG_BT_GTBS)) {
		BT_DBG("GTBS: value 0x%04x", value);
		gtbs_inst.notify_current_calls = (value == BT_GATT_CCC_NOTIFY);
	}
}

static ssize_t read_current_calls(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr,
				  void *buf, uint16_t len, uint16_t offset)
{
	net_buf_put_current_calls(attr->user_data);

	if (IS_ENABLED(CONFIG_BT_GTBS) && attr->user_data == &gtbs_inst) {
		BT_DBG("GTBS");
	} else {
		struct tbs_service_inst_t *inst =
			(struct tbs_service_inst_t *)attr->user_data;
		BT_DBG("Index %u", inst->index);
	}

	if (offset == 0) {
		BT_HEXDUMP_DBG(read_buf.data, read_buf.len, "Current calls");
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 read_buf.data, read_buf.len);
}

static ssize_t read_ccid(struct bt_conn *conn,
			 const struct bt_gatt_attr *attr,
			 void *buf, uint16_t len, uint16_t offset)
{
	uint8_t ccid;

	if (IS_ENABLED(CONFIG_BT_GTBS) && attr->user_data == &gtbs_inst) {
		ccid = gtbs_inst.ccid;
		BT_DBG("GTBS: CCID 0x%02X", ccid);
	} else {
		struct tbs_service_inst_t *inst =
			(struct tbs_service_inst_t *)attr->user_data;
		ccid = inst->ccid;
		BT_DBG("Index %u: CCID 0x%02X", inst->index, ccid);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &ccid, sizeof(ccid));
}

static ssize_t read_status_flags(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr,
				 void *buf, uint16_t len, uint16_t offset)
{
	uint16_t status_flags;

	if (IS_ENABLED(CONFIG_BT_GTBS) && attr->user_data == &gtbs_inst) {
		status_flags = gtbs_inst.status_flags;
		BT_DBG("GTBS: status_flags 0x%04X", status_flags);
	} else {
		struct tbs_service_inst_t *inst =
			(struct tbs_service_inst_t *)attr->user_data;
		status_flags = inst->status_flags;
		BT_DBG("Index %u: status_flags 0x%04X",
		       inst->index, status_flags);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &status_flags, sizeof(status_flags));
}

static void status_flags_cfg_changed(const struct bt_gatt_attr *attr,
					   uint16_t value)
{
	struct tbs_service_inst_t *inst = lookup_instance_by_ccc(attr);

	if (inst) {
		BT_DBG("Index %u: value 0x%04x", inst->index, value);
	} else if (IS_ENABLED(CONFIG_BT_GTBS)) {
		BT_DBG("GTBS: value 0x%04x", value);
	}
}

static ssize_t read_incoming_uri(struct bt_conn *conn,
					    const struct bt_gatt_attr *attr,
					    void *buf, uint16_t len,
					    uint16_t offset)
{
	struct bt_tbs_in_uri_t *inc_call_target;
	size_t val_len;

	if (IS_ENABLED(CONFIG_BT_GTBS) && attr->user_data == &gtbs_inst) {
		inc_call_target = &gtbs_inst.incoming_uri;
		BT_DBG("GTBS: call index 0x%02X, URI %s",
		       inc_call_target->call_index,
		       log_strdup(inc_call_target->uri));
	} else {
		struct tbs_service_inst_t *inst =
			(struct tbs_service_inst_t *)attr->user_data;
		inc_call_target = &inst->incoming_uri;
		BT_DBG("Index %u: call index 0x%02X, URI %s",
		       inst->index, inc_call_target->call_index,
		       log_strdup(inc_call_target->uri));
	}

	if (!inc_call_target->call_index) {
		BT_DBG("URI not set");
		return bt_gatt_attr_read(conn, attr, buf, len, offset, NULL, 0);
	}

	val_len = sizeof(inc_call_target->call_index) +
			strlen(inc_call_target->uri);

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 inc_call_target, val_len);
}

static void incoming_uri_cfg_changed(const struct bt_gatt_attr *attr,
						uint16_t value)
{
	struct tbs_service_inst_t *inst = lookup_instance_by_ccc(attr);

	if (inst) {
		BT_DBG("Index %u: value 0x%04x", inst->index, value);
	} else if (IS_ENABLED(CONFIG_BT_GTBS)) {
		BT_DBG("GTBS: value 0x%04x", value);
	}
}

static ssize_t read_call_state(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr,
			       void *buf, uint16_t len, uint16_t offset)
{
	net_buf_put_call_state(attr->user_data);

	if (IS_ENABLED(CONFIG_BT_GTBS) && attr->user_data == &gtbs_inst) {
		BT_DBG("GTBS");
	} else {
		struct tbs_service_inst_t *inst =
			(struct tbs_service_inst_t *)attr->user_data;
		BT_DBG("Index %u", inst->index);
	}

	if (offset == 0) {
		BT_HEXDUMP_DBG(read_buf.data, read_buf.len, "Call state");
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 read_buf.data, read_buf.len);
}

static void call_state_cfg_changed(const struct bt_gatt_attr *attr,
				   uint16_t value)
{
	struct tbs_service_inst_t *inst = lookup_instance_by_ccc(attr);

	if (inst) {
		BT_DBG("Index %u: value 0x%04x", inst->index, value);
		inst->notify_call_states = (value == BT_GATT_CCC_NOTIFY);
	} else if (IS_ENABLED(CONFIG_BT_GTBS)) {
		BT_DBG("GTBS: value 0x%04x", value);
		gtbs_inst.notify_call_states = (value == BT_GATT_CCC_NOTIFY);
	}
}

static int notify_ccp(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		      uint8_t call_index, uint8_t opcode, uint8_t status)
{
	struct bt_tbs_call_cp_not_t ccp_not = {
		.call_index = call_index,
		.opcode = opcode,
		.status = status
	};

	BT_DBG("Notifying CCP: Call index %u, %s opcode and status %s",
	       call_index, tbs_opcode_str(opcode), tbs_status_str(status));

	return bt_gatt_notify(conn, attr, &ccp_not, sizeof(ccp_not));
}

static void hold_other_calls(struct tbs_service_inst_t *inst,
			     uint8_t call_index_cnt, uint8_t *call_indexes)
{
	uint8_t call_state;
	bool hold_call;

	held_calls_cnt = 0;

	for (int i = 0; i < ARRAY_SIZE(inst->calls); i++) {
		hold_call = true;
		for (int j = 0; j < call_index_cnt; j++) {
			if (inst->calls[i].index == call_indexes[j]) {
				hold_call = false;
				break;
			}
		}

		if (!hold_call) {
			continue;
		}

		call_state = inst->calls[i].state;
		if (call_state == BT_CCP_CALL_STATE_ACTIVE) {
			inst->calls[i].state = BT_CCP_CALL_STATE_LOCALLY_HELD;
			held_calls[held_calls_cnt++] = &inst->calls[i];
		} else if (call_state == BT_CCP_CALL_STATE_REMOTELY_HELD) {
			inst->calls[i].state =
				BT_CCP_CALL_STATE_LOCALLY_AND_REMOTELY_HELD;
			held_calls[held_calls_cnt++] = &inst->calls[i];
		}
	}
}

static uint8_t accept_call(struct tbs_service_inst_t *inst,
			   struct bt_tbs_call_cp_acc_t *ccp)
{
	struct bt_tbs_call_t *call =
		lookup_call_in_instance(inst, ccp->call_index);

	if (!call) {
		return BT_CCP_RESULT_CODE_INVALID_CALL_INDEX;
	}

	if (call->state == BT_CCP_CALL_STATE_INCOMING) {
		call->state = BT_CCP_CALL_STATE_ACTIVE;

		hold_other_calls(inst, 1, &ccp->call_index);

		return BT_CCP_RESULT_CODE_SUCCESS;
	} else {
		return BT_CCP_RESULT_CODE_STATE_MISMATCH;
	}
}

static uint8_t terminate_call(struct tbs_service_inst_t *inst,
			      struct bt_tbs_call_cp_term_t *ccp,
			      uint8_t reason)
{
	struct bt_tbs_call_t *call =
		lookup_call_in_instance(inst, ccp->call_index);

	if (!call) {
		return BT_CCP_RESULT_CODE_INVALID_CALL_INDEX;
	}

	call->index = BT_TBS_FREE_CALL_INDEX;
	tbs_set_terminate_reason(inst, ccp->call_index, reason);
	return BT_CCP_RESULT_CODE_SUCCESS;
}

static uint8_t hold_call(struct tbs_service_inst_t *inst,
			 struct bt_tbs_call_cp_hold_t *ccp)
{
	struct bt_tbs_call_t *call =
		lookup_call_in_instance(inst, ccp->call_index);

	if ((inst->optional_opcodes & BT_CCP_FEATURE_HOLD) == 0) {
		return BT_CCP_RESULT_CODE_OPCODE_NOT_SUPPORTED;
	}

	if (!call) {
		return BT_CCP_RESULT_CODE_INVALID_CALL_INDEX;
	}

	if (call->state == BT_CCP_CALL_STATE_ACTIVE) {
		call->state = BT_CCP_CALL_STATE_LOCALLY_HELD;
	} else if (call->state == BT_CCP_CALL_STATE_REMOTELY_HELD) {
		call->state = BT_CCP_CALL_STATE_LOCALLY_AND_REMOTELY_HELD;
	} else if (call->state == BT_CCP_CALL_STATE_INCOMING) {
		call->state = BT_CCP_CALL_STATE_LOCALLY_HELD;
	} else {
		return BT_CCP_RESULT_CODE_STATE_MISMATCH;
	}

	return BT_CCP_RESULT_CODE_SUCCESS;
}

static uint8_t retrieve_call(struct tbs_service_inst_t *inst,
			     struct bt_tbs_call_cp_retr_t *ccp)
{
	struct bt_tbs_call_t *call =
		lookup_call_in_instance(inst, ccp->call_index);

	if ((inst->optional_opcodes & BT_CCP_FEATURE_HOLD) == 0) {
		return BT_CCP_RESULT_CODE_OPCODE_NOT_SUPPORTED;
	}

	if (!call) {
		return BT_CCP_RESULT_CODE_INVALID_CALL_INDEX;
	}

	if (call->state == BT_CCP_CALL_STATE_LOCALLY_HELD) {
		call->state = BT_CCP_CALL_STATE_ACTIVE;
	} else if (call->state == BT_CCP_CALL_STATE_LOCALLY_AND_REMOTELY_HELD) {
		call->state = BT_CCP_CALL_STATE_REMOTELY_HELD;
	} else {
		return BT_CCP_RESULT_CODE_STATE_MISMATCH;
	}

	hold_other_calls(inst, 1, &ccp->call_index);

	return BT_CCP_RESULT_CODE_SUCCESS;
}

static int originate_call(struct tbs_service_inst_t *inst,
			  struct bt_tbs_call_cp_originate_t *ccp,
			  uint16_t uri_len, uint8_t *call_index)
{
	struct bt_tbs_call_t *call = NULL;

	/* New call - Look for unused call item */
	for (int i = 0; i < CONFIG_BT_TBS_MAX_CALLS; i++) {
		if (inst->calls[i].index == BT_TBS_FREE_CALL_INDEX) {
			call = &inst->calls[i];
			break;
		}
	}

	/* Only allow one active outgoing call */
	for (int i = 0; i < CONFIG_BT_TBS_MAX_CALLS; i++) {
		if (inst->calls[i].state == BT_CCP_CALL_STATE_ALERTING) {
			return BT_CCP_RESULT_CODE_OPERATION_NOT_POSSIBLE;
		}
	}

	if (!call) {
		return BT_CCP_RESULT_CODE_OUT_OF_RESOURCES;
	}

	call->index = next_free_call_index();

	if (call->index == BT_TBS_FREE_CALL_INDEX) {
		return BT_CCP_RESULT_CODE_OUT_OF_RESOURCES;
	}

	if (uri_len == 0 || uri_len > CONFIG_BT_TBS_MAX_URI_LENGTH) {
		call_index = BT_TBS_FREE_CALL_INDEX;
		return BT_CCP_RESULT_CODE_INVALID_URI;
	}

	memcpy(call->remote_uri, ccp->uri, uri_len);
	call->remote_uri[uri_len] = '\0';
	if (!tbs_valid_uri(call->remote_uri)) {
		BT_DBG("Invalid URI: %s", log_strdup(call->remote_uri));
		call_index = BT_TBS_FREE_CALL_INDEX;
		return BT_CCP_RESULT_CODE_INVALID_URI;
	}

	/* We need to notify dialing state for test,
	 * even though we don't have an internal dialing state.
	 */
	call->state = BT_CCP_CALL_STATE_DIALING;
	if (call_index) {
		*call_index = call->index;
	}
	BT_CCP_CALL_FLAG_SET_OUTGOING(call->flags);

	hold_other_calls(inst, 1, &call->index);
	notify_calls(inst);

	BT_DBG("New call with call index %u", call->index);

	return BT_CCP_RESULT_CODE_SUCCESS;
}

static uint8_t join_calls(struct tbs_service_inst_t *inst,
			  struct bt_tbs_call_cp_join_t *ccp,
			  uint16_t call_index_cnt)
{
	struct bt_tbs_call_t *joined_calls[CONFIG_BT_TBS_MAX_CALLS];
	uint8_t call_state;

	if ((inst->optional_opcodes & BT_CCP_FEATURE_JOIN) == 0) {
		return BT_CCP_RESULT_CODE_OPCODE_NOT_SUPPORTED;
	}

	/* Check length */
	if (call_index_cnt < 2 || call_index_cnt > CONFIG_BT_TBS_MAX_CALLS) {
		return BT_CCP_RESULT_CODE_OPERATION_NOT_POSSIBLE;
	}

	/* Check for duplicates */
	for (int i = 0; i < call_index_cnt; i++) {
		for (int j = 0; j < i; j++) {
			if (ccp->call_indexes[i] == ccp->call_indexes[j]) {
				return BT_CCP_RESULT_CODE_INVALID_CALL_INDEX;
			}
		}
	}

	/* Validate that all calls are in a joinable state */
	for (int i = 0; i < call_index_cnt; i++) {
		joined_calls[i] = lookup_call_in_instance(inst,
							  ccp->call_indexes[i]);
		if (!joined_calls[i]) {
			return BT_CCP_RESULT_CODE_INVALID_CALL_INDEX;
		}

		call_state = joined_calls[i]->state;

		if (call_state == BT_CCP_CALL_STATE_INCOMING) {
			return BT_CCP_RESULT_CODE_OPERATION_NOT_POSSIBLE;
		}

		if (call_state != BT_CCP_CALL_STATE_LOCALLY_HELD &&
		    call_state != BT_CCP_CALL_STATE_LOCALLY_AND_REMOTELY_HELD &&
		    call_state != BT_CCP_CALL_STATE_ACTIVE) {
			return BT_CCP_RESULT_CODE_STATE_MISMATCH;
		}
	}

	/* Join all calls */
	for (int i = 0; i < call_index_cnt; i++) {
		call_state = joined_calls[i]->state;

		if (call_state == BT_CCP_CALL_STATE_LOCALLY_HELD) {
			joined_calls[i]->state = BT_CCP_CALL_STATE_ACTIVE;
		} else if (call_state ==
				BT_CCP_CALL_STATE_LOCALLY_AND_REMOTELY_HELD) {
			joined_calls[i]->state =
				BT_CCP_CALL_STATE_REMOTELY_HELD;
		} else if (call_state == BT_CCP_CALL_STATE_INCOMING) {
			joined_calls[i]->state = BT_CCP_CALL_STATE_ACTIVE;
		}
		/* else active => Do nothing */
	}

	hold_other_calls(inst, call_index_cnt, ccp->call_indexes);

	return BT_CCP_RESULT_CODE_SUCCESS;
}

static void notify_app(struct bt_conn *conn, uint16_t len,
		       union bt_tbs_call_cp_t *ccp, uint8_t status,
		       uint8_t call_index)
{
	if (!tbs_cbs) {
		return;
	}

	switch (ccp->opcode) {
	case BT_TBS_CALL_OPCODE_ACCEPT:
		if (tbs_cbs->accept_call) {
			tbs_cbs->accept_call(conn, call_index);
		}
		break;
	case BT_TBS_CALL_OPCODE_TERMINATE:
		if (tbs_cbs->terminate_call) {
			struct tbs_service_inst_t *inst =
				lookup_instance_by_call_index(
					ccp->terminate.call_index);
			tbs_cbs->terminate_call(conn, call_index,
						inst->terminate_reason.reason);
		}
		break;
	case BT_TBS_CALL_OPCODE_HOLD:
		if (tbs_cbs->hold_call) {
			tbs_cbs->hold_call(conn, call_index);
		}
		break;
	case BT_TBS_CALL_OPCODE_RETRIEVE:
		if (tbs_cbs->retrieve_call) {
			tbs_cbs->retrieve_call(conn, call_index);
		}
		break;
	case BT_TBS_CALL_OPCODE_ORIGINATE:
	{
		char uri[CONFIG_BT_TBS_MAX_URI_LENGTH + 1];
		uint16_t uri_len = len - sizeof(ccp->originate);
		bool remote_party_alerted = false;
		struct bt_tbs_call_t *call;
		struct tbs_service_inst_t *inst;

		inst = lookup_instance_by_call_index(call_index);

		if (!inst) {
			BT_DBG("Could not find instance by call index 0x%02X",
			       call_index);
			break;
		}

		call = lookup_call_in_instance(inst, call_index);

		if (!call) {
			BT_DBG("Could not find call by call index 0x%02X",
			       call_index);
			break;
		}

		memcpy(uri, ccp->originate.uri, uri_len);
		uri[uri_len] = '\0';
		if (tbs_cbs->originate_call) {
			remote_party_alerted =
				tbs_cbs->originate_call(conn, call_index, uri);
		}

		if (remote_party_alerted) {
			call->state = BT_CCP_CALL_STATE_ALERTING;
		} else {
			struct bt_tbs_call_cp_term_t term = {
				.call_index = call_index,
				.opcode = BT_TBS_CALL_OPCODE_TERMINATE
			};

			/* Terminate and remove call */
			terminate_call(inst, &term,
				       BT_CCP_REASON_CALL_FAILED);
		}
		notify_calls(inst);

		break;
	}
	case BT_TBS_CALL_OPCODE_JOIN:
	{
		uint16_t call_index_cnt = len - sizeof(ccp->join);

		/* Let the app know about joined calls */
		if (tbs_cbs->join_calls) {
			tbs_cbs->join_calls(conn, call_index_cnt,
					    ccp->join.call_indexes);
		}
		break;
	}
	default:
		break;
	}

	/* Let the app know about held calls */
	if (held_calls_cnt && tbs_cbs->hold_call) {
		for (int i = 0; i < held_calls_cnt; i++) {
			tbs_cbs->hold_call(conn, held_calls[i]->index);
		}
	}
}

static ssize_t write_call_cp(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     const void *buf, uint16_t len,
			     uint16_t offset, uint8_t flags)
{
	struct tbs_service_inst_t *inst = NULL;
	union bt_tbs_call_cp_t *ccp = (union bt_tbs_call_cp_t *)buf;
	uint8_t status;
	uint8_t call_index = 0;
	bool is_gtbs = IS_ENABLED(CONFIG_BT_GTBS) &&
			attr->user_data == &gtbs_inst;

	if (!is_authorized(conn)) {
		return BT_GATT_ERR(BT_ATT_ERR_AUTHORIZATION);
	}

	if (offset) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len < sizeof(ccp->opcode)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (is_gtbs) {
		BT_DBG("GTBS: Processing the %s opcode",
		       tbs_opcode_str(ccp->opcode));
	} else {
		inst = (struct tbs_service_inst_t *)attr->user_data;
		BT_DBG("Index %u: Processing the %s opcode",
		       inst->index, tbs_opcode_str(ccp->opcode));
	}

	switch (ccp->opcode) {
	case BT_TBS_CALL_OPCODE_ACCEPT:
		if (len != sizeof(ccp->accept)) {
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}

		call_index = ccp->accept.call_index;

		if (is_gtbs) {
			inst = lookup_instance_by_call_index(call_index);
			if (!inst) {
				status = BT_CCP_RESULT_CODE_INVALID_CALL_INDEX;
				break;
			}
		}

		status = accept_call(inst, &ccp->accept);
		break;
	case BT_TBS_CALL_OPCODE_TERMINATE:
		if (len != sizeof(ccp->terminate)) {
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}

		call_index = ccp->terminate.call_index;

		if (is_gtbs) {
			inst = lookup_instance_by_call_index(call_index);
			if (!inst) {
				status = BT_CCP_RESULT_CODE_INVALID_CALL_INDEX;
				break;
			}
		}

		status = terminate_call(inst, &ccp->terminate,
					BT_CCP_REASON_CLIENT_TERMINATED);
		break;
	case BT_TBS_CALL_OPCODE_HOLD:
		if (len != sizeof(ccp->hold)) {
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}

		call_index = ccp->hold.call_index;

		if (is_gtbs) {
			inst = lookup_instance_by_call_index(call_index);
			if (!inst) {
				status = BT_CCP_RESULT_CODE_INVALID_CALL_INDEX;
				break;
			}
		}

		status = hold_call(inst, &ccp->hold);
		break;
	case BT_TBS_CALL_OPCODE_RETRIEVE:
		if (len != sizeof(ccp->retrieve)) {
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}

		call_index = ccp->retrieve.call_index;

		if (is_gtbs) {
			inst = lookup_instance_by_call_index(call_index);
			if (!inst) {
				status = BT_CCP_RESULT_CODE_INVALID_CALL_INDEX;
				break;
			}
		}

		status = retrieve_call(inst, &ccp->retrieve);
		break;
	case BT_TBS_CALL_OPCODE_ORIGINATE:
	{
		uint16_t uri_len = len - sizeof(ccp->originate);

		if (len < sizeof(ccp->originate) + BT_TBS_MIN_URI_LEN) {
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}

		if (is_gtbs) {
			inst = find_tbs_by_uri_scheme(ccp->originate.uri,
						      uri_len);
			if (!inst) {
				/* TODO: Couldn't find fitting TBS instance;
				 * use the first. If we want to be
				 * restrictive about URIs, return
				 * Invalid Caller ID instead
				 */
				inst = &svc_insts[0];
			}
		}

		status = originate_call(inst, &ccp->originate, uri_len,
					&call_index);
		break;
	}
	case BT_TBS_CALL_OPCODE_JOIN:
	{
		uint16_t call_index_cnt = len - sizeof(ccp->join);

		if (len < sizeof(ccp->join) + 1) { /* at least 1 call index */
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}

		call_index = ccp->join.call_indexes[0];

		if (is_gtbs) {
			inst = lookup_instance_by_call_index(call_index);
			if (!inst) {
				status = BT_CCP_RESULT_CODE_INVALID_CALL_INDEX;
				break;
			}
		}

		status = join_calls(inst, &ccp->join, call_index_cnt);
		break;
	}
	default:
		status = BT_CCP_RESULT_CODE_OPCODE_NOT_SUPPORTED;
		call_index = 0;
		break;
	}

	if (inst) {
		if (is_gtbs) {
			BT_DBG("GTBS: Processed the %s opcode with status %s "
			       "for call index %u",
			       tbs_opcode_str(ccp->opcode),
			       tbs_status_str(status),
			       call_index);
		} else {
			BT_DBG("Index %u: Processed the %s opcode with status "
			       "%s for call index %u",
			       inst->index,
			       tbs_opcode_str(ccp->opcode),
			       tbs_status_str(status),
			       call_index);
		}
		if (status == BT_CCP_RESULT_CODE_SUCCESS) {
			struct bt_tbs_call_t *call = lookup_call(call_index);
			if (call) {
				BT_DBG("Call is now in the %s state",
				       tbs_state_str(call->state));
			} else {
				BT_DBG("Call is now terminated");
			}
		}
	}

	if (status != BT_CCP_RESULT_CODE_SUCCESS) {
		call_index = 0;
	}

	if (conn) {
		notify_ccp(conn, attr, call_index, ccp->opcode, status);
	} /* else local operation; don't notify */

	if (status == BT_CCP_RESULT_CODE_SUCCESS) {
		notify_calls(inst);
		notify_app(conn, len, ccp, status, call_index);
	}

	return len;
}

static void call_cp_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	struct tbs_service_inst_t *inst = lookup_instance_by_ccc(attr);

	if (inst) {
		BT_DBG("Index %u: value 0x%04x", inst->index, value);
	} else if (IS_ENABLED(CONFIG_BT_GTBS)) {
		BT_DBG("GTBS: value 0x%04x", value);
	}
}


static ssize_t read_optional_opcodes(struct bt_conn *conn,
				      const struct bt_gatt_attr *attr,
				      void *buf, uint16_t len, uint16_t offset)
{
	uint16_t optional_opcodes;

	if (IS_ENABLED(CONFIG_BT_GTBS) && attr->user_data == &gtbs_inst) {
		optional_opcodes = gtbs_inst.optional_opcodes;
		BT_DBG("GTBS: Supported opcodes 0x%02x", optional_opcodes);
	} else {
		struct tbs_service_inst_t *inst =
			(struct tbs_service_inst_t *)attr->user_data;
		optional_opcodes = inst->optional_opcodes;
		BT_DBG("Index %u: Supported opcodes 0x%02x",
			inst->index, optional_opcodes);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &optional_opcodes, sizeof(optional_opcodes));
}

static void terminate_reason_cfg_changed(const struct bt_gatt_attr *attr,
					 uint16_t value)
{
	struct tbs_service_inst_t *inst = lookup_instance_by_ccc(attr);

	if (inst) {
		BT_DBG("Index %u: value 0x%04x", inst->index, value);
	} else if (IS_ENABLED(CONFIG_BT_GTBS)) {
		BT_DBG("GTBS: value 0x%04x", value);
	}
}

static ssize_t read_friendly_name(struct bt_conn *conn,
				     const struct bt_gatt_attr *attr,
				     void *buf, uint16_t len, uint16_t offset)
{
	struct bt_tbs_in_uri_t *friendly_name;
	size_t val_len;

	if (IS_ENABLED(CONFIG_BT_GTBS) && attr->user_data == &gtbs_inst) {
		friendly_name = &gtbs_inst.friendly_name;
		BT_DBG("GTBS: call index 0x%02X, URI %s",
		       friendly_name->call_index,
		       log_strdup(friendly_name->uri));
	} else {
		struct tbs_service_inst_t *inst =
			(struct tbs_service_inst_t *)attr->user_data;
		friendly_name = &inst->friendly_name;
		BT_DBG("Index %u: call index 0x%02X, URI %s",
		       inst->index, friendly_name->call_index,
		       log_strdup(friendly_name->uri));
	}

	if (!friendly_name->call_index) {
		BT_DBG("URI not set");
		return bt_gatt_attr_read(conn, attr, buf, len, offset, NULL, 0);
	}

	val_len = sizeof(friendly_name->call_index) +
			strlen(friendly_name->uri);

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 friendly_name, val_len);
}

static void friendly_name_cfg_changed(const struct bt_gatt_attr *attr,
					 uint16_t value)
{
	struct tbs_service_inst_t *inst = lookup_instance_by_ccc(attr);

	if (inst) {
		BT_DBG("Index %u: value 0x%04x", inst->index, value);
	} else if (IS_ENABLED(CONFIG_BT_GTBS)) {
		BT_DBG("GTBS: value 0x%04x", value);
	}
}

static ssize_t read_in_call(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr,
				  void *buf, uint16_t len, uint16_t offset)
{
	struct bt_tbs_in_uri_t *remote_uri;
	size_t val_len;

	if (IS_ENABLED(CONFIG_BT_GTBS) && attr->user_data == &gtbs_inst) {
		remote_uri = &gtbs_inst.in_call;
		BT_DBG("GTBS: call index 0x%02X, URI %s",
		       remote_uri->call_index, log_strdup(remote_uri->uri));
	} else {
		struct tbs_service_inst_t *inst =
			(struct tbs_service_inst_t *)attr->user_data;
		remote_uri = &inst->in_call;
		BT_DBG("Index %u: call index 0x%02X, URI %s",
		       inst->index, remote_uri->call_index,
		       log_strdup(remote_uri->uri));
	}

	if (!remote_uri->call_index) {
		BT_DBG("URI not set");
		return bt_gatt_attr_read(conn, attr, buf, len, offset, NULL, 0);
	}

	val_len = sizeof(remote_uri->call_index) + strlen(remote_uri->uri);

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 remote_uri, val_len);
}

static void in_call_cfg_changed(const struct bt_gatt_attr *attr,
				      uint16_t value)
{
	struct tbs_service_inst_t *inst = lookup_instance_by_ccc(attr);

	if (inst) {
		BT_DBG("Index %u: value 0x%04x", inst->index, value);
	} else if (IS_ENABLED(CONFIG_BT_GTBS)) {
		BT_DBG("GTBS: value 0x%04x", value);
	}
}

#define BT_TBS_CHR_PROVIDER_NAME(inst) \
	BT_GATT_CHARACTERISTIC(BT_UUID_TBS_PROVIDER_NAME, \
			BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
			BT_GATT_PERM_READ_ENCRYPT, \
			read_provider_name, NULL, inst), \
	BT_GATT_CCC(provider_name_cfg_changed, \
			BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT)

#define BT_TBS_CHR_UCI(inst) \
	BT_GATT_CHARACTERISTIC(BT_UUID_TBS_UCI, \
			BT_GATT_CHRC_READ, \
			BT_GATT_PERM_READ_ENCRYPT, \
			read_uci, NULL, inst)

#define BT_TBS_CHR_TECHNOLOGY(inst) \
	BT_GATT_CHARACTERISTIC(BT_UUID_TBS_TECHNOLOGY, \
			BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
			BT_GATT_PERM_READ_ENCRYPT, \
			read_technology, NULL, inst), \
	BT_GATT_CCC(technology_cfg_changed, \
			BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT)

#define BT_TBS_CHR_URI_LIST(inst) \
	BT_GATT_CHARACTERISTIC(BT_UUID_TBS_URI_LIST, \
			BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
			BT_GATT_PERM_READ_ENCRYPT, \
			read_uri_scheme_list, NULL, inst), \
	BT_GATT_CCC(uri_scheme_list_cfg_changed, \
			BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT)

#define BT_TBS_CHR_SIGNAL_STRENGTH(inst) \
	BT_GATT_CHARACTERISTIC(BT_UUID_TBS_SIGNAL_STRENGTH, /* Optional */ \
			BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
			BT_GATT_PERM_READ_ENCRYPT, \
			read_signal_strength, NULL, inst), \
	BT_GATT_CCC(signal_strength_cfg_changed, \
			BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT)

#define BT_TBS_CHR_SIGNAL_INTERVAL(inst) \
	BT_GATT_CHARACTERISTIC(BT_UUID_TBS_SIGNAL_INTERVAL, /* Conditional */ \
			BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | \
				BT_GATT_CHRC_WRITE_WITHOUT_RESP, \
			BT_GATT_PERM_READ_ENCRYPT | \
				BT_GATT_PERM_WRITE_ENCRYPT, \
			read_signal_strength_interval,  \
			write_signal_strength_interval, inst)

#define BT_TBS_CHR_CURRENT_CALLS(inst) \
	BT_GATT_CHARACTERISTIC(BT_UUID_TBS_LIST_CURRENT_CALLS, \
			BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
			BT_GATT_PERM_READ_ENCRYPT, \
			read_current_calls, NULL, inst), \
	BT_GATT_CCC(current_calls_cfg_changed, \
			BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT)

#define BT_TBS_CHR_CCID(inst) \
	BT_GATT_CHARACTERISTIC(BT_UUID_CCID, \
			BT_GATT_CHRC_READ, \
			BT_GATT_PERM_READ_ENCRYPT, \
			read_ccid, NULL, inst)

#define BT_TBS_CHR_STATUS_FLAGS(inst) \
	BT_GATT_CHARACTERISTIC(BT_UUID_TBS_STATUS_FLAGS, \
			BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
			BT_GATT_PERM_READ_ENCRYPT, \
			read_status_flags, NULL, inst), \
	BT_GATT_CCC(status_flags_cfg_changed, \
			BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT)

#define BT_TBS_CHR_INCOMING_URI(inst) \
	BT_GATT_CHARACTERISTIC(BT_UUID_TBS_INCOMING_URI, /* Optional */ \
			BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
			BT_GATT_PERM_READ_ENCRYPT, \
			read_incoming_uri, NULL, inst), \
	BT_GATT_CCC(incoming_uri_cfg_changed, \
			BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT)

#define BT_TBS_CHR_CALL_STATE(inst) \
	BT_GATT_CHARACTERISTIC(BT_UUID_TBS_CALL_STATE, \
			BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
			BT_GATT_PERM_READ_ENCRYPT, \
			read_call_state, NULL, inst), \
	BT_GATT_CCC(call_state_cfg_changed, \
			BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT)

#define BT_TBS_CHR_CONTROL_POINT(inst) \
	BT_GATT_CHARACTERISTIC(BT_UUID_TBS_CALL_CONTROL_POINT, \
			BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY | \
				BT_GATT_CHRC_WRITE_WITHOUT_RESP, \
			BT_GATT_PERM_WRITE_ENCRYPT, \
			NULL, write_call_cp, inst), \
	BT_GATT_CCC(call_cp_cfg_changed, \
			BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT)

#define BT_TBS_CHR_OPTIONAL_OPCODES(inst) \
	BT_GATT_CHARACTERISTIC(BT_UUID_TBS_OPTIONAL_OPCODES, \
			BT_GATT_CHRC_READ, \
			BT_GATT_PERM_READ_ENCRYPT, \
			read_optional_opcodes, NULL, inst) \

#define BT_TBS_CHR_TERMINATE_REASON(inst) \
	BT_GATT_CHARACTERISTIC(BT_UUID_TBS_TERMINATE_REASON, \
			BT_GATT_CHRC_NOTIFY, \
			BT_GATT_PERM_READ_ENCRYPT, \
			NULL, NULL, inst), \
	BT_GATT_CCC(terminate_reason_cfg_changed, \
			BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT)

#define BT_TBS_CHR_INCOMING_CALL(inst) \
	BT_GATT_CHARACTERISTIC(BT_UUID_TBS_INCOMING_CALL, \
			BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
			BT_GATT_PERM_READ_ENCRYPT, \
			read_in_call, NULL, inst), \
	BT_GATT_CCC(in_call_cfg_changed, \
			BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT)

#define BT_TBS_CHR_FRIENDLY_NAME(inst) \
	BT_GATT_CHARACTERISTIC(BT_UUID_TBS_FRIENDLY_NAME, /* Optional */ \
			BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
			BT_GATT_PERM_READ_ENCRYPT, \
			read_friendly_name, NULL, inst), \
	BT_GATT_CCC(friendly_name_cfg_changed, \
			BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT)

#define BT_TBS_SERVICE_DEFINITION(inst) \
	BT_GATT_PRIMARY_SERVICE(BT_UUID_TBS), \
	BT_TBS_CHR_PROVIDER_NAME(inst), \
	BT_TBS_CHR_UCI(inst), \
	BT_TBS_CHR_TECHNOLOGY(inst), \
	BT_TBS_CHR_URI_LIST(inst), \
	BT_TBS_CHR_SIGNAL_STRENGTH(inst), \
	BT_TBS_CHR_SIGNAL_INTERVAL(inst), \
	BT_TBS_CHR_CURRENT_CALLS(inst), \
	BT_TBS_CHR_CCID(inst), \
	BT_TBS_CHR_STATUS_FLAGS(inst), \
	BT_TBS_CHR_INCOMING_URI(inst), \
	BT_TBS_CHR_CALL_STATE(inst), \
	BT_TBS_CHR_CONTROL_POINT(inst), \
	BT_TBS_CHR_OPTIONAL_OPCODES(inst), \
	BT_TBS_CHR_TERMINATE_REASON(inst), \
	BT_TBS_CHR_INCOMING_CALL(inst), \
	BT_TBS_CHR_FRIENDLY_NAME(inst)


#define BT_GTBS_SERVICE_DEFINITION(inst) \
	BT_GATT_PRIMARY_SERVICE(BT_UUID_GTBS), \
	BT_TBS_CHR_PROVIDER_NAME(inst), \
	BT_TBS_CHR_UCI(inst), \
	BT_TBS_CHR_TECHNOLOGY(inst), \
	BT_TBS_CHR_URI_LIST(inst), \
	BT_TBS_CHR_SIGNAL_STRENGTH(inst), \
	BT_TBS_CHR_SIGNAL_INTERVAL(inst), \
	BT_TBS_CHR_CURRENT_CALLS(inst), \
	BT_TBS_CHR_CCID(inst), \
	BT_TBS_CHR_STATUS_FLAGS(inst), \
	BT_TBS_CHR_INCOMING_URI(inst), \
	BT_TBS_CHR_CALL_STATE(inst), \
	BT_TBS_CHR_CONTROL_POINT(inst), \
	BT_TBS_CHR_OPTIONAL_OPCODES(inst), \
	BT_TBS_CHR_TERMINATE_REASON(inst), \
	BT_TBS_CHR_INCOMING_CALL(inst), \
	BT_TBS_CHR_FRIENDLY_NAME(inst)

/*
 * Defining these as extern make it possible to link code that otherwise would
 * give "unknown identifier" linking errors.
 */
extern const struct bt_gatt_service_static gtbs_svc;
extern const struct bt_gatt_service_static tbs_svc_1;
extern const struct bt_gatt_service_static tbs_svc_2;
extern const struct bt_gatt_service_static tbs_svc_3;

/* TODO: Can we make the multiple service instance more generic? */
#if CONFIG_BT_GTBS
BT_GATT_SERVICE_DEFINE(gtbs_svc, BT_GTBS_SERVICE_DEFINITION(&gtbs_inst));
#endif /* CONFIG_BT_GTBS */
#if CONFIG_BT_TBS_SERVICE_COUNT > 0
BT_GATT_SERVICE_DEFINE(tbs_svc_1, BT_TBS_SERVICE_DEFINITION(&svc_insts[0]));
#if CONFIG_BT_TBS_SERVICE_COUNT > 1
BT_GATT_SERVICE_DEFINE(tbs_svc_2, BT_TBS_SERVICE_DEFINITION(&svc_insts[1]));
#if CONFIG_BT_TBS_SERVICE_COUNT > 2
BT_GATT_SERVICE_DEFINE(tbs_svc_3, BT_TBS_SERVICE_DEFINITION(&svc_insts[2]));
#endif /* CONFIG_BT_TBS_SERVICE_COUNT > 2 */
#endif /* CONFIG_BT_TBS_SERVICE_COUNT > 1 */
#endif /* CONFIG_BT_TBS_SERVICE_COUNT > 0 */

static void signal_interval_timeout(struct k_work *work)
{
	struct tbs_service_inst_t *inst = lookup_instance_by_work(work);

	if (inst && inst->pending_signal_strength_notification) {
		bt_gatt_notify_uuid(NULL, BT_UUID_TBS_SIGNAL_STRENGTH,
				    inst->service_p->attrs,
				    &inst->signal_strength,
				    sizeof(inst->signal_strength));

		if (inst->signal_strength_interval) {
			k_work_reschedule(
				&inst->reporting_interval_work,
				K_SECONDS(inst->signal_strength_interval));
		}
		inst->pending_signal_strength_notification = false;
	} else if (IS_ENABLED(CONFIG_BT_GTBS) &&
		   gtbs_inst.pending_signal_strength_notification) {

		bt_gatt_notify_uuid(NULL, BT_UUID_TBS_SIGNAL_STRENGTH,
				    gtbs_inst.service_p->attrs,
				    &gtbs_inst.signal_strength,
				    sizeof(gtbs_inst.signal_strength));

		if (gtbs_inst.signal_strength_interval) {
			k_work_reschedule(
				&gtbs_inst.reporting_interval_work,
				K_SECONDS(gtbs_inst.signal_strength_interval));
		}
		gtbs_inst.pending_signal_strength_notification = false;
	}
}

static int bt_tbs_init(const struct device *unused)
{
	if (CONFIG_BT_TBS_SERVICE_COUNT > 0) {
		svc_insts[0].service_p = &tbs_svc_1;
		if (CONFIG_BT_TBS_SERVICE_COUNT > 1) {
			svc_insts[1].service_p = &tbs_svc_2;
			if (CONFIG_BT_TBS_SERVICE_COUNT > 2) {
				svc_insts[2].service_p = &tbs_svc_3;
			}
		}
	}

	if (IS_ENABLED(CONFIG_BT_GTBS)) {
		gtbs_inst.service_p = &gtbs_svc;
		strcpy(gtbs_inst.provider_name, "Generic TBS");
		gtbs_inst.optional_opcodes = CONFIG_BT_TBS_SUPPORTED_FEATURES;
		gtbs_inst.ccid = ccid_get_value();
		strcpy(gtbs_inst.uci, "un000");

		k_work_init_delayable(&gtbs_inst.reporting_interval_work,
				    signal_interval_timeout);
	}

	for (int i = 0; i < ARRAY_SIZE(svc_insts); i++) {
		/* Init default values */
		svc_insts[i].index = i;
		svc_insts[i].ccid = ccid_get_value();
		strcpy(svc_insts[i].provider_name, CONFIG_BT_TBS_PROVIDER_NAME);
		strcpy(svc_insts[i].uci, CONFIG_BT_TBS_UCI);
		strcpy(svc_insts[i].uri_scheme_list,
		       CONFIG_BT_TBS_URI_SCHEMES_LIST);
		svc_insts[i].optional_opcodes =
			CONFIG_BT_TBS_SUPPORTED_FEATURES;
		svc_insts[i].technology = CONFIG_BT_TBS_TECHNOLOGY;
		svc_insts[i].signal_strength_interval =
			CONFIG_BT_TBS_SIGNAL_STRENGTH_INTERVAL;
		svc_insts[i].status_flags =
			CONFIG_BT_TBS_STATUS_FLAGS;

		k_work_init_delayable(&svc_insts[i].reporting_interval_work,
				    signal_interval_timeout);
	}

	return 0;
}

DEVICE_DEFINE(bt_tbs, "bt_tbs", &bt_tbs_init, NULL, NULL, NULL,
	      APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);


/***************************** Profile API *****************************/
int bt_tbs_accept(uint8_t call_index)
{
	struct tbs_service_inst_t *inst =
		lookup_instance_by_call_index(call_index);
	int status = -EINVAL;
	struct bt_tbs_call_cp_acc_t ccp = {
		.call_index = call_index,
		.opcode = BT_TBS_CALL_OPCODE_ACCEPT
	};

	if (inst) {
		status = accept_call(inst, &ccp);
	}

	if (status == BT_CCP_RESULT_CODE_SUCCESS) {
		notify_calls(inst);
	}


	return status;
}

int bt_tbs_hold(uint8_t call_index)
{
	struct tbs_service_inst_t *inst =
		lookup_instance_by_call_index(call_index);
	int status = -EINVAL;
	struct bt_tbs_call_cp_hold_t ccp = {
		.call_index = call_index,
		.opcode = BT_TBS_CALL_OPCODE_HOLD
	};

	if (inst) {
		status = hold_call(inst, &ccp);
	}

	return status;
}

int bt_tbs_retrieve(uint8_t call_index)
{
	struct tbs_service_inst_t *inst =
		lookup_instance_by_call_index(call_index);
	int status = -EINVAL;
	struct bt_tbs_call_cp_retr_t ccp = {
		.call_index = call_index,
		.opcode = BT_TBS_CALL_OPCODE_RETRIEVE
	};

	if (inst) {
		status = retrieve_call(inst, &ccp);
	}

	return status;
}

int bt_tbs_terminate(uint8_t call_index)
{
	struct tbs_service_inst_t *inst =
		lookup_instance_by_call_index(call_index);
	int status = -EINVAL;
	struct bt_tbs_call_cp_term_t ccp = {
		.call_index = call_index,
		.opcode = BT_TBS_CALL_OPCODE_TERMINATE
	};

	if (inst) {
		status = terminate_call(inst, &ccp,
					BT_CCP_REASON_SERVER_ENDED_CALL);
	}

	return status;
}

int bt_tbs_originate(uint8_t bearer_idx, char *remote_uri, uint8_t *call_index)
{
	struct tbs_service_inst_t *inst;
	uint8_t buf[CONFIG_BT_TBS_MAX_URI_LENGTH +
		    sizeof(struct bt_tbs_call_cp_originate_t)];
	struct bt_tbs_call_cp_originate_t *ccp =
		(struct bt_tbs_call_cp_originate_t *)buf;
	size_t uri_len;

	if (bearer_idx >= CONFIG_BT_TBS_BEARER_COUNT) {
		return -EINVAL;
	} else if (!tbs_valid_uri(remote_uri)) {
		BT_DBG("Invalid URI %s", log_strdup(remote_uri));
		return -EINVAL;
	}

	uri_len = strlen(remote_uri);

	inst = &svc_insts[bearer_idx];

	ccp->opcode = BT_TBS_CALL_OPCODE_ORIGINATE;
	memcpy(ccp->uri, remote_uri, uri_len);

	return originate_call(inst, ccp, uri_len, call_index);
}

int bt_tbs_join(uint8_t call_index_cnt, uint8_t *call_indexes)
{
	struct tbs_service_inst_t *inst;
	uint8_t buf[CONFIG_BT_TBS_MAX_CALLS +
		 sizeof(struct bt_tbs_call_cp_join_t)];
	struct bt_tbs_call_cp_join_t *ccp = (struct bt_tbs_call_cp_join_t *)buf;
	int status = -EINVAL;

	if (call_index_cnt && call_indexes) {
		inst = lookup_instance_by_call_index(call_indexes[0]);
	} else {
		return status;
	}

	if (inst) {
		ccp->opcode = BT_TBS_CALL_OPCODE_JOIN;
		memcpy(ccp->call_indexes, call_indexes,
		MIN(call_index_cnt, CONFIG_BT_TBS_MAX_CALLS));

		status = join_calls(inst, ccp, call_index_cnt);
	}

	return status;
}

int bt_tbs_remote_answer(uint8_t call_index)
{
	struct tbs_service_inst_t *inst =
		lookup_instance_by_call_index(call_index);
	struct bt_tbs_call_t *call;

	if (!inst) {
		return BT_CCP_RESULT_CODE_INVALID_CALL_INDEX;
	}

	call = lookup_call_in_instance(inst, call_index);

	if (!call) {
		return BT_CCP_RESULT_CODE_INVALID_CALL_INDEX;
	}

	if (call->state == BT_CCP_CALL_STATE_ALERTING) {
		call->state = BT_CCP_CALL_STATE_ACTIVE;
		notify_calls(inst);
		return BT_CCP_RESULT_CODE_SUCCESS;
	} else {
		return BT_CCP_RESULT_CODE_STATE_MISMATCH;
	}
}

int bt_tbs_remote_hold(uint8_t call_index)
{
	struct tbs_service_inst_t *inst =
		lookup_instance_by_call_index(call_index);
	struct bt_tbs_call_t *call;
	uint8_t status;

	if (!inst) {
		return BT_CCP_RESULT_CODE_INVALID_CALL_INDEX;
	}

	call = lookup_call_in_instance(inst, call_index);

	if (!call) {
		return BT_CCP_RESULT_CODE_INVALID_CALL_INDEX;
	}

	if (call->state == BT_CCP_CALL_STATE_ACTIVE) {
		call->state = BT_CCP_CALL_STATE_REMOTELY_HELD;
		status = BT_CCP_RESULT_CODE_SUCCESS;
	} else if (call->state == BT_CCP_CALL_STATE_LOCALLY_HELD) {
		call->state = BT_CCP_CALL_STATE_LOCALLY_AND_REMOTELY_HELD;
		status = BT_CCP_RESULT_CODE_SUCCESS;
	} else {
		status = BT_CCP_RESULT_CODE_STATE_MISMATCH;
	}

	if (status == BT_CCP_RESULT_CODE_SUCCESS) {
		notify_calls(inst);
	}

	return status;
}

int bt_tbs_remote_retrieve(uint8_t call_index)
{
	struct tbs_service_inst_t *inst =
		lookup_instance_by_call_index(call_index);
	struct bt_tbs_call_t *call;
	int status;

	if (!inst) {
		return BT_CCP_RESULT_CODE_INVALID_CALL_INDEX;
	}

	call = lookup_call_in_instance(inst, call_index);

	if (!call) {
		return BT_CCP_RESULT_CODE_INVALID_CALL_INDEX;
	}

	if (call->state == BT_CCP_CALL_STATE_REMOTELY_HELD) {
		call->state = BT_CCP_CALL_STATE_ACTIVE;
		status = BT_CCP_RESULT_CODE_SUCCESS;
	} else if (call->state == BT_CCP_CALL_STATE_LOCALLY_AND_REMOTELY_HELD) {
		call->state = BT_CCP_CALL_STATE_LOCALLY_HELD;
		status = BT_CCP_RESULT_CODE_SUCCESS;
	} else {
		status = BT_CCP_RESULT_CODE_STATE_MISMATCH;
	}

	if (status == BT_CCP_RESULT_CODE_SUCCESS) {
		notify_calls(inst);
	}

	return status;
}

int bt_tbs_remote_terminate(uint8_t call_index)
{
	struct tbs_service_inst_t *inst =
		lookup_instance_by_call_index(call_index);
	int status = -EINVAL;
	struct bt_tbs_call_cp_term_t ccp = {
		.call_index = call_index,
		.opcode = BT_TBS_CALL_OPCODE_TERMINATE
	};

	if (inst) {
		status = terminate_call(inst, &ccp,
					BT_CCP_REASON_REMOTE_ENDED_CALL);
	}

	return status;
}

int bt_tbs_remote_incoming(uint8_t bearer_idx, const char *to, const char *from,
			   const char *friendly_name)
{
	struct tbs_service_inst_t *inst;
	struct bt_tbs_call_t *call = NULL;
	size_t local_uri_ind_len;
	size_t remote_uri_ind_len;
	size_t friend_name_ind_len;

	if (bearer_idx >= CONFIG_BT_TBS_BEARER_COUNT) {
		return -EINVAL;
	} else if (!tbs_valid_uri(to)) {
		BT_DBG("Invalid \"to\" URI: %s", log_strdup(to));
		return -EINVAL;
	} else if (!tbs_valid_uri(from)) {
		BT_DBG("Invalid \"from\" URI: %s", log_strdup(from));
		return -EINVAL;
	}

	local_uri_ind_len = strlen(to) + 1;
	remote_uri_ind_len = strlen(from) + 1;

	inst = &svc_insts[bearer_idx];

	/* New call - Look for unused call item */
	for (int i = 0; i < CONFIG_BT_TBS_MAX_CALLS; i++) {
		if (inst->calls[i].index == BT_TBS_FREE_CALL_INDEX) {
			call = &inst->calls[i];
			break;
		}
	}

	if (!call) {
		return -BT_CCP_RESULT_CODE_OUT_OF_RESOURCES;
	}

	call->index = next_free_call_index();

	if (call->index == BT_TBS_FREE_CALL_INDEX) {
		return -BT_CCP_RESULT_CODE_OUT_OF_RESOURCES;
	}

	BT_CCP_CALL_FLAG_SET_INCOMING(call->flags);

	strcpy(call->remote_uri, from);
	call->state = BT_CCP_CALL_STATE_INCOMING;

	inst->in_call.call_index = call->index;
	strcpy(inst->in_call.uri, from);

	inst->incoming_uri.call_index = call->index;
	strcpy(inst->incoming_uri.uri, to);

	bt_gatt_notify_uuid(NULL, BT_UUID_TBS_INCOMING_URI,
			    inst->service_p->attrs,
			    &inst->incoming_uri, local_uri_ind_len);

	bt_gatt_notify_uuid(NULL, BT_UUID_TBS_INCOMING_CALL,
			    inst->service_p->attrs,
			    &inst->in_call, remote_uri_ind_len);

	if (friendly_name) {
		inst->friendly_name.call_index = call->index;
		strcpy(inst->friendly_name.uri, friendly_name);
		friend_name_ind_len = strlen(from) + 1;

		bt_gatt_notify_uuid(NULL, BT_UUID_TBS_FRIENDLY_NAME,
				    inst->service_p->attrs,
				    &inst->friendly_name,
				    friend_name_ind_len);
	} else {
		inst->friendly_name.call_index = 0;
		bt_gatt_notify_uuid(NULL, BT_UUID_TBS_FRIENDLY_NAME,
				    inst->service_p->attrs, NULL, 0);
	}

	if (IS_ENABLED(CONFIG_BT_GTBS)) {
		gtbs_inst.in_call.call_index = call->index;
		strcpy(gtbs_inst.in_call.uri, from);

		gtbs_inst.incoming_uri.call_index = call->index;
		strcpy(gtbs_inst.incoming_uri.uri, to);

		bt_gatt_notify_uuid(NULL, BT_UUID_TBS_INCOMING_URI,
				    gtbs_inst.service_p->attrs,
				    &gtbs_inst.incoming_uri, local_uri_ind_len);

		bt_gatt_notify_uuid(NULL, BT_UUID_TBS_INCOMING_CALL,
				    gtbs_inst.service_p->attrs,
				    &gtbs_inst.in_call, remote_uri_ind_len);

		if (friendly_name) {
			gtbs_inst.friendly_name.call_index = call->index;
			strcpy(gtbs_inst.friendly_name.uri, friendly_name);
			friend_name_ind_len = strlen(from) + 1;

			bt_gatt_notify_uuid(NULL, BT_UUID_TBS_FRIENDLY_NAME,
					    gtbs_inst.service_p->attrs,
					    &gtbs_inst.friendly_name,
					    friend_name_ind_len);
		} else {
			gtbs_inst.friendly_name.call_index = 0;
			bt_gatt_notify_uuid(NULL, BT_UUID_TBS_FRIENDLY_NAME,
					    gtbs_inst.service_p->attrs,
					    NULL, 0);
		}
	}

	notify_calls(inst);

	BT_DBG("New call with call index %u", call->index);

	return call->index;
}

int bt_tbs_set_bearer_provider_name(uint8_t bearer_idx, const char *name)
{
	const size_t len = strlen(name);
	const struct bt_gatt_attr *attr;

	if (len >= CONFIG_BT_TBS_MAX_PROVIDER_NAME_LENGTH || len == 0) {
		return -EINVAL;
	} else if (bearer_idx >= CONFIG_BT_TBS_BEARER_COUNT) {
		if (!(IS_ENABLED(CONFIG_BT_GTBS) &&
		    bearer_idx == BT_CCP_GTBS_INDEX)) {
			return -EINVAL;
		}
	}

	if (bearer_idx == BT_CCP_GTBS_INDEX) {
		if (strcmp(gtbs_inst.provider_name, name) == 0) {
			return 0;
		}

		strcpy(gtbs_inst.provider_name, name);
		attr = gtbs_inst.service_p->attrs;
	} else {
		if (strcmp(svc_insts[bearer_idx].provider_name, name) == 0) {
			return 0;
		}

		strcpy(svc_insts[bearer_idx].provider_name, name);
		attr = svc_insts[bearer_idx].service_p->attrs;
	}

	bt_gatt_notify_uuid(NULL, BT_UUID_TBS_PROVIDER_NAME,
			    attr, name, strlen(name));
	return 0;
}

int bt_tbs_set_bearer_technology(uint8_t bearer_idx, uint8_t new_technology)
{
	const struct bt_gatt_attr *attr;

	if (new_technology < BT_TBS_TECHNOLOGY_3G ||
	    new_technology > BT_TBS_TECHNOLOGY_IP) {
		return -EINVAL;
	} else if (bearer_idx >= CONFIG_BT_TBS_BEARER_COUNT) {
		if (!(IS_ENABLED(CONFIG_BT_GTBS) &&
		    bearer_idx == BT_CCP_GTBS_INDEX)) {
			return -EINVAL;
		}
	}

	if (bearer_idx == BT_CCP_GTBS_INDEX) {
		if (gtbs_inst.technology == new_technology) {
			return 0;
		}

		gtbs_inst.technology = new_technology;
		attr = gtbs_inst.service_p->attrs;
	} else {
		if (svc_insts[bearer_idx].technology == new_technology) {
			return 0;
		}

		svc_insts[bearer_idx].technology = new_technology;
		attr = svc_insts[bearer_idx].service_p->attrs;
	}

	bt_gatt_notify_uuid(NULL, BT_UUID_TBS_TECHNOLOGY,
			   attr, &new_technology, sizeof(new_technology));

	return 0;
}

int bt_tbs_set_signal_strength(uint8_t bearer_idx, uint8_t new_signal_strength)
{
	const struct bt_gatt_attr *attr;
	uint32_t timer_status;
	uint8_t interval;
	struct k_work_delayable *reporting_interval_work;
	struct tbs_service_inst_t *inst;

	if (new_signal_strength > BT_CCP_SIGNAL_STRENGTH_MAX &&
	    new_signal_strength != BT_CCP_SIGNAL_STRENGTH_UNKNOWN) {
		return -EINVAL;
	} else if (bearer_idx >= CONFIG_BT_TBS_BEARER_COUNT) {
		if (!(IS_ENABLED(CONFIG_BT_GTBS) &&
		    bearer_idx == BT_CCP_GTBS_INDEX)) {
			return -EINVAL;
		}
	}

	if (bearer_idx == BT_CCP_GTBS_INDEX) {
		if (gtbs_inst.signal_strength == new_signal_strength) {
			return 0;
		}

		gtbs_inst.signal_strength = new_signal_strength;
		attr = gtbs_inst.service_p->attrs;
		timer_status = k_work_delayable_remaining_get(
					&gtbs_inst.reporting_interval_work);
		interval = gtbs_inst.signal_strength_interval;
		reporting_interval_work = &gtbs_inst.reporting_interval_work;
	} else {
		inst = &svc_insts[bearer_idx];
		if (inst->signal_strength == new_signal_strength) {
			return 0;
		}

		inst->signal_strength = new_signal_strength;
		attr = inst->service_p->attrs;
		timer_status = k_work_delayable_remaining_get(
					&inst->reporting_interval_work);
		interval = inst->signal_strength_interval;
		reporting_interval_work = &inst->reporting_interval_work;
	}

	if (timer_status == 0) {
		k_timeout_t delay = K_SECONDS(interval);

		bt_gatt_notify_uuid(NULL, BT_UUID_TBS_SIGNAL_STRENGTH,
				    attr, &new_signal_strength,
				    sizeof(new_signal_strength));
		if (interval) {
			k_work_reschedule(reporting_interval_work, delay);
		}
	} else {
		if (bearer_idx == BT_CCP_GTBS_INDEX) {
			BT_DBG("GTBS: Reporting signal strength in %d ms",
			       timer_status);
			gtbs_inst.pending_signal_strength_notification = true;

		} else {
			BT_DBG("Index %u: Reporting signal strength in %d ms",
			       bearer_idx, timer_status);
			inst->pending_signal_strength_notification = true;
		}
	}

	return 0;
}

int bt_tbs_set_status_flags(uint8_t bearer_idx, uint16_t status_flags)
{
	const struct bt_gatt_attr *attr;

	if (!BT_TBS_VALID_STATUS_FLAGS(status_flags)) {
		return -EINVAL;
	} else if (bearer_idx >= CONFIG_BT_TBS_BEARER_COUNT) {
		if (!(IS_ENABLED(CONFIG_BT_GTBS) &&
		    bearer_idx == BT_CCP_GTBS_INDEX)) {
			return -EINVAL;
		}
	}

	if (bearer_idx == BT_CCP_GTBS_INDEX) {
		if (gtbs_inst.status_flags == status_flags) {
			return 0;
		}

		gtbs_inst.status_flags = status_flags;
		attr = gtbs_inst.service_p->attrs;
	} else {
		if (svc_insts[bearer_idx].status_flags == status_flags) {
			return 0;
		}

		svc_insts[bearer_idx].status_flags = status_flags;
		attr = svc_insts[bearer_idx].service_p->attrs;
	}

	bt_gatt_notify_uuid(NULL, BT_UUID_TBS_STATUS_FLAGS,
			   attr, &status_flags, sizeof(status_flags));
	return 0;
}

int bt_tbs_set_uri_scheme_list(uint8_t bearer_idx, const char **uri_list,
			       uint8_t uri_count)
{
	char uri_scheme_list[CONFIG_BT_TBS_MAX_SCHEME_LIST_LENGTH];
	size_t len = 0;
	struct tbs_service_inst_t *inst;

	if (bearer_idx >= CONFIG_BT_TBS_BEARER_COUNT) {
		return -EINVAL;
	}

	inst = &svc_insts[bearer_idx];
	memset(uri_scheme_list, 0, sizeof(uri_scheme_list));

	for (int i = 0; i < uri_count; i++) {
		if (len) {
			len++;
			if (len > sizeof(uri_scheme_list) - 1)  {
				return -ENOMEM;
			}
			strcat(uri_scheme_list, ",");
		}

		len += strlen(uri_list[i]);
		if (len > sizeof(uri_scheme_list) - 1)  {
			return -ENOMEM;
		}

		/* Store list in temp list in case something goes wrong */
		strcat(uri_scheme_list, uri_list[i]);
	}

	if (strcmp(inst->uri_scheme_list, uri_scheme_list) == 0) {
		/* identical; don't update or notify */
		return 0;
	}

	/* Store final result */
	strcpy(inst->uri_scheme_list, uri_scheme_list);

	BT_DBG("TBS instance %u uri prefix list is now %s",
	       bearer_idx, log_strdup(inst->uri_scheme_list));

	bt_gatt_notify_uuid(NULL, BT_UUID_TBS_URI_LIST,
			    inst->service_p->attrs, &inst->uri_scheme_list,
			    strlen(inst->uri_scheme_list));

	if (IS_ENABLED(CONFIG_BT_GTBS)) {
		NET_BUF_SIMPLE_DEFINE(uri_scheme_buf, READ_BUF_SIZE);

		/* TODO: Make uri schemes unique */
		for (int i = 0; i < ARRAY_SIZE(svc_insts); i++) {
			size_t len = strlen(svc_insts[i].uri_scheme_list);

			if (uri_scheme_buf.len + len >= uri_scheme_buf.size) {
				BT_WARN("Cannot fit all TBS instances in GTBS "
					"URI scheme list");
				break;
			}

			net_buf_simple_add_mem(&uri_scheme_buf,
					       svc_insts[i].uri_scheme_list,
					       len);
		}

		/* Add null terminator for printing */
		uri_scheme_buf.data[uri_scheme_buf.len] = '\0';
		BT_DBG("GTBS: URI scheme %s", log_strdup(uri_scheme_buf.data));


		bt_gatt_notify_uuid(NULL, BT_UUID_TBS_URI_LIST,
				    gtbs_inst.service_p->attrs,
				    uri_scheme_buf.data, uri_scheme_buf.len);
	}

	return 0;
}

void bt_tbs_register_cb(struct bt_tbs_cb_t *cbs)
{
	tbs_cbs = cbs;
}

#if defined(CONFIG_BT_DEBUG_TBS)
void bt_tbs_dbg_print_calls(void)
{
	for (int i = 0; i < CONFIG_BT_TBS_BEARER_COUNT; i++) {
		BT_DBG("Bearer #%u", i);
		for (int j = 0; j < ARRAY_SIZE(svc_insts[i].calls); j++) {
			struct bt_tbs_call_t *call = &svc_insts[i].calls[j];

			if (call->index == BT_TBS_FREE_CALL_INDEX) {
				continue;
			}

			BT_DBG("  Call #%u", call->index);
			BT_DBG("    State: %s", tbs_state_str(call->state));
			BT_DBG("    Flags: 0x%02X", call->flags);
			BT_DBG("    URI  : %s", log_strdup(call->remote_uri));
		}
	}
}
#endif /* defined(CONFIG_BT_DEBUG_TBS) */
