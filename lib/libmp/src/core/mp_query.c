/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "mp_query.h"

static struct mp_query *mp_query_new(enum mp_query_type type, ...)
{
	va_list args;
	struct mp_value *value;
	const char *field_name;
	struct mp_query *query = (struct mp_query *)k_malloc(sizeof(struct mp_query));

	if (query == NULL) {
		return NULL;
	}

	query->type = type;
	mp_structure_init(&query->structure, NULL);
	va_start(args, type);
	while ((field_name = va_arg(args, const char *)) != NULL) {
		value = mp_value_new_va_list(va_arg(args, int), &args);
		if (value == NULL) {
			break;
		}
		mp_structure_append(&query->structure, field_name, value);
	}
	va_end(args);

	return query;
}

void mp_query_destroy(struct mp_query *query)
{
	mp_structure_clear(&query->structure);
	k_free(query);
}

struct mp_query *mp_query_new_caps(struct mp_caps *caps)
{
	return mp_query_new(MP_QUERY_CAPS, "caps", MP_TYPE_OBJECT, caps, NULL);
}

struct mp_caps *mp_query_get_caps(struct mp_query *query)
{
	if (query == NULL || query->type != MP_QUERY_CAPS) {
		return NULL;
	}

	return MP_CAPS(mp_value_get_object(mp_structure_get_value(&query->structure, "caps")));
}

bool mp_query_set_caps(struct mp_query *query, struct mp_caps *caps)
{
	if (query == NULL || query->type != MP_QUERY_CAPS) {
		return false;
	}

	struct mp_value *value = mp_structure_get_value(&query->structure, "caps");

	if (value) {
		mp_value_set(value, MP_TYPE_OBJECT, caps);
	} else {
		mp_structure_append(&query->structure, "caps", mp_value_new(MP_TYPE_OBJECT, caps));
	}

	return true;
}

struct mp_query *mp_query_new_allocation(struct mp_caps *caps)
{
	return mp_query_new(MP_QUERY_ALLOCATION, "allocation", MP_TYPE_OBJECT, caps, NULL);
}

static bool mp_query_set_ptr(struct mp_query *query, void *ptr, const char *name)
{
	struct mp_value *value;

	if (query == NULL || query->type != MP_QUERY_ALLOCATION) {
		return false;
	}

	value = mp_structure_get_value(&query->structure, name);
	if (value != NULL) {
		mp_value_set(value, MP_TYPE_PTR, ptr);
	} else {
		mp_structure_append(&query->structure, name, mp_value_new(MP_TYPE_PTR, ptr));
	}

	return true;
}

static void *mp_query_get_ptr(struct mp_query *query, const char *name)
{
	if (query == NULL || query->type != MP_QUERY_ALLOCATION) {
		return NULL;
	}

	return mp_value_get_ptr(mp_structure_get_value(&query->structure, name));
}

bool mp_query_set_pool(struct mp_query *query, struct mp_buffer_pool *pool)
{
	return mp_query_set_ptr(query, pool, "pool");
}

bool mp_query_set_pool_config(struct mp_query *query, struct mp_buffer_pool_config *config)
{
	return mp_query_set_ptr(query, config, "pool_config");
}

struct mp_buffer_pool *mp_query_get_pool(struct mp_query *query)
{
	return (struct mp_buffer_pool *)mp_query_get_ptr(query, "pool");
}

struct mp_buffer_pool_config *mp_query_get_pool_config(struct mp_query *query)
{
	return (struct mp_buffer_pool_config *)mp_query_get_ptr(query, "pool_config");
}
