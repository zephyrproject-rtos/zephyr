/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/toolchain.h>

/**
 * @file
 * @brief Provide soft float function stubs for long double operations.
 *
 * GCC soft float does not support long double so these need to be
 * stubbed out.
 *
 * The function names come from the GCC public documentation.
 */

extern void abort(void);

__weak void __addtf3(long double a, long double b)
{
	k_oops();
}

__weak void __addxf3(long double a, long double b)
{
	k_oops();
}

__weak void __subtf3(long double a, long double b)
{
	k_oops();
}

__weak void __subxf3(long double a, long double b)
{
	k_oops();
}

__weak void __multf3(long double a, long double b)
{
	k_oops();
}

__weak void __mulxf3(long double a, long double b)
{
	k_oops();
}

__weak void __divtf3(long double a, long double b)
{
	k_oops();
}

__weak void __divxf3(long double a, long double b)
{
	k_oops();
}

__weak void __negtf2(long double a)
{
	k_oops();
}

__weak void __negxf2(long double a)
{
	k_oops();
}

__weak void __extendsftf2(float a)
{
	k_oops();
}

__weak void __extendsfxf2(float a)
{
	k_oops();
}

__weak void __extenddftf2(double a)
{
	k_oops();
}

__weak void __extenddfxf2(double a)
{
	k_oops();
}

__weak void __truncxfdf2(long double a)
{
	k_oops();
}

__weak void __trunctfdf2(long double a)
{
	k_oops();
}

__weak void __truncxfsf2(long double a)
{
	k_oops();
}

__weak void __trunctfsf2(long double a)
{
	k_oops();
}

__weak void __fixtfsi(long double a)
{
	k_oops();
}

__weak void __fixxfsi(long double a)
{
	k_oops();
}

__weak void __fixtfdi(long double a)
{
	k_oops();
}

__weak void __fixxfdi(long double a)
{
	k_oops();
}

__weak void __fixtfti(long double a)
{
	k_oops();
}

__weak void __fixxfti(long double a)
{
	k_oops();
}

__weak void __fixunstfsi(long double a)
{
	k_oops();
}

__weak void __fixunsxfsi(long double a)
{
	k_oops();
}

__weak void __fixunstfdi(long double a)
{
	k_oops();
}

__weak void __fixunsxfdi(long double a)
{
	k_oops();
}

__weak void __fixunstfti(long double a)
{
	k_oops();
}

__weak void __fixunsxfti(long double a)
{
	k_oops();
}

__weak void __floatsitf(int i)
{
	k_oops();
}

__weak void __floatsixf(int i)
{
	k_oops();
}

__weak void __floatditf(long i)
{
	k_oops();
}

__weak void __floatdixf(long i)
{
	k_oops();
}

__weak void __floattitf(long long i)
{
	k_oops();
}

__weak void __floattixf(long long i)
{
	k_oops();
}

__weak void __floatunsitf(unsigned int i)
{
	k_oops();
}

__weak void __floatunsixf(unsigned int i)
{
	k_oops();
}

__weak void __floatunditf(unsigned long i)
{
	k_oops();
}

__weak void __floatundixf(unsigned long i)
{
	k_oops();
}

__weak void __floatuntitf(unsigned long long i)
{
	k_oops();
}

__weak void __floatuntixf(unsigned long long i)
{
	k_oops();
}

__weak void __cmptf2(long double a, long double b)
{
	k_oops();
}

__weak void __unordtf2(long double a, long double b)
{
	k_oops();
}

__weak void __eqtf2(long double a, long double b)
{
	k_oops();
}

__weak void __netf2(long double a, long double b)
{
	k_oops();
}

__weak void __getf2(long double a, long double b)
{
	k_oops();
}

__weak void __lttf2(long double a, long double b)
{
	k_oops();
}

__weak void __letf2(long double a, long double b)
{
	k_oops();
}

__weak void __gttf2(long double a, long double b)
{
	k_oops();
}

__weak void __powitf2(long double a, int b)
{
	k_oops();
}

__weak void __powixf2(long double a, int b)
{
	k_oops();
}
