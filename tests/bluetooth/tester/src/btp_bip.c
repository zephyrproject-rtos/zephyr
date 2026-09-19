/* btp_bip.c - Bluetooth BIP Tester */

/*
 * Copyright 2025 NXP
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


#define BIP_POOL_BUF_SIZE                                                                         \
	MAX(BT_RFCOMM_BUF_SIZE(CONFIG_BT_GOEP_RFCOMM_MTU),                                         \
	    BT_L2CAP_BUF_SIZE(CONFIG_BT_GOEP_L2CAP_MTU))

NET_BUF_POOL_FIXED_DEFINE(bip_tx_pool, CONFIG_BT_MAX_CONN, BIP_POOL_BUF_SIZE,
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

/* Multi-instance support: one instance per BR/EDR connection, located by address. */
#define BIP_MAX_INSTANCES CONFIG_BT_MAX_CONN

struct bip_app {
	struct bt_bip_client client;
	struct bt_bip bip;
	struct bt_bip_server server;
	struct bt_bip_client second_client;
	struct bt_bip_server second_server;
	/*
	 * Dedicated bt_bip for the secondary (Referenced/Archived Objects)
	 * server. The secondary server is reached over its own transport (a
	 * different BIP PSM/RFCOMM channel), which may coexist with the primary
	 * transport. Since a single bt_bip carries only one transport at a time,
	 * the secondary server must live on its own bt_bip so the incoming
	 * secondary transport binds here and its OBEX already owns the server.
	 */
	struct bt_bip second_bip;
	struct bt_conn *conn;
	struct bt_conn *second_conn;
	bt_addr_t address;

	uint16_t client_mopl;
	uint16_t server_mopl;
	uint32_t conn_id;
	uint16_t second_client_mopl;
	uint16_t second_server_mopl;
	uint32_t second_conn_id;
	bool in_use;
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

/* SDP record */
static struct bt_sdp_attribute bip_responder_attrs[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(BT_SDP_ATTR_SVCLASS_ID_LIST, BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		    BT_SDP_DATA_ELEM_LIST({BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
					   BT_SDP_ARRAY_16(BT_SDP_IMAGING_RESPONDER_SVCLASS)}, )),
	BT_SDP_LIST(BT_SDP_ATTR_PROTO_DESC_LIST, BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 17),
		    BT_SDP_DATA_ELEM_LIST(
			    {BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
			     BT_SDP_DATA_ELEM_LIST({BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
						    BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP)}, )},
			    {BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 5),
			     BT_SDP_DATA_ELEM_LIST(
				     {BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				      BT_SDP_ARRAY_16(BT_SDP_PROTO_RFCOMM)},
				     {BT_SDP_TYPE_SIZE(BT_SDP_UINT8), &bip_rfcomm_channel}, )},
			    {BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
			     BT_SDP_DATA_ELEM_LIST({BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
						    BT_SDP_ARRAY_16(BT_SDP_PROTO_OBEX)}, )}, )),
	BT_SDP_SERVICE_NAME("imaging"),
	BT_SDP_LIST(BT_SDP_ATTR_PROFILE_DESC_LIST, BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
		    BT_SDP_DATA_ELEM_LIST(
			    {BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			     BT_SDP_DATA_ELEM_LIST({BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
						    BT_SDP_ARRAY_16(BT_SDP_IMAGING_SVCLASS)},
						   {BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
						    BT_SDP_ARRAY_16(0x0102)}, )}, )),
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

/* SDP record - Imaging Automatic Archive */
static struct bt_sdp_attribute bip_archive_attrs[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(BT_SDP_ATTR_SVCLASS_ID_LIST, BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		    BT_SDP_DATA_ELEM_LIST({BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
					   BT_SDP_ARRAY_16(BT_SDP_IMAGING_ARCHIVE_SVCLASS)}, )),
	BT_SDP_LIST(BT_SDP_ATTR_PROTO_DESC_LIST, BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 17),
		    BT_SDP_DATA_ELEM_LIST(
			    {BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
			     BT_SDP_DATA_ELEM_LIST({BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
						    BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP)}, )},
			    {BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 5),
			     BT_SDP_DATA_ELEM_LIST(
				     {BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				      BT_SDP_ARRAY_16(BT_SDP_PROTO_RFCOMM)},
				     {BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
				      &bip_archive_rfcomm_channel}, )},
			    {BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
			     BT_SDP_DATA_ELEM_LIST({BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
						    BT_SDP_ARRAY_16(BT_SDP_PROTO_OBEX)}, )}, )),
	BT_SDP_SERVICE_NAME("imaging_archive"),
        BT_SDP_SERVICE_ID(BT_UUID_INIT_16(BT_SDP_IMAGING_ARCHIVE_SVCLASS)),
	BT_SDP_LIST(BT_SDP_ATTR_PROFILE_DESC_LIST, BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
		    BT_SDP_DATA_ELEM_LIST(
			    {BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			     BT_SDP_DATA_ELEM_LIST({BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
						    BT_SDP_ARRAY_16(BT_SDP_IMAGING_SVCLASS)},
						   {BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
						    BT_SDP_ARRAY_16(0x0102)}, )}, )),
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

/* SDP record - Imaging Referenced Objects */
static struct bt_sdp_attribute bip_refobj_attrs[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(BT_SDP_ATTR_SVCLASS_ID_LIST, BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		    BT_SDP_DATA_ELEM_LIST({BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
					   BT_SDP_ARRAY_16(BT_SDP_IMAGING_REFOBJS_SVCLASS)}, )),
	BT_SDP_LIST(BT_SDP_ATTR_PROTO_DESC_LIST, BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 17),
		    BT_SDP_DATA_ELEM_LIST(
			    {BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
			     BT_SDP_DATA_ELEM_LIST({BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
						    BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP)}, )},
			    {BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 5),
			     BT_SDP_DATA_ELEM_LIST(
				     {BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				      BT_SDP_ARRAY_16(BT_SDP_PROTO_RFCOMM)},
				     {BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
				      &bip_refobj_rfcomm_channel}, )},
			    {BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
			     BT_SDP_DATA_ELEM_LIST({BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
						    BT_SDP_ARRAY_16(BT_SDP_PROTO_OBEX)}, )}, )),
	BT_SDP_SERVICE_NAME("imaging_referenced_objects"),
        BT_SDP_SERVICE_ID(BT_UUID_INIT_16(BT_SDP_IMAGING_REFOBJS_SVCLASS)),
	BT_SDP_LIST(BT_SDP_ATTR_PROFILE_DESC_LIST, BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
		    BT_SDP_DATA_ELEM_LIST(
			    {BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			     BT_SDP_DATA_ELEM_LIST({BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
						    BT_SDP_ARRAY_16(BT_SDP_IMAGING_SVCLASS)},
						   {BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
						    BT_SDP_ARRAY_16(0x0102)}, )}, )),
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

/* Instance management helpers */
static struct bip_app *find_instance_by_address(const bt_addr_t *address)
{
	for (uint8_t i = 0; i < BIP_MAX_INSTANCES; i++) {
		if (bip_apps[i].in_use && bip_apps[i].conn != NULL &&
		    bt_addr_eq(&bip_apps[i].address, address)) {
			return &bip_apps[i];
		}
	}
	return NULL;
}

/*
 * Find a connection-less instance that was pre-registered by address before
 * the remote peer connected (server registered while conn == NULL). Used by
 * the transport accept callbacks so that the incoming transport reuses the
 * instance that already has a registered OBEX server, instead of allocating a
 * fresh instance with an empty server list.
 */
static struct bip_app *find_preregistered_instance_by_address(const bt_addr_t *address)
{
	for (uint8_t i = 0; i < BIP_MAX_INSTANCES; i++) {
		if (bip_apps[i].in_use && bip_apps[i].conn == NULL &&
		    bt_addr_eq(&bip_apps[i].address, address)) {
			return &bip_apps[i];
		}
	}
	return NULL;
}

static struct bip_app *bip_instance_allocate(struct bt_conn *conn)
{
	for (uint8_t i = 0; i < BIP_MAX_INSTANCES; i++) {
		if (!bip_apps[i].in_use) {
			memset(&bip_apps[i], 0, sizeof(struct bip_app));
			bip_apps[i].in_use = true;
			bip_apps[i].conn = conn;
                        if (conn != NULL){
                                bt_addr_copy(&bip_apps[i].address, bt_conn_get_dst_br(conn));
                        }
			return &bip_apps[i];
		}
	}
	return NULL;
}

static void bip_instance_free(struct bip_app *inst)
{
	if (inst->conn != NULL) {
		bt_conn_unref(inst->conn);
		inst->conn = NULL;
	}
	inst->in_use = false;
}

/*
 * True if this instance owns a persistent OBEX server registration, i.e. it
 * was created by server_register() before the peer connected. Such an instance
 * has its BIP server bound to its own bt_bip; a purely transport-carrying
 * instance allocated in the accept callback never registers a server, so its
 * server._bip stays NULL. This reuses existing state instead of a dedicated
 * flag to distinguish the two lifetimes.
 *
 * Both the primary server and the secondary server (for example the Advanced
 * Image Printing Referenced Objects server, registered after the primary
 * client connection) count as persistent registrations. The secondary case
 * matters when the peer tears down the primary transport and then opens a new
 * transport (a different BIP PSM) to reach the secondary server: the instance
 * must survive the primary disconnect so the incoming transport rebinds to the
 * same bt_bip that owns the already-registered secondary OBEX server, instead
 * of landing on a fresh instance with an empty server list (which the OBEX
 * CONNECT would reject with NOT_FOUND).
 */
static bool bip_instance_is_preregistered(const struct bip_app *inst)
{
	return inst->server._bip == &inst->bip ||
	       inst->second_server._bip == &inst->second_bip;
}

/*
 * Release the transport binding of an instance on disconnect. A pre-registered
 * instance owns a persistent OBEX server registration keyed on the peer
 * address, so it must NOT be torn down here: only its transport connection is
 * released, returning it to the connection-less state so a later incoming
 * transport for the same peer (for example a different BIP PSM such as the
 * referenced-objects server) can be bound to it again by the accept callback.
 * Instances that were allocated dynamically for a transport are freed.
 */
static void bip_instance_release_transport(struct bip_app *inst)
{
	if (inst->conn != NULL) {
		bt_conn_unref(inst->conn);
		inst->conn = NULL;
	}

	if (!bip_instance_is_preregistered(inst)) {
		inst->in_use = false;
	}
}

/*
 * Drop an instance after an OBEX server unregister if it no longer serves any
 * purpose: no live transport on either bt_bip and no server still registered.
 *
 * bt_bip_server_unregister() only removes the server from the host's list; it
 * leaves server._bip set, so the unregister handlers must clear it themselves
 * before calling this (otherwise bip_instance_is_preregistered() keeps
 * reporting the instance as pre-registered). Without this collection the slot
 * stays in_use forever, exhausting the CONFIG_BT_MAX_CONN-sized pool after a
 * few register/unregister cycles and leaving a zombie that
 * find_preregistered_instance_by_address() would rebind an incoming transport
 * to (its OBEX server is gone, so the CONNECT is answered NOT_FOUND).
 */
static void bip_instance_gc(struct bip_app *inst)
{
	if (inst->conn == NULL && inst->second_conn == NULL &&
	    !bip_instance_is_preregistered(inst)) {
		bip_instance_free(inst);
	}
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
	if (inst != NULL && inst->in_use) {
		bt_addr_copy(&addr->a, &inst->address);
		addr->type = BTP_BR_ADDRESS_TYPE;
	} else {
		memset(addr, 0, sizeof(*addr));
	}
}

/* SDP discovery */
#define BIP_SDP_DISCOVER_BUF_LEN 512
NET_BUF_POOL_FIXED_DEFINE(bip_sdp_pool, CONFIG_BT_MAX_CONN, BIP_SDP_DISCOVER_BUF_LEN,
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static struct bt_sdp_discover_params sdp_bip_params;

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

	if ((value.type != BT_SDP_ATTR_VALUE_TYPE_UINT) || (value.uint.size != sizeof(*psm))) {
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

	if ((value.type != BT_SDP_ATTR_VALUE_TYPE_UINT) || (value.uint.size != sizeof(*funcs))) {
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

	if ((value.type != BT_SDP_ATTR_VALUE_TYPE_UINT) || (value.uint.size != sizeof(*caps))) {
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
	int err;

	if (result == NULL || result->resp_buf == NULL || conn == NULL || params == NULL) {
		return BT_SDP_DISCOVER_UUID_STOP;
	}

	is_responder = bt_uuid_cmp(params->uuid, BT_UUID_DECLARE_16(BT_SDP_IMAGING_RESPONDER_SVCLASS)) == 0;

	{
		int rfcomm_err = bt_sdp_get_proto_param(result->resp_buf, BT_SDP_PROTO_RFCOMM,
							&rfcomm_channel);
		int l2cap_err = bip_sdp_get_goep_l2cap_psm(result->resp_buf, &l2cap_psm);

		if (rfcomm_err != 0 && l2cap_err != 0) {
			LOG_DBG("No RFCOMM channel or L2CAP PSM attribute");
		}
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

/* Transport callbacks */
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

/*
 * Canonical wire layouts for the two families of variable-length BIP events.
 * The generic serializers below write through these structures instead of
 * computing byte offsets by hand, so one structure definition is the source of
 * truth for the wire format of each family. They carry no opcode of their own
 * and are private to this file; every per-event structure in btp_bip.h must
 * stay byte-compatible with one of them.
 *
 * bip_param_ev covers the events carrying a leading parameter byte (Final on
 * server requests, response code on client responses).
 * bip_plain_ev covers the events that have no such byte, i.e. the server and
 * secondary server disconnect/abort requests.
 */
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

/*
 * The BTP response buffer is a fixed BTP_MTU-sized static buffer; the amount of
 * OBEX payload an event carries is bounded by the GOEP MOPL, which can exceed
 * it. Drop the event with a log line instead of overrunning the buffer, which
 * tester_rsp_buffer_allocate() only catches with an assert (compiled out in
 * release builds).
 */
static bool bip_ev_len_ok(uint8_t ev_opcode, size_t ev_len)
{
	if (ev_len > BTP_DATA_MAX_SIZE) {
		LOG_ERR("BIP event 0x%02x dropped: %zu bytes exceed BTP limit %zu", ev_opcode,
			ev_len, (size_t)BTP_DATA_MAX_SIZE);
		return false;
	}

	return true;
}

/* Helper to send variable-length server events carrying a Final flag */
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

/*
 * Helper to send the variable-length server events that have no Final flag,
 * i.e. the disconnect and abort requests. These must not borrow
 * send_server_event(): OBEX DISCONNECT and ABORT are single-packet operations,
 * so their event structures carry no parameter byte and the upper tester parses
 * a header one byte shorter.
 */
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

/* Helper to send variable-length client events */
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

/* Server callbacks */
static void bip_server_connect(struct bt_bip_server *server, uint8_t version, uint16_t mopl,
			       struct net_buf *buf)
{
	struct bip_app *inst = inst_from_server(server);
	uint8_t *ev_data;
	size_t ev_len;
	uint16_t data_len = (buf != NULL) ? buf->len : 0;

	inst->client_mopl = mopl;

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

#define BIP_SERVER_REQ_CB(cbname, evop)                                                       \
	static void cbname(struct bt_bip_server *server, bool final, struct net_buf *buf)     \
	{                                                                                     \
		send_server_event(inst_from_server(server), evop, final, buf);                \
	}

BIP_SERVER_REQ_CB(bip_server_get_caps, BTP_BIP_EV_SERVER_GET_CAPS_REQ)
BIP_SERVER_REQ_CB(bip_server_get_image_list, BTP_BIP_EV_SERVER_GET_IMAGE_LIST_REQ)
BIP_SERVER_REQ_CB(bip_server_get_image_properties, BTP_BIP_EV_SERVER_GET_IMAGE_PROPERTIES_REQ)
BIP_SERVER_REQ_CB(bip_server_get_image, BTP_BIP_EV_SERVER_GET_IMAGE_REQ)
BIP_SERVER_REQ_CB(bip_server_get_linked_thumbnail, BTP_BIP_EV_SERVER_GET_LINKED_THUMBNAIL_REQ)
BIP_SERVER_REQ_CB(bip_server_get_linked_attachment, BTP_BIP_EV_SERVER_GET_LINKED_ATTACHMENT_REQ)
BIP_SERVER_REQ_CB(bip_server_get_monitoring_image, BTP_BIP_EV_SERVER_GET_MONITORING_IMAGE_REQ)
BIP_SERVER_REQ_CB(bip_server_get_status, BTP_BIP_EV_SERVER_GET_STATUS_REQ)
BIP_SERVER_REQ_CB(bip_server_put_image, BTP_BIP_EV_SERVER_PUT_IMAGE_REQ)
BIP_SERVER_REQ_CB(bip_server_put_linked_thumbnail, BTP_BIP_EV_SERVER_PUT_LINKED_THUMBNAIL_REQ)
BIP_SERVER_REQ_CB(bip_server_put_linked_attachment, BTP_BIP_EV_SERVER_PUT_LINKED_ATTACHMENT_REQ)
BIP_SERVER_REQ_CB(bip_server_remote_display, BTP_BIP_EV_SERVER_REMOTE_DISPLAY_REQ)
BIP_SERVER_REQ_CB(bip_server_delete_image, BTP_BIP_EV_SERVER_DELETE_IMAGE_REQ)
BIP_SERVER_REQ_CB(bip_server_start_print, BTP_BIP_EV_SERVER_START_PRINT_REQ)
BIP_SERVER_REQ_CB(bip_server_start_archive, BTP_BIP_EV_SERVER_START_ARCHIVE_REQ)

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

/* Client callbacks */
static void bip_client_connect(struct bt_bip_client *client, uint8_t rsp_code, uint8_t version,
			       uint16_t mopl, struct net_buf *buf)
{
	struct bip_app *inst = inst_from_client(client);
	uint8_t *ev_data;
	size_t ev_len;
	uint16_t data_len = (buf != NULL) ? buf->len : 0;

	inst->server_mopl = mopl;
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

#define BIP_CLIENT_RSP_CB(cbname, evop)                                                        \
	static void cbname(struct bt_bip_client *client, uint8_t rsp_code, struct net_buf *buf) \
	{                                                                                      \
		send_client_event(inst_from_client(client), evop, rsp_code, buf);              \
	}

BIP_CLIENT_RSP_CB(bip_client_get_caps, BTP_BIP_EV_CLIENT_GET_CAPS_RSP)
BIP_CLIENT_RSP_CB(bip_client_get_image_list, BTP_BIP_EV_CLIENT_GET_IMAGE_LIST_RSP)
BIP_CLIENT_RSP_CB(bip_client_get_image_properties, BTP_BIP_EV_CLIENT_GET_IMAGE_PROPERTIES_RSP)
BIP_CLIENT_RSP_CB(bip_client_get_image, BTP_BIP_EV_CLIENT_GET_IMAGE_RSP)
BIP_CLIENT_RSP_CB(bip_client_get_linked_thumbnail, BTP_BIP_EV_CLIENT_GET_LINKED_THUMBNAIL_RSP)
BIP_CLIENT_RSP_CB(bip_client_get_linked_attachment, BTP_BIP_EV_CLIENT_GET_LINKED_ATTACHMENT_RSP)
BIP_CLIENT_RSP_CB(bip_client_get_partial_image, BTP_BIP_EV_CLIENT_GET_PARTIAL_IMAGE_RSP)
BIP_CLIENT_RSP_CB(bip_client_get_monitoring_image, BTP_BIP_EV_CLIENT_GET_MONITORING_IMAGE_RSP)
BIP_CLIENT_RSP_CB(bip_client_get_status, BTP_BIP_EV_CLIENT_GET_STATUS_RSP)
BIP_CLIENT_RSP_CB(bip_client_put_image, BTP_BIP_EV_CLIENT_PUT_IMAGE_RSP)
BIP_CLIENT_RSP_CB(bip_client_put_linked_thumbnail, BTP_BIP_EV_CLIENT_PUT_LINKED_THUMBNAIL_RSP)
BIP_CLIENT_RSP_CB(bip_client_put_linked_attachment, BTP_BIP_EV_CLIENT_PUT_LINKED_ATTACHMENT_RSP)
BIP_CLIENT_RSP_CB(bip_client_remote_display, BTP_BIP_EV_CLIENT_REMOTE_DISPLAY_RSP)
BIP_CLIENT_RSP_CB(bip_client_delete_image, BTP_BIP_EV_CLIENT_DELETE_IMAGE_RSP)
BIP_CLIENT_RSP_CB(bip_client_start_print, BTP_BIP_EV_CLIENT_START_PRINT_RSP)
BIP_CLIENT_RSP_CB(bip_client_start_archive, BTP_BIP_EV_CLIENT_START_ARCHIVE_RSP)

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

/* Secondary server callbacks - connection establishment/teardown only */
static void bip_second_server_connect(struct bt_bip_server *server, uint8_t version, uint16_t mopl,
				      struct net_buf *buf)
{
	struct bip_app *inst = inst_from_second_server(server);
	uint8_t *ev_data;
	size_t ev_len;
	uint16_t data_len = (buf != NULL) ? buf->len : 0;

	inst->second_client_mopl = mopl;

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

/*
 * Secondary-server request callbacks. These must NOT reuse the primary
 * bip_server_* callbacks: those recover the instance via inst_from_server()
 * (CONTAINER_OF on the .server field), whereas requests on the secondary
 * server are delivered with &inst->second_server, so the instance must be
 * recovered with inst_from_second_server(). Reusing the primary callbacks here
 * would compute a wrong bip_app pointer and report a bogus peer address.
 */
#define BIP_SECOND_SERVER_REQ_CB(cbname, evop)                                                \
	static void cbname(struct bt_bip_server *server, bool final, struct net_buf *buf)     \
	{                                                                                     \
		send_server_event(inst_from_second_server(server), evop, final, buf);         \
	}

BIP_SECOND_SERVER_REQ_CB(bip_second_server_get_partial_image,
			 BTP_BIP_EV_SERVER_GET_PARTIAL_IMAGE_REQ)
/*
 * The Archived Objects requests arrive on the secondary OBEX connection and
 * must be reported with the dedicated BTP_BIP_EV_SECOND_SERVER_*_REQ events so
 * the upper tester replies with the matching BTP_BIP_SECOND_*_RSP command,
 * which is routed back to inst->second_server. Reusing the primary
 * BTP_BIP_EV_SERVER_*_REQ events would make the tester answer with the primary
 * BTP_BIP_*_RSP commands (routed to inst->server), which has no pending GET on
 * the primary connection and fails with "Invalid state".
 *
 * GetPartialImage is the exception: it is a Referenced-Objects-only operation
 * with no BTP_BIP_EV_SECOND_SERVER_* variant, so it keeps the primary event.
 * The primary server never registers get_partial_image (the operation cannot
 * arrive on the primary connection), so this event is only ever emitted here.
 */
BIP_SECOND_SERVER_REQ_CB(bip_second_server_get_caps, BTP_BIP_EV_SECOND_SERVER_GET_CAPS_REQ)
BIP_SECOND_SERVER_REQ_CB(bip_second_server_get_image_list,
			 BTP_BIP_EV_SECOND_SERVER_GET_IMAGE_LIST_REQ)
BIP_SECOND_SERVER_REQ_CB(bip_second_server_get_image_properties,
			 BTP_BIP_EV_SECOND_SERVER_GET_IMAGE_PROPERTIES_REQ)
BIP_SECOND_SERVER_REQ_CB(bip_second_server_get_image, BTP_BIP_EV_SECOND_SERVER_GET_IMAGE_REQ)
BIP_SECOND_SERVER_REQ_CB(bip_second_server_get_linked_thumbnail,
			 BTP_BIP_EV_SECOND_SERVER_GET_LINKED_THUMBNAIL_REQ)
BIP_SECOND_SERVER_REQ_CB(bip_second_server_get_linked_attachment,
			 BTP_BIP_EV_SECOND_SERVER_GET_LINKED_ATTACHMENT_REQ)
BIP_SECOND_SERVER_REQ_CB(bip_second_server_delete_image,
			 BTP_BIP_EV_SECOND_SERVER_DELETE_IMAGE_REQ)

static struct bt_bip_server_cb bip_second_server_cb = {
	.connect = bip_second_server_connect,
	.disconnect = bip_second_server_disconnect,
	.abort = bip_second_server_abort,
	/*
	 * The secondary server serves the Imaging Referenced Objects and
	 * Imaging Archived Objects connections. Its request callbacks must be
	 * registered here, otherwise the BIP stack cannot match a callback for
	 * the incoming request and returns OBEX Not Implemented (0x51 / 0xD1).
	 *
	 * Referenced Objects feature: GetPartialImage.
	 * Archived Objects feature: GetCapabilities, GetImagesList,
	 * GetImageProperties, GetImage, GetLinkedThumbnail,
	 * GetLinkedAttachment, DeleteImage.
	 */
	.get_partial_image = bip_second_server_get_partial_image,
	.get_caps = bip_second_server_get_caps,
	.get_image_list = bip_second_server_get_image_list,
	.get_image_properties = bip_second_server_get_image_properties,
	.get_image = bip_second_server_get_image,
	.get_linked_thumbnail = bip_second_server_get_linked_thumbnail,
	.get_linked_attachment = bip_second_server_get_linked_attachment,
	.delete_image = bip_second_server_delete_image,
};

/* Secondary client callbacks - connection establishment/teardown only */
static void bip_second_client_connect(struct bt_bip_client *client, uint8_t rsp_code,
				      uint8_t version, uint16_t mopl, struct net_buf *buf)
{
	struct bip_app *inst = inst_from_second_client(client);
	uint8_t *ev_data;
	size_t ev_len;
	uint16_t data_len = (buf != NULL) ? buf->len : 0;

	inst->second_server_mopl = mopl;
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

/*
 * Secondary-client response callbacks.
 *
 * When the IUT is the client on the secondary OBEX connection (for example
 * BIP/AAI/ACH pulling the archived image list), the BIP host looks up the
 * matching response callback in bip_second_client_cb before it will accept the
 * request; a NULL entry makes the host reject the operation locally with
 * "Unsupported request" (-EINVAL). These callbacks must be registered and must
 * recover the instance via inst_from_second_client() (the secondary client is
 * &inst->second_client, not &inst->client), then report the response on the
 * dedicated BTP_BIP_EV_SECOND_CLIENT_*_RSP event so the upper tester does not
 * confuse it with a primary-connection response.
 */
#define BIP_SECOND_CLIENT_RSP_CB(cbname, evop)                                                 \
	static void cbname(struct bt_bip_client *client, uint8_t rsp_code, struct net_buf *buf) \
	{                                                                                      \
		send_client_event(inst_from_second_client(client), evop, rsp_code, buf);       \
	}

BIP_SECOND_CLIENT_RSP_CB(bip_second_client_get_caps, BTP_BIP_EV_SECOND_CLIENT_GET_CAPS_RSP)
BIP_SECOND_CLIENT_RSP_CB(bip_second_client_get_image_list,
			 BTP_BIP_EV_SECOND_CLIENT_GET_IMAGE_LIST_RSP)
BIP_SECOND_CLIENT_RSP_CB(bip_second_client_get_image_properties,
			 BTP_BIP_EV_SECOND_CLIENT_GET_IMAGE_PROPERTIES_RSP)
BIP_SECOND_CLIENT_RSP_CB(bip_second_client_get_image, BTP_BIP_EV_SECOND_CLIENT_GET_IMAGE_RSP)
BIP_SECOND_CLIENT_RSP_CB(bip_second_client_get_linked_thumbnail,
			 BTP_BIP_EV_SECOND_CLIENT_GET_LINKED_THUMBNAIL_RSP)
BIP_SECOND_CLIENT_RSP_CB(bip_second_client_get_linked_attachment,
			 BTP_BIP_EV_SECOND_CLIENT_GET_LINKED_ATTACHMENT_RSP)
BIP_SECOND_CLIENT_RSP_CB(bip_second_client_get_partial_image,
			 BTP_BIP_EV_SECOND_CLIENT_GET_PARTIAL_IMAGE_RSP)
BIP_SECOND_CLIENT_RSP_CB(bip_second_client_delete_image,
			 BTP_BIP_EV_SECOND_CLIENT_DELETE_IMAGE_RSP)

static struct bt_bip_client_cb bip_second_client_cb = {
	.connect = bip_second_client_connect,
	.disconnect = bip_second_client_disconnect,
	.abort = bip_second_client_abort,
	/*
	 * Response callbacks for the secondary client. The secondary connection
	 * serves the Imaging Referenced Objects and Imaging Archived Objects
	 * services; without these the BIP host rejects the outgoing GET/DELETE
	 * requests with "Unsupported request".
	 *
	 * Referenced Objects feature: GetPartialImage.
	 * Archived Objects feature: GetCapabilities, GetImagesList,
	 * GetImageProperties, GetImage, GetLinkedThumbnail,
	 * GetLinkedAttachment, DeleteImage.
	 */
	.get_caps = bip_second_client_get_caps,
	.get_image_list = bip_second_client_get_image_list,
	.get_image_properties = bip_second_client_get_image_properties,
	.get_image = bip_second_client_get_image,
	.get_linked_thumbnail = bip_second_client_get_linked_thumbnail,
	.get_linked_attachment = bip_second_client_get_linked_attachment,
	.get_partial_image = bip_second_client_get_partial_image,
	.delete_image = bip_second_client_delete_image,
};

/* RFCOMM/L2CAP accept callbacks */

/*
 * Bind an incoming BIP transport (RFCOMM or L2CAP) to a bip_app instance.
 *
 * A single peer address may establish more than one BIP transport over the
 * life of an ACL link (for example the peer connects the primary imaging
 * service, disconnects it, then connects the referenced-objects service on a
 * different PSM). Each such transport needs its own instance, so this must NOT
 * reject a second connection from the same address: doing so previously caused
 * the host L2CAP layer to answer the second connect request with
 * BT_L2CAP_BR_ERR_NO_RESOURCES.
 *
 * Binding order:
 *   1. Reuse a connection-less pre-registered instance for this address (it
 *      already owns a registered OBEX server; otherwise the OBEX CONNECT would
 *      land on a fresh instance with an empty server list and be rejected with
 *      NOT_FOUND).
 *   2. Otherwise allocate a fresh instance from the pool.
 */
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

/*
 * Secondary (Referenced/Archived Objects) transport handling.
 *
 * The secondary server lives on its own bt_bip (inst->second_bip) so that its
 * OBEX server registration survives a teardown of the primary transport and is
 * reached over a distinct BIP PSM/RFCOMM channel that may overlap in time with
 * the primary transport. The dedicated accept callbacks below therefore bind
 * the incoming secondary transport to inst->second_bip (whose OBEX already owns
 * the secondary server), instead of inst->bip.
 */
static struct bip_app *find_second_server_instance_by_address(const bt_addr_t *address)
{
	for (uint8_t i = 0; i < BIP_MAX_INSTANCES; i++) {
		if (bip_apps[i].in_use && bt_addr_eq(&bip_apps[i].address, address) &&
		    bip_apps[i].second_server._bip == &bip_apps[i].second_bip) {
			return &bip_apps[i];
		}
	}
	return NULL;
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

	if (inst->second_conn != NULL) {
		bt_conn_unref(inst->second_conn);
		inst->second_conn = NULL;
	}
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

	if (inst->second_conn != NULL) {
		bt_conn_unref(inst->second_conn);
		inst->second_conn = NULL;
	}
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

	/* Release any stale secondary transport ref before taking a new one. */
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

/*
 * Auto-Archive shares a single BIP PSM/RFCOMM channel for two OBEX sessions:
 *   1. the initial primary Auto-Archive OBEX CONNECT, and
 *   2. a later secondary Archived-Objects OBEX CONNECT.
 *
 * The primary connect always arrives first, before StartArchive triggers the
 * secondary-server registration (second_server._bip == &second_bip). At that
 * point no secondary server exists for the address, so route to the primary
 * bt_bip. Once the Archived-Objects secondary server is registered, a further
 * transport from the same address carries the Archived-Objects CONNECT, whose
 * target server lives on inst->second_bip; route it there so OBEX target
 * matching succeeds instead of answering Not Found (0xC4) on the primary
 * context.
 */
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

/*
 * Helper to allocate a tx buffer and copy data payload.
 *
 * The PDU must be created from the GOEP transport that will actually send it.
 * Primary-connection responses/requests use inst->bip; the secondary
 * (Referenced/Archived Objects) connection uses inst->second_bip. Callers on
 * the secondary path must pass &inst->second_bip.
 */
static struct net_buf *alloc_buf_with_data_bip(struct bt_bip *bip, const uint8_t *data,
					       uint16_t data_len)
{
	struct net_buf *buf;

	buf = bt_goep_create_pdu(&bip->goep, &bip_tx_pool);
	if (buf == NULL) {
		return NULL;
	}

	if (data_len > 0 && data != NULL) {
		if (net_buf_tailroom(buf) < data_len) {
			net_buf_unref(buf);
			return NULL;
		}
		net_buf_add_mem(buf, data, data_len);
	}

	return buf;
}

/* BTP command handlers */

/*
 * The variable-length BIP commands are registered with
 * BTP_HANDLER_LENGTH_VARIABLE, so the framework skips its own length check and
 * each handler has to validate cmd_len itself. cp->data_len is redundant with
 * cmd_len: reject the command unless the two agree, otherwise a too-large
 * data_len makes the handler read past the received command and put whatever
 * follows it in memory on the wire.
 *
 * The length of the fixed part is checked first so that cp->data_len itself is
 * known to be within the received data before it is read.
 */
#define BIP_CHECK_VAR_LEN(_cp, _cmd_len)                                                       \
	do {                                                                                   \
		if ((_cmd_len) < sizeof(*(_cp))) {                                             \
			LOG_ERR("BIP cmd too short: %u < %zu", (_cmd_len), sizeof(*(_cp)));    \
			return BTP_STATUS_FAILED;                                              \
		}                                                                              \
		if ((_cmd_len) != sizeof(*(_cp)) + sys_le16_to_cpu((_cp)->data_len)) {         \
			LOG_ERR("BIP cmd len mismatch: cmd_len %u, data_len %u", (_cmd_len),   \
				sys_le16_to_cpu((_cp)->data_len));                             \
			return BTP_STATUS_FAILED;                                              \
		}                                                                              \
	} while (0)

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

	if (cp->address.type != BTP_BR_ADDRESS_TYPE) {
		return BTP_STATUS_FAILED;
	}

	conn = bt_conn_lookup_addr_br(&cp->address.a);
	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	/*
	 * If a secondary (Referenced/Archived Objects) server is already
	 * registered for this peer, this actively-opened transport carries the
	 * secondary OBEX connection: bind it to the existing instance's
	 * inst->second_bip (which owns the secondary server) so that the
	 * subsequent second_connect() issues the OBEX CONNECT on the same
	 * bt_bip. This mirrors the accept-side routing in
	 * archive_rfcomm_accept()/refobj_rfcomm_accept(). Using a fresh
	 * inst->bip here would set that context's role to INITIATOR and leave
	 * inst->second_bip without a transport, making second_connect() fail
	 * with "Invalid role initiator".
	 */
	inst = find_second_server_instance_by_address(&cp->address.a);
	if (inst != NULL) {
		/* Release any stale secondary transport ref before taking a new one. */
		bt_conn_drop(&inst->second_conn);
		inst->second_conn = bt_conn_ref(conn);
		inst->second_bip.ops = &bip_second_rfcomm_transport_ops;

		err = bt_bip_rfcomm_connect(conn, &inst->second_bip, cp->channel);
		bt_conn_unref(conn);
		if (err != 0) {
			bt_conn_unref(inst->second_conn);
			inst->second_conn = NULL;
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

	if (cp->address.type != BTP_BR_ADDRESS_TYPE) {
		return BTP_STATUS_FAILED;
	}

	conn = bt_conn_lookup_addr_br(&cp->address.a);
	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	/*
	 * If a secondary server instance already exists for this peer, this
	 * L2CAP connect is opening the transport for the secondary (Referenced
	 * or Archived Objects) OBEX connection. Bind it to the dedicated
	 * second_bip so its role is kept separate from the primary bip. Binding
	 * to inst->bip would set that bip's role to INITIATOR and the host would
	 * later reject the secondary OBEX CONNECT with "Invalid role initiator".
	 */
	inst = find_second_server_instance_by_address(&cp->address.a);
	if (inst != NULL) {
		/* Release any stale secondary transport ref before taking a new one. */
		bt_conn_drop(&inst->second_conn);
		inst->second_conn = bt_conn_ref(conn);
		inst->second_bip.ops = &bip_second_l2cap_transport_ops;
		err = bt_bip_l2cap_connect(conn, &inst->second_bip, sys_le16_to_cpu(cp->psm));
		bt_conn_unref(conn);
		if (err != 0) {
			bt_conn_unref(inst->second_conn);
			inst->second_conn = NULL;
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
	}

	if (err != 0) {
		bip_instance_free(inst);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

/*
 * Resolve the bip_app instance that a secondary transport connect belongs to.
 *
 * In the Auto-Archive (AAI/ACH) initiator flow the primary transport has
 * already been torn down, so the instance is in the connection-less
 * pre-registered state (conn == NULL); prefer that. Otherwise (secondary
 * transport opened while the primary transport is still up) fall back to the
 * connected instance.
 */
static struct bip_app *find_second_connect_instance(const bt_addr_t *address)
{
	struct bip_app *inst;

	inst = find_preregistered_instance_by_address(address);
	if (inst != NULL) {
		return inst;
	}

	return find_instance_by_address(address);
}

/*
 * Open the transport for the secondary (Referenced/Archived Objects) OBEX
 * connection that the IUT itself initiates as an Initiator. Bind it to the
 * dedicated inst->second_bip so its role becomes INITIATOR, then the following
 * second_connect() issues the OBEX CONNECT on the same second_bip. Binding to
 * inst->bip would leave second_bip without a transport (role 0) and
 * second_connect() would be rejected with "Invalid role".
 */
static uint8_t second_connect_l2cap(const void *cmd, uint16_t cmd_len, void *rsp,
				    uint16_t *rsp_len)
{
	const struct btp_bip_second_connect_l2cap_cmd *cp = cmd;
	struct bt_conn *conn;
	struct bip_app *inst;
	int err;

	if (cp->address.type != BTP_BR_ADDRESS_TYPE) {
		return BTP_STATUS_FAILED;
	}

	conn = bt_conn_lookup_addr_br(&cp->address.a);
	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	inst = find_second_connect_instance(&cp->address.a);
	if (inst == NULL) {
		bt_conn_unref(conn);
		return BTP_STATUS_FAILED;
	}

	if (inst->conn == NULL) {
		inst->conn = bt_conn_ref(conn);
		bt_addr_copy(&inst->address, &cp->address.a);
	}
	/* Release any stale secondary transport ref before taking a new one. */
	bt_conn_drop(&inst->second_conn);
	inst->second_conn = bt_conn_ref(conn);
	inst->second_bip.ops = &bip_second_l2cap_transport_ops;

	err = bt_bip_l2cap_connect(conn, &inst->second_bip, sys_le16_to_cpu(cp->psm));
	bt_conn_unref(conn);
	if (err != 0) {
		bt_conn_unref(inst->second_conn);
		inst->second_conn = NULL;
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

	if (cp->address.type != BTP_BR_ADDRESS_TYPE) {
		return BTP_STATUS_FAILED;
	}

	conn = bt_conn_lookup_addr_br(&cp->address.a);
	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	inst = find_second_connect_instance(&cp->address.a);
	if (inst == NULL) {
		bt_conn_unref(conn);
		return BTP_STATUS_FAILED;
	}

	if (inst->conn == NULL) {
		inst->conn = bt_conn_ref(conn);
		bt_addr_copy(&inst->address, &cp->address.a);
	}
	/* Release any stale secondary transport ref before taking a new one. */
	bt_conn_drop(&inst->second_conn);
	inst->second_conn = bt_conn_ref(conn);
	inst->second_bip.ops = &bip_second_rfcomm_transport_ops;

	err = bt_bip_rfcomm_connect(conn, &inst->second_bip, cp->channel);
	bt_conn_unref(conn);
	if (err != 0) {
		bt_conn_unref(inst->second_conn);
		inst->second_conn = NULL;
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

	if (cp->address.type != BTP_BR_ADDRESS_TYPE) {
		return BTP_STATUS_FAILED;
	}

	inst = find_second_connect_instance(&cp->address.a);
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

	if (cp->address.type != BTP_BR_ADDRESS_TYPE) {
		return BTP_STATUS_FAILED;
	}

	inst = find_second_connect_instance(&cp->address.a);
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
	static struct bt_uuid_16 uuid;
	struct bt_conn *conn;
	int err;

	if (cp->address.type != BTP_BR_ADDRESS_TYPE) {
		return BTP_STATUS_FAILED;
	}

	conn = bt_conn_lookup_addr_br(&cp->address.a);
	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	uuid.uuid.type = BT_UUID_TYPE_16;
	uuid.val = sys_le16_to_cpu(cp->uuid);
	sdp_bip_params.uuid = &uuid.uuid;
	sdp_bip_params.func = bip_discover_func;
	sdp_bip_params.pool = &bip_sdp_pool;
	sdp_bip_params.type = BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR;

	err = bt_sdp_discover(conn, &sdp_bip_params);
	bt_conn_unref(conn);

	if (err != 0) {
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

	if (cp->address.type != BTP_BR_ADDRESS_TYPE) {
		return BTP_STATUS_FAILED;
	}

	if (type >= ARRAY_SIZE(bip_uuids) || bip_uuids[type] == NULL) {
		return BTP_STATUS_FAILED;
	}
	u = bip_uuids[type];

	/*
	 * Reuse an existing instance for this address instead of unconditionally
	 * allocating a fresh slot: a repeated register (or a register after the
	 * peer already connected) would otherwise create a second instance for the
	 * same address, and find_instance_by_address() would then route *_RSP
	 * commands to the wrong one.
	 */
	inst = find_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		inst = find_preregistered_instance_by_address(&cp->address.a);
	}

	if (inst != NULL && inst->server._bip == &inst->bip) {
		/* Primary server already registered for this address: idempotent. */
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

	return BTP_STATUS_SUCCESS;
}

static uint8_t server_unregister(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_server_unregister_cmd *cp = cmd;
	struct bip_app *inst;
	int err;

	/*
	 * The server may have been registered before the peer connected, so it
	 * can still be connection-less; fall back to the pre-registered lookup
	 * when no transport-carrying instance matches the address.
	 */
	inst = find_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		inst = find_preregistered_instance_by_address(&cp->address.a);
	}
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_server_unregister(&inst->server);
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	/*
	 * bt_bip_server_unregister() leaves server._bip pointing at inst->bip,
	 * so clear it here to let bip_instance_is_preregistered() (and thus the
	 * garbage collector) see that this server is gone.
	 */
	inst->server._bip = NULL;
	bip_instance_gc(inst);

	return BTP_STATUS_SUCCESS;
}

static uint8_t client_connect(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_bip_client_connect_cmd *cp = cmd;
	enum bt_bip_conn_type type = cp->conn_type;
	struct bip_app *inst;
	int err;

	inst = find_instance_by_address(&cp->address.a);
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

	inst = find_instance_by_address(&cp->address.a);
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

	inst = find_instance_by_address(&cp->address.a);
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

	BIP_CHECK_VAR_LEN(cp, cmd_len);
	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_instance_by_address(&cp->address.a);
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

	inst = find_instance_by_address(&cp->address.a);
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

	inst = find_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_abort_rsp(&inst->server, cp->rsp_code, NULL);
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

/* Client operation handlers - each sends data in a net_buf */
#define BIP_CLIENT_OP_HANDLER(hname)                                                          \
	static uint8_t hname(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len) \
	{                                                                                     \
		const struct btp_bip_##hname##_cmd *cp = cmd;                                 \
		BIP_CHECK_VAR_LEN(cp, cmd_len);                                               \
		uint16_t data_len = sys_le16_to_cpu(cp->data_len);                            \
		struct bip_app *inst;                                                         \
		struct net_buf *buf;                                                          \
		int err;                                                                      \
		inst = find_instance_by_address(&cp->address.a);                              \
		if (inst == NULL) {                                                           \
			return BTP_STATUS_FAILED;                                             \
		}                                                                             \
		buf = alloc_buf_with_data_bip(&inst->bip, cp->data, data_len);                          \
		if (buf == NULL) {                                                            \
			return BTP_STATUS_FAILED;                                             \
		}                                                                             \
		err = bt_bip_##hname(&inst->client, cp->final, buf);                          \
		if (err != 0) {                                                               \
			net_buf_unref(buf);                                                   \
			return BTP_STATUS_FAILED;                                             \
		}                                                                             \
		return BTP_STATUS_SUCCESS;                                                     \
	}

#define BIP_SERVER_RSP_HANDLER(hname)                                                         \
	static uint8_t hname(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len) \
	{                                                                                     \
		const struct btp_bip_##hname##_cmd *cp = cmd;                                 \
		BIP_CHECK_VAR_LEN(cp, cmd_len);                                               \
		uint16_t data_len = sys_le16_to_cpu(cp->data_len);                            \
		struct bip_app *inst;                                                         \
		struct net_buf *buf;                                                          \
		int err;                                                                      \
		inst = find_instance_by_address(&cp->address.a);                              \
		if (inst == NULL) {                                                           \
			return BTP_STATUS_FAILED;                                             \
		}                                                                             \
		buf = alloc_buf_with_data_bip(&inst->bip, cp->data, data_len);                          \
		if (buf == NULL) {                                                            \
			return BTP_STATUS_FAILED;                                             \
		}                                                                             \
		err = bt_bip_##hname(&inst->server, cp->rsp_code, buf);                       \
		if (err != 0) {                                                               \
			net_buf_unref(buf);                                                   \
			return BTP_STATUS_FAILED;                                             \
		}                                                                             \
		return BTP_STATUS_SUCCESS;                                                     \
	}

BIP_CLIENT_OP_HANDLER(get_capabilities)
BIP_SERVER_RSP_HANDLER(get_capabilities_rsp)
BIP_CLIENT_OP_HANDLER(get_image_list)
BIP_SERVER_RSP_HANDLER(get_image_list_rsp)
BIP_CLIENT_OP_HANDLER(get_image_properties)
BIP_SERVER_RSP_HANDLER(get_image_properties_rsp)
BIP_CLIENT_OP_HANDLER(get_image)
BIP_SERVER_RSP_HANDLER(get_image_rsp)
BIP_CLIENT_OP_HANDLER(get_linked_thumbnail)
BIP_SERVER_RSP_HANDLER(get_linked_thumbnail_rsp)
BIP_CLIENT_OP_HANDLER(get_linked_attachment)
BIP_SERVER_RSP_HANDLER(get_linked_attachment_rsp)
BIP_CLIENT_OP_HANDLER(get_partial_image)
/*
 * GetPartialImage is a Referenced-Objects-only operation, i.e. it can only be
 * requested over the secondary OBEX connection. Its response must therefore be
 * sent from the secondary server (&inst->second_server); the generic
 * BIP_SERVER_RSP_HANDLER macro targets the primary server and would send the
 * response on the wrong OBEX connection. This dedicated handler routes it
 * correctly.
 */
static uint8_t get_partial_image_rsp(const void *cmd, uint16_t cmd_len, void *rsp,
				     uint16_t *rsp_len)
{
	const struct btp_bip_get_partial_image_rsp_cmd *cp = cmd;
	uint16_t data_len;
	struct bip_app *inst;
	struct net_buf *buf;
	int err;

	BIP_CHECK_VAR_LEN(cp, cmd_len);
	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_instance_by_address(&cp->address.a);
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

/*
 * Client request handlers for the secondary (Referenced/Archived Objects) OBEX
 * connection. The request must be issued from the secondary client
 * (&inst->second_client) over the secondary transport (&inst->second_bip); the
 * generic BIP_CLIENT_OP_HANDLER macro targets the primary client and would send
 * the request on the wrong OBEX connection. This macro routes it correctly.
 *
 * "hname" is the generated handler name (e.g. second_get_image_list); "apiname"
 * is the bt_bip client API to invoke (e.g. bt_bip_get_image_list).
 */
#define BIP_SECOND_CLIENT_OP_HANDLER(hname, apiname)                                          \
	static uint8_t hname(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len) \
	{                                                                                     \
		const struct btp_bip_##hname##_cmd *cp = cmd;                                 \
		BIP_CHECK_VAR_LEN(cp, cmd_len);                                               \
		uint16_t data_len = sys_le16_to_cpu(cp->data_len);                            \
		struct bip_app *inst;                                                         \
		struct net_buf *buf;                                                          \
		int err;                                                                      \
		inst = find_instance_by_address(&cp->address.a);                              \
		if (inst == NULL) {                                                           \
			return BTP_STATUS_FAILED;                                             \
		}                                                                             \
		buf = alloc_buf_with_data_bip(&inst->second_bip, cp->data, data_len);         \
		if (buf == NULL) {                                                            \
			return BTP_STATUS_FAILED;                                             \
		}                                                                             \
		err = apiname(&inst->second_client, cp->final, buf);                          \
		if (err != 0) {                                                               \
			net_buf_unref(buf);                                                   \
			return BTP_STATUS_FAILED;                                             \
		}                                                                             \
		return BTP_STATUS_SUCCESS;                                                     \
	}

BIP_SECOND_CLIENT_OP_HANDLER(second_get_capabilities, bt_bip_get_capabilities)
BIP_SECOND_CLIENT_OP_HANDLER(second_get_image_list, bt_bip_get_image_list)
BIP_SECOND_CLIENT_OP_HANDLER(second_get_image_properties, bt_bip_get_image_properties)
BIP_SECOND_CLIENT_OP_HANDLER(second_get_image, bt_bip_get_image)
BIP_SECOND_CLIENT_OP_HANDLER(second_get_linked_thumbnail, bt_bip_get_linked_thumbnail)
BIP_SECOND_CLIENT_OP_HANDLER(second_get_linked_attachment, bt_bip_get_linked_attachment)
BIP_SECOND_CLIENT_OP_HANDLER(second_get_partial_image, bt_bip_get_partial_image)
BIP_SECOND_CLIENT_OP_HANDLER(second_delete_image, bt_bip_delete_image)

BIP_CLIENT_OP_HANDLER(get_monitoring_image)

BIP_SERVER_RSP_HANDLER(get_monitoring_image_rsp)
BIP_CLIENT_OP_HANDLER(get_status)
BIP_SERVER_RSP_HANDLER(get_status_rsp)
BIP_CLIENT_OP_HANDLER(put_image)
BIP_SERVER_RSP_HANDLER(put_image_rsp)
BIP_CLIENT_OP_HANDLER(put_linked_thumbnail)
BIP_SERVER_RSP_HANDLER(put_linked_thumbnail_rsp)
BIP_CLIENT_OP_HANDLER(put_linked_attachment)
BIP_SERVER_RSP_HANDLER(put_linked_attachment_rsp)
BIP_CLIENT_OP_HANDLER(remote_display)
BIP_SERVER_RSP_HANDLER(remote_display_rsp)
BIP_CLIENT_OP_HANDLER(delete_image)
BIP_SERVER_RSP_HANDLER(delete_image_rsp)
BIP_CLIENT_OP_HANDLER(start_print)
BIP_SERVER_RSP_HANDLER(start_print_rsp)
BIP_CLIENT_OP_HANDLER(start_archive)
BIP_SERVER_RSP_HANDLER(start_archive_rsp)

/* Secondary connection UUID mapping (indices 10 and 11 only) */
/*
 * These UUID pointers must have static storage duration. The BT_BIP_UUID_*
 * macros expand to a compound literal; when such a literal is used inside a
 * function body it has automatic storage and becomes dangling once the
 * function returns (the server would then be registered with an all-zero
 * UUID and OBEX CONNECT matching by target would fail with NOT_FOUND).
 * Declaring them at file scope gives the compound literals static storage.
 */
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

	/*
	 * A secondary (Referenced/Archived Objects) server must be registered on
	 * the same instance that already owns the connected primary client. The
	 * host stack requires an existing primary client (client._bip != NULL)
	 * before a secondary server can be registered, so reuse the connected
	 * instance for this peer instead of allocating a fresh, empty one.
	 */
	inst = find_instance_by_address(&cp->address.a);
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

	return BTP_STATUS_SUCCESS;
}

static uint8_t second_server_unregister(const void *cmd, uint16_t cmd_len, void *rsp,
					uint16_t *rsp_len)
{
	const struct btp_bip_second_server_unregister_cmd *cp = cmd;
	struct bip_app *inst;
	int err;

	/*
	 * The secondary server is keyed on the address and may be
	 * connection-less; find_second_server_instance_by_address() locates it
	 * by its second_server binding rather than requiring a live transport.
	 */
	inst = find_second_server_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_server_unregister(&inst->second_server);
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	/* See server_unregister(): clear the stale _bip before collecting. */
	inst->second_server._bip = NULL;
	bip_instance_gc(inst);

	return BTP_STATUS_SUCCESS;
}

#define BIP_2ND_CONN_TYPE_ARCHIVED_OBJECTS

static uint8_t second_connect(const void *cmd, uint16_t cmd_len, void *rsp,
				     uint16_t *rsp_len)
{
	const struct btp_bip_second_connect_cmd *cp = cmd;
	enum bt_bip_conn_type type = cp->conn_type;
	struct bip_app *inst;
	int err;

	inst = find_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	/*
	 * The secondary (Referenced/Archived Objects) OBEX connection must be
	 * carried on the dedicated inst->second_bip, not the primary inst->bip.
	 * The secondary transport opened in connect_l2cap()/connect_rfcomm() is
	 * bound to inst->second_bip, and the secondary server responses use
	 * inst->second_server / inst->second_bip too, so the client connect must
	 * be issued on the same bt_bip. Using inst->bip here would reuse the
	 * primary context (whose role was set to INITIATOR by the primary
	 * transport connect) and be rejected by the host with "Invalid role
	 * initiator".
	 */
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

	inst = find_instance_by_address(&cp->address.a);
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

	inst = find_instance_by_address(&cp->address.a);
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

	BIP_CHECK_VAR_LEN(cp, cmd_len);
	data_len = sys_le16_to_cpu(cp->data_len);

	inst = find_instance_by_address(&cp->address.a);
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

	inst = find_instance_by_address(&cp->address.a);
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

	inst = find_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bip_abort_rsp(&inst->second_server, cp->rsp_code, NULL);
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

/*
 * Secondary-server response handlers for the Archived-Objects operations.
 *
 * These responses must be sent from the secondary server (&inst->second_server)
 * over the secondary OBEX connection (&inst->second_bip), analogous to
 * get_partial_image_rsp above. The generic BIP_SERVER_RSP_HANDLER macro targets
 * the primary server and would route the response on the wrong OBEX connection.
 */
#define BIP_SECOND_SERVER_RSP_HANDLER(hname, apiname)                                         \
	static uint8_t hname(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len) \
	{                                                                                     \
		const struct btp_bip_##hname##_cmd *cp = cmd;                                 \
		BIP_CHECK_VAR_LEN(cp, cmd_len);                                               \
		uint16_t data_len = sys_le16_to_cpu(cp->data_len);                            \
		struct bip_app *inst;                                                         \
		struct net_buf *buf;                                                          \
		int err;                                                                      \
		inst = find_instance_by_address(&cp->address.a);                              \
		if (inst == NULL) {                                                           \
			return BTP_STATUS_FAILED;                                             \
		}                                                                             \
		buf = alloc_buf_with_data_bip(&inst->second_bip, cp->data, data_len);         \
		if (buf == NULL) {                                                            \
			return BTP_STATUS_FAILED;                                             \
		}                                                                             \
		err = apiname(&inst->second_server, cp->rsp_code, buf);                       \
		if (err != 0) {                                                               \
			net_buf_unref(buf);                                                   \
			return BTP_STATUS_FAILED;                                             \
		}                                                                             \
		return BTP_STATUS_SUCCESS;                                                     \
	}

BIP_SECOND_SERVER_RSP_HANDLER(second_get_capabilities_rsp, bt_bip_get_capabilities_rsp)
BIP_SECOND_SERVER_RSP_HANDLER(second_get_image_list_rsp, bt_bip_get_image_list_rsp)
BIP_SECOND_SERVER_RSP_HANDLER(second_get_image_properties_rsp, bt_bip_get_image_properties_rsp)
BIP_SECOND_SERVER_RSP_HANDLER(second_get_image_rsp, bt_bip_get_image_rsp)
BIP_SECOND_SERVER_RSP_HANDLER(second_get_linked_thumbnail_rsp, bt_bip_get_linked_thumbnail_rsp)
BIP_SECOND_SERVER_RSP_HANDLER(second_get_linked_attachment_rsp, bt_bip_get_linked_attachment_rsp)
BIP_SECOND_SERVER_RSP_HANDLER(second_delete_image_rsp, bt_bip_delete_image_rsp)

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
	for (uint8_t i = 0; i < BIP_MAX_INSTANCES; i++) {
		if (bip_apps[i].in_use) {
			bip_instance_free(&bip_apps[i]);
		}
	}

	return BTP_STATUS_SUCCESS;
}
