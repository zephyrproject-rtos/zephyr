/*
 * Copyright (c) 2025 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_INPUT_CY8CMBR3XXX_H_
#define ZEPHYR_INCLUDE_INPUT_CY8CMBR3XXX_H_

#include <zephyr/types.h>

#define CY8CMBR3XXX_EZ_CLICK_CONFIG_SIZE 128

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct cy8cmbr3xxx_config_data {
	uint8_t data[CY8CMBR3XXX_EZ_CLICK_CONFIG_SIZE];
};

/**
 * @brief Configure the CY8CMBR3xxx device with an EZ-Click generated configuration.
 *
 * @param dev Pointer to the input device instance
 * @param config_data Pointer to the configuration data for the device
 *
 * @retval 0 if successful
 * @retval <0 if failed
 */
int cy8cmbr3xxx_configure(const struct device *dev,
			  const struct cy8cmbr3xxx_config_data *config_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZEPHYR_INCLUDE_INPUT_CY8CMBR3XXX_H_ */
