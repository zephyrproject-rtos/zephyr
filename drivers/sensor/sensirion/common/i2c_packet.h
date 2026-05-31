/*
 * Copyright (c) 2025 Sensirion
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef I2C_PACKET_H
#define I2C_PACKET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <zephyr/drivers/i2c.h>

/**
 * @brief Structure to define a Sensirion i2c packet.
 *
 * Sensirion sensors use 16-bit words on the I2C layer. Each word is usually
 * followed by a CRC8 checksum byte.
 * The i2c_packet structure is used to hold the data, as
 * well as the CRC8 definition and the maximum length of the data array.
 *
 * The maximum length of the array is used to prevent buffer overflows
 * when accessing the data buffer.
 */
typedef struct {

	/** CRC8 definition data */
	uint8_t crc8_poly; //< CRC8 polynomial for calculating the CRC8.
	uint8_t crc8_init; //< Initial value for the CRC8 calculation.

	/** Maximum length of data buffer */
	uint8_t max_length; //< Maximum length of the data array.

	/** Pointer to the data buffer */
	uint8_t *data; //< Pointer to the data array.
} i2c_packet;

/**
 * @brief Calculate the CRC8 for a word in the i2c_packet at position <index>.
 *
 * @param packet Pointer to the i2c_packet containing the data for which the
 *               CRC8 will be calculated.
 * @param index  Index of the word in the data array for which the CRC8 will
 *               be calculated.
 * @return uint8_t Calculated CRC8 value.
 */
uint8_t sensirion_i2c_packet_get_crc(const i2c_packet *packet, uint16_t index);

/**
 * @brief Check the CRC8 for a word in the i2c_packet at position <index>.
 *
 * @param packet Pointer to the i2c_packet containing the data for which the
 *               CRC8 will be checked.
 * @param index  Index of the word in the data array for which the CRC8 will
 *               be checked.
 * @return true  If the CRC8 is valid.
 * @return false If the CRC8 is invalid.
 */
bool sensirion_i2c_packet_check_crc(const i2c_packet *packet, uint16_t index);

/**
 * Add a command to the i2c_packet at the specified offset.
 *
 * Most sensirion Sensors use 16 bit commands. This function adds a 2 bytes
 * to the i2c_packet.
 *
 * @param packet  Pointer to i2c_packet in which the write frame will be prepared.
 *                Caller needs to make sure that there is enough space after
 *                offset left to write the data into the i2c_packet.
 * @param offset  Offset of the next free byte in the i2c_packet.
 * @param command Command to be written into the i2c_packet.
 *
 * @return Offset of next free byte in the i2c_packet after writing the data.
 */
uint16_t sensirion_i2c_packet_add_command16(i2c_packet *packet, uint16_t offset, uint16_t command);

/**
 * Add a command to the i2c_packet at the specified offset.
 *
 * Adds one byte command to the i2c_packet.
 * This is used for sensors that only take one command byte such as SHT.
 *
 * @param packet  Pointer to i2c_packet in which the write frame will be prepared.
 *                Caller needs to make sure that there is enough space after
 *                offset left to write the data into the i2c_packet.
 * @param offset  Offset of the next free byte in the i2c_packet.
 * @param command Command to be written into the i2c_packet.
 *
 * @return Offset of next free byte in the i2c_packet after writing the data.
 */
uint16_t sensirion_i2c_packet_add_command8(i2c_packet *packet, uint16_t offset, uint8_t command);

/**
 * Add a uint64_t to the i2c_packet at the specified offset.
 *
 * Adds 12 bytes to the i2c_packet.
 *
 * @param packet  Pointer to i2c_packet in which the write frame will be prepared.
 *                Caller needs to make sure that there is enough space after
 *                offset left to write the data into the i2c_packet.
 * @param offset  Offset of the next free byte in the i2c_packet.
 * @param data    uint64_t to be written into the i2c_packet.
 *
 * @return Offset of next free byte in the i2c_packet after writing the data.
 */
uint16_t sensirion_i2c_packet_add_uint64_t(i2c_packet *packet, uint16_t offset, uint64_t data);

/**
 * Add a uint32_t to the i2c_packet at the specified offset.
 *
 * Adds 6 bytes to the i2c_packet.
 *
 * @param packet  Pointer to i2c_packet in which the write frame will be prepared.
 *                Caller needs to make sure that there is enough space after
 *                offset left to write the data into the i2c_packet.
 * @param offset  Offset of the next free byte in the i2c_packet.
 * @param data    uint32_t to be written into the i2c_packet.
 *
 * @return Offset of next free byte in the i2c_packet after writing the data.
 */
uint16_t sensirion_i2c_packet_add_uint32_t(i2c_packet *packet, uint16_t offset, uint32_t data);

/**
 * Add a int32_t to the i2c_packet at the specified offset.
 *
 * Adds 6 bytes to the i2c_packet.
 *
 * @param packet  Pointer to packet in which the write frame will be prepared.
 *                Caller needs to make sure that there is enough space after
 *                offset left to write the data into the i2c_packet.
 * @param offset  Offset of the next free byte in the i2c_packet.
 * @param data    int32_t to be written into the i2c_packet.
 *
 * @return Offset of next free byte in the i2c_packet after writing the data.
 */
uint16_t sensirion_i2c_packet_add_int32_t(i2c_packet *packet, uint16_t offset, int32_t data);

/**
 * Add a uint16_t to the i2c_packet at the specified offset. Adds 3 bytes to the i2c_packet.
 *
 * @param packet  Pointer to packet in which the write frame will be prepared.
 *                Caller needs to make sure that there is enough space after
 *                offset left to write the data into the i2c_packet.
 * @param offset  Offset of the next free byte in the i2c_packet.
 * @param data    uint16_t to be written into the i2c_packet.
 *
 * @return Offset of next free byte in the i2c_packet after writing the data.
 */
uint16_t sensirion_i2c_packet_add_uint16_t(i2c_packet *packet, uint16_t offset, uint16_t data);

/**
 * Add a int16_t to the i2c_packet at the specified offset. Adds 3 bytes to the i2c_packet.
 *
 * @param packet  Pointer to packet in which the write frame will be prepared.
 *                Caller needs to make sure that there is enough space after
 *                offset left to write the data into the i2c_packet.
 * @param offset  Offset of the next free byte in the i2c_packet.
 * @param data    int16_t to be written into the i2c_packet.
 *
 * @return Offset of next free byte in the i2c_packet after writing the data.
 */
uint16_t sensirion_i2c_packet_add_int16_t(i2c_packet *packet, uint16_t offset, int16_t data);

/**
 * Add a float to the i2c_packet at the specified offset. Adds 6 bytes to the i2c_packet.
 *
 * @param packet  Pointer to packet in which the write frame will be prepared.
 *                Caller needs to make sure that there is enough space after
 *                offset left to write the data into the i2c_packet.
 * @param offset  Offset of the next free byte in the i2c_packet.
 * @param data    float to be written into the i2c_packet.
 *
 * @return Offset of next free byte in the i2c_packet after writing the data.
 */
uint16_t sensirion_i2c_packet_add_float(i2c_packet *packet, uint16_t offset, float data);

/**
 * Add a byte array to the i2c_packet at the specified offset.
 *
 * @param packet      Pointer to i2c_packet in which the write frame will be
 *                    prepared. Caller needs to make sure that there is
 *                    enough space after offset left to write the data
 *                    into the i2c_packet.
 * @param offset      Offset of the next free byte in the i2c_packet.
 * @param data        Pointer to data to be written into the i2c_packet.
 * @param data_length Number of bytes to be written into the i2c_packet. Needs to
 *                    be a multiple of SENSIRION_WORD_SIZE otherwise the
 *                    function returns BYTE_NUM_ERROR.
 *
 * @return            Offset of next free byte in the i2c_packet after writing the
 *                    data.
 */
uint16_t sensirion_i2c_packet_add_bytes(i2c_packet *packet, uint16_t offset, const uint8_t *data,
					uint16_t data_length);

/**
 * Writes the i2c_packet to the Sensor.
 *
 *
 * @param i2c_packet   Pointer to the i2c_packet containing the data to write.
 * @param data_length  Number of bytes to send to the Sensor.
 * @param i2c_spec     Sensor I2C specification
 *
 * @return        0 on success, error code otherwise
 */
int sensirion_i2c_packet_write(const i2c_packet const *packet, uint16_t data_length,
			       const struct i2c_dt_spec *i2c_spec);

/**
 * Reads data from the Sensor.
 *
 * @param i2c_packet           Pointer to i2c_packet to store data as bytes. Needs
 *                             to be big enough to store the data including
 *                             CRC. Twice the size of data should always
 *                             suffice.
 * @param expected_data_length Number of bytes to read (without CRC). Needs
 *                             to be a multiple of SENSIRION_WORD_SIZE,
 *                             otherwise the function returns BYTE_NUM_ERROR.
 * @param i2c_spec             Sensor I2C specification
 *
 * @return            	       0 on success, an error code otherwise
 */
int sensirion_i2c_packet_read(const i2c_packet *i2c_packet, uint16_t expected_data_length,
			      const struct i2c_dt_spec *i2c_spec);

#ifdef __cplusplus
}
#endif

#endif /* I2C_PACKET_H */
