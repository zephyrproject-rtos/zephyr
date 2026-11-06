/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup pmbus
 * @brief PMBus (Power Management Bus) core API.
 */

#ifndef ZEPHYR_INCLUDE_PMBUS_PMBUS_H_
#define ZEPHYR_INCLUDE_PMBUS_PMBUS_H_

/**
 * @brief PMBus core used by drivers of PMBus devices.
 * @defgroup pmbus PMBus
 * @since 4.6
 * @version 0.1.0
 * @ingroup connectivity
 * @{
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/drivers/smbus.h>
#include <zephyr/kernel.h>
#include <zephyr/pmbus/pmbus_cmd.h>
#include <zephyr/pmbus/pmbus_format.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Runtime state of a PMBus device, kept in the driver data */
struct pmbus_dev {
	/** SMBus bus and address */
	const struct smbus_dt_spec *bus;
	/** Serializes page selection with the command that follows it */
	struct k_mutex lock;
	/** Number of pages */
	uint8_t pages;
	/** Cached PAGE, -1 when unknown */
	int16_t page;
	/** CAPABILITY value, 0 when the device does not answer it */
	uint8_t capability;
	/** VOUT_MODE of each page, -1 when the device does not answer it */
	int16_t vout_mode[CONFIG_PMBUS_MAX_PAGES];
};

/**
 * @brief Initialize a PMBus device.
 *
 * Logs a warning with the STATUS_WORD of each page that reports a fault,
 * then reads CAPABILITY and the VOUT_MODE of each page. A device that does
 * not answer these commands is accepted; CLEAR_FAULTS is then sent to drop
 * the communication fault this raised in the device.
 *
 * Packet Error Checking is a setting of the SMBus controller, see
 * smbus_configure(). It must only be enabled when every device on the bus
 * supports it.
 *
 * @param pmbus PMBus device state to initialize.
 * @param bus SMBus bus and address of the device.
 * @param pages Number of pages of the device, 1 if it has no PAGE command.
 *
 * @retval 0 on success.
 * @retval -ENODEV if the SMBus controller is not ready.
 * @retval -EINVAL if @p pages is 0 or above CONFIG_PMBUS_MAX_PAGES.
 * @retval -ENOTSUP if the device uses the IEEE half precision format, or
 *         the bus uses PEC and the device does not support it.
 * @retval <0 negative errno from the bus.
 */
int pmbus_init(struct pmbus_dev *pmbus, const struct smbus_dt_spec *bus, uint8_t pages);

/**
 * @name Command access
 *
 * Each call selects @p page with the PAGE command when the device has more
 * than one page and the page differs from the cached one, then runs the
 * command. @ref PMBUS_PAGE_ALL is accepted only for writes.
 * @{
 */

/**
 * @brief Read Byte command.
 *
 * @param pmbus PMBus device.
 * @param page Page to select.
 * @param cmd Command code.
 * @param val Byte read.
 *
 * @retval 0 on success.
 * @retval -EINVAL for an invalid page.
 * @retval <0 negative errno from the bus.
 */
int pmbus_read_byte(struct pmbus_dev *pmbus, uint8_t page, uint8_t cmd, uint8_t *val);

/**
 * @brief Read Word command.
 *
 * @param pmbus PMBus device.
 * @param page Page to select.
 * @param cmd Command code.
 * @param val Word read.
 *
 * @retval 0 on success.
 * @retval -EINVAL for an invalid page.
 * @retval <0 negative errno from the bus.
 */
int pmbus_read_word(struct pmbus_dev *pmbus, uint8_t page, uint8_t cmd, uint16_t *val);

/**
 * @brief Send Byte command.
 *
 * @param pmbus PMBus device.
 * @param page Page to select, or @ref PMBUS_PAGE_ALL.
 * @param cmd Command code.
 *
 * @retval 0 on success.
 * @retval -EINVAL for an invalid page.
 * @retval <0 negative errno from the bus.
 */
int pmbus_send_byte(struct pmbus_dev *pmbus, uint8_t page, uint8_t cmd);

/**
 * @brief Write Byte command.
 *
 * @param pmbus PMBus device.
 * @param page Page to select, or @ref PMBUS_PAGE_ALL.
 * @param cmd Command code.
 * @param val Byte to write.
 *
 * @retval 0 on success.
 * @retval -EINVAL for an invalid page.
 * @retval <0 negative errno from the bus.
 */
int pmbus_write_byte(struct pmbus_dev *pmbus, uint8_t page, uint8_t cmd, uint8_t val);

/**
 * @brief Write Word command.
 *
 * @param pmbus PMBus device.
 * @param page Page to select, or @ref PMBUS_PAGE_ALL.
 * @param cmd Command code.
 * @param val Word to write.
 *
 * @retval 0 on success.
 * @retval -EINVAL for an invalid page.
 * @retval <0 negative errno from the bus.
 */
int pmbus_write_word(struct pmbus_dev *pmbus, uint8_t page, uint8_t cmd, uint16_t val);

/** @} */

/**
 * @brief Check if the device supports a read command.
 *
 * Reads the command and checks STATUS_CML for an invalid command, as a
 * device reports an unsupported command there when it cannot NACK it. The
 * fault is then cleared with CLEAR_FAULTS, which also clears any other
 * latched fault of the device, so it is meant to be used at initialization.
 *
 * @param pmbus PMBus device.
 * @param page Page to select.
 * @param cmd Command code.
 * @param len Data length of the command, 1 or 2 bytes.
 *
 * @retval true if the command is supported.
 * @retval false otherwise.
 */
bool pmbus_cmd_supported(struct pmbus_dev *pmbus, uint8_t page, uint8_t cmd, size_t len);

/**
 * @brief Read a telemetry value.
 *
 * READ_VOUT is decoded with the VOUT_MODE of the page, which must be the
 * ULINEAR16 mode. The other commands are decoded as LINEAR11.
 *
 * Supported commands: READ_VIN, READ_IIN, READ_VOUT, READ_IOUT,
 * READ_TEMPERATURE_1, READ_TEMPERATURE_2, READ_TEMPERATURE_3, READ_POUT and
 * READ_PIN.
 *
 * @param pmbus PMBus device.
 * @param page Page to select.
 * @param cmd READ command code.
 * @param val Value in milli units: mV, mA, mW or milli degree Celsius.
 *
 * @retval 0 on success.
 * @retval -EINVAL for an invalid page.
 * @retval -ENOTSUP for another command, or for READ_VOUT when the device
 *         has no VOUT_MODE or uses a mode other than ULINEAR16.
 * @retval -ERANGE if the value does not fit in 32 bits.
 * @retval <0 negative errno from the bus.
 */
int pmbus_read_value(struct pmbus_dev *pmbus, uint8_t page, uint8_t cmd, int32_t *val);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_PMBUS_PMBUS_H_ */
