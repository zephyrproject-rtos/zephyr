/*
 * Copyright (c) 2019 Thomas Burdick <thomas.burdick@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ZIO variant data type and helpers.
 */

#ifndef ZEPHYR_INCLUDE_ZIO_VARIANT_H_
#define ZEPHYR_INCLUDE_ZIO_VARIANT_H_

#include <zephyr/types.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief A type tag enum
 */
/**< Variant type identifier. */
enum zio_variant_type {
	zio_variant_float,                 /**< Single-precision float tag. */
	zio_variant_double,                /**< Double-precision float tag. */
	zio_variant_bool,                  /**< 1-bit boolean tag. */
	zio_variant_u8,                    /**< Unsigned 8-bit integer tag. */
	zio_variant_u16,                   /**< Unsigned 16-bit integer tag. */
	zio_variant_u32,                   /**< Unsigned 32-bit integer tag. */
	zio_variant_u64,                   /**< Unsigned 64-bit integer tag. */
	zio_variant_s8,                    /**< Signed 8-bit integer tag. */
	zio_variant_s16,                   /**< Signed 16-bit integer tag. */
	zio_variant_s32,                   /**< Signed 32-bit integer tag. */
	zio_variant_s64                   /**< Signed 64-bit integer tag. */
};
/**
 * @brief A type tagged variant for attributes
 */
struct zio_variant {
	enum zio_variant_type type;

	/**< Assigned value. */
	union {
		float float_val;        /**< Single-precision float value. */
		double double_val;      /**< Double-precision float value. */
		u8_t bool_val : 1;      /**< 1-bit boolean value. */
		u8_t u8_val;            /**< Unsigned 8-bit integer value. */
		u16_t u16_val;          /**< Unsigned 16-bit integer value. */
		u32_t u32_val;          /**< Unsigned 32-bit integer value. */
		u64_t u64_val;          /**< Unsigned 64-bit integer value. */
		s8_t s8_val;            /**< Signed 8-bit integer value. */
		s16_t s16_val;          /**< Signed 16-bit integer value. */
		s32_t s32_val;          /**< Signed 32-bit integer value. */
		s64_t s64_val;          /**< Signed 64-bit integer value. */
	};
};

#define zio_variant_wrap_u16(val) \
	(struct zio_variant){ .type = zio_variant_u16, .u16_val = val }
#define zio_variant_wrap_u32(val) \
	(struct zio_variant){ .type = zio_variant_u32, .u32_val = val }
#define zio_variant_wrap_u64(val) \
	(struct zio_variant){ .type = zio_variant_u64, .u64_val = val }

/**
 * @brief Create a variant with the appropriate type and value
 *
 * @param val A value of any type supported by zio_variant
 *
 * @return A zio_variant
 *
 * TODO determine if there a way of doing this that doesn't warn when -pedantic
 * is enabled, avoiding gcc specific extensions in the static lifetime
 * scenario.
 */
#define zio_variant_wrap(val) _Generic((val), \
		unsigned short : zio_variant_wrap_u16(val), \
		unsigned int : zio_variant_wrap_u32(val), \
		unsigned long : zio_variant_wrap_u32(val), \
		unsigned long long : zio_variant_wrap_u64(val) \
		)

static inline int zio_variant_unwrap_u32(struct zio_variant var, u32_t *val)
{
	__ASSERT(var.type == zio_variant_u32,
		"Unwrap variant into mismatched type");
	if (var.type == zio_variant_u32) {
		*val = var.u32_val;
		return 0;
	}  else {
		return -EINVAL;
	}
}

/**
 * @brief Copy a variants value to a given value
 *
 * @param variant A zio_variant to unwrap
 * @param val A value of type matching the one stored in the variant
 *
 * TODO decide whether this returns failures at run time or just asserts
 * when asserts are enabled
 */
#define zio_variant_unwrap(variant, val)  _Generic((val), \
		unsigned int : zio_variant_unwrap_u32, \
		unsigned long : zio_variant_unwrap_u32 \
		)(variant, &val)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZIO_VARIANT_H_ */
