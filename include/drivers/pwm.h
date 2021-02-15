/*
 * Copyright (c) 2016 Intel Corporation.
 * Copyright (c) 2020-2021 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public PWM Driver APIs
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PWM_H_
#define ZEPHYR_INCLUDE_DRIVERS_PWM_H_

/**
 * @brief PWM Interface
 * @defgroup pwm_interface PWM Interface
 * @ingroup io_interfaces
 * @{
 */

#include <errno.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <sys/math_extras.h>
#include <device.h>
#include <dt-bindings/pwm/pwm.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name PWM capture configuration flags
 * @{
 */

/** @cond INTERNAL_HIDDEN */
/* Bit 0 is used for PWM_POLARITY_NORMAL/PWM_POLARITY_INVERTED */
#define PWM_CAPTURE_TYPE_SHIFT		1U
#define PWM_CAPTURE_TYPE_MASK		(3U << PWM_CAPTURE_TYPE_SHIFT)
#define PWM_CAPTURE_MODE_SHIFT		3U
#define PWM_CAPTURE_MODE_MASK		(1U << PWM_CAPTURE_MODE_SHIFT)
/** @endcond */

/** PWM pin capture captures period. */
#define PWM_CAPTURE_TYPE_PERIOD		(1U << PWM_CAPTURE_TYPE_SHIFT)

/** PWM pin capture captures pulse width. */
#define PWM_CAPTURE_TYPE_PULSE		(2U << PWM_CAPTURE_TYPE_SHIFT)

/** PWM pin capture captures both period and pulse width. */
#define PWM_CAPTURE_TYPE_BOTH		(PWM_CAPTURE_TYPE_PERIOD | \
					 PWM_CAPTURE_TYPE_PULSE)

/** PWM pin capture captures a single period/pulse width. */
#define PWM_CAPTURE_MODE_SINGLE		(0U << PWM_CAPTURE_MODE_SHIFT)

/** PWM pin capture captures period/pulse width continuously. */
#define PWM_CAPTURE_MODE_CONTINUOUS	(1U << PWM_CAPTURE_MODE_SHIFT)

/** @} */

/**
 * @brief Provides a type to hold PWM configuration flags.
 */
typedef uint8_t pwm_flags_t;

/**
 * @typedef pwm_pin_set_t
 * @brief Callback API upon setting the pin
 * See @a pwm_pin_set_cycles() for argument description
 */
typedef int (*pwm_pin_set_t)(const struct device *dev, uint32_t pwm,
			     uint32_t period_cycles, uint32_t pulse_cycles,
			     pwm_flags_t flags);

/**
 * @typedef pwm_capture_callback_handler_t
 * @brief PWM capture callback handler function signature
 *
 * @note The callback handler will be called in interrupt context.
 *
 * @note @option{CONFIG_PWM_CAPTURE} must be selected to enable PWM capture
 * support.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.

 * @param period_cycles Captured PWM period width (in clock cycles). HW
 *                      specific.
 * @param pulse_cycles Captured PWM pulse width (in clock cycles). HW specific.
 * @param status Status for the PWM capture (0 if no error, negative errno
 *               otherwise. See @a pwm_pin_capture_cycles() return value
 *               descriptions for details).
 * @param user_data User data passed to @a pwm_pin_configure_capture()
 */
typedef void (*pwm_capture_callback_handler_t)(const struct device *dev,
					       uint32_t pwm,
					       uint32_t period_cycles,
					       uint32_t pulse_cycles,
					       int status,
					       void *user_data);

/**
 * @typedef pwm_pin_configure_capture_t
 * @brief Callback API upon configuring PWM pin capture
 * See @a pwm_pin_configure_capture() for argument description
 */
typedef int (*pwm_pin_configure_capture_t)(const struct device *dev,
					   uint32_t pwm,
					   pwm_flags_t flags,
					   pwm_capture_callback_handler_t cb,
					   void *user_data);
/**
 * @typedef pwm_pin_enable_capture_t
 * @brief Callback API upon enabling PWM pin capture
 * See @a pwm_pin_enable_capture() for argument description
 */
typedef int (*pwm_pin_enable_capture_t)(const struct device *dev,
					uint32_t pwm);

/**
 * @typedef pwm_pin_disable_capture_t
 * @brief Callback API upon disabling PWM pin capture
 * See @a pwm_pin_disable_capture() for argument description
 */
typedef int (*pwm_pin_disable_capture_t)(const struct device *dev,
					 uint32_t pwm);

/**
 * @typedef pwm_get_cycles_per_sec_t
 * @brief Callback API upon getting cycles per second
 * See @a pwm_get_cycles_per_sec() for argument description
 */
typedef int (*pwm_get_cycles_per_sec_t)(const struct device *dev,
					uint32_t pwm,
					uint64_t *cycles);

/** @brief PWM driver API definition. */
__subsystem struct pwm_driver_api {
	pwm_pin_set_t pin_set;
#ifdef CONFIG_PWM_CAPTURE
	pwm_pin_configure_capture_t pin_configure_capture;
	pwm_pin_enable_capture_t pin_enable_capture;
	pwm_pin_disable_capture_t pin_disable_capture;
#endif /* CONFIG_PWM_CAPTURE */
	pwm_get_cycles_per_sec_t get_cycles_per_sec;
};

/**
 * @brief Set the period and pulse width for a single PWM output.
 *
 * Passing 0 as @p pulse will cause the pin to be driven to a constant
 * inactive level.
 * Passing a non-zero @p pulse equal to @p period will cause the pin
 * to be driven to a constant active level.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.
 * @param period Period (in clock cycle) set to the PWM. HW specific.
 * @param pulse Pulse width (in clock cycle) set to the PWM. HW specific.
 * @param flags Flags for pin configuration (polarity).
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int pwm_pin_set_cycles(const struct device *dev, uint32_t pwm,
				 uint32_t period, uint32_t pulse, pwm_flags_t flags);

static inline int z_impl_pwm_pin_set_cycles(const struct device *dev,
					    uint32_t pwm,
					    uint32_t period, uint32_t pulse,
					    pwm_flags_t flags)
{
	struct pwm_driver_api *api;

	api = (struct pwm_driver_api *)dev->api;
	return api->pin_set(dev, pwm, period, pulse, flags);
}

/**
 * @brief Configure PWM period/pulse width capture for a single PWM input.
 *
 * After configuring PWM capture using this function, the capture can be
 * enabled/disabled using @a pwm_pin_enable_capture() and @a
 * pwm_pin_disable_capture().
 *
 * @note This API function cannot be invoked from user space due to the use of a
 * function callback. In user space, one of the simpler API functions (@a
 * pwm_pin_capture_cycles(), @a pwm_pin_capture_usec(), or @a
 * pwm_pin_capture_nsec()) can be used instead.
 *
 * @note @option{CONFIG_PWM_CAPTURE} must be selected for this function to be
 * available.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.
 * @param flags PWM capture flags
 * @param cb Application callback handler function to be called upon capture
 * @param user_data User data to pass to the application callback handler
 *                  function
 *
 * @retval -EINVAL if invalid function parameters were given
 * @retval -ENOTSUP if PWM capture is not supported or the given flags are not
 *                  supported
 * @retval -EIO if IO error occurred while configuring
 * @retval -EBUSY if PWM capture is already in progress
 */
#ifdef CONFIG_PWM_CAPTURE
static inline int pwm_pin_configure_capture(const struct device *dev,
					    uint32_t pwm,
					    pwm_flags_t flags,
					    pwm_capture_callback_handler_t cb,
					    void *user_data)
{
	const struct pwm_driver_api *api = (struct pwm_driver_api *)dev->api;

	if (!api->pin_configure_capture) {
		return -ENOTSUP;
	}

	return api->pin_configure_capture(dev, pwm, flags, cb, user_data);
}
#endif /* CONFIG_PWM_CAPTURE */

/**
 * @brief Enable PWM period/pulse width capture for a single PWM input.
 *
 * The PWM pin must be configured using @a pwm_pin_configure_capture() prior to
 * calling this function.
 *
 * @note @option{CONFIG_PWM_CAPTURE} must be selected for this function to be
 * available.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.
 *
 * @retval 0 If successful.
 * @retval -EINVAL if invalid function parameters were given
 * @retval -ENOTSUP if PWM capture is not supported
 * @retval -EIO if IO error occurred while enabling PWM capture
 * @retval -EBUSY if PWM capture is already in progress
 */
__syscall int pwm_pin_enable_capture(const struct device *dev, uint32_t pwm);

#ifdef CONFIG_PWM_CAPTURE
static inline int z_impl_pwm_pin_enable_capture(const struct device *dev,
						uint32_t pwm)
{
	const struct pwm_driver_api *api = (struct pwm_driver_api *)dev->api;

	if (!api->pin_enable_capture) {
		return -ENOTSUP;
	}

	return api->pin_enable_capture(dev, pwm);
}
#endif /* CONFIG_PWM_CAPTURE */

/**
 * @brief Disable PWM period/pulse width capture for a single PWM input.
 *
 *
 * @note @option{CONFIG_PWM_CAPTURE} must be selected for this function to be
 * available.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.
 *
 * @retval 0 If successful.
 * @retval -EINVAL if invalid function parameters were given
 * @retval -ENOTSUP if PWM capture is not supported
 * @retval -EIO if IO error occurred while disabling PWM capture
 */
__syscall int pwm_pin_disable_capture(const struct device *dev, uint32_t pwm);

#ifdef CONFIG_PWM_CAPTURE
static inline int z_impl_pwm_pin_disable_capture(const struct device *dev,
						 uint32_t pwm)
{
	const struct pwm_driver_api *api = (struct pwm_driver_api *)dev->api;

	if (!api->pin_disable_capture) {
		return -ENOTSUP;
	}

	return api->pin_disable_capture(dev, pwm);
}
#endif /* CONFIG_PWM_CAPTURE */

/**
 * @brief Capture a single PWM period/pulse width in clock cycles for a single
 *        PWM input.
 *
 * This API function wraps calls to @a pwm_pin_configure_capture(), @a
 * pwm_pin_enable_capture(), and @a pwm_pin_disable_capture() and passes the
 * capture result to the caller. The function is blocking until either the PWM
 * capture is completed or a timeout occurs.
 *
 * @note @option{CONFIG_PWM_CAPTURE} must be selected for this function to be
 * available.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.
 * @param flags PWM capture flags.
 * @param period Pointer to the memory to store the captured PWM period width
 *               (in clock cycles). HW specific.
 * @param pulse Pointer to the memory to store the captured PWM pulse width (in
 *              clock cycles). HW specific.
 * @param timeout Waiting period for the capture to complete.
 *
 * @retval 0 If successful.
 * @retval -EBUSY PWM capture already in progress.
 * @retval -EAGAIN Waiting period timed out.
 * @retval -EIO IO error while capturing.
 * @retval -ERANGE If result is too large.
 */
__syscall int pwm_pin_capture_cycles(const struct device *dev, uint32_t pwm,
				     pwm_flags_t flags,
				     uint32_t *period,
				     uint32_t *pulse,
				     k_timeout_t timeout);

/**
 * @brief Get the clock rate (cycles per second) for a single PWM output.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.
 * @param cycles Pointer to the memory to store clock rate (cycles per sec).
 *		 HW specific.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int pwm_get_cycles_per_sec(const struct device *dev, uint32_t pwm,
				     uint64_t *cycles);

static inline int z_impl_pwm_get_cycles_per_sec(const struct device *dev,
						uint32_t pwm,
						uint64_t *cycles)
{
	struct pwm_driver_api *api;

	api = (struct pwm_driver_api *)dev->api;
	return api->get_cycles_per_sec(dev, pwm, cycles);
}

/**
 * @brief Set the period and pulse width for a single PWM output.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.
 * @param period Period (in microseconds) set to the PWM.
 * @param pulse Pulse width (in microseconds) set to the PWM.
 * @param flags Flags for pin configuration (polarity).
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int pwm_pin_set_usec(const struct device *dev, uint32_t pwm,
				   uint32_t period, uint32_t pulse,
				   pwm_flags_t flags)
{
	uint64_t period_cycles, pulse_cycles, cycles_per_sec;

	if (pwm_get_cycles_per_sec(dev, pwm, &cycles_per_sec) != 0) {
		return -EIO;
	}

	period_cycles = (period * cycles_per_sec) / USEC_PER_SEC;
	if (period_cycles >= ((uint64_t)1 << 32)) {
		return -ENOTSUP;
	}

	pulse_cycles = (pulse * cycles_per_sec) / USEC_PER_SEC;
	if (pulse_cycles >= ((uint64_t)1 << 32)) {
		return -ENOTSUP;
	}

	return pwm_pin_set_cycles(dev, pwm, (uint32_t)period_cycles,
				  (uint32_t)pulse_cycles, flags);
}

/**
 * @brief Set the period and pulse width for a single PWM output.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.
 * @param period Period (in nanoseconds) set to the PWM.
 * @param pulse Pulse width (in nanoseconds) set to the PWM.
 * @param flags Flags for pin configuration (polarity).
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int pwm_pin_set_nsec(const struct device *dev, uint32_t pwm,
				   uint32_t period, uint32_t pulse,
				   pwm_flags_t flags)
{
	uint64_t period_cycles, pulse_cycles, cycles_per_sec;

	if (pwm_get_cycles_per_sec(dev, pwm, &cycles_per_sec) != 0) {
		return -EIO;
	}

	period_cycles = (period * cycles_per_sec) / NSEC_PER_SEC;
	if (period_cycles >= ((uint64_t)1 << 32)) {
		return -ENOTSUP;
	}

	pulse_cycles = (pulse * cycles_per_sec) / NSEC_PER_SEC;
	if (pulse_cycles >= ((uint64_t)1 << 32)) {
		return -ENOTSUP;
	}

	return pwm_pin_set_cycles(dev, pwm, (uint32_t)period_cycles,
				  (uint32_t)pulse_cycles, flags);
}

/**
 * @brief Convert from PWM cycles to microseconds.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.
 * @param cycles Cycles to be converted.
 * @param usec Pointer to the memory to store calculated usec.
 *
 * @retval 0 If successful.
 * @retval -EIO If cycles per second cannot be determined.
 * @retval -ERANGE If result is too large.
 */
static inline int pwm_pin_cycles_to_usec(const struct device *dev, uint32_t pwm,
					 uint32_t cycles, uint64_t *usec)
{
	uint64_t cycles_per_sec;
	uint64_t temp;

	if (pwm_get_cycles_per_sec(dev, pwm, &cycles_per_sec) != 0) {
		return -EIO;
	}

	if (u64_mul_overflow(cycles, (uint64_t)USEC_PER_SEC, &temp)) {
		return -ERANGE;
	}

	*usec = temp / cycles_per_sec;

	return 0;
}

/**
 * @brief Convert from PWM cycles to nanoseconds.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.
 * @param cycles Cycles to be converted.
 * @param nsec Pointer to the memory to store the calculated nsec.
 *
 * @retval 0 If successful.
 * @retval -EIO If cycles per second cannot be determined.
 * @retval -ERANGE If result is too large.
 */
static inline int pwm_pin_cycles_to_nsec(const struct device *dev, uint32_t pwm,
					 uint32_t cycles, uint64_t *nsec)
{
	uint64_t cycles_per_sec;
	uint64_t temp;

	if (pwm_get_cycles_per_sec(dev, pwm, &cycles_per_sec) != 0) {
		return -EIO;
	}

	if (u64_mul_overflow(cycles, (uint64_t)NSEC_PER_SEC, &temp)) {
		return -ERANGE;
	}

	*nsec = temp / cycles_per_sec;

	return 0;
}

/**
 * @brief Capture a single PWM period/pulse width in microseconds for a single
 *        PWM input.
 *
 * This API function wraps calls to @a pwm_pin_capture_cycles() and @a
 * pwm_pin_cycles_to_usec() and passes the capture result to the caller. The
 * function is blocking until either the PWM capture is completed or a timeout
 * occurs.
 *
 * @note @option{CONFIG_PWM_CAPTURE} must be selected for this function to be
 * available.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.
 * @param flags PWM capture flags.
 * @param period Pointer to the memory to store the captured PWM period width
 *               (in usec).
 * @param pulse Pointer to the memory to store the captured PWM pulse width (in
 *              usec).
 * @param timeout Waiting period for the capture to complete.
 *
 * @retval 0 If successful.
 * @retval -EBUSY PWM capture already in progress.
 * @retval -EAGAIN Waiting period timed out.
 * @retval -EIO IO error while capturing.
 * @retval -ERANGE If result is too large.
 */
static inline int pwm_pin_capture_usec(const struct device *dev, uint32_t pwm,
				       pwm_flags_t flags,
				       uint64_t *period,
				       uint64_t *pulse,
				       k_timeout_t timeout)
{
	uint32_t period_cycles;
	uint32_t pulse_cycles;
	int err;

	err = pwm_pin_capture_cycles(dev, pwm, flags, &period_cycles,
				     &pulse_cycles, timeout);
	if (err) {
		return err;
	}

	err = pwm_pin_cycles_to_usec(dev, pwm, period_cycles, period);
	if (err) {
		return err;
	}

	err = pwm_pin_cycles_to_usec(dev, pwm, pulse_cycles, pulse);
	if (err) {
		return err;
	}

	return 0;
}

/**
 * @brief Capture a single PWM period/pulse width in nanoseconds for a single
 *        PWM input.
 *
 * This API function wraps calls to @a pwm_pin_capture_cycles() and @a
 * pwm_pin_cycles_to_nsec() and passes the capture result to the caller. The
 * function is blocking until either the PWM capture is completed or a timeout
 * occurs.
 *
 * @note @option{CONFIG_PWM_CAPTURE} must be selected for this function to be
 * available.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.
 * @param flags PWM capture flags.
 * @param period Pointer to the memory to store the captured PWM period width
 *               (in nsec).
 * @param pulse Pointer to the memory to store the captured PWM pulse width (in
 *              nsec).
 * @param timeout Waiting period for the capture to complete.
 *
 * @retval 0 If successful.
 * @retval -EBUSY PWM capture already in progress.
 * @retval -EAGAIN Waiting period timed out.
 * @retval -EIO IO error while capturing.
 * @retval -ERANGE If result is too large.
 */
static inline int pwm_pin_capture_nsec(const struct device *dev, uint32_t pwm,
				       pwm_flags_t flags,
				       uint64_t *period,
				       uint64_t *pulse,
				       k_timeout_t timeout)
{
	uint32_t period_cycles;
	uint32_t pulse_cycles;
	int err;

	err = pwm_pin_capture_cycles(dev, pwm, flags, &period_cycles,
				     &pulse_cycles, timeout);
	if (err) {
		return err;
	}

	err = pwm_pin_cycles_to_nsec(dev, pwm, period_cycles, period);
	if (err) {
		return err;
	}

	err = pwm_pin_cycles_to_nsec(dev, pwm, pulse_cycles, pulse);
	if (err) {
		return err;
	}

	return 0;
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/pwm.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_PWM_H_ */
