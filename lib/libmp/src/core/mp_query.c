/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "mp_query.h"

static MpQuery *mp_query_new(MpQueryType type, ...)
{
	va_list args;
	MpValue *value;
	const char *field_name;
	MpQuery *query = (MpQuery *)k_malloc(sizeof(MpQuery));

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

void mp_query_destroy(MpQuery *query)
{
	mp_structure_clear(&query->structure);
	k_free(query);
}

MpQuery *mp_query_new_caps(MpCaps *caps)
{
	return mp_query_new(MP_QUERY_CAPS, "caps", MP_TYPE_OBJECT, caps, NULL);
}

MpCaps *mp_query_get_caps(MpQuery *query)
{
	if (query == NULL || query->type != MP_QUERY_CAPS) {
		return NULL;
	}

	return MP_CAPS(mp_value_get_object(mp_structure_get_value(&query->structure, "caps")));
}

bool mp_query_set_caps(MpQuery *query, MpCaps *caps)
{
	if (query == NULL || query->type != MP_QUERY_CAPS) {
		return false;
	}

	MpValue *value = mp_structure_get_value(&query->structure, "caps");
	if (value) {
		mp_value_set(value, MP_TYPE_OBJECT, caps);
	} else {
		mp_structure_append(&query->structure, "caps", mp_value_new(MP_TYPE_OBJECT, caps));
	}

	return true;
}

MpQuery *mp_query_new_allocation(MpCaps *caps)
{
	return mp_query_new(MP_QUERY_ALLOCATION, "allocation", MP_TYPE_OBJECT, caps, NULL);
}

static bool mp_query_set_ptr(MpQuery *query, void *ptr, const char *name)
{
	MpValue *value;

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

static void *mp_query_get_ptr(MpQuery *query, const char *name)
{
	if (query == NULL || query->type != MP_QUERY_ALLOCATION) {
		return NULL;
	}

	return mp_value_get_ptr(mp_structure_get_value(&query->structure, name));
}

bool mp_query_set_pool(MpQuery *query, MpBufferPool *pool)
{
	return mp_query_set_ptr(query, pool, "pool");
}

bool mp_query_set_pool_config(MpQuery *query, MpBufferPoolConfig *config)
{
	return mp_query_set_ptr(query, config, "pool_config");
}

MpBufferPool *mp_query_get_pool(MpQuery *query)
{
	return (MpBufferPool *)mp_query_get_ptr(query, "pool");
}

MpBufferPoolConfig *mp_query_get_pool_config(MpQuery *query)
{
	return (MpBufferPoolConfig *)mp_query_get_ptr(query, "pool_config");
}
