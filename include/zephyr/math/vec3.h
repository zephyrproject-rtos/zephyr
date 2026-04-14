/*
 * Copyright (c) 2026 Sylvain Mosnier
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Floating-point 3-D vector type and inline arithmetic helpers.
 */

#ifndef ZEPHYR_INCLUDE_MATH_VEC3_H_
#define ZEPHYR_INCLUDE_MATH_VEC3_H_

#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup math_vec3 3-D vector math
 * @ingroup math
 * @{
 */

/**
 * @brief 3-D floating-point vector.
 */
typedef struct {
	float x; /**< X component. */
	float y; /**< Y component. */
	float z; /**< Z component. */
} vec3_t;

/** @brief Zero vector (0, 0, 0). */
#define VEC3_ZERO ((vec3_t){.x = 0.0f, .y = 0.0f, .z = 0.0f})

/**
 * @brief Construct a vec3_t from three scalar components.
 *
 * @param x_ X component.
 * @param y_ Y component.
 * @param z_ Z component.
 */
#define VEC3(x_, y_, z_) ((vec3_t){.x = (float)(x_), .y = (float)(y_), .z = (float)(z_)})

/**
 * @brief Component-wise addition.
 *
 * @param[in] a First operand.
 * @param[in] b Second operand.
 *
 * @return a + b
 */
static inline vec3_t vec3_add(vec3_t a, vec3_t b)
{
	return (vec3_t){.x = a.x + b.x, .y = a.y + b.y, .z = a.z + b.z};
}

/**
 * @brief Component-wise subtraction.
 *
 * @param[in] a First operand.
 * @param[in] b Second operand.
 *
 * @return a - b
 */
static inline vec3_t vec3_sub(vec3_t a, vec3_t b)
{
	return (vec3_t){.x = a.x - b.x, .y = a.y - b.y, .z = a.z - b.z};
}

/**
 * @brief Scalar multiplication.
 *
 * @param[in] v Vector to scale.
 * @param[in] s Scalar factor.
 *
 * @return v * s
 */
static inline vec3_t vec3_scale(vec3_t v, float s)
{
	return (vec3_t){.x = v.x * s, .y = v.y * s, .z = v.z * s};
}

/**
 * @brief Negate a vector.
 *
 * @param[in] v Vector to negate.
 *
 * @return -v
 */
static inline vec3_t vec3_neg(vec3_t v)
{
	return (vec3_t){.x = -v.x, .y = -v.y, .z = -v.z};
}

/**
 * @brief Dot product.
 *
 * @param[in] a First operand.
 * @param[in] b Second operand.
 *
 * @return a · b
 */
static inline float vec3_dot(vec3_t a, vec3_t b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

/**
 * @brief Cross product.
 *
 * @param[in] a First operand.
 * @param[in] b Second operand.
 *
 * @return a × b
 */
static inline vec3_t vec3_cross(vec3_t a, vec3_t b)
{
	return (vec3_t){
		.x = a.y * b.z - a.z * b.y,
		.y = a.z * b.x - a.x * b.z,
		.z = a.x * b.y - a.y * b.x,
	};
}

/**
 * @brief Squared Euclidean norm.
 *
 * Prefer this over vec3_norm() when only relative magnitudes are compared,
 * to avoid a square root.
 *
 * @param[in] v Input vector.
 *
 * @return ||v||^2
 */
static inline float vec3_norm_sq(vec3_t v)
{
	return v.x * v.x + v.y * v.y + v.z * v.z;
}

/**
 * @brief Euclidean norm.
 *
 * @param[in] v Input vector.
 *
 * @return ||v||
 */
static inline float vec3_norm(vec3_t v)
{
	return sqrtf(vec3_norm_sq(v));
}

/**
 * @brief Normalise a vector to unit length.
 *
 * Returns @ref VEC3_ZERO if @p v is the zero vector.
 *
 * @param[in] v Input vector.
 *
 * @return v / ||v||, or zero if ||v|| == 0.
 */
static inline vec3_t vec3_normalize(vec3_t v)
{
	const float n_2 = vec3_norm_sq(v);
	return (n_2 > 0.0f) ? vec3_scale(v, 1.0f / sqrtf(n_2)) : VEC3_ZERO;
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_MATH_VEC3_H_ */
