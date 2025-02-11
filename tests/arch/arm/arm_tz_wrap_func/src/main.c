/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <cortex_m/tz_ns.h>
#include <cmsis_core.h>

static bool expect_preface;
static bool expect_postface;
static bool expect_foo1;
static bool preface_called;
static bool postface_called;
static bool foo1_called;
static uint32_t foo1_retval;
static uint32_t foo1_arg1;
static uint32_t foo1_arg2;
static uint32_t foo1_arg3;
static uint32_t foo1_arg4;

void reset_mocks(void)
{
	expect_preface = true;
	foo1_called = false;
	preface_called = false;
	postface_called = false;
	foo1_retval = 0;
	foo1_arg1 = 0;
	foo1_arg2 = 0;
	foo1_arg3 = 0;
	foo1_arg4 = 0;
}


void preface(void)
{
	zassert_true(expect_preface, "%s unexpectedly called", __func__);
	expect_preface = false;
	preface_called = true;
	expect_foo1 = true;
}


uint32_t foo1(uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4)
{
	zassert_true(expect_foo1, "%s unexpectedly called", __func__);
	zassert_equal(arg1, foo1_arg1, "Was 0x%"PRIx32", expected 0x%"PRIx32,
		arg1, foo1_arg1);
	zassert_equal(arg2, foo1_arg2);
	zassert_equal(arg3, foo1_arg3);
	zassert_equal(arg4, foo1_arg4);
	expect_foo1 = false;
	foo1_called = true;
	expect_postface = true;
	return foo1_retval;
}


void postface(void)
{
	zassert_true(expect_postface, "%s unexpectedly called", __func__);
	expect_postface = false;
	postface_called = true;
}


uint32_t __attribute__((naked)) wrap_foo1(uint32_t arg1, uint32_t arg2,
				uint32_t arg3, uint32_t arg4)
{
	__TZ_WRAP_FUNC(preface, foo1, postface);
}


ZTEST(tz_wrap_func, test_tz_wrap_func)
{
	reset_mocks();
	foo1_retval = 0x01234567;
	foo1_arg1 = 0x12345678;
	foo1_arg2 = 0x23456789;
	foo1_arg3 = 0x3456789a;
	foo1_arg4 = 0x456789ab;

	uint32_t msp1, psp1;
	msp1 = __get_MSP();
	psp1 = __get_PSP();

	zassert_equal(foo1_retval,
		wrap_foo1(foo1_arg1, foo1_arg2, foo1_arg3, foo1_arg4), NULL);

	zassert_equal(msp1, __get_MSP());
	zassert_equal(psp1, __get_PSP());

	zassert_true(preface_called);
	zassert_true(foo1_called);
	zassert_true(postface_called);
	zassert_false(expect_preface);
	zassert_false(expect_foo1);
	zassert_false(expect_postface);
}

ZTEST_SUITE(tz_wrap_func, NULL, NULL, NULL, NULL, NULL);
