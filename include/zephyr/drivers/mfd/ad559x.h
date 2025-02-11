/*
 * Copyright (c) 2023 Grinn
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MFD_AD559X_H_
#define ZEPHYR_INCLUDE_DRIVERS_MFD_AD559X_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/device.h>

#define AD559X_REG_SEQ_ADC        0x02U
#define AD559X_REG_ADC_CONFIG     0x04U
#define AD559X_REG_LDAC_EN        0x05U
#define AD559X_REG_GPIO_PULLDOWN  0x06U
#define AD559X_REG_READ_AND_LDAC  0x07U
#define AD559X_REG_GPIO_OUTPUT_EN 0x08U
#define AD559X_REG_GPIO_SET       0x09U
#define AD559X_REG_GPIO_INPUT_EN  0x0AU
#define AD559X_REG_PD_REF_CTRL    0x0BU

#define AD559X_EN_REF BIT(9)

#define AD559X_PIN_MAX 8U

/**
 * @defgroup mdf_interface_ad559x MFD AD559X interface
 * @ingroup mfd_interfaces
 * @{
 */

/**
 * @brief Check if the chip has a pointer byte map
 *
 * @param[in] dev Pointer to MFD device
 *
 * @retval true if chip has a pointer byte map, false if it has normal register map
 */
bool mfd_ad559x_has_pointer_byte_map(const struct device *dev);

/**
 * @brief Read raw data from the chip
 *
 * @param[in] dev Pointer to MFD device
 * @param[in] val Pointer to data buffer
 * @param[in] len Number of bytes to be read
 *
 * @retval 0 if success
 * @retval negative errno if failure
 */
int mfd_ad559x_read_raw(const struct device *dev, uint8_t *val, size_t len);

/**
 * @brief Write raw data to chip
 *
 * @param[in] dev Pointer to MFD device
 * @param[in] val Data to be written
 * @param[in] len Number of bytes to be written
 *
 * @retval 0 if success
 * @retval negative errno if failure
 */
int mfd_ad559x_write_raw(const struct device *dev, uint8_t *val, size_t len);

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
int mfd_ad559x_read_reg(const struct device *dev, uint8_t reg, uint8_t reg_data, uint16_t *val);

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
int mfd_ad559x_write_reg(const struct device *dev, uint8_t reg, uint16_t val);

/**
 * @brief Read ADC channel data from the chip
 *
 * @param[in] dev Pointer to MFD device
 * @param[in] channel Channel to read
 * @param[out] result ADC channel value read
 *
 * @retval 0 if success
 * @retval negative errno if failure
 */
int mfd_ad559x_read_adc_chan(const struct device *dev, uint8_t channel, uint16_t *result);

/**
 * @brief Write ADC channel data to the chip
 *
 * @param[in] dev Pointer to MFD device
 * @param[in] channel Channel to write to
 * @param[in] value DAC channel value
 *
 * @retval 0 if success
 * @retval negative errno if failure
 */
int mfd_ad559x_write_dac_chan(const struct device *dev, uint8_t channel, uint16_t value);

/**
 * @brief Read GPIO port from the chip
 *
 * @param[in] dev Pointer to MFD device
 * @param[in] gpio GPIO to read
 * @param[in] value DAC channel value
 *
 * @retval 0 if success
 * @retval negative errno if failure
 */
int mfd_ad559x_gpio_port_get_raw(const struct device *dev, uint8_t gpio, uint16_t *value);
/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_AD559X_H_ */
