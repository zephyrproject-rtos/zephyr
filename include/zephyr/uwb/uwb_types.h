/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_UWB_TYPES_H_
#define ZEPHYR_INCLUDE_DRIVERS_UWB_TYPES_H_

#include <stdint.h>

/*
 * Bit position macros.
 * Example usage: (value & UWB_BIT_3) to test bit 3.
 */
#define UWB_BIT_0 (1 << 0) /**< Bit 0 mask */
#define UWB_BIT_1 (1 << 1) /**< Bit 1 mask */
#define UWB_BIT_2 (1 << 2) /**< Bit 2 mask */
#define UWB_BIT_3 (1 << 3) /**< Bit 3 mask */
#define UWB_BIT_4 (1 << 4) /**< Bit 4 mask */
#define UWB_BIT_5 (1 << 5) /**< Bit 5 mask */
#define UWB_BIT_6 (1 << 6) /**< Bit 6 mask */
#define UWB_BIT_7 (1 << 7) /**< Bit 7 mask */

/**
 * Macros to get and put bytes to and from a stream (Little Endian format).
 */

/**
 * Copy uint64_t to buffer stream from index
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
/**
 * Copy uint32_t to buffer stream from index
 */
#define UWB_UINT32_TO_STREAM(p, u32, index)                                                        \
	{                                                                                          \
		(p)[(index) + 0U] = (uint8_t)((u32) >> 0U);                                        \
		(p)[(index) + 1U] = (uint8_t)((u32) >> 8U);                                        \
		(p)[(index) + 2U] = (uint8_t)((u32) >> 16U);                                       \
		(p)[(index) + 3U] = (uint8_t)((u32) >> 24U);                                       \
		(index) = ((index) + sizeof(uint32_t));                                            \
	}
/**
 * Copy uint16_t to buffer stream from index
 */
#define UWB_UINT16_TO_STREAM(p, u16, index)                                                        \
	{                                                                                          \
		(p)[(index) + 0U] = (uint8_t)((u16) >> 0U);                                        \
		(p)[(index) + 1U] = (uint8_t)((u16) >> 8U);                                        \
		(index) = ((index) + (sizeof(uint16_t)));                                          \
	}
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
	{                                                                                          \
		uint32_t ijk;                                                                      \
		for (ijk = 0; ijk < (uint32_t)len; ijk++) {                                        \
			(p)[(index)] = (uint8_t)a[ijk];                                            \
			(index) = ((index) + (sizeof(uint8_t)));                                   \
		}                                                                                  \
	}
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
	{                                                                                          \
		u16 = (uint16_t)(((uint16_t)((p)[(index) + 0U]) << 0U) |                           \
				 ((uint16_t)((p)[(index) + 1U]) << 8U));                           \
		(index) = ((index) + (sizeof(uint16_t)));                                          \
	}
/**
 * Copy int16_t from buffer stream at index
 */
#define UWB_STREAM_TO_INT16(i16, p, index)                                                         \
	{                                                                                          \
		i16 = (int16_t)(((int16_t)((p)[(index) + 0U]) << 0U) |                             \
				((int16_t)((p)[(index) + 1U]) << 8U));                             \
		(index) = ((index) + (sizeof(int16_t)));                                           \
	}
/**
 * Copy uint32_t from buffer stream at index
 */
#define UWB_STREAM_TO_UINT32(u32, p, index)                                                        \
	{                                                                                          \
		u32 = (uint32_t)(((uint32_t)((p)[(index) + 0U]) << 0U) |                           \
				 ((uint32_t)((p)[(index) + 1U]) << 8U) |                           \
				 ((uint32_t)((p)[(index) + 2U]) << 16U) |                          \
				 ((uint32_t)((p)[(index) + 3U]) << 24U));                          \
		(index) = ((index) + sizeof(uint32_t));                                            \
	}
/**
 * Copy uint64_t from buffer stream at index
 */
#define UWB_STREAM_TO_UINT64(u64, p, index)                                                        \
	{                                                                                          \
		u64 = (uint64_t)(((uint64_t)((p)[(index) + 0U]) << 0U) |                           \
				 ((uint64_t)((p)[(index) + 1U]) << 8U) |                           \
				 ((uint64_t)((p)[(index) + 2U]) << 16U) |                          \
				 ((uint64_t)((p)[(index) + 3U]) << 24U) |                          \
				 ((uint64_t)((p)[(index) + 4U]) << 32U) |                          \
				 ((uint64_t)((p)[(index) + 5U]) << 40U) |                          \
				 ((uint64_t)((p)[(index) + 6U]) << 48U) |                          \
				 ((uint64_t)((p)[(index) + 7U]) << 56U));                          \
		(index) = ((index) + sizeof(uint64_t));                                            \
	}
/**
 * Copy array of length len from buffer stream at index
 */
#define UWB_STREAM_TO_ARRAY(a, p, len, index)                                                      \
	{                                                                                          \
		uint32_t ijk;                                                                      \
		for (ijk = 0; ijk < (uint32_t)len; ijk++) {                                        \
			((uint8_t *)(a))[ijk] = (p)[(index)];                                      \
			(index) = ((index) + (sizeof(uint8_t)));                                   \
		}                                                                                  \
	}

/**
 * Copy uint32_t to buffer
 */
#define UWB_UINT32_TO_FIELD(p, u32)                                                                \
	{                                                                                          \
		(p)[0] = (uint8_t)((u32) >> 0U);                                                   \
		(p)[1] = (uint8_t)((u32) >> 8U);                                                   \
		(p)[2] = (uint8_t)((u32) >> 16U);                                                  \
		(p)[3] = (uint8_t)((u32) >> 24U);                                                  \
	}
/**
 * Copy uint16_t to buffer
 */
#define UWB_UINT16_TO_FIELD(p, u16)                                                                \
	{                                                                                          \
		(p)[0] = (uint8_t)((u16) >> 0U);                                                   \
		(p)[1] = (uint8_t)((u16) >> 8U);                                                   \
	}
/**
 * Copy uint8_t to buffer
 */
#define UWB_UINT8_TO_FIELD(p, u8)                                                                  \
	{                                                                                          \
		(p)[0] = (uint8_t)(u8);                                                            \
	}

/**
 * Macros to get and put bytes to and from a stream (Big Endian format)
 */
/**
 * Copy uint32_t to buffer stream from index in BE format
 */
#define UWB_UINT32_TO_BE_STREAM(p, u32, index)                                                     \
	{                                                                                          \
		(p)[(index) + 0U] = (uint8_t)((u32) >> 24U);                                       \
		(p)[(index) + 1U] = (uint8_t)((u32) >> 16U);                                       \
		(p)[(index) + 2U] = (uint8_t)((u32) >> 8U);                                        \
		(p)[(index) + 3U] = (uint8_t)((u32) >> 0U);                                        \
		(index) = ((index) + (sizeof(uint32_t)));                                          \
	}
/**
 * Copy uint16_t to buffer stream from index in BE format
 */
#define UWB_UINT16_TO_BE_STREAM(p, u16, index)                                                     \
	{                                                                                          \
		(p)[(index) + 0U] = (uint8_t)((u16) >> 8U);                                        \
		(p)[(index) + 1U] = (uint8_t)((u16) >> 0U);                                        \
		(index) = ((index) + (sizeof(uint16_t)));                                          \
	}
/**
 * Copy uint8_t to buffer stream from index in BE format
 */
#define UWB_UINT8_TO_BE_STREAM(p, u8, index)                                                       \
	{                                                                                          \
		(p)[(index)] = (uint8_t)(u8);                                                      \
		(index) = ((index) + (sizeof(uint8_t)));                                           \
	}
/**
 * Copy array of length len to buffer stream from index in BE format
 */
#define UWB_ARRAY_TO_BE_STREAM(p, a, len, index)                                                   \
	{                                                                                          \
		uint32_t ijk;                                                                      \
		for (ijk = 0; ijk < (uint32_t)len; ijk++) {                                        \
			(p)[(index)] = (uint8_t)a[ijk];                                            \
			(index) = ((index) + (sizeof(uint8_t)));                                   \
		}                                                                                  \
	}
/**
 * Copy uint16_t from buffer stream from index in BE format
 */
#define UWB_BE_STREAM_TO_UINT16(u16, p, index)                                                     \
	{                                                                                          \
		u16 = (uint16_t)(((uint16_t)(p)[(index) + 0U] << 8U) |                             \
				 ((uint16_t)(p)[(index) + 1U] << 0U));                             \
		(index) = ((index) + sizeof(uint16_t));                                            \
	}
/**
 * Copy uint32_t from buffer stream from index in BE format
 */
#define UWB_BE_STREAM_TO_UINT32(u32, p, index)                                                     \
	{                                                                                          \
		u32 = (((uint32_t)((p)[(index) + 3U]) << 0U) |                                     \
		       ((uint32_t)((p)[(index) + 2U]) << 8U) |                                     \
		       ((uint32_t)((p)[(index) + 1U]) << 16U) |                                    \
		       ((uint32_t)((p)[(index) + 0U]) << 24U));                                    \
		(index) = ((index) + 4U);                                                          \
	}
/**
 * Copy array of length len from buffer stream from index in BE format
 */
#define UWB_BE_STREAM_TO_ARRAY(p, a, len, index)                                                   \
	{                                                                                          \
		uint32_t ijk;                                                                      \
		for (ijk = 0; ijk < (uint32_t)len; ijk++) {                                        \
			((uint8_t *)a)[ijk] = p[(index)];                                          \
			(index) = ((index) + (sizeof(uint8_t)));                                   \
		}                                                                                  \
	}

/**
 * Copy uint32_t to buffer in BE format
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

#endif /* ZEPHYR_INCLUDE_DRIVERS_UWB_TYPES_H_ */
