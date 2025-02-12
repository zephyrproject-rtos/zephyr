/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_NRF_CLOCK_CONTROL_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_NRF_CLOCK_CONTROL_H_

#include <zephyr/device.h>
#include <hal/nrf_clock.h>
#include <zephyr/sys/onoff.h>
#include <zephyr/drivers/clock_control.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Clocks handled by the CLOCK peripheral.
 *
 * Enum shall be used as a sys argument in clock_control API.
 */
enum clock_control_nrf_type {
	CLOCK_CONTROL_NRF_TYPE_HFCLK,
	CLOCK_CONTROL_NRF_TYPE_LFCLK,
#if NRF_CLOCK_HAS_HFCLK192M
	CLOCK_CONTROL_NRF_TYPE_HFCLK192M,
#endif
#if NRF_CLOCK_HAS_HFCLKAUDIO
	CLOCK_CONTROL_NRF_TYPE_HFCLKAUDIO,
#endif
	CLOCK_CONTROL_NRF_TYPE_COUNT
};

/* Define can be used with clock control API instead of enum directly to
 * increase code readability.
 */
#define CLOCK_CONTROL_NRF_SUBSYS_HF \
	((clock_control_subsys_t)CLOCK_CONTROL_NRF_TYPE_HFCLK)
#define CLOCK_CONTROL_NRF_SUBSYS_LF \
	((clock_control_subsys_t)CLOCK_CONTROL_NRF_TYPE_LFCLK)
#define CLOCK_CONTROL_NRF_SUBSYS_HF192M \
	((clock_control_subsys_t)CLOCK_CONTROL_NRF_TYPE_HFCLK192M)
#define CLOCK_CONTROL_NRF_SUBSYS_HFAUDIO \
	((clock_control_subsys_t)CLOCK_CONTROL_NRF_TYPE_HFCLKAUDIO)

/** @brief LF clock start modes. */
enum nrf_lfclk_start_mode {
	CLOCK_CONTROL_NRF_LF_START_NOWAIT,
	CLOCK_CONTROL_NRF_LF_START_AVAILABLE,
	CLOCK_CONTROL_NRF_LF_START_STABLE,
};

/* Define 32KHz clock source */
#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC
#define CLOCK_CONTROL_NRF_K32SRC NRF_CLOCK_LFCLK_RC
#endif
#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_XTAL
#define CLOCK_CONTROL_NRF_K32SRC NRF_CLOCK_LFCLK_XTAL
#endif
#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_SYNTH
#define CLOCK_CONTROL_NRF_K32SRC NRF_CLOCK_LFCLK_SYNTH
#endif
#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_EXT_LOW_SWING
#define CLOCK_CONTROL_NRF_K32SRC NRF_CLOCK_LFCLK_XTAL_LOW_SWING
#endif
#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_EXT_FULL_SWING
#define CLOCK_CONTROL_NRF_K32SRC NRF_CLOCK_LFCLK_XTAL_FULL_SWING
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

/** @brief Force LF clock calibration. */
void z_nrf_clock_calibration_force_start(void);

/** @brief Return number of calibrations performed.
 *
 * Valid when @kconfig{CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_DEBUG} is set.
 *
 * @return Number of calibrations or -1 if feature is disabled.
 */
int z_nrf_clock_calibration_count(void);

/** @brief Return number of attempts when calibration was skipped.
 *
 * Valid when @kconfig{CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_DEBUG} is set.
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

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_NRF_CLOCK_CONTROL_H_ */
