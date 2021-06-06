/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/dlist.h>
#include <sys/byteorder.h>

#include <bluetooth/services/ots.h>
#include "ots_internal.h"
#include "ots_obj_manager_internal.h"
#include "ots_dir_list_internal.h"

#include <logging/log.h>

LOG_MODULE_DECLARE(bt_ots, CONFIG_BT_OTS_LOG_LEVEL);

static struct bt_ots_dir_list dir_lists[CONFIG_BT_OTS_MAX_INST_CNT];

static void dir_list_object_encode(struct bt_gatt_ots_object *obj,
				   struct net_buf_simple *net_buf)
{
	uint8_t flags = 0;
	uint8_t *start;
	uint16_t len;
	size_t obj_name_len;

	BT_OTS_DIR_LIST_SET_FLAG_PROPERTIES(flags);
	BT_OTS_DIR_LIST_SET_FLAG_CUR_SIZE(flags);
	if (obj->metadata.type.uuid.type == BT_UUID_TYPE_128) {
		BT_OTS_DIR_LIST_SET_FLAG_TYPE_128(flags);
	}

	/* skip 16bits at the beginning of the record for the record's length */
	start = net_buf_simple_add(net_buf, sizeof(uint16_t));

	/* ID */
	net_buf_simple_add_le48(net_buf, obj->id);

	/* Name length */
	obj_name_len = strlen(obj->metadata.name);
	__ASSERT(obj_name_len > 0 && obj_name_len <= CONFIG_BT_OTS_OBJ_MAX_NAME_LEN,
		 "Dir list object len is incorrect %zu", len);
	net_buf_simple_add_u8(net_buf, obj_name_len);

	/* Name */
	net_buf_simple_add_mem(net_buf, obj->metadata.name,
			       strlen(obj->metadata.name));

	/* Flags */
	net_buf_simple_add_u8(net_buf, flags);

	/* encode Object type */
	if (obj->metadata.type.uuid.type == BT_UUID_TYPE_16) {
		net_buf_simple_add_le16(net_buf, obj->metadata.type.uuid_16.val);
	} else {
		net_buf_simple_add_mem(net_buf, obj->metadata.type.uuid_128.val,
				       BT_UUID_SIZE_128);
	}

	/* encode Object Current size */
	net_buf_simple_add_le32(net_buf, obj->metadata.size.cur);

	/* Object properties */
	net_buf_simple_add_le32(net_buf, obj->metadata.props);

	len = net_buf_simple_tail(net_buf) - start;

	__ASSERT(len >= DIR_LIST_OBJ_RECORD_MIN_SIZE,
		 "Dir list object len is too small %u", len);
	__ASSERT(len <= DIR_LIST_OBJ_RECORD_MAX_SIZE,
		 "Dir list object len is too large %u", len);

	/* Update the record length at the beginning */
	sys_put_le16(len, start);
}

void bt_ots_dir_list_obj_add(struct bt_ots_dir_list *dir_list, void *obj_manager,
			     struct bt_gatt_ots_object *cur_obj, struct bt_gatt_ots_object *obj)
{
	__ASSERT(dir_list->dir_list_obj != obj,
		 "Cannot add Directory Listing Object");

	__ASSERT(bt_gatt_ots_obj_manager_obj_contains(obj_manager, obj),
		 "Object not part of OTS instance");

	if (dir_list->dir_list_obj != cur_obj) {
		/* We only need to update the object directory listing if it is currently selected,
		 * as we otherwise only create it when it is selected.
		 */
		return;
	}

	dir_list_object_encode(obj, &dir_list->net_buf);

	/*re-encode the Directory Listing Object size with the is new size*/
	sys_put_le16(dir_list->net_buf.len, dir_list->net_buf.data);

	/* Update Directory Listing Object metadata size */
	dir_list->dir_list_obj->metadata.size.cur = dir_list->net_buf.len;
}

void bt_ots_dir_list_obj_remove(struct bt_ots_dir_list *dir_list, void *obj_manager,
				struct bt_gatt_ots_object *cur_obj, struct bt_gatt_ots_object *obj)
{
	uint16_t offset = 0;

	__ASSERT(dir_list->dir_list_obj != obj,
		 "Cannot remove Directory Listing Object");

	__ASSERT(bt_gatt_ots_obj_manager_obj_contains(obj_manager, obj),
		 "Object not part of OTS instance");

	if (dir_list->dir_list_obj != cur_obj) {
		/* We only need to update the object directory listing if it is currently selected,
		 * as we otherwise only create it when it is selected.
		 */
		return;
	}

	while (offset < dir_list->net_buf.len) {
		uint16_t len;
		uint64_t id;

		__ASSERT((DIR_LIST_OBJ_RECORD_MIN_SIZE + offset) <= dir_list->net_buf.len,
			 "Invalid dir_list buf length %u, expected at least %u",
			 dir_list->net_buf.len, DIR_LIST_OBJ_RECORD_MIN_SIZE + offset);

		len = sys_get_le16(dir_list->net_buf.data + offset);
		id = sys_get_le64(dir_list->net_buf.data + offset + sizeof(len));

		__ASSERT(len, "Invalid object length");
		__ASSERT((len + offset) <= dir_list->net_buf.len,
			 "Invalid dir_list buf length %u, expected at least %u",
			 dir_list->net_buf.len, len + offset);

		if (id == obj->id) {
			/* Delete object by moving memory after the object to
			 * the objects current location
			 */
			memmove(dir_list->net_buf.data + offset,
				dir_list->net_buf.data + offset + len,
				dir_list->net_buf.len - (offset + len));
			/* Decrement net_buf len to new length */
			dir_list->net_buf.len -= len;
			break;
		}

		offset += len;
	}

	__ASSERT(offset <= dir_list->net_buf.len, "Object was not removed");

	/*re-encode the Directory Listing Object size with the is new size*/
	sys_put_le16(dir_list->net_buf.len, dir_list->net_buf.data);

	/* Update Directory Listing Object metadata size */
	dir_list->dir_list_obj->metadata.size.cur = dir_list->net_buf.len;
}

static void dir_list_encode(struct bt_ots_dir_list *dir_list, void *obj_manager)
{
	struct bt_gatt_ots_object *obj;
	int err;

	err = bt_gatt_ots_obj_manager_first_obj_get(obj_manager, &obj);

	__ASSERT(err == 0 && first_obj == dir_list->dir_list_obj,
		 "Expecting first object to be the Directory Listing Object");

	/* Init with len = 0 to reset data */
	net_buf_simple_init_with_data(&dir_list->net_buf, dir_list->_content,
				      sizeof(dir_list->_content));
	dir_list->net_buf.len = 0;

	do {
		dir_list_object_encode(obj, &dir_list->net_buf);

		err = bt_gatt_ots_obj_manager_next_obj_get(obj_manager, obj, &obj);
	} while (!err);

	/*re-encode the Directory Listing Object size with the is new size*/
	sys_put_le16(dir_list->net_buf.len, dir_list->net_buf.data);

	/* Update Directory Listing Object metadata size */
	dir_list->dir_list_obj->metadata.size.cur = dir_list->net_buf.len;
}

void bt_ots_dir_list_selected(struct bt_ots_dir_list *dir_list, void *obj_manager,
			      struct bt_gatt_ots_object *cur_obj)
{
	if (dir_list->dir_list_obj != cur_obj) {
		/* We only need to update the object directory listing if it is currently selected,
		 * as we otherwise only create it when it is selected.
		 */
		return;
	}

	dir_list_encode(dir_list, obj_manager);
}

void bt_ots_dir_list_init(struct bt_ots_dir_list **dir_list, void *obj_manager)
{
	struct bt_gatt_ots_object *dir_list_obj;
	int err;
	static char *dir_list_obj_name = CONFIG_BT_OTS_DIR_LIST_OBJ_NAME;

	__ASSERT(*dir_list, "Already initialized");

	for (int i = 0; i < ARRAY_SIZE(dir_lists); i++) {
		if (!dir_lists[i].dir_list_obj) {
			*dir_list = &dir_lists[i];
		}
	}

	__ASSERT(*dir_list, "Could not assign Directory Listing Object");

	__ASSERT(strlen(dir_list_obj_name) <= CONFIG_BT_OTS_OBJ_MAX_NAME_LEN,
		 "BT_OTS_DIR_LIST_OBJ_NAME shall be less than or equal to %u octets",
		 CONFIG_BT_OTS_OBJ_MAX_NAME_LEN);

	err = bt_gatt_ots_obj_manager_obj_add(obj_manager, &dir_list_obj);

	__ASSERT(!err, "Could not add Directory Listing Object for object manager %p", obj_man);

	memset(&dir_list_obj->metadata, 0, sizeof(dir_list_obj->metadata));
	dir_list_obj->metadata.name = dir_list_obj_name;
	dir_list_obj->metadata.size.alloc = sizeof((*dir_list)->_content);
	dir_list_obj->metadata.type.uuid.type = BT_UUID_TYPE_16;
	dir_list_obj->metadata.type.uuid_16.val = BT_UUID_OTS_DIRECTORY_LISTING_VAL;
	BT_OTS_OBJ_SET_PROP_READ(dir_list_obj->metadata.props);

	(*dir_list)->dir_list_obj = dir_list_obj;

	/* Set size in dir_list_encode */
	dir_list_encode(*dir_list, obj_manager);
}

int bt_ots_dir_list_content_get(struct bt_ots_dir_list *dir_list, uint8_t **data,
				uint32_t len, uint32_t offset)
{
	if (offset >= dir_list->net_buf.len) {
		*data = NULL;
		return 0;
	}

	*data = dir_list->net_buf.data + offset;

	return MIN(len, dir_list->net_buf.len - offset);
}
