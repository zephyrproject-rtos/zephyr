/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <errno.h>
#include <stdlib.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/net/buf.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/mesh.h>

#include "common/bt_str.h"

#include "host/testing.h"

#include "mesh.h"
#include "adv.h"
#include "net.h"
#include "lpn.h"
#include "transport.h"
#include "access.h"
#include "foundation.h"
#include "op_agg.h"
#include "settings.h"

#define LOG_LEVEL CONFIG_BT_MESH_ACCESS_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_access);

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

struct comp_foreach_model_arg {
	struct net_buf_simple *buf;
	size_t *offset;
};

static const struct bt_mesh_comp *dev_comp;
static uint16_t dev_primary_addr;
static void (*msg_cb)(uint32_t opcode, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf);

/* Structure containing information about model extension */
struct mod_relation {
	/** Element that composition data base model belongs to. */
	uint8_t elem_base;
	/** Index of composition data base model in its element. */
	uint8_t idx_base;
	/** Element that composition data extension model belongs to. */
	uint8_t elem_ext;
	/** Index of composition data extension model in its element. */
	uint8_t idx_ext;
	/** Type of relation; value in range 0x00-0xFE marks correspondence
	 * and equals to Correspondence ID; value 0xFF marks extension
	 */
	uint8_t type;
};

#ifdef CONFIG_BT_MESH_MODEL_EXTENSION_LIST_SIZE
#define MOD_REL_LIST_SIZE CONFIG_BT_MESH_MODEL_EXTENSION_LIST_SIZE
#else
#define MOD_REL_LIST_SIZE 0
#endif

/* List of all existing extension relations between models */
static struct mod_relation mod_rel_list[MOD_REL_LIST_SIZE];

#define MOD_REL_LIST_FOR_EACH(idx) \
	for ((idx) = 0; \
		(idx) < ARRAY_SIZE(mod_rel_list) && \
		!(mod_rel_list[(idx)].elem_base == 0 && \
		  mod_rel_list[(idx)].idx_base == 0 && \
		  mod_rel_list[(idx)].elem_ext == 0 && \
		  mod_rel_list[(idx)].idx_ext == 0); \
		 (idx)++)

#define IS_MOD_BASE(mod, idx) \
	(mod_rel_list[(idx)].elem_base == (mod)->elem_idx && \
	 mod_rel_list[(idx)].idx_base == (mod)->mod_idx &&   \
	 !(mod_rel_list[(idx)].elem_ext != (mod)->elem_idx && \
	   mod_rel_list[(idx)].idx_ext != (mod)->mod_idx))

#define IS_MOD_EXTENSION(mod, idx) \
	 (mod_rel_list[(idx)].elem_ext == (mod)->elem_idx && \
	  mod_rel_list[(idx)].idx_ext == (mod)->mod_idx &&   \
	  !(mod_rel_list[(idx)].elem_base != (mod)->elem_idx && \
	    mod_rel_list[(idx)].idx_base != (mod)->mod_idx))

#define RELATION_TYPE_EXT 0xFF

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

static size_t bt_mesh_comp_elem_size(const struct bt_mesh_elem *elem)
{
	return (4 + (elem->model_count * 2U) + (elem->vnd_model_count * 4U));
}

static uint8_t *data_buf_add_u8_offset(struct net_buf_simple *buf,
				       uint8_t val, size_t *offset)
{
	if (*offset >= 1) {
		*offset -= 1;
		return NULL;
	}

	return net_buf_simple_add_u8(buf, val);
}

static void data_buf_add_le16_offset(struct net_buf_simple *buf,
				     uint16_t val, size_t *offset)
{
	if (*offset >= 2) {
		*offset -= 2;
		return;
	} else if (*offset == 1) {
		*offset -= 1;
		net_buf_simple_add_u8(buf, (val >> 8));
	} else {
		net_buf_simple_add_le16(buf, val);
	}
}

static void comp_add_model(struct bt_mesh_model *mod, struct bt_mesh_elem *elem,
			   bool vnd, void *user_data)
{
	struct comp_foreach_model_arg *arg = user_data;

	if (vnd) {
		data_buf_add_le16_offset(arg->buf, mod->vnd.company, arg->offset);
		data_buf_add_le16_offset(arg->buf, mod->vnd.id, arg->offset);
	} else {
		data_buf_add_le16_offset(arg->buf, mod->id, arg->offset);
	}
}

#if defined(CONFIG_BT_MESH_LARGE_COMP_DATA_SRV)
static void data_buf_add_mem_offset(struct net_buf_simple *buf,
				    const void *mem, size_t len,
				    size_t *offset)
{
	if (*offset >= len) {
		*offset -= len;
		return;
	} else if (*offset > 0) {
		net_buf_simple_add_mem(buf, ((uint8_t *)mem), (len - *offset));

	} else {
		net_buf_simple_add_mem(buf, mem, len);
	}
}

static size_t metadata_model_size(struct bt_mesh_model *mod,
				  struct bt_mesh_elem *elem, bool vnd)
{
	const struct bt_mesh_models_metadata_entry *entry;
	size_t size = 0;

	if (!mod->metadata) {
		return size;
	}

	if (vnd) {
		size += sizeof(mod->vnd.company);
		size += sizeof(mod->vnd.id);
	} else {
		size += sizeof(mod->id);
	}

	size += sizeof(uint8_t);

	for (entry = *mod->metadata; entry && entry->len; ++entry) {
		size += sizeof(entry->len) + sizeof(entry->id) + entry->len;
	}

	return size;
}

size_t bt_mesh_metadata_page_0_size(void)
{
	const struct bt_mesh_comp *comp;
	size_t size = 0;
	int i, j;

	comp = bt_mesh_comp_get();

	for (i = 0; i < dev_comp->elem_count; i++) {
		struct bt_mesh_elem *elem = &dev_comp->elem[i];

		size += sizeof(elem->model_count) +
			sizeof(elem->vnd_model_count);

		for (j = 0; j < elem->model_count; j++) {
			struct bt_mesh_model *model = &elem->models[j];

			size += metadata_model_size(model, elem, false);
		}

		for (j = 0; j < elem->vnd_model_count; j++) {
			struct bt_mesh_model *model = &elem->vnd_models[j];

			size += metadata_model_size(model, elem, true);
		}
	}

	return size;
}

static int metadata_add_model(struct bt_mesh_model *mod,
			      struct bt_mesh_elem *elem, bool vnd,
			      void *user_data)
{
	const struct bt_mesh_models_metadata_entry *entry;
	struct comp_foreach_model_arg *arg = user_data;
	struct net_buf_simple *buf = arg->buf;
	size_t *offset = arg->offset;
	size_t model_size;
	uint8_t count = 0;
	uint8_t *count_ptr;

	model_size = metadata_model_size(mod, elem, vnd);

	if (*offset >= model_size) {
		*offset -= model_size;
		return 0;
	}

	if (net_buf_simple_tailroom(buf) < (model_size + BT_MESH_MIC_SHORT)) {
		LOG_DBG("Model metadata didn't fit in the buffer");
		return -E2BIG;
	}

	comp_add_model(mod, elem, vnd, user_data);

	count_ptr = data_buf_add_u8_offset(buf, 0, offset);

	if (mod->metadata) {
		for (entry = *mod->metadata; entry && entry->data != NULL; ++entry) {
			data_buf_add_le16_offset(buf, entry->len, offset);
			data_buf_add_le16_offset(buf, entry->id, offset);
			data_buf_add_mem_offset(buf, entry->data, entry->len, offset);
			count++;
		}
	}

	if (count_ptr) {
		*count_ptr = count;
	}

	return 0;
}

int bt_mesh_metadata_get_page_0(struct net_buf_simple *buf, size_t offset)
{
	const struct bt_mesh_comp *comp;
	struct comp_foreach_model_arg arg = {
		.buf = buf,
		.offset = &offset,
	};
	uint8_t *mod_count_ptr;
	uint8_t *vnd_count_ptr;
	int i, j, err;

	comp = bt_mesh_comp_get();

	for (i = 0; i < comp->elem_count; i++) {
		struct bt_mesh_elem *elem = &dev_comp->elem[i];

		/* Check that the buffer has available tailroom for metadata item counts */
		if (net_buf_simple_tailroom(buf) < (((offset == 0) ? 2 : (offset == 1) ? 1 : 0)
				+ BT_MESH_MIC_SHORT)) {
			LOG_DBG("Model metadata didn't fit in the buffer");
			return -E2BIG;
		}
		mod_count_ptr = data_buf_add_u8_offset(buf, 0, &offset);
		vnd_count_ptr = data_buf_add_u8_offset(buf, 0, &offset);

		for (j = 0; j < elem->model_count; j++) {
			struct bt_mesh_model *model = &elem->models[j];

			if (!model->metadata) {
				continue;
			}

			err = metadata_add_model(model, elem, false, &arg);
			if (err) {
				return err;
			}

			if (mod_count_ptr) {
				(*mod_count_ptr) += 1;
			}
		}

		for (j = 0; j < elem->vnd_model_count; j++) {
			struct bt_mesh_model *model = &elem->vnd_models[j];

			if (!model->metadata) {
				continue;
			}

			err = metadata_add_model(model, elem, true, &arg);
			if (err) {
				return err;
			}

			if (vnd_count_ptr) {
				(*vnd_count_ptr) += 1;
			}
		}
	}

	return 0;
}
#endif

size_t bt_mesh_comp_page_0_size(void)
{
	const struct bt_mesh_comp *comp;
	const struct bt_mesh_elem *elem;
	size_t size = 10;
	int i;

	comp = bt_mesh_comp_get();

	for (i = 0; i < comp->elem_count; i++) {
		elem = &comp->elem[i];
		size += bt_mesh_comp_elem_size(elem);
	}

	return size;
}

static int comp_add_elem(struct net_buf_simple *buf, struct bt_mesh_elem *elem,
			 size_t *offset)
{
	struct comp_foreach_model_arg arg = {
		.buf = buf,
		.offset = offset,
	};
	const size_t elem_size = bt_mesh_comp_elem_size(elem);
	int i;

	if (*offset >= elem_size) {
		*offset -= elem_size;
		return 0;
	}

	if (net_buf_simple_tailroom(buf) < (elem_size + BT_MESH_MIC_SHORT)) {
		if (IS_ENABLED(CONFIG_BT_MESH_LARGE_COMP_DATA_SRV)) {
			/* Mesh Profile 1.1 Section 4.4.1.2.2:
			 * If the complete list of models does not fit in the Data field,
			 * the element shall not be reported.
			 */
			LOG_DBG("Element 0x%04x didn't fit in the Data field",
				elem->addr);
			return 0;
		}

		LOG_ERR("Too large device composition");
		return -E2BIG;
	}

	data_buf_add_le16_offset(buf, elem->loc, offset);

	data_buf_add_u8_offset(buf, elem->model_count, offset);
	data_buf_add_u8_offset(buf, elem->vnd_model_count, offset);

	for (i = 0; i < elem->model_count; i++) {
		struct bt_mesh_model *model = &elem->models[i];

		comp_add_model(model, elem, false, &arg);
	}

	for (i = 0; i < elem->vnd_model_count; i++) {
		struct bt_mesh_model *model = &elem->vnd_models[i];

		comp_add_model(model, elem, true, &arg);
	}

	return 0;
}

int bt_mesh_comp_data_get_page_0(struct net_buf_simple *buf, size_t offset)
{
	uint16_t feat = 0U;
	const struct bt_mesh_comp *comp;
	int i;

	comp = bt_mesh_comp_get();

	if (IS_ENABLED(CONFIG_BT_MESH_RELAY)) {
		feat |= BT_MESH_FEAT_RELAY;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_GATT_PROXY)) {
		feat |= BT_MESH_FEAT_PROXY;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_FRIEND)) {
		feat |= BT_MESH_FEAT_FRIEND;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_LOW_POWER)) {
		feat |= BT_MESH_FEAT_LOW_POWER;
	}

	data_buf_add_le16_offset(buf, comp->cid, &offset);
	data_buf_add_le16_offset(buf, comp->pid, &offset);
	data_buf_add_le16_offset(buf, comp->vid, &offset);
	data_buf_add_le16_offset(buf, CONFIG_BT_MESH_CRPL, &offset);
	data_buf_add_le16_offset(buf, feat, &offset);

	for (i = 0; i < comp->elem_count; i++) {
		int err;

		err = comp_add_elem(buf, &comp->elem[i], &offset);
		if (err) {
			return err;
		}
	}

	return 0;
}

static uint8_t count_mod_ext(struct bt_mesh_model *mod, uint8_t *max_offset)
{
	int i;
	uint8_t extensions = 0;
	int8_t offset, offset_record = 0;

	MOD_REL_LIST_FOR_EACH(i) {
		if (IS_MOD_EXTENSION(mod, i) &&
		    mod_rel_list[i].type == RELATION_TYPE_EXT) {
			extensions++;
			offset = mod_rel_list[i].elem_ext -
				mod_rel_list[i].elem_base;
			if (abs(offset) > abs(offset_record)) {
				offset_record = offset;
			}
		}
	}
	memcpy(max_offset, &offset_record, sizeof(uint8_t));
	return extensions;
}

static bool is_cor_present(struct bt_mesh_model *mod, uint8_t *cor_id)
{
	int i;

	MOD_REL_LIST_FOR_EACH(i) {
		if ((IS_MOD_BASE(mod, i) ||
		     IS_MOD_EXTENSION(mod, i)) &&
		     mod_rel_list[i].type < RELATION_TYPE_EXT) {
			memcpy(cor_id, &mod_rel_list[i].type, sizeof(uint8_t));
			return true;
		}
	}
	return false;
}

static void prep_model_item_header(struct bt_mesh_model *mod, uint8_t *cor_id,
			    uint8_t *mod_cnt, struct net_buf_simple *buf)
{
	uint8_t ext_mod_cnt;
	bool cor_present;
	uint8_t mod_elem_info = 0;
	int8_t max_offset;

	ext_mod_cnt = count_mod_ext(mod, &max_offset);
	cor_present = is_cor_present(mod, cor_id);

	mod_elem_info = ext_mod_cnt << 2;
	if (ext_mod_cnt > 31 ||
		max_offset > 3 ||
		max_offset < -4) {
		mod_elem_info |= BIT(1);
	}
	if (cor_present) {
		mod_elem_info |= BIT(0);
	}
	net_buf_simple_add_u8(buf, mod_elem_info);

	if (cor_present) {
		net_buf_simple_add_u8(buf, *cor_id);
	}
	memset(mod_cnt, ext_mod_cnt, sizeof(uint8_t));
}

static void add_items_to_page(struct net_buf_simple *buf, struct bt_mesh_model *mod,
		       uint8_t ext_mod_cnt)
{
	int i, offset;
	uint8_t mod_idx;

	MOD_REL_LIST_FOR_EACH(i) {
		if (IS_MOD_EXTENSION(mod, i)) {
			offset = mod->elem_idx - mod_rel_list[i].elem_base;
			mod_idx = mod_rel_list[i].idx_base;
			if (ext_mod_cnt < 32 &&
				offset < 4 &&
				offset > -5) {
				/* short format */
				if (offset < 0) {
					offset += 8;
				}

				offset |= mod_idx << 3;
				net_buf_simple_add_u8(buf, offset);
			} else {
				/* long format */
				if (offset < 0) {
					offset += 256;
				}
				net_buf_simple_add_u8(buf, offset);
				net_buf_simple_add_u8(buf, mod_idx);
			}
		}
	}
}

int bt_mesh_comp_data_get_page_1(struct net_buf_simple *buf)
{
	const struct bt_mesh_comp *comp;
	uint8_t cor_id = 0;
	uint8_t ext_mod_cnt = 0;
	int i, j;

	comp = bt_mesh_comp_get();

	for (i = 0; i < comp->elem_count; i++) {
		net_buf_simple_add_u8(buf, comp->elem[i].model_count);
		net_buf_simple_add_u8(buf, comp->elem[i].vnd_model_count);
		for (j = 0; j < comp->elem[i].model_count; j++) {
			prep_model_item_header(&comp->elem[i].models[j],
					       &cor_id, &ext_mod_cnt, buf);
			if (ext_mod_cnt != 0) {
				add_items_to_page(buf,
						  &comp->elem[i].models[j],
						  ext_mod_cnt);
			}
		}

		for (j = 0; j < comp->elem[i].vnd_model_count; j++) {
			prep_model_item_header(&comp->elem[i].vnd_models[j],
					       &cor_id, &ext_mod_cnt, buf);
			if (ext_mod_cnt != 0) {
				add_items_to_page(buf,
						  &comp->elem[i].vnd_models[j],
						  ext_mod_cnt);
			}
		}
	}
	return 0;
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
		if (!period) {
			return 0;
		}

		return MAX(period >> mod->pub->period_div, 100);
	} else {
		return period;
	}
}

static int32_t next_period(struct bt_mesh_model *mod)
{
	struct bt_mesh_model_pub *pub = mod->pub;
	uint32_t period = 0;
	uint32_t elapsed;

	elapsed = k_uptime_get_32() - pub->period_start;
	LOG_DBG("Publishing took %ums", elapsed);

	if (mod->pub->count) {
		/* If a message is to be retransmitted, period should include time since the first
		 * publication until the last publication.
		 */
		period = BT_MESH_PUB_TRANSMIT_INT(mod->pub->retransmit);
		period *= BT_MESH_PUB_MSG_NUM(mod->pub);

		if (period && elapsed >= period) {
			LOG_WRN("Retransmission interval is too short");
			/* Return smallest positive number since 0 means disabled */
			return 1;
		}
	}

	if (!period) {
		period = bt_mesh_model_pub_period_get(mod);
		if (!period) {
			return 0;
		}
	}

	if (elapsed >= period) {
		LOG_WRN("Publication sending took longer than the period");
		/* Return smallest positive number since 0 means disabled */
		return 1;
	}

	return period - elapsed;
}

static void publish_sent(int err, void *user_data)
{
	struct bt_mesh_model *mod = user_data;
	int32_t delay;

	LOG_DBG("err %d, time %u", err, k_uptime_get_32());

	delay = next_period(mod);

	if (delay) {
		LOG_DBG("Publishing next time in %dms", delay);
		/* Using schedule() in case the application has already called
		 * bt_mesh_publish, and a publication is pending.
		 */
		k_work_schedule(&mod->pub->timer, K_MSEC(delay));
	}
}

static void publish_start(uint16_t duration, int err, void *user_data)
{
	if (err) {
		LOG_ERR("Failed to publish: err %d", err);
		publish_sent(err, user_data);
		return;
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
	struct bt_mesh_msg_ctx ctx = BT_MESH_MSG_CTX_INIT_PUB(pub);
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

	pub->period_start = k_uptime_get_32();

	if (err) {
		/* Skip this publish attempt. */
		LOG_DBG("Update failed, skipping publish (err: %d)", err);
		pub->count = 0;
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

	LOG_DBG("%u", k_uptime_get_32());

	if (pub->count) {
		pub->count--;

		if (pub->retr_update && pub->update &&
		    bt_mesh_model_pub_is_retransmission(pub->mod)) {
			err = pub->update(pub->mod);
			if (err) {
				publish_sent(err, pub->mod);
				return;
			}
		}
	} else {
		/* First publication in this period */
		err = pub_period_start(pub);
		if (err) {
			return;
		}
	}

	err = publish_transmit(pub->mod);
	if (err) {
		LOG_ERR("Failed to publish (err %d)", err);
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
		LOG_ERR("Invalid element index %u", elem_idx);
		return NULL;
	}

	elem = &dev_comp->elem[elem_idx];

	if (vnd) {
		if (mod_idx >= elem->vnd_model_count) {
			LOG_ERR("Invalid vendor model index %u", mod_idx);
			return NULL;
		}

		return &elem->vnd_models[mod_idx];
	} else {
		if (mod_idx >= elem->model_count) {
			LOG_ERR("Invalid SIG model index %u", mod_idx);
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

		LOG_ERR("Invalid vendor model(company:0x%04x"
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

	for (i = 0; i < mod->keys_cnt; i++) {
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

	if (IS_ENABLED(CONFIG_BT_MESH_COMP_PAGE_1)) {
		memset(mod_rel_list, 0, sizeof(mod_rel_list));
	}

	bt_mesh_model_foreach(mod_init, &err);

	if (IS_ENABLED(CONFIG_BT_MESH_COMP_PAGE_1)) {
		int i;

		MOD_REL_LIST_FOR_EACH(i) {
			LOG_DBG("registered %s",
				mod_rel_list[i].type < RELATION_TYPE_EXT ?
				"correspondence" : "extension");
			LOG_DBG("\tbase: elem %u idx %u",
				mod_rel_list[i].elem_base,
				mod_rel_list[i].idx_base);
			LOG_DBG("\text: elem %u idx %u",
				mod_rel_list[i].elem_ext,
				mod_rel_list[i].idx_ext);
		}
		if (i < MOD_REL_LIST_SIZE) {
			LOG_WRN("Unused space in relation list: %d",
				MOD_REL_LIST_SIZE - i);
		}
	}

	return err;
}

void bt_mesh_comp_provision(uint16_t addr)
{
	int i;

	dev_primary_addr = addr;

	LOG_DBG("addr 0x%04x elem_count %zu", addr, dev_comp->elem_count);

	for (i = 0; i < dev_comp->elem_count; i++) {
		struct bt_mesh_elem *elem = &dev_comp->elem[i];

		elem->addr = addr++;

		LOG_DBG("addr 0x%04x mod_count %u vnd_mod_count %u", elem->addr, elem->model_count,
			elem->vnd_model_count);
	}
}

void bt_mesh_comp_unprovision(void)
{
	LOG_DBG("");

	dev_primary_addr = BT_MESH_ADDR_UNASSIGNED;
}

uint16_t bt_mesh_primary_addr(void)
{
	return dev_primary_addr;
}

static uint16_t *model_group_get(struct bt_mesh_model *mod, uint16_t addr)
{
	int i;

	for (i = 0; i < mod->groups_cnt; i++) {
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

static enum bt_mesh_walk find_group_mod_visitor(struct bt_mesh_model *mod, void *user_data)
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

	bt_mesh_model_extensions_walk(*mod, find_group_mod_visitor, &ctx);

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

int bt_mesh_access_send(struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf, uint16_t src_addr,
			const struct bt_mesh_send_cb *cb, void *cb_data)
{
	struct bt_mesh_net_tx tx = {
		.ctx = ctx,
		.src = src_addr,
	};

	LOG_DBG("net_idx 0x%04x app_idx 0x%04x dst 0x%04x", tx.ctx->net_idx, tx.ctx->app_idx,
		tx.ctx->addr);
	LOG_DBG("len %u: %s", buf->len, bt_hex(buf->data, buf->len));

	if (!bt_mesh_is_provisioned()) {
		LOG_ERR("Local node is not yet provisioned");
		return -EAGAIN;
	}

	return bt_mesh_trans_send(&tx, buf, cb, cb_data);
}

uint8_t bt_mesh_elem_count(void)
{
	return dev_comp->elem_count;
}

bool bt_mesh_model_has_key(struct bt_mesh_model *mod, uint16_t key)
{
	int i;

	for (i = 0; i < mod->keys_cnt; i++) {
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
	} else if (BT_MESH_ADDR_IS_GROUP(dst) || BT_MESH_ADDR_IS_VIRTUAL(dst) ||
		  (BT_MESH_ADDR_IS_FIXED_GROUP(dst) &&  mod->elem_idx != 0)) {
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
			LOG_ERR("Ignoring RFU OpCode");
			return -EINVAL;
		}

		*opcode = net_buf_simple_pull_u8(buf);
		return 0;
	case 0x02:
		if (buf->len < 2) {
			LOG_ERR("Too short payload for 2-octet OpCode");
			return -EINVAL;
		}

		*opcode = net_buf_simple_pull_be16(buf);
		return 0;
	case 0x03:
		if (buf->len < 3) {
			LOG_ERR("Too short payload for 3-octet OpCode");
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

static int element_model_recv(struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf, uint16_t addr,
			      struct bt_mesh_elem *elem, uint32_t opcode)
{
	const struct bt_mesh_model_op *op;
	struct bt_mesh_model *model;
	struct net_buf_simple_state state;
	int err;

	op = find_op(elem, opcode, &model);
	if (!op) {
		LOG_ERR("No OpCode 0x%08x for elem 0x%02x", opcode, elem->addr);
		return ACCESS_STATUS_WRONG_OPCODE;
	}

	if (!bt_mesh_model_has_key(model, ctx->app_idx)) {
		LOG_ERR("Wrong key");
		return ACCESS_STATUS_WRONG_KEY;
	}

	if (!model_has_dst(model, addr)) {
		LOG_ERR("Invalid address 0x%02x", addr);
		return ACCESS_STATUS_INVALID_ADDRESS;
	}

	if ((op->len >= 0) && (buf->len < (size_t)op->len)) {
		LOG_ERR("Too short message for OpCode 0x%08x", opcode);
		return ACCESS_STATUS_MESSAGE_NOT_UNDERSTOOD;
	} else if ((op->len < 0) && (buf->len != (size_t)(-op->len))) {
		LOG_ERR("Invalid message size for OpCode 0x%08x", opcode);
		return ACCESS_STATUS_MESSAGE_NOT_UNDERSTOOD;
	}

	net_buf_simple_save(buf, &state);
	err = op->func(model, ctx, buf);
	net_buf_simple_restore(buf, &state);

	if (err) {
		return ACCESS_STATUS_MESSAGE_NOT_UNDERSTOOD;
	}
	return ACCESS_STATUS_SUCCESS;
}

int bt_mesh_model_recv(struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
	int err = ACCESS_STATUS_SUCCESS;
	uint32_t opcode;
	uint16_t index;

	LOG_DBG("app_idx 0x%04x src 0x%04x dst 0x%04x", ctx->app_idx, ctx->addr,
		ctx->recv_dst);
	LOG_DBG("len %u: %s", buf->len, bt_hex(buf->data, buf->len));

	if (IS_ENABLED(CONFIG_BT_TESTING)) {
		bt_test_mesh_model_recv(ctx->addr, ctx->recv_dst, buf->data,
					buf->len);
	}

	if (get_opcode(buf, &opcode) < 0) {
		LOG_WRN("Unable to decode OpCode");
		return ACCESS_STATUS_WRONG_OPCODE;
	}

	LOG_DBG("OpCode 0x%08x", opcode);

	if (BT_MESH_ADDR_IS_UNICAST(ctx->recv_dst)) {
		index = ctx->recv_dst - dev_comp->elem[0].addr;

		if (index >= dev_comp->elem_count) {
			LOG_ERR("Invalid address 0x%02x", ctx->recv_dst);
			err = ACCESS_STATUS_INVALID_ADDRESS;
		} else {
			struct bt_mesh_elem *elem = &dev_comp->elem[index];

			err = element_model_recv(ctx, buf, ctx->recv_dst, elem, opcode);
		}
	} else {
		for (index = 0; index < dev_comp->elem_count; index++) {
			struct bt_mesh_elem *elem = &dev_comp->elem[index];

			(void)element_model_recv(ctx, buf, ctx->recv_dst, elem, opcode);
		}
	}

	if (IS_ENABLED(CONFIG_BT_MESH_ACCESS_LAYER_MSG) && msg_cb) {
		msg_cb(opcode, ctx, buf);
	}

	return err;
}

int bt_mesh_model_send(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		       struct net_buf_simple *msg,
		       const struct bt_mesh_send_cb *cb, void *cb_data)
{
	if (IS_ENABLED(CONFIG_BT_MESH_OP_AGG) && bt_mesh_op_agg_accept(ctx)) {
		return bt_mesh_op_agg_send(model, ctx, msg, cb);
	}

	if (!bt_mesh_model_has_key(model, ctx->app_idx)) {
		LOG_ERR("Model not bound to AppKey 0x%04x", ctx->app_idx);
		return -EINVAL;
	}

	return bt_mesh_access_send(ctx, msg, bt_mesh_model_elem(model)->addr, cb, cb_data);
}

int bt_mesh_model_publish(struct bt_mesh_model *model)
{
	struct bt_mesh_model_pub *pub = model->pub;

	if (!pub) {
		return -ENOTSUP;
	}

	LOG_DBG("");

	if (pub->addr == BT_MESH_ADDR_UNASSIGNED) {
		return -EADDRNOTAVAIL;
	}

	if (!pub->msg || !pub->msg->len) {
		LOG_ERR("No publication message");
		return -EINVAL;
	}

	if (pub->msg->len + BT_MESH_MIC_SHORT > BT_MESH_TX_SDU_MAX) {
		LOG_ERR("Message does not fit maximum SDU size");
		return -EMSGSIZE;
	}

	if (pub->count) {
		LOG_WRN("Clearing publish retransmit timer");
	}

	/* Account for initial transmission */
	pub->count = BT_MESH_PUB_MSG_TOTAL(pub);
	pub->period_start = k_uptime_get_32();

	LOG_DBG("Publish Retransmit Count %u Interval %ums", pub->count,
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

void bt_mesh_model_extensions_walk(struct bt_mesh_model *model,
				   enum bt_mesh_walk (*cb)(struct bt_mesh_model *mod,
							   void *user_data),
				   void *user_data)
{
#ifndef CONFIG_BT_MESH_MODEL_EXTENSIONS
	(void)cb(model, user_data);
	return;
#else
	struct bt_mesh_model *it;

	if (cb(model, user_data) == BT_MESH_WALK_STOP || !model->next) {
		return;
	}

	/* List is circular. Step through all models until we reach the start: */
	for (it = model->next; it != model; it = it->next) {
		if (cb(it, user_data) == BT_MESH_WALK_STOP) {
			return;
		}
	}
#endif
}

#ifdef CONFIG_BT_MESH_MODEL_EXTENSIONS
static int mod_rel_register(struct bt_mesh_model *base,
				 struct bt_mesh_model *ext,
				 uint8_t type)
{
	LOG_DBG("");
	struct mod_relation extension = {
		base->elem_idx,
		base->mod_idx,
		ext->elem_idx,
		ext->mod_idx,
		type,
	};
	int i;

	for (i = 0; i < ARRAY_SIZE(mod_rel_list); i++) {
		if (mod_rel_list[i].elem_base == 0 &&
			mod_rel_list[i].idx_base == 0 &&
			mod_rel_list[i].elem_ext == 0 &&
			mod_rel_list[i].idx_ext == 0) {
			memcpy(&mod_rel_list[i], &extension,
			       sizeof(extension));
			return 0;
		}
	}
	LOG_ERR("Failed to extend");
	return -ENOMEM;
}

int bt_mesh_model_extend(struct bt_mesh_model *extending_mod, struct bt_mesh_model *base_mod)
{
	struct bt_mesh_model *a = extending_mod;
	struct bt_mesh_model *b = base_mod;
	struct bt_mesh_model *a_next = a->next;
	struct bt_mesh_model *b_next = b->next;
	struct bt_mesh_model *it;

	base_mod->flags |= BT_MESH_MOD_EXTENDED;

	if (a == b) {
		return 0;
	}

	/* Check if a's list contains b */
	for (it = a; (it != NULL) && (it->next != a); it = it->next) {
		if (it == b) {
			return 0;
		}
	}

	/* Merge lists */
	if (a_next) {
		b->next = a_next;
	} else {
		b->next = a;
	}

	if (b_next) {
		a->next = b_next;
	} else {
		a->next = b;
	}


	if (IS_ENABLED(CONFIG_BT_MESH_COMP_PAGE_1)) {
		return mod_rel_register(base_mod, extending_mod, RELATION_TYPE_EXT);
	}

	return 0;
}

int bt_mesh_model_correspond(struct bt_mesh_model *corresponding_mod,
			     struct bt_mesh_model *base_mod)
{
	int i, err;
	uint8_t cor_id = 0;

	if (!IS_ENABLED(CONFIG_BT_MESH_COMP_PAGE_1)) {
		return -ENOTSUP;
	}

	MOD_REL_LIST_FOR_EACH(i) {
		if (mod_rel_list[i].type < RELATION_TYPE_EXT &&
		    mod_rel_list[i].type > cor_id) {
			cor_id = mod_rel_list[i].type;
		}

		if ((IS_MOD_BASE(base_mod, i) ||
		     IS_MOD_EXTENSION(base_mod, i) ||
		     IS_MOD_BASE(corresponding_mod, i) ||
		     IS_MOD_EXTENSION(corresponding_mod, i)) &&
		    mod_rel_list[i].type < RELATION_TYPE_EXT) {
			return mod_rel_register(base_mod, corresponding_mod, mod_rel_list[i].type);
		}
	}
	err = mod_rel_register(base_mod, corresponding_mod, cor_id);
	if (err) {
		return err;
	}
	return 0;
}
#endif /* CONFIG_BT_MESH_MODEL_EXTENSIONS */

bool bt_mesh_model_is_extended(struct bt_mesh_model *model)
{
	return model->flags & BT_MESH_MOD_EXTENDED;
}

static int mod_set_bind(struct bt_mesh_model *mod, size_t len_rd,
			settings_read_cb read_cb, void *cb_arg)
{
	ssize_t len;
	int i;

	/* Start with empty array regardless of cleared or set value */
	for (i = 0; i < mod->keys_cnt; i++) {
		mod->keys[i] = BT_MESH_KEY_UNUSED;
	}

	if (len_rd == 0) {
		LOG_DBG("Cleared bindings for model");
		return 0;
	}

	len = read_cb(cb_arg, mod->keys, mod->keys_cnt * sizeof(mod->keys[0]));
	if (len < 0) {
		LOG_ERR("Failed to read value (err %zd)", len);
		return len;
	}

	LOG_HEXDUMP_DBG(mod->keys, len, "val");

	LOG_DBG("Decoded %zu bound keys for model", len / sizeof(mod->keys[0]));
	return 0;
}

static int mod_set_sub(struct bt_mesh_model *mod, size_t len_rd,
		       settings_read_cb read_cb, void *cb_arg)
{
	size_t size = mod->groups_cnt * sizeof(mod->groups[0]);
	ssize_t len;

	/* Start with empty array regardless of cleared or set value */
	(void)memset(mod->groups, 0, size);

	if (len_rd == 0) {
		LOG_DBG("Cleared subscriptions for model");
		return 0;
	}

	len = read_cb(cb_arg, mod->groups, size);
	if (len < 0) {
		LOG_ERR("Failed to read value (err %zd)", len);
		return len;
	}

	LOG_HEXDUMP_DBG(mod->groups, len, "val");

	LOG_DBG("Decoded %zu subscribed group addresses for model", len / sizeof(mod->groups[0]));
	return 0;
}

static int mod_set_pub(struct bt_mesh_model *mod, size_t len_rd,
		       settings_read_cb read_cb, void *cb_arg)
{
	struct mod_pub_val pub;
	int err;

	if (!mod->pub) {
		LOG_WRN("Model has no publication context!");
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

		LOG_DBG("Cleared publication for model");
		return 0;
	}

	if (!IS_ENABLED(CONFIG_BT_SETTINGS)) {
		return 0;
	}

	err = bt_mesh_settings_set(read_cb, cb_arg, &pub, sizeof(pub));
	if (err) {
		LOG_ERR("Failed to set \'model-pub\'");
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

	LOG_DBG("Restored model publication, dst 0x%04x app_idx 0x%03x", pub.addr, pub.key);

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
		LOG_ERR("Insufficient number of arguments");
		return -ENOENT;
	}

	mod_key = strtol(name, NULL, 16);
	elem_idx = mod_key >> 8;
	mod_idx = mod_key;

	LOG_DBG("Decoded mod_key 0x%04x as elem_idx %u mod_idx %u", mod_key, elem_idx, mod_idx);

	mod = bt_mesh_model_get(vnd, elem_idx, mod_idx);
	if (!mod) {
		LOG_ERR("Failed to get model for elem_idx %u mod_idx %u", elem_idx, mod_idx);
		return -ENOENT;
	}

	len = settings_name_next(name, &next);

	if (!next) {
		LOG_ERR("Insufficient number of arguments");
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

	LOG_WRN("Unknown module key %s", next);
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

static int comp_set(const char *name, size_t len_rd, settings_read_cb read_cb,
		    void *cb_arg)
{
	/* Only need to know that the entry exists. Will load the contents on
	 * demand.
	 */
	if (len_rd > 0) {
		atomic_set_bit(bt_mesh.flags, BT_MESH_COMP_DIRTY);
	}

	return 0;
}
BT_MESH_SETTINGS_DEFINE(comp, "cmp", comp_set);

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

	for (i = 0, count = 0; i < mod->keys_cnt; i++) {
		if (mod->keys[i] != BT_MESH_KEY_UNUSED) {
			keys[count++] = mod->keys[i];
			LOG_DBG("model key 0x%04x", mod->keys[i]);
		}
	}

	encode_mod_path(mod, vnd, "bind", path, sizeof(path));

	if (count) {
		err = settings_save_one(path, keys, count * sizeof(keys[0]));
	} else {
		err = settings_delete(path);
	}

	if (err) {
		LOG_ERR("Failed to store %s value", path);
	} else {
		LOG_DBG("Stored %s value", path);
	}
}

static void store_pending_mod_sub(struct bt_mesh_model *mod, bool vnd)
{
	uint16_t groups[CONFIG_BT_MESH_MODEL_GROUP_COUNT];
	char path[20];
	int i, count, err;

	for (i = 0, count = 0; i < mod->groups_cnt; i++) {
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
		LOG_ERR("Failed to store %s value", path);
	} else {
		LOG_DBG("Stored %s value", path);
	}
}

static void store_pending_mod_pub(struct bt_mesh_model *mod, bool vnd)
{
	struct mod_pub_val pub = {0};
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
		LOG_ERR("Failed to store %s value", path);
	} else {
		LOG_DBG("Stored %s value", path);
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

	if (mod->flags & BT_MESH_MOD_DATA_PENDING) {
		mod->flags &= ~BT_MESH_MOD_DATA_PENDING;
		mod->cb->pending_store(mod);
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

int bt_mesh_comp_store(void)
{
	NET_BUF_SIMPLE_DEFINE(buf, BT_MESH_TX_SDU_MAX);
	int err;

	err = bt_mesh_comp_data_get_page_0(&buf, 0);
	if (err) {
		LOG_ERR("Failed to read composition data: %d", err);
		return err;
	}

	err = settings_save_one("bt/mesh/cmp", buf.data, buf.len);
	if (err) {
		LOG_ERR("Failed to store composition data: %d", err);
	} else {
		LOG_DBG("Stored composition data");
	}

	return err;
}

int bt_mesh_comp_change_prepare(void)
{
	if (!IS_ENABLED(CONFIG_BT_SETTINGS)) {
		return -ENOTSUP;
	}

	return bt_mesh_comp_store();
}

void bt_mesh_comp_clear(void)
{
	int err;

	err = settings_delete("bt/mesh/cmp");
	if (err) {
		LOG_ERR("Failed to clear composition data: %d", err);
	} else {
		LOG_DBG("Cleared composition data page 128");
	}

	atomic_clear_bit(bt_mesh.flags, BT_MESH_COMP_DIRTY);
}

static int read_comp_cb(const char *key, size_t len, settings_read_cb read_cb,
			void *cb_arg, void *param)
{
	struct net_buf_simple *buf = param;

	if (len > net_buf_simple_tailroom(buf)) {
		return -ENOBUFS;
	}

	len = read_cb(cb_arg, net_buf_simple_tail(buf), len);
	if (len > 0) {
		net_buf_simple_add(buf, len);
	}

	return -EALREADY;
}

int bt_mesh_comp_read(struct net_buf_simple *buf)
{
	size_t original_len = buf->len;
	int err;

	if (!IS_ENABLED(CONFIG_BT_SETTINGS)) {
		return -ENOTSUP;
	}

	err = settings_load_subtree_direct("bt/mesh/cmp", read_comp_cb, buf);
	if (err) {
		LOG_ERR("Failed reading composition data: %d", err);
		return err;
	}

	if (buf->len == original_len) {
		return -ENOENT;
	}

	return 0;
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
		strncat(path, name, SETTINGS_MAX_DIR_DEPTH);
	}

	if (data_len) {
		err = settings_save_one(path, data, data_len);
	} else {
		err = settings_delete(path);
	}

	if (err) {
		LOG_ERR("Failed to store %s value", path);
	} else {
		LOG_DBG("Stored %s value", path);
	}
	return err;
}

#if defined(CONFIG_BT_MESH_LARGE_COMP_DATA_SRV)
static int metadata_set(const char *name, size_t len_rd, settings_read_cb read_cb, void *cb_arg)
{
	/* Only need to know that the entry exists. Will load the contents on
	 * demand.
	 */
	if (len_rd > 0) {
		atomic_set_bit(bt_mesh.flags, BT_MESH_METADATA_DIRTY);
	}

	return 0;
}
BT_MESH_SETTINGS_DEFINE(metadata, "metadata", metadata_set);

int bt_mesh_models_metadata_store(void)
{
	NET_BUF_SIMPLE_DEFINE(buf, CONFIG_BT_MESH_MODELS_METADATA_PAGE_LEN);
	size_t total_size;
	int err;

	total_size = bt_mesh_metadata_page_0_size();
	LOG_DBG("bt/mesh/metadata total %d", total_size);

	net_buf_simple_init(&buf, 0);
	net_buf_simple_add_le16(&buf, total_size);

	err = bt_mesh_metadata_get_page_0(&buf, 0);
	if (err == -E2BIG) {
		LOG_ERR("Metadata too large");
		return err;
	}
	if (err) {
		LOG_ERR("Failed to read models metadata: %d", err);
		return err;
	}

	LOG_DBG("bt/mesh/metadata len %d", buf.len);

	err = settings_save_one("bt/mesh/metadata", buf.data, buf.len);
	if (err) {
		LOG_ERR("Failed to store models metadata: %d", err);
	} else {
		LOG_DBG("Stored models metadata");
	}

	return err;
}

int bt_mesh_models_metadata_read(struct net_buf_simple *buf, size_t offset)
{
	NET_BUF_SIMPLE_DEFINE(stored_buf, CONFIG_BT_MESH_MODELS_METADATA_PAGE_LEN);
	size_t original_len = buf->len;
	int err;

	if (!IS_ENABLED(CONFIG_BT_SETTINGS)) {
		return -ENOTSUP;
	}

	net_buf_simple_init(&stored_buf, 0);

	err = settings_load_subtree_direct("bt/mesh/metadata", read_comp_cb, &stored_buf);
	if (err) {
		LOG_ERR("Failed reading models metadata: %d", err);
		return err;
	}

	/* First two bytes are total length */
	offset += 2;

	net_buf_simple_add_mem(buf, &stored_buf.data, MIN(net_buf_simple_tailroom(buf), 2));

	if (offset >= stored_buf.len) {
		return 0;
	}

	net_buf_simple_add_mem(buf, &stored_buf.data[offset],
			       MIN(net_buf_simple_tailroom(buf), stored_buf.len - offset));

	LOG_DBG("metadata read %d", buf->len);

	if (buf->len == original_len) {
		return -ENOENT;
	}

	return 0;
}
#endif

void bt_mesh_models_metadata_clear(void)
{
	int err;

	err = settings_delete("bt/mesh/metadata");
	if (err) {
		LOG_ERR("Failed to clear models metadata: %d", err);
	} else {
		LOG_DBG("Cleared models metadata");
	}

	atomic_clear_bit(bt_mesh.flags, BT_MESH_METADATA_DIRTY);
}

int bt_mesh_models_metadata_change_prepare(void)
{
#if !IS_ENABLED(CONFIG_BT_MESH_LARGE_COMP_DATA_SRV)
	return -ENOTSUP;
#endif
	return bt_mesh_models_metadata_store();
}

static void commit_mod(struct bt_mesh_model *mod, struct bt_mesh_elem *elem,
		       bool vnd, bool primary, void *user_data)
{
	if (mod->pub && mod->pub->update &&
	    mod->pub->addr != BT_MESH_ADDR_UNASSIGNED) {
		int32_t ms = bt_mesh_model_pub_period_get(mod);

		if (ms > 0) {
			LOG_DBG("Starting publish timer (period %u ms)", ms);
			k_work_schedule(&mod->pub->timer, K_MSEC(ms));
		}
	}

	if (!IS_ENABLED(CONFIG_BT_MESH_LOW_POWER)) {
		return;
	}

	for (int i = 0; i < mod->groups_cnt; i++) {
		if (mod->groups[i] != BT_MESH_ADDR_UNASSIGNED) {
			bt_mesh_lpn_group_add(mod->groups[i]);
		}
	}
}

void bt_mesh_model_settings_commit(void)
{
	bt_mesh_model_foreach(commit_mod, NULL);
}

void bt_mesh_model_data_store_schedule(struct bt_mesh_model *mod)
{
	mod->flags |= BT_MESH_MOD_DATA_PENDING;
	bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_MOD_PENDING);
}
