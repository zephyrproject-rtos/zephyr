/* att_br.c - Attribute Protocol bearer over BR/EDR */

/*
 * Copyright 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/util.h>

#include "host/att_internal.h"
#include "host/conn_internal.h"
#include "host/hci_core.h"

#define LOG_LEVEL CONFIG_BT_ATT_LOG_LEVEL
LOG_MODULE_REGISTER(bt_att_br);

static int att_br_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
			 struct bt_l2cap_chan **chan)
{
	ARG_UNUSED(server);

	return bt_att_br_chan_accept(conn, chan);
}

/* Core v6.2 Vol 3 Part G 5.1.2: the channel shall be encrypted */
static struct bt_l2cap_server att_br_server = {
	.psm = BT_ATT_PSM,
	.sec_level = BT_SECURITY_L2,
	.accept = att_br_accept,
};

int bt_att_br_connect(struct bt_conn *conn)
{
	if (conn == NULL || !bt_conn_is_br(conn)) {
		return -EINVAL;
	}

	if (conn->state != BT_CONN_CONNECTED) {
		return -ENOTCONN;
	}

	return bt_att_br_chan_connect(conn);
}

int bt_att_br_disconnect(struct bt_conn *conn)
{
	if (conn == NULL || !bt_conn_is_br(conn)) {
		return -EINVAL;
	}

	return bt_att_br_chan_disconnect(conn);
}

/* SDP record of a primary service, Core v6.2 Vol 3 Part G 9 and Part C 15.4:
 * ServiceClassIDList with the service UUID, ProtocolDescriptorList with
 * L2CAP (ATT PSM) and ATT (service handle range), and the public browse group.
 */
struct att_br_sdp_record {
	struct bt_sdp_record record;
	struct bt_sdp_attribute attrs[4];
	struct bt_sdp_data_elem class_list[1];
	struct bt_sdp_data_elem proto_list[2];
	struct bt_sdp_data_elem att_proto[3];
	uint32_t record_handle;
	uint16_t start_handle;
	uint16_t end_handle;
	union {
		uint16_t u16;
		uint32_t u32;
		uint8_t u128[BT_UUID_SIZE_128];
	} uuid;
	bool registered;
};

static struct att_br_sdp_record sdp_records[CONFIG_BT_ATT_BR_SDP_RECORDS];

static const struct bt_sdp_data_elem sdp_l2cap_proto[] = {
	{BT_SDP_TYPE_SIZE(BT_SDP_UUID16), BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP)},
	{BT_SDP_TYPE_SIZE(BT_SDP_UINT16), BT_SDP_ARRAY_16(BT_ATT_PSM)},
};

static const struct bt_sdp_data_elem sdp_browse_group[] = {
	{BT_SDP_TYPE_SIZE(BT_SDP_UUID16), BT_SDP_ARRAY_16(BT_SDP_PUBLIC_BROWSE_GROUP)},
};

static const uint16_t sdp_att_uuid = BT_SDP_PROTO_ATT;

static void sdp_record_init(struct att_br_sdp_record *rec, const struct bt_uuid *uuid,
			    uint16_t handle)
{
	struct bt_sdp_data_elem *class_id = &rec->class_list[0];

	(void)memset(rec, 0, sizeof(*rec));

	switch (uuid->type) {
	case BT_UUID_TYPE_16:
		rec->uuid.u16 = BT_UUID_16(uuid)->val;
		*class_id =
			(struct bt_sdp_data_elem){BT_SDP_TYPE_SIZE(BT_SDP_UUID16), &rec->uuid.u16};
		break;
	case BT_UUID_TYPE_32:
		rec->uuid.u32 = BT_UUID_32(uuid)->val;
		*class_id =
			(struct bt_sdp_data_elem){BT_SDP_TYPE_SIZE(BT_SDP_UUID32), &rec->uuid.u32};
		break;
	default:
		(void)memcpy(rec->uuid.u128, BT_UUID_128(uuid)->val, sizeof(rec->uuid.u128));
		*class_id =
			(struct bt_sdp_data_elem){BT_SDP_TYPE_SIZE(BT_SDP_UUID128), rec->uuid.u128};
		break;
	}

	rec->start_handle = handle;
	rec->end_handle = handle;

	rec->att_proto[0] =
		(struct bt_sdp_data_elem){BT_SDP_TYPE_SIZE(BT_SDP_UUID16), &sdp_att_uuid};
	rec->att_proto[1] =
		(struct bt_sdp_data_elem){BT_SDP_TYPE_SIZE(BT_SDP_UINT16), &rec->start_handle};
	rec->att_proto[2] =
		(struct bt_sdp_data_elem){BT_SDP_TYPE_SIZE(BT_SDP_UINT16), &rec->end_handle};

	rec->proto_list[0] = (struct bt_sdp_data_elem){
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8,
				     sdp_l2cap_proto[0].total_size + sdp_l2cap_proto[1].total_size),
		sdp_l2cap_proto};
	rec->proto_list[1] = (struct bt_sdp_data_elem){
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, rec->att_proto[0].total_size +
							  rec->att_proto[1].total_size +
							  rec->att_proto[2].total_size),
		rec->att_proto};

	/* Attributes in ascending ID order, the record handle first */
	rec->attrs[0] = (struct bt_sdp_attribute){
		BT_SDP_ATTR_RECORD_HANDLE, {BT_SDP_TYPE_SIZE(BT_SDP_UINT32), &rec->record_handle}};
	rec->attrs[1] = (struct bt_sdp_attribute){
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		{BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, class_id->total_size), rec->class_list}};
	rec->attrs[2] = (struct bt_sdp_attribute){
		BT_SDP_ATTR_PROTO_DESC_LIST,
		{BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, rec->proto_list[0].total_size +
							   rec->proto_list[1].total_size),
		 rec->proto_list}};
	rec->attrs[3] = (struct bt_sdp_attribute){
		BT_SDP_ATTR_BROWSE_GRP_LIST,
		{BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, sdp_browse_group[0].total_size),
		 sdp_browse_group}};

	rec->record.attrs = rec->attrs;
	rec->record.attr_count = ARRAY_SIZE(rec->attrs);
}

/* The GAP and GATT services always have a record, other services only if
 * defined with BT_ATT_BR_SDP_SERVICE().
 */
static bool sdp_service_published(const struct bt_uuid *uuid)
{
	if (bt_uuid_cmp(uuid, BT_UUID_GAP) == 0 || bt_uuid_cmp(uuid, BT_UUID_GATT) == 0) {
		return true;
	}

	STRUCT_SECTION_FOREACH(bt_att_br_sdp_service, svc) {
		if (bt_uuid_cmp(uuid, svc->uuid) == 0) {
			return true;
		}
	}

	return false;
}

struct sdp_build_data {
	struct att_br_sdp_record *cur;
	size_t count;
	size_t dropped;
};

static uint8_t sdp_build_cb(const struct bt_gatt_attr *attr, uint16_t handle, void *user_data)
{
	struct sdp_build_data *data = user_data;

	if (bt_uuid_cmp(attr->uuid, BT_UUID_GATT_PRIMARY) == 0) {
		data->cur = NULL;

		if (!sdp_service_published(attr->user_data)) {
			return BT_GATT_ITER_CONTINUE;
		}

		if (data->count == ARRAY_SIZE(sdp_records)) {
			data->dropped++;
			return BT_GATT_ITER_CONTINUE;
		}

		data->cur = &sdp_records[data->count++];
		sdp_record_init(data->cur, attr->user_data, handle);

		return BT_GATT_ITER_CONTINUE;
	}

	if (bt_uuid_cmp(attr->uuid, BT_UUID_GATT_SECONDARY) == 0) {
		/* A secondary service ends the group of the previous service */
		data->cur = NULL;
		return BT_GATT_ITER_CONTINUE;
	}

	if (data->cur != NULL) {
		data->cur->end_handle = handle;
	}

	return BT_GATT_ITER_CONTINUE;
}

static void sdp_update_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(sdp_update_work, sdp_update_handler);

static void sdp_update_handler(struct k_work *work)
{
	struct sdp_build_data data = {0};

	ARG_UNUSED(work);

	/* Records are only rebuilt once none of them is in the SDP database */
	ARRAY_FOR_EACH_PTR(sdp_records, rec) {
		int err;

		if (!rec->registered) {
			continue;
		}

		err = bt_sdp_unregister_service(&rec->record);
		if (err == -EBUSY) {
			/* Retry once the SDP session has been closed */
			bt_work_reschedule(&sdp_update_work, K_SECONDS(1));
			return;
		}

		rec->registered = false;
	}

	bt_gatt_foreach_attr(BT_ATT_FIRST_ATTRIBUTE_HANDLE, BT_ATT_LAST_ATTRIBUTE_HANDLE,
			     sdp_build_cb, &data);

	if (data.dropped > 0) {
		LOG_WRN("%zu services not published in SDP, increase CONFIG_BT_ATT_BR_SDP_RECORDS",
			data.dropped);
	}

	for (size_t i = 0; i < data.count; i++) {
		struct att_br_sdp_record *rec = &sdp_records[i];
		int err;

		err = bt_sdp_register_service(&rec->record);
		if (err != 0) {
			LOG_ERR("Failed to register SDP record for handle 0x%04x (err %d)",
				rec->start_handle, err);
			continue;
		}

		rec->registered = true;

		LOG_DBG("SDP record 0x%08x for handles 0x%04x-0x%04x", rec->record_handle,
			rec->start_handle, rec->end_handle);
	}
}

void bt_att_br_db_changed(void)
{
	bt_work_reschedule(&sdp_update_work, K_NO_WAIT);
}

void bt_att_br_init(void)
{
	int err;

	err = bt_l2cap_br_server_register(&att_br_server);
	if (err != 0) {
		LOG_ERR("ATT L2CAP server register failed (%d)", err);
		return;
	}

	sdp_update_handler(NULL);
}
