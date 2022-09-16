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

#include <zephyr/sys/__assert.h>
#include <zephyr/sys/slist.h>

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
 * @ingroup io_interfaces
 * @{
 */

/**
 * @deprecated Use the GPIO controller/SoC specific `*_GPIO_DEBOUNCE` flag instead.
 */
#define GPIO_INT_DEBOUNCE (1U << 8) __DEPRECATED_MACRO

/**
 * @deprecated Use the GPIO controller/SoC specific `*_GPIO_DS_*` flags instead.
 * @{
 */
/** @cond INTERNAL_HIDDEN */
#define GPIO_DS_LOW_POS   9                                      __DEPRECATED_MACRO
#define GPIO_DS_LOW_MASK  (0x1U << GPIO_DS_LOW_POS)              __DEPRECATED_MACRO
#define GPIO_DS_HIGH_POS  10                                     __DEPRECATED_MACRO
#define GPIO_DS_HIGH_MASK (0x1U << GPIO_DS_HIGH_POS)             __DEPRECATED_MACRO
#define GPIO_DS_MASK      (GPIO_DS_LOW_MASK | GPIO_DS_HIGH_MASK) __DEPRECATED_MACRO
/** @endcond */
#define GPIO_DS_DFLT_LOW  (0x0U << GPIO_DS_LOW_POS)              __DEPRECATED_MACRO
#define GPIO_DS_ALT_LOW   (0x1U << GPIO_DS_LOW_POS)              __DEPRECATED_MACRO
#define GPIO_DS_DFLT_HIGH (0x0U << GPIO_DS_HIGH_POS)             __DEPRECATED_MACRO
#define GPIO_DS_ALT_HIGH  (0x1U << GPIO_DS_HIGH_POS)             __DEPRECATED_MACRO
#define GPIO_DS_DFLT      (GPIO_DS_DFLT_LOW | GPIO_DS_DFLT_HIGH) __DEPRECATED_MACRO
#define GPIO_DS_ALT       (GPIO_DS_ALT_LOW | GPIO_DS_ALT_HIGH)   __DEPRECATED_MACRO
/** @} */

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
 * property 'prop', this expands to
 * <tt>GPIO_DT_SPEC_GET_BY_IDX(node_id, prop, idx)</tt>. The @p
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
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, prop),			\
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
#define GPIO_DT_SPEC_INST_GET_BY_IDX_OR(inst, prop, idx, default_value)	\
	GPIO_DT_SPEC_GET_BY_IDX_OR(DT_DRV_INST(inst), prop, idx, default_value)

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
	/* Mask identifying pins supported by the controller.
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
	/* Mask identifying pins that are configured as active low.
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
};

enum gpio_int_trig {
	/* Trigger detection when input state is (or transitions to)
	 * physical low. (Edge Failing or Active Low) */
	GPIO_INT_TRIG_LOW = GPIO_INT_LOW_0,
	/* Trigger detection when input state is (or transitions to)
	 * physical high. (Edge Rising or Active High) */
	GPIO_INT_TRIG_HIGH = GPIO_INT_HIGH_1,
	/* Trigger detection on pin rising or falling edge. */
	GPIO_INT_TRIG_BOTH = GPIO_INT_LOW_0 | GPIO_INT_HIGH_1,
};

__subsystem struct gpio_driver_api {
	int (*pin_configure)(const struct device *port, gpio_pin_t pin,
			     gpio_flags_t flags);
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
				       enum gpio_int_mode, enum gpio_int_trig);
	int (*manage_callback)(const struct device *port,
			       struct gpio_callback *cb,
			       bool set);
	uint32_t (*get_pending_int)(const struct device *dev);
};

/**
 * @endcond
 */

/**
 * @brief Configure pin interrupt.
 *
 * @note This function can also be used to configure interrupts on pins
 *       not controlled directly by the GPIO module. That is, pins which are
 *       routed to other modules such as I2C, SPI, UART.
 *
 * @param port Pointer to device structure for the driver instance.
 * @param pin Pin number.
 * @param flags Interrupt configuration flags as defined by GPIO_INT_*.
 *
 * @retval 0 If successful.
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
	const struct gpio_driver_config *const cfg =
		(const struct gpio_driver_config *)port->config;
	const struct gpio_driver_data *const data =
		(const struct gpio_driver_data *)port->data;
	enum gpio_int_trig trig;
	enum gpio_int_mode mode;

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
		 ((flags & (GPIO_INT_LOW_0 | GPIO_INT_HIGH_1)) != 0),
		 "At least one of GPIO_INT_LOW_0, GPIO_INT_HIGH_1 has to be "
		 "enabled.");

	(void)cfg;
	__ASSERT((cfg->port_pin_mask & (gpio_port_pins_t)BIT(pin)) != 0U,
		 "Unsupported pin");

	if (((flags & GPIO_INT_LEVELS_LOGICAL) != 0) &&
	    ((data->invert & (gpio_port_pins_t)BIT(pin)) != 0)) {
		/* Invert signal bits */
		flags ^= (GPIO_INT_LOW_0 | GPIO_INT_HIGH_1);
	}

	trig = (enum gpio_int_trig)(flags & (GPIO_INT_LOW_0 | GPIO_INT_HIGH_1));
	mode = (enum gpio_int_mode)(flags & (GPIO_INT_EDGE | GPIO_INT_DISABLE | GPIO_INT_ENABLE));

	return api->pin_interrupt_configure(port, pin, mode, trig);
}

/**
 * @brief Configure pin interrupts from a @p gpio_dt_spec.
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
	const struct gpio_driver_config *const cfg =
		(const struct gpio_driver_config *)port->config;
	struct gpio_driver_data *data =
		(struct gpio_driver_data *)port->data;

	__ASSERT((flags & GPIO_INT_MASK) == 0,
		 "Interrupt flags are not supported");

	__ASSERT((flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) !=
		 (GPIO_PULL_UP | GPIO_PULL_DOWN),
		 "Pull Up and Pull Down should not be enabled simultaneously");

	__ASSERT((flags & GPIO_OUTPUT) != 0 || (flags & GPIO_SINGLE_ENDED) == 0,
		 "Output needs to be enabled for 'Open Drain', 'Open Source' "
		 "mode to be supported");

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

	(void)cfg;
	__ASSERT((cfg->port_pin_mask & (gpio_port_pins_t)BIT(pin)) != 0U,
		 "Unsupported pin");

	if ((flags & GPIO_ACTIVE_LOW) != 0) {
		data->invert |= (gpio_port_pins_t)BIT(pin);
	} else {
		data->invert &= ~(gpio_port_pins_t)BIT(pin);
	}

	return api->pin_configure(port, pin, flags);
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

static inline int z_impl_gpio_port_get_raw(const struct device *port,
					   gpio_port_value_t *value)
{
	const struct gpio_driver_api *api =
		(const struct gpio_driver_api *)port->api;

	return api->port_get_raw(port, value);
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

	return api->port_set_masked_raw(port, mask, value);
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

	return api->port_set_bits_raw(port, pins);
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

	return api->port_clear_bits_raw(port, pins);
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

	return api->port_toggle_bits(port, pins);
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
	const struct gpio_driver_config *const cfg =
		(const struct gpio_driver_config *)port->config;
	gpio_port_value_t value;
	int ret;

	(void)cfg;
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
	const struct gpio_driver_config *const cfg =
		(const struct gpio_driver_config *)port->config;
	gpio_port_value_t value;
	int ret;

	(void)cfg;
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
	const struct gpio_driver_config *const cfg =
		(const struct gpio_driver_config *)port->config;
	int ret;

	(void)cfg;
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
	const struct gpio_driver_config *const cfg =
		(const struct gpio_driver_config *)port->config;
	const struct gpio_driver_data *const data =
			(const struct gpio_driver_data *)port->data;

	(void)cfg;
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
	const struct gpio_driver_config *const cfg =
		(const struct gpio_driver_config *)port->config;

	(void)cfg;
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
	__ASSERT(callback, "Callback pointer should not be NULL");
	__ASSERT(handler, "Callback handler pointer should not be NULL");

	callback->handler = handler;
	callback->pin_mask = pin_mask;
}

/**
 * @brief Add an application callback.
 * @param port Pointer to the device structure for the driver instance.
 * @param callback A valid Application's callback structure pointer.
 * @return 0 if successful, negative errno code on failure.
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

	if (api->manage_callback == NULL) {
		return -ENOTSUP;
	}

	return api->manage_callback(port, callback, true);
}

/**
 * @brief Remove an application callback.
 * @param port Pointer to the device structure for the driver instance.
 * @param callback A valid application's callback structure pointer.
 * @return 0 if successful, negative errno code on failure.
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

	if (api->manage_callback == NULL) {
		return -ENOTSUP;
	}

	return api->manage_callback(port, callback, false);
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
 */
__syscall int gpio_get_pending_int(const struct device *dev);

static inline int z_impl_gpio_get_pending_int(const struct device *dev)
{
	const struct gpio_driver_api *api =
		(const struct gpio_driver_api *)dev->api;

	if (api->get_pending_int == NULL) {
		return -ENOTSUP;
	}

	return api->get_pending_int(dev);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <syscalls/gpio.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_GPIO_H_ */
