/*
 * Copyright (c) 2026 Sylvain Mosnier
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Unit quaternion type and inline rotation helpers (Hamilton convention).
 *
 * All rotation functions assume |q| = 1. Convention: q = w + x*i + y*j + z*k.
 */

#ifndef ZEPHYR_INCLUDE_MATH_QUAT_H_
#define ZEPHYR_INCLUDE_MATH_QUAT_H_

#include <math.h>
#include <zephyr/math/vec3.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup math_quat Quaternion math
 * @ingroup math
 * @{
 */

/**
 * @brief Unit quaternion (Hamilton convention: q = w + x*i + y*j + z*k).
 *
 * All rotation functions assume |q| = 1.
 */
typedef struct {
	float w; /**< Scalar part. */
	float x; /**< i component. */
	float y; /**< j component. */
	float z; /**< k component. */
} quat_t;

/** @brief Identity quaternion (no rotation). */
#define QUAT_IDENTITY ((quat_t){.w = 1.0f, .x = 0.0f, .y = 0.0f, .z = 0.0f})

/**
 * @brief Hamilton product a x b.
 *
 * Composes rotation @p b applied first, then @p a.
 *
 * @param[in] a Left operand.
 * @param[in] b Right operand.
 *
 * @return a x b
 */
static inline quat_t quat_mul(quat_t a, quat_t b)
{
	return (quat_t){
		.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
		.x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
		.y = a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
		.z = a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
	};
}

/**
 * @brief Scalar multiplication.
 *
 * @param[in] q Quaternion to scale.
 * @param[in] s Scalar factor.
 *
 * @return q * s
 */
static inline quat_t quat_scale(quat_t q, float s)
{
	return (quat_t){.w = q.w * s, .x = q.x * s, .y = q.y * s, .z = q.z * s};
}

/**
 * @brief Conjugate of a quaternion.
 *
 * For unit quaternions, the conjugate equals the inverse.
 *
 * @param[in] q Input quaternion.
 *
 * @return q*
 */
static inline quat_t quat_conj(quat_t q)
{
	return (quat_t){.w = q.w, .x = -q.x, .y = -q.y, .z = -q.z};
}

/**
 * @brief Rotate a vector by a unit quaternion.
 *
 * Rodrigues formula expanded:
 * @code
 *   tx = 2*(q.y*v.z - q.z*v.y)
 *   ty = 2*(q.z*v.x - q.x*v.z)
 *   tz = 2*(q.x*v.y - q.y*v.x)
 *   v' = v + q.w*[tx,ty,tz] + q.xyz × [tx,ty,tz]
 * @endcode
 *
 * @param[in] q Unit quaternion representing the rotation.
 * @param[in] v Vector to rotate.
 *
 * @return Rotated vector.
 */
static inline vec3_t quat_rotate(quat_t q, vec3_t v)
{
	const float tx = 2.0f * (q.y * v.z - q.z * v.y);
	const float ty = 2.0f * (q.z * v.x - q.x * v.z);
	const float tz = 2.0f * (q.x * v.y - q.y * v.x);

	return (vec3_t){
		.x = v.x + q.w * tx + q.y * tz - q.z * ty,
		.y = v.y + q.w * ty + q.z * tx - q.x * tz,
		.z = v.z + q.w * tz + q.x * ty - q.y * tx,
	};
}

/**
 * @brief Squared norm of a quaternion.
 *
 * Should equal 1 for a unit quaternion. Prefer this over computing the norm
 * when only drift detection is needed, to avoid a square root.
 *
 * @param[in] q Input quaternion.
 *
 * @return ||q||^2
 */
static inline float quat_norm_sq(quat_t q)
{
	return q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z;
}

/**
 * @brief Re-normalise a quaternion that has drifted from unit length.
 *
 * Returns @ref QUAT_IDENTITY if the norm is zero.
 *
 * @param[in] q Input quaternion.
 *
 * @return q / ||q||, or identity if ||q|| == 0.
 */
static inline quat_t quat_normalize(quat_t q)
{
	const float n = quat_norm_sq(q);

	return (n > 0.0f) ? quat_scale(q, 1.0f / sqrtf(n)) : QUAT_IDENTITY;
}

/**
 * @brief Build a unit quaternion from an axis-angle rotation.
 *
 * @p axis must be a unit vector. The resulting quaternion represents a
 * rotation of @p angle radians around @p axis (right-hand rule).
 *
 * @param[in] axis  Unit rotation axis.
 * @param[in] angle Rotation angle in radians.
 *
 * @return Unit quaternion representing the rotation.
 */
static inline quat_t quat_from_axis_angle(vec3_t axis, float angle)
{
	const float half = angle * 0.5f;
	const float s = sinf(half);

	return (quat_t){
		.w = cosf(half),
		.x = axis.x * s,
		.y = axis.y * s,
		.z = axis.z * s,
	};
}

/**
 * @brief Build a unit quaternion from ZYX intrinsic Euler angles.
 *
 * Equivalent to q_z(yaw) x q_y(pitch) x q_x(roll).
 *
 * @param[in] roll   Rotation around X axis, in radians.
 * @param[in] pitch  Rotation around Y axis, in radians.
 * @param[in] yaw    Rotation around Z axis, in radians.
 *
 * @return Unit quaternion representing the composed rotation.
 */
static inline quat_t quat_from_euler_zyx(float roll, float pitch, float yaw)
{
	const float cr = cosf(roll * 0.5f), sr = sinf(roll * 0.5f);
	const float cp = cosf(pitch * 0.5f), sp = sinf(pitch * 0.5f);
	const float cy = cosf(yaw * 0.5f), sy = sinf(yaw * 0.5f);

	return (quat_t){
		.w = cr * cp * cy + sr * sp * sy,
		.x = sr * cp * cy - cr * sp * sy,
		.y = cr * sp * cy + sr * cp * sy,
		.z = cr * cp * sy - sr * sp * cy,
	};
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_MATH_QUAT_H_ */
