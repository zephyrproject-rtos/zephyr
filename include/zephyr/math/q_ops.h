/*
 * Copyright (c) 2022 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INCLUDE_ZEPHYR_MATH_Q_OPS_H_
#define INCLUDE_ZEPHYR_MATH_Q_OPS_H_

#include <zephyr/math/q_types.h>
#include <zephyr/sys/__assert.h>

#ifndef INCLUDE_MATH_FP_H_
#error "Please include <zephyr/math/fp.h> instead"
#endif

#define DECLARE_INTERNAL_GET_VALUE(ARG_TYPE)                                                       \
	static inline int64_t z_q_get_value_##ARG_TYPE(ARG_TYPE x)                                 \
	{                                                                                          \
		return (int64_t)x << 32;                                                           \
	}

DECLARE_INTERNAL_GET_VALUE(uint8_t)
DECLARE_INTERNAL_GET_VALUE(uint16_t)
DECLARE_INTERNAL_GET_VALUE(uint32_t)
DECLARE_INTERNAL_GET_VALUE(int8_t)
DECLARE_INTERNAL_GET_VALUE(int16_t)
DECLARE_INTERNAL_GET_VALUE(int32_t)

#undef DECLARE_INTERNAL_GET_VALUE

static inline int64_t z_q_get_value_float(float x)
{
	return (int64_t)(x * (float)(INT64_C(1) << 32));
}

static inline int64_t z_q_get_value_double(double x)
{
	return (int64_t)(x * (double)(INT64_C(1) << 32));
}

static inline int64_t z_q_get_value_q1_23_8_t(q1_23_8_t x)
{
	return (int64_t)x.value << 24;
}

static inline int64_t z_q_get_value_q1_15_16_t(q1_15_16_t x)
{
	return (int64_t)x.value << 16;
}

static inline int64_t z_q_get_value_q1_0_31_t(q1_0_31_t x)
{
	return (int64_t)x.value << 1;
}

/* clang-format off */
#define z_q_val_to_intermediate(x)                                                                 \
	(_Generic((x),                                                                             \
		  uint8_t : z_q_get_value_uint8_t,                                                 \
		  uint16_t : z_q_get_value_uint16_t,                                               \
		  uint32_t : z_q_get_value_uint32_t,                                               \
		  int8_t : z_q_get_value_int8_t,                                                   \
		  int16_t : z_q_get_value_int16_t,                                                 \
		  int32_t : z_q_get_value_int32_t,                                                 \
		  float : z_q_get_value_float,                                                     \
		  double : z_q_get_value_double,                                                   \
		  q1_23_8_t : z_q_get_value_q1_23_8_t,                                             \
		  q1_15_16_t : z_q_get_value_q1_15_16_t,                                           \
		  q1_0_31_t : z_q_get_value_q1_0_31_t)(x))

#define z_q1_23_8_t_intermediate_to_val_shift 24
#define z_q1_15_16_t_intermediate_to_val_shift 16
#define z_q1_0_31_t_intermediate_to_val_shift 1

#define q_static(type, x)                                                                          \
	((type) {                                                                                  \
		.value = z_q_val_to_intermediate(x) >> z_##type##_intermediate_to_val_shift,       \
	})

#define z_q_right_shift_bits(qval)                                                                 \
	_Generic((qval),                                                                           \
		 q1_23_8_t : 24,                                                                   \
		 q1_15_16_t : 16,                                                                  \
		 q1_0_31_t : 1)

#define q_assign(qval, x)                                                                          \
	do {                                                                                       \
		int64_t intermediate =  z_q_val_to_intermediate(x);                                \
		intermediate = intermediate >> z_q_right_shift_bits(qval);                         \
        	__ASSERT_NO_MSG(intermediate <= INT32_MAX && intermediate >= INT32_MIN);           \
		qval.value = intermediate & 0xffffffff;                                            \
	} while (false)

#define q_add(qval, x)                                                                             \
	do {                                                                                       \
                int64_t lhs = (int64_t)qval.value;                                                 \
                int64_t rhs = z_q_val_to_intermediate(x) >> z_q_right_shift_bits(qval);            \
		int64_t intermediate = lhs + rhs;                                                  \
        	__ASSERT_NO_MSG(intermediate <= INT32_MAX && intermediate >= INT32_MIN);           \
		qval.value = intermediate & 0xffffffff;                                            \
	} while (false)

#define q_sub(qval, x)                                                                             \
	do {                                                                                       \
                int64_t lhs = (int64_t)qval.value;                                                 \
                int64_t rhs = z_q_val_to_intermediate(x) >> z_q_right_shift_bits(qval);            \
		int64_t intermediate = lhs - rhs;                                                  \
        	__ASSERT_NO_MSG(intermediate <= INT32_MAX && intermediate >= INT32_MIN);           \
		qval.value = intermediate & 0xffffffff;                                            \
	} while (false)

#define q_mul(qval, x)                                                                             \
	do {                                                                                       \
                int64_t lhs = (int64_t)qval.value;                                                 \
		int64_t rhs = z_q_val_to_intermediate(x) >> z_q_right_shift_bits(qval);            \
		int64_t intermediate =  (lhs * rhs) >> (32 - z_q_right_shift_bits(qval));          \
        	__ASSERT_NO_MSG(intermediate <= INT32_MAX && intermediate >= INT32_MIN);           \
		qval.value = intermediate & 0xffffffff;                                            \
	} while (false)

#define q_div(qval, x)                                                                             \
	do {                                                                                       \
		int64_t lhs = (int64_t)qval.value << (32 - z_q_right_shift_bits(qval));            \
		int64_t rhs = z_q_val_to_intermediate(x) >> z_q_right_shift_bits(qval);            \
		int64_t intermediate = lhs / rhs;                                                  \
        	__ASSERT_NO_MSG(intermediate <= INT32_MAX && intermediate >= INT32_MIN);           \
		qval.value = intermediate & 0xffffffff;                                            \
	} while (false)

/* clang-format on */

#endif /* INCLUDE_ZEPHYR_MATH_Q_OPS_H_ */
