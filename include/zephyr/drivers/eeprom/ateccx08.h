/*
 * Copyright (c) 2024 Vogl Electronic GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_EEPROM_ATECCX08_H_
#define ZEPHYR_INCLUDE_DRIVERS_EEPROM_ATECCX08_H_

#include <zephyr/drivers/eeprom.h>

#ifdef __cplusplus
extern "C" {
#endif

enum atecc_zone {
	ATECC_ZONE_CONFIG = 0x00U,
	ATECC_ZONE_OTP = 0x01U,
	ATECC_ZONE_DATA = 0x02U,
};

/**
 * @brief Set the zone for the EEPROM device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param zone Zone to set: ATECC_ZONE_CONFIG, ATECC_ZONE_OTP, or
 *             ATECC_ZONE_DATA.
 *
 * @retval 0 on success
 * @retval -EINVAL if zone is invalid
 */
int eeprom_ateccx08_set_zone(const struct device *dev, enum atecc_zone zone);

/**
 * @brief Set the slot for the EEPROM device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param slot Slot to set.
 *
 * @retval 0 on success
 * @retval -EINVAL if slot is invalid
 */
int eeprom_ateccx08_set_slot(const struct device *dev, uint8_t slot);

/**
 * @brief Calculates CRC over the given raw data and returns the CRC in
 *        little-endian byte order.
 *
 * @param length Size of data not including the CRC byte positions
 * @param data Pointer to the data over which to compute the CRC
 * @param crc_le Pointer to the place where the two-bytes of CRC will be
 *               returned in little-endian byte order.
 */
void atecc_crc(size_t length, const uint8_t *data, uint8_t *crc_le);

/**
 * @brief Use the Info command to get the device revision (DevRev).
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param revision Device revision is returned here (4 bytes).
 *
 * @return 0 on success, negative errno code on failure.
 */
int atecc_info(const struct device *dev, uint8_t *revision);

/**
 * @brief Unconditionally (no CRC required) lock a zone.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param zone Zone to lock. Option are ATECC_ZONE_CONFIG or ATECC_ZONE_DATA.
 *
 * @return 0 on success, negative errno code on failure.
 */
int atecc_lock_zone(const struct device *dev, enum atecc_zone zone);

/**
 * @brief Lock a zone with summary CRC.
 * For the config zone, the CRC is calculated over the entire config zone contents (128 bytes)
 * For the data zone (slots and OTP), the CRC is calculated over the entire data zone (slots and
 * OTP) Lock will fail if the provided CRC doesn't match the internally calculated one.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param zone Zone to lock. Option are ATECC_ZONE_CONFIG or ATECC_ZONE_DATA
 * @param summary_crc Expected CRC over the zone.
 *
 * @return 0 on success, negative errno code on failure.
 */
int atecc_lock_zone_crc(const struct device *dev, enum atecc_zone zone, uint16_t summary_crc);

/**
 * @brief Check the lock status of the configuration zone.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval true if the configuration zone is locked
 * @retval false if the configuration zone is not locked
 */
bool atecc_is_locked_config(const struct device *dev);

/**
 * @brief Check the lock status of the data zones.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval true if the data zones are locked
 * @retval false if the data zones are not locked
 */
bool atecc_is_locked_data(const struct device *dev);

/**
 * @brief Executes Random command, which generates a 32 byte random number
 *        from the device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param rand_out 32 bytes of random data is returned here.
 * @param update_seed If true, the seed will be updated.
 *
 * @return 0 on success, negative errno code on failure.
 */
int atecc_random(const struct device *dev, uint8_t *rand_out, bool update_seed);

/**
 * @brief Executes Read command, which reads the 9 byte serial number of the
 *        device from the config zone.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param serial_number  9 byte serial number is returned here.
 *
 * @return 0 on success, negative errno code on failure.
 */
int atecc_read_serial_number(const struct device *dev, uint8_t *serial_number);

/**
 * @brief Used to read an arbitrary number of bytes from any zone configured
 *        for clear reads.
 *
 * This function will issue the Read command as many times as is required to
 * read the requested data.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param zone Zone to read data from. Option are ATECC_ZONE_CONFIG,
 *             ATECC_ZONE_OTP, or ATECC_ZONE_DATA.
 * @param slot Slot number to read from if zone is ATECC_ZONE_DATA.
 *             Ignored for all other zones.
 * @param offset Byte offset within the zone to read from.
 * @param data Read data is returned here.
 * @param len Number of bytes to read starting from the offset.
 *
 * @return 0 on success, negative errno code on failure.
 */
int atecc_read_bytes(const struct device *dev, enum atecc_zone zone, uint8_t slot, uint16_t offset,
		     uint8_t *data, uint16_t len);

/**
 * @brief Executes the Write command, which writes data into the
 * configuration, otp, or data zones with a given byte offset and
 * length. Offset and length must be multiples of a word (4 bytes).
 *
 * Config zone must be unlocked for writes to that zone. If data zone is
 * unlocked, only 32-byte writes are allowed to slots and OTP and the offset
 * and length must be multiples of 32 or the write will fail.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param zone Zone to write data to: ATECC_ZONE_CONFIG,
 *             ATECC_ZONE_OTP, or ATECC_ZONE_DATA.
 * @param slot If zone is ATECC_ZONE_DATA, the slot number to
 *             write to. Ignored for all other zones.
 * @param offset_bytes Byte offset within the zone to write to. Must be
 *                     a multiple of a word (4 bytes).
 * @param data Data to be written.
 * @param len Number of bytes to be written. Must be a multiple
 *            of a word (4 bytes).
 *
 * @return 0 on success, negative errno code on failure.
 */
int atecc_write_bytes(const struct device *dev, enum atecc_zone zone, uint8_t slot,
		      uint16_t offset_bytes, const uint8_t *data, uint16_t len);

/**
 * @brief Executes the Write command, which writes the configuration zone.
 *
 * First 16 bytes are skipped as they are not writable. LockValue and
 * LockConfig are also skipped.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param config_data  Data to the config zone data. This should be 128 bytes.
 *
 * @return 0 on success, negative errno code on failure.
 */
int atecc_write_config(const struct device *dev, const uint8_t *config_data);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_EEPROM_ATECCX08_H_ */
