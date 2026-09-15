/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Dev It Wise
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Pins the omitted-field contract against a poisoned target: any write to
 * untouched storage fails these tests immediately.
 */

#include <errno.h>
#include <string.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>
#include <zephyr/data/json.h>

#define POISON_BYTE 0xaa

struct omitted_nested {
	int32_t nested_present;
	const char *nested_string;
};

struct omitted_outer {
	int32_t first;
	int32_t second;
	const char *string;
	struct omitted_nested nested;
	int32_t items[4];
	size_t items_count;
};

static const struct json_obj_descr omitted_nested_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct omitted_nested, nested_present, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct omitted_nested, nested_string, JSON_TOK_STRING),
};

static const struct json_obj_descr omitted_outer_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct omitted_outer, first, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct omitted_outer, second, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct omitted_outer, string, JSON_TOK_STRING),
	JSON_OBJ_DESCR_OBJECT(struct omitted_outer, nested, omitted_nested_descr),
	JSON_OBJ_DESCR_ARRAY(struct omitted_outer, items, 4, items_count, JSON_TOK_NUMBER),
};

/* Poisoned target and an untouched twin for comparison. */
static struct omitted_outer target;
static struct omitted_outer pristine;

static void poison_target(void)
{
	memset(&target, POISON_BYTE, sizeof(target));
	memset(&pristine, POISON_BYTE, sizeof(pristine));
}

#define zassert_untouched(field)                                                                   \
	zassert_mem_equal(&target.field, &pristine.field, sizeof(target.field),                    \
			  #field " was written although the document omitted it")

ZTEST(lib_json_omitted_fields, test_omitted_field_keeps_prior_contents)
{
	char encoded[] = "{\"first\":1}";
	int64_t ret;

	poison_target();

	ret = json_obj_parse(encoded, strlen(encoded), omitted_outer_descr,
			     ARRAY_SIZE(omitted_outer_descr), &target);

	zassert_equal(ret, BIT64(0), "Only the first descriptor should be reported decoded");
	zassert_equal(target.first, 1, "Present field was not decoded");
	zassert_untouched(second);
	zassert_untouched(string);
	zassert_untouched(nested);
	zassert_untouched(items);
	zassert_untouched(items_count);
}

/* Bitmap is indexed by descriptor position, not document order. */
ZTEST(lib_json_omitted_fields, test_bitmap_follows_descriptor_order)
{
	char encoded[] = "{\"string\":\"x\",\"first\":2}";
	int64_t ret;

	poison_target();

	ret = json_obj_parse(encoded, strlen(encoded), omitted_outer_descr,
			     ARRAY_SIZE(omitted_outer_descr), &target);

	zassert_equal(ret, BIT64(0) | BIT64(2), "Bitmap does not follow descriptor order");
	zassert_untouched(second);
	zassert_untouched(nested);
}

/* A nested object's bit means only that the object was decoded, not its
 * members; the sub-object's own bitmap is discarded.
 */
ZTEST(lib_json_omitted_fields, test_nested_bit_does_not_cover_nested_members)
{
	char encoded[] = "{\"nested\":{}}";
	int64_t ret;

	poison_target();

	ret = json_obj_parse(encoded, strlen(encoded), omitted_outer_descr,
			     ARRAY_SIZE(omitted_outer_descr), &target);

	zassert_equal(ret, BIT64(3), "Nested object should be reported decoded");
	zassert_untouched(nested.nested_present);
	zassert_untouched(nested.nested_string);
}

ZTEST(lib_json_omitted_fields, test_partial_nested_object_leaves_rest)
{
	char encoded[] = "{\"nested\":{\"nested_present\":7}}";
	int64_t ret;

	poison_target();

	ret = json_obj_parse(encoded, strlen(encoded), omitted_outer_descr,
			     ARRAY_SIZE(omitted_outer_descr), &target);

	zassert_equal(ret, BIT64(3), "Nested object should be reported decoded");
	zassert_equal(target.nested.nested_present, 7, "Present nested field was not decoded");
	zassert_untouched(nested.nested_string);
}

/* Array elements past items_count keep their prior contents. */
ZTEST(lib_json_omitted_fields, test_short_array_leaves_trailing_elements)
{
	char encoded[] = "{\"items\":[10,11]}";
	int64_t ret;

	poison_target();

	ret = json_obj_parse(encoded, strlen(encoded), omitted_outer_descr,
			     ARRAY_SIZE(omitted_outer_descr), &target);

	zassert_equal(ret, BIT64(4), "Array should be reported decoded");
	zassert_equal(target.items_count, 2, "Element count was not decoded");
	zassert_equal(target.items[0], 10, "First element was not decoded");
	zassert_equal(target.items[1], 11, "Second element was not decoded");
	zassert_untouched(items[2]);
	zassert_untouched(items[3]);
}

/* Negative return does not mean untouched: members decoded before the
 * failure are already written.
 */
ZTEST(lib_json_omitted_fields, test_error_return_leaves_target_half_written)
{
	char encoded[] = "{\"first\":5,\"string\":false}";
	int64_t ret;

	poison_target();

	ret = json_obj_parse(encoded, strlen(encoded), omitted_outer_descr,
			     ARRAY_SIZE(omitted_outer_descr), &target);

	zassert_equal(ret, -EINVAL, "Type mismatch should be reported");
	zassert_equal(target.first, 5, "Field decoded before the error was rolled back");
	zassert_untouched(second);
	zassert_untouched(string);
	zassert_untouched(nested);
}

ZTEST_SUITE(lib_json_omitted_fields, NULL, NULL, NULL, NULL, NULL);
