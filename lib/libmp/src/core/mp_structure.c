/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/kernel.h>

#include "mp_structure.h"
#include "mp_value.h"

typedef struct {
	const char *name;
	MpValue *value;
	sys_snode_t node;
} MpStructureField;

void mp_structure_init(MpStructure *structure, const char *name)
{
	if (structure != NULL) {
		structure->name = name;
		sys_slist_init(&structure->fields);
	}
}

MpStructure *mp_structure_new_empty(const char *name)
{
	MpStructure *structure = k_malloc(sizeof(MpStructure));

	mp_structure_init(structure, name);

	return structure;
}

void mp_structure_clear(MpStructure *structure)
{
	MpStructureField *field;

	if (structure == NULL) {
		return;
	}

	while (!sys_slist_is_empty(&structure->fields)) {
		field = CONTAINER_OF(sys_slist_get(&structure->fields), MpStructureField, node);
		mp_value_destroy(field->value);
		k_free(field);
	}
}

void mp_structure_destroy(MpStructure *structure)
{
	mp_structure_clear(structure);
	k_free(structure);
}

void mp_structure_append(MpStructure *structure, const char *name, MpValue *value)
{
	MpStructureField *field;

	if (structure == NULL || name == NULL || value == NULL) {
		return;
	}

	field = k_malloc(sizeof(MpStructureField));
	field->name = name;
	field->value = value;
	sys_slist_append(&structure->fields, &field->node);
}

MpStructure *mp_structure_new(const char *name, ...)
{
	va_list args;
	MpStructure *structure = k_malloc(sizeof(MpStructure));
	MpValueType type;
	MpValue *value;
	const char *field_name;

	if (structure == NULL) {
		return NULL;
	}

	structure->name = name;
	sys_slist_init(&structure->fields);
	va_start(args, name);
	while (true) {
		field_name = va_arg(args, const char *);
		if (field_name == NULL) {
			break;
		}

		type = va_arg(args, int);
		if (type != MP_TYPE_LIST) {
			value = mp_value_new_va_list(type, &args);
		} else {
			value = va_arg(args, MpValue *);
		}

		mp_structure_append(structure, field_name, value);
	}
	va_end(args);

	return structure;
}

void mp_structure_print(MpStructure *structure)
{
	MpStructureField *field;

	printk("\n");
	printk("%s\n", structure->name);
	SYS_SLIST_FOR_EACH_CONTAINER(&structure->fields, field, node) {
		printk("%s: ", field->name);
		mp_value_print(field->value, true);
	}
}

MpValue *mp_structure_get_value(MpStructure *structure, const char *name)
{
	MpStructureField *field;

	if (structure == NULL || name == NULL) {
		return NULL;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&structure->fields, field, node) {
		if (strcmp(field->name, name) == 0) {
			return field->value;
		}
	}

	return NULL;
}

bool mp_structure_remove_field(MpStructure *structure, const char *name)
{
	MpStructureField *field, *prev_field = NULL;

	if (structure == NULL || name == NULL) {
		return false;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&structure->fields, field, node) {
		if (strcmp(field->name, name) == 0) {
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

int mp_structure_len(MpStructure *structure)
{
	return sys_slist_len(&structure->fields);
}

bool mp_structure_can_intersect(MpStructure *struct1, MpStructure *struct2)
{
	MpStructure *big_structure, *small_structure;
	MpStructureField *field;
	MpValue *compared_value, *intersect_value;

	if (struct1 == NULL || struct2 == NULL) {
		return false;
	}

	/* Check media type */
	if (strcmp(struct1->name, struct2->name) != 0) {
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
		compared_value = mp_structure_get_value(small_structure, field->name);
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

MpStructure *mp_structure_intersect(MpStructure *struct1, MpStructure *struct2)
{
	MpStructureField *field;
	MpStructure *big_structure, *small_structure;
	MpStructure *intersect_structure;
	MpValue *compared_value, *intersect_value;

	if (!mp_structure_can_intersect(struct1, struct2)) {
		return NULL;
	}

	intersect_structure = mp_structure_new_empty(struct1->name);
	/* Check which one has more field than other */
	if (mp_structure_len(struct1) >= mp_structure_len(struct2)) {
		big_structure = struct1;
		small_structure = struct2;
	} else {
		big_structure = struct2;
		small_structure = struct1;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&big_structure->fields, field, node) {
		compared_value = mp_structure_get_value(small_structure, field->name);
		if (compared_value == NULL) {
			mp_structure_append(intersect_structure, field->name,
					    mp_value_duplicate(field->value));
		} else {
			intersect_value = mp_value_intersect(field->value, compared_value);
			mp_structure_append(intersect_structure, field->name, intersect_value);
		}
	}

	return intersect_structure;
}

MpStructure *mp_structure_duplicate(MpStructure *src)
{
	MpStructureField *field;
	MpStructure *dup;
	MpValue *copy_value;

	if (src == NULL) {
		return NULL;
	}

	dup = mp_structure_new_empty(src->name);
	SYS_SLIST_FOR_EACH_CONTAINER(&src->fields, field, node) {
		copy_value = mp_value_duplicate(field->value);
		mp_structure_append(dup, field->name, copy_value);
	}

	return dup;
}

bool mp_structure_is_fixed(MpStructure *structure)
{
	MpStructureField *field;

	SYS_SLIST_FOR_EACH_CONTAINER(&structure->fields, field, node) {
		if (!mp_value_is_primitive(field->value)) {
			return false;
		}
	}

	return true;
}

MpStructure *mp_structure_fixate(MpStructure *src)
{
	MpStructureField *field;
	MpStructure *fixated_structure = mp_structure_new_empty(src->name);
	MpValue *fixated_value;

	SYS_SLIST_FOR_EACH_CONTAINER(&src->fields, field, node) {
		switch (field->value->type) {
		case MP_TYPE_INT_RANGE:
			fixated_value =
				mp_value_new(MP_TYPE_INT, mp_value_get_int_range_min(field->value));
			break;
		case MP_TYPE_FRACTION_RANGE:
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

		mp_structure_append(fixated_structure, field->name, fixated_value);
	}

	return fixated_structure;
}
