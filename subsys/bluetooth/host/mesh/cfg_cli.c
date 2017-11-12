/*  Bluetooth Mesh */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <zephyr/types.h>
#include <misc/util.h>
#include <misc/byteorder.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/mesh.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG_MODEL)
#include "common/log.h"

#include "foundation.h"

#define MSG_TIMEOUT K_SECONDS(10)

struct comp_data {
	u8_t *status;
	struct net_buf_simple *comp;
};

static struct bt_mesh_cfg_cli *cli;

static void comp_data_status(struct bt_mesh_model *model,
			     struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	struct comp_data *param;
	size_t to_copy;

	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));

	if (cli->op_pending != OP_DEV_COMP_DATA_STATUS) {
		BT_WARN("Unexpected Composition Data Status");
		return;
	}

	param = cli->op_param;

	*(param->status) = net_buf_simple_pull_u8(buf);
	to_copy  = min(net_buf_simple_tailroom(param->comp), buf->len);
	net_buf_simple_add_mem(param->comp, buf->data, to_copy);

	k_sem_give(&cli->op_sync);
}

const struct bt_mesh_model_op bt_mesh_cfg_cli_op[] = {
	{ OP_DEV_COMP_DATA_STATUS,   15,  comp_data_status },
	BT_MESH_MODEL_OP_END,
};

int bt_mesh_cfg_comp_data_get(u16_t net_idx, u16_t addr, u8_t page,
			      u8_t *status, struct net_buf_simple *comp)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 1 + 4);
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.addr = addr,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	struct comp_data param = {
		.status = status,
		.comp = comp,
	};
	int err;

	if (!cli) {
		BT_ERR("No available Configuration Client context!\n");
		return -EINVAL;
	}

	if (cli->op_pending) {
		BT_WARN("Another synchronous operation pending");
		return -EBUSY;
	}

	bt_mesh_model_msg_init(msg, OP_DEV_COMP_DATA_GET);
	net_buf_simple_add_u8(msg, page);

	err = bt_mesh_model_send(cli->model, &ctx, msg, NULL, NULL);
	if (err) {
		BT_ERR("model_send() failed (err %d)", err);
		return err;
	}

	cli->op_param = &param;
	cli->op_pending = OP_DEV_COMP_DATA_STATUS;

	err = k_sem_take(&cli->op_sync, MSG_TIMEOUT);

	cli->op_pending = 0;
	cli->op_param = NULL;

	return err;
}

int bt_mesh_cfg_cli_init(struct bt_mesh_model *model, bool primary)
{
	BT_DBG("primary %u", primary);

	if (!primary) {
		BT_ERR("Configuration Client only allowed in primary element");
		return -EINVAL;
	}

	if (!model->user_data) {
		BT_ERR("No Configuration Client context provided");
		return -EINVAL;
	}

	cli = model->user_data;
	cli->model = model;

	/* Configuration Model security is device-key based */
	model->keys[0] = BT_MESH_KEY_DEV;

	k_sem_init(&cli->op_sync, 0, 1);

	return 0;
}
