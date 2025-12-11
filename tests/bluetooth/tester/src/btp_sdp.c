/* btp_sdp.c - Bluetooth SDP Tester */

/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/atomic.h>
#include <zephyr/types.h>
#include <string.h>

#include <zephyr/toolchain.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/uuid.h>

#include <zephyr/logging/log.h>
#define LOG_MODULE_NAME bttester_sdp
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);

#include "btp/btp.h"

#define TEST_INSTANCES_MAX 10
#define TEST_ICON_URL      "http://pts.tester/public/icons/24x24x8.png"
#define TEST_DOC_URL       "http://pts.tester/public/readme.html"
#define TEST_CLNT_EXEC_URL "http://pts.tester/public/readme.html"

#define BT_SDP_ICON_URL(_url) \
	BT_SDP_LIST( \
		BT_SDP_ATTR_ICON_URL, \
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_URL_STR8, (sizeof(_url) - 1)), \
		_url \
	)

#define BT_SDP_DOC_URL(_url) \
	BT_SDP_LIST( \
		BT_SDP_ATTR_DOC_URL, \
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_URL_STR8, (sizeof(_url) - 1)), \
		_url \
	)

#define BT_SDP_CLNT_EXEC_URL(_url) \
	BT_SDP_LIST( \
		BT_SDP_ATTR_CLNT_EXEC_URL, \
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_URL_STR8, (sizeof(_url) - 1)), \
		_url \
	)

#define BT_SDP_SERVICE_DESCRIPTION(_dec) \
	BT_SDP_LIST( \
		BT_SDP_ATTR_SVCDESC_PRIMARY, \
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_TEXT_STR8, (sizeof(_dec)-1)), \
		_dec \
	)

#define BT_SDP_PROVIDER_NAME(_name) \
	BT_SDP_LIST( \
		BT_SDP_ATTR_PROVNAME_PRIMARY, \
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_TEXT_STR8, (sizeof(_name)-1)), \
		_name \
	)

/* HFP Hands-Free SDP record */
#define BT_SDP_TEST_ATT_DEFINE(channel) \
	BT_SDP_NEW_SERVICE, \
	BT_SDP_LIST( \
		BT_SDP_ATTR_SERVICE_ID, \
		BT_SDP_TYPE_SIZE(BT_SDP_UUID16), \
		BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP) \
	), \
	BT_SDP_LIST( \
		BT_SDP_ATTR_SVCINFO_TTL, \
		BT_SDP_TYPE_SIZE(BT_SDP_UINT32), \
		BT_SDP_ARRAY_32(0xFFFFFFFF) \
	), \
	BT_SDP_LIST( \
		BT_SDP_ATTR_SERVICE_AVAILABILITY, \
		BT_SDP_TYPE_SIZE(BT_SDP_UINT8), \
		BT_SDP_ARRAY_8(0xFF) \
	), \
	BT_SDP_ICON_URL(TEST_ICON_URL), \
	BT_SDP_DOC_URL(TEST_DOC_URL), \
	BT_SDP_CLNT_EXEC_URL(TEST_CLNT_EXEC_URL), \
	BT_SDP_SERVICE_NAME("tester"), \
	BT_SDP_SERVICE_DESCRIPTION("pts tester"), \
	BT_SDP_PROVIDER_NAME("zephyr"), \
	BT_SDP_LIST( \
		BT_SDP_ATTR_VERSION_NUM_LIST, \
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3), \
		BT_SDP_DATA_ELEM_LIST( \
		{ \
			BT_SDP_TYPE_SIZE(BT_SDP_UINT16), \
			BT_SDP_ARRAY_16(0x0100) \
		}, \
		) \
	), \
	BT_SDP_LIST( \
		BT_SDP_ATTR_SVCDB_STATE, \
		BT_SDP_TYPE_SIZE(BT_SDP_UINT32), \
		BT_SDP_ARRAY_32(0) \
	), \
	BT_SDP_LIST( \
		BT_SDP_ATTR_SVCLASS_ID_LIST, \
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 9), \
		BT_SDP_DATA_ELEM_LIST( \
		{ \
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16), \
			BT_SDP_ARRAY_16(BT_SDP_HANDSFREE_SVCLASS) \
		}, \
		{ \
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16), \
			BT_SDP_ARRAY_16(BT_SDP_GENERIC_AUDIO_SVCLASS) \
		}, \
		{ \
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16), \
			BT_SDP_ARRAY_16(BT_SDP_SDP_SERVER_SVCLASS) \
		} \
		) \
	), \
	BT_SDP_LIST( \
		BT_SDP_ATTR_PROTO_DESC_LIST, \
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 12), \
		BT_SDP_DATA_ELEM_LIST( \
		{ \
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3), \
			BT_SDP_DATA_ELEM_LIST( \
			{ \
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16), \
				BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP) \
			}, \
			) \
		}, \
		{ \
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 5), \
			BT_SDP_DATA_ELEM_LIST( \
			{ \
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16), \
				BT_SDP_ARRAY_16(BT_SDP_PROTO_RFCOMM) \
			}, \
			{ \
				BT_SDP_TYPE_SIZE(BT_SDP_UINT8), \
				BT_SDP_ARRAY_8(channel) \
			}, \
			) \
		}, \
		) \
	), \
	BT_SDP_LIST( \
		BT_SDP_ATTR_PROFILE_DESC_LIST, \
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6), \
		BT_SDP_DATA_ELEM_LIST( \
		{ \
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16), \
			BT_SDP_ARRAY_16(BT_SDP_HANDSFREE_SVCLASS) \
		}, \
		{ \
			BT_SDP_TYPE_SIZE(BT_SDP_UINT16), \
			BT_SDP_ARRAY_16(0x0109) \
		}, \
		) \
	), \
	BT_SDP_LIST( \
		BT_SDP_ATTR_ADD_PROTO_DESC_LIST, \
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 12), \
		BT_SDP_DATA_ELEM_LIST( \
		{ \
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3), \
			BT_SDP_DATA_ELEM_LIST( \
			{ \
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16), \
				BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP) \
			}, \
			) \
		}, \
		{ \
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 5), \
			BT_SDP_DATA_ELEM_LIST( \
			{ \
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16), \
				BT_SDP_ARRAY_16(BT_SDP_PROTO_RFCOMM) \
			}, \
			{ \
				BT_SDP_TYPE_SIZE(BT_SDP_UINT8), \
				BT_SDP_ARRAY_8(channel) \
			}, \
			) \
		}, \
		) \
	), \
	BT_SDP_SUPPORTED_FEATURES(0),

#define BT_SDP_TEST_RECORD_START 1

#define BT_SDP_TEST_RECORD_DEFINE(n)                                                        \
	{                                                                                   \
		BT_SDP_TEST_ATT_DEFINE(n + BT_SDP_TEST_RECORD_START)                        \
	}

#define _BT_SDP_RECORD_ITEM(n, _) &rec_##n

#define _BT_SDP_RECORD_DEFINE(n, _)                                                         \
	static struct bt_sdp_record rec_##n = BT_SDP_RECORD(attrs_##n)

#define _BT_SDP_ATTRS_ARRAY_DEFINE(n, _instances, _attrs_def)                               \
	static struct bt_sdp_attribute attrs_##n[] = _attrs_def(n)

#define BT_SDP_INSTANCE_DEFINE(_name, _instances, _instance_num, _attrs_def)                \
	BUILD_ASSERT(ARRAY_SIZE(_instances) == _instance_num,                               \
		     "The number of array elements does not match its size");               \
	LISTIFY(_instance_num, _BT_SDP_ATTRS_ARRAY_DEFINE, (;), _instances, _attrs_def);    \
	LISTIFY(_instance_num, _BT_SDP_RECORD_DEFINE, (;));                                 \
	static struct bt_sdp_record *_name[] = {                                            \
		LISTIFY(_instance_num, _BT_SDP_RECORD_ITEM, (,))                            \
	}

static uint8_t hfp_hf[TEST_INSTANCES_MAX];

BT_SDP_INSTANCE_DEFINE(hfp_hf_record_list, hfp_hf, TEST_INSTANCES_MAX, BT_SDP_TEST_RECORD_DEFINE);

static struct bt_sdp_discover_params sdp_discover;

NET_BUF_POOL_DEFINE(sdp_discover_pool, TEST_INSTANCES_MAX,
		    BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_TX_MTU),
		    CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static uint8_t supported_commands(const void *cmd, uint16_t cmd_len,
				  void *rsp, uint16_t *rsp_len)
{
	struct btp_sdp_read_supported_commands_rp *rp = rsp;

	*rsp_len = tester_supported_commands(BTP_SERVICE_ID_SDP, rp->data);
	*rsp_len += sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

uint8_t search_attr_req_cb(struct bt_conn *conn, struct bt_sdp_client_result *result,
			   const struct bt_sdp_discover_params *params)
{
	return BT_SDP_DISCOVER_UUID_CONTINUE;
}

#define SERVICE_RECORD_COUNT 5
#define RECV_CB_BUF_SIZE (sizeof(uint32_t) * SERVICE_RECORD_COUNT)
static uint8_t recv_cb_buf[RECV_CB_BUF_SIZE + sizeof(struct btp_sdp_service_record_handle_ev)];

uint8_t search_req_cb(struct bt_conn *conn, struct bt_sdp_client_result *result,
		      const struct bt_sdp_discover_params *params)
{
	struct btp_sdp_service_record_handle_ev *ev;
	uint32_t record_handle;
	uint8_t count;

	if (!result || !result->resp_buf) {
		return BT_SDP_DISCOVER_UUID_CONTINUE;
	}

	while (result->resp_buf->len >= sizeof(record_handle)) {
		ev = (void *)recv_cb_buf;

		bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(conn));
		ev->address.type = BTP_BR_ADDRESS_TYPE;
		count = 0;
		while (result->resp_buf->len >= sizeof(record_handle)) {
			record_handle = net_buf_pull_be32(result->resp_buf);
			ev->service_record_handle[count] = sys_cpu_to_le32(record_handle);
			count++;
			if (count >= SERVICE_RECORD_COUNT) {
				break;
			}
		}
		ev->service_record_handle_count = count;

		tester_event(BTP_SERVICE_ID_SDP, BTP_SDP_EV_SERVICE_RECORD_HANDLE, ev,
			     sizeof(*ev) + sizeof(record_handle) * count);
	}

	return BT_SDP_DISCOVER_UUID_CONTINUE;
}

uint8_t attr_req_cb(struct bt_conn *conn, struct bt_sdp_client_result *result,
		    const struct bt_sdp_discover_params *params)
{
	return BT_SDP_DISCOVER_UUID_CONTINUE;
}

union sdp_uuid {
	struct bt_uuid uuid;
	struct bt_uuid_16 u16;
	struct bt_uuid_32 u32;
	struct bt_uuid_128 u128;
};

/* Convert UUID from BTP command to bt_uuid */
static uint8_t btp2bt_uuid(const uint8_t *uuid, uint8_t len, struct bt_uuid *u)
{
	uint16_t le16;
	uint32_t le32;

	switch (len) {
	case 0x02: /* UUID 16 */
		u->type = BT_UUID_TYPE_16;
		memcpy(&le16, uuid, sizeof(le16));
		BT_UUID_16(u)->val = sys_le16_to_cpu(le16);
		break;
	case 0x04: /* UUID 32 */
		u->type = BT_UUID_TYPE_32;
		memcpy(&le32, uuid, sizeof(le32));
		BT_UUID_32(u)->val = sys_le32_to_cpu(le32);
		break;
	case 0x10: /* UUID 128*/
		u->type = BT_UUID_TYPE_128;
		memcpy(BT_UUID_128(u)->val, uuid, 16);
		break;
	default:
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t search_req(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	struct bt_conn *conn;
	const struct btp_sdp_search_req_cmd *req;
	static union sdp_uuid u;

	req = (const struct btp_sdp_search_req_cmd *)cmd;
	if (req->address.type != BTP_BR_ADDRESS_TYPE) {
		return BTP_STATUS_FAILED;
	}

	if (btp2bt_uuid(req->uuid, req->uuid_length, &u.uuid) != BTP_STATUS_SUCCESS) {
		return BTP_STATUS_FAILED;
	}

	conn = bt_conn_lookup_addr_br(&req->address.a);
	if (!conn) {
		return BTP_STATUS_FAILED;
	}

	sdp_discover.type = BT_SDP_DISCOVER_SERVICE_SEARCH;
	sdp_discover.pool = &sdp_discover_pool;
	sdp_discover.func = search_req_cb;
	sdp_discover.uuid = &u.uuid;

	if (bt_sdp_discover(conn, &sdp_discover)) {
		goto failed;
	}

	bt_conn_unref(conn);
	return BTP_STATUS_SUCCESS;

failed:
	bt_conn_unref(conn);
	return BTP_STATUS_FAILED;
}

static uint8_t attr_req(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	struct bt_conn *conn;
	const struct btp_sdp_attr_req_cmd *req;

	req = (const struct btp_sdp_attr_req_cmd *)cmd;
	if (req->address.type != BTP_BR_ADDRESS_TYPE) {
		return BTP_STATUS_FAILED;
	}

	conn = bt_conn_lookup_addr_br(&req->address.a);
	if (!conn) {
		return BTP_STATUS_FAILED;
	}

	sdp_discover.type = BT_SDP_DISCOVER_SERVICE_ATTR;
	sdp_discover.pool = &sdp_discover_pool;
	sdp_discover.func = attr_req_cb;
	sdp_discover.handle = sys_le32_to_cpu(req->service_record_handle);

	if (bt_sdp_discover(conn, &sdp_discover)) {
		goto failed;
	}

	bt_conn_unref(conn);
	return BTP_STATUS_SUCCESS;

failed:
	bt_conn_unref(conn);
	return BTP_STATUS_FAILED;
}

static uint8_t search_attr_req(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	struct bt_conn *conn;
	const struct btp_sdp_search_attr_req_cmd *req;
	static union sdp_uuid u;

	req = (const struct btp_sdp_search_attr_req_cmd *)cmd;
	if (req->address.type != BTP_BR_ADDRESS_TYPE) {
		return BTP_STATUS_FAILED;
	}

	if (btp2bt_uuid(req->uuid, req->uuid_length, &u.uuid) != BTP_STATUS_SUCCESS) {
		return BTP_STATUS_FAILED;
	}

	conn = bt_conn_lookup_addr_br(&req->address.a);
	if (!conn) {
		return BTP_STATUS_FAILED;
	}

	sdp_discover.type = BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR;
	sdp_discover.pool = &sdp_discover_pool;
	sdp_discover.func = search_attr_req_cb;
	sdp_discover.uuid = &u.uuid;

	if (bt_sdp_discover(conn, &sdp_discover)) {
		goto failed;
	}

	bt_conn_unref(conn);
	return BTP_STATUS_SUCCESS;

failed:
	bt_conn_unref(conn);
	return BTP_STATUS_FAILED;
}

static const struct btp_handler handlers[] = {
	{
		.opcode = BTP_SDP_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = supported_commands,
	},
	{
		.opcode = BTP_SDP_SEARCH_REQ,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = search_req,
	},
	{
		.opcode = BTP_SDP_ATTR_REQ,
		.expect_len = sizeof(struct btp_sdp_attr_req_cmd),
		.func = attr_req,
	},
	{
		.opcode = BTP_SDP_SEARCH_ATTR_REQ,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = search_attr_req,
	},
};

uint8_t tester_init_sdp(void)
{
	static bool initialized;
	int err;

	if (!initialized) {
		ARRAY_FOR_EACH(hfp_hf_record_list, index) {
			err = bt_sdp_register_service(hfp_hf_record_list[index]);
			if (err) {
				return BTP_STATUS_FAILED;
			}
		}
		initialized = true;
	}

	tester_register_command_handlers(BTP_SERVICE_ID_SDP, handlers, ARRAY_SIZE(handlers));

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_sdp(void)
{
	return BTP_STATUS_SUCCESS;
}
