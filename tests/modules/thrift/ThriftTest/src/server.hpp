/*
 * Copyright 2022 Young Mei
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <cstdio>

#include <sstream>
#include <stdexcept>

#include "SecondService.h"
#include "ThriftTest.h"

using namespace std;

using namespace apache::thrift;
using namespace apache::thrift::transport;
using namespace thrift::test;

class TestHandler : public ThriftTestIf
{
public:
	TestHandler() = default;

	void testVoid() override
	{
		printf("testVoid()\n");
	}

	void testString(string &out, const string &thing) override
	{
		printf("testString(\"%s\")\n", thing.c_str());
		out = thing;
	}

	bool testBool(const bool thing) override
	{
		printf("testBool(%s)\n", thing ? "true" : "false");
		return thing;
	}

	int8_t testByte(const int8_t thing) override
	{
		printf("testByte(%d)\n", (int)thing);
		return thing;
	}

	int32_t testI32(const int32_t thing) override
	{
		printf("testI32(%d)\n", thing);
		return thing;
	}

	int64_t testI64(const int64_t thing) override
	{
		printf("testI64(%" PRId64 ")\n", thing);
		return thing;
	}

	double testDouble(const double thing) override
	{
		printf("testDouble(%f)\n", thing);
		return thing;
	}

	void testBinary(std::string &_return, const std::string &thing) override
	{
		std::ostringstream hexstr;

		hexstr << std::hex << thing;
		printf("testBinary(%lu: %s)\n", safe_numeric_cast<unsigned long>(thing.size()),
		       hexstr.str().c_str());
		_return = thing;
	}

	void testStruct(Xtruct &out, const Xtruct &thing) override
	{
		printf("testStruct({\"%s\", %d, %d, %" PRId64 "})\n", thing.string_thing.c_str(),
		       (int)thing.byte_thing, thing.i32_thing, thing.i64_thing);
		out = thing;
	}

	void testNest(Xtruct2 &out, const Xtruct2 &nest) override
	{
		const Xtruct &thing = nest.struct_thing;

		printf("testNest({%d, {\"%s\", %d, %d, %" PRId64 "}, %d})\n", (int)nest.byte_thing,
		       thing.string_thing.c_str(), (int)thing.byte_thing, thing.i32_thing,
		       thing.i64_thing, nest.i32_thing);
		out = nest;
	}

	void testMap(map<int32_t, int32_t> &out, const map<int32_t, int32_t> &thing) override
	{
		map<int32_t, int32_t>::const_iterator m_iter;
		bool first = true;

		printf("testMap({");
		for (m_iter = thing.begin(); m_iter != thing.end(); ++m_iter) {
			if (first) {
				first = false;
			} else {
				printf(", ");
			}

			printf("%d => %d", m_iter->first, m_iter->second);
		}

		printf("})\n");
		out = thing;
	}

	void testStringMap(map<std::string, std::string> &out,
			   const map<std::string, std::string> &thing) override
	{
		map<std::string, std::string>::const_iterator m_iter;
		bool first = true;

		printf("testMap({");
		for (m_iter = thing.begin(); m_iter != thing.end(); ++m_iter) {
			if (first) {
				first = false;
			} else {
				printf(", ");
			}
			printf("%s => %s", (m_iter->first).c_str(), (m_iter->second).c_str());
		}

		printf("})\n");
		out = thing;
	}

	void testSet(set<int32_t> &out, const set<int32_t> &thing) override
	{
		set<int32_t>::const_iterator s_iter;
		bool first = true;

		printf("testSet({");
		for (s_iter = thing.begin(); s_iter != thing.end(); ++s_iter) {
			if (first) {
				first = false;
			} else {
				printf(", ");
			}

			printf("%d", *s_iter);
		}

		printf("})\n");
		out = thing;
	}

	void testList(vector<int32_t> &out, const vector<int32_t> &thing) override
	{
		vector<int32_t>::const_iterator l_iter;
		bool first = true;

		printf("testList({");
		for (l_iter = thing.begin(); l_iter != thing.end(); ++l_iter) {
			if (first) {
				first = false;
			} else {
				printf(", ");
			}
			printf("%d", *l_iter);
		}

		printf("})\n");
		out = thing;
	}

	Numberz::type testEnum(const Numberz::type thing) override
	{
		printf("testEnum(%d)\n", thing);
		return thing;
	}

	UserId testTypedef(const UserId thing) override
	{
		printf("testTypedef(%" PRId64 ")\n", thing);
		return thing;
	}

	void testMapMap(map<int32_t, map<int32_t, int32_t>> &mapmap, const int32_t hello) override
	{
		map<int32_t, int32_t> pos;
		map<int32_t, int32_t> neg;

		printf("testMapMap(%d)\n", hello);
		for (int i = 1; i < 5; i++) {
			pos.insert(make_pair(i, i));
			neg.insert(make_pair(-i, -i));
		}

		mapmap.insert(make_pair(4, pos));
		mapmap.insert(make_pair(-4, neg));
	}

	void testInsanity(map<UserId, map<Numberz::type, Insanity>> &insane,
			  const Insanity &argument) override
	{
		Insanity looney;
		map<Numberz::type, Insanity> first_map;
		map<Numberz::type, Insanity> second_map;

		first_map.insert(make_pair(Numberz::TWO, argument));
		first_map.insert(make_pair(Numberz::THREE, argument));

		second_map.insert(make_pair(Numberz::SIX, looney));

		insane.insert(make_pair(1, first_map));
		insane.insert(make_pair(2, second_map));

		printf("testInsanity()\n");
		printf("return");
		printf(" = {");
		map<UserId, map<Numberz::type, Insanity>>::const_iterator i_iter;

		for (i_iter = insane.begin(); i_iter != insane.end(); ++i_iter) {
			printf("%" PRId64 " => {", i_iter->first);
			map<Numberz::type, Insanity>::const_iterator i2_iter;

			for (i2_iter = i_iter->second.begin(); i2_iter != i_iter->second.end();
			     ++i2_iter) {
				printf("%d => {", i2_iter->first);
				map<Numberz::type, UserId> userMap = i2_iter->second.userMap;
				map<Numberz::type, UserId>::const_iterator um;

				printf("{");
				for (um = userMap.begin(); um != userMap.end(); ++um) {
					printf("%d => %" PRId64 ", ", um->first, um->second);
				}

				printf("}, ");
				vector<Xtruct> xtructs = i2_iter->second.xtructs;
				vector<Xtruct>::const_iterator x;

				printf("{");
				for (x = xtructs.begin(); x != xtructs.end(); ++x) {
					printf("{\"%s\", %d, %d, %" PRId64 "}, ",
					       x->string_thing.c_str(), (int)x->byte_thing,
					       x->i32_thing, x->i64_thing);
				}

				printf("}");
				printf("}, ");
			}

			printf("}, ");
		}

		printf("}\n");
	}

	void testMulti(Xtruct &hello, const int8_t arg0, const int32_t arg1, const int64_t arg2,
		       const std::map<int16_t, std::string> &arg3, const Numberz::type arg4,
		       const UserId arg5) override
	{
		(void)arg3;
		(void)arg4;
		(void)arg5;
		printf("testMulti()\n");
		hello.string_thing = "Hello2";
		hello.byte_thing = arg0;
		hello.i32_thing = arg1;
		hello.i64_thing = (int64_t)arg2;
	}

	void testException(const std::string &arg) override
	{
		printf("testException(%s)\n", arg.c_str());
		if (arg.compare("Xception") == 0) {
			Xception e;
			e.errorCode = 1001;
			e.message = arg;
			throw e;
		} else if (arg.compare("TException") == 0) {
			apache::thrift::TException e;
			throw e;
		} else {
			Xtruct result;
			result.string_thing = arg;
			return;
		}
	}

	void testMultiException(Xtruct &result, const std::string &arg0,
				const std::string &arg1) override
	{

		printf("testMultiException(%s, %s)\n", arg0.c_str(), arg1.c_str());
		if (arg0.compare("Xception") == 0) {
			Xception e;
			e.errorCode = 1001;
			e.message = "This is an Xception";
			throw e;
		} else if (arg0.compare("Xception2") == 0) {
			Xception2 e;
			e.errorCode = 2002;
			e.struct_thing.string_thing = "This is an Xception2";
			throw e;
		} else {
			result.string_thing = arg1;
			return;
		}
	}

	void testOneway(const int32_t aNum) override
	{
		printf("testOneway(%d): call received\n", aNum);
	}
};

class SecondHandler : public SecondServiceIf
{
public:
	void secondtestString(std::string &result, const std::string &thing) override
	{
		result = "testString(\"" + thing + "\")";
	}
};
