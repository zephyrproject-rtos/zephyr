/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Helper utilities for defining and working with register maps, their fields and enums.
 *
 * @details Memory mapped registers are defined as C preprocessor tokens
 * following a strict format, allowing them to be interfaced with in a generic
 * way.
 *
 * The strict format is summarized below:
 *
 * @code{.text}
 *
 *   #define <reg_name>_ADDR <addr>
 *   #define <reg_name>_FIELD_<field_name>_OFFSET <offset>
 *   #define <reg_name>_FIELD_<field_name>_SIZE <size>
 *   #define <reg_name>_FIELD_<field_name>_ENUM_<enum> <value>
 *
 * @endcode
 *
 * The utilities are based on the following widely used macros:
 *
 *   @ref FIELD_GET
 *   @ref FIELD_PREP
 *   @ref GENMASK
 *   @ref GENMASK64
 *
 * The following example register:
 *
 * @code{.text}
 *
 *   +-------------------------------------------------------------------------+
 *   |                                  Foo                                    |
 *   +---------+-------+-------+-------+-------+-------+-------+-------+-------+
 *   | Address | Bit 7 | Bit 6 | Bit 5 | Bit 4 | Bit 3 | Bit 2 | Bit 1 | Bit 0 |
 *   +---------+-------+-------+-------+-------+-------+-------+-------+-------+
 *   |  0x39   |          Qux          |    Reserved   |  Baz  |      Bar      |
 *   +---------+-----------------------+---------------+-------+---------------+
 *
 * @endcode
 *
 * which has the following enumeration for the Bar field:
 *
 * @code{.text}
 *
 *   +----------------------------------+
 *   |               Bar                |
 *   +----------------+-----------------+
 *   |      Value     |    Meaning      |
 *   +----------------+-----------------+
 *   |        0       |    Disabled     |
 *   +----------------+-----------------+
 *   |        1       |    Push pull    |
 *   +----------------+-----------------+
 *   |        2       |    Open drain   |
 *   +----------------+-----------------+
 *   |        3       |    Open source  |
 *   +----------------+-----------------+
 *
 * @endcode
 *
 * repeated in YAML:
 *
 * @code{.yaml}
 *
 *   "RegName": "Foo"
 *   "Addr": 0x39
 *   "Fields":
 *     "Bar":
 *       "Offset": 0
 *       "Size": 2
 *       "Enums":
 *         "Disabled": 0
 *         "Push pull": 1
 *         "Open drain": 2
 *         "Open source": 3
 *     "Baz":
 *       "Offset": 2
 *       "Size": 1
 *     "Reserved":
 *       "Offset": 3
 *       "Size": 2
 *     "Qux":
 *       "Offset": 5
 *       "Size": 3
 *
 * @endcode
 *
 * shall be defined by a number of C preprocessor (identifier,value) pairs, adhering to the
 * following strict format:
 *
 *   All names, RegName, Fields, Enums, are normalized to valid C preprocessor
 *   identifiers in upper case.
 *
 *   The identifier format for the Addr value of a register is:
 *
 *     RegName + "_ADDR"
 *
 *   The identifier format for the Offset value of a field is:
 *
 *     RegName + "_FIELD_" + FieldName + "_OFFSET"
 *
 *   The identifier format for the Size value of a field is:
 *
 *     RegName + "_FIELD_" + FieldName + "_SIZE"
 *
 *   The identifier format for an Enum value of a field is:
 *
 *     RegName + "_FIELD_" + FieldName + "_ENUM_" + EnumName
 *
 *   Reserved fields shall not be included.
 *
 *   All (identifier,value) pairs are grouped by RegName.
 *
 *   Within each (identifier,value) pair group, the following order is adhered to:
 *
 *     1. Addr
 *     2. Fields sorted by Offset in ascending order
 *       1. Offset
 *       2. Size
 *       3. Enums sorted by value in ascending order
 *
 * The strict format will result in the following register definition for the Foo register:
 *
 * @code{.c}
 *
 *   #define FOO_ADDR 0x39
 *   #define FOO_FIELD_BAR_OFFSET 0
 *   #define FOO_FIELD_BAR_SIZE 2
 *   #define FOO_FIELD_BAR_ENUM_DISABLED 0
 *   #define FOO_FIELD_BAR_ENUM_PUSH_PULL 1
 *   #define FOO_FIELD_BAR_ENUM_OPEN_DRAIN 2
 *   #define FOO_FIELD_BAR_ENUM_OPEN_SOURCE 3
 *   #define FOO_FIELD_BAZ_OFFSET 2
 *   #define FOO_FIELD_BAZ_SIZE 1
 *   #define FOO_FIELD_QUX_OFFSET 5
 *   #define FOO_FIELD_QUX_SIZE 3
 *
 * @endcode
 */

#ifndef ZEPHYR_INCLUDE_SYS_REGMAP_H_
#define ZEPHYR_INCLUDE_SYS_REGMAP_H_

#include <zephyr/sys/util.h>

#include <stdint.h>

/**
 * @brief Get the address of a sys regmap defined register
 *
 * @details Expands to @p _reg_name + "_ADDR"
 *
 * @param _reg_name The name of the register
 *
 * @returns The register address
 */
#define SYS_REGMAP_GET_ADDR(_reg_name) \
	CONCAT(_reg_name, _ADDR)

/**
 * @brief Get the start offset in bits of a field in a sys regmap defined register
 *
 * @details Expands to @p _reg_name + "_FIELD_" + @p _field_name + "_OFFSET"
 *
 * @param _reg_name The name of the register
 * @param _field_name The name of the field within the register
 *
 * @returns Least significant bit index of the field
 */
#define SYS_REGMAP_GET_OFFSET(_reg_name, _field_name) \
	CONCAT(_reg_name, _FIELD_, _field_name, _OFFSET)

/**
 * @brief Get the size in bits of a field in a sys regmap defined register
 *
 * @details Expands to @p _reg_name + "_FIELD_" + @p _field_name + "_SIZE"
 *
 * @param _reg_name The name of the register
 * @param _field_name The name of the field within the register
 *
 * @returns Field width in bits
 */
#define SYS_REGMAP_GET_SIZE(_reg_name, _field_name) \
	CONCAT(_reg_name, _FIELD_, _field_name, _SIZE)

/**
 * @brief Get the value of an enum of a field in a sys regmap defined register
 *
 * @details Expands to @p _reg_name + "_FIELD_" + @p _field_name + "_ENUM_" + @p _enum_name
 *
 * @param _reg_name The name of the register
 * @param _field_name The name of the field within the register
 * @param _enum_name The enum name of the field to get
 *
 * @returns Enum value of the field
 */
#define SYS_REGMAP_GET_ENUM(_reg_name, _field_name, _enum_name) \
	CONCAT(_reg_name, _FIELD_, _field_name, _ENUM_, _enum_name)

/**
 * @brief Get the end offset in bits of a field in a sys regmap defined register
 *
 * @param _reg_name The name of the register
 * @param _field_name The name of the field within the register
 *
 * @returns Most significant bit index of the field
 */
#define SYS_REGMAP_GET_END_OFFSET(_reg_name, _field_name)					\
	(											\
		SYS_REGMAP_GET_OFFSET(_reg_name, _field_name) +					\
		SYS_REGMAP_GET_SIZE(_reg_name, _field_name) -					\
		1										\
	)

/**
 * @brief Generate the bitmask for a field in a sys regmap defined register
 *
 * @param _reg_name The name of the register
 * @param _field_name The name of the field within the register
 *
 * @returns Bit mask covering the field
 */
#define SYS_REGMAP_GEN_MASK(_reg_name, _field_name)						\
	GENMASK(										\
		SYS_REGMAP_GET_END_OFFSET(_reg_name, _field_name),				\
		SYS_REGMAP_GET_OFFSET(_reg_name, _field_name)					\
	)

/**
 * @brief Generate the bitmask for a field in a 64-bit sys regmap defined register
 *
 * @param _reg_name The name of the register
 * @param _field_name The name of the field within the register
 *
 * @returns 64-bit mask covering the field
 */
#define SYS_REGMAP_GEN_MASK64(_reg_name, _field_name)						\
	GENMASK64(										\
		SYS_REGMAP_GET_END_OFFSET(_reg_name, _field_name),				\
		SYS_REGMAP_GET_OFFSET(_reg_name, _field_name)					\
	)

/**
 * @brief Get the value of a field from a sys regmap defined register
 *
 * @param _reg_name The name of the register
 * @param _field_name The name of the field within the register
 * @param _value The raw register value
 *
 * @returns Field value, shifted to bit 0
 */
#define SYS_REGMAP_GET_FIELD(_reg_name, _field_name, _value)					\
	FIELD_GET(										\
		SYS_REGMAP_GEN_MASK(_reg_name, _field_name),					\
		_value										\
	)

/**
 * @brief Get the value of a field from a 64-bit sys regmap defined register
 *
 * @param _reg_name The name of the register
 * @param _field_name The name of the field within the register
 * @param _value The raw 64-bit register value
 *
 * @returns Field value, shifted to bit 0
 */
#define SYS_REGMAP_GET_FIELD64(_reg_name, _field_name, _value)					\
	FIELD_GET(										\
		SYS_REGMAP_GEN_MASK64(_reg_name, _field_name),					\
		_value										\
	)

/**
 * @brief Generate the value of a register from a 32-bit field of a sys regmap defined register
 *
 * @param _reg_name The name of the register
 * @param _field_name The name of the field within the register
 * @param _value The value of the field to generate
 *
 * @returns Generated field with provided value
 */
#define SYS_REGMAP_GEN_FIELD(_reg_name, _field_name, _value)					\
	FIELD_PREP(										\
		SYS_REGMAP_GEN_MASK(_reg_name, _field_name),					\
		_value										\
	)

/**
 * @brief Generate the enum value of a register from a 32-bit field of a sys regmap defined
 * register
 *
 * @param _reg_name The name of the register
 * @param _field_name The name of the field within the register
 * @param _enum_name The enum name of the field to generate
 *
 * @returns Generated field with provided value
 */
#define SYS_REGMAP_GEN_FIELD_ENUM(_reg_name, _field_name, _enum_name)				\
	FIELD_PREP(										\
		SYS_REGMAP_GEN_MASK(_reg_name, _field_name),					\
		SYS_REGMAP_GET_ENUM(_reg_name, _field_name, _enum_name)				\
	)

/**
 * @brief Generate the value of a register from a 64-bit field of a sys regmap defined register
 *
 * @param _reg_name The name of the register
 * @param _field_name The name of the field within the register
 * @param _value The value of the field to generate
 *
 * @returns Generated field with provided value
 */
#define SYS_REGMAP_GEN_FIELD64(_reg_name, _field_name, _value)					\
	FIELD_PREP(										\
		SYS_REGMAP_GEN_MASK64(_reg_name, _field_name),					\
		_value										\
	)

/**
 * @brief Generate the enum value of a register from a 64-bit field of a sys regmap defined
 * register
 *
 * @param _reg_name The name of the register
 * @param _field_name The name of the field within the register
 * @param _enum_name The enum name of the field to generate
 *
 * @returns Generated field with provided value
 */
#define SYS_REGMAP_GEN_FIELD_ENUM64(_reg_name, _field_name, _enum_name)				\
	FIELD_PREP(										\
		SYS_REGMAP_GEN_MASK64(_reg_name, _field_name),					\
		SYS_REGMAP_GET_ENUM(_reg_name, _field_name, _enum_name)				\
	)

#endif /* ZEPHYR_INCLUDE_SYS_REGMAP_H_ */
