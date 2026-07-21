/*
 * Copyright (c) 2025 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for CY8CMBR3xxx input driver.
 * @ingroup cy8cmbr3xxx_interface
 */

#ifndef ZEPHYR_INCLUDE_INPUT_CY8CMBR3XXX_H_
#define ZEPHYR_INCLUDE_INPUT_CY8CMBR3XXX_H_

/**
 * @defgroup cy8cmbr3xxx_interface CY8CMBR3xxx
 * @ingroup input_interface_ext
 * @brief CY8CMBR3xxx capacitive touch sensor
 * @{
 */

#include <zephyr/types.h>

/**
 * @brief Size of the EZ-Click configuration data
 */
#define CY8CMBR3XXX_EZ_CLICK_CONFIG_SIZE 128

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Configuration data for the CY8CMBR3xxx device
 */
struct cy8cmbr3xxx_config_data {
	/** EZ-Click configuration data */
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

/** @} */

#endif /* ZEPHYR_INCLUDE_INPUT_CY8CMBR3XXX_H_ */
