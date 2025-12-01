/*
 * Copyright (c) 2025 Vaisala Oyj
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVER_DAC_DAC_EMUL_H_
#define ZEPHYR_INCLUDE_DRIVER_DAC_DAC_EMUL_H_

#include <zephyr/device.h>
#include <stdint.h>

/**
 * @brief Read a previously written value from the driver instance
 *
 * The channel must have been previously configured.
 *
 * @param dev The dac emulator device
 * @param channel The channel number to read
 * @param value The current channel value
 *
 * @retval 0 Success
 * @retval -EINVAL Invalid channel or NULL pointer
 * @retval -ENXIO Channel not configured
 * @retval -EBUSY Could not acquire channel lock
 *
 **/
int dac_emul_value_get(const struct device *dev, uint8_t channel, uint32_t *value);

#endif /* ZEPHYR_INCLUDE_DRIVER_DAC_DAC_EMUL_H_ */
