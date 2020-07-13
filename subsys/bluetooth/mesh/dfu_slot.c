/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/settings/settings.h>
#include "dfu_slot.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <zephyr/sys/util.h>
#include <common/bt_str.h>

#define LOG_LEVEL CONFIG_BT_MESH_DFU_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_dfu_slot);

#define SLOT_ENTRY_BUFLEN 25

#define DFU_SLOT_SETTINGS_PATH "bt/mesh-dfu/slot"

#define HEADER_SIZE offsetof(struct bt_mesh_dfu_slot, fwid)

#define PROP_HEADER "h"
#define PROP_FWID "id"
#define PROP_METADATA "m"
#define PROP_URI "u"
#define VALID_SLOTS_TAG "v"

#define SLOT_IN_ARRAY(_slot) PART_OF_ARRAY(slots, CONTAINER_OF(_slot, struct slot, slot))

static ATOMIC_DEFINE(valid_slots, CONFIG_BT_MESH_DFU_SLOT_CNT);
static sys_slist_t list;
static struct slot {
	sys_snode_t n;
	struct bt_mesh_dfu_slot slot;
} slots[CONFIG_BT_MESH_DFU_SLOT_CNT];

static char *slot_entry_encode(uint16_t idx, char buf[SLOT_ENTRY_BUFLEN],
			       const char *property)
{
	snprintf(buf, SLOT_ENTRY_BUFLEN, DFU_SLOT_SETTINGS_PATH "/%x/%s", idx,
		 property);

	return buf;
}

static inline bool slot_in_use(const struct bt_mesh_dfu_slot *slot)
{
	return slot->size > 0U;
}

static inline uint16_t slot_idx(const struct bt_mesh_dfu_slot *slot)
{
	return CONTAINER_OF(slot, struct slot, slot) - &slots[0];
}

static inline void slot_invalidate(struct slot *slot_to_invalidate)
{
	slot_to_invalidate->slot.size = 0U;
	atomic_clear_bit(valid_slots, slot_to_invalidate - &slots[0]);
}

static bool slot_eq(const struct bt_mesh_dfu_slot *slot,
		    const uint8_t *fwid, size_t fwid_len)
{
	return (slot->fwid_len == fwid_len) &&
	       !memcmp(fwid, slot->fwid, fwid_len);
}

static int slot_store(const struct slot *slot_to_store)
{
	uint16_t idx = slot_to_store - &slots[0];
	char buf[SLOT_ENTRY_BUFLEN];
	int err;

	err = settings_save_one(slot_entry_encode(idx, buf, PROP_HEADER),
				slot_to_store, HEADER_SIZE);
	if (err) {
		return err;
	}

	err = settings_save_one(slot_entry_encode(idx, buf, PROP_FWID),
				slot_to_store->slot.fwid, slot_to_store->slot.fwid_len);
	if (err) {
		return err;
	}

	err = settings_save_one(slot_entry_encode(idx, buf,
						  PROP_METADATA),
				slot_to_store->slot.metadata, slot_to_store->slot.metadata_len);
	if (err) {
		return err;
	}

	return settings_save_one(slot_entry_encode(idx, buf, PROP_URI),
				 slot_to_store->slot.uri, slot_to_store->slot.uri_len);
}

static void slot_erase(struct slot *slot_to_erase)
{
	uint16_t idx = slot_to_erase - &slots[0];
	char buf[SLOT_ENTRY_BUFLEN];

	settings_delete(slot_entry_encode(idx, buf, PROP_HEADER));
	settings_delete(slot_entry_encode(idx, buf, PROP_FWID));
	settings_delete(slot_entry_encode(idx, buf, PROP_METADATA));
	settings_delete(slot_entry_encode(idx, buf, PROP_URI));
}

static int valid_slots_store(void)
{
	return settings_save_one(DFU_SLOT_SETTINGS_PATH "/" VALID_SLOTS_TAG,
				 valid_slots, sizeof(valid_slots));
}

const struct bt_mesh_dfu_slot *
bt_mesh_dfu_slot_add(size_t size, const uint8_t *fwid,
		     size_t fwid_len, const uint8_t *metadata,
		     size_t metadata_len, const char *uri, size_t uri_len)
{
	struct slot *slot = NULL;
	int err, i;

	if (size == 0 || fwid_len > CONFIG_BT_MESH_DFU_FWID_MAXLEN ||
	    metadata_len > CONFIG_BT_MESH_DFU_METADATA_MAXLEN ||
	    uri_len > CONFIG_BT_MESH_DFU_URI_MAXLEN) {
		LOG_WRN("Param too large: (size: %d, fwid: %d, metadata: %d, uri: %d)",
			size, fwid_len, metadata_len, uri_len);
		return NULL;
	}

	for (i = 0; i < ARRAY_SIZE(slots); ++i) {
		if (!slot_in_use(&slots[i].slot)) {
			slot = &slots[i];
			continue;
		}

		if (slot_eq(&slots[i].slot, fwid, fwid_len)) {
			return &slots[i].slot;
		}
	}

	if (!slot) {
		LOG_WRN("No space");
		return NULL;
	}

	slot->slot.fwid_len = fwid_len;
	slot->slot.metadata_len = metadata_len;
	slot->slot.uri_len = uri_len;
	memcpy(slot->slot.fwid, fwid, fwid_len);
	memcpy(slot->slot.metadata, metadata, metadata_len);
	memcpy(slot->slot.uri, uri, uri_len);
	slot->slot.size = size;

	err = slot_store(slot);
	if (err) {
		slot_invalidate(slot);
		LOG_WRN("Store failed (err: %d)", err);
		return NULL;
	}

	sys_slist_append(&list, &slot->n);

	LOG_DBG("Added slot #%u: %s", slot - &slots[0],
		bt_hex(slot->slot.fwid, slot->slot.fwid_len));
	return &slot->slot;
}

int bt_mesh_dfu_slot_valid_set(const struct bt_mesh_dfu_slot *slot, bool valid)
{
	uint16_t idx;
	bool prev;
	int err;

	if (!SLOT_IN_ARRAY(slot) || !slot_in_use(slot)) {
		return -ENOENT;
	}

	idx = slot_idx(slot);

	LOG_DBG("%u: %u", idx, valid);

	if (valid) {
		prev = atomic_test_and_set_bit(valid_slots, idx);
	} else {
		prev = atomic_test_and_clear_bit(valid_slots, idx);
	}

	if (valid == prev) {
		return 0;
	}

	err = valid_slots_store();
	if (err) {
		LOG_WRN("Storage failed. err: %d", err);
		atomic_set_bit_to(valid_slots, idx, prev);
	}

	return err;
}

bool bt_mesh_dfu_slot_is_valid(const struct bt_mesh_dfu_slot *slot)
{
	uint16_t idx;

	if (!SLOT_IN_ARRAY(slot) || !slot_in_use(slot)) {
		return false;
	}

	idx = slot_idx(slot);
	return atomic_test_bit(valid_slots, idx);
}

int bt_mesh_dfu_slot_del(const struct bt_mesh_dfu_slot *slot)
{
	struct slot *s = CONTAINER_OF(slot, struct slot, slot);

	if (!SLOT_IN_ARRAY(slot) || !slot_in_use(slot)) {
		return -ENOENT;
	}

	LOG_DBG("%u", s - &slots[0]);

	slot_erase(s);
	slot_invalidate(s);
	sys_slist_find_and_remove(&list, &s->n);

	return 0;
}

int bt_mesh_dfu_slot_del_all(void)
{
	struct slot *s;

	SYS_SLIST_FOR_EACH_CONTAINER(&list, s, n) {
		slot_erase(s);
		slot_invalidate(s);
	}

	sys_slist_init(&list);

	return 0;
}

const struct bt_mesh_dfu_slot *bt_mesh_dfu_slot_at(uint16_t idx)
{
	struct slot *s;

	SYS_SLIST_FOR_EACH_CONTAINER(&list, s, n) {
		if (!idx--) {
			return &s->slot;
		}
	}

	return NULL;
}

int bt_mesh_dfu_slot_get(const uint8_t *fwid, size_t fwid_len,
			 const struct bt_mesh_dfu_slot **slot)
{
	struct slot *s;
	int idx = 0;

	SYS_SLIST_FOR_EACH_CONTAINER(&list, s, n) {
		if (slot_eq(&s->slot, fwid, fwid_len)) {
			*slot = &s->slot;
			return idx;
		}

		idx++;
	}

	return -ENOENT;
}

int bt_mesh_dfu_slot_idx_get(const struct bt_mesh_dfu_slot *slot)
{
	struct slot *s;
	int idx = 0;

	SYS_SLIST_FOR_EACH_CONTAINER(&list, s, n) {
		if (&s->slot == slot) {
			return idx;
		}

		idx++;
	}

	return -ENOENT;
}

size_t bt_mesh_dfu_slot_foreach(bt_mesh_dfu_slot_cb_t cb, void *user_data)
{
	enum bt_mesh_dfu_iter iter;
	struct slot *s;
	size_t cnt = 0;

	SYS_SLIST_FOR_EACH_CONTAINER(&list, s, n) {
		cnt++;
		if (!cb) {
			continue;
		}

		iter = cb(&s->slot, user_data);
		if (iter != BT_MESH_DFU_ITER_CONTINUE) {
			break;
		}
	}

	return cnt;
}

static int slot_data_load(const char *key, size_t len_rd,
			  settings_read_cb read_cb, void *cb_arg)
{
	const char *prop;
	size_t len;
	uint16_t idx;

	if (!strncmp(key, VALID_SLOTS_TAG, 1)) {
		if (read_cb(cb_arg, valid_slots,
			    MIN(sizeof(valid_slots), len_rd)) < 0) {
			return -EINVAL;
		}

		return 0;
	}

	idx = strtol(key, NULL, 16);

	if (idx >= ARRAY_SIZE(slots)) {
		return 0;
	}

	len = settings_name_next(key, &prop);

	if (!strncmp(prop, PROP_HEADER, len)) {
		if (read_cb(cb_arg, &slots[idx], HEADER_SIZE) > 0) {
			sys_slist_append(&list, &slots[idx].n);
		}

		return 0;
	}

	if (!strncmp(prop, PROP_FWID, len)) {
		if (read_cb(cb_arg, &slots[idx].slot.fwid,
			    sizeof(slots[idx].slot.fwid)) < 0) {
			slot_invalidate(&slots[idx]);
			return 0;
		}

		slots[idx].slot.fwid_len = len_rd;
		return 0;
	}

	if (!strncmp(prop, PROP_METADATA, len)) {
		if (read_cb(cb_arg, &slots[idx].slot.metadata,
			    sizeof(slots[idx].slot.metadata)) < 0) {
			slot_invalidate(&slots[idx]);
			return 0;
		}

		slots[idx].slot.metadata_len = len_rd;
		return 0;
	}

	if (!strncmp(prop, PROP_URI, len)) {
		if (read_cb(cb_arg, &slots[idx].slot.uri,
			    sizeof(slots[idx].slot.uri)) < 0) {
			slot_invalidate(&slots[idx]);
			return 0;
		}

		slots[idx].slot.uri_len = len_rd;
	}

	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(bt_mesh_dfu_slots, DFU_SLOT_SETTINGS_PATH, NULL,
			       slot_data_load, NULL, NULL);
