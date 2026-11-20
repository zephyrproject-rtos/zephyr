/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup regulator_parent_max20356
 * @brief Public API for the MAX20356/MAX20358 PMIC regulator driver.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_REGULATOR_MAX20356_H_
#define ZEPHYR_INCLUDE_DRIVERS_REGULATOR_MAX20356_H_

#include <stdint.h>

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup regulator_parent_max20356 MAX20356 API
 * @ingroup regulator_parent_interface
 * @brief Public API for the MAX20356/MAX20358 regulator driver.
 * @{
 */

/**
 * @brief Set a buck's output voltage over the SPI DVS interface.
 *
 * @param dev Regulator rail device (a buck in SPI DVS mode).
 * @param min_uv Minimum acceptable voltage in microvolts.
 * @param max_uv Maximum acceptable voltage in microvolts.
 *
 * @retval 0 On success.
 * @retval -ENOTSUP Rail is not a buck in SPI DVS mode.
 * @retval -EINVAL No voltage code falls within [min_uv, max_uv].
 * @retval -ENODEV SPI device not present or not ready.
 * @retval -errno Negative errno propagated from the SPI bus.
 */
int regulator_max20356_spi_set_voltage(const struct device *dev, int min_uv, int max_uv);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_REGULATOR_MAX20356_H_ */
