/*
 * Copyright 2022 Young Mei
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include <array>
#include <cfloat>
#include <locale>
#include <stdexcept>

#include <sys/time.h>

#include "context.hpp"

using namespace apache::thrift;

using namespace std;

static void init_Xtruct(Xtruct &s);

ZTEST(thrift, test_void)
{
	context.client->testVoid();
}

ZTEST(thrift, test_string)
{
	string s;
	context.client->testString(s, "Test");
	zassert_equal(s, "Test", "");
}

ZTEST(thrift, test_bool)
{
	zassert_equal(false, context.client->testBool(false), "");
	zassert_equal(true, context.client->testBool(true), "");
}

ZTEST(thrift, test_byte)
{
	zassert_equal(0, context.client->testByte(0), "");
	zassert_equal(-1, context.client->testByte(-1), "");
	zassert_equal(42, context.client->testByte(42), "");
	zassert_equal(-42, context.client->testByte(-42), "");
	zassert_equal(127, context.client->testByte(127), "");
	zassert_equal(-128, context.client->testByte(-128), "");
}

ZTEST(thrift, test_i32)
{
	zassert_equal(0, context.client->testI32(0), "");
	zassert_equal(-1, context.client->testI32(-1), "");
	zassert_equal(190000013, context.client->testI32(190000013), "");
	zassert_equal(-190000013, context.client->testI32(-190000013), "");
	zassert_equal(INT32_MAX, context.client->testI32(INT32_MAX), "");
	zassert_equal(INT32_MIN, context.client->testI32(INT32_MIN), "");
}

ZTEST(thrift, test_i64)
{
	zassert_equal(0, context.client->testI64(0), "");
	zassert_equal(-1, context.client->testI64(-1), "");
	zassert_equal(7000000000000000123LL, context.client->testI64(7000000000000000123LL), "");
	zassert_equal(-7000000000000000123LL, context.client->testI64(-7000000000000000123LL), "");
	zassert_equal(INT64_MAX, context.client->testI64(INT64_MAX), "");
	zassert_equal(INT64_MIN, context.client->testI64(INT64_MIN), "");
}

ZTEST(thrift, test_double)
{
	zassert_equal(0.0, context.client->testDouble(0.0), "");
	zassert_equal(-1.0, context.client->testDouble(-1.0), "");
	zassert_equal(-5.2098523, context.client->testDouble(-5.2098523), "");
	zassert_equal(-0.000341012439638598279,
		      context.client->testDouble(-0.000341012439638598279), "");
	zassert_equal(DBL_MAX, context.client->testDouble(DBL_MAX), "");
	zassert_equal(-DBL_MAX, context.client->testDouble(-DBL_MAX), "");
}

ZTEST(thrift, test_binary)
{
	string rsp;

	context.client->testBinary(rsp, "");
	zassert_equal("", rsp, "");
	context.client->testBinary(rsp, "Hello");
	zassert_equal("Hello", rsp, "");
	context.client->testBinary(rsp, "H\x03\x01\x01\x00");
	zassert_equal("H\x03\x01\x01\x00", rsp, "");
}

ZTEST(thrift, test_struct)
{
	Xtruct request_struct;
	init_Xtruct(request_struct);
	Xtruct response_struct;
	context.client->testStruct(response_struct, request_struct);

	zassert_equal(response_struct, request_struct, NULL);
}

ZTEST(thrift, test_nested_struct)
{
	Xtruct2 request_struct;
	request_struct.byte_thing = 1;
	init_Xtruct(request_struct.struct_thing);
	request_struct.i32_thing = 5;
	Xtruct2 response_struct;
	context.client->testNest(response_struct, request_struct);

	zassert_equal(response_struct, request_struct, NULL);
}

ZTEST(thrift, test_map)
{
	static const map<int32_t, int32_t> request_map = {
		{0, -10}, {1, -9}, {2, -8}, {3, -7}, {4, -6}};

	map<int32_t, int32_t> response_map;
	context.client->testMap(response_map, request_map);

	zassert_equal(request_map, response_map, "");
}

ZTEST(thrift, test_string_map)
{
	static const map<string, string> request_smap = {
		{"a", "2"}, {"b", "blah"}, {"some", "thing"}
	};
	map<string, string> response_smap;

	context.client->testStringMap(response_smap, request_smap);
	zassert_equal(response_smap, request_smap, "");
}

ZTEST(thrift, test_set)
{
	static const set<int32_t> request_set = {-2, -1, 0, 1, 2};

	set<int32_t> response_set;
	context.client->testSet(response_set, request_set);

	zassert_equal(request_set, response_set, "");
}

ZTEST(thrift, test_list)
{
	vector<int32_t> response_list;
	context.client->testList(response_list, vector<int32_t>());
	zassert_true(response_list.empty(), "Unexpected list size: %llu", response_list.size());

	static const vector<int32_t> request_list = {-2, -1, 0, 1, 2};

	response_list.clear();
	context.client->testList(response_list, request_list);
	zassert_equal(request_list, response_list, "");
}

ZTEST(thrift, test_enum)
{
	Numberz::type response = context.client->testEnum(Numberz::ONE);
	zassert_equal(response, Numberz::ONE, NULL);

	response = context.client->testEnum(Numberz::TWO);
	zassert_equal(response, Numberz::TWO, NULL);

	response = context.client->testEnum(Numberz::EIGHT);
	zassert_equal(response, Numberz::EIGHT, NULL);
}

ZTEST(thrift, test_typedef)
{
	UserId uid = context.client->testTypedef(309858235082523LL);
	zassert_equal(uid, 309858235082523LL, "Unexpected uid: %llu", uid);
}

ZTEST(thrift, test_nested_map)
{
	map<int32_t, map<int32_t, int32_t>> mm;
	context.client->testMapMap(mm, 1);

	zassert_equal(mm.size(), 2, NULL);
	zassert_equal(mm[-4][-4], -4, NULL);
	zassert_equal(mm[-4][-3], -3, NULL);
	zassert_equal(mm[-4][-2], -2, NULL);
	zassert_equal(mm[-4][-1], -1, NULL);
	zassert_equal(mm[4][4], 4, NULL);
	zassert_equal(mm[4][3], 3, NULL);
	zassert_equal(mm[4][2], 2, NULL);
	zassert_equal(mm[4][1], 1, NULL);
}

ZTEST(thrift, test_exception)
{
	std::exception_ptr eptr = nullptr;

	try {
		context.client->testException("Xception");
	} catch (...) {
		eptr = std::current_exception();
	}
	zassert_not_equal(nullptr, eptr, "an exception was not thrown");

	eptr = nullptr;
	try {
		context.client->testException("TException");
	} catch (...) {
		eptr = std::current_exception();
	}
	zassert_not_equal(nullptr, eptr, "an exception was not thrown");

	context.client->testException("success");
}

ZTEST(thrift, test_multi_exception)
{
	std::exception_ptr eptr = nullptr;

	try {
		Xtruct result;
		context.client->testMultiException(result, "Xception", "test 1");
	} catch (...) {
		eptr = std::current_exception();
	}
	zassert_not_equal(nullptr, eptr, "an exception was not thrown");

	eptr = nullptr;
	try {
		Xtruct result;
		context.client->testMultiException(result, "Xception2", "test 2");
	} catch (...) {
		eptr = std::current_exception();
	}
	zassert_not_equal(nullptr, eptr, "an exception was not thrown");
}

static void init_Xtruct(Xtruct &s)
{
	s.string_thing = "Zero";
	s.byte_thing = 1;
	s.i32_thing = -3;
	s.i64_thing = -5;
}
