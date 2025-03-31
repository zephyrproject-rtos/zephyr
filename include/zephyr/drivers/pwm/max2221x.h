/*
 * Copyright (c) 2025 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for MAX2221X PWM driver
 * @ingroup pwm_max2221x_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PWM_MAX2221X_H_
#define ZEPHYR_INCLUDE_DRIVERS_PWM_MAX2221X_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/device.h>
#include <zephyr/sys/util.h>

/**
 * @defgroup pwm_max2221x_interface MAX2221X PWM
 * @ingroup pwm_interface_ext
 * @{
 */

/** @brief Master chopping frequency options */
enum max2221x_master_chop_freq {
	MAX2221X_FREQ_100KHZ = 0, /**< 100 kHz */
	MAX2221X_FREQ_80KHZ,      /**< 80 kHz */
	MAX2221X_FREQ_60KHZ,      /**< 60 kHz */
	MAX2221X_FREQ_50KHZ,      /**< 50 kHz */
	MAX2221X_FREQ_40KHZ,      /**< 40 kHz */
	MAX2221X_FREQ_30KHZ,      /**< 30 kHz */
	MAX2221X_FREQ_25KHZ,      /**< 25 kHz */
	MAX2221X_FREQ_20KHZ,      /**< 20 kHz */
	MAX2221X_FREQ_15KHZ,      /**< 15 kHz */
	MAX2221X_FREQ_10KHZ,      /**< 10 kHz */
	MAX2221X_FREQ_7500HZ,     /**< 7.5 kHz */
	MAX2221X_FREQ_5000HZ,     /**< 5 kHz */
	MAX2221X_FREQ_2500HZ,     /**< 2.5 kHz */

	MAX2221X_FREQ_INVALID, /**< Invalid frequency sentinel */
};

/** @brief Individual channel chopping frequency divisor options */
enum max2221x_individual_chop_freq {
	MAX2221X_FREQ_M = 0, /**< Master frequency (no division) */
	MAX2221X_FREQ_M_2,   /**< Master frequency divided by 2 */
	MAX2221X_FREQ_M_4,   /**< Master frequency divided by 4 */
	MAX2221X_FREQ_M_8,   /**< Master frequency divided by 8 */

	MAX2221X_FREQ_M_INVALID, /**< Invalid frequency divisor sentinel */
};

/**
 * @brief Get the master chop frequency of the MAX2221X device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return The master chop frequency in Hz on success, or a negative error code on failure.
 */
int max2221x_get_master_chop_freq(const struct device *dev);

/**
 * @brief Get the individual channel frequency of the MAX2221X device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel The channel to read.
 * @param channel_freq Pointer to the variable to store the individual channel frequency.
 * @return The individual channel frequency in Hz on success, or a negative error code on failure.
 */
int max2221x_get_channel_freq(const struct device *dev, uint32_t channel, uint32_t *channel_freq);

/**
 * @brief Calculate the duty cycle.
 *
 * @param pulse Pulse width (in microseconds) set to the PWM. HW specific.
 * @param period Period (in microseconds) set to the PWM. HW specific.
 * @param duty_cycle Pointer to the variable to store the duty cycle.
 * @return 0 on success, or a negative error code on failure.
 */
int max2221x_calculate_duty_cycle(uint32_t pulse, uint32_t period, uint16_t *duty_cycle);

/**
 * @brief Calculate the master frequency divisor used for calculating individual frequency.
 *
 * @param master_freq Master frequency (in Hz) set to the PWM. HW specific.
 * @param period Period (in microseconds) set to the PWM. HW specific.
 * @param freq_divisor Master frequency divisor set per channel.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int max2221x_calculate_master_freq_divisor(uint32_t master_freq, uint32_t period,
					   int *freq_divisor);

/**
 * @brief Get the cycles per second for the pwm channel.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel The channel to get the cycles.
 * @param cycles Pointer to the variable to store the cycles.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int max2221x_get_cycles_per_sec(const struct device *dev, uint32_t channel, uint64_t *cycles);

/**
 * @brief Set the duty cycle for the pwm channel.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel The channel to set the cycles.
 * @param period Period (in microseconds) set to the PWM. HW specific.
 * @param pulse Pulse width (in microseconds) set to the PWM. HW specific.
 * @param flags Flags for pin configuration.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int max2221x_set_cycles(const struct device *dev, uint32_t channel, uint32_t period, uint32_t pulse,
			pwm_flags_t flags);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_PWM_MAX2221X_H_ */
