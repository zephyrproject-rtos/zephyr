/*
 * Copyright (c) 2018 Findlay Feng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zffs/queue.h"
#include "zffs/object.h"

struct dlist_node_disk_head {
	u8_t object_type;
	u8_t prev[sizeof(u32_t)];
	u8_t next[sizeof(u32_t)];
};

struct dlist_node_disk_tail {
	u8_t crc[2];
};

static int dlist_node_new(struct zffs_data *zffs,
			  struct zffs_area_pointer *pointer,
			  struct zffs_dlist_node *node, const void *ex_data,
			  u32_t ex_len)
{
	int rc;
	struct dlist_node_disk_head head;
	struct dlist_node_disk_tail tail;
	u16_t crc;

	rc = zffs_object_new(zffs, pointer, node->id,
			     ex_len + sizeof(head) + sizeof(tail));
	if (rc) {
		return rc;
	}

	head.object_type = ZFFS_OBJECT_TYPE_DLIST_NODE;
	sys_put_le32(node->next, head.next);
	sys_put_le32(node->prev, head.prev);
	crc = crc16_ccitt(0, (const void *)&head, sizeof(head));
	rc = zffs_area_write(zffs, pointer, &head, sizeof(head));
	if (rc) {
		return rc;
	}

	if (ex_len) {
		crc = crc16_ccitt(crc, ex_data, ex_len);
		rc = zffs_area_write(zffs, pointer, ex_data, ex_len);
		if (rc) {
			return rc;
		}
	}

	sys_put_le16(crc, tail.crc);
	return zffs_area_write(zffs, pointer, (const void *)&tail,
			       sizeof(tail));
}

static int dlist_node_update(struct zffs_data *zffs,
			     struct zffs_area_pointer *to, u32_t id,
			     u32_t *prev, u32_t *next, const void *ex_data,
			     u32_t update_ex_len)
{
	int rc;
	struct zffs_area_pointer from = *to;
	struct dlist_node_disk_head head;
	struct dlist_node_disk_tail tail;
	u16_t crc;
	u32_t ex_len;

	rc = zffs_object_open(zffs, &from, id, NULL);
	if (rc < 0) {
		return rc;
	}

	if (rc < sizeof(head) + sizeof(tail)) {
		return -EIO;
	}

	ex_len = rc - sizeof(head) - sizeof(tail);
	if (!ex_data) {
		update_ex_len = 0;
	}

	rc = zffs_object_update(zffs, to, id, max(ex_len, update_ex_len) +
				sizeof(head) + sizeof(tail));
	if (rc) {
		return rc;
	}

	rc = zffs_area_read(zffs, &from, &head, sizeof(head));
	if (rc) {
		return rc;
	}

	if (head.object_type != ZFFS_OBJECT_TYPE_DLIST_NODE) {
		return -EIO;
	}

	if (next) {
		sys_put_le32(*next, head.next);
	}

	if (prev) {
		sys_put_le32(*prev, head.prev);
	}

	crc = crc16_ccitt(0, (const void *)&head, sizeof(head));
	rc = zffs_area_write(zffs, to, &head, sizeof(head));
	if (rc) {
		return rc;
	}

	if (ex_data) {
		rc = zffs_area_write(zffs, to, ex_data, update_ex_len);
		if (rc) {
			return rc;
		}
		crc = crc16_ccitt(crc, ex_data, update_ex_len);

		from.offset += update_ex_len;
	}

	if (update_ex_len < ex_len) {
		rc = zffs_area_copy_crc(zffs, &from, to,
					ex_len - update_ex_len, &crc);
		if (rc) {
			return rc;
		}
	}

	sys_put_le16(crc, tail.crc);
	return zffs_area_write(zffs, to, (const void *)&tail, sizeof(tail));
}

static int dlist_node_load(struct zffs_data *zffs,
			   struct zffs_area_pointer *pointer,
			   struct zffs_dlist_node *node)
{
	int rc;
	u32_t ex_len;
	struct dlist_node_disk_head head;

	rc = zffs_object_open(zffs, pointer, node->id, NULL);
	if (rc < 0) {
		return rc;
	}

	if (rc < sizeof(head) + sizeof(struct dlist_node_disk_tail)) {
		return -EIO;
	}

	ex_len = rc - sizeof(head) + sizeof(struct dlist_node_disk_tail);

	rc = zffs_area_read(zffs, pointer, &head, sizeof(head));
	if (rc) {
		return rc;
	}

	if (head.object_type != ZFFS_OBJECT_TYPE_DLIST_NODE) {
		return -EIO;
	}

	node->prev = sys_get_le32(head.prev);
	node->next = sys_get_le32(head.next);
	return ex_len;
}

int zffs_dlist_append(struct zffs_data *zffs,
		      struct zffs_area_pointer *pointer,
		      struct zffs_dlist *dlist, struct zffs_dlist_node *node,
		      const void *ex_data, u32_t ex_len)
{
	node->prev = dlist->tail;
	node->next = ZFFS_NULL;

	if (zffs_dlist_is_empty(dlist)) {
		dlist->head = node->id;
	} else {
		int rc;
		rc = dlist_node_update(zffs, pointer, node->prev, NULL,
				       &node->id, NULL, 0);
		if (rc) {
			return rc;
		}
	}

	dlist->tail = node->id;
	dlist->wait_update = true;

	return dlist_node_new(zffs, pointer, node, ex_data, ex_len);
}

int zffs_dlist_head(struct zffs_data *zffs,
		    struct zffs_area_pointer *pointer,
		    struct zffs_dlist *dlist, struct zffs_dlist_node *node)
{
	if (zffs_dlist_is_empty(dlist)) {
		return -ECHILD;
	}

	node->id = dlist->head;

	return dlist_node_load(zffs, pointer, node);
}

int zffs_dlist_tail(struct zffs_data *zffs,
		    struct zffs_area_pointer *pointer,
		    struct zffs_dlist *dlist, struct zffs_dlist_node *node)
{
	if (zffs_dlist_is_empty(dlist)) {
		return -ECHILD;
	}

	node->id = dlist->tail;

	return dlist_node_load(zffs, pointer, node);
}

int zffs_dlist_next(struct zffs_data *zffs,
		    struct zffs_area_pointer *pointer,
		    struct zffs_dlist_node *node)
{
	if (zffs_dlist_is_tail(node)) {
		return -ECHILD;
	}

	node->id = node->next;

	return dlist_node_load(zffs, pointer, node);
}

int zffs_dlist_prev(struct zffs_data *zffs,
		    struct zffs_area_pointer *pointer,
		    struct zffs_dlist_node *node)
{
	if (zffs_dlist_is_head(node)) {
		return -ECHILD;
	}

	node->id = node->prev;

	return dlist_node_load(zffs, pointer, node);
}

int zffs_dlist_updata_ex(struct zffs_data *zffs,
			 struct zffs_area_pointer *pointer, u32_t id,
			 const void *ex_data, u32_t ex_len)
{
	return dlist_node_update(zffs, pointer, id, NULL, NULL, ex_data,
				 ex_len);
}
