/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_COMP_NRF_COMP_H_
#define ZEPHYR_INCLUDE_DRIVERS_COMP_NRF_COMP_H_

#include <zephyr/dt-bindings/comparator/nrf-comp.h>
#include <zephyr/drivers/comparator.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Reference selection */
enum comp_nrf_comp_refsel {
	/** Internal 1.2V reference */
	COMP_NRF_COMP_REFSEL_INT_1V2,
	/** Internal 1.8V reference */
	COMP_NRF_COMP_REFSEL_INT_1V8,
	/** Internal 2.4V reference */
	COMP_NRF_COMP_REFSEL_INT_2V4,
	/** AVDD 1.8V reference */
	COMP_NRF_COMP_REFSEL_AVDDAO1V8,
	/** VDD reference */
	COMP_NRF_COMP_REFSEL_VDD,
	/** Use external analog reference */
	COMP_NRF_COMP_REFSEL_AREF,
};

/** Speed mode selection */
enum comp_nrf_comp_sp_mode {
	/** Low-power mode */
	COMP_NRF_COMP_SP_MODE_LOW,
	/** Normal mode */
	COMP_NRF_COMP_SP_MODE_NORMAL,
	/** High-speed mode */
	COMP_NRF_COMP_SP_MODE_HIGH,
};

/** Current source configuration */
enum comp_nrf_comp_isource {
	/** Current source disabled */
	COMP_NRF_COMP_ISOURCE_DISABLED,
	/** 2.5uA current source enabled */
	COMP_NRF_COMP_ISOURCE_2UA5,
	/** 5uA current source enabled */
	COMP_NRF_COMP_ISOURCE_5UA,
	/** 10uA current source enabled */
	COMP_NRF_COMP_ISOURCE_10UA,
};

/**
 * @brief Single-ended mode configuration structure
 *
 * @note extrefsel is only used if refsel == COMP_NRF_COMP_REFSEL_AREF
 * @note Hysteresis down in volts = ((th_down + 1) / 64) * ref
 * @note Hysteresis up in volts = ((th_up + 1) / 64) * ref
 */
struct comp_nrf_comp_se_config {
	/** Positive input selection defined by the NRF_COMP_AIN defines */
	uint8_t psel;
	/** Speed mode selection */
	enum comp_nrf_comp_sp_mode sp_mode;
	/** Current source configuration */
	enum comp_nrf_comp_isource isource;
	/** External reference input selection defined by the NRF_COMP_AIN defines */
	uint8_t extrefsel;
	/** Reference selection */
	enum comp_nrf_comp_refsel refsel;
	/** Hysteresis down threshold configuration */
	uint8_t th_down;
	/** Hysteresis up threshold configuration */
	uint8_t th_up;
};

/**
 * @brief Configure comparator in single-ended mode
 *
 * @param dev Comparator device instance
 * @param config Single-ended mode configuration
 *
 * @retval 0 if successful
 * @retval negative errno-code otherwise
 */
int comp_nrf_comp_configure_se(const struct device *dev,
			       const struct comp_nrf_comp_se_config *config);

/** Differential mode configuration structure */
struct comp_nrf_comp_diff_config {
	/** Positive input selection defined by the NRF_COMP_AIN defines */
	uint8_t psel;
	/** Speed mode selection */
	enum comp_nrf_comp_sp_mode sp_mode;
	/** Current source configuration */
	enum comp_nrf_comp_isource isource;
	/** Negative input selection defined by the NRF_COMP_AIN defines */
	uint8_t extrefsel;
	/** Hysteresis configuration */
	bool enable_hyst;
};

/**
 * @brief Configure comparator in differential mode
 *
 * @param dev Comparator device instance
 * @param config Differential mode configuration
 *
 * @retval 0 if successful
 * @retval negative errno-code otherwise
 */
int comp_nrf_comp_configure_diff(const struct device *dev,
				 const struct comp_nrf_comp_diff_config *config);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_COMP_NRF_COMP_H_ */
