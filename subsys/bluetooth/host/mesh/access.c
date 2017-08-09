/*  Bluetooth Mesh */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <errno.h>
#include <misc/util.h>
#include <misc/byteorder.h>

#include <net/buf.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/mesh.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG_ACCESS)
#include "common/log.h"

#include "mesh.h"
#include "adv.h"
#include "net.h"
#include "lpn.h"
#include "transport.h"
#include "access.h"
#include "foundation.h"

static const struct bt_mesh_comp *dev_comp;
static u16_t dev_primary_addr;

static const struct {
	const u16_t id;
	int (*const init)(struct bt_mesh_model *model, bool primary);
} const model_init[] = {
	{ BT_MESH_MODEL_ID_CFG_SRV, bt_mesh_conf_init },
	{ BT_MESH_MODEL_ID_HEALTH_SRV, bt_mesh_health_init },
};

void bt_mesh_model_foreach(void (*func)(struct bt_mesh_model *mod,
					struct bt_mesh_elem *elem,
					bool vnd, bool primary,
					void *user_data),
			   void *user_data)
{
	int i, j;

	for (i = 0; i < dev_comp->elem_count; i++) {
		struct bt_mesh_elem *elem = &dev_comp->elem[i];

		for (j = 0; j < elem->model_count; j++) {
			struct bt_mesh_model *model = &elem->models[j];

			func(model, elem, false, i == 0, user_data);
		}

		for (j = 0; j < elem->vnd_model_count; j++) {
			struct bt_mesh_model *model = &elem->vnd_models[j];

			func(model, elem, true, i == 0, user_data);
		}
	}
}

s32_t bt_mesh_model_pub_period_get(struct bt_mesh_model *mod)
{
	int period;

	if (!mod->pub) {
		return 0;
	}

	switch (mod->pub->period >> 6) {
	case 0x00:
		/* 1 step is 100 ms */
		period = K_MSEC((mod->pub->period & BIT_MASK(6)) * 100);
		break;
	case 0x01:
		/* 1 step is 1 second */
		period = K_SECONDS(mod->pub->period & BIT_MASK(6));
		break;
	case 0x02:
		/* 1 step is 10 seconds */
		period = K_SECONDS((mod->pub->period & BIT_MASK(6)) * 10);
		break;
	case 0x03:
		/* 1 step is 10 minutes */
		period = K_MINUTES((mod->pub->period & BIT_MASK(6)) * 10);
		break;
	default:
		CODE_UNREACHABLE;
	}

	return period >> mod->pub->period_div;
}

static void mod_publish(struct k_work *work)
{
	struct bt_mesh_model_pub *pub = CONTAINER_OF(work,
						     struct bt_mesh_model_pub,
						     timer.work);
	s32_t period_ms;

	BT_DBG("");

	if (pub->func) {
		pub->func(pub->mod);
	}

	period_ms = bt_mesh_model_pub_period_get(pub->mod);
	BT_DBG("period %u ms", period_ms);
	if (period_ms) {
		k_delayed_work_submit(&pub->timer, period_ms);
	}
}

static void mod_init(struct bt_mesh_model *mod, struct bt_mesh_elem *elem,
		     bool vnd, bool primary, void *user_data)
{
	int i;

	mod->elem = elem;

	if (mod->pub) {
		mod->pub->mod = mod;
		k_delayed_work_init(&mod->pub->timer, mod_publish);
	}

	for (i = 0; i < ARRAY_SIZE(mod->keys); i++) {
		mod->keys[i] = BT_MESH_KEY_UNUSED;
	}

	if (vnd) {
		return;
	}

	for (i = 0; i < ARRAY_SIZE(model_init); i++) {
		if (model_init[i].id == mod->id) {
			model_init[i].init(mod, primary);
		}
	}
}

int bt_mesh_comp_register(const struct bt_mesh_comp *comp)
{
	/* There must be at least one element */
	if (!comp->elem_count) {
		return -EINVAL;
	}

	dev_comp = comp;

	bt_mesh_model_foreach(mod_init, NULL);

	return 0;
}

void bt_mesh_comp_provision(u16_t addr)
{
	int i;

	dev_primary_addr = addr;

	BT_DBG("addr 0x%04x elem_count %zu", addr, dev_comp->elem_count);

	for (i = 0; i < dev_comp->elem_count; i++) {
		struct bt_mesh_elem *elem = &dev_comp->elem[i];

		elem->addr = addr++;

		BT_DBG("addr 0x%04x mod_count %u vnd_mod_count %u",
		       elem->addr, elem->model_count, elem->vnd_model_count);
	}
}

void bt_mesh_comp_unprovision(void)
{
	BT_DBG("");

	dev_primary_addr = BT_MESH_ADDR_UNASSIGNED;

	bt_mesh_model_foreach(mod_init, NULL);
}

u16_t bt_mesh_primary_addr(void)
{
	return dev_primary_addr;
}

u16_t *bt_mesh_model_find_group(struct bt_mesh_model *mod, u16_t addr)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mod->groups); i++) {
		if (mod->groups[i] == addr) {
			return &mod->groups[i];
		}
	}

	return NULL;
}

static struct bt_mesh_model *bt_mesh_elem_find_group(struct bt_mesh_elem *elem,
						     u16_t group_addr)
{
	struct bt_mesh_model *model;
	u16_t *match;
	int i;

	for (i = 0; i < elem->model_count; i++) {
		model = &elem->models[i];

		match = bt_mesh_model_find_group(model, group_addr);
		if (match) {
			return model;
		}
	}

	for (i = 0; i < elem->vnd_model_count; i++) {
		model = &elem->vnd_models[i];

		match = bt_mesh_model_find_group(model, group_addr);
		if (match) {
			return model;
		}
	}

	return NULL;
}

struct bt_mesh_elem *bt_mesh_elem_find(u16_t addr)
{
	int i;

	for (i = 0; i < dev_comp->elem_count; i++) {
		struct bt_mesh_elem *elem = &dev_comp->elem[i];

		if (BT_MESH_ADDR_IS_GROUP(addr) ||
		    BT_MESH_ADDR_IS_VIRTUAL(addr)) {
			if (bt_mesh_elem_find_group(elem, addr)) {
				return elem;
			}
		} else if (elem->addr == addr) {
			return elem;
		}
	}

	return NULL;
}

u8_t bt_mesh_elem_count(void)
{
	return dev_comp->elem_count;
}

static bool model_has_key(struct bt_mesh_model *mod, u16_t key)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mod->keys); i++) {
		if (mod->keys[i] == key) {
			return true;
		}
	}

	return false;
}

static const struct bt_mesh_model_op *find_op(struct bt_mesh_model *models,
					      u8_t model_count,
					      u16_t app_idx, u32_t opcode,
					      struct bt_mesh_model **model)
{
	u8_t i;

	for (i = 0; i < model_count; i++) {
		const struct bt_mesh_model_op *op;

		*model = &models[i];

		if (!model_has_key(*model, app_idx)) {
			continue;
		}

		for (op = (*model)->op; op->func; op++) {
			if (op->opcode == opcode) {
				return op;
			}
		}
	}

	*model = NULL;
	return NULL;
}

static int get_opcode(struct net_buf_simple *buf, u32_t *opcode)
{
	switch (buf->data[0] >> 6) {
	case 0x00:
	case 0x01:
		if (buf->data[0] == 0x7f) {
			BT_ERR("Ignoring RFU OpCode");
			return -EINVAL;
		}

		*opcode = net_buf_simple_pull_u8(buf);
		return 0;
	case 0x02:
		if (buf->len < 2) {
			BT_ERR("Too short payload for 2-octet OpCode");
			return -EINVAL;
		}

		*opcode = net_buf_simple_pull_be16(buf);
		return 0;
	case 0x03:
		if (buf->len < 3) {
			BT_ERR("Too short payload for 3-octet OpCode");
			return -EINVAL;
		}

		*opcode = net_buf_simple_pull_u8(buf) << 16;
		*opcode |= net_buf_simple_pull_le16(buf);
		return 0;
	}

	CODE_UNREACHABLE;
}

bool bt_mesh_fixed_group_match(u16_t addr)
{
	/* Check for fixed group addresses */
	switch (addr) {
	case BT_MESH_ADDR_ALL_NODES:
		return true;
	case BT_MESH_ADDR_PROXIES:
		/* TODO: Proxy not yet supported */
		return false;
	case BT_MESH_ADDR_FRIENDS:
		return (bt_mesh_friend_get() == BT_MESH_FRIEND_ENABLED);
	case BT_MESH_ADDR_RELAYS:
		return (bt_mesh_relay_get() == BT_MESH_RELAY_ENABLED);
	default:
		return false;
	}
}

void bt_mesh_model_recv(struct bt_mesh_net_rx *rx, struct net_buf_simple *buf)
{
	struct bt_mesh_model *models, *model;
	const struct bt_mesh_model_op *op;
	u32_t opcode;
	u8_t count;
	int i;

	BT_DBG("app_idx 0x%04x src 0x%04x dst 0x%04x", rx->ctx.app_idx,
	       rx->ctx.addr, rx->dst);
	BT_DBG("len %u: %s", buf->len, bt_hex(buf->data, buf->len));

	if (get_opcode(buf, &opcode) < 0) {
		BT_WARN("Unable to decode OpCode");
		return;
	}

	BT_DBG("OpCode 0x%08x", opcode);

	for (i = 0; i < dev_comp->elem_count; i++) {
		struct bt_mesh_elem *elem = &dev_comp->elem[i];

		if (BT_MESH_ADDR_IS_UNICAST(rx->dst)) {
			if (elem->addr != rx->dst) {
				continue;
			}
		} else if (BT_MESH_ADDR_IS_GROUP(rx->dst) ||
			   BT_MESH_ADDR_IS_VIRTUAL(rx->dst)) {
			if (!bt_mesh_elem_find_group(elem, rx->dst)) {
				continue;
			}
		} else if (i != 0 || !bt_mesh_fixed_group_match(rx->dst)) {
			continue;
		}

		/* SIG models cannot contain 3-byte (vendor) OpCodes, and
		 * vendor models cannot contain SIG (1- or 2-byte) OpCodes, so
		 * we only need to do the lookup in one of the model lists.
		 */
		if (opcode < 0x10000) {
			models = elem->models;
			count = elem->model_count;
		} else {
			models = elem->vnd_models;
			count = elem->vnd_model_count;
		}

		op = find_op(models, count, rx->ctx.app_idx, opcode, &model);
		if (op) {
			struct net_buf_simple_state state;

			if (buf->len < op->min_len) {
				BT_ERR("Too short message for OpCode 0x%08x",
				       opcode);
				continue;
			}

			/* The callback will likely parse the buffer, so
			 * store the parsing state in case multiple models
			 * receive the message.
			 */
			net_buf_simple_save(buf, &state);
			op->func(model, &rx->ctx, buf);
			net_buf_simple_restore(buf, &state);

		} else {
			BT_DBG("No OpCode 0x%08x for elem %d", opcode, i);
		}
	}
}

void bt_mesh_model_msg_init(struct net_buf_simple *msg, u32_t opcode)
{
	net_buf_simple_init(msg, 0);

	if (opcode < 0x100) {
		/* 1-byte OpCode */
		net_buf_simple_add_u8(msg, opcode);
		return;
	}

	if (opcode < 0x10000) {
		/* 2-byte OpCode */
		net_buf_simple_add_be16(msg, opcode);
		return;
	}

	/* 3-byte OpCode */
	net_buf_simple_add_u8(msg, ((opcode >> 16) & 0xff));
	net_buf_simple_add_le16(msg, opcode & 0xffff);
}

int bt_mesh_model_send(struct bt_mesh_model *model,
		       struct bt_mesh_msg_ctx *ctx,
		       struct net_buf_simple *msg, bt_mesh_cb_t cb,
		       void *cb_data)
{
	struct bt_mesh_net_tx tx = {
		.sub = bt_mesh_subnet_get(ctx->net_idx),
		.ctx = ctx,
		.src = model->elem->addr,
	};

	if (ctx->friend_cred && !bt_mesh_lpn_established()) {
		BT_ERR("Friendship Credentials requested without a Friend");
		return -EINVAL;
	}

	BT_DBG("net_idx 0x%04x app_idx 0x%04x dst 0x%04x", ctx->net_idx,
	       ctx->app_idx, ctx->addr);
	BT_DBG("len %u: %s", msg->len, bt_hex(msg->data, msg->len));

	if (net_buf_simple_tailroom(msg) < 4) {
		BT_ERR("Not enough tailroom for TransMIC");
		return -EINVAL;
	}

	if (msg->len > BT_MESH_TX_SDU_MAX - 4) {
		BT_ERR("Too big message");
		return -EMSGSIZE;
	}

	if (!model_has_key(model, ctx->app_idx)) {
		BT_ERR("Model not bound to AppKey 0x%04x", ctx->app_idx);
		return -EINVAL;
	}

	return bt_mesh_trans_send(&tx, msg, cb, cb_data);
}

int bt_mesh_model_publish(struct bt_mesh_model *model,
			  struct net_buf_simple *msg)
{
	struct bt_mesh_app_key *key;
	struct bt_mesh_msg_ctx ctx;

	if (!model->pub) {
		return -ENOTSUP;
	}

	if (model->pub->key == BT_MESH_KEY_UNUSED ||
	    model->pub->addr == BT_MESH_ADDR_UNASSIGNED) {
		return -EADDRNOTAVAIL;
	}

	key = bt_mesh_app_key_find(model->pub->key);
	if (!key) {
		return -EADDRNOTAVAIL;
	}

	ctx.net_idx = key->net_idx;
	ctx.app_idx = key->app_idx;
	ctx.addr = model->pub->addr;
	ctx.friend_cred = model->pub->cred;
	ctx.send_ttl = model->pub->ttl;

	return bt_mesh_model_send(model, &ctx, msg, NULL, NULL);
}

struct bt_mesh_model *bt_mesh_model_find_vnd(struct bt_mesh_elem *elem,
					     u16_t company, u16_t id)
{
	u8_t i;

	for (i = 0; i < elem->vnd_model_count; i++) {
		if (elem->vnd_models[i].vnd.company == company &&
		    elem->vnd_models[i].vnd.id == id) {
			return &elem->vnd_models[i];
		}
	}

	return NULL;
}

struct bt_mesh_model *bt_mesh_model_find(struct bt_mesh_elem *elem,
					 u16_t id)
{
	u8_t i;

	for (i = 0; i < elem->model_count; i++) {
		if (elem->models[i].id == id) {
			return &elem->models[i];
		}
	}

	return NULL;
}

const struct bt_mesh_comp *bt_mesh_comp_get(void)
{
	return dev_comp;
}
