/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_CBPRINTF_CXX_H_
#define ZEPHYR_INCLUDE_SYS_CBPRINTF_CXX_H_
#ifdef __cplusplus

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

static inline int z_cbprintf_cxx_is_pchar(unsigned char *, bool const_as_fixed)
{
	ARG_UNUSED(const_as_fixed);
	return 1;
}

static inline int z_cbprintf_cxx_is_pchar(const unsigned char *, bool const_as_fixed)
{
	return const_as_fixed ? 0 : 1;
}

static inline int z_cbprintf_cxx_is_pchar(volatile unsigned char *, bool const_as_fixed)
{
	ARG_UNUSED(const_as_fixed);
	return 1;
}

static inline int z_cbprintf_cxx_is_pchar(const volatile unsigned char *, bool const_as_fixed)
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

/* C++ version for determining if variable type is numeric and fits in 32 bit word. */
static inline int z_cbprintf_cxx_is_word_num(char)
{
	return 1;
}

static inline int z_cbprintf_cxx_is_word_num(unsigned char)
{
	return 1;
}

static inline int z_cbprintf_cxx_is_word_num(short)
{
	return 1;
}

static inline int z_cbprintf_cxx_is_word_num(unsigned short)
{
	return 1;
}

static inline int z_cbprintf_cxx_is_word_num(int)
{
	return 1;
}

static inline int z_cbprintf_cxx_is_word_num(unsigned int)
{
	return 1;
}

static inline int z_cbprintf_cxx_is_word_num(long)
{
	return (sizeof(long) <= sizeof(uint32_t)) ? 1 : 0;
}

static inline int z_cbprintf_cxx_is_word_num(unsigned long)
{
	return (sizeof(long) <= sizeof(uint32_t)) ? 1 : 0;
}

template < typename T >
static inline int z_cbprintf_cxx_is_word_num(T arg)
{
	ARG_UNUSED(arg);
	_Pragma("GCC diagnostic push")
	_Pragma("GCC diagnostic ignored \"-Wpointer-arith\"")
	return 0;
	_Pragma("GCC diagnostic pop")
}

/* C++ version for determining if argument is a none character pointer. */
static inline int z_cbprintf_cxx_is_none_char_ptr(char)
{
	return 0;
}

static inline int z_cbprintf_cxx_is_none_char_ptr(unsigned char)
{
	return 0;
}

static inline int z_cbprintf_cxx_is_none_char_ptr(short)
{
	return 0;
}

static inline int z_cbprintf_cxx_is_none_char_ptr(unsigned short)
{
	return 0;
}

static inline int z_cbprintf_cxx_is_none_char_ptr(int)
{
	return 0;
}

static inline int z_cbprintf_cxx_is_none_char_ptr(unsigned int)
{
	return 0;
}

static inline int z_cbprintf_cxx_is_none_char_ptr(long)
{
	return 0;
}

static inline int z_cbprintf_cxx_is_none_char_ptr(unsigned long)
{
	return 0;
}

static inline int z_cbprintf_cxx_is_none_char_ptr(long long)
{
	return 0;
}

static inline int z_cbprintf_cxx_is_none_char_ptr(unsigned long long)
{
	return 0;
}

static inline int z_cbprintf_cxx_is_none_char_ptr(float)
{
	return 0;
}

static inline int z_cbprintf_cxx_is_none_char_ptr(double)
{
	return 0;
}

static inline int z_cbprintf_cxx_is_none_char_ptr(char *)
{
	return 0;
}

static inline int z_cbprintf_cxx_is_none_char_ptr(volatile char *)
{
	return 0;
}

static inline int z_cbprintf_cxx_is_none_char_ptr(const char *)
{
	return 0;
}

static inline int z_cbprintf_cxx_is_none_char_ptr(const volatile char *)
{
	return 0;
}

static inline int z_cbprintf_cxx_is_none_char_ptr(unsigned char *)
{
	return 0;
}

static inline int z_cbprintf_cxx_is_none_char_ptr(volatile unsigned char *)
{
	return 0;
}

static inline int z_cbprintf_cxx_is_none_char_ptr(const unsigned char *)
{
	return 0;
}

static inline int z_cbprintf_cxx_is_none_char_ptr(const volatile unsigned char *)
{
	return 0;
}

template < typename T >
static inline int z_cbprintf_cxx_is_none_char_ptr(T arg)
{
	ARG_UNUSED(arg);

	return 1;
}

/* C++ version for calculating argument size. */
static inline size_t z_cbprintf_cxx_arg_size(float f)
{
	ARG_UNUSED(f);

	return sizeof(double);
}

template < typename T >
static inline size_t z_cbprintf_cxx_arg_size(T arg)
{
	ARG_UNUSED(arg);

	return MAX(sizeof(T), sizeof(int));
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

/* C++ version for checking if two arguments are same type */
template < typename T1, typename T2 >
struct z_cbprintf_cxx_is_same_type {
	enum {
		value = false
	};
};

template < typename T >
struct z_cbprintf_cxx_is_same_type < T, T > {
	enum {
		value = true
	};
};

template < typename T >
struct z_cbprintf_cxx_remove_reference {
	typedef T type;
};

template < typename T >
struct z_cbprintf_cxx_remove_reference < T & > {
	typedef T type;
};

#if __cplusplus >= 201103L
template < typename T >
struct z_cbprintf_cxx_remove_reference < T && > {
	typedef T type;
};
#endif

template < typename T >
struct z_cbprintf_cxx_remove_cv {
	typedef T type;
};

template < typename T >
struct z_cbprintf_cxx_remove_cv < const T > {
	typedef T type;
};

template < typename T >
struct z_cbprintf_cxx_remove_cv < volatile T > {
	typedef T type;
};

template < typename T >
struct z_cbprintf_cxx_remove_cv < const volatile T > {
	typedef T type;
};

/* Determine if a type is an array */
template < typename T >
struct z_cbprintf_cxx_is_array {
	enum {
		value = false
	};
};

template < typename T >
struct z_cbprintf_cxx_is_array < T[] > {
	enum {
		value = true
	};
};

template < typename T, size_t N >
struct z_cbprintf_cxx_is_array < T[N] > {
	enum {
		value = true
	};
};

/* Determine the type of elements in an array */
template < typename T >
struct z_cbprintf_cxx_remove_extent {
	typedef T type;
};

template < typename T >
struct z_cbprintf_cxx_remove_extent < T[] > {
	typedef T type;
};

template < typename T, size_t N >
struct z_cbprintf_cxx_remove_extent < T[N] > {
	typedef T type;
};

#endif /* __cplusplus */
#endif /* ZEPHYR_INCLUDE_SYS_CBPRINTF_CXX_H_ */
