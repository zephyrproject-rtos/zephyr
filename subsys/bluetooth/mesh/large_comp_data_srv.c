/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <zephyr/types.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/mesh.h>

#include <common/bt_str.h>

#define LOG_LEVEL CONFIG_BT_MESH_MODEL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_large_comp_data_srv);

#include "access.h"
#include "cfg.h"
#include "foundation.h"
#include "net.h"
#include "mesh.h"
#include "transport.h"

#define DUMMY_2_BYTE_OP BT_MESH_MODEL_OP_2(0xff, 0xff)
#define BT_MESH_MODEL_PAYLOAD_MAX                                              \
	(BT_MESH_TX_SDU_MAX - BT_MESH_MODEL_OP_LEN(DUMMY_2_BYTE_OP) -          \
	 BT_MESH_MIC_SHORT)

/** Mesh Large Composition Data Server Model Context */
static struct bt_mesh_large_comp_data_srv {
	/** Composition data model entry pointer. */
	const struct bt_mesh_model *model;
} srv;

static int handle_large_comp_data_get(const struct bt_mesh_model *model,
				      struct bt_mesh_msg_ctx *ctx,
				      struct net_buf_simple *buf)
{
	BT_MESH_MODEL_BUF_DEFINE(rsp, OP_LARGE_COMP_DATA_STATUS,
				 BT_MESH_MODEL_PAYLOAD_MAX);
	uint8_t page;
	size_t offset, total_size;
	int err;

	LOG_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
		ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
		bt_hex(buf->data, buf->len));

	page = bt_mesh_comp_parse_page(buf);
	offset = net_buf_simple_pull_le16(buf);

	LOG_DBG("page %u offset %u", page, offset);

	bt_mesh_model_msg_init(&rsp, OP_LARGE_COMP_DATA_STATUS);
	net_buf_simple_add_u8(&rsp, page);
	net_buf_simple_add_le16(&rsp, offset);

	if (atomic_test_bit(bt_mesh.flags, BT_MESH_COMP_DIRTY) && page < 128) {
		size_t msg_space;

		NET_BUF_SIMPLE_DEFINE(temp_buf, CONFIG_BT_MESH_COMP_PST_BUF_SIZE);
		err = bt_mesh_comp_read(&temp_buf, page);
		if (err) {
			LOG_ERR("Could not read comp data p%d, err: %d", page, err);
			return err;
		}

		net_buf_simple_add_le16(&rsp, temp_buf.len);
		if (offset > temp_buf.len) {
			return 0;
		}

		msg_space = net_buf_simple_tailroom(&rsp) - BT_MESH_MIC_SHORT;
		net_buf_simple_add_mem(
			&rsp, temp_buf.data + offset,
			(msg_space < (temp_buf.len - offset)) ? msg_space : temp_buf.len - offset);
	} else {
		total_size = bt_mesh_comp_page_size(page);
		net_buf_simple_add_le16(&rsp, total_size);

		if (offset < total_size) {
			err = bt_mesh_comp_data_get_page(&rsp, page, offset);
			if (err && err != -E2BIG) {
				LOG_ERR("Could not read comp data p%d, err: %d", page, err);
				return err;
			}
		}
	}

	if (bt_mesh_model_send(model, ctx, &rsp, NULL, NULL)) {
		LOG_ERR("Unable to send Large Composition Data Status");
	}

	return 0;
}

static int handle_models_metadata_get(const struct bt_mesh_model *model,
				      struct bt_mesh_msg_ctx *ctx,
				      struct net_buf_simple *buf)
{
	BT_MESH_MODEL_BUF_DEFINE(rsp, OP_MODELS_METADATA_STATUS,
				 BT_MESH_MODEL_PAYLOAD_MAX);
	size_t offset, total_size;
	uint8_t page;
	int err;

	LOG_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
		ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
		bt_hex(buf->data, buf->len));

	page = net_buf_simple_pull_u8(buf);
	offset = net_buf_simple_pull_le16(buf);

	LOG_DBG("page %u offset %u", page, offset);

	if (page >= 128U && atomic_test_bit(bt_mesh.flags, BT_MESH_METADATA_DIRTY)) {
		LOG_DBG("Models Metadata Page 128");
		page = 128U;
	} else if (page != 0U) {
		LOG_DBG("Models Metadata Page %u not available", page);
		page = 0U;
	}

	bt_mesh_model_msg_init(&rsp, OP_MODELS_METADATA_STATUS);
	net_buf_simple_add_u8(&rsp, page);
	net_buf_simple_add_le16(&rsp, offset);

	if (atomic_test_bit(bt_mesh.flags, BT_MESH_METADATA_DIRTY) == (page == 0U)) {
		rsp.size -= BT_MESH_MIC_SHORT;
		err = bt_mesh_models_metadata_read(&rsp, offset);
		if (err) {
			LOG_ERR("Unable to get stored models metadata");
			return err;
		}

		rsp.size += BT_MESH_MIC_SHORT;
	} else {
		total_size = bt_mesh_metadata_page_0_size();
		net_buf_simple_add_le16(&rsp, total_size);

		if (offset < total_size) {
			err = bt_mesh_metadata_get_page_0(&rsp, offset);
			if (err && err != -E2BIG) {
				LOG_ERR("Failed to get Models Metadata Page 0: %d", err);
				return err;
			}
		}
	}

	if (bt_mesh_model_send(model, ctx, &rsp, NULL, NULL)) {
		LOG_ERR("Unable to send Models Metadata Status");
	}

	return 0;
}

const struct bt_mesh_model_op _bt_mesh_large_comp_data_srv_op[] = {
	{ OP_LARGE_COMP_DATA_GET, BT_MESH_LEN_EXACT(3), handle_large_comp_data_get },
	{ OP_MODELS_METADATA_GET, BT_MESH_LEN_EXACT(3), handle_models_metadata_get },
	BT_MESH_MODEL_OP_END,
};

static int large_comp_data_srv_init(const struct bt_mesh_model *model)
{
	const struct bt_mesh_model *config_srv =
		bt_mesh_model_find(bt_mesh_model_elem(model), BT_MESH_MODEL_ID_CFG_SRV);

	if (config_srv == NULL) {
		LOG_ERR("Large Composition Data Server cannot extend Configuration server");
		return -EINVAL;
	}

	/* Large Composition Data Server model shall use the device key */
	model->keys[0] = BT_MESH_KEY_DEV;
	model->rt->flags |= BT_MESH_MOD_DEVKEY_ONLY;

	srv.model = model;

	bt_mesh_model_extend(model, config_srv);

	return 0;
}

const struct bt_mesh_model_cb _bt_mesh_large_comp_data_srv_cb = {
	.init = large_comp_data_srv_init,
};
