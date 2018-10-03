/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <string.h>
#include <zephyr/types.h>
#include <stdbool.h>
#include <ztest.h>
#include <json.h>

struct test_nested {
	int nested_int;
	bool nested_bool;
	const char *nested_string;
};

struct test_struct {
	const char *some_string;
	int some_int;
	bool some_bool;
	struct test_nested some_nested_struct;
	int some_array[16];
	size_t some_array_len;
	bool another_bxxl;		 /* JSON field: "another_b!@l" */
	bool if_;			 /* JSON: "if" */
	int another_array[10];		 /* JSON: "another-array" */
	size_t another_array_len;
	struct test_nested xnother_nexx; /* JSON: "4nother_ne$+" */
};

struct elt {
	const char *name;
	int height;
};

struct obj_array {
	struct elt elements[10];
	size_t num_elements;
};

static const struct json_obj_descr nested_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct test_nested, nested_int, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct test_nested, nested_bool, JSON_TOK_TRUE),
	JSON_OBJ_DESCR_PRIM(struct test_nested, nested_string,
			    JSON_TOK_STRING),
};

static const struct json_obj_descr test_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct test_struct, some_string, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct test_struct, some_int, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct test_struct, some_bool, JSON_TOK_TRUE),
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
};

static const struct json_obj_descr elt_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct elt, name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct elt, height, JSON_TOK_NUMBER),
};

static const struct json_obj_descr obj_array_descr[] = {
	JSON_OBJ_DESCR_OBJ_ARRAY(struct obj_array, elements, 10, num_elements,
				 elt_descr, ARRAY_SIZE(elt_descr)),
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

static void test_json_encoding(void)
{
	struct test_struct ts = {
		.some_string = "zephyr 123",
		.some_int = 42,
		.some_bool = true,
		.some_nested_struct = {
			.nested_int = -1234,
			.nested_bool = false,
			.nested_string = "this should be escaped: \t"
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
		},
	};
	char encoded[] = "{\"some_string\":\"zephyr 123\","
		"\"some_int\":42,\"some_bool\":true,"
		"\"some_nested_struct\":{\"nested_int\":-1234,"
		"\"nested_bool\":false,\"nested_string\":"
		"\"this should be escaped: \\t\"},"
		"\"some_array\":[1,4,8,16,32],"
		"\"another_b!@l\":true,"
		"\"if\":false,"
		"\"another-array\":[2,3,5,7],"
		"\"4nother_ne$+\":{\"nested_int\":1234,"
		"\"nested_bool\":true,"
		"\"nested_string\":\"no escape necessary\"}"
		"}";
	char buffer[sizeof(encoded)];
	int ret;

	ret = json_obj_encode_buf(test_descr, ARRAY_SIZE(test_descr),
				  &ts, buffer, sizeof(buffer));
	zassert_equal(ret, 0, "Encoding function returned no errors");

	ret = strncmp(buffer, encoded, sizeof(encoded) - 1);
	zassert_equal(ret, 0, "Encoded contents consistent");
}

static void test_json_decoding(void)
{
	struct test_struct ts;
	char encoded[] = "{\"some_string\":\"zephyr 123\","
		"\"some_int\":\t42\n,"
		"\"some_bool\":true    \t  "
		"\n"
		"\r   ,"
		"\"some_nested_struct\":{    "
		"\"nested_int\":-1234,\n\n"
		"\"nested_bool\":false,\t"
		"\"nested_string\":\"this should be escaped: \\t\"},"
		"\"some_array\":[11,22, 33,\t45,\n299]"
		"\"another_b!@l\":true,"
		"\"if\":false,"
		"\"another-array\":[2,3,5,7],"
		"\"4nother_ne$+\":{\"nested_int\":1234,"
		"\"nested_bool\":true,"
		"\"nested_string\":\"no escape necessary\"}"
		"}";
	const int expected_array[] = { 11, 22, 33, 45, 299 };
	const int expected_other_array[] = { 2, 3, 5, 7 };
	int ret;

	ret = json_obj_parse(encoded, sizeof(encoded) - 1, test_descr,
			     ARRAY_SIZE(test_descr), &ts);

	zassert_equal(ret, (1 << ARRAY_SIZE(test_descr)) - 1,
		     "All fields decoded correctly");

	zassert_true(!strcmp(ts.some_string, "zephyr 123"),
		    "String decoded correctly");
	zassert_equal(ts.some_int, 42, "Positive integer decoded correctly");
	zassert_equal(ts.some_bool, true, "Boolean decoded correctly");
	zassert_equal(ts.some_nested_struct.nested_int, -1234,
		     "Nested negative integer decoded correctly");
	zassert_equal(ts.some_nested_struct.nested_bool, false,
		     "Nested boolean value decoded correctly");
	zassert_true(!strcmp(ts.some_nested_struct.nested_string,
			    "this should be escaped: \\t"),
		    "Nested string decoded correctly");
	zassert_equal(ts.some_array_len, 5, "Array has correct number of items");
	zassert_true(!memcmp(ts.some_array, expected_array,
		    sizeof(expected_array)),
		    "Array decoded with expected values");
	zassert_true(ts.another_bxxl,
		     "Named boolean (special chars) decoded correctly");
	zassert_false(ts.if_,
		      "Named boolean (reserved word) decoded correctly");
	zassert_equal(ts.another_array_len, 4,
		      "Named array has correct number of items");
	zassert_true(!memcmp(ts.another_array, expected_other_array,
			     sizeof(expected_other_array)),
		     "Decoded named array with expected values");
	zassert_equal(ts.xnother_nexx.nested_int, 1234,
		      "Named nested integer decoded correctly");
	zassert_equal(ts.xnother_nexx.nested_bool, true,
		      "Named nested boolean decoded correctly");
	zassert_true(!strcmp(ts.xnother_nexx.nested_string,
			     "no escape necessary"),
		     "Named nested string decoded correctly");
}

static void test_json_decoding_array_array(void)
{
	int ret;
	struct obj_array_array obj_array_array_ts;
	char encoded[] = "{\"objects_array\":["
			  "[{\"height\":168,\"name\":\"Simón Bolívar\"}],"
			  "[{\"height\":173,\"name\":\"Pelé\"}],"
			  "[{\"height\":195,\"name\":\"Usain Bolt\"}]]"
			  "}";

	ret = json_obj_parse(encoded, sizeof(encoded),
			     array_array_descr,
			     ARRAY_SIZE(array_array_descr),
			     &obj_array_array_ts);

	zassert_equal(ret, 1, "Encoding array of object returned no errors");
	zassert_true(!strcmp(obj_array_array_ts.objects_array[1].objects.name,
			 "Pelé"), "String decoded correctly");
	zassert_equal(obj_array_array_ts.objects_array[2].objects.height, 195,
		      "Usain Bolt height decoded correctly");
}

static void test_json_obj_arr_encoding(void)
{
	struct obj_array oa = {
		.elements = {
			[0] = { .name = "Simón Bolívar",   .height = 168 },
			[1] = { .name = "Muggsy Bogues",   .height = 160 },
			[2] = { .name = "Pelé",            .height = 173 },
			[3] = { .name = "Hakeem Olajuwon", .height = 213 },
			[4] = { .name = "Alex Honnold",    .height = 180 },
			[5] = { .name = "Hazel Findlay",   .height = 157 },
			[6] = { .name = "Daila Ojeda",     .height = 158 },
			[7] = { .name = "Albert Einstein", .height = 172 },
			[8] = { .name = "Usain Bolt",      .height = 195 },
			[9] = { .name = "Paavo Nurmi",     .height = 174 },
		},
		.num_elements = 10,
	};
	const char encoded[] = "{\"elements\":["
		"{\"name\":\"Simón Bolívar\",\"height\":168},"
		"{\"name\":\"Muggsy Bogues\",\"height\":160},"
		"{\"name\":\"Pelé\",\"height\":173},"
		"{\"name\":\"Hakeem Olajuwon\",\"height\":213},"
		"{\"name\":\"Alex Honnold\",\"height\":180},"
		"{\"name\":\"Hazel Findlay\",\"height\":157},"
		"{\"name\":\"Daila Ojeda\",\"height\":158},"
		"{\"name\":\"Albert Einstein\",\"height\":172},"
		"{\"name\":\"Usain Bolt\",\"height\":195},"
		"{\"name\":\"Paavo Nurmi\",\"height\":174}"
		"]}";
	char buffer[sizeof(encoded)];
	int ret;

	ret = json_obj_encode_buf(obj_array_descr, ARRAY_SIZE(obj_array_descr),
				  &oa, buffer, sizeof(buffer));
	zassert_equal(ret, 0, "Encoding array of object returned no errors");
	zassert_true(!strcmp(buffer, encoded),
		     "Encoded array of objects is consistent");
}

static void test_json_obj_arr_decoding(void)
{
	struct obj_array oa;
	char encoded[] = "{\"elements\":["
		"{\"name\":\"Simón Bolívar\",\"height\":168},"
		"{\"name\":\"Muggsy Bogues\",\"height\":160},"
		"{\"name\":\"Pelé\",\"height\":173},"
		"{\"name\":\"Hakeem Olajuwon\",\"height\":213},"
		"{\"name\":\"Alex Honnold\",\"height\":180},"
		"{\"name\":\"Hazel Findlay\",\"height\":157},"
		"{\"name\":\"Daila Ojeda\",\"height\":158},"
		"{\"name\":\"Albert Einstein\",\"height\":172},"
		"{\"name\":\"Usain Bolt\",\"height\":195},"
		"{\"name\":\"Paavo Nurmi\",\"height\":174}"
		"]}";
	const struct obj_array expected = {
		.elements = {
			[0] = { .name = "Simón Bolívar",   .height = 168 },
			[1] = { .name = "Muggsy Bogues",   .height = 160 },
			[2] = { .name = "Pelé",            .height = 173 },
			[3] = { .name = "Hakeem Olajuwon", .height = 213 },
			[4] = { .name = "Alex Honnold",    .height = 180 },
			[5] = { .name = "Hazel Findlay",   .height = 157 },
			[6] = { .name = "Daila Ojeda",     .height = 158 },
			[7] = { .name = "Albert Einstein", .height = 172 },
			[8] = { .name = "Usain Bolt",      .height = 195 },
			[9] = { .name = "Paavo Nurmi",     .height = 174 },
		},
		.num_elements = 10,
	};
	int ret;

	ret = json_obj_parse(encoded, sizeof(encoded) - 1, obj_array_descr,
			     ARRAY_SIZE(obj_array_descr), &oa);

	zassert_equal(ret, (1 << ARRAY_SIZE(obj_array_descr)) - 1,
		      "Array of object fields decoded correctly");
	zassert_equal(oa.num_elements, 10,
		      "Number of object fields decoded correctly");
	zassert_true(!strcmp(oa.elements[0].name, expected.elements[0].name),
		     "Element 0 name decoded correctly");
	zassert_equal(oa.elements[0].height, expected.elements[0].height,
		     "Element 0 height decoded correctly");
	zassert_true(!strcmp(oa.elements[1].name, expected.elements[1].name),
		     "Element 1 name decoded correctly");
	zassert_equal(oa.elements[1].height, expected.elements[1].height,
		     "Element 1 height decoded correctly");
	zassert_true(!strcmp(oa.elements[2].name, expected.elements[2].name),
		     "Element 2 name decoded correctly");
	zassert_equal(oa.elements[2].height, expected.elements[2].height,
		     "Element 2 height decoded correctly");
	zassert_true(!strcmp(oa.elements[3].name, expected.elements[3].name),
		     "Element 3 name decoded correctly");
	zassert_equal(oa.elements[3].height, expected.elements[3].height,
		     "Element 3 height decoded correctly");
	zassert_true(!strcmp(oa.elements[4].name, expected.elements[4].name),
		     "Element 4 name decoded correctly");
	zassert_equal(oa.elements[4].height, expected.elements[4].height,
		     "Element 4 height decoded correctly");
	zassert_true(!strcmp(oa.elements[5].name, expected.elements[5].name),
		     "Element 5 name decoded correctly");
	zassert_equal(oa.elements[5].height, expected.elements[5].height,
		     "Element 5 height decoded correctly");
	zassert_true(!strcmp(oa.elements[6].name, expected.elements[6].name),
		     "Element 6 name decoded correctly");
	zassert_equal(oa.elements[6].height, expected.elements[6].height,
		     "Element 6 height decoded correctly");
	zassert_true(!strcmp(oa.elements[7].name, expected.elements[7].name),
		     "Element 7 name decoded correctly");
	zassert_equal(oa.elements[7].height, expected.elements[7].height,
		     "Element 7 height decoded correctly");
	zassert_true(!strcmp(oa.elements[8].name, expected.elements[8].name),
		     "Element 8 name decoded correctly");
	zassert_equal(oa.elements[8].height, expected.elements[8].height,
		     "Element 8 height decoded correctly");
	zassert_true(!strcmp(oa.elements[9].name, expected.elements[9].name),
		     "Element 9 name decoded correctly");
	zassert_equal(oa.elements[9].height, expected.elements[9].height,
		     "Element 9 height decoded correctly");
}

static void test_json_invalid_unicode(void)
{
	struct test_struct ts;
	char encoded[] = "{\"some_string\":\"\\uABC@\"}";
	int ret;

	ret = json_obj_parse(encoded, sizeof(encoded) - 1, test_descr,
			     ARRAY_SIZE(test_descr), &ts);
	zassert_equal(ret, -EINVAL, "Decoding has to fail");
}

static void test_json_missing_quote(void)
{
	struct test_struct ts;
	char encoded[] = "{\"some_string";
	int ret;

	ret = json_obj_parse(encoded, sizeof(encoded) - 1, test_descr,
			     ARRAY_SIZE(test_descr), &ts);
	zassert_equal(ret, -EINVAL, "Decoding has to fail");
}

static void test_json_wrong_token(void)
{
	struct test_struct ts;
	char encoded[] = "{\"some_string\",}";
	int ret;

	ret = json_obj_parse(encoded, sizeof(encoded) - 1, test_descr,
			     ARRAY_SIZE(test_descr), &ts);
	zassert_equal(ret, -EINVAL, "Decoding has to fail");
}

static void test_json_item_wrong_type(void)
{
	struct test_struct ts;
	char encoded[] = "{\"some_string\":false}";
	int ret;

	ret = json_obj_parse(encoded, sizeof(encoded) - 1, test_descr,
			     ARRAY_SIZE(test_descr), &ts);
	zassert_equal(ret, -EINVAL, "Decoding has to fail");
}

static void test_json_key_not_in_descr(void)
{
	struct test_struct ts;
	char encoded[] = "{\"key_not_in_descr\":123456}";
	int ret;

	ret = json_obj_parse(encoded, sizeof(encoded) - 1, test_descr,
			     ARRAY_SIZE(test_descr), &ts);
	zassert_equal(ret, 0, "No items should be decoded");
}

static void test_json_escape(void)
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
	zassert_equal(ret, 0, "Escape succeeded");
	zassert_equal(len, sizeof(buf) - 1,
		      "Escaped length computed correctly");
	zassert_true(!strcmp(buf, expected),
		     "Escaped value is correct");
}

/* Edge case: only one character, which must be escaped. */
static void test_json_escape_one(void)
{
	char buf[3] = {'\t', '\0', '\0'};
	const char *expected = "\\t";
	size_t len = strlen(buf);
	ssize_t ret;

	ret = json_escape(buf, &len, sizeof(buf));
	zassert_equal(ret, 0,
		      "Escaping one character succeded");
	zassert_equal(len, sizeof(buf) - 1,
		      "Escaping one character length is correct");
	zassert_true(!strcmp(buf, expected),
		     "Escaped value is correct");
}

static void test_json_escape_empty(void)
{
	char empty[] = "";
	size_t len = sizeof(empty) - 1;
	ssize_t ret;

	ret = json_escape(empty, &len, sizeof(empty));
	zassert_equal(ret, 0, "Escaped empty string");
	zassert_equal(len, 0, "Length of empty escaped string is zero");
	zassert_equal(empty[0], '\0', "Empty string remains empty");
}

static void test_json_escape_no_op(void)
{
	char nothing_to_escape[] = "hello,world:!";
	const char *expected = "hello,world:!";
	size_t len = sizeof(nothing_to_escape) - 1;
	ssize_t ret;

	ret = json_escape(nothing_to_escape, &len, sizeof(nothing_to_escape));
	zassert_equal(ret, 0, "Nothing to escape handled correctly");
	zassert_equal(len, sizeof(nothing_to_escape) - 1,
		      "Didn't change length of already escaped string");
	zassert_true(!strcmp(nothing_to_escape, expected),
		     "Didn't alter string with nothing to escape");
}

static void test_json_escape_bounds_check(void)
{
	char not_enough_memory[] = "\tfoo";
	size_t len = sizeof(not_enough_memory) - 1;
	ssize_t ret;

	ret = json_escape(not_enough_memory, &len, sizeof(not_enough_memory));
	zassert_equal(ret, -ENOMEM, "Bounds check OK");
}

void test_main(void)
{
	ztest_test_suite(lib_json_test,
			 ztest_unit_test(test_json_encoding),
			 ztest_unit_test(test_json_decoding),
			 ztest_unit_test(test_json_decoding_array_array),
			 ztest_unit_test(test_json_obj_arr_encoding),
			 ztest_unit_test(test_json_obj_arr_decoding),
			 ztest_unit_test(test_json_invalid_unicode),
			 ztest_unit_test(test_json_missing_quote),
			 ztest_unit_test(test_json_wrong_token),
			 ztest_unit_test(test_json_item_wrong_type),
			 ztest_unit_test(test_json_key_not_in_descr),
			 ztest_unit_test(test_json_escape),
			 ztest_unit_test(test_json_escape_one),
			 ztest_unit_test(test_json_escape_empty),
			 ztest_unit_test(test_json_escape_no_op),
			 ztest_unit_test(test_json_escape_bounds_check)
			 );

	ztest_run_test_suite(lib_json_test);
}
