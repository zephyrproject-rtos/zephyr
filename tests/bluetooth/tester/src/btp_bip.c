/*
 * SPDX-FileCopyrightText: Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/classic/bip.h>
#include <zephyr/bluetooth/classic/goep.h>
#include <zephyr/bluetooth/classic/obex.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
#define LOG_MODULE_NAME btp_bip
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);

#include "btp/btp.h"

#define BIP_MAX_INSTANCES CONFIG_BT_MAX_CONN

#define BIP_POOL_BUF_SIZE					\
	MAX(BT_RFCOMM_BUF_SIZE(CONFIG_BT_GOEP_RFCOMM_MTU),	\
	    BT_L2CAP_BUF_SIZE(CONFIG_BT_GOEP_L2CAP_MTU))

#define BIP_TX_POOL_COUNT (BIP_MAX_INSTANCES * 2 + 2)

NET_BUF_POOL_FIXED_DEFINE(bip_tx_pool, BIP_TX_POOL_COUNT, BIP_POOL_BUF_SIZE,
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

struct bip_app {
	struct bt_bip_client client;
	struct bt_bip bip;
	struct bt_bip_server server;
	struct bt_bip_client second_client;
	struct bt_bip_server second_server;
	struct bt_bip second_bip;
	struct bt_conn *conn;
	struct bt_conn *second_conn;
	bt_addr_t address;

	uint32_t conn_id;
	uint32_t second_conn_id;
	bool in_use;

	bool primary_registered;
	bool secondary_registered;
};

static struct bip_app bip_apps[BIP_MAX_INSTANCES];

static struct bt_bip_rfcomm_server rfcomm_server;
static struct bt_bip_l2cap_server l2cap_server;

static struct bt_bip_rfcomm_server archive_rfcomm_server;
static struct bt_bip_l2cap_server archive_l2cap_server;

static struct bt_bip_rfcomm_server refobj_rfcomm_server;
static struct bt_bip_l2cap_server refobj_l2cap_server;

static uint16_t bip_l2cap_psm = 0x1009;
static uint8_t bip_rfcomm_channel = 0x9;
static uint8_t bip_supported_caps = BIT(BT_BIP_SUPP_CAP_GENERIC_IMAGE) |
				    BIT(BT_BIP_SUPP_CAP_CAPTURING) |
				    BIT(BT_BIP_SUPP_CAP_PRINTING) |
				    BIT(BT_BIP_SUPP_CAP_DISPLAYING);
static uint16_t bip_supported_features = BIT(BT_BIP_SUPP_FEAT_IMAGE_PUSH) |
					 BIT(BT_BIP_SUPP_FEAT_IMAGE_PUSH_STORE) |
					 BIT(BT_BIP_SUPP_FEAT_IMAGE_PUSH_PRINT) |
					 BIT(BT_BIP_SUPP_FEAT_IMAGE_PUSH_DISPLAY) |
					 BIT(BT_BIP_SUPP_FEAT_IMAGE_PULL) |
					 BIT(BT_BIP_SUPP_FEAT_ADVANCED_IMAGE_PRINT) |
					 BIT(BT_BIP_SUPP_FEAT_AUTO_ARCHIVE) |
					 BIT(BT_BIP_SUPP_FEAT_REMOTE_CAMERA) |
					 BIT(BT_BIP_SUPP_FEAT_REMOTE_DISPLAY);
static uint32_t bip_supported_functions = BIT(BT_BIP_SUPP_FUNC_GET_CAPS) |
					  BIT(BT_BIP_SUPP_FUNC_PUT_IMAGE) |
					  BIT(BT_BIP_SUPP_FUNC_PUT_LINKED_ATTACHMENT) |
					  BIT(BT_BIP_SUPP_FUNC_PUT_LINKED_THUMBNAIL) |
					  BIT(BT_BIP_SUPP_FUNC_REMOTE_DISPLAY) |
					  BIT(BT_BIP_SUPP_FUNC_GET_IMAGE_LIST) |
					  BIT(BT_BIP_SUPP_FUNC_GET_IMAGE_PROPERTIES) |
					  BIT(BT_BIP_SUPP_FUNC_GET_IMAGE) |
					  BIT(BT_BIP_SUPP_FUNC_GET_LINKED_THUMBNAIL) |
					  BIT(BT_BIP_SUPP_FUNC_GET_LINKED_ATTACHMENT) |
					  BIT(BT_BIP_SUPP_FUNC_DELETE_IMAGE) |
					  BIT(BT_BIP_SUPP_FUNC_START_PRINT) |
					  BIT(BT_BIP_SUPP_FUNC_GET_PARTIAL_IMAGE) |
					  BIT(BT_BIP_SUPP_FUNC_START_ARCHIVE) |
					  BIT(BT_BIP_SUPP_FUNC_GET_MONITORING_IMAGE) |
					  BIT(BT_BIP_SUPP_FUNC_GET_STATUS);
static uint64_t bip_max_memory_space = 1024;

static uint16_t bip_archive_l2cap_psm = 0x100b;
static uint8_t bip_archive_rfcomm_channel = 0x0a;
static uint32_t bip_archive_supported_functions = BIT(BT_BIP_SUPP_FUNC_GET_CAPS) |
						   BIT(BT_BIP_SUPP_FUNC_GET_IMAGE_LIST) |
						   BIT(BT_BIP_SUPP_FUNC_GET_IMAGE_PROPERTIES) |
						   BIT(BT_BIP_SUPP_FUNC_GET_IMAGE) |
						   BIT(BT_BIP_SUPP_FUNC_GET_LINKED_THUMBNAIL) |
						   BIT(BT_BIP_SUPP_FUNC_GET_LINKED_ATTACHMENT) |
						   BIT(BT_BIP_SUPP_FUNC_DELETE_IMAGE);

static uint16_t bip_refobj_l2cap_psm = 0x100d;
static uint8_t bip_refobj_rfcomm_channel = 0x0b;
static uint32_t bip_refobj_supported_functions = BIT(BT_BIP_SUPP_FUNC_GET_CAPS) |
						  BIT(BT_BIP_SUPP_FUNC_GET_PARTIAL_IMAGE);

static struct bt_sdp_attribute bip_responder_attrs[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			BT_SDP_ARRAY_16(BT_SDP_IMAGING_RESPONDER_SVCLASS)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROTO_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 17),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP)
			},
			)
		},
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 5),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_RFCOMM)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
				&bip_rfcomm_channel
			},
			)
		},
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_OBEX)
			},
			)
		},
		)
	),
	BT_SDP_SERVICE_NAME("imaging"),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROFILE_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_IMAGING_SVCLASS)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(0x0102)
			},
			)
		},
		)
	),
	{
		BT_SDP_ATTR_SUPPORTED_CAPABILITIES,
		{BT_SDP_TYPE_SIZE(BT_SDP_UINT8), &bip_supported_caps},
	},
	{
		BT_SDP_ATTR_SUPPORTED_FEATURES,
		{BT_SDP_TYPE_SIZE(BT_SDP_UINT16), &bip_supported_features},
	},
	{
		BT_SDP_ATTR_SUPPORTED_FUNCTIONS,
		{BT_SDP_TYPE_SIZE(BT_SDP_UINT32), &bip_supported_functions},
	},
	{
		BT_SDP_ATTR_TOTAL_IMAGING_DATA_CAPACITY,
		{BT_SDP_TYPE_SIZE(BT_SDP_UINT64), &bip_max_memory_space},
	},
	{
		BT_SDP_ATTR_GOEP_L2CAP_PSM,
		{BT_SDP_TYPE_SIZE(BT_SDP_UINT16), &bip_l2cap_psm},
	},
};

static struct bt_sdp_record bip_responder_rec = BT_SDP_RECORD(bip_responder_attrs);

static struct bt_sdp_attribute bip_archive_attrs[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			BT_SDP_ARRAY_16(BT_SDP_IMAGING_ARCHIVE_SVCLASS)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROTO_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 17),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP)
			},
			)
		},
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 5),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_RFCOMM)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
				&bip_archive_rfcomm_channel
			},
			)
		},
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_OBEX)
			},
			)
		},
		)
	),
	BT_SDP_SERVICE_NAME("imaging_archive"),
	BT_SDP_SERVICE_ID(BT_UUID_INIT_16(BT_SDP_IMAGING_ARCHIVE_SVCLASS)),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROFILE_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_IMAGING_SVCLASS)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(0x0102)
			},
			)
		},
		)
	),
	{
		BT_SDP_ATTR_SUPPORTED_FUNCTIONS,
		{BT_SDP_TYPE_SIZE(BT_SDP_UINT32), &bip_archive_supported_functions},
	},
	{
		BT_SDP_ATTR_GOEP_L2CAP_PSM,
		{BT_SDP_TYPE_SIZE(BT_SDP_UINT16), &bip_archive_l2cap_psm},
	},
};

static struct bt_sdp_record bip_archive_rec = BT_SDP_RECORD(bip_archive_attrs);

static struct bt_sdp_attribute bip_refobj_attrs[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			BT_SDP_ARRAY_16(BT_SDP_IMAGING_REFOBJS_SVCLASS)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROTO_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 17),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP)
			},
			)
		},
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 5),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_RFCOMM)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
				&bip_refobj_rfcomm_channel
			},
			)
		},
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_OBEX)
			},
			)
		},
		)
	),
	BT_SDP_SERVICE_NAME("imaging_referenced_objects"),
	BT_SDP_SERVICE_ID(BT_UUID_INIT_16(BT_SDP_IMAGING_REFOBJS_SVCLASS)),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROFILE_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_IMAGING_SVCLASS)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(0x0102)
			},
			)
		},
		)
	),
	{
		BT_SDP_ATTR_SUPPORTED_FUNCTIONS,
		{BT_SDP_TYPE_SIZE(BT_SDP_UINT32), &bip_refobj_supported_functions},
	},
	{
		BT_SDP_ATTR_GOEP_L2CAP_PSM,
		{BT_SDP_TYPE_SIZE(BT_SDP_UINT16), &bip_refobj_l2cap_psm},
	},
};

static struct bt_sdp_record bip_refobj_rec = BT_SDP_RECORD(bip_refobj_attrs);

static K_MUTEX_DEFINE(bip_apps_lock);

static struct bip_app *find_instance_by_address(const bt_addr_t *address)
{
	struct bip_app *found = NULL;

	k_mutex_lock(&bip_apps_lock, K_FOREVER);

	for (uint8_t i = 0; i < BIP_MAX_INSTANCES; i++) {
		if (bip_apps[i].in_use && bip_apps[i].conn != NULL &&
		    bt_addr_eq(&bip_apps[i].address, address)) {
			found = &bip_apps[i];
			break;
		}
	}

	k_mutex_unlock(&bip_apps_lock);

	return found;
}

static struct bip_app *find_any_instance_by_address(const bt_addr_t *address)
{
	struct bip_app *connection_less = NULL;
	struct bip_app *found = NULL;

	k_mutex_lock(&bip_apps_lock, K_FOREVER);

	for (uint8_t i = 0; i < BIP_MAX_INSTANCES; i++) {
		if (!bip_apps[i].in_use || !bt_addr_eq(&bip_apps[i].address, address)) {
			continue;
		}

		if (bip_apps[i].conn != NULL) {
			found = &bip_apps[i];
			break;
		}

		if (connection_less == NULL) {
			connection_less = &bip_apps[i];
		}
	}

	k_mutex_unlock(&bip_apps_lock);

	return found != NULL ? found : connection_less;
}

static struct bip_app *find_preregistered_instance_by_address(const bt_addr_t *address)
{
	struct bip_app *found = NULL;

	k_mutex_lock(&bip_apps_lock, K_FOREVER);

	for (uint8_t i = 0; i < BIP_MAX_INSTANCES; i++) {
		if (bip_apps[i].in_use && bip_apps[i].conn == NULL &&
		    bt_addr_eq(&bip_apps[i].address, address)) {
			found = &bip_apps[i];
			break;
		}
	}

	k_mutex_unlock(&bip_apps_lock);

	return found;
}

static struct bip_app *bip_instance_allocate(struct bt_conn *conn)
{
	struct bip_app *inst = NULL;

	k_mutex_lock(&bip_apps_lock, K_FOREVER);

	for (uint8_t i = 0; i < BIP_MAX_INSTANCES; i++) {
		if (!bip_apps[i].in_use) {
			memset(&bip_apps[i], 0, sizeof(struct bip_app));
			bip_apps[i].in_use = true;
			bip_apps[i].conn = conn;
			if (conn != NULL) {
				bt_addr_copy(&bip_apps[i].address, bt_conn_get_dst_br(conn));
			}
			inst = &bip_apps[i];
			break;
		}
	}

	k_mutex_unlock(&bip_apps_lock);

	if (inst == NULL) {
		LOG_ERR("No free BIP instance (%u in use)", BIP_MAX_INSTANCES);
	}

	return inst;
}

static void bip_instance_free(struct bip_app *inst)
{
	k_mutex_lock(&bip_apps_lock, K_FOREVER);

	bt_conn_drop(&inst->conn);
	bt_conn_drop(&inst->second_conn);
	inst->primary_registered = false;
	inst->secondary_registered = false;
	inst->in_use = false;

	k_mutex_unlock(&bip_apps_lock);
}

static bool bip_instance_is_preregistered(const struct bip_app *inst)
{
	return inst->primary_registered || inst->secondary_registered;
}

static void bip_instance_gc(struct bip_app *inst)
{
	k_mutex_lock(&bip_apps_lock, K_FOREVER);

	if (inst->conn == NULL && inst->second_conn == NULL &&
	    !bip_instance_is_preregistered(inst)) {
		bip_instance_free(inst);
	}

	k_mutex_unlock(&bip_apps_lock);
}

static void bip_instance_release_transport(struct bip_app *inst)
{
	k_mutex_lock(&bip_apps_lock, K_FOREVER);

	bt_conn_drop(&inst->conn);
	bip_instance_gc(inst);

	k_mutex_unlock(&bip_apps_lock);
}

static inline struct bip_app *inst_from_bip(struct bt_bip *bip)
{
	return CONTAINER_OF(bip, struct bip_app, bip);
}

static inline struct bip_app *inst_from_server(struct bt_bip_server *server)
{
	return CONTAINER_OF(server, struct bip_app, server);
}

static inline struct bip_app *inst_from_client(struct bt_bip_client *client)
{
	return CONTAINER_OF(client, struct bip_app, client);
}

static inline struct bip_app *inst_from_second_server(struct bt_bip_server *server)
{
	return CONTAINER_OF(server, struct bip_app, second_server);
}

static inline struct bip_app *inst_from_second_client(struct bt_bip_client *client)
{
	return CONTAINER_OF(client, struct bip_app, second_client);
}

static inline struct bip_app *inst_from_second_bip(struct bt_bip *bip)
{
	return CONTAINER_OF(bip, struct bip_app, second_bip);
}

static void bip_inst_get_address(struct bip_app *inst, bt_addr_le_t *addr)
{
	addr->type = BTP_BR_ADDRESS_TYPE;

	if (inst->in_use) {
		bt_addr_copy(&addr->a, &inst->address);
	} else {
		LOG_WRN("BIP event on a released instance, reporting a zero address");
		bt_addr_copy(&addr->a, BT_ADDR_ANY);
	}
}

#define BIP_SDP_DISCOVER_BUF_LEN 512
NET_BUF_POOL_FIXED_DEFINE(bip_sdp_pool, CONFIG_BT_MAX_CONN, BIP_SDP_DISCOVER_BUF_LEN,
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

struct bip_sdp_discover {
	struct bt_sdp_discover_params params;
	struct bt_uuid_16 uuid;
	bool in_flight;
};

static struct bip_sdp_discover bip_sdp_discovers[CONFIG_BT_MAX_CONN];

static struct bip_sdp_discover *sdp_from_params(const struct bt_sdp_discover_params *params)
{
	return CONTAINER_OF(params, struct bip_sdp_discover, params);
}

static int bip_sdp_get_goep_l2cap_psm(const struct net_buf *buf, uint16_t *psm)
{
	int err;
	struct bt_sdp_attribute attr;
	struct bt_sdp_attr_value value;

	err = bt_sdp_get_attr(buf, BT_SDP_ATTR_GOEP_L2CAP_PSM, &attr);
	if (err != 0) {
		return err;
	}

	err = bt_sdp_attr_read(&attr, NULL, &value);
	if (err != 0) {
		return err;
	}

	if (value.type != BT_SDP_ATTR_VALUE_TYPE_UINT || value.uint.size != sizeof(*psm)) {
		return -EINVAL;
	}

	*psm = value.uint.u16;
	return 0;
}

static int bip_sdp_get_functions(const struct net_buf *buf, uint32_t *funcs)
{
	int err;
	struct bt_sdp_attribute attr;
	struct bt_sdp_attr_value value;

	err = bt_sdp_get_attr(buf, BT_SDP_ATTR_SUPPORTED_FUNCTIONS, &attr);
	if (err != 0) {
		return err;
	}

	err = bt_sdp_attr_read(&attr, NULL, &value);
	if (err != 0) {
		return err;
	}

	if (value.type != BT_SDP_ATTR_VALUE_TYPE_UINT || value.uint.size != sizeof(*funcs)) {
		return -EINVAL;
	}

	*funcs = value.uint.u32;
	return 0;
}

static int bip_sdp_get_caps(const struct net_buf *buf, uint8_t *caps)
{
	int err;
	struct bt_sdp_attribute attr;
	struct bt_sdp_attr_value value;

	err = bt_sdp_get_attr(buf, BT_SDP_ATTR_SUPPORTED_CAPABILITIES, &attr);
	if (err != 0) {
		return err;
	}

	err = bt_sdp_attr_read(&attr, NULL, &value);
	if (err != 0) {
		return err;
	}

	if (value.type != BT_SDP_ATTR_VALUE_TYPE_UINT || value.uint.size != sizeof(*caps)) {
		return -EINVAL;
	}

	*caps = value.uint.u8;
	return 0;
}

static uint8_t bip_discover_func(struct bt_conn *conn, struct bt_sdp_client_result *result,
				 const struct bt_sdp_discover_params *params)
{
	struct btp_bip_sdp_discovered_ev ev;
	struct bt_conn_info info;
	uint16_t rfcomm_channel = 0;
	uint16_t l2cap_psm = 0;
	uint16_t features = 0;
	uint32_t functions = 0;
	uint8_t caps = 0;
	bool is_responder;
	int rfcomm_err;
	int l2cap_err;
	int err;

	if (result == NULL || result->resp_buf == NULL || conn == NULL || params == NULL) {
		if (params != NULL) {
			sdp_from_params(params)->in_flight = false;
		}

		return BT_SDP_DISCOVER_UUID_STOP;
	}

	sdp_from_params(params)->in_flight = false;

	is_responder = bt_uuid_cmp(params->uuid,
				   BT_UUID_DECLARE_16(BT_SDP_IMAGING_RESPONDER_SVCLASS)) == 0;

	rfcomm_err = bt_sdp_get_proto_param(result->resp_buf, BT_SDP_PROTO_RFCOMM,
					    &rfcomm_channel);
	l2cap_err = bip_sdp_get_goep_l2cap_psm(result->resp_buf, &l2cap_psm);
	if (rfcomm_err != 0 && l2cap_err != 0) {
		LOG_DBG("No RFCOMM channel or L2CAP PSM attribute");
	}

	if (bip_sdp_get_functions(result->resp_buf, &functions) != 0) {
		LOG_DBG("No supported functions attribute");
	}

	if (is_responder) {
		if (bip_sdp_get_caps(result->resp_buf, &caps) != 0) {
			LOG_DBG("No supported capabilities attribute");
		}
		if (bt_sdp_get_features(result->resp_buf, &features) != 0) {
			LOG_DBG("No supported features attribute");
		}
	}

	err = bt_conn_get_info(conn, &info);
	if (err != 0) {
		return BT_SDP_DISCOVER_UUID_STOP;
	}

	memset(&ev, 0, sizeof(ev));
	bt_addr_copy(&ev.address.a, info.br.dst);
	ev.address.type = BTP_BR_ADDRESS_TYPE;
	ev.channel = (uint8_t)rfcomm_channel;
	ev.psm = sys_cpu_to_le16(l2cap_psm);
	ev.caps = caps;
	ev.features = sys_cpu_to_le16(features);
	ev.functions = sys_cpu_to_le32(functions);

	tester_event(BTP_SERVICE_ID_BIP, BTP_BIP_EV_SDP_DISCOVERED, &ev, sizeof(ev));

	return BT_SDP_DISCOVER_UUID_STOP;
}

static void bip_rfcomm_transport_connected(struct bt_conn *conn, struct bt_bip *bip)
{
	struct btp_bip_rfcomm_connected_ev ev;
	struct bt_conn_info info;
	int err;

	err = bt_conn_get_info(conn, &info);
	if (err != 0) {
		return;
	}

	bt_addr_copy(&ev.address.a, info.br.dst);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	tester_event(BTP_SERVICE_ID_BIP, BTP_BIP_EV_RFCOMM_CONNECTED, &ev, sizeof(ev));
}

static void bip_rfcomm_transport_disconnected(struct bt_bip *bip)
{
	struct bip_app *inst = inst_from_bip(bip);
	struct btp_bip_rfcomm_disconnected_ev ev;

	bip_inst_get_address(inst, &ev.address);

	tester_event(BTP_SERVICE_ID_BIP, BTP_BIP_EV_RFCOMM_DISCONNECTED, &ev, sizeof(ev));

	bip_instance_release_transport(inst);
}

static void bip_l2cap_transport_connected(struct bt_conn *conn, struct bt_bip *bip)
{
	struct btp_bip_l2cap_connected_ev ev;
	struct bt_conn_info info;
	int err;

	err = bt_conn_get_info(conn, &info);
	if (err != 0) {
		return;
	}

	bt_addr_copy(&ev.address.a, info.br.dst);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	tester_event(BTP_SERVICE_ID_BIP, BTP_BIP_EV_L2CAP_CONNECTED, &ev, sizeof(ev));
}

static void bip_l2cap_transport_disconnected(struct bt_bip *bip)
{
	struct bip_app *inst = inst_from_bip(bip);
	struct btp_bip_l2cap_disconnected_ev ev;

	bip_inst_get_address(inst, &ev.address);

	tester_event(BTP_SERVICE_ID_BIP, BTP_BIP_EV_L2CAP_DISCONNECTED, &ev, sizeof(ev));

	bip_instance_release_transport(inst);
}

static struct bt_bip_transport_ops bip_rfcomm_transport_ops = {
	.connected = bip_rfcomm_transport_connected,
	.disconnected = bip_rfcomm_transport_disconnected,
};

static struct bt_bip_transport_ops bip_l2cap_transport_ops = {
	.connected = bip_l2cap_transport_connected,
	.disconnected = bip_l2cap_transport_disconnected,
};

struct bip_param_ev {
	bt_addr_le_t address;
	uint8_t param;
	uint16_t data_len;
	uint8_t data[];
} __packed;

struct bip_plain_ev {
	bt_addr_le_t address;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BIP_ASSERT_PARAM_EV(_st, _param)							\
	BUILD_ASSERT(sizeof(struct _st) == sizeof(struct bip_param_ev) &&			\
		     offsetof(struct _st, address) ==						\
			     offsetof(struct bip_param_ev, address) &&				\
		     offsetof(struct _st, _param) == offsetof(struct bip_param_ev, param) &&	\
		     offsetof(struct _st, data_len) ==						\
			     offsetof(struct bip_param_ev, data_len) &&				\
		     offsetof(struct _st, data) == offsetof(struct bip_param_ev, data),		\
		     #_st " is not byte-compatible with bip_param_ev")

#define BIP_ASSERT_PLAIN_EV(_st)								\
	BUILD_ASSERT(sizeof(struct _st) == sizeof(struct bip_plain_ev) &&			\
		     offsetof(struct _st, address) ==						\
			     offsetof(struct bip_plain_ev, address) &&				\
		     offsetof(struct _st, data_len) ==						\
			     offsetof(struct bip_plain_ev, data_len) &&				\
		     offsetof(struct _st, data) == offsetof(struct bip_plain_ev, data),		\
		     #_st " is not byte-compatible with bip_plain_ev")

BIP_ASSERT_PARAM_EV(btp_bip_server_get_caps_req_ev, final);
BIP_ASSERT_PARAM_EV(btp_bip_server_get_image_list_req_ev, final);
BIP_ASSERT_PARAM_EV(btp_bip_server_get_image_properties_req_ev, final);
BIP_ASSERT_PARAM_EV(btp_bip_server_get_image_req_ev, final);
BIP_ASSERT_PARAM_EV(btp_bip_server_get_linked_thumbnail_req_ev, final);
BIP_ASSERT_PARAM_EV(btp_bip_server_get_linked_attachment_req_ev, final);
BIP_ASSERT_PARAM_EV(btp_bip_server_get_partial_image_req_ev, final);
BIP_ASSERT_PARAM_EV(btp_bip_server_get_monitoring_image_req_ev, final);
BIP_ASSERT_PARAM_EV(btp_bip_server_get_status_req_ev, final);
BIP_ASSERT_PARAM_EV(btp_bip_server_put_image_req_ev, final);
BIP_ASSERT_PARAM_EV(btp_bip_server_put_linked_thumbnail_req_ev, final);
BIP_ASSERT_PARAM_EV(btp_bip_server_put_linked_attachment_req_ev, final);
BIP_ASSERT_PARAM_EV(btp_bip_server_remote_display_req_ev, final);
BIP_ASSERT_PARAM_EV(btp_bip_server_delete_image_req_ev, final);
BIP_ASSERT_PARAM_EV(btp_bip_server_start_print_req_ev, final);
BIP_ASSERT_PARAM_EV(btp_bip_server_start_archive_req_ev, final);
BIP_ASSERT_PARAM_EV(btp_bip_second_server_get_caps_req_ev, final);
BIP_ASSERT_PARAM_EV(btp_bip_second_server_get_image_list_req_ev, final);
BIP_ASSERT_PARAM_EV(btp_bip_second_server_get_image_properties_req_ev, final);
BIP_ASSERT_PARAM_EV(btp_bip_second_server_get_image_req_ev, final);
BIP_ASSERT_PARAM_EV(btp_bip_second_server_get_linked_thumbnail_req_ev, final);
BIP_ASSERT_PARAM_EV(btp_bip_second_server_get_linked_attachment_req_ev, final);
BIP_ASSERT_PARAM_EV(btp_bip_second_server_delete_image_req_ev, final);

BIP_ASSERT_PARAM_EV(btp_bip_client_disconnected_ev, rsp_code);
BIP_ASSERT_PARAM_EV(btp_bip_client_aborted_ev, rsp_code);
BIP_ASSERT_PARAM_EV(btp_bip_client_get_caps_rsp_ev, rsp_code);
BIP_ASSERT_PARAM_EV(btp_bip_client_get_image_list_rsp_ev, rsp_code);
BIP_ASSERT_PARAM_EV(btp_bip_client_get_image_properties_rsp_ev, rsp_code);
BIP_ASSERT_PARAM_EV(btp_bip_client_get_image_rsp_ev, rsp_code);
BIP_ASSERT_PARAM_EV(btp_bip_client_get_linked_thumbnail_rsp_ev, rsp_code);
BIP_ASSERT_PARAM_EV(btp_bip_client_get_linked_attachment_rsp_ev, rsp_code);
BIP_ASSERT_PARAM_EV(btp_bip_client_get_partial_image_rsp_ev, rsp_code);
BIP_ASSERT_PARAM_EV(btp_bip_client_get_monitoring_image_rsp_ev, rsp_code);
BIP_ASSERT_PARAM_EV(btp_bip_client_get_status_rsp_ev, rsp_code);
BIP_ASSERT_PARAM_EV(btp_bip_client_put_image_rsp_ev, rsp_code);
BIP_ASSERT_PARAM_EV(btp_bip_client_put_linked_thumbnail_rsp_ev, rsp_code);
BIP_ASSERT_PARAM_EV(btp_bip_client_put_linked_attachment_rsp_ev, rsp_code);
BIP_ASSERT_PARAM_EV(btp_bip_client_remote_display_rsp_ev, rsp_code);
BIP_ASSERT_PARAM_EV(btp_bip_client_delete_image_rsp_ev, rsp_code);
BIP_ASSERT_PARAM_EV(btp_bip_client_start_print_rsp_ev, rsp_code);
BIP_ASSERT_PARAM_EV(btp_bip_client_start_archive_rsp_ev, rsp_code);
BIP_ASSERT_PARAM_EV(btp_bip_second_client_disconnected_ev, rsp_code);
BIP_ASSERT_PARAM_EV(btp_bip_second_client_aborted_ev, rsp_code);
BIP_ASSERT_PARAM_EV(btp_bip_second_client_get_caps_rsp_ev, rsp_code);
BIP_ASSERT_PARAM_EV(btp_bip_second_client_get_image_list_rsp_ev, rsp_code);
BIP_ASSERT_PARAM_EV(btp_bip_second_client_get_image_properties_rsp_ev, rsp_code);
BIP_ASSERT_PARAM_EV(btp_bip_second_client_get_image_rsp_ev, rsp_code);
BIP_ASSERT_PARAM_EV(btp_bip_second_client_get_linked_thumbnail_rsp_ev, rsp_code);
BIP_ASSERT_PARAM_EV(btp_bip_second_client_get_linked_attachment_rsp_ev, rsp_code);
BIP_ASSERT_PARAM_EV(btp_bip_second_client_get_partial_image_rsp_ev, rsp_code);
BIP_ASSERT_PARAM_EV(btp_bip_second_client_delete_image_rsp_ev, rsp_code);

BIP_ASSERT_PLAIN_EV(btp_bip_server_disconnect_req_ev);
BIP_ASSERT_PLAIN_EV(btp_bip_server_abort_req_ev);
BIP_ASSERT_PLAIN_EV(btp_bip_second_server_disconnect_req_ev);
BIP_ASSERT_PLAIN_EV(btp_bip_second_server_abort_req_ev);

static bool bip_ev_len_ok(uint8_t ev_opcode, size_t ev_len)
{
	if (ev_len > BTP_DATA_MAX_SIZE) {
		LOG_ERR("BIP event 0x%02x dropped: %zu bytes exceed BTP limit %zu", ev_opcode,
			ev_len, (size_t)BTP_DATA_MAX_SIZE);
		return false;
	}

	return true;
}

static void send_server_event(struct bip_app *inst, uint8_t ev_opcode, uint8_t final,
			      struct net_buf *buf)
{
	uint8_t *ev_data;
	uint16_t data_len = (buf != NULL) ? buf->len : 0;
	size_t ev_len = sizeof(struct bip_param_ev) + data_len;

	if (!bip_ev_len_ok(ev_opcode, ev_len)) {
		return;
	}

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, &ev_data);

	struct bip_param_ev *ev = (void *)ev_data;

	bip_inst_get_address(inst, &ev->address);
	ev->param = final;
	ev->data_len = sys_cpu_to_le16(data_len);
	if (data_len > 0) {
		memcpy(ev->data, buf->data, data_len);
	}

	tester_event(BTP_SERVICE_ID_BIP, ev_opcode, ev, ev_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void send_server_event_nofinal(struct bip_app *inst, uint8_t ev_opcode, struct net_buf *buf)
{
	uint8_t *ev_data;
	uint16_t data_len = (buf != NULL) ? buf->len : 0;
	size_t ev_len = sizeof(struct bip_plain_ev) + data_len;

	if (!bip_ev_len_ok(ev_opcode, ev_len)) {
		return;
	}

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, &ev_data);

	struct bip_plain_ev *ev = (void *)ev_data;

	bip_inst_get_address(inst, &ev->address);
	ev->data_len = sys_cpu_to_le16(data_len);
	if (data_len > 0) {
		memcpy(ev->data, buf->data, data_len);
	}

	tester_event(BTP_SERVICE_ID_BIP, ev_opcode, ev, ev_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void send_client_event(struct bip_app *inst, uint8_t ev_opcode, uint8_t rsp_code,
			      struct net_buf *buf)
{
	uint8_t *ev_data;
	uint16_t data_len = (buf != NULL) ? buf->len : 0;
	size_t ev_len = sizeof(struct bip_param_ev) + data_len;

	if (!bip_ev_len_ok(ev_opcode, ev_len)) {
		return;
	}

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, &ev_data);

	struct bip_param_ev *ev = (void *)ev_data;

	bip_inst_get_address(inst, &ev->address);
	ev->param = rsp_code;
	ev->data_len = sys_cpu_to_le16(data_len);
	if (data_len > 0) {
		memcpy(ev->data, buf->data, data_len);
	}

	tester_event(BTP_SERVICE_ID_BIP, ev_opcode, ev, ev_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void bip_server_connect(struct bt_bip_server *server, uint8_t version, uint16_t mopl,
			       struct net_buf *buf)
{
	struct bip_app *inst = inst_from_server(server);
	uint8_t *ev_data;
	size_t ev_len;
	uint16_t data_len = (buf != NULL) ? buf->len : 0;

	ev_len = sizeof(struct btp_bip_server_connect_req_ev) + data_len;

	if (!bip_ev_len_ok(BTP_BIP_EV_SERVER_CONNECT_REQ, ev_len)) {
		return;
	}

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, &ev_data);

	struct btp_bip_server_connect_req_ev *ev = (void *)ev_data;

	bip_inst_get_address(inst, &ev->address);
	ev->version = version;
	ev->mopl = sys_cpu_to_le16(mopl);
	ev->data_len = sys_cpu_to_le16(data_len);
	if (data_len > 0) {
		memcpy(ev->data, buf->data, data_len);
	}

	tester_event(BTP_SERVICE_ID_BIP, BTP_BIP_EV_SERVER_CONNECT_REQ, ev, ev_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void bip_server_disconnect(struct bt_bip_server *server, struct net_buf *buf)
{
	send_server_event_nofinal(inst_from_server(server), BTP_BIP_EV_SERVER_DISCONNECT_REQ, buf);
}

static void bip_server_abort(struct bt_bip_server *server, struct net_buf *buf)
{
	send_server_event_nofinal(inst_from_server(server), BTP_BIP_EV_SERVER_ABORT_REQ, buf);
}

static void bip_server_get_caps(struct bt_bip_server *server, bool final, struct net_buf *buf)
{
	send_server_event(inst_from_server(server), BTP_BIP_EV_SERVER_GET_CAPS_REQ, final, buf);
}

static void bip_server_get_image_list(struct bt_bip_server *server, bool final, struct net_buf *buf)
{
	send_server_event(inst_from_server(server), BTP_BIP_EV_SERVER_GET_IMAGE_LIST_REQ, final,
			  buf);
}

static void bip_server_get_image_properties(struct bt_bip_server *server, bool final,
					    struct net_buf *buf)
{
	send_server_event(inst_from_server(server), BTP_BIP_EV_SERVER_GET_IMAGE_PROPERTIES_REQ,
			  final, buf);
}

static void bip_server_get_image(struct bt_bip_server *server, bool final, struct net_buf *buf)
{
	send_server_event(inst_from_server(server), BTP_BIP_EV_SERVER_GET_IMAGE_REQ, final, buf);
}

static void bip_server_get_linked_thumbnail(struct bt_bip_server *server, bool final,
					    struct net_buf *buf)
{
	send_server_event(inst_from_server(server), BTP_BIP_EV_SERVER_GET_LINKED_THUMBNAIL_REQ,
			  final, buf);
}

static void bip_server_get_linked_attachment(struct bt_bip_server *server, bool final,
					     struct net_buf *buf)
{
	send_server_event(inst_from_server(server), BTP_BIP_EV_SERVER_GET_LINKED_ATTACHMENT_REQ,
			  final, buf);
}

static void bip_server_get_monitoring_image(struct bt_bip_server *server, bool final,
					    struct net_buf *buf)
{
	send_server_event(inst_from_server(server), BTP_BIP_EV_SERVER_GET_MONITORING_IMAGE_REQ,
			  final, buf);
}

static void bip_server_get_status(struct bt_bip_server *server, bool final, struct net_buf *buf)
{
	send_server_event(inst_from_server(server), BTP_BIP_EV_SERVER_GET_STATUS_REQ, final, buf);
}

static void bip_server_put_image(struct bt_bip_server *server, bool final, struct net_buf *buf)
{
	send_server_event(inst_from_server(server), BTP_BIP_EV_SERVER_PUT_IMAGE_REQ, final, buf);
}

static void bip_server_put_linked_thumbnail(struct bt_bip_server *server, bool final,
					    struct net_buf *buf)
{
	send_server_event(inst_from_server(server), BTP_BIP_EV_SERVER_PUT_LINKED_THUMBNAIL_REQ,
			  final, buf);
}

static void bip_server_put_linked_attachment(struct bt_bip_server *server, bool final,
					     struct net_buf *buf)
{
	send_server_event(inst_from_server(server), BTP_BIP_EV_SERVER_PUT_LINKED_ATTACHMENT_REQ,
			  final, buf);
}

static void bip_server_remote_display(struct bt_bip_server *server, bool final, struct net_buf *buf)
{
	send_server_event(inst_from_server(server), BTP_BIP_EV_SERVER_REMOTE_DISPLAY_REQ, final,
			  buf);
}

static void bip_server_delete_image(struct bt_bip_server *server, bool final, struct net_buf *buf)
{
	send_server_event(inst_from_server(server), BTP_BIP_EV_SERVER_DELETE_IMAGE_REQ, final, buf);
}

static void bip_server_start_print(struct bt_bip_server *server, bool final, struct net_buf *buf)
{
	send_server_event(inst_from_server(server), BTP_BIP_EV_SERVER_START_PRINT_REQ, final, buf);
}

static void bip_server_start_archive(struct bt_bip_server *server, bool final, struct net_buf *buf)
{
	send_server_event(inst_from_server(server), BTP_BIP_EV_SERVER_START_ARCHIVE_REQ, final,
			  buf);
}

static struct bt_bip_server_cb bip_server_cb = {
	.connect = bip_server_connect,
	.disconnect = bip_server_disconnect,
	.abort = bip_server_abort,
	.get_caps = bip_server_get_caps,
	.get_image_list = bip_server_get_image_list,
	.get_image_properties = bip_server_get_image_properties,
	.get_image = bip_server_get_image,
	.get_linked_thumbnail = bip_server_get_linked_thumbnail,
	.get_linked_attachment = bip_server_get_linked_attachment,
	.get_monitoring_image = bip_server_get_monitoring_image,
	.get_status = bip_server_get_status,
	.put_image = bip_server_put_image,
	.put_linked_thumbnail = bip_server_put_linked_thumbnail,
	.put_linked_attachment = bip_server_put_linked_attachment,
	.remote_display = bip_server_remote_display,
	.delete_image = bip_server_delete_image,
	.start_print = bip_server_start_print,
	.start_archive = bip_server_start_archive,
};

static void bip_client_connect(struct bt_bip_client *client, uint8_t rsp_code, uint8_t version,
			       uint16_t mopl, struct net_buf *buf)
{
	struct bip_app *inst = inst_from_client(client);
	uint8_t *ev_data;
	size_t ev_len;
	uint16_t data_len = (buf != NULL) ? buf->len : 0;

	if (buf != NULL) {
		bt_obex_get_header_conn_id(buf, &inst->conn_id);
	}

	ev_len = sizeof(struct btp_bip_client_connected_ev) + data_len;

	if (!bip_ev_len_ok(BTP_BIP_EV_CLIENT_CONNECTED, ev_len)) {
		return;
	}

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, &ev_data);

	struct btp_bip_client_connected_ev *ev = (void *)ev_data;

	bip_inst_get_address(inst, &ev->address);
	ev->rsp_code = rsp_code;
	ev->version = version;
	ev->mopl = sys_cpu_to_le16(mopl);
	ev->conn_id = sys_cpu_to_le32(inst->conn_id);
	ev->data_len = sys_cpu_to_le16(data_len);
	if (data_len > 0) {
		memcpy(ev->data, buf->data, data_len);
	}

	tester_event(BTP_SERVICE_ID_BIP, BTP_BIP_EV_CLIENT_CONNECTED, ev, ev_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void bip_client_disconnect(struct bt_bip_client *client, uint8_t rsp_code,
				  struct net_buf *buf)
{
	send_client_event(inst_from_client(client), BTP_BIP_EV_CLIENT_DISCONNECTED, rsp_code, buf);
}

static void bip_client_abort(struct bt_bip_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	send_client_event(inst_from_client(client), BTP_BIP_EV_CLIENT_ABORTED, rsp_code, buf);
}

static void bip_client_get_caps(struct bt_bip_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	send_client_event(inst_from_client(client), BTP_BIP_EV_CLIENT_GET_CAPS_RSP, rsp_code, buf);
}

static void bip_client_get_image_list(struct bt_bip_client *client, uint8_t rsp_code,
				      struct net_buf *buf)
{
	send_client_event(inst_from_client(client), BTP_BIP_EV_CLIENT_GET_IMAGE_LIST_RSP, rsp_code,
			  buf);
}

static void bip_client_get_image_properties(struct bt_bip_client *client, uint8_t rsp_code,
					    struct net_buf *buf)
{
	send_client_event(inst_from_client(client), BTP_BIP_EV_CLIENT_GET_IMAGE_PROPERTIES_RSP,
			  rsp_code, buf);
}

static void bip_client_get_image(struct bt_bip_client *client, uint8_t rsp_code,
				 struct net_buf *buf)
{
	send_client_event(inst_from_client(client), BTP_BIP_EV_CLIENT_GET_IMAGE_RSP, rsp_code, buf);
}

static void bip_client_get_linked_thumbnail(struct bt_bip_client *client, uint8_t rsp_code,
					    struct net_buf *buf)
{
	send_client_event(inst_from_client(client), BTP_BIP_EV_CLIENT_GET_LINKED_THUMBNAIL_RSP,
			  rsp_code, buf);
}

static void bip_client_get_linked_attachment(struct bt_bip_client *client, uint8_t rsp_code,
					     struct net_buf *buf)
{
	send_client_event(inst_from_client(client), BTP_BIP_EV_CLIENT_GET_LINKED_ATTACHMENT_RSP,
			  rsp_code, buf);
}

static void bip_client_get_partial_image(struct bt_bip_client *client, uint8_t rsp_code,
					 struct net_buf *buf)
{
	send_client_event(inst_from_client(client), BTP_BIP_EV_CLIENT_GET_PARTIAL_IMAGE_RSP,
			  rsp_code, buf);
}

static void bip_client_get_monitoring_image(struct bt_bip_client *client, uint8_t rsp_code,
					    struct net_buf *buf)
{
	send_client_event(inst_from_client(client), BTP_BIP_EV_CLIENT_GET_MONITORING_IMAGE_RSP,
			  rsp_code, buf);
}

static void bip_client_get_status(struct bt_bip_client *client, uint8_t rsp_code,
				  struct net_buf *buf)
{
	send_client_event(inst_from_client(client), BTP_BIP_EV_CLIENT_GET_STATUS_RSP, rsp_code,
			  buf);
}

static void bip_client_put_image(struct bt_bip_client *client, uint8_t rsp_code,
				 struct net_buf *buf)
{
	send_client_event(inst_from_client(client), BTP_BIP_EV_CLIENT_PUT_IMAGE_RSP, rsp_code, buf);
}

static void bip_client_put_linked_thumbnail(struct bt_bip_client *client, uint8_t rsp_code,
					    struct net_buf *buf)
{
	send_client_event(inst_from_client(client), BTP_BIP_EV_CLIENT_PUT_LINKED_THUMBNAIL_RSP,
			  rsp_code, buf);
}

static void bip_client_put_linked_attachment(struct bt_bip_client *client, uint8_t rsp_code,
					     struct net_buf *buf)
{
	send_client_event(inst_from_client(client), BTP_BIP_EV_CLIENT_PUT_LINKED_ATTACHMENT_RSP,
			  rsp_code, buf);
}

static void bip_client_remote_display(struct bt_bip_client *client, uint8_t rsp_code,
				      struct net_buf *buf)
{
	send_client_event(inst_from_client(client), BTP_BIP_EV_CLIENT_REMOTE_DISPLAY_RSP, rsp_code,
			  buf);
}

static void bip_client_delete_image(struct bt_bip_client *client, uint8_t rsp_code,
				    struct net_buf *buf)
{
	send_client_event(inst_from_client(client), BTP_BIP_EV_CLIENT_DELETE_IMAGE_RSP, rsp_code,
			  buf);
}

static void bip_client_start_print(struct bt_bip_client *client, uint8_t rsp_code,
				   struct net_buf *buf)
{
	send_client_event(inst_from_client(client), BTP_BIP_EV_CLIENT_START_PRINT_RSP, rsp_code,
			  buf);
}

static void bip_client_start_archive(struct bt_bip_client *client, uint8_t rsp_code,
				     struct net_buf *buf)
{
	send_client_event(inst_from_client(client), BTP_BIP_EV_CLIENT_START_ARCHIVE_RSP, rsp_code,
			  buf);
}

static struct bt_bip_client_cb bip_client_cb = {
	.connect = bip_client_connect,
	.disconnect = bip_client_disconnect,
	.abort = bip_client_abort,
	.get_caps = bip_client_get_caps,
	.get_image_list = bip_client_get_image_list,
	.get_image_properties = bip_client_get_image_properties,
	.get_image = bip_client_get_image,
	.get_linked_thumbnail = bip_client_get_linked_thumbnail,
	.get_linked_attachment = bip_client_get_linked_attachment,
	.get_partial_image = bip_client_get_partial_image,
	.get_monitoring_image = bip_client_get_monitoring_image,
	.get_status = bip_client_get_status,
	.put_image = bip_client_put_image,
	.put_linked_thumbnail = bip_client_put_linked_thumbnail,
	.put_linked_attachment = bip_client_put_linked_attachment,
	.remote_display = bip_client_remote_display,
	.delete_image = bip_client_delete_image,
	.start_print = bip_client_start_print,
	.start_archive = bip_client_start_archive,
};

static void bip_second_server_connect(struct bt_bip_server *server, uint8_t version, uint16_t mopl,
				      struct net_buf *buf)
{
	struct bip_app *inst = inst_from_second_server(server);
	uint8_t *ev_data;
	size_t ev_len;
	uint16_t data_len = (buf != NULL) ? buf->len : 0;

	ev_len = sizeof(struct btp_bip_second_server_connect_req_ev) + data_len;

	if (!bip_ev_len_ok(BTP_BIP_EV_SECOND_SERVER_CONNECT_REQ, ev_len)) {
		return;
	}

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, &ev_data);

	struct btp_bip_second_server_connect_req_ev *ev = (void *)ev_data;

	bip_inst_get_address(inst, &ev->address);
	ev->version = version;
	ev->mopl = sys_cpu_to_le16(mopl);
	ev->data_len = sys_cpu_to_le16(data_len);
	if (data_len > 0) {
		memcpy(ev->data, buf->data, data_len);
	}

	tester_event(BTP_SERVICE_ID_BIP, BTP_BIP_EV_SECOND_SERVER_CONNECT_REQ, ev, ev_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void bip_second_server_disconnect(struct bt_bip_server *server, struct net_buf *buf)
{
	send_server_event_nofinal(inst_from_second_server(server),
				  BTP_BIP_EV_SECOND_SERVER_DISCONNECT_REQ, buf);
}

static void bip_second_server_abort(struct bt_bip_server *server, struct net_buf *buf)
{
	send_server_event_nofinal(inst_from_second_server(server),
				  BTP_BIP_EV_SECOND_SERVER_ABORT_REQ, buf);
}

static void bip_second_server_get_partial_image(struct bt_bip_server *server, bool final,
						struct net_buf *buf)
{
	send_server_event(inst_from_second_server(server), BTP_BIP_EV_SERVER_GET_PARTIAL_IMAGE_REQ,
			  final, buf);
}

static void bip_second_server_get_caps(struct bt_bip_server *server, bool final,
				       struct net_buf *buf)
{
	send_server_event(inst_from_second_server(server), BTP_BIP_EV_SECOND_SERVER_GET_CAPS_REQ,
			  final, buf);
}

static void bip_second_server_get_image_list(struct bt_bip_server *server, bool final,
					     struct net_buf *buf)
{
	send_server_event(inst_from_second_server(server),
			  BTP_BIP_EV_SECOND_SERVER_GET_IMAGE_LIST_REQ, final, buf);
}

static void bip_second_server_get_image_properties(struct bt_bip_server *server, bool final,
						   struct net_buf *buf)
{
	send_server_event(inst_from_second_server(server),
			  BTP_BIP_EV_SECOND_SERVER_GET_IMAGE_PROPERTIES_REQ, final, buf);
}

static void bip_second_server_get_image(struct bt_bip_server *server, bool final,
					struct net_buf *buf)
{
	send_server_event(inst_from_second_server(server), BTP_BIP_EV_SECOND_SERVER_GET_IMAGE_REQ,
			  final, buf);
}

static void bip_second_server_get_linked_thumbnail(struct bt_bip_server *server, bool final,
						   struct net_buf *buf)
{
	send_server_event(inst_from_second_server(server),
			  BTP_BIP_EV_SECOND_SERVER_GET_LINKED_THUMBNAIL_REQ, final, buf);
}

static void bip_second_server_get_linked_attachment(struct bt_bip_server *server, bool final,
						    struct net_buf *buf)
{
	send_server_event(inst_from_second_server(server),
			  BTP_BIP_EV_SECOND_SERVER_GET_LINKED_ATTACHMENT_REQ, final, buf);
}

static void bip_second_server_delete_image(struct bt_bip_server *server, bool final,
					   struct net_buf *buf)
{
	send_server_event(inst_from_second_server(server),
			  BTP_BIP_EV_SECOND_SERVER_DELETE_IMAGE_REQ, final, buf);
}

static struct bt_bip_server_cb bip_second_server_cb = {
	.connect = bip_second_server_connect,
	.disconnect = bip_second_server_disconnect,
	.abort = bip_second_server_abort,
	.get_partial_image = bip_second_server_get_partial_image,
	.get_caps = bip_second_server_get_caps,
	.get_image_list = bip_second_server_get_image_list,
	.get_image_properties = bip_second_server_get_image_properties,
	.get_image = bip_second_server_get_image,
	.get_linked_thumbnail = bip_second_server_get_linked_thumbnail,
	.get_linked_attachment = bip_second_server_get_linked_attachment,
	.delete_image = bip_second_server_delete_image,
};

static void bip_second_client_connect(struct bt_bip_client *client, uint8_t rsp_code,
				      uint8_t version, uint16_t mopl, struct net_buf *buf)
{
	struct bip_app *inst = inst_from_second_client(client);
	uint8_t *ev_data;
	size_t ev_len;
	uint16_t data_len = (buf != NULL) ? buf->len : 0;

	if (buf != NULL) {
		bt_obex_get_header_conn_id(buf, &inst->second_conn_id);
	}

	ev_len = sizeof(struct btp_bip_second_client_connected_ev) + data_len;

	if (!bip_ev_len_ok(BTP_BIP_EV_SECOND_CLIENT_CONNECTED, ev_len)) {
		return;
	}

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, &ev_data);

	struct btp_bip_second_client_connected_ev *ev = (void *)ev_data;

	bip_inst_get_address(inst, &ev->address);
	ev->rsp_code = rsp_code;
	ev->version = version;
	ev->mopl = sys_cpu_to_le16(mopl);
	ev->conn_id = sys_cpu_to_le32(inst->second_conn_id);
	ev->data_len = sys_cpu_to_le16(data_len);
	if (data_len > 0) {
		memcpy(ev->data, buf->data, data_len);
	}

	tester_event(BTP_SERVICE_ID_BIP, BTP_BIP_EV_SECOND_CLIENT_CONNECTED, ev, ev_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void bip_second_client_disconnect(struct bt_bip_client *client, uint8_t rsp_code,
					 struct net_buf *buf)
{
	send_client_event(inst_from_second_client(client), BTP_BIP_EV_SECOND_CLIENT_DISCONNECTED,
			  rsp_code, buf);
}

static void bip_second_client_abort(struct bt_bip_client *client, uint8_t rsp_code,
				    struct net_buf *buf)
{
	send_client_event(inst_from_second_client(client), BTP_BIP_EV_SECOND_CLIENT_ABORTED,
			  rsp_code, buf);
}

static void bip_second_client_get_caps(struct bt_bip_client *client, uint8_t rsp_code,
				       struct net_buf *buf)
{
	send_client_event(inst_from_second_client(client), BTP_BIP_EV_SECOND_CLIENT_GET_CAPS_RSP,
			  rsp_code, buf);
}

static void bip_second_client_get_image_list(struct bt_bip_client *client, uint8_t rsp_code,
					     struct net_buf *buf)
{
	send_client_event(inst_from_second_client(client),
			  BTP_BIP_EV_SECOND_CLIENT_GET_IMAGE_LIST_RSP, rsp_code, buf);
}

static void bip_second_client_get_image_properties(struct bt_bip_client *client, uint8_t rsp_code,
						   struct net_buf *buf)
{
	send_client_event(inst_from_second_client(client),
			  BTP_BIP_EV_SECOND_CLIENT_GET_IMAGE_PROPERTIES_RSP, rsp_code, buf);
}

static void bip_second_client_get_image(struct bt_bip_client *client, uint8_t rsp_code,
					struct net_buf *buf)
{
	send_client_event(inst_from_second_client(client), BTP_BIP_EV_SECOND_CLIENT_GET_IMAGE_RSP,
			  rsp_code, buf);
}

static void bip_second_client_get_linked_thumbnail(struct bt_bip_client *client, uint8_t rsp_code,
						   struct net_buf *buf)
{
	send_client_event(inst_from_second_client(client),
			  BTP_BIP_EV_SECOND_CLIENT_GET_LINKED_THUMBNAIL_RSP, rsp_code, buf);
}

static void bip_second_client_get_linked_attachment(struct bt_bip_client *client, uint8_t rsp_code,
						    struct net_buf *buf)
{
	send_client_event(inst_from_second_client(client),
			  BTP_BIP_EV_SECOND_CLIENT_GET_LINKED_ATTACHMENT_RSP, rsp_code, buf);
}

static void bip_second_client_get_partial_image(struct bt_bip_client *client, uint8_t rsp_code,
						struct net_buf *buf)
{
	send_client_event(inst_from_second_client(client),
			  BTP_BIP_EV_SECOND_CLIENT_GET_PARTIAL_IMAGE_RSP, rsp_code, buf);
}

static void bip_second_client_delete_image(struct bt_bip_client *client, uint8_t rsp_code,
					   struct net_buf *buf)
{
	send_client_event(inst_from_second_client(client),
			  BTP_BIP_EV_SECOND_CLIENT_DELETE_IMAGE_RSP, rsp_code, buf);
}

static struct bt_bip_client_cb bip_second_client_cb = {
	.connect = bip_second_client_connect,
	.disconnect = bip_second_client_disconnect,
	.abort = bip_second_client_abort,
	.get_caps = bip_second_client_get_caps,
	.get_image_list = bip_second_client_get_image_list,
	.get_image_properties = bip_second_client_get_image_properties,
	.get_image = bip_second_client_get_image,
	.get_linked_thumbnail = bip_second_client_get_linked_thumbnail,
	.get_linked_attachment = bip_second_client_get_linked_attachment,
	.get_partial_image = bip_second_client_get_partial_image,
	.delete_image = bip_second_client_delete_image,
};

static int bip_transport_accept(struct bt_conn *conn, struct bt_bip **bip,
				struct bt_bip_transport_ops *ops)
{
	struct bip_app *inst;

	inst = find_preregistered_instance_by_address(bt_conn_get_dst_br(conn));
	if (inst != NULL) {
		inst->conn = bt_conn_ref(conn);
	} else {
		inst = bip_instance_allocate(bt_conn_ref(conn));
		if (inst == NULL) {
			bt_conn_unref(conn);
			return -ENOMEM;
		}
	}

	inst->bip.ops = ops;
	*bip = &inst->bip;

	return 0;
}

static int rfcomm_accept(struct bt_conn *conn, struct bt_bip_rfcomm_server *server,
			 struct bt_bip **bip)
{
	ARG_UNUSED(server);

	return bip_transport_accept(conn, bip, &bip_rfcomm_transport_ops);
}

static int l2cap_accept(struct bt_conn *conn, struct bt_bip_l2cap_server *server,
			struct bt_bip **bip)
{
	ARG_UNUSED(server);

	return bip_transport_accept(conn, bip, &bip_l2cap_transport_ops);
}

static struct bip_app *find_second_server_instance_by_address(const bt_addr_t *address)
{
	struct bip_app *found = NULL;

	k_mutex_lock(&bip_apps_lock, K_FOREVER);

	for (uint8_t i = 0; i < BIP_MAX_INSTANCES; i++) {
		if (bip_apps[i].in_use && bt_addr_eq(&bip_apps[i].address, address) &&
		    bip_apps[i].secondary_registered) {
			found = &bip_apps[i];
			break;
		}
	}

	k_mutex_unlock(&bip_apps_lock);

	return found;
}

static void bip_second_rfcomm_transport_connected(struct bt_conn *conn, struct bt_bip *bip)
{
	struct btp_bip_second_rfcomm_connected_ev ev;
	struct bt_conn_info info;
	int err;

	err = bt_conn_get_info(conn, &info);
	if (err != 0) {
		return;
	}

	bt_addr_copy(&ev.address.a, info.br.dst);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	tester_event(BTP_SERVICE_ID_BIP, BTP_BIP_EV_SECOND_RFCOMM_CONNECTED, &ev, sizeof(ev));
}

static void bip_second_rfcomm_transport_disconnected(struct bt_bip *bip)
{
	struct bip_app *inst = inst_from_second_bip(bip);
	struct btp_bip_second_rfcomm_disconnected_ev ev;

	bip_inst_get_address(inst, &ev.address);

	tester_event(BTP_SERVICE_ID_BIP, BTP_BIP_EV_SECOND_RFCOMM_DISCONNECTED, &ev, sizeof(ev));

	bt_conn_drop(&inst->second_conn);
	bip_instance_gc(inst);
}

static void bip_second_l2cap_transport_connected(struct bt_conn *conn, struct bt_bip *bip)
{
	struct btp_bip_second_l2cap_connected_ev ev;
	struct bt_conn_info info;
	int err;

	err = bt_conn_get_info(conn, &info);
	if (err != 0) {
		return;
	}

	bt_addr_copy(&ev.address.a, info.br.dst);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	tester_event(BTP_SERVICE_ID_BIP, BTP_BIP_EV_SECOND_L2CAP_CONNECTED, &ev, sizeof(ev));
}

static void bip_second_l2cap_transport_disconnected(struct bt_bip *bip)
{
	struct bip_app *inst = inst_from_second_bip(bip);
	struct btp_bip_second_l2cap_disconnected_ev ev;

	bip_inst_get_address(inst, &ev.address);

	tester_event(BTP_SERVICE_ID_BIP, BTP_BIP_EV_SECOND_L2CAP_DISCONNECTED, &ev, sizeof(ev));

	bt_conn_drop(&inst->second_conn);
	bip_instance_gc(inst);
}

static struct bt_bip_transport_ops bip_second_rfcomm_transport_ops = {
	.connected = bip_second_rfcomm_transport_connected,
	.disconnected = bip_second_rfcomm_transport_disconnected,
};

static struct bt_bip_transport_ops bip_second_l2cap_transport_ops = {
	.connected = bip_second_l2cap_transport_connected,
	.disconnected = bip_second_l2cap_transport_disconnected,
};

static int bip_second_transport_accept(struct bt_conn *conn, struct bt_bip **bip,
				       struct bt_bip_transport_ops *ops)
{
	struct bip_app *inst;

	inst = find_second_server_instance_by_address(bt_conn_get_dst_br(conn));
	if (inst == NULL) {
		return -ENOMEM;
	}

	bt_conn_drop(&inst->second_conn);
	inst->second_conn = bt_conn_ref(conn);
	inst->second_bip.ops = ops;
	*bip = &inst->second_bip;

	return 0;
}

static int refobj_rfcomm_accept(struct bt_conn *conn, struct bt_bip_rfcomm_server *server,
				struct bt_bip **bip)
{
	ARG_UNUSED(server);

	return bip_second_transport_accept(conn, bip, &bip_second_rfcomm_transport_ops);
}

static int refobj_l2cap_accept(struct bt_conn *conn, struct bt_bip_l2cap_server *server,
			       struct bt_bip **bip)
{
	ARG_UNUSED(server);

	return bip_second_transport_accept(conn, bip, &bip_second_l2cap_transport_ops);
}

static int archive_rfcomm_accept(struct bt_conn *conn, struct bt_bip_rfcomm_server *server,
				 struct bt_bip **bip)
{
	ARG_UNUSED(server);

	if (find_second_server_instance_by_address(bt_conn_get_dst_br(conn)) != NULL) {
		return bip_second_transport_accept(conn, bip, &bip_second_rfcomm_transport_ops);
	}

	return bip_transport_accept(conn, bip, &bip_rfcomm_transport_ops);
}

static int archive_l2cap_accept(struct bt_conn *conn, struct bt_bip_l2cap_server *server,
				struct bt_bip **bip)
{
	ARG_UNUSED(server);

	if (find_second_server_instance_by_address(bt_conn_get_dst_br(conn)) != NULL) {
		return bip_second_transport_accept(conn, bip, &bip_second_l2cap_transport_ops);
	}

	return bip_transport_accept(conn, bip, &bip_l2cap_transport_ops);
}

static struct net_buf *alloc_buf_with_data_bip(struct bt_bip *bip, const uint8_t *data,
					       uint16_t data_len)
{
	struct net_buf *buf;

	buf = bt_goep_create_pdu(&bip->goep, &bip_tx_pool);
	if (buf == NULL) {
		LOG_ERR("Failed to create a BIP PDU");
		return NULL;
	}

	if (data_len > 0 && data != NULL) {
		if (net_buf_tailroom(buf) < data_len) {
			LOG_ERR("BIP payload of %u does not fit in %zu bytes of tailroom",
				data_len, net_buf_tailroom(buf));
			net_buf_unref(buf);
			return NULL;
		}
		net_buf_add_mem(buf, data, data_len);
	}

	return buf;
}

static inline bool bip_var_len_valid(uint16_t cmd_len, size_t fixed_size, const void *data_len)
{
	uint16_t len;

	if (cmd_len < fixed_size) {
		LOG_ERR("BIP cmd too short: %u < %zu", cmd_len, fixed_size);
		return false;
	}

	len = sys_get_le16(data_len);

	if (cmd_len != fixed_size + len) {
		LOG_ERR("BIP cmd len mismatch: cmd_len %u, data_len %u", cmd_len, len);
		return false;
	}

	return true;
}

static inline bool bip_addr_valid(const bt_addr_le_t *addr)
{
	if (addr->type != BTP_BR_ADDRESS_TYPE) {
		LOG_ERR("BIP cmd with non-BR/EDR address type %u", addr->type);
		return false;
	}

	return true;
}

static inline bool bip_final_valid(uint8_t final)
{
	if (final > 1) {
		LOG_ERR("BIP cmd with invalid final %u", final);
		return false;
	}

	return true;
}

static uint8_t supported_commands(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	struct btp_bip_read_supported_commands_rp *rp = rsp;

	*rsp_len = tester_supported_commands(BTP_SERVICE_ID_BIP, rp->data);
	*rsp_len += sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t connect_rfcomm(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_connect_rfcomm_cmd *cp = cmd;
	struct bt_conn *conn;
	struct bip_app *inst;
	int err;

	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	conn = bt_conn_lookup_addr_br(&cp->address.a);
	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	inst = find_second_server_instance_by_address(&cp->address.a);
	if (inst != NULL) {
		bt_conn_drop(&inst->second_conn);
		inst->second_conn = bt_conn_ref(conn);
		inst->second_bip.ops = &bip_second_rfcomm_transport_ops;

		err = bt_bip_rfcomm_connect(conn, &inst->second_bip, cp->channel);
		bt_conn_unref(conn);
		if (err != 0) {
			bt_conn_drop(&inst->second_conn);
			return BTP_STATUS_FAILED;
		}

		return BTP_STATUS_SUCCESS;
	}

	if (find_instance_by_address(&cp->address.a) != NULL) {
		bt_conn_unref(conn);
		return BTP_STATUS_FAILED;
	}

	inst = bip_instance_allocate(conn);
	if (inst == NULL) {
		bt_conn_unref(conn);
		return BTP_STATUS_FAILED;
	}

	inst->bip.ops = &bip_rfcomm_transport_ops;

	err = bt_bip_rfcomm_connect(conn, &inst->bip, cp->channel);
	if (err != 0) {
		bip_instance_free(inst);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t disconnect_rfcomm(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_disconnect_rfcomm_cmd *cp = cmd;
	struct bip_app *inst;
	int err;

	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	inst = find_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_rfcomm_disconnect(&inst->bip);
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t connect_l2cap(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_connect_l2cap_cmd *cp = cmd;
	struct bt_conn *conn;
	struct bip_app *inst;
	int err;

	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	conn = bt_conn_lookup_addr_br(&cp->address.a);
	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	inst = find_second_server_instance_by_address(&cp->address.a);
	if (inst != NULL) {
		bt_conn_drop(&inst->second_conn);
		inst->second_conn = bt_conn_ref(conn);
		inst->second_bip.ops = &bip_second_l2cap_transport_ops;
		err = bt_bip_l2cap_connect(conn, &inst->second_bip, sys_le16_to_cpu(cp->psm));
		bt_conn_unref(conn);
		if (err != 0) {
			bt_conn_drop(&inst->second_conn);
			return BTP_STATUS_FAILED;
		}
		return BTP_STATUS_SUCCESS;
	}

	if (find_instance_by_address(&cp->address.a) != NULL) {
		bt_conn_unref(conn);
		return BTP_STATUS_FAILED;
	}

	inst = bip_instance_allocate(conn);
	if (inst == NULL) {
		bt_conn_unref(conn);
		return BTP_STATUS_FAILED;
	}

	inst->bip.ops = &bip_l2cap_transport_ops;

	err = bt_bip_l2cap_connect(conn, &inst->bip, sys_le16_to_cpu(cp->psm));
	if (err != 0) {
		bip_instance_free(inst);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t second_connect_l2cap(const void *cmd, uint16_t cmd_len, void *rsp,
				    uint16_t *rsp_len)
{
	const struct btp_bip_second_connect_l2cap_cmd *cp = cmd;
	struct bt_conn *conn;
	struct bip_app *inst;
	int err;

	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	conn = bt_conn_lookup_addr_br(&cp->address.a);
	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		bt_conn_unref(conn);
		return BTP_STATUS_FAILED;
	}

	bt_conn_drop(&inst->second_conn);
	inst->second_conn = bt_conn_ref(conn);
	inst->second_bip.ops = &bip_second_l2cap_transport_ops;

	err = bt_bip_l2cap_connect(conn, &inst->second_bip, sys_le16_to_cpu(cp->psm));
	bt_conn_unref(conn);
	if (err != 0) {
		bt_conn_drop(&inst->second_conn);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t second_connect_rfcomm(const void *cmd, uint16_t cmd_len, void *rsp,
				     uint16_t *rsp_len)
{
	const struct btp_bip_second_connect_rfcomm_cmd *cp = cmd;
	struct bt_conn *conn;
	struct bip_app *inst;
	int err;

	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	conn = bt_conn_lookup_addr_br(&cp->address.a);
	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		bt_conn_unref(conn);
		return BTP_STATUS_FAILED;
	}

	bt_conn_drop(&inst->second_conn);
	inst->second_conn = bt_conn_ref(conn);
	inst->second_bip.ops = &bip_second_rfcomm_transport_ops;

	err = bt_bip_rfcomm_connect(conn, &inst->second_bip, cp->channel);
	bt_conn_unref(conn);
	if (err != 0) {
		bt_conn_drop(&inst->second_conn);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t second_disconnect_l2cap(const void *cmd, uint16_t cmd_len, void *rsp,
				       uint16_t *rsp_len)
{
	const struct btp_bip_second_disconnect_l2cap_cmd *cp = cmd;
	struct bip_app *inst;
	int err;

	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_l2cap_disconnect(&inst->second_bip);
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t second_disconnect_rfcomm(const void *cmd, uint16_t cmd_len, void *rsp,
					uint16_t *rsp_len)
{
	const struct btp_bip_second_disconnect_rfcomm_cmd *cp = cmd;
	struct bip_app *inst;
	int err;

	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_rfcomm_disconnect(&inst->second_bip);
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t disconnect_l2cap(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_disconnect_l2cap_cmd *cp = cmd;
	struct bip_app *inst;
	int err;

	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	inst = find_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_l2cap_disconnect(&inst->bip);

	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t sdp_discover(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_sdp_discover_cmd *cp = cmd;
	struct bip_sdp_discover *slot;
	struct bt_conn *conn;
	uint8_t index;
	int err;

	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	conn = bt_conn_lookup_addr_br(&cp->address.a);
	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	index = bt_conn_index(conn);
	if (index >= ARRAY_SIZE(bip_sdp_discovers)) {
		LOG_ERR("ACL index %u out of range", index);
		bt_conn_unref(conn);
		return BTP_STATUS_FAILED;
	}

	slot = &bip_sdp_discovers[index];
	if (slot->in_flight) {
		LOG_ERR("SDP discovery already in progress for this peer");
		bt_conn_unref(conn);
		return BTP_STATUS_FAILED;
	}

	slot->uuid.uuid.type = BT_UUID_TYPE_16;
	slot->uuid.val = sys_le16_to_cpu(cp->uuid);
	slot->params.uuid = &slot->uuid.uuid;
	slot->params.func = bip_discover_func;
	slot->params.pool = &bip_sdp_pool;
	slot->params.type = BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR;
	slot->in_flight = true;

	err = bt_sdp_discover(conn, &slot->params);
	bt_conn_unref(conn);

	if (err != 0) {
		slot->in_flight = false;
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static const struct bt_uuid_128 *const bip_uuids[] = {
	[BT_BIP_PRIM_CONN_TYPE_IMAGE_PUSH] = BT_BIP_UUID_IMAGE_PUSH,
	[BT_BIP_PRIM_CONN_TYPE_IMAGE_PULL] = BT_BIP_UUID_IMAGE_PULL,
	[BT_BIP_PRIM_CONN_TYPE_ADVANCED_IMAGE_PRINTING] = BT_BIP_UUID_IMAGE_PRINT,
	[BT_BIP_PRIM_CONN_TYPE_AUTO_ARCHIVE] = BT_BIP_UUID_AUTO_ARCHIVE,
	[BT_BIP_PRIM_CONN_TYPE_REMOTE_CAMERA] = BT_BIP_UUID_REMOTE_CAMERA,
	[BT_BIP_PRIM_CONN_TYPE_REMOTE_DISPLAY] = BT_BIP_UUID_REMOTE_DISPLAY,
};

static uint8_t server_register(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_server_register_cmd *cp = cmd;
	enum bt_bip_conn_type type = cp->conn_type;
	struct bip_app *inst;
	const struct bt_uuid_128 *u;
	struct bt_conn *conn;
	bool allocated = false;
	int err;

	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	if (type >= ARRAY_SIZE(bip_uuids) || bip_uuids[type] == NULL) {
		return BTP_STATUS_FAILED;
	}
	u = bip_uuids[type];

	inst = find_any_instance_by_address(&cp->address.a);

	if (inst != NULL && inst->primary_registered) {
		return BTP_STATUS_SUCCESS;
	}

	if (inst == NULL) {
		conn = bt_conn_lookup_addr_br(&cp->address.a);
		if (conn != NULL) {
			inst = bip_instance_allocate(conn);
			if (inst == NULL) {
				bt_conn_unref(conn);
				return BTP_STATUS_FAILED;
			}
		} else {
			inst = bip_instance_allocate(NULL);
			if (inst == NULL) {
				return BTP_STATUS_FAILED;
			}
			bt_addr_copy(&inst->address, &cp->address.a);
		}
		allocated = true;
	}

	err = bt_bip_primary_server_register(&inst->bip, &inst->server, type, u, &bip_server_cb);
	if (err != 0) {
		if (allocated) {
			bip_instance_free(inst);
		}
		return BTP_STATUS_FAILED;
	}

	inst->primary_registered = true;

	return BTP_STATUS_SUCCESS;
}

static uint8_t server_unregister(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_server_unregister_cmd *cp = cmd;
	struct bip_app *inst;
	int err;

	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_server_unregister(&inst->server);
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	inst->primary_registered = false;
	bip_instance_gc(inst);

	return BTP_STATUS_SUCCESS;
}

static uint8_t client_connect(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_client_connect_cmd *cp = cmd;
	enum bt_bip_conn_type type = cp->conn_type;
	struct bip_app *inst;
	int err;

	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	if (type >= ARRAY_SIZE(bip_uuids) || bip_uuids[type] == NULL) {
		LOG_ERR("BIP client connect with invalid conn_type %u", cp->conn_type);
		return BTP_STATUS_FAILED;
	}

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	bt_bip_set_supported_capabilities(&inst->bip, bip_supported_caps);
	bt_bip_set_supported_features(&inst->bip, bip_supported_features);
	bt_bip_set_supported_functions(&inst->bip, bip_supported_functions);

	err = bt_bip_primary_client_connect(&inst->bip, &inst->client, type, &bip_client_cb, NULL);

	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t obex_disconnect(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_obex_disconnect_cmd *cp = cmd;
	struct bip_app *inst;
	int err;

	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_disconnect(&inst->client, NULL);
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t obex_abort(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_obex_abort_cmd *cp = cmd;
	struct bip_app *inst;
	int err;

	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_abort(&inst->client, NULL);
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t connect_rsp(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_connect_rsp_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf = NULL;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	if (data_len > 0) {
		buf = alloc_buf_with_data_bip(&inst->bip, cp->data, data_len);
		if (buf == NULL) {
			return BTP_STATUS_FAILED;
		}
	}

	err = bt_bip_connect_rsp(&inst->server, cp->rsp_code, buf);
	if (err != 0) {
		if (buf != NULL) {
			net_buf_unref(buf);
		}
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t disconnect_rsp(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_disconnect_rsp_cmd *cp = cmd;
	struct bip_app *inst;
	int err;

	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_disconnect_rsp(&inst->server, cp->rsp_code, NULL);
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t abort_rsp(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_abort_rsp_cmd *cp = cmd;
	struct bip_app *inst;
	int err;

	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_abort_rsp(&inst->server, cp->rsp_code, NULL);
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t get_capabilities(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_get_capabilities_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_final_valid(cp->final)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_get_capabilities(&inst->client, cp->final, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t get_capabilities_rsp(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_get_capabilities_rsp_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_get_capabilities_rsp(&inst->server, cp->rsp_code, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t get_image_list(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_get_image_list_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_final_valid(cp->final)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_get_image_list(&inst->client, cp->final, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t get_image_list_rsp(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_get_image_list_rsp_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_get_image_list_rsp(&inst->server, cp->rsp_code, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t get_image_properties(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_get_image_properties_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_final_valid(cp->final)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_get_image_properties(&inst->client, cp->final, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t get_image_properties_rsp(const void *cmd, uint16_t cmd_len, void *rsp,
					uint16_t *rsp_len)
{
	const struct btp_bip_get_image_properties_rsp_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_get_image_properties_rsp(&inst->server, cp->rsp_code, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t get_image(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_get_image_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_final_valid(cp->final)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_get_image(&inst->client, cp->final, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t get_image_rsp(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_get_image_rsp_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_get_image_rsp(&inst->server, cp->rsp_code, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t get_linked_thumbnail(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_get_linked_thumbnail_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_final_valid(cp->final)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_get_linked_thumbnail(&inst->client, cp->final, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t get_linked_thumbnail_rsp(const void *cmd, uint16_t cmd_len, void *rsp,
					uint16_t *rsp_len)
{
	const struct btp_bip_get_linked_thumbnail_rsp_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_get_linked_thumbnail_rsp(&inst->server, cp->rsp_code, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t get_linked_attachment(const void *cmd, uint16_t cmd_len, void *rsp,
				     uint16_t *rsp_len)
{
	const struct btp_bip_get_linked_attachment_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_final_valid(cp->final)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_get_linked_attachment(&inst->client, cp->final, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t get_linked_attachment_rsp(const void *cmd, uint16_t cmd_len, void *rsp,
					 uint16_t *rsp_len)
{
	const struct btp_bip_get_linked_attachment_rsp_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_get_linked_attachment_rsp(&inst->server, cp->rsp_code, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t get_partial_image(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_get_partial_image_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_final_valid(cp->final)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_get_partial_image(&inst->client, cp->final, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t get_partial_image_rsp(const void *cmd, uint16_t cmd_len, void *rsp,
				     uint16_t *rsp_len)
{
	const struct btp_bip_get_partial_image_rsp_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->second_bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_get_partial_image_rsp(&inst->second_server, cp->rsp_code, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t second_get_capabilities(const void *cmd, uint16_t cmd_len, void *rsp,
				       uint16_t *rsp_len)
{
	const struct btp_bip_second_get_capabilities_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_final_valid(cp->final)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->second_bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_get_capabilities(&inst->second_client, cp->final, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t second_get_image_list(const void *cmd, uint16_t cmd_len, void *rsp,
				     uint16_t *rsp_len)
{
	const struct btp_bip_second_get_image_list_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_final_valid(cp->final)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->second_bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_get_image_list(&inst->second_client, cp->final, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t second_get_image_properties(const void *cmd, uint16_t cmd_len, void *rsp,
					   uint16_t *rsp_len)
{
	const struct btp_bip_second_get_image_properties_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_final_valid(cp->final)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->second_bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_get_image_properties(&inst->second_client, cp->final, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t second_get_image(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_second_get_image_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_final_valid(cp->final)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->second_bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_get_image(&inst->second_client, cp->final, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t second_get_linked_thumbnail(const void *cmd, uint16_t cmd_len, void *rsp,
					   uint16_t *rsp_len)
{
	const struct btp_bip_second_get_linked_thumbnail_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_final_valid(cp->final)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->second_bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_get_linked_thumbnail(&inst->second_client, cp->final, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t second_get_linked_attachment(const void *cmd, uint16_t cmd_len, void *rsp,
					    uint16_t *rsp_len)
{
	const struct btp_bip_second_get_linked_attachment_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_final_valid(cp->final)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->second_bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_get_linked_attachment(&inst->second_client, cp->final, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t second_get_partial_image(const void *cmd, uint16_t cmd_len, void *rsp,
					uint16_t *rsp_len)
{
	const struct btp_bip_second_get_partial_image_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_final_valid(cp->final)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->second_bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_get_partial_image(&inst->second_client, cp->final, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t second_delete_image(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_second_delete_image_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_final_valid(cp->final)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->second_bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_delete_image(&inst->second_client, cp->final, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t get_monitoring_image(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_get_monitoring_image_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_final_valid(cp->final)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_get_monitoring_image(&inst->client, cp->final, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t get_monitoring_image_rsp(const void *cmd, uint16_t cmd_len, void *rsp,
					uint16_t *rsp_len)
{
	const struct btp_bip_get_monitoring_image_rsp_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_get_monitoring_image_rsp(&inst->server, cp->rsp_code, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t get_status(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_get_status_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_final_valid(cp->final)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_get_status(&inst->client, cp->final, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t get_status_rsp(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_get_status_rsp_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_get_status_rsp(&inst->server, cp->rsp_code, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t put_image(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_put_image_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_final_valid(cp->final)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_put_image(&inst->client, cp->final, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t put_image_rsp(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_put_image_rsp_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_put_image_rsp(&inst->server, cp->rsp_code, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t put_linked_thumbnail(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_put_linked_thumbnail_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_final_valid(cp->final)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_put_linked_thumbnail(&inst->client, cp->final, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t put_linked_thumbnail_rsp(const void *cmd, uint16_t cmd_len, void *rsp,
					uint16_t *rsp_len)
{
	const struct btp_bip_put_linked_thumbnail_rsp_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_put_linked_thumbnail_rsp(&inst->server, cp->rsp_code, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t put_linked_attachment(const void *cmd, uint16_t cmd_len, void *rsp,
				     uint16_t *rsp_len)
{
	const struct btp_bip_put_linked_attachment_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_final_valid(cp->final)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_put_linked_attachment(&inst->client, cp->final, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t put_linked_attachment_rsp(const void *cmd, uint16_t cmd_len, void *rsp,
					 uint16_t *rsp_len)
{
	const struct btp_bip_put_linked_attachment_rsp_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_put_linked_attachment_rsp(&inst->server, cp->rsp_code, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t remote_display(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_remote_display_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_final_valid(cp->final)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_remote_display(&inst->client, cp->final, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t remote_display_rsp(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_remote_display_rsp_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_remote_display_rsp(&inst->server, cp->rsp_code, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t delete_image(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_delete_image_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_final_valid(cp->final)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_delete_image(&inst->client, cp->final, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t delete_image_rsp(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_delete_image_rsp_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_delete_image_rsp(&inst->server, cp->rsp_code, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t start_print(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_start_print_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_final_valid(cp->final)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_start_print(&inst->client, cp->final, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t start_print_rsp(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_start_print_rsp_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_start_print_rsp(&inst->server, cp->rsp_code, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t start_archive(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_start_archive_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_final_valid(cp->final)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_start_archive(&inst->client, cp->final, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t start_archive_rsp(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_start_archive_rsp_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_start_archive_rsp(&inst->server, cp->rsp_code, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static const struct bt_uuid_128 *const bip_second_referenced_obj = BT_BIP_UUID_REFERENCED_OBJ;
static const struct bt_uuid_128 *const bip_second_archived_obj = BT_BIP_UUID_ARCHIVED_OBJ;

static const struct bt_uuid_128 *bip_second_uuid(enum bt_bip_conn_type type)
{
	switch (type) {
	case BT_BIP_2ND_CONN_TYPE_REFERENCED_OBJECTS:
		return bip_second_referenced_obj;
	case BT_BIP_2ND_CONN_TYPE_ARCHIVED_OBJECTS:
		return bip_second_archived_obj;
	default:
		return NULL;
	}
}

static uint8_t second_server_register(const void *cmd, uint16_t cmd_len, void *rsp,
				      uint16_t *rsp_len)
{
	const struct btp_bip_second_server_register_cmd *cp = cmd;
	enum bt_bip_conn_type type = cp->conn_type;
	struct bip_app *inst;
	const struct bt_uuid_128 *u;
	int err;

	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	u = bip_second_uuid(type);

	if (u == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_secondary_server_register(&inst->second_bip, &inst->second_server, type, u,
					       &bip_second_server_cb, &inst->client);
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	inst->secondary_registered = true;

	return BTP_STATUS_SUCCESS;
}

static uint8_t second_server_unregister(const void *cmd, uint16_t cmd_len, void *rsp,
					uint16_t *rsp_len)
{
	const struct btp_bip_second_server_unregister_cmd *cp = cmd;
	struct bip_app *inst;
	int err;

	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	inst = find_second_server_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_server_unregister(&inst->second_server);
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	inst->secondary_registered = false;
	bip_instance_gc(inst);

	return BTP_STATUS_SUCCESS;
}

static uint8_t second_connect(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_second_connect_cmd *cp = cmd;
	enum bt_bip_conn_type type = cp->conn_type;
	struct bip_app *inst;
	int err;

	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	if (bip_second_uuid(type) == NULL) {
		LOG_ERR("BIP secondary connect with invalid conn_type %u", cp->conn_type);
		return BTP_STATUS_FAILED;
	}

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	bt_bip_set_supported_capabilities(&inst->second_bip, bip_supported_caps);
	bt_bip_set_supported_features(&inst->second_bip, bip_supported_features);
	bt_bip_set_supported_functions(&inst->second_bip,
				       type == BT_BIP_2ND_CONN_TYPE_ARCHIVED_OBJECTS
					       ? bip_archive_supported_functions
					       : bip_refobj_supported_functions);

	err = bt_bip_secondary_client_connect(&inst->second_bip, &inst->second_client, type,
					      &bip_second_client_cb, NULL, &inst->server);
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t second_obex_disconnect(const void *cmd, uint16_t cmd_len, void *rsp,
				      uint16_t *rsp_len)
{
	const struct btp_bip_second_obex_disconnect_cmd *cp = cmd;
	struct bip_app *inst;
	int err;

	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_disconnect(&inst->second_client, NULL);
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t second_obex_abort(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_second_obex_abort_cmd *cp = cmd;
	struct bip_app *inst;
	int err;

	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_abort(&inst->second_client, NULL);
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t second_connect_rsp(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_second_connect_rsp_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf = NULL;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	if (data_len > 0) {
		buf = alloc_buf_with_data_bip(&inst->second_bip, cp->data, data_len);
		if (buf == NULL) {
			return BTP_STATUS_FAILED;
		}
	}

	err = bt_bip_connect_rsp(&inst->second_server, cp->rsp_code, buf);
	if (err != 0) {
		if (buf != NULL) {
			net_buf_unref(buf);
		}
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t second_disconnect_rsp(const void *cmd, uint16_t cmd_len, void *rsp,
				     uint16_t *rsp_len)
{
	const struct btp_bip_second_disconnect_rsp_cmd *cp = cmd;
	struct bip_app *inst;
	int err;

	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_disconnect_rsp(&inst->second_server, cp->rsp_code, NULL);
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t second_abort_rsp(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_second_abort_rsp_cmd *cp = cmd;
	struct bip_app *inst;
	int err;

	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_abort_rsp(&inst->second_server, cp->rsp_code, NULL);
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t second_get_capabilities_rsp(const void *cmd, uint16_t cmd_len, void *rsp,
					   uint16_t *rsp_len)
{
	const struct btp_bip_second_get_capabilities_rsp_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->second_bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_get_capabilities_rsp(&inst->second_server, cp->rsp_code, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t second_get_image_list_rsp(const void *cmd, uint16_t cmd_len, void *rsp,
					 uint16_t *rsp_len)
{
	const struct btp_bip_second_get_image_list_rsp_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->second_bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_get_image_list_rsp(&inst->second_server, cp->rsp_code, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t second_get_image_properties_rsp(const void *cmd, uint16_t cmd_len, void *rsp,
					       uint16_t *rsp_len)
{
	const struct btp_bip_second_get_image_properties_rsp_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->second_bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_get_image_properties_rsp(&inst->second_server, cp->rsp_code, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t second_get_image_rsp(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_second_get_image_rsp_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->second_bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_get_image_rsp(&inst->second_server, cp->rsp_code, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t second_get_linked_thumbnail_rsp(const void *cmd, uint16_t cmd_len, void *rsp,
					       uint16_t *rsp_len)
{
	const struct btp_bip_second_get_linked_thumbnail_rsp_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->second_bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_get_linked_thumbnail_rsp(&inst->second_server, cp->rsp_code, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t second_get_linked_attachment_rsp(const void *cmd, uint16_t cmd_len, void *rsp,
						uint16_t *rsp_len)
{
	const struct btp_bip_second_get_linked_attachment_rsp_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->second_bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_get_linked_attachment_rsp(&inst->second_server, cp->rsp_code, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t second_delete_image_rsp(const void *cmd, uint16_t cmd_len, void *rsp,
				       uint16_t *rsp_len)
{
	const struct btp_bip_second_delete_image_rsp_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	if (!bip_var_len_valid(cmd_len, sizeof(*cp), &cp->data_len)) {
		return BTP_STATUS_FAILED;
	}
	if (!bip_addr_valid(&cp->address)) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_any_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = alloc_buf_with_data_bip(&inst->second_bip, cp->data, data_len);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_delete_image_rsp(&inst->second_server, cp->rsp_code, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static const struct btp_handler handlers[] = {
	{
		.opcode = BTP_BIP_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = supported_commands,
	},
	{
		.opcode = BTP_BIP_CONNECT_RFCOMM,
		.expect_len = sizeof(struct btp_bip_connect_rfcomm_cmd),
		.func = connect_rfcomm,
	},
	{
		.opcode = BTP_BIP_DISCONNECT_RFCOMM,
		.expect_len = sizeof(struct btp_bip_disconnect_rfcomm_cmd),
		.func = disconnect_rfcomm,
	},
	{
		.opcode = BTP_BIP_CONNECT_L2CAP,
		.expect_len = sizeof(struct btp_bip_connect_l2cap_cmd),
		.func = connect_l2cap,
	},
	{
		.opcode = BTP_BIP_DISCONNECT_L2CAP,
		.expect_len = sizeof(struct btp_bip_disconnect_l2cap_cmd),
		.func = disconnect_l2cap,
	},
	{
		.opcode = BTP_BIP_SDP_DISCOVER,
		.expect_len = sizeof(struct btp_bip_sdp_discover_cmd),
		.func = sdp_discover,
	},
	{
		.opcode = BTP_BIP_SERVER_REGISTER,
		.expect_len = sizeof(struct btp_bip_server_register_cmd),
		.func = server_register,
	},
	{
		.opcode = BTP_BIP_SERVER_UNREGISTER,
		.expect_len = sizeof(struct btp_bip_server_unregister_cmd),
		.func = server_unregister,
	},
	{
		.opcode = BTP_BIP_CLIENT_CONNECT,
		.expect_len = sizeof(struct btp_bip_client_connect_cmd),
		.func = client_connect,
	},
	{
		.opcode = BTP_BIP_OBEX_DISCONNECT,
		.expect_len = sizeof(struct btp_bip_obex_disconnect_cmd),
		.func = obex_disconnect,
	},
	{
		.opcode = BTP_BIP_OBEX_ABORT,
		.expect_len = sizeof(struct btp_bip_obex_abort_cmd),
		.func = obex_abort,
	},
	{
		.opcode = BTP_BIP_CONNECT_RSP,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = connect_rsp,
	},
	{
		.opcode = BTP_BIP_DISCONNECT_RSP,
		.expect_len = sizeof(struct btp_bip_disconnect_rsp_cmd),
		.func = disconnect_rsp,
	},
	{
		.opcode = BTP_BIP_ABORT_RSP,
		.expect_len = sizeof(struct btp_bip_abort_rsp_cmd),
		.func = abort_rsp,
	},
	{
		.opcode = BTP_BIP_GET_CAPABILITIES,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = get_capabilities,
	},
	{
		.opcode = BTP_BIP_GET_CAPABILITIES_RSP,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = get_capabilities_rsp,
	},
	{
		.opcode = BTP_BIP_GET_IMAGE_LIST,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = get_image_list,
	},
	{
		.opcode = BTP_BIP_GET_IMAGE_LIST_RSP,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = get_image_list_rsp,
	},
	{
		.opcode = BTP_BIP_GET_IMAGE_PROPERTIES,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = get_image_properties,
	},
	{
		.opcode = BTP_BIP_GET_IMAGE_PROPERTIES_RSP,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = get_image_properties_rsp,
	},
	{
		.opcode = BTP_BIP_GET_IMAGE,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = get_image,
	},
	{
		.opcode = BTP_BIP_GET_IMAGE_RSP,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = get_image_rsp,
	},
	{
		.opcode = BTP_BIP_GET_LINKED_THUMBNAIL,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = get_linked_thumbnail,
	},
	{
		.opcode = BTP_BIP_GET_LINKED_THUMBNAIL_RSP,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = get_linked_thumbnail_rsp,
	},
	{
		.opcode = BTP_BIP_GET_LINKED_ATTACHMENT,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = get_linked_attachment,
	},
	{
		.opcode = BTP_BIP_GET_LINKED_ATTACHMENT_RSP,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = get_linked_attachment_rsp,
	},
	{
		.opcode = BTP_BIP_GET_PARTIAL_IMAGE,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = get_partial_image,
	},
	{
		.opcode = BTP_BIP_GET_PARTIAL_IMAGE_RSP,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = get_partial_image_rsp,
	},
	{
		.opcode = BTP_BIP_GET_MONITORING_IMAGE,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = get_monitoring_image,
	},
	{
		.opcode = BTP_BIP_GET_MONITORING_IMAGE_RSP,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = get_monitoring_image_rsp,
	},
	{
		.opcode = BTP_BIP_GET_STATUS,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = get_status,
	},
	{
		.opcode = BTP_BIP_GET_STATUS_RSP,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = get_status_rsp,
	},
	{
		.opcode = BTP_BIP_PUT_IMAGE,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = put_image,
	},
	{
		.opcode = BTP_BIP_PUT_IMAGE_RSP,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = put_image_rsp,
	},
	{
		.opcode = BTP_BIP_PUT_LINKED_THUMBNAIL,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = put_linked_thumbnail,
	},
	{
		.opcode = BTP_BIP_PUT_LINKED_THUMBNAIL_RSP,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = put_linked_thumbnail_rsp,
	},
	{
		.opcode = BTP_BIP_PUT_LINKED_ATTACHMENT,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = put_linked_attachment,
	},
	{
		.opcode = BTP_BIP_PUT_LINKED_ATTACHMENT_RSP,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = put_linked_attachment_rsp,
	},
	{
		.opcode = BTP_BIP_REMOTE_DISPLAY,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = remote_display,
	},
	{
		.opcode = BTP_BIP_REMOTE_DISPLAY_RSP,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = remote_display_rsp,
	},
	{
		.opcode = BTP_BIP_DELETE_IMAGE,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = delete_image,
	},
	{
		.opcode = BTP_BIP_DELETE_IMAGE_RSP,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = delete_image_rsp,
	},
	{
		.opcode = BTP_BIP_START_PRINT,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = start_print,
	},
	{
		.opcode = BTP_BIP_START_PRINT_RSP,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = start_print_rsp,
	},
	{
		.opcode = BTP_BIP_START_ARCHIVE,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = start_archive,
	},
	{
		.opcode = BTP_BIP_START_ARCHIVE_RSP,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = start_archive_rsp,
	},
	{
		.opcode = BTP_BIP_SECOND_SERVER_REGISTER,
		.expect_len = sizeof(struct btp_bip_second_server_register_cmd),
		.func = second_server_register,
	},
	{
		.opcode = BTP_BIP_SECOND_CONNECT,
		.expect_len = sizeof(struct btp_bip_second_connect_cmd),
		.func = second_connect,
	},
	{
		.opcode = BTP_BIP_SECOND_OBEX_DISCONNECT,
		.expect_len = sizeof(struct btp_bip_second_obex_disconnect_cmd),
		.func = second_obex_disconnect,
	},
	{
		.opcode = BTP_BIP_SECOND_OBEX_ABORT,
		.expect_len = sizeof(struct btp_bip_second_obex_abort_cmd),
		.func = second_obex_abort,
	},
	{
		.opcode = BTP_BIP_SECOND_CONNECT_L2CAP,
		.expect_len = sizeof(struct btp_bip_second_connect_l2cap_cmd),
		.func = second_connect_l2cap,
	},
	{
		.opcode = BTP_BIP_SECOND_CONNECT_RFCOMM,
		.expect_len = sizeof(struct btp_bip_second_connect_rfcomm_cmd),
		.func = second_connect_rfcomm,
	},
	{
		.opcode = BTP_BIP_SECOND_DISCONNECT_L2CAP,
		.expect_len = sizeof(struct btp_bip_second_disconnect_l2cap_cmd),
		.func = second_disconnect_l2cap,
	},
	{
		.opcode = BTP_BIP_SECOND_DISCONNECT_RFCOMM,
		.expect_len = sizeof(struct btp_bip_second_disconnect_rfcomm_cmd),
		.func = second_disconnect_rfcomm,
	},

	{
		.opcode = BTP_BIP_SECOND_CONNECT_RSP,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = second_connect_rsp,
	},
	{
		.opcode = BTP_BIP_SECOND_DISCONNECT_RSP,
		.expect_len = sizeof(struct btp_bip_second_disconnect_rsp_cmd),
		.func = second_disconnect_rsp,
	},
	{
		.opcode = BTP_BIP_SECOND_ABORT_RSP,
		.expect_len = sizeof(struct btp_bip_second_abort_rsp_cmd),
		.func = second_abort_rsp,
	},
	{
		.opcode = BTP_BIP_SECOND_SERVER_UNREGISTER,
		.expect_len = sizeof(struct btp_bip_second_server_unregister_cmd),
		.func = second_server_unregister,
	},
	{
		.opcode = BTP_BIP_SECOND_GET_CAPABILITIES,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = second_get_capabilities,
	},
	{
		.opcode = BTP_BIP_SECOND_GET_IMAGE_LIST,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = second_get_image_list,
	},
	{
		.opcode = BTP_BIP_SECOND_GET_IMAGE_PROPERTIES,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = second_get_image_properties,
	},
	{
		.opcode = BTP_BIP_SECOND_GET_IMAGE,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = second_get_image,
	},
	{
		.opcode = BTP_BIP_SECOND_GET_LINKED_THUMBNAIL,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = second_get_linked_thumbnail,
	},
	{
		.opcode = BTP_BIP_SECOND_GET_LINKED_ATTACHMENT,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = second_get_linked_attachment,
	},
	{
		.opcode = BTP_BIP_SECOND_GET_PARTIAL_IMAGE,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = second_get_partial_image,
	},
	{
		.opcode = BTP_BIP_SECOND_DELETE_IMAGE,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = second_delete_image,
	},
	{
		.opcode = BTP_BIP_SECOND_GET_CAPABILITIES_RSP,

		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = second_get_capabilities_rsp,
	},

	{
		.opcode = BTP_BIP_SECOND_GET_IMAGE_LIST_RSP,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = second_get_image_list_rsp,
	},
	{
		.opcode = BTP_BIP_SECOND_GET_IMAGE_PROPERTIES_RSP,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = second_get_image_properties_rsp,
	},
	{
		.opcode = BTP_BIP_SECOND_GET_IMAGE_RSP,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = second_get_image_rsp,
	},
	{
		.opcode = BTP_BIP_SECOND_GET_LINKED_THUMBNAIL_RSP,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = second_get_linked_thumbnail_rsp,
	},
	{
		.opcode = BTP_BIP_SECOND_GET_LINKED_ATTACHMENT_RSP,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = second_get_linked_attachment_rsp,
	},
	{
		.opcode = BTP_BIP_SECOND_DELETE_IMAGE_RSP,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = second_delete_image_rsp,
	},
};

static int bip_responder_register(void)
{
	int err;

	rfcomm_server.server.rfcomm.channel = bip_rfcomm_channel;
	rfcomm_server.accept = rfcomm_accept;
	err = bt_bip_rfcomm_register(&rfcomm_server);
	if (err != 0) {
		return err;
	}

	l2cap_server.server.l2cap.psm = bip_l2cap_psm;
	l2cap_server.accept = l2cap_accept;
	err = bt_bip_l2cap_register(&l2cap_server);
	if (err != 0) {
		return err;
	}

	err = bt_sdp_register_service(&bip_responder_rec);
	if (err != 0) {
		return err;
	}

	return 0;
}

static int bip_archive_register(void)
{
	int err;

	archive_rfcomm_server.server.rfcomm.channel = bip_archive_rfcomm_channel;
	archive_rfcomm_server.accept = archive_rfcomm_accept;
	err = bt_bip_rfcomm_register(&archive_rfcomm_server);
	if (err != 0) {
		return err;
	}

	archive_l2cap_server.server.l2cap.psm = bip_archive_l2cap_psm;
	archive_l2cap_server.accept = archive_l2cap_accept;

	err = bt_bip_l2cap_register(&archive_l2cap_server);
	if (err != 0) {
		return err;
	}

	err = bt_sdp_register_service(&bip_archive_rec);
	if (err != 0) {
		return err;
	}

	return 0;
}

static int bip_refobj_register(void)
{
	int err;

	refobj_rfcomm_server.server.rfcomm.channel = bip_refobj_rfcomm_channel;
	refobj_rfcomm_server.accept = refobj_rfcomm_accept;
	err = bt_bip_rfcomm_register(&refobj_rfcomm_server);
	if (err != 0) {
		return err;
	}

	refobj_l2cap_server.server.l2cap.psm = bip_refobj_l2cap_psm;
	refobj_l2cap_server.accept = refobj_l2cap_accept;
	err = bt_bip_l2cap_register(&refobj_l2cap_server);
	if (err != 0) {
		return err;
	}

	err = bt_sdp_register_service(&bip_refobj_rec);
	if (err != 0) {
		return err;
	}

	return 0;
}

uint8_t tester_init_bip(void)
{
	tester_register_command_handlers(BTP_SERVICE_ID_BIP, handlers, ARRAY_SIZE(handlers));

	if (bip_responder_register() != 0) {
		return BTP_STATUS_FAILED;
	}

	if (bip_archive_register() != 0) {
		return BTP_STATUS_FAILED;
	}

	if (bip_refobj_register() != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_bip(void)
{
	k_mutex_lock(&bip_apps_lock, K_FOREVER);

	for (uint8_t i = 0; i < BIP_MAX_INSTANCES; i++) {
		if (bip_apps[i].in_use) {
			bip_instance_free(&bip_apps[i]);
		}
	}

	k_mutex_unlock(&bip_apps_lock);

	return BTP_STATUS_SUCCESS;
}
