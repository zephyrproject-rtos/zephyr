/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/kernel.h>

#include "mp_caps.h"
#include "mp_structure.h"
#include "mp_value.h"

#define MP_STRUCTURE_END UINT8_MAX

struct mp_structure_field {
	uint8_t field_id;
	struct mp_value *value;
	sys_snode_t node;
};

void mp_structure_init(struct mp_structure *structure, uint8_t media_type_id)
{
	if (structure != NULL) {
		structure->media_type_id = media_type_id;
		sys_slist_init(&structure->fields);
	}
}

struct mp_structure *mp_structure_new_empty(uint8_t media_type_id)
{
	struct mp_structure *structure = k_malloc(sizeof(struct mp_structure));

	mp_structure_init(structure, media_type_id);

	return structure;
}

void mp_structure_clear(struct mp_structure *structure)
{
	struct mp_structure_field *field;

	if (structure == NULL) {
		return;
	}

	while (!sys_slist_is_empty(&structure->fields)) {
		field = CONTAINER_OF(sys_slist_get(&structure->fields), struct mp_structure_field,
				     node);
		mp_value_destroy(field->value);
		k_free(field);
	}
}

void mp_structure_destroy(struct mp_structure *structure)
{
	mp_structure_clear(structure);
	k_free(structure);
}

void mp_structure_append(struct mp_structure *structure, uint8_t field_id, struct mp_value *value)
{
	struct mp_structure_field *field;

	if (structure == NULL || value == NULL) {
		return;
	}

	field = k_malloc(sizeof(struct mp_structure_field));
	field->field_id = field_id;
	field->value = value;
	sys_slist_append(&structure->fields, &field->node);
}

struct mp_structure *mp_structure_new(uint8_t media_type_id, ...)
{
	va_list args;
	struct mp_structure *structure;
	enum mp_value_type type;
	struct mp_value *value;
	uint8_t field_id;

	if (media_type_id == MP_MEDIA_END) {
		return NULL;
	}

	structure = k_malloc(sizeof(struct mp_structure));
	if (structure == NULL) {
		return NULL;
	}

	structure->media_type_id = media_type_id;
	sys_slist_init(&structure->fields);
	va_start(args, media_type_id);
	while (1) {
		field_id = va_arg(args, uint32_t);
		if (field_id == MP_CAPS_END) {
			break;
		}

		type = va_arg(args, int);
		if (type != MP_TYPE_LIST) {
			value = mp_value_new_va_list(type, &args);
		} else {
			value = va_arg(args, struct mp_value *);
		}

		mp_structure_append(structure, field_id, value);
	}
	va_end(args);

	return structure;
}

void mp_structure_print(struct mp_structure *structure)
{
	struct mp_structure_field *field;

	printk("\n");
	printk("Media Type ID: %u\n", structure->media_type_id);
	SYS_SLIST_FOR_EACH_CONTAINER(&structure->fields, field, node) {
		printk("Field ID: %u\n", field->field_id);
		mp_value_print(field->value, true);
	}
}

struct mp_value *mp_structure_get_value(struct mp_structure *structure, uint8_t field_id)
{
	struct mp_structure_field *field;

	if (structure == NULL) {
		return NULL;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&structure->fields, field, node) {
		if (field->field_id == field_id) {
			return field->value;
		}
	}

	return NULL;
}

bool mp_structure_remove_field(struct mp_structure *structure, uint8_t field_id)
{
	struct mp_structure_field *field, *prev_field = NULL;

	if (structure == NULL) {
		return false;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&structure->fields, field, node) {
		if (field->field_id == field_id) {
			sys_slist_remove(&structure->fields, prev_field ? &prev_field->node : NULL,
					 &field->node);
			mp_value_destroy(field->value);
			k_free(field);
			return true;
		}
		prev_field = field;
	}

	return false;
}

int mp_structure_len(struct mp_structure *structure)
{
	return sys_slist_len(&structure->fields);
}

bool mp_structure_can_intersect(struct mp_structure *struct1, struct mp_structure *struct2)
{
	struct mp_structure *big_structure, *small_structure;
	struct mp_structure_field *field;
	struct mp_value *compared_value, *intersect_value;

	if (struct1 == NULL || struct2 == NULL) {
		return false;
	}

	/* Check media type ID */
	if (struct1->media_type_id != struct2->media_type_id) {
		return false;
	}

	/* Check which one has more field than other */
	if (mp_structure_len(struct1) >= mp_structure_len(struct2)) {
		big_structure = struct1;
		small_structure = struct2;
	} else {
		big_structure = struct2;
		small_structure = struct1;
	}

	/* Check fields in struct1 against struct2 */
	SYS_SLIST_FOR_EACH_CONTAINER(&big_structure->fields, field, node) {
		compared_value = mp_structure_get_value(small_structure, field->field_id);
		if (compared_value != NULL) {
			/* Check if there is a full intersection */
			intersect_value = mp_value_intersect(field->value, compared_value);
			if (intersect_value != NULL) {
				mp_value_destroy(intersect_value);
			} else {
				return false;
			}
		}
	}

	return true;
}

struct mp_structure *mp_structure_intersect(struct mp_structure *struct1,
					    struct mp_structure *struct2)
{
	struct mp_structure_field *field;
	struct mp_structure *big_structure, *small_structure;
	struct mp_structure *intersect_structure;
	struct mp_value *compared_value, *intersect_value;

	if (!mp_structure_can_intersect(struct1, struct2)) {
		return NULL;
	}

	intersect_structure = mp_structure_new_empty(struct1->media_type_id);
	/* Check which one has more field than other */
	if (mp_structure_len(struct1) >= mp_structure_len(struct2)) {
		big_structure = struct1;
		small_structure = struct2;
	} else {
		big_structure = struct2;
		small_structure = struct1;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&big_structure->fields, field, node) {
		compared_value = mp_structure_get_value(small_structure, field->field_id);
		if (compared_value == NULL) {
			mp_structure_append(intersect_structure, field->field_id,
					    mp_value_duplicate(field->value));
		} else {
			intersect_value = mp_value_intersect(field->value, compared_value);
			mp_structure_append(intersect_structure, field->field_id, intersect_value);
		}
	}

	return intersect_structure;
}

struct mp_structure *mp_structure_duplicate(struct mp_structure *src)
{
	struct mp_structure_field *field;
	struct mp_structure *dup;
	struct mp_value *copy_value;

	if (src == NULL) {
		return NULL;
	}

	dup = mp_structure_new_empty(src->media_type_id);
	SYS_SLIST_FOR_EACH_CONTAINER(&src->fields, field, node) {
		copy_value = mp_value_duplicate(field->value);
		mp_structure_append(dup, field->field_id, copy_value);
	}

	return dup;
}

bool mp_structure_is_fixed(struct mp_structure *structure)
{
	struct mp_structure_field *field;

	SYS_SLIST_FOR_EACH_CONTAINER(&structure->fields, field, node) {
		if (!mp_value_is_primitive(field->value)) {
			return false;
		}
	}

	return true;
}

struct mp_structure *mp_structure_fixate(struct mp_structure *src)
{
	struct mp_structure_field *field;
	struct mp_structure *fixated_structure = mp_structure_new_empty(src->media_type_id);
	struct mp_value *fixated_value;

	SYS_SLIST_FOR_EACH_CONTAINER(&src->fields, field, node) {
		switch (field->value->type) {
		case MP_TYPE_INT_RANGE:
			fixated_value =
				mp_value_new(MP_TYPE_INT, mp_value_get_int_range_min(field->value));
			break;
		case MP_TYPE_UINT_RANGE:
			fixated_value = mp_value_new(MP_TYPE_UINT,
						     mp_value_get_int_range_min(field->value));
			break;
		case MP_TYPE_INT_FRACTION_RANGE:
		case MP_TYPE_UINT_FRACTION_RANGE:
			fixated_value =
				mp_value_duplicate(mp_value_get_fraction_range_min(field->value));
			break;
		case MP_TYPE_LIST:
			fixated_value = mp_value_duplicate(mp_value_list_get(field->value, 0));
			break;
		default:
			fixated_value = mp_value_duplicate(field->value);
			break;
		}

		mp_structure_append(fixated_structure, field->field_id, fixated_value);
	}

	return fixated_structure;
}
