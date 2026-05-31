/*
 * Copyright (c) 2025 Sensirion
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SENSIRION_COMMON_H
#define SENSIRION_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <zephyr/sys/byteorder.h>

/**
 * NO_ERROR is used as alias for 0 return values of functions to improve
 * readability of the code.
 */
#define NO_ERROR              0

/**
 * @defgroup API to unmarshal basic data type from byte arrays
 * 
 * This API provides functions for converting byte arrays received from the
 * sensor.
 * The function names in this API use the pattern bytes_to_<type> to indicate
 * conversions from byte arrays to basic data types.
 * Since Zephyr does not provide a function for converting big-endian
 * (MSB-first) byte arrays to floating-point values, the function
 * sensirion_common_bytes_to_float() is
 * provided for this purpose.
 * To maintain a consistent naming scheme for all functions that convert byte
 * arrays to basic types, the functions
 * sensirion_common_bytes_to_<integer>() are also provided,
 * even in cases where Zephyr already provides equivalent functions for
 * specific integer
 * types.
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
float sensirion_common_bytes_to_float(const uint8_t *bytes);


#define sensirion_common_bytes_to_int16_t(bytes) ((int16_t)sys_get_be16(bytes))
#define sensirion_common_bytes_to_uint16_t(bytes) ((uint16_t)sys_get_be16(bytes))
#define sensirion_common_bytes_to_int32_t(bytes) ((int32_t)sys_get_be32(bytes))
#define sensirion_common_bytes_to_uint32_t(bytes) ((uint32_t)sys_get_be32(bytes))
#define sensirion_common_bytes_to_int64_t(bytes) ((int64_t)sys_get_be64(bytes))
#define sensirion_common_bytes_to_uint64_t(bytes) ((uint64_t)sys_get_be64(bytes))

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
void sensirion_common_to_integer(const uint8_t *source, uint8_t *destination, 
				 size_t destination_size,
				 uint8_t data_length);



#ifdef __cplusplus
}
#endif

#endif /* SENSIRION_COMMON_H */
