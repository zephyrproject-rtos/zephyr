/*
 * Copyright 2026 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ADC comparator driver API.
 * @ingroup comparator_adc
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_COMPARATOR_ADC_COMP_H_
#define ZEPHYR_INCLUDE_DRIVERS_COMPARATOR_ADC_COMP_H_

#include <stdint.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ADC comparator driver
 * @defgroup comparator_adc ADC comparator
 * @ingroup comparator_interface
 * @{
 */

/**
 * @brief Set the voltage threshold for an ADC comparator.
 *
 * @param dev ADC comparator device instance.
 * @param ref_mv Threshold voltage in millivolts above which output is high.
 *
 * @retval 0 On success.
 */
int comp_adc_comp_set_threshold(const struct device *dev, int32_t ref_mv);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_COMPARATOR_ADC_COMP_H_ */
