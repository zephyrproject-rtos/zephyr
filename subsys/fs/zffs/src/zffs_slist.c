/*
 * Copyright (c) 2018 Findlay Feng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zffs/queue.h"
#include "zffs/object.h"

struct slist_node_disk_head {
	u8_t object_type;
	u8_t next[sizeof(u32_t)];
};

struct slist_node_disk_tail {
	u8_t crc[2];
};

static int slist_node_new(struct zffs_data *zffs,
			  struct zffs_area_pointer *pointer,
			  struct zffs_slist_node *node, const void *ex_data,
			  u32_t ex_len)
{
	int rc;
	struct slist_node_disk_head head;
	struct slist_node_disk_tail tail;
	u16_t crc;

	rc = zffs_object_new(zffs, pointer, node->id,
			      ex_len + sizeof(head) + sizeof(tail));
	if (rc) {
		return rc;
	}

	head.object_type = ZFFS_OBJECT_TYPE_SLIST_NODE;
	sys_put_le32(node->next, head.next);
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

static int slist_node_update(struct zffs_data *zffs,
			     struct zffs_area_pointer *to, u32_t id,
			     u32_t *next, const void *ex_data,
			     u32_t update_ex_len)
{
	int rc;
	struct zffs_area_pointer from = *to;
	struct slist_node_disk_head head;
	struct slist_node_disk_tail tail;
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

	rc =
	    zffs_object_update(zffs, to, id, max(ex_len, update_ex_len) +
						   sizeof(head) + sizeof(tail));
	if (rc) {
		return rc;
	}

	rc = zffs_area_read(zffs, &from, &head, sizeof(head));
	if (rc) {
		return rc;
	}

	if (head.object_type != ZFFS_OBJECT_TYPE_SLIST_NODE) {
		return -EIO;
	}

	if (next) {
		sys_put_le32(*next, head.next);
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
		rc = zffs_area_copy_crc(zffs, &from, to, ex_len, &crc);
		if (rc) {
			return rc;
		}
	}

	sys_put_le16(crc, tail.crc);
	return zffs_area_write(zffs, to, (const void *)&tail, sizeof(tail));
}

static int slist_node_load(struct zffs_data *zffs,
			   struct zffs_area_pointer *pointer,
			   struct zffs_slist_node *node)
{
	int rc;
	struct slist_node_disk_head head;
	u32_t ex_len;

	rc = zffs_object_open(zffs, pointer, node->id, NULL);
	if (rc < 0) {
		return rc;
	}

	if (rc < sizeof(head) + sizeof(struct slist_node_disk_tail)) {
		return -EIO;
	}

	ex_len = rc - sizeof(head) - sizeof(struct slist_node_disk_tail);
	rc = zffs_area_read(zffs, pointer, &head, sizeof(head));

	if (rc) {
		return rc;
	}
	if (head.object_type != ZFFS_OBJECT_TYPE_SLIST_NODE) {
		return -EIO;
	}

	node->next = sys_get_le32(head.next);
	return ex_len;
}

int zffs_slist_open_ex(struct zffs_data *zffs,
			struct zffs_area_pointer *pointer, u32_t id)
{
	struct zffs_slist_node node = {.id = id};

	return slist_node_load(zffs, pointer, &node);
}

int zffs_slist_prepend(struct zffs_data *zffs,
			struct zffs_area_pointer *pointer,
			struct zffs_slist *slist,
			struct zffs_slist_node *node, const void *ex_data,
			u32_t ex_len)
{
	node->next = slist->head;

	slist->head = node->id;
	slist->wait_update = true;

	return slist_node_new(zffs, pointer, node, ex_data, ex_len);
}

int zffs_slist_head(struct zffs_data *zffs,
		     struct zffs_area_pointer *pointer,
		     const struct zffs_slist *slist,
		     struct zffs_slist_node *node)
{
	if (zffs_slist_is_empty(slist)) {
		return -ECHILD;
	}

	node->id = slist->head;

	return slist_node_load(zffs, pointer, node);
}

int zffs_slist_next(struct zffs_data *zffs,
		     struct zffs_area_pointer *pointer,
		     struct zffs_slist_node *node)
{
	if (zffs_slist_is_tail(node)) {
		return -ECHILD;
	}

	node->id = node->next;

	return slist_node_load(zffs, pointer, node);
}

int zffs_slist_search(struct zffs_data *zffs,
		       struct zffs_area_pointer pointer,
		       const struct zffs_slist *slist,
		       struct zffs_slist_node *node, void *node_data,
		       const void *data, zffs_node_compar_fn_t compar_fn)
{
	int rc;
	for (rc = zffs_slist_head(zffs, &pointer, slist, node); rc >= 0;
	     rc = zffs_slist_next(zffs, &pointer, node)) {
		if (compar_fn(zffs, &pointer, node, rc, node_data, data) ==
		    0) {
			return 0;
		}
	}

	return rc;
}

int zffs_slist_remove(struct zffs_data *zffs,
		       struct zffs_area_pointer *pointer,
		       struct zffs_slist *slist, struct zffs_slist_node *node)
{
	struct zffs_slist_node prev;
	struct zffs_area_pointer rp;
	int rc;

	if (slist->head == node->id) {
		slist->head = node->next;
		node->next = ZFFS_NULL;
		slist->wait_update = true;
		return 0;
	}

	rp = *pointer;

	for (rc = zffs_slist_head(zffs, &rp, slist, &prev); rc >= 0;
	     rc = zffs_slist_next(zffs, &rp, &prev)) {
		if (prev.next == node->id) {
			return slist_node_update(zffs, pointer, prev.id,
						 &node->next, NULL, 0);
		}
	}

	return rc;
}

int zffs_slist_updata_ex(struct zffs_data *zffs,
			  struct zffs_area_pointer *pointer, u32_t id,
			  const void *ex_data, u32_t ex_len)
{
	return slist_node_update(zffs, pointer, id, NULL, ex_data, ex_len);
}
