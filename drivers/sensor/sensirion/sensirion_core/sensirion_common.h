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

#define NO_ERROR              0
#define NOT_IMPLEMENTED_ERROR ENOSYS

#define SENSIRION_COMMAND_SIZE     2
#define SENSIRION_WORD_SIZE        2
#define SENSIRION_NUM_WORDS(x)     (sizeof(x) / SENSIRION_WORD_SIZE)
#define SENSIRION_MAX_BUFFER_WORDS 32

#define sensirion_common_bytes_to_int16_t(src)  ((int16_t)sys_get_be16((src)))
#define sensirion_common_bytes_to_uint16_t(src) (sys_get_be16((src)))
#define sensirion_common_bytes_to_int32_t(src)  ((int32_t)sys_get_be32((src)))
#define sensirion_common_bytes_to_uint32_t(src) (sys_get_be32((src)))

#define sensirion_common_uint32_t_to_bytes(val, dst) (sys_put_be32((val), (dst)))
#define sensirion_common_int32_t_to_bytes(val, dst)  (sys_put_be32((uint32_t)(val), (dst)))
#define sensirion_common_uint16_t_to_bytes(val, dst) (sys_put_be16((val), (dst)))
#define sensirion_common_int16_t_to_bytes(val, dst)  (sys_put_be16((uint16_t)(val), (dst)))
#define sensirion_common_copy_bytes(src, dst, len)   (memcpy((dst), (src), (len)))

/**
 * Enum to describe the type of an integer
 */
typedef enum {
	BYTE = 1,
	SHORT = 2,
	INTEGER = 4,
	LONG_INTEGER = 8
} INT_TYPE;

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

/**
 * Convert an float to an array of bytes
 *
 * Convert an float value in system-endianness to big-endian/MBS-first
 * format to send to the sensor.
 *
 * @param value Value to convert
 * @param bytes An array of at least four bytes
 */
void sensirion_common_float_to_bytes(const float value, uint8_t *bytes);

/**
 *  Copy bytes from byte array to integer.
 *
 * @param source      Array of bytes to be copied.
 * @param int_value Pointer to integer of bytes to be copied to.
 * @param int_type Type (size) of the integer to be copied.
 * @param data_length Number of bytes to copy.
 */
void sensirion_common_to_integer(const uint8_t *source, uint8_t *destination, INT_TYPE int_type,
				 uint8_t data_length);

#ifdef __cplusplus
}
#endif

#endif /* SENSIRION_COMMON_H */
