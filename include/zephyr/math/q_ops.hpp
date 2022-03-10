/*
 * Copyright (c) 2022 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <zephyr/math/q_types.h>
#include <zephyr/sys/__assert.h>

#ifndef INCLUDE_MATH_FP_H_
#error "Please include <zephyr/math/fp.h> instead"
#endif

template <class T> constexpr int64_t z_q_val_to_intermediate(const T& x)
{
	return static_cast<int64_t>(x) << 32;
}

template <> constexpr int64_t z_q_val_to_intermediate<float>(const float& x)
{
	return static_cast<int64_t>(x * (INT64_C(1) << 32));
}

template <> constexpr int64_t z_q_val_to_intermediate<double>(const double& x)
{
	return static_cast<int64_t>(x * (INT64_C(1) << 32));
}

template <> constexpr int64_t z_q_val_to_intermediate<q1_0_31_t>(const q1_0_31_t& x)
{
	return static_cast<int64_t>(x.value) << 1;
}

template <> constexpr int64_t z_q_val_to_intermediate<q1_15_16_t>(const q1_15_16_t& x)
{
	return static_cast<int64_t>(x.value) << 16;
}

template <> constexpr int64_t z_q_val_to_intermediate<q1_23_8_t>(const q1_23_8_t& x)
{
	return static_cast<int64_t>(x.value) << 24;
}

template <class T> constexpr int z_intermediate_to_val_shift();

template <> constexpr int z_intermediate_to_val_shift<q1_0_31_t>()
{
	return 1;
}

template <> constexpr int z_intermediate_to_val_shift<q1_15_16_t>()
{
	return 16;
}
template <> constexpr int z_intermediate_to_val_shift<q1_23_8_t>()
{
	return 24;
}

template <class T, class X> static constexpr T z_q_static(X x)
{
	return {
		.value = static_cast<int32_t>(z_q_val_to_intermediate<X>(x) >>
					      z_intermediate_to_val_shift<T>()),
	};
}

#define q_static(type, x) z_q_static<type>(x)

template <class Q, class X> inline void q_assign(Q &qval, const X &x)
{
	int64_t intermediate = z_q_val_to_intermediate(x) >> z_intermediate_to_val_shift<Q>();
	__ASSERT_NO_MSG(intermediate <= INT32_MAX && intermediate >= INT32_MIN);
	qval.value = intermediate & 0xffffffff;
}

template <class Q, class X> inline void q_add(Q &qval, const X &x)
{
	int64_t lhs = static_cast<int64_t>(qval.value);
	int64_t rhs = z_q_val_to_intermediate(x) >> z_intermediate_to_val_shift<Q>();
	int64_t intermediate = lhs + rhs;
	__ASSERT_NO_MSG(intermediate <= INT32_MAX && intermediate >= INT32_MIN);
	qval.value = intermediate & 0xffffffff;
}

template <class Q, class X> inline void q_sub(Q &qval, const X &x)
{
	int64_t lhs = static_cast<int64_t>(qval.value);
	int64_t rhs = z_q_val_to_intermediate(x) >> z_intermediate_to_val_shift<Q>();
	int64_t intermediate = lhs - rhs;
	__ASSERT_NO_MSG(intermediate <= INT32_MAX && intermediate >= INT32_MIN);
	qval.value = intermediate & 0xffffffff;
}

template <class Q, class X> inline void q_mul(Q &qval, const X &x)
{
	int64_t lhs = static_cast<int64_t>(qval.value);
	int64_t rhs = z_q_val_to_intermediate(x) >> z_intermediate_to_val_shift<Q>();
	int64_t intermediate = (lhs * rhs) >> (32 - z_intermediate_to_val_shift<Q>());
	__ASSERT_NO_MSG(intermediate <= INT32_MAX && intermediate >= INT32_MIN);
	qval.value = intermediate & 0xffffffff;
}

template <class Q, class X> inline void q_div(Q &qval, const X &x)
{
	int64_t lhs = static_cast<int64_t>(qval.value) << (32 - z_intermediate_to_val_shift<Q>());
	int64_t rhs = z_q_val_to_intermediate(x) >> z_intermediate_to_val_shift<Q>();
	int64_t intermediate = lhs / rhs;
	__ASSERT_NO_MSG(intermediate <= INT32_MAX && intermediate >= INT32_MIN);
	qval.value = intermediate & 0xffffffff;
}
