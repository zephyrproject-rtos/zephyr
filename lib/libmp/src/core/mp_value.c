/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "mp_value.h"

LOG_MODULE_REGISTER(mp_value, CONFIG_LIBMP_LOG_LEVEL);

#define MP_VALUE(value)                      ((struct mp_value *)value)
#define MP_VALUE_SIMPLE(value)               ((struct mp_value_simple *)value)
#define MP_VALUE_RANGE(value)                ((struct mp_value_range *)value)
#define MP_VALUE_FRACTION(value)             ((struct mp_value_fraction *)value)
#define MP_VALUE_FRACTION_RANGE(value)       ((struct mp_value_fraction_range *)value)
#define MP_VALUE_LIST(value)                 ((struct mp_value_list *)value)
#define MP_VALUE_CONST(value)                ((const struct mp_value *)value)
#define MP_VALUE_SIMPLE_CONST(value)         ((const struct mp_value_simple *)value)
#define MP_VALUE_RANGE_CONST(value)          ((const struct mp_value_range *)value)
#define MP_VALUE_FRACTION_CONST(value)       ((const struct mp_value_fraction *)value)
#define MP_VALUE_FRACTION_RANGE_CONST(value) ((const struct mp_value_fraction_range *)value)
#define MP_VALUE_LIST_CONST(value)           ((const struct mp_value_list *)value)

#define mp_compare(a, b)                                                                           \
	({                                                                                         \
		__typeof__(a) _a = (a);                                                            \
		__typeof__(b) _b = (b);                                                            \
		(_a < _b) ? MP_VALUE_LESS_THAN                                                     \
			  : ((_a > _b) ? MP_VALUE_GREATER_THAN : MP_VALUE_EQUAL);                  \
	})

#define mp_fraction_compare(a_num, a_den, b_num, b_den) \
	({ \
		__typeof__(a_num) _a_num = (a_num); \
		__typeof__(a_den) _a_den = (a_den); \
		__typeof__(b_num) _b_num = (b_num); \
		__typeof__(b_den) _b_den = (b_den); \
		__typeof__(a_num) sign_a_positive = ((_a_num < 0) ^ (_a_den < 0)); \
		__typeof__(b_num) sign_b_positive = ((_b_num < 0) ^ (_b_den < 0)); \
		_Generic((_a_num), \
			int32_t : ((sign_a_positive != sign_b_positive) \
					  ? (sign_a_positive ? MP_VALUE_LESS_THAN \
							     : MP_VALUE_GREATER_THAN) \
					  : mp_compare((int64_t)_a_num * (int64_t)_b_den, \
						       (int64_t)_b_num * (int64_t)_a_den)), \
			uint32_t : mp_compare((uint64_t)_a_num * (uint64_t)_b_den, \
					     (uint64_t)_b_num * (uint64_t)_a_den)); \
	})

#define MP_VALUE_RANGES_OVERLAP(ref_val, cmp_val, vtype)                                           \
	!(MP_VALUE_RANGE(ref_val)->min.vtype > MP_VALUE_RANGE(cmp_val)->max.vtype ||               \
	  MP_VALUE_RANGE(cmp_val)->min.vtype > MP_VALUE_RANGE(ref_val)->max.vtype)

#define MP_VALUE_CREATE_INTERSECT_RANGE(ref_val, cmp_val, vtype, type_enum)                        \
	MP_VALUE_RANGES_OVERLAP(ref_val, cmp_val, vtype)                                           \
	? mp_value_new(                                                                            \
		  type_enum,                                                                       \
		  MAX(MP_VALUE_RANGE(ref_val)->min.vtype, MP_VALUE_RANGE(cmp_val)->min.vtype),     \
		  MIN(MP_VALUE_RANGE(ref_val)->max.vtype, MP_VALUE_RANGE(cmp_val)->max.vtype),     \
		  sys_gcd(MP_VALUE_RANGE(ref_val)->step.vtype,                                     \
			  MP_VALUE_RANGE(compare_val)->step.vtype),                                \
		  NULL)                                                                            \
	: NULL

#define MP_SINGLE_VALUE_IN_RANGE(ref_val, cmp_val, vtype)                                          \
	(IN_RANGE(MP_VALUE_SIMPLE(cmp_val)->vtype, MP_VALUE_RANGE(ref_val)->min.vtype,             \
		  MP_VALUE_RANGE(ref_val)->max.vtype))

struct mp_value_simple {
	struct mp_value base;
	union {
		bool v_boolean;
		int32_t v_int;
		uint32_t v_uint;
		const char *v_cstring;
		struct mp_object *v_obj;
		void *v_ptr;
	};
};

struct mp_value_list {
	struct mp_value base;
	sys_slist_t v_list;
};

struct mp_value_range {
	struct mp_value base;
	union {
		int32_t v_int;
		uint32_t v_uint;
	} min, max, step;
};

struct mp_value_fraction {
	struct mp_value base;
	union {
		int32_t v_int;
		uint32_t v_uint;
	} num, denom;
};

struct mp_value_fraction_range {
	struct mp_value base;
	struct mp_value_fraction min, max, step;
};

struct mp_value_node {
	struct mp_value *value;
	sys_snode_t node;
};

static const size_t mp_value_type_sizes[MP_TYPE_COUNT] = {
	[MP_TYPE_NONE] = sizeof(struct mp_value_simple),
	[MP_TYPE_BOOLEAN] = sizeof(struct mp_value_simple),
	[MP_TYPE_ENUM] = sizeof(struct mp_value_simple),
	[MP_TYPE_INT] = sizeof(struct mp_value_simple),
	[MP_TYPE_UINT] = sizeof(struct mp_value_simple),
	[MP_TYPE_UINT_FRACTION] = sizeof(struct mp_value_fraction),
	[MP_TYPE_INT_FRACTION] = sizeof(struct mp_value_fraction),
	[MP_TYPE_STRING] = sizeof(struct mp_value_simple),
	[MP_TYPE_INT_RANGE] = sizeof(struct mp_value_range),
	[MP_TYPE_UINT_RANGE] = sizeof(struct mp_value_range),
	[MP_TYPE_INT_FRACTION_RANGE] = sizeof(struct mp_value_fraction_range),
	[MP_TYPE_UINT_FRACTION_RANGE] = sizeof(struct mp_value_fraction_range),
	[MP_TYPE_LIST] = sizeof(struct mp_value_list),
	[MP_TYPE_OBJECT] = sizeof(struct mp_value_simple),
	[MP_TYPE_PTR] = sizeof(struct mp_value_simple),
};

static const uint32_t mp_value_intersect_mask[MP_TYPE_COUNT] = {
	[MP_TYPE_NONE] = 0,
	[MP_TYPE_BOOLEAN] = BIT(MP_TYPE_BOOLEAN) | BIT(MP_TYPE_LIST),
	[MP_TYPE_ENUM] = BIT(MP_TYPE_ENUM) | BIT(MP_TYPE_LIST),
	[MP_TYPE_INT] = BIT(MP_TYPE_INT) | BIT(MP_TYPE_INT_RANGE) | BIT(MP_TYPE_LIST),
	[MP_TYPE_UINT] = BIT(MP_TYPE_UINT) | BIT(MP_TYPE_UINT_RANGE) | BIT(MP_TYPE_LIST),
	[MP_TYPE_UINT_FRACTION] =
		BIT(MP_TYPE_UINT_FRACTION) | BIT(MP_TYPE_UINT_FRACTION_RANGE) | BIT(MP_TYPE_LIST),
	[MP_TYPE_INT_FRACTION] =
		BIT(MP_TYPE_INT_FRACTION) | BIT(MP_TYPE_INT_FRACTION_RANGE) | BIT(MP_TYPE_LIST),
	[MP_TYPE_STRING] = BIT(MP_TYPE_STRING) | BIT(MP_TYPE_LIST),
	[MP_TYPE_INT_RANGE] = BIT(MP_TYPE_INT) | BIT(MP_TYPE_INT_RANGE) | BIT(MP_TYPE_LIST),
	[MP_TYPE_UINT_RANGE] = BIT(MP_TYPE_UINT) | BIT(MP_TYPE_UINT_RANGE) | BIT(MP_TYPE_LIST),
	[MP_TYPE_UINT_FRACTION_RANGE] =
		BIT(MP_TYPE_UINT_FRACTION) | BIT(MP_TYPE_UINT_FRACTION_RANGE) | BIT(MP_TYPE_LIST),
	[MP_TYPE_INT_FRACTION_RANGE] =
		BIT(MP_TYPE_INT_FRACTION) | BIT(MP_TYPE_INT_FRACTION_RANGE) | BIT(MP_TYPE_LIST),
	[MP_TYPE_LIST] = BIT(MP_TYPE_BOOLEAN) | BIT(MP_TYPE_ENUM) | BIT(MP_TYPE_INT) |
			 BIT(MP_TYPE_UINT) | BIT(MP_TYPE_UINT_FRACTION) |
			 BIT(MP_TYPE_INT_FRACTION) | BIT(MP_TYPE_STRING) | BIT(MP_TYPE_INT_RANGE) |
			 BIT(MP_TYPE_UINT_RANGE) | BIT(MP_TYPE_UINT_FRACTION_RANGE) |
			 BIT(MP_TYPE_INT_FRACTION_RANGE) | BIT(MP_TYPE_LIST),
	[MP_TYPE_OBJECT] = 0,
	[MP_TYPE_PTR] = 0,
};

bool mp_value_is_primitive(const struct mp_value *value)
{
	if (value == NULL || !IN_RANGE(value->type, MP_TYPE_NONE + 1, MP_TYPE_COUNT - 1)) {
		return false;
	}

	return ((BIT(MP_TYPE_BOOLEAN) | BIT(MP_TYPE_ENUM) | BIT(MP_TYPE_INT) | BIT(MP_TYPE_UINT) |
		 BIT(MP_TYPE_INT_FRACTION) | BIT(MP_TYPE_UINT_FRACTION) | BIT(MP_TYPE_STRING)) &
		BIT(value->type)) != 0;
}

static void mp_value_set_range(struct mp_value *value, int type, va_list *args)
{
	value->type = type;
	MP_VALUE_RANGE(value)->min.v_uint = va_arg(*args, uint32_t);
	MP_VALUE_RANGE(value)->max.v_uint = va_arg(*args, uint32_t);
	MP_VALUE_RANGE(value)->step.v_uint = va_arg(*args, uint32_t);
}

static void mp_value_set_fraction(struct mp_value *value, int type, va_list *args)
{
	uint32_t gcd = 1;

	value->type = type;

	MP_VALUE_FRACTION(value)->num.v_uint = va_arg(*args, uint32_t);
	MP_VALUE_FRACTION(value)->denom.v_uint = va_arg(*args, uint32_t);
	__ASSERT_NO_MSG(MP_VALUE_FRACTION(value)->denom.v_uint != 0);
	if (type == MP_TYPE_UINT_FRACTION) {
		gcd = sys_gcd(MP_VALUE_FRACTION(value)->num.v_int,
			      MP_VALUE_FRACTION(value)->denom.v_int);
	} else {
		gcd = sys_gcd(MP_VALUE_FRACTION(value)->num.v_uint,
			      MP_VALUE_FRACTION(value)->denom.v_uint);
	}

	MP_VALUE_FRACTION(value)->num.v_uint /= gcd;
	MP_VALUE_FRACTION(value)->denom.v_uint /= gcd;
}

static void mp_value_set_fraction_range(struct mp_value *value, int type, va_list *args)
{
	int base_type = (type == MP_TYPE_UINT_FRACTION_RANGE) ? MP_TYPE_UINT_FRACTION
							      : MP_TYPE_INT_FRACTION;

	value->type = type;
	mp_value_set_fraction(MP_VALUE(&MP_VALUE_FRACTION_RANGE(value)->min), base_type, args);
	mp_value_set_fraction(MP_VALUE(&MP_VALUE_FRACTION_RANGE(value)->max), base_type, args);
	mp_value_set_fraction(MP_VALUE(&MP_VALUE_FRACTION_RANGE(value)->step), base_type, args);
}

static void mp_value_set_list(struct mp_value *value, va_list *args)
{
	struct mp_value *list_item;

	while ((list_item = va_arg(*args, struct mp_value *)) != NULL) {
		mp_value_list_append(value, list_item);
	}
}

static void mp_value_set_va_list(struct mp_value *value, int type, va_list *args)
{
	if (value == NULL) {
		return;
	}

	value->type = type;
	switch (value->type) {
	case MP_TYPE_BOOLEAN:
	case MP_TYPE_ENUM:
	case MP_TYPE_INT:
		MP_VALUE_SIMPLE(value)->v_int = va_arg(*args, int);
		break;
	case MP_TYPE_STRING:
		MP_VALUE_SIMPLE(value)->v_cstring = va_arg(*args, const char *);
		break;
	case MP_TYPE_UINT:
		MP_VALUE_SIMPLE(value)->v_uint = va_arg(*args, uint32_t);
		break;
	case MP_TYPE_OBJECT:
		mp_object_replace(&MP_VALUE_SIMPLE(value)->v_obj,
				  va_arg(*args, struct mp_object *));
		break;
	case MP_TYPE_PTR:
		MP_VALUE_SIMPLE(value)->v_ptr = va_arg(*args, void *);
		break;
	case MP_TYPE_UINT_FRACTION:
	case MP_TYPE_INT_FRACTION:
		mp_value_set_fraction(value, type, args);
		break;
	case MP_TYPE_INT_RANGE:
	case MP_TYPE_UINT_RANGE:
		mp_value_set_range(value, type, args);
		break;
		break;

	case MP_TYPE_UINT_FRACTION_RANGE:
	case MP_TYPE_INT_FRACTION_RANGE:
		mp_value_set_fraction_range(value, type, args);
		break;
	case MP_TYPE_LIST:
		mp_value_set_list(value, args);
		break;
	default:
		break;
	}
}

void mp_value_set(struct mp_value *value, int type, ...)
{
	va_list args;

	va_start(args, type);
	mp_value_set_va_list(value, type, &args);
	va_end(args);
}

int mp_value_get_fraction_numerator(const struct mp_value *frac)
{
	return MP_VALUE_FRACTION_CONST(frac)->num.v_int;
}

int mp_value_get_fraction_denominator(const struct mp_value *frac)
{
	return MP_VALUE_FRACTION_CONST(frac)->denom.v_int;
}

const struct mp_value *mp_value_get_fraction_range_min(const struct mp_value *fraction_range)
{
	return MP_VALUE_CONST(&MP_VALUE_FRACTION_RANGE_CONST(fraction_range)->min);
}

const struct mp_value *mp_value_get_fraction_range_max(const struct mp_value *fraction_range)
{
	return MP_VALUE_CONST(&MP_VALUE_FRACTION_RANGE_CONST(fraction_range)->max);
}

const struct mp_value *mp_value_get_fraction_range_step(const struct mp_value *fraction_range)
{
	return MP_VALUE_CONST(&MP_VALUE_FRACTION_RANGE_CONST(fraction_range)->step);
}

const char *mp_value_get_string(const struct mp_value *value)
{
	return MP_VALUE_SIMPLE_CONST(value)->v_cstring;
}

int mp_value_get_int(const struct mp_value *value)
{
	return MP_VALUE_SIMPLE_CONST(value)->v_int;
}

uint32_t mp_value_get_uint(const struct mp_value *value)
{
	return MP_VALUE_SIMPLE_CONST(value)->v_uint;
}

void *mp_value_get_ptr(const struct mp_value *value)
{
	return value ? MP_VALUE_SIMPLE_CONST(value)->v_ptr : NULL;
}

bool mp_value_get_boolean(const struct mp_value *value)
{
	return MP_VALUE_SIMPLE_CONST(value)->v_boolean;
}

struct mp_value *mp_value_new_empty(enum mp_value_type type)
{
	struct mp_value *value;

	if (type >= MP_TYPE_COUNT || type < MP_TYPE_NONE) {
		LOG_ERR("Invalid value type: %d", type);
		return NULL;
	}

	value = k_calloc(1, mp_value_type_sizes[type]);
	if (value == NULL) {
		LOG_ERR("Failed to allocate memory for mp_value type %d", type);
		return NULL;
	}

	value->type = type;
	if (value->type == MP_TYPE_LIST) {
		sys_slist_init(&MP_VALUE_LIST(value)->v_list);
	}

	return value;
}

void mp_value_destroy(struct mp_value *value)
{
	struct mp_value_node *value_node;

	if (value->type == MP_TYPE_LIST) {
		while (!sys_slist_is_empty(&MP_VALUE_LIST(value)->v_list)) {
			value_node = CONTAINER_OF(sys_slist_get(&MP_VALUE_LIST(value)->v_list),
						  struct mp_value_node, node);
			mp_value_destroy(value_node->value);
			k_free(value_node);
		}
	}

	if (value->type == MP_TYPE_OBJECT) {
		mp_object_unref(MP_VALUE_SIMPLE(value)->v_obj);
	}

	k_free(value);
}

struct mp_value *mp_value_new(enum mp_value_type type, ...)
{

	struct mp_value *value;
	va_list args;

	va_start(args, type);

	value = mp_value_new_va_list(type, &args);

	va_end(args);

	return value;
}

struct mp_value *mp_value_new_va_list(enum mp_value_type type, va_list *args)
{
	struct mp_value *value = mp_value_new_empty(type);

	mp_value_set_va_list(value, type, args);

	return value;
}

static void mp_value_copy(struct mp_value *dst, const struct mp_value *src)
{
	if (src->type == MP_TYPE_LIST) {
		struct mp_value_node *v_node;

		SYS_SLIST_FOR_EACH_CONTAINER(&MP_VALUE_LIST(src)->v_list, v_node, node) {
			mp_value_list_append(dst, mp_value_duplicate(v_node->value));
		}
	} else if (src->type == MP_TYPE_OBJECT) {
		MP_VALUE_SIMPLE(dst)->v_obj = MP_VALUE_SIMPLE(src)->v_obj;
		mp_object_ref(MP_VALUE_SIMPLE(dst)->v_obj);
	} else {
		memcpy(dst, src, mp_value_type_sizes[src->type]);
	}
}

struct mp_value *mp_value_duplicate(const struct mp_value *value)
{
	struct mp_value *dup_value = mp_value_new_empty(value->type);

	if (dup_value == NULL) {
		return NULL;
	}

	mp_value_copy(dup_value, value);

	return dup_value;
}

void mp_value_list_append(struct mp_value *list, struct mp_value *append_value)
{
	struct mp_value_node *node;

	__ASSERT_NO_MSG(append_value != NULL && list != NULL);
	node = k_malloc(sizeof(struct mp_value_node));
	__ASSERT_NO_MSG(node != NULL);
	node->value = append_value;
	sys_slist_append(&MP_VALUE_LIST(list)->v_list, &node->node);
}

struct mp_value *mp_value_list_get(const struct mp_value *list, int index)
{
	sys_snode_t *node;
	struct mp_value_node *value_node = NULL;
	int count = 0;

	SYS_SLIST_FOR_EACH_NODE(&MP_VALUE_LIST_CONST(list)->v_list, node) {
		if (count++ == index) {
			value_node = CONTAINER_OF(node, struct mp_value_node, node);
			break;
		}
	}

	return value_node ? value_node->value : NULL;
}

bool mp_value_list_is_empty(const struct mp_value *list)
{
	return sys_slist_is_empty(&MP_VALUE_LIST_CONST(list)->v_list);
}

size_t mp_value_list_get_size(const struct mp_value *list)
{
	return sys_slist_len(&MP_VALUE_LIST_CONST(list)->v_list);
}

int mp_value_get_int_range_min(const struct mp_value *range)
{
	return MP_VALUE_RANGE_CONST(range)->min.v_int;
}

int mp_value_get_int_range_max(const struct mp_value *range)
{
	return MP_VALUE_RANGE_CONST(range)->max.v_int;
}

int mp_value_get_int_range_step(const struct mp_value *range)
{
	return MP_VALUE_RANGE_CONST(range)->step.v_int;
}

uint32_t mp_value_get_uint_range_min(const struct mp_value *range)
{
	return MP_VALUE_RANGE_CONST(range)->min.v_uint;
}

uint32_t mp_value_get_uint_range_max(const struct mp_value *range)
{
	return MP_VALUE_RANGE_CONST(range)->max.v_uint;
}

uint32_t mp_value_get_uint_range_step(const struct mp_value *range)
{
	return MP_VALUE_RANGE_CONST(range)->step.v_uint;
}

struct mp_object *mp_value_get_object(struct mp_value *value)
{
	return value ? MP_VALUE_SIMPLE_CONST(value)->v_obj : NULL;
}

int mp_value_compare_fraction(const struct mp_value *frac1, const struct mp_value *frac2)
{
	if (frac1->type == MP_TYPE_INT_FRACTION && frac2->type == MP_TYPE_INT_FRACTION) {
		return mp_fraction_compare(
			MP_VALUE_FRACTION(frac1)->num.v_int, MP_VALUE_FRACTION(frac1)->denom.v_int,
			MP_VALUE_FRACTION(frac2)->num.v_int, MP_VALUE_FRACTION(frac2)->denom.v_int);
	}

	if (frac1->type == MP_TYPE_UINT_FRACTION && frac2->type == MP_TYPE_UINT_FRACTION) {
		return mp_fraction_compare(MP_VALUE_FRACTION(frac1)->num.v_uint,
					   MP_VALUE_FRACTION(frac1)->denom.v_uint,
					   MP_VALUE_FRACTION(frac2)->num.v_uint,
					   MP_VALUE_FRACTION(frac2)->denom.v_uint);
	}

	return MP_VALUE_COMPARE_FAILED;
}

static int mp_value_list_compare(const struct mp_value *list1, const struct mp_value *list2);

int mp_value_compare(const struct mp_value *val1, const struct mp_value *val2)
{
	bool is_equal;

	if (val1->type != val2->type) {
		return MP_VALUE_COMPARE_FAILED;
	}

	switch (val1->type) {
	case MP_TYPE_BOOLEAN:
	case MP_TYPE_ENUM:
		return MP_VALUE_SIMPLE_CONST(val1)->v_uint == MP_VALUE_SIMPLE_CONST(val2)->v_uint
			       ? MP_VALUE_EQUAL
			       : MP_VALUE_UNORDERED;
	case MP_TYPE_INT:
		return mp_compare(MP_VALUE_SIMPLE_CONST(val1)->v_int,
				  MP_VALUE_SIMPLE_CONST(val2)->v_int);
	case MP_TYPE_UINT:
		return mp_compare(MP_VALUE_SIMPLE_CONST(val1)->v_uint,
				  MP_VALUE_SIMPLE_CONST(val2)->v_uint);
	case MP_TYPE_UINT_FRACTION:
	case MP_TYPE_INT_FRACTION:
		return mp_value_compare_fraction(val1, val2);
	case MP_TYPE_STRING:
		return strcmp(MP_VALUE_SIMPLE_CONST(val1)->v_cstring,
			      MP_VALUE_SIMPLE_CONST(val2)->v_cstring) == 0
			       ? MP_VALUE_EQUAL
			       : MP_VALUE_UNORDERED;
	case MP_TYPE_UINT_RANGE:
	case MP_TYPE_INT_RANGE:
		is_equal = (MP_VALUE_RANGE_CONST(val1)->min.v_uint ==
				    MP_VALUE_RANGE_CONST(val2)->min.v_uint &&
			    MP_VALUE_RANGE_CONST(val1)->max.v_uint ==
				    MP_VALUE_RANGE_CONST(val2)->max.v_uint &&
			    MP_VALUE_RANGE_CONST(val1)->step.v_uint ==
				    MP_VALUE_RANGE_CONST(val2)->step.v_uint);

		return is_equal ? MP_VALUE_EQUAL : MP_VALUE_UNORDERED;
	case MP_TYPE_INT_FRACTION_RANGE:
	case MP_TYPE_UINT_FRACTION_RANGE:
		is_equal = mp_value_compare_fraction(
				   MP_VALUE_CONST(&MP_VALUE_FRACTION_RANGE_CONST(val1)->min),
				   MP_VALUE_CONST(&MP_VALUE_FRACTION_RANGE_CONST(val2)->min)) ==
				   MP_VALUE_EQUAL &&
			   mp_value_compare_fraction(
				   MP_VALUE_CONST(&MP_VALUE_FRACTION_RANGE_CONST(val1)->max),
				   MP_VALUE_CONST(&MP_VALUE_FRACTION_RANGE_CONST(val2)->max)) ==
				   MP_VALUE_EQUAL &&
			   mp_value_compare_fraction(
				   MP_VALUE_CONST(&MP_VALUE_FRACTION_RANGE_CONST(val1)->step),
				   MP_VALUE_CONST(&MP_VALUE_FRACTION_RANGE_CONST(val2)->step)) ==
				   MP_VALUE_EQUAL;

		return is_equal ? MP_VALUE_EQUAL : MP_VALUE_UNORDERED;
	case MP_TYPE_LIST:
		return mp_value_list_compare(val1, val2);
	default:
		return MP_VALUE_COMPARE_FAILED;
	}
}

static int mp_value_list_compare(const struct mp_value *list1, const struct mp_value *list2)
{
	int size1 = mp_value_list_get_size(list1);
	int size2 = mp_value_list_get_size(list1);
	int count_matched = 0;
	struct mp_value_node *v_node1, *v_node2;

	if (list1->type != MP_TYPE_LIST || list2->type != MP_TYPE_LIST) {
		return MP_VALUE_COMPARE_FAILED;
	}

	if (size1 != size2) {
		return MP_VALUE_UNORDERED;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&MP_VALUE_LIST(list1)->v_list, v_node1, node) {
		SYS_SLIST_FOR_EACH_CONTAINER(&MP_VALUE_LIST(list2)->v_list, v_node2, node) {
			if (mp_value_compare(v_node1->value, v_node2->value) == MP_VALUE_EQUAL) {
				count_matched++;
			}
		}
	}

	return count_matched == size1 ? MP_VALUE_EQUAL : MP_VALUE_UNORDERED;
}

bool mp_value_can_intersect(const struct mp_value *val1, const struct mp_value *val2)
{
	if (val1 == NULL || val2 == NULL ||
	    !IN_RANGE(val1->type, MP_TYPE_NONE, MP_TYPE_COUNT - 1)) {
		return false;
	}

	return (mp_value_intersect_mask[val1->type] & BIT(val2->type)) != 0;
}

struct mp_value *mp_value_intersect_int_range(const struct mp_value *ref_val,
					      const struct mp_value *compare_val)
{
	struct mp_value *intersect_value;

	if (compare_val->type == MP_TYPE_INT_RANGE && ref_val->type == MP_TYPE_INT_RANGE) {
		intersect_value = MP_VALUE_CREATE_INTERSECT_RANGE(ref_val, compare_val, v_int,
								  MP_TYPE_INT_RANGE);
	} else if (compare_val->type == MP_TYPE_UINT_RANGE && ref_val->type == MP_TYPE_UINT_RANGE) {
		intersect_value = MP_VALUE_CREATE_INTERSECT_RANGE(ref_val, compare_val, v_uint,
								  MP_TYPE_UINT_RANGE);
	} else if ((ref_val->type == MP_TYPE_INT_RANGE && compare_val->type == MP_TYPE_INT &&
		    MP_SINGLE_VALUE_IN_RANGE(ref_val, compare_val, v_int)) ||
		   (ref_val->type == MP_TYPE_UINT_RANGE && compare_val->type == MP_TYPE_UINT &&
		    MP_SINGLE_VALUE_IN_RANGE(ref_val, compare_val, v_uint))) {
		intersect_value =
			mp_value_new(compare_val->type, MP_VALUE_SIMPLE(compare_val)->v_uint, NULL);
	} else {
		intersect_value = NULL;
	}

	return intersect_value;
}

/**
 * Find the min or max value between two primitive values
 * @param value1 the first value
 * @param value2 the second value
 * @param find_min true if find min, false if find max
 * @return the min or max value between two values
 */
static const struct mp_value *mp_value_min_max(const struct mp_value *value1,
					       const struct mp_value *value2, bool find_min)
{
	switch (mp_value_compare(value1, value2)) {
	case MP_VALUE_LESS_THAN:
		return find_min ? value1 : value2;
	case MP_VALUE_EQUAL:
		return value1;
	case MP_VALUE_GREATER_THAN:
		return find_min ? value2 : value1;
	default:
		return NULL;
	}
}

#define MP_VALUE_FRACTION_RANGES_OVERLAP(ref_val, cmp_val)                                         \
	!(mp_value_compare_fraction(MP_VALUE(&MP_VALUE_FRACTION_RANGE(ref_val)->min),              \
				    MP_VALUE(&MP_VALUE_FRACTION_RANGE(cmp_val)->max)) ==           \
		  MP_VALUE_GREATER_THAN ||                                                         \
	  mp_value_compare_fraction(MP_VALUE(&MP_VALUE_FRACTION_RANGE(ref_val)->max),              \
				    MP_VALUE(&MP_VALUE_FRACTION_RANGE(cmp_val)->min)) ==           \
		  MP_VALUE_LESS_THAN)

#define MP_VALUE_FRACTION_IN_RANGE(frac_val, range_val)                                            \
	!(mp_value_compare_fraction(frac_val,                                                      \
				    MP_VALUE(&MP_VALUE_FRACTION_RANGE(range_val)->min)) ==         \
		  MP_VALUE_LESS_THAN ||                                                            \
	  mp_value_compare_fraction(frac_val,                                                      \
				    MP_VALUE(&MP_VALUE_FRACTION_RANGE(range_val)->max)) ==         \
		  MP_VALUE_GREATER_THAN)

struct mp_value *mp_value_intersect_fraction_range(const struct mp_value *ref_val,
						   const struct mp_value *compare_val)
{
	struct mp_value *intersect_value;
	int f_type = MP_VALUE(&MP_VALUE_FRACTION_RANGE(ref_val)->step)->type;

	if ((compare_val->type == MP_TYPE_UINT_FRACTION_RANGE ||
	     compare_val->type == MP_TYPE_INT_FRACTION_RANGE) &&
	    MP_VALUE_FRACTION_RANGES_OVERLAP(ref_val, compare_val)) {
		intersect_value = mp_value_new_empty(ref_val->type);
		MP_VALUE_FRACTION_RANGE(intersect_value)->min = *MP_VALUE_FRACTION(mp_value_min_max(
			MP_VALUE(&MP_VALUE_FRACTION_RANGE(ref_val)->min),
			MP_VALUE(&MP_VALUE_FRACTION_RANGE(compare_val)->min), false));
		MP_VALUE_FRACTION_RANGE(intersect_value)->max = *MP_VALUE_FRACTION(mp_value_min_max(
			MP_VALUE(&MP_VALUE_FRACTION_RANGE(ref_val)->max),
			MP_VALUE(&MP_VALUE_FRACTION_RANGE(compare_val)->max), true));
		if (f_type == MP_TYPE_INT_FRACTION) {
			mp_value_set(
				MP_VALUE(&MP_VALUE_FRACTION_RANGE(intersect_value)->step), f_type,
				sys_gcd(MP_VALUE_FRACTION_RANGE(ref_val)->step.num.v_int,
					MP_VALUE_FRACTION_RANGE(compare_val)->step.num.v_int),
				sys_lcm(MP_VALUE_FRACTION_RANGE(ref_val)->step.denom.v_int,
					MP_VALUE_FRACTION_RANGE(compare_val)->step.denom.v_int));
		} else {
			mp_value_set(
				MP_VALUE(&MP_VALUE_FRACTION_RANGE(intersect_value)->step), f_type,
				sys_gcd(MP_VALUE_FRACTION_RANGE(ref_val)->step.num.v_uint,
					MP_VALUE_FRACTION_RANGE(compare_val)->step.num.v_uint),
				sys_lcm(MP_VALUE_FRACTION_RANGE(ref_val)->step.denom.v_uint,
					MP_VALUE_FRACTION_RANGE(compare_val)->step.denom.v_uint));
		}
	} else if ((compare_val->type == MP_TYPE_INT_FRACTION ||
		    compare_val->type == MP_TYPE_UINT_FRACTION) &&
		   MP_VALUE_FRACTION_IN_RANGE(compare_val, ref_val)) {
		intersect_value = mp_value_new(f_type, MP_VALUE_FRACTION(compare_val)->num.v_uint,
					       MP_VALUE_FRACTION(compare_val)->denom.v_uint, NULL);
	} else {
		printk("%d\n",
		       mp_value_compare_fraction(
			       compare_val, MP_VALUE(&MP_VALUE_FRACTION_RANGE(ref_val)->min)) ==
			       MP_VALUE_LESS_THAN);
		printk("%d\n",
		       mp_value_compare_fraction(
			       compare_val, MP_VALUE(&MP_VALUE_FRACTION_RANGE(ref_val)->max)) ==
			       MP_VALUE_GREATER_THAN);
		printk("Failed to intersect fraction ranges %d %d", compare_val->type,
		       ref_val->type);
		intersect_value = NULL;
	}

	return intersect_value;
}

struct mp_value *mp_value_intersect_list(const struct mp_value *list,
					 const struct mp_value *compare_val)
{
	struct mp_value *intersect_value = NULL;
	struct mp_value *intersect_list = NULL;
	struct mp_value_node *v_node1, *v_node2;

	if (list == NULL || compare_val == NULL || compare_val->type == MP_TYPE_NONE) {
		return NULL;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&MP_VALUE_LIST_CONST(list)->v_list, v_node1, node) {
		intersect_value = NULL;
		switch (compare_val->type) {
		case MP_TYPE_BOOLEAN:
		case MP_TYPE_ENUM:
		case MP_TYPE_INT:
		case MP_TYPE_UINT:
		case MP_TYPE_UINT_FRACTION:
		case MP_TYPE_INT_FRACTION:
		case MP_TYPE_STRING:
			if (mp_value_compare(compare_val, v_node1->value) == MP_VALUE_EQUAL) {
				intersect_value = mp_value_duplicate(compare_val);
			}
			break;
		case MP_TYPE_INT_RANGE:
		case MP_TYPE_UINT_RANGE:
			intersect_value = mp_value_intersect_int_range(compare_val, v_node1->value);
			break;
		case MP_TYPE_UINT_FRACTION_RANGE:
		case MP_TYPE_INT_FRACTION_RANGE:
			intersect_value =
				mp_value_intersect_fraction_range(compare_val, v_node1->value);
			break;
		case MP_TYPE_LIST:
			SYS_SLIST_FOR_EACH_CONTAINER(&MP_VALUE_LIST_CONST(compare_val)->v_list,
						     v_node2, node) {
				if (mp_value_compare(v_node1->value, v_node2->value) ==
				    MP_VALUE_EQUAL) {
					intersect_value = mp_value_duplicate(v_node2->value);
					break;
				}
			}
			break;
		default:
			break;
		}

		if (intersect_value != NULL) {
			if (intersect_list == NULL) {
				intersect_list = mp_value_new_empty(MP_TYPE_LIST);
			}
			mp_value_list_append(intersect_list, intersect_value);
		}
	}

	return intersect_list;
}

struct mp_value *mp_value_intersect(const struct mp_value *val1, const struct mp_value *val2)
{
	const struct mp_value *ref_val, *compare_val;
	struct mp_value *intersect_val = NULL;

	/* Check if intersect */
	if (!mp_value_can_intersect(val1, val2)) {
		return NULL;
	}

	/* When two values don't have the same type */
	if (val1->type >= val2->type) {
		ref_val = val1;
		compare_val = val2;
	} else {
		ref_val = val2;
		compare_val = val1;
	}

	if (mp_value_is_primitive(ref_val)) {
		if (mp_value_compare(val1, val2) == MP_VALUE_EQUAL) {
			intersect_val = mp_value_duplicate(val1);
		}
	} else {
		switch (ref_val->type) {
		case MP_TYPE_INT_RANGE:
		case MP_TYPE_UINT_RANGE:
			intersect_val = mp_value_intersect_int_range(ref_val, compare_val);
			break;
		case MP_TYPE_INT_FRACTION_RANGE:
		case MP_TYPE_UINT_FRACTION_RANGE:
			intersect_val = mp_value_intersect_fraction_range(ref_val, compare_val);
			break;
		case MP_TYPE_LIST:
			intersect_val = mp_value_intersect_list(ref_val, compare_val);
			break;
		default:
			break;
		}
	}

	return intersect_val;
}

static inline void mp_value_print_int(const struct mp_value *value)
{
	printk("%d", MP_VALUE_SIMPLE_CONST(value)->v_int);
}

static inline void mp_value_print_uint(const struct mp_value *value)
{
	printk("%u", MP_VALUE_SIMPLE_CONST(value)->v_uint);
}

static inline void mp_value_print_string(const struct mp_value *value)
{
	printk("%s", MP_VALUE_SIMPLE_CONST(value)->v_cstring);
}

static inline void mp_value_print_int_range(const struct mp_value *value)
{
	printk("[%d, %d, %d]", MP_VALUE_RANGE_CONST(value)->min.v_int,
	       MP_VALUE_RANGE_CONST(value)->max.v_int, MP_VALUE_RANGE_CONST(value)->step.v_int);
}

static inline void mp_value_print_uint_range(const struct mp_value *value)
{
	printk("[%u, %u, %u]", MP_VALUE_RANGE_CONST(value)->min.v_uint,
	       MP_VALUE_RANGE_CONST(value)->max.v_uint, MP_VALUE_RANGE_CONST(value)->step.v_uint);
}

static inline void mp_value_print_fraction(const struct mp_value *value)
{
	if (value->type == MP_TYPE_UINT_FRACTION) {
		printk("%u/%u", MP_VALUE_FRACTION_CONST(value)->num.v_uint,
		       MP_VALUE_FRACTION_CONST(value)->denom.v_uint);
	} else {
		printk("%d/%d", MP_VALUE_FRACTION_CONST(value)->num.v_int,
		       MP_VALUE_FRACTION_CONST(value)->denom.v_int);
	}
}

static inline void mp_value_print_fraction_range(const struct mp_value *value)
{
	printk("[");
	mp_value_print_fraction(MP_VALUE_CONST(&MP_VALUE_FRACTION_RANGE_CONST(value)->min));
	printk(",");
	mp_value_print_fraction(MP_VALUE_CONST(&MP_VALUE_FRACTION_RANGE_CONST(value)->max));
	printk(",");
	mp_value_print_fraction(MP_VALUE_CONST(&MP_VALUE_FRACTION_RANGE_CONST(value)->step));
	printk("]");
}

static inline void mp_value_print_list(const struct mp_value *value)
{
	struct mp_value_node *value_node;

	printk("{");
	SYS_SLIST_FOR_EACH_CONTAINER(&MP_VALUE_LIST_CONST(value)->v_list, value_node, node) {
		mp_value_print(value_node->value, false);
		if (sys_slist_peek_next(&value_node->node) != NULL) {

			printk(", ");
		}
	}
	printk("}");
}

void mp_value_print(const struct mp_value *value, bool new_line)
{
	typedef void (*mp_value_print_fn)(const struct mp_value *value);
	static const mp_value_print_fn mp_value_print_table[MP_TYPE_COUNT] = {
		[MP_TYPE_NONE] = NULL,
		[MP_TYPE_BOOLEAN] = mp_value_print_int,
		[MP_TYPE_ENUM] = mp_value_print_int,
		[MP_TYPE_INT] = mp_value_print_int,
		[MP_TYPE_UINT] = mp_value_print_uint,
		[MP_TYPE_UINT_FRACTION] = mp_value_print_fraction,
		[MP_TYPE_INT_FRACTION] = mp_value_print_fraction,
		[MP_TYPE_INT_RANGE] = mp_value_print_int_range,
		[MP_TYPE_UINT_RANGE] = mp_value_print_uint_range,
		[MP_TYPE_STRING] = mp_value_print_string,
		[MP_TYPE_LIST] = mp_value_print_list,
		[MP_TYPE_UINT_FRACTION_RANGE] = mp_value_print_fraction_range,
		[MP_TYPE_INT_FRACTION_RANGE] = mp_value_print_fraction_range,
		[MP_TYPE_OBJECT] = NULL,
		[MP_TYPE_PTR] = NULL,
	};

	if (value == NULL || value->type >= ARRAY_SIZE(mp_value_print_table) ||
	    mp_value_print_table[value->type] == NULL) {
		LOG_ERR("Invalid mp_value to print");
		return;
	}

	mp_value_print_fn print_fn = mp_value_print_table[value->type];

	if (print_fn) {
		print_fn(value);
	}

	if (new_line) {
		printk("\n");
	}
}
