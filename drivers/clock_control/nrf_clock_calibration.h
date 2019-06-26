/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_DRIVERS_CLOCK_CONTROL_NRF_CLOCK_CALIBRATION_H_
#define ZEPHYR_DRIVERS_CLOCK_CONTROL_NRF_CLOCK_CALIBRATION_H_

#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize LFCLK RC calibration.
 *
 * @param hfclk_dev HFCLK device.
 */
void z_nrf_clock_calibration_init(struct device *hfclk_dev);

/**
 * @brief Calibration interrupts handler
 *
 * Must be called from clock interrupt context.
 */
void z_nrf_clock_calibration_isr(void);

/**
 * @brief Start calibration.
 *
 * Function called when LFCLK RC clock is being started.
 *
 * @param dev LFCLK device.
 *
 * @retval true if clock can be started.
 * @retval false if clock was not stopped due to ongoing calibration and don't
 *	   need to be started again because it is still on.
 */
bool z_nrf_clock_calibration_start(struct device *dev);

/**
 * @brief Notify calibration module about LF clock start
 *
 * @param dev LFCLK device.
 */
void z_nrf_clock_calibration_lfclk_started(struct device *dev);

/**
 * @brief Stop calibration.
 *
 * Function called when LFCLK RC clock is being stopped.
 *
 * @param dev LFCLK device.
 *
 * @retval true if clock can be stopped.
 * @retval false if due to ongoing calibration clock cannot be stopped. In that
 *	   case calibration module will stop clock when calibration is
 *	   completed.
 */
bool z_nrf_clock_calibration_stop(struct device *dev);


#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_CLOCK_CONTROL_NRF_CLOCK_CALIBRATION_H_ */
