/*
 * Copyright (c) 2021, Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Arm SiP services driver ID manager and ID mapping table force
 * clients and transactions.
 */

#include <kernel.h>
#include <drivers/sip_svc/sip_svc_ll.h>
#include "sip_svc_ll_id_mgr.h"

struct sip_svc_id_pool *sip_svc_ll_id_mgr_create(uint32_t size)
{
	struct sip_svc_id_pool *id_pool = NULL;
	uint32_t mask_size = 0;
	uint32_t i = 0;


	if (size == SIP_SVC_ID_INVALID) {
		return NULL;
	}

	/* Allow memory for ID pool */
	id_pool = k_malloc(sizeof(struct sip_svc_id_pool));
	if (!id_pool) {
		return NULL;
	}
	id_pool->size = size;

	id_pool->id_list = k_malloc(size * sizeof(uint32_t));
	if (!id_pool) {
		k_free(id_pool);
		return NULL;
	}

	mask_size = size / sizeof(uint32_t);
	if (size % sizeof(uint32_t))
		mask_size++;

	id_pool->id_mask = k_malloc(mask_size * sizeof(uint32_t));
	if (!id_pool->id_mask) {
		k_free(id_pool->id_list);
		k_free(id_pool);
		return NULL;
	}

	/* Initialize ID */
	for (i = 0; i < size; i++)
		id_pool->id_list[i] = i;

	/* ID pool is full during initialization */
	id_pool->head = 0;
	id_pool->tail = size-1;

	/* Initialize ID in use flags */
	for (i = 0; i < mask_size; i++)
		id_pool->id_mask[i] = 0;

	return id_pool;
}

void sip_svc_ll_id_mgr_delete(struct sip_svc_id_pool *id_pool)
{
	if (id_pool) {
		k_free(id_pool->id_mask);
		k_free(id_pool->id_list);
		k_free(id_pool);
	}
}

uint32_t sip_svc_ll_id_mgr_alloc(struct sip_svc_id_pool *id_pool)
{
	uint32_t id;
	uint32_t row;
	uint32_t col;

	if (!id_pool)
		return SIP_SVC_ID_INVALID;

	if (id_pool->head == SIP_SVC_ID_INVALID)
		return SIP_SVC_ID_INVALID;

	id = id_pool->id_list[id_pool->head];

	row = id / sizeof(uint32_t);
	col = id % sizeof(uint32_t);
	id_pool->id_mask[row] |= (1 << col);

	if (id_pool->head == id_pool->tail) {
		id_pool->head = SIP_SVC_ID_INVALID;
		id_pool->tail = SIP_SVC_ID_INVALID;
	} else {
		id_pool->head++;
		if (id_pool->head == id_pool->size)
			id_pool->head = 0;
	}

	return id;
}

void sip_svc_ll_id_mgr_free(struct sip_svc_id_pool *id_pool, uint32_t id)
{
	uint32_t row;
	uint32_t col;

	if (!id_pool)
		return;

	if (id >= id_pool->size)
		return;


	row = id / sizeof(uint32_t);
	col = id % sizeof(uint32_t);

	if (!(id_pool->id_mask[row] & (1 << col)))
		return;

	id_pool->id_mask[row] &= ~(1 << col);

	if (id_pool->head == SIP_SVC_ID_INVALID) {
		id_pool->head = 0;
		id_pool->tail = 0;
	} else {
		id_pool->tail++;
		if (id_pool->tail == id_pool->size)
			id_pool->tail = 0;
		if (id_pool->head == id_pool->tail)
			return;
	}

	id_pool->id_list[id_pool->tail] = id;
}

struct sip_svc_id_map *sip_svc_ll_id_map_create(uint32_t size)
{
	struct sip_svc_id_map *id_map = NULL;
	uint32_t items_size = (sizeof(struct sip_svc_id_map_item)) * size;
	int i;

	id_map = k_malloc(sizeof(struct sip_svc_id_map));
	if (!id_map) {
		return NULL;
	}

	id_map->items = k_malloc(items_size);
	if (!id_map) {
		k_free(id_map);
		return NULL;
	}

	id_map->size = size;
	for (i = 0; i < size; i++) {
		id_map->items[i].id = SIP_SVC_ID_INVALID;
		id_map->items[i].flag = 0;
		id_map->items[i].arg1 = NULL;
		id_map->items[i].arg2 = NULL;
		id_map->items[i].arg3 = NULL;
		id_map->items[i].arg4 = NULL;
		id_map->items[i].arg5 = NULL;
	}

	return id_map;
}

void sip_svc_ll_id_map_delete(struct sip_svc_id_map *id_map)
{
	if (id_map) {
		k_free(id_map->items);
		k_free(id_map);
	}
}

int sip_svc_ll_id_map_insert_item(struct sip_svc_id_map *id_map,
				uint32_t id,
				uint32_t map_id,
				void *arg1,
				void *arg2,
				void *arg3,
				void *arg4,
				void *arg5)
{
	if (!id_map) {
		return -EINVAL;
	}

	if (id >= id_map->size)
		return -EINVAL;

	id_map->items[id].id = map_id;
	id_map->items[id].arg1 = arg1;
	id_map->items[id].arg2 = arg2;
	id_map->items[id].arg3 = arg3;
	id_map->items[id].arg4 = arg4;
	id_map->items[id].arg5 = arg5;

	return 0;
}

int sip_svc_ll_id_map_remove_item(struct sip_svc_id_map *id_map,
				uint32_t id)
{
	if (!id_map) {
		return -EINVAL;
	}

	if (id >= id_map->size) {
		return -EINVAL;
	}

	id_map->items[id].id = SIP_SVC_ID_INVALID;
	id_map->items[id].flag = 0;
	id_map->items[id].arg1 = NULL;
	id_map->items[id].arg2 = NULL;
	id_map->items[id].arg3 = NULL;
	id_map->items[id].arg4 = NULL;
	id_map->items[id].arg5 = NULL;

	return 0;
}

struct sip_svc_id_map_item *sip_svc_ll_id_map_query_item(
				struct sip_svc_id_map *id_map,
				uint32_t id)
{
	if (!id_map) {
		return NULL;
	}

	if (id >= id_map->size) {
		return NULL;
	}

	return &id_map->items[id];
}
