/*
 * Copyright (c) 2019-2020 Nordic Semiconductor ASA
 * Copyright (c) 2019 Piotr Mienkowski
 * Copyright (c) 2017 ARM Ltd
 * Copyright (c) 2015-2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for GPIO drivers
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_GPIO_H_
#define ZEPHYR_INCLUDE_DRIVERS_GPIO_H_

#include <errno.h>

#include <zephyr/sys/__assert.h>
#include <zephyr/sys/slist.h>
#include <zephyr/tracing/tracing.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/device.h>
#include <zephyr/dt-bindings/gpio/gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief GPIO Driver APIs
 * @defgroup gpio_interface GPIO Driver APIs
 * @since 1.0
 * @version 1.0.0
 * @ingroup io_interfaces
 * @{
 */

/**
 * @name GPIO input/output configuration flags
 * @{
 */

/** Enables pin as input. */
#define GPIO_INPUT              (1U << 16)

/** Enables pin as output, no change to the output state. */
#define GPIO_OUTPUT             (1U << 17)

/** Disables pin for both input and output. */
#define GPIO_DISCONNECTED	0

/** @cond INTERNAL_HIDDEN */

/* Initializes output to a low state. */
#define GPIO_OUTPUT_INIT_LOW    (1U << 18)

/* Initializes output to a high state. */
#define GPIO_OUTPUT_INIT_HIGH   (1U << 19)

/* Initializes output based on logic level */
#define GPIO_OUTPUT_INIT_LOGICAL (1U << 20)

/** @endcond */

/** Configures GPIO pin as output and initializes it to a low state. */
#define GPIO_OUTPUT_LOW         (GPIO_OUTPUT | GPIO_OUTPUT_INIT_LOW)
/** Configures GPIO pin as output and initializes it to a high state. */
#define GPIO_OUTPUT_HIGH        (GPIO_OUTPUT | GPIO_OUTPUT_INIT_HIGH)
/** Configures GPIO pin as output and initializes it to a logic 0. */
#define GPIO_OUTPUT_INACTIVE    (GPIO_OUTPUT |			\
				 GPIO_OUTPUT_INIT_LOW |		\
				 GPIO_OUTPUT_INIT_LOGICAL)
/** Configures GPIO pin as output and initializes it to a logic 1. */
#define GPIO_OUTPUT_ACTIVE      (GPIO_OUTPUT |			\
				 GPIO_OUTPUT_INIT_HIGH |	\
				 GPIO_OUTPUT_INIT_LOGICAL)

/** @} */

/**
 * @name GPIO interrupt configuration flags
 * The `GPIO_INT_*` flags are used to specify how input GPIO pins will trigger
 * interrupts. The interrupts can be sensitive to pin physical or logical level.
 * Interrupts sensitive to pin logical level take into account GPIO_ACTIVE_LOW
 * flag. If a pin was configured as Active Low, physical level low will be
 * considered as logical level 1 (an active state), physical level high will
 * be considered as logical level 0 (an inactive state).
 * The GPIO controller should reset the interrupt status, such as clearing the
 * pending bit, etc, when configuring the interrupt triggering properties.
 * Applications should use the `GPIO_INT_MODE_ENABLE_ONLY` and
 * `GPIO_INT_MODE_DISABLE_ONLY` flags to enable and disable interrupts on the
 * pin without changing any GPIO settings.
 * @{
 */

/** Disables GPIO pin interrupt. */
#define GPIO_INT_DISABLE               (1U << 21)

/** @cond INTERNAL_HIDDEN */

/* Enables GPIO pin interrupt. */
#define GPIO_INT_ENABLE                (1U << 22)

/* GPIO interrupt is sensitive to logical levels.
 *
 * This is a component flag that should be combined with other
 * `GPIO_INT_*` flags to produce a meaningful configuration.
 */
#define GPIO_INT_LEVELS_LOGICAL        (1U << 23)

/* GPIO interrupt is edge sensitive.
 *
 * Note: by default interrupts are level sensitive.
 *
 * This is a component flag that should be combined with other
 * `GPIO_INT_*` flags to produce a meaningful configuration.
 */
#define GPIO_INT_EDGE                  (1U << 24)

/* Trigger detection when input state is (or transitions to) physical low or
 * logical 0 level.
 *
 * This is a component flag that should be combined with other
 * `GPIO_INT_*` flags to produce a meaningful configuration.
 */
#define GPIO_INT_LOW_0                 (1U << 25)

/* Trigger detection on input state is (or transitions to) physical high or
 * logical 1 level.
 *
 * This is a component flag that should be combined with other
 * `GPIO_INT_*` flags to produce a meaningful configuration.
 */
#define GPIO_INT_HIGH_1                (1U << 26)

#ifdef CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT
/* Disable/Enable interrupt functionality without changing other interrupt
 * related register, such as clearing the pending register.
 *
 * This is a component flag that should be combined with `GPIO_INT_ENABLE` or
 * `GPIO_INT_DISABLE` flags to produce a meaningful configuration.
 */
#define GPIO_INT_ENABLE_DISABLE_ONLY   (1u << 27)
#endif /* CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT */

#define GPIO_INT_MASK                  (GPIO_INT_DISABLE | \
					GPIO_INT_ENABLE | \
					GPIO_INT_LEVELS_LOGICAL | \
					GPIO_INT_EDGE | \
					GPIO_INT_LOW_0 | \
					GPIO_INT_HIGH_1)

/** @endcond */

/** Configures GPIO interrupt to be triggered on pin rising edge and enables it.
 */
#define GPIO_INT_EDGE_RISING           (GPIO_INT_ENABLE | \
					GPIO_INT_EDGE | \
					GPIO_INT_HIGH_1)

/** Configures GPIO interrupt to be triggered on pin falling edge and enables
 * it.
 */
#define GPIO_INT_EDGE_FALLING          (GPIO_INT_ENABLE | \
					GPIO_INT_EDGE | \
					GPIO_INT_LOW_0)

/** Configures GPIO interrupt to be triggered on pin rising or falling edge and
 * enables it.
 */
#define GPIO_INT_EDGE_BOTH             (GPIO_INT_ENABLE | \
					GPIO_INT_EDGE | \
					GPIO_INT_LOW_0 | \
					GPIO_INT_HIGH_1)

/** Configures GPIO interrupt to be triggered on pin physical level low and
 * enables it.
 */
#define GPIO_INT_LEVEL_LOW             (GPIO_INT_ENABLE | \
					GPIO_INT_LOW_0)

/** Configures GPIO interrupt to be triggered on pin physical level high and
 * enables it.
 */
#define GPIO_INT_LEVEL_HIGH            (GPIO_INT_ENABLE | \
					GPIO_INT_HIGH_1)

/** Configures GPIO interrupt to be triggered on pin state change to logical
 * level 0 and enables it.
 */
#define GPIO_INT_EDGE_TO_INACTIVE      (GPIO_INT_ENABLE | \
					GPIO_INT_LEVELS_LOGICAL | \
					GPIO_INT_EDGE | \
					GPIO_INT_LOW_0)

/** Configures GPIO interrupt to be triggered on pin state change to logical
 * level 1 and enables it.
 */
#define GPIO_INT_EDGE_TO_ACTIVE        (GPIO_INT_ENABLE | \
					GPIO_INT_LEVELS_LOGICAL | \
					GPIO_INT_EDGE | \
					GPIO_INT_HIGH_1)

/** Configures GPIO interrupt to be triggered on pin logical level 0 and enables
 * it.
 */
#define GPIO_INT_LEVEL_INACTIVE        (GPIO_INT_ENABLE | \
					GPIO_INT_LEVELS_LOGICAL | \
					GPIO_INT_LOW_0)

/** Configures GPIO interrupt to be triggered on pin logical level 1 and enables
 * it.
 */
#define GPIO_INT_LEVEL_ACTIVE          (GPIO_INT_ENABLE | \
					GPIO_INT_LEVELS_LOGICAL | \
					GPIO_INT_HIGH_1)

/** @} */

/** @cond INTERNAL_HIDDEN */
#define GPIO_DIR_MASK		(GPIO_INPUT | GPIO_OUTPUT)
/** @endcond */

/**
 * @brief Identifies a set of pins associated with a port.
 *
 * The pin with index n is present in the set if and only if the bit
 * identified by (1U << n) is set.
 */
typedef uint32_t gpio_port_pins_t;

/**
 * @brief Provides values for a set of pins associated with a port.
 *
 * The value for a pin with index n is high (physical mode) or active
 * (logical mode) if and only if the bit identified by (1U << n) is set.
 * Otherwise the value for the pin is low (physical mode) or inactive
 * (logical mode).
 *
 * Values of this type are often paired with a `gpio_port_pins_t` value
 * that specifies which encoded pin values are valid for the operation.
 */
typedef uint32_t gpio_port_value_t;

/**
 * @brief Provides a type to hold a GPIO pin index.
 *
 * This reduced-size type is sufficient to record a pin number,
 * e.g. from a devicetree GPIOS property.
 */
typedef uint8_t gpio_pin_t;

/**
 * @brief Provides a type to hold GPIO devicetree flags.
 *
 * All GPIO flags that can be expressed in devicetree fit in the low 16
 * bits of the full flags field, so use a reduced-size type to record
 * that part of a GPIOS property.
 *
 * The lower 8 bits are used for standard flags. The upper 8 bits are reserved
 * for SoC specific flags.
 */
typedef uint16_t gpio_dt_flags_t;

/**
 * @brief Provides a type to hold GPIO configuration flags.
 *
 * This type is sufficient to hold all flags used to control GPIO
 * configuration, whether pin or interrupt.
 */
typedef uint32_t gpio_flags_t;

/**
 * @brief Container for GPIO pin information specified in devicetree
 *
 * This type contains a pointer to a GPIO device, pin number for a pin
 * controlled by that device, and the subset of pin configuration
 * flags which may be given in devicetree.
 *
 * @see GPIO_DT_SPEC_GET_BY_IDX
 * @see GPIO_DT_SPEC_GET_BY_IDX_OR
 * @see GPIO_DT_SPEC_GET
 * @see GPIO_DT_SPEC_GET_OR
 */
struct gpio_dt_spec {
	/** GPIO device controlling the pin */
	const struct device *port;
	/** The pin's number on the device */
	gpio_pin_t pin;
	/** The pin's configuration flags as specified in devicetree */
	gpio_dt_flags_t dt_flags;
};

/**
 * @brief Static initializer for a @p gpio_dt_spec
 *
 * This returns a static initializer for a @p gpio_dt_spec structure given a
 * devicetree node identifier, a property specifying a GPIO and an index.
 *
 * Example devicetree fragment:
 *
 *	n: node {
 *		foo-gpios = <&gpio0 1 GPIO_ACTIVE_LOW>,
 *			    <&gpio1 2 GPIO_ACTIVE_LOW>;
 *	}
 *
 * Example usage:
 *
 *	const struct gpio_dt_spec spec = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(n),
 *								 foo_gpios, 1);
 *	// Initializes 'spec' to:
 *	// {
 *	//         .port = DEVICE_DT_GET(DT_NODELABEL(gpio1)),
 *	//         .pin = 2,
 *	//         .dt_flags = GPIO_ACTIVE_LOW
 *	// }
 *
 * The 'gpio' field must still be checked for readiness, e.g. using
 * device_is_ready(). It is an error to use this macro unless the node
 * exists, has the given property, and that property specifies a GPIO
 * controller, pin number, and flags as shown above.
 *
 * @param node_id devicetree node identifier
 * @param prop lowercase-and-underscores property name
 * @param idx logical index into "prop"
 * @return static initializer for a struct gpio_dt_spec for the property
 */
#define GPIO_DT_SPEC_GET_BY_IDX(node_id, prop, idx)			       \
	{								       \
		.port = DEVICE_DT_GET(DT_GPIO_CTLR_BY_IDX(node_id, prop, idx)),\
		.pin = DT_GPIO_PIN_BY_IDX(node_id, prop, idx),		       \
		.dt_flags = DT_GPIO_FLAGS_BY_IDX(node_id, prop, idx),	       \
	}

/**
 * @brief Like GPIO_DT_SPEC_GET_BY_IDX(), with a fallback to a default value
 *
 * If the devicetree node identifier 'node_id' refers to a node with a
 * property 'prop', and the index @p idx is valid for that property,
 * this expands to <tt>GPIO_DT_SPEC_GET_BY_IDX(node_id, prop, idx)</tt>. The @p
 * default_value parameter is not expanded in this case.
 *
 * Otherwise, this expands to @p default_value.
 *
 * @param node_id devicetree node identifier
 * @param prop lowercase-and-underscores property name
 * @param idx logical index into "prop"
 * @param default_value fallback value to expand to
 * @return static initializer for a struct gpio_dt_spec for the property,
 *         or default_value if the node or property do not exist
 */
#define GPIO_DT_SPEC_GET_BY_IDX_OR(node_id, prop, idx, default_value)	\
	COND_CODE_1(DT_PROP_HAS_IDX(node_id, prop, idx),		\
		    (GPIO_DT_SPEC_GET_BY_IDX(node_id, prop, idx)),	\
		    (default_value))

/**
 * @brief Equivalent to GPIO_DT_SPEC_GET_BY_IDX(node_id, prop, 0).
 *
 * @param node_id devicetree node identifier
 * @param prop lowercase-and-underscores property name
 * @return static initializer for a struct gpio_dt_spec for the property
 * @see GPIO_DT_SPEC_GET_BY_IDX()
 */
#define GPIO_DT_SPEC_GET(node_id, prop) \
	GPIO_DT_SPEC_GET_BY_IDX(node_id, prop, 0)

/**
 * @brief Equivalent to
 *        GPIO_DT_SPEC_GET_BY_IDX_OR(node_id, prop, 0, default_value).
 *
 * @param node_id devicetree node identifier
 * @param prop lowercase-and-underscores property name
 * @param default_value fallback value to expand to
 * @return static initializer for a struct gpio_dt_spec for the property
 * @see GPIO_DT_SPEC_GET_BY_IDX_OR()
 */
#define GPIO_DT_SPEC_GET_OR(node_id, prop, default_value) \
	GPIO_DT_SPEC_GET_BY_IDX_OR(node_id, prop, 0, default_value)

/**
 * @brief Static initializer for a @p gpio_dt_spec from a DT_DRV_COMPAT
 * instance's GPIO property at an index.
 *
 * @param inst DT_DRV_COMPAT instance number
 * @param prop lowercase-and-underscores property name
 * @param idx logical index into "prop"
 * @return static initializer for a struct gpio_dt_spec for the property
 * @see GPIO_DT_SPEC_GET_BY_IDX()
 */
#define GPIO_DT_SPEC_INST_GET_BY_IDX(inst, prop, idx) \
	GPIO_DT_SPEC_GET_BY_IDX(DT_DRV_INST(inst), prop, idx)

/**
 * @brief Static initializer for a @p gpio_dt_spec from a DT_DRV_COMPAT
 *        instance's GPIO property at an index, with fallback
 *
 * @param inst DT_DRV_COMPAT instance number
 * @param prop lowercase-and-underscores property name
 * @param idx logical index into "prop"
 * @param default_value fallback value to expand to
 * @return static initializer for a struct gpio_dt_spec for the property
 * @see GPIO_DT_SPEC_GET_BY_IDX()
 */
#define GPIO_DT_SPEC_INST_GET_BY_IDX_OR(inst, prop, idx, default_value)		\
	COND_CODE_1(DT_PROP_HAS_IDX(DT_DRV_INST(inst), prop, idx),		\
		    (GPIO_DT_SPEC_GET_BY_IDX(DT_DRV_INST(inst), prop, idx)),	\
		    (default_value))

/**
 * @brief Equivalent to GPIO_DT_SPEC_INST_GET_BY_IDX(inst, prop, 0).
 *
 * @param inst DT_DRV_COMPAT instance number
 * @param prop lowercase-and-underscores property name
 * @return static initializer for a struct gpio_dt_spec for the property
 * @see GPIO_DT_SPEC_INST_GET_BY_IDX()
 */
#define GPIO_DT_SPEC_INST_GET(inst, prop) \
	GPIO_DT_SPEC_INST_GET_BY_IDX(inst, prop, 0)

/**
 * @brief Equivalent to
 *        GPIO_DT_SPEC_INST_GET_BY_IDX_OR(inst, prop, 0, default_value).
 *
 * @param inst DT_DRV_COMPAT instance number
 * @param prop lowercase-and-underscores property name
 * @param default_value fallback value to expand to
 * @return static initializer for a struct gpio_dt_spec for the property
 * @see GPIO_DT_SPEC_INST_GET_BY_IDX()
 */
#define GPIO_DT_SPEC_INST_GET_OR(inst, prop, default_value) \
	GPIO_DT_SPEC_INST_GET_BY_IDX_OR(inst, prop, 0, default_value)

/*
 * @cond INTERNAL_HIDDEN
 */

/**
 * Auxiliary conditional macro that generates a bitmask for the range
 * from @p "prop" array defined by the (off_idx, sz_idx) pair,
 * or 0 if the range does not exist.
 *
 * @param node_id devicetree node identifier
 * @param prop lowercase-and-underscores array property name
 * @param off_idx logical index of bitmask offset value into "prop" array
 * @param sz_idx logical index of bitmask size value into "prop" array
 */
#define Z_GPIO_GEN_BITMASK_COND(node_id, prop, off_idx, sz_idx)		     \
	COND_CODE_1(DT_PROP_HAS_IDX(node_id, prop, off_idx),		     \
		(COND_CODE_0(DT_PROP_BY_IDX(node_id, prop, sz_idx),	     \
			(0),						     \
			(GENMASK64(DT_PROP_BY_IDX(node_id, prop, off_idx) +  \
				DT_PROP_BY_IDX(node_id, prop, sz_idx) - 1,   \
				DT_PROP_BY_IDX(node_id, prop, off_idx))))    \
		), (0))

/**
 * A helper conditional macro returning generated bitmask for one element
 * from @p "gpio-reserved-ranges"
 *
 * @param odd_it the value of an odd sequential iterator
 * @param node_id devicetree node identifier
 */
#define Z_GPIO_GEN_RESERVED_RANGES_COND(odd_it, node_id)		      \
	COND_CODE_1(DT_PROP_HAS_IDX(node_id, gpio_reserved_ranges, odd_it),   \
		(Z_GPIO_GEN_BITMASK_COND(node_id,			      \
			gpio_reserved_ranges,				      \
			GET_ARG_N(odd_it, Z_SPARSE_LIST_EVEN_NUMBERS),	      \
			odd_it)),					      \
		(0))

/**
 * @endcond
 */

/**
 * @brief Makes a bitmask of reserved GPIOs from DT @p "gpio-reserved-ranges"
 *        property and @p "ngpios" argument
 *
 * This macro returns the value as a bitmask of the @p "gpio-reserved-ranges"
 * property. This property defines the disabled (or 'reserved') GPIOs in the
 * range @p 0...ngpios-1 and is specified as an array of value's pairs that
 * define the start offset and size of the reserved ranges.
 *
 * For example, setting "gpio-reserved-ranges = <3 2>, <10 1>;"
 * means that GPIO offsets 3, 4 and 10 cannot be used even if @p ngpios = <18>.
 *
 * The implementation constraint is inherited from common DT limitations:
 * a maximum of 64 pairs can be used (with result limited to bitsize
 * of gpio_port_pins_t type).
 *
 * NB: Due to the nature of C macros, some incorrect tuple definitions
 *    (for example, overlapping or out of range) will produce undefined results.
 *
 *    Also be aware that if @p ngpios is less than 32 (bit size of DT int type),
 *    then all unused MSBs outside the range defined by @p ngpios will be
 *    marked as reserved too.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *	a {
 *		compatible = "some,gpio-controller";
 *		ngpios = <32>;
 *		gpio-reserved-ranges = <0  4>, <5  3>, <9  5>, <11 2>, <15 2>,
 *					<18 2>, <21 1>, <23 1>, <25 4>, <30 2>;
 *	};
 *
 *	b {
 *		compatible = "some,gpio-controller";
 *		ngpios = <18>;
 *		gpio-reserved-ranges = <3 2>, <10 1>;
 *	};
 *
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *	struct some_config {
 *		uint32_t ngpios;
 *		uint32_t gpios_reserved;
 *	};
 *
 *	static const struct some_config dev_cfg_a = {
 *		.ngpios = DT_PROP_OR(DT_LABEL(a), ngpios, 0),
 *		.gpios_reserved = GPIO_DT_RESERVED_RANGES_NGPIOS(DT_LABEL(a),
 *					DT_PROP(DT_LABEL(a), ngpios)),
 *	};
 *
 *	static const struct some_config dev_cfg_b = {
 *		.ngpios = DT_PROP_OR(DT_LABEL(b), ngpios, 0),
 *		.gpios_reserved = GPIO_DT_RESERVED_RANGES_NGPIOS(DT_LABEL(b),
 *					DT_PROP(DT_LABEL(b), ngpios)),
 *	};
 *@endcode
 *
 * This expands to:
 *
 * @code{.c}
 *	struct some_config {
 *		uint32_t ngpios;
 *		uint32_t gpios_reserved;
 *	};
 *
 *	static const struct some_config dev_cfg_a = {
 *		.ngpios = 32,
 *		.gpios_reserved = 0xdeadbeef,
 *		               // 0b1101 1110 1010 1101 1011 1110 1110 1111
 *	};
 *	static const struct some_config dev_cfg_b = {
 *		.ngpios = 18,
 *		.gpios_reserved = 0xfffc0418,
 *		               // 0b1111 1111 1111 1100 0000 0100 0001 1000
 *		               // unused MSBs were marked as reserved too
 *	};
 * @endcode
 *
 * @param node_id GPIO controller node identifier.
 * @param ngpios number of GPIOs.
 * @return the bitmask of reserved gpios
 */
#define GPIO_DT_RESERVED_RANGES_NGPIOS(node_id, ngpios)			       \
	((gpio_port_pins_t)						       \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, gpio_reserved_ranges),	       \
		(GENMASK64(BITS_PER_LONG_LONG - 1, ngpios)		       \
		| FOR_EACH_FIXED_ARG(Z_GPIO_GEN_RESERVED_RANGES_COND,	       \
			(|),						       \
			node_id,					       \
			LIST_DROP_EMPTY(Z_SPARSE_LIST_ODD_NUMBERS))),	       \
		(0)))

/**
 * @brief Makes a bitmask of reserved GPIOs from the @p "gpio-reserved-ranges"
 *        and @p "ngpios" DT properties values
 *
 * @param node_id GPIO controller node identifier.
 * @return the bitmask of reserved gpios
 */
#define GPIO_DT_RESERVED_RANGES(node_id)				       \
	GPIO_DT_RESERVED_RANGES_NGPIOS(node_id, DT_PROP(node_id, ngpios))

/**
 * @brief Makes a bitmask of reserved GPIOs from a DT_DRV_COMPAT instance's
 *        @p "gpio-reserved-ranges" property and @p "ngpios" argument
 *
 * @param inst DT_DRV_COMPAT instance number
 * @return the bitmask of reserved gpios
 * @param ngpios  number of GPIOs
 * @see GPIO_DT_RESERVED_RANGES()
 */
#define GPIO_DT_INST_RESERVED_RANGES_NGPIOS(inst, ngpios)		       \
		GPIO_DT_RESERVED_RANGES_NGPIOS(DT_DRV_INST(inst), ngpios)

/**
 * @brief Make a bitmask of reserved GPIOs from a DT_DRV_COMPAT instance's GPIO
 *        @p "gpio-reserved-ranges" and @p "ngpios" properties
 *
 * @param inst DT_DRV_COMPAT instance number
 * @return the bitmask of reserved gpios
 * @see GPIO_DT_RESERVED_RANGES()
 */
#define GPIO_DT_INST_RESERVED_RANGES(inst)				       \
		GPIO_DT_RESERVED_RANGES(DT_DRV_INST(inst))

/**
 * @brief Makes a bitmask of allowed GPIOs from DT @p "gpio-reserved-ranges"
 *        property and @p "ngpios" argument
 *
 * This macro is paired with GPIO_DT_RESERVED_RANGES_NGPIOS(), however unlike
 * the latter, it returns a bitmask of ALLOWED gpios.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *	a {
 *		compatible = "some,gpio-controller";
 *		ngpios = <32>;
 *		gpio-reserved-ranges = <0 8>, <9 5>, <15 16>;
 *	};
 *
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *	struct some_config {
 *		uint32_t port_pin_mask;
 *	};
 *
 *	static const struct some_config dev_cfg = {
 *		.port_pin_mask = GPIO_DT_PORT_PIN_MASK_NGPIOS_EXC(
 *					DT_LABEL(a), 32),
 *	};
 * @endcode
 *
 * This expands to:
 *
 * @code{.c}
 *	struct some_config {
 *		uint32_t port_pin_mask;
 *	};
 *
 *	static const struct some_config dev_cfg = {
 *		.port_pin_mask = 0x80004100,
 *				// 0b1000 0000 0000 0000 0100 0001 00000 000
 *	};
 * @endcode
 *
 * @param node_id GPIO controller node identifier.
 * @param ngpios  number of GPIOs
 * @return the bitmask of allowed gpios
 */
#define GPIO_DT_PORT_PIN_MASK_NGPIOS_EXC(node_id, ngpios)		       \
	((gpio_port_pins_t)						       \
	COND_CODE_0(ngpios,						       \
		(0),							       \
		(COND_CODE_1(DT_NODE_HAS_PROP(node_id, gpio_reserved_ranges),  \
			((GENMASK64(ngpios - 1, 0) &			       \
			~GPIO_DT_RESERVED_RANGES_NGPIOS(node_id, ngpios))),    \
			(GENMASK64(ngpios - 1, 0)))			       \
		)							       \
	))

/**
 * @brief Makes a bitmask of allowed GPIOs from a DT_DRV_COMPAT instance's
 *        @p "gpio-reserved-ranges" property and @p "ngpios" argument
 *
 * @param inst DT_DRV_COMPAT instance number
 * @param ngpios number of GPIOs
 * @return the bitmask of allowed gpios
 * @see GPIO_DT_NGPIOS_PORT_PIN_MASK_EXC()
 */
#define GPIO_DT_INST_PORT_PIN_MASK_NGPIOS_EXC(inst, ngpios)	\
		GPIO_DT_PORT_PIN_MASK_NGPIOS_EXC(DT_DRV_INST(inst), ngpios)

/**
 * @brief Maximum number of pins that are supported by `gpio_port_pins_t`.
 */
#define GPIO_MAX_PINS_PER_PORT (sizeof(gpio_port_pins_t) * __CHAR_BIT__)

/**
 * This structure is common to all GPIO drivers and is expected to be
 * the first element in the object pointed to by the config field
 * in the device structure.
 */
struct gpio_driver_config {
	/** Mask identifying pins supported by the controller.
	 *
	 * Initialization of this mask is the responsibility of device
	 * instance generation in the driver.
	 */
	gpio_port_pins_t port_pin_mask;
};

/**
 * This structure is common to all GPIO drivers and is expected to be the first
 * element in the driver's struct driver_data declaration.
 */
struct gpio_driver_data {
	/** Mask identifying pins that are configured as active low.
	 *
	 * Management of this mask is the responsibility of the
	 * wrapper functions in this header.
	 */
	gpio_port_pins_t invert;
};

struct gpio_callback;

/**
 * @typedef gpio_callback_handler_t
 * @brief Define the application callback handler function signature
 *
 * @param port Device struct for the GPIO device.
 * @param cb Original struct gpio_callback owning this handler
 * @param pins Mask of pins that triggers the callback handler
 *
 * Note: cb pointer can be used to retrieve private data through
 * CONTAINER_OF() if original struct gpio_callback is stored in
 * another private structure.
 */
typedef void (*gpio_callback_handler_t)(const struct device *port,
					struct gpio_callback *cb,
					gpio_port_pins_t pins);

/**
 * @brief GPIO callback structure
 *
 * Used to register a callback in the driver instance callback list.
 * As many callbacks as needed can be added as long as each of them
 * are unique pointers of struct gpio_callback.
 * Beware such structure should not be allocated on stack.
 *
 * Note: To help setting it, see gpio_init_callback() below
 */
struct gpio_callback {
	/** This is meant to be used in the driver and the user should not
	 * mess with it (see drivers/gpio/gpio_utils.h)
	 */
	sys_snode_t node;

	/** Actual callback function being called when relevant. */
	gpio_callback_handler_t handler;

	/** A mask of pins the callback is interested in, if 0 the callback
	 * will never be called. Such pin_mask can be modified whenever
	 * necessary by the owner, and thus will affect the handler being
	 * called or not. The selected pins must be configured to trigger
	 * an interrupt.
	 */
	gpio_port_pins_t pin_mask;
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * For internal use only, skip these in public documentation.
 */

/* Used by driver api function pin_interrupt_configure, these are defined
 * in terms of the public flags so we can just mask and pass them
 * through to the driver api
 */
enum gpio_int_mode {
	GPIO_INT_MODE_DISABLED = GPIO_INT_DISABLE,
	GPIO_INT_MODE_LEVEL = GPIO_INT_ENABLE,
	GPIO_INT_MODE_EDGE = GPIO_INT_ENABLE | GPIO_INT_EDGE,
#ifdef CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT
	GPIO_INT_MODE_DISABLE_ONLY = GPIO_INT_DISABLE | GPIO_INT_ENABLE_DISABLE_ONLY,
	GPIO_INT_MODE_ENABLE_ONLY = GPIO_INT_ENABLE | GPIO_INT_ENABLE_DISABLE_ONLY,
#endif /* CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT */
};

enum gpio_int_trig {
	/* Trigger detection when input state is (or transitions to)
	 * physical low. (Edge Falling or Active Low)
	 */
	GPIO_INT_TRIG_LOW = GPIO_INT_LOW_0,
	/* Trigger detection when input state is (or transitions to)
	 * physical high. (Edge Rising or Active High) */
	GPIO_INT_TRIG_HIGH = GPIO_INT_HIGH_1,
	/* Trigger detection on pin rising or falling edge. */
	GPIO_INT_TRIG_BOTH = GPIO_INT_LOW_0 | GPIO_INT_HIGH_1,
	/* Trigger a system wakeup. */
	GPIO_INT_TRIG_WAKE = GPIO_INT_WAKEUP,
};

__subsystem struct gpio_driver_api {
	int (*pin_configure)(const struct device *port, gpio_pin_t pin,
			     gpio_flags_t flags);
#ifdef CONFIG_GPIO_GET_CONFIG
	int (*pin_get_config)(const struct device *port, gpio_pin_t pin,
			      gpio_flags_t *flags);
#endif
	int (*port_get_raw)(const struct device *port,
			    gpio_port_value_t *value);
	int (*port_set_masked_raw)(const struct device *port,
				   gpio_port_pins_t mask,
				   gpio_port_value_t value);
	int (*port_set_bits_raw)(const struct device *port,
				 gpio_port_pins_t pins);
	int (*port_clear_bits_raw)(const struct device *port,
				   gpio_port_pins_t pins);
	int (*port_toggle_bits)(const struct device *port,
				gpio_port_pins_t pins);
	int (*pin_interrupt_configure)(const struct device *port,
				       gpio_pin_t pin,
				       enum gpio_int_mode mode,
				       enum gpio_int_trig trig);
	int (*manage_callback)(const struct device *port,
			       struct gpio_callback *cb,
			       bool set);
	uint32_t (*get_pending_int)(const struct device *dev);
#ifdef CONFIG_GPIO_GET_DIRECTION
	int (*port_get_direction)(const struct device *port, gpio_port_pins_t map,
				  gpio_port_pins_t *inputs, gpio_port_pins_t *outputs);
#endif /* CONFIG_GPIO_GET_DIRECTION */
};

/**
 * @endcond
 */

/**
 * @brief Validate that GPIO port is ready.
 *
 * @param spec GPIO specification from devicetree
 *
 * @retval true if the GPIO spec is ready for use.
 * @retval false if the GPIO spec is not ready for use.
 */
static inline bool gpio_is_ready_dt(const struct gpio_dt_spec *spec)
{
	/* Validate port is ready */
	return device_is_ready(spec->port);
}

/**
 * @brief Configure pin interrupt.
 *
 * @note This function can also be used to configure interrupts on pins
 *       not controlled directly by the GPIO module. That is, pins which are
 *       routed to other modules such as I2C, SPI, UART.
 *
 * @funcprops \isr_ok
 *
 * @param port Pointer to device structure for the driver instance.
 * @param pin Pin number.
 * @param flags Interrupt configuration flags as defined by GPIO_INT_*.
 *
 * @retval 0 If successful.
 * @retval -ENOSYS If the operation is not implemented by the driver.
 * @retval -ENOTSUP If any of the configuration options is not supported
 *                  (unless otherwise directed by flag documentation).
 * @retval -EINVAL  Invalid argument.
 * @retval -EBUSY   Interrupt line required to configure pin interrupt is
 *                  already in use.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
__syscall int gpio_pin_interrupt_configure(const struct device *port,
					   gpio_pin_t pin,
					   gpio_flags_t flags);

static inline int z_impl_gpio_pin_interrupt_configure(const struct device *port,
						      gpio_pin_t pin,
						      gpio_flags_t flags)
{
	const struct gpio_driver_api *api =
		(const struct gpio_driver_api *)port->api;
	__unused const struct gpio_driver_config *const cfg =
		(const struct gpio_driver_config *)port->config;
	const struct gpio_driver_data *const data =
		(const struct gpio_driver_data *)port->data;
	enum gpio_int_trig trig;
	enum gpio_int_mode mode;
	int ret;

	SYS_PORT_TRACING_FUNC_ENTER(gpio_pin, interrupt_configure, port, pin, flags);

	if (api->pin_interrupt_configure == NULL) {
		SYS_PORT_TRACING_FUNC_EXIT(gpio_pin, interrupt_configure, port, pin, -ENOSYS);
		return -ENOSYS;
	}

	__ASSERT((flags & (GPIO_INT_DISABLE | GPIO_INT_ENABLE))
		 != (GPIO_INT_DISABLE | GPIO_INT_ENABLE),
		 "Cannot both enable and disable interrupts");

	__ASSERT((flags & (GPIO_INT_DISABLE | GPIO_INT_ENABLE)) != 0U,
		 "Must either enable or disable interrupts");

	__ASSERT(((flags & GPIO_INT_ENABLE) == 0) ||
		 ((flags & GPIO_INT_EDGE) != 0) ||
		 ((flags & (GPIO_INT_LOW_0 | GPIO_INT_HIGH_1)) !=
		  (GPIO_INT_LOW_0 | GPIO_INT_HIGH_1)),
		 "Only one of GPIO_INT_LOW_0, GPIO_INT_HIGH_1 can be "
		 "enabled for a level interrupt.");

	__ASSERT(((flags & GPIO_INT_ENABLE) == 0) ||
#ifdef CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT
			 ((flags & (GPIO_INT_LOW_0 | GPIO_INT_HIGH_1)) != 0) ||
			 (flags & GPIO_INT_ENABLE_DISABLE_ONLY) != 0,
#else
			 ((flags & (GPIO_INT_LOW_0 | GPIO_INT_HIGH_1)) != 0),
#endif /* CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT */
		 "At least one of GPIO_INT_LOW_0, GPIO_INT_HIGH_1 has to be "
		 "enabled.");

	__ASSERT((cfg->port_pin_mask & (gpio_port_pins_t)BIT(pin)) != 0U,
		 "Unsupported pin");

	if (((flags & GPIO_INT_LEVELS_LOGICAL) != 0) &&
	    ((data->invert & (gpio_port_pins_t)BIT(pin)) != 0)) {
		/* Invert signal bits */
		flags ^= (GPIO_INT_LOW_0 | GPIO_INT_HIGH_1);
	}

	trig = (enum gpio_int_trig)(flags & (GPIO_INT_LOW_0 | GPIO_INT_HIGH_1 | GPIO_INT_WAKEUP));
#ifdef CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT
	mode = (enum gpio_int_mode)(flags & (GPIO_INT_EDGE | GPIO_INT_DISABLE | GPIO_INT_ENABLE |
					     GPIO_INT_ENABLE_DISABLE_ONLY));
#else
	mode = (enum gpio_int_mode)(flags & (GPIO_INT_EDGE | GPIO_INT_DISABLE | GPIO_INT_ENABLE));
#endif /* CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT */

	ret = api->pin_interrupt_configure(port, pin, mode, trig);
	SYS_PORT_TRACING_FUNC_EXIT(gpio_pin, interrupt_configure, port, pin, ret);
	return ret;
}

/**
 * @brief Configure pin interrupts from a @p gpio_dt_spec.
 *
 * @funcprops \isr_ok
 *
 * This is equivalent to:
 *
 *     gpio_pin_interrupt_configure(spec->port, spec->pin, flags);
 *
 * The <tt>spec->dt_flags</tt> value is not used.
 *
 * @param spec GPIO specification from devicetree
 * @param flags interrupt configuration flags
 * @return a value from gpio_pin_interrupt_configure()
 */
static inline int gpio_pin_interrupt_configure_dt(const struct gpio_dt_spec *spec,
						  gpio_flags_t flags)
{
	return gpio_pin_interrupt_configure(spec->port, spec->pin, flags);
}

/**
 * @brief Configure a single pin.
 *
 * @param port Pointer to device structure for the driver instance.
 * @param pin Pin number to configure.
 * @param flags Flags for pin configuration: 'GPIO input/output configuration
 *        flags', 'GPIO pin drive flags', 'GPIO pin bias flags'.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP if any of the configuration options is not supported
 *                  (unless otherwise directed by flag documentation).
 * @retval -EINVAL Invalid argument.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
__syscall int gpio_pin_configure(const struct device *port,
				 gpio_pin_t pin,
				 gpio_flags_t flags);

static inline int z_impl_gpio_pin_configure(const struct device *port,
					    gpio_pin_t pin,
					    gpio_flags_t flags)
{
	const struct gpio_driver_api *api =
		(const struct gpio_driver_api *)port->api;
	__unused const struct gpio_driver_config *const cfg =
		(const struct gpio_driver_config *)port->config;
	struct gpio_driver_data *data =
		(struct gpio_driver_data *)port->data;
	int ret;

	SYS_PORT_TRACING_FUNC_ENTER(gpio_pin, configure, port, pin, flags);

	__ASSERT((flags & GPIO_INT_MASK) == 0,
		 "Interrupt flags are not supported");

	__ASSERT((flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) !=
		 (GPIO_PULL_UP | GPIO_PULL_DOWN),
		 "Pull Up and Pull Down should not be enabled simultaneously");

	__ASSERT(!((flags & GPIO_INPUT) && !(flags & GPIO_OUTPUT) && (flags & GPIO_SINGLE_ENDED)),
		 "Input cannot be enabled for 'Open Drain', 'Open Source' modes without Output");

	__ASSERT_NO_MSG((flags & GPIO_SINGLE_ENDED) != 0 ||
			(flags & GPIO_LINE_OPEN_DRAIN) == 0);

	__ASSERT((flags & (GPIO_OUTPUT_INIT_LOW | GPIO_OUTPUT_INIT_HIGH)) == 0
		 || (flags & GPIO_OUTPUT) != 0,
		 "Output needs to be enabled to be initialized low or high");

	__ASSERT((flags & (GPIO_OUTPUT_INIT_LOW | GPIO_OUTPUT_INIT_HIGH))
		 != (GPIO_OUTPUT_INIT_LOW | GPIO_OUTPUT_INIT_HIGH),
		 "Output cannot be initialized low and high");

	if (((flags & GPIO_OUTPUT_INIT_LOGICAL) != 0)
	    && ((flags & (GPIO_OUTPUT_INIT_LOW | GPIO_OUTPUT_INIT_HIGH)) != 0)
	    && ((flags & GPIO_ACTIVE_LOW) != 0)) {
		flags ^= GPIO_OUTPUT_INIT_LOW | GPIO_OUTPUT_INIT_HIGH;
	}

	flags &= ~GPIO_OUTPUT_INIT_LOGICAL;

	__ASSERT((cfg->port_pin_mask & (gpio_port_pins_t)BIT(pin)) != 0U,
		 "Unsupported pin");

	if ((flags & GPIO_ACTIVE_LOW) != 0) {
		data->invert |= (gpio_port_pins_t)BIT(pin);
	} else {
		data->invert &= ~(gpio_port_pins_t)BIT(pin);
	}

	ret = api->pin_configure(port, pin, flags);
	SYS_PORT_TRACING_FUNC_EXIT(gpio_pin, configure, port, pin, ret);
	return ret;
}

/**
 * @brief Configure a single pin from a @p gpio_dt_spec and some extra flags.
 *
 * This is equivalent to:
 *
 *     gpio_pin_configure(spec->port, spec->pin, spec->dt_flags | extra_flags);
 *
 * @param spec GPIO specification from devicetree
 * @param extra_flags additional flags
 * @return a value from gpio_pin_configure()
 */
static inline int gpio_pin_configure_dt(const struct gpio_dt_spec *spec,
					gpio_flags_t extra_flags)
{
	return gpio_pin_configure(spec->port,
				  spec->pin,
				  spec->dt_flags | extra_flags);
}

/**
 * @brief Get direction of select pins in a port.
 *
 * Retrieve direction of each pin specified in @p map.
 *
 * If @p inputs or @p outputs is NULL, then this function does not get the
 * respective input or output direction information.
 *
 * @param port Pointer to the device structure for the driver instance.
 * @param map Bitmap of pin directions to query.
 * @param inputs Pointer to a variable where input directions will be stored.
 * @param outputs Pointer to a variable where output directions will be stored.
 *
 * @retval 0 If successful.
 * @retval -ENOSYS if the underlying driver does not support this call.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
__syscall int gpio_port_get_direction(const struct device *port, gpio_port_pins_t map,
				      gpio_port_pins_t *inputs, gpio_port_pins_t *outputs);

#ifdef CONFIG_GPIO_GET_DIRECTION
static inline int z_impl_gpio_port_get_direction(const struct device *port, gpio_port_pins_t map,
						 gpio_port_pins_t *inputs,
						 gpio_port_pins_t *outputs)
{
	const struct gpio_driver_api *api = (const struct gpio_driver_api *)port->api;
	int ret;

	SYS_PORT_TRACING_FUNC_ENTER(gpio_port, get_direction, port, map, inputs, outputs);

	if (api->port_get_direction == NULL) {
		SYS_PORT_TRACING_FUNC_EXIT(gpio_port, get_direction, port, -ENOSYS);
		return -ENOSYS;
	}

	ret = api->port_get_direction(port, map, inputs, outputs);
	SYS_PORT_TRACING_FUNC_EXIT(gpio_port, get_direction, port, ret);
	return ret;
}
#endif /* CONFIG_GPIO_GET_DIRECTION */

/**
 * @brief Check if @p pin is configured for input
 *
 * @param port Pointer to device structure for the driver instance.
 * @param pin Pin number to query the direction of
 *
 * @retval 1 if @p pin is configured as @ref GPIO_INPUT.
 * @retval 0 if @p pin is not configured as @ref GPIO_INPUT.
 * @retval -ENOSYS if the underlying driver does not support this call.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
static inline int gpio_pin_is_input(const struct device *port, gpio_pin_t pin)
{
	int rv;
	gpio_port_pins_t pins;
	__unused const struct gpio_driver_config *cfg =
		(const struct gpio_driver_config *)port->config;

	__ASSERT((cfg->port_pin_mask & (gpio_port_pins_t)BIT(pin)) != 0U, "Unsupported pin");

	rv = gpio_port_get_direction(port, BIT(pin), &pins, NULL);
	if (rv < 0) {
		return rv;
	}

	return (int)!!((gpio_port_pins_t)BIT(pin) & pins);
}

/**
 * @brief Check if a single pin from @p gpio_dt_spec is configured for input
 *
 * This is equivalent to:
 *
 *     gpio_pin_is_input(spec->port, spec->pin);
 *
 * @param spec GPIO specification from devicetree.
 *
 * @return A value from gpio_pin_is_input().
 */
static inline int gpio_pin_is_input_dt(const struct gpio_dt_spec *spec)
{
	return gpio_pin_is_input(spec->port, spec->pin);
}

/**
 * @brief Check if @p pin is configured for output
 *
 * @param port Pointer to device structure for the driver instance.
 * @param pin Pin number to query the direction of
 *
 * @retval 1 if @p pin is configured as @ref GPIO_OUTPUT.
 * @retval 0 if @p pin is not configured as @ref GPIO_OUTPUT.
 * @retval -ENOSYS if the underlying driver does not support this call.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
static inline int gpio_pin_is_output(const struct device *port, gpio_pin_t pin)
{
	int rv;
	gpio_port_pins_t pins;
	__unused const struct gpio_driver_config *cfg =
		(const struct gpio_driver_config *)port->config;

	__ASSERT((cfg->port_pin_mask & (gpio_port_pins_t)BIT(pin)) != 0U, "Unsupported pin");

	rv = gpio_port_get_direction(port, BIT(pin), NULL, &pins);
	if (rv < 0) {
		return rv;
	}

	return (int)!!((gpio_port_pins_t)BIT(pin) & pins);
}

/**
 * @brief Check if a single pin from @p gpio_dt_spec is configured for output
 *
 * This is equivalent to:
 *
 *     gpio_pin_is_output(spec->port, spec->pin);
 *
 * @param spec GPIO specification from devicetree.
 *
 * @return A value from gpio_pin_is_output().
 */
static inline int gpio_pin_is_output_dt(const struct gpio_dt_spec *spec)
{
	return gpio_pin_is_output(spec->port, spec->pin);
}

/**
 * @brief Get a configuration of a single pin.
 *
 * @param port Pointer to device structure for the driver instance.
 * @param pin Pin number which configuration is get.
 * @param flags Pointer to variable in which the current configuration will
 *              be stored if function is successful.
 *
 * @retval 0 If successful.
 * @retval -ENOSYS if getting current pin configuration is not implemented
 *                  by the driver.
 * @retval -EINVAL Invalid argument.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
__syscall int gpio_pin_get_config(const struct device *port, gpio_pin_t pin,
				  gpio_flags_t *flags);

#ifdef CONFIG_GPIO_GET_CONFIG
static inline int z_impl_gpio_pin_get_config(const struct device *port,
					     gpio_pin_t pin,
					     gpio_flags_t *flags)
{
	const struct gpio_driver_api *api =
		(const struct gpio_driver_api *)port->api;
	int ret;

	SYS_PORT_TRACING_FUNC_ENTER(gpio_pin, get_config, port, pin, *flags);

	if (api->pin_get_config == NULL) {
		SYS_PORT_TRACING_FUNC_EXIT(gpio_pin, get_config, port, pin, -ENOSYS);
		return -ENOSYS;
	}

	ret = api->pin_get_config(port, pin, flags);
	SYS_PORT_TRACING_FUNC_EXIT(gpio_pin, get_config, port, pin, ret);
	return ret;
}
#endif

/**
 * @brief Get a configuration of a single pin from a @p gpio_dt_spec.
 *
 * This is equivalent to:
 *
 *     gpio_pin_get_config(spec->port, spec->pin, flags);
 *
 * @param spec GPIO specification from devicetree
 * @param flags Pointer to variable in which the current configuration will
 *              be stored if function is successful.
 * @return a value from gpio_pin_configure()
 */
static inline int gpio_pin_get_config_dt(const struct gpio_dt_spec *spec,
					gpio_flags_t *flags)
{
	return gpio_pin_get_config(spec->port, spec->pin, flags);
}

/**
 * @brief Get physical level of all input pins in a port.
 *
 * A low physical level on the pin will be interpreted as value 0. A high
 * physical level will be interpreted as value 1. This function ignores
 * GPIO_ACTIVE_LOW flag.
 *
 * Value of a pin with index n will be represented by bit n in the returned
 * port value.
 *
 * @param port Pointer to the device structure for the driver instance.
 * @param value Pointer to a variable where pin values will be stored.
 *
 * @retval 0 If successful.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
__syscall int gpio_port_get_raw(const struct device *port,
				gpio_port_value_t *value);

static inline int z_impl_gpio_port_get_raw(const struct device *port, gpio_port_value_t *value)
{
	const struct gpio_driver_api *api = (const struct gpio_driver_api *)port->api;
	int ret;

	SYS_PORT_TRACING_FUNC_ENTER(gpio_port, get_raw, port, value);

	ret = api->port_get_raw(port, value);
	SYS_PORT_TRACING_FUNC_EXIT(gpio_port, get_raw, port, ret);
	return ret;
}

/**
 * @brief Get logical level of all input pins in a port.
 *
 * Get logical level of an input pin taking into account GPIO_ACTIVE_LOW flag.
 * If pin is configured as Active High, a low physical level will be interpreted
 * as logical value 0. If pin is configured as Active Low, a low physical level
 * will be interpreted as logical value 1.
 *
 * Value of a pin with index n will be represented by bit n in the returned
 * port value.
 *
 * @param port Pointer to the device structure for the driver instance.
 * @param value Pointer to a variable where pin values will be stored.
 *
 * @retval 0 If successful.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
static inline int gpio_port_get(const struct device *port,
				gpio_port_value_t *value)
{
	const struct gpio_driver_data *const data =
			(const struct gpio_driver_data *)port->data;
	int ret;

	ret = gpio_port_get_raw(port, value);
	if (ret == 0) {
		*value ^= data->invert;
	}

	return ret;
}

/**
 * @brief Set physical level of output pins in a port.
 *
 * Writing value 0 to the pin will set it to a low physical level. Writing
 * value 1 will set it to a high physical level. This function ignores
 * GPIO_ACTIVE_LOW flag.
 *
 * Pin with index n is represented by bit n in mask and value parameter.
 *
 * @param port Pointer to the device structure for the driver instance.
 * @param mask Mask indicating which pins will be modified.
 * @param value Value assigned to the output pins.
 *
 * @retval 0 If successful.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
__syscall int gpio_port_set_masked_raw(const struct device *port,
				       gpio_port_pins_t mask,
				       gpio_port_value_t value);

static inline int z_impl_gpio_port_set_masked_raw(const struct device *port,
						  gpio_port_pins_t mask,
						  gpio_port_value_t value)
{
	const struct gpio_driver_api *api =
		(const struct gpio_driver_api *)port->api;
	int ret;

	SYS_PORT_TRACING_FUNC_ENTER(gpio_port, set_masked_raw, port, mask, value);

	ret = api->port_set_masked_raw(port, mask, value);
	SYS_PORT_TRACING_FUNC_EXIT(gpio_port, set_masked_raw, port, ret);
	return ret;
}

/**
 * @brief Set logical level of output pins in a port.
 *
 * Set logical level of an output pin taking into account GPIO_ACTIVE_LOW flag.
 * Value 0 sets the pin in logical 0 / inactive state. Value 1 sets the pin in
 * logical 1 / active state. If pin is configured as Active High, the default,
 * setting it in inactive state will force the pin to a low physical level. If
 * pin is configured as Active Low, setting it in inactive state will force the
 * pin to a high physical level.
 *
 * Pin with index n is represented by bit n in mask and value parameter.
 *
 * @param port Pointer to the device structure for the driver instance.
 * @param mask Mask indicating which pins will be modified.
 * @param value Value assigned to the output pins.
 *
 * @retval 0 If successful.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
static inline int gpio_port_set_masked(const struct device *port,
				       gpio_port_pins_t mask,
				       gpio_port_value_t value)
{
	const struct gpio_driver_data *const data =
			(const struct gpio_driver_data *)port->data;

	value ^= data->invert;

	return gpio_port_set_masked_raw(port, mask, value);
}

/**
 * @brief Set physical level of selected output pins to high.
 *
 * @param port Pointer to the device structure for the driver instance.
 * @param pins Value indicating which pins will be modified.
 *
 * @retval 0 If successful.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
__syscall int gpio_port_set_bits_raw(const struct device *port,
				     gpio_port_pins_t pins);

static inline int z_impl_gpio_port_set_bits_raw(const struct device *port,
						gpio_port_pins_t pins)
{
	const struct gpio_driver_api *api =
		(const struct gpio_driver_api *)port->api;
	int ret;

	SYS_PORT_TRACING_FUNC_ENTER(gpio_port, set_bits_raw, port, pins);

	ret = api->port_set_bits_raw(port, pins);
	SYS_PORT_TRACING_FUNC_EXIT(gpio_port, set_bits_raw, port, ret);
	return ret;
}

/**
 * @brief Set logical level of selected output pins to active.
 *
 * @param port Pointer to the device structure for the driver instance.
 * @param pins Value indicating which pins will be modified.
 *
 * @retval 0 If successful.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
static inline int gpio_port_set_bits(const struct device *port,
				     gpio_port_pins_t pins)
{
	return gpio_port_set_masked(port, pins, pins);
}

/**
 * @brief Set physical level of selected output pins to low.
 *
 * @param port Pointer to the device structure for the driver instance.
 * @param pins Value indicating which pins will be modified.
 *
 * @retval 0 If successful.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
__syscall int gpio_port_clear_bits_raw(const struct device *port,
				       gpio_port_pins_t pins);

static inline int z_impl_gpio_port_clear_bits_raw(const struct device *port,
						  gpio_port_pins_t pins)
{
	const struct gpio_driver_api *api =
		(const struct gpio_driver_api *)port->api;
	int ret;

	SYS_PORT_TRACING_FUNC_ENTER(gpio_port, clear_bits_raw, port, pins);

	ret = api->port_clear_bits_raw(port, pins);
	SYS_PORT_TRACING_FUNC_EXIT(gpio_port, clear_bits_raw, port, ret);
	return ret;
}

/**
 * @brief Set logical level of selected output pins to inactive.
 *
 * @param port Pointer to the device structure for the driver instance.
 * @param pins Value indicating which pins will be modified.
 *
 * @retval 0 If successful.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
static inline int gpio_port_clear_bits(const struct device *port,
				       gpio_port_pins_t pins)
{
	return gpio_port_set_masked(port, pins, 0);
}

/**
 * @brief Toggle level of selected output pins.
 *
 * @param port Pointer to the device structure for the driver instance.
 * @param pins Value indicating which pins will be modified.
 *
 * @retval 0 If successful.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
__syscall int gpio_port_toggle_bits(const struct device *port,
				    gpio_port_pins_t pins);

static inline int z_impl_gpio_port_toggle_bits(const struct device *port,
					       gpio_port_pins_t pins)
{
	const struct gpio_driver_api *api =
		(const struct gpio_driver_api *)port->api;
	int ret;

	SYS_PORT_TRACING_FUNC_ENTER(gpio_port, toggle_bits, port, pins);

	ret = api->port_toggle_bits(port, pins);
	SYS_PORT_TRACING_FUNC_EXIT(gpio_port, toggle_bits, port, ret);
	return ret;
}

/**
 * @brief Set physical level of selected output pins.
 *
 * @param port Pointer to the device structure for the driver instance.
 * @param set_pins Value indicating which pins will be set to high.
 * @param clear_pins Value indicating which pins will be set to low.
 *
 * @retval 0 If successful.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
static inline int gpio_port_set_clr_bits_raw(const struct device *port,
					     gpio_port_pins_t set_pins,
					     gpio_port_pins_t clear_pins)
{
	__ASSERT((set_pins & clear_pins) == 0, "Set and Clear pins overlap");

	return gpio_port_set_masked_raw(port, set_pins | clear_pins, set_pins);
}

/**
 * @brief Set logical level of selected output pins.
 *
 * @param port Pointer to the device structure for the driver instance.
 * @param set_pins Value indicating which pins will be set to active.
 * @param clear_pins Value indicating which pins will be set to inactive.
 *
 * @retval 0 If successful.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
static inline int gpio_port_set_clr_bits(const struct device *port,
					 gpio_port_pins_t set_pins,
					 gpio_port_pins_t clear_pins)
{
	__ASSERT((set_pins & clear_pins) == 0, "Set and Clear pins overlap");

	return gpio_port_set_masked(port, set_pins | clear_pins, set_pins);
}

/**
 * @brief Get physical level of an input pin.
 *
 * A low physical level on the pin will be interpreted as value 0. A high
 * physical level will be interpreted as value 1. This function ignores
 * GPIO_ACTIVE_LOW flag.
 *
 * @param port Pointer to the device structure for the driver instance.
 * @param pin Pin number.
 *
 * @retval 1 If pin physical level is high.
 * @retval 0 If pin physical level is low.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
static inline int gpio_pin_get_raw(const struct device *port, gpio_pin_t pin)
{
	__unused const struct gpio_driver_config *const cfg =
		(const struct gpio_driver_config *)port->config;
	gpio_port_value_t value;
	int ret;

	__ASSERT((cfg->port_pin_mask & (gpio_port_pins_t)BIT(pin)) != 0U,
		 "Unsupported pin");

	ret = gpio_port_get_raw(port, &value);
	if (ret == 0) {
		ret = (value & (gpio_port_pins_t)BIT(pin)) != 0 ? 1 : 0;
	}

	return ret;
}

/**
 * @brief Get logical level of an input pin.
 *
 * Get logical level of an input pin taking into account GPIO_ACTIVE_LOW flag.
 * If pin is configured as Active High, a low physical level will be interpreted
 * as logical value 0. If pin is configured as Active Low, a low physical level
 * will be interpreted as logical value 1.
 *
 * Note: If pin is configured as Active High, the default, gpio_pin_get()
 *       function is equivalent to gpio_pin_get_raw().
 *
 * @param port Pointer to the device structure for the driver instance.
 * @param pin Pin number.
 *
 * @retval 1 If pin logical value is 1 / active.
 * @retval 0 If pin logical value is 0 / inactive.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
static inline int gpio_pin_get(const struct device *port, gpio_pin_t pin)
{
	__unused const struct gpio_driver_config *const cfg =
		(const struct gpio_driver_config *)port->config;
	gpio_port_value_t value;
	int ret;

	__ASSERT((cfg->port_pin_mask & (gpio_port_pins_t)BIT(pin)) != 0U,
		 "Unsupported pin");

	ret = gpio_port_get(port, &value);
	if (ret == 0) {
		ret = (value & (gpio_port_pins_t)BIT(pin)) != 0 ? 1 : 0;
	}

	return ret;
}

/**
 * @brief Get logical level of an input pin from a @p gpio_dt_spec.
 *
 * This is equivalent to:
 *
 *     gpio_pin_get(spec->port, spec->pin);
 *
 * @param spec GPIO specification from devicetree
 * @return a value from gpio_pin_get()
 */
static inline int gpio_pin_get_dt(const struct gpio_dt_spec *spec)
{
	return gpio_pin_get(spec->port, spec->pin);
}

/**
 * @brief Set physical level of an output pin.
 *
 * Writing value 0 to the pin will set it to a low physical level. Writing any
 * value other than 0 will set it to a high physical level. This function
 * ignores GPIO_ACTIVE_LOW flag.
 *
 * @param port Pointer to the device structure for the driver instance.
 * @param pin Pin number.
 * @param value Value assigned to the pin.
 *
 * @retval 0 If successful.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
static inline int gpio_pin_set_raw(const struct device *port, gpio_pin_t pin,
				   int value)
{
	__unused const struct gpio_driver_config *const cfg =
		(const struct gpio_driver_config *)port->config;
	int ret;

	__ASSERT((cfg->port_pin_mask & (gpio_port_pins_t)BIT(pin)) != 0U,
		 "Unsupported pin");

	if (value != 0)	{
		ret = gpio_port_set_bits_raw(port, (gpio_port_pins_t)BIT(pin));
	} else {
		ret = gpio_port_clear_bits_raw(port, (gpio_port_pins_t)BIT(pin));
	}

	return ret;
}

/**
 * @brief Set logical level of an output pin.
 *
 * Set logical level of an output pin taking into account GPIO_ACTIVE_LOW flag.
 * Value 0 sets the pin in logical 0 / inactive state. Any value other than 0
 * sets the pin in logical 1 / active state. If pin is configured as Active
 * High, the default, setting it in inactive state will force the pin to a low
 * physical level. If pin is configured as Active Low, setting it in inactive
 * state will force the pin to a high physical level.
 *
 * Note: If pin is configured as Active High, gpio_pin_set() function is
 *       equivalent to gpio_pin_set_raw().
 *
 * @param port Pointer to the device structure for the driver instance.
 * @param pin Pin number.
 * @param value Value assigned to the pin.
 *
 * @retval 0 If successful.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
static inline int gpio_pin_set(const struct device *port, gpio_pin_t pin,
			       int value)
{
	__unused const struct gpio_driver_config *const cfg =
		(const struct gpio_driver_config *)port->config;
	const struct gpio_driver_data *const data =
			(const struct gpio_driver_data *)port->data;

	__ASSERT((cfg->port_pin_mask & (gpio_port_pins_t)BIT(pin)) != 0U,
		 "Unsupported pin");

	if (data->invert & (gpio_port_pins_t)BIT(pin)) {
		value = (value != 0) ? 0 : 1;
	}

	return gpio_pin_set_raw(port, pin, value);
}

/**
 * @brief Set logical level of a output pin from a @p gpio_dt_spec.
 *
 * This is equivalent to:
 *
 *     gpio_pin_set(spec->port, spec->pin, value);
 *
 * @param spec GPIO specification from devicetree
 * @param value Value assigned to the pin.
 * @return a value from gpio_pin_set()
 */
static inline int gpio_pin_set_dt(const struct gpio_dt_spec *spec, int value)
{
	return gpio_pin_set(spec->port, spec->pin, value);
}

/**
 * @brief Toggle pin level.
 *
 * @param port Pointer to the device structure for the driver instance.
 * @param pin Pin number.
 *
 * @retval 0 If successful.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
static inline int gpio_pin_toggle(const struct device *port, gpio_pin_t pin)
{
	__unused const struct gpio_driver_config *const cfg =
		(const struct gpio_driver_config *)port->config;

	__ASSERT((cfg->port_pin_mask & (gpio_port_pins_t)BIT(pin)) != 0U,
		 "Unsupported pin");

	return gpio_port_toggle_bits(port, (gpio_port_pins_t)BIT(pin));
}

/**
 * @brief Toggle pin level from a @p gpio_dt_spec.
 *
 * This is equivalent to:
 *
 *     gpio_pin_toggle(spec->port, spec->pin);
 *
 * @param spec GPIO specification from devicetree
 * @return a value from gpio_pin_toggle()
 */
static inline int gpio_pin_toggle_dt(const struct gpio_dt_spec *spec)
{
	return gpio_pin_toggle(spec->port, spec->pin);
}

/**
 * @brief Helper to initialize a struct gpio_callback properly
 * @param callback A valid Application's callback structure pointer.
 * @param handler A valid handler function pointer.
 * @param pin_mask A bit mask of relevant pins for the handler
 */
static inline void gpio_init_callback(struct gpio_callback *callback,
				      gpio_callback_handler_t handler,
				      gpio_port_pins_t pin_mask)
{
	SYS_PORT_TRACING_FUNC_ENTER(gpio, init_callback, callback, handler, pin_mask);

	__ASSERT(callback, "Callback pointer should not be NULL");
	__ASSERT(handler, "Callback handler pointer should not be NULL");

	callback->handler = handler;
	callback->pin_mask = pin_mask;

	SYS_PORT_TRACING_FUNC_EXIT(gpio, init_callback, callback);
}

/**
 * @brief Add an application callback.
 * @param port Pointer to the device structure for the driver instance.
 * @param callback A valid Application's callback structure pointer.
 * @retval 0 If successful
 * @retval -ENOSYS If driver does not implement the operation
 * @retval -errno Other negative errno code on failure.
 *
 * @note Callbacks may be added to the device from within a callback
 * handler invocation, but whether they are invoked for the current
 * GPIO event is not specified.
 *
 * Note: enables to add as many callback as needed on the same port.
 */
static inline int gpio_add_callback(const struct device *port,
				    struct gpio_callback *callback)
{
	const struct gpio_driver_api *api =
		(const struct gpio_driver_api *)port->api;
	int ret;

	SYS_PORT_TRACING_FUNC_ENTER(gpio, add_callback, port, callback);

	if (api->manage_callback == NULL) {
		SYS_PORT_TRACING_FUNC_EXIT(gpio, add_callback, port, -ENOSYS);
		return -ENOSYS;
	}

	ret = api->manage_callback(port, callback, true);
	SYS_PORT_TRACING_FUNC_EXIT(gpio, add_callback, port, ret);
	return ret;
}

/**
 * @brief Add an application callback.
 *
 * This is equivalent to:
 *
 *     gpio_add_callback(spec->port, callback);
 *
 * @param spec GPIO specification from devicetree.
 * @param callback A valid application's callback structure pointer.
 * @return a value from gpio_add_callback().
 */
static inline int gpio_add_callback_dt(const struct gpio_dt_spec *spec,
				       struct gpio_callback *callback)
{
	return gpio_add_callback(spec->port, callback);
}

/**
 * @brief Remove an application callback.
 * @param port Pointer to the device structure for the driver instance.
 * @param callback A valid application's callback structure pointer.
 * @retval 0 If successful
 * @retval -ENOSYS If driver does not implement the operation
 * @retval -errno Other negative errno code on failure.
 *
 * @warning It is explicitly permitted, within a callback handler, to
 * remove the registration for the callback that is running, i.e. @p
 * callback.  Attempts to remove other registrations on the same
 * device may result in undefined behavior, including failure to
 * invoke callbacks that remain registered and unintended invocation
 * of removed callbacks.
 *
 * Note: enables to remove as many callbacks as added through
 *       gpio_add_callback().
 */
static inline int gpio_remove_callback(const struct device *port,
				       struct gpio_callback *callback)
{
	const struct gpio_driver_api *api =
		(const struct gpio_driver_api *)port->api;
	int ret;

	SYS_PORT_TRACING_FUNC_ENTER(gpio, remove_callback, port, callback);

	if (api->manage_callback == NULL) {
		SYS_PORT_TRACING_FUNC_EXIT(gpio, remove_callback, port, -ENOSYS);
		return -ENOSYS;
	}

	ret = api->manage_callback(port, callback, false);
	SYS_PORT_TRACING_FUNC_EXIT(gpio, remove_callback, port, ret);
	return ret;
}

/**
 * @brief Remove an application callback.
 *
 * This is equivalent to:
 *
 *     gpio_remove_callback(spec->port, callback);
 *
 * @param spec GPIO specification from devicetree.
 * @param callback A valid application's callback structure pointer.
 * @return a value from gpio_remove_callback().
 */
static inline int gpio_remove_callback_dt(const struct gpio_dt_spec *spec,
					  struct gpio_callback *callback)
{
	return gpio_remove_callback(spec->port, callback);
}

/**
 * @brief Function to get pending interrupts
 *
 * The purpose of this function is to return the interrupt
 * status register for the device.
 * This is especially useful when waking up from
 * low power states to check the wake up source.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval status != 0 if at least one gpio interrupt is pending.
 * @retval 0 if no gpio interrupt is pending.
 * @retval -ENOSYS If driver does not implement the operation
 */
__syscall int gpio_get_pending_int(const struct device *dev);

static inline int z_impl_gpio_get_pending_int(const struct device *dev)
{
	const struct gpio_driver_api *api =
		(const struct gpio_driver_api *)dev->api;
	int ret;

	SYS_PORT_TRACING_FUNC_ENTER(gpio, get_pending_int, dev);

	if (api->get_pending_int == NULL) {
		SYS_PORT_TRACING_FUNC_EXIT(gpio, get_pending_int, dev, -ENOSYS);
		return -ENOSYS;
	}

	ret = api->get_pending_int(dev);
	SYS_PORT_TRACING_FUNC_EXIT(gpio, get_pending_int, dev, ret);
	return ret;
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/gpio.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_GPIO_H_ */
