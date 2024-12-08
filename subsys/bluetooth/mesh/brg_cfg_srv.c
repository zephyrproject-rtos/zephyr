/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/mesh.h>
#include <zephyr/bluetooth/mesh/brg_cfg.h>
#include "access.h"
#include "brg_cfg.h"
#include "foundation.h"
#include "subnet.h"

#define LOG_LEVEL CONFIG_BT_MESH_MODEL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_brg_cfg_srv);

static void bridge_status_send(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_SUBNET_BRIDGE_STATUS, 1);

	bt_mesh_model_msg_init(&msg, OP_SUBNET_BRIDGE_STATUS);
	net_buf_simple_add_u8(&msg, bt_mesh_brg_cfg_enable_get() ? 1 : 0);

	if (bt_mesh_model_send(model, ctx, &msg, NULL, NULL)) {
		LOG_ERR("Brg Status send failed");
	}
}

static int subnet_bridge_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	bridge_status_send(model, ctx);

	return 0;
}

static int subnet_bridge_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	uint8_t enable = net_buf_simple_pull_u8(buf);

	if (enable > BT_MESH_BRG_CFG_ENABLED) {
		return -EINVAL;
	}

	bt_mesh_brg_cfg_enable_set(enable);
	bridge_status_send(model, ctx);

	return 0;
}

static void bridging_table_status_send(const struct bt_mesh_model *model,
				       struct bt_mesh_msg_ctx *ctx, uint8_t status,
				       struct bt_mesh_brg_cfg_table_entry *entry)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_BRIDGING_TABLE_STATUS, 9);

	bt_mesh_model_msg_init(&msg, OP_BRIDGING_TABLE_STATUS);
	net_buf_simple_add_u8(&msg, status);
	net_buf_simple_add_u8(&msg, entry->directions);
	key_idx_pack_pair(&msg, entry->net_idx1, entry->net_idx2);
	net_buf_simple_add_le16(&msg, entry->addr1);
	net_buf_simple_add_le16(&msg, entry->addr2);

	if (bt_mesh_model_send(model, ctx, &msg, NULL, NULL)) {
		LOG_ERR("Brg Tbl Status send failed");
	}
}

static bool netkey_check(uint16_t net_idx1, uint16_t net_idx2)
{
	return bt_mesh_subnet_get(net_idx1) && bt_mesh_subnet_get(net_idx2);
}

static int bridging_table_add(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	struct bt_mesh_brg_cfg_table_entry entry;
	uint8_t status = STATUS_SUCCESS;
	int err;

	entry.directions = net_buf_simple_pull_u8(buf);
	key_idx_unpack_pair(buf, &entry.net_idx1, &entry.net_idx2);
	entry.addr1 = net_buf_simple_pull_le16(buf);
	entry.addr2 = net_buf_simple_pull_le16(buf);

	err = bt_mesh_brg_cfg_tbl_add(entry.directions, entry.net_idx1, entry.net_idx2, entry.addr1,
				      entry.addr2, &status);
	if (err) {
		return err;
	}

	bridging_table_status_send(model, ctx, status, &entry);

	return 0;
}

static int bridging_table_remove(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				 struct net_buf_simple *buf)
{
	struct bt_mesh_brg_cfg_table_entry entry;
	uint8_t status = STATUS_SUCCESS;
	int err;

	entry.directions = 0;
	key_idx_unpack_pair(buf, &entry.net_idx1, &entry.net_idx2);
	entry.addr1 = net_buf_simple_pull_le16(buf);
	entry.addr2 = net_buf_simple_pull_le16(buf);

	err = bt_mesh_brg_cfg_tbl_remove(entry.net_idx1, entry.net_idx2, entry.addr1, entry.addr2,
					 &status);
	if (err) {
		return err;
	}

	bridging_table_status_send(model, ctx, status, &entry);

	return 0;
}

static int bridged_subnets_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_BRIDGED_SUBNETS_LIST,
				 BT_MESH_TX_SDU_MAX -
					 BT_MESH_MODEL_OP_LEN(OP_BRIDGED_SUBNETS_LIST));
	bt_mesh_model_msg_init(&msg, OP_BRIDGED_SUBNETS_LIST);

	const struct bt_mesh_brg_cfg_row *brg_tbl;
	int rows = bt_mesh_brg_cfg_tbl_get(&brg_tbl);
	int16_t net_idx_filter = net_buf_simple_pull_le16(buf);

	if (net_idx_filter & BT_MESH_BRG_CFG_NKEY_PRHB_FLT_MASK) {
		return -EINVAL;
	}

	struct bt_mesh_brg_cfg_filter_netkey filter_net_idx;

	filter_net_idx.filter = net_idx_filter & BIT_MASK(2);
	filter_net_idx.net_idx = (net_idx_filter >> 4) & BIT_MASK(12);

	uint8_t start_id = net_buf_simple_pull_u8(buf);

	net_buf_simple_add_le16(&msg, net_idx_filter);
	net_buf_simple_add_u8(&msg, start_id);

	uint8_t cnt = 0;
	uint16_t net_idx1, net_idx2;

	for (int i = 0; i < rows; i++) {
		net_idx1 = brg_tbl[i].net_idx1;
		net_idx2 = brg_tbl[i].net_idx2;

		if (net_buf_simple_tailroom(&msg) < 3 + BT_MESH_MIC_SHORT) {
			break;
		}

		switch (filter_net_idx.filter) {
		/* Report pair of NetKeys from the table, starting from start_id. */
		case 0:
			if (i >= start_id) {
				key_idx_pack_pair(&msg, net_idx1, net_idx2);
			}
			break;

		/* Report pair of NetKeys in which (NetKeyIndex1) matches the net_idx */
		case 1:
			if (net_idx1 == filter_net_idx.net_idx) {
				if (cnt >= start_id) {
					key_idx_pack_pair(&msg, net_idx1, net_idx2);
				}
				cnt++;
			}
			break;

		/* Report pair of NetKeys in which (NetKeyIndex2) matches the net_idx */
		case 2:
			if (net_idx2 == filter_net_idx.net_idx) {
				if (cnt >= start_id) {
					key_idx_pack_pair(&msg, net_idx1, net_idx2);
				}
				cnt++;
			}
			break;

		/* Report pair of NetKeys in which (NetKeyIndex1 or NetKeyIndex2) matches the
		 * net_idx
		 */
		case 3:
			if (net_idx1 == filter_net_idx.net_idx ||
			    net_idx2 == filter_net_idx.net_idx) {
				if (cnt >= start_id) {
					key_idx_pack_pair(&msg, net_idx1, net_idx2);
				}
				cnt++;
			}
			break;

		default:
			CODE_UNREACHABLE;
		}
	}

	if (bt_mesh_model_send(model, ctx, &msg, NULL, NULL)) {
		LOG_ERR("Brg Subnet List send failed");
	}

	return 0;
}

static int bridging_table_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_BRIDGING_TABLE_LIST,
				 BT_MESH_TX_SDU_MAX - BT_MESH_MODEL_OP_LEN(OP_BRIDGING_TABLE_LIST));
	uint8_t status = STATUS_SUCCESS;
	uint16_t net_idx1, net_idx2;

	bt_mesh_model_msg_init(&msg, OP_BRIDGING_TABLE_LIST);

	key_idx_unpack_pair(buf, &net_idx1, &net_idx2);

	uint16_t start_id = net_buf_simple_pull_le16(buf);

	if (!netkey_check(net_idx1, net_idx2)) {
		status = STATUS_INVALID_NETKEY;
	}

	net_buf_simple_add_u8(&msg, status);
	key_idx_pack_pair(&msg, net_idx1, net_idx2);
	net_buf_simple_add_le16(&msg, start_id);

	if (status != STATUS_SUCCESS) {
		goto tbl_get_respond;
	}

	int cnt = 0;
	const struct bt_mesh_brg_cfg_row *brg_tbl;
	int rows = bt_mesh_brg_cfg_tbl_get(&brg_tbl);

	for (int i = 0; i < rows; i++) {
		if (brg_tbl[i].net_idx1 == net_idx1 && brg_tbl[i].net_idx2 == net_idx2) {
			if (cnt >= start_id) {
				if (net_buf_simple_tailroom(&msg) < 5 + BT_MESH_MIC_SHORT) {
					LOG_WRN("Bridging Table List message too large");
					break;
				}

				net_buf_simple_add_le16(&msg, brg_tbl[i].addr1);
				net_buf_simple_add_le16(&msg, brg_tbl[i].addr2);
				net_buf_simple_add_u8(&msg, brg_tbl[i].direction);
			}
			cnt++;
		}
	}

tbl_get_respond:
	if (bt_mesh_model_send(model, ctx, &msg, NULL, NULL)) {
		LOG_ERR("Brg Tbl List send failed");
	}

	return 0;
}

static int bridging_table_size_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				   struct net_buf_simple *buf)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_BRIDGING_TABLE_SIZE_STATUS, 2);
	bt_mesh_model_msg_init(&msg, OP_BRIDGING_TABLE_SIZE_STATUS);

	net_buf_simple_add_le16(&msg, CONFIG_BT_MESH_BRG_TABLE_ITEMS_MAX);

	if (bt_mesh_model_send(model, ctx, &msg, NULL, NULL)) {
		LOG_ERR("Brg Tbl Size Status send failed");
	}

	return 0;
}

const struct bt_mesh_model_op _bt_mesh_brg_cfg_srv_op[] = {
	{OP_SUBNET_BRIDGE_GET, BT_MESH_LEN_EXACT(0), subnet_bridge_get},
	{OP_SUBNET_BRIDGE_SET, BT_MESH_LEN_EXACT(1), subnet_bridge_set},
	{OP_BRIDGING_TABLE_ADD, BT_MESH_LEN_EXACT(8), bridging_table_add},
	{OP_BRIDGING_TABLE_REMOVE, BT_MESH_LEN_EXACT(7), bridging_table_remove},
	{OP_BRIDGED_SUBNETS_GET, BT_MESH_LEN_EXACT(3), bridged_subnets_get},
	{OP_BRIDGING_TABLE_GET, BT_MESH_LEN_EXACT(5), bridging_table_get},
	{OP_BRIDGING_TABLE_SIZE_GET, BT_MESH_LEN_EXACT(0), bridging_table_size_get},
	BT_MESH_MODEL_OP_END,
};

static int brg_cfg_srv_init(const struct bt_mesh_model *model)
{
	const struct bt_mesh_model *config_srv =
		bt_mesh_model_find(bt_mesh_model_elem(model), BT_MESH_MODEL_ID_CFG_SRV);

	if (config_srv == NULL) {
		LOG_ERR("Not on primary element");
		return -EINVAL;
	}

	/*
	 * Bridge Configuration Server model security is device key based and only the local
	 * device key is allowed to access this model.
	 */
	model->keys[0] = BT_MESH_KEY_DEV_LOCAL;
	model->rt->flags |= BT_MESH_MOD_DEVKEY_ONLY;

	bt_mesh_model_extend(model, config_srv);

	return 0;
}

void brg_cfg_srv_reset(const struct bt_mesh_model *model)
{
	bt_mesh_brg_cfg_tbl_reset();
}

const struct bt_mesh_model_cb _bt_mesh_brg_cfg_srv_cb = {
	.init = brg_cfg_srv_init,
	.reset = brg_cfg_srv_reset,
};
