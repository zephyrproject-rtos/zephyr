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

#include <logging/log.h>

LOG_MODULE_DECLARE(bt_ots, CONFIG_BT_OTS_LOG_LEVEL);

static struct bt_ots_dir_list dir_lists[CONFIG_BT_OTS_MAX_INST_CNT];

static void dir_list_object_encode(struct bt_gatt_ots_object *obj,
				   struct net_buf_simple *net_buf)
{
	uint8_t flags = 0;
	uint8_t *start;

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
	net_buf_simple_add_u8(net_buf, strlen(obj->metadata.name));

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

	/* Update the record length at the beginning */
	sys_put_le16(net_buf_simple_tail(net_buf) - start, start);
}

void bt_ots_dir_list_obj_add(struct bt_ots *ots,
			     struct bt_gatt_ots_object *obj)
{
	__ASSERT(ots->dir_list->dir_list_obj != obj,
		 "Cannot add Directory Listing Object");

	if (ots->dir_list->dir_list_obj != ots->cur_obj) {
		/* We only need to update the object directory listing if it is currently selected,
		 * as we otherwise only create it when it is selected.
		 */
		return;
	}

	dir_list_object_encode(obj, &ots->dir_list->net_buf);

	/*re-encode the Directory Listing Object size with the is new size*/
	sys_put_le16(ots->dir_list->net_buf.len, ots->dir_list->net_buf.data);

	/* Update Directory Listing Object metadata size */
	ots->dir_list->dir_list_obj->metadata.size.cur = ots->dir_list->net_buf.len;
}

void bt_ots_dir_list_obj_remove(struct bt_ots *ots,
				struct bt_gatt_ots_object *obj)
{
	uint16_t offset = 0;

	__ASSERT(ots->dir_list->dir_list_obj != obj,
		 "Cannot remove Directory Listing Object");

	if (ots->dir_list->dir_list_obj != ots->cur_obj) {
		/* We only need to update the object directory listing if it is currently selected,
		 * as we otherwise only create it when it is selected.
		 */
		return;
	}

	while (offset < ots->dir_list->net_buf.len) {
		uint16_t len = sys_get_le16(ots->dir_list->net_buf.data + offset);
		uint64_t id = sys_get_le64(ots->dir_list->net_buf.data + offset + sizeof(len));

		__ASSERT(len, "Invalid object length");

		if (id == obj->id) {
			/* Delete object by moving memory after the object to
			 * the objects current location
			 */
			memmove(ots->dir_list->net_buf.data + offset,
				ots->dir_list->net_buf.data + offset + len,
				ots->dir_list->net_buf.len - (offset + len));
			/* Decrement net_buf len to new length */
			ots->dir_list->net_buf.len -= len;
			break;
		}

		offset += len;
	}

	__ASSERT(offset <= ots->dir_list->net_buf.len, "Object was not removed");

	/*re-encode the Directory Listing Object size with the is new size*/
	sys_put_le16(ots->dir_list->net_buf.len, ots->dir_list->net_buf.data);

	/* Update Directory Listing Object metadata size */
	ots->dir_list->dir_list_obj->metadata.size.cur = ots->dir_list->net_buf.len;
}

static void dir_list_encode(struct bt_ots *ots)
{
	struct bt_gatt_ots_object *obj;
	int err;

	err = bt_gatt_ots_obj_manager_first_obj_get(ots->obj_manager, &obj);

	__ASSERT(err == 0 && first_obj == ots->dir_list->dir_list_obj,
		 "Expecting first object to be the Directory Listing Object");

	/* Init with len = 0 to reset data */
	net_buf_simple_init_with_data(&ots->dir_list->net_buf,
				      ots->dir_list->_content,
				      sizeof(ots->dir_list->_content));
	ots->dir_list->net_buf.len = 0;

	do {
		dir_list_object_encode(obj, &ots->dir_list->net_buf);

		err = bt_gatt_ots_obj_manager_next_obj_get(ots->obj_manager, obj, &obj);
	} while (!err);

	/*re-encode the Directory Listing Object size with the is new size*/
	sys_put_le16(ots->dir_list->net_buf.len, ots->dir_list->net_buf.data);

	/* Update Directory Listing Object metadata size */
	ots->dir_list->dir_list_obj->metadata.size.cur = ots->dir_list->net_buf.len;
}

void bt_ots_dir_list_selected(struct bt_ots *ots)
{
	if (ots->dir_list->dir_list_obj != ots->cur_obj) {
		/* We only need to update the object directory listing if it is currently selected,
		 * as we otherwise only create it when it is selected.
		 */
		return;
	}

	dir_list_encode(ots);
}

void bt_ots_dir_list_init(struct bt_ots *ots)
{
	struct bt_gatt_ots_object *dir_list_obj;
	int err;
	static char *dir_list_obj_name = CONFIG_BT_OTS_DIR_LIST_OBJ_NAME;

	if (!ots->dir_list) {
		for (int i = 0; i < ARRAY_SIZE(dir_lists); i++) {
			if (!dir_lists[i].dir_list_obj) {
				ots->dir_list = &dir_lists[i];
			}
		}
	}

	__ASSERT(ots->dir_list,
		 "Could not assign Directory Listing Object to OTS instance %p", ots);

	__ASSERT(strlen(dir_list_obj_name) < UINT8_MAX,
		 "BT_OTS_DIR_LIST_OBJ_NAME shall be less than 255 octets");

	err = bt_gatt_ots_obj_manager_obj_add(ots->obj_manager, &dir_list_obj);

	__ASSERT(!err, "Could not add Directory Listing Object for object manager %p", obj_man);

	memset(&dir_list_obj->metadata, 0, sizeof(dir_list_obj->metadata));
	dir_list_obj->metadata.name = dir_list_obj_name;
	dir_list_obj->metadata.size.alloc = sizeof(ots->dir_list->_content);
	dir_list_obj->metadata.type.uuid.type = BT_UUID_TYPE_16;
	dir_list_obj->metadata.type.uuid_16.val = BT_UUID_OTS_DIRECTORY_LISTING_VAL;
	BT_OTS_OBJ_SET_PROP_READ(dir_list_obj->metadata.props);

	ots->dir_list->dir_list_obj = dir_list_obj;

	/* Set size in dir_list_encode */
	dir_list_encode(ots);
}

int bt_ots_dir_list_content_get(struct bt_ots *ots, uint8_t **data,
				uint32_t len, uint32_t offset)
{
	if (offset >= ots->dir_list->net_buf.len) {
		*data = NULL;
		return 0;
	}

	*data = ots->dir_list->net_buf.data + offset;

	return MIN(len, ots->dir_list->net_buf.len - offset);
}
