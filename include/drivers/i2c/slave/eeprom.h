/**
 * @file
 *
 * @brief Public APIs for the I2C EEPROM Slave driver.
 */

/*
 * Copyright (c) 2017 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef I2C_EEPROM_SLAVE_H
#define I2C_EEPROM_SLAVE_H

/**
 * @brief I2C EEPROM Slave Driver API
 * @defgroup i2c_eeprom_slave_api I2C EEPROM Slave Driver API
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
int eeprom_slave_program(struct device *dev, u8_t *eeprom_data,
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
int eeprom_slave_read(struct device *dev, u8_t *eeprom_data,
		      unsigned int offset);

/**
 * @}
 */

#endif /* I2C_EEPROM_SLAVE_H */
