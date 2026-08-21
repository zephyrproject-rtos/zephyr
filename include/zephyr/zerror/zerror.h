/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright The Zephyr Project Contributors
 */

#ifndef ZEPHYR_INCLUDE_ZERROR_ZERROR_H_
#define ZEPHYR_INCLUDE_ZERROR_ZERROR_H_

#include <stdint.h>
#include <sys/errno.h>
#include <zephyr/sys/util_macro.h>

/**
 * @file
 * @brief Error handling helper API interface
 * @details This header provides a set of macros to create and manipulate
 *          error codes in a compact and structured way. It allows combining a module identifier,
 *          a custom error code, and a standard errno value into a single integer value, and
 *          provides macros to extract each component from the combined error code.
 * 			The integer is encoded as follow [MSB:LSB]: [sign:1bit][module:7bits][custom_code:8bits][errno:16bits]
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Type definition for zerror_t.
 * @details This type is used to represent error codes in a compact format, combining module,
 *          custom code, and errno values into a single integer. `zerror_t` should always
 *          refer to a unique error source and hence can be simply propagated to upper functions.
 */
typedef int zerror_t;

/** @cond INTERNAL_HIDDEN */

#define Z_ZERRORNO_BITS           (16)
#define Z_ZERROR_CUSTOM_CODE_BITS (8)
#define Z_ZERROR_MODULE_BITS      (7)

#define Z_ZERROR_CUSTOM_CODE_POS (Z_ZERRORNO_BITS)
#define Z_ZERROR_MODULE_POS      (Z_ZERROR_CUSTOM_CODE_POS + Z_ZERROR_CUSTOM_CODE_BITS)

#define Z_ZERRORNO_MASK             BIT_MASK(Z_ZERRORNO_BITS)
#define Z_ZERROR_CUSTOM_CODE_MASK   (BIT_MASK(Z_ZERROR_CUSTOM_CODE_BITS) << Z_ZERROR_CUSTOM_CODE_POS)
#define Z_ZERROR_CUSTOM_MODULE_MASK (BIT_MASK(Z_ZERROR_MODULE_BITS) << Z_ZERROR_MODULE_POS)
#define Z_ZERROR_ABS(zerror)        (((int)(zerror)) < 0 ? -(zerror) : (zerror))

#ifdef __ELASTERROR
BUILD_ASSERT(__ELASTERROR <= INT16_MAX, "The maximum errno value must fit within 16 bits");
#endif

/** @endcond */

/**
 * @brief Combines a custom error code and an errno into a single integer value.
 * @param module The error module. This value is limited to 7 bits unsigned value and it's
 * application specific.
 * @param custom_code The custom error code. This value is limited to a int8_t value and it's
 * application specific.
 * @param errno The standard errno value, limited to a int16_t value. The sign of the value is
 * preserved, so negative errno values are supported.
 */
#define ZERROR(module, custom_code, errno)                                                         \
	((zerror_t)(FIELD_PREP(Z_ZERROR_CUSTOM_MODULE_MASK, (module)) |                   \
		    FIELD_PREP(Z_ZERROR_CUSTOM_CODE_MASK, (custom_code)) |                \
		    FIELD_PREP(Z_ZERRORNO_MASK, (errno))))

/**
 * @brief Extracts the errno value from a zerror_t.
 * @param zerror The zerror_t value.
 * @return The extracted errno value.
 */
#define ZERROR_GET_ERRNO(zerror)                                                                   \
	((int)((int16_t)FIELD_GET(Z_ZERRORNO_MASK, Z_ZERROR_ABS(zerror))))

/**
 * @brief Extracts the module value from a zerror_t.
 * @param zerror The zerror_t value.
 * @return The extracted module value.
 */
#define ZERROR_GET_MODULE(zerror)                                                                  \
	((int)((uint8_t)FIELD_GET(Z_ZERROR_CUSTOM_MODULE_MASK, Z_ZERROR_ABS(zerror))))

/**
 * @brief Extracts the custom code value from a zerror_t.
 * @param zerror The zerror_t value.
 * @return The extracted custom code value.
 */
#define ZERROR_GET_CUSTOM_CODE(zerror)                                                             \
	((int)((int8_t)FIELD_GET(Z_ZERROR_CUSTOM_CODE_MASK, Z_ZERROR_ABS(zerror))))

/**
 * @brief Returns from the current function if the expression evaluates to a non-zero zerror_t. The
 * expression's returned value is used as return value,
 * @note The expression must return a zerror_t value, if the expression returns an errno value, the
 * errno sign will NOT be preserved in this case.
 * @param expr The expression to evaluate.
 */
#define ZERROR_RETURN_ON_ERROR(expr)                                                               \
	do {                                                                                       \
		zerror_t _zerror_rc = (expr);                                                      \
		if (_zerror_rc != 0) {                                                             \
			return _zerror_rc;                                                         \
		}                                                                                  \
	} while (0)

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZERROR_ZERROR_H_ */
