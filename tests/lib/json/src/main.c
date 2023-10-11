/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <string.h>
#include <zephyr/types.h>
#include <stdbool.h>
#include <zephyr/ztest.h>
#include <zephyr/data/json.h>

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
	struct test_nested nested_obj_array[2];
	size_t obj_array_len;
};

struct elt {
	const char *name;
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
	JSON_OBJ_DESCR_OBJ_ARRAY(struct test_struct, nested_obj_array, 2,
				 obj_array_len, nested_descr, ARRAY_SIZE(nested_descr)),
};

static const struct json_obj_descr elt_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct elt, name, JSON_TOK_STRING),
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

ZTEST(lib_json_test, test_json_encoding)
{
	struct test_struct ts = {
		.some_string = "zephyr 123\uABCD",
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
		.nested_obj_array = {
			{1, true, "true"},
			{0, false, "false"}
		},
		.obj_array_len = 2
	};
	char encoded[] = "{\"some_string\":\"zephyr 123\uABCD\","
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
		"\"nested_string\":\"no escape necessary\"},"
		"\"nested_obj_array\":["
		"{\"nested_int\":1,\"nested_bool\":true,\"nested_string\":\"true\"},"
		"{\"nested_int\":0,\"nested_bool\":false,\"nested_string\":\"false\"}]"
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
		"\"some_int\":\t42\n,"
		"\"some_bool\":true    \t  "
		"\n"
		"\r   ,"
		"\"some_nested_struct\":{    "
		"\"nested_int\":-1234,\n\n"
		"\"nested_bool\":false,\t"
		"\"nested_string\":\"this should be escaped: \\t\","
		"\"extra_nested_array\":[0,-1]},"
		"\"extra_struct\":{\"nested_bool\":false},"
		"\"extra_bool\":true,"
		"\"some_array\":[11,22, 33,\t45,\n299],"
		"\"another_b!@l\":true,"
		"\"if\":false,"
		"\"another-array\":[2,3,5,7],"
		"\"4nother_ne$+\":{\"nested_int\":1234,"
		"\"nested_bool\":true,"
		"\"nested_string\":\"no escape necessary\"},"
		"\"nested_obj_array\":["
		"{\"nested_int\":1,\"nested_bool\":true,\"nested_string\":\"true\"},"
		"{\"nested_int\":0,\"nested_bool\":false,\"nested_string\":\"false\"}]"
		"}\n";
	const int expected_array[] = { 11, 22, 33, 45, 299 };
	const int expected_other_array[] = { 2, 3, 5, 7 };
	int ret;

	ret = json_obj_parse(encoded, sizeof(encoded) - 1, test_descr,
			     ARRAY_SIZE(test_descr), &ts);

	zassert_equal(ret, (1 << ARRAY_SIZE(test_descr)) - 1,
		      "Not all fields decoded correctly");

	zassert_true(!strcmp(ts.some_string, "zephyr 123\\uABCD456"),
		     "String not decoded correctly");
	zassert_equal(ts.some_int, 42, "Positive integer not decoded correctly");
	zassert_equal(ts.some_bool, true, "Boolean not decoded correctly");
	zassert_equal(ts.some_nested_struct.nested_int, -1234,
		      "Nested negative integer not decoded correctly");
	zassert_equal(ts.some_nested_struct.nested_bool, false,
		      "Nested boolean value not decoded correctly");
	zassert_true(!strcmp(ts.some_nested_struct.nested_string,
			    "this should be escaped: \\t"),
		     "Nested string not decoded correctly");
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
	zassert_equal(ts.xnother_nexx.nested_bool, true,
		      "Named nested boolean not decoded correctly");
	zassert_true(!strcmp(ts.xnother_nexx.nested_string,
			     "no escape necessary"),
		     "Named nested string not decoded correctly");
	zassert_equal(ts.obj_array_len, 2,
		      "Array of objects does not have correct number of items");
	zassert_equal(ts.nested_obj_array[0].nested_int, 1,
		      "Integer in first object array element not decoded correctly");
	zassert_equal(ts.nested_obj_array[0].nested_bool, true,
		      "Boolean value in first object array element not decoded correctly");
	zassert_true(!strcmp(ts.nested_obj_array[0].nested_string, "true"),
		     "String in first object array element not decoded correctly");
	zassert_equal(ts.nested_obj_array[1].nested_int, 0,
		      "Integer in second object array element not decoded correctly");
	zassert_equal(ts.nested_obj_array[1].nested_bool, false,
		      "Boolean value in second object array element not decoded correctly");
	zassert_true(!strcmp(ts.nested_obj_array[1].nested_string, "false"),
		     "String in second object array element not decoded correctly");
}

ZTEST(lib_json_test, test_json_limits)
{
	int ret = 0;
	char encoded[] = "{\"int_max\":2147483647,"
			 "\"int_cero\":0,"
			 "\"int_min\":-2147483648"
			 "}";

	struct test_int_limits limits = {
		.int_max = INT_MAX,
		.int_cero = 0,
		.int_min = INT_MIN,
	};

	char buffer[sizeof(encoded)];
	struct test_int_limits limits_decoded = {0};

	ret = json_obj_encode_buf(obj_limits_descr, ARRAY_SIZE(obj_limits_descr),
				&limits, buffer, sizeof(buffer));
	ret = json_obj_parse(encoded, sizeof(encoded) - 1, obj_limits_descr,
			     ARRAY_SIZE(obj_limits_descr), &limits_decoded);

	zassert_true(!strcmp(encoded, buffer), "Integer limits not encoded correctly");
	zassert_true(!memcmp(&limits, &limits_decoded, sizeof(limits)),
		     "Integer limits not decoded correctly");
}

ZTEST(lib_json_test, test_json_encoding_array_array)
{
	struct obj_array_array obj_array_array_ts = {
		.objects_array = {
			[0] = { { .name = "Sim\303\263n Bol\303\255var", .height = 168 } },
			[1] = { { .name = "Pel\303\251",                 .height = 173 } },
			[2] = { { .name = "Usain Bolt",                  .height = 195 } },
		},
		.objects_array_len = 3,
	};
	char encoded[] = "{\"objects_array\":["
		"{\"name\":\"Sim\303\263n Bol\303\255var\",\"height\":168},"
		"{\"name\":\"Pel\303\251\",\"height\":173},"
		"{\"name\":\"Usain Bolt\",\"height\":195}"
		"]}";
	char buffer[sizeof(encoded)];
	int ret;

	ret = json_obj_encode_buf(array_array_descr, ARRAY_SIZE(array_array_descr),
				  &obj_array_array_ts, buffer, sizeof(buffer));
	zassert_equal(ret, 0, "Encoding array returned error");
	zassert_true(!strcmp(buffer, encoded),
		     "Encoded array of objects is not consistent");
}

ZTEST(lib_json_test, test_json_decoding_array_array)
{
	int ret;
	struct obj_array_array obj_array_array_ts;
	char encoded[] = "{\"objects_array\":["
			  "{\"height\":168,\"name\":\"Sim\303\263n Bol\303\255var\"},"
			  "{\"height\":173,\"name\":\"Pel\303\251\"},"
			  "{\"height\":195,\"name\":\"Usain Bolt\"}]"
			  "}";

	ret = json_obj_parse(encoded, sizeof(encoded),
			     array_array_descr,
			     ARRAY_SIZE(array_array_descr),
			     &obj_array_array_ts);

	zassert_equal(ret, 1, "Encoding array of objects returned error");
	zassert_equal(obj_array_array_ts.objects_array_len, 3,
		      "Array doesn't have correct number of items");

	zassert_true(!strcmp(obj_array_array_ts.objects_array[0].objects.name,
			 "Sim\303\263n Bol\303\255var"), "String not decoded correctly");
	zassert_equal(obj_array_array_ts.objects_array[0].objects.height, 168,
		      "Sim\303\263n Bol\303\255var height not decoded correctly");

	zassert_true(!strcmp(obj_array_array_ts.objects_array[1].objects.name,
			 "Pel\303\251"), "String not decoded correctly");
	zassert_equal(obj_array_array_ts.objects_array[1].objects.height, 173,
		      "Pel\303\251 height not decoded correctly");

	zassert_true(!strcmp(obj_array_array_ts.objects_array[2].objects.name,
			 "Usain Bolt"), "String not decoded correctly");
	zassert_equal(obj_array_array_ts.objects_array[2].objects.height, 195,
		      "Usain Bolt height not decoded correctly");
}

ZTEST(lib_json_test, test_json_obj_arr_encoding)
{
	struct obj_array oa = {
		.elements = {
			[0] = { .name = "Sim\303\263n Bol\303\255var", .height = 168 },
			[1] = { .name = "Muggsy Bogues",               .height = 160 },
			[2] = { .name = "Pel\303\251",                 .height = 173 },
			[3] = { .name = "Hakeem Olajuwon",             .height = 213 },
			[4] = { .name = "Alex Honnold",                .height = 180 },
			[5] = { .name = "Hazel Findlay",               .height = 157 },
			[6] = { .name = "Daila Ojeda",                 .height = 158 },
			[7] = { .name = "Albert Einstein",             .height = 172 },
			[8] = { .name = "Usain Bolt",                  .height = 195 },
			[9] = { .name = "Paavo Nurmi",                 .height = 174 },
		},
		.num_elements = 10,
	};
	char encoded[] = "{\"elements\":["
		"{\"name\":\"Sim\303\263n Bol\303\255var\",\"height\":168},"
		"{\"name\":\"Muggsy Bogues\",\"height\":160},"
		"{\"name\":\"Pel\303\251\",\"height\":173},"
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
	zassert_equal(ret, 0, "Encoding array of object returned error");
	zassert_true(!strcmp(buffer, encoded),
		     "Encoded array of objects is not consistent");
}

ZTEST(lib_json_test, test_json_arr_obj_decoding)
{
	int ret;
	struct obj_array obj_array_array_ts;
	char encoded[] = "[{\"height\":168,\"name\":\"Sim\303\263n Bol\303\255var\"},"
					"{\"height\":173,\"name\":\"Pel\303\251\"},"
					"{\"height\":195,\"name\":\"Usain Bolt\"}"
					"]";

	ret = json_arr_parse(encoded, sizeof(encoded),
			     obj_array_descr,
			     &obj_array_array_ts);

	zassert_equal(ret, 0, "Encoding array of objects returned error %d", ret);
	zassert_equal(obj_array_array_ts.num_elements, 3,
		      "Array doesn't have correct number of items");

	zassert_true(!strcmp(obj_array_array_ts.elements[0].name,
			 "Sim\303\263n Bol\303\255var"), "String not decoded correctly");
	zassert_equal(obj_array_array_ts.elements[0].height, 168,
		      "Sim\303\263n Bol\303\255var height not decoded correctly");

	zassert_true(!strcmp(obj_array_array_ts.elements[1].name,
			 "Pel\303\251"), "String not decoded correctly");
	zassert_equal(obj_array_array_ts.elements[1].height, 173,
		      "Pel\303\251 height not decoded correctly");

	zassert_true(!strcmp(obj_array_array_ts.elements[2].name,
			 "Usain Bolt"), "String not decoded correctly");
	zassert_equal(obj_array_array_ts.elements[2].height, 195,
		      "Usain Bolt height not decoded correctly");
}

ZTEST(lib_json_test, test_json_arr_obj_encoding)
{
	struct obj_array oa = {
		.elements = {
			[0] = { .name = "Sim\303\263n Bol\303\255var", .height = 168 },
			[1] = { .name = "Muggsy Bogues",               .height = 160 },
			[2] = { .name = "Pel\303\251",                 .height = 173 },
			[3] = { .name = "Hakeem Olajuwon",             .height = 213 },
			[4] = { .name = "Alex Honnold",                .height = 180 },
			[5] = { .name = "Hazel Findlay",               .height = 157 },
			[6] = { .name = "Daila Ojeda",                 .height = 158 },
			[7] = { .name = "Albert Einstein",             .height = 172 },
			[8] = { .name = "Usain Bolt",                  .height = 195 },
			[9] = { .name = "Paavo Nurmi",                 .height = 174 },
		},
		.num_elements = 10,
	};
	char encoded[] = "["
		"{\"name\":\"Sim\303\263n Bol\303\255var\",\"height\":168},"
		"{\"name\":\"Muggsy Bogues\",\"height\":160},"
		"{\"name\":\"Pel\303\251\",\"height\":173},"
		"{\"name\":\"Hakeem Olajuwon\",\"height\":213},"
		"{\"name\":\"Alex Honnold\",\"height\":180},"
		"{\"name\":\"Hazel Findlay\",\"height\":157},"
		"{\"name\":\"Daila Ojeda\",\"height\":158},"
		"{\"name\":\"Albert Einstein\",\"height\":172},"
		"{\"name\":\"Usain Bolt\",\"height\":195},"
		"{\"name\":\"Paavo Nurmi\",\"height\":174}"
		"]";
	char buffer[sizeof(encoded)];
	int ret;
	ssize_t len;

	len = json_calc_encoded_arr_len(obj_array_descr, &oa);
	zassert_equal(len, strlen(encoded), "encoded size mismatch");

	ret = json_arr_encode_buf(obj_array_descr, &oa, buffer, sizeof(buffer));
	zassert_equal(ret, 0, "Encoding array of object returned error %d", ret);
	zassert_true(!strcmp(buffer, encoded),
		     "Encoded array of objects is not consistent");
}

ZTEST(lib_json_test, test_json_obj_arr_decoding)
{
	struct obj_array oa;
	char encoded[] = "{\"elements\":["
		"{\"name\":\"Sim\303\263n Bol\303\255var\",\"height\":168},"
		"{\"name\":\"Muggsy Bogues\",\"height\":160},"
		"{\"name\":\"Pel\303\251\",\"height\":173},"
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
			[0] = { .name = "Sim\303\263n Bol\303\255var", .height = 168 },
			[1] = { .name = "Muggsy Bogues",               .height = 160 },
			[2] = { .name = "Pel\303\251",                 .height = 173 },
			[3] = { .name = "Hakeem Olajuwon",             .height = 213 },
			[4] = { .name = "Alex Honnold",                .height = 180 },
			[5] = { .name = "Hazel Findlay",               .height = 157 },
			[6] = { .name = "Daila Ojeda",                 .height = 158 },
			[7] = { .name = "Albert Einstein",             .height = 172 },
			[8] = { .name = "Usain Bolt",                  .height = 195 },
			[9] = { .name = "Paavo Nurmi",                 .height = 174 },
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
						.height = 168
					},
					[1] = {
						.name = "Pel\303\251",
						.height = 173
					},
					[2] = {
						.name = "Usain Bolt",
						.height = 195
					},
				},
				.num_elements = 3
			},
			[1] = {
				.elements = {
					[0] = {
						.name = "Muggsy Bogues",
						.height = 160
					},
					[1] = {
						.name = "Hakeem Olajuwon",
						.height = 213
					},
				},
				.num_elements = 2
			},
			[2] = {
				.elements = {
					[0] = {
						.name = "Alex Honnold",
						.height = 180
					},
					[1] = {
						.name = "Hazel Findlay",
						.height = 157
					},
					[2] = {
						.name = "Daila Ojeda",
						.height = 158
					},
					[3] = {
						.name = "Albert Einstein",
						.height = 172
					},
				},
				.num_elements = 4
			},
		},
		.objects_array_array_len = 3,
	};
	char encoded[] = "{\"objects_array_array\":["
		"[{\"name\":\"Sim\303\263n Bol\303\255var\",\"height\":168},"
		 "{\"name\":\"Pel\303\251\",\"height\":173},"
		 "{\"name\":\"Usain Bolt\",\"height\":195}],"
		"[{\"name\":\"Muggsy Bogues\",\"height\":160},"
		 "{\"name\":\"Hakeem Olajuwon\",\"height\":213}],"
		"[{\"name\":\"Alex Honnold\",\"height\":180},"
		 "{\"name\":\"Hazel Findlay\",\"height\":157},"
		 "{\"name\":\"Daila Ojeda\",\"height\":158},"
		 "{\"name\":\"Albert Einstein\",\"height\":172}]"
		"]}";
	char buffer[sizeof(encoded)];
	int ret;

	ret = json_obj_encode_buf(array_2dim_descr, ARRAY_SIZE(array_2dim_descr),
				  &obj_array_array_ts, buffer, sizeof(buffer));
	zassert_equal(ret, 0, "Encoding two-dimensional array returned error");
	zassert_true(!strcmp(buffer, encoded),
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
						.height = 168
					},
					[1] = {
						.name = "Pel\303\251",
						.height = 173
					},
					[2] = {
						.name = "Usain Bolt",
						.height = 195
					},
				},
				.num_elements = 3
			},
			[1] = {
				.elements = {
					[0] = {
						.name = "Muggsy Bogues",
						.height = 160
					},
					[1] = {
						.name = "Hakeem Olajuwon",
						.height = 213
					},
				},
				.num_elements = 2
			},
			[2] = {
				.elements = {
					[0] = {
						.name = "Alex Honnold",
						.height = 180
					},
					[1] = {
						.name = "Hazel Findlay",
						.height = 157
					},
					[2] = {
						.name = "Daila Ojeda",
						.height = 158
					},
					[3] = {
						.name = "Albert Einstein",
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
		"[{\"name\":\"Sim\303\263n Bol\303\255var\",\"height\":168},"
		 "{\"name\":\"Pel\303\251\",\"height\":173},"
		 "{\"name\":\"Usain Bolt\",\"height\":195}],"
		"[{\"name\":\"Muggsy Bogues\",\"height\":160},"
		 "{\"name\":\"Hakeem Olajuwon\",\"height\":213}],"
		"[{\"name\":\"Alex Honnold\",\"height\":180},"
		 "{\"name\":\"Hazel Findlay\",\"height\":157},"
		 "{\"name\":\"Daila Ojeda\",\"height\":158},"
		 "{\"name\":\"Albert Einstein\",\"height\":172}]"
		"]}";
	char buffer[sizeof(encoded)];
	int ret;

	ret = json_obj_encode_buf(array_2dim_extra_descr, ARRAY_SIZE(array_2dim_extra_descr),
				  &obj_array_2dim_extra_ts, buffer, sizeof(buffer));
	zassert_equal(ret, 0, "Encoding two-dimensional extra array returned error");
	zassert_true(!strcmp(buffer, encoded),
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
						.height = 168
					},
					[1] = {
						.name = "Pel\303\251",
						.height = 173
					},
					[2] = {
						.name = "Usain Bolt",
						.height = 195
					},
				},
				.num_elements = 3
			},
			[1] = {
				.elements = {
					[0] = {
						.name = "Muggsy Bogues",
						.height = 160
					},
					[1] = {
						.name = "Hakeem Olajuwon",
						.height = 213
					},
				},
				.num_elements = 2
			},
			[2] = {
				.elements = {
					[0] = {
						.name = "Alex Honnold",
						.height = 180
					},
					[1] = {
						.name = "Hazel Findlay",
						.height = 157
					},
					[2] = {
						.name = "Daila Ojeda",
						.height = 158
					},
					[3] = {
						.name = "Albert Einstein",
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
		"[{\"name\":\"Sim\303\263n Bol\303\255var\",\"height\":168},"
		 "{\"name\":\"Pel\303\251\",\"height\":173},"
		 "{\"name\":\"Usain Bolt\",\"height\":195}],"
		"[{\"name\":\"Muggsy Bogues\",\"height\":160},"
		 "{\"name\":\"Hakeem Olajuwon\",\"height\":213}],"
		"[{\"name\":\"Alex Honnold\",\"height\":180},"
		 "{\"name\":\"Hazel Findlay\",\"height\":157},"
		 "{\"name\":\"Daila Ojeda\",\"height\":158},"
		 "{\"name\":\"Albert Einstein\",\"height\":172}]"
		"]}";
	char buffer[sizeof(encoded)];
	int ret;

	ret = json_obj_encode_buf(array_2dim_extra_named_descr,
				  ARRAY_SIZE(array_2dim_extra_named_descr),
				  &obj_array_2dim_extra_ts, buffer, sizeof(buffer));
	zassert_equal(ret, 0, "Encoding two-dimensional extra named array returned error");
	zassert_true(!strcmp(buffer, encoded),
		     "Encoded two-dimensional extra named array is not consistent");
}

ZTEST(lib_json_test, test_json_2dim_obj_arr_decoding)
{
	struct obj_array_2dim oaa;
	char encoded[] = "{\"objects_array_array\":["
		"[{\"name\":\"Sim\303\263n Bol\303\255var\",\"height\":168},"
		 "{\"name\":\"Pel\303\251\",\"height\":173},"
		 "{\"name\":\"Usain Bolt\",\"height\":195}],"
		"[{\"name\":\"Muggsy Bogues\",\"height\":160},"
		 "{\"name\":\"Hakeem Olajuwon\",\"height\":213}],"
		"[{\"name\":\"Alex Honnold\",\"height\":180},"
		 "{\"name\":\"Hazel Findlay\",\"height\":157},"
		 "{\"name\":\"Daila Ojeda\",\"height\":158},"
		 "{\"name\":\"Albert Einstein\",\"height\":172}]"
		"]}";
	const struct obj_array_2dim expected = {
		.objects_array_array = {
			[0] = {
				.elements = {
					[0] = {
						.name = "Sim\303\263n Bol\303\255var",
						.height = 168
					},
					[1] = {
						.name = "Pel\303\251",
						.height = 173
					},
					[2] = {
						.name = "Usain Bolt",
						.height = 195
					},
				},
				.num_elements = 3
			},
			[1] = {
				.elements = {
					[0] = {
						.name = "Muggsy Bogues",
						.height = 160
					},
					[1] = {
						.name = "Hakeem Olajuwon",
						.height = 213
					},
				},
				.num_elements = 2
			},
			[2] = {
				.elements = {
					[0] = {
						.name = "Alex Honnold",
						.height = 180
					},
					[1] = {
						.name = "Hazel Findlay",
						.height = 157
					},
					[2] = {
						.name = "Daila Ojeda",
						.height = 158
					},
					[3] = {
						.name = "Albert Einstein",
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
			zassert_equal(oaa.objects_array_array[i].elements[j].height,
				      expected.objects_array_array[i].elements[j].height,
				      "Element [%d][%d] height not decoded correctly", i, j);
		}
	}
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
	zassert_true(!strcmp(buf, expected),
		     "Escaped value is not correct");
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
	zassert_true(!strcmp(buf, expected),
		     "Escaped value is not correct");
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
	zassert_true(!strcmp(nothing_to_escape, expected),
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

	zassert_false(ret < 0, "json_obj_parse returned error %d", ret);
	zassert_false(ret & ((int64_t)1 << 2), "Field int2 erroneously decoded");
	zassert_false(ret & ((int64_t)1 << 35), "Field int35 erroneously decoded");
	zassert_true(ret & ((int64_t)1 << 1), "Field int1 not decoded");
	zassert_true(ret & ((int64_t)1 << 21), "Field int21 not decoded");
	zassert_true(ret & ((int64_t)1 << 31), "Field int31 not decoded");
	zassert_true(ret & ((int64_t)1 << 39), "Field int39 not decoded");
}

ZTEST_SUITE(lib_json_test, NULL, NULL, NULL, NULL, NULL);
