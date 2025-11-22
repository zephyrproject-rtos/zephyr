/*
 * Copyright (c) 2025 Sensirion
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SENSIRION_I2C_H
#define SENSIRION_I2C_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <zephyr/drivers/i2c.h>

#define SENSIRION_CRC8_POLY 0x31
#define SENSIRION_CRC8_INIT 0xFF
#define SENSIRION_CRC8_LEN  1

#define SENSIRION_COMMAND_SIZE     2
#define SENSIRION_WORD_SIZE        2
#define SENSIRION_NUM_WORDS(x)     (sizeof(x) / SENSIRION_WORD_SIZE)
#define SENSIRION_MAX_BUFFER_WORDS 32

/**
 * Compute the CRC checksum for I2C data.
 *
 * This function calculates the CRC8 checksum over a sequence of bytes
 * with a zephyr helper function.
 *
 * @param data:   Pointer to the input data buffer to compute the CRC for.
 * @param count:  Number of bytes in the data buffer.
 *
 * @return  The computed 8-bit CRC checksum.
 */
uint8_t sensirion_i2c_generate_crc(const uint8_t *data, uint16_t count);

/**
 * Verify the CRC checksum of received I2C data.
 *
 * This function calculates the CRC8 of the provided data buffer and compares
 * it against the expected checksum byte. It validates that the received data
 * has not been corrupted during transfer.
 *
 * @param data:       Pointer to the data buffer containing the bytes to check.
 * @param count:      Number of bytes in the data buffer (excluding checksum).
 * @param checksum:   The expected CRC checksum byte received from the sensor.
 *
 * @return  0 if the checksum is valid, a negative error code otherwise.
 */
int8_t sensirion_i2c_check_crc(const uint8_t *data, uint16_t count, uint8_t checksum);

/**
 * Send a general call reset.
 *
 * @warning This will reset all attached I2C devices on the bus which support
 *          general call reset.
 *
 * @param i2c_spec  Sensor I2C specification
 *
 * @return  0 on success, an error code otherwise
 */
int sensirion_i2c_general_call_reset(const struct i2c_dt_spec *i2c_spec);

/**
 * Create the i2c send buffer for a command and a set of argument words.
 * The output buffer interleaves argument words with their checksums.
 *
 * @param buf:        The generated buffer to send over i2c. Then buffer length
 * must be at least SENSIRION_COMMAND_LEN + num_args * (SENSIRION_WORD_SIZE +
 * CRC8_LEN).
 * @param cmd:        The i2c command to send. It already includes a checksum.
 * @param args:       The arguments to the command. Can be NULL if none.
 * @param num_args:   The number of word arguments in args.
 *
 * @return      The number of bytes written to buf
 */
uint16_t sensirion_i2c_fill_cmd_send_buf(uint8_t *buf, uint16_t cmd, const uint16_t *args,
					 uint8_t num_args);

/**
 * Read data words from sensor
 *
 * @param i2c_spec:    Sensor I2C specification
 * @param data_words: Allocated buffer to store the read words.
 *              The buffer may also have been modified in case of an error.
 * @param num_words:  Number of data words to read (without CRC bytes)
 *
 * @return      0 on success, an error code otherwise
 */
int sensirion_i2c_read_words(const struct i2c_dt_spec *i2c_spec, uint16_t *data_words,
			     uint16_t num_words);

/**
 * Rread data words as byte-stream from sensor
 *
 * Read bytes without adjusting values to the uP's word-order.
 *
 * @param i2c_spec:   Sensor I2C specification
 * @param data:       Allocated buffer to store the read bytes.
 *              The buffer may also have been modified in case of an error.
 * @param num_words:  Number of data words(!) to read (without CRC bytes)
 *              Since only word-chunks can be read from the sensor the size
 *              is still specified in sensor-words (num_words = num_bytes *
 *              SENSIRION_WORD_SIZE)
 *
 * @return      0 on success, an error code otherwise
 */
int sensirion_i2c_read_words_as_bytes(const struct i2c_dt_spec *i2c_spec, uint8_t *data,
				      uint16_t num_words);

/**
 * Writes a command to the sensor
 * @param i2c_spec:    Sensor I2C specification
 * @param command:    Sensor command
 *
 * @return      0 on success, an error code otherwise
 */
int sensirion_i2c_write_cmd(const struct i2c_dt_spec *i2c_spec, uint16_t command);

/**
 * Writes a command with arguments to the sensor
 *
 * @param i2c_spec:    Sensor I2C specification
 * @param command:    Sensor command
 * @param data:       Argument buffer with words to send
 * @param num_words:  Number of data words to send (without CRC bytes)
 *
 * @return      0 on success, an error code otherwise
 */
int sensirion_i2c_write_cmd_with_args(const struct i2c_dt_spec *i2c_spec, uint16_t command,
				      const uint16_t *data_words, uint16_t num_words);

/**
 * Send a command, wait for the sensor to process and read data back
 *
 * @param i2c_spec:    Sensor I2C specification
 * @param cmd:        Command
 * @param delay:      Time in microseconds to delay sending the read request
 * @param data_words: Allocated buffer to store the read data
 * @param num_words:  Data words to read (without CRC bytes)
 *
 * @return      0 on success, an error code otherwise
 */
int sensirion_i2c_delayed_read_cmd(const struct i2c_dt_spec *i2c_spec, uint16_t cmd,
				   uint32_t delay_us, uint16_t *data_words, uint16_t num_words);
/**
 * Reads data words from the sensor after a command
 *                            is issued
 * @param i2c_spec:    Sensor I2C specification
 * @param cmd:        Command
 * @param data_words: Allocated buffer to store the read data
 * @param num_words:  Data words to read (without CRC bytes)
 *
 * @return      0 on success, an error code otherwise
 */
int sensirion_i2c_read_cmd(const struct i2c_dt_spec *i2c_spec, uint16_t cmd, uint16_t *data_words,
			   uint16_t num_words);

/**
 * Add a command to the buffer at offset. Adds 2 bytes to the buffer.
 *
 * @param buffer  Pointer to buffer in which the write frame will be prepared.
 *                Caller needs to make sure that there is enough space after
 *                offset left to write the data into the buffer.
 * @param offset  Offset of the next free byte in the buffer.
 * @param command Command to be written into the buffer.
 *
 * @return        Offset of next free byte in the buffer after writing the data.
 */
uint16_t sensirion_i2c_add_command_to_buffer(uint8_t *buffer, uint16_t offset, uint16_t command);

/**
 * Add a command to the buffer at the specified offset. This function is equivalent to the
 * function sensirion_i2c_add_command_to_buffer().
 *
 * @param buffer  Pointer to buffer in which the write frame will be prepared.
 *                Caller needs to make sure that there is enough space after
 *                offset left to write the data into the buffer.
 * @param offset  Offset of the next free byte in the buffer.
 * @param command Command to be written into the buffer.
 *
 * @return        Offset of next free byte in the buffer after writing the data.
 */
uint16_t sensirion_i2c_add_command16_to_buffer(uint8_t *buffer, uint16_t offset, uint16_t command);

/**
 * Add a command to the buffer at offset. Adds one bytes command to the buffer.
 * This is used for sensor that only take one command byte such as SHT.
 *
 * @param buffer  Pointer to buffer in which the write frame will be prepared.
 *                Caller needs to make sure that there is enough space after
 *                offset left to write the data into the buffer.
 * @param offset  Offset of the next free byte in the buffer.
 * @param command Command to be written into the buffer.
 *
 * @return        Offset of next free byte in the buffer after writing the data.
 */
uint16_t sensirion_i2c_add_command8_to_buffer(uint8_t *buffer, uint16_t offset, uint8_t command);

/**
 * Add a uint32_t to the buffer at offset. Adds 6 bytes to the buffer.
 *
 * @param buffer  Pointer to buffer in which the write frame will be prepared.
 *                Caller needs to make sure that there is enough space after
 *                offset left to write the data into the buffer.
 * @param offset  Offset of the next free byte in the buffer.
 * @param data    uint32_t to be written into the buffer.
 *
 * @return        Offset of next free byte in the buffer after writing the data.
 */
uint16_t sensirion_i2c_add_uint32_t_to_buffer(uint8_t *buffer, uint16_t offset, uint32_t data);

/**
 * Add a int32_t to the buffer at offset. Adds 6 bytes to the buffer.
 *
 * @param buffer  Pointer to buffer in which the write frame will be prepared.
 *                Caller needs to make sure that there is enough space after
 *                offset left to write the data into the buffer.
 * @param offset  Offset of the next free byte in the buffer.
 * @param data    int32_t to be written into the buffer.
 *
 * @return        Offset of next free byte in the buffer after writing the data.
 */
uint16_t sensirion_i2c_add_int32_t_to_buffer(uint8_t *buffer, uint16_t offset, int32_t data);

/**
 * Add a uint16_t to the buffer at offset. Adds 3 bytes to the buffer.
 *
 * @param buffer  Pointer to buffer in which the write frame will be prepared.
 *                Caller needs to make sure that there is enough space after
 *                offset left to write the data into the buffer.
 * @param offset  Offset of the next free byte in the buffer.
 * @param data    uint16_t to be written into the buffer.
 *
 * @return        Offset of next free byte in the buffer after writing the data.
 */
uint16_t sensirion_i2c_add_uint16_t_to_buffer(uint8_t *buffer, uint16_t offset, uint16_t data);

/**
 * Add a int16_t to the buffer at offset. Adds 3 bytes to the buffer.
 *
 * @param buffer  Pointer to buffer in which the write frame will be prepared.
 *                Caller needs to make sure that there is enough space after
 *                offset left to write the data into the buffer.
 * @param offset  Offset of the next free byte in the buffer.
 * @param data    int16_t to be written into the buffer.
 *
 * @return        Offset of next free byte in the buffer after writing the data.
 */
uint16_t sensirion_i2c_add_int16_t_to_buffer(uint8_t *buffer, uint16_t offset, int16_t data);

/**
 * Add a float to the buffer at offset. Adds 6 bytes to the buffer.
 *
 * @param buffer  Pointer to buffer in which the write frame will be prepared.
 *                Caller needs to make sure that there is enough space after
 *                offset left to write the data into the buffer.
 * @param offset  Offset of the next free byte in the buffer.
 * @param data    float to be written into the buffer.
 *
 * @return        Offset of next free byte in the buffer after writing the data.
 */
uint16_t sensirion_i2c_add_float_to_buffer(uint8_t *buffer, uint16_t offset, float data);

/**
 * Add a byte array to the buffer at offset.
 *
 * @param buffer      Pointer to buffer in which the write frame will be
 *                    prepared. Caller needs to make sure that there is
 *                    enough space after offset left to write the data
 *                    into the buffer.
 * @param offset      Offset of the next free byte in the buffer.
 * @param data        Pointer to data to be written into the buffer.
 * @param data_length Number of bytes to be written into the buffer. Needs to
 *                    be a multiple of SENSIRION_WORD_SIZE otherwise the
 *                    function returns BYTE_NUM_ERROR.
 *
 * @return            Offset of next free byte in the buffer after writing the
 *                    data.
 */
uint16_t sensirion_i2c_add_bytes_to_buffer(uint8_t *buffer, uint16_t offset, const uint8_t *data,
					   uint16_t data_length);

/**
 * Writes data to the Sensor.
 *
 * @note This is just a wrapper for sensirion_i2c_hal_write() to
 *       not need to include the HAL in the drivers.
 *
 * @param i2c_spec     Sensor I2C specification
 * @param data        Pointer to the buffer containing the data to write.
 * @param data_length Number of bytes to send to the Sensor.
 *
 * @return        0 on success, error code otherwise
 */
int sensirion_i2c_write_data(const struct i2c_dt_spec *i2c_spec, const uint8_t *data,
			     uint16_t data_length);

/**
 * Reads data from the Sensor.
 *
 * @param i2c_spec             Sensor I2C specification
 * @param buffer               Allocated buffer to store data as bytes. Needs
 *                             to be big enough to store the data including
 *                             CRC. Twice the size of data should always
 *                             suffice.
 * @param expected_data_length Number of bytes to read (without CRC). Needs
 *                             to be a multiple of SENSIRION_WORD_SIZE,
 *                             otherwise the function returns BYTE_NUM_ERROR.
 *
 * @return            0 on success, an error code otherwise
 */
int sensirion_i2c_read_data_inplace(const struct i2c_dt_spec *i2c_spec, uint8_t *buffer,
				    uint16_t expected_data_length);

#ifdef __cplusplus
}
#endif

#endif /* SENSIRION_I2C_H */
