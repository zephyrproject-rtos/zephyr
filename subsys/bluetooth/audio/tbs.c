/* Bluetooth TBS - Telephone Bearer Service
 *
 * Copyright (c) 2020 Bose Corporation
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/types.h>

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <stdlib.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>

#include "audio_internal.h"
#include "tbs_internal.h"
#include "ccid_internal.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_tbs, CONFIG_BT_TBS_LOG_LEVEL);

#define BT_TBS_VALID_STATUS_FLAGS(val)         ((val) <= (BIT(0) | BIT(1)))
#define IS_GTBS_CHRC(_attr) \
	IS_ENABLED(CONFIG_BT_GTBS) && BT_AUDIO_CHRC_USER_DATA(_attr) == &gtbs_inst

/* TODO: Have tbs_service_inst include gtbs_service_inst and use CONTAINER_OF
 * to get a specific TBS instance from a GTBS pointer.
 */
struct tbs_service_inst {
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
	struct bt_tbs_in_uri incoming_uri;
	struct bt_tbs_terminate_reason terminate_reason;
	struct bt_tbs_in_uri friendly_name;
	struct bt_tbs_in_uri in_call;

	/* Instance values */
	uint8_t index;
	struct bt_tbs_call calls[CONFIG_BT_TBS_MAX_CALLS];
	bool notify_current_calls;
	bool notify_call_states;
	bool pending_signal_strength_notification;
	struct k_work_delayable reporting_interval_work;

	/* TODO: The TBS (service) and the Telephone Bearers should be separated
	 * into two different instances. This is due to the addition of GTBS,
	 * where we now are in a state where this isn't a 1-to-1 correlation
	 * between TBS and the Telephone Bearers
	 */
	struct bt_gatt_service *service_p;
};

struct gtbs_service_inst {
	/* Attribute values */
	char provider_name[CONFIG_BT_TBS_MAX_PROVIDER_NAME_LENGTH];
	char uci[BT_TBS_MAX_UCI_SIZE];
	uint8_t technology;
	uint8_t signal_strength;
	uint8_t signal_strength_interval;
	uint8_t ccid;
	uint16_t optional_opcodes;
	uint16_t status_flags;
	struct bt_tbs_in_uri incoming_uri;
	struct bt_tbs_in_uri friendly_name;
	struct bt_tbs_in_uri in_call;

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

#if defined(CONFIG_BT_GTBS)
#define READ_BUF_SIZE   (CONFIG_BT_TBS_MAX_CALLS * \
			 sizeof(struct bt_tbs_current_call_item) * \
			 CONFIG_BT_TBS_BEARER_COUNT)
#else
#define READ_BUF_SIZE   (CONFIG_BT_TBS_MAX_CALLS * \
			 sizeof(struct bt_tbs_current_call_item))
#endif /* IS_ENABLED(CONFIG_BT_GTBS) */
NET_BUF_SIMPLE_DEFINE_STATIC(read_buf, READ_BUF_SIZE);

static struct tbs_service_inst svc_insts[CONFIG_BT_TBS_BEARER_COUNT];
static struct gtbs_service_inst gtbs_inst;

/* Used to notify app with held calls in case of join */
static struct bt_tbs_call *held_calls[CONFIG_BT_TBS_MAX_CALLS];
static uint8_t held_calls_cnt;

static struct bt_tbs_cb *tbs_cbs;

static struct bt_tbs_call *lookup_call_in_inst(struct tbs_service_inst *inst,
					       uint8_t call_index)
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
 * @brief Finds and returns a call
 *
 * @param call_index The ID of the call
 * @return struct bt_tbs_call* Pointer to the call. NULL if not found
 */
static struct bt_tbs_call *lookup_call(uint8_t call_index)
{

	if (call_index == BT_TBS_FREE_CALL_INDEX) {
		return NULL;
	}

	for (int i = 0; i < ARRAY_SIZE(svc_insts); i++) {
		struct bt_tbs_call *call = lookup_call_in_inst(&svc_insts[i],
							       call_index);

		if (call != NULL) {
			return call;
		}
	}

	return NULL;
}

static struct tbs_service_inst *lookup_inst_by_ccc(const struct bt_gatt_attr *ccc)
{
	if (ccc == NULL) {
		return NULL;
	}

	for (int i = 0; i < ARRAY_SIZE(svc_insts); i++) {
		struct tbs_service_inst *inst = &svc_insts[i];

		if (inst->service_p == NULL) {
			continue;
		}

		for (size_t j = 0; j < inst->service_p->attr_count; j++) {
			if (inst->service_p->attrs[j].user_data == ccc->user_data) {
				return inst;
			}
		}
	}

	return NULL;
}

static struct tbs_service_inst *lookup_inst_by_call_index(uint8_t call_index)
{
	if (call_index == BT_TBS_FREE_CALL_INDEX) {
		return NULL;
	}

	for (size_t i = 0; i < ARRAY_SIZE(svc_insts); i++) {
		if (lookup_call_in_inst(&svc_insts[i], call_index) != NULL) {
			return &svc_insts[i];
		}
	}

	return NULL;
}

static bool is_authorized(struct bt_conn *conn)
{
	if (IS_ENABLED(CONFIG_BT_TBS_AUTHORIZATION)) {
		if (tbs_cbs != NULL && tbs_cbs->authorize != NULL) {
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
	const size_t scheme_len = strlen(uri_scheme);
	const size_t scheme_list_len = strlen(uri_scheme_list);
	const char *uri_scheme_cand = uri_scheme_list;
	size_t uri_scheme_cand_len;
	size_t start_idx = 0;

	for (size_t i = 0; i < scheme_list_len; i++) {
		if (uri_scheme_list[i] == ',') {
			uri_scheme_cand_len = i - start_idx;
			if (uri_scheme_cand_len != scheme_len) {
				continue;
			}

			if (memcmp(uri_scheme, uri_scheme_cand, scheme_len) == 0) {
				return true;
			}

			if (i + 1 < scheme_list_len) {
				uri_scheme_cand = &uri_scheme_list[i + 1];
			}
		}
	}

	return false;
}

static struct tbs_service_inst *lookup_inst_by_uri_scheme(const char *uri,
							  uint8_t uri_len)
{
	char uri_scheme[CONFIG_BT_TBS_MAX_URI_LENGTH] = { 0 };

	/* Look for ':' between the first and last char */
	for (int i = 1; i < uri_len - 1; i++) {
		if (uri[i] == ':') {
			(void)memcpy(uri_scheme, uri, i);
		}
	}

	if (strlen(uri_scheme) == 0) {
		/* No URI scheme found */
		return NULL;
	}

	for (size_t i = 0; i < ARRAY_SIZE(svc_insts); i++) {
		for (size_t j = 0; j < ARRAY_SIZE(svc_insts[i].calls); j++) {
			if (uri_scheme_in_list(uri_scheme,
					       svc_insts[i].uri_scheme_list)) {
				return &svc_insts[i];
			}
		}
	}

	return NULL;
}

static struct tbs_service_inst *lookup_inst_by_work(const struct k_work *work)
{
	if (work == NULL) {
		return NULL;
	}

	for (size_t i = 0; i < ARRAY_SIZE(svc_insts); i++) {
		if (&svc_insts[i].reporting_interval_work.work == work) {
			return &svc_insts[i];
		}
	}

	return NULL;
}

static void tbs_set_terminate_reason(struct tbs_service_inst *inst,
				     uint8_t call_index, uint8_t reason)
{
	inst->terminate_reason.call_index = call_index;
	inst->terminate_reason.reason = reason;
	LOG_DBG("Index %u: call index 0x%02x, reason %s", inst->index, call_index,
		bt_tbs_term_reason_str(reason));

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
	for (int i = 0; i < CONFIG_BT_TBS_MAX_CALLS; i++) {
		static uint8_t next_call_index = 1;
		const struct bt_tbs_call *call = lookup_call(next_call_index);

		if (call == NULL) {
			return next_call_index++;
		}

		next_call_index++;
		if (next_call_index == UINT8_MAX) {
			/* call_index = 0 reserved for outgoing calls */
			next_call_index = 1;
		}
	}

	LOG_DBG("No more free call spots");

	return BT_TBS_FREE_CALL_INDEX;
}

static void net_buf_put_call_state(const void *inst_p)
{
	const struct bt_tbs_call *call;
	const struct bt_tbs_call *calls;
	size_t call_count;

	if (inst_p == NULL) {
		return;
	}

	net_buf_simple_reset(&read_buf);

	if (IS_ENABLED(CONFIG_BT_GTBS) && inst_p == &gtbs_inst) {
		for (size_t i = 0; i < ARRAY_SIZE(svc_insts); i++) {
			calls = svc_insts[i].calls;
			call_count = ARRAY_SIZE(svc_insts[i].calls);

			for (size_t j = 0; j < call_count; j++) {
				call = &calls[j];
				if (call->index == BT_TBS_FREE_CALL_INDEX) {
					continue;
				}

				net_buf_simple_add_u8(&read_buf, call->index);
				net_buf_simple_add_u8(&read_buf, call->state);
				net_buf_simple_add_u8(&read_buf, call->flags);
			}

		}
	} else {
		const struct tbs_service_inst *inst = (struct tbs_service_inst *)inst_p;

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

static void net_buf_put_current_calls(const void *inst_p)
{
	const struct bt_tbs_call *call;
	const struct bt_tbs_call *calls;
	size_t call_count;
	size_t uri_length;
	size_t item_len;

	if (inst_p == NULL) {
		return;
	}

	net_buf_simple_reset(&read_buf);

	if (IS_ENABLED(CONFIG_BT_GTBS) && inst_p == &gtbs_inst) {
		for (size_t i = 0; i < ARRAY_SIZE(svc_insts); i++) {
			calls = svc_insts[i].calls;
			call_count = ARRAY_SIZE(svc_insts[i].calls);

			for (size_t j = 0; j < call_count; j++) {
				call = &calls[j];
				if (call->index == BT_TBS_FREE_CALL_INDEX) {
					continue;
				}
				uri_length = strlen(call->remote_uri);
				item_len = sizeof(call->index) + sizeof(call->state) +
					   sizeof(call->flags) + uri_length;

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
		const struct tbs_service_inst *inst = (struct tbs_service_inst *)inst_p;

		calls = inst->calls;
		call_count = ARRAY_SIZE(inst->calls);

		for (size_t i = 0; i < call_count; i++) {
			call = &calls[i];
			if (call->index == BT_TBS_FREE_CALL_INDEX) {
				continue;
			}

			uri_length = strlen(call->remote_uri);
			item_len = sizeof(call->index != BT_TBS_FREE_CALL_INDEX) +
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

static int notify_calls(const struct tbs_service_inst *inst)
{
	int err = 0;

	if (inst == NULL) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_GTBS)) {
		if (gtbs_inst.notify_call_states) {
			net_buf_put_call_state(&gtbs_inst);

			err = bt_gatt_notify_uuid(NULL, BT_UUID_TBS_CALL_STATE,
						  gtbs_inst.service_p->attrs,
						  read_buf.data, read_buf.len);
			if (err != 0) {
				return err;
			}
		}

		if (gtbs_inst.notify_current_calls) {
			net_buf_put_current_calls(&gtbs_inst);

			err = bt_gatt_notify_uuid(
				NULL, BT_UUID_TBS_LIST_CURRENT_CALLS,
				gtbs_inst.service_p->attrs,
				read_buf.data, read_buf.len);
			if (err != 0) {
				return err;
			}
		}
	}

	if (inst->notify_call_states) {
		net_buf_put_call_state(inst);

		err = bt_gatt_notify_uuid(NULL, BT_UUID_TBS_CALL_STATE,
					  inst->service_p->attrs,
					  read_buf.data, read_buf.len);
		if (err != 0) {
			return err;
		}
	}
	if (inst->notify_current_calls) {
		net_buf_put_current_calls(inst);

		err = bt_gatt_notify_uuid(NULL, BT_UUID_TBS_LIST_CURRENT_CALLS,
					  inst->service_p->attrs,
					  read_buf.data, read_buf.len);
		if (err != 0) {
			return err;
		}
	}

	return err;
}

static ssize_t read_provider_name(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr,
				  void *buf, uint16_t len, uint16_t offset)
{
	const char *provider_name;

	if (IS_GTBS_CHRC(attr)) {
		provider_name = gtbs_inst.provider_name;
		LOG_DBG("GTBS: Provider name %s", provider_name);
	} else {
		const struct tbs_service_inst *inst = BT_AUDIO_CHRC_USER_DATA(attr);

		provider_name = inst->provider_name;
		LOG_DBG("Index %u, Provider name %s", inst->index, provider_name);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 provider_name,
				 strlen(provider_name));
}

static void provider_name_cfg_changed(const struct bt_gatt_attr *attr,
				      uint16_t value)
{
	const struct tbs_service_inst *inst = lookup_inst_by_ccc(attr);

	if (inst != NULL) {
		LOG_DBG("Index %u: value 0x%04x", inst->index, value);
	} else if (IS_ENABLED(CONFIG_BT_GTBS)) {
		LOG_DBG("GTBS: value 0x%04x", value);
	}
}

static ssize_t read_uci(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, uint16_t len, uint16_t offset)
{
	const char *uci;

	if (IS_GTBS_CHRC(attr)) {
		uci = gtbs_inst.uci;
		LOG_DBG("GTBS: UCI %s", uci);
	} else {
		const struct tbs_service_inst *inst = BT_AUDIO_CHRC_USER_DATA(attr);

		uci = inst->uci;
		LOG_DBG("Index %u: UCI %s", inst->index, uci);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 uci, strlen(uci));
}

static ssize_t read_technology(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr,
			       void *buf, uint16_t len, uint16_t offset)
{
	uint8_t technology;

	if (IS_GTBS_CHRC(attr)) {
		technology = gtbs_inst.technology;
		LOG_DBG("GTBS: Technology 0x%02X", technology);
	} else {
		const struct tbs_service_inst *inst = BT_AUDIO_CHRC_USER_DATA(attr);

		technology = inst->technology;
		LOG_DBG("Index %u: Technology 0x%02X", inst->index, technology);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &technology, sizeof(technology));
}

static void technology_cfg_changed(const struct bt_gatt_attr *attr,
				   uint16_t value)
{
	const struct tbs_service_inst *inst = lookup_inst_by_ccc(attr);

	if (inst != NULL) {
		LOG_DBG("Index %u: value 0x%04x", inst->index, value);
	} else if (IS_ENABLED(CONFIG_BT_GTBS)) {
		LOG_DBG("GTBS: value 0x%04x", value);
	}
}

static ssize_t read_uri_scheme_list(struct bt_conn *conn,
				    const struct bt_gatt_attr *attr,
				    void *buf, uint16_t len, uint16_t offset)
{
	net_buf_simple_reset(&read_buf);

	if (IS_GTBS_CHRC(attr)) {
		/* TODO: Make uri schemes unique */
		for (size_t i = 0; i < ARRAY_SIZE(svc_insts); i++) {
			size_t uri_len = strlen(svc_insts[i].uri_scheme_list);

			if (read_buf.len + uri_len >= read_buf.size) {
				LOG_WRN("Cannot fit all TBS instances in GTBS "
					"URI scheme list");
				break;
			}

			net_buf_simple_add_mem(&read_buf,
					       svc_insts[i].uri_scheme_list,
					       uri_len);
		}
		/* Add null terminator for printing */
		read_buf.data[read_buf.len] = '\0';
		LOG_DBG("GTBS: URI scheme %s", read_buf.data);
	} else {
		const struct tbs_service_inst *inst = BT_AUDIO_CHRC_USER_DATA(attr);

		net_buf_simple_add_mem(&read_buf, inst->uri_scheme_list,
				       strlen(inst->uri_scheme_list));
		/* Add null terminator for printing */
		read_buf.data[read_buf.len] = '\0';
		LOG_DBG("Index %u: URI scheme %s", inst->index, read_buf.data);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 read_buf.data, read_buf.len);
}

static void uri_scheme_list_cfg_changed(const struct bt_gatt_attr *attr,
				   uint16_t value)
{
	const struct tbs_service_inst *inst = lookup_inst_by_ccc(attr);

	if (inst != NULL) {
		LOG_DBG("Index %u: value 0x%04x", inst->index, value);
	} else if (IS_ENABLED(CONFIG_BT_GTBS)) {
		LOG_DBG("GTBS: value 0x%04x", value);
	}
}

static ssize_t read_signal_strength(struct bt_conn *conn,
				    const struct bt_gatt_attr *attr,
				    void *buf, uint16_t len, uint16_t offset)
{
	uint8_t signal_strength;

	if (IS_GTBS_CHRC(attr)) {
		signal_strength = gtbs_inst.signal_strength;
		LOG_DBG("GTBS: Signal strength 0x%02x", signal_strength);
	} else {
		const struct tbs_service_inst *inst = BT_AUDIO_CHRC_USER_DATA(attr);

		signal_strength = inst->signal_strength;
		LOG_DBG("Index %u: Signal strength 0x%02x", inst->index, signal_strength);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &signal_strength, sizeof(signal_strength));
}

static void signal_strength_cfg_changed(const struct bt_gatt_attr *attr,
					uint16_t value)
{
	const struct tbs_service_inst *inst = lookup_inst_by_ccc(attr);

	if (inst != NULL) {
		LOG_DBG("Index %u: value 0x%04x", inst->index, value);
	} else if (IS_ENABLED(CONFIG_BT_GTBS)) {
		LOG_DBG("GTBS: value 0x%04x", value);
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

	if (IS_GTBS_CHRC(attr)) {
		signal_strength_interval = gtbs_inst.signal_strength_interval;
		LOG_DBG("GTBS: Signal strength interval 0x%02x", signal_strength_interval);
	} else {
		const struct tbs_service_inst *inst = BT_AUDIO_CHRC_USER_DATA(attr);

		signal_strength_interval = inst->signal_strength_interval;
		LOG_DBG("Index %u: Signal strength interval 0x%02x", inst->index,
			signal_strength_interval);
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

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != sizeof(signal_strength_interval)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	net_buf_simple_init_with_data(&net_buf, (void *)buf, len);
	signal_strength_interval = net_buf_simple_pull_u8(&net_buf);

	if (IS_GTBS_CHRC(attr)) {
		gtbs_inst.signal_strength_interval = signal_strength_interval;
		LOG_DBG("GTBS: 0x%02x", signal_strength_interval);
	} else {
		struct tbs_service_inst *inst = BT_AUDIO_CHRC_USER_DATA(attr);

		inst->signal_strength_interval = signal_strength_interval;
		LOG_DBG("Index %u: 0x%02x", inst->index, signal_strength_interval);
	}

	return len;
}

static void current_calls_cfg_changed(const struct bt_gatt_attr *attr,
				      uint16_t value)
{
	struct tbs_service_inst *inst = lookup_inst_by_ccc(attr);

	if (inst != NULL) {
		LOG_DBG("Index %u: value 0x%04x", inst->index, value);
		inst->notify_current_calls = (value == BT_GATT_CCC_NOTIFY);
	} else if (IS_ENABLED(CONFIG_BT_GTBS)) {
		LOG_DBG("GTBS: value 0x%04x", value);
		gtbs_inst.notify_current_calls = (value == BT_GATT_CCC_NOTIFY);
	}
}

static ssize_t read_current_calls(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr,
				  void *buf, uint16_t len, uint16_t offset)
{
	net_buf_put_current_calls(BT_AUDIO_CHRC_USER_DATA(attr));

	if (IS_GTBS_CHRC(attr)) {
		LOG_DBG("GTBS");
	} else {
		const struct tbs_service_inst *inst = BT_AUDIO_CHRC_USER_DATA(attr);

		LOG_DBG("Index %u", inst->index);
	}

	if (offset == 0) {
		LOG_HEXDUMP_DBG(read_buf.data, read_buf.len, "Current calls");
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 read_buf.data, read_buf.len);
}

static ssize_t read_ccid(struct bt_conn *conn,
			 const struct bt_gatt_attr *attr,
			 void *buf, uint16_t len, uint16_t offset)
{
	uint8_t ccid;

	if (IS_GTBS_CHRC(attr)) {
		ccid = gtbs_inst.ccid;
		LOG_DBG("GTBS: CCID 0x%02X", ccid);
	} else {
		const struct tbs_service_inst *inst = BT_AUDIO_CHRC_USER_DATA(attr);

		ccid = inst->ccid;
		LOG_DBG("Index %u: CCID 0x%02X", inst->index, ccid);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &ccid, sizeof(ccid));
}

static ssize_t read_status_flags(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr,
				 void *buf, uint16_t len, uint16_t offset)
{
	uint16_t status_flags;

	if (IS_GTBS_CHRC(attr)) {
		status_flags = gtbs_inst.status_flags;
		LOG_DBG("GTBS: status_flags 0x%04X", status_flags);
	} else {
		const struct tbs_service_inst *inst = BT_AUDIO_CHRC_USER_DATA(attr);

		status_flags = inst->status_flags;
		LOG_DBG("Index %u: status_flags 0x%04X", inst->index, status_flags);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &status_flags, sizeof(status_flags));
}

static void status_flags_cfg_changed(const struct bt_gatt_attr *attr,
					   uint16_t value)
{
	const struct tbs_service_inst *inst = lookup_inst_by_ccc(attr);

	if (inst != NULL) {
		LOG_DBG("Index %u: value 0x%04x", inst->index, value);
	} else if (IS_ENABLED(CONFIG_BT_GTBS)) {
		LOG_DBG("GTBS: value 0x%04x", value);
	}
}

static ssize_t read_incoming_uri(struct bt_conn *conn,
					    const struct bt_gatt_attr *attr,
					    void *buf, uint16_t len,
					    uint16_t offset)
{
	const struct bt_tbs_in_uri *inc_call_target;
	size_t val_len;

	if (IS_GTBS_CHRC(attr)) {
		inc_call_target = &gtbs_inst.incoming_uri;
		LOG_DBG("GTBS: call index 0x%02X, URI %s", inc_call_target->call_index,
			inc_call_target->uri);
	} else {
		const struct tbs_service_inst *inst = BT_AUDIO_CHRC_USER_DATA(attr);

		inc_call_target = &inst->incoming_uri;
		LOG_DBG("Index %u: call index 0x%02X, URI %s", inst->index,
			inc_call_target->call_index, inc_call_target->uri);
	}

	if (!inc_call_target->call_index) {
		LOG_DBG("URI not set");

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
	const struct tbs_service_inst *inst = lookup_inst_by_ccc(attr);

	if (inst != NULL) {
		LOG_DBG("Index %u: value 0x%04x", inst->index, value);
	} else if (IS_ENABLED(CONFIG_BT_GTBS)) {
		LOG_DBG("GTBS: value 0x%04x", value);
	}
}

static ssize_t read_call_state(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr,
			       void *buf, uint16_t len, uint16_t offset)
{
	net_buf_put_call_state(BT_AUDIO_CHRC_USER_DATA(attr));

	if (IS_GTBS_CHRC(attr)) {
		LOG_DBG("GTBS");
	} else {
		const struct tbs_service_inst *inst = BT_AUDIO_CHRC_USER_DATA(attr);

		LOG_DBG("Index %u", inst->index);
	}

	if (offset == 0) {
		LOG_HEXDUMP_DBG(read_buf.data, read_buf.len, "Call state");
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 read_buf.data, read_buf.len);
}

static void call_state_cfg_changed(const struct bt_gatt_attr *attr,
				   uint16_t value)
{
	struct tbs_service_inst *inst = lookup_inst_by_ccc(attr);

	if (inst != NULL) {
		LOG_DBG("Index %u: value 0x%04x", inst->index, value);
		inst->notify_call_states = (value == BT_GATT_CCC_NOTIFY);
	} else if (IS_ENABLED(CONFIG_BT_GTBS)) {
		LOG_DBG("GTBS: value 0x%04x", value);
		gtbs_inst.notify_call_states = (value == BT_GATT_CCC_NOTIFY);
	}
}

static int notify_ccp(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		      uint8_t call_index, uint8_t opcode, uint8_t status)
{
	const struct bt_tbs_call_cp_notify ccp_not = {
		.call_index = call_index,
		.opcode = opcode,
		.status = status
	};

	LOG_DBG("Notifying CCP: Call index %u, %s opcode and status %s", call_index,
		bt_tbs_opcode_str(opcode), bt_tbs_status_str(status));

	return bt_gatt_notify(conn, attr, &ccp_not, sizeof(ccp_not));
}

static void hold_other_calls(struct tbs_service_inst *inst,
			     uint8_t call_index_cnt,
			     const uint8_t *call_indexes)
{
	held_calls_cnt = 0;

	for (int i = 0; i < ARRAY_SIZE(inst->calls); i++) {
		bool hold_call = true;
		uint8_t call_state;

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
		if (call_state == BT_TBS_CALL_STATE_ACTIVE) {
			inst->calls[i].state = BT_TBS_CALL_STATE_LOCALLY_HELD;
			held_calls[held_calls_cnt++] = &inst->calls[i];
		} else if (call_state == BT_TBS_CALL_STATE_REMOTELY_HELD) {
			inst->calls[i].state =
				BT_TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD;
			held_calls[held_calls_cnt++] = &inst->calls[i];
		}
	}
}

static uint8_t accept_call(struct tbs_service_inst *inst,
			   const struct bt_tbs_call_cp_acc *ccp)
{
	struct bt_tbs_call *call = lookup_call_in_inst(inst, ccp->call_index);

	if (call == NULL) {
		return BT_TBS_RESULT_CODE_INVALID_CALL_INDEX;
	}

	if (call->state == BT_TBS_CALL_STATE_INCOMING) {
		call->state = BT_TBS_CALL_STATE_ACTIVE;

		hold_other_calls(inst, 1, &ccp->call_index);

		return BT_TBS_RESULT_CODE_SUCCESS;
	} else {
		return BT_TBS_RESULT_CODE_STATE_MISMATCH;
	}
}

static uint8_t terminate_call(struct tbs_service_inst *inst,
			      const struct bt_tbs_call_cp_term *ccp,
			      uint8_t reason)
{
	struct bt_tbs_call *call = lookup_call_in_inst(inst, ccp->call_index);

	if (call == NULL) {
		return BT_TBS_RESULT_CODE_INVALID_CALL_INDEX;
	}

	call->index = BT_TBS_FREE_CALL_INDEX;
	tbs_set_terminate_reason(inst, ccp->call_index, reason);

	return BT_TBS_RESULT_CODE_SUCCESS;
}

static uint8_t tbs_hold_call(struct tbs_service_inst *inst,
			     const struct bt_tbs_call_cp_hold *ccp)
{
	struct bt_tbs_call *call = lookup_call_in_inst(inst, ccp->call_index);

	if ((inst->optional_opcodes & BT_TBS_FEATURE_HOLD) == 0) {
		return BT_TBS_RESULT_CODE_OPCODE_NOT_SUPPORTED;
	}

	if (call == NULL) {
		return BT_TBS_RESULT_CODE_INVALID_CALL_INDEX;
	}

	if (call->state == BT_TBS_CALL_STATE_ACTIVE) {
		call->state = BT_TBS_CALL_STATE_LOCALLY_HELD;
	} else if (call->state == BT_TBS_CALL_STATE_REMOTELY_HELD) {
		call->state = BT_TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD;
	} else if (call->state == BT_TBS_CALL_STATE_INCOMING) {
		call->state = BT_TBS_CALL_STATE_LOCALLY_HELD;
	} else {
		return BT_TBS_RESULT_CODE_STATE_MISMATCH;
	}

	return BT_TBS_RESULT_CODE_SUCCESS;
}

static uint8_t retrieve_call(struct tbs_service_inst *inst,
			     const struct bt_tbs_call_cp_retrieve *ccp)
{
	struct bt_tbs_call *call = lookup_call_in_inst(inst, ccp->call_index);

	if ((inst->optional_opcodes & BT_TBS_FEATURE_HOLD) == 0) {
		return BT_TBS_RESULT_CODE_OPCODE_NOT_SUPPORTED;
	}

	if (call == NULL) {
		return BT_TBS_RESULT_CODE_INVALID_CALL_INDEX;
	}

	if (call->state == BT_TBS_CALL_STATE_LOCALLY_HELD) {
		call->state = BT_TBS_CALL_STATE_ACTIVE;
	} else if (call->state == BT_TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD) {
		call->state = BT_TBS_CALL_STATE_REMOTELY_HELD;
	} else {
		return BT_TBS_RESULT_CODE_STATE_MISMATCH;
	}

	hold_other_calls(inst, 1, &ccp->call_index);

	return BT_TBS_RESULT_CODE_SUCCESS;
}

static int originate_call(struct tbs_service_inst *inst,
			  const struct bt_tbs_call_cp_originate *ccp,
			  uint16_t uri_len, uint8_t *call_index)
{
	struct bt_tbs_call *call = NULL;

	/* New call - Look for unused call item */
	for (int i = 0; i < CONFIG_BT_TBS_MAX_CALLS; i++) {
		if (inst->calls[i].index == BT_TBS_FREE_CALL_INDEX) {
			call = &inst->calls[i];
			break;
		}
	}

	/* Only allow one active outgoing call */
	for (int i = 0; i < CONFIG_BT_TBS_MAX_CALLS; i++) {
		if (inst->calls[i].state == BT_TBS_CALL_STATE_ALERTING) {
			return BT_TBS_RESULT_CODE_OPERATION_NOT_POSSIBLE;
		}
	}

	if (call == NULL) {
		return BT_TBS_RESULT_CODE_OUT_OF_RESOURCES;
	}

	call->index = next_free_call_index();

	if (call->index == BT_TBS_FREE_CALL_INDEX) {
		return BT_TBS_RESULT_CODE_OUT_OF_RESOURCES;
	}

	if (uri_len == 0 || uri_len > CONFIG_BT_TBS_MAX_URI_LENGTH) {
		call->index = BT_TBS_FREE_CALL_INDEX;
		return BT_TBS_RESULT_CODE_INVALID_URI;
	}

	(void)memcpy(call->remote_uri, ccp->uri, uri_len);
	call->remote_uri[uri_len] = '\0';
	if (!bt_tbs_valid_uri(call->remote_uri)) {
		LOG_DBG("Invalid URI: %s", call->remote_uri);
		call->index = BT_TBS_FREE_CALL_INDEX;

		return BT_TBS_RESULT_CODE_INVALID_URI;
	}

	/* We need to notify dialing state for test,
	 * even though we don't have an internal dialing state.
	 */
	call->state = BT_TBS_CALL_STATE_DIALING;
	if (call->index != BT_TBS_FREE_CALL_INDEX) {
		*call_index = call->index;
	}
	BT_TBS_CALL_FLAG_SET_OUTGOING(call->flags);

	hold_other_calls(inst, 1, &call->index);
	notify_calls(inst);

	LOG_DBG("New call with call index %u", call->index);

	return BT_TBS_RESULT_CODE_SUCCESS;
}

static uint8_t join_calls(struct tbs_service_inst *inst,
			  const struct bt_tbs_call_cp_join *ccp,
			  uint16_t call_index_cnt)
{
	struct bt_tbs_call *joined_calls[CONFIG_BT_TBS_MAX_CALLS];
	uint8_t call_state;

	if ((inst->optional_opcodes & BT_TBS_FEATURE_JOIN) == 0) {
		return BT_TBS_RESULT_CODE_OPCODE_NOT_SUPPORTED;
	}

	/* Check length */
	if (call_index_cnt < 2 || call_index_cnt > CONFIG_BT_TBS_MAX_CALLS) {
		return BT_TBS_RESULT_CODE_OPERATION_NOT_POSSIBLE;
	}

	/* Check for duplicates */
	for (int i = 0; i < call_index_cnt; i++) {
		for (int j = 0; j < i; j++) {
			if (ccp->call_indexes[i] == ccp->call_indexes[j]) {
				return BT_TBS_RESULT_CODE_INVALID_CALL_INDEX;
			}
		}
	}

	/* Validate that all calls are in a joinable state */
	for (int i = 0; i < call_index_cnt; i++) {
		joined_calls[i] = lookup_call_in_inst(inst,
						      ccp->call_indexes[i]);
		if (joined_calls[i] == NULL) {
			return BT_TBS_RESULT_CODE_INVALID_CALL_INDEX;
		}

		call_state = joined_calls[i]->state;

		if (call_state == BT_TBS_CALL_STATE_INCOMING) {
			return BT_TBS_RESULT_CODE_OPERATION_NOT_POSSIBLE;
		}

		if (call_state != BT_TBS_CALL_STATE_LOCALLY_HELD &&
		    call_state != BT_TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD &&
		    call_state != BT_TBS_CALL_STATE_ACTIVE) {
			return BT_TBS_RESULT_CODE_STATE_MISMATCH;
		}
	}

	/* Join all calls */
	for (int i = 0; i < call_index_cnt; i++) {
		call_state = joined_calls[i]->state;

		if (call_state == BT_TBS_CALL_STATE_LOCALLY_HELD) {
			joined_calls[i]->state = BT_TBS_CALL_STATE_ACTIVE;
		} else if (call_state ==
				BT_TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD) {
			joined_calls[i]->state =
				BT_TBS_CALL_STATE_REMOTELY_HELD;
		} else if (call_state == BT_TBS_CALL_STATE_INCOMING) {
			joined_calls[i]->state = BT_TBS_CALL_STATE_ACTIVE;
		}
		/* else active => Do nothing */
	}

	hold_other_calls(inst, call_index_cnt, ccp->call_indexes);

	return BT_TBS_RESULT_CODE_SUCCESS;
}

static void notify_app(struct bt_conn *conn, uint16_t len,
		       const union bt_tbs_call_cp_t *ccp, uint8_t status,
		       uint8_t call_index)
{
	if (tbs_cbs == NULL) {
		return;
	}

	switch (ccp->opcode) {
	case BT_TBS_CALL_OPCODE_ACCEPT:
		if (tbs_cbs->accept_call != NULL) {
			tbs_cbs->accept_call(conn, call_index);
		}
		break;
	case BT_TBS_CALL_OPCODE_TERMINATE:
		if (tbs_cbs->terminate_call != NULL) {
			const struct tbs_service_inst *inst =
				lookup_inst_by_call_index(ccp->terminate.call_index);

			tbs_cbs->terminate_call(conn, call_index,
						inst->terminate_reason.reason);
		}
		break;
	case BT_TBS_CALL_OPCODE_HOLD:
		if (tbs_cbs->hold_call != NULL) {
			tbs_cbs->hold_call(conn, call_index);
		}
		break;
	case BT_TBS_CALL_OPCODE_RETRIEVE:
		if (tbs_cbs->retrieve_call != NULL) {
			tbs_cbs->retrieve_call(conn, call_index);
		}
		break;
	case BT_TBS_CALL_OPCODE_ORIGINATE:
	{
		char uri[CONFIG_BT_TBS_MAX_URI_LENGTH + 1];
		const uint16_t uri_len = len - sizeof(ccp->originate);
		bool remote_party_alerted = false;
		struct bt_tbs_call *call;
		struct tbs_service_inst *inst;

		inst = lookup_inst_by_call_index(call_index);

		if (inst == NULL) {
			LOG_DBG("Could not find instance by call index 0x%02X", call_index);
			break;
		}

		call = lookup_call_in_inst(inst, call_index);

		if (call == NULL) {
			LOG_DBG("Could not find call by call index 0x%02X", call_index);
			break;
		}

		(void)memcpy(uri, ccp->originate.uri, uri_len);
		uri[uri_len] = '\0';
		if (tbs_cbs->originate_call != NULL) {
			remote_party_alerted = tbs_cbs->originate_call(conn,
								       call_index,
								       uri);
		}

		if (remote_party_alerted) {
			call->state = BT_TBS_CALL_STATE_ALERTING;
		} else {
			const struct bt_tbs_call_cp_term term = {
				.call_index = call_index,
				.opcode = BT_TBS_CALL_OPCODE_TERMINATE
			};

			/* Terminate and remove call */
			terminate_call(inst, &term, BT_TBS_REASON_CALL_FAILED);
		}

		notify_calls(inst);

		break;
	}
	case BT_TBS_CALL_OPCODE_JOIN:
	{
		const uint16_t call_index_cnt = len - sizeof(ccp->join);

		/* Let the app know about joined calls */
		if (tbs_cbs->join_calls != NULL) {
			tbs_cbs->join_calls(conn, call_index_cnt,
					    ccp->join.call_indexes);
		}
		break;
	}
	default:
		break;
	}

	/* Let the app know about held calls */
	if (held_calls_cnt != 0 && tbs_cbs->hold_call != NULL) {
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
	struct tbs_service_inst *inst = NULL;
	const union bt_tbs_call_cp_t *ccp = (union bt_tbs_call_cp_t *)buf;
	uint8_t status;
	uint8_t call_index = 0;
	const bool is_gtbs = IS_GTBS_CHRC(attr);

	if (!is_authorized(conn)) {
		return BT_GATT_ERR(BT_ATT_ERR_AUTHORIZATION);
	}

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len < sizeof(ccp->opcode)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (is_gtbs) {
		LOG_DBG("GTBS: Processing the %s opcode", bt_tbs_opcode_str(ccp->opcode));
	} else {
		inst = BT_AUDIO_CHRC_USER_DATA(attr);

		LOG_DBG("Index %u: Processing the %s opcode", inst->index,
			bt_tbs_opcode_str(ccp->opcode));
	}

	switch (ccp->opcode) {
	case BT_TBS_CALL_OPCODE_ACCEPT:
		if (len != sizeof(ccp->accept)) {
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}

		call_index = ccp->accept.call_index;

		if (is_gtbs) {
			inst = lookup_inst_by_call_index(call_index);
			if (inst == NULL) {
				status = BT_TBS_RESULT_CODE_INVALID_CALL_INDEX;
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
			inst = lookup_inst_by_call_index(call_index);
			if (inst == NULL) {
				status = BT_TBS_RESULT_CODE_INVALID_CALL_INDEX;
				break;
			}
		}

		status = terminate_call(inst, &ccp->terminate,
					BT_TBS_REASON_CLIENT_TERMINATED);
		break;
	case BT_TBS_CALL_OPCODE_HOLD:
		if (len != sizeof(ccp->hold)) {
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}

		call_index = ccp->hold.call_index;

		if (is_gtbs) {
			inst = lookup_inst_by_call_index(call_index);
			if (inst == NULL) {
				status = BT_TBS_RESULT_CODE_INVALID_CALL_INDEX;
				break;
			}
		}

		status = tbs_hold_call(inst, &ccp->hold);
		break;
	case BT_TBS_CALL_OPCODE_RETRIEVE:
		if (len != sizeof(ccp->retrieve)) {
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}

		call_index = ccp->retrieve.call_index;

		if (is_gtbs) {
			inst = lookup_inst_by_call_index(call_index);
			if (inst == NULL) {
				status = BT_TBS_RESULT_CODE_INVALID_CALL_INDEX;
				break;
			}
		}

		status = retrieve_call(inst, &ccp->retrieve);
		break;
	case BT_TBS_CALL_OPCODE_ORIGINATE:
	{
		const uint16_t uri_len = len - sizeof(ccp->originate);

		if (len < sizeof(ccp->originate) + BT_TBS_MIN_URI_LEN) {
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}

		if (is_gtbs) {
			inst = lookup_inst_by_uri_scheme(ccp->originate.uri,
						      uri_len);
			if (inst == NULL) {
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
		const uint16_t call_index_cnt = len - sizeof(ccp->join);

		if (len < sizeof(ccp->join) + 1) { /* at least 1 call index */
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}

		call_index = ccp->join.call_indexes[0];

		if (is_gtbs) {
			inst = lookup_inst_by_call_index(call_index);
			if (inst == NULL) {
				status = BT_TBS_RESULT_CODE_INVALID_CALL_INDEX;
				break;
			}
		}

		status = join_calls(inst, &ccp->join, call_index_cnt);
		break;
	}
	default:
		status = BT_TBS_RESULT_CODE_OPCODE_NOT_SUPPORTED;
		call_index = 0;
		break;
	}

	if (inst != NULL) {
		if (is_gtbs) {
			LOG_DBG("GTBS: Processed the %s opcode with status %s "
			       "for call index %u",
			       bt_tbs_opcode_str(ccp->opcode),
			       bt_tbs_status_str(status),
			       call_index);
		} else {
			LOG_DBG("Index %u: Processed the %s opcode with status "
			       "%s for call index %u",
			       inst->index,
			       bt_tbs_opcode_str(ccp->opcode),
			       bt_tbs_status_str(status),
			       call_index);
		}

		if (status == BT_TBS_RESULT_CODE_SUCCESS) {
			const struct bt_tbs_call *call = lookup_call(call_index);

			if (call != NULL) {
				LOG_DBG("Call is now in the %s state",
					bt_tbs_state_str(call->state));
			} else {
				LOG_DBG("Call is now terminated");
			}
		}
	}

	if (status != BT_TBS_RESULT_CODE_SUCCESS) {
		call_index = 0;
	}

	if (conn != NULL) {
		notify_ccp(conn, attr, call_index, ccp->opcode, status);
	} /* else local operation; don't notify */

	if (status == BT_TBS_RESULT_CODE_SUCCESS) {
		notify_calls(inst);
		notify_app(conn, len, ccp, status, call_index);
	}

	return len;
}

static void call_cp_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	const struct tbs_service_inst *inst = lookup_inst_by_ccc(attr);

	if (inst != NULL) {
		LOG_DBG("Index %u: value 0x%04x", inst->index, value);
	} else if (IS_ENABLED(CONFIG_BT_GTBS)) {
		LOG_DBG("GTBS: value 0x%04x", value);
	}
}

static ssize_t read_optional_opcodes(struct bt_conn *conn,
				     const struct bt_gatt_attr *attr,
				     void *buf, uint16_t len, uint16_t offset)
{
	uint16_t optional_opcodes;

	if (IS_GTBS_CHRC(attr)) {
		optional_opcodes = gtbs_inst.optional_opcodes;
		LOG_DBG("GTBS: Supported opcodes 0x%02x", optional_opcodes);
	} else {
		const struct tbs_service_inst *inst = BT_AUDIO_CHRC_USER_DATA(attr);

		optional_opcodes = inst->optional_opcodes;
		LOG_DBG("Index %u: Supported opcodes 0x%02x", inst->index, optional_opcodes);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &optional_opcodes, sizeof(optional_opcodes));
}

static void terminate_reason_cfg_changed(const struct bt_gatt_attr *attr,
					 uint16_t value)
{
	const struct tbs_service_inst *inst = lookup_inst_by_ccc(attr);

	if (inst != NULL) {
		LOG_DBG("Index %u: value 0x%04x", inst->index, value);
	} else if (IS_ENABLED(CONFIG_BT_GTBS)) {
		LOG_DBG("GTBS: value 0x%04x", value);
	}
}

static ssize_t read_friendly_name(struct bt_conn *conn,
				     const struct bt_gatt_attr *attr,
				     void *buf, uint16_t len, uint16_t offset)
{
	const struct bt_tbs_in_uri *friendly_name;
	size_t val_len;

	if (IS_GTBS_CHRC(attr)) {
		friendly_name = &gtbs_inst.friendly_name;
		LOG_DBG("GTBS: call index 0x%02X, URI %s", friendly_name->call_index,
			friendly_name->uri);
	} else {
		const struct tbs_service_inst *inst = BT_AUDIO_CHRC_USER_DATA(attr);

		friendly_name = &inst->friendly_name;
		LOG_DBG("Index %u: call index 0x%02X, URI %s", inst->index,
			friendly_name->call_index, friendly_name->uri);
	}

	if (friendly_name->call_index == BT_TBS_FREE_CALL_INDEX) {
		LOG_DBG("URI not set");
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
	const struct tbs_service_inst *inst = lookup_inst_by_ccc(attr);

	if (inst != NULL) {
		LOG_DBG("Index %u: value 0x%04x", inst->index, value);
	} else if (IS_ENABLED(CONFIG_BT_GTBS)) {
		LOG_DBG("GTBS: value 0x%04x", value);
	}
}

static ssize_t read_incoming_call(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr,
				  void *buf, uint16_t len, uint16_t offset)
{
	const struct bt_tbs_in_uri *remote_uri;
	size_t val_len;

	if (IS_GTBS_CHRC(attr)) {
		remote_uri = &gtbs_inst.in_call;
		LOG_DBG("GTBS: call index 0x%02X, URI %s", remote_uri->call_index, remote_uri->uri);
	} else {
		const struct tbs_service_inst *inst = BT_AUDIO_CHRC_USER_DATA(attr);

		remote_uri = &inst->in_call;
		LOG_DBG("Index %u: call index 0x%02X, URI %s", inst->index, remote_uri->call_index,
			remote_uri->uri);
	}

	if (remote_uri->call_index == BT_TBS_FREE_CALL_INDEX) {
		LOG_DBG("URI not set");

		return bt_gatt_attr_read(conn, attr, buf, len, offset, NULL, 0);
	}

	val_len = sizeof(remote_uri->call_index) + strlen(remote_uri->uri);

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 remote_uri, val_len);
}

static void in_call_cfg_changed(const struct bt_gatt_attr *attr,
				      uint16_t value)
{
	const struct tbs_service_inst *inst = lookup_inst_by_ccc(attr);

	if (inst != NULL) {
		LOG_DBG("Index %u: value 0x%04x", inst->index, value);
	} else if (IS_ENABLED(CONFIG_BT_GTBS)) {
		LOG_DBG("GTBS: value 0x%04x", value);
	}
}

#define BT_TBS_CHR_PROVIDER_NAME(inst) \
	BT_AUDIO_CHRC(BT_UUID_TBS_PROVIDER_NAME, \
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
		      BT_GATT_PERM_READ_ENCRYPT, \
		      read_provider_name, NULL, inst), \
	BT_AUDIO_CCC(provider_name_cfg_changed)

#define BT_TBS_CHR_UCI(inst) \
	BT_AUDIO_CHRC(BT_UUID_TBS_UCI, \
		      BT_GATT_CHRC_READ, \
		      BT_GATT_PERM_READ_ENCRYPT, \
		      read_uci, NULL, inst)

#define BT_TBS_CHR_TECHNOLOGY(inst) \
	BT_AUDIO_CHRC(BT_UUID_TBS_TECHNOLOGY, \
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
		      BT_GATT_PERM_READ_ENCRYPT, \
		      read_technology, NULL, inst), \
	BT_AUDIO_CCC(technology_cfg_changed)

#define BT_TBS_CHR_URI_LIST(inst) \
	BT_AUDIO_CHRC(BT_UUID_TBS_URI_LIST, \
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
		      BT_GATT_PERM_READ_ENCRYPT, \
		      read_uri_scheme_list, NULL, inst), \
	BT_AUDIO_CCC(uri_scheme_list_cfg_changed)

#define BT_TBS_CHR_SIGNAL_STRENGTH(inst) \
	BT_AUDIO_CHRC(BT_UUID_TBS_SIGNAL_STRENGTH, /* Optional */ \
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
		      BT_GATT_PERM_READ_ENCRYPT, \
		      read_signal_strength, NULL, inst), \
	BT_AUDIO_CCC(signal_strength_cfg_changed)

#define BT_TBS_CHR_SIGNAL_INTERVAL(inst) \
	BT_AUDIO_CHRC(BT_UUID_TBS_SIGNAL_INTERVAL, /* Conditional */ \
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP, \
		      BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT, \
		      read_signal_strength_interval, write_signal_strength_interval, inst)

#define BT_TBS_CHR_CURRENT_CALLS(inst) \
	BT_AUDIO_CHRC(BT_UUID_TBS_LIST_CURRENT_CALLS, \
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
		      BT_GATT_PERM_READ_ENCRYPT, \
		      read_current_calls, NULL, inst), \
	BT_AUDIO_CCC(current_calls_cfg_changed)

#define BT_TBS_CHR_CCID(inst) \
	BT_AUDIO_CHRC(BT_UUID_CCID, \
		      BT_GATT_CHRC_READ, \
		      BT_GATT_PERM_READ_ENCRYPT, \
		      read_ccid, NULL, inst)

#define BT_TBS_CHR_STATUS_FLAGS(inst) \
	BT_AUDIO_CHRC(BT_UUID_TBS_STATUS_FLAGS, \
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
		      BT_GATT_PERM_READ_ENCRYPT, \
		      read_status_flags, NULL, inst), \
	BT_AUDIO_CCC(status_flags_cfg_changed)

#define BT_TBS_CHR_INCOMING_URI(inst) \
	BT_AUDIO_CHRC(BT_UUID_TBS_INCOMING_URI, /* Optional */ \
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
		      BT_GATT_PERM_READ_ENCRYPT, \
		      read_incoming_uri, NULL, inst), \
	BT_AUDIO_CCC(incoming_uri_cfg_changed)

#define BT_TBS_CHR_CALL_STATE(inst) \
	BT_AUDIO_CHRC(BT_UUID_TBS_CALL_STATE, \
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
		      BT_GATT_PERM_READ_ENCRYPT, \
		      read_call_state, NULL, inst), \
	BT_AUDIO_CCC(call_state_cfg_changed)

#define BT_TBS_CHR_CONTROL_POINT(inst) \
	BT_AUDIO_CHRC(BT_UUID_TBS_CALL_CONTROL_POINT, \
		      BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_WRITE_WITHOUT_RESP, \
		      BT_GATT_PERM_WRITE_ENCRYPT, \
		      NULL, write_call_cp, inst), \
	BT_AUDIO_CCC(call_cp_cfg_changed)

#define BT_TBS_CHR_OPTIONAL_OPCODES(inst) \
	BT_AUDIO_CHRC(BT_UUID_TBS_OPTIONAL_OPCODES, \
		      BT_GATT_CHRC_READ, \
		      BT_GATT_PERM_READ_ENCRYPT, \
		      read_optional_opcodes, NULL, inst) \

#define BT_TBS_CHR_TERMINATE_REASON(inst) \
	BT_AUDIO_CHRC(BT_UUID_TBS_TERMINATE_REASON, \
		      BT_GATT_CHRC_NOTIFY, \
		      BT_GATT_PERM_READ_ENCRYPT, \
		      NULL, NULL, inst), \
	BT_AUDIO_CCC(terminate_reason_cfg_changed)

#define BT_TBS_CHR_INCOMING_CALL(inst) \
	BT_AUDIO_CHRC(BT_UUID_TBS_INCOMING_CALL, \
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
		      BT_GATT_PERM_READ_ENCRYPT, \
		      read_incoming_call, NULL, inst), \
	BT_AUDIO_CCC(in_call_cfg_changed)

#define BT_TBS_CHR_FRIENDLY_NAME(inst) \
	BT_AUDIO_CHRC(BT_UUID_TBS_FRIENDLY_NAME, /* Optional */ \
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
		      BT_GATT_PERM_READ_ENCRYPT, \
		      read_friendly_name, NULL, inst), \
	BT_AUDIO_CCC(friendly_name_cfg_changed)

#define BT_TBS_SERVICE_DEFINITION(inst) {\
	BT_GATT_PRIMARY_SERVICE(BT_UUID_TBS), \
	BT_TBS_CHR_PROVIDER_NAME(&inst), \
	BT_TBS_CHR_UCI(&inst), \
	BT_TBS_CHR_TECHNOLOGY(&inst), \
	BT_TBS_CHR_URI_LIST(&inst), \
	BT_TBS_CHR_SIGNAL_STRENGTH(&inst), \
	BT_TBS_CHR_SIGNAL_INTERVAL(&inst), \
	BT_TBS_CHR_CURRENT_CALLS(&inst), \
	BT_TBS_CHR_CCID(&inst), \
	BT_TBS_CHR_STATUS_FLAGS(&inst), \
	BT_TBS_CHR_INCOMING_URI(&inst), \
	BT_TBS_CHR_CALL_STATE(&inst), \
	BT_TBS_CHR_CONTROL_POINT(&inst), \
	BT_TBS_CHR_OPTIONAL_OPCODES(&inst), \
	BT_TBS_CHR_TERMINATE_REASON(&inst), \
	BT_TBS_CHR_INCOMING_CALL(&inst), \
	BT_TBS_CHR_FRIENDLY_NAME(&inst) \
	}

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
 * Defining this as extern make it possible to link code that otherwise would
 * give "unknown identifier" linking errors.
 */
extern const struct bt_gatt_service_static gtbs_svc;

/* TODO: Can we make the multiple service instance more generic? */
#if CONFIG_BT_GTBS
BT_GATT_SERVICE_DEFINE(gtbs_svc, BT_GTBS_SERVICE_DEFINITION(&gtbs_inst));
#endif /* CONFIG_BT_GTBS */

BT_GATT_SERVICE_INSTANCE_DEFINE(tbs_service_list, svc_insts, CONFIG_BT_TBS_BEARER_COUNT,
				BT_TBS_SERVICE_DEFINITION);

static void signal_interval_timeout(struct k_work *work)
{
	struct tbs_service_inst *inst = lookup_inst_by_work(work);

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
	for (size_t i = 0; i < ARRAY_SIZE(svc_insts); i++) {
		int err;

		svc_insts[i].service_p = &tbs_service_list[i];

		err = bt_gatt_service_register(svc_insts[i].service_p);
		if (err != 0) {
			LOG_ERR("Could not register TBS[%d]: %d", i, err);
		}
	}

	if (IS_ENABLED(CONFIG_BT_GTBS)) {
		gtbs_inst.service_p = &gtbs_svc;
		(void)strcpy(gtbs_inst.provider_name, "Generic TBS");
		gtbs_inst.optional_opcodes = CONFIG_BT_TBS_SUPPORTED_FEATURES;
		gtbs_inst.ccid = bt_ccid_get_value();
		(void)strcpy(gtbs_inst.uci, "un000");

		k_work_init_delayable(&gtbs_inst.reporting_interval_work,
				      signal_interval_timeout);
	}

	for (int i = 0; i < ARRAY_SIZE(svc_insts); i++) {
		/* Init default values */
		svc_insts[i].index = i;
		svc_insts[i].ccid = bt_ccid_get_value();
		(void)strcpy(svc_insts[i].provider_name,
			     CONFIG_BT_TBS_PROVIDER_NAME);
		(void)strcpy(svc_insts[i].uci, CONFIG_BT_TBS_UCI);
		(void)strcpy(svc_insts[i].uri_scheme_list,
			     CONFIG_BT_TBS_URI_SCHEMES_LIST);
		svc_insts[i].optional_opcodes = CONFIG_BT_TBS_SUPPORTED_FEATURES;
		svc_insts[i].technology = CONFIG_BT_TBS_TECHNOLOGY;
		svc_insts[i].signal_strength_interval = CONFIG_BT_TBS_SIGNAL_STRENGTH_INTERVAL;
		svc_insts[i].status_flags = CONFIG_BT_TBS_STATUS_FLAGS;

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
	struct tbs_service_inst *inst = lookup_inst_by_call_index(call_index);
	int status = -EINVAL;
	const struct bt_tbs_call_cp_acc ccp = {
		.call_index = call_index,
		.opcode = BT_TBS_CALL_OPCODE_ACCEPT
	};

	if (inst != NULL) {
		status = accept_call(inst, &ccp);
	}

	if (status == BT_TBS_RESULT_CODE_SUCCESS) {
		notify_calls(inst);
	}

	return status;
}

int bt_tbs_hold(uint8_t call_index)
{
	struct tbs_service_inst *inst = lookup_inst_by_call_index(call_index);
	int status = -EINVAL;
	const struct bt_tbs_call_cp_hold ccp = {
		.call_index = call_index,
		.opcode = BT_TBS_CALL_OPCODE_HOLD
	};

	if (inst != NULL) {
		status = tbs_hold_call(inst, &ccp);
	}

	return status;
}

int bt_tbs_retrieve(uint8_t call_index)
{
	struct tbs_service_inst *inst = lookup_inst_by_call_index(call_index);
	int status = -EINVAL;
	const struct bt_tbs_call_cp_retrieve ccp = {
		.call_index = call_index,
		.opcode = BT_TBS_CALL_OPCODE_RETRIEVE
	};

	if (inst != NULL) {
		status = retrieve_call(inst, &ccp);
	}

	return status;
}

int bt_tbs_terminate(uint8_t call_index)
{
	struct tbs_service_inst *inst = lookup_inst_by_call_index(call_index);
	int status = -EINVAL;
	const struct bt_tbs_call_cp_term ccp = {
		.call_index = call_index,
		.opcode = BT_TBS_CALL_OPCODE_TERMINATE
	};

	if (inst != NULL) {
		status = terminate_call(inst, &ccp,
					BT_TBS_REASON_SERVER_ENDED_CALL);
	}

	return status;
}

int bt_tbs_originate(uint8_t bearer_index, char *remote_uri,
		     uint8_t *call_index)
{
	struct tbs_service_inst *inst;
	uint8_t buf[CONFIG_BT_TBS_MAX_URI_LENGTH +
		    sizeof(struct bt_tbs_call_cp_originate)];
	struct bt_tbs_call_cp_originate *ccp =
		(struct bt_tbs_call_cp_originate *)buf;
	size_t uri_len;

	if (bearer_index >= CONFIG_BT_TBS_BEARER_COUNT) {
		return -EINVAL;
	} else if (!bt_tbs_valid_uri(remote_uri)) {
		LOG_DBG("Invalid URI %s", remote_uri);
		return -EINVAL;
	}

	uri_len = strlen(remote_uri);

	inst = &svc_insts[bearer_index];

	ccp->opcode = BT_TBS_CALL_OPCODE_ORIGINATE;
	(void)memcpy(ccp->uri, remote_uri, uri_len);

	return originate_call(inst, ccp, uri_len, call_index);
}

int bt_tbs_join(uint8_t call_index_cnt, uint8_t *call_indexes)
{
	struct tbs_service_inst *inst;
	uint8_t buf[CONFIG_BT_TBS_MAX_CALLS +
		 sizeof(struct bt_tbs_call_cp_join)];
	struct bt_tbs_call_cp_join *ccp = (struct bt_tbs_call_cp_join *)buf;
	int status = -EINVAL;

	if (call_index_cnt != 0 && call_indexes != 0) {
		inst = lookup_inst_by_call_index(call_indexes[0]);
	} else {
		return status;
	}

	if (inst != NULL) {
		ccp->opcode = BT_TBS_CALL_OPCODE_JOIN;
		(void)memcpy(ccp->call_indexes, call_indexes,
		MIN(call_index_cnt, CONFIG_BT_TBS_MAX_CALLS));

		status = join_calls(inst, ccp, call_index_cnt);
	}

	return status;
}

int bt_tbs_remote_answer(uint8_t call_index)
{
	struct tbs_service_inst *inst = lookup_inst_by_call_index(call_index);
	struct bt_tbs_call *call;

	if (inst == NULL) {
		return BT_TBS_RESULT_CODE_INVALID_CALL_INDEX;
	}

	call = lookup_call_in_inst(inst, call_index);

	if (call == NULL) {
		return BT_TBS_RESULT_CODE_INVALID_CALL_INDEX;
	}

	if (call->state == BT_TBS_CALL_STATE_ALERTING) {
		call->state = BT_TBS_CALL_STATE_ACTIVE;
		notify_calls(inst);
		return BT_TBS_RESULT_CODE_SUCCESS;
	} else {
		return BT_TBS_RESULT_CODE_STATE_MISMATCH;
	}
}

int bt_tbs_remote_hold(uint8_t call_index)
{
	struct tbs_service_inst *inst = lookup_inst_by_call_index(call_index);
	struct bt_tbs_call *call;
	uint8_t status;

	if (inst == NULL) {
		return BT_TBS_RESULT_CODE_INVALID_CALL_INDEX;
	}

	call = lookup_call_in_inst(inst, call_index);

	if (call == NULL) {
		return BT_TBS_RESULT_CODE_INVALID_CALL_INDEX;
	}

	if (call->state == BT_TBS_CALL_STATE_ACTIVE) {
		call->state = BT_TBS_CALL_STATE_REMOTELY_HELD;
		status = BT_TBS_RESULT_CODE_SUCCESS;
	} else if (call->state == BT_TBS_CALL_STATE_LOCALLY_HELD) {
		call->state = BT_TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD;
		status = BT_TBS_RESULT_CODE_SUCCESS;
	} else {
		status = BT_TBS_RESULT_CODE_STATE_MISMATCH;
	}

	if (status == BT_TBS_RESULT_CODE_SUCCESS) {
		notify_calls(inst);
	}

	return status;
}

int bt_tbs_remote_retrieve(uint8_t call_index)
{
	struct tbs_service_inst *inst = lookup_inst_by_call_index(call_index);
	struct bt_tbs_call *call;
	int status;

	if (inst == NULL) {
		return BT_TBS_RESULT_CODE_INVALID_CALL_INDEX;
	}

	call = lookup_call_in_inst(inst, call_index);

	if (call == NULL) {
		return BT_TBS_RESULT_CODE_INVALID_CALL_INDEX;
	}

	if (call->state == BT_TBS_CALL_STATE_REMOTELY_HELD) {
		call->state = BT_TBS_CALL_STATE_ACTIVE;
		status = BT_TBS_RESULT_CODE_SUCCESS;
	} else if (call->state == BT_TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD) {
		call->state = BT_TBS_CALL_STATE_LOCALLY_HELD;
		status = BT_TBS_RESULT_CODE_SUCCESS;
	} else {
		status = BT_TBS_RESULT_CODE_STATE_MISMATCH;
	}

	if (status == BT_TBS_RESULT_CODE_SUCCESS) {
		notify_calls(inst);
	}

	return status;
}

int bt_tbs_remote_terminate(uint8_t call_index)
{
	struct tbs_service_inst *inst = lookup_inst_by_call_index(call_index);
	int status = -EINVAL;
	const struct bt_tbs_call_cp_term ccp = {
		.call_index = call_index,
		.opcode = BT_TBS_CALL_OPCODE_TERMINATE
	};

	if (inst != NULL) {
		status = terminate_call(inst, &ccp,
					BT_TBS_REASON_REMOTE_ENDED_CALL);
	}

	return status;
}

int bt_tbs_remote_incoming(uint8_t bearer_index, const char *to,
			   const char *from, const char *friendly_name)
{
	struct tbs_service_inst *inst;
	struct bt_tbs_call *call = NULL;
	size_t local_uri_ind_len;
	size_t remote_uri_ind_len;
	size_t friend_name_ind_len;

	if (bearer_index >= CONFIG_BT_TBS_BEARER_COUNT) {
		return -EINVAL;
	} else if (!bt_tbs_valid_uri(to)) {
		LOG_DBG("Invalid \"to\" URI: %s", to);
		return -EINVAL;
	} else if (!bt_tbs_valid_uri(from)) {
		LOG_DBG("Invalid \"from\" URI: %s", from);
		return -EINVAL;
	}

	local_uri_ind_len = strlen(to) + 1;
	remote_uri_ind_len = strlen(from) + 1;

	inst = &svc_insts[bearer_index];

	/* New call - Look for unused call item */
	for (int i = 0; i < CONFIG_BT_TBS_MAX_CALLS; i++) {
		if (inst->calls[i].index == BT_TBS_FREE_CALL_INDEX) {
			call = &inst->calls[i];
			break;
		}
	}

	if (call == NULL) {
		return -BT_TBS_RESULT_CODE_OUT_OF_RESOURCES;
	}

	call->index = next_free_call_index();

	if (call->index == BT_TBS_FREE_CALL_INDEX) {
		return -BT_TBS_RESULT_CODE_OUT_OF_RESOURCES;
	}

	BT_TBS_CALL_FLAG_SET_INCOMING(call->flags);

	(void)strcpy(call->remote_uri, from);
	call->state = BT_TBS_CALL_STATE_INCOMING;

	inst->in_call.call_index = call->index;
	(void)strcpy(inst->in_call.uri, from);

	inst->incoming_uri.call_index = call->index;
	(void)strcpy(inst->incoming_uri.uri, to);

	bt_gatt_notify_uuid(NULL, BT_UUID_TBS_INCOMING_URI,
			    inst->service_p->attrs,
			    &inst->incoming_uri, local_uri_ind_len);

	bt_gatt_notify_uuid(NULL, BT_UUID_TBS_INCOMING_CALL,
			    inst->service_p->attrs,
			    &inst->in_call, remote_uri_ind_len);

	if (friendly_name) {
		inst->friendly_name.call_index = call->index;
		(void)strcpy(inst->friendly_name.uri, friendly_name);
		friend_name_ind_len = strlen(from) + 1;

		bt_gatt_notify_uuid(NULL, BT_UUID_TBS_FRIENDLY_NAME,
				    inst->service_p->attrs,
				    &inst->friendly_name,
				    friend_name_ind_len);
	} else {
		inst->friendly_name.call_index = BT_TBS_FREE_CALL_INDEX;
		bt_gatt_notify_uuid(NULL, BT_UUID_TBS_FRIENDLY_NAME,
				    inst->service_p->attrs, NULL, 0);
	}

	if (IS_ENABLED(CONFIG_BT_GTBS)) {
		gtbs_inst.in_call.call_index = call->index;
		(void)strcpy(gtbs_inst.in_call.uri, from);

		gtbs_inst.incoming_uri.call_index = call->index;
		(void)strcpy(gtbs_inst.incoming_uri.uri, to);

		bt_gatt_notify_uuid(NULL, BT_UUID_TBS_INCOMING_URI,
				    gtbs_inst.service_p->attrs,
				    &gtbs_inst.incoming_uri, local_uri_ind_len);

		bt_gatt_notify_uuid(NULL, BT_UUID_TBS_INCOMING_CALL,
				    gtbs_inst.service_p->attrs,
				    &gtbs_inst.in_call, remote_uri_ind_len);

		if (friendly_name) {
			gtbs_inst.friendly_name.call_index = call->index;
			(void)strcpy(gtbs_inst.friendly_name.uri, friendly_name);
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

	LOG_DBG("New call with call index %u", call->index);

	return call->index;
}

int bt_tbs_set_bearer_provider_name(uint8_t bearer_index, const char *name)
{
	const size_t len = strlen(name);
	const struct bt_gatt_attr *attr;

	if (len >= CONFIG_BT_TBS_MAX_PROVIDER_NAME_LENGTH || len == 0) {
		return -EINVAL;
	} else if (bearer_index >= CONFIG_BT_TBS_BEARER_COUNT) {
		if (!(IS_ENABLED(CONFIG_BT_GTBS) &&
		    bearer_index == BT_TBS_GTBS_INDEX)) {
			return -EINVAL;
		}
	}

	if (bearer_index == BT_TBS_GTBS_INDEX) {
		if (strcmp(gtbs_inst.provider_name, name) == 0) {
			return 0;
		}

		(void)strcpy(gtbs_inst.provider_name, name);
		attr = gtbs_inst.service_p->attrs;
	} else {
		if (strcmp(svc_insts[bearer_index].provider_name, name) == 0) {
			return 0;
		}

		(void)strcpy(svc_insts[bearer_index].provider_name, name);
		attr = svc_insts[bearer_index].service_p->attrs;
	}

	bt_gatt_notify_uuid(NULL, BT_UUID_TBS_PROVIDER_NAME,
			    attr, name, strlen(name));
	return 0;
}

int bt_tbs_set_bearer_technology(uint8_t bearer_index, uint8_t new_technology)
{
	const struct bt_gatt_attr *attr;

	if (new_technology < BT_TBS_TECHNOLOGY_3G ||
	    new_technology > BT_TBS_TECHNOLOGY_IP) {
		return -EINVAL;
	} else if (bearer_index >= CONFIG_BT_TBS_BEARER_COUNT) {
		if (!(IS_ENABLED(CONFIG_BT_GTBS) &&
		    bearer_index == BT_TBS_GTBS_INDEX)) {
			return -EINVAL;
		}
	}

	if (bearer_index == BT_TBS_GTBS_INDEX) {
		if (gtbs_inst.technology == new_technology) {
			return 0;
		}

		gtbs_inst.technology = new_technology;
		attr = gtbs_inst.service_p->attrs;
	} else {
		if (svc_insts[bearer_index].technology == new_technology) {
			return 0;
		}

		svc_insts[bearer_index].technology = new_technology;
		attr = svc_insts[bearer_index].service_p->attrs;
	}

	bt_gatt_notify_uuid(NULL, BT_UUID_TBS_TECHNOLOGY,
			   attr, &new_technology, sizeof(new_technology));

	return 0;
}

int bt_tbs_set_signal_strength(uint8_t bearer_index,
			       uint8_t new_signal_strength)
{
	const struct bt_gatt_attr *attr;
	uint32_t timer_status;
	uint8_t interval;
	struct k_work_delayable *reporting_interval_work;
	struct tbs_service_inst *inst;

	if (new_signal_strength > BT_TBS_SIGNAL_STRENGTH_MAX &&
	    new_signal_strength != BT_TBS_SIGNAL_STRENGTH_UNKNOWN) {
		return -EINVAL;
	} else if (bearer_index >= CONFIG_BT_TBS_BEARER_COUNT) {
		if (!(IS_ENABLED(CONFIG_BT_GTBS) &&
		    bearer_index == BT_TBS_GTBS_INDEX)) {
			return -EINVAL;
		}
	}

	if (bearer_index == BT_TBS_GTBS_INDEX) {
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
		inst = &svc_insts[bearer_index];
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
		const k_timeout_t delay = K_SECONDS(interval);

		bt_gatt_notify_uuid(NULL, BT_UUID_TBS_SIGNAL_STRENGTH,
				    attr, &new_signal_strength,
				    sizeof(new_signal_strength));
		if (interval) {
			k_work_reschedule(reporting_interval_work, delay);
		}
	} else {
		if (bearer_index == BT_TBS_GTBS_INDEX) {
			LOG_DBG("GTBS: Reporting signal strength in %d ms", timer_status);
			gtbs_inst.pending_signal_strength_notification = true;

		} else {
			LOG_DBG("Index %u: Reporting signal strength in %d ms", bearer_index,
				timer_status);
			inst->pending_signal_strength_notification = true;
		}
	}

	return 0;
}

int bt_tbs_set_status_flags(uint8_t bearer_index, uint16_t status_flags)
{
	const struct bt_gatt_attr *attr;

	if (!BT_TBS_VALID_STATUS_FLAGS(status_flags)) {
		return -EINVAL;
	} else if (bearer_index >= CONFIG_BT_TBS_BEARER_COUNT) {
		if (!(IS_ENABLED(CONFIG_BT_GTBS) &&
		    bearer_index == BT_TBS_GTBS_INDEX)) {
			return -EINVAL;
		}
	}

	if (bearer_index == BT_TBS_GTBS_INDEX) {
		if (gtbs_inst.status_flags == status_flags) {
			return 0;
		}

		gtbs_inst.status_flags = status_flags;
		attr = gtbs_inst.service_p->attrs;
	} else {
		if (svc_insts[bearer_index].status_flags == status_flags) {
			return 0;
		}

		svc_insts[bearer_index].status_flags = status_flags;
		attr = svc_insts[bearer_index].service_p->attrs;
	}

	bt_gatt_notify_uuid(NULL, BT_UUID_TBS_STATUS_FLAGS,
			   attr, &status_flags, sizeof(status_flags));
	return 0;
}

int bt_tbs_set_uri_scheme_list(uint8_t bearer_index, const char **uri_list,
			       uint8_t uri_count)
{
	char uri_scheme_list[CONFIG_BT_TBS_MAX_SCHEME_LIST_LENGTH];
	size_t len = 0;
	struct tbs_service_inst *inst;

	if (bearer_index >= CONFIG_BT_TBS_BEARER_COUNT) {
		return -EINVAL;
	}

	inst = &svc_insts[bearer_index];
	(void)memset(uri_scheme_list, 0, sizeof(uri_scheme_list));

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
	(void)strcpy(inst->uri_scheme_list, uri_scheme_list);

	LOG_DBG("TBS instance %u uri prefix list is now %s", bearer_index, inst->uri_scheme_list);

	bt_gatt_notify_uuid(NULL, BT_UUID_TBS_URI_LIST,
			    inst->service_p->attrs, &inst->uri_scheme_list,
			    strlen(inst->uri_scheme_list));

	if (IS_ENABLED(CONFIG_BT_GTBS)) {
		NET_BUF_SIMPLE_DEFINE(uri_scheme_buf, READ_BUF_SIZE);

		/* TODO: Make uri schemes unique */
		for (int i = 0; i < ARRAY_SIZE(svc_insts); i++) {
			const size_t uri_len = strlen(svc_insts[i].uri_scheme_list);

			if (uri_scheme_buf.len + uri_len >= uri_scheme_buf.size) {
				LOG_WRN("Cannot fit all TBS instances in GTBS "
					"URI scheme list");
				break;
			}

			net_buf_simple_add_mem(&uri_scheme_buf,
					       svc_insts[i].uri_scheme_list,
					       uri_len);
		}

		/* Add null terminator for printing */
		uri_scheme_buf.data[uri_scheme_buf.len] = '\0';
		LOG_DBG("GTBS: URI scheme %s", uri_scheme_buf.data);

		bt_gatt_notify_uuid(NULL, BT_UUID_TBS_URI_LIST,
				    gtbs_inst.service_p->attrs,
				    uri_scheme_buf.data, uri_scheme_buf.len);
	}

	return 0;
}

void bt_tbs_register_cb(struct bt_tbs_cb *cbs)
{
	tbs_cbs = cbs;
}

#if defined(CONFIG_BT_TBS_LOG_LEVEL_DBG)
void bt_tbs_dbg_print_calls(void)
{
	for (int i = 0; i < CONFIG_BT_TBS_BEARER_COUNT; i++) {
		LOG_DBG("Bearer #%u", i);
		for (int j = 0; j < ARRAY_SIZE(svc_insts[i].calls); j++) {
			struct bt_tbs_call *call = &svc_insts[i].calls[j];

			if (call->index == BT_TBS_FREE_CALL_INDEX) {
				continue;
			}

			LOG_DBG("  Call #%u", call->index);
			LOG_DBG("    State: %s", bt_tbs_state_str(call->state));
			LOG_DBG("    Flags: 0x%02X", call->flags);
			LOG_DBG("    URI  : %s", call->remote_uri);
		}
	}
}
#endif /* defined(CONFIG_BT_TBS_LOG_LEVEL_DBG) */
