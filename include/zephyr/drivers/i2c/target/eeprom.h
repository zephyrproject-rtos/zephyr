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

#include <stddef.h>
#include <sys/types.h>

/**
 * @brief Interfaces for I2C EEPROM target devices.
 * @defgroup i2c_eeprom_target_api I2C EEPROM Target
 * @since 1.13
 * @version 1.0.0
 * @ingroup i2c_interface
 * @{
 */

/**
 * @typedef eeprom_target_changed_handler_t
 * @brief Define the application callback handler function signature
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param user_data Optional user data provided when callback is set.
 */
typedef void (*eeprom_target_changed_handler_t)(const struct device *dev, void *user_data);

/**
 * @brief Set the EEPROM changed callback handler
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param handler Handler to call on EEPROM changes
 * @param user_data Optional user data passed to callback
 */
void eeprom_target_set_changed_callback(const struct device *dev,
					eeprom_target_changed_handler_t handler,
					void *user_data);

/**
 * @brief Get size of the virtual EEPROM
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return Size of EEPROM in bytes
 */
size_t eeprom_target_get_size(const struct device *dev);

/**
 * @brief Read data from the virtual EEPROM
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param offset Address offset to read from.
 * @param data Buffer to store read data.
 * @param len Number of bytes to read.
 *
 * @return 0 on success, negative errno code on failure.
 */
int eeprom_target_read_data(const struct device *dev, off_t offset,
			    void *data, size_t len);

/**
 * @brief Write data to the virtual EEPROM
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param offset Address offset to write data to.
 * @param data Buffer with data to write.
 * @param len Number of bytes to write.
 *
 * @return 0 on success, negative errno code on failure.
 */
int eeprom_target_write_data(const struct device *dev, off_t offset,
			     const void *data, size_t len);

/**
 * @brief Program memory of the virtual EEPROM
 *
 * @deprecated Use @ref eeprom_target_write_data instead.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param eeprom_data Pointer of data to program into the virtual eeprom memory
 * @param length Length of data to program into the virtual eeprom memory
 *
 * @retval 0 If successful.
 * @retval -EINVAL Invalid data size
 */
static inline __deprecated int eeprom_target_program(const struct device *dev,
						     const uint8_t *eeprom_data,
						     unsigned int length)
{
	return eeprom_target_write_data(dev, 0, eeprom_data, length);
}

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
static inline int eeprom_target_read(const struct device *dev, uint8_t *eeprom_data,
				     unsigned int offset)
{
	return eeprom_target_read_data(dev, offset, eeprom_data, 1);
}

/**
 * @brief Change the address of eeprom target at runtime
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param addr New address to assign to the eeprom target device
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
