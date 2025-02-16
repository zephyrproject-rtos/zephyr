/*
 * Copyright (c) 2025 TOKITA Hiroshi
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DT_SHIELD_UTILS_H
#define __DT_SHIELD_UTILS_H

#include <zephyr/dt-bindings/dt-util.h>

/**
 * @defgroup dts_shield_option_apis Shield Option APIs for DTS
 * @brief Utility macros for Shield Parameterization
 *
 * The Shield Option API provides macros to support parameterized configurations
 * in shield overlay. It allows shield developers to customize shield behavior
 * using options specified via build-time arguments.
 * These macros make specified options accessible from overlay files and flexible
 * for multi-instance support.
 *
 * ### Overview
 *
 * This API is integrated with the shield framework.
 * The framework converts shield arguments into macros that can be used in DTS files.
 * The API provides simple access to these macros, which shield developers can use
 * to parameterize their overlays depending on the context.
 *
 * ### Usage
 *
 * Specify shield options during the build process using `--shield` arguments:
 *
 * ```bash
 * west build --shield example_grove_i2c:irq-gpio-pin=1
 * west build --shield example_grove_i2c@1:addr=0x10
 * ```
 *
 * - The first command specifies a shield with `irq-gpio-pin=1`.
 * - The second command specifies a shield instance with the index `@1` and `addr=0x10`.
 *
 * Use these macros in DTS overlays to reference shield options:
 *
 * - `SHIELD_OPTION(IRQ_GPIO_PIN)`
 * - `SHIELD_OPTION(__INDEX)`
 * - `SHIELD_OPTION(ADDR)`
 *
 * For frequently used options, aliases are available, such as `SHIELD_ADDR`
 * for `SHIELD_OPTION(ADDR)`. See specific macro definitions for details.
 *
 * ### Example parametrized shield overlay
 *
 * The following example how to use the Shield option API within a DTS overlay:
 *
 * ```dts
 * &SHIELD_CONN_N {
 *     SHIELD_CONVENTIONAL_LABEL(example): example@SHIELD_ADDR {
 *         compatible = "example,i2c-module"
 *         reg = <SHIELD_0X_ADDR>;
 *
 * #if SHIELD_OPTION_DEFINED(IRQ_GPIO_PIN)
 *         irq-gpios = <&gpio0 SHIELD_OPTION(IRQ_GPIO_PIN) 0>;
 * #endif
 *     };
 * };
 * ```
 *
 * This example illustrates how shield parameters like IRQ_GPIO_PORT
 * can be dynamically inserted into a DTS node.
 *
 * The `shield.yml` is also required. Such as follows.
 *
 * ```yaml
 * shield:
 *   name: example_grove_i2c
 *   full_name: Example Grove I2C module
 *   options:
 *     conn:
 *       type: string
 *       default: "grove_i2c"
 *       description: Specifies the connection type of the shield.
 *     addr:
 *       type: int
 *       default: 0x11
 *       description: Defines the I2C address of the shield.
 *     irq-gpio-pin:
 *       type: int
 *       default: 0
 *       description: Specifies the GPIO pin for interrupt signaling.
 * ```
 *
 * And process with the `west build` command with shield option arguments,
 *
 * ```bash
 * west build --shield example_grove_i2c@1:addr=0x10:int-gpio-pin=1
 * ```
 *
 * Finally, we get the resolved devicetree fragment as follows.
 *
 * ```dts
 * &grove_i2c1 {
 *     example_10_grove_i2c1: example@10 {
 *         compatible = "example,i2c-module"
 *         reg = <0x10>;
 *
 *         irq-gpios = <&gpio0 1 0>;
 *     };
 * };
 * ```
 *
 * ### Internal Behavior
 *
 * When running `west build` with `--shield` arguments, the build system generates derived
 * overlay files containing macros representing the specified options.
 * In the above example, the following definitions will be created.
 *
 * ```c
 * #define SHIELD_OPTION_EXAMPLE_GROVE_I2C_1_ADDR_0X10_IRQ_GPIO_PIN_1___INDEX_EXISTS 1
 * #define SHIELD_OPTION_EXAMPLE_GROVE_I2C_1_ADDR_0X10_IRQ_GPIO_PIN_1___INDEX 1
 * #define SHIELD_OPTION_EXAMPLE_GROVE_I2C_1_ADDR_0X10_IRQ_GPIO_PIN_1_ADDR_EXISTS 1
 * #define SHIELD_OPTION_EXAMPLE_GROVE_I2C_1_ADDR_0X10_IRQ_GPIO_PIN_1_ADDR 16
 * #define SHIELD_OPTION_EXAMPLE_GROVE_I2C_1_ADDR_0X10_IRQ_GPIO_PIN_1_ADDR_HEX 10
 * #define SHIELD_OPTION_EXAMPLE_GROVE_I2C_1_ADDR_0X10_IRQ_GPIO_PIN_1_IRQ_GPIO_PIN_EXISTS 1
 * #define SHIELD_OPTION_EXAMPLE_GROVE_I2C_1_ADDR_0X10_IRQ_GPIO_PIN_1_IRQ_GPIO_PIN 1
 * #define SHIELD_OPTION_EXAMPLE_GROVE_I2C_1_ADDR_0X10_IRQ_GPIO_PIN_1_IRQ_GPIO_PIN_HEX 1
 * ```
 *
 * Default values from shield.yml are also converted into macros:
 *
 * ```c
 * #define SHIELD_OPTION_DEFAULT_EXAMPLE_GROVE_I2C_CONN_EXISTS 1
 * #define SHIELD_OPTION_DEFAULT_EXAMPLE_GROVE_I2C_CONN "grove_i2c"
 * #define SHIELD_OPTION_DEFAULT_EXAMPLE_GROVE_I2C_CONN_STRING_UNQUOTED grove_i2c
 * #define SHIELD_OPTION_DEFAULT_EXAMPLE_GROVE_I2C_CONN_STRING_TOKEN grove_i2c
 * #define SHIELD_OPTION_DEFAULT_EXAMPLE_GROVE_I2C_CONN_STRING_UPPER_TOKEN GROVE_I2C
 * #define SHIELD_OPTION_DEFAULT_EXAMPLE_GROVE_I2C_ADDR_EXISTS 1
 * #define SHIELD_OPTION_DEFAULT_EXAMPLE_GROVE_I2C_ADDR 17
 * #define SHIELD_OPTION_DEFAULT_EXAMPLE_GROVE_I2C_ADDR_HEX 11
 * #define SHIELD_OPTION_DEFAULT_EXAMPLE_GROVE_I2C_IRQ_GPIO_PIN_EXISTS 1
 * #define SHIELD_OPTION_DEFAULT_EXAMPLE_GROVE_I2C_IRQ_GPIO_PIN 0
 * #define SHIELD_OPTION_DEFAULT_EXAMPLE_GROVE_I2C_IRQ_GPIO_PIN_HEX 0
 * ```
 *
 * These macros are contextualized.
 * The macro bound to the current context can resolve with ``SHIELD_BASE_NAME`` and
 * ``SHIELD_DERIVED_NAME`` values.
 *
 * ```c
 * #define SHIELD_BASE_NAME EXAMPLE_GROVE_I2C
 * #define SHIELD_BASE_NAME_EXISTS 1
 * #define SHIELD_DERIVED_NAME EXAMPLE_GROVE_I2C_1_ADDR_0X10_IRQ_GPIO_PIN_1
 * #define SHIELD_DERIVED_NAME_EXISTS 1
 * ```
 *
 * After defining these, the original overlay is included:
 *
 * ```c
 * #include </full_path_for/example_grove_i2c.overlay>
 * ```
 *
 * To ensure proper scoping, the `SHIELD_BASE_NAME` and `SHIELD_DERIVED_NAME` are undefined
 * immediately after the overlay inclusion:
 *
 * ```c
 * #undef SHIELD_DERIVED_NAME_EXISTS
 * #undef SHIELD_DERIVED_NAME
 * #undef SHIELD_BASE_NAME_EXISTS
 * #undef SHIELD_BASE_NAME
 * ```
 *
 * This mechanism ensures that subsequent shield instances with different options can
 * redefine these macros as needed.
 *
 * The Shield Options API makes it easy to reference these generated definitions,
 * helping shield authors to create easily parameterized definitions.
 *
 * @ingroup dts_apis
 * @{
 */

#define SHIELD_OPTION_VALUE(o, var)                                                                \
	UTIL_CAT(SHIELD_OPTION_, UTIL_CAT(SHIELD_DERIVED_NAME, UTIL_CAT(_, UTIL_CAT(o, var))))

#define SHIELD_OPTION_DEFAULT(o, var)                                                              \
	UTIL_CAT(SHIELD_OPTION_DEFAULT_, UTIL_CAT(SHIELD_BASE_NAME, UTIL_CAT(_, UTIL_CAT(o, var))))

/**
 * Determines whether a specified shield option is defined.
 *
 * @param o Option name to check.
 */
#define SHIELD_OPTION_EXISTS(o) IS_ENABLED(SHIELD_OPTION_VALUE(o, _EXISTS))

#define SHIELD_OPTION_BASE(o, var)                                                                 \
	COND_CODE_1(SHIELD_OPTION_EXISTS(o),                                                       \
		    (SHIELD_OPTION_VALUE(o, var)), (SHIELD_OPTION_DEFAULT(o, var)))

/**
 * References the shield options in plain form.
 *
 * @p o option name that is specified in shield option strings
 */
#define SHIELD_OPTION(o) SHIELD_OPTION_BASE(o, /* nothing */)

/**
 * References the shield options in hex form.
 *
 * @p o option name that is specified in shield option strings
 */
#define SHIELD_OPTION_HEX(o) SHIELD_OPTION_BASE(o, _HEX)

/**
 * References the shield options in unquoted string form.
 *
 * @p o option name that is specified in shield option strings
 */
#define SHIELD_OPTION_STRING_UNQUOTED(o) SHIELD_OPTION_BASE(o, _STRING_UNQUOTED)

/**
 * References the shield options in token form.
 *
 * @p o option name that is specified in shield option strings
 */
#define SHIELD_OPTION_STRING_TOKEN(o) SHIELD_OPTION_BASE(o, _STRING_TOKEN)

/**
 * References the shield options in uppercase token form.
 *
 * @p o option name that is specified in shield option strings
 */
#define SHIELD_OPTION_STRING_UPPER_TOKEN(o) SHIELD_OPTION_BASE(o, _STRING_UPPER_TOKEN)

/*
 * Aliases for common options...
 */

/**
 * Get an index parameter which is specified in the shield specifier.
 */
#define SHIELD_INDEX COND_CODE_1(SHIELD_OPTION_EXISTS(__INDEX),	                                   \
						    (SHIELD_OPTION(__INDEX)), ())

/**
 * Gets whether the `conn` option is set or not.
 */
#define SHIELD_CONN_EXISTS SHIELD_OPTION_EXISTS(CONN)

/**
 * Get the `conn` option value.
 */
#define SHIELD_CONN SHIELD_OPTION_STRING_TOKEN(CONN)

/**
 * Gets the connector name with an index appended to it.
 * If the connector name is grove_i2c and the index is not specified,
 * `grove_i2c` is returned. If an index is specified with `@1`, `grove_i2c1` is returned.
 */
#define SHIELD_CONN_N UTIL_CAT(SHIELD_CONN, SHIELD_INDEX)

/**
 * Gets whether the `label` option is set or not.
 */
#define SHIELD_LABEL_EXISTS SHIELD_OPTION_EXISTS(LABEL)

/**
 * Get the `label` option value
 */
#define SHIELD_LABEL SHIELD_OPTION_STRING_TOKEN(LABEL)

/**
 * Gets whether the `addr` option is set or not.
 */
#define SHIELD_ADDR_EXISTS SHIELD_OPTION_EXISTS(ADDR)

/**
 * Get `addr` option value
 */
#define SHIELD_ADDR SHIELD_OPTION(ADDR)

/**
 * Get hexadecimal representation of `addr` option value
 */
#define SHIELD_ADDR_HEX SHIELD_OPTION_HEX(ADDR)

/**
 * Get addr option value with '0x' prefix
 */
#define SHIELD_0X_ADDR UTIL_CAT(0x, SHIELD_ADDR_HEX)

/*
 * Various utility macros...
 */

#define SHIELD_CONVENTIONAL_LABEL_(stem)                                                           \
	COND_CODE_1(SHIELD_OPTION_EXISTS(ADDR),						           \
		    (COND_CODE_1(SHIELD_OPTION_EXISTS(CONN),				           \
				 (UTIL_CAT(stem,						   \
				   UTIL_CAT(_,						           \
				    UTIL_CAT(SHIELD_ADDR_HEX,					   \
				     UTIL_CAT(_, SHIELD_CONN))))),				   \
				 (UTIL_CAT(stem,						   \
				   UTIL_CAT(_, SHIELD_ADDR))))),				   \
		    (COND_CODE_1(SHIELD_OPTION_EXISTS(CONN),				           \
				 (UTIL_CAT(stem,						   \
				   UTIL_CAT(_, SHIELD_CONN))),				           \
				 (stem))))

/**
 * Generate a conventional label name
 *
 * If a label definition exists, it will be returned.
 * Otherwise, it will return a format of the specified stem with the address and connector added,
 * in the format `<stem>[_<address>][_<connnector>]`
 *
 * @p stem Specifies the stem portion of the label name.
 */
#define SHIELD_CONVENTIONAL_LABEL(stem)                                                            \
	COND_CODE_1(SHIELD_OPTION_EXISTS(LABEL),						   \
		    (SHIELD_LABEL),								   \
		    (SHIELD_CONVENTIONAL_LABEL_(stem)))

/**
 * @}
 */

#endif /* __DT_SHIELD_UTILS_H */
