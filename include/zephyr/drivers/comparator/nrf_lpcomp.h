/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_COMP_NRF_LPCOMP_H_
#define ZEPHYR_INCLUDE_DRIVERS_COMP_NRF_LPCOMP_H_

#include <zephyr/dt-bindings/comparator/nrf-comp.h>
#include <zephyr/drivers/comparator.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Reference selection */
enum comp_nrf_lpcomp_refsel {
	/** Use (VDD * (1/8)) as reference */
	COMP_NRF_LPCOMP_REFSEL_VDD_1_8,
	/** Use (VDD * (2/8)) as reference */
	COMP_NRF_LPCOMP_REFSEL_VDD_2_8,
	/** Use (VDD * (3/8)) as reference */
	COMP_NRF_LPCOMP_REFSEL_VDD_3_8,
	/** Use (VDD * (4/8)) as reference */
	COMP_NRF_LPCOMP_REFSEL_VDD_4_8,
	/** Use (VDD * (5/8)) as reference */
	COMP_NRF_LPCOMP_REFSEL_VDD_5_8,
	/** Use (VDD * (6/8)) as reference */
	COMP_NRF_LPCOMP_REFSEL_VDD_6_8,
	/** Use (VDD * (7/8)) as reference */
	COMP_NRF_LPCOMP_REFSEL_VDD_7_8,
	/** Use (VDD * (1/16)) as reference */
	COMP_NRF_LPCOMP_REFSEL_VDD_1_16,
	/** Use (VDD * (3/16)) as reference */
	COMP_NRF_LPCOMP_REFSEL_VDD_3_16,
	/** Use (VDD * (5/16)) as reference */
	COMP_NRF_LPCOMP_REFSEL_VDD_5_16,
	/** Use (VDD * (7/16)) as reference */
	COMP_NRF_LPCOMP_REFSEL_VDD_7_16,
	/** Use (VDD * (9/16)) as reference */
	COMP_NRF_LPCOMP_REFSEL_VDD_9_16,
	/** Use (VDD * (11/16)) as reference */
	COMP_NRF_LPCOMP_REFSEL_VDD_11_16,
	/** Use (VDD * (13/16)) as reference */
	COMP_NRF_LPCOMP_REFSEL_VDD_13_16,
	/** Use (VDD * (15/16)) as reference */
	COMP_NRF_LPCOMP_REFSEL_VDD_15_16,
	/** Use external analog reference */
	COMP_NRF_LPCOMP_REFSEL_AREF,
};

/**
 * @brief Configuration structure
 *
 * @note extrefsel is only used if refsel == COMP_NRF_LPCOMP_REFSEL_AREF
 */
struct comp_nrf_lpcomp_config {
	/** Positive input selection defined by the NRF_COMP_AIN defines */
	uint8_t psel;
	/** External reference input selection defined by the NRF_COMP_AIN defines */
	uint8_t extrefsel;
	/** Reference selection */
	enum comp_nrf_lpcomp_refsel refsel;
	/** Hysteresis configuration */
	bool enable_hyst;
};

/**
 * @brief Configure comparator
 *
 * @param dev Comparator device instance
 * @param config Configuration
 *
 * @retval 0 if successful
 * @retval negative errno-code otherwise
 */
int comp_nrf_lpcomp_configure(const struct device *dev,
			      const struct comp_nrf_lpcomp_config *config);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_COMP_NRF_LPCOMP_H_ */
