/*
 * SPDX-FileCopyrightText: 2026 SMILE (smile.eu)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for WCH CH5xx comparator driver
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_COMPARATOR_WCH_CH5XX_H_
#define ZEPHYR_INCLUDE_DRIVERS_COMPARATOR_WCH_CH5XX_H_

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Available negative reference levels */
enum comp_nref_level {
	COMP_NREF_LEVEL_50 = 0, /**< 50 mV internal negative reference */
	COMP_NREF_LEVEL_100,    /**< 100 mV internal negative reference */
	COMP_NREF_LEVEL_150,    /**< 150 mV internal negative reference */
	COMP_NREF_LEVEL_200,    /**< 200 mV internal negative reference */
	COMP_NREF_LEVEL_250,    /**< 250 mV internal negative reference */
	COMP_NREF_LEVEL_300,    /**< 300 mV internal negative reference */
	COMP_NREF_LEVEL_350,    /**< 350 mV internal negative reference */
	COMP_NREF_LEVEL_400,    /**< 400 mV internal negative reference */
	COMP_NREF_LEVEL_450,    /**< 450 mV internal negative reference */
	COMP_NREF_LEVEL_500,    /**< 500 mV internal negative reference */
	COMP_NREF_LEVEL_550,    /**< 550 mV internal negative reference */
	COMP_NREF_LEVEL_600,    /**< 600 mV internal negative reference */
	COMP_NREF_LEVEL_650,    /**< 650 mV internal negative reference */
	COMP_NREF_LEVEL_700,    /**< 700 mV internal negative reference */
	COMP_NREF_LEVEL_750,    /**< 750 mV internal negative reference */
	COMP_NREF_LEVEL_800,    /**< 800 mV internal negative reference */
};

/**
 * @brief Set the negative reference level for the comparator
 *
 * @param dev Comparator device
 * @param level Negative reference level to set
 *
 * @return 0 on success, negative errno value on failure.
 */
int comparator_wch_set_nref_level(const struct device *dev, enum comp_nref_level level);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_COMPARATOR_WCH_CH5XX_H_ */
