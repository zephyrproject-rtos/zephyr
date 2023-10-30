/*
 * Copyright (c) 2023 Grinn
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MFD_AD5592_H_
#define ZEPHYR_INCLUDE_DRIVERS_MFD_AD5592_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/device.h>

#define AD5592_REG_SEQ_ADC 0x02U
#define AD5592_REG_ADC_CONFIG 0x04U
#define AD5592_REG_LDAC_EN 0x05U
#define AD5592_REG_GPIO_PULLDOWN 0x06U
#define AD5592_REG_READ_AND_LDAC 0x07U
#define AD5592_REG_GPIO_OUTPUT_EN 0x08U
#define AD5592_REG_GPIO_SET 0x09U
#define AD5592_REG_GPIO_INPUT_EN 0x0AU
#define AD5592_REG_PD_REF_CTRL 0x0BU

#define AD5592_EN_REF BIT(9)

#define AD5592_PIN_MAX 8U

/**
 * @defgroup mdf_interface_ad5592 MFD AD5592 interface
 * @ingroup mfd_interfaces
 * @{
 */

/**
 * @brief Read raw data from the chip
 *
 * @param[in] dev Pointer to MFD device
 * @param[in] val Pointer to data buffer
 *
 * @retval 0 if success
 * @retval negative errno if failure
 */
int mfd_ad5592_read_raw(const struct device *dev, uint16_t *val);

/**
 * @brief Write raw data to chip
 *
 * @param[in] dev Pointer to MFD device
 * @param[in] val Data to be written
 *
 *
 * @retval 0 if success
 * @retval negative errno if failure
 */
int mfd_ad5592_write_raw(const struct device *dev, uint16_t val);

/**
 * @brief Read data from provided register
 *
 * @param[in] dev Pointer to MFD device
 * @param[in] reg Register to be read
 * @param[in] reg_data Additional data passed to selected register
 * @param[in] val Pointer to data buffer
 *
 * @retval 0 if success
 * @retval negative errno if failure
 */
int mfd_ad5592_read_reg(const struct device *dev, uint8_t reg, uint8_t reg_data, uint16_t *val);

/**
 * @brief Write data to provided register
 *
 * @param[in] dev Pointer to MFD device
 * @param[in] reg Register to be written
 * @param[in] val Data to be written
 *
 * @retval 0 if success
 * @retval negative errno if failure
 */
int mfd_ad5592_write_reg(const struct device *dev, uint8_t reg, uint16_t val);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_AD5952_H_ */
