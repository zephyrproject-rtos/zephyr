/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/printk.h>

#include "canopen_internal.h"

LOG_MODULE_DECLARE(canopen, CONFIG_CANOPEN_LOG_LEVEL);

#define CANOPEN_OD_STORE 0x1010U
#define CANOPEN_OD_RPDO  0x1400U
#define CANOPEN_OD_TPDO  0x1800U

#define STORAGE_SUB_ALL     1U
#define STORAGE_SUB_COMM    2U
#define STORAGE_SUB_PROFILE 3U
#define STORAGE_SUB_MANUF   4U

#define STORAGE_MANUF_FIRST   0x2000U
#define STORAGE_MANUF_LAST    0x5FFFU
#define STORAGE_PROFILE_FIRST 0x6000U
#define STORAGE_PROFILE_LAST  0x9FFFU

#define STORAGE_SAVE    0x65766173U
#define STORAGE_LOAD    0x64616F6CU
#define STORAGE_KEY_LEN 32

struct storage_obj {
	uint16_t index;
	uint8_t sub;
	uint8_t count;
};

struct storage_load {
	struct canopen_node *node;
	bool app;
};

typedef int (*storage_op_t)(const char *key, const void *value, size_t len);

static const uint8_t storage_highest = STORAGE_SUB_MANUF;
static const uint32_t storage_capability = BIT(0);

static const struct storage_obj storage_comm_objs[] = {
	{0x1005U, 0U, 1U},
	{0x1006U, 0U, 1U},
	{0x1007U, 0U, 1U},
	{CANOPEN_OD_GUARD_TIME, 0U, 1U},
	{CANOPEN_OD_LIFE_FACTOR, 0U, 1U},
	{CANOPEN_OD_TIME, 0U, 1U},
	{0x1014U, 0U, 1U},
	{0x1015U, 0U, 1U},
	{CANOPEN_OD_HB_CONSUMER, 1U, CONFIG_CANOPEN_HB_CONSUMER_COUNT},
	{0x1017U, 0U, 1U},
	{0x1019U, 0U, 1U},
	{0x1029U, 1U, 1U},
};

static int storage_save(const char *key, const void *value, size_t len)
{
	return settings_save_one(key, value, len);
}

static int storage_delete(const char *key, const void *value, size_t len)
{
	ARG_UNUSED(value);
	ARG_UNUSED(len);

	return settings_delete(key);
}

static int storage_pdos(struct canopen_node *node, storage_op_t op, struct canopen_pdo *pdos,
			uint16_t count, uint16_t index)
{
	char key[STORAGE_KEY_LEN];
	int err;

	for (uint16_t n = 0U; n < count; n++) {
		snprintk(key, sizeof(key), "canopen/%u/c/%x", node->cfg->node_id, index + n);
		err = op(key, &pdos[n].param, sizeof(pdos[n].param));
		if (err != 0) {
			return err;
		}
	}

	return 0;
}

static int storage_comm(struct canopen_node *node, storage_op_t op)
{
	char key[STORAGE_KEY_LEN];
	struct canopen_od_ref ref;
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(storage_comm_objs); i++) {
		const struct storage_obj *obj = &storage_comm_objs[i];

		for (uint8_t sub = obj->sub; sub < (obj->sub + obj->count); sub++) {
			uint32_t abort = canopen_od_find(node, obj->index, sub, &ref);

			if (abort != CANOPEN_SDO_ABORT_NONE) {
				continue;
			}

			snprintk(key, sizeof(key), "canopen/%u/c/%x/%x", node->cfg->node_id,
				 obj->index, sub);
			err = op(key, ref.src, ref.size);
			if (err != 0) {
				return err;
			}
		}
	}

	err = storage_pdos(node, op, node->tpdo, CONFIG_CANOPEN_TPDO_COUNT, CANOPEN_OD_TPDO);
	if (err != 0) {
		return err;
	}

	return storage_pdos(node, op, node->rpdo, CONFIG_CANOPEN_RPDO_COUNT, CANOPEN_OD_RPDO);
}

static int storage_app_sub(struct canopen_node *node, storage_op_t op, uint16_t index, uint8_t sub)
{
	char key[STORAGE_KEY_LEN];
	struct canopen_od_ref ref;

	if ((canopen_od_find(node, index, sub, &ref) != CANOPEN_SDO_ABORT_NONE) ||
	    ((ref.attr & CANOPEN_OD_ATTR_PERSIST) == 0U)) {
		return 0;
	}

	snprintk(key, sizeof(key), "canopen/%u/a/%x/%x", node->cfg->node_id, index, sub);

	return op(key, ref.src, ref.size);
}

static int storage_app(struct canopen_node *node, storage_op_t op, uint16_t first, uint16_t last)
{
	const struct canopen_od *od = node->cfg->od;
	int err = 0;

	if (od == NULL) {
		return 0;
	}

	for (size_t i = 0U; (i < od->count) && (err == 0); i++) {
		const struct canopen_od_entry *entry = &od->entries[i];

		if ((entry->index < first) || (entry->index > last)) {
			continue;
		}

		for (uint8_t j = 0U; (j < entry->count) && (err == 0); j++) {
			uint8_t sub;

			if (entry->code == CANOPEN_OD_CODE_ARRAY) {
				sub = j + 1U;
			} else {
				sub = entry->subs[j].sub;
			}

			err = storage_app_sub(node, op, entry->index, sub);
		}
	}

	return err;
}

static const char *storage_hex(const char *s, uint32_t *value)
{
	const char *start = s;
	uint32_t v = 0U;

	while (true) {
		char c = *s;

		if ((c >= '0') && (c <= '9')) {
			v = (v << 4) | (uint32_t)(c - '0');
		} else if ((c >= 'a') && (c <= 'f')) {
			v = (v << 4) | (uint32_t)(c - 'a' + 10);
		} else {
			break;
		}
		s++;
	}

	*value = v;

	return (s == start) ? NULL : s;
}

static void *storage_pdo_param(struct canopen_node *node, uint32_t index, size_t *size)
{
	*size = sizeof(struct canopen_pdo_param);

	if ((index >= CANOPEN_OD_TPDO) && (index < (CANOPEN_OD_TPDO + CONFIG_CANOPEN_TPDO_COUNT))) {
		return &node->tpdo[index - CANOPEN_OD_TPDO].param;
	}

	if ((index >= CANOPEN_OD_RPDO) && (index < (CANOPEN_OD_RPDO + CONFIG_CANOPEN_RPDO_COUNT))) {
		return &node->rpdo[index - CANOPEN_OD_RPDO].param;
	}

	return NULL;
}

static int storage_load_cb(const char *key, size_t len, settings_read_cb read_cb, void *cb_arg,
			   void *param)
{
	const struct storage_load *ctx = param;
	struct canopen_od_ref ref;
	const char *next;
	uint32_t index;
	uint32_t sub;
	void *dst = NULL;
	size_t size = 0U;

	next = storage_hex(key, &index);
	if ((next == NULL) || (index > UINT16_MAX)) {
		return 0;
	}

	if (*next == '\0') {
		if (!ctx->app) {
			dst = storage_pdo_param(ctx->node, index, &size);
		}
	} else if (*next == '/') {
		next = storage_hex(next + 1, &sub);
		if ((next != NULL) && (*next == '\0') && (sub <= UINT8_MAX) &&
		    (canopen_od_find(ctx->node, (uint16_t)index, (uint8_t)sub, &ref) ==
		     CANOPEN_SDO_ABORT_NONE) &&
		    (!ctx->app || ((ref.attr & CANOPEN_OD_ATTR_PERSIST) != 0U))) {
			dst = ref.dst;
			size = ref.size;
		}
	}

	if ((dst != NULL) && (len == size)) {
		(void)read_cb(cb_arg, dst, len);
	}

	return 0;
}

void canopen_storage_load(struct canopen_node *node, bool app)
{
	const struct storage_load ctx = {
		.node = node,
		.app = app,
	};
	char subtree[STORAGE_KEY_LEN];

	snprintk(subtree, sizeof(subtree), "canopen/%u/%c", node->cfg->node_id, app ? 'a' : 'c');
	(void)settings_load_subtree_direct(subtree, storage_load_cb, (void *)&ctx);
}

uint32_t canopen_storage_find(uint16_t index, uint8_t sub, struct canopen_od_ref *ref)
{
	if (sub == 0U) {
		canopen_od_comm_ref(ref, index, sub, CANOPEN_OD_TYPE_UNSIGNED8, CANOPEN_OD_CONST,
				    &storage_highest, NULL, sizeof(storage_highest));
	} else if (sub <= STORAGE_SUB_MANUF) {
		canopen_od_comm_ref(ref, index, sub, CANOPEN_OD_TYPE_UNSIGNED32, CANOPEN_OD_RW,
				    &storage_capability, NULL, sizeof(storage_capability));
	} else {
		return CANOPEN_SDO_ABORT_NO_SUB_INDEX;
	}

	return CANOPEN_SDO_ABORT_NONE;
}

uint32_t canopen_storage_write(struct canopen_node *node, uint16_t index, uint8_t sub,
			       uint32_t value)
{
	bool save = index == CANOPEN_OD_STORE;
	storage_op_t op = save ? storage_save : storage_delete;
	int err = 0;

	if (value != (save ? STORAGE_SAVE : STORAGE_LOAD)) {
		return CANOPEN_SDO_ABORT_DATA_TRANSFER;
	}

	if ((sub == STORAGE_SUB_ALL) || (sub == STORAGE_SUB_COMM)) {
		err = storage_comm(node, op);
	}

	if ((err == 0) && ((sub == STORAGE_SUB_ALL) || (sub == STORAGE_SUB_PROFILE))) {
		err = storage_app(node, op, STORAGE_PROFILE_FIRST, STORAGE_PROFILE_LAST);
	}

	if ((err == 0) && ((sub == STORAGE_SUB_ALL) || (sub == STORAGE_SUB_MANUF))) {
		err = storage_app(node, op, STORAGE_MANUF_FIRST, STORAGE_MANUF_LAST);
	}

	if (err != 0) {
		LOG_ERR("failed to %s parameters (%d)", save ? "store" : "restore", err);
		return CANOPEN_SDO_ABORT_HARDWARE;
	}

	return CANOPEN_SDO_ABORT_NONE;
}
