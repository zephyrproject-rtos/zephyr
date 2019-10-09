/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_NRF_CLOCK_CONTROL_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_NRF_CLOCK_CONTROL_H_

#include <device.h>
#include <hal/nrf_clock.h>

/* TODO: move all these to clock_control.h ? */

/* Define 32KHz clock source */
#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC
#define CLOCK_CONTROL_NRF_K32SRC NRF_CLOCK_LFCLK_RC
#endif
#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_XTAL
#define CLOCK_CONTROL_NRF_K32SRC NRF_CLOCK_LFCLK_Xtal
#endif
#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_SYNTH
#define CLOCK_CONTROL_NRF_K32SRC NRF_CLOCK_LFCLK_Synth
#endif
#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_EXT_LOW_SWING
#define CLOCK_CONTROL_NRF_K32SRC NRF_CLOCK_LFCLK_Xtal_Low_Swing
#endif
#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_EXT_FULL_SWING
#define CLOCK_CONTROL_NRF_K32SRC NRF_CLOCK_LFCLK_Xtal_Full_Swing
#endif

/* Define 32KHz clock accuracy */
#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_500PPM
#define CLOCK_CONTROL_NRF_K32SRC_ACCURACY 0
#endif
#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_250PPM
#define CLOCK_CONTROL_NRF_K32SRC_ACCURACY 1
#endif
#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_150PPM
#define CLOCK_CONTROL_NRF_K32SRC_ACCURACY 2
#endif
#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_100PPM
#define CLOCK_CONTROL_NRF_K32SRC_ACCURACY 3
#endif
#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_75PPM
#define CLOCK_CONTROL_NRF_K32SRC_ACCURACY 4
#endif
#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_50PPM
#define CLOCK_CONTROL_NRF_K32SRC_ACCURACY 5
#endif
#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_30PPM
#define CLOCK_CONTROL_NRF_K32SRC_ACCURACY 6
#endif
#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_20PPM
#define CLOCK_CONTROL_NRF_K32SRC_ACCURACY 7
#endif

void nrf5_power_usb_power_int_enable(bool enable);

/** @brief Force LF clock calibration. */
void z_nrf_clock_calibration_force_start(void);

/** @brief Return number of calibrations performed.
 *
 * Valid when @ref CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_DEBUG is set.
 *
 * @return Number of calibrations or -1 if feature is disabled.
 */
int z_nrf_clock_calibration_count(void);

/** @brief Return number of attempts when calibration was skipped.
 *
 * Valid when @ref CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_DEBUG is set.
 *
 * @return Number of calibrations or -1 if feature is disabled.
 */
int z_nrf_clock_calibration_skips_count(void);

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_NRF_CLOCK_CONTROL_H_ */
