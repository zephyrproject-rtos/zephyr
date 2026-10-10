/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INFINEON_MXCORDIC_H_
#define ZEPHYR_INCLUDE_DRIVERS_INFINEON_MXCORDIC_H_

#include <zephyr/device.h>

/**
 * @file
 * @brief Blocking MXCORDIC APIs for thread context with interrupts enabled.
 *
 * Hardware completion waits time out after 100 ms and return -ETIMEDOUT.
 * This timeout does not include waiting to acquire the device mutex.
 * A timeout disables the accelerator until reboot; further hardware requests
 * return -EIO. Hardware error interrupts also return -EIO.
 * Software-only special cases remain available when the accelerator is faulted.
 * Output arguments are unchanged on error.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Compute the sine of an angle.
 *
 * @param dev    MXCORDIC device instance.
 * @param angle  Angle in radians. Valid range: [-1.74, 1.74].
 * @param result Pointer that receives sin(angle).
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p result is NULL or @p angle is out of range.
 * @retval -ETIMEDOUT if the hardware completion wait times out.
 * @retval -EIO if the hardware reports an error or is faulted after an earlier timeout.
 */
int cordic_ifx_mxcordic_sin(const struct device *dev, float angle, float *result);

/**
 * @brief Compute the cosine of an angle.
 *
 * @param dev    MXCORDIC device instance.
 * @param angle  Angle in radians. Valid range: [-1.74, 1.74].
 * @param result Pointer that receives cos(angle).
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p result is NULL or @p angle is out of range.
 * @retval -ETIMEDOUT if the hardware completion wait times out.
 * @retval -EIO if the hardware reports an error or is faulted after an earlier timeout.
 */
int cordic_ifx_mxcordic_cos(const struct device *dev, float angle, float *result);

/**
 * @brief Compute the tangent of an angle.
 *
 * @param dev    MXCORDIC device instance.
 * @param angle  Angle in radians. Valid range: [-1.74, 1.74].
 * @param result Pointer that receives tan(angle).
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p result is NULL or @p angle is out of range.
 * @retval -ETIMEDOUT if the hardware completion wait times out.
 * @retval -EIO if the hardware reports an error or is faulted after an earlier timeout.
 */
int cordic_ifx_mxcordic_tan(const struct device *dev, float angle, float *result);

/**
 * @brief Compute the four-quadrant arc tangent of @p y / @p x.
 *
 * Finite inputs are normalized before conversion to fixed point.
 * If both inputs are zero, returns success with a zero result.
 *
 * @param dev    MXCORDIC device instance.
 * @param y      Numerator (ordinate). Must be finite.
 * @param x      Denominator (abscissa). Must be finite.
 * @param result Pointer that receives atan2(y, x) in radians, range [-pi, pi].
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p result is NULL or an input is not finite.
 * @retval -ETIMEDOUT if the hardware completion wait times out.
 * @retval -EIO if the hardware reports an error or is faulted after an earlier timeout.
 */
int cordic_ifx_mxcordic_atan2(const struct device *dev, float y, float x, float *result);

/**
 * @brief Compute the inverse sine of a value.
 *
 * @param dev    MXCORDIC device instance.
 * @param x      Input value. Valid range: [-1, 1].
 * @param result Pointer that receives asin(x) in radians.
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p result is NULL or @p x is out of range.
 * @retval -ETIMEDOUT if the hardware completion wait times out.
 * @retval -EIO if the hardware reports an error or is faulted after an earlier timeout.
 */
int cordic_ifx_mxcordic_asin(const struct device *dev, float x, float *result);

/**
 * @brief Compute the inverse cosine of a value.
 *
 * @param dev    MXCORDIC device instance.
 * @param x      Input value. Valid range: [-1, 1].
 * @param result Pointer that receives acos(x) in radians.
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p result is NULL or @p x is out of range.
 * @retval -ETIMEDOUT if the hardware completion wait times out.
 * @retval -EIO if the hardware reports an error or is faulted after an earlier timeout.
 */
int cordic_ifx_mxcordic_acos(const struct device *dev, float x, float *result);

/**
 * @brief Compute the hyperbolic sine of an angle.
 *
 * @param dev    MXCORDIC device instance.
 * @param angle  Angle in radians. Valid range: (-1.11, 1.11).
 * @param result Pointer that receives sinh(angle).
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p result is NULL or @p angle is out of range.
 * @retval -ETIMEDOUT if the hardware completion wait times out.
 * @retval -EIO if the hardware reports an error or is faulted after an earlier timeout.
 */
int cordic_ifx_mxcordic_sinh(const struct device *dev, float angle, float *result);

/**
 * @brief Compute the hyperbolic cosine of an angle.
 *
 * @param dev    MXCORDIC device instance.
 * @param angle  Angle in radians. Valid range: (-1.11, 1.11).
 * @param result Pointer that receives cosh(angle).
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p result is NULL or @p angle is out of range.
 * @retval -ETIMEDOUT if the hardware completion wait times out.
 * @retval -EIO if the hardware reports an error or is faulted after an earlier timeout.
 */
int cordic_ifx_mxcordic_cosh(const struct device *dev, float angle, float *result);

/**
 * @brief Compute the hyperbolic tangent of an angle.
 *
 * @param dev    MXCORDIC device instance.
 * @param angle  Angle in radians. Valid range: (-1.11, 1.11).
 * @param result Pointer that receives tanh(angle).
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p result is NULL or @p angle is out of range.
 * @retval -ETIMEDOUT if the hardware completion wait times out.
 * @retval -EIO if the hardware reports an error or is faulted after an earlier timeout.
 */
int cordic_ifx_mxcordic_tanh(const struct device *dev, float angle, float *result);

/**
 * @brief Compute the inverse hyperbolic tangent.
 *
 * @param dev    MXCORDIC device instance.
 * @param x      Input value. Valid range: (-0.8, 0.8).
 * @param result Pointer that receives atanh(x) in radians.
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p result is NULL or @p x is out of range.
 * @retval -ETIMEDOUT if the hardware completion wait times out.
 * @retval -EIO if the hardware reports an error or is faulted after an earlier timeout.
 */
int cordic_ifx_mxcordic_atanh(const struct device *dev, float x, float *result);

/**
 * @brief Compute the square root of a value.
 *
 * @param dev    MXCORDIC device instance.
 * @param x      Input value. Valid range: (0, 1].
 * @param result Pointer that receives sqrt(x).
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p result is NULL or @p x is out of range.
 * @retval -ETIMEDOUT if the hardware completion wait times out.
 * @retval -EIO if the hardware reports an error or is faulted after an earlier timeout.
 */
int cordic_ifx_mxcordic_sqrt(const struct device *dev, float x, float *result);

/**
 * @brief Compute the magnitude (hypotenuse) of a 2D vector.
 *
 * Computes sqrt(x^2 + y^2) using the CORDIC circular vectoring mode.
 *
 * @param dev    MXCORDIC device instance.
 * @param x      X component. Must be finite.
 * @param y      Y component. Must be finite.
 * @param result Pointer that receives sqrt(x^2 + y^2).
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p result is NULL or an input is not finite.
 * @retval -ETIMEDOUT if the hardware completion wait times out.
 * @retval -EIO if the hardware reports an error or is faulted after an earlier timeout.
 */
int cordic_ifx_mxcordic_magnitude(const struct device *dev, float x, float y, float *result);

/**
 * @brief Compute the gain-scaled Park transform of an alpha-beta vector.
 *
 * @param dev    MXCORDIC device instance.
 * @param angle  Finite rotor angle in radians. Valid range: [-1.74, 1.74].
 * @param alpha  Alpha-axis input. Valid range: [-1, 1).
 * @param beta   Beta-axis input. Valid range: [-1, 1).
 * @param d      Pointer that receives the d-axis result.
 * @param q      Pointer that receives the q-axis result.
 *
 * @retval 0 on success.
 * @retval -EINVAL if a result pointer is NULL or an input is not finite or out of range.
 * @retval -ETIMEDOUT if the hardware completion wait times out.
 * @retval -EIO if the hardware reports an error or is faulted after an earlier timeout.
 */
int cordic_ifx_mxcordic_park_transform(const struct device *dev, float angle, float alpha,
				       float beta, float *d, float *q);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_INFINEON_MXCORDIC_H_ */
