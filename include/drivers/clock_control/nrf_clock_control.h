/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_NRF_CLOCK_CONTROL_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_NRF_CLOCK_CONTROL_H_

#include <device.h>
#include <hal/nrf_clock.h>
#include <sys/onoff.h>
#include <drivers/clock_control.h>

/** @brief Clocks handled by the CLOCK peripheral.
 *
 * Enum shall be used as a sys argument in clock_control API.
 */
enum clock_control_nrf_type {
	CLOCK_CONTROL_NRF_TYPE_HFCLK,
	CLOCK_CONTROL_NRF_TYPE_LFCLK,
	CLOCK_CONTROL_NRF_TYPE_COUNT
};

/* Define can be used with clock control API instead of enum directly to
 * increase code readability.
 */
#define CLOCK_CONTROL_NRF_SUBSYS_HF \
	((clock_control_subsys_t)CLOCK_CONTROL_NRF_TYPE_HFCLK)
#define CLOCK_CONTROL_NRF_SUBSYS_LF \
	((clock_control_subsys_t)CLOCK_CONTROL_NRF_TYPE_LFCLK)

/** @brief LF clock start modes. */
enum nrf_lfclk_start_mode {
	NRF_LFCLK_START_MODE_NOWAIT,
	NRF_LFCLK_START_MODE_SPINWAIT_RUNNING,
	NRF_LFCLK_START_MODE_SPINWAIT_STABLE,
};

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

#if defined(CONFIG_USB_NRFX)
void nrf5_power_usb_power_int_enable(bool enable);
#endif

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

/** @brief Get onoff service for given clock subsystem.
 *
 * @param sys Subsystem.
 *
 * @return Service handler or NULL.
 */
struct onoff_manager *z_nrf_clock_control_get_onoff(clock_control_subsys_t sys);

/** @brief Permanently enable low frequency clock.
 *
 * Low frequency clock is usually enabled during application lifetime because
 * of long startup time and low power consumption. Multiple modules can request
 * it but never release.
 *
 * @param start_mode Specify if function should block until clock is available.
 */
void z_nrf_clock_control_lf_on(enum nrf_lfclk_start_mode start_mode);

/** @brief Request high frequency clock from Bluetooth Controller.
 *
 * Function is optimized for Bluetooth Controller which turns HF clock before
 * each radio activity and has hard timing requirements but does not require
 * any confirmation when clock is ready because it assumes that request is
 * performed long enough before radio activity. Clock is released immediately
 * after radio activity.
 *
 * Function does not perform any validation. It is the caller responsibility to
 * ensure that every z_nrf_clock_bt_ctlr_hf_request matches
 * z_nrf_clock_bt_ctlr_hf_release call.
 */
void z_nrf_clock_bt_ctlr_hf_request(void);

/** @brief Release high frequency clock from Bluetooth Controller.
 *
 * See z_nrf_clock_bt_ctlr_hf_request for details.
 */
void z_nrf_clock_bt_ctlr_hf_release(void);

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_NRF_CLOCK_CONTROL_H_ */
