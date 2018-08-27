/*
 * Copyright (c) 2018 Bobby Noelte
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * Code generation template for PINCTRL drivers
 */

/**
 * @brief PINCTRL template.
 * @defgroup pinctrl_template PINCTRL template.
 * @ingroup device_driver_support
 *
 * Code generation template for PINCTRL drivers.
 *
 * Usage example:
 * compatible = 'st,stm32-pinctrl'
 * data_info = 'struct pinctrl_stm32_data'
 * config_get = 'pinctrl_stm32_config_get'
 * config_set = 'pinctrl_stm32_config_set'
 * mux_get = 'pinctrl_stm32_mux_get'
 * mux_set = 'pinctrl_stm32_mux_set'
 * device_init = 'pinctrl_stm32_device_init'
 * codegen.out_include('templates/drivers/pinctrl_tmpl.c')
 *
 * The template expects the following globals to be set:
 * - compatible
 *   The compatible string of the driver (e.g. 'st,stm32-pinctrl')
 * - data_info
 *   device data type definition (e.g. 'struct pinctrl_stm32_data')
 * - config_get
 *   C function name of device config_get function.
 * - mux_free
 *   C function name of device mux_free function.
 * - mux_get
 *   C function name of device mux_get function.
 * - mux_set
 *   C function name of device mux_set function.
 * - device_init
 *   C function name of device init function
 *
 * @{
 */

/** Helper functions for code generation
 * @code{.codegen}
 *
 * # template shall only be included once
 * codegen.guard_include()
 *
 * # Pin controller device tree data access
 * codegen.import_module('pincontroller')
 *
 * _pin_controllers = \
 *     pincontroller.PinController.create_all_compatible(compatible)
 *
 * codegen.outl("/" + "* {} x '{}' pin controller devices *" \
 *                  .format(len(_pin_controllers), compatible) + "/")
 *
 * @endcode{.codegen}
 */
/** @code{.codeins}@endcode */

#undef SYS_LOG_LEVEL
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_PINCTRL_LEVEL
#include <logging/sys_log.h>

#include <device.h>
#include <gpio.h>
#include <pinctrl.h>

/** Define template types for minimum footprint.
 * @code{.codegen}
 *
 * if pincontroller.PinController.max_pin_count() < 256:
 *     codegen.outl("typedef u8_t pinctrl_tmpl_pin_t;")
 * else:
 *     codegen.outl("typedef u16_t pinctrl_tmpl_pin_t;")
 * if pincontroller.PinController.max_function_count() < 256:
 *     codegen.outl("typedef u8_t pinctrl_tmpl_mux_t;")
 *     codegen.outl("typedef u8_t pinctrl_tmpl_function_id_t;")
 * else:
 *     codegen.outl("typedef u16_t pinctrl_tmpl_mux_t;")
 *     codegen.outl("typedef u16_t pinctrl_tmpl_function_id_t;")
 * if pincontroller.PinController.max_state_name_count() < 256:
 *     codegen.outl("typedef u8_t pinctrl_tmpl_state_name_id_t;")
 * else:
 *     codegen.outl("typedef u16_t pinctrl_tmpl_state_name_id_t;")
 * if pincontroller.PinController.max_state_count() < 256:
 *     codegen.outl("typedef u8_t pinctrl_tmpl_state_id_t;")
 * else:
 *     codegen.outl("typedef u16_t pinctrl_tmpl_state_id_t;")
 * if pincontroller.PinController.max_pinctrl_count() < 256:
 *     codegen.outl("typedef u8_t pinctrl_tmpl_pinctrl_id_t;")
 * else:
 *     codegen.outl("typedef u16_t pinctrl_tmpl_pinctrl_id_t;")
 * @endcode{.codegen}
 */
/** @code{.codeins}@endcode */

#if CONFIG_PINCTRL_RUNTIME_DTS

/**
 * @brief Device function info.
 */
struct pinctrl_tmpl_device_function {
	const char *name;
};

/**
 * @brief State name info.
 */
struct pinctrl_tmpl_state_name {
	const char *name;
};

/**
 * @brief Pinctrl state info.
 */
struct pinctrl_tmpl_pinctrl_state {
	pinctrl_tmpl_function_id_t device_function;
	pinctrl_tmpl_state_name_id_t name;
};

#endif /* CONFIG_PINCTRL_RUNTIME_DTS */

/**
 * @brief Pinctrl info.
 */
struct pinctrl_tmpl_pinctrl {
	u32_t config;
	pinctrl_tmpl_pin_t pin;		/* pin ID */
	pinctrl_tmpl_mux_t mux;
#if CONFIG_PINCTRL_RUNTIME_DTS
	pinctrl_tmpl_state_id_t state;
#endif /* CONFIG_PINCTRL_RUNTIME_DTS */
};

/**
 * @brief GPIO range info.
 */
struct pinctrl_tmpl_gpio_range {
	const char *name;
	pinctrl_tmpl_pin_t base;
	u8_t gpio_base_idx;
	u8_t npins;
};

/**
 * @brief Pin controller configuration info.
 */
struct pinctrl_tmpl_config {
#if CONFIG_PINCTRL_RUNTIME_DTS
	const struct pinctrl_tmpl_device_function *device_function_data;
	const struct pinctrl_tmpl_state_name *state_name_data;
	const struct pinctrl_tmpl_pinctrl_state *pinctrl_state_data;
	u8_t *mux_pinmap;
#endif /* CONFIG_PINCTRL_RUNTIME_DTS */
	const struct pinctrl_tmpl_pinctrl *pinctrl_data;
	const struct pinctrl_tmpl_gpio_range *gpio_range_data;
	int (*device_init)(struct device *dev);
	int (*mux_set)(struct device *dev, u16_t pin, u16_t func);
	pinctrl_tmpl_pin_t pin_count;
#if CONFIG_PINCTRL_RUNTIME_DTS
	pinctrl_tmpl_function_id_t device_function_count;
	pinctrl_tmpl_state_name_id_t state_name_count;
	pinctrl_tmpl_state_id_t pinctrl_state_count;
#endif /* CONFIG_PINCTRL_RUNTIME_DTS */
	pinctrl_tmpl_pinctrl_id_t pinctrl_count;
	u8_t gpio_range_count;
};

static inline const struct pinctrl_tmpl_config
	*pinctrl_tmpl_device_config_info(struct device *dev)
{
	return (const struct pinctrl_tmpl_config *) dev->config->config_info;
}

#if CONFIG_PINCTRL_RUNTIME_DTS

static inline pinctrl_tmpl_function_id_t
	pinctrl_tmpl_device_function_id(u16_t func)
{
	return func - PINCTRL_FUNCTION_DEVICE_BASE;
}

static int pinctrl_tmpl_is_device_function(struct device *dev, u16_t func)
{
	const struct pinctrl_tmpl_config *config =
		pinctrl_tmpl_device_config_info(dev);

	if (func < PINCTRL_FUNCTION_DEVICE_BASE) {
		return 0;
	}
	func = pinctrl_tmpl_device_function_id(func);
	if (func >= config->device_function_count) {
		return 0;
	}
	return 1;
}

/**
 * @brief Pin multiplex owner info
 */
struct pinctrl_tmpl_mux_owner {
	const char *name;
};

/**
 * @brief Pin multiplex owner data
 */
struct pinctrl_tmpl_mux_owner pinctrl_tmpl_mux_owner_data[15];

static const char *pinctrl_tmpl_mux_owner_get(struct device *dev, u16_t pin)
{
	const struct pinctrl_tmpl_config *config =
		pinctrl_tmpl_device_config_info(dev);

	int pinmap_idx = pin >> 1;
	u8_t owner_idx = config->mux_pinmap[pinmap_idx];

	if (pin & 0x01) {
		owner_idx = owner_idx >> 4;
	} else {
		owner_idx &= 0x0F;
	}
	if (owner_idx >= 15) {
		/* no owner */
		return 0;
	}
	return pinctrl_tmpl_mux_owner_data[owner_idx].name;
}

static int pinctrl_tmpl_mux_owner_set(struct device *dev, u16_t pin,
				     const char *owner)
{
	const struct pinctrl_tmpl_config *config =
		pinctrl_tmpl_device_config_info(dev);

	int pinmap_idx = pin >> 1;
	u8_t owner_idx;

	if (owner == 0) {
		/* No owner */
		owner_idx = 15;
	} else {
		for (owner_idx = 0; owner_idx < 15; owner_idx++) {
			const char *name =
				pinctrl_tmpl_mux_owner_data[owner_idx].name;
			if (name && (strcmp(name, owner) == 0)) {
				break;
			}
		}
		if (owner_idx >= 15) {
			/* owner unknown - add it */
			for (owner_idx = 0; owner_idx < 15; owner_idx++) {
				if (pinctrl_tmpl_mux_owner_data[owner_idx]
				    .name == 0) {
					pinctrl_tmpl_mux_owner_data[owner_idx]
						.name = owner;
					break;
				}
			}
		}
		if (owner_idx >= 15) {
			/* no more memory for owner info */
			return -ENOMEM;
		}
	}
	if (pin & 0x01) {
		config->mux_pinmap[pinmap_idx] =
			(config->mux_pinmap[pinmap_idx] & 0x0F) |
			(owner_idx << 4);
	} else {
		config->mux_pinmap[pinmap_idx] =
			(config->mux_pinmap[pinmap_idx] & 0xF0) | owner_idx;
	}
	return 0;
}

static void pinctrl_tmpl_mux_owner_init(void)
{
	for (int owner_idx = 0; owner_idx < 15; owner_idx++) {
		pinctrl_tmpl_mux_owner_data[owner_idx].name = 0;
	}
}

static u8_t pinctrl_tmpl_mux_owner_initialized; /* initialized to 0 */

static int pinctrl_tmpl_mux_request_init(struct device *dev)
{
	const struct pinctrl_tmpl_config *config =
		pinctrl_tmpl_device_config_info(dev);

	if (!pinctrl_tmpl_mux_owner_initialized) {
		pinctrl_tmpl_mux_owner_init();
		pinctrl_tmpl_mux_owner_initialized = 1;
	}
	for (int i = 0; i < config->pin_count / 2; i++) {
		config->mux_pinmap[i] = 0xFF;
	}
	return 0;
}

#endif /* CONFIG_PINCTRL_RUNTIME_DTS */

/* --- API --- */

static u16_t pinctrl_tmpl_control_get_pins_count(struct device *dev)
{
	return pinctrl_tmpl_device_config_info(dev)->pin_count;
}

#if CONFIG_PINCTRL_RUNTIME_DTS

static int pinctrl_tmpl_control_get_groups_count(struct device *dev)
{
	return pinctrl_tmpl_device_config_info(dev)->pinctrl_state_count;
}

/*
 * @note The pin group is defined by the pinctrl state.
 *       The group number equals the pinctrl state index.
 */
static int pinctrl_tmpl_control_get_group_pins(struct device *dev, u16_t group,
					      u16_t *pins, u16_t *num_pins)
{
	const struct pinctrl_tmpl_config *config =
		pinctrl_tmpl_device_config_info(dev);

	pinctrl_tmpl_pin_t num_pins_found = 0;

	for (int i = 0; i < config->pinctrl_count; i++) {
		if (config->pinctrl_data[i].state == group) {
			if (num_pins_found < *num_pins) {
				pins[num_pins_found] =
					config->pinctrl_data[i].pin;
			}
			num_pins_found++;
		}
	}
	if (num_pins_found == 0) {
		*num_pins = 0;
		return -ENOTSUP;
	}
	if (num_pins_found > *num_pins) {
		*num_pins = num_pins_found;
		return -EINVAL;
	}
	*num_pins = num_pins_found;
	return 0;
}

static u16_t pinctrl_tmpl_control_get_states_count(struct device *dev)
{
	return pinctrl_tmpl_device_config_info(dev)->pinctrl_state_count;
}

/*
 * @note Every state is regarded a group. Resulting multi definition
 *       of the same set of pins is accepted for the sake of code simplicity.
 */
static int pinctrl_tmpl_control_get_state_group(struct device *dev,
						u16_t state, u16_t *group)
{
	const struct pinctrl_tmpl_config *config =
		pinctrl_tmpl_device_config_info(dev);

	if (state >= config->pinctrl_state_count) {
		return -ENOTSUP;
	}
	*group = state;
	return 0;
}

static u16_t pinctrl_tmpl_control_get_functions_count(struct device *dev)
{
	return pinctrl_tmpl_device_config_info(dev)->device_function_count;
}

static int pinctrl_tmpl_control_get_function_group(struct device *dev,
						   u16_t func,
						   const char *name,
						   u16_t *group)
{
	const struct pinctrl_tmpl_config *config =
		pinctrl_tmpl_device_config_info(dev);

	if (!pinctrl_tmpl_is_device_function(dev, func)) {
		return -ENODEV;
	}

	func = pinctrl_tmpl_device_function_id(func);
	for (u16_t i = 0; i < config->pinctrl_state_count; i++) {
		/* state name is name also used as group name */
		const char *group_name =
			config->state_name_data[config->pinctrl_state_data[i]
						.name].name;

		if ((func == config->pinctrl_state_data[i].device_function) &&
		    (strcmp(name, group_name) == 0)) {
			*group = i;
			return 0;
		}
	}
	return -ENOTSUP;
}

static int pinctrl_tmpl_control_get_function_groups(struct device *dev,
						   u16_t func, u16_t *groups,
						   u16_t *num_groups)
{
	const struct pinctrl_tmpl_config *config =
		pinctrl_tmpl_device_config_info(dev);

	if (!pinctrl_tmpl_is_device_function(dev, func)) {
		*num_groups = 0;
		return -ENODEV;
	}

	func = pinctrl_tmpl_device_function_id(func);
	u16_t num_groups_found = 0;

	for (u16_t i = 0; i < config->pinctrl_state_count; i++) {
		if (func == config->pinctrl_state_data[i].device_function) {
			if (num_groups_found < *num_groups) {
				groups[num_groups_found] = i;
			}
			num_groups_found++;
		}
	}
	if (num_groups_found == 0) {
		*num_groups = 0;
		return -ENOTSUP;
	}
	if (num_groups_found > *num_groups) {
		*num_groups = num_groups_found;
		return -EINVAL;
	}
	*num_groups = num_groups_found;
	return 0;
}

static int pinctrl_tmpl_control_get_function_state(struct device *dev,
						   u16_t func,
						   const char *name,
						   u16_t *state)
{
	const struct pinctrl_tmpl_config *config =
		pinctrl_tmpl_device_config_info(dev);

	if (!pinctrl_tmpl_is_device_function(dev, func)) {
		return -ENODEV;
	}

	func = pinctrl_tmpl_device_function_id(func);

	for (pinctrl_tmpl_state_id_t i = 0;
	     i < config->pinctrl_state_count; i++) {
		u16_t function = config->pinctrl_state_data[i].device_function;
		const char *state_name =
			config->state_name_data[config->pinctrl_state_data[i]
						.name].name;

		if ((func == function) && (strcmp(name, state_name) == 0)) {
			*state = i;
			return 0;
		}
	}

	return -ENOTSUP;
}

static int pinctrl_tmpl_control_get_function_states(struct device *dev,
						    u16_t func, u16_t *states,
						    u16_t *num_states)
{
	const struct pinctrl_tmpl_config *config =
		pinctrl_tmpl_device_config_info(dev);

	if (!pinctrl_tmpl_is_device_function(dev, func)) {
		*num_states = 0;
		return -ENODEV;
	}

	func = pinctrl_tmpl_device_function_id(func);
	pinctrl_tmpl_state_id_t num_states_found = 0;

	for (u16_t i = 0; i < config->pinctrl_state_count; i++) {
		if (func == config->pinctrl_state_data[i].device_function) {
			if (num_states_found < *num_states) {
				states[num_states_found] = i;
			}
			num_states_found++;
		}
	}

	if (num_states_found == 0) {
		*num_states = 0;
		return -ENOTSUP;
	}
	if (num_states_found > *num_states) {
		*num_states = num_states_found;
		return -EINVAL;
	}
	*num_states = num_states_found;
	return 0;
}

static int pinctrl_tmpl_control_get_device_function(struct device *dev,
						    struct device *client,
						    u16_t *func)
{
	const struct pinctrl_tmpl_config *config =
		pinctrl_tmpl_device_config_info(dev);

	for (u32_t i = 0; i < config->device_function_count; i++) {
		if (strcmp(client->config->name,
			   config->device_function_data[i].name) == 0) {
			*func = i + PINCTRL_FUNCTION_DEVICE_BASE;
			return 0;
		}
	}

	return -ENOTSUP;
}

#endif /* CONFIG_PINCTRL_RUNTIME_DTS */

static int pinctrl_tmpl_control_get_gpio_range(struct device *dev,
					      struct device *gpio,
					      u32_t gpio_pin, u16_t *pin,
					      u16_t *base_pin,
					      u8_t *num_pins)
{
	const struct pinctrl_tmpl_config *config =
		pinctrl_tmpl_device_config_info(dev);

	int gpio_pin_idx = gpio_port_pin_idx(gpio_pin);

	if (gpio_pin_idx < 0) {
		/* invalid gpio_pin */
		return gpio_pin_idx;
	}
	for (u8_t i = 0; i < config->gpio_range_count; i++) {
		if ((strcmp(gpio->config->name,
			    config->gpio_range_data[i].name) == 0) &&
		    (gpio_pin_idx >=
		     config->gpio_range_data[i].gpio_base_idx) &&
		    (gpio_pin_idx < config->gpio_range_data[i].npins)) {
			*pin = config->gpio_range_data[i].base +
				(gpio_pin_idx -
				 config->gpio_range_data[i].gpio_base_idx);
			*base_pin = config->gpio_range_data[i].base;
			*num_pins = config->gpio_range_data[i].npins;
			return 0;
		}
	}

	return -ENOTSUP;
}

/* Wrapper to access externally provided function */
static inline int pinctrl_tmpl_config_get(struct device *dev, u16_t pin,
					 u32_t *config)
{
	/**
	 * @code{.codegen}
	 * codegen.out('return {}(dev, pin, config);'.format(config_get))
	 * @endcode{.codegen}
	 */
	/** @code{.codeins}@endcode */
}

/* Wrapper to access externally provided function */
static inline int pinctrl_tmpl_config_set(struct device *dev, u16_t pin,
					 u32_t config)
{
	/**
	 * @code{.codegen}
	 * codegen.out('return {}(dev, pin, config);'.format(config_set))
	 * @endcode{.codegen}
	 */
	/** @code{.codeins}@endcode */
}

#if CONFIG_PINCTRL_RUNTIME_DTS

static int pinctrl_tmpl_config_group_get(struct device *dev, u16_t group,
					u32_t *configs, u16_t *num_configs)
{
	int err;
	const struct pinctrl_tmpl_config *config =
		pinctrl_tmpl_device_config_info(dev);

	if (group >= config->pinctrl_state_count) {
		*num_configs = 0;
		return -ENOTSUP;
	}

	u16_t num_pins_found = 0;

	for (int i = 0; i < config->pinctrl_count; i++) {
		if (config->pinctrl_data[i].state == group) {
			if (num_pins_found < *num_configs) {
				err = pinctrl_tmpl_config_get(dev, i,
							     &configs
							     [num_pins_found]);
				if (err) {
					*num_configs = num_pins_found;
					return err;
				}
			}
			num_pins_found++;
		}
	}
	if (num_pins_found == 0) {
		*num_configs = 0;
		return -ENOTSUP;
	}
	if (num_pins_found > *num_configs) {
		*num_configs = num_pins_found;
		return -EINVAL;
	}
	*num_configs = num_pins_found;
	return 0;
}

static int pinctrl_tmpl_config_group_set(struct device *dev, u16_t group,
					const u32_t *configs,
					u16_t num_configs)
{
	int err;
	const struct pinctrl_tmpl_config *config =
		pinctrl_tmpl_device_config_info(dev);

	if (group >= config->pinctrl_state_count) {
		return -ENOTSUP;
	}

	u16_t num_pins_found = 0;

	for (int i = 0; i < config->pinctrl_count; i++) {
		if (config->pinctrl_data[i].state == group) {
			if (num_pins_found < num_configs) {
				err = pinctrl_tmpl_config_set(dev, i,
							     configs
							     [num_pins_found]);
				if (err) {
					return err;
				}
			}
			num_pins_found++;
		}
	}
	if (num_pins_found == 0) {
		return -ENOTSUP;
	}
	if (num_pins_found != num_configs) {
		return -EINVAL;
	}
	return 0;
}

static int pinctrl_tmpl_mux_request(struct device *dev, u16_t pin,
				   const char *owner)
{
	const char *current_owner;

	if (pin >= pinctrl_tmpl_device_config_info(dev)->pin_count) {
		SYS_LOG_ERR("Invalid pin %d of %s on mux request by %s.",
			    (int)pin, dev->config->name, owner);
		return -ENOTSUP;
	}
	current_owner = pinctrl_tmpl_mux_owner_get(dev, pin);
	if (current_owner == 0) {
		return pinctrl_tmpl_mux_owner_set(dev, pin, owner);
	}
	if (current_owner != owner) {
		SYS_LOG_WRN("Pin %d of %s owned by %s but mux requested by %s.",
			    (int)pin, dev->config->name, current_owner, owner);
		return -EBUSY;
	}
	return 0;
}

static int pinctrl_tmpl_mux_free(struct device *dev, u16_t pin,
				const char *owner)
{
	const char *current_owner;

	if (pin >= pinctrl_tmpl_device_config_info(dev)->pin_count) {
		SYS_LOG_ERR("Invalid pin %d of %s on mux free by %s.",
			    (int)pin, dev->config->name, owner);
		return -ENOTSUP;
	}
	current_owner = pinctrl_tmpl_mux_owner_get(dev, pin);
	if (current_owner != owner) {
		SYS_LOG_WRN(
			"Pin %d of %s owned by %s but mux free tried by %s.",
			(int)pin, dev->config->name, current_owner, owner);
		return -EACCES;
	}
	return pinctrl_tmpl_mux_owner_set(dev, pin, 0);
}

#endif /* CONFIG_PINCTRL_RUNTIME_DTS */

/* Wrapper to access externally provided function */
static inline int pinctrl_tmpl_mux_get(struct device *dev, u16_t pin,
				      u16_t *func)
{
/**
 * @code{.codegen}
 * codegen.outl('\treturn {}(dev, pin, func);'.format(mux_get))
 * @endcode{.codegen}
 */
/** @code{.codeins}@endcode */
}

static int pinctrl_tmpl_mux_set(struct device *dev, u16_t pin,
				u16_t func)
{
	const struct pinctrl_tmpl_config *config =
		pinctrl_tmpl_device_config_info(dev);

	if (pin >= pinctrl_tmpl_control_get_pins_count(dev)) {
		return -ENOTSUP;
	}

	if (pinctrl_is_device_function(func)) {
#if CONFIG_PINCTRL_RUNTIME_DTS
		pinctrl_tmpl_function_id_t device_function =
			pinctrl_tmpl_device_function_id(func);

		/* Get hardware pinmux control for pin
		 * from a pinctrl for the device function.
		 */
		for (int i = 0; i < config->pinctrl_count; i++) {
			u16_t pinctrl_state = config->pinctrl_data[i].state;
			u16_t pinctrl_func =
				config->pinctrl_state_data[pinctrl_state]
					.device_function;
			u16_t pinctrl_pin = config->pinctrl_data[i].pin;

			if ((pinctrl_func == device_function)
			     && (pinctrl_pin == pin)) {
				func = config->pinctrl_data[i].mux;
				break;
			}
		}
#else
		return -ENOTSUP;
#endif /* CONFIG_PINCTRL_RUNTIME_DTS */
	}
	if (!pinctrl_is_pinmux_function(func)) {
		return -EINVAL;
	}

	return config->mux_set(dev, pin, func);
}

#if CONFIG_PINCTRL_RUNTIME_DTS

static int pinctrl_tmpl_mux_group_set(struct device *dev, u16_t group,
				     u16_t func)
{
	const struct pinctrl_tmpl_config *config =
		pinctrl_tmpl_device_config_info(dev);
	int err;

	if (!pinctrl_tmpl_is_device_function(dev, func)) {
		/* function does not denote a device */
		return -ENOTSUP;
	}
	if (group >= config->pinctrl_state_count) {
		/* pin group does not correspond to a pinctrl state */
		return -ENOTSUP;
	}
	func = pinctrl_tmpl_device_function_id(func);
	if (config->pinctrl_state_data[group].device_function != func) {
		/* Group (aka pinctrl state) was defined by a different
		 * function (aka. device)
		 */
		return -EINVAL;
	}
	u16_t num_pins_found = 0;

	for (u16_t i = 0; i < config->pinctrl_count; i++) {
		if (config->pinctrl_data[i].state == group) {
			err = pinctrl_tmpl_mux_set(dev,
						  config->pinctrl_data[i].pin,
						  config->pinctrl_data[i].mux);
			if (err) {
				return err;
			}
			num_pins_found++;
		}
	}
	if (num_pins_found == 0) {
		/* no pins found to mux pin group to function */
		return -ENOTSUP;
	}
	return 0;
}

#endif /* CONFIG_PINCTRL_RUNTIME_DTS */

static int pinctrl_tmpl_state_set(struct device *dev, u16_t state)
{
	const struct pinctrl_tmpl_config *config =
		pinctrl_tmpl_device_config_info(dev);
	int err = -ENOTSUP; /* Returned if no corresponding state is found */

#if CONFIG_PINCTRL_RUNTIME_DTS
	if (state >= config->pinctrl_state_count) {
		return err;
	}

	SYS_LOG_DBG("Set '%s' state.",
		    config->state_name_data[
				config->pinctrl_state_data[state]
					.name].name);

	for (pinctrl_tmpl_pinctrl_id_t i = 0; i < config->pinctrl_count; i++) {
		if (config->pinctrl_data[i].state == state) {
			const char *owner = config->device_function_data[
				config->pinctrl_state_data[state]
					.device_function].name;

			err = pinctrl_tmpl_mux_request(
				dev, config->pinctrl_data[i].pin, owner);
			if (err) {
				return err;
			}
			err = pinctrl_tmpl_config_set(dev,
				config->pinctrl_data[i].pin,
				config->pinctrl_data[i].config);
			if (err) {
				return err;
			}
			err = pinctrl_tmpl_mux_set(dev,
				config->pinctrl_data[i].pin,
				config->pinctrl_data[i].mux);
			if (err) {
				return err;
			}
		}
	}
#else
	SYS_LOG_DBG("Set 'default' state.");

	/* all pinctrl data belongs to the "default" state */
	for (pinctrl_tmpl_pinctrl_id_t i = 0; i < config->pinctrl_count; i++) {
		err = pinctrl_tmpl_config_set(dev,
			config->pinctrl_data[i].pin,
			config->pinctrl_data[i].config);
		if (err) {
			return err;
		}
		err = pinctrl_tmpl_mux_set(dev,
			config->pinctrl_data[i].pin,
			config->pinctrl_data[i].mux);
		if (err) {
			return err;
		}
	}
#endif /* CONFIG_PINCTRL_RUNTIME_DTS */
	return err;
}

/**
 * @brief Initialise pin controller.
 *
 * @param dev Pointer to the device structure for the driver instance.
 */
static int pinctrl_tmpl_init(struct device *dev)
{
	const struct pinctrl_tmpl_config *config =
		pinctrl_tmpl_device_config_info(dev);
	int err;

	err = config->device_init(dev);
	if (err) {
		return err;
	}
#if CONFIG_PINCTRL_RUNTIME_DTS
	err = pinctrl_tmpl_mux_request_init(dev);
	if (err) {
		return err;
	}
	for (pinctrl_tmpl_state_id_t state = 0;
	     (state < config->pinctrl_state_count) && !err;
	     state++) {
		const char *state_name =
			config->state_name_data
			[config->pinctrl_state_data[state].name]
			.name;

		if (strcmp(state_name, "default") == 0) {
			err = pinctrl_tmpl_state_set(dev, state);
		}
	}
#else
	/* all pinctrl data belongs to the "default" state */
	err = pinctrl_tmpl_state_set(dev, 0);
#endif /* CONFIG_PINCTRL_RUNTIME_DTS */
	return err;
}

static const struct pinctrl_driver_api pinctrl_tmpl_driver_api = {
	.control = {
		.get_pins_count = pinctrl_tmpl_control_get_pins_count,
		.get_gpio_range = pinctrl_tmpl_control_get_gpio_range,
#if CONFIG_PINCTRL_RUNTIME_DTS
		.get_groups_count = pinctrl_tmpl_control_get_groups_count,
		.get_group_pins = pinctrl_tmpl_control_get_group_pins,
		.get_states_count = pinctrl_tmpl_control_get_states_count,
		.get_state_group = pinctrl_tmpl_control_get_state_group,
		.get_functions_count =
			pinctrl_tmpl_control_get_functions_count,
		.get_function_group =
			pinctrl_tmpl_control_get_function_group,
		.get_function_groups =
			pinctrl_tmpl_control_get_function_groups,
		.get_function_state =
			pinctrl_tmpl_control_get_function_state,
		.get_function_states =
			pinctrl_tmpl_control_get_function_states,
		.get_device_function =
			pinctrl_tmpl_control_get_device_function,
#endif /* CONFIG_PINCTRL_RUNTIME_DTS */
		},
	.config = {
		/**
		 * @code{.codegen}
		 * codegen.outl('.get = {},'.format(config_get))
		 * codegen.outl('.set = {},'.format(config_set))
		 * @endcode{.codegen}
		 */
		/** @code{.codeins}@endcode */
#if CONFIG_PINCTRL_RUNTIME_DTS
		.group_get = pinctrl_tmpl_config_group_get,
		.group_set = pinctrl_tmpl_config_group_set,
#endif /* CONFIG_PINCTRL_RUNTIME_DTS */
		},
	.mux = {
		/**
		 * @code{.codegen}
		 * codegen.outl('.get = {},'.format(mux_get))
		 * @endcode{.codegen}
		 */
		/** @code{.codeins}@endcode */
		.set = pinctrl_tmpl_mux_set,
#if CONFIG_PINCTRL_RUNTIME_DTS
		.group_set = pinctrl_tmpl_mux_group_set,
		.request = pinctrl_tmpl_mux_request,
		.free = pinctrl_tmpl_mux_free,
#endif /* CONFIG_PINCTRL_RUNTIME_DTS */
		},
	.state = {
		.set = pinctrl_tmpl_state_set,
		},
};

/**
 * @code{.codegen}
 *
 * codegen.import_module('devicedeclare')
 *
 * _device_init = 'pinctrl_tmpl_init'
 * _device_level = 'PRE_KERNEL_1'
 * _device_prio = codegen.config_property('PINCTRL_INIT_PRIORITY', 2)
 * _device_api = 'pinctrl_tmpl_driver_api'
 *
 * for _pin_controller in _pin_controllers:
 *     _device_id = _pin_controller.device_id()
 *     _driver_name = codegen.edts().device_property(_device_id, 'label')
 *     _device_config = "CONFIG_{}".format(_driver_name.strip('"'))
 *     _device_info = ""
 *
 *     _pin_count = _pin_controller.pin_count()
 *     _mux_pinmap = '${device-name}_mux_pinmap'
 *     _device_function_data = '${device-name}_function'
 *     _device_function_count = _pin_controller.function_count()
 *     _state_name_data = '${device-name}_state_name'
 *     _state_name_count = _pin_controller.state_name_count()
 *     _pinctrl_state_data = '${device-name}_pinctrl_state'
 *     _pinctrl_state_count = _pin_controller.state_count()
 *     _pinctrl_data = '${device-name}_pinctrl'
 *     _pinctrl_count = _pin_controller.pinctrl_count()
 *     _gpio_range_data = '${device-name}_gpio_range'
 *     _gpio_range_count = _pin_controller.gpio_range_count()
 *     _config_info = '${device-name}_config'
 *     _data = '${device-name}_data'
 *
 *     if codegen.config_property('CONFIG_PINCTRL_RUNTIME_DTS', 0):
 *         #
 *         # mux_pinmap data
 *         _device_info += 'static u8_t {}[{}];\n' \
 *             .format(_mux_pinmap, (_pin_count + 1) >> 1)
 *         #
 *         # function data
 *         _device_info += \
 *             'static const struct pinctrl_tmpl_device_function {}[{}] = {{\n'\
 *                 .format(_device_function_data, _device_function_count)
 *         for function in _pin_controller.function_data():
 *             _device_info += '\t{{\n\t\t.name = "{}",\n\t}},\n' \
 *                             .format(function)
 *         _device_info += "};\n"
 *         #
 *         # state name data
 *         _device_info += \
 *             'static const struct pinctrl_tmpl_state_name {}[{}] = {{\n' \
 *                 .format(_state_name_data, _state_name_count)
 *         for state_name in _pin_controller.state_name_data():
 *             _device_info += '\t{{\n\t\t.name = "{}",\n\t}},\n' \
 *                             .format(state_name)
 *         _device_info += "};\n"
 *         #
 *         # pinctrl state data
 *         _device_info += \
 *             'static const struct pinctrl_tmpl_pinctrl_state {}[{}] = {{\n' \
 *                 .format(_pinctrl_state_data, _pinctrl_state_count)
 *         for pinctrl_state in _pin_controller.state_data():
 *             _device_info += \
 *                 '\t{\n' + \
 *                 '\t\t.device_function = {},\n' \
 *                     .format(pinctrl_state['function-id']) + \
 *                 '\t\t.name = {},\n' \
 *                     .format(pinctrl_state['state-name-id']) + \
 *                 '\t},\n'
 *         _device_info += "};\n"
 *     #
 *     # pinctrl data
 *     # only "default" state data if no CONFIG_PINCTRL_RUNTIME_DTS
 *     if codegen.config_property('CONFIG_PINCTRL_RUNTIME_DTS', 0) == 0:
 *         _pinctrl_state_name = 'default'
 *     else:
 *         _pinctrl_state_name = None
 *     _pinctrl_data_list = _pin_controller.pinctrl_data(_pinctrl_state_name)
 *     _pinctrl_data_count = len(_pinctrl_data_list)
 *     _device_info += \
 *         'static const struct pinctrl_tmpl_pinctrl {}[{}] = {{ \n' \
 *             .format(_pinctrl_data, _pinctrl_data_count)
 *     for pinctrl_id, pinctrl in enumerate(_pinctrl_data_list):
 *         _pin = pinctrl['pin']
 *         _mux =  pinctrl['mux']
 *         _state_id = pinctrl['state-id']
 *         _device_info += ('\t{{ /' '* {}: {} {} *' '/\n') \
 *             .format(pinctrl_id,
 *                     _pin_controller.state_desc(_state_id),
 *                     pinctrl['name'])
 *         _device_info += '\t.pin = {},\n'.format(_pin)
 *         _device_info += '\t.mux = {},\n'.format(_mux)
 *         if codegen.config_property('CONFIG_PINCTRL_RUNTIME_DTS', 0):
 *             _device_info += '\t.state = {},\n'.format(_state_id)
 *         _device_info += '\t.config = \\\n'
 *         _config = []
 *         for config_x in ('bias-disable', 'bias-high-impedance',
 *                          'bias-bus-hold', 'bias-pull-up', 'bias-pull-down',
 *                          'bias-pull-pin-default', 'drive-push-pull',
 *                          'drive-open_drain', 'drive-open-source',
 *                          'input-enable', 'input-disable',
 *                          'input-schmitt-enable', 'input-schmitt-disable',
 *                          'low-power-enable', 'low-power-disable',
 *                          'output-enable', 'output-disable',
 *                          'output-low', 'output-high'):
 *             _config.append('({} * PINCTRL_CONFIG_{})'.format( \
 *                 pinctrl['config'].get(config_x, 0),
 *                 config_x.replace('-', '_').upper()))
 *         _config.append('({} & PINCTRL_CONFIG_DRIVE_STRENGTH_7)'.format( \
 *             pinctrl['config'].get('drive-strength', 0)))
 *         _config.append('({} & PINCTRL_CONFIG_POWER_SOURCE_3)'.format( \
 *             pinctrl['config'].get('power-source', 0)))
 *         _config.append('({} & PINCTRL_CONFIG_SLEW_RATE_FAST)'.format( \
 *             pinctrl['config'].get('slew-rate', 0)))
 *         _config.append('({} & PINCTRL_CONFIG_SPEED_HIGH)'.format( \
 *             pinctrl['config'].get('speed', 0)))
 *         for config_x in _config:
 *             _device_info += '\t\t{} |\\\n'.format(config_x)
 *         _device_info += '\t\t0,\n'
 *         _device_info += '\t},\n'
 *     _device_info += '};\n'
 *     #
 *     # gpio range data
 *     _device_info += \
 *         'static const struct pinctrl_tmpl_gpio_range {}[{}] = {{\n' \
 *             .format(_gpio_range_data, _gpio_range_count)
 *     for gpio_range in _pin_controller.gpio_range_data():
 *         _device_info += '\t{\n'
 *         _device_info += '\t.gpio_base_idx = {},\n'.format( \
 *                                                gpio_range['gpio-base-idx'])
 *         _device_info += '\t.npins = {},\n'.format(gpio_range['npins'])
 *         _device_info += '\t.base = {},\n'.format(gpio_range['base'])
 *         _device_info += '\t.name = "{}",\n'.format(gpio_range['name'])
 *         _device_info += '\t},\n'
 *     _device_info += '};\n'
 *     #
 *     # config info
 *     _device_info += 'static const struct pinctrl_tmpl_config {} = {{\n' \
 *         .format(_config_info)
 *     _device_info += '\t.pin_count = {},\n'.format(_pin_count)
 *     if codegen.config_property('CONFIG_PINCTRL_RUNTIME_DTS', 0):
 *         _device_info += '\t.device_function_count = {},\n' \
 *                      .format(_device_function_count)
 *         _device_info += '\t.device_function_data = {},\n' \
 *                      .format(_device_function_data)
 *         _device_info += '\t.state_name_count = {},\n' \
 *                      .format(_state_name_count)
 *         _device_info += '\t.state_name_data = {},\n' \
 *                      .format(_state_name_data)
 *         _device_info += '\t.pinctrl_state_count = {},\n' \
 *                      .format(_pinctrl_state_count)
 *         _device_info += '\t.pinctrl_state_data = {},\n' \
 *                      .format(_pinctrl_state_data)
 *         _device_info += '\t.mux_pinmap = {},\n' \
 *                      .format(_mux_pinmap)
 *     _device_info += '\t.pinctrl_count = {},\n'.format(_pinctrl_count)
 *     _device_info += '\t.pinctrl_data = {},\n'.format(_pinctrl_data)
 *     _device_info += '\t.gpio_range_count = {},\n'.format(_gpio_range_count)
 *     _device_info += '\t.gpio_range_data = {},\n'.format(_gpio_range_data)
 *     _device_info += '\t.device_init = {},\n'.format(device_init)
 *     _device_info += '\t.mux_set = {},\n'.format(mux_set)
 *     _device_info += '};\n'
 *     #
 *     # data
 *     _device_info += 'static {} {};\n'.format(data_info ,_data)
 *     #
 *     devicedeclare.device_declare_single(_device_config,
 *                                         _driver_name,
 *                                         _device_init,
 *                                         _device_level,
 *                                         _device_prio,
 *                                         _device_api,
 *                                         _device_info)
 * @endcode{.codegen}
 */
/** @code{.codeins}@endcode */

/** @} pinctrl_template */
