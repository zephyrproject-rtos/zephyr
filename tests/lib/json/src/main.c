/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <string.h>
#include <float.h>
#include <math.h>
#include <zephyr/types.h>
#include <stdbool.h>
#include <zephyr/ztest.h>
#include <zephyr/data/json.h>

struct test_nested {
	int nested_int;
	bool nested_bool;
	const char *nested_string;
	char nested_string_buf[10];
	int8_t nested_int8;
	uint8_t nested_uint8;
	int64_t nested_int64;
	uint64_t nested_uint64;
};

struct test_struct {
	const char *some_string;
	char some_string_buf[10];
	int some_int;
	bool some_bool;
	int16_t some_int16;
	int64_t some_int64;
	int64_t another_int64;
	int64_t some_uint64;
	int64_t another_uint64;
	struct test_nested some_nested_struct;
	int some_array[16];
	size_t some_array_len;
	bool another_bxxl;		 /* JSON field: "another_b!@l" */
	bool if_;			 /* JSON: "if" */
	int another_array[10];		 /* JSON: "another-array" */
	size_t another_array_len;
	struct test_nested xnother_nexx; /* JSON: "4nother_ne$+" */
	struct test_nested nested_obj_array[2];
	size_t obj_array_len;
};

struct elt {
	const char *name;
	const char name_buf[10];
	int height;
};

struct obj_array {
	struct elt elements[10];
	size_t num_elements;
};

struct test_int_limits {
	int int_max;
	int int_cero;
	int int_min;
	int64_t int64_max;
	int64_t int64_cero;
	int64_t int64_min;
	uint64_t uint64_max;
	uint64_t uint64_cero;
	uint64_t uint64_min;
	uint32_t uint32_max;
	uint32_t uint32_cero;
	uint32_t uint32_min;
	int16_t int16_max;
	int16_t int16_cero;
	int16_t int16_min;
	uint16_t uint16_max;
	uint16_t uint16_cero;
	uint16_t uint16_min;
	int8_t int8_max;
	int8_t int8_cero;
	int8_t int8_min;
	uint8_t uint8_max;
	uint8_t uint8_cero;
	uint8_t uint8_min;
};

struct test_float {
	float some_float;
	float another_float;
	float some_array[16];
	size_t some_array_len;
};

struct test_float_limits {
	float float_max;
	float float_cero;
	float float_min;
};

struct test_double {
	double some_double;
	double another_double;
	double some_array[16];
	size_t some_array_len;
};

struct test_double_limits {
	double double_max;
	double double_cero;
	double double_min;
};

struct escape_test_data {
	char string_value[128];
	char string_buf[64];
	int integer_value;
};

static const struct json_obj_descr escape_test_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct escape_test_data, string_value, JSON_TOK_STRING_BUF),
	JSON_OBJ_DESCR_PRIM(struct escape_test_data, string_buf, JSON_TOK_STRING_BUF),
	JSON_OBJ_DESCR_PRIM(struct escape_test_data, integer_value, JSON_TOK_NUMBER),
};

static const struct json_obj_descr nested_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct test_nested, nested_int, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct test_nested, nested_bool, JSON_TOK_TRUE),
	JSON_OBJ_DESCR_PRIM(struct test_nested, nested_string,
			    JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct test_nested, nested_string_buf,
			    JSON_TOK_STRING_BUF),
	JSON_OBJ_DESCR_PRIM(struct test_nested, nested_int8, JSON_TOK_INT),
	JSON_OBJ_DESCR_PRIM(struct test_nested, nested_uint8, JSON_TOK_UINT),
	JSON_OBJ_DESCR_PRIM(struct test_nested, nested_int64,
			    JSON_TOK_INT64),
	JSON_OBJ_DESCR_PRIM(struct test_nested, nested_uint64,
			    JSON_TOK_UINT64),
};

static const struct json_obj_descr test_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct test_struct, some_string, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct test_struct, some_string_buf, JSON_TOK_STRING_BUF),
	JSON_OBJ_DESCR_PRIM(struct test_struct, some_int, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct test_struct, some_bool, JSON_TOK_TRUE),
	JSON_OBJ_DESCR_PRIM(struct test_struct, some_int16, JSON_TOK_INT),
	JSON_OBJ_DESCR_PRIM(struct test_struct, some_int64,
			    JSON_TOK_INT64),
	JSON_OBJ_DESCR_PRIM(struct test_struct, another_int64,
			    JSON_TOK_INT64),
	JSON_OBJ_DESCR_PRIM(struct test_struct, some_uint64,
			    JSON_TOK_UINT64),
	JSON_OBJ_DESCR_PRIM(struct test_struct, another_uint64,
			    JSON_TOK_UINT64),
	JSON_OBJ_DESCR_OBJECT(struct test_struct, some_nested_struct,
			      nested_descr),
	JSON_OBJ_DESCR_ARRAY(struct test_struct, some_array,
			     16, some_array_len, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct test_struct, "another_b!@l",
				  another_bxxl, JSON_TOK_TRUE),
	JSON_OBJ_DESCR_PRIM_NAMED(struct test_struct, "if",
				  if_, JSON_TOK_TRUE),
	JSON_OBJ_DESCR_ARRAY_NAMED(struct test_struct, "another-array",
				   another_array, 10, another_array_len,
				   JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_OBJECT_NAMED(struct test_struct, "4nother_ne$+",
				    xnother_nexx, nested_descr),
	JSON_OBJ_DESCR_OBJ_ARRAY(struct test_struct, nested_obj_array, 2,
				 obj_array_len, nested_descr, ARRAY_SIZE(nested_descr)),
};

static const struct json_obj_descr elt_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct elt, name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct elt, name_buf, JSON_TOK_STRING_BUF),
	JSON_OBJ_DESCR_PRIM(struct elt, height, JSON_TOK_NUMBER),
};

static const struct json_obj_descr obj_array_descr[] = {
	JSON_OBJ_DESCR_OBJ_ARRAY(struct obj_array, elements, 10, num_elements,
				 elt_descr, ARRAY_SIZE(elt_descr)),
};

static const struct json_obj_descr obj_limits_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct test_int_limits, int_max, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct test_int_limits, int_cero, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct test_int_limits, int_min, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct test_int_limits, int64_max, JSON_TOK_INT64),
	JSON_OBJ_DESCR_PRIM(struct test_int_limits, int64_cero, JSON_TOK_INT64),
	JSON_OBJ_DESCR_PRIM(struct test_int_limits, int64_min, JSON_TOK_INT64),
	JSON_OBJ_DESCR_PRIM(struct test_int_limits, uint64_max, JSON_TOK_UINT64),
	JSON_OBJ_DESCR_PRIM(struct test_int_limits, uint64_cero, JSON_TOK_UINT64),
	JSON_OBJ_DESCR_PRIM(struct test_int_limits, uint64_min, JSON_TOK_UINT64),
	JSON_OBJ_DESCR_PRIM(struct test_int_limits, uint32_max, JSON_TOK_UINT),
	JSON_OBJ_DESCR_PRIM(struct test_int_limits, uint32_cero, JSON_TOK_UINT),
	JSON_OBJ_DESCR_PRIM(struct test_int_limits, uint32_min, JSON_TOK_UINT),
	JSON_OBJ_DESCR_PRIM(struct test_int_limits, int16_max, JSON_TOK_INT),
	JSON_OBJ_DESCR_PRIM(struct test_int_limits, int16_cero, JSON_TOK_INT),
	JSON_OBJ_DESCR_PRIM(struct test_int_limits, int16_min, JSON_TOK_INT),
	JSON_OBJ_DESCR_PRIM(struct test_int_limits, uint16_max, JSON_TOK_UINT),
	JSON_OBJ_DESCR_PRIM(struct test_int_limits, uint16_cero, JSON_TOK_UINT),
	JSON_OBJ_DESCR_PRIM(struct test_int_limits, uint16_min, JSON_TOK_UINT),
	JSON_OBJ_DESCR_PRIM(struct test_int_limits, int8_max, JSON_TOK_INT),
	JSON_OBJ_DESCR_PRIM(struct test_int_limits, int8_cero, JSON_TOK_INT),
	JSON_OBJ_DESCR_PRIM(struct test_int_limits, int8_min, JSON_TOK_INT),
	JSON_OBJ_DESCR_PRIM(struct test_int_limits, uint8_max, JSON_TOK_UINT),
	JSON_OBJ_DESCR_PRIM(struct test_int_limits, uint8_cero, JSON_TOK_UINT),
	JSON_OBJ_DESCR_PRIM(struct test_int_limits, uint8_min, JSON_TOK_UINT),
};

static const struct json_obj_descr obj_float_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct test_float, some_float, JSON_TOK_FLOAT_FP),
	JSON_OBJ_DESCR_PRIM(struct test_float, another_float, JSON_TOK_FLOAT_FP),
	JSON_OBJ_DESCR_ARRAY(struct test_float, some_array,
			     16, some_array_len, JSON_TOK_FLOAT_FP),
};

static const struct json_obj_descr obj_float_limits_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct test_float_limits, float_max, JSON_TOK_FLOAT_FP),
	JSON_OBJ_DESCR_PRIM(struct test_float_limits, float_cero, JSON_TOK_FLOAT_FP),
	JSON_OBJ_DESCR_PRIM(struct test_float_limits, float_min, JSON_TOK_FLOAT_FP),
};

static const struct json_obj_descr obj_double_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct test_double, some_double, JSON_TOK_DOUBLE_FP),
	JSON_OBJ_DESCR_PRIM(struct test_double, another_double, JSON_TOK_DOUBLE_FP),
	JSON_OBJ_DESCR_ARRAY(struct test_double, some_array,
			     16, some_array_len, JSON_TOK_DOUBLE_FP),
};

static const struct json_obj_descr obj_double_limits_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct test_double_limits, double_max, JSON_TOK_DOUBLE_FP),
	JSON_OBJ_DESCR_PRIM(struct test_double_limits, double_cero, JSON_TOK_DOUBLE_FP),
	JSON_OBJ_DESCR_PRIM(struct test_double_limits, double_min, JSON_TOK_DOUBLE_FP),
};

struct array {
	struct elt objects;
};

struct obj_array_array {
	struct array objects_array[4];
	size_t objects_array_len;
};

static const struct json_obj_descr array_descr[] = {
	JSON_OBJ_DESCR_OBJECT(struct array, objects, elt_descr),
};

static const struct json_obj_descr array_array_descr[] = {
	JSON_OBJ_DESCR_ARRAY_ARRAY(struct obj_array_array, objects_array, 4,
				   objects_array_len, array_descr,
				   ARRAY_SIZE(array_descr)),
};

struct obj_array_2dim {
	struct obj_array objects_array_array[3];
	size_t objects_array_array_len;
};

static const struct json_obj_descr array_2dim_descr[] = {
	JSON_OBJ_DESCR_ARRAY_ARRAY(struct obj_array_2dim, objects_array_array, 3,
				   objects_array_array_len, obj_array_descr,
				   ARRAY_SIZE(obj_array_descr)),
};

struct obj_array_2dim_extra {
	const char *name;
	int val;
	struct obj_array_2dim obj_array_2dim;
};

static const struct json_obj_descr array_2dim_extra_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct obj_array_2dim_extra, name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct obj_array_2dim_extra, val, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_ARRAY_ARRAY(struct obj_array_2dim_extra, obj_array_2dim, 3,
				   obj_array_2dim.objects_array_array_len, obj_array_descr,
				   ARRAY_SIZE(obj_array_descr)),
};

static const struct json_obj_descr array_2dim_extra_named_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct obj_array_2dim_extra, name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct obj_array_2dim_extra, val, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_ARRAY_ARRAY_NAMED(struct obj_array_2dim_extra, data, obj_array_2dim, 3,
				   obj_array_2dim.objects_array_array_len, obj_array_descr,
				   ARRAY_SIZE(obj_array_descr)),
};

struct test_json_tok_encoded_obj {
	const char *encoded_obj;
	const char *encoded_obj_array[3];
	size_t encoded_obj_array_len;
	int ok;
};

static const struct json_obj_descr test_json_tok_encoded_obj_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct test_json_tok_encoded_obj, encoded_obj, JSON_TOK_ENCODED_OBJ),
	JSON_OBJ_DESCR_ARRAY(struct test_json_tok_encoded_obj, encoded_obj_array,
			     3, encoded_obj_array_len, JSON_TOK_ENCODED_OBJ),
	JSON_OBJ_DESCR_PRIM(struct test_json_tok_encoded_obj, ok, JSON_TOK_NUMBER),
};

struct test_element {
	int int1;
	int int2;
	int int3;
};

struct test_outer {
	struct test_element array[5];
	size_t num_elements;
};

static const struct json_obj_descr element_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct test_element, int1, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct test_element, int2, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct test_element, int3, JSON_TOK_NUMBER),
};

static const struct json_obj_descr outer_descr[] = {
	JSON_OBJ_DESCR_OBJ_ARRAY(struct test_outer, array, 5,
				num_elements, element_descr, ARRAY_SIZE(element_descr))
};

struct test_alignment_nested {
	bool bool1;
	int int1;
	bool bool2;
};

struct test_alignment_bool {
	struct test_alignment_nested array[3];
	size_t num_elements;
};

static const struct json_obj_descr alignment_nested_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct test_alignment_nested, bool1, JSON_TOK_TRUE),
	JSON_OBJ_DESCR_PRIM(struct test_alignment_nested, int1, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct test_alignment_nested, bool2, JSON_TOK_TRUE),
};

static const struct json_obj_descr alignment_bool_descr[] = {
	JSON_OBJ_DESCR_OBJ_ARRAY(struct test_alignment_bool, array, 3, num_elements,
				 alignment_nested_descr, ARRAY_SIZE(alignment_nested_descr)) };

enum int8_enum { I8_MIN = INT8_MIN, I8_MAX = INT8_MAX };
enum uint8_enum { U8_MIN = 0, U8_MAX = UINT8_MAX };
enum int16_enum { I16_MIN = INT16_MIN, I16_MAX = INT16_MAX };
enum uint16_enum { U16_MIN = 0, U16_MAX = UINT16_MAX };
enum int32_enum { I32_MIN = INT32_MIN, I32_MAX = INT32_MAX };
enum uint32_enum { U32_MIN = 0, U32_MAX = UINT32_MAX };

struct test_enums {
	enum int8_enum i8;
	enum uint8_enum u8;
	enum int16_enum i16;
	enum uint16_enum u16;
	enum int32_enum i32;
	enum uint32_enum u32;
};

static const struct json_obj_descr enums_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct test_enums, i8, JSON_TOK_INT),
	JSON_OBJ_DESCR_PRIM(struct test_enums, u8, JSON_TOK_UINT),
	JSON_OBJ_DESCR_PRIM(struct test_enums, i16, JSON_TOK_INT),
	JSON_OBJ_DESCR_PRIM(struct test_enums, u16, JSON_TOK_UINT),
	JSON_OBJ_DESCR_PRIM(struct test_enums, i32, JSON_TOK_INT),
	JSON_OBJ_DESCR_PRIM(struct test_enums, u32, JSON_TOK_UINT),
};

struct test_mixed_arr {
	const char *msg_type;
	uint64_t dev_id;
	struct test_nested nested;
	int arr[3];
	size_t arr_len;
	char status_buf[10];
	size_t count;
};

static const struct json_obj_descr test_mixed_arr_descr_arr[] = {
	JSON_OBJ_DESCR_ARRAY(struct test_mixed_arr, arr, 3, arr_len, JSON_TOK_NUMBER),
};

static const struct json_mixed_arr_descr test_mixed_arr_descr[] = {
	JSON_MIXED_ARR_DESCR_PRIM(struct test_mixed_arr, msg_type, JSON_TOK_STRING, count),
	JSON_MIXED_ARR_DESCR_PRIM(struct test_mixed_arr, dev_id, JSON_TOK_UINT64, count),
	JSON_MIXED_ARR_DESCR_OBJECT(struct test_mixed_arr, nested, nested_descr, count),
	JSON_MIXED_ARR_DESCR_ARRAY(struct test_mixed_arr, arr, 3, test_mixed_arr_descr_arr, count),
	JSON_MIXED_ARR_DESCR_PRIM(struct test_mixed_arr, status_buf, JSON_TOK_STRING_BUF, count),
};


ZTEST(lib_json_test, test_json_encoding)
{
	struct test_struct ts = {
		.some_string = "zephyr 123\uABCD",
		.some_string_buf = "z 123\uABCD",
		.some_int = 42,
		.some_int16 = 16,
		.some_int64 = 1152921504606846977,
		.another_int64 = -2305843009213693937,
		.some_uint64 = 18446744073709551615U,
		.another_uint64 = 0,
		.some_bool = true,
		.some_nested_struct = {
			.nested_int = -1234,
			.nested_bool = false,
			.nested_string = "this should be escaped: \t",
			.nested_string_buf = "esc: \t",
			.nested_int8 = -123,
			.nested_uint8 = 231,
			.nested_int64 = 4503599627370496,
			.nested_uint64 = 18446744073709551610U,
		},
		.some_array[0] = 1,
		.some_array[1] = 4,
		.some_array[2] = 8,
		.some_array[3] = 16,
		.some_array[4] = 32,
		.some_array_len = 5,
		.another_bxxl = true,
		.if_ = false,
		.another_array[0] = 2,
		.another_array[1] = 3,
		.another_array[2] = 5,
		.another_array[3] = 7,
		.another_array_len = 4,
		.xnother_nexx = {
			.nested_int = 1234,
			.nested_bool = true,
			.nested_string = "no escape necessary",
			.nested_string_buf = "no escape",
			.nested_int8 = 123,
			.nested_uint8 = 231,
			.nested_int64 = 4503599627370496,
			.nested_uint64 = 18446744073709551610U,
		},
		.nested_obj_array = {
			{1, true, "true", "true"},
			{0, false, "false", "false"}
		},
		.obj_array_len = 2
	};
	char encoded[] = "{\"some_string\":\"zephyr 123\uABCD\","
		"\"some_string_buf\":\"z 123\uABCD\","
		"\"some_int\":42,\"some_bool\":true,\"some_int16\":16,"
		"\"some_int64\":1152921504606846977,"
		"\"another_int64\":-2305843009213693937,"
		"\"some_uint64\":18446744073709551615,"
		"\"another_uint64\":0,"
		"\"some_nested_struct\":{\"nested_int\":-1234,"
		"\"nested_bool\":false,\"nested_string\":"
		"\"this should be escaped: \\t\","
		"\"nested_string_buf\":\"esc: \\t\","
		"\"nested_int8\":-123,"
		"\"nested_uint8\":231,"
		"\"nested_int64\":4503599627370496,"
		"\"nested_uint64\":18446744073709551610},"
		"\"some_array\":[1,4,8,16,32],"
		"\"another_b!@l\":true,"
		"\"if\":false,"
		"\"another-array\":[2,3,5,7],"
		"\"4nother_ne$+\":{\"nested_int\":1234,"
		"\"nested_bool\":true,"
		"\"nested_string\":\"no escape necessary\","
		"\"nested_string_buf\":\"no escape\","
		"\"nested_int8\":123,"
		"\"nested_uint8\":231,"
		"\"nested_int64\":4503599627370496,"
		"\"nested_uint64\":18446744073709551610},"
		"\"nested_obj_array\":["
		"{\"nested_int\":1,\"nested_bool\":true,\"nested_string\":\"true\",\"nested_string_buf\":\"true\",\"nested_int8\":0,\"nested_uint8\":0,\"nested_int64\":0,\"nested_uint64\":0},"
		"{\"nested_int\":0,\"nested_bool\":false,\"nested_string\":\"false\",\"nested_string_buf\":\"false\",\"nested_int8\":0,\"nested_uint8\":0,\"nested_int64\":0,\"nested_uint64\":0}]"
		"}";
	char buffer[sizeof(encoded)];
	int ret;
	ssize_t len;

	len = json_calc_encoded_len(test_descr, ARRAY_SIZE(test_descr), &ts);
	zassert_equal(len, strlen(encoded), "encoded size mismatch");

	ret = json_obj_encode_buf(test_descr, ARRAY_SIZE(test_descr),
				  &ts, buffer, sizeof(buffer));
	zassert_equal(ret, 0, "Encoding function failed");

	ret = strncmp(buffer, encoded, sizeof(encoded) - 1);
	zassert_equal(ret, 0, "Encoded contents not consistent");
}

ZTEST(lib_json_test, test_json_decoding)
{
	struct test_struct ts;
	char encoded[] = "{\"some_string\":\"zephyr 123\\uABCD456\","
		"\"some_string_buf\":\"z\\uABCD\","
		"\"some_int\":\t42\n,"
		"\"some_int16\":\t16\n,"
		"\"some_bool\":true    \t  "
		"\n"
		"\r   ,"
		"\"some_int64\":-4611686018427387904,"
		"\"another_int64\":-2147483648,"
		"\"some_uint64\":18446744073709551615,"
		"\"another_uint64\":0,"
		"\"some_nested_struct\":{    "
		"\"nested_int\":-1234,\n\n"
		"\"nested_bool\":false,\t"
		"\"nested_string\":\"this should be escaped: \\t\","
		"\"nested_string_buf\":\"esc: \\t\","
		"\"nested_int8\":123,"
		"\"nested_int64\":9223372036854775807,"
		"\"extra_nested_array\":[0,-1]},"
		"\"extra_struct\":{\"nested_bool\":false},"
		"\"extra_bool\":true,"
		"\"some_array\":[11,22, 33,\t45,\n299],"
		"\"another_b!@l\":true,"
		"\"if\":false,"
		"\"another-array\":[2,3,5,7],"
		"\"4nother_ne$+\":{\"nested_int\":1234,"
		"\"nested_bool\":true,"
		"\"nested_string\":\"no escape necessary\","
		"\"nested_string_buf\":\"no escape\","
		"\"nested_int8\":-123,"
		"\"nested_int64\":-9223372036854775806},"
		"\"nested_obj_array\":["
		"{\"nested_int\":1,\"nested_bool\":true,\"nested_string\":\"true\",\"nested_string_buf\":\"true\"},"
		"{\"nested_int\":0,\"nested_bool\":false,\"nested_string\":\"false\",\"nested_string_buf\":\"false\"}]"
		"}\n";
	const int expected_array[] = { 11, 22, 33, 45, 299 };
	const int expected_other_array[] = { 2, 3, 5, 7 };
	int ret;

	ret = json_obj_parse(encoded, sizeof(encoded) - 1, test_descr,
			     ARRAY_SIZE(test_descr), &ts);

	zassert_equal(ret, (1 << ARRAY_SIZE(test_descr)) - 1,
		      "Not all fields decoded correctly");

	zassert_str_equal(ts.some_string, "zephyr 123\\uABCD456",
			  "String not decoded correctly");
	zassert_str_equal(ts.some_string_buf, "z\\uABCD",
			  "String (array) not decoded correctly");
	zassert_equal(ts.some_int, 42, "Positive integer not decoded correctly");
	zassert_equal(ts.some_int16, 16, "Positive integer not decoded correctly");
	zassert_equal(ts.some_bool, true, "Boolean not decoded correctly");
	zassert_equal(ts.some_int64, -4611686018427387904,
		      "int64 not decoded correctly");
	zassert_equal(ts.another_int64, -2147483648,
		      "int64 not decoded correctly");
	zassert_equal(ts.some_nested_struct.nested_int, -1234,
		      "Nested negative integer not decoded correctly");
	zassert_equal(ts.some_nested_struct.nested_int8, 123,
		      "Nested int8 not decoded correctly");
	zassert_equal(ts.some_nested_struct.nested_int64, 9223372036854775807,
		      "Nested int64 not decoded correctly");
	zassert_equal(ts.some_nested_struct.nested_bool, false,
		      "Nested boolean value not decoded correctly");
	zassert_str_equal(ts.some_nested_struct.nested_string,
			  "this should be escaped: \\t",
			  "Nested string not decoded correctly");
	zassert_str_equal(ts.some_nested_struct.nested_string_buf, "esc: \t",
			  "Nested string-array not decoded correctly");
	zassert_equal(ts.some_array_len, 5,
		      "Array doesn't have correct number of items");
	zassert_true(!memcmp(ts.some_array, expected_array,
		    sizeof(expected_array)),
		    "Array not decoded with expected values");
	zassert_true(ts.another_bxxl,
		     "Named boolean (special chars) not decoded correctly");
	zassert_false(ts.if_,
		      "Named boolean (reserved word) not decoded correctly");
	zassert_equal(ts.another_array_len, 4,
		      "Named array does not have correct number of items");
	zassert_true(!memcmp(ts.another_array, expected_other_array,
			     sizeof(expected_other_array)),
		     "Decoded named array not with expected values");
	zassert_equal(ts.xnother_nexx.nested_int, 1234,
		      "Named nested integer not decoded correctly");
	zassert_equal(ts.xnother_nexx.nested_int8, -123,
		      "Named nested int8 not decoded correctly");
	zassert_equal(ts.xnother_nexx.nested_int64, -9223372036854775806,
		      "Named nested int64 not decoded correctly");
	zassert_equal(ts.xnother_nexx.nested_bool, true,
		      "Named nested boolean not decoded correctly");
	zassert_str_equal(ts.xnother_nexx.nested_string,
			  "no escape necessary",
			  "Named nested string not decoded correctly");
	zassert_str_equal(ts.xnother_nexx.nested_string_buf,
			  "no escape",
			  "Named nested string-array not decoded correctly");
	zassert_equal(ts.obj_array_len, 2,
		      "Array of objects does not have correct number of items");
	zassert_equal(ts.nested_obj_array[0].nested_int, 1,
		      "Integer in first object array element not decoded correctly");
	zassert_equal(ts.nested_obj_array[0].nested_bool, true,
		      "Boolean value in first object array element not decoded correctly");
	zassert_str_equal(ts.nested_obj_array[0].nested_string, "true",
			  "String in first object array element not decoded correctly");
	zassert_str_equal(ts.nested_obj_array[0].nested_string_buf, "true",
			  "String buffer in first object array element not decoded correctly");
	zassert_equal(ts.nested_obj_array[1].nested_int, 0,
		      "Integer in second object array element not decoded correctly");
	zassert_equal(ts.nested_obj_array[1].nested_bool, false,
		      "Boolean value in second object array element not decoded correctly");
	zassert_str_equal(ts.nested_obj_array[1].nested_string, "false",
			  "String in second object array element not decoded correctly");
	zassert_str_equal(ts.nested_obj_array[1].nested_string_buf, "false",
			  "String buffer in second object array element not decoded correctly");
}

ZTEST(lib_json_test, test_json_limits)
{
	int ret = 0;
	char encoded[] = "{\"int_max\":2147483647,"
			 "\"int_cero\":0,"
			 "\"int_min\":-2147483648,"
			 "\"int64_max\":9223372036854775807,"
			 "\"int64_cero\":0,"
			 "\"int64_min\":-9223372036854775808,"
			 "\"uint64_max\":18446744073709551615,"
			 "\"uint64_cero\":0,"
			 "\"uint64_min\":0,"
			 "\"uint32_max\":4294967295,"
			 "\"uint32_cero\":0,"
			 "\"uint32_min\":0,"
			 "\"int16_max\":32767,"
			 "\"int16_cero\":0,"
			 "\"int16_min\":-32768,"
			 "\"uint16_max\":65535,"
			 "\"uint16_cero\":0,"
			 "\"uint16_min\":0,"
			 "\"int8_max\":127,"
			 "\"int8_cero\":0,"
			 "\"int8_min\":-128,"
			 "\"uint8_max\":255,"
			 "\"uint8_cero\":0,"
			 "\"uint8_min\":0"
			 "}";

	struct test_int_limits limits = {0};
	struct test_int_limits limits_decoded = {0};

	limits.int_max = INT_MAX;
	limits.int_cero = 0;
	limits.int_min = INT_MIN;
	limits.int64_max = INT64_MAX;
	limits.int64_cero = 0;
	limits.int64_min = INT64_MIN;
	limits.uint64_max = UINT64_MAX;
	limits.uint64_cero = 0;
	limits.uint64_min = 0;
	limits.uint32_max = UINT32_MAX;
	limits.uint32_cero = 0;
	limits.uint32_min = 0;
	limits.int16_max = INT16_MAX;
	limits.int16_cero = 0;
	limits.int16_min = INT16_MIN;
	limits.uint16_max = UINT16_MAX;
	limits.uint16_cero = 0;
	limits.uint16_min = 0;
	limits.int8_max = INT8_MAX;
	limits.int8_cero = 0;
	limits.int8_min = INT8_MIN;
	limits.uint8_max = UINT8_MAX;
	limits.uint8_cero = 0;
	limits.uint8_min = 0;

	char buffer[sizeof(encoded)];

	ret = json_obj_encode_buf(obj_limits_descr, ARRAY_SIZE(obj_limits_descr),
				&limits, buffer, sizeof(buffer));
	ret = json_obj_parse(encoded, sizeof(encoded) - 1, obj_limits_descr,
			     ARRAY_SIZE(obj_limits_descr), &limits_decoded);

	zassert_str_equal(encoded, buffer,
			  "Integer limits not encoded correctly");
	zassert_true(!memcmp(&limits, &limits_decoded, sizeof(limits)),
		     "Integer limits not decoded correctly");
}

ZTEST(lib_json_test, test_json_float)
{
	char encoded[] = "{\"some_float\":-0.000244140625,"
			 "\"another_float\":12345600,"
			 "\"some_array\":[1.5,2.25]"
			 "}";

	struct test_float floats = {
		.some_float = -0.000244140625,
		.another_float = 12345600.0,
		.some_array[0] = 1.5,
		.some_array[1] = 2.25,
		.some_array_len = 2,
	};

	char buffer[sizeof(encoded)];
	struct test_float floats_decoded = {0};
	int ret;

	ret = json_obj_encode_buf(obj_float_descr, ARRAY_SIZE(obj_float_descr),
				&floats, buffer, sizeof(buffer));
	zassert_equal(ret, 0, "Encoding of float returned error");
	ret = json_obj_parse(encoded, sizeof(encoded), obj_float_descr,
			     ARRAY_SIZE(obj_float_descr), &floats_decoded);
	zassert_str_equal(encoded, buffer,
			  "Float not encoded correctly");
	zassert_true(!memcmp(&floats, &floats_decoded, sizeof(floats)),
		     "Float not decoded correctly");
}

ZTEST(lib_json_test, test_json_float_format)
{
	struct sub_test {
		float num;
		char *str;
	};
	struct sub_test encoded[] = {
		{ .num = 0, .str = "0" },
		{ .num = 0, .str = "0.0" },
		{ .num = 0, .str = "0e0" },
		{ .num = 0, .str = "0e+0" },
		{ .num = 0, .str = "0e-0" },

		{ .num = 12345, .str = "12345" },
		{ .num = 12345, .str = "1.2345e+4" },
		{ .num = 12345, .str = "1.2345e+04" },

		{ .num = -12345, .str = "-12345" },
		{ .num = -12345, .str = "-1.2345e+4" },
		{ .num = -12345, .str = "-1.2345e+04" },

		{ .num = 0.03125, .str = "0.03125" },
		{ .num = 0.03125, .str = "3.125e-2" },
		{ .num = 0.03125, .str = "3.125e-02" },

		{ .num = -0.03125, .str = "-0.03125" },
		{ .num = -0.03125, .str = "-3.125e-2" },
		{ .num = -0.03125, .str = "-3.125e-02" },
	};

	struct test_float ts;
	char str_encoded[32];

	for (int i = 0; i < ARRAY_SIZE(encoded); i++) {
		snprintf(str_encoded, sizeof(str_encoded), "{\"some_float\":%s}", encoded[i].str);
		int ret = json_obj_parse(str_encoded, strlen(str_encoded),
			       obj_float_descr, ARRAY_SIZE(obj_float_descr), &ts);
		zassert_equal(ret, 1, "Decoding failed: %s result: %d", str_encoded, ret);
		zassert_equal(ts.some_float, encoded[i].num,
			      "Decoding failed '%s' float: %g exepcted: %g",
			      str_encoded, (double)ts.some_float, (double)encoded[i].num);
	}
}

ZTEST(lib_json_test, test_json_float_nan)
{
	char encoded[] = "{\"some_float\":NaN,"
			 "\"another_float\":NaN,"
			 "\"some_array\":[NaN,NaN]"
			 "}";

	struct test_float floats = {
		.some_float = NAN,
		.another_float = NAN,
		.some_array[0] = NAN,
		.some_array[1] = NAN,
		.some_array_len = 2,
	};

	char buffer[sizeof(encoded)];
	struct test_float floats_decoded = {0};
	int ret;

	ret = json_obj_encode_buf(obj_float_descr, ARRAY_SIZE(obj_float_descr),
				&floats, buffer, sizeof(buffer));
	zassert_equal(ret, 0, "Encoding of float nan returned error");
	ret = json_obj_parse(encoded, sizeof(encoded), obj_float_descr,
			     ARRAY_SIZE(obj_float_descr), &floats_decoded);
	zassert_str_equal(encoded, buffer,
			  "Float not encoded correctly");
	zassert_true(!memcmp(&floats, &floats_decoded, sizeof(floats)),
		     "Float not decoded correctly");
}

ZTEST(lib_json_test, test_json_float_infinity)
{
	char encoded[] = "{\"some_float\":Infinity,"
			 "\"another_float\":-Infinity,"
			 "\"some_array\":[Infinity,-Infinity]"
			 "}";

	struct test_float floats = {
		.some_float = INFINITY,
		.another_float = -INFINITY,
		.some_array[0] = INFINITY,
		.some_array[1] = -INFINITY,
		.some_array_len = 2,
	};

	char buffer[sizeof(encoded)];
	struct test_float floats_decoded = {0};
	int ret;

	ret = json_obj_encode_buf(obj_float_descr, ARRAY_SIZE(obj_float_descr),
				&floats, buffer, sizeof(buffer));
	zassert_equal(ret, 0, "Encoding of float inf returned error");
	ret = json_obj_parse(encoded, sizeof(encoded), obj_float_descr,
			     ARRAY_SIZE(obj_float_descr), &floats_decoded);
	zassert_str_equal(encoded, buffer,
			  "Float not encoded correctly");
	zassert_true(!memcmp(&floats, &floats_decoded, sizeof(floats)),
		     "Float not decoded correctly");
}

ZTEST(lib_json_test, test_json_float_limits)
{
	char encoded[] = "{\"float_max\":3.40282347e+38,"
			 "\"float_cero\":0,"
			 "\"float_min\":-3.40282347e+38"
			 "}";

	struct test_float_limits limits = {
		.float_max = 3.40282347e+38,
		.float_cero = 0,
		.float_min = -3.40282347e+38,
	};

	char buffer[sizeof(encoded)];
	struct test_float_limits limits_decoded = {0};
	int ret;

	ret = json_obj_encode_buf(obj_float_limits_descr, ARRAY_SIZE(obj_float_limits_descr),
				&limits, buffer, sizeof(buffer));
	zassert_equal(ret, 0, "Encoding of float limits returned error");
	ret = json_obj_parse(encoded, sizeof(encoded), obj_float_limits_descr,
			     ARRAY_SIZE(obj_float_limits_descr), &limits_decoded);
	zassert_str_equal(encoded, buffer,
			  "Float limits not encoded correctly");
	zassert_true(!memcmp(&limits, &limits_decoded, sizeof(limits)),
		     "Float limits not decoded correctly");
}

ZTEST(lib_json_test, test_json_double)
{
	char encoded[] = "{\"some_double\":-0.000244140625,"
			 "\"another_double\":1234567890000000,"
			 "\"some_array\":[1.5,2.25]"
			 "}";

	struct test_double doubles = {
		.some_double = -0.000244140625,
		.another_double = 1234567890000000.0,
		.some_array[0] = 1.5,
		.some_array[1] = 2.25,
		.some_array_len = 2,
	};

	char buffer[sizeof(encoded)];
	struct test_double doubles_decoded = {0};
	int ret;

	ret = json_obj_encode_buf(obj_double_descr, ARRAY_SIZE(obj_double_descr),
				&doubles, buffer, sizeof(buffer));
	zassert_equal(ret, 0, "Encoding of double returned error");
	ret = json_obj_parse(encoded, sizeof(encoded), obj_double_descr,
			     ARRAY_SIZE(obj_double_descr), &doubles_decoded);
	zassert_str_equal(encoded, buffer,
			  "Double not encoded correctly");
	zassert_true(!memcmp(&doubles, &doubles_decoded, sizeof(doubles)),
		     "Double not decoded correctly");
}

ZTEST(lib_json_test, test_json_double_format)
{
	struct sub_test {
		double num;
		char *str;
	};
	struct sub_test encoded[] = {
		{ .num = 0, .str = "0" },
		{ .num = 0, .str = "0.0" },
		{ .num = 0, .str = "0e0" },
		{ .num = 0, .str = "0e+0" },
		{ .num = 0, .str = "0e-0" },

		{ .num = 12345, .str = "12345" },
		{ .num = 12345, .str = "1.2345e+4" },
		{ .num = 12345, .str = "1.2345e+04" },

		{ .num = -12345, .str = "-12345" },
		{ .num = -12345, .str = "-1.2345e+4" },
		{ .num = -12345, .str = "-1.2345e+04" },

		{ .num = 0.03125, .str = "0.03125" },
		{ .num = 0.03125, .str = "3.125e-2" },
		{ .num = 0.03125, .str = "3.125e-02" },

		{ .num = -0.03125, .str = "-0.03125" },
		{ .num = -0.03125, .str = "-3.125e-2" },
		{ .num = -0.03125, .str = "-3.125e-02" },
	};

	struct test_double ts;
	char str_encoded[32];

	for (int i = 0; i < ARRAY_SIZE(encoded); i++) {
		snprintf(str_encoded, sizeof(str_encoded), "{\"some_double\":%s}", encoded[i].str);
		int ret = json_obj_parse(str_encoded, strlen(str_encoded),
					 obj_double_descr, ARRAY_SIZE(obj_double_descr), &ts);
		zassert_equal(ret, 1, "Decoding failed: %s result: %d", str_encoded, ret);
		zassert_equal(ts.some_double, encoded[i].num,
			      "Decoding failed '%s' double: %g exepcted: %g",
			      str_encoded, ts.some_double, encoded[i].num);
	}
}


ZTEST(lib_json_test, test_json_double_nan)
{
	char encoded[] = "{\"some_double\":NaN,"
			 "\"another_double\":NaN,"
			 "\"some_array\":[NaN,NaN]"
			 "}";

	struct test_double doubles = {
		.some_double = (double)NAN,
		.another_double = (double)NAN,
		.some_array[0] = (double)NAN,
		.some_array[1] = (double)NAN,
		.some_array_len = 2,
	};

	char buffer[sizeof(encoded)];
	struct test_double doubles_decoded = {0};
	int ret;

	ret = json_obj_encode_buf(obj_double_descr, ARRAY_SIZE(obj_double_descr),
				&doubles, buffer, sizeof(buffer));
	zassert_equal(ret, 0, "Encoding of double nan returned error");
	ret = json_obj_parse(encoded, sizeof(encoded), obj_double_descr,
			     ARRAY_SIZE(obj_double_descr), &doubles_decoded);
	zassert_str_equal(encoded, buffer,
			  "Double not encoded correctly");
	zassert_true(!memcmp(&doubles, &doubles_decoded, sizeof(doubles)),
		     "Double not decoded correctly");
}

ZTEST(lib_json_test, test_json_double_infinity)
{
	char encoded[] = "{\"some_double\":Infinity,"
			 "\"another_double\":-Infinity,"
			 "\"some_array\":[Infinity,-Infinity]"
			 "}";

	struct test_double doubles = {
		.some_double = (double)INFINITY,
		.another_double = (double)-INFINITY,
		.some_array[0] = (double)INFINITY,
		.some_array[1] = (double)-INFINITY,
		.some_array_len = 2,
	};

	char buffer[sizeof(encoded)];
	struct test_double doubles_decoded = {0};
	int ret;

	ret = json_obj_encode_buf(obj_double_descr, ARRAY_SIZE(obj_double_descr),
				&doubles, buffer, sizeof(buffer));
	zassert_equal(ret, 0, "Encoding of double inf returned error");
	ret = json_obj_parse(encoded, sizeof(encoded), obj_double_descr,
			     ARRAY_SIZE(obj_double_descr), &doubles_decoded);
	zassert_str_equal(encoded, buffer,
			  "Double not encoded correctly");
	zassert_true(!memcmp(&doubles, &doubles_decoded, sizeof(doubles)),
		     "Double not decoded correctly");
}

ZTEST(lib_json_test, test_json_doubles_limits)
{
	char encoded[] = "{\"double_max\":1.797693134862315e+308,"
			 "\"double_cero\":0,"
			 "\"double_min\":-1.797693134862315e+308"
			 "}";

	struct test_double_limits limits = {
		.double_max = 1.797693134862315e+308,
		.double_cero = 0,
		.double_min = -1.797693134862315e+308,
	};

	char buffer[sizeof(encoded)];
	struct test_double_limits limits_decoded = {0};
	int ret;

	ret = json_obj_encode_buf(obj_double_limits_descr, ARRAY_SIZE(obj_double_limits_descr),
				&limits, buffer, sizeof(buffer));
	zassert_equal(ret, 0, "Encoding of double limits returned error");
	ret = json_obj_parse(encoded, sizeof(encoded), obj_double_limits_descr,
			     ARRAY_SIZE(obj_double_limits_descr), &limits_decoded);
	zassert_str_equal(encoded, buffer,
			  "Double limits not encoded correctly");
	zassert_true(!memcmp(&limits, &limits_decoded, sizeof(limits)),
		     "Double limits not decoded correctly");
}

ZTEST(lib_json_test, test_json_encoding_array_array)
{
	struct obj_array_array obj_array_array_ts = {
		.objects_array = {
			[0] = {{.name = "Sim\303\263n Bol\303\255var",
				.name_buf = "Sim\303\263n",
				.height = 168}},
			[1] = {{.name = "Pel\303\251",
				.name_buf = "Pel\303\251",
				.height = 173}},
			[2] = {{.name = "Usain Bolt",
				.name_buf = "Usain",
				.height = 195}},
		},
		.objects_array_len = 3,
	};
	char encoded[] = "{\"objects_array\":["
			 "{\"name\":\"Sim\303\263n Bol\303\255var\",\"name_buf\":\"Sim\303\263n\",\"height\":168},"
			 "{\"name\":\"Pel\303\251\",\"name_buf\":\"Pel\303\251\",\"height\":173},"
			 "{\"name\":\"Usain Bolt\",\"name_buf\":\"Usain\",\"height\":195}"
			 "]}";

	char buffer[sizeof(encoded)];
	int ret;

	ret = json_obj_encode_buf(array_array_descr, ARRAY_SIZE(array_array_descr),
				  &obj_array_array_ts, buffer, sizeof(buffer));
	zassert_equal(ret, 0, "Encoding array returned error");
	zassert_str_equal(buffer, encoded,
			  "Encoded array of objects is not consistent");
}

ZTEST(lib_json_test, test_json_decoding_array_array)
{
	int ret;
	struct obj_array_array obj_array_array_ts;
	char encoded[] = "{\"objects_array\":["
			  "{\"height\":168,\"name\":\"Sim\303\263n Bol\303\255var\",\"name_buf\":\"Sim\303\263n\"},"
			  "{\"height\":173,\"name\":\"Pel\303\251\",\"name_buf\":\"Pel\303\251\"},"
			  "{\"height\":195,\"name\":\"Usain Bolt\",\"name_buf\":\"Usain\"}]"
			  "}";

	ret = json_obj_parse(encoded, sizeof(encoded),
			     array_array_descr,
			     ARRAY_SIZE(array_array_descr),
			     &obj_array_array_ts);

	zassert_equal(ret, 1, "Encoding array of objects returned error");
	zassert_equal(obj_array_array_ts.objects_array_len, 3,
		      "Array doesn't have correct number of items");

	zassert_str_equal(obj_array_array_ts.objects_array[0].objects.name,
			  "Sim\303\263n Bol\303\255var",
			  "String not decoded correctly");
	zassert_str_equal(obj_array_array_ts.objects_array[0].objects.name_buf,
			  "Sim\303\263n",
			  "String buffer not decoded correctly");
	zassert_equal(obj_array_array_ts.objects_array[0].objects.height, 168,
		      "Sim\303\263n Bol\303\255var height not decoded correctly");

	zassert_str_equal(obj_array_array_ts.objects_array[1].objects.name,
			  "Pel\303\251", "String not decoded correctly");
	zassert_str_equal(obj_array_array_ts.objects_array[1].objects.name_buf,
			  "Pel\303\251", "String buffer not decoded correctly");
	zassert_equal(obj_array_array_ts.objects_array[1].objects.height, 173,
		      "Pel\303\251 height not decoded correctly");

	zassert_str_equal(obj_array_array_ts.objects_array[2].objects.name,
			  "Usain Bolt", "String not decoded correctly");
	zassert_str_equal(obj_array_array_ts.objects_array[2].objects.name_buf,
			  "Usain", "String buffer not decoded correctly");
	zassert_equal(obj_array_array_ts.objects_array[2].objects.height, 195,
		      "Usain Bolt height not decoded correctly");
}

ZTEST(lib_json_test, test_json_obj_arr_encoding)
{
	struct obj_array oa = {
		.elements = {
			[0] = {
				.name = "Sim\303\263n Bol\303\255var",
				.name_buf = "Sim\303\263n",
				.height = 168 },
			[1] = {
				.name = "Muggsy Bogues",
				.name_buf = "Muggsy",
				.height = 160 },
			[2] = {
				.name = "Pel\303\251",
				.name_buf = "Pel\303\251",
				.height = 173 },
			[3] = {
				.name = "Hakeem Olajuwon",
				.name_buf = "Hakeem",
				.height = 213 },
			[4] = {
				.name = "Alex Honnold",
				.name_buf = "Alex",
				.height = 180 },
			[5] = {
				.name = "Hazel Findlay",
				.name_buf = "Hazel",
				.height = 157 },
			[6] = {
				.name = "Daila Ojeda",
				.name_buf = "Daila",
				.height = 158 },
			[7] = {
				.name = "Albert Einstein",
				.name_buf = "Albert",
				.height = 172 },
			[8] = {
				.name = "Usain Bolt",
				.name_buf = "Usain",
				.height = 195 },
			[9] = {
				.name = "Paavo Nurmi",
				.name_buf = "Paavo",
				.height = 174 },
		},
		.num_elements = 10,
	};
	char encoded[] = "{\"elements\":["
		"{\"name\":\"Sim\303\263n Bol\303\255var\",\"name_buf\":\"Sim\303\263n\",\"height\":168},"
		"{\"name\":\"Muggsy Bogues\",\"name_buf\":\"Muggsy\",\"height\":160},"
		"{\"name\":\"Pel\303\251\",\"name_buf\":\"Pel\303\251\",\"height\":173},"
		"{\"name\":\"Hakeem Olajuwon\",\"name_buf\":\"Hakeem\",\"height\":213},"
		"{\"name\":\"Alex Honnold\",\"name_buf\":\"Alex\",\"height\":180},"
		"{\"name\":\"Hazel Findlay\",\"name_buf\":\"Hazel\",\"height\":157},"
		"{\"name\":\"Daila Ojeda\",\"name_buf\":\"Daila\",\"height\":158},"
		"{\"name\":\"Albert Einstein\",\"name_buf\":\"Albert\",\"height\":172},"
		"{\"name\":\"Usain Bolt\",\"name_buf\":\"Usain\",\"height\":195},"
		"{\"name\":\"Paavo Nurmi\",\"name_buf\":\"Paavo\",\"height\":174}"
		"]}";
	char buffer[sizeof(encoded)];
	int ret;

	ret = json_obj_encode_buf(obj_array_descr, ARRAY_SIZE(obj_array_descr),
				  &oa, buffer, sizeof(buffer));
	zassert_equal(ret, 0, "Encoding array of object returned error");
	zassert_str_equal(buffer, encoded,
			  "Encoded array of objects is not consistent");
}

ZTEST(lib_json_test, test_json_arr_obj_decoding)
{
	int ret;
	struct obj_array obj_array_array_ts;
	char encoded[] = "[{\"height\":168,\"name\":\"Sim\303\263n Bol\303\255var\","
			 "\"name_buf\":\"Sim\303\263n\"},"
			 "{\"height\":173,\"name\":\"Pel\303\251\",\"name_buf\":\"Pel\303\251\"},"
			 "{\"height\":195,\"name\":\"Usain Bolt\",\"name_buf\":\"Usain\"}"
			 "]";

	ret = json_arr_parse(encoded, sizeof(encoded),
			     obj_array_descr,
			     &obj_array_array_ts);

	zassert_equal(ret, 0, "Encoding array of objects returned error %d", ret);
	zassert_equal(obj_array_array_ts.num_elements, 3,
		      "Array doesn't have correct number of items");

	zassert_str_equal(obj_array_array_ts.elements[0].name,
			  "Sim\303\263n Bol\303\255var",
			  "String not decoded correctly");
	zassert_str_equal(obj_array_array_ts.elements[0].name_buf,
			  "Sim\303\263n",
			  "String buffer not decoded correctly");
	zassert_equal(obj_array_array_ts.elements[0].height, 168,
		      "Sim\303\263n Bol\303\255var height not decoded correctly");

	zassert_str_equal(obj_array_array_ts.elements[1].name, "Pel\303\251",
			  "String not decoded correctly");
	zassert_str_equal(obj_array_array_ts.elements[1].name_buf, "Pel\303\251",
			  "String buffer not decoded correctly");
	zassert_equal(obj_array_array_ts.elements[1].height, 173,
		      "Pel\303\251 height not decoded correctly");

	zassert_str_equal(obj_array_array_ts.elements[2].name, "Usain Bolt",
			  "String not decoded correctly");
	zassert_str_equal(obj_array_array_ts.elements[2].name_buf, "Usain",
			  "String buffer not decoded correctly");
	zassert_equal(obj_array_array_ts.elements[2].height, 195,
		      "Usain Bolt height not decoded correctly");
}

ZTEST(lib_json_test, test_json_arr_obj_encoding)
{
	struct obj_array oa = {
		.elements = {
			[0] = {
				.name = "Sim\303\263n Bol\303\255var",
				.name_buf = "Sim\303\263n",
				.height = 168 },
			[1] = {
				.name = "Muggsy Bogues",
				.name_buf = "Muggsy",
				.height = 160 },
			[2] = {
				.name = "Pel\303\251",
				.name_buf = "Pel\303\251",
				.height = 173 },
			[3] = {
				.name = "Hakeem Olajuwon",
				.name_buf = "Hakeem",
				.height = 213 },
			[4] = {
				.name = "Alex Honnold",
				.name_buf = "Alex",
				.height = 180 },
			[5] = {
				.name = "Hazel Findlay",
				.name_buf = "Hazel",
				.height = 157 },
			[6] = {
				.name = "Daila Ojeda",
				.name_buf = "Daila",
				.height = 158 },
			[7] = {
				.name = "Albert Einstein",
				.name_buf = "Albert",
				.height = 172 },
			[8] = {
				.name = "Usain Bolt",
				.name_buf = "Usain",
				.height = 195 },
			[9] = {
				.name = "Paavo Nurmi",
				.name_buf = "Paavo",
				.height = 174 },
		},
		.num_elements = 10,
	};
	char encoded[] = "["
		"{\"name\":\"Sim\303\263n Bol\303\255var\",\"name_buf\":\"Sim\303\263n\",\"height\":168},"
		"{\"name\":\"Muggsy Bogues\",\"name_buf\":\"Muggsy\",\"height\":160},"
		"{\"name\":\"Pel\303\251\",\"name_buf\":\"Pel\303\251\",\"height\":173},"
		"{\"name\":\"Hakeem Olajuwon\",\"name_buf\":\"Hakeem\",\"height\":213},"
		"{\"name\":\"Alex Honnold\",\"name_buf\":\"Alex\",\"height\":180},"
		"{\"name\":\"Hazel Findlay\",\"name_buf\":\"Hazel\",\"height\":157},"
		"{\"name\":\"Daila Ojeda\",\"name_buf\":\"Daila\",\"height\":158},"
		"{\"name\":\"Albert Einstein\",\"name_buf\":\"Albert\",\"height\":172},"
		"{\"name\":\"Usain Bolt\",\"name_buf\":\"Usain\",\"height\":195},"
		"{\"name\":\"Paavo Nurmi\",\"name_buf\":\"Paavo\",\"height\":174}"
		"]";
	char buffer[sizeof(encoded)];
	int ret;
	ssize_t len;

	len = json_calc_encoded_arr_len(obj_array_descr, &oa);
	zassert_equal(len, strlen(encoded), "encoded size mismatch");

	ret = json_arr_encode_buf(obj_array_descr, &oa, buffer, sizeof(buffer));
	zassert_equal(ret, 0, "Encoding array of object returned error %d", ret);
	zassert_str_equal(buffer, encoded,
			  "Encoded array of objects is not consistent");
}

ZTEST(lib_json_test, test_json_obj_arr_decoding)
{
	struct obj_array oa;
	char encoded[] = "{\"elements\":["
		"{\"name\":\"Sim\303\263n Bol\303\255var\",\"name_buf\":\"Sim\303\263n\",\"height\":168},"
		"{\"name\":\"Muggsy Bogues\",\"name_buf\":\"Muggsy\",\"height\":160},"
		"{\"name\":\"Pel\303\251\",\"name_buf\":\"Pel\303\251\",\"height\":173},"
		"{\"name\":\"Hakeem Olajuwon\",\"name_buf\":\"Hakeem\",\"height\":213},"
		"{\"name\":\"Alex Honnold\",\"name_buf\":\"Alex\",\"height\":180},"
		"{\"name\":\"Hazel Findlay\",\"name_buf\":\"Hazel\",\"height\":157},"
		"{\"name\":\"Daila Ojeda\",\"name_buf\":\"Daila\",\"height\":158},"
		"{\"name\":\"Albert Einstein\",\"name_buf\":\"Albert\",\"height\":172},"
		"{\"name\":\"Usain Bolt\",\"name_buf\":\"Usain\",\"height\":195},"
		"{\"name\":\"Paavo Nurmi\",\"name_buf\":\"Paavo\",\"height\":174}"
		"]}";
	const struct obj_array expected = {
		.elements = {
			[0] = {
				.name = "Sim\303\263n Bol\303\255var",
				.name_buf = "Sim\303\263n",
				.height = 168 },
			[1] = {
				.name = "Muggsy Bogues",
				.name_buf = "Muggsy",
				.height = 160 },
			[2] = {
				.name = "Pel\303\251",
				.name_buf = "Pel\303\251",
				.height = 173 },
			[3] = {
				.name = "Hakeem Olajuwon",
				.name_buf = "Hakeem",
				.height = 213 },
			[4] = {
				.name = "Alex Honnold",
				.name_buf = "Alex",
				.height = 180 },
			[5] = {
				.name = "Hazel Findlay",
				.name_buf = "Hazel",
				.height = 157 },
			[6] = {
				.name = "Daila Ojeda",
				.name_buf = "Daila",
				.height = 158 },
			[7] = {
				.name = "Albert Einstein",
				.name_buf = "Albert",
				.height = 172 },
			[8] = {
				.name = "Usain Bolt",
				.name_buf = "Usain",
				.height = 195 },
			[9] = {
				.name = "Paavo Nurmi",
				.name_buf = "Paavo",
				.height = 174 },
		},
		.num_elements = 10,
	};
	int ret;

	ret = json_obj_parse(encoded, sizeof(encoded) - 1, obj_array_descr,
			     ARRAY_SIZE(obj_array_descr), &oa);

	zassert_equal(ret, (1 << ARRAY_SIZE(obj_array_descr)) - 1,
		      "Array of object fields not decoded correctly");
	zassert_equal(oa.num_elements, 10,
		      "Number of object fields not decoded correctly");

	for (int i = 0; i < expected.num_elements; i++) {
		zassert_true(!strcmp(oa.elements[i].name,
				     expected.elements[i].name),
			     "Element %d name not decoded correctly", i);
		zassert_equal(oa.elements[i].height,
			      expected.elements[i].height,
			      "Element %d height not decoded correctly", i);
	}
}

ZTEST(lib_json_test, test_json_2dim_arr_obj_encoding)
{
	struct obj_array_2dim obj_array_array_ts = {
		.objects_array_array = {
			[0] = {
				.elements = {
					[0] = {
						.name = "Sim\303\263n Bol\303\255var",
						.name_buf = "Sim\303\263n",
						.height = 168
					},
					[1] = {
						.name = "Pel\303\251",
						.name_buf = "Pel\303\251",
						.height = 173
					},
					[2] = {
						.name = "Usain Bolt",
						.name_buf = "Usain",
						.height = 195
					},
				},
				.num_elements = 3
			},
			[1] = {
				.elements = {
					[0] = {
						.name = "Muggsy Bogues",
						.name_buf = "Muggsy",
						.height = 160
					},
					[1] = {
						.name = "Hakeem Olajuwon",
						.name_buf = "Hakeem",
						.height = 213
					},
				},
				.num_elements = 2
			},
			[2] = {
				.elements = {
					[0] = {
						.name = "Alex Honnold",
						.name_buf = "Alex",
						.height = 180
					},
					[1] = {
						.name = "Hazel Findlay",
						.name_buf = "Hazel",
						.height = 157
					},
					[2] = {
						.name = "Daila Ojeda",
						.name_buf = "Daila",
						.height = 158
					},
					[3] = {
						.name = "Albert Einstein",
						.name_buf = "Albert",
						.height = 172
					},
				},
				.num_elements = 4
			},
		},
		.objects_array_array_len = 3,
	};
	char encoded[] = "{\"objects_array_array\":["
		"[{\"name\":\"Sim\303\263n Bol\303\255var\",\"name_buf\":\"Sim\303\263n\",\"height\":168},"
		 "{\"name\":\"Pel\303\251\",\"name_buf\":\"Pel\303\251\",\"height\":173},"
		 "{\"name\":\"Usain Bolt\",\"name_buf\":\"Usain\",\"height\":195}],"
		"[{\"name\":\"Muggsy Bogues\",\"name_buf\":\"Muggsy\",\"height\":160},"
		 "{\"name\":\"Hakeem Olajuwon\",\"name_buf\":\"Hakeem\",\"height\":213}],"
		"[{\"name\":\"Alex Honnold\",\"name_buf\":\"Alex\",\"height\":180},"
		 "{\"name\":\"Hazel Findlay\",\"name_buf\":\"Hazel\",\"height\":157},"
		 "{\"name\":\"Daila Ojeda\",\"name_buf\":\"Daila\",\"height\":158},"
		 "{\"name\":\"Albert Einstein\",\"name_buf\":\"Albert\",\"height\":172}]"
		"]}";
	char buffer[sizeof(encoded)];
	int ret;

	ret = json_obj_encode_buf(array_2dim_descr, ARRAY_SIZE(array_2dim_descr),
				  &obj_array_array_ts, buffer, sizeof(buffer));
	zassert_equal(ret, 0, "Encoding two-dimensional array returned error");
	zassert_str_equal(buffer, encoded,
			  "Encoded two-dimensional array is not consistent");
}

ZTEST(lib_json_test, test_json_2dim_arr_extra_obj_encoding)
{
	struct obj_array_2dim_extra obj_array_2dim_extra_ts = {
		.name = "Paavo Nurmi",
		.val = 123,
		.obj_array_2dim.objects_array_array = {
			[0] = {
				.elements = {
					[0] = {
						.name = "Sim\303\263n Bol\303\255var",
						.name_buf = "Sim\303\263n",
						.height = 168
					},
					[1] = {
						.name = "Pel\303\251",
						.name_buf = "Pel\303\251",
						.height = 173
					},
					[2] = {
						.name = "Usain Bolt",
						.name_buf = "Usain",
						.height = 195
					},
				},
				.num_elements = 3
			},
			[1] = {
				.elements = {
					[0] = {
						.name = "Muggsy Bogues",
						.name_buf = "Muggsy",
						.height = 160
					},
					[1] = {
						.name = "Hakeem Olajuwon",
						.name_buf = "Hakeem",
						.height = 213
					},
				},
				.num_elements = 2
			},
			[2] = {
				.elements = {
					[0] = {
						.name = "Alex Honnold",
						.name_buf = "Alex",
						.height = 180
					},
					[1] = {
						.name = "Hazel Findlay",
						.name_buf = "Hazel",
						.height = 157
					},
					[2] = {
						.name = "Daila Ojeda",
						.name_buf = "Daila",
						.height = 158
					},
					[3] = {
						.name = "Albert Einstein",
						.name_buf = "Albert",
						.height = 172
					},
				},
				.num_elements = 4
			},
		},
		.obj_array_2dim.objects_array_array_len = 3,
	};

	char encoded[] = "{\"name\":\"Paavo Nurmi\",\"val\":123,"
		"\"obj_array_2dim\":["
		"[{\"name\":\"Sim\303\263n Bol\303\255var\",\"name_buf\":\"Sim\303\263n\",\"height\":168},"
		 "{\"name\":\"Pel\303\251\",\"name_buf\":\"Pel\303\251\",\"height\":173},"
		 "{\"name\":\"Usain Bolt\",\"name_buf\":\"Usain\",\"height\":195}],"
		"[{\"name\":\"Muggsy Bogues\",\"name_buf\":\"Muggsy\",\"height\":160},"
		 "{\"name\":\"Hakeem Olajuwon\",\"name_buf\":\"Hakeem\",\"height\":213}],"
		"[{\"name\":\"Alex Honnold\",\"name_buf\":\"Alex\",\"height\":180},"
		 "{\"name\":\"Hazel Findlay\",\"name_buf\":\"Hazel\",\"height\":157},"
		 "{\"name\":\"Daila Ojeda\",\"name_buf\":\"Daila\",\"height\":158},"
		 "{\"name\":\"Albert Einstein\",\"name_buf\":\"Albert\",\"height\":172}]"
		"]}";
	char buffer[sizeof(encoded)];
	int ret;

	ret = json_obj_encode_buf(array_2dim_extra_descr, ARRAY_SIZE(array_2dim_extra_descr),
				  &obj_array_2dim_extra_ts, buffer, sizeof(buffer));
	zassert_equal(ret, 0, "Encoding two-dimensional extra array returned error");
	zassert_str_equal(buffer, encoded,
			  "Encoded two-dimensional extra array is not consistent");
}

ZTEST(lib_json_test, test_json_2dim_arr_extra_named_obj_encoding)
{
	struct obj_array_2dim_extra obj_array_2dim_extra_ts = {
		.name = "Paavo Nurmi",
		.val = 123,
		.obj_array_2dim.objects_array_array = {
			[0] = {
				.elements = {
					[0] = {
						.name = "Sim\303\263n Bol\303\255var",
						.name_buf = "Sim\303\263n",
						.height = 168
					},
					[1] = {
						.name = "Pel\303\251",
						.name_buf = "Pel\303\251",
						.height = 173
					},
					[2] = {
						.name = "Usain Bolt",
						.name_buf = "Usain",
						.height = 195
					},
				},
				.num_elements = 3
			},
			[1] = {
				.elements = {
					[0] = {
						.name = "Muggsy Bogues",
						.name_buf = "Muggsy",
						.height = 160
					},
					[1] = {
						.name = "Hakeem Olajuwon",
						.name_buf = "Hakeem",
						.height = 213
					},
				},
				.num_elements = 2
			},
			[2] = {
				.elements = {
					[0] = {
						.name = "Alex Honnold",
						.name_buf = "Alex",
						.height = 180
					},
					[1] = {
						.name = "Hazel Findlay",
						.name_buf = "Hazel",
						.height = 157
					},
					[2] = {
						.name = "Daila Ojeda",
						.name_buf = "Daila",
						.height = 158
					},
					[3] = {
						.name = "Albert Einstein",
						.name_buf = "Albert",
						.height = 172
					},
				},
				.num_elements = 4
			},
		},
		.obj_array_2dim.objects_array_array_len = 3,
	};

	char encoded[] = "{\"name\":\"Paavo Nurmi\",\"val\":123,"
		"\"data\":["
		"[{\"name\":\"Sim\303\263n Bol\303\255var\",\"name_buf\":\"Sim\303\263n\",\"height\":168},"
		 "{\"name\":\"Pel\303\251\",\"name_buf\":\"Pel\303\251\",\"height\":173},"
		 "{\"name\":\"Usain Bolt\",\"name_buf\":\"Usain\",\"height\":195}],"
		"[{\"name\":\"Muggsy Bogues\",\"name_buf\":\"Muggsy\",\"height\":160},"
		 "{\"name\":\"Hakeem Olajuwon\",\"name_buf\":\"Hakeem\",\"height\":213}],"
		"[{\"name\":\"Alex Honnold\",\"name_buf\":\"Alex\",\"height\":180},"
		 "{\"name\":\"Hazel Findlay\",\"name_buf\":\"Hazel\",\"height\":157},"
		 "{\"name\":\"Daila Ojeda\",\"name_buf\":\"Daila\",\"height\":158},"
		 "{\"name\":\"Albert Einstein\",\"name_buf\":\"Albert\",\"height\":172}]"
		"]}";
	char buffer[sizeof(encoded)];
	int ret;

	ret = json_obj_encode_buf(array_2dim_extra_named_descr,
				  ARRAY_SIZE(array_2dim_extra_named_descr),
				  &obj_array_2dim_extra_ts, buffer, sizeof(buffer));
	zassert_equal(ret, 0, "Encoding two-dimensional extra named array returned error");
	zassert_str_equal(buffer, encoded,
			  "Encoded two-dimensional extra named array is not consistent");
}

ZTEST(lib_json_test, test_json_2dim_obj_arr_decoding)
{
	struct obj_array_2dim oaa;
	char encoded[] = "{\"objects_array_array\":["
		"[{\"name\":\"Sim\303\263n Bol\303\255var\",\"name_buf\":\"Sim\303\263n\",\"height\":168},"
		 "{\"name\":\"Pel\303\251\",\"name_buf\":\"Pel\303\251\",\"height\":173},"
		 "{\"name\":\"Usain Bolt\",\"name_buf\":\"Usain\",\"height\":195}],"
		"[{\"name\":\"Muggsy Bogues\",\"name_buf\":\"Muggsy\",\"height\":160},"
		 "{\"name\":\"Hakeem Olajuwon\",\"name_buf\":\"Hakeem\",\"height\":213}],"
		"[{\"name\":\"Alex Honnold\",\"name_buf\":\"Alex\",\"height\":180},"
		 "{\"name\":\"Hazel Findlay\",\"name_buf\":\"Hazel\",\"height\":157},"
		 "{\"name\":\"Daila Ojeda\",\"name_buf\":\"Daila\",\"height\":158},"
		 "{\"name\":\"Albert Einstein\",\"name_buf\":\"Albert\",\"height\":172}]"
		"]}";
	const struct obj_array_2dim expected = {
		.objects_array_array = {
			[0] = {
				.elements = {
					[0] = {
						.name = "Sim\303\263n Bol\303\255var",
						.name_buf = "Sim\303\263n",
						.height = 168
					},
					[1] = {
						.name = "Pel\303\251",
						.name_buf = "Pel\303\251",
						.height = 173
					},
					[2] = {
						.name = "Usain Bolt",
						.name_buf = "Usain",
						.height = 195
					},
				},
				.num_elements = 3
			},
			[1] = {
				.elements = {
					[0] = {
						.name = "Muggsy Bogues",
						.name_buf = "Muggsy",
						.height = 160
					},
					[1] = {
						.name = "Hakeem Olajuwon",
						.name_buf = "Hakeem",
						.height = 213
					},
				},
				.num_elements = 2
			},
			[2] = {
				.elements = {
					[0] = {
						.name = "Alex Honnold",
						.name_buf = "Alex",
						.height = 180
					},
					[1] = {
						.name = "Hazel Findlay",
						.name_buf = "Hazel",
						.height = 157
					},
					[2] = {
						.name = "Daila Ojeda",
						.name_buf = "Daila",
						.height = 158
					},
					[3] = {
						.name = "Albert Einstein",
						.name_buf = "Albert",
						.height = 172
					},
				},
				.num_elements = 4
			},
		},
		.objects_array_array_len = 3,
	};
	int ret;

	ret = json_obj_parse(encoded, sizeof(encoded),
			     array_2dim_descr,
			     ARRAY_SIZE(array_2dim_descr),
			     &oaa);

	zassert_equal(ret, 1, "Array of arrays fields not decoded correctly");
	zassert_equal(oaa.objects_array_array_len, 3,
		      "Number of subarrays not decoded correctly");
	zassert_equal(oaa.objects_array_array[0].num_elements, 3,
		      "Number of object fields not decoded correctly");
	zassert_equal(oaa.objects_array_array[1].num_elements, 2,
		      "Number of object fields not decoded correctly");
	zassert_equal(oaa.objects_array_array[2].num_elements, 4,
		      "Number of object fields not decoded correctly");

	for (int i = 0; i < expected.objects_array_array_len; i++) {
		for (int j = 0; j < expected.objects_array_array[i].num_elements; j++) {
			zassert_true(!strcmp(oaa.objects_array_array[i].elements[j].name,
					     expected.objects_array_array[i].elements[j].name),
				     "Element [%d][%d] name not decoded correctly", i, j);
			zassert_true(!strcmp(oaa.objects_array_array[i].elements[j].name_buf,
					     expected.objects_array_array[i].elements[j].name_buf),
				     "Element [%d][%d] name array not decoded correctly", i, j);
			zassert_equal(oaa.objects_array_array[i].elements[j].height,
				      expected.objects_array_array[i].elements[j].height,
				      "Element [%d][%d] height not decoded correctly", i, j);
		}
	}
}

ZTEST(lib_json_test, test_json_string_array_size)
{
	int ret;
	struct elt elt_ts;
	char encoded[] = "{\"name_buf\":\"a12345678\"}";

	ret = json_obj_parse(encoded, sizeof(encoded),
			     elt_descr,
			     ARRAY_SIZE(elt_descr),
			     &elt_ts);

	/* size of name_buf is 10 */
	zassert_str_equal(elt_ts.name_buf,
			  "a12345678", "String not decoded correctly");
}

ZTEST(lib_json_test, test_json_string_array_empty)
{
	int ret;
	struct elt elt_ts;
	char encoded[] = "{\"name_buf\":\"\"}";

	ret = json_obj_parse(encoded, sizeof(encoded),
			     elt_descr,
			     ARRAY_SIZE(elt_descr),
			     &elt_ts);

	/* size of name_buf is 10 */
	zassert_str_equal(elt_ts.name_buf, "", "String not decoded correctly");
}

ZTEST(lib_json_test, test_json_string_array_max)
{
	int ret;
	struct elt elt_ts;
	char encoded[] = "{\"name_buf\":\"a123456789\"}";

	ret = json_obj_parse(encoded, sizeof(encoded),
			     elt_descr,
			     ARRAY_SIZE(elt_descr),
			     &elt_ts);

	/* string does not fit into name_buf */
	zassert_equal(ret, -EINVAL, "Decoding has to fail");
}

struct encoding_test {
	char *str;
	int result;
};

static void parse_harness(struct encoding_test encoded[], size_t size)
{
	struct test_struct ts;
	int ret;

	for (int i = 0; i < size; i++) {
		ret = json_obj_parse(encoded[i].str, strlen(encoded[i].str),
				     test_descr, ARRAY_SIZE(test_descr), &ts);
		zassert_equal(ret, encoded[i].result,
			      "Decoding '%s' result %d, expected %d",
			      encoded[i].str, ret, encoded[i].result);
	}
}

ZTEST(lib_json_test, test_json_invalid_string)
{
	struct encoding_test encoded[] = {
		{ "{\"some_string\":\"\\u@@@@\"}", -EINVAL },
		{ "{\"some_string\":\"\\uA@@@\"}", -EINVAL },
		{ "{\"some_string\":\"\\uAB@@\"}", -EINVAL },
		{ "{\"some_string\":\"\\uABC@\"}", -EINVAL },
		{ "{\"some_string\":\"\\X\"}", -EINVAL }
	};

	parse_harness(encoded, ARRAY_SIZE(encoded));
}

ZTEST(lib_json_test, test_json_invalid_bool)
{
	struct encoding_test encoded[] = {
		{ "{\"some_bool\":truffle }", -EINVAL},
		{ "{\"some_bool\":fallacy }", -EINVAL},
	};

	parse_harness(encoded, ARRAY_SIZE(encoded));
}

ZTEST(lib_json_test, test_json_invalid_null)
{
	struct encoding_test encoded[] = {
		/* Parser will recognize 'null', but refuse to decode it */
		{ "{\"some_string\":null }", -EINVAL},
		/* Null spelled wrong */
		{ "{\"some_string\":nutella }", -EINVAL},
	};

	parse_harness(encoded, ARRAY_SIZE(encoded));
}

ZTEST(lib_json_test, test_json_invalid_number)
{
	struct encoding_test encoded[] = {
		{ "{\"some_int\":xxx }", -EINVAL},
	};

	parse_harness(encoded, ARRAY_SIZE(encoded));
}

ZTEST(lib_json_test, test_json_missing_quote)
{
	struct test_struct ts;
	char encoded[] = "{\"some_string";
	int ret;

	ret = json_obj_parse(encoded, sizeof(encoded) - 1, test_descr,
			     ARRAY_SIZE(test_descr), &ts);
	zassert_equal(ret, -EINVAL, "Decoding has to fail");
}

ZTEST(lib_json_test, test_json_wrong_token)
{
	struct test_struct ts;
	char encoded[] = "{\"some_string\",}";
	int ret;

	ret = json_obj_parse(encoded, sizeof(encoded) - 1, test_descr,
			     ARRAY_SIZE(test_descr), &ts);
	zassert_equal(ret, -EINVAL, "Decoding has to fail");
}

ZTEST(lib_json_test, test_json_item_wrong_type)
{
	struct test_struct ts;
	char encoded[] = "{\"some_string\":false}";
	int ret;

	ret = json_obj_parse(encoded, sizeof(encoded) - 1, test_descr,
			     ARRAY_SIZE(test_descr), &ts);
	zassert_equal(ret, -EINVAL, "Decoding has to fail");
}

ZTEST(lib_json_test, test_json_key_not_in_descr)
{
	struct test_struct ts;
	char encoded[] = "{\"key_not_in_descr\":123456}";
	int ret;

	ret = json_obj_parse(encoded, sizeof(encoded) - 1, test_descr,
			     ARRAY_SIZE(test_descr), &ts);
	zassert_equal(ret, 0, "No items should be decoded");
}

ZTEST(lib_json_test, test_json_escape)
{
	char buf[42];
	char string[] = "\"abc"
			"\\1`23"
			"\bf'oo"
			"\fbar"
			"\nbaz"
			"\rquux"
			"\tfred\"";
	const char *expected = "\\\"abc"
			       "\\\\1`23"
			       "\\bf'oo"
			       "\\fbar"
			       "\\nbaz"
			       "\\rquux"
			       "\\tfred\\\"";
	size_t len;
	ssize_t ret;

	strncpy(buf, string, sizeof(buf) - 1);
	len = strlen(buf);

	ret = json_escape(buf, &len, sizeof(buf));
	zassert_equal(ret, 0, "Escape did not succeed");
	zassert_equal(len, sizeof(buf) - 1,
		      "Escaped length not computed correctly");
	zassert_str_equal(buf, expected, "Escaped value is not correct");
}

/* Edge case: only one character, which must be escaped. */
ZTEST(lib_json_test, test_json_escape_one)
{
	char buf[3] = {'\t', '\0', '\0'};
	const char *expected = "\\t";
	size_t len = strlen(buf);
	ssize_t ret;

	ret = json_escape(buf, &len, sizeof(buf));
	zassert_equal(ret, 0,
		      "Escaping one character did not succeed");
	zassert_equal(len, sizeof(buf) - 1,
		      "Escaping one character length is not correct");
	zassert_str_equal(buf, expected, "Escaped value is not correct");
}

ZTEST(lib_json_test, test_json_escape_empty)
{
	char empty[] = "";
	size_t len = sizeof(empty) - 1;
	ssize_t ret;

	ret = json_escape(empty, &len, sizeof(empty));
	zassert_equal(ret, 0, "Escaping empty string not successful");
	zassert_equal(len, 0, "Length of empty escaped string is not zero");
	zassert_equal(empty[0], '\0', "Empty string does not remain empty");
}

ZTEST(lib_json_test, test_json_escape_no_op)
{
	char nothing_to_escape[] = "hello,world:!";
	const char *expected = "hello,world:!";
	size_t len = sizeof(nothing_to_escape) - 1;
	ssize_t ret;

	ret = json_escape(nothing_to_escape, &len, sizeof(nothing_to_escape));
	zassert_equal(ret, 0, "Escape no-op not handled correctly");
	zassert_equal(len, sizeof(nothing_to_escape) - 1,
		      "Changed length of already escaped string");
	zassert_str_equal(nothing_to_escape, expected,
			  "Altered string with nothing to escape");
}

ZTEST(lib_json_test, test_json_escape_bounds_check)
{
	char not_enough_memory[] = "\tfoo";
	size_t len = sizeof(not_enough_memory) - 1;
	ssize_t ret;

	ret = json_escape(not_enough_memory, &len, sizeof(not_enough_memory));
	zassert_equal(ret, -ENOMEM, "Bounds check failed");
}

ZTEST(lib_json_test, test_json_encode_bounds_check)
{
	struct number {
		uint32_t val;
	} str = { 0 };
	const struct json_obj_descr descr[] = {
		JSON_OBJ_DESCR_PRIM(struct number, val, JSON_TOK_NUMBER),
	};
	/* Encodes to {"val":0}\0 for a total of 10 bytes */
	uint8_t buf[10];
	ssize_t ret = json_obj_encode_buf(descr, ARRAY_SIZE(descr),
					  &str, buf, 10);
	zassert_equal(ret, 0, "Encoding failed despite large enough buffer");
	zassert_equal(strlen(buf), 9, "Encoded string length mismatch");

	ret = json_obj_encode_buf(descr, ARRAY_SIZE(descr),
				     &str, buf, 9);
	zassert_equal(ret, -ENOMEM, "Bounds check failed");
}

ZTEST(lib_json_test, test_large_descriptor)
{
	struct large_struct {
		int int0;
		int int1;
		int int2;
		int int3;
		int int4;
		int int5;
		int int6;
		int int7;
		int int8;
		int int9;
		int int10;
		int int11;
		int int12;
		int int13;
		int int14;
		int int15;
		int int16;
		int int17;
		int int18;
		int int19;
		int int20;
		int int21;
		int int22;
		int int23;
		int int24;
		int int25;
		int int26;
		int int27;
		int int28;
		int int29;
		int int30;
		int int31;
		int int32;
		int int33;
		int int34;
		int int35;
		int int36;
		int int37;
		int int38;
		int int39;
	};

	static const struct json_obj_descr large_descr[] = {
		JSON_OBJ_DESCR_PRIM(struct large_struct, int0, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct large_struct, int1, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct large_struct, int2, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct large_struct, int3, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct large_struct, int4, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct large_struct, int5, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct large_struct, int6, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct large_struct, int7, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct large_struct, int8, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct large_struct, int9, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct large_struct, int10, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct large_struct, int11, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct large_struct, int12, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct large_struct, int13, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct large_struct, int14, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct large_struct, int15, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct large_struct, int16, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct large_struct, int17, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct large_struct, int18, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct large_struct, int19, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct large_struct, int20, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct large_struct, int21, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct large_struct, int22, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct large_struct, int23, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct large_struct, int24, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct large_struct, int25, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct large_struct, int26, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct large_struct, int27, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct large_struct, int28, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct large_struct, int29, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct large_struct, int30, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct large_struct, int31, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct large_struct, int32, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct large_struct, int33, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct large_struct, int34, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct large_struct, int35, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct large_struct, int36, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct large_struct, int37, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct large_struct, int38, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct large_struct, int39, JSON_TOK_NUMBER),
	};
	char encoded[] = "{"
		"\"int1\": 1,"
		"\"int21\": 21,"
		"\"int31\": 31,"
		"\"int39\": 39"
		"}";

	struct large_struct ls;

	int64_t ret = json_obj_parse(encoded, sizeof(encoded) - 1, large_descr,
				     ARRAY_SIZE(large_descr), &ls);

	zassert_false(ret < 0, "json_obj_parse returned error %lld", ret);
	zassert_false(ret & ((int64_t)1 << 2), "Field int2 erroneously decoded");
	zassert_false(ret & ((int64_t)1 << 35), "Field int35 erroneously decoded");
	zassert_true(ret & ((int64_t)1 << 1), "Field int1 not decoded");
	zassert_true(ret & ((int64_t)1 << 21), "Field int21 not decoded");
	zassert_true(ret & ((int64_t)1 << 31), "Field int31 not decoded");
	zassert_true(ret & ((int64_t)1 << 39), "Field int39 not decoded");
}

ZTEST(lib_json_test, test_json_encoded_object_tok_encoding)
{
	static const char encoded[] = "{"
		"\"encoded_obj\":{\"test\":{\"nested\":\"yes\"}},"
		"\"encoded_obj_array\":["
		"{\"array_1\":{\"nested\":\"yes\"}},"
		"{\"array_2\":{\"nested\":\"yes\"}},"
		"{\"array_3\":{\"nested\":\"yes\"}}],"
		"\"ok\":1234}";
	const struct test_json_tok_encoded_obj obj = {
		.encoded_obj = "{\"test\":{\"nested\":\"yes\"}}",
		.encoded_obj_array[0] = "{\"array_1\":{\"nested\":\"yes\"}}",
		.encoded_obj_array[1] = "{\"array_2\":{\"nested\":\"yes\"}}",
		.encoded_obj_array[2] = "{\"array_3\":{\"nested\":\"yes\"}}",
		.encoded_obj_array_len = 3,
		.ok = 1234,
	};
	char buffer[sizeof(encoded)];
	int ret;

	ret = json_obj_encode_buf(test_json_tok_encoded_obj_descr,
				  ARRAY_SIZE(test_json_tok_encoded_obj_descr), &obj, buffer,
				  sizeof(buffer));

	zassert_equal(ret, 0, "Encoding function failed");
	zassert_mem_equal(buffer, encoded, sizeof(encoded), "Encoded contents not consistent");
}

ZTEST(lib_json_test, test_json_array_alignment)
{
	char encoded[] = "{"
	"\"array\": [ "
	"{ \"int1\": 1, "
	"\"int2\": 2, "
	"\"int3\":  3 }, "
	"{ \"int1\": 4, "
	"\"int2\": 5, "
	"\"int3\": 6 } "
	"] "
	"}";

	struct test_outer o;
	int64_t ret = json_obj_parse(encoded, sizeof(encoded) - 1, outer_descr,
				     ARRAY_SIZE(outer_descr), &o);

	zassert_false(ret < 0, "json_obj_parse returned error %lld", ret);
	zassert_equal(o.num_elements, 2, "Number of elements not decoded correctly");

	zassert_equal(o.array[0].int1, 1, "Element 0 int1 not decoded correctly");
	zassert_equal(o.array[0].int2, 2, "Element 0 int2 not decoded correctly");
	zassert_equal(o.array[0].int3, 3, "Element 0 int3 not decoded correctly");

	zassert_equal(o.array[1].int1, 4, "Element 1 int1 not decoded correctly");
	zassert_equal(o.array[1].int2, 5, "Element 1 int2 not decoded correctly");
	zassert_equal(o.array[1].int3, 6, "Element 1 int3 not decoded correctly");
}

ZTEST(lib_json_test, test_json_array_alignment_bool)
{
	char encoded[] = "{\"array\":["
			 "{\"bool1\":true,\"int1\":1,\"bool2\":false},"
			 "{\"bool1\":true,\"int1\":2,\"bool2\":false}"
			 "]}";

	struct test_alignment_bool o = { 0 };
	int64_t ret = json_obj_parse(encoded, sizeof(encoded) - 1, alignment_bool_descr,
				     ARRAY_SIZE(alignment_bool_descr), &o);

	zassert_false(ret < 0, "json_obj_parse returned error %lld", ret);
	zassert_equal(o.num_elements, 2, "Number of elements not decoded correctly");

	zassert_equal(o.array[0].bool1, true, "Element 0 bool1 not decoded correctly");
	zassert_equal(o.array[0].int1, 1, "Element 0 int1 not decoded correctly");
	zassert_equal(o.array[0].bool2, false, "Element 0 bool2 not decoded correctly");

	zassert_equal(o.array[1].bool1, true, "Element 1 bool1 not decoded correctly");
	zassert_equal(o.array[1].int1, 2, "Element 1 int1 not decoded correctly");
	zassert_equal(o.array[1].bool2, false, "Element 1 bool2 not decoded correctly");
}

ZTEST(lib_json_test, test_json_invalid_int)
{
	struct sub_test {
		char str[30];
		int result;
	};
	struct sub_test encoded[] = {
		{ "{\"int8_cero\":128}", -EINVAL },
		{ "{\"int8_cero\":-129}", -EINVAL },
		{ "{\"uint8_cero\":257}", -EINVAL },
		{ "{\"int16_cero\":32768}", -EINVAL },
		{ "{\"int16_cero\":-32769}", -EINVAL },
		{ "{\"uint16_cero\":65536}", -EINVAL },
	};

	struct test_int_limits ts;
	int ret;

	for (int i = 0; i < ARRAY_SIZE(encoded); i++) {
		ret = json_obj_parse(encoded[i].str, strlen(encoded[i].str),
				     obj_limits_descr, ARRAY_SIZE(obj_limits_descr), &ts);
		zassert_equal(ret, encoded[i].result,
			      "Decoding '%s' result %d, expected %d",
			      encoded[i].str, ret, encoded[i].result);
	}
}

ZTEST(lib_json_test, test_json_enums)
{
	int ret = 0;
	char encoded[] = "{\"i8\":-128,"
			 "\"u8\":255,"
			 "\"i16\":-32768,"
			 "\"u16\":65535,"
			 "\"i32\":-2147483648,"
			 "\"u32\":4294967295"
			 "}";
	char buffer[sizeof(encoded)];
	struct test_enums enums_decoded = {0};
	struct test_enums enums = {0};

	enums.i8 = I8_MIN;
	enums.u8 = U8_MAX;
	enums.i16 = I16_MIN;
	enums.u16 = U16_MAX;
	enums.i32 = I32_MIN;
	enums.u32 = U32_MAX;

	ret = json_obj_encode_buf(enums_descr, ARRAY_SIZE(enums_descr),
				&enums, buffer, sizeof(buffer));
	ret = json_obj_parse(encoded, sizeof(encoded) - 1, enums_descr,
			     ARRAY_SIZE(enums_descr), &enums_decoded);

	zassert_str_equal(encoded, buffer,
			  "Enums not encoded correctly");
	zassert_true(!memcmp(&enums, &enums_decoded, sizeof(enums)),
		     "Enums not decoded correctly");
}

ZTEST(lib_json_test, test_json_string_nullptr)
{
	int ret = 0;

	struct test_struct ts = {0};
	char *buffer;
	size_t len;

	len = json_calc_encoded_len(test_descr, ARRAY_SIZE(test_descr), &ts);
	zassert(len > 0, "encoded size incorrect");

	buffer = alloca(len + 1);
	ret = json_obj_encode_buf(test_descr, ARRAY_SIZE(test_descr), &ts, buffer, len + 1);
	zassert_equal(ret, 0, "Encoding function failed");

	ret = json_obj_parse(buffer, len, test_descr, ARRAY_SIZE(test_descr), &ts);
	zassert_equal(ret, (1 << ARRAY_SIZE(test_descr)) - 1, "Not all fields decoded correctly");
	zassert_str_equal(ts.some_string, "", "String not decoded correctly");
}

ZTEST(lib_json_test, test_json_mixed_arr_parse)
{
	struct test_mixed_arr arr = {0};
	char json[] = "[\"msg\", 123456, {\"nested_int\":42,\"nested_bool\":true,"
		      "\"nested_string\":\"abc\","
		      "\"nested_string_buf\":\"buf\",\"nested_int8\":1,\"nested_uint8\":2,"
		      "\"nested_int64\":3,\"nested_uint64\":4}, [10,20,30], \"ok\"]";
	int ret = json_mixed_arr_parse(json, strlen(json),
				       test_mixed_arr_descr,
				       ARRAY_SIZE(test_mixed_arr_descr), &arr);

	zassert_equal(ret, 5, "Should parse 5 elements");
	zassert_str_equal(arr.msg_type, "msg", NULL);
	zassert_equal(arr.dev_id, 123456, NULL);
	zassert_equal(arr.nested.nested_int, 42, NULL);
	zassert_equal(arr.arr_len, 3, NULL);
	zassert_equal(arr.arr[0], 10, NULL);
	zassert_equal(arr.arr[1], 20, NULL);
	zassert_equal(arr.arr[2], 30, NULL);
	zassert_str_equal(arr.status_buf, "ok", NULL);
}

ZTEST(lib_json_test, test_json_mixed_arr_encode)
{
	char buf[256];
	struct test_mixed_arr pkt = {0};
	struct test_mixed_arr arr = {
		.msg_type = "msg",
		.dev_id = 123456,
		.nested = {
			.nested_int = 42,
			.nested_bool = true,
			.nested_string = "abc",
			.nested_string_buf = "buf",
			.nested_int8 = 1,
			.nested_uint8 = 2,
			.nested_int64 = 3,
			.nested_uint64 = 4,
		},
		.arr = {10, 20, 30},
		.arr_len = 3,
		.status_buf = "ok",
		.count = 5
	};
	int ret = json_mixed_arr_encode_buf(test_mixed_arr_descr,
					    ARRAY_SIZE(test_mixed_arr_descr),
					    &arr, buf, sizeof(buf));

	zassert_equal(ret, 0, NULL);

	ret = json_mixed_arr_parse(buf, strlen(buf),
				   test_mixed_arr_descr,
				   ARRAY_SIZE(test_mixed_arr_descr), &pkt);

	zassert_equal(ret, 5, NULL);
	zassert_str_equal(pkt.msg_type, "msg", NULL);
	zassert_equal(pkt.dev_id, 123456, NULL);
	zassert_equal(pkt.arr[0], 10, NULL);
	zassert_str_equal(pkt.status_buf, "ok", NULL);
}

ZTEST(lib_json_test, test_json_mixed_arr_empty)
{
	struct test_mixed_arr arr = {0};
	char json[] = "[]";
	int ret = json_mixed_arr_parse(json, strlen(json),
		test_mixed_arr_descr, 0, &arr);

	zassert_equal(ret, 0, NULL);
}

ZTEST(lib_json_test, test_json_mixed_arr_calc_len)
{
	char buf[256];
	ssize_t calc_len;
	int ret;
	struct test_mixed_arr arr = {
		.msg_type = "msg",
		.dev_id = 123456,
		.nested = {
			.nested_int = 42,
			.nested_bool = true,
			.nested_string = "abc",
			.nested_string_buf = "buf",
			.nested_int8 = 1,
			.nested_uint8 = 2,
			.nested_int64 = 3,
			.nested_uint64 = 4,
		},
		.arr = {10, 20, 30},
		.arr_len = 3,
		.status_buf = "ok",
		.count = 5
	};

	calc_len = json_calc_mixed_arr_len(test_mixed_arr_descr,
					   ARRAY_SIZE(test_mixed_arr_descr),
					   &arr);
	ret = json_mixed_arr_encode_buf(test_mixed_arr_descr,
					ARRAY_SIZE(test_mixed_arr_descr),
					&arr, buf, sizeof(buf));
	zassert_equal(ret, 0, NULL);

	zassert_equal(calc_len, (ssize_t)strlen(buf), "Length mismatch");
}

/**
 * @brief Test that escape sequences don't duplicate on encode/decode cycles
 *
 * This test specifically targets the bug where backslashes in escape sequences
 * like \n, \t, etc. get duplicated with each encode/decode cycle.
 */
ZTEST(lib_json_test, test_json_escape_sequence_stability)
{
	struct escape_test_data original = {
		.string_value = "Line1\nLine2\tTabbed",
		.integer_value = 42,
	};
	strncpy(original.string_value, "Line1\nLine2\tTabbed", sizeof(original.string_value) - 1);
	strncpy(original.string_buf, "Text with\nnewline and\ttab",
		sizeof(original.string_buf) - 1);

	char buffer[512];
	struct escape_test_data decoded = {0};
	int ret;

	/* Encode -> Decode -> Encode -> Decode cycle */

	/* First encode */
	ret = json_obj_encode_buf(escape_test_descr, ARRAY_SIZE(escape_test_descr), &original,
				  buffer, sizeof(buffer));
	zassert_equal(ret, 0, "First encoding failed");

	/* First decode */
	ret = json_obj_parse(buffer, strlen(buffer), escape_test_descr,
			     ARRAY_SIZE(escape_test_descr), &decoded);
	zassert_equal(ret, (1 << ARRAY_SIZE(escape_test_descr)) - 1, "First decoding failed");

	/* Verify first decode matches original */
	zassert_str_equal(decoded.string_value, original.string_value,
			  "String value changed after first decode");
	zassert_str_equal(decoded.string_buf, original.string_buf,
			  "String buffer changed after first decode");
	zassert_equal(decoded.integer_value, original.integer_value,
		      "Integer value changed after first decode");

	/* Second encode */
	ret = json_obj_encode_buf(escape_test_descr, ARRAY_SIZE(escape_test_descr), &decoded,
				  buffer, sizeof(buffer));
	zassert_equal(ret, 0, "Second encoding failed");

	/* Second decode */
	struct escape_test_data decoded2 = {0};

	ret = json_obj_parse(buffer, strlen(buffer), escape_test_descr,
			     ARRAY_SIZE(escape_test_descr), &decoded2);
	zassert_equal(ret, (1 << ARRAY_SIZE(escape_test_descr)) - 1, "Second decoding failed");

	/* CRITICAL: Verify strings remain unchanged after multiple cycles */
	zassert_str_equal(decoded2.string_value, original.string_value,
			  "String value corrupted after encode/decode cycle");
	zassert_str_equal(decoded2.string_buf, original.string_buf,
			  "String buffer corrupted after encode/decode cycle");
	zassert_equal(decoded2.integer_value, original.integer_value,
		      "Integer value changed after encode/decode cycle");
}

/**
 * @brief Test specific escape sequences individually
 */
ZTEST(lib_json_test, test_json_specific_escape_sequences)
{
	struct test_case {
		const char *input;
		const char *description;
	};

	struct test_case test_cases[] = {
		{"Simple newline\n", "newline"},
		{"Multiple\nnew\nlines\n", "multiple newlines"},
		{"Tab\tseparated", "tab"},
		{"Mixed\n\tboth", "mixed newline and tab"},
		{"Backslash\\character", "backslash"},
		{"Quote\"test", "quote"},
		{"Backspace\btest", "backspace"},
		{"Form\ffeed", "form feed"},
		{"Carriage\rreturn", "carriage return"},
		{"Slash/test", "slash"},
	};

	for (size_t i = 0; i < ARRAY_SIZE(test_cases); i++) {
		struct escape_test_data test_data = {
			.integer_value = (int)i,
		};
		strncpy(test_data.string_value, test_cases[i].input,
			sizeof(test_data.string_value) - 1);
		strncpy(test_data.string_buf, test_cases[i].input,
			sizeof(test_data.string_buf) - 1);

		char buffer[256];
		struct escape_test_data decoded = {0};
		int ret;

		/* Encode */
		ret = json_obj_encode_buf(escape_test_descr, ARRAY_SIZE(escape_test_descr),
					  &test_data, buffer, sizeof(buffer));
		zassert_equal(ret, 0, "Encoding failed for %s", test_cases[i].description);

		/* Decode */
		ret = json_obj_parse(buffer, strlen(buffer), escape_test_descr,
				     ARRAY_SIZE(escape_test_descr), &decoded);
		zassert_equal(ret, (1 << ARRAY_SIZE(escape_test_descr)) - 1,
			      "Decoding failed for %s", test_cases[i].description);

		/* Verify string is preserved */
		zassert_str_equal(decoded.string_value, test_data.string_value,
				  "Escape sequence corrupted for %s", test_cases[i].description);
		zassert_str_equal(decoded.string_buf, test_data.string_buf,
				  "String buffer corrupted for %s", test_cases[i].description);
	}
}

/**
 * @brief Test that encoded JSON contains proper escape sequences
 */
ZTEST(lib_json_test, test_json_escape_encoding_correctness)
{
	struct escape_test_data test_data = {
		.string_value = "Test\nLine",
		.integer_value = 123,
	};
	strncpy(test_data.string_buf, "Buffer\tTest", sizeof(test_data.string_buf) - 1);

	char buffer[256];
	int ret;

	/* Encode the data */
	ret = json_obj_encode_buf(escape_test_descr, ARRAY_SIZE(escape_test_descr), &test_data,
				  buffer, sizeof(buffer));
	zassert_equal(ret, 0, "Encoding failed");

	/* Verify the encoded JSON contains proper escape sequences */
	zassert_not_null(strstr(buffer, "\\n"), "Newline not properly escaped in JSON");
	zassert_not_null(strstr(buffer, "\\t"), "Tab not properly escaped in JSON");

	/* The encoded JSON should NOT contain raw control characters */
	zassert_is_null(strchr(buffer, '\n'), "Raw newline found in encoded JSON");
	zassert_is_null(strchr(buffer, '\t'), "Raw tab found in encoded JSON");
}

/**
 * @brief Test multiple encode/decode cycles to catch gradual corruption
 */
ZTEST(lib_json_test, test_json_multiple_cycle_stability)
{
	struct escape_test_data original = {
		.string_value = "Start\nMiddle\tEnd",
		.integer_value = 99,
	};
	strncpy(original.string_value, "Start\nMiddle\tEnd", sizeof(original.string_value) - 1);
	strncpy(original.string_buf, "Cyclic\ntest\tdata", sizeof(original.string_buf) - 1);

	char buffer[512];
	struct escape_test_data current = {0};
	int ret;

	/* Initialize current by copying from original */
	current.integer_value = original.integer_value;
	strncpy(current.string_value, original.string_value, sizeof(current.string_value));
	strncpy(current.string_buf, original.string_buf, sizeof(current.string_buf));

	/* Run multiple encode/decode cycles */
	for (int cycle = 0; cycle < 5; cycle++) {
		/* Encode */
		ret = json_obj_encode_buf(escape_test_descr, ARRAY_SIZE(escape_test_descr),
					  &current, buffer, sizeof(buffer));
		zassert_equal(ret, 0, "Encoding failed at cycle %d", cycle);

		/* Decode into a fresh struct */
		struct escape_test_data next = {0};

		ret = json_obj_parse(buffer, strlen(buffer), escape_test_descr,
				     ARRAY_SIZE(escape_test_descr), &next);
		zassert_equal(ret, (1 << ARRAY_SIZE(escape_test_descr)) - 1,
			      "Decoding failed at cycle %d", cycle);

		/* Verify no corruption */
		zassert_str_equal(next.string_value, original.string_value,
				  "String corrupted after %d cycles", cycle + 1);
		zassert_str_equal(next.string_buf, original.string_buf,
				  "String buffer corrupted after %d cycles", cycle + 1);
		zassert_equal(next.integer_value, original.integer_value,
			      "Integer corrupted after %d cycles", cycle + 1);

		/* Prepare for next cycle */
		current.integer_value = next.integer_value;
		strncpy(current.string_value, next.string_value, sizeof(current.string_value));
		strncpy(current.string_buf, next.string_buf, sizeof(current.string_buf));
	}
}

/**
 * @brief Test the exact scenario from the bug report
 */
ZTEST(lib_json_test, test_json_escape_sequence_regression)
{
	/* This test reproduces the exact scenario described in GitHub issue #88552 */
	struct escape_test_data config_data = {
		.string_value = "some_string\n",
		.integer_value = 69,
	};
	strncpy(config_data.string_value, "some_string\n", sizeof(config_data.string_value) - 1);
	strncpy(config_data.string_buf, "config\nvalue", sizeof(config_data.string_buf) - 1);

	char encoded_json[256];
	struct escape_test_data decoded_data = {0};
	int ret;

	/* Simulate multiple boot cycles with encode/decode - this is where the bug manifests */
	for (int boot_cycle = 0; boot_cycle < 3; boot_cycle++) {
		/* Encode (simulate saving to flash) */
		ret = json_obj_encode_buf(escape_test_descr, ARRAY_SIZE(escape_test_descr),
					  &config_data, encoded_json, sizeof(encoded_json));
		zassert_equal(ret, 0, "Encode failed at boot cycle %d", boot_cycle);

		/* Decode (simulate reading from flash) */
		ret = json_obj_parse(encoded_json, strlen(encoded_json), escape_test_descr,
				     ARRAY_SIZE(escape_test_descr), &decoded_data);
		zassert_equal(ret, (1 << ARRAY_SIZE(escape_test_descr)) - 1,
			      "Decode failed at boot cycle %d", boot_cycle);

		/* Update for next cycle (simulate config change) */
		decoded_data.integer_value++;
		config_data = decoded_data;
	}

	/* After multiple cycles, the string should be unchanged */
	zassert_str_equal(config_data.string_value, "some_string\n",
			  "String value corrupted after multiple boot cycles");
	zassert_str_equal(config_data.string_buf, "config\nvalue",
			  "String buffer corrupted after multiple boot cycles");

	/* Additional verification: check that backslashes didn't duplicate */
	const char *expected_value = "some_string\n";
	const char *expected_buf = "config\nvalue";

	zassert_str_equal(config_data.string_value, expected_value,
			  "Escape sequence regression detected in string_value");
	zassert_str_equal(config_data.string_buf, expected_buf,
			  "Escape sequence regression detected in string_buf");
}

/**
 * @brief Test to detect backslash duplication specifically
 */
ZTEST(lib_json_test, test_json_backslash_duplication)
{
	/* This test specifically checks for the backslash duplication bug */
	struct escape_test_data test_data = {
		.string_value = "test\nstring",
		.integer_value = 1,
	};
	strncpy(test_data.string_buf, "buffer\ncontent", sizeof(test_data.string_buf) - 1);

	char buffer1[256], buffer2[256];
	struct escape_test_data decoded1 = {0}, decoded2 = {0};
	int ret;

	/* First encode/decode cycle */
	ret = json_obj_encode_buf(escape_test_descr, ARRAY_SIZE(escape_test_descr), &test_data,
				  buffer1, sizeof(buffer1));
	zassert_equal(ret, 0, "First encode failed");

	ret = json_obj_parse(buffer1, strlen(buffer1), escape_test_descr,
			     ARRAY_SIZE(escape_test_descr), &decoded1);
	zassert_equal(ret, (1 << ARRAY_SIZE(escape_test_descr)) - 1, "First decode failed");

	/* Second encode/decode cycle */
	ret = json_obj_encode_buf(escape_test_descr, ARRAY_SIZE(escape_test_descr), &decoded1,
				  buffer2, sizeof(buffer2));
	zassert_equal(ret, 0, "Second encode failed");

	ret = json_obj_parse(buffer2, strlen(buffer2), escape_test_descr,
			     ARRAY_SIZE(escape_test_descr), &decoded2);
	zassert_equal(ret, (1 << ARRAY_SIZE(escape_test_descr)) - 1, "Second decode failed");

	/* The critical assertion: strings should be identical across cycles */
	zassert_str_equal(decoded1.string_value, decoded2.string_value,
			  "Backslash duplication detected in string_value");
	zassert_str_equal(decoded1.string_buf, decoded2.string_buf,
			  "Backslash duplication detected in string_buf");
	zassert_str_equal(test_data.string_value, decoded2.string_value,
			  "Original string value not preserved");
	zassert_str_equal(test_data.string_buf, decoded2.string_buf,
			  "Original string buffer not preserved");
}

ZTEST(lib_json_test, test_json_quote_escaping_encoding)
{
	struct escape_test_data test_data = {
		.integer_value = 42,
	};
	strncpy(test_data.string_value, "Text with \"quotes\" inside",
		sizeof(test_data.string_value) - 1);
	strncpy(test_data.string_buf, "Buffer with \"quotes\"", sizeof(test_data.string_buf) - 1);

	char buffer[256];
	int ret;

	/* Encode the data */
	ret = json_obj_encode_buf(escape_test_descr, ARRAY_SIZE(escape_test_descr), &test_data,
				  buffer, sizeof(buffer));
	zassert_equal(ret, 0, "Encoding failed");

	/* Verify the encoded JSON contains escaped quotes */
	zassert_not_null(strstr(buffer, "\\\""), "Quotes not properly escaped in JSON output");

	/* The encoded JSON should NOT contain the pattern: "... "..." (unescaped quotes in content)
	 * Look for the specific escaped quote pattern
	 */
	const char *escaped_quote = strstr(buffer, "\\\"");

	zassert_not_null(escaped_quote, "No escaped quotes found in encoded JSON");

	/* Make sure we found at least 2 escaped quotes (one for each field) */
	const char *second_escaped_quote = strstr(escaped_quote + 1, "\\\"");

	zassert_not_null(second_escaped_quote, "Should have multiple escaped quotes in JSON");

	/* Verify the overall structure looks correct */
	zassert_not_null(strstr(buffer, "\"string_value\""), "string_value field missing");
	zassert_not_null(strstr(buffer, "\"string_buf\""), "string_buf field missing");
	zassert_not_null(strstr(buffer, "\"integer_value\""), "integer_value field missing");
}

/**
 * @brief Test that escaped quotes are properly unescaped during decoding
 */
ZTEST(lib_json_test, test_json_quote_unescaping_decoding)
{
	/* JSON with escaped quotes */
	char encoded_json[] = "{\"string_value\":\"Text with \\\"quotes\\\" inside\","
			      "\"string_buf\":\"Buffer with \\\"quotes\\\"\","
			      "\"integer_value\":42}";

	struct escape_test_data decoded = {0};
	int ret;

	/* Decode the JSON */
	ret = json_obj_parse(encoded_json, strlen(encoded_json), escape_test_descr,
			     ARRAY_SIZE(escape_test_descr), &decoded);
	zassert_equal(ret, (1 << ARRAY_SIZE(escape_test_descr)) - 1, "Decoding failed");

	/* Verify quotes are properly unescaped */
	zassert_str_equal(decoded.string_value, "Text with \"quotes\" inside",
			  "Quotes not properly unescaped in string_value");
	zassert_str_equal(decoded.string_buf, "Buffer with \"quotes\"",
			  "Quotes not properly unescaped in string_buf");
}

/**
 * @brief Test complete round-trip: quote -> escaped -> unescaped -> original
 */
ZTEST(lib_json_test, test_json_quote_round_trip)
{
	struct escape_test_data original = {
		.string_value = "Start \"middle\" end",
		.integer_value = 123,
	};
	strncpy(original.string_buf, "Quote: \"test\" here", sizeof(original.string_buf) - 1);

	char buffer[256];
	struct escape_test_data decoded = {0};
	int ret;

	/* Encode (quotes should become \") */
	ret = json_obj_encode_buf(escape_test_descr, ARRAY_SIZE(escape_test_descr), &original,
				  buffer, sizeof(buffer));
	zassert_equal(ret, 0, "Encoding failed");

	/* Verify encoding produced escaped quotes */
	zassert_not_null(strstr(buffer, "\\\""), "Quotes not escaped in encoding");

	/* Decode (\" should become ") */
	ret = json_obj_parse(buffer, strlen(buffer), escape_test_descr,
			     ARRAY_SIZE(escape_test_descr), &decoded);
	zassert_equal(ret, (1 << ARRAY_SIZE(escape_test_descr)) - 1, "Decoding failed");

	/* Verify we get back the original strings */
	zassert_str_equal(decoded.string_value, original.string_value,
			  "Quote round-trip failed for string_value");
	zassert_str_equal(decoded.string_buf, original.string_buf,
			  "Quote round-trip failed for string_buf");
}

/**
 * @brief Test that backslashes don't get duplicated in quote escaping
 */
ZTEST(lib_json_test, test_json_quote_no_backslash_duplication)
{
	struct escape_test_data test_data = {
		.string_value = "Text\"with\"quotes",
		.integer_value = 1,
	};
	strncpy(test_data.string_buf, "More\"quotes\"here", sizeof(test_data.string_buf) - 1);

	char buffer1[256], buffer2[256];
	struct escape_test_data decoded1 = {0}, decoded2 = {0};
	int ret;

	/* First encode/decode cycle */
	ret = json_obj_encode_buf(escape_test_descr, ARRAY_SIZE(escape_test_descr), &test_data,
				  buffer1, sizeof(buffer1));
	zassert_equal(ret, 0, "First encode failed");

	ret = json_obj_parse(buffer1, strlen(buffer1), escape_test_descr,
			     ARRAY_SIZE(escape_test_descr), &decoded1);
	zassert_equal(ret, (1 << ARRAY_SIZE(escape_test_descr)) - 1, "First decode failed");

	/* Second encode/decode cycle */
	ret = json_obj_encode_buf(escape_test_descr, ARRAY_SIZE(escape_test_descr), &decoded1,
				  buffer2, sizeof(buffer2));
	zassert_equal(ret, 0, "Second encode failed");

	ret = json_obj_parse(buffer2, strlen(buffer2), escape_test_descr,
			     ARRAY_SIZE(escape_test_descr), &decoded2);
	zassert_equal(ret, (1 << ARRAY_SIZE(escape_test_descr)) - 1, "Second decode failed");

	/* Critical: No backslash duplication should occur */
	zassert_str_equal(decoded1.string_value, decoded2.string_value,
			  "Backslash duplication detected in string_value after quote handling");
	zassert_str_equal(decoded1.string_buf, decoded2.string_buf,
			  "Backslash duplication detected in string_buf after quote handling");

	/* Verify original content is preserved */
	zassert_str_equal(test_data.string_value, decoded2.string_value,
			  "Original string with quotes not preserved");
	zassert_str_equal(test_data.string_buf, decoded2.string_buf,
			  "Original string buffer with quotes not preserved");
}

/**
 * @brief Test mixed escape sequences including quotes
 */
ZTEST(lib_json_test, test_json_mixed_escape_sequences_with_quotes)
{
	struct escape_test_data test_data = {
		.string_value = "Line1\nTab\tQuote\"End",
		.integer_value = 999,
	};
	strncpy(test_data.string_buf, "Mix\n\t\"\\chars", sizeof(test_data.string_buf) - 1);

	char buffer[256];
	struct escape_test_data decoded = {0};
	int ret;

	/* Encode */
	ret = json_obj_encode_buf(escape_test_descr, ARRAY_SIZE(escape_test_descr), &test_data,
				  buffer, sizeof(buffer));
	zassert_equal(ret, 0, "Encoding failed");

	/* Verify all escape sequences are present in encoded form */
	zassert_not_null(strstr(buffer, "\\n"), "Newline not escaped");
	zassert_not_null(strstr(buffer, "\\t"), "Tab not escaped");
	zassert_not_null(strstr(buffer, "\\\""), "Quote not escaped");
	zassert_not_null(strstr(buffer, "\\\\"), "Backslash not escaped");

	/* Decode */
	ret = json_obj_parse(buffer, strlen(buffer), escape_test_descr,
			     ARRAY_SIZE(escape_test_descr), &decoded);
	zassert_equal(ret, (1 << ARRAY_SIZE(escape_test_descr)) - 1, "Decoding failed");

	/* Verify all sequences are properly unescaped */
	zassert_str_equal(decoded.string_value, "Line1\nTab\tQuote\"End",
			  "Mixed escape sequences corrupted in string_value");
	zassert_str_equal(decoded.string_buf, "Mix\n\t\"\\chars",
			  "Mixed escape sequences corrupted in string_buf");
}

/**
 * @brief Test multiple cycles with quotes to detect gradual corruption
 */
ZTEST(lib_json_test, test_json_quote_multiple_cycle_stability)
{
	struct escape_test_data original = {
		.string_value = "Config\"value\"here",
		.integer_value = 50,
	};
	strncpy(original.string_value, "Config\"value\"here", sizeof(original.string_value) - 1);
	strncpy(original.string_buf, "Setting\"name\"value", sizeof(original.string_buf) - 1);

	char buffer[256];
	struct escape_test_data current = {0};
	int ret;

	/* Initialize current by copying from original */
	current.integer_value = original.integer_value;
	strncpy(current.string_value, original.string_value, sizeof(current.string_value));
	strncpy(current.string_buf, original.string_buf, sizeof(current.string_buf));

	/* Run multiple encode/decode cycles */
	for (int cycle = 0; cycle < 3; cycle++) {
		/* Encode */
		ret = json_obj_encode_buf(escape_test_descr, ARRAY_SIZE(escape_test_descr),
					  &current, buffer, sizeof(buffer));
		zassert_equal(ret, 0, "Encoding failed at cycle %d", cycle);

		/* Decode into fresh struct */
		struct escape_test_data next = {0};

		ret = json_obj_parse(buffer, strlen(buffer), escape_test_descr,
				     ARRAY_SIZE(escape_test_descr), &next);
		zassert_equal(ret, (1 << ARRAY_SIZE(escape_test_descr)) - 1,
			      "Decoding failed at cycle %d", cycle);

		/* Verify no corruption of quotes */
		zassert_str_equal(next.string_value, original.string_value,
				  "Quote string corrupted after %d cycles", cycle + 1);
		zassert_str_equal(next.string_buf, original.string_buf,
				  "Quote string buffer corrupted after %d cycles", cycle + 1);

		/* Prepare for next cycle */
		current.integer_value = next.integer_value;
		strncpy(current.string_value, next.string_value, sizeof(current.string_value));
		strncpy(current.string_buf, next.string_buf, sizeof(current.string_buf));
		current.integer_value++; /* Change integer to simulate config updates */
	}
}

/**
 * @brief Test edge case: string containing only a quote
 */
ZTEST(lib_json_test, test_json_single_quote_string)
{
	struct escape_test_data test_data = {
		.string_value = "\"",
		.integer_value = 1,
	};
	strncpy(test_data.string_buf, "\"", sizeof(test_data.string_buf) - 1);

	char buffer[256];
	struct escape_test_data decoded = {0};
	int ret;

	/* Encode */
	ret = json_obj_encode_buf(escape_test_descr, ARRAY_SIZE(escape_test_descr), &test_data,
				  buffer, sizeof(buffer));
	zassert_equal(ret, 0, "Encoding single quote failed");

	/* Should contain escaped quote */
	zassert_not_null(strstr(buffer, "\\\""), "Single quote not escaped");

	/* Decode */
	ret = json_obj_parse(buffer, strlen(buffer), escape_test_descr,
			     ARRAY_SIZE(escape_test_descr), &decoded);
	zassert_equal(ret, (1 << ARRAY_SIZE(escape_test_descr)) - 1,
		      "Decoding single quote failed");

	/* Should get back the single quote */
	zassert_str_equal(decoded.string_value, "\"", "Single quote not preserved");
	zassert_str_equal(decoded.string_buf, "\"", "Single quote in buffer not preserved");
}

ZTEST(lib_json_test, test_debug_string_types)
{
	char encoded[] = "{\"string_value\":\"test\\nvalue\",\"string_buf\":\"buffer\\ttab\"}";
	struct escape_test_data decoded = {0};

	printf("=== DEBUG TEST ===\n");
	printf("Input JSON: %s\n", encoded);

	int ret = json_obj_parse(encoded, strlen(encoded), escape_test_descr,
				 ARRAY_SIZE(escape_test_descr), &decoded);

	printf("Parse result: %d\n", ret);
	printf("string_value: '%s'\n", decoded.string_value);
	printf("string_buf: '%s'\n", decoded.string_buf);
	printf("=== END DEBUG ===\n");

	zassert_str_equal(decoded.string_value, "test\nvalue", "string_value not unescaped");
	zassert_str_equal(decoded.string_buf, "buffer\ttab", "string_buf not unescaped");
}

ZTEST_SUITE(lib_json_test, NULL, NULL, NULL, NULL, NULL);
