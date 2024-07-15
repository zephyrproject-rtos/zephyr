/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/mesh.h>
#include "access.h"
#include "foundation.h"
#include "msg.h"

#define LOG_LEVEL CONFIG_BT_MESH_MODEL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_brg_cfg_cli);

static int32_t msg_timeout;

static struct bt_mesh_brg_cfg_cli *cli;

static int subnet_bridge_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	enum bt_mesh_subnet_bridge_state status =
		(enum bt_mesh_subnet_bridge_state)net_buf_simple_pull_u8(buf);
	enum bt_mesh_subnet_bridge_state *rsp;

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, OP_SUBNET_BRIDGE_STATUS, ctx->addr,
				      (void **)&rsp)) {
		*rsp = status;
		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}

	if (cli->cb && cli->cb->subnet_bridge_status) {
		cli->cb->subnet_bridge_status(cli, ctx->addr, status);
	}
	return 0;
}

static int bridging_table_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	struct bt_mesh_bridging_table_status table_status;
	struct bt_mesh_bridging_table_status *rsp;

	table_status.status = net_buf_simple_pull_u8(buf);
	table_status.entry.directions = net_buf_simple_pull_u8(buf);
	key_idx_unpack_pair(buf, &table_status.entry.net_idx1, &table_status.entry.net_idx2);
	table_status.entry.addr1 = net_buf_simple_pull_le16(buf);
	table_status.entry.addr2 = net_buf_simple_pull_le16(buf);

	if (!(table_status.entry.addr1 == BT_MESH_ADDR_UNASSIGNED ||
	     BT_MESH_ADDR_IS_UNICAST(table_status.entry.addr1))) {
		LOG_ERR("addr1 shall be a unicast address or unassigned.");
		return -EINVAL;
	} else if (table_status.entry.addr2 == BT_MESH_ADDR_ALL_NODES) {
		LOG_ERR("addr2 shall not be the all-nodes fixed group address.");
		return -EINVAL;
	}

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, OP_BRIDGING_TABLE_STATUS, ctx->addr,
				      (void **)&rsp)) {
		*rsp = table_status;
		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}

	if (cli->cb && cli->cb->bridging_table_status) {
		cli->cb->bridging_table_status(cli, ctx->addr, &table_status);
	}
	return 0;
}

static int bridged_subnets_list(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	struct bt_mesh_bridged_subnets_list subnets_list;
	struct bt_mesh_bridged_subnets_list *rsp;
	uint16_t net_idx_filter;

	net_idx_filter = net_buf_simple_pull_le16(buf);
	subnets_list.net_idx_filter.filter = net_idx_filter & BIT_MASK(2);
	subnets_list.net_idx_filter.net_idx = (net_idx_filter >> 4) & BIT_MASK(12);
	subnets_list.start_idx = net_buf_simple_pull_u8(buf);

	if (buf->len && !(buf->len % 3)) {
		subnets_list.list = buf;
	} else {
		subnets_list.list = NULL;
	}

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, OP_BRIDGED_SUBNETS_LIST, ctx->addr,
				      (void **)&rsp)) {
		*rsp = subnets_list;
		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}

	if (cli->cb && cli->cb->bridged_subnets_list) {
		cli->cb->bridged_subnets_list(cli, ctx->addr, &subnets_list);
	}
	return 0;
}

static int bridging_table_list(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	struct bt_mesh_bridging_table_list table_list;
	struct bt_mesh_bridging_table_list *rsp;

	table_list.status = net_buf_simple_pull_u8(buf);
	key_idx_unpack_pair(buf, &table_list.net_idx1, &table_list.net_idx2);
	table_list.start_idx = net_buf_simple_pull_le16(buf);

	if ((table_list.status == STATUS_SUCCESS) && buf->len && !(buf->len % 5)) {
		table_list.list = buf;
	} else {
		table_list.list = NULL;
	}

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, OP_BRIDGING_TABLE_LIST, ctx->addr,
				      (void **)&rsp)) {
		*rsp = table_list;
		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}

	if (cli->cb && cli->cb->bridging_table_list) {
		cli->cb->bridging_table_list(cli, ctx->addr, &table_list);
	}
	return 0;
}

static int bridging_table_size_status(const struct bt_mesh_model *model,
				      struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
	uint16_t size = net_buf_simple_pull_le16(buf);
	uint16_t *rsp;

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, OP_BRIDGING_TABLE_SIZE_STATUS, ctx->addr,
				      (void **)&rsp)) {
		*rsp = size;
		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}

	if (cli->cb && cli->cb->bridging_table_size_status) {
		cli->cb->bridging_table_size_status(cli, ctx->addr, size);
	}
	return 0;
}

const struct bt_mesh_model_op _bt_mesh_brg_cfg_cli_op[] = {
	{ OP_SUBNET_BRIDGE_STATUS,        BT_MESH_LEN_EXACT(1),  subnet_bridge_status },
	{ OP_BRIDGING_TABLE_STATUS,       BT_MESH_LEN_EXACT(9),  bridging_table_status },
	{ OP_BRIDGED_SUBNETS_LIST,        BT_MESH_LEN_MIN(3),    bridged_subnets_list },
	{ OP_BRIDGING_TABLE_LIST,         BT_MESH_LEN_MIN(6),    bridging_table_list },
	{ OP_BRIDGING_TABLE_SIZE_STATUS,  BT_MESH_LEN_EXACT(2),  bridging_table_size_status },
	BT_MESH_MODEL_OP_END,
};

static int brg_cfg_cli_init(const struct bt_mesh_model *model)
{
	if (!bt_mesh_model_in_primary(model)) {
		LOG_ERR("Bridge Configuration Client only allowed in primary element");
		return -EINVAL;
	}

	if (!model->rt->user_data) {
		LOG_ERR("No Bridge Configuration Client context provided");
		return -EINVAL;
	}

	cli = model->rt->user_data;
	cli->model = model;
	msg_timeout = CONFIG_BT_MESH_BRG_CFG_CLI_TIMEOUT;

	model->keys[0] = BT_MESH_KEY_DEV_ANY;
	model->rt->flags |= BT_MESH_MOD_DEVKEY_ONLY;

	bt_mesh_msg_ack_ctx_init(&cli->ack_ctx);

	return 0;
}

const struct bt_mesh_model_cb _bt_mesh_brg_cfg_cli_cb = {
	.init = brg_cfg_cli_init,
};

int bt_mesh_brg_cfg_cli_subnet_bridge_get(uint16_t net_idx, uint16_t addr,
					  enum bt_mesh_subnet_bridge_state *status)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_SUBNET_BRIDGE_GET, 0);
	struct bt_mesh_msg_ctx ctx = BT_MESH_MSG_CTX_INIT_DEV(net_idx, addr);
	const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = OP_SUBNET_BRIDGE_STATUS,
		.user_data = status,
		.timeout = msg_timeout,
	};

	bt_mesh_model_msg_init(&msg, OP_SUBNET_BRIDGE_GET);

	return bt_mesh_msg_ackd_send(cli->model, &ctx, &msg, !status ? NULL : &rsp_ctx);
}

int bt_mesh_brg_cfg_cli_subnet_bridge_set(uint16_t net_idx, uint16_t addr,
					  enum bt_mesh_subnet_bridge_state val,
					  enum bt_mesh_subnet_bridge_state *status)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_SUBNET_BRIDGE_SET, 1);
	struct bt_mesh_msg_ctx ctx = BT_MESH_MSG_CTX_INIT_DEV(net_idx, addr);
	const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = OP_SUBNET_BRIDGE_STATUS,
		.user_data = status,
		.timeout = msg_timeout,
	};

	bt_mesh_model_msg_init(&msg, OP_SUBNET_BRIDGE_SET);
	net_buf_simple_add_u8(&msg, (uint8_t)val);

	return bt_mesh_msg_ackd_send(cli->model, &ctx, &msg, !status ? NULL : &rsp_ctx);
}

int bt_mesh_brg_cfg_cli_bridging_table_size_get(uint16_t net_idx, uint16_t addr, uint16_t *size)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_BRIDGING_TABLE_SIZE_GET, 0);
	struct bt_mesh_msg_ctx ctx = BT_MESH_MSG_CTX_INIT_DEV(net_idx, addr);
	const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = OP_BRIDGING_TABLE_SIZE_STATUS,
		.user_data = size,
		.timeout = msg_timeout,
	};

	bt_mesh_model_msg_init(&msg, OP_BRIDGING_TABLE_SIZE_GET);

	return bt_mesh_msg_ackd_send(cli->model, &ctx, &msg, !size ? NULL : &rsp_ctx);
}

int bt_mesh_brg_cfg_cli_bridging_table_add(uint16_t net_idx, uint16_t addr,
					   struct bt_mesh_bridging_table_entry *entry,
					   struct bt_mesh_bridging_table_status *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_BRIDGING_TABLE_ADD, 8);
	struct bt_mesh_msg_ctx ctx = BT_MESH_MSG_CTX_INIT_DEV(net_idx, addr);
	const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = OP_BRIDGING_TABLE_STATUS,
		.user_data = rsp,
		.timeout = msg_timeout,
	};

	if (entry->addr1 == entry->addr2) {
		LOG_ERR("addr1 and addr2 shall have different values.");
		return -EINVAL;
	} else if (!BT_MESH_ADDR_IS_UNICAST(entry->addr1)) {
		LOG_ERR("addr1 shall be a unicast address.");
		return -EINVAL;
	} else if (entry->directions == 0x01 && (entry->addr2 == BT_MESH_ADDR_UNASSIGNED ||
						 entry->addr2 == BT_MESH_ADDR_ALL_NODES)) {
		LOG_ERR("For direction 0x01: addr2 shall not be unassigned or the all-nodes fixed "
			"group address.");
		return -EINVAL;
	} else if (entry->directions == 0x02 && !BT_MESH_ADDR_IS_UNICAST(entry->addr2)) {
		LOG_ERR("For direction 0x02: addr2 shall be a unicast address.");
		return -EINVAL;
	}

	bt_mesh_model_msg_init(&msg, OP_BRIDGING_TABLE_ADD);
	net_buf_simple_add_u8(&msg, entry->directions);
	key_idx_pack_pair(&msg, entry->net_idx1, entry->net_idx2);
	net_buf_simple_add_le16(&msg, entry->addr1);
	net_buf_simple_add_le16(&msg, entry->addr2);

	return bt_mesh_msg_ackd_send(cli->model, &ctx, &msg, !rsp ? NULL : &rsp_ctx);
}

int bt_mesh_brg_cfg_cli_bridging_table_remove(uint16_t net_idx, uint16_t addr, uint16_t net_idx1,
					      uint16_t net_idx2, uint16_t addr1, uint16_t addr2,
					      struct bt_mesh_bridging_table_status *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_BRIDGING_TABLE_REMOVE, 7);
	struct bt_mesh_msg_ctx ctx = BT_MESH_MSG_CTX_INIT_DEV(net_idx, addr);
	const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = OP_BRIDGING_TABLE_STATUS,
		.user_data = rsp,
		.timeout = msg_timeout,
	};

	if (!(addr1 == BT_MESH_ADDR_UNASSIGNED || BT_MESH_ADDR_IS_UNICAST(addr1))) {
		LOG_ERR("addr1 shall be a unicast address or unassigned.");
		return -EINVAL;
	} else if (addr2 == BT_MESH_ADDR_ALL_NODES) {
		LOG_ERR("addr2 shall not be the all-nodes fixed group address.");
		return -EINVAL;
	}

	bt_mesh_model_msg_init(&msg, OP_BRIDGING_TABLE_REMOVE);
	key_idx_pack_pair(&msg, net_idx1, net_idx2);
	net_buf_simple_add_le16(&msg, addr1);
	net_buf_simple_add_le16(&msg, addr2);

	return bt_mesh_msg_ackd_send(cli->model, &ctx, &msg, !rsp ? NULL : &rsp_ctx);
}

int bt_mesh_brg_cfg_cli_bridged_subnets_get(uint16_t net_idx, uint16_t addr,
					    struct bt_mesh_filter_netkey filter_net_idx,
					    uint8_t start_idx,
					    struct bt_mesh_bridged_subnets_list *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_BRIDGED_SUBNETS_GET, 3);
	struct bt_mesh_msg_ctx ctx = BT_MESH_MSG_CTX_INIT_DEV(net_idx, addr);
	const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = OP_BRIDGED_SUBNETS_LIST,
		.user_data = rsp,
		.timeout = msg_timeout,
	};

	bt_mesh_model_msg_init(&msg, OP_BRIDGED_SUBNETS_GET);
	net_buf_simple_add_le16(&msg, (filter_net_idx.filter | filter_net_idx.net_idx << 4));
	net_buf_simple_add_u8(&msg, start_idx);

	return bt_mesh_msg_ackd_send(cli->model, &ctx, &msg, !rsp ? NULL : &rsp_ctx);
}

int bt_mesh_brg_cfg_cli_bridging_table_get(uint16_t net_idx, uint16_t addr, uint16_t net_idx1,
					   uint16_t net_idx2, uint16_t start_idx,
					   struct bt_mesh_bridging_table_list *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_BRIDGING_TABLE_GET, 5);
	struct bt_mesh_msg_ctx ctx = BT_MESH_MSG_CTX_INIT_DEV(net_idx, addr);
	const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = OP_BRIDGING_TABLE_LIST,
		.user_data = rsp,
		.timeout = msg_timeout,
	};

	bt_mesh_model_msg_init(&msg, OP_BRIDGING_TABLE_GET);
	key_idx_pack_pair(&msg, net_idx1, net_idx2);
	net_buf_simple_add_le16(&msg, start_idx);

	return bt_mesh_msg_ackd_send(cli->model, &ctx, &msg, !rsp ? NULL : &rsp_ctx);
}

int32_t bt_mesh_brg_cfg_cli_timeout_get(void)
{
	return msg_timeout;
}

void bt_mesh_brg_cfg_cli_timeout_set(int32_t timeout)
{
	msg_timeout = timeout;
}
