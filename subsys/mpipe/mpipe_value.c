/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>

#include <zephyr/mpipe/mpipe_value.h>

LOG_MODULE_REGISTER(mpipe_value, CONFIG_MPIPE_LOG_LEVEL);

#define MPIPE_VALUE_RANGES_OVERLAP(ref_val, cmp_val, vtype)                                        \
	!((ref_val)->range.min.vtype > (cmp_val)->range.max.vtype ||                               \
	  (cmp_val)->range.min.vtype > (ref_val)->range.max.vtype)

#define MPIPE_SINGLE_VALUE_IN_RANGE(ref_val, cmp_val, vtype)                                       \
	(IN_RANGE((cmp_val)->vtype, (ref_val)->range.min.vtype, (ref_val)->range.max.vtype))

#define MPIPE_VALUE_PRIMITIVE_MASK                                                                 \
	(BIT(MPIPE_TYPE_BOOLEAN) | BIT(MPIPE_TYPE_INT) | BIT(MPIPE_TYPE_UINT))

static const uint32_t mpipe_value_intersect_mask[MPIPE_TYPE_COUNT] = {
	[MPIPE_TYPE_NONE] = 0,
	[MPIPE_TYPE_BOOLEAN] = BIT(MPIPE_TYPE_BOOLEAN),
	[MPIPE_TYPE_INT] = BIT(MPIPE_TYPE_INT) | BIT(MPIPE_TYPE_INT_RANGE),
	[MPIPE_TYPE_UINT] = BIT(MPIPE_TYPE_UINT) | BIT(MPIPE_TYPE_UINT_RANGE),
	[MPIPE_TYPE_INT_RANGE] = BIT(MPIPE_TYPE_INT) | BIT(MPIPE_TYPE_INT_RANGE),
	[MPIPE_TYPE_UINT_RANGE] = BIT(MPIPE_TYPE_UINT) | BIT(MPIPE_TYPE_UINT_RANGE),
};

bool mpipe_value_is_primitive(const struct mpipe_value *value)
{
	__ASSERT_NO_MSG(value != NULL);

	if (!IN_RANGE(value->type, MPIPE_TYPE_NONE + 1, MPIPE_TYPE_COUNT - 1)) {
		return false;
	}

	return (MPIPE_VALUE_PRIMITIVE_MASK & BIT(value->type)) != 0;
}

static void mpipe_value_set_range(struct mpipe_value *value, enum mpipe_value_type type,
				  va_list *args)
{
	/*
	 * Reading an argument as a type it was not passed as is undefined, so
	 * the two range types are read apart even though the bounds share a
	 * union: a signed range is passed as int, an unsigned one as uint32_t.
	 */
	if (type == MPIPE_TYPE_INT_RANGE) {
		value->range.min.v_int = va_arg(*args, int);
		value->range.max.v_int = va_arg(*args, int);
		value->range.step.v_int = va_arg(*args, int);
	} else {
		value->range.min.v_uint = va_arg(*args, uint32_t);
		value->range.max.v_uint = va_arg(*args, uint32_t);
		value->range.step.v_uint = va_arg(*args, uint32_t);
	}
}

int mpipe_value_set_va_list(struct mpipe_value *value, enum mpipe_value_type type, va_list *args)
{
	__ASSERT_NO_MSG(args != NULL);

	__ASSERT_NO_MSG(value != NULL);

	/* Any integer type narrower than int arrives as int through the variadic argument list */
	switch (type) {
	case MPIPE_TYPE_BOOLEAN:
		/* A bool was promoted to int so it has to be narrowed here */
		value->v_boolean = (va_arg(*args, int) != 0);
		break;
	case MPIPE_TYPE_INT:
		value->v_int = va_arg(*args, int);
		break;
	case MPIPE_TYPE_UINT:
		value->v_uint = va_arg(*args, uint32_t);
		break;
	case MPIPE_TYPE_INT_RANGE:
	case MPIPE_TYPE_UINT_RANGE:
		mpipe_value_set_range(value, type, args);
		break;
	default:
		LOG_ERR("Unknown mpipe_value type: %d", type);
		return -EINVAL;
	}

	/* Stamp the type last so a rejected one leaves the value untouched */
	value->type = type;

	return 0;
}

int mpipe_value_set(struct mpipe_value *value, int type, ...)
{
	int ret;
	va_list args;

	va_start(args, type);
	ret = mpipe_value_set_va_list(value, type, &args);
	va_end(args);

	return ret;
}

int32_t mpipe_value_get_int(const struct mpipe_value *value)
{
	__ASSERT_NO_MSG(value != NULL);

	return value->v_int;
}

uint32_t mpipe_value_get_uint(const struct mpipe_value *value)
{
	__ASSERT_NO_MSG(value != NULL);

	return value->v_uint;
}

bool mpipe_value_get_boolean(const struct mpipe_value *value)
{
	__ASSERT_NO_MSG(value != NULL);

	return value->v_boolean;
}

int32_t mpipe_value_get_int_range_min(const struct mpipe_value *range)
{
	__ASSERT_NO_MSG(range != NULL);

	return range->range.min.v_int;
}

int32_t mpipe_value_get_int_range_max(const struct mpipe_value *range)
{
	__ASSERT_NO_MSG(range != NULL);

	return range->range.max.v_int;
}

int32_t mpipe_value_get_int_range_step(const struct mpipe_value *range)
{
	__ASSERT_NO_MSG(range != NULL);

	return range->range.step.v_int;
}

uint32_t mpipe_value_get_uint_range_min(const struct mpipe_value *range)
{
	__ASSERT_NO_MSG(range != NULL);

	return range->range.min.v_uint;
}

uint32_t mpipe_value_get_uint_range_max(const struct mpipe_value *range)
{
	__ASSERT_NO_MSG(range != NULL);

	return range->range.max.v_uint;
}

uint32_t mpipe_value_get_uint_range_step(const struct mpipe_value *range)
{
	__ASSERT_NO_MSG(range != NULL);

	return range->range.step.v_uint;
}

static bool mpipe_value_primitive_equal(const struct mpipe_value *val1,
					const struct mpipe_value *val2)
{
	switch (val1->type) {
	case MPIPE_TYPE_BOOLEAN:
		return val1->v_boolean == val2->v_boolean;
	case MPIPE_TYPE_INT:
		return val1->v_int == val2->v_int;
	case MPIPE_TYPE_UINT:
		return val1->v_uint == val2->v_uint;
	default:
		return false;
	}
}

static int mpipe_value_intersect_range(const struct mpipe_value *ref_val,
				       const struct mpipe_value *compare_val,
				       struct mpipe_value *out)
{
	if (ref_val->type == MPIPE_TYPE_INT_RANGE && compare_val->type == MPIPE_TYPE_INT_RANGE) {
		if (!MPIPE_VALUE_RANGES_OVERLAP(ref_val, compare_val, v_int)) {
			return -ENOENT;
		}

		out->type = MPIPE_TYPE_INT_RANGE;
		out->range.min.v_int = MAX(ref_val->range.min.v_int, compare_val->range.min.v_int);
		out->range.max.v_int = MIN(ref_val->range.max.v_int, compare_val->range.max.v_int);
		out->range.step.v_int =
			(int32_t)sys_gcd(ref_val->range.step.v_int, compare_val->range.step.v_int);

		return 0;
	}

	if (ref_val->type == MPIPE_TYPE_UINT_RANGE && compare_val->type == MPIPE_TYPE_UINT_RANGE) {
		if (!MPIPE_VALUE_RANGES_OVERLAP(ref_val, compare_val, v_uint)) {
			return -ENOENT;
		}

		out->type = MPIPE_TYPE_UINT_RANGE;
		out->range.min.v_uint =
			MAX(ref_val->range.min.v_uint, compare_val->range.min.v_uint);
		out->range.max.v_uint =
			MIN(ref_val->range.max.v_uint, compare_val->range.max.v_uint);
		out->range.step.v_uint =
			sys_gcd(ref_val->range.step.v_uint, compare_val->range.step.v_uint);

		return 0;
	}

	if ((ref_val->type == MPIPE_TYPE_INT_RANGE && compare_val->type == MPIPE_TYPE_INT &&
	     MPIPE_SINGLE_VALUE_IN_RANGE(ref_val, compare_val, v_int)) ||
	    (ref_val->type == MPIPE_TYPE_UINT_RANGE && compare_val->type == MPIPE_TYPE_UINT &&
	     MPIPE_SINGLE_VALUE_IN_RANGE(ref_val, compare_val, v_uint))) {
		*out = *compare_val;
		return 0;
	}

	return -ENOENT;
}

int mpipe_value_intersect(const struct mpipe_value *val1, const struct mpipe_value *val2,
			  struct mpipe_value *out)
{
	const struct mpipe_value *ref_val, *compare_val;

	__ASSERT_NO_MSG(out != NULL);

	/* Only a pair of types the mask allows can have a common value */
	__ASSERT_NO_MSG(val1 != NULL);
	__ASSERT_NO_MSG(val2 != NULL);

	if (!IN_RANGE(val1->type, MPIPE_TYPE_NONE, MPIPE_TYPE_COUNT - 1) ||
	    !IN_RANGE(val2->type, MPIPE_TYPE_NONE, MPIPE_TYPE_COUNT - 1) ||
	    (mpipe_value_intersect_mask[val1->type] & BIT(val2->type)) == 0) {
		return -ENOENT;
	}

	/*
	 * A container type always has a higher ordinal than the scalar type
	 * it can contain, so the greater of the two is the one to dispatch on.
	 */
	if (val1->type >= val2->type) {
		ref_val = val1;
		compare_val = val2;
	} else {
		ref_val = val2;
		compare_val = val1;
	}

	if (mpipe_value_is_primitive(ref_val)) {
		if (!mpipe_value_primitive_equal(val1, val2)) {
			return -ENOENT;
		}

		*out = *val1;

		return 0;
	}

	switch (ref_val->type) {
	case MPIPE_TYPE_INT_RANGE:
	case MPIPE_TYPE_UINT_RANGE:
		return mpipe_value_intersect_range(ref_val, compare_val, out);
	default:
		return -ENOENT;
	}
}

static inline void mpipe_value_print_boolean(const struct mpipe_value *value)
{
	printk("%s", value->v_boolean ? "true" : "false");
}

static inline void mpipe_value_print_int(const struct mpipe_value *value)
{
	printk("%d", value->v_int);
}

static inline void mpipe_value_print_uint(const struct mpipe_value *value)
{
	printk("%u", value->v_uint);
}

static inline void mpipe_value_print_int_range(const struct mpipe_value *value)
{
	printk("[%d, %d, %d]", value->range.min.v_int, value->range.max.v_int,
	       value->range.step.v_int);
}

static inline void mpipe_value_print_uint_range(const struct mpipe_value *value)
{
	printk("[%u, %u, %u]", value->range.min.v_uint, value->range.max.v_uint,
	       value->range.step.v_uint);
}

typedef void (*mpipe_value_print_fn)(const struct mpipe_value *);

static const mpipe_value_print_fn mpipe_value_print_table[MPIPE_TYPE_COUNT] = {
	[MPIPE_TYPE_NONE] = NULL,
	[MPIPE_TYPE_BOOLEAN] = mpipe_value_print_boolean,
	[MPIPE_TYPE_INT] = mpipe_value_print_int,
	[MPIPE_TYPE_UINT] = mpipe_value_print_uint,
	[MPIPE_TYPE_INT_RANGE] = mpipe_value_print_int_range,
	[MPIPE_TYPE_UINT_RANGE] = mpipe_value_print_uint_range,
};

void mpipe_value_print(const struct mpipe_value *value, bool new_line)
{
	__ASSERT_NO_MSG(value != NULL);

	if (value->type >= ARRAY_SIZE(mpipe_value_print_table) ||
	    mpipe_value_print_table[value->type] == NULL) {
		LOG_ERR("Invalid mpipe_value to print");
		return;
	}

	mpipe_value_print_table[value->type](value);

	if (new_line) {
		printk("\n");
	}
}
