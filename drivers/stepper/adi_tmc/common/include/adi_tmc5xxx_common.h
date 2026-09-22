/**
 * @file drivers/stepper/adi_tmc/adi_tmc5xxx_common.h
 *
 * @brief Common TMC5xxx stepper controller driver definitions
 */

/**
 * SPDX-FileCopyrightText: Copyright (c) 2024 Jilay Sandeep Pandya
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_STEPPER_ADI_TMC_COMMON_ADI_TMC5XXX_COMMON_H_
#define ZEPHYR_DRIVERS_STEPPER_ADI_TMC_COMMON_ADI_TMC5XXX_COMMON_H_

#include <zephyr/sys/util.h>

#include <adi_tmc_reg.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name TMC5xxx module functions
 * @anchor TMC5XXX_FUNCTIONS
 *
 * @{
 */

/**
 * @brief Calculate the velocity in full clock cycles from the velocity in Hz
 *
 * @param velocity_hz Velocity in Hz
 * @param clock_frequency Clock frequency in Hz
 *
 * @return Calculated velocity in full clock cycles
 */
static inline uint32_t tmc5xxx_calculate_velocity_from_hz_to_fclk(uint64_t velocity_hz,
								  uint32_t clock_frequency)
{
	__ASSERT_NO_MSG(clock_frequency);
	return (velocity_hz << TMC5XXX_CLOCK_FREQ_SHIFT) / clock_frequency;
}

/**
 * @brief Calculate the acceleration in full clock cycles from the acceleration in Hz
 * @note a[Hz/s] = a[5160] * fCLK[Hz]^2 / (512*256) / 2^24
 *
 * @param acceleration_hz Acceleration in Hz
 * @param clock_frequency Clock frequency in Hz
 *
 * @return Calculated acceleration in full clock cycles
 */
static inline uint32_t tmc5xxx_calculate_acceleration_from_hz_to_fclk(uint64_t acceleration_hz,
								      uint64_t clock_frequency)
{
	uint64_t shifted_accel = MIN(acceleration_hz, UINT64_MAX >> (TMC5XXX_CLOCK_FREQ_SHIFT +
								     TMC5XXX_ACCEL_CALC_SHIFT));

	return DIV_ROUND_CLOSEST((shifted_accel << TMC5XXX_CLOCK_FREQ_SHIFT)
					 << TMC5XXX_ACCEL_CALC_SHIFT,
				 (clock_frequency * clock_frequency));
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_STEPPER_ADI_TMC_COMMON_ADI_TMC5XXX_COMMON_H_ */
