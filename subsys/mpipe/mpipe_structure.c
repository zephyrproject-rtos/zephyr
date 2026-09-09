/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>

#include <zephyr/mpipe/mpipe_structure.h>
#include <zephyr/mpipe/mpipe_value.h>

LOG_MODULE_REGISTER(mpipe_structure, CONFIG_MPIPE_LOG_LEVEL);

int mpipe_structure_init(struct mpipe_structure *structure, uint8_t media_type_id)
{
	__ASSERT_NO_MSG(structure != NULL);

	if (media_type_id >= MPIPE_MEDIA_END) {
		return -EINVAL;
	}

	structure->media_type_id = media_type_id;
	structure->flags = 0;
	structure->num_fields = 0;

	return 0;
}

int mpipe_structure_init_any(struct mpipe_structure *structure)
{
	__ASSERT_NO_MSG(structure != NULL);

	int ret = mpipe_structure_init(structure, MPIPE_MEDIA_UNKNOWN);

	if (ret != 0) {
		return ret;
	}

	structure->flags = MPIPE_STRUCTURE_FLAG_ANY;

	return 0;
}

bool mpipe_structure_is_any(const struct mpipe_structure *structure)
{
	return structure != NULL && (structure->flags & MPIPE_STRUCTURE_FLAG_ANY) != 0;
}

bool mpipe_structure_is_empty(const struct mpipe_structure *structure)
{
	return structure != NULL && !mpipe_structure_is_any(structure) &&
	       structure->num_fields == 0;
}

int mpipe_structure_clear(struct mpipe_structure *structure)
{
	__ASSERT_NO_MSG(structure != NULL);

	/*
	 * Resetting num_fields to empty the structure. Every read of ids and values is
	 * bounded by num_fields, so the slots past it are already unreachable. The media
	 * type and the flags are left alone so the structure can be filled in again as it is.
	 */
	structure->num_fields = 0;

	return 0;
}

int mpipe_structure_append_value(struct mpipe_structure *structure, uint8_t field_id,
				 const struct mpipe_value *value)
{
	__ASSERT_NO_MSG(structure != NULL);
	__ASSERT_NO_MSG(value != NULL);

	if (field_id >= MPIPE_CAPS_END) {
		return -EINVAL;
	}

	if (mpipe_structure_get_value(structure, field_id) != NULL) {
		return -EEXIST;
	}

	if (structure->num_fields >= CONFIG_MPIPE_STRUCTURE_MAX_FIELDS) {
		LOG_ERR("No slot left for field %u, raise CONFIG_MPIPE_STRUCTURE_MAX_FIELDS",
			field_id);
		return -ENOSPC;
	}

	/* A value owns nothing, so filling the slot is a plain copy */
	structure->ids[structure->num_fields] = field_id;
	structure->values[structure->num_fields] = *value;
	structure->num_fields++;

	return 0;
}

int mpipe_structure_copy_field(const struct mpipe_structure *src, struct mpipe_structure *dst,
			       uint8_t field_id)
{
	const struct mpipe_value *value = mpipe_structure_get_value(src, field_id);

	if (value == NULL) {
		return -ENOENT;
	}

	return mpipe_structure_append_value(dst, field_id, value);
}

int mpipe_structure_init_fields(struct mpipe_structure *structure, uint8_t media_type_id, ...)
{
	va_list args;
	struct mpipe_value value;
	enum mpipe_value_type type;
	uint8_t field_id;
	int ret;

	ret = mpipe_structure_init(structure, media_type_id);
	if (ret != 0) {
		return ret;
	}

	va_start(args, media_type_id);
	while (1) {
		field_id = (uint8_t)va_arg(args, uint32_t);
		if (field_id == MPIPE_CAPS_END) {
			break;
		}

		type = va_arg(args, int);
		ret = mpipe_value_set_va_list(&value, type, &args);
		if (ret != 0) {
			break;
		}

		ret = mpipe_structure_append_value(structure, field_id, &value);
		if (ret != 0) {
			break;
		}
	}
	va_end(args);

	if (ret != 0) {
		mpipe_structure_clear(structure);
	}

	return ret;
}

void mpipe_structure_print(const struct mpipe_structure *structure)
{
	__ASSERT_NO_MSG(structure != NULL);

	printk("\n");
	printk("Media Type ID: %u\n", structure->media_type_id);
	for (uint8_t i = 0; i < structure->num_fields; i++) {
		printk("Field ID: %u\n", structure->ids[i]);
		mpipe_value_print(&structure->values[i], true);
	}
}

const struct mpipe_value *mpipe_structure_get_value(const struct mpipe_structure *structure,
						    uint8_t field_id)
{
	if (structure == NULL) {
		return NULL;
	}

	for (uint8_t i = 0; i < structure->num_fields; i++) {
		if (structure->ids[i] == field_id) {
			return &structure->values[i];
		}
	}

	return NULL;
}

int mpipe_structure_remove_field(struct mpipe_structure *structure, uint8_t field_id)
{
	__ASSERT_NO_MSG(structure != NULL);

	for (uint8_t i = 0; i < structure->num_fields; i++) {
		if (structure->ids[i] != field_id) {
			continue;
		}

		/* Slots carry no order, so the last one can fill the hole */
		structure->num_fields--;
		structure->ids[i] = structure->ids[structure->num_fields];
		structure->values[i] = structure->values[structure->num_fields];

		return 0;
	}

	return -ENOENT;
}

int mpipe_structure_intersect(const struct mpipe_structure *struct1,
			      const struct mpipe_structure *struct2, struct mpipe_structure *out)
{
	int ret;
	struct mpipe_value intersect_value;
	bool common = false;

	/*
	 * The result is built into out field by field, so an out that is also
	 * an input would be read after it has been reset. Nothing needs it, so
	 * it is refused rather than paid for with a scratch structure.
	 */
	__ASSERT_NO_MSG(struct1 != NULL);
	__ASSERT_NO_MSG(struct2 != NULL);
	__ASSERT_NO_MSG(out != NULL);

	if (out == struct1 || out == struct2) {
		return -EINVAL;
	}

	/* A structure that constrains nothing leaves the other side unchanged */
	if (mpipe_structure_is_any(struct1)) {
		*out = *struct2;
		return 0;
	}

	if (mpipe_structure_is_any(struct2)) {
		*out = *struct1;
		return 0;
	}

	if (struct1->media_type_id != struct2->media_type_id) {
		return -EINVAL;
	}

	ret = mpipe_structure_init(out, struct1->media_type_id);
	if (ret != 0) {
		return ret;
	}

	for (uint8_t i = 0; i < struct1->num_fields; i++) {
		const struct mpipe_value *other =
			mpipe_structure_get_value(struct2, struct1->ids[i]);

		if (other != NULL) {
			if (mpipe_value_intersect(&struct1->values[i], other, &intersect_value) !=
			    0) {
				mpipe_structure_clear(out);
				return -ENOENT;
			}

			common = true;
		} else {
			/* A field only one side constrains is carried through unchanged */
			intersect_value = struct1->values[i];
		}

		ret = mpipe_structure_append_value(out, struct1->ids[i], &intersect_value);
		if (ret != 0) {
			mpipe_structure_clear(out);
			return ret;
		}
	}

	/* Structures with nothing in common do not intersect */
	if (!common) {
		mpipe_structure_clear(out);
		return -ENOENT;
	}

	/* Whatever only the other side constrains is carried through as well */
	for (uint8_t i = 0; i < struct2->num_fields; i++) {
		if (mpipe_structure_get_value(struct1, struct2->ids[i]) != NULL) {
			continue;
		}

		ret = mpipe_structure_append_value(out, struct2->ids[i], &struct2->values[i]);
		if (ret != 0) {
			mpipe_structure_clear(out);
			return ret;
		}
	}

	return 0;
}

bool mpipe_structure_is_fixed(const struct mpipe_structure *structure)
{
	__ASSERT_NO_MSG(structure != NULL);

	if (mpipe_structure_is_any(structure) || structure->num_fields == 0) {
		return false;
	}

	for (uint8_t i = 0; i < structure->num_fields; i++) {
		if (!mpipe_value_is_primitive(&structure->values[i])) {
			return false;
		}
	}

	return true;
}

int mpipe_structure_fixate(const struct mpipe_structure *src, struct mpipe_structure *out)
{
	struct mpipe_value fixated_value;
	int ret;

	/* Same as the intersection: out is built from src, so it cannot be src */
	__ASSERT_NO_MSG(src != NULL);
	__ASSERT_NO_MSG(out != NULL);

	if (out == src) {
		return -EINVAL;
	}

	if (mpipe_structure_is_any(src) || src->num_fields == 0) {
		return -ENOENT;
	}

	ret = mpipe_structure_init(out, src->media_type_id);
	if (ret != 0) {
		return ret;
	}

	for (uint8_t i = 0; i < src->num_fields; i++) {
		const struct mpipe_value *value = &src->values[i];

		switch (value->type) {
		case MPIPE_TYPE_INT_RANGE:
			fixated_value.type = MPIPE_TYPE_INT;
			fixated_value.v_int = mpipe_value_get_int_range_min(value);
			break;
		case MPIPE_TYPE_UINT_RANGE:
			fixated_value.type = MPIPE_TYPE_UINT;
			fixated_value.v_uint = mpipe_value_get_uint_range_min(value);
			break;
		default:
			/* Already a single value, so it fixates to itself */
			fixated_value = *value;
			break;
		}

		ret = mpipe_structure_append_value(out, src->ids[i], &fixated_value);
		if (ret != 0) {
			mpipe_structure_clear(out);
			return ret;
		}
	}

	return 0;
}
