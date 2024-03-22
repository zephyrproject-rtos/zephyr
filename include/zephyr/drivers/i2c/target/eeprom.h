/**
 * @file
 *
 * @brief Public APIs for the I2C EEPROM Target driver.
 */

/*
 * Copyright (c) 2017 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_I2C_TARGET_EEPROM_H_
#define ZEPHYR_INCLUDE_DRIVERS_I2C_TARGET_EEPROM_H_

/**
 * @brief I2C EEPROM Target Driver API
 * @defgroup i2c_eeprom_target_api I2C EEPROM Target Driver API
 * @since 1.13
 * @version 1.0.0
 * @ingroup io_interfaces
 * @{
 */

/**
 * @brief Program memory of the virtual EEPROM
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param eeprom_data Pointer of data to program into the virtual eeprom memory
 * @param length Length of data to program into the virtual eeprom memory
 *
 * @retval 0 If successful.
 * @retval -EINVAL Invalid data size
 */
int eeprom_target_program(const struct device *dev, const uint8_t *eeprom_data,
			 unsigned int length);

/**
 * @brief Read single byte of virtual EEPROM memory
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param eeprom_data Pointer of byte where to store the virtual eeprom memory
 * @param offset Offset into EEPROM memory where to read the byte
 *
 * @retval 0 If successful.
 * @retval -EINVAL Invalid data pointer or offset
 */
int eeprom_target_read(const struct device *dev, uint8_t *eeprom_data,
		      unsigned int offset);

/**
 * @brief Change the address of eeprom taget at runtime
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param addr New address to assign to the eeprom target devide
 *
 * @retval 0 Is successful
 * @retval -EINVAL If parameters are invalid
 * @retval -EIO General input / output error during i2c_taget_register
 * @retval -ENOSYS If target mode is not implemented
 */
int eeprom_target_set_addr(const struct device *dev, uint8_t addr);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_I2C_TARGET_EEPROM_H_ */
