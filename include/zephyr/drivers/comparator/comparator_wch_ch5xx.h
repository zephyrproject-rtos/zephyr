/*
 * SPDX-FileCopyrightText: 2026 SMILE (smile.eu)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for WCH CH5xx comparator driver
 * @ingroup comparator_wch_ch5xx_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_COMPARATOR_WCH_CH5XX_H_
#define ZEPHYR_INCLUDE_DRIVERS_COMPARATOR_WCH_CH5XX_H_

/**
 * @defgroup comparator_wch_ch5xx_interface WCH CH5xx comparator
 * @ingroup comparator_interface_ext
 * @brief WCH CH5xx comparator driver
 * @{
 */

#include <zephyr/drivers/comparator.h>

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

/** @cond INTERNAL_HIDDEN */

typedef int (*comparator_api_set_nref_level)(const struct device *dev, enum comp_nref_level level);

__subsystem struct comparator_wch_driver_api {
	struct comparator_driver_api parent_api;
	comparator_api_set_nref_level set_nref_level;
};

DEVICE_API_EXTENDS(comparator_wch, comparator, parent_api);

/** @endcond */

/**
 * @brief Set the negative reference level for the comparator
 *
 * @param dev Comparator device
 * @param level Negative reference level to set
 *
 * @return 0 on success, negative errno value on failure.
 */
__syscall int comparator_set_nref_level(const struct device *dev, enum comp_nref_level level);

static inline int z_impl_comparator_set_nref_level(const struct device *dev,
						   enum comp_nref_level level)
{
	return DEVICE_API_GET(comparator_wch, dev)->set_nref_level(dev, level);
}

#ifdef __cplusplus
}
#endif

/** @} */

#include <zephyr/syscalls/comparator_wch_ch5xx.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_COMPARATOR_WCH_CH5XX_H_ */
