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

static void mp_caps_destroy(struct mp_object *obj)
{
	struct mp_cap_structure *caps_structure;

	__ASSERT_NO_MSG(obj != NULL);
	while (!sys_slist_is_empty(&MP_CAPS(obj)->caps_structures)) {
		caps_structure = CONTAINER_OF(sys_slist_get(&MP_CAPS(obj)->caps_structures),
					      struct mp_cap_structure, node);

		mp_structure_destroy(caps_structure->structure);
		k_free(caps_structure);
	}
	k_free(obj);
}

void mp_caps_init(struct mp_caps *caps, uint8_t flag)
{
	__ASSERT_NO_MSG(caps != NULL);
	sys_slist_init(&caps->caps_structures);
	caps->object.release = mp_caps_destroy;
	caps->object.ref = ATOMIC_INIT(0);
	caps->object.flags = flag;
}

static struct mp_caps *mp_caps_new_empty_with_flag(uint8_t flags)
{
	struct mp_caps *caps = k_malloc(sizeof(struct mp_caps));

	if (caps == NULL) {
		return NULL;
	}

	mp_caps_init(caps, flags);
	return mp_caps_ref(caps);
}

static inline struct mp_caps *mp_caps_new_empty(void)
{
	return mp_caps_new_empty_with_flag(0);
}

struct mp_caps *mp_caps_new_any(void)
{
	return mp_caps_new_empty_with_flag(MP_CAPS_FLAG_ANY);
}

struct mp_caps *mp_caps_new(uint8_t media_type_id, ...)
{
	struct mp_caps *caps = mp_caps_new_empty();
	va_list var_args;
	struct mp_structure *structure;
	uint8_t field_id;
	enum mp_value_type type;
	struct mp_value *value;

	if (media_type_id == MP_MEDIA_END) {
		return caps;
	}

	va_start(var_args, media_type_id);
	structure = mp_structure_new(media_type_id, MP_CAPS_END);
	while (1) {
		field_id = (uint8_t)va_arg(var_args, uint32_t);
		if (field_id == MP_CAPS_END) {
			break;
		}

		type = va_arg(var_args, int);
		if (type != MP_TYPE_LIST) {
			value = mp_value_new_va_list(type, &var_args);
		} else {
			value = va_arg(var_args, struct mp_value *);
		}

		if (value == NULL) {
			break;
		}

		mp_structure_append(structure, field_id, value);
	}
	mp_caps_append(caps, structure);
	va_end(var_args);

	return caps;
}

void mp_caps_replace(struct mp_caps **target_caps, struct mp_caps *new_caps)
{
	__ASSERT_NO_MSG(target_caps != NULL);

	struct mp_caps *old_caps = *target_caps;

	/* Update the target with a new reference */
	*target_caps = mp_caps_ref(new_caps);

	/* Release the old reference */
	mp_caps_unref(old_caps);
}

bool mp_caps_append(struct mp_caps *caps, struct mp_structure *structure)
{
	struct mp_cap_structure *cs;

	if (caps == NULL || caps->object.flags == MP_CAPS_FLAG_ANY || !structure) {
		return false;
	}

	cs = k_malloc(sizeof(struct mp_cap_structure));
	if (cs == NULL) {
		return false;
	}

	cs->structure = structure;
	sys_slist_append(&caps->caps_structures, &cs->node);

	return true;
}

void mp_caps_print(struct mp_caps *caps)
{
	struct mp_cap_structure *cs;

	if (caps == NULL) {
		printk("Caps NULL\n");
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

bool mp_caps_is_empty(struct mp_caps *caps)
{
	return caps != NULL && caps->object.flags != MP_CAPS_FLAG_ANY &&
	       sys_slist_is_empty(&caps->caps_structures);
}

bool mp_caps_is_any(struct mp_caps *caps)
{
	return caps != NULL && caps->object.flags == MP_CAPS_FLAG_ANY;
}

bool mp_caps_is_fixed(struct mp_caps *caps)
{
	struct mp_structure *first_structure;
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

struct mp_caps *mp_caps_intersect(struct mp_caps *caps1, struct mp_caps *caps2)
{
	struct mp_caps *intersect_caps;
	struct mp_cap_structure *cs1, *cs2;

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
			struct mp_structure *struct_intersect =
				mp_structure_intersect(cs1->structure, cs2->structure);

			if (struct_intersect) {
				mp_caps_append(intersect_caps, struct_intersect);
			}
		}
	}

	return intersect_caps;
}

bool mp_caps_can_intersect(struct mp_caps *caps1, struct mp_caps *caps2)
{

	struct mp_cap_structure *cs1, *cs2;

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

struct mp_caps *mp_caps_duplicate(struct mp_caps *caps)
{
	struct mp_cap_structure *cs;
	struct mp_structure *structure;
	struct mp_caps *caps_copy;

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

struct mp_structure *mp_caps_get_structure(struct mp_caps *caps, int index)
{
	int i = 0;
	struct mp_cap_structure *cs;

	SYS_SLIST_FOR_EACH_CONTAINER(&caps->caps_structures, cs, node) {
		if (i++ == index) {
			return cs->structure;
		}
	}

	return NULL;
}

struct mp_caps *mp_caps_fixate(struct mp_caps *caps)
{
	sys_snode_t *node;
	struct mp_caps *fixed_caps;
	struct mp_structure *fixated_structure;
	struct mp_cap_structure *cs;

	if (caps == NULL || mp_caps_is_any(caps) || mp_caps_is_empty(caps)) {
		return NULL;
	}

	fixed_caps = mp_caps_new_empty();
	node = sys_slist_peek_head(&caps->caps_structures);
	if (node == NULL) {
		return NULL;
	}

	cs = CONTAINER_OF(node, struct mp_cap_structure, node);
	fixated_structure = mp_structure_fixate(cs->structure);
	if (fixated_structure == NULL) {
		return NULL;
	}

	mp_caps_append(fixed_caps, fixated_structure);

	return fixed_caps;
}
