/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdarg.h>
#include <string.h>
#include <zephyr/kernel.h>

#include "mp_caps.h"
#include "mp_structure.h"
#include "mp_object.h"

static void mp_caps_destroy(MpObject *obj)
{
	MpCapStructure *caps_structure;

	__ASSERT_NO_MSG(obj != NULL);
	while (!sys_slist_is_empty(&MP_CAPS(obj)->caps_structures)) {
		caps_structure = CONTAINER_OF(sys_slist_get(&MP_CAPS(obj)->caps_structures),
					      MpCapStructure, node);

		mp_structure_destroy(caps_structure->structure);
		k_free(caps_structure);
	}
	k_free(obj);
}

void mp_caps_init(MpCaps *caps, uint32_t flag)
{
	__ASSERT_NO_MSG(caps != NULL);
	sys_slist_init(&caps->caps_structures);
	caps->object.release = mp_caps_destroy;
	caps->object.ref = ATOMIC_INIT(0);
	caps->object.flags = flag;
}

static MpCaps *mp_caps_new_empty()
{
	MpCaps *caps = k_malloc(sizeof(MpCaps));

	mp_caps_init(caps, 0);

	return mp_caps_ref(caps);
};

MpCaps *mp_caps_new_any()
{
	MpCaps *caps = k_malloc(sizeof(MpCaps));

	mp_caps_init(caps, MP_CAPS_FLAG_ANY);

	return mp_caps_ref(caps);
}

MpCaps *mp_caps_new(const char *media_type, ...)
{
	MpCaps *caps = mp_caps_new_empty();
	va_list var_args;
	MpStructure *structure;
	const char *field_name;
	MpValueType type;
	MpValue *value;

	if (media_type == NULL) {
		return caps;
	}

	va_start(var_args, media_type);
	structure = mp_structure_new(media_type, NULL);
	while (true) {
		field_name = va_arg(var_args, const char *);
		if (field_name == NULL) {
			break;
		}

		type = va_arg(var_args, int);
		if (type != MP_TYPE_LIST) {
			value = mp_value_new_va_list(type, &var_args);

		} else {
			value = va_arg(var_args, MpValue *);
		}
		if (value == NULL) {
			break;
		}

		mp_structure_append(structure, field_name, value);
	}
	mp_caps_append(caps, structure);
	va_end(var_args);

	return caps;
}

void mp_caps_replace(MpCaps **target_caps, MpCaps *new_caps)
{
	__ASSERT_NO_MSG(target_caps != NULL);

	MpCaps *old_caps = *target_caps;

	/* Update the target with a new reference */
	*target_caps = mp_caps_ref(new_caps);

	/* Release the old reference */
	mp_caps_unref(old_caps);
}

bool mp_caps_append(MpCaps *caps, MpStructure *structure)
{
	MpCapStructure *cs;

	if (caps == NULL || caps->object.flags == MP_CAPS_FLAG_ANY || !structure) {
		return false;
	}

	cs = k_malloc(sizeof(MpCapStructure));
	if (cs == NULL) {
		return false;
	}

	cs->structure = structure;
	sys_slist_append(&caps->caps_structures, &cs->node);

	return true;
}

void mp_caps_print(MpCaps *caps)
{
	MpCapStructure *cs;

	if (caps == NULL) {
		return;
	}

	if (mp_caps_is_any(caps)) {
		printk("Caps ANY\n");
		return;
	}

	if (mp_caps_is_empty(caps)) {
		printk("Caps EMPTY\n");
		return;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&caps->caps_structures, cs, node) {
		mp_structure_print(cs->structure);
	}
}

bool mp_caps_is_empty(MpCaps *caps)
{
	return caps != NULL && caps->object.flags != MP_CAPS_FLAG_ANY &&
	       sys_slist_is_empty(&caps->caps_structures);
}

bool mp_caps_is_any(MpCaps *caps)
{
	return caps != NULL && caps->object.flags == MP_CAPS_FLAG_ANY;
}

bool mp_caps_is_fixed(MpCaps *caps)
{
	MpStructure *first_structure;
	int structure_count;

	if (caps == NULL) {
		return false;
	}

	structure_count = sys_slist_len(&caps->caps_structures);
	if (structure_count == 0 || structure_count > 1) {
		return false;
	}

	first_structure = mp_caps_get_structure(caps, 0);

	return first_structure ? mp_structure_is_fixed(first_structure) : false;
}

MpCaps *mp_caps_intersect(MpCaps *caps1, MpCaps *caps2)
{
	MpCaps *intersect_caps;
	MpCapStructure *cs1, *cs2;

	if (caps1 == NULL || caps2 == NULL || mp_caps_is_empty(caps1) || mp_caps_is_empty(caps2)) {
		return NULL;
	}

	if (mp_caps_is_any(caps1)) {
		return mp_caps_duplicate(caps2);
	}

	if (mp_caps_is_any(caps2)) {
		return mp_caps_duplicate(caps1);
	}

	intersect_caps = mp_caps_new_empty();
	SYS_SLIST_FOR_EACH_CONTAINER(&caps1->caps_structures, cs1, node) {
		SYS_SLIST_FOR_EACH_CONTAINER(&caps2->caps_structures, cs2, node) {
			MpStructure *struct_intersect =
				mp_structure_intersect(cs1->structure, cs2->structure);

			if (struct_intersect) {
				mp_caps_append(intersect_caps, struct_intersect);
			}
		}
	}

	return intersect_caps;
}

bool mp_caps_can_intersect(MpCaps *caps1, MpCaps *caps2)
{

	MpCapStructure *cs1, *cs2;

	if (caps1 == NULL || caps2 == NULL || mp_caps_is_empty(caps1) || mp_caps_is_empty(caps2)) {
		return false;
	}

	if (mp_caps_is_any(caps1) || mp_caps_is_any(caps2)) {
		return true;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&caps1->caps_structures, cs1, node) {
		SYS_SLIST_FOR_EACH_CONTAINER(&caps2->caps_structures, cs2, node) {
			if (mp_structure_can_intersect(cs1->structure, cs2->structure)) {
				return true;
			}
		}
	}

	return false;
}

MpCaps *mp_caps_duplicate(MpCaps *caps)
{
	MpCapStructure *cs;
	MpStructure *structure;
	MpCaps *caps_copy;

	if (caps == NULL) {
		return NULL;
	}

	if (mp_caps_is_any(caps)) {
		return mp_caps_new_any();
	}

	caps_copy = mp_caps_new_empty();
	SYS_SLIST_FOR_EACH_CONTAINER(&caps->caps_structures, cs, node) {
		structure = mp_structure_duplicate(cs->structure);
		if (structure) {
			mp_caps_append(caps_copy, structure);
		}
	}

	return caps_copy;
}

MpStructure *mp_caps_get_structure(MpCaps *caps, int index)
{
	int i = 0;
	MpCapStructure *cs;

	SYS_SLIST_FOR_EACH_CONTAINER(&caps->caps_structures, cs, node) {
		if (i++ == index) {
			return cs->structure;
		}
	}

	return NULL;
}

MpCaps *mp_caps_fixate(MpCaps *caps)
{
	sys_snode_t *node;
	MpCaps *fixed_caps;
	MpStructure *fixated_structure;
	MpCapStructure *cs;

	if (caps == NULL || mp_caps_is_any(caps) || mp_caps_is_empty(caps)) {
		return NULL;
	}

	fixed_caps = mp_caps_new_empty();
	node = sys_slist_peek_head(&caps->caps_structures);
	if (node == NULL) {
		return NULL;
	}

	cs = CONTAINER_OF(node, MpCapStructure, node);
	fixated_structure = mp_structure_fixate(cs->structure);
	if (fixated_structure == NULL) {
		return NULL;
	}

	mp_caps_append(fixed_caps, fixated_structure);

	return fixed_caps;
}
