/*
 * Copyright (c) 2026 Sensirion
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CONVERSIONS_H
#define CONVERSIONS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <zephyr/sys/byteorder.h>

/**
 * NO_ERROR is used as alias for 0 return values of functions to improve
 * readability of the code.
 */
#define NO_ERROR 0

/**
 * @defgroup API to deserialize basic data types from a byte array
 *
 * Sensirion drivers follow a very strict design pattern wich is OS and
 * platform independent. As part of this pattern, the drivers
 * require a common API for converting byte arrays to basic data types.
 *
 * The following functions provide a Zephyr specific implementation of
 * this API.
 *
 * The function names in this API use the pattern bytes_to_<type> to
 * indicate conversions from byte arrays to basic data types.
 * @{
 */

/**
 * Convert an array of bytes to a float
 *
 * Convert an array of bytes received from the sensor in big-endian/MSB-first
 * format to an float value in the correct system-endianness.
 *
 * @param bytes An array of at least four bytes (MSB first)
 * @return      The byte array represented as float
 */
float sensirion_conversions_bytes_to_float(const uint8_t *const bytes);

/**
 * Convert an array of bytes to a 16-bit signed integer.
 *
 * @param bytes
 * @return int16_t
 */
static inline int16_t sensirion_conversions_bytes_to_int16_t(const uint8_t bytes[2])
{
	return (int16_t)sys_get_be16(bytes);
}

/**
 * Convert an array of bytes to a 16-bit unsigned integer.
 *
 * @param bytes
 * @return uint16_t
 */
static inline uint16_t sensirion_conversions_bytes_to_uint16_t(const uint8_t bytes[2])
{
	return sys_get_be16(bytes);
}

/**
 * Convert an array of bytes to a 32-bit signed integer.
 *
 * @param bytes
 * @return int32_t
 */
static inline int32_t sensirion_conversions_bytes_to_int32_t(const uint8_t bytes[4])
{
	return (int32_t)sys_get_be32(bytes);
}

/**
 * Convert an array of bytes to a 32-bit unsigned integer.
 *
 * @param bytes
 * @return uint32_t
 */
static inline uint32_t sensirion_conversions_bytes_to_uint32_t(const uint8_t bytes[4])
{
	return sys_get_be32(bytes);
}

/**
 * Convert an array of bytes to a 64-bit signed integer.
 *
 * @param bytes
 * @return int64_t
 */
static inline int64_t sensirion_conversions_bytes_to_int64_t(const uint8_t bytes[8])
{
	return (int64_t)sys_get_be64(bytes);
}

/**
 * @brief Convert an array of bytes to a 64-bit unsigned integer.
 *
 * @param bytes
 * @return uint64_t
 */
static inline uint64_t sensirion_conversions_bytes_to_uint64_t(const uint8_t bytes[8])
{
	return sys_get_be64(bytes);
}

/** @} */

/**
 *  Copy bytes from byte array to an integer of the specified type.
 *
 * The byte array is expected to be in big-endian/MSB-first format as it is
 * received from the sensor.
 * The data_length specifies how many bytes are available in the source byte
 * array to copy.
 * This length may not match the size of the destination integer.
 * In this case the function will fill the missing bytes with zeros or
 * set the destination to 0 if it is smaller than the source.
 *
 * @param source      Array of bytes to be copied.
 * @param destination Pointer to integer that will be filled with the copied
 *                    data.
 * @param destination_size Size of the destination integer in bytes.
 * @param data_length Number of available bytes in source to copy.
 */
void sensirion_conversions_to_integer(const uint8_t *const source, uint8_t *const destination,
				      size_t destination_size, uint8_t data_length);

#ifdef __cplusplus
}
#endif

#endif /* CONVERSIONS_H */
