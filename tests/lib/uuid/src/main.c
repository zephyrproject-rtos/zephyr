/*
 * Copyright (c) 2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/sys/uuid.h>

#ifdef CONFIG_UUID_V4
ZTEST(uuid, test_uuid_v4)
{
	struct uuid gen_uuid = {0};

	int result = uuid_generate_v4(&gen_uuid);
	/* Check version and variant fields */
	zassert_true(result == 0, "uuid_generate_v4 returned an error");
	zassert_equal(gen_uuid.val[6U] >> 4U, 4U,
		      "Generated UUID v4 contains an incorrect 'ver' field");
	zassert_equal(gen_uuid.val[8U] >> 6U, 2U,
		      "Generated UUID v4 contains an incorrect 'var' field");
}
#else
ZTEST(uuid, test_uuid_v4)
{
	ztest_test_skip();
}
#endif

#ifdef CONFIG_UUID_V5
ZTEST(uuid, test_uuid_v5)
{
	struct uuid namespace;
	int result;
	struct uuid gen_uuid;
	char uuid_str[UUID_STR_LEN];

	result = uuid_from_string("6ba7b810-9dad-11d1-80b4-00c04fd430c8", &namespace);
	zassert_true(result == 0, "uuid_from_string returned an error");
	result = uuid_generate_v5(&namespace, "www.example.com", 15, &gen_uuid);
	zassert_true(result == 0, "uuid_generate_v5 returned an error");
	result = uuid_to_string(&gen_uuid, uuid_str);
	zassert_true(result == 0, "uuid_to_string returned an error");
	zassert_str_equal("2ed6657d-e927-568b-95e1-2665a8aea6a2", uuid_str,
			  "uuid_str != 2ed6657d-e927-568b-95e1-2665a8aea6a2");
}
#else
ZTEST(uuid, test_uuid_v5)
{
	ztest_test_skip();
}
#endif

ZTEST(uuid, test_uuid_copy)
{
	int result;
	const char *data_str = "6ba7b810-9dad-11d1-80b4-00c04fd430c8";
	struct uuid data;
	struct uuid out;
	char out_str[UUID_STR_LEN] = {0};

	result = uuid_from_string(data_str, &data);
	zassert_true(result == 0, "uuid_from_string returned an error");
	result = uuid_copy(&data, &out);
	zassert_true(result == 0, "uuid_copy returned an error");
	result = uuid_to_string(&out, out_str);
	zassert_true(result == 0, "uuid_to_string returned an error");
	zassert_str_equal(out_str, data_str, "Expected %s, gotten: %s", data_str, out_str);
}

ZTEST(uuid, test_uuid_from_buffer)
{
	uint8_t uuid_buffer[UUID_SIZE] = {0x44, 0xb3, 0x5f, 0x73, 0xcf, 0xbd, 0x43, 0xb4,
					  0x8f, 0xef, 0xca, 0x7b, 0xae, 0xa1, 0x37, 0x5f};
	struct uuid gen_uuid;
	char uuid_string[UUID_STR_LEN] = {0};
	const char *expected_uuid_string = "44b35f73-cfbd-43b4-8fef-ca7baea1375f";
	int res;

	res = uuid_from_buffer(uuid_buffer, &gen_uuid);
	zassert_equal(0, res, "uuid_from_buffer returned an error");
	res = uuid_to_string(&gen_uuid, uuid_string);
	zassert_equal(0, res, "uuid_to_string returned an error");
	zassert_str_equal(expected_uuid_string, uuid_string, "expected %s", expected_uuid_string);
}

ZTEST(uuid, test_uuid_from_string)
{
	const char *first_uuid_v4_string = "44b35f73-cfbd-43b4-8fef-ca7baea1375f";
	const char *second_uuid_v4_string = "6f2fd4cb-94a0-41c7-8d27-864c6b13b8c0";
	const char *third_uuid_v4_string = "8f65dbbc-5868-4015-8523-891cc0bffa58";
	const char *first_uuid_v5_string = "0575a569-51eb-575c-afe4-ce7fc03bcdc5";

	struct uuid first_uuid_v4;
	struct uuid second_uuid_v4;
	struct uuid third_uuid_v4;
	struct uuid first_uuid_v5;

	uint8_t expected_first_uuid_v4_byte_array[UUID_SIZE] = {0x44, 0xb3, 0x5f, 0x73, 0xcf, 0xbd,
								0x43, 0xb4, 0x8f, 0xef, 0xca, 0x7b,
								0xae, 0xa1, 0x37, 0x5f};
	uint8_t expected_second_uuid_v4_byte_array[UUID_SIZE] = {0x6f, 0x2f, 0xd4, 0xcb, 0x94, 0xa0,
								 0x41, 0xc7, 0x8d, 0x27, 0x86, 0x4c,
								 0x6b, 0x13, 0xb8, 0xc0};
	uint8_t expected_third_uuid_v4_byte_array[UUID_SIZE] = {0x8f, 0x65, 0xdb, 0xbc, 0x58, 0x68,
								0x40, 0x15, 0x85, 0x23, 0x89, 0x1c,
								0xc0, 0xbf, 0xfa, 0x58};
	uint8_t expected_first_uuid_v5_byte_array[UUID_SIZE] = {0x05, 0x75, 0xa5, 0x69, 0x51, 0xeb,
								0x57, 0x5c, 0xaf, 0xe4, 0xce, 0x7f,
								0xc0, 0x3b, 0xcd, 0xc5};

	int res;

	res = uuid_from_string(first_uuid_v4_string, &first_uuid_v4);
	zassert_equal(0, res, "uuid_from_string returned an error");
	zassert_mem_equal(first_uuid_v4.val, expected_first_uuid_v4_byte_array, UUID_SIZE,
			  "first_uuid != from expected value");

	res = uuid_from_string(second_uuid_v4_string, &second_uuid_v4);
	zassert_equal(0, res, "uuid_from_string returned an error");
	zassert_mem_equal(second_uuid_v4.val, expected_second_uuid_v4_byte_array, UUID_SIZE,
			  "second_uuid != from expected value");

	res = uuid_from_string(third_uuid_v4_string, &third_uuid_v4);
	zassert_equal(0, res, "uuid_from_string returned an error");
	zassert_mem_equal(third_uuid_v4.val, expected_third_uuid_v4_byte_array, UUID_SIZE,
			  "third_uuid != from expected value");

	res = uuid_from_string(first_uuid_v5_string, &first_uuid_v5);
	zassert_equal(0, res, "uuid_from_string returned an error");
	zassert_mem_equal(first_uuid_v5.val, expected_first_uuid_v5_byte_array, UUID_SIZE,
			  "uuid_v5 != from expected value");
}

ZTEST(uuid, test_uuid_from_string_errors)
{
	const char *uuid_string_missing_hyphen = "44b35f73-cfbd-43b4-8fef0ca7baea1375f";
	const char *uuid_string_non_hex_digit = "44b35f73-cfLd-43b4-8fef-ca7baea1375f";

	int res;
	struct uuid gen_uuid;

	res = uuid_from_string(NULL, &gen_uuid);
	zassert_equal(-EINVAL, res, "uuid_from_string returned incorrect error");

	res = uuid_from_string(uuid_string_missing_hyphen, &gen_uuid);
	zassert_equal(-EINVAL, res, "uuid_from_string returned incorrect error");

	res = uuid_from_string(uuid_string_non_hex_digit, &gen_uuid);
	zassert_equal(-EINVAL, res, "uuid_from_string returned incorrect error");
}

ZTEST(uuid, test_uuid_to_buffer)
{
	int result;
	const char *input_str = "44b35f73-cfbd-43b4-8fef-ca7baea1375f";
	struct uuid input;
	uint8_t buffer[UUID_SIZE] = {0};
	uint8_t expected_buffer[UUID_SIZE] = {0x44, 0xb3, 0x5f, 0x73, 0xcf, 0xbd, 0x43, 0xb4,
					      0x8f, 0xef, 0xca, 0x7b, 0xae, 0xa1, 0x37, 0x5f};

	result = uuid_from_string(input_str, &input);
	zassert_true(result == 0, "uuid_from_string returned an error");
	result = uuid_to_buffer(&input, buffer);
	zassert_true(result == 0, "uuid_to_buffer returned an error");
	zassert_mem_equal(buffer, expected_buffer, UUID_SIZE,
			  "Incorrect buffer converted to buffer");
}

ZTEST(uuid, test_uuid_to_string)
{
	struct uuid first_uuid_v4 = {.val = {0x44, 0xb3, 0x5f, 0x73, 0xcf, 0xbd, 0x43, 0xb4, 0x8f,
					     0xef, 0xca, 0x7b, 0xae, 0xa1, 0x37, 0x5f}};
	struct uuid second_uuid_v4 = {.val = {0x6f, 0x2f, 0xd4, 0xcb, 0x94, 0xa0, 0x41, 0xc7, 0x8d,
					      0x27, 0x86, 0x4c, 0x6b, 0x13, 0xb8, 0xc0}};
	struct uuid first_uuid_v5 = {.val = {0x05, 0x75, 0xa5, 0x69, 0x51, 0xeb, 0x57, 0x5c, 0xaf,
					     0xe4, 0xce, 0x7f, 0xc0, 0x3b, 0xcd, 0xc5}};

	char first_uuid_v4_string[UUID_STR_LEN];
	char second_uuid_v4_string[UUID_STR_LEN];
	char first_uuid_v5_string[UUID_STR_LEN];

	const char *expected_first_uuid_v4_string = "44b35f73-cfbd-43b4-8fef-ca7baea1375f";
	const char *expected_second_uuid_v4_string = "6f2fd4cb-94a0-41c7-8d27-864c6b13b8c0";
	const char *expected_first_uuid_v5_string = "0575a569-51eb-575c-afe4-ce7fc03bcdc5";

	int err;

	err = uuid_to_string(&first_uuid_v4, first_uuid_v4_string);
	zassert_equal(0, err, "uuid_to_string returned an error");
	zassert_str_equal(expected_first_uuid_v4_string, first_uuid_v4_string, "expected %s",
			  expected_first_uuid_v4_string);

	err = uuid_to_string(&second_uuid_v4, second_uuid_v4_string);
	zassert_equal(0, err, "uuid_to_string returned an error");
	zassert_str_equal(expected_second_uuid_v4_string, second_uuid_v4_string, "expected %s",
			  expected_second_uuid_v4_string);

	err = uuid_to_string(&first_uuid_v5, first_uuid_v5_string);
	zassert_equal(0, err, "uuid_to_string returned an error");
	zassert_str_equal(expected_first_uuid_v5_string, first_uuid_v5_string, "expected %s",
			  expected_first_uuid_v5_string);
}

#ifdef CONFIG_UUID_BASE64
ZTEST(uuid, test_uuid_to_base64)
{
	char first_uuid_v4_base64[UUID_BASE64_LEN] = {0};
	char second_uuid_v4_base64[UUID_BASE64_LEN] = {0};
	char first_uuid_v5_base64[UUID_BASE64_LEN] = {0};

	struct uuid first_uuid_v4 = {.val = {0x44, 0xb3, 0x5f, 0x73, 0xcf, 0xbd, 0x43, 0xb4, 0x8f,
					     0xef, 0xca, 0x7b, 0xae, 0xa1, 0x37, 0x5f}};
	struct uuid second_uuid_v4 = {.val = {0x6f, 0x2f, 0xd4, 0xcb, 0x94, 0xa0, 0x41, 0xc7, 0x8d,
					      0x27, 0x86, 0x4c, 0x6b, 0x13, 0xb8, 0xc0}};
	struct uuid first_uuid_v5 = {.val = {0x05, 0x75, 0xa5, 0x69, 0x51, 0xeb, 0x57, 0x5c, 0xaf,
					     0xe4, 0xce, 0x7f, 0xc0, 0x3b, 0xcd, 0xc5}};

	char expected_first_uuid_v4_base64[] = "RLNfc8+9Q7SP78p7rqE3Xw==";
	char expected_second_uuid_v4_base64[] = "by/Uy5SgQceNJ4ZMaxO4wA==";
	char expected_first_uuid_v5_base64[] = "BXWlaVHrV1yv5M5/wDvNxQ==";

	int err;

	err = uuid_to_base64(&first_uuid_v4, first_uuid_v4_base64);
	zassert_equal(0, err, "uuid_to_base64 returned an error");
	zassert_str_equal(expected_first_uuid_v4_base64, first_uuid_v4_base64,
			  "expected: '%s', gotten: '%s'", expected_first_uuid_v4_base64,
			  first_uuid_v4_base64);

	err = uuid_to_base64(&second_uuid_v4, second_uuid_v4_base64);
	zassert_equal(0, err, "uuid_to_base64 returned an error");
	zassert_str_equal(expected_second_uuid_v4_base64, second_uuid_v4_base64,
			  "expected: '%s', gotten: '%s'", expected_second_uuid_v4_base64,
			  second_uuid_v4_base64);

	err = uuid_to_base64(&first_uuid_v5, first_uuid_v5_base64);
	zassert_equal(0, err, "uuid_to_base64 returned an error");
	zassert_str_equal(expected_first_uuid_v5_base64, first_uuid_v5_base64,
			  "expected: '%s', gotten: '%s'", expected_first_uuid_v5_base64,
			  first_uuid_v5_base64);
}
#else
ZTEST(uuid, test_uuid_to_base64)
{
	ztest_test_skip();
}
#endif

#ifdef CONFIG_UUID_BASE64
ZTEST(uuid, test_uuid_to_base64url)
{
	char first_uuid_v4_base64url[UUID_BASE64URL_LEN] = {0};
	char second_uuid_v4_base64url[UUID_BASE64URL_LEN] = {0};
	char first_uuid_v5_base64url[UUID_BASE64URL_LEN] = {0};

	struct uuid first_uuid_v4 = {.val = {0x44, 0xb3, 0x5f, 0x73, 0xcf, 0xbd, 0x43, 0xb4, 0x8f,
					     0xef, 0xca, 0x7b, 0xae, 0xa1, 0x37, 0x5f}};
	struct uuid second_uuid_v4 = {.val = {0x6f, 0x2f, 0xd4, 0xcb, 0x94, 0xa0, 0x41, 0xc7, 0x8d,
					      0x27, 0x86, 0x4c, 0x6b, 0x13, 0xb8, 0xc0}};
	struct uuid first_uuid_v5 = {.val = {0x05, 0x75, 0xa5, 0x69, 0x51, 0xeb, 0x57, 0x5c, 0xaf,
					     0xe4, 0xce, 0x7f, 0xc0, 0x3b, 0xcd, 0xc5}};

	char expected_first_uuid_v4_base64url[] = "RLNfc8-9Q7SP78p7rqE3Xw";
	char expected_second_uuid_v4_base64url[] = "by_Uy5SgQceNJ4ZMaxO4wA";
	char expected_first_uuid_v5_base64url[] = "BXWlaVHrV1yv5M5_wDvNxQ";

	int err;

	err = uuid_to_base64url(&first_uuid_v4, first_uuid_v4_base64url);
	zassert_equal(0, err, "uuid_to_base64url returned an error");
	zassert_str_equal(expected_first_uuid_v4_base64url, first_uuid_v4_base64url,
			  "expected: '%s', gotten: '%s'", expected_first_uuid_v4_base64url,
			  first_uuid_v4_base64url);

	err = uuid_to_base64url(&second_uuid_v4, second_uuid_v4_base64url);
	zassert_equal(0, err, "uuid_to_base64url returned an error");
	zassert_str_equal(expected_second_uuid_v4_base64url, second_uuid_v4_base64url,
			  "expected: '%s', gotten: '%s'", expected_second_uuid_v4_base64url,
			  second_uuid_v4_base64url);

	err = uuid_to_base64url(&first_uuid_v5, first_uuid_v5_base64url);
	zassert_equal(0, err, "uuid_to_base64url returned an error");
	zassert_str_equal(expected_first_uuid_v5_base64url, first_uuid_v5_base64url,
			  "expected: '%s', gotten: '%s'", expected_first_uuid_v5_base64url,
			  first_uuid_v5_base64url);
}
#else
ZTEST(uuid, test_uuid_to_base64url)
{
	ztest_test_skip();
}
#endif

ZTEST_SUITE(uuid, NULL, NULL, NULL, NULL, NULL);
