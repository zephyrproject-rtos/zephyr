/* btp_sdp.c - Bluetooth SDP Tester */

/*
 * Copyright 2024 NXP
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

uint8_t hfp_hf[TEST_INSTANCES_MAX];

BT_SDP_INSTANCE_DEFINE(hfp_hf_record_list, hfp_hf, TEST_INSTANCES_MAX, BT_SDP_TEST_RECORD_DEFINE);

static struct bt_sdp_discover_params sdp_discover;

NET_BUF_POOL_DEFINE(sdp_discover_pool, 10, BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_TX_MTU),
		    CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

uint8_t search_attr_req_cb(struct bt_conn *conn, struct bt_sdp_client_result *result,
			   const struct bt_sdp_discover_params *params)
{
	return BT_SDP_DISCOVER_UUID_CONTINUE;
}

uint8_t search_req_cb(struct bt_conn *conn, struct bt_sdp_client_result *result,
		      const struct bt_sdp_discover_params *params)
{
	struct btp_sdp_service_record_handle_ev ev;
	struct bt_conn_info info;

	if (!result || !result->resp_buf) {
		return BT_SDP_DISCOVER_UUID_CONTINUE;
	}

	if (result->resp_buf->len < sizeof(ev.service_record_handle)) {
		return BT_SDP_DISCOVER_UUID_CONTINUE;
	}

	bt_conn_get_info(conn, &info);

	bt_addr_copy(&ev.address, info.br.dst);
	ev.service_record_handle = net_buf_pull_be32(result->resp_buf);

	tester_event(BTP_SERVICE_ID_SDP, BTP_SDP_EV_SERVICE_RECORD_HANDLE, &ev, sizeof(ev));
	return BT_SDP_DISCOVER_UUID_CONTINUE;
}

uint8_t attr_req_cb(struct bt_conn *conn, struct bt_sdp_client_result *result,
		    const struct bt_sdp_discover_params *params)
{
	return BT_SDP_DISCOVER_UUID_CONTINUE;
}

extern struct bt_conn *bt_conn_lookup_addr_br(const bt_addr_t *peer);

static uint8_t search_req(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	struct bt_conn *conn;
	struct btp_sdp_search_req *req;
	static struct bt_uuid_16 uuid_16 = BT_UUID_INIT_16(0);

	req = (struct btp_sdp_search_req *)cmd;
	conn = bt_conn_lookup_addr_br(&req->address);
	if (!conn) {
		return BTP_STATUS_FAILED;
	}

	uuid_16.val = req->uuid;

	sdp_discover.type = BT_SDP_DISCOVER_SERVICE_SEARCH;
	sdp_discover.pool = &sdp_discover_pool;
	sdp_discover.func = search_req_cb;
	sdp_discover.uuid = &uuid_16.uuid;

	if (bt_sdp_discover(conn, &sdp_discover)) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t attr_req(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	struct bt_conn *conn;
	struct btp_sdp_attr_req *req;

	req = (struct btp_sdp_attr_req *)cmd;
	conn = bt_conn_lookup_addr_br(&req->address);
	if (!conn) {
		return BTP_STATUS_FAILED;
	}

	sdp_discover.type = BT_SDP_DISCOVER_SERVICE_ATTR;
	sdp_discover.pool = &sdp_discover_pool;
	sdp_discover.func = attr_req_cb;
	sdp_discover.handle = req->service_record_handle;

	if (bt_sdp_discover(conn, &sdp_discover)) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t search_attr_req(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	struct bt_conn *conn;
	struct btp_sdp_search_attr_req *req;
	static struct bt_uuid_16 uuid_16 = BT_UUID_INIT_16(0);

	req = (struct btp_sdp_search_attr_req *)cmd;
	conn = bt_conn_lookup_addr_br(&req->address);
	if (!conn) {
		return BTP_STATUS_FAILED;
	}

	uuid_16.val = req->uuid;

	sdp_discover.type = BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR;
	sdp_discover.pool = &sdp_discover_pool;
	sdp_discover.func = search_attr_req_cb;
	sdp_discover.uuid = &uuid_16.uuid;

	if (bt_sdp_discover(conn, &sdp_discover)) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static const struct btp_handler handlers[] = {
	{
		.opcode = BTP_SDP_SEARCH_REQ,
		.index = BTP_INDEX_NONE,
		.expect_len = sizeof(struct btp_sdp_search_req),
		.func = search_req,
	},
	{
		.opcode = BTP_SDP_ATTR_REQ,
		.index = BTP_INDEX_NONE,
		.expect_len = sizeof(struct btp_sdp_attr_req),
		.func = attr_req,
	},
	{
		.opcode = BTP_SDP_SEARCH_ATTR_REQ,
		.index = BTP_INDEX_NONE,
		.expect_len = sizeof(struct btp_sdp_search_attr_req),
		.func = search_attr_req,
	},
};

uint8_t tester_init_sdp(void)
{
	static bool initialized;
	int err = -EINVAL;

	if (!initialized) {
		for (size_t index = 0; index < ARRAY_SIZE(hfp_hf_record_list); index++) {
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
