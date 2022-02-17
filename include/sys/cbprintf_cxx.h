/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_CBPRINTF_CXX_H_
#define ZEPHYR_INCLUDE_SYS_CBPRINTF_CXX_H_

#ifdef __cplusplus

#include <sys/cbprintf_enums.h>

/* C++ version for detecting a pointer to a string. */
static inline int z_cbprintf_cxx_is_pchar(char *, bool const_as_fixed)
{
	ARG_UNUSED(const_as_fixed);
	return 1;
}

static inline int z_cbprintf_cxx_is_pchar(const char *, bool const_as_fixed)
{
	return const_as_fixed ? 0 : 1;
}

static inline int z_cbprintf_cxx_is_pchar(volatile char *, bool const_as_fixed)
{
	ARG_UNUSED(const_as_fixed);
	return 1;
}

static inline int z_cbprintf_cxx_is_pchar(const volatile char *, bool const_as_fixed)
{
	ARG_UNUSED(const_as_fixed);
	return 1;
}

static inline int z_cbprintf_cxx_is_pchar(wchar_t *, bool const_as_fixed)
{
	ARG_UNUSED(const_as_fixed);
	return 1;
}

static inline int z_cbprintf_cxx_is_pchar(const wchar_t *, bool const_as_fixed)
{
	return const_as_fixed ? 0 : 1;
}

static inline int z_cbprintf_cxx_is_pchar(volatile wchar_t *, bool const_as_fixed)
{
	ARG_UNUSED(const_as_fixed);
	return 1;
}

static inline int z_cbprintf_cxx_is_pchar(const volatile wchar_t *, bool const_as_fixed)
{
	ARG_UNUSED(const_as_fixed);
	return 1;
}

template < typename T >
static inline int z_cbprintf_cxx_is_pchar(T arg, bool const_as_fixed)
{
	ARG_UNUSED(arg);
	_Pragma("GCC diagnostic push")
	_Pragma("GCC diagnostic ignored \"-Wpointer-arith\"")
	ARG_UNUSED(const_as_fixed);
	return 0;
	_Pragma("GCC diagnostic pop")
}

/* C++ version for calculating argument size. */
static inline size_t z_cbprintf_cxx_arg_size(float f)
{
	ARG_UNUSED(f);

	return sizeof(double);
}

static inline size_t z_cbprintf_cxx_arg_size(void *p)
{
	ARG_UNUSED(p);

	return sizeof(void *);
}

template < typename T >
static inline size_t z_cbprintf_cxx_arg_size(T arg)
{
	return sizeof(arg + 0);
}

/* C++ version for storing arguments. */
static inline void z_cbprintf_cxx_store_arg(uint8_t *dst, float arg)
{
	double d = (double)arg;
	void *p = &d;

	z_cbprintf_wcpy((int *)dst, (int *)p, sizeof(d) / sizeof(int));
}

static inline void z_cbprintf_cxx_store_arg(uint8_t *dst, void *p)
{
	z_cbprintf_wcpy((int *)dst, (int *)&p, sizeof(p) / sizeof(int));
}

static inline void z_cbprintf_cxx_store_arg(uint8_t *dst, char arg)
{
	int tmp = arg + 0;

	z_cbprintf_wcpy((int *)dst, &tmp, 1);
}

static inline void z_cbprintf_cxx_store_arg(uint8_t *dst, unsigned char arg)
{
	int tmp = arg + 0;

	z_cbprintf_wcpy((int *)dst, &tmp, 1);
}

static inline void z_cbprintf_cxx_store_arg(uint8_t *dst, signed char arg)
{
	int tmp = arg + 0;

	z_cbprintf_wcpy((int *)dst, &tmp, 1);
}

static inline void z_cbprintf_cxx_store_arg(uint8_t *dst, short arg)
{
	int tmp = arg + 0;

	z_cbprintf_wcpy((int *)dst, &tmp, 1);
}

static inline void z_cbprintf_cxx_store_arg(uint8_t *dst, unsigned short arg)
{
	int tmp = arg + 0;

	z_cbprintf_wcpy((int *)dst, &tmp, 1);
}

template < typename T >
static inline void z_cbprintf_cxx_store_arg(uint8_t *dst, T arg)
{
	size_t wlen = z_cbprintf_cxx_arg_size(arg) / sizeof(int);
	void *p = &arg;

	z_cbprintf_wcpy((int *)dst, (int *)p, wlen);
}

/* C++ version for long double detection. */
static inline int z_cbprintf_cxx_is_longdouble(long double arg)
{
	ARG_UNUSED(arg);
	return 1;
}

template < typename T >
static inline int z_cbprintf_cxx_is_longdouble(T arg)
{
	ARG_UNUSED(arg);

	return 0;
}

/* C++ version for caluculating argument alignment. */
static inline size_t z_cbprintf_cxx_alignment(float arg)
{
	ARG_UNUSED(arg);

	return VA_STACK_ALIGN(double);
}

static inline size_t z_cbprintf_cxx_alignment(double arg)
{
	ARG_UNUSED(arg);

	return VA_STACK_ALIGN(double);
}

static inline size_t z_cbprintf_cxx_alignment(long double arg)
{
	ARG_UNUSED(arg);

	return VA_STACK_ALIGN(long double);
}

static inline size_t z_cbprintf_cxx_alignment(long long arg)
{
	ARG_UNUSED(arg);

	return VA_STACK_ALIGN(long long);
}

static inline size_t z_cbprintf_cxx_alignment(unsigned long long arg)
{
	ARG_UNUSED(arg);

	return VA_STACK_ALIGN(long long);
}

template < typename T >
static inline size_t z_cbprintf_cxx_alignment(T arg)
{
	return MAX(__alignof__(arg), VA_STACK_MIN_ALIGN);
}

/* C++ version for returning argument type enum value. */
static inline size_t z_cbprintf_cxx_arg_type(char arg)
{
	ARG_UNUSED(arg);

	return CBPRINTF_PACKAGE_ARG_TYPE_CHAR;
}

static inline size_t z_cbprintf_cxx_arg_type(unsigned char arg)
{
	ARG_UNUSED(arg);

	return CBPRINTF_PACKAGE_ARG_TYPE_UNSIGNED_CHAR;
}

static inline size_t z_cbprintf_cxx_arg_type(short arg)
{
	ARG_UNUSED(arg);

	return CBPRINTF_PACKAGE_ARG_TYPE_SHORT;
}

static inline size_t z_cbprintf_cxx_arg_type(unsigned short arg)
{
	ARG_UNUSED(arg);

	return CBPRINTF_PACKAGE_ARG_TYPE_UNSIGNED_SHORT;
}

static inline size_t z_cbprintf_cxx_arg_type(int arg)
{
	ARG_UNUSED(arg);

	return CBPRINTF_PACKAGE_ARG_TYPE_INT;
}

static inline size_t z_cbprintf_cxx_arg_type(unsigned int arg)
{
	ARG_UNUSED(arg);

	return CBPRINTF_PACKAGE_ARG_TYPE_UNSIGNED_INT;
}

static inline size_t z_cbprintf_cxx_arg_type(long arg)
{
	ARG_UNUSED(arg);

	return CBPRINTF_PACKAGE_ARG_TYPE_LONG;
}

static inline size_t z_cbprintf_cxx_arg_type(unsigned long arg)
{
	ARG_UNUSED(arg);

	return CBPRINTF_PACKAGE_ARG_TYPE_UNSIGNED_LONG;
}

static inline size_t z_cbprintf_cxx_arg_type(long long arg)
{
	ARG_UNUSED(arg);

	return CBPRINTF_PACKAGE_ARG_TYPE_LONG_LONG;
}

static inline size_t z_cbprintf_cxx_arg_type(unsigned long long arg)
{
	ARG_UNUSED(arg);

	return CBPRINTF_PACKAGE_ARG_TYPE_UNSIGNED_LONG_LONG;
}

static inline size_t z_cbprintf_cxx_arg_type(float arg)
{
	ARG_UNUSED(arg);

	return CBPRINTF_PACKAGE_ARG_TYPE_FLOAT;
}

static inline size_t z_cbprintf_cxx_arg_type(double arg)
{
	ARG_UNUSED(arg);

	return CBPRINTF_PACKAGE_ARG_TYPE_DOUBLE;
}

static inline size_t z_cbprintf_cxx_arg_type(long double arg)
{
	ARG_UNUSED(arg);

	return CBPRINTF_PACKAGE_ARG_TYPE_LONG_DOUBLE;
}

static inline size_t z_cbprintf_cxx_arg_type(const char *arg)
{
	ARG_UNUSED(arg);

	return CBPRINTF_PACKAGE_ARG_TYPE_PTR_CONST_CHAR;
}

static inline size_t z_cbprintf_cxx_arg_type(char *arg)
{
	ARG_UNUSED(arg);

	return CBPRINTF_PACKAGE_ARG_TYPE_PTR_CHAR;
}

static inline size_t z_cbprintf_cxx_arg_type(void *arg)
{
	ARG_UNUSED(arg);

	return CBPRINTF_PACKAGE_ARG_TYPE_PTR_VOID;
}

template < typename T >
static inline size_t z_cbprintf_cxx_arg_type(T arg)
{
	ARG_UNUSED(arg);

	return CBPRINTF_PACKAGE_ARG_TYPE_PTR_VOID;
}

#endif /* __cplusplus */
#endif /* ZEPHYR_INCLUDE_SYS_CBPRINTF_CXX_H_ */
