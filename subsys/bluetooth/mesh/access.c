/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/util.h>
#include <sys/byteorder.h>

#include <net/buf.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/mesh.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG_ACCESS)
#define LOG_MODULE_NAME bt_mesh_access
#include "common/log.h"

#include "mesh.h"
#include "adv.h"
#include "net.h"
#include "lpn.h"
#include "transport.h"
#include "access.h"
#include "foundation.h"
#include "settings.h"

/* bt_mesh_model.flags */
enum {
	BT_MESH_MOD_BIND_PENDING = BIT(0),
	BT_MESH_MOD_SUB_PENDING = BIT(1),
	BT_MESH_MOD_PUB_PENDING = BIT(2),
	BT_MESH_MOD_NEXT_IS_PARENT = BIT(3),
};

/* Model publication information for persistent storage. */
struct mod_pub_val {
	uint16_t addr;
	uint16_t key;
	uint8_t  ttl;
	uint8_t  retransmit;
	uint8_t  period;
	uint8_t  period_div:4,
		 cred:1;
};

static const struct bt_mesh_comp *dev_comp;
static uint16_t dev_primary_addr;
static void (*msg_cb)(uint32_t opcode, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf);

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

int32_t bt_mesh_model_pub_period_get(struct bt_mesh_model *mod)
{
	int32_t period;

	if (!mod->pub) {
		return 0;
	}

	switch (mod->pub->period >> 6) {
	case 0x00:
		/* 1 step is 100 ms */
		period = (mod->pub->period & BIT_MASK(6)) * 100U;
		break;
	case 0x01:
		/* 1 step is 1 second */
		period = (mod->pub->period & BIT_MASK(6)) * MSEC_PER_SEC;
		break;
	case 0x02:
		/* 1 step is 10 seconds */
		period = (mod->pub->period & BIT_MASK(6)) * 10U * MSEC_PER_SEC;
		break;
	case 0x03:
		/* 1 step is 10 minutes */
		period = (mod->pub->period & BIT_MASK(6)) * 600U * MSEC_PER_SEC;
		break;
	default:
		CODE_UNREACHABLE;
	}

	if (mod->pub->fast_period) {
		return period >> mod->pub->period_div;
	} else {
		return period;
	}
}

static int32_t next_period(struct bt_mesh_model *mod)
{
	struct bt_mesh_model_pub *pub = mod->pub;
	uint32_t elapsed, period;

	period = bt_mesh_model_pub_period_get(mod);
	if (!period) {
		return 0;
	}

	elapsed = k_uptime_get_32() - pub->period_start;

	BT_DBG("Publishing took %ums", elapsed);

	if (elapsed >= period) {
		BT_WARN("Publication sending took longer than the period");
		/* Return smallest positive number since 0 means disabled */
		return 1;
	}

	return period - elapsed;
}

static void publish_sent(int err, void *user_data)
{
	struct bt_mesh_model *mod = user_data;
	int32_t delay;

	BT_DBG("err %d", err);

	if (mod->pub->count) {
		delay = BT_MESH_PUB_TRANSMIT_INT(mod->pub->retransmit);
	} else {
		delay = next_period(mod);
	}

	if (delay) {
		BT_DBG("Publishing next time in %dms", delay);
		/* Using schedule() in case the application has already called
		 * bt_mesh_publish, and a publication is pending.
		 */
		k_work_schedule(&mod->pub->timer, K_MSEC(delay));
	}
}

static void publish_start(uint16_t duration, int err, void *user_data)
{
	struct bt_mesh_model *mod = user_data;
	struct bt_mesh_model_pub *pub = mod->pub;

	if (err) {
		BT_ERR("Failed to publish: err %d", err);
		publish_sent(err, user_data);
		return;
	}

	/* Initialize the timestamp for the beginning of a new period */
	if (pub->count == BT_MESH_PUB_TRANSMIT_COUNT(pub->retransmit)) {
		pub->period_start = k_uptime_get_32();
	}
}

static const struct bt_mesh_send_cb pub_sent_cb = {
	.start = publish_start,
	.end = publish_sent,
};

static int publish_transmit(struct bt_mesh_model *mod)
{
	NET_BUF_SIMPLE_DEFINE(sdu, BT_MESH_TX_SDU_MAX);
	struct bt_mesh_model_pub *pub = mod->pub;
	struct bt_mesh_msg_ctx ctx = {
		.addr = pub->addr,
		.send_ttl = pub->ttl,
		.app_idx = pub->key,
	};
	struct bt_mesh_net_tx tx = {
		.ctx = &ctx,
		.src = bt_mesh_model_elem(mod)->addr,
		.friend_cred = pub->cred,
	};

	net_buf_simple_add_mem(&sdu, pub->msg->data, pub->msg->len);

	return bt_mesh_trans_send(&tx, &sdu, &pub_sent_cb, mod);
}

static int pub_period_start(struct bt_mesh_model_pub *pub)
{
	int err;

	pub->count = BT_MESH_PUB_TRANSMIT_COUNT(pub->retransmit);

	if (!pub->update) {
		return 0;
	}

	err = pub->update(pub->mod);
	if (err) {
		/* Skip this publish attempt. */
		BT_DBG("Update failed, skipping publish (err: %d)", err);
		pub->count = 0;
		pub->period_start = k_uptime_get_32();
		publish_sent(err, pub->mod);
		return err;
	}

	return 0;
}

static void mod_publish(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct bt_mesh_model_pub *pub = CONTAINER_OF(dwork,
						     struct bt_mesh_model_pub,
						     timer);
	int err;

	if (pub->addr == BT_MESH_ADDR_UNASSIGNED ||
	    atomic_test_bit(bt_mesh.flags, BT_MESH_SUSPENDED)) {
		/* Publication is no longer active, but the cancellation of the
		 * delayed work failed. Abandon recurring timer.
		 */
		return;
	}

	BT_DBG("");

	if (pub->count) {
		pub->count--;
	} else {
		/* First publication in this period */
		err = pub_period_start(pub);
		if (err) {
			return;
		}
	}

	err = publish_transmit(pub->mod);
	if (err) {
		BT_ERR("Failed to publish (err %d)", err);
		if (pub->count == BT_MESH_PUB_TRANSMIT_COUNT(pub->retransmit)) {
			pub->period_start = k_uptime_get_32();
		}

		publish_sent(err, pub->mod);
	}
}

struct bt_mesh_elem *bt_mesh_model_elem(struct bt_mesh_model *mod)
{
	return &dev_comp->elem[mod->elem_idx];
}

struct bt_mesh_model *bt_mesh_model_get(bool vnd, uint8_t elem_idx, uint8_t mod_idx)
{
	struct bt_mesh_elem *elem;

	if (elem_idx >= dev_comp->elem_count) {
		BT_ERR("Invalid element index %u", elem_idx);
		return NULL;
	}

	elem = &dev_comp->elem[elem_idx];

	if (vnd) {
		if (mod_idx >= elem->vnd_model_count) {
			BT_ERR("Invalid vendor model index %u", mod_idx);
			return NULL;
		}

		return &elem->vnd_models[mod_idx];
	} else {
		if (mod_idx >= elem->model_count) {
			BT_ERR("Invalid SIG model index %u", mod_idx);
			return NULL;
		}

		return &elem->models[mod_idx];
	}
}

#if defined(CONFIG_BT_MESH_MODEL_VND_MSG_CID_FORCE)
static int bt_mesh_vnd_mod_msg_cid_check(struct bt_mesh_model *mod)
{
	uint16_t cid;
	const struct bt_mesh_model_op *op;

	for (op = mod->op; op->func; op++) {
		cid = (uint16_t)(op->opcode & 0xffff);

		if (cid == mod->vnd.company) {
			continue;
		}

		BT_ERR("Invalid vendor model(company:0x%04x"
		       " id:0x%04x) message opcode 0x%08x",
		       mod->vnd.company, mod->vnd.id, op->opcode);

		return -EINVAL;
	}

	return 0;
}
#endif

static void mod_init(struct bt_mesh_model *mod, struct bt_mesh_elem *elem,
		     bool vnd, bool primary, void *user_data)
{
	int i;
	int *err = user_data;

	if (*err) {
		return;
	}

	if (mod->pub) {
		mod->pub->mod = mod;
		k_work_init_delayable(&mod->pub->timer, mod_publish);
	}

	for (i = 0; i < ARRAY_SIZE(mod->keys); i++) {
		mod->keys[i] = BT_MESH_KEY_UNUSED;
	}

	mod->elem_idx = elem - dev_comp->elem;
	if (vnd) {
		mod->mod_idx = mod - elem->vnd_models;

		if (IS_ENABLED(CONFIG_BT_MESH_MODEL_VND_MSG_CID_FORCE)) {
			*err = bt_mesh_vnd_mod_msg_cid_check(mod);
			if (*err) {
				return;
			}
		}

	} else {
		mod->mod_idx = mod - elem->models;
	}

	if (mod->cb && mod->cb->init) {
		*err = mod->cb->init(mod);
	}
}

int bt_mesh_comp_register(const struct bt_mesh_comp *comp)
{
	int err;

	/* There must be at least one element */
	if (!comp || !comp->elem_count) {
		return -EINVAL;
	}

	dev_comp = comp;

	err = 0;
	bt_mesh_model_foreach(mod_init, &err);

	return err;
}

void bt_mesh_comp_provision(uint16_t addr)
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
}

uint16_t bt_mesh_primary_addr(void)
{
	return dev_primary_addr;
}

static uint16_t *model_group_get(struct bt_mesh_model *mod, uint16_t addr)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mod->groups); i++) {
		if (mod->groups[i] == addr) {
			return &mod->groups[i];
		}
	}

	return NULL;
}

struct find_group_visitor_ctx {
	uint16_t *entry;
	struct bt_mesh_model *mod;
	uint16_t addr;
};

static enum bt_mesh_walk find_group_mod_visitor(struct bt_mesh_model *mod,
						uint32_t depth, void *user_data)
{
	struct find_group_visitor_ctx *ctx = user_data;

	if (mod->elem_idx != ctx->mod->elem_idx) {
		return BT_MESH_WALK_CONTINUE;
	}

	ctx->entry = model_group_get(mod, ctx->addr);
	if (ctx->entry) {
		ctx->mod = mod;
		return BT_MESH_WALK_STOP;
	}

	return BT_MESH_WALK_CONTINUE;
}

uint16_t *bt_mesh_model_find_group(struct bt_mesh_model **mod, uint16_t addr)
{
	struct find_group_visitor_ctx ctx = {
		.mod = *mod,
		.entry = NULL,
		.addr = addr,
	};

	bt_mesh_model_tree_walk(bt_mesh_model_root(*mod),
				find_group_mod_visitor, &ctx);

	*mod = ctx.mod;
	return ctx.entry;
}

static struct bt_mesh_model *bt_mesh_elem_find_group(struct bt_mesh_elem *elem,
						     uint16_t group_addr)
{
	struct bt_mesh_model *model;
	uint16_t *match;
	int i;

	for (i = 0; i < elem->model_count; i++) {
		model = &elem->models[i];

		match = model_group_get(model, group_addr);
		if (match) {
			return model;
		}
	}

	for (i = 0; i < elem->vnd_model_count; i++) {
		model = &elem->vnd_models[i];

		match = model_group_get(model, group_addr);
		if (match) {
			return model;
		}
	}

	return NULL;
}

struct bt_mesh_elem *bt_mesh_elem_find(uint16_t addr)
{
	uint16_t index;

	if (!BT_MESH_ADDR_IS_UNICAST(addr)) {
		return NULL;
	}

	index = addr - dev_comp->elem[0].addr;
	if (index >= dev_comp->elem_count) {
		return NULL;
	}

	return &dev_comp->elem[index];
}

bool bt_mesh_has_addr(uint16_t addr)
{
	uint16_t index;

	if (BT_MESH_ADDR_IS_UNICAST(addr)) {
		return bt_mesh_elem_find(addr) != NULL;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_ACCESS_LAYER_MSG) && msg_cb) {
		return true;
	}

	for (index = 0; index < dev_comp->elem_count; index++) {
		struct bt_mesh_elem *elem = &dev_comp->elem[index];

		if (bt_mesh_elem_find_group(elem, addr)) {
			return true;
		}
	}

	return false;
}

#if defined(CONFIG_BT_MESH_ACCESS_LAYER_MSG)
void bt_mesh_msg_cb_set(void (*cb)(uint32_t opcode, struct bt_mesh_msg_ctx *ctx,
			struct net_buf_simple *buf))
{
	msg_cb = cb;
}
#endif

int bt_mesh_msg_send(struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf, uint16_t src_addr,
		const struct bt_mesh_send_cb *cb, void *cb_data)
{
	struct bt_mesh_net_tx tx = {
		.ctx = ctx,
		.src = src_addr,
	};

	BT_DBG("net_idx 0x%04x app_idx 0x%04x dst 0x%04x", tx.ctx->net_idx,
	       tx.ctx->app_idx, tx.ctx->addr);
	BT_DBG("len %u: %s", buf->len, bt_hex(buf->data, buf->len));

	if (!bt_mesh_is_provisioned()) {
		BT_ERR("Local node is not yet provisioned");
		return -EAGAIN;
	}

	return bt_mesh_trans_send(&tx, buf, cb, cb_data);
}

uint8_t bt_mesh_elem_count(void)
{
	return dev_comp->elem_count;
}

static bool model_has_key(struct bt_mesh_model *mod, uint16_t key)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mod->keys); i++) {
		if (mod->keys[i] == key ||
		    (mod->keys[i] == BT_MESH_KEY_DEV_ANY &&
		     BT_MESH_IS_DEV_KEY(key))) {
			return true;
		}
	}

	return false;
}

static bool model_has_dst(struct bt_mesh_model *mod, uint16_t dst)
{
	if (BT_MESH_ADDR_IS_UNICAST(dst)) {
		return (dev_comp->elem[mod->elem_idx].addr == dst);
	} else if (BT_MESH_ADDR_IS_GROUP(dst) || BT_MESH_ADDR_IS_VIRTUAL(dst)) {
		return !!bt_mesh_model_find_group(&mod, dst);
	}

	/* If a message with a fixed group address is sent to the access layer,
	 * the lower layers have already confirmed that we are subscribing to
	 * it. All models on the primary element should receive the message.
	 */
	return mod->elem_idx == 0;
}

static const struct bt_mesh_model_op *find_op(struct bt_mesh_elem *elem,
					      uint32_t opcode, struct bt_mesh_model **model)
{
	uint8_t i;
	uint8_t count;
	/* This value shall not be used in shipping end products. */
	uint32_t cid = UINT32_MAX;
	struct bt_mesh_model *models;

	/* SIG models cannot contain 3-byte (vendor) OpCodes, and
	 * vendor models cannot contain SIG (1- or 2-byte) OpCodes, so
	 * we only need to do the lookup in one of the model lists.
	 */
	if (BT_MESH_MODEL_OP_LEN(opcode) < 3) {
		models = elem->models;
		count = elem->model_count;
	} else {
		models = elem->vnd_models;
		count = elem->vnd_model_count;

		cid = (uint16_t)(opcode & 0xffff);
	}

	for (i = 0U; i < count; i++) {

		const struct bt_mesh_model_op *op;

		if (IS_ENABLED(CONFIG_BT_MESH_MODEL_VND_MSG_CID_FORCE) &&
		     cid != UINT32_MAX &&
		     cid != models[i].vnd.company) {
			continue;
		}

		*model = &models[i];

		for (op = (*model)->op; op->func; op++) {
			if (op->opcode == opcode) {
				return op;
			}
		}
	}

	*model = NULL;
	return NULL;
}

static int get_opcode(struct net_buf_simple *buf, uint32_t *opcode)
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
		/* Using LE for the CID since the model layer is defined as
		 * little-endian in the mesh spec and using BT_MESH_MODEL_OP_3
		 * will declare the opcode in this way.
		 */
		*opcode |= net_buf_simple_pull_le16(buf);
		return 0;
	}

	CODE_UNREACHABLE;
}

void bt_mesh_model_recv(struct bt_mesh_net_rx *rx, struct net_buf_simple *buf)
{
	struct bt_mesh_model *model;
	const struct bt_mesh_model_op *op;
	uint32_t opcode;
	int i;

	BT_DBG("app_idx 0x%04x src 0x%04x dst 0x%04x", rx->ctx.app_idx,
	       rx->ctx.addr, rx->ctx.recv_dst);
	BT_DBG("len %u: %s", buf->len, bt_hex(buf->data, buf->len));

	if (get_opcode(buf, &opcode) < 0) {
		BT_WARN("Unable to decode OpCode");
		return;
	}

	BT_DBG("OpCode 0x%08x", opcode);

	for (i = 0; i < dev_comp->elem_count; i++) {
		struct net_buf_simple_state state;

		op = find_op(&dev_comp->elem[i], opcode, &model);
		if (!op) {
			BT_DBG("No OpCode 0x%08x for elem %d", opcode, i);
			continue;
		}

		if (!model_has_key(model, rx->ctx.app_idx)) {
			continue;
		}

		if (!model_has_dst(model, rx->ctx.recv_dst)) {
			continue;
		}

		if ((op->len >= 0) && (buf->len < (size_t)op->len)) {
			BT_ERR("Too short message for OpCode 0x%08x", opcode);
			continue;
		} else if ((op->len < 0) && (buf->len != (size_t)(-op->len))) {
			BT_ERR("Invalid message size for OpCode 0x%08x",
			       opcode);
			continue;
		}

		/* The callback will likely parse the buffer, so
		 * store the parsing state in case multiple models
		 * receive the message.
		 */
		net_buf_simple_save(buf, &state);
		(void)op->func(model, &rx->ctx, buf);
		net_buf_simple_restore(buf, &state);
	}

	if (IS_ENABLED(CONFIG_BT_MESH_ACCESS_LAYER_MSG) && msg_cb) {
		msg_cb(opcode, &rx->ctx, buf);
	}
}

int bt_mesh_model_send(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		       struct net_buf_simple *msg,
		       const struct bt_mesh_send_cb *cb, void *cb_data)
{
	if (!model_has_key(model, ctx->app_idx)) {
		BT_ERR("Model not bound to AppKey 0x%04x", ctx->app_idx);
		return -EINVAL;
	}

	return bt_mesh_msg_send(ctx, msg, bt_mesh_model_elem(model)->addr, cb, cb_data);
}

int bt_mesh_model_publish(struct bt_mesh_model *model)
{
	struct bt_mesh_model_pub *pub = model->pub;

	if (!pub) {
		return -ENOTSUP;
	}

	BT_DBG("");

	if (pub->addr == BT_MESH_ADDR_UNASSIGNED) {
		return -EADDRNOTAVAIL;
	}

	if (!pub->msg || !pub->msg->len) {
		BT_ERR("No publication message");
		return -EINVAL;
	}

	if (pub->msg->len + BT_MESH_MIC_SHORT > BT_MESH_TX_SDU_MAX) {
		BT_ERR("Message does not fit maximum SDU size");
		return -EMSGSIZE;
	}

	if (pub->count) {
		BT_WARN("Clearing publish retransmit timer");
	}

	/* Account for initial transmission */
	pub->count = BT_MESH_PUB_TRANSMIT_COUNT(pub->retransmit) + 1;

	BT_DBG("Publish Retransmit Count %u Interval %ums", pub->count,
	       BT_MESH_PUB_TRANSMIT_INT(pub->retransmit));

	k_work_reschedule(&pub->timer, K_NO_WAIT);

	return 0;
}

struct bt_mesh_model *bt_mesh_model_find_vnd(const struct bt_mesh_elem *elem,
					     uint16_t company, uint16_t id)
{
	uint8_t i;

	for (i = 0U; i < elem->vnd_model_count; i++) {
		if (elem->vnd_models[i].vnd.company == company &&
		    elem->vnd_models[i].vnd.id == id) {
			return &elem->vnd_models[i];
		}
	}

	return NULL;
}

struct bt_mesh_model *bt_mesh_model_find(const struct bt_mesh_elem *elem,
					 uint16_t id)
{
	uint8_t i;

	for (i = 0U; i < elem->model_count; i++) {
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

struct bt_mesh_model *bt_mesh_model_root(struct bt_mesh_model *mod)
{
#ifdef CONFIG_BT_MESH_MODEL_EXTENSIONS
	while (mod->next) {
		mod = mod->next;
	}
#endif
	return mod;
}

void bt_mesh_model_tree_walk(struct bt_mesh_model *root,
			     enum bt_mesh_walk (*cb)(struct bt_mesh_model *mod,
						     uint32_t depth,
						     void *user_data),
			     void *user_data)
{
	struct bt_mesh_model *m = root;
	int depth = 0;
	/* 'skip' is set to true when we ascend from child to parent node.
	 * In that case, we want to skip calling the callback on the parent
	 * node and we don't want to descend onto a child node as those
	 * nodes have already been visited.
	 */
	bool skip = false;

	do {
		if (!skip &&
		    cb(m, (uint32_t)depth, user_data) == BT_MESH_WALK_STOP) {
			return;
		}
#ifdef CONFIG_BT_MESH_MODEL_EXTENSIONS
		if (!skip && m->extends) {
			m = m->extends;
			depth++;
		} else if (m->flags & BT_MESH_MOD_NEXT_IS_PARENT) {
			m = m->next;
			depth--;
			skip = true;
		} else {
			m = m->next;
			skip = false;
		}
#endif
	} while (m && depth > 0);
}

#ifdef CONFIG_BT_MESH_MODEL_EXTENSIONS
int bt_mesh_model_extend(struct bt_mesh_model *mod,
			 struct bt_mesh_model *base_mod)
{
	/* Form a cyclical LCRS tree:
	 * The extends-pointer points to the first child, and the next-pointer
	 * points to the next sibling. The last sibling is marked by the
	 * BT_MESH_MOD_NEXT_IS_PARENT flag, and its next-pointer points back to
	 * the parent. This way, the whole tree is accessible from any node.
	 *
	 * We add children (extend them) by inserting them as the first child.
	 */
	if (base_mod->next) {
		return -EALREADY;
	}

	if (mod->extends) {
		base_mod->next = mod->extends;
	} else {
		base_mod->next = mod;
		base_mod->flags |= BT_MESH_MOD_NEXT_IS_PARENT;
	}

	mod->extends = base_mod;
	return 0;
}
#endif

static int mod_set_bind(struct bt_mesh_model *mod, size_t len_rd,
			settings_read_cb read_cb, void *cb_arg)
{
	ssize_t len;
	int i;

	/* Start with empty array regardless of cleared or set value */
	for (i = 0; i < ARRAY_SIZE(mod->keys); i++) {
		mod->keys[i] = BT_MESH_KEY_UNUSED;
	}

	if (len_rd == 0) {
		BT_DBG("Cleared bindings for model");
		return 0;
	}

	len = read_cb(cb_arg, mod->keys, sizeof(mod->keys));
	if (len < 0) {
		BT_ERR("Failed to read value (err %zd)", len);
		return len;
	}

	BT_HEXDUMP_DBG(mod->keys, len, "val");

	BT_DBG("Decoded %zu bound keys for model", len / sizeof(mod->keys[0]));
	return 0;
}

static int mod_set_sub(struct bt_mesh_model *mod, size_t len_rd,
		       settings_read_cb read_cb, void *cb_arg)
{
	ssize_t len;

	/* Start with empty array regardless of cleared or set value */
	(void)memset(mod->groups, 0, sizeof(mod->groups));

	if (len_rd == 0) {
		BT_DBG("Cleared subscriptions for model");
		return 0;
	}

	len = read_cb(cb_arg, mod->groups, sizeof(mod->groups));
	if (len < 0) {
		BT_ERR("Failed to read value (err %zd)", len);
		return len;
	}

	BT_HEXDUMP_DBG(mod->groups, len, "val");

	BT_DBG("Decoded %zu subscribed group addresses for model",
	       len / sizeof(mod->groups[0]));
	return 0;
}

static int mod_set_pub(struct bt_mesh_model *mod, size_t len_rd,
		       settings_read_cb read_cb, void *cb_arg)
{
	struct mod_pub_val pub;
	int err;

	if (!mod->pub) {
		BT_WARN("Model has no publication context!");
		return -EINVAL;
	}

	if (len_rd == 0) {
		mod->pub->addr = BT_MESH_ADDR_UNASSIGNED;
		mod->pub->key = 0U;
		mod->pub->cred = 0U;
		mod->pub->ttl = 0U;
		mod->pub->period = 0U;
		mod->pub->retransmit = 0U;
		mod->pub->count = 0U;

		BT_DBG("Cleared publication for model");
		return 0;
	}

	if (!IS_ENABLED(CONFIG_BT_SETTINGS)) {
		return 0;
	}

	err = bt_mesh_settings_set(read_cb, cb_arg, &pub, sizeof(pub));
	if (err) {
		BT_ERR("Failed to set \'model-pub\'");
		return err;
	}

	mod->pub->addr = pub.addr;
	mod->pub->key = pub.key;
	mod->pub->cred = pub.cred;
	mod->pub->ttl = pub.ttl;
	mod->pub->period = pub.period;
	mod->pub->retransmit = pub.retransmit;
	mod->pub->period_div = pub.period_div;
	mod->pub->count = 0U;

	BT_DBG("Restored model publication, dst 0x%04x app_idx 0x%03x",
	       pub.addr, pub.key);

	return 0;
}

static int mod_data_set(struct bt_mesh_model *mod,
			const char *name, size_t len_rd,
			settings_read_cb read_cb, void *cb_arg)
{
	const char *next;

	settings_name_next(name, &next);

	if (mod->cb && mod->cb->settings_set) {
		return mod->cb->settings_set(mod, next, len_rd,
			read_cb, cb_arg);
	}

	return 0;
}

static int mod_set(bool vnd, const char *name, size_t len_rd,
		   settings_read_cb read_cb, void *cb_arg)
{
	struct bt_mesh_model *mod;
	uint8_t elem_idx, mod_idx;
	uint16_t mod_key;
	int len;
	const char *next;

	if (!name) {
		BT_ERR("Insufficient number of arguments");
		return -ENOENT;
	}

	mod_key = strtol(name, NULL, 16);
	elem_idx = mod_key >> 8;
	mod_idx = mod_key;

	BT_DBG("Decoded mod_key 0x%04x as elem_idx %u mod_idx %u",
	       mod_key, elem_idx, mod_idx);

	mod = bt_mesh_model_get(vnd, elem_idx, mod_idx);
	if (!mod) {
		BT_ERR("Failed to get model for elem_idx %u mod_idx %u",
		       elem_idx, mod_idx);
		return -ENOENT;
	}

	len = settings_name_next(name, &next);

	if (!next) {
		BT_ERR("Insufficient number of arguments");
		return -ENOENT;
	}

	if (!strncmp(next, "bind", len)) {
		return mod_set_bind(mod, len_rd, read_cb, cb_arg);
	}

	if (!strncmp(next, "sub", len)) {
		return mod_set_sub(mod, len_rd, read_cb, cb_arg);
	}

	if (!strncmp(next, "pub", len)) {
		return mod_set_pub(mod, len_rd, read_cb, cb_arg);
	}

	if (!strncmp(next, "data", len)) {
		return mod_data_set(mod, next, len_rd, read_cb, cb_arg);
	}

	BT_WARN("Unknown module key %s", next);
	return -ENOENT;
}

static int sig_mod_set(const char *name, size_t len_rd,
		       settings_read_cb read_cb, void *cb_arg)
{
	return mod_set(false, name, len_rd, read_cb, cb_arg);
}

BT_MESH_SETTINGS_DEFINE(sig_mod, "s", sig_mod_set);

static int vnd_mod_set(const char *name, size_t len_rd,
		       settings_read_cb read_cb, void *cb_arg)
{
	return mod_set(true, name, len_rd, read_cb, cb_arg);
}

BT_MESH_SETTINGS_DEFINE(vnd_mod, "v", vnd_mod_set);

static void encode_mod_path(struct bt_mesh_model *mod, bool vnd,
			    const char *key, char *path, size_t path_len)
{
	uint16_t mod_key = (((uint16_t)mod->elem_idx << 8) | mod->mod_idx);

	if (vnd) {
		snprintk(path, path_len, "bt/mesh/v/%x/%s", mod_key, key);
	} else {
		snprintk(path, path_len, "bt/mesh/s/%x/%s", mod_key, key);
	}
}

static void store_pending_mod_bind(struct bt_mesh_model *mod, bool vnd)
{
	uint16_t keys[CONFIG_BT_MESH_MODEL_KEY_COUNT];
	char path[20];
	int i, count, err;

	for (i = 0, count = 0; i < ARRAY_SIZE(mod->keys); i++) {
		if (mod->keys[i] != BT_MESH_KEY_UNUSED) {
			keys[count++] = mod->keys[i];
			BT_DBG("model key 0x%04x", mod->keys[i]);
		}
	}

	encode_mod_path(mod, vnd, "bind", path, sizeof(path));

	if (count) {
		err = settings_save_one(path, keys, count * sizeof(keys[0]));
	} else {
		err = settings_delete(path);
	}

	if (err) {
		BT_ERR("Failed to store %s value", log_strdup(path));
	} else {
		BT_DBG("Stored %s value", log_strdup(path));
	}
}

static void store_pending_mod_sub(struct bt_mesh_model *mod, bool vnd)
{
	uint16_t groups[CONFIG_BT_MESH_MODEL_GROUP_COUNT];
	char path[20];
	int i, count, err;

	for (i = 0, count = 0; i < CONFIG_BT_MESH_MODEL_GROUP_COUNT; i++) {
		if (mod->groups[i] != BT_MESH_ADDR_UNASSIGNED) {
			groups[count++] = mod->groups[i];
		}
	}

	encode_mod_path(mod, vnd, "sub", path, sizeof(path));

	if (count) {
		err = settings_save_one(path, groups,
					count * sizeof(groups[0]));
	} else {
		err = settings_delete(path);
	}

	if (err) {
		BT_ERR("Failed to store %s value", log_strdup(path));
	} else {
		BT_DBG("Stored %s value", log_strdup(path));
	}
}

static void store_pending_mod_pub(struct bt_mesh_model *mod, bool vnd)
{
	struct mod_pub_val pub;
	char path[20];
	int err;

	encode_mod_path(mod, vnd, "pub", path, sizeof(path));

	if (!mod->pub || mod->pub->addr == BT_MESH_ADDR_UNASSIGNED) {
		err = settings_delete(path);
	} else {
		pub.addr = mod->pub->addr;
		pub.key = mod->pub->key;
		pub.ttl = mod->pub->ttl;
		pub.retransmit = mod->pub->retransmit;
		pub.period = mod->pub->period;
		pub.period_div = mod->pub->period_div;
		pub.cred = mod->pub->cred;

		err = settings_save_one(path, &pub, sizeof(pub));
	}

	if (err) {
		BT_ERR("Failed to store %s value", log_strdup(path));
	} else {
		BT_DBG("Stored %s value", log_strdup(path));
	}
}

static void store_pending_mod(struct bt_mesh_model *mod,
			      struct bt_mesh_elem *elem, bool vnd,
			      bool primary, void *user_data)
{
	if (!mod->flags) {
		return;
	}

	if (mod->flags & BT_MESH_MOD_BIND_PENDING) {
		mod->flags &= ~BT_MESH_MOD_BIND_PENDING;
		store_pending_mod_bind(mod, vnd);
	}

	if (mod->flags & BT_MESH_MOD_SUB_PENDING) {
		mod->flags &= ~BT_MESH_MOD_SUB_PENDING;
		store_pending_mod_sub(mod, vnd);
	}

	if (mod->flags & BT_MESH_MOD_PUB_PENDING) {
		mod->flags &= ~BT_MESH_MOD_PUB_PENDING;
		store_pending_mod_pub(mod, vnd);
	}
}

void bt_mesh_model_pending_store(void)
{
	bt_mesh_model_foreach(store_pending_mod, NULL);
}

void bt_mesh_model_bind_store(struct bt_mesh_model *mod)
{
	mod->flags |= BT_MESH_MOD_BIND_PENDING;
	bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_MOD_PENDING);
}

void bt_mesh_model_sub_store(struct bt_mesh_model *mod)
{
	mod->flags |= BT_MESH_MOD_SUB_PENDING;
	bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_MOD_PENDING);
}

void bt_mesh_model_pub_store(struct bt_mesh_model *mod)
{
	mod->flags |= BT_MESH_MOD_PUB_PENDING;
	bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_MOD_PENDING);
}

int bt_mesh_model_data_store(struct bt_mesh_model *mod, bool vnd,
			     const char *name, const void *data,
			     size_t data_len)
{
	char path[30];
	int err;

	encode_mod_path(mod, vnd, "data", path, sizeof(path));
	if (name) {
		strcat(path, "/");
		strncat(path, name, 8);
	}

	if (data_len) {
		err = settings_save_one(path, data, data_len);
	} else {
		err = settings_delete(path);
	}

	if (err) {
		BT_ERR("Failed to store %s value", log_strdup(path));
	} else {
		BT_DBG("Stored %s value", log_strdup(path));
	}
	return err;
}

static void commit_mod(struct bt_mesh_model *mod, struct bt_mesh_elem *elem,
		       bool vnd, bool primary, void *user_data)
{
	if (mod->pub && mod->pub->update &&
	    mod->pub->addr != BT_MESH_ADDR_UNASSIGNED) {
		int32_t ms = bt_mesh_model_pub_period_get(mod);

		if (ms > 0) {
			BT_DBG("Starting publish timer (period %u ms)", ms);
			k_work_schedule(&mod->pub->timer, K_MSEC(ms));
		}
	}

	if (!IS_ENABLED(CONFIG_BT_MESH_LOW_POWER)) {
		return;
	}

	for (int i = 0; i < ARRAY_SIZE(mod->groups); i++) {
		if (mod->groups[i] != BT_MESH_ADDR_UNASSIGNED) {
			bt_mesh_lpn_group_add(mod->groups[i]);
		}
	}
}

void bt_mesh_model_settings_commit(void)
{
	bt_mesh_model_foreach(commit_mod, NULL);
}
