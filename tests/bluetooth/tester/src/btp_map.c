/* btp_map.c - Bluetooth MAP Tester */

/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/classic/map.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "btp/btp.h"

#define LOG_MODULE_NAME bttester_map
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);

#define MAP_MOPL BT_OBEX_MIN_MTU

/*
 * 6 bytes - Field length of L2CAP I-frame, including,
 *   4 bytes - extended control field,
 *   2 bytes - FCS field.
 */
#define MAP_TX_BUF_SIZE BT_L2CAP_BUF_SIZE(MAP_MOPL + 6 + BT_OBEX_SEND_BUF_RESERVE - BT_OBEX_HDR_LEN)

#define MAP_MAS_MAX_NUM            2
#define MAP_MCE_SUPPORTED_FEATURES 0x0077FFFF
#define MAP_MSE_SUPPORTED_FEATURES 0x007FFFFF, 0x007FFFFF
#define MAP_MSE_SUPPORTED_MSG_TYPE 0x1F, 0x1F

NET_BUF_POOL_FIXED_DEFINE(tx_pool, (CONFIG_BT_MAX_CONN * MAP_MAS_MAX_NUM), MAP_TX_BUF_SIZE,
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

/* MAP Client MAS instance tracking */
struct mce_mas_instance {
	struct bt_map_mce_mas mce_mas;
	struct bt_conn *conn;
	uint8_t instance_id;
};

/* MAP Client MNS instance tracking */
struct mce_mns_instance {
	struct bt_map_mce_mns mce_mns;
	struct bt_conn *conn;
};

/* MAP Server MAS instance tracking */
struct mse_mas_instance {
	struct bt_map_mse_mas mse_mas;
	struct bt_conn *conn;
	uint16_t psm;
	uint8_t channel;
	uint8_t instance_id;
};

/* MAP Server MNS instance tracking */
struct mse_mns_instance {
	struct bt_map_mse_mns mse_mns;
	struct bt_conn *conn;
};

static struct mce_mas_instance mce_mas_instances[CONFIG_BT_MAX_CONN][MAP_MAS_MAX_NUM];
static struct mce_mns_instance mce_mns_instances[CONFIG_BT_MAX_CONN];
static struct mse_mas_instance mse_mas_instances[CONFIG_BT_MAX_CONN][MAP_MAS_MAX_NUM];
static struct mse_mns_instance mse_mns_instances[CONFIG_BT_MAX_CONN];

/* MCE MNS Server structure */
struct mce_server {
	struct bt_map_mce_mns_rfcomm_server rfcomm_server;
	struct bt_map_mce_mns_l2cap_server l2cap_server;
	uint32_t supported_features;
};

static struct mce_server mce_server = {
	.supported_features = MAP_MCE_SUPPORTED_FEATURES,
};

/* MCE MNS SDP record */
static struct bt_sdp_attribute mce_mns_attrs[] = {
	BT_SDP_NEW_SERVICE,
	/* ServiceClassIDList */
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_MAP_MCE_SVCLASS)
			},
		)
	),
	/* ProtocolDescriptorList - RFCOMM */
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
						&mce_server.rfcomm_server.server.rfcomm.channel
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
	/* BluetoothProfileDescriptorList */
	BT_SDP_LIST(
		BT_SDP_ATTR_PROFILE_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
		BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
				BT_SDP_DATA_ELEM_LIST(
					{
						BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
						BT_SDP_ARRAY_16(BT_SDP_MAP_SVCLASS)
					},
					{
						BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
						BT_SDP_ARRAY_16(0x0104)
					},
				)
			},
		)
	),
	/* ServiceName */
	BT_SDP_SERVICE_NAME("MAP MNS"),
	/* GOEP L2CAP PSM (Optional) */
	{
		BT_SDP_ATTR_GOEP_L2CAP_PSM,
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
			&mce_server.l2cap_server.server.l2cap.psm
		}
	},
	/* MAPSupportedFeatures */
	{
		BT_SDP_ATTR_MAP_SUPPORTED_FEATURES,
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT32),
			&mce_server.supported_features
		}
	},
};

static struct bt_sdp_record mce_mns_rec = BT_SDP_RECORD(mce_mns_attrs);

/* MSE MAS Server structure */
struct mse_server {
	struct bt_map_mse_mas_rfcomm_server rfcomm_server;
	struct bt_map_mse_mas_l2cap_server l2cap_server;
	uint32_t supported_features;
	uint8_t instance_id;
	uint8_t supported_msg_type;
};

#define MSE_SERVER_INIT(i, _)                                                                      \
	{                                                                                          \
		.supported_features = GET_ARG_N(UTIL_INC(i), MAP_MSE_SUPPORTED_FEATURES),          \
		.instance_id = i,                                                                  \
		.supported_msg_type = GET_ARG_N(UTIL_INC(i), MAP_MSE_SUPPORTED_MSG_TYPE),          \
	}

static struct mse_server mse_server[MAP_MAS_MAX_NUM] = {
	LISTIFY(MAP_MAS_MAX_NUM, MSE_SERVER_INIT, (,)) };

/* MSE MAS SDP record */
#define MSE_MAS_ATTRS(i, _) \
static struct bt_sdp_attribute UTIL_CAT(mse_mas_, UTIL_CAT(i, _attrs))[] = { \
	BT_SDP_NEW_SERVICE, \
	BT_SDP_LIST( \
		BT_SDP_ATTR_SVCLASS_ID_LIST, \
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3), \
		BT_SDP_DATA_ELEM_LIST( \
			{ \
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16), \
				BT_SDP_ARRAY_16(BT_SDP_MAP_MSE_SVCLASS) \
			}, \
		) \
	), \
	BT_SDP_LIST( \
		BT_SDP_ATTR_PROTO_DESC_LIST, \
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 17), \
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
						&mse_server[i].rfcomm_server.server.rfcomm.channel \
					}, \
				) \
			}, \
			{ \
				BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3), \
				BT_SDP_DATA_ELEM_LIST( \
					{ \
						BT_SDP_TYPE_SIZE(BT_SDP_UUID16), \
						BT_SDP_ARRAY_16(BT_SDP_PROTO_OBEX) \
					}, \
				) \
			}, \
		) \
	), \
	BT_SDP_LIST( \
		BT_SDP_ATTR_PROFILE_DESC_LIST, \
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8), \
		BT_SDP_DATA_ELEM_LIST( \
			{ \
				BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6), \
				BT_SDP_DATA_ELEM_LIST( \
					{ \
						BT_SDP_TYPE_SIZE(BT_SDP_UUID16), \
						BT_SDP_ARRAY_16(BT_SDP_MAP_SVCLASS) \
					}, \
					{ \
						BT_SDP_TYPE_SIZE(BT_SDP_UINT16), \
						BT_SDP_ARRAY_16(0x0104) \
					}, \
				) \
			}, \
		) \
	), \
	BT_SDP_SERVICE_NAME("MAP MAS " STRINGIFY(i)), \
	{ \
		BT_SDP_ATTR_GOEP_L2CAP_PSM, \
		{ \
			BT_SDP_TYPE_SIZE(BT_SDP_UINT16), \
			&mse_server[i].l2cap_server.server.l2cap.psm \
		} \
	}, \
	{ \
		BT_SDP_ATTR_MAS_INSTANCE_ID, \
		{ \
			BT_SDP_TYPE_SIZE(BT_SDP_UINT8), \
			&mse_server[i].instance_id \
		} \
	}, \
	{ \
		BT_SDP_ATTR_SUPPORTED_MESSAGE_TYPES, \
		{ \
			BT_SDP_TYPE_SIZE(BT_SDP_UINT8), \
			&mse_server[i].supported_msg_type \
		} \
	}, \
	{ \
		BT_SDP_ATTR_MAP_SUPPORTED_FEATURES, \
		{ \
			BT_SDP_TYPE_SIZE(BT_SDP_UINT32), \
			&mse_server[i].supported_features \
		} \
	}, \
};

#define MSE_MAS_SDP_RECORD_INIT(idx, _) BT_SDP_RECORD(UTIL_CAT(mse_mas_, UTIL_CAT(idx, _attrs)))

LISTIFY(MAP_MAS_MAX_NUM, MSE_MAS_ATTRS, (;))

static struct bt_sdp_record mse_mas_rec[MAP_MAS_MAX_NUM] = {
	LISTIFY(MAP_MAS_MAX_NUM, MSE_MAS_SDP_RECORD_INIT, (,)) };

/* SDP discover support */
static struct bt_sdp_discover_params map_sdp_discover_params;

NET_BUF_POOL_DEFINE(map_sdp_discover_pool, 1, BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_TX_MTU),
		    CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

/* SDP helper functions to extract MAP attributes */
static int map_sdp_get_goep_l2cap_psm(const struct net_buf *buf, uint16_t *psm)
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

	if ((value.type != BT_SDP_ATTR_VALUE_TYPE_UINT) || (value.uint.size != sizeof(*psm))) {
		return -EINVAL;
	}

	*psm = value.uint.u16;
	return 0;
}

static int map_sdp_get_features(const struct net_buf *buf, uint32_t *feature)
{
	int err;
	struct bt_sdp_attribute attr;
	struct bt_sdp_attr_value value;

	err = bt_sdp_get_attr(buf, BT_SDP_ATTR_MAP_SUPPORTED_FEATURES, &attr);
	if (err != 0) {
		return err;
	}

	err = bt_sdp_attr_read(&attr, NULL, &value);
	if (err != 0) {
		return err;
	}

	if ((value.type != BT_SDP_ATTR_VALUE_TYPE_UINT) || (value.uint.size != sizeof(*feature))) {
		return -EINVAL;
	}

	*feature = value.uint.u32;
	return 0;
}

static int map_sdp_get_instance_id(const struct net_buf *buf, uint8_t *id)
{
	int err;
	struct bt_sdp_attribute attr;
	struct bt_sdp_attr_value value;

	err = bt_sdp_get_attr(buf, BT_SDP_ATTR_MAS_INSTANCE_ID, &attr);
	if (err != 0) {
		return err;
	}

	err = bt_sdp_attr_read(&attr, NULL, &value);
	if (err != 0) {
		return err;
	}

	if ((value.type != BT_SDP_ATTR_VALUE_TYPE_UINT) || (value.uint.size != sizeof(*id))) {
		return -EINVAL;
	}

	*id = value.uint.u8;
	return 0;
}

static int map_sdp_get_msg_type(const struct net_buf *buf, uint8_t *msg_type)
{
	int err;
	struct bt_sdp_attribute attr;
	struct bt_sdp_attr_value value;

	err = bt_sdp_get_attr(buf, BT_SDP_ATTR_SUPPORTED_MESSAGE_TYPES, &attr);
	if (err != 0) {
		return err;
	}

	err = bt_sdp_attr_read(&attr, NULL, &value);
	if (err != 0) {
		return err;
	}

	if ((value.type != BT_SDP_ATTR_VALUE_TYPE_UINT) || (value.uint.size != sizeof(*msg_type))) {
		return -EINVAL;
	}

	*msg_type = value.uint.u8;
	return 0;
}

static int map_sdp_get_service_name(const struct net_buf *buf, char *name, size_t name_len)
{
	int err;
	struct bt_sdp_attribute attr;
	struct bt_sdp_attr_value value;
	size_t copy_len;

	err = bt_sdp_get_attr(buf, BT_SDP_ATTR_SVCNAME_PRIMARY, &attr);
	if (err != 0) {
		return err;
	}

	err = bt_sdp_attr_read(&attr, NULL, &value);
	if (err != 0) {
		return err;
	}

	if (value.type != BT_SDP_ATTR_VALUE_TYPE_TEXT) {
		return -EINVAL;
	}

	copy_len = MIN(value.text.len, name_len - 1);
	memcpy(name, value.text.text, copy_len);
	name[copy_len] = '\0';

	return 0;
}

static uint8_t map_sdp_discover_cb(struct bt_conn *conn, struct bt_sdp_client_result *result,
				   const struct bt_sdp_discover_params *params)
{
	struct btp_map_sdp_record_ev *ev;
	struct bt_uuid_16 *uuid;
	int err;
	uint32_t features = 0;
	uint16_t rfcomm_channel = 0;
	uint16_t l2cap_psm = 0;
	uint16_t version = 0;
	uint8_t instance_id = 0;
	uint8_t msg_type = 0;
	char service_name[64] = {0};
	size_t service_name_len = 0;
	uint16_t ev_len = sizeof(struct btp_map_sdp_record_ev) + ARRAY_SIZE(service_name);

	if (result == NULL || result->resp_buf == NULL || conn == NULL) {
		LOG_DBG("SDP discovery completed or no record found");
		return BT_SDP_DISCOVER_UUID_CONTINUE;
	}

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	ev->final = result->next_record_hint ? 0 : 1;

	/* Get connection address */
	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;

	uuid = CONTAINER_OF(params->uuid, struct bt_uuid_16, uuid);
	ev->uuid = sys_cpu_to_le16(uuid->val);

	/* Get RFCOMM channel */
	err = bt_sdp_get_proto_param(result->resp_buf, BT_SDP_PROTO_RFCOMM, &rfcomm_channel);
	if (err == 0) {
		ev->rfcomm_channel = (uint8_t)rfcomm_channel;
		LOG_DBG("Found RFCOMM channel 0x%02x", ev->rfcomm_channel);
	} else {
		ev->rfcomm_channel = 0;
	}

	/* Get L2CAP PSM */
	err = map_sdp_get_goep_l2cap_psm(result->resp_buf, &l2cap_psm);
	if (err == 0) {
		ev->l2cap_psm = sys_cpu_to_le16(l2cap_psm);
		LOG_DBG("Found L2CAP PSM 0x%04x", l2cap_psm);
	} else {
		ev->l2cap_psm = 0;
	}

	/* Get MAP version */
	err = bt_sdp_get_profile_version(result->resp_buf, BT_SDP_MAP_SVCLASS, &version);
	if (err != 0) {
		ev->version = version;
		LOG_DBG("Found MAP MSG version 0x%04x", version);
	} else {
		ev->version = 0;
	}

	/* Get MAP features */
	err = map_sdp_get_features(result->resp_buf, &features);
	if (err == 0) {
		ev->supported_features = sys_cpu_to_le32(features);
		LOG_DBG("Found MAP features 0x%08x", features);
	} else {
		ev->supported_features = BT_MAP_MANDATORY_SUPPORTED_FEATURES;
	}

	/* Get instance ID (MSE only) */
	err = map_sdp_get_instance_id(result->resp_buf, &instance_id);
	if (err == 0) {
		ev->instance_id = instance_id;
		LOG_DBG("Found MAP instance ID %u", instance_id);
	} else {
		ev->instance_id = 0;
	}

	/* Get supported message types (MSE only) */
	err = map_sdp_get_msg_type(result->resp_buf, &msg_type);
	if (err == 0) {
		ev->msg_types = msg_type;
		LOG_DBG("Found MAP MSG type 0x%02x", msg_type);
	} else {
		ev->msg_types = 0;
	}

	/* Get service name */
	err = map_sdp_get_service_name(result->resp_buf, service_name, sizeof(service_name));
	if (err == 0) {
		service_name_len = strlen(service_name);
		ev->service_name_len = (uint8_t)service_name_len;
		memcpy(ev->service_name, service_name, service_name_len);
		LOG_DBG("Found service name: %s", service_name);
	} else {
		ev->service_name_len = 0;
	}

	/* Send event */
	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_EV_SDP_RECORD, ev,
		     sizeof(*ev) + ev->service_name_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();

	return BT_SDP_DISCOVER_UUID_CONTINUE;
}

static uint8_t map_sdp_discover(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_map_sdp_discover_cmd *cp = cmd;
	struct bt_conn *conn;
	static struct bt_uuid_16 uuid;
	int err;

	if (cp->address.type != BTP_BR_ADDRESS_TYPE) {
		return BTP_STATUS_FAILED;
	}

	conn = bt_conn_lookup_addr_br(&cp->address.a);
	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	/* Search for MAP MSE service (Message Server Equipment) */
	uuid.uuid.type = BT_UUID_TYPE_16;
	uuid.val = sys_le16_to_cpu(cp->uuid);

	map_sdp_discover_params.uuid = &uuid.uuid;
	map_sdp_discover_params.func = map_sdp_discover_cb;
	map_sdp_discover_params.pool = &map_sdp_discover_pool;
	map_sdp_discover_params.type = BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR;

	err = bt_sdp_discover(conn, &map_sdp_discover_params);
	bt_conn_unref(conn);

	if (err < 0) {
		LOG_ERR("SDP discovery failed (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

/* Helper functions to find instances */
static struct mce_mas_instance *mce_mas_alloc(struct bt_conn *conn)
{
	uint8_t index;

	if (conn == NULL) {
		return NULL;
	}

	index = bt_conn_index(conn);
	if (index >= CONFIG_BT_MAX_CONN) {
		return NULL;
	}

	ARRAY_FOR_EACH(mce_mas_instances[index], i) {
		struct mce_mas_instance *inst = &mce_mas_instances[index][i];

		if (inst->conn != NULL) {
			continue;
		}

		inst->conn = bt_conn_ref(conn);
		return inst;
	}

	return NULL;
}

static struct mce_mas_instance *mce_mas_find(const bt_addr_le_t *address, uint8_t instance_id)
{
	struct bt_conn *conn;

	if (address->type != BTP_BR_ADDRESS_TYPE) {
		return NULL;
	}

	conn = bt_conn_lookup_addr_br(&address->a);

	if (conn == NULL) {
		return NULL;
	}

	ARRAY_FOR_EACH(mce_mas_instances, i) {
		ARRAY_FOR_EACH(mce_mas_instances[i], j) {
			struct mce_mas_instance *inst = &mce_mas_instances[i][j];

			if ((inst->conn == conn) && (inst->instance_id == instance_id)) {
				return inst;
			}
		}
	}

	return NULL;
}

static void mce_mas_free(struct mce_mas_instance *inst)
{
	if (inst->conn != NULL) {
		bt_conn_unref(inst->conn);
		memset(&inst->conn, 0, sizeof(*inst) - offsetof(struct mce_mas_instance, conn));
	}
}

static struct mce_mns_instance *mce_mns_alloc(struct bt_conn *conn)
{
	uint8_t index;

	if (conn == NULL) {
		return NULL;
	}

	index = bt_conn_index(conn);
	if (index >= CONFIG_BT_MAX_CONN) {
		return NULL;
	}

	struct mce_mns_instance *inst = &mce_mns_instances[index];

	if (inst->conn != NULL) {
		return NULL;
	}

	inst->conn = bt_conn_ref(conn);
	return inst;
}

static struct mce_mns_instance *mce_mns_find(const bt_addr_le_t *address)
{
	struct bt_conn *conn;

	if (address->type != BTP_BR_ADDRESS_TYPE) {
		return NULL;
	}

	conn = bt_conn_lookup_addr_br(&address->a);

	if (conn == NULL) {
		return NULL;
	}

	ARRAY_FOR_EACH(mce_mns_instances, i) {
		struct mce_mns_instance *inst = &mce_mns_instances[i];

		if (inst->conn == conn) {
			return inst;
		}
	}

	return NULL;
}

static void mce_mns_free(struct mce_mns_instance *inst)
{
	if (inst->conn != NULL) {
		bt_conn_unref(inst->conn);
		memset(&inst->conn, 0, sizeof(*inst) - offsetof(struct mce_mns_instance, conn));
	}
}

static struct mse_mas_instance *mse_mas_alloc(struct bt_conn *conn)
{
	uint8_t index;

	if (conn == NULL) {
		return NULL;
	}

	index = bt_conn_index(conn);
	if (index >= CONFIG_BT_MAX_CONN) {
		return NULL;
	}

	ARRAY_FOR_EACH(mse_mas_instances[index], i) {
		struct mse_mas_instance *inst = &mse_mas_instances[index][i];

		if (inst->conn != NULL) {
			continue;
		}

		inst->conn = bt_conn_ref(conn);
		return inst;
	}

	return NULL;
}

static struct mse_mas_instance *mse_mas_find(const bt_addr_le_t *address, uint8_t instance_id)
{
	struct bt_conn *conn;

	if (address->type != BTP_BR_ADDRESS_TYPE) {
		return NULL;
	}

	conn = bt_conn_lookup_addr_br(&address->a);

	if (conn == NULL) {
		return NULL;
	}

	ARRAY_FOR_EACH(mse_mas_instances, i) {
		ARRAY_FOR_EACH(mse_mas_instances[i], j) {
			struct mse_mas_instance *inst = &mse_mas_instances[i][j];

			if ((inst->conn == conn) && (inst->instance_id == instance_id)) {
				return inst;
			}
		}
	}

	return NULL;
}

static void mse_mas_free(struct mse_mas_instance *inst)
{
	if (inst->conn != NULL) {
		bt_conn_unref(inst->conn);
		memset(&inst->conn, 0, sizeof(*inst) - offsetof(struct mse_mas_instance, conn));
	}
}

static struct mse_mns_instance *mse_mns_alloc(struct bt_conn *conn)
{
	uint8_t index;

	if (conn == NULL) {
		return NULL;
	}

	index = bt_conn_index(conn);
	if (index >= CONFIG_BT_MAX_CONN) {
		return NULL;
	}

	struct mse_mns_instance *inst = &mse_mns_instances[index];

	if (inst->conn != NULL) {
		return NULL;
	}

	inst->conn = bt_conn_ref(conn);
	return inst;
}

static struct mse_mns_instance *mse_mns_find(const bt_addr_le_t *address)
{
	struct bt_conn *conn;

	if (address->type != BTP_BR_ADDRESS_TYPE) {
		return NULL;
	}

	conn = bt_conn_lookup_addr_br(&address->a);

	if (conn == NULL) {
		return NULL;
	}

	ARRAY_FOR_EACH(mse_mns_instances, i) {
		struct mse_mns_instance *inst = &mse_mns_instances[i];

		if (inst->conn == conn) {
			return inst;
		}
	}

	return NULL;
}

static void mse_mns_free(struct mse_mns_instance *inst)
{
	if (inst->conn != NULL) {
		bt_conn_unref(inst->conn);
		memset(&inst->conn, 0, sizeof(*inst) - offsetof(struct mse_mns_instance, conn));
	}
}

/* MAP Client MAS callbacks */
static void mce_mas_rfcomm_connected_cb(struct bt_conn *conn, struct bt_map_mce_mas *mce_mas)
{
	struct mce_mas_instance *inst = CONTAINER_OF(mce_mas, struct mce_mas_instance, mce_mas);
	struct btp_map_mce_mas_rfcomm_connected_ev ev;

	bt_addr_copy(&ev.address.a, bt_conn_get_dst_br(conn));
	ev.address.type = BTP_BR_ADDRESS_TYPE;
	ev.instance_id = inst->instance_id;

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MCE_MAS_EV_RFCOMM_CONNECTED, &ev, sizeof(ev));
}

static void mce_mas_rfcomm_disconnected_cb(struct bt_map_mce_mas *mce_mas)
{
	struct mce_mas_instance *inst = CONTAINER_OF(mce_mas, struct mce_mas_instance, mce_mas);
	struct btp_map_mce_mas_rfcomm_disconnected_ev ev;

	bt_addr_copy(&ev.address.a, bt_conn_get_dst_br(inst->conn));
	ev.address.type = BTP_BR_ADDRESS_TYPE;
	ev.instance_id = inst->instance_id;

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MCE_MAS_EV_RFCOMM_DISCONNECTED, &ev, sizeof(ev));
	mce_mas_free(inst);
}

static void mce_mas_l2cap_connected_cb(struct bt_conn *conn, struct bt_map_mce_mas *mce_mas)
{
	struct mce_mas_instance *inst = CONTAINER_OF(mce_mas, struct mce_mas_instance, mce_mas);
	struct btp_map_mce_mas_l2cap_connected_ev ev;

	bt_addr_copy(&ev.address.a, bt_conn_get_dst_br(conn));
	ev.address.type = BTP_BR_ADDRESS_TYPE;
	ev.instance_id = inst->instance_id;

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MCE_MAS_EV_L2CAP_CONNECTED, &ev, sizeof(ev));
}

static void mce_mas_l2cap_disconnected_cb(struct bt_map_mce_mas *mce_mas)
{
	struct mce_mas_instance *inst = CONTAINER_OF(mce_mas, struct mce_mas_instance, mce_mas);
	struct btp_map_mce_mas_l2cap_disconnected_ev ev;

	bt_addr_copy(&ev.address.a, bt_conn_get_dst_br(inst->conn));
	ev.address.type = BTP_BR_ADDRESS_TYPE;
	ev.instance_id = inst->instance_id;

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MCE_MAS_EV_L2CAP_DISCONNECTED, &ev, sizeof(ev));
	mce_mas_free(inst);
}

static void mce_mas_connect_cb(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code, uint8_t version,
			       uint16_t mopl, struct net_buf *buf)
{
	struct mce_mas_instance *inst = CONTAINER_OF(mce_mas, struct mce_mas_instance, mce_mas);
	struct btp_map_mce_mas_connect_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->instance_id = inst->instance_id;
	ev->rsp_code = rsp_code;
	ev->version = version;
	mopl = MIN(MAP_MOPL, mopl);
	ev->mopl = sys_cpu_to_le16(mopl);
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MCE_MAS_EV_CONNECT, ev, ev_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void mce_mas_disconnect_cb(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code,
				  struct net_buf *buf)
{
	struct mce_mas_instance *inst = CONTAINER_OF(mce_mas, struct mce_mas_instance, mce_mas);
	struct btp_map_mce_mas_disconnect_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->instance_id = inst->instance_id;
	ev->rsp_code = rsp_code;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MCE_MAS_EV_DISCONNECT, ev, ev_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void mce_mas_abort_cb(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code, struct net_buf *buf)
{
	struct mce_mas_instance *inst = CONTAINER_OF(mce_mas, struct mce_mas_instance, mce_mas);
	struct btp_map_mce_mas_abort_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->instance_id = inst->instance_id;
	ev->rsp_code = rsp_code;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MCE_MAS_EV_ABORT, ev, ev_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void mce_mas_set_ntf_reg_cb(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code,
				   struct net_buf *buf)
{
	struct mce_mas_instance *inst = CONTAINER_OF(mce_mas, struct mce_mas_instance, mce_mas);
	struct btp_map_mce_mas_set_ntf_reg_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->instance_id = inst->instance_id;
	ev->rsp_code = rsp_code;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MCE_MAS_EV_SET_NTF_REG, ev, ev_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void mce_mas_set_folder_cb(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code,
				  struct net_buf *buf)
{
	struct mce_mas_instance *inst = CONTAINER_OF(mce_mas, struct mce_mas_instance, mce_mas);
	struct btp_map_mce_mas_set_folder_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->instance_id = inst->instance_id;
	ev->rsp_code = rsp_code;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MCE_MAS_EV_SET_FOLDER, ev, ev_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void mce_mas_get_folder_listing_cb(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code,
					  struct net_buf *buf)
{
	struct mce_mas_instance *inst = CONTAINER_OF(mce_mas, struct mce_mas_instance, mce_mas);
	struct btp_map_mce_mas_get_folder_listing_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->instance_id = inst->instance_id;
	ev->rsp_code = rsp_code;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MCE_MAS_EV_GET_FOLDER_LISTING, ev, ev_len);
	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void mce_mas_get_msg_listing_cb(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code,
				       struct net_buf *buf)
{
	struct mce_mas_instance *inst = CONTAINER_OF(mce_mas, struct mce_mas_instance, mce_mas);
	struct btp_map_mce_mas_get_msg_listing_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->instance_id = inst->instance_id;
	ev->rsp_code = rsp_code;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MCE_MAS_EV_GET_MSG_LISTING, ev, ev_len);
	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void mce_mas_get_msg_cb(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code,
			       struct net_buf *buf)
{
	struct mce_mas_instance *inst = CONTAINER_OF(mce_mas, struct mce_mas_instance, mce_mas);
	struct btp_map_mce_mas_get_msg_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->instance_id = inst->instance_id;
	ev->rsp_code = rsp_code;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MCE_MAS_EV_GET_MSG, ev, ev_len);
	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void mce_mas_set_msg_status_cb(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code,
				      struct net_buf *buf)
{
	struct mce_mas_instance *inst = CONTAINER_OF(mce_mas, struct mce_mas_instance, mce_mas);
	struct btp_map_mce_mas_set_msg_status_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->instance_id = inst->instance_id;
	ev->rsp_code = rsp_code;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MCE_MAS_EV_SET_MSG_STATUS, ev, ev_len);
	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void mce_mas_push_msg_cb(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code,
				struct net_buf *buf)
{
	struct mce_mas_instance *inst = CONTAINER_OF(mce_mas, struct mce_mas_instance, mce_mas);
	struct btp_map_mce_mas_push_msg_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->instance_id = inst->instance_id;
	ev->rsp_code = rsp_code;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MCE_MAS_EV_PUSH_MSG, ev, ev_len);
	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void mce_mas_update_inbox_cb(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code,
				    struct net_buf *buf)
{
	struct mce_mas_instance *inst = CONTAINER_OF(mce_mas, struct mce_mas_instance, mce_mas);
	struct btp_map_mce_mas_update_inbox_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->instance_id = inst->instance_id;
	ev->rsp_code = rsp_code;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MCE_MAS_EV_UPDATE_INBOX, ev, ev_len);
	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void mce_mas_get_mas_inst_info_cb(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code,
					 struct net_buf *buf)
{
	struct mce_mas_instance *inst = CONTAINER_OF(mce_mas, struct mce_mas_instance, mce_mas);
	struct btp_map_mce_mas_get_mas_inst_info_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->instance_id = inst->instance_id;
	ev->rsp_code = rsp_code;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MCE_MAS_EV_GET_MAS_INST_INFO, ev, ev_len);
	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void mce_mas_set_owner_status_cb(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code,
					struct net_buf *buf)
{
	struct mce_mas_instance *inst = CONTAINER_OF(mce_mas, struct mce_mas_instance, mce_mas);
	struct btp_map_mce_mas_set_owner_status_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->instance_id = inst->instance_id;
	ev->rsp_code = rsp_code;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MCE_MAS_EV_SET_OWNER_STATUS, ev, ev_len);
	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void mce_mas_get_owner_status_cb(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code,
					struct net_buf *buf)
{
	struct mce_mas_instance *inst = CONTAINER_OF(mce_mas, struct mce_mas_instance, mce_mas);
	struct btp_map_mce_mas_get_owner_status_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->instance_id = inst->instance_id;
	ev->rsp_code = rsp_code;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MCE_MAS_EV_GET_OWNER_STATUS, ev, ev_len);
	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void mce_mas_get_convo_listing_cb(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code,
					 struct net_buf *buf)
{
	struct mce_mas_instance *inst = CONTAINER_OF(mce_mas, struct mce_mas_instance, mce_mas);
	struct btp_map_mce_mas_get_convo_listing_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->instance_id = inst->instance_id;
	ev->rsp_code = rsp_code;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MCE_MAS_EV_GET_CONVO_LISTING, ev, ev_len);
	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void mce_mas_set_ntf_filter_cb(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code,
				      struct net_buf *buf)
{
	struct mce_mas_instance *inst = CONTAINER_OF(mce_mas, struct mce_mas_instance, mce_mas);
	struct btp_map_mce_mas_set_ntf_filter_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->instance_id = inst->instance_id;
	ev->rsp_code = rsp_code;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MCE_MAS_EV_SET_NTF_FILTER, ev, ev_len);
	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static const struct bt_map_mce_mas_cb mce_mas_cb = {
	.rfcomm_connected = mce_mas_rfcomm_connected_cb,
	.rfcomm_disconnected = mce_mas_rfcomm_disconnected_cb,
	.l2cap_connected = mce_mas_l2cap_connected_cb,
	.l2cap_disconnected = mce_mas_l2cap_disconnected_cb,
	.connect = mce_mas_connect_cb,
	.disconnect = mce_mas_disconnect_cb,
	.abort = mce_mas_abort_cb,
	.set_ntf_reg = mce_mas_set_ntf_reg_cb,
	.set_folder = mce_mas_set_folder_cb,
	.get_folder_listing = mce_mas_get_folder_listing_cb,
	.get_msg_listing = mce_mas_get_msg_listing_cb,
	.get_msg = mce_mas_get_msg_cb,
	.set_msg_status = mce_mas_set_msg_status_cb,
	.push_msg = mce_mas_push_msg_cb,
	.update_inbox = mce_mas_update_inbox_cb,
	.get_mas_inst_info = mce_mas_get_mas_inst_info_cb,
	.set_owner_status = mce_mas_set_owner_status_cb,
	.get_owner_status = mce_mas_get_owner_status_cb,
	.get_convo_listing = mce_mas_get_convo_listing_cb,
	.set_ntf_filter = mce_mas_set_ntf_filter_cb,
};

/* MAP Client MNS callbacks */
static void mce_mns_rfcomm_connected_cb(struct bt_conn *conn, struct bt_map_mce_mns *mce_mns)
{
	struct btp_map_mce_mns_rfcomm_connected_ev ev;

	bt_addr_copy(&ev.address.a, bt_conn_get_dst_br(conn));
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MCE_MNS_EV_RFCOMM_CONNECTED, &ev, sizeof(ev));
}

static void mce_mns_rfcomm_disconnected_cb(struct bt_map_mce_mns *mce_mns)
{
	struct mce_mns_instance *inst = CONTAINER_OF(mce_mns, struct mce_mns_instance, mce_mns);
	struct btp_map_mce_mns_rfcomm_disconnected_ev ev;

	bt_addr_copy(&ev.address.a, bt_conn_get_dst_br(inst->conn));
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MCE_MNS_EV_RFCOMM_DISCONNECTED, &ev, sizeof(ev));
	mce_mns_free(inst);
}

static void mce_mns_l2cap_connected_cb(struct bt_conn *conn, struct bt_map_mce_mns *mce_mns)
{
	struct btp_map_mce_mns_l2cap_connected_ev ev;

	bt_addr_copy(&ev.address.a, bt_conn_get_dst_br(conn));
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MCE_MNS_EV_L2CAP_CONNECTED, &ev, sizeof(ev));
}

static void mce_mns_l2cap_disconnected_cb(struct bt_map_mce_mns *mce_mns)
{
	struct mce_mns_instance *inst = CONTAINER_OF(mce_mns, struct mce_mns_instance, mce_mns);
	struct btp_map_mce_mns_l2cap_disconnected_ev ev;

	bt_addr_copy(&ev.address.a, bt_conn_get_dst_br(inst->conn));
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MCE_MNS_EV_L2CAP_DISCONNECTED, &ev, sizeof(ev));
	mce_mns_free(inst);
}

static void mce_mns_connect_cb(struct bt_map_mce_mns *mce_mns, uint8_t version, uint16_t mopl,
			       struct net_buf *buf)
{
	struct mce_mns_instance *inst = CONTAINER_OF(mce_mns, struct mce_mns_instance, mce_mns);
	struct btp_map_mce_mns_connect_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->version = version;
	mopl = MIN(MAP_MOPL, mopl);
	ev->mopl = sys_cpu_to_le16(mopl);
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MCE_MNS_EV_CONNECT, ev, ev_len);
	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void mce_mns_disconnect_cb(struct bt_map_mce_mns *mce_mns, struct net_buf *buf)
{
	struct mce_mns_instance *inst = CONTAINER_OF(mce_mns, struct mce_mns_instance, mce_mns);
	struct btp_map_mce_mns_disconnect_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MCE_MNS_EV_DISCONNECT, ev, ev_len);
	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void mce_mns_abort_cb(struct bt_map_mce_mns *mce_mns, struct net_buf *buf)
{
	struct mce_mns_instance *inst = CONTAINER_OF(mce_mns, struct mce_mns_instance, mce_mns);
	struct btp_map_mce_mns_abort_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MCE_MNS_EV_ABORT, ev, ev_len);
	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void mce_mns_send_event_cb(struct bt_map_mce_mns *mce_mns, bool final, struct net_buf *buf)
{
	struct mce_mns_instance *inst = CONTAINER_OF(mce_mns, struct mce_mns_instance, mce_mns);
	struct btp_map_mce_mns_send_event_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->final = final ? 1 : 0;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MCE_MNS_EV_SEND_EVENT, ev, ev_len);
	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static const struct bt_map_mce_mns_cb mce_mns_cb = {
	.rfcomm_connected = mce_mns_rfcomm_connected_cb,
	.rfcomm_disconnected = mce_mns_rfcomm_disconnected_cb,
	.l2cap_connected = mce_mns_l2cap_connected_cb,
	.l2cap_disconnected = mce_mns_l2cap_disconnected_cb,
	.connect = mce_mns_connect_cb,
	.disconnect = mce_mns_disconnect_cb,
	.abort = mce_mns_abort_cb,
	.send_event = mce_mns_send_event_cb,
};

/* MCE MNS Server accept callbacks */
static int mce_mns_rfcomm_accept(struct bt_conn *conn, struct bt_map_mce_mns_rfcomm_server *server,
				 struct bt_map_mce_mns **mce_mns)
{
	struct mce_mns_instance *inst;
	int err;

	inst = mce_mns_alloc(conn);
	if (inst == NULL) {
		LOG_ERR("Cannot allocate MCE MNS instance");
		return -ENOMEM;
	}

	err = bt_map_mce_mns_cb_register(&inst->mce_mns, &mce_mns_cb);
	if (err != 0) {
		mce_mns_free(inst);
		LOG_ERR("Failed to register MCE MNS cb (err %d)", err);
		return err;
	}
	*mce_mns = &inst->mce_mns;
	return 0;
}

static int mce_mns_l2cap_accept(struct bt_conn *conn, struct bt_map_mce_mns_l2cap_server *server,
				struct bt_map_mce_mns **mce_mns)
{
	struct mce_mns_instance *inst;
	int err;

	inst = mce_mns_alloc(conn);
	if (inst == NULL) {
		LOG_ERR("Cannot allocate MCE MNS instance");
		return -ENOMEM;
	}

	err = bt_map_mce_mns_cb_register(&inst->mce_mns, &mce_mns_cb);
	if (err != 0) {
		mce_mns_free(inst);
		LOG_ERR("Failed to register MCE MNS cb (err %d)", err);
		return err;
	}
	*mce_mns = &inst->mce_mns;
	return 0;
}

/* MAP Server MAS callbacks */
static void mse_mas_rfcomm_connected_cb(struct bt_conn *conn, struct bt_map_mse_mas *mse_mas)
{
	struct mse_mas_instance *inst = CONTAINER_OF(mse_mas, struct mse_mas_instance, mse_mas);
	struct btp_map_mse_mas_rfcomm_connected_ev ev;

	bt_addr_copy(&ev.address.a, bt_conn_get_dst_br(conn));
	ev.address.type = BTP_BR_ADDRESS_TYPE;
	ev.instance_id = inst->instance_id;

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MSE_MAS_EV_RFCOMM_CONNECTED, &ev, sizeof(ev));
}

static void mse_mas_rfcomm_disconnected_cb(struct bt_map_mse_mas *mse_mas)
{
	struct mse_mas_instance *inst = CONTAINER_OF(mse_mas, struct mse_mas_instance, mse_mas);
	struct btp_map_mse_mas_rfcomm_disconnected_ev ev;

	bt_addr_copy(&ev.address.a, bt_conn_get_dst_br(inst->conn));
	ev.address.type = BTP_BR_ADDRESS_TYPE;
	ev.instance_id = inst->instance_id;

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MSE_MAS_EV_RFCOMM_DISCONNECTED, &ev, sizeof(ev));
	mse_mas_free(inst);
}

static void mse_mas_l2cap_connected_cb(struct bt_conn *conn, struct bt_map_mse_mas *mse_mas)
{
	struct mse_mas_instance *inst = CONTAINER_OF(mse_mas, struct mse_mas_instance, mse_mas);
	struct btp_map_mse_mas_l2cap_connected_ev ev;

	bt_addr_copy(&ev.address.a, bt_conn_get_dst_br(conn));
	ev.address.type = BTP_BR_ADDRESS_TYPE;
	ev.instance_id = inst->instance_id;

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MSE_MAS_EV_L2CAP_CONNECTED, &ev, sizeof(ev));
}

static void mse_mas_l2cap_disconnected_cb(struct bt_map_mse_mas *mse_mas)
{
	struct mse_mas_instance *inst = CONTAINER_OF(mse_mas, struct mse_mas_instance, mse_mas);
	struct btp_map_mse_mas_l2cap_disconnected_ev ev;

	bt_addr_copy(&ev.address.a, bt_conn_get_dst_br(inst->conn));
	ev.address.type = BTP_BR_ADDRESS_TYPE;
	ev.instance_id = inst->instance_id;

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MSE_MAS_EV_L2CAP_DISCONNECTED, &ev, sizeof(ev));
	mse_mas_free(inst);
}

static void mse_mas_connect_cb(struct bt_map_mse_mas *mse_mas, uint8_t version, uint16_t mopl,
			       struct net_buf *buf)
{
	struct mse_mas_instance *inst = CONTAINER_OF(mse_mas, struct mse_mas_instance, mse_mas);
	struct btp_map_mse_mas_connect_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->instance_id = inst->instance_id;
	ev->version = version;
	mopl = MIN(MAP_MOPL, mopl);
	ev->mopl = sys_cpu_to_le16(mopl);
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MSE_MAS_EV_CONNECT, ev, ev_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void mse_mas_disconnect_cb(struct bt_map_mse_mas *mse_mas, struct net_buf *buf)
{
	struct mse_mas_instance *inst = CONTAINER_OF(mse_mas, struct mse_mas_instance, mse_mas);
	struct btp_map_mse_mas_disconnect_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->instance_id = inst->instance_id;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MSE_MAS_EV_DISCONNECT, ev, ev_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void mse_mas_abort_cb(struct bt_map_mse_mas *mse_mas, struct net_buf *buf)
{
	struct mse_mas_instance *inst = CONTAINER_OF(mse_mas, struct mse_mas_instance, mse_mas);
	struct btp_map_mse_mas_abort_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->instance_id = inst->instance_id;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MSE_MAS_EV_ABORT, ev, ev_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void mse_mas_set_ntf_reg_cb(struct bt_map_mse_mas *mse_mas, bool final, struct net_buf *buf)
{
	struct mse_mas_instance *inst = CONTAINER_OF(mse_mas, struct mse_mas_instance, mse_mas);
	struct btp_map_mse_mas_set_ntf_reg_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->instance_id = inst->instance_id;
	ev->final = final ? 1 : 0;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MSE_MAS_EV_SET_NTF_REG, ev, ev_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void mse_mas_set_folder_cb(struct bt_map_mse_mas *mse_mas, uint8_t flags,
				  struct net_buf *buf)
{
	struct mse_mas_instance *inst = CONTAINER_OF(mse_mas, struct mse_mas_instance, mse_mas);
	struct btp_map_mse_mas_set_folder_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->instance_id = inst->instance_id;
	ev->flags = flags;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MSE_MAS_EV_SET_FOLDER, ev, ev_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void mse_mas_get_folder_listing_cb(struct bt_map_mse_mas *mse_mas, bool final,
					  struct net_buf *buf)
{
	struct mse_mas_instance *inst = CONTAINER_OF(mse_mas, struct mse_mas_instance, mse_mas);
	struct btp_map_mse_mas_get_folder_listing_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->instance_id = inst->instance_id;
	ev->final = final ? 1 : 0;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MSE_MAS_EV_GET_FOLDER_LISTING, ev, ev_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void mse_mas_get_msg_listing_cb(struct bt_map_mse_mas *mse_mas, bool final,
				       struct net_buf *buf)
{
	struct mse_mas_instance *inst = CONTAINER_OF(mse_mas, struct mse_mas_instance, mse_mas);
	struct btp_map_mse_mas_get_msg_listing_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->instance_id = inst->instance_id;
	ev->final = final ? 1 : 0;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MSE_MAS_EV_GET_MSG_LISTING, ev, ev_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void mse_mas_get_msg_cb(struct bt_map_mse_mas *mse_mas, bool final, struct net_buf *buf)
{
	struct mse_mas_instance *inst = CONTAINER_OF(mse_mas, struct mse_mas_instance, mse_mas);
	struct btp_map_mse_mas_get_msg_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->instance_id = inst->instance_id;
	ev->final = final ? 1 : 0;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MSE_MAS_EV_GET_MSG, ev, ev_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void mse_mas_set_msg_status_cb(struct bt_map_mse_mas *mse_mas, bool final,
				      struct net_buf *buf)
{
	struct mse_mas_instance *inst = CONTAINER_OF(mse_mas, struct mse_mas_instance, mse_mas);
	struct btp_map_mse_mas_set_msg_status_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->instance_id = inst->instance_id;
	ev->final = final ? 1 : 0;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MSE_MAS_EV_SET_MSG_STATUS, ev, ev_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void mse_mas_push_msg_cb(struct bt_map_mse_mas *mse_mas, bool final, struct net_buf *buf)
{
	struct mse_mas_instance *inst = CONTAINER_OF(mse_mas, struct mse_mas_instance, mse_mas);
	struct btp_map_mse_mas_push_msg_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->instance_id = inst->instance_id;
	ev->final = final ? 1 : 0;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MSE_MAS_EV_PUSH_MSG, ev, ev_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void mse_mas_update_inbox_cb(struct bt_map_mse_mas *mse_mas, bool final, struct net_buf *buf)
{
	struct mse_mas_instance *inst = CONTAINER_OF(mse_mas, struct mse_mas_instance, mse_mas);
	struct btp_map_mse_mas_update_inbox_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->instance_id = inst->instance_id;
	ev->final = final ? 1 : 0;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MSE_MAS_EV_UPDATE_INBOX, ev, ev_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void mse_mas_get_mas_inst_info_cb(struct bt_map_mse_mas *mse_mas, bool final,
					 struct net_buf *buf)
{
	struct mse_mas_instance *inst = CONTAINER_OF(mse_mas, struct mse_mas_instance, mse_mas);
	struct btp_map_mse_mas_get_mas_inst_info_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->instance_id = inst->instance_id;
	ev->final = final ? 1 : 0;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MSE_MAS_EV_GET_MAS_INST_INFO, ev, ev_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void mse_mas_set_owner_status_cb(struct bt_map_mse_mas *mse_mas, bool final,
					struct net_buf *buf)
{
	struct mse_mas_instance *inst = CONTAINER_OF(mse_mas, struct mse_mas_instance, mse_mas);
	struct btp_map_mse_mas_set_owner_status_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->instance_id = inst->instance_id;
	ev->final = final ? 1 : 0;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MSE_MAS_EV_SET_OWNER_STATUS, ev, ev_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void mse_mas_get_owner_status_cb(struct bt_map_mse_mas *mse_mas, bool final,
					struct net_buf *buf)
{
	struct mse_mas_instance *inst = CONTAINER_OF(mse_mas, struct mse_mas_instance, mse_mas);
	struct btp_map_mse_mas_get_owner_status_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->instance_id = inst->instance_id;
	ev->final = final ? 1 : 0;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MSE_MAS_EV_GET_OWNER_STATUS, ev, ev_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void mse_mas_get_convo_listing_cb(struct bt_map_mse_mas *mse_mas, bool final,
					 struct net_buf *buf)
{
	struct mse_mas_instance *inst = CONTAINER_OF(mse_mas, struct mse_mas_instance, mse_mas);
	struct btp_map_mse_mas_get_convo_listing_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->instance_id = inst->instance_id;
	ev->final = final ? 1 : 0;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MSE_MAS_EV_GET_CONVO_LISTING, ev, ev_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void mse_mas_set_ntf_filter_cb(struct bt_map_mse_mas *mse_mas, bool final,
				      struct net_buf *buf)
{
	struct mse_mas_instance *inst = CONTAINER_OF(mse_mas, struct mse_mas_instance, mse_mas);
	struct btp_map_mse_mas_set_ntf_filter_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->instance_id = inst->instance_id;
	ev->final = final ? 1 : 0;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MSE_MAS_EV_SET_NTF_FILTER, ev, ev_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static const struct bt_map_mse_mas_cb mse_mas_cb = {
	.rfcomm_connected = mse_mas_rfcomm_connected_cb,
	.rfcomm_disconnected = mse_mas_rfcomm_disconnected_cb,
	.l2cap_connected = mse_mas_l2cap_connected_cb,
	.l2cap_disconnected = mse_mas_l2cap_disconnected_cb,
	.connect = mse_mas_connect_cb,
	.disconnect = mse_mas_disconnect_cb,
	.abort = mse_mas_abort_cb,
	.set_ntf_reg = mse_mas_set_ntf_reg_cb,
	.set_folder = mse_mas_set_folder_cb,
	.get_folder_listing = mse_mas_get_folder_listing_cb,
	.get_msg_listing = mse_mas_get_msg_listing_cb,
	.get_msg = mse_mas_get_msg_cb,
	.set_msg_status = mse_mas_set_msg_status_cb,
	.push_msg = mse_mas_push_msg_cb,
	.update_inbox = mse_mas_update_inbox_cb,
	.get_mas_inst_info = mse_mas_get_mas_inst_info_cb,
	.set_owner_status = mse_mas_set_owner_status_cb,
	.get_owner_status = mse_mas_get_owner_status_cb,
	.get_convo_listing = mse_mas_get_convo_listing_cb,
	.set_ntf_filter = mse_mas_set_ntf_filter_cb,
};

static int mse_mas_rfcomm_accept(struct bt_conn *conn, struct bt_map_mse_mas_rfcomm_server *server,
				 struct bt_map_mse_mas **mse_mas)
{
	struct mse_mas_instance *inst;
	uint8_t index;
	uint8_t conn_index;
	int err;

	for (index = 0; index < ARRAY_SIZE(mse_server); index++) {
		if (&mse_server[index].rfcomm_server == server) {
			break;
		}
	}

	if (index == ARRAY_SIZE(mse_server)) {
		LOG_ERR("Cannot find MSE MAS server");
		return -ENOMEM;
	}

	/* Check if L2CAP connection already exists */
	conn_index = bt_conn_index(conn);
	if (conn_index >= CONFIG_BT_MAX_CONN) {
		LOG_WRN("conn index %u out of range (max %u)", conn_index, CONFIG_BT_MAX_CONN);
		return -ENODEV;
	}

	for (size_t i = 0; i < ARRAY_SIZE(mse_mas_instances[conn_index]); i++) {
		inst = &mse_mas_instances[conn_index][i];

		if ((inst->conn != NULL) &&
		    (inst->psm == mse_server[index].l2cap_server.server.l2cap.psm)) {
			return -EAGAIN;
		}
	}

	inst = mse_mas_alloc(conn);
	if (inst == NULL) {
		LOG_ERR("Cannot allocate MSE MAS instance");
		return -ENOMEM;
	}

	inst->channel = server->server.rfcomm.channel;
	inst->instance_id = mse_server[index].instance_id;
	err = bt_map_mse_mas_cb_register(&inst->mse_mas, &mse_mas_cb);
	if (err != 0) {
		mse_mas_free(inst);
		LOG_ERR("Failed to register MSE MAS cb (err %d)", err);
		return err;
	}
	*mse_mas = &inst->mse_mas;
	return 0;
}

static int mse_mas_l2cap_accept(struct bt_conn *conn, struct bt_map_mse_mas_l2cap_server *server,
				struct bt_map_mse_mas **mse_mas)
{
	struct mse_mas_instance *inst;
	uint8_t index;
	uint8_t conn_index;
	int err;

	for (index = 0; index < ARRAY_SIZE(mse_server); index++) {
		if (&mse_server[index].l2cap_server == server) {
			break;
		}
	}

	if (index == ARRAY_SIZE(mse_server)) {
		LOG_ERR("Cannot find MSE MAS server");
		return -ENOMEM;
	}

	/* Check if RFCOMM connection already exists */
	conn_index = bt_conn_index(conn);
	if (conn_index >= CONFIG_BT_MAX_CONN) {
		LOG_WRN("conn index %u out of range (max %u)", conn_index, CONFIG_BT_MAX_CONN);
		return -ENODEV;
	}

	for (size_t i = 0; i < ARRAY_SIZE(mse_mas_instances[conn_index]); i++) {
		inst = &mse_mas_instances[conn_index][i];

		if ((inst->conn != NULL) &&
		    (inst->channel == mse_server[index].rfcomm_server.server.rfcomm.channel)) {
			return -EAGAIN;
		}
	}

	inst = mse_mas_alloc(conn);
	if (inst == NULL) {
		LOG_ERR("Cannot allocate MSE MAS instance");
		return -ENOMEM;
	}

	inst->psm = server->server.l2cap.psm;
	inst->instance_id = mse_server[index].instance_id;
	err = bt_map_mse_mas_cb_register(&inst->mse_mas, &mse_mas_cb);
	if (err != 0) {
		mse_mas_free(inst);
		LOG_ERR("Failed to register MSE MAS cb (err %d)", err);
		return err;
	}
	*mse_mas = &inst->mse_mas;
	return 0;
}

/* MAP Server MNS callbacks */
static void mse_mns_rfcomm_connected_cb(struct bt_conn *conn, struct bt_map_mse_mns *mse_mns)
{
	struct btp_map_mse_mns_rfcomm_connected_ev ev;

	bt_addr_copy(&ev.address.a, bt_conn_get_dst_br(conn));
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MSE_MNS_EV_RFCOMM_CONNECTED, &ev, sizeof(ev));
}

static void mse_mns_rfcomm_disconnected_cb(struct bt_map_mse_mns *mse_mns)
{
	struct mse_mns_instance *inst = CONTAINER_OF(mse_mns, struct mse_mns_instance, mse_mns);
	struct btp_map_mse_mns_rfcomm_disconnected_ev ev;

	bt_addr_copy(&ev.address.a, bt_conn_get_dst_br(inst->conn));
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MSE_MNS_EV_RFCOMM_DISCONNECTED, &ev, sizeof(ev));
	mse_mns_free(inst);
}

static void mse_mns_l2cap_connected_cb(struct bt_conn *conn, struct bt_map_mse_mns *mse_mns)
{
	struct btp_map_mse_mns_l2cap_connected_ev ev;

	bt_addr_copy(&ev.address.a, bt_conn_get_dst_br(conn));
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MSE_MNS_EV_L2CAP_CONNECTED, &ev, sizeof(ev));
}

static void mse_mns_l2cap_disconnected_cb(struct bt_map_mse_mns *mse_mns)
{
	struct mse_mns_instance *inst = CONTAINER_OF(mse_mns, struct mse_mns_instance, mse_mns);
	struct btp_map_mse_mns_l2cap_disconnected_ev ev;

	bt_addr_copy(&ev.address.a, bt_conn_get_dst_br(inst->conn));
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MSE_MNS_EV_L2CAP_DISCONNECTED, &ev, sizeof(ev));
	mse_mns_free(inst);
}

static void mse_mns_connect_cb(struct bt_map_mse_mns *mse_mns, uint8_t rsp_code, uint8_t version,
			       uint16_t mopl, struct net_buf *buf)
{
	struct mse_mns_instance *inst = CONTAINER_OF(mse_mns, struct mse_mns_instance, mse_mns);
	struct btp_map_mse_mns_connect_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->rsp_code = rsp_code;
	ev->version = version;
	mopl = MIN(MAP_MOPL, mopl);
	ev->mopl = sys_cpu_to_le16(mopl);
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MSE_MNS_EV_CONNECT, ev, ev_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void mse_mns_disconnect_cb(struct bt_map_mse_mns *mse_mns, uint8_t rsp_code,
				  struct net_buf *buf)
{
	struct mse_mns_instance *inst = CONTAINER_OF(mse_mns, struct mse_mns_instance, mse_mns);
	struct btp_map_mse_mns_disconnect_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->rsp_code = rsp_code;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MSE_MNS_EV_DISCONNECT, ev, ev_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void mse_mns_abort_cb(struct bt_map_mse_mns *mse_mns, uint8_t rsp_code, struct net_buf *buf)
{
	struct mse_mns_instance *inst = CONTAINER_OF(mse_mns, struct mse_mns_instance, mse_mns);
	struct btp_map_mse_mns_abort_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->rsp_code = rsp_code;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MSE_MNS_EV_ABORT, ev, ev_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void mse_mns_send_event_cb(struct bt_map_mse_mns *mse_mns, uint8_t rsp_code,
				  struct net_buf *buf)
{
	struct mse_mns_instance *inst = CONTAINER_OF(mse_mns, struct mse_mns_instance, mse_mns);
	struct btp_map_mse_mns_send_event_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->rsp_code = rsp_code;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_MAP, BTP_MAP_MSE_MNS_EV_SEND_EVENT, ev, ev_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static const struct bt_map_mse_mns_cb mse_mns_cb = {
	.rfcomm_connected = mse_mns_rfcomm_connected_cb,
	.rfcomm_disconnected = mse_mns_rfcomm_disconnected_cb,
	.l2cap_connected = mse_mns_l2cap_connected_cb,
	.l2cap_disconnected = mse_mns_l2cap_disconnected_cb,
	.connect = mse_mns_connect_cb,
	.disconnect = mse_mns_disconnect_cb,
	.abort = mse_mns_abort_cb,
	.send_event = mse_mns_send_event_cb,
};

/* BTP command handlers - MAP Client MAS */
static uint8_t mce_mas_rfcomm_connect(const void *cmd, uint16_t cmd_len, void *rsp,
				      uint16_t *rsp_len)
{
	const struct btp_map_mce_mas_rfcomm_connect_cmd *cp = cmd;
	struct mce_mas_instance *inst;
	struct bt_conn *conn;

	if (cp->address.type != BTP_BR_ADDRESS_TYPE) {
		return BTP_STATUS_FAILED;
	}

	conn = bt_conn_lookup_addr_br(&cp->address.a);
	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	inst = mce_mas_alloc(conn);
	if (inst == NULL) {
		bt_conn_unref(conn);
		return BTP_STATUS_FAILED;
	}

	inst->instance_id = cp->instance_id;

	if (bt_map_mce_mas_cb_register(&inst->mce_mas, &mce_mas_cb) < 0) {
		mce_mas_free(inst);
		bt_conn_unref(conn);
		return BTP_STATUS_FAILED;
	}

	if (bt_map_mce_mas_rfcomm_connect(conn, &inst->mce_mas, cp->channel) < 0) {
		mce_mas_free(inst);
		bt_conn_unref(conn);
		return BTP_STATUS_FAILED;
	}

	bt_conn_unref(conn);
	return BTP_STATUS_SUCCESS;
}

static uint8_t mce_mas_rfcomm_disconnect(const void *cmd, uint16_t cmd_len, void *rsp,
					 uint16_t *rsp_len)
{
	const struct btp_map_mce_mas_rfcomm_disconnect_cmd *cp = cmd;
	struct mce_mas_instance *inst;

	inst = mce_mas_find(&cp->address, cp->instance_id);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	if (bt_map_mce_mas_rfcomm_disconnect(&inst->mce_mas) < 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mce_mas_l2cap_connect(const void *cmd, uint16_t cmd_len, void *rsp,
				     uint16_t *rsp_len)
{
	const struct btp_map_mce_mas_l2cap_connect_cmd *cp = cmd;
	struct mce_mas_instance *inst;
	struct bt_conn *conn;

	if (cp->address.type != BTP_BR_ADDRESS_TYPE) {
		return BTP_STATUS_FAILED;
	}

	conn = bt_conn_lookup_addr_br(&cp->address.a);
	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	inst = mce_mas_alloc(conn);
	if (inst == NULL) {
		bt_conn_unref(conn);
		return BTP_STATUS_FAILED;
	}

	inst->instance_id = cp->instance_id;

	if (bt_map_mce_mas_cb_register(&inst->mce_mas, &mce_mas_cb) < 0) {
		mce_mas_free(inst);
		bt_conn_unref(conn);
		return BTP_STATUS_FAILED;
	}

	if (bt_map_mce_mas_l2cap_connect(conn, &inst->mce_mas, sys_le16_to_cpu(cp->psm)) < 0) {
		mce_mas_free(inst);
		bt_conn_unref(conn);
		return BTP_STATUS_FAILED;
	}

	bt_conn_unref(conn);
	return BTP_STATUS_SUCCESS;
}

static uint8_t mce_mas_l2cap_disconnect(const void *cmd, uint16_t cmd_len, void *rsp,
					uint16_t *rsp_len)
{
	const struct btp_map_mce_mas_l2cap_disconnect_cmd *cp = cmd;
	struct mce_mas_instance *inst;

	inst = mce_mas_find(&cp->address, cp->instance_id);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	if (bt_map_mce_mas_l2cap_disconnect(&inst->mce_mas) < 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mce_mas_connect(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_map_mce_mas_connect_cmd *cp = cmd;
	struct mce_mas_instance *inst;
	struct net_buf *buf = NULL;
	const struct bt_uuid_128 *uuid = BT_MAP_UUID_MAS;
	uint8_t val[BT_UUID_SIZE_128];
	int err;

	inst = mce_mas_find(&cp->address, cp->instance_id);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_map_mce_mas_create_pdu(&inst->mce_mas, &tx_pool);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	sys_memcpy_swap(val, uuid->val, sizeof(val));
	err = bt_obex_add_header_target(buf, sizeof(val), val);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	/* Add supported features as application parameter */
	if (cp->send_supp_feat != 0U) {
		uint32_t supported_features = sys_cpu_to_be32(MAP_MCE_SUPPORTED_FEATURES);
		struct bt_obex_tlv appl_params[] = {
			{BT_MAP_APPL_PARAM_TAG_ID_MAP_SUPPORTED_FEATURES,
			 sizeof(supported_features), (const uint8_t *)&supported_features},
		};

		err = bt_obex_add_header_app_param(buf, ARRAY_SIZE(appl_params), appl_params);
		if (err != 0) {
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
	}

	if (bt_map_mce_mas_connect(&inst->mce_mas, buf) < 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mce_mas_disconnect(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_map_mce_mas_disconnect_cmd *cp = cmd;
	struct mce_mas_instance *inst;

	inst = mce_mas_find(&cp->address, cp->instance_id);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	if (bt_map_mce_mas_disconnect(&inst->mce_mas, NULL) < 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mce_mas_abort(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_map_mce_mas_abort_cmd *cp = cmd;
	struct mce_mas_instance *inst;

	inst = mce_mas_find(&cp->address, cp->instance_id);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	if (bt_map_mce_mas_abort(&inst->mce_mas, NULL) < 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mce_mas_set_folder(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_map_mce_mas_set_folder_cmd *cp = cmd;
	struct mce_mas_instance *inst;
	struct net_buf *buf = NULL;
	uint16_t buf_len;

	if (cmd_len < sizeof(*cp)) {
		return BTP_STATUS_FAILED;
	}

	buf_len = sys_le16_to_cpu(cp->buf_len);
	if (cmd_len != sizeof(*cp) + buf_len) {
		return BTP_STATUS_FAILED;
	}

	inst = mce_mas_find(&cp->address, cp->instance_id);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_map_mce_mas_create_pdu(&inst->mce_mas, &tx_pool);
	if (!buf) {
		return BTP_STATUS_FAILED;
	}

	if (buf_len > 0) {
		if (net_buf_tailroom(buf) < buf_len) {
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
		net_buf_add_mem(buf, cp->buf, buf_len);
	}

	if (bt_map_mce_mas_set_folder(&inst->mce_mas, cp->flags, buf) < 0) {
		if (buf) {
			net_buf_unref(buf);
		}
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mce_mas_set_ntf_reg(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_map_mce_mas_set_ntf_reg_cmd *cp = cmd;
	struct mce_mas_instance *inst;
	struct net_buf *buf = NULL;
	uint16_t buf_len;

	if (cmd_len < sizeof(*cp)) {
		return BTP_STATUS_FAILED;
	}

	buf_len = sys_le16_to_cpu(cp->buf_len);
	if (cmd_len != sizeof(*cp) + buf_len) {
		return BTP_STATUS_FAILED;
	}

	inst = mce_mas_find(&cp->address, cp->instance_id);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_map_mce_mas_create_pdu(&inst->mce_mas, &tx_pool);
	if (!buf) {
		return BTP_STATUS_FAILED;
	}

	if (buf_len > 0) {
		if (net_buf_tailroom(buf) < buf_len) {
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
		net_buf_add_mem(buf, cp->buf, buf_len);
	}

	if (bt_map_mce_mas_set_ntf_reg(&inst->mce_mas, cp->final, buf) < 0) {
		if (buf) {
			net_buf_unref(buf);
		}
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mce_mas_get_folder_listing(const void *cmd, uint16_t cmd_len, void *rsp,
					  uint16_t *rsp_len)
{
	const struct btp_map_mce_mas_get_folder_listing_cmd *cp = cmd;
	struct mce_mas_instance *inst;
	struct net_buf *buf = NULL;
	uint16_t buf_len;

	if (cmd_len < sizeof(*cp)) {
		return BTP_STATUS_FAILED;
	}

	buf_len = sys_le16_to_cpu(cp->buf_len);
	if (cmd_len != sizeof(*cp) + buf_len) {
		return BTP_STATUS_FAILED;
	}

	inst = mce_mas_find(&cp->address, cp->instance_id);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_map_mce_mas_create_pdu(&inst->mce_mas, &tx_pool);
	if (!buf) {
		return BTP_STATUS_FAILED;
	}

	if (buf_len > 0) {
		if (net_buf_tailroom(buf) < buf_len) {
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
		net_buf_add_mem(buf, cp->buf, buf_len);
	}

	if (bt_map_mce_mas_get_folder_listing(&inst->mce_mas, cp->final, buf) < 0) {
		if (buf) {
			net_buf_unref(buf);
		}
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mce_mas_get_msg_listing(const void *cmd, uint16_t cmd_len, void *rsp,
				       uint16_t *rsp_len)
{
	const struct btp_map_mce_mas_get_msg_listing_cmd *cp = cmd;
	struct mce_mas_instance *inst;
	struct net_buf *buf = NULL;
	uint16_t buf_len;

	if (cmd_len < sizeof(*cp)) {
		return BTP_STATUS_FAILED;
	}

	buf_len = sys_le16_to_cpu(cp->buf_len);
	if (cmd_len != sizeof(*cp) + buf_len) {
		return BTP_STATUS_FAILED;
	}

	inst = mce_mas_find(&cp->address, cp->instance_id);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_map_mce_mas_create_pdu(&inst->mce_mas, &tx_pool);
	if (!buf) {
		return BTP_STATUS_FAILED;
	}

	if (buf_len > 0) {
		if (net_buf_tailroom(buf) < buf_len) {
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
		net_buf_add_mem(buf, cp->buf, buf_len);
	}

	if (bt_map_mce_mas_get_msg_listing(&inst->mce_mas, cp->final, buf) < 0) {
		if (buf) {
			net_buf_unref(buf);
		}
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mce_mas_get_msg(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_map_mce_mas_get_msg_cmd *cp = cmd;
	struct mce_mas_instance *inst;
	struct net_buf *buf = NULL;
	uint16_t buf_len;

	if (cmd_len < sizeof(*cp)) {
		return BTP_STATUS_FAILED;
	}

	buf_len = sys_le16_to_cpu(cp->buf_len);
	if (cmd_len != sizeof(*cp) + buf_len) {
		return BTP_STATUS_FAILED;
	}

	inst = mce_mas_find(&cp->address, cp->instance_id);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_map_mce_mas_create_pdu(&inst->mce_mas, &tx_pool);
	if (!buf) {
		return BTP_STATUS_FAILED;
	}

	if (buf_len > 0) {
		if (net_buf_tailroom(buf) < buf_len) {
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
		net_buf_add_mem(buf, cp->buf, buf_len);
	}

	if (bt_map_mce_mas_get_msg(&inst->mce_mas, cp->final, buf) < 0) {
		if (buf) {
			net_buf_unref(buf);
		}
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mce_mas_set_msg_status(const void *cmd, uint16_t cmd_len, void *rsp,
				      uint16_t *rsp_len)
{
	const struct btp_map_mce_mas_set_msg_status_cmd *cp = cmd;
	struct mce_mas_instance *inst;
	struct net_buf *buf = NULL;
	uint16_t buf_len;

	if (cmd_len < sizeof(*cp)) {
		return BTP_STATUS_FAILED;
	}

	buf_len = sys_le16_to_cpu(cp->buf_len);
	if (cmd_len != sizeof(*cp) + buf_len) {
		return BTP_STATUS_FAILED;
	}

	inst = mce_mas_find(&cp->address, cp->instance_id);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_map_mce_mas_create_pdu(&inst->mce_mas, &tx_pool);
	if (!buf) {
		return BTP_STATUS_FAILED;
	}

	if (buf_len > 0) {
		if (net_buf_tailroom(buf) < buf_len) {
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
		net_buf_add_mem(buf, cp->buf, buf_len);
	}

	if (bt_map_mce_mas_set_msg_status(&inst->mce_mas, cp->final, buf) < 0) {
		if (buf) {
			net_buf_unref(buf);
		}
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mce_mas_push_msg(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_map_mce_mas_push_msg_cmd *cp = cmd;
	struct mce_mas_instance *inst;
	struct net_buf *buf = NULL;
	uint16_t buf_len;

	if (cmd_len < sizeof(*cp)) {
		return BTP_STATUS_FAILED;
	}

	buf_len = sys_le16_to_cpu(cp->buf_len);
	if (cmd_len != sizeof(*cp) + buf_len) {
		return BTP_STATUS_FAILED;
	}

	inst = mce_mas_find(&cp->address, cp->instance_id);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_map_mce_mas_create_pdu(&inst->mce_mas, &tx_pool);
	if (!buf) {
		return BTP_STATUS_FAILED;
	}

	if (buf_len > 0) {
		if (net_buf_tailroom(buf) < buf_len) {
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
		net_buf_add_mem(buf, cp->buf, buf_len);
	}

	if (bt_map_mce_mas_push_msg(&inst->mce_mas, cp->final, buf) < 0) {
		if (buf) {
			net_buf_unref(buf);
		}
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mce_mas_update_inbox(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_map_mce_mas_update_inbox_cmd *cp = cmd;
	struct mce_mas_instance *inst;
	struct net_buf *buf = NULL;
	uint16_t buf_len;

	if (cmd_len < sizeof(*cp)) {
		return BTP_STATUS_FAILED;
	}

	buf_len = sys_le16_to_cpu(cp->buf_len);
	if (cmd_len != sizeof(*cp) + buf_len) {
		return BTP_STATUS_FAILED;
	}

	inst = mce_mas_find(&cp->address, cp->instance_id);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_map_mce_mas_create_pdu(&inst->mce_mas, &tx_pool);
	if (!buf) {
		return BTP_STATUS_FAILED;
	}

	if (buf_len > 0) {
		if (net_buf_tailroom(buf) < buf_len) {
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
		net_buf_add_mem(buf, cp->buf, buf_len);
	}

	if (bt_map_mce_mas_update_inbox(&inst->mce_mas, cp->final, buf) < 0) {
		if (buf) {
			net_buf_unref(buf);
		}
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mce_mas_get_mas_inst_info(const void *cmd, uint16_t cmd_len, void *rsp,
					 uint16_t *rsp_len)
{
	const struct btp_map_mce_mas_get_mas_inst_info_cmd *cp = cmd;
	struct mce_mas_instance *inst;
	struct net_buf *buf = NULL;
	uint16_t buf_len;

	if (cmd_len < sizeof(*cp)) {
		return BTP_STATUS_FAILED;
	}

	buf_len = sys_le16_to_cpu(cp->buf_len);
	if (cmd_len != sizeof(*cp) + buf_len) {
		return BTP_STATUS_FAILED;
	}

	inst = mce_mas_find(&cp->address, cp->instance_id);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_map_mce_mas_create_pdu(&inst->mce_mas, &tx_pool);
	if (!buf) {
		return BTP_STATUS_FAILED;
	}

	if (buf_len > 0) {
		if (net_buf_tailroom(buf) < buf_len) {
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
		net_buf_add_mem(buf, cp->buf, buf_len);
	}

	if (bt_map_mce_mas_get_mas_inst_info(&inst->mce_mas, cp->final, buf) < 0) {
		if (buf) {
			net_buf_unref(buf);
		}
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mce_mas_set_owner_status(const void *cmd, uint16_t cmd_len, void *rsp,
					uint16_t *rsp_len)
{
	const struct btp_map_mce_mas_set_owner_status_cmd *cp = cmd;
	struct mce_mas_instance *inst;
	struct net_buf *buf = NULL;
	uint16_t buf_len;

	if (cmd_len < sizeof(*cp)) {
		return BTP_STATUS_FAILED;
	}

	buf_len = sys_le16_to_cpu(cp->buf_len);
	if (cmd_len != sizeof(*cp) + buf_len) {
		return BTP_STATUS_FAILED;
	}

	inst = mce_mas_find(&cp->address, cp->instance_id);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_map_mce_mas_create_pdu(&inst->mce_mas, &tx_pool);
	if (!buf) {
		return BTP_STATUS_FAILED;
	}

	if (buf_len > 0) {
		if (net_buf_tailroom(buf) < buf_len) {
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
		net_buf_add_mem(buf, cp->buf, buf_len);
	}

	if (bt_map_mce_mas_set_owner_status(&inst->mce_mas, cp->final, buf) < 0) {
		if (buf) {
			net_buf_unref(buf);
		}
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mce_mas_get_owner_status(const void *cmd, uint16_t cmd_len, void *rsp,
					uint16_t *rsp_len)
{
	const struct btp_map_mce_mas_get_owner_status_cmd *cp = cmd;
	struct mce_mas_instance *inst;
	struct net_buf *buf = NULL;
	uint16_t buf_len;

	if (cmd_len < sizeof(*cp)) {
		return BTP_STATUS_FAILED;
	}

	buf_len = sys_le16_to_cpu(cp->buf_len);
	if (cmd_len != sizeof(*cp) + buf_len) {
		return BTP_STATUS_FAILED;
	}

	inst = mce_mas_find(&cp->address, cp->instance_id);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_map_mce_mas_create_pdu(&inst->mce_mas, &tx_pool);
	if (!buf) {
		return BTP_STATUS_FAILED;
	}

	if (buf_len > 0) {
		if (net_buf_tailroom(buf) < buf_len) {
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
		net_buf_add_mem(buf, cp->buf, buf_len);
	}

	if (bt_map_mce_mas_get_owner_status(&inst->mce_mas, cp->final, buf) < 0) {
		if (buf) {
			net_buf_unref(buf);
		}
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mce_mas_get_convo_listing(const void *cmd, uint16_t cmd_len, void *rsp,
					 uint16_t *rsp_len)
{
	const struct btp_map_mce_mas_get_convo_listing_cmd *cp = cmd;
	struct mce_mas_instance *inst;
	struct net_buf *buf = NULL;
	uint16_t buf_len;

	if (cmd_len < sizeof(*cp)) {
		return BTP_STATUS_FAILED;
	}

	buf_len = sys_le16_to_cpu(cp->buf_len);
	if (cmd_len != sizeof(*cp) + buf_len) {
		return BTP_STATUS_FAILED;
	}

	inst = mce_mas_find(&cp->address, cp->instance_id);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_map_mce_mas_create_pdu(&inst->mce_mas, &tx_pool);
	if (!buf) {
		return BTP_STATUS_FAILED;
	}

	if (buf_len > 0) {
		if (net_buf_tailroom(buf) < buf_len) {
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
		net_buf_add_mem(buf, cp->buf, buf_len);
	}

	if (bt_map_mce_mas_get_convo_listing(&inst->mce_mas, cp->final, buf) < 0) {
		if (buf) {
			net_buf_unref(buf);
		}
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mce_mas_set_ntf_filter(const void *cmd, uint16_t cmd_len, void *rsp,
				      uint16_t *rsp_len)
{
	const struct btp_map_mce_mas_set_ntf_filter_cmd *cp = cmd;
	struct mce_mas_instance *inst;
	struct net_buf *buf = NULL;
	uint16_t buf_len;

	if (cmd_len < sizeof(*cp)) {
		return BTP_STATUS_FAILED;
	}

	buf_len = sys_le16_to_cpu(cp->buf_len);
	if (cmd_len != sizeof(*cp) + buf_len) {
		return BTP_STATUS_FAILED;
	}

	inst = mce_mas_find(&cp->address, cp->instance_id);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_map_mce_mas_create_pdu(&inst->mce_mas, &tx_pool);
	if (!buf) {
		return BTP_STATUS_FAILED;
	}

	if (buf_len > 0) {
		if (net_buf_tailroom(buf) < buf_len) {
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
		net_buf_add_mem(buf, cp->buf, buf_len);
	}

	if (bt_map_mce_mas_set_ntf_filter(&inst->mce_mas, cp->final, buf) < 0) {
		if (buf) {
			net_buf_unref(buf);
		}
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

/* BTP command handlers - MAP Client MNS */
static uint8_t mce_mns_rfcomm_disconnect(const void *cmd, uint16_t cmd_len, void *rsp,
					 uint16_t *rsp_len)
{
	const struct btp_map_mce_mns_rfcomm_disconnect_cmd *cp = cmd;
	struct mce_mns_instance *inst;

	inst = mce_mns_find(&cp->address);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	if (bt_map_mce_mns_rfcomm_disconnect(&inst->mce_mns) < 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mce_mns_l2cap_disconnect(const void *cmd, uint16_t cmd_len, void *rsp,
					uint16_t *rsp_len)
{
	const struct btp_map_mce_mns_l2cap_disconnect_cmd *cp = cmd;
	struct mce_mns_instance *inst;

	inst = mce_mns_find(&cp->address);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	if (bt_map_mce_mns_l2cap_disconnect(&inst->mce_mns) < 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mce_mns_connect(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_map_mce_mns_connect_cmd *cp = cmd;
	struct mce_mns_instance *inst;

	inst = mce_mns_find(&cp->address);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	if (bt_map_mce_mns_connect(&inst->mce_mns, cp->rsp_code, NULL) < 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mce_mns_disconnect(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_map_mce_mns_disconnect_cmd *cp = cmd;
	struct mce_mns_instance *inst;

	inst = mce_mns_find(&cp->address);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	if (bt_map_mce_mns_disconnect(&inst->mce_mns, cp->rsp_code, NULL) < 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mce_mns_abort(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_map_mce_mns_abort_cmd *cp = cmd;
	struct mce_mns_instance *inst;

	inst = mce_mns_find(&cp->address);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	if (bt_map_mce_mns_abort(&inst->mce_mns, cp->rsp_code, NULL) < 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mce_mns_send_event(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_map_mce_mns_send_event_cmd *cp = cmd;
	struct mce_mns_instance *inst;
	struct net_buf *buf = NULL;
	uint16_t buf_len;

	if (cmd_len < sizeof(*cp)) {
		return BTP_STATUS_FAILED;
	}

	buf_len = sys_le16_to_cpu(cp->buf_len);
	if (cmd_len != sizeof(*cp) + buf_len) {
		return BTP_STATUS_FAILED;
	}

	inst = mce_mns_find(&cp->address);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_map_mce_mns_create_pdu(&inst->mce_mns, &tx_pool);
	if (!buf) {
		return BTP_STATUS_FAILED;
	}

	if (buf_len > 0) {
		if (net_buf_tailroom(buf) < buf_len) {
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
		net_buf_add_mem(buf, cp->buf, buf_len);
	}

	if (bt_map_mce_mns_send_event(&inst->mce_mns, cp->rsp_code, buf) < 0) {
		if (buf) {
			net_buf_unref(buf);
		}
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

/* BTP command handlers - MAP Server MAS */
static uint8_t mse_mas_rfcomm_disconnect(const void *cmd, uint16_t cmd_len, void *rsp,
					 uint16_t *rsp_len)
{
	const struct btp_map_mse_mas_rfcomm_disconnect_cmd *cp = cmd;
	struct mse_mas_instance *inst;

	inst = mse_mas_find(&cp->address, cp->instance_id);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	if (bt_map_mse_mas_rfcomm_disconnect(&inst->mse_mas) < 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mse_mas_l2cap_disconnect(const void *cmd, uint16_t cmd_len, void *rsp,
					uint16_t *rsp_len)
{
	const struct btp_map_mse_mas_l2cap_disconnect_cmd *cp = cmd;
	struct mse_mas_instance *inst;

	inst = mse_mas_find(&cp->address, cp->instance_id);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	if (bt_map_mse_mas_l2cap_disconnect(&inst->mse_mas) < 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mse_mas_connect(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_map_mse_mas_connect_cmd *cp = cmd;
	struct mse_mas_instance *inst;

	inst = mse_mas_find(&cp->address, cp->instance_id);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	if (bt_map_mse_mas_connect(&inst->mse_mas, cp->rsp_code, NULL) < 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mse_mas_disconnect(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_map_mse_mas_disconnect_cmd *cp = cmd;
	struct mse_mas_instance *inst;

	inst = mse_mas_find(&cp->address, cp->instance_id);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	if (bt_map_mse_mas_disconnect(&inst->mse_mas, cp->rsp_code, NULL) < 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mse_mas_abort(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_map_mse_mas_abort_cmd *cp = cmd;
	struct mse_mas_instance *inst;

	inst = mse_mas_find(&cp->address, cp->instance_id);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	if (bt_map_mse_mas_abort(&inst->mse_mas, cp->rsp_code, NULL) < 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mse_mas_set_folder(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_map_mse_mas_set_folder_cmd *cp = cmd;
	struct mse_mas_instance *inst;
	struct net_buf *buf = NULL;
	uint16_t buf_len;

	if (cmd_len < sizeof(*cp)) {
		return BTP_STATUS_FAILED;
	}

	buf_len = sys_le16_to_cpu(cp->buf_len);
	if (cmd_len != sizeof(*cp) + buf_len) {
		return BTP_STATUS_FAILED;
	}

	inst = mse_mas_find(&cp->address, cp->instance_id);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_map_mse_mas_create_pdu(&inst->mse_mas, &tx_pool);
	if (!buf) {
		return BTP_STATUS_FAILED;
	}

	if (buf_len > 0) {
		if (net_buf_tailroom(buf) < buf_len) {
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
		net_buf_add_mem(buf, cp->buf, buf_len);
	}

	if (bt_map_mse_mas_set_folder(&inst->mse_mas, cp->rsp_code, buf) < 0) {
		if (buf) {
			net_buf_unref(buf);
		}
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mse_mas_set_ntf_reg(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_map_mse_mas_set_ntf_reg_cmd *cp = cmd;
	struct mse_mas_instance *inst;
	struct net_buf *buf = NULL;
	uint16_t buf_len;

	if (cmd_len < sizeof(*cp)) {
		return BTP_STATUS_FAILED;
	}

	buf_len = sys_le16_to_cpu(cp->buf_len);
	if (cmd_len != sizeof(*cp) + buf_len) {
		return BTP_STATUS_FAILED;
	}

	inst = mse_mas_find(&cp->address, cp->instance_id);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_map_mse_mas_create_pdu(&inst->mse_mas, &tx_pool);
	if (!buf) {
		return BTP_STATUS_FAILED;
	}

	if (buf_len > 0) {
		if (net_buf_tailroom(buf) < buf_len) {
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
		net_buf_add_mem(buf, cp->buf, buf_len);
	}

	if (bt_map_mse_mas_set_ntf_reg(&inst->mse_mas, cp->rsp_code, buf) < 0) {
		if (buf) {
			net_buf_unref(buf);
		}
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mse_mas_get_folder_listing(const void *cmd, uint16_t cmd_len, void *rsp,
					  uint16_t *rsp_len)
{
	const struct btp_map_mse_mas_get_folder_listing_cmd *cp = cmd;
	struct mse_mas_instance *inst;
	struct net_buf *buf = NULL;
	uint16_t buf_len;

	if (cmd_len < sizeof(*cp)) {
		return BTP_STATUS_FAILED;
	}

	buf_len = sys_le16_to_cpu(cp->buf_len);
	if (cmd_len != sizeof(*cp) + buf_len) {
		return BTP_STATUS_FAILED;
	}

	inst = mse_mas_find(&cp->address, cp->instance_id);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_map_mse_mas_create_pdu(&inst->mse_mas, &tx_pool);
	if (!buf) {
		return BTP_STATUS_FAILED;
	}

	if (buf_len > 0) {
		if (net_buf_tailroom(buf) < buf_len) {
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
		net_buf_add_mem(buf, cp->buf, buf_len);
	}

	if (bt_map_mse_mas_get_folder_listing(&inst->mse_mas, cp->rsp_code, buf) < 0) {
		if (buf) {
			net_buf_unref(buf);
		}
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mse_mas_get_msg_listing(const void *cmd, uint16_t cmd_len, void *rsp,
				       uint16_t *rsp_len)
{
	const struct btp_map_mse_mas_get_msg_listing_cmd *cp = cmd;
	struct mse_mas_instance *inst;
	struct net_buf *buf = NULL;
	uint16_t buf_len;

	if (cmd_len < sizeof(*cp)) {
		return BTP_STATUS_FAILED;
	}

	buf_len = sys_le16_to_cpu(cp->buf_len);
	if (cmd_len != sizeof(*cp) + buf_len) {
		return BTP_STATUS_FAILED;
	}

	inst = mse_mas_find(&cp->address, cp->instance_id);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_map_mse_mas_create_pdu(&inst->mse_mas, &tx_pool);
	if (!buf) {
		return BTP_STATUS_FAILED;
	}

	if (buf_len > 0) {
		if (net_buf_tailroom(buf) < buf_len) {
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
		net_buf_add_mem(buf, cp->buf, buf_len);
	}

	if (bt_map_mse_mas_get_msg_listing(&inst->mse_mas, cp->rsp_code, buf) < 0) {
		if (buf) {
			net_buf_unref(buf);
		}
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mse_mas_get_msg(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_map_mse_mas_get_msg_cmd *cp = cmd;
	struct mse_mas_instance *inst;
	struct net_buf *buf = NULL;
	uint16_t buf_len;

	if (cmd_len < sizeof(*cp)) {
		return BTP_STATUS_FAILED;
	}

	buf_len = sys_le16_to_cpu(cp->buf_len);
	if (cmd_len != sizeof(*cp) + buf_len) {
		return BTP_STATUS_FAILED;
	}

	inst = mse_mas_find(&cp->address, cp->instance_id);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_map_mse_mas_create_pdu(&inst->mse_mas, &tx_pool);
	if (!buf) {
		return BTP_STATUS_FAILED;
	}

	if (buf_len > 0) {
		if (net_buf_tailroom(buf) < buf_len) {
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
		net_buf_add_mem(buf, cp->buf, buf_len);
	}

	if (bt_map_mse_mas_get_msg(&inst->mse_mas, cp->rsp_code, buf) < 0) {
		if (buf) {
			net_buf_unref(buf);
		}
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mse_mas_set_msg_status(const void *cmd, uint16_t cmd_len, void *rsp,
				      uint16_t *rsp_len)
{
	const struct btp_map_mse_mas_set_msg_status_cmd *cp = cmd;
	struct mse_mas_instance *inst;
	struct net_buf *buf = NULL;
	uint16_t buf_len;

	if (cmd_len < sizeof(*cp)) {
		return BTP_STATUS_FAILED;
	}

	buf_len = sys_le16_to_cpu(cp->buf_len);
	if (cmd_len != sizeof(*cp) + buf_len) {
		return BTP_STATUS_FAILED;
	}

	inst = mse_mas_find(&cp->address, cp->instance_id);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_map_mse_mas_create_pdu(&inst->mse_mas, &tx_pool);
	if (!buf) {
		return BTP_STATUS_FAILED;
	}

	if (buf_len > 0) {
		if (net_buf_tailroom(buf) < buf_len) {
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
		net_buf_add_mem(buf, cp->buf, buf_len);
	}

	if (bt_map_mse_mas_set_msg_status(&inst->mse_mas, cp->rsp_code, buf) < 0) {
		if (buf) {
			net_buf_unref(buf);
		}
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mse_mas_push_msg(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_map_mse_mas_push_msg_cmd *cp = cmd;
	struct mse_mas_instance *inst;
	struct net_buf *buf = NULL;
	uint16_t buf_len;

	if (cmd_len < sizeof(*cp)) {
		return BTP_STATUS_FAILED;
	}

	buf_len = sys_le16_to_cpu(cp->buf_len);
	if (cmd_len != sizeof(*cp) + buf_len) {
		return BTP_STATUS_FAILED;
	}

	inst = mse_mas_find(&cp->address, cp->instance_id);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_map_mse_mas_create_pdu(&inst->mse_mas, &tx_pool);
	if (!buf) {
		return BTP_STATUS_FAILED;
	}

	if (buf_len > 0) {
		if (net_buf_tailroom(buf) < buf_len) {
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
		net_buf_add_mem(buf, cp->buf, buf_len);
	}

	if (bt_map_mse_mas_push_msg(&inst->mse_mas, cp->rsp_code, buf) < 0) {
		if (buf) {
			net_buf_unref(buf);
		}
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mse_mas_update_inbox(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_map_mse_mas_update_inbox_cmd *cp = cmd;
	struct mse_mas_instance *inst;
	struct net_buf *buf = NULL;
	uint16_t buf_len;

	if (cmd_len < sizeof(*cp)) {
		return BTP_STATUS_FAILED;
	}

	buf_len = sys_le16_to_cpu(cp->buf_len);
	if (cmd_len != sizeof(*cp) + buf_len) {
		return BTP_STATUS_FAILED;
	}

	inst = mse_mas_find(&cp->address, cp->instance_id);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_map_mse_mas_create_pdu(&inst->mse_mas, &tx_pool);
	if (!buf) {
		return BTP_STATUS_FAILED;
	}

	if (buf_len > 0) {
		if (net_buf_tailroom(buf) < buf_len) {
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
		net_buf_add_mem(buf, cp->buf, buf_len);
	}

	if (bt_map_mse_mas_update_inbox(&inst->mse_mas, cp->rsp_code, buf) < 0) {
		if (buf) {
			net_buf_unref(buf);
		}
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mse_mas_get_mas_inst_info(const void *cmd, uint16_t cmd_len, void *rsp,
					 uint16_t *rsp_len)
{
	const struct btp_map_mse_mas_get_mas_inst_info_cmd *cp = cmd;
	struct mse_mas_instance *inst;
	struct net_buf *buf = NULL;
	uint16_t buf_len;

	if (cmd_len < sizeof(*cp)) {
		return BTP_STATUS_FAILED;
	}

	buf_len = sys_le16_to_cpu(cp->buf_len);
	if (cmd_len != sizeof(*cp) + buf_len) {
		return BTP_STATUS_FAILED;
	}

	inst = mse_mas_find(&cp->address, cp->instance_id);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_map_mse_mas_create_pdu(&inst->mse_mas, &tx_pool);
	if (!buf) {
		return BTP_STATUS_FAILED;
	}

	if (buf_len > 0) {
		if (net_buf_tailroom(buf) < buf_len) {
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
		net_buf_add_mem(buf, cp->buf, buf_len);
	}

	if (bt_map_mse_mas_get_mas_inst_info(&inst->mse_mas, cp->rsp_code, buf) < 0) {
		if (buf) {
			net_buf_unref(buf);
		}
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mse_mas_set_owner_status(const void *cmd, uint16_t cmd_len, void *rsp,
					uint16_t *rsp_len)
{
	const struct btp_map_mse_mas_set_owner_status_cmd *cp = cmd;
	struct mse_mas_instance *inst;
	struct net_buf *buf = NULL;
	uint16_t buf_len;

	if (cmd_len < sizeof(*cp)) {
		return BTP_STATUS_FAILED;
	}

	buf_len = sys_le16_to_cpu(cp->buf_len);
	if (cmd_len != sizeof(*cp) + buf_len) {
		return BTP_STATUS_FAILED;
	}

	inst = mse_mas_find(&cp->address, cp->instance_id);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_map_mse_mas_create_pdu(&inst->mse_mas, &tx_pool);
	if (!buf) {
		return BTP_STATUS_FAILED;
	}

	if (buf_len > 0) {
		if (net_buf_tailroom(buf) < buf_len) {
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
		net_buf_add_mem(buf, cp->buf, buf_len);
	}

	if (bt_map_mse_mas_set_owner_status(&inst->mse_mas, cp->rsp_code, buf) < 0) {
		if (buf) {
			net_buf_unref(buf);
		}
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mse_mas_get_owner_status(const void *cmd, uint16_t cmd_len, void *rsp,
					uint16_t *rsp_len)
{
	const struct btp_map_mse_mas_get_owner_status_cmd *cp = cmd;
	struct mse_mas_instance *inst;
	struct net_buf *buf = NULL;
	uint16_t buf_len;

	if (cmd_len < sizeof(*cp)) {
		return BTP_STATUS_FAILED;
	}

	buf_len = sys_le16_to_cpu(cp->buf_len);
	if (cmd_len != sizeof(*cp) + buf_len) {
		return BTP_STATUS_FAILED;
	}

	inst = mse_mas_find(&cp->address, cp->instance_id);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_map_mse_mas_create_pdu(&inst->mse_mas, &tx_pool);
	if (!buf) {
		return BTP_STATUS_FAILED;
	}

	if (buf_len > 0) {
		if (net_buf_tailroom(buf) < buf_len) {
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
		net_buf_add_mem(buf, cp->buf, buf_len);
	}

	if (bt_map_mse_mas_get_owner_status(&inst->mse_mas, cp->rsp_code, buf) < 0) {
		if (buf) {
			net_buf_unref(buf);
		}
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mse_mas_get_convo_listing(const void *cmd, uint16_t cmd_len, void *rsp,
					 uint16_t *rsp_len)
{
	const struct btp_map_mse_mas_get_convo_listing_cmd *cp = cmd;
	struct mse_mas_instance *inst;
	struct net_buf *buf = NULL;
	uint16_t buf_len;

	if (cmd_len < sizeof(*cp)) {
		return BTP_STATUS_FAILED;
	}

	buf_len = sys_le16_to_cpu(cp->buf_len);
	if (cmd_len != sizeof(*cp) + buf_len) {
		return BTP_STATUS_FAILED;
	}

	inst = mse_mas_find(&cp->address, cp->instance_id);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_map_mse_mas_create_pdu(&inst->mse_mas, &tx_pool);
	if (!buf) {
		return BTP_STATUS_FAILED;
	}

	if (buf_len > 0) {
		if (net_buf_tailroom(buf) < buf_len) {
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
		net_buf_add_mem(buf, cp->buf, buf_len);
	}

	if (bt_map_mse_mas_get_convo_listing(&inst->mse_mas, cp->rsp_code, buf) < 0) {
		if (buf) {
			net_buf_unref(buf);
		}
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mse_mas_set_ntf_filter(const void *cmd, uint16_t cmd_len, void *rsp,
				      uint16_t *rsp_len)
{
	const struct btp_map_mse_mas_set_ntf_filter_cmd *cp = cmd;
	struct mse_mas_instance *inst;
	struct net_buf *buf = NULL;
	uint16_t buf_len;

	if (cmd_len < sizeof(*cp)) {
		return BTP_STATUS_FAILED;
	}

	buf_len = sys_le16_to_cpu(cp->buf_len);
	if (cmd_len != sizeof(*cp) + buf_len) {
		return BTP_STATUS_FAILED;
	}

	inst = mse_mas_find(&cp->address, cp->instance_id);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_map_mse_mas_create_pdu(&inst->mse_mas, &tx_pool);
	if (!buf) {
		return BTP_STATUS_FAILED;
	}

	if (buf_len > 0) {
		if (net_buf_tailroom(buf) < buf_len) {
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
		net_buf_add_mem(buf, cp->buf, buf_len);
	}

	if (bt_map_mse_mas_set_ntf_filter(&inst->mse_mas, cp->rsp_code, buf) < 0) {
		if (buf) {
			net_buf_unref(buf);
		}
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

/* BTP command handlers - MAP Server MNS */
static uint8_t mse_mns_rfcomm_connect(const void *cmd, uint16_t cmd_len, void *rsp,
				      uint16_t *rsp_len)
{
	const struct btp_map_mse_mns_rfcomm_connect_cmd *cp = cmd;
	struct mse_mns_instance *inst;
	struct bt_conn *conn;

	if (cp->address.type != BTP_BR_ADDRESS_TYPE) {
		return BTP_STATUS_FAILED;
	}

	conn = bt_conn_lookup_addr_br(&cp->address.a);
	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	inst = mse_mns_alloc(conn);
	if (inst == NULL) {
		bt_conn_unref(conn);
		return BTP_STATUS_FAILED;
	}

	if (bt_map_mse_mns_cb_register(&inst->mse_mns, &mse_mns_cb) < 0) {
		mse_mns_free(inst);
		bt_conn_unref(conn);
		return BTP_STATUS_FAILED;
	}

	if (bt_map_mse_mns_rfcomm_connect(conn, &inst->mse_mns, cp->channel) < 0) {
		mse_mns_free(inst);
		bt_conn_unref(conn);
		return BTP_STATUS_FAILED;
	}

	bt_conn_unref(conn);
	return BTP_STATUS_SUCCESS;
}

static uint8_t mse_mns_rfcomm_disconnect(const void *cmd, uint16_t cmd_len, void *rsp,
					 uint16_t *rsp_len)
{
	const struct btp_map_mse_mns_rfcomm_disconnect_cmd *cp = cmd;
	struct mse_mns_instance *inst;

	inst = mse_mns_find(&cp->address);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	if (bt_map_mse_mns_rfcomm_disconnect(&inst->mse_mns) < 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mse_mns_l2cap_connect(const void *cmd, uint16_t cmd_len, void *rsp,
				     uint16_t *rsp_len)
{
	const struct btp_map_mse_mns_l2cap_connect_cmd *cp = cmd;
	struct mse_mns_instance *inst;
	struct bt_conn *conn;

	if (cp->address.type != BTP_BR_ADDRESS_TYPE) {
		return BTP_STATUS_FAILED;
	}

	conn = bt_conn_lookup_addr_br(&cp->address.a);
	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	inst = mse_mns_alloc(conn);
	if (inst == NULL) {
		bt_conn_unref(conn);
		return BTP_STATUS_FAILED;
	}

	if (bt_map_mse_mns_cb_register(&inst->mse_mns, &mse_mns_cb) < 0) {
		mse_mns_free(inst);
		bt_conn_unref(conn);
		return BTP_STATUS_FAILED;
	}

	if (bt_map_mse_mns_l2cap_connect(conn, &inst->mse_mns, sys_le16_to_cpu(cp->psm)) < 0) {
		mse_mns_free(inst);
		bt_conn_unref(conn);
		return BTP_STATUS_FAILED;
	}

	bt_conn_unref(conn);
	return BTP_STATUS_SUCCESS;
}

static uint8_t mse_mns_l2cap_disconnect(const void *cmd, uint16_t cmd_len, void *rsp,
					uint16_t *rsp_len)
{
	const struct btp_map_mse_mns_l2cap_disconnect_cmd *cp = cmd;
	struct mse_mns_instance *inst;

	inst = mse_mns_find(&cp->address);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	if (bt_map_mse_mns_l2cap_disconnect(&inst->mse_mns) < 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mse_mns_connect(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_map_mse_mns_connect_cmd *cp = cmd;
	struct mse_mns_instance *inst;

	inst = mse_mns_find(&cp->address);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	if (bt_map_mse_mns_connect(&inst->mse_mns, NULL) < 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mse_mns_disconnect(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_map_mse_mns_disconnect_cmd *cp = cmd;
	struct mse_mns_instance *inst;

	inst = mse_mns_find(&cp->address);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	if (bt_map_mse_mns_disconnect(&inst->mse_mns, NULL) < 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mse_mns_abort(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_map_mse_mns_abort_cmd *cp = cmd;
	struct mse_mns_instance *inst;

	inst = mse_mns_find(&cp->address);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	if (bt_map_mse_mns_abort(&inst->mse_mns, NULL) < 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mse_mns_send_event(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_map_mse_mns_send_event_cmd *cp = cmd;
	struct mse_mns_instance *inst;
	struct net_buf *buf = NULL;
	uint16_t buf_len;

	if (cmd_len < sizeof(*cp)) {
		return BTP_STATUS_FAILED;
	}

	buf_len = sys_le16_to_cpu(cp->buf_len);
	if (cmd_len != sizeof(*cp) + buf_len) {
		return BTP_STATUS_FAILED;
	}

	inst = mse_mns_find(&cp->address);
	if (!inst) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_map_mse_mns_create_pdu(&inst->mse_mns, &tx_pool);
	if (!buf) {
		return BTP_STATUS_FAILED;
	}

	if (buf_len > 0) {
		if (net_buf_tailroom(buf) < buf_len) {
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
		net_buf_add_mem(buf, cp->buf, buf_len);
	}

	if (bt_map_mse_mns_send_event(&inst->mse_mns, cp->final, buf) < 0) {
		if (buf) {
			net_buf_unref(buf);
		}
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t supported_commands(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	struct btp_map_read_supported_commands_rp *rp = rsp;

	*rsp_len = tester_supported_commands(BTP_SERVICE_ID_MAP, rp->data);
	*rsp_len += sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static const struct btp_handler handlers[] = {
	{
		.opcode = BTP_MAP_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = supported_commands,
	},
	{
		.opcode = BTP_MAP_MCE_MAS_RFCOMM_CONNECT,
		.expect_len = sizeof(struct btp_map_mce_mas_rfcomm_connect_cmd),
		.func = mce_mas_rfcomm_connect,
	},
	{
		.opcode = BTP_MAP_MCE_MAS_RFCOMM_DISCONNECT,
		.expect_len = sizeof(struct btp_map_mce_mas_rfcomm_disconnect_cmd),
		.func = mce_mas_rfcomm_disconnect,
	},
	{
		.opcode = BTP_MAP_MCE_MAS_L2CAP_CONNECT,
		.expect_len = sizeof(struct btp_map_mce_mas_l2cap_connect_cmd),
		.func = mce_mas_l2cap_connect,
	},
	{
		.opcode = BTP_MAP_MCE_MAS_L2CAP_DISCONNECT,
		.expect_len = sizeof(struct btp_map_mce_mas_l2cap_disconnect_cmd),
		.func = mce_mas_l2cap_disconnect,
	},
	{
		.opcode = BTP_MAP_MCE_MAS_CONNECT,
		.expect_len = sizeof(struct btp_map_mce_mas_connect_cmd),
		.func = mce_mas_connect,
	},
	{
		.opcode = BTP_MAP_MCE_MAS_DISCONNECT,
		.expect_len = sizeof(struct btp_map_mce_mas_disconnect_cmd),
		.func = mce_mas_disconnect,
	},
	{
		.opcode = BTP_MAP_MCE_MAS_ABORT,
		.expect_len = sizeof(struct btp_map_mce_mas_abort_cmd),
		.func = mce_mas_abort,
	},
	{
		.opcode = BTP_MAP_MCE_MAS_SET_FOLDER,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = mce_mas_set_folder,
	},
	{
		.opcode = BTP_MAP_MCE_MAS_SET_NTF_REG,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = mce_mas_set_ntf_reg,
	},
	{
		.opcode = BTP_MAP_MCE_MAS_GET_FOLDER_LISTING,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = mce_mas_get_folder_listing,
	},
	{
		.opcode = BTP_MAP_MCE_MAS_GET_MSG_LISTING,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = mce_mas_get_msg_listing,
	},
	{
		.opcode = BTP_MAP_MCE_MAS_GET_MSG,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = mce_mas_get_msg,
	},
	{
		.opcode = BTP_MAP_MCE_MAS_SET_MSG_STATUS,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = mce_mas_set_msg_status,
	},
	{
		.opcode = BTP_MAP_MCE_MAS_PUSH_MSG,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = mce_mas_push_msg,
	},
	{
		.opcode = BTP_MAP_MCE_MAS_UPDATE_INBOX,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = mce_mas_update_inbox,
	},
	{
		.opcode = BTP_MAP_MCE_MAS_GET_MAS_INST_INFO,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = mce_mas_get_mas_inst_info,
	},
	{
		.opcode = BTP_MAP_MCE_MAS_SET_OWNER_STATUS,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = mce_mas_set_owner_status,
	},
	{
		.opcode = BTP_MAP_MCE_MAS_GET_OWNER_STATUS,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = mce_mas_get_owner_status,
	},
	{
		.opcode = BTP_MAP_MCE_MAS_GET_CONVO_LISTING,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = mce_mas_get_convo_listing,
	},
	{
		.opcode = BTP_MAP_MCE_MAS_SET_NTF_FILTER,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = mce_mas_set_ntf_filter,
	},
	{
		.opcode = BTP_MAP_MCE_MNS_RFCOMM_DISCONNECT,
		.expect_len = sizeof(struct btp_map_mce_mns_rfcomm_disconnect_cmd),
		.func = mce_mns_rfcomm_disconnect,
	},
	{
		.opcode = BTP_MAP_MCE_MNS_L2CAP_DISCONNECT,
		.expect_len = sizeof(struct btp_map_mce_mns_l2cap_disconnect_cmd),
		.func = mce_mns_l2cap_disconnect,
	},
	{
		.opcode = BTP_MAP_MCE_MNS_CONNECT,
		.expect_len = sizeof(struct btp_map_mce_mns_connect_cmd),
		.func = mce_mns_connect,
	},
	{
		.opcode = BTP_MAP_MCE_MNS_DISCONNECT,
		.expect_len = sizeof(struct btp_map_mce_mns_disconnect_cmd),
		.func = mce_mns_disconnect,
	},
	{
		.opcode = BTP_MAP_MCE_MNS_ABORT,
		.expect_len = sizeof(struct btp_map_mce_mns_abort_cmd),
		.func = mce_mns_abort,
	},
	{
		.opcode = BTP_MAP_MCE_MNS_SEND_EVENT,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = mce_mns_send_event,
	},
	{
		.opcode = BTP_MAP_MSE_MAS_RFCOMM_DISCONNECT,
		.expect_len = sizeof(struct btp_map_mse_mas_rfcomm_disconnect_cmd),
		.func = mse_mas_rfcomm_disconnect,
	},
	{
		.opcode = BTP_MAP_MSE_MAS_L2CAP_DISCONNECT,
		.expect_len = sizeof(struct btp_map_mse_mas_l2cap_disconnect_cmd),
		.func = mse_mas_l2cap_disconnect,
	},
	{
		.opcode = BTP_MAP_MSE_MAS_CONNECT,
		.expect_len = sizeof(struct btp_map_mse_mas_connect_cmd),
		.func = mse_mas_connect,
	},
	{
		.opcode = BTP_MAP_MSE_MAS_DISCONNECT,
		.expect_len = sizeof(struct btp_map_mse_mas_disconnect_cmd),
		.func = mse_mas_disconnect,
	},
	{
		.opcode = BTP_MAP_MSE_MAS_ABORT,
		.expect_len = sizeof(struct btp_map_mse_mas_abort_cmd),
		.func = mse_mas_abort,
	},
	{
		.opcode = BTP_MAP_MSE_MAS_SET_FOLDER,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = mse_mas_set_folder,
	},
	{
		.opcode = BTP_MAP_MSE_MAS_SET_NTF_REG,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = mse_mas_set_ntf_reg,
	},
	{
		.opcode = BTP_MAP_MSE_MAS_GET_FOLDER_LISTING,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = mse_mas_get_folder_listing,
	},
	{
		.opcode = BTP_MAP_MSE_MAS_GET_MSG_LISTING,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = mse_mas_get_msg_listing,
	},
	{
		.opcode = BTP_MAP_MSE_MAS_GET_MSG,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = mse_mas_get_msg,
	},
	{
		.opcode = BTP_MAP_MSE_MAS_SET_MSG_STATUS,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = mse_mas_set_msg_status,
	},
	{
		.opcode = BTP_MAP_MSE_MAS_PUSH_MSG,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = mse_mas_push_msg,
	},
	{
		.opcode = BTP_MAP_MSE_MAS_UPDATE_INBOX,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = mse_mas_update_inbox,
	},
	{
		.opcode = BTP_MAP_MSE_MAS_GET_MAS_INST_INFO,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = mse_mas_get_mas_inst_info,
	},
	{
		.opcode = BTP_MAP_MSE_MAS_SET_OWNER_STATUS,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = mse_mas_set_owner_status,
	},
	{
		.opcode = BTP_MAP_MSE_MAS_GET_OWNER_STATUS,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = mse_mas_get_owner_status,
	},
	{
		.opcode = BTP_MAP_MSE_MAS_GET_CONVO_LISTING,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = mse_mas_get_convo_listing,
	},
	{
		.opcode = BTP_MAP_MSE_MAS_SET_NTF_FILTER,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = mse_mas_set_ntf_filter,
	},
	{
		.opcode = BTP_MAP_MSE_MNS_RFCOMM_CONNECT,
		.expect_len = sizeof(struct btp_map_mse_mns_rfcomm_connect_cmd),
		.func = mse_mns_rfcomm_connect,
	},
	{
		.opcode = BTP_MAP_MSE_MNS_RFCOMM_DISCONNECT,
		.expect_len = sizeof(struct btp_map_mse_mns_rfcomm_disconnect_cmd),
		.func = mse_mns_rfcomm_disconnect,
	},
	{
		.opcode = BTP_MAP_MSE_MNS_L2CAP_CONNECT,
		.expect_len = sizeof(struct btp_map_mse_mns_l2cap_connect_cmd),
		.func = mse_mns_l2cap_connect,
	},
	{
		.opcode = BTP_MAP_MSE_MNS_L2CAP_DISCONNECT,
		.expect_len = sizeof(struct btp_map_mse_mns_l2cap_disconnect_cmd),
		.func = mse_mns_l2cap_disconnect,
	},
	{
		.opcode = BTP_MAP_MSE_MNS_CONNECT,
		.expect_len = sizeof(struct btp_map_mse_mns_connect_cmd),
		.func = mse_mns_connect,
	},
	{
		.opcode = BTP_MAP_MSE_MNS_DISCONNECT,
		.expect_len = sizeof(struct btp_map_mse_mns_disconnect_cmd),
		.func = mse_mns_disconnect,
	},
	{
		.opcode = BTP_MAP_MSE_MNS_ABORT,
		.expect_len = sizeof(struct btp_map_mse_mns_abort_cmd),
		.func = mse_mns_abort,
	},
	{
		.opcode = BTP_MAP_MSE_MNS_SEND_EVENT,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = mse_mns_send_event,
	},
	{
		.opcode = BTP_MAP_SDP_DISCOVER,
		.expect_len = sizeof(struct btp_map_sdp_discover_cmd),
		.func = map_sdp_discover,
	},
};

static int mce_mns_rfcomm_register(void)
{
	int err;

	/* Register MCE MNS RFCOMM server */
	mce_server.rfcomm_server.server.rfcomm.channel = 0;
	mce_server.rfcomm_server.accept = mce_mns_rfcomm_accept;
	err = bt_map_mce_mns_rfcomm_register(&mce_server.rfcomm_server);
	if (err != 0) {
		LOG_ERR("Failed to register MCE MNS RFCOMM server (err %d)", err);
		return err;
	}
	LOG_DBG("MCE MNS RFCOMM server (channel %02x) registered",
		mce_server.rfcomm_server.server.rfcomm.channel);

	return 0;
}

static int mce_mns_l2cap_register(void)
{
	int err;

	/* Register MCE MNS L2CAP server */
	mce_server.l2cap_server.server.l2cap.psm = 0;
	mce_server.l2cap_server.accept = mce_mns_l2cap_accept;
	err = bt_map_mce_mns_l2cap_register(&mce_server.l2cap_server);
	if (err != 0) {
		LOG_ERR("Failed to register MCE MNS L2CAP server (err %d)", err);
		return err;
	}
	LOG_DBG("MCE MNS L2CAP server (psm %04x) registered",
		mce_server.l2cap_server.server.l2cap.psm);

	return 0;
}

static int mse_mas_rfcomm_register(void)
{
	int err;
	uint8_t i;

	/* Register MSE MAS RFCOMM servers */
	for (i = 0; i < MAP_MAS_MAX_NUM; i++) {
		mse_server[i].rfcomm_server.server.rfcomm.channel = 0;
		mse_server[i].rfcomm_server.accept = mse_mas_rfcomm_accept;
		err = bt_map_mse_mas_rfcomm_register(&mse_server[i].rfcomm_server);
		if (err != 0) {
			LOG_ERR("Failed to register MSE MAS RFCOMM server %d (err %d)", i, err);
			return err;
		}
		LOG_DBG("MSE MAS RFCOMM server %d (channel %02x) registered", i,
			mse_server[i].rfcomm_server.server.rfcomm.channel);
	}

	return 0;
}

static int mse_mas_l2cap_register(void)
{
	int err;
	uint8_t i;

	/* Register MSE MAS L2CAP servers */
	for (i = 0; i < MAP_MAS_MAX_NUM; i++) {
		mse_server[i].l2cap_server.server.l2cap.psm = 0;
		mse_server[i].l2cap_server.accept = mse_mas_l2cap_accept;
		err = bt_map_mse_mas_l2cap_register(&mse_server[i].l2cap_server);
		if (err != 0) {
			LOG_ERR("Failed to register MSE MAS L2CAP server %d (err %d)", i, err);
			return err;
		}
		LOG_DBG("MSE MAS L2CAP server %d (psm %04x) registered", i,
			mse_server[i].l2cap_server.server.l2cap.psm);
	}

	return 0;
}

uint8_t tester_init_map(void)
{
	int err;

	tester_register_command_handlers(BTP_SERVICE_ID_MAP, handlers, ARRAY_SIZE(handlers));

	/* Register MCE MNS RFCOMM server */
	err = mce_mns_rfcomm_register();
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	/* Register MCE MNS L2CAP server */
	err = mce_mns_l2cap_register();
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	/* Register MCE MNS SDP record */
	err = bt_sdp_register_service(&mce_mns_rec);
	if (err < 0) {
		return BTP_STATUS_FAILED;
	}

	/* Register MSE MAS RFCOMM servers */
	err = mse_mas_rfcomm_register();
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	/* Register MSE MAS L2CAP servers */
	err = mse_mas_l2cap_register();
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	/* Register MSE MAS SDP records */
	for (uint8_t i = 0; i < MAP_MAS_MAX_NUM; i++) {
		err = bt_sdp_register_service(&mse_mas_rec[i]);
		if (err < 0) {
			return BTP_STATUS_FAILED;
		}
	}

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_map(void)
{
	return BTP_STATUS_SUCCESS;
}
