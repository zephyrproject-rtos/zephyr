/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "mp_query.h"

enum {
	MP_QUERY_POOL = 0,
	MP_QUERY_POOL_CONFIG,
};

static struct mp_query *mp_query_new(enum mp_query_type qtype, uint8_t vtype, const void *value)
{
	struct mp_query *query = (struct mp_query *)k_malloc(sizeof(struct mp_query));

	if (query == NULL) {
		return NULL;
	}

	query->type = qtype;
	mp_structure_init(&query->structure, MP_MEDIA_UNKNOWN);
	mp_structure_append(&query->structure, query->type, mp_value_new(vtype, value, NULL));

	return query;
}

void mp_query_destroy(struct mp_query *query)
{
	mp_structure_clear(&query->structure);
	k_free(query);
}

struct mp_query *mp_query_new_caps(struct mp_caps *caps)
{
	return mp_query_new(MP_QUERY_CAPS, MP_TYPE_OBJECT, caps);
}

struct mp_caps *mp_query_get_caps(struct mp_query *query)
{
	if (query == NULL || query->type != MP_QUERY_CAPS) {
		return NULL;
	}

	return MP_CAPS(
		mp_value_get_object(mp_structure_get_value(&query->structure, MP_QUERY_CAPS)));
}

bool mp_query_set_caps(struct mp_query *query, struct mp_caps *caps)
{
	if (query == NULL || query->type != MP_QUERY_CAPS) {
		return false;
	}

	struct mp_value *value = mp_structure_get_value(&query->structure, MP_QUERY_CAPS);

	if (value) {
		mp_value_set(value, MP_TYPE_OBJECT, caps);
	} else {
		mp_structure_append(&query->structure, MP_QUERY_CAPS,
				    mp_value_new(MP_TYPE_OBJECT, caps));
	}

	return true;
}

struct mp_query *mp_query_new_allocation(struct mp_caps *caps)
{
	return mp_query_new(MP_QUERY_ALLOCATION, MP_TYPE_OBJECT, caps);
}

static bool mp_query_set_ptr(struct mp_query *query, void *ptr, uint8_t field)
{
	struct mp_value *value;

	if (query == NULL || query->type != MP_QUERY_ALLOCATION) {
		return false;
	}

	value = mp_structure_get_value(&query->structure, field);
	if (value != NULL) {
		mp_value_set(value, MP_TYPE_PTR, ptr);
	} else {
		mp_structure_append(&query->structure, field, mp_value_new(MP_TYPE_PTR, ptr));
	}

	return true;
}

static void *mp_query_get_ptr(struct mp_query *query, uint8_t field)
{
	if (query == NULL || query->type != MP_QUERY_ALLOCATION) {
		return NULL;
	}

	return mp_value_get_ptr(mp_structure_get_value(&query->structure, field));
}

bool mp_query_set_pool(struct mp_query *query, struct mp_buffer_pool *pool)
{
	return mp_query_set_ptr(query, pool, MP_QUERY_POOL);
}

bool mp_query_set_pool_config(struct mp_query *query, struct mp_buffer_pool_config *config)
{
	return mp_query_set_ptr(query, config, MP_QUERY_POOL_CONFIG);
}

struct mp_buffer_pool *mp_query_get_pool(struct mp_query *query)
{
	return (struct mp_buffer_pool *)mp_query_get_ptr(query, MP_QUERY_POOL);
}

struct mp_buffer_pool_config *mp_query_get_pool_config(struct mp_query *query)
{
	return (struct mp_buffer_pool_config *)mp_query_get_ptr(query, MP_QUERY_POOL_CONFIG);
}
