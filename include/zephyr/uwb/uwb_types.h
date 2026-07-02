/*
 *
 *  Copyright (C) 1999-2014 Broadcom Corporation
 *  Copyright 2018-2020,2022-2024 NXP
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */
#ifndef UWB_TYPES_H
#define UWB_TYPES_H

#include <stdint.h>

/*
 *  @brief Example usage of Bitfields
 */

#define UWB_BIT_0 (1 << 0)
#define UWB_BIT_1 (1 << 1)
#define UWB_BIT_2 (1 << 2)
#define UWB_BIT_3 (1 << 3)
#define UWB_BIT_4 (1 << 4)
#define UWB_BIT_5 (1 << 5)
#define UWB_BIT_6 (1 << 6)
#define UWB_BIT_7 (1 << 7)

/**
 * Macros to get and put bytes to and from a stream (Little Endian format).
 */
#define UWB_UINT64_TO_STREAM(p, u64, index)                                                        \
	{                                                                                          \
		(p)[(index) + 0U] = (uint8_t)((u64) >> 0U);                                        \
		(p)[(index) + 1U] = (uint8_t)((u64) >> 8U);                                        \
		(p)[(index) + 2U] = (uint8_t)((u64) >> 16U);                                       \
		(p)[(index) + 3U] = (uint8_t)((u64) >> 24U);                                       \
		(p)[(index) + 4U] = (uint8_t)((u64) >> 32U);                                       \
		(p)[(index) + 5U] = (uint8_t)((u64) >> 40U);                                       \
		(p)[(index) + 6U] = (uint8_t)((u64) >> 48U);                                       \
		(p)[(index) + 7U] = (uint8_t)((u64) >> 56U);                                       \
		(index) = ((index) + sizeof(uint64_t));                                            \
	}
#define UWB_UINT32_TO_STREAM(p, u32, index)                                                        \
	{                                                                                          \
		(p)[(index) + 0U] = (uint8_t)((u32) >> 0U);                                        \
		(p)[(index) + 1U] = (uint8_t)((u32) >> 8U);                                        \
		(p)[(index) + 2U] = (uint8_t)((u32) >> 16U);                                       \
		(p)[(index) + 3U] = (uint8_t)((u32) >> 24U);                                       \
		(index) = ((index) + sizeof(uint32_t));                                            \
	}
#define UWB_UINT16_TO_STREAM(p, u16, index)                                                        \
	{                                                                                          \
		(p)[(index) + 0U] = (uint8_t)((u16) >> 0U);                                        \
		(p)[(index) + 1U] = (uint8_t)((u16) >> 8U);                                        \
		(index) = ((index) + (sizeof(uint16_t)));                                          \
	}
#define UWB_UINT8_TO_STREAM(p, u8, index)                                                          \
	{                                                                                          \
		(p)[(index)] = (uint8_t)(u8);                                                      \
		(index) = ((index) + (sizeof(uint8_t)));                                           \
	}
#define UWB_INT8_TO_STREAM(p, i8, index)                                                           \
	{                                                                                          \
		(p)[(index)] = (int8_t)(i8);                                                       \
		(index) = ((index) + (sizeof(int8_t)));                                            \
	}
#define UWB_ARRAY_TO_STREAM(p, a, len, index)                                                      \
	{                                                                                          \
		uint32_t ijk;                                                                      \
		for (ijk = 0; ijk < (uint32_t)len; ijk++) {                                        \
			(p)[(index)] = (uint8_t)a[ijk];                                            \
			(index) = ((index) + (sizeof(uint8_t)));                                   \
		}                                                                                  \
	}
#define UWB_STREAM_TO_UINT8(u8, p, index)                                                          \
	{                                                                                          \
		u8 = (uint8_t)((p)[(index)]);                                                      \
		(index) = ((index) + (sizeof(uint8_t)));                                           \
	}
#define UWB_STREAM_TO_INT8(i8, p, index)                                                           \
	{                                                                                          \
		i8 = (int8_t)((p)[(index)]);                                                       \
		(index) = ((index) + (sizeof(int8_t)));                                            \
	}
#define UWB_STREAM_TO_UINT16(u16, p, index)                                                        \
	{                                                                                          \
		u16 = (uint16_t)(((uint16_t)((p)[(index) + 0U]) << 0U) |                           \
				 ((uint16_t)((p)[(index) + 1U]) << 8U));                           \
		(index) = ((index) + (sizeof(uint16_t)));                                          \
	}
#define UWB_STREAM_TO_INT16(i16, p, index)                                                         \
	{                                                                                          \
		i16 = (int16_t)(((int16_t)((p)[(index) + 0U]) << 0U) |                             \
				((int16_t)((p)[(index) + 1U]) << 8U));                             \
		(index) = ((index) + (sizeof(int16_t)));                                           \
	}
#define UWB_STREAM_TO_UINT24(u32, p, index)                                                        \
	{                                                                                          \
		u32 = (uint32_t)(((uint32_t)((p)[(index) + 0U]) << 0U) |                           \
				 ((uint32_t)((p)[(index) + 1U]) << 8U) |                           \
				 ((uint32_t)((p)[(index) + 2U]) << 16U));                          \
		(index) = ((index) + 3U);                                                          \
	}
#define UWB_STREAM_TO_UINT32(u32, p, index)                                                        \
	{                                                                                          \
		u32 = (uint32_t)(((uint32_t)((p)[(index) + 0U]) << 0U) |                           \
				 ((uint32_t)((p)[(index) + 1U]) << 8U) |                           \
				 ((uint32_t)((p)[(index) + 2U]) << 16U) |                          \
				 ((uint32_t)((p)[(index) + 3U]) << 24U));                          \
		(index) = ((index) + sizeof(uint32_t));                                            \
	}
#define UWB_STREAM_TO_ARRAY(a, p, len, index)                                                      \
	{                                                                                          \
		uint32_t ijk;                                                                      \
		for (ijk = 0; ijk < (uint32_t)len; ijk++) {                                        \
			((uint8_t *)(a))[ijk] = (p)[(index)];                                      \
			(index) = ((index) + (sizeof(uint8_t)));                                   \
		}                                                                                  \
	}

/**
 * Macros to get and put bytes to and from a field (Little Endian format).
 * These are the same as to stream, except the pointer is not incremented.
 */

#define UWB_UINT32_TO_FIELD(p, u32)                                                                \
	{                                                                                          \
		(p)[0] = (uint8_t)((u32) >> 0U);                                                   \
		(p)[1] = (uint8_t)((u32) >> 8U);                                                   \
		(p)[2] = (uint8_t)((u32) >> 16U);                                                  \
		(p)[3] = (uint8_t)((u32) >> 24U);                                                  \
	}
#define UWB_UINT16_TO_FIELD(p, u16)                                                                \
	{                                                                                          \
		(p)[0] = (uint8_t)((u16) >> 0U);                                                   \
		(p)[1] = (uint8_t)((u16) >> 8U);                                                   \
	}
#define UWB_UINT8_TO_FIELD(p, u8)                                                                  \
	{                                                                                          \
		(p)[0] = (uint8_t)(u8);                                                            \
	}

/**
 * Macros to get and put bytes to and from a stream (Big Endian format)
 */

#define UWB_UINT32_TO_BE_STREAM(p, u32, index)                                                     \
	{                                                                                          \
		(p)[(index) + 0U] = (uint8_t)((u32) >> 24U);                                       \
		(p)[(index) + 1U] = (uint8_t)((u32) >> 16U);                                       \
		(p)[(index) + 2U] = (uint8_t)((u32) >> 8U);                                        \
		(p)[(index) + 3U] = (uint8_t)((u32) >> 0U);                                        \
		(index) = ((index) + (sizeof(uint32_t)));                                          \
	}
#define UWB_UINT16_TO_BE_STREAM(p, u16, index)                                                     \
	{                                                                                          \
		(p)[(index) + 0U] = (uint8_t)((u16) >> 8U);                                        \
		(p)[(index) + 1U] = (uint8_t)((u16) >> 0U);                                        \
		(index) = ((index) + (sizeof(uint16_t)));                                          \
	}
#define UWB_UINT8_TO_BE_STREAM(p, u8, index)                                                       \
	{                                                                                          \
		(p)[(index)] = (uint8_t)(u8);                                                      \
		(index) = ((index) + (sizeof(uint8_t)));                                           \
	}
#define UWB_ARRAY_TO_BE_STREAM(p, a, len, index)                                                   \
	{                                                                                          \
		uint32_t ijk;                                                                      \
		for (ijk = 0; ijk < (uint32_t)len; ijk++) {                                        \
			(p)[(index)] = (uint8_t)a[ijk];                                            \
			(index) = ((index) + (sizeof(uint8_t)));                                   \
		}                                                                                  \
	}
#define UWB_BE_STREAM_TO_UINT16(u16, p, index)                                                     \
	{                                                                                          \
		u16 = (uint16_t)(((uint16_t)(p)[(index) + 0U] << 8U) |                             \
				 ((uint16_t)(p)[(index) + 1U] << 0U));                             \
		(index) = ((index) + sizeof(uint16_t));                                            \
	}
#define UWB_BE_STREAM_TO_UINT32(u32, p, index)                                                     \
	{                                                                                          \
		u32 = (((uint32_t)((p)[(index) + 3U]) << 0U) |                                     \
		       ((uint32_t)((p)[(index) + 2U]) << 8U) |                                     \
		       ((uint32_t)((p)[(index) + 1U]) << 16U) |                                    \
		       ((uint32_t)((p)[(index) + 0U]) << 24U));                                    \
		(index) = ((index) + 4U);                                                          \
	}
#define UWB_BE_STREAM_TO_ARRAY(p, a, len, index)                                                   \
	{                                                                                          \
		uint32_t ijk;                                                                      \
		for (ijk = 0; ijk < (uint32_t)len; ijk++) {                                        \
			((uint8_t *)a)[ijk] = p[(index)];                                          \
			(index) = ((index) + (sizeof(uint8_t)));                                   \
		}                                                                                  \
	}

/**
 * Macros to get and put bytes to and from a field (Big Endian format).
 * These are the same as to stream, except the pointer is not incremented.
 */

#define UWB_UINT32_TO_BE_FIELD(p, u32)                                                             \
	{                                                                                          \
		(p)[0] = (uint8_t)((u32) >> 24U);                                                  \
		(p)[1] = (uint8_t)((u32) >> 16U);                                                  \
		(p)[2] = (uint8_t)((u32) >> 8U);                                                   \
		(p)[3] = (uint8_t)((u32) >> 0U);                                                   \
	}
/**
 * Macro to reverse the uint32 bytes
 */
#define UWB_REVERSE_BYTES_32(x)                                                                    \
	((((x) & 0xFF000000) >> 24) | (((x) & 0x00FF0000) >> 8) | (((x) & 0x0000FF00) << 8) |      \
	 (((x) & 0x000000FF) << 24))

#endif /* UWB_TYPES_H */
