/*
 * Copyright (c) 2022-2023, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Arm SiP services service ID manager and ID mapping table force
 * clients and transactions.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sip_svc/sip_svc_proto.h>
#include "sip_svc_id_mgr.h"

/**
 * Create a id key pool using size variable  0..size-1, where we can
 * track the allocated id.
 */
struct sip_svc_id_pool *sip_svc_id_mgr_create(uint32_t size)
{
	struct sip_svc_id_pool *id_pool = NULL;
	uint32_t mask_size = 0;
	uint32_t i = 0;

	if (size == SIP_SVC_ID_INVALID) {
		return NULL;
	}

	/* Allocate memory for ID pool */
	id_pool = k_malloc(sizeof(struct sip_svc_id_pool));
	if (!id_pool) {
		return NULL;
	}
	id_pool->size = size;

	id_pool->id_list = k_malloc(size * sizeof(uint32_t));
	if (!id_pool->id_list) {
		k_free(id_pool);
		return NULL;
	}

	mask_size = size / sizeof(uint32_t);
	if (size % sizeof(uint32_t)) {
		mask_size++;
	}

	id_pool->id_mask = k_malloc(mask_size * sizeof(uint32_t));
	if (!id_pool->id_mask) {
		k_free(id_pool->id_list);
		k_free(id_pool);
		return NULL;
	}

	/* Initialize ID */
	for (i = 0; i < size; i++) {
		id_pool->id_list[i] = i;
	}

	/* ID pool is full during initialization */
	id_pool->head = 0;
	id_pool->tail = size - 1;

	/* Initialize ID in use flags */
	for (i = 0; i < mask_size; i++) {
		id_pool->id_mask[i] = 0;
	}

	return id_pool;
}

/* Delete a created id pool*/
void sip_svc_id_mgr_delete(struct sip_svc_id_pool *id_pool)
{
	if (id_pool) {
		k_free(id_pool->id_mask);
		k_free(id_pool->id_list);
		k_free(id_pool);
	}
}

/* Retrieve an id from the id pool*/
uint32_t sip_svc_id_mgr_alloc(struct sip_svc_id_pool *id_pool)
{
	uint32_t id;
	uint32_t row;
	uint32_t col;

	if (!id_pool) {
		return SIP_SVC_ID_INVALID;
	}

	if (id_pool->head == SIP_SVC_ID_INVALID) {
		return SIP_SVC_ID_INVALID;
	}

	id = id_pool->id_list[id_pool->head];

	/* Calculate the mask bit position in id mask array*/
	row = id / sizeof(uint32_t);
	col = id % sizeof(uint32_t);
	id_pool->id_mask[row] |= (1 << col);

	if (id_pool->head == id_pool->tail) {
		id_pool->head = SIP_SVC_ID_INVALID;
		id_pool->tail = SIP_SVC_ID_INVALID;
	} else {
		id_pool->head++;
		if (id_pool->head == id_pool->size) {
			id_pool->head = 0;
		}
	}

	return id;
}

/* Free the id that was allocated from the id_pool*/
void sip_svc_id_mgr_free(struct sip_svc_id_pool *id_pool, uint32_t id)
{
	uint32_t row;
	uint32_t col;

	if (!id_pool) {
		return;
	}

	if (id >= id_pool->size) {
		return;
	}

	row = id / sizeof(uint32_t);
	col = id % sizeof(uint32_t);

	/* check if the id was allocated earlier*/
	if (!(id_pool->id_mask[row] & (1 << col))) {
		return;
	}

	/* Clear the id mask bit */
	id_pool->id_mask[row] &= ~(1 << col);

	if (id_pool->head == SIP_SVC_ID_INVALID) {
		id_pool->head = 0;
		id_pool->tail = 0;
	} else {
		id_pool->tail++;
		if (id_pool->tail == id_pool->size) {
			id_pool->tail = 0;
		}
		if (id_pool->head == id_pool->tail) {
			return;
		}
	}

	id_pool->id_list[id_pool->tail] = id;
}

/* Allocate database to store values */
struct sip_svc_id_map *sip_svc_id_map_create(uint32_t size)
{
	struct sip_svc_id_map *id_map = NULL;
	uint32_t items_size = (sizeof(struct sip_svc_id_map_item)) * size;
	int i;

	id_map = k_malloc(sizeof(struct sip_svc_id_map));
	if (!id_map) {
		return NULL;
	}

	id_map->items = k_malloc(items_size);
	if (!id_map->items) {
		k_free(id_map);
		return NULL;
	}

	id_map->size = size;
	for (i = 0; i < size; i++) {
		id_map->items[i].id = SIP_SVC_ID_INVALID;
		id_map->items[i].arg1 = NULL;
		id_map->items[i].arg2 = NULL;
		id_map->items[i].arg3 = NULL;
		id_map->items[i].arg4 = NULL;
		id_map->items[i].arg5 = NULL;
		id_map->items[i].arg6 = NULL;
	}

	return id_map;
}


/* Delete a created database */
void sip_svc_id_map_delete(struct sip_svc_id_map *id_map)
{
	if (id_map) {
		k_free(id_map->items);
		k_free(id_map);
	}
}

/* Retrieve index from database with key(id)*/
static int sip_svc_id_map_get_idx(struct sip_svc_id_map *id_map, uint32_t id)
{
	int i;

	if (!id_map) {
		return -EINVAL;
	}

	for (i = 0; i < id_map->size; i++) {
		if (id_map->items[i].id == id) {
			return i;
		}
	}

	return -EINVAL;
}

/* Insert an entry into database with key as id*/
int sip_svc_id_map_insert_item(struct sip_svc_id_map *id_map, uint32_t id, void *arg1, void *arg2,
			       void *arg3, void *arg4, void *arg5, void *arg6)
{
	int i;

	if (!id_map) {
		return -EINVAL;
	}

	i = sip_svc_id_map_get_idx(id_map, SIP_SVC_ID_INVALID);
	if (i < 0) {
		return -EINVAL;
	}

	id_map->items[i].id = id;
	id_map->items[i].arg1 = arg1;
	id_map->items[i].arg2 = arg2;
	id_map->items[i].arg3 = arg3;
	id_map->items[i].arg4 = arg4;
	id_map->items[i].arg5 = arg5;
	id_map->items[i].arg6 = arg6;

	return 0;
}

/* Remove an entry with key id */
int sip_svc_id_map_remove_item(struct sip_svc_id_map *id_map, uint32_t id)
{
	int i;

	if (!id_map) {
		return -EINVAL;
	}

	i = sip_svc_id_map_get_idx(id_map, id);
	if (i < 0) {
		return -EINVAL;
	}

	id_map->items[i].id = SIP_SVC_ID_INVALID;
	id_map->items[i].arg1 = NULL;
	id_map->items[i].arg2 = NULL;
	id_map->items[i].arg3 = NULL;
	id_map->items[i].arg4 = NULL;
	id_map->items[i].arg5 = NULL;
	id_map->items[i].arg6 = NULL;

	return 0;
}

/* Query an entry from database using key id*/
struct sip_svc_id_map_item *sip_svc_id_map_query_item(struct sip_svc_id_map *id_map, uint32_t id)
{
	int i;

	if (!id_map) {
		return NULL;
	}

	i = sip_svc_id_map_get_idx(id_map, id);
	if (i < 0) {
		return NULL;
	}

	return &id_map->items[i];
}
