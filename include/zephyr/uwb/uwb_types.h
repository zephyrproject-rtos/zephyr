/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief UWB Helper macros
 *
 * This file contains definitions of helper streaming
 * macros to easily copy data to/from buffer from/to
 * integer fields
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_UWB_TYPES_H_
#define ZEPHYR_INCLUDE_DRIVERS_UWB_TYPES_H_

#include <stdint.h>
#include <zephyr/sys/byteorder.h>

/**
 * @brief UWB helper macros to stream data
 * @addtogroup uwb_types
 * @{
 */

/**
 * Macros to get and put bytes to and from a stream (Little Endian format).
 */

/**
 * Copy uint64_t to buffer stream from index
 */
#define UWB_UINT64_TO_STREAM(p, u64, index)                                                        \
	sys_put_le64((u64), &p[(index)]);                                                          \
	(index) = (index) + sizeof(uint64_t)

/**
 * Copy uint32_t to buffer stream from index
 */
#define UWB_UINT32_TO_STREAM(p, u32, index)                                                        \
	sys_put_le32((u32), &p[(index)]);                                                          \
	(index) = (index) + sizeof(uint32_t)

/**
 * Copy uint16_t to buffer stream from index
 */
#define UWB_UINT16_TO_STREAM(p, u16, index)                                                        \
	sys_put_le16((u16), &p[(index)]);                                                          \
	(index) = (index) + sizeof(uint16_t)

/**
 * Copy uint8_t to buffer stream from index
 */
#define UWB_UINT8_TO_STREAM(p, u8, index)                                                          \
	{                                                                                          \
		(p)[(index)] = (uint8_t)(u8);                                                      \
		(index) = ((index) + (sizeof(uint8_t)));                                           \
	}

/**
 * Copy int8_t to buffer stream from index
 */
#define UWB_INT8_TO_STREAM(p, i8, index)                                                           \
	{                                                                                          \
		(p)[(index)] = (int8_t)(i8);                                                       \
		(index) = ((index) + (sizeof(int8_t)));                                            \
	}

/**
 * Copy array of length len to buffer stream from index
 */
#define UWB_ARRAY_TO_STREAM(p, a, len, index)                                                      \
	memcpy(((p) + (index)), (a), (len));                                                       \
	(index) = (index) + (len)

/**
 * Copy uint8_t from buffer stream at index
 */
#define UWB_STREAM_TO_UINT8(u8, p, index)                                                          \
	{                                                                                          \
		u8 = (uint8_t)((p)[(index)]);                                                      \
		(index) = ((index) + (sizeof(uint8_t)));                                           \
	}

/**
 * Copy int8_t from buffer stream at index
 */
#define UWB_STREAM_TO_INT8(i8, p, index)                                                           \
	{                                                                                          \
		i8 = (int8_t)((p)[(index)]);                                                       \
		(index) = ((index) + (sizeof(int8_t)));                                            \
	}

/**
 * Copy uint16_t from buffer stream at index
 */
#define UWB_STREAM_TO_UINT16(u16, p, index)                                                        \
	u16 = sys_get_le16(&p[(index)]);                                                           \
	(index) = (index) + sizeof(uint16_t);

/**
 * Copy int16_t from buffer stream at index
 */
#define UWB_STREAM_TO_INT16(i16, p, index)                                                         \
	i16 = (int16_t)sys_get_le16(&p[(index)]);                                                  \
	(index) = (index) + sizeof(uint16_t);

/**
 * Copy uint32_t from buffer stream at index
 */
#define UWB_STREAM_TO_UINT32(u32, p, index)                                                        \
	u32 = sys_get_le32(&p[(index)]);                                                           \
	(index) = (index) + sizeof(uint32_t);

/**
 * Copy uint64_t from buffer stream at index
 */
#define UWB_STREAM_TO_UINT64(u64, p, index)                                                        \
	u64 = sys_get_le64(&p[(index)]);                                                           \
	(index) = (index) + sizeof(uint64_t);

/**
 * Copy array of length len from buffer stream at index
 */
#define UWB_STREAM_TO_ARRAY(a, p, len, index)                                                      \
	memcpy((a), (p) + (index), (len));                                                         \
	(index) = (index) + (len);

/**
 * Macros to get and put bytes to and from a stream (Big Endian format)
 */
/**
 * Copy uint32_t to buffer stream from index in BE format
 */
#define UWB_UINT32_TO_BE_STREAM(p, u32, index)                                                     \
	sys_put_be32((u32), &p[(index)]);                                                          \
	(index) = (index) + sizeof(uint32_t);

/**
 * Copy uint16_t to buffer stream from index in BE format
 */
#define UWB_UINT16_TO_BE_STREAM(p, u16, index)                                                     \
	sys_put_be16((u16), &p[(index)]);                                                          \
	(index) = (index) + sizeof(uint16_t);

/**
 * Copy uint8_t to buffer stream from index in BE format
 */
#define UWB_UINT8_TO_BE_STREAM(p, u8, index)                                                       \
	{                                                                                          \
		(p)[(index)] = (uint8_t)(u8);                                                      \
		(index) = ((index) + (sizeof(uint8_t)));                                           \
	}

/**
 * Copy uint16_t from buffer stream from index in BE format
 */
#define UWB_BE_STREAM_TO_UINT16(u16, p, index)                                                     \
	(u16) = sys_get_be16(&p[(index)]);                                                         \
	index = index + sizeof(uint16_t);

/**
 * Copy uint32_t from buffer stream from index in BE format
 */
#define UWB_BE_STREAM_TO_UINT32(u32, p, index)                                                     \
	(u32) = sys_get_be32(&p[(index)]);                                                         \
	index = index + sizeof(uint32_t);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_UWB_TYPES_H_ */
