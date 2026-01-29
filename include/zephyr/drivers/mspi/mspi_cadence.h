/*
 * Copyright (c) 2026 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MSPI_CADENCE_H_
#define ZEPHYR_INCLUDE_DRIVERS_MSPI_CADENCE_H_

#include <stdint.h>

/**
 * @brief Configure amount of read data capture cycles
 *
 * This routine configures the amount of read data capture cycles that are
 * applied to the internal data capture circuit.

 * @param controller Pointer to the device structure for the driver instance.
 * @param read_delay The amount of delay to be set
 *
 * @retval 0 If successful.
 * @retval -EINVAL If the read delay provided is too big
 */

int mspi_cadence_configure_read_delay(const struct device *controller, uint8_t read_delay);

#endif /* ZEPHYR_INCLUDE_DRIVERS_MSPI_CADENCE_H_ */
