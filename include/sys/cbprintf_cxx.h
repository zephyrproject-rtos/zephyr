/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_CBPRINTF_CXX_H_
#define ZEPHYR_INCLUDE_SYS_CBPRINTF_CXX_H_
#ifdef __cplusplus

/* C++ version for detecting a pointer to a string. */
static inline int z_cbprintf_cxx_is_pchar(char *)
{
	return 1;
}

static inline int z_cbprintf_cxx_is_pchar(const char *)
{
	return 1;
}

static inline int z_cbprintf_cxx_is_pchar(volatile char *)
{
	return 1;
}

static inline int z_cbprintf_cxx_is_pchar(const volatile char *)
{
	return 1;
}

static inline int z_cbprintf_cxx_is_pchar(wchar_t *)
{
	return 1;
}

static inline int z_cbprintf_cxx_is_pchar(const wchar_t *)
{
	return 1;
}

static inline int z_cbprintf_cxx_is_pchar(volatile wchar_t *)
{
	return 1;
}

static inline int z_cbprintf_cxx_is_pchar(const volatile wchar_t *)
{
	return 1;
}

template < typename T >
static inline int z_cbprintf_cxx_is_pchar(T arg)
{
	_Pragma("GCC diagnostic push")
	_Pragma("GCC diagnostic ignored \"-Wpointer-arith\"")
	return 0;
	_Pragma("GCC diagnostic pop")
}

/* C++ version for calculating argument size. */
static inline size_t z_cbprintf_cxx_arg_size(float f)
{
	return sizeof(double);
}

static inline size_t z_cbprintf_cxx_arg_size(void *p)
{
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

	z_cbprintf_wcpy((int *)dst, (int *)&d, sizeof(d) / sizeof(int));
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

	z_cbprintf_wcpy((int *)dst, (int *)&arg, wlen);
}

/* C++ version for long double detection. */
static inline int z_cbprintf_cxx_is_longdouble(long double arg)
{
	return 1;
}

template < typename T >
static inline int z_cbprintf_cxx_is_longdouble(T arg)
{
	return 0;
}

/* C++ version for caluculating argument alignment. */
static inline size_t z_cbprintf_cxx_alignment(float arg)
{
	return VA_STACK_ALIGN(double);
}

static inline size_t z_cbprintf_cxx_alignment(double arg)
{
	return VA_STACK_ALIGN(double);
}

static inline size_t z_cbprintf_cxx_alignment(long double arg)
{
	return VA_STACK_ALIGN(long double);
}

static inline size_t z_cbprintf_cxx_alignment(long long arg)
{
	return VA_STACK_ALIGN(long long);
}

static inline size_t z_cbprintf_cxx_alignment(unsigned long long arg)
{
	return VA_STACK_ALIGN(long long);
}

template < typename T >
static inline size_t z_cbprintf_cxx_alignment(T arg)
{
	return MAX(__alignof__(arg), VA_STACK_MIN_ALIGN);
}

#endif /* __cplusplus */
#endif /* ZEPHYR_INCLUDE_SYS_CBPRINTF_CXX_H_ */
