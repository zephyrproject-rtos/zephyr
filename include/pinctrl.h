/*
 * Copyright (c) 2018 Bobby Noelte
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * Public APIs for PINCTRL drivers
 */

#ifndef __INCLUDE_PINCTRL_H
#define __INCLUDE_PINCTRL_H

/**
 * @brief PINCTRL Interface
 * @defgroup pinctrl_interface PINCTRL Interface
 * @ingroup io_interfaces
 *
 * Pin control interface for
 * - pin control: get pin control properties
 * - pin configuration: configure a pins electronic properties
 * - pin multiplexing: reuse the same pin for different purposes
 * @{
 */

#include <pinctrl_common.h>

#include <zephyr/types.h>
#include <misc/__assert.h>
#include <device.h>


/**
 * @def PINCTRL_CONFIG
 * @brief Create a pinctrl pin configuration value.
 *
 * The configuration defines shall be the
 * @ref pinctrl_interface_pin_configurations "pinctrl pin configurations"
 * given in the pinctrl_common.h file.
 */
#define PINCTRL_CONFIG(...) _PINCTRL_CONFIG0(__VA_ARGS__)
/**
 * @def _PINCTRL_CONFIG0
 * @internal
 */
#define _PINCTRL_CONFIG0(...)                                   \
	_PINCTRL_CONFIG32(__VA_ARGS__,                          \
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
/**
 * @def _PINCTRL_CONFIG32
 * @internal
 */
#define _PINCTRL_CONFIG32(c0,  c1,  c2,  c3,  c4,  c5,  c6,  c7,  \
			  c8,  c9,  c10, c11, c12, c13, c14, c15, \
			  c16, c17, c18, c19, c20, c21, c22, c23, \
			  c24, c25, c26, c27, c28, c29, c30, c31, \
			  ...)                                    \
	(c0  | c1  | c2  | c3  | c4  | c5  | c6  | c7  |          \
	 c8  | c9  | c10 | c11 | c12 | c13 | c14 | c15 |          \
	 c16 | c17 | c18 | c19 | c20 | c21 | c22 | c23 |          \
	 c24 | c25 | c26 | c17 | c28 | c29 | c30 | c31)


#ifdef __cplusplus
extern "C" {
#endif

struct pinctrl_control_api {
	u16_t (*get_pins_count)(struct device *dev);
#if CONFIG_PINCTRL_RUNTIME_DTS
	int (*get_groups_count)(struct device *dev);
	int (*get_group_pins)(struct device *dev, u16_t group,
			      u16_t *pins, u16_t *num_pins);
	u16_t (*get_states_count)(struct device *dev);
	int (*get_state_group)(struct device *dev, u16_t state, u16_t *group);
	u16_t (*get_functions_count)(struct device *dev);
	int (*get_function_group)(struct device *dev, u16_t func,
				  const char *name, u16_t *group);
	int (*get_function_groups)(struct device *dev, u16_t func,
				   u16_t *groups, u16_t *num_groups);
	int (*get_function_state)(struct device *dev, u16_t func,
				  const char *name, u16_t *state);
	int (*get_function_states)(struct device *dev, u16_t func,
				   u16_t *states, u16_t *num_states);
	int (*get_device_function)(struct device *dev, struct device *client,
				   u16_t *func);
#endif /* CONFIG_PINCTRL_RUNTIME_DTS */
	int (*get_gpio_range)(struct device *dev, struct device *gpio,
			      u32_t gpio_pin, u16_t *pin, u16_t *base_pin,
			      u8_t *num_pins);
};

struct pinctrl_config_api {
	int (*get)(struct device *dev, u16_t pin, u32_t *config);
	int (*set)(struct device *dev, u16_t pin, u32_t config);
#if CONFIG_PINCTRL_RUNTIME_DTS
	int (*group_get)(struct device *dev, u16_t group,
			 u32_t *configs, u16_t *num_configs);
	int (*group_set)(struct device *dev, u16_t group,
			 const u32_t *configs, u16_t num_configs);
#endif /* CONFIG_PINCTRL_RUNTIME_DTS */
};

struct pinctrl_mux_api {
	int (*get)(struct device *dev, u16_t pin, u16_t *func);
	int (*set)(struct device *dev, u16_t pin, u16_t func);
#if CONFIG_PINCTRL_RUNTIME_DTS
	int (*group_set)(struct device *dev, u16_t group, u16_t func);
	int (*request)(struct device *dev, u16_t pin, const char *owner);
	int (*free)(struct device *dev, u16_t pin, const char *owner);
#endif /* CONFIG_PINCTRL_RUNTIME_DTS */
};

struct pinctrl_state_api {
	int (*set)(struct device *dev, u16_t state);
};

struct pinctrl_driver_api {
	struct pinctrl_control_api control;
	struct pinctrl_config_api config;
	struct pinctrl_mux_api mux;
	struct pinctrl_state_api state;
};

/**
 * Get the number of pins controlled by this pin controller.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @return number of pins
 */
__syscall u16_t pinctrl_get_pins_count(struct device *dev);

/** @internal
 */
static inline u16_t _impl_pinctrl_get_pins_count(struct device *dev)
{
	const struct pinctrl_driver_api *api =
		(const struct pinctrl_driver_api *)dev->driver_api;

	return api->control.get_pins_count(dev);
}

/**
 * Get the number of groups selectable by this pin controller.
 *
 * @note To be enabled by CONFIG_PINCTRL_RUNTIME_DTS.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @return number of groups
 */
__syscall u16_t pinctrl_get_groups_count(struct device *dev);

/** @internal
 */
static inline u16_t _impl_pinctrl_get_groups_count(struct device *dev)
{
#if CONFIG_PINCTRL_RUNTIME_DTS
	const struct pinctrl_driver_api *api =
		(const struct pinctrl_driver_api *)dev->driver_api;

	return api->control.get_groups_count(dev);
#else
	__ASSERT(0, "To be enabled by CONFIG_PINCTRL_RUNTIME_DTS.");
	return 0;
#endif /* CONFIG_PINCTRL_RUNTIME_DTS */
}

/**
 * Get the pins that are in a pin group.
 *
 * Returns an array of pin numbers. The applicable pins
 * are returned in @p pins and the number of pins in @p num_pins. The
 * number of pins returned is bounded by the initial value of @p num_pins
 * when called.
 *
 * @note To be enabled by CONFIG_PINCTRL_RUNTIME_DTS.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param group Group
 * @param[in,out] pins Array of pins.
 * @param[in,out] num_pins Size of the array of pins.
 * @retval 0 on success
 * @retval -EINVAL if number of pins exceeds array size.
 * @retval -ENOTSUP if requested group is not available on this controller
 */
__syscall int pinctrl_get_group_pins(struct device *dev, u16_t group,
				     u16_t *pins, u16_t *num_pins);

/** @internal
 */
static inline int _impl_pinctrl_get_group_pins(struct device *dev, u16_t group,
					       u16_t *pins, u16_t *num_pins)
{
#if CONFIG_PINCTRL_RUNTIME_DTS
	const struct pinctrl_driver_api *api =
		(const struct pinctrl_driver_api *)dev->driver_api;

	return api->control.get_group_pins(dev, group, pins, num_pins);
#else
	__ASSERT(0, "To be enabled by CONFIG_PINCTRL_RUNTIME_DTS.");
	return -ENOTSUP;
#endif /* CONFIG_PINCTRL_RUNTIME_DTS */
}

/**
 * Get the number of states selectable by this pin controller.
 *
 * @note To be enabled by CONFIG_PINCTRL_RUNTIME_DTS.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @return number of states
 */
__syscall u16_t pinctrl_get_states_count(struct device *dev);

/** @internal
 */
static inline u16_t _impl_pinctrl_get_states_count(struct device *dev)
{
#if CONFIG_PINCTRL_RUNTIME_DTS
	const struct pinctrl_driver_api *api =
		(const struct pinctrl_driver_api *)dev->driver_api;

	return api->control.get_states_count(dev);
#else
	__ASSERT(0, "To be enabled by CONFIG_PINCTRL_RUNTIME_DTS.");
	return 0;
#endif /* CONFIG_PINCTRL_RUNTIME_DTS */
}

/**
 * Get the group of pins controlled by the pinctrl state.
 *
 * @note To be enabled by CONFIG_PINCTRL_RUNTIME_DTS.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param state State
 * @param[out] group Group
 * @retval 0 on success
 * @retval -ENOTSUP if requested state is not available on this controller
 */
__syscall int pinctrl_get_state_group(struct device *dev, u16_t state,
				      u16_t *group);

/** @internal
 */
static inline int _impl_pinctrl_get_state_group(struct device *dev,
						u16_t state, u16_t *group)
{
#if CONFIG_PINCTRL_RUNTIME_DTS
	const struct pinctrl_driver_api *api =
		(const struct pinctrl_driver_api *)dev->driver_api;

	return api->control.get_state_group(dev, state, group);
#else
	__ASSERT(0, "To be enabled by CONFIG_PINCTRL_RUNTIME_DTS.");
	return -ENOTSUP;
#endif /* CONFIG_PINCTRL_RUNTIME_DTS */
}

/**
 * Get the number of selectable functions of this pin controller.
 *
 * @note To be enabled by CONFIG_PINCTRL_RUNTIME_DTS.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @return number of functions
 */
__syscall u16_t pinctrl_get_functions_count(struct device *dev);

/** @internal
 */
static inline u16_t _impl_pinctrl_get_functions_count(struct device *dev)
{
#if CONFIG_PINCTRL_RUNTIME_DTS
	const struct pinctrl_driver_api *api =
		(const struct pinctrl_driver_api *)dev->driver_api;

	return api->control.get_functions_count(dev);
#else
	__ASSERT(0, "To be enabled by CONFIG_PINCTRL_RUNTIME_DTS.");
	return 0;
#endif /* CONFIG_PINCTRL_RUNTIME_DTS */
}

/**
 * Get the pin group of given name that is related to given function.
 *
 * @note To be enabled by CONFIG_PINCTRL_RUNTIME_DTS.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param func Function
 * @param name Group name
 * @param[out] group Group
 * @retval 0 on success
 * @retval -ENODEV if requested function is not available on this controller
 * @retval -ENOTSUP if there is no group with the requested name for
 *                  the function
 */
__syscall int pinctrl_get_function_group(struct device *dev, u16_t func,
					 const char *name, u16_t *group);

/** @internal
 */
static inline int _impl_pinctrl_get_function_group(struct device *dev,
						   u16_t func,
						   const char *name,
						   u16_t *group)
{
#if CONFIG_PINCTRL_RUNTIME_DTS
	const struct pinctrl_driver_api *api =
		(const struct pinctrl_driver_api *)dev->driver_api;

	return api->control.get_function_group(dev, func, name, group);
#else
	__ASSERT(0, "To be enabled by CONFIG_PINCTRL_RUNTIME_DTS.");
	return -ENOTSUP;
#endif /* CONFIG_PINCTRL_RUNTIME_DTS */
}

/**
 * Get the groups of pins that can be multiplexed to the function.
 *
 * Returns an array of group numbers. The group number can be used with
 * pinctrl_get_group_pins() to retrieve the pins. The applicable groups
 * are returned in @p groups and the number of groups in @p num_groups. The
 * number of groups returned is bounded by the initial value of @p num_groups
 * when called.
 *
 * @note To be enabled by CONFIG_PINCTRL_RUNTIME_DTS.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param func Function
 * @param[out] groups Array of groups.
 * @param[in,out] num_groups Size of the array of groups.
 * @retval 0 on success
 * @retval -ENODEV if requested function is not available on this controller
 * @retval -ENOTSUP if there is no group
 * @retval -EINVAL if the number of available groups exceeds array size
 */
__syscall int pinctrl_get_function_groups(struct device *dev, u16_t func,
					  u16_t *groups, u16_t *num_groups);

/** @internal
 */
static inline int _impl_pinctrl_get_function_groups(struct device *dev,
						    u16_t func,
						    u16_t *groups,
						    u16_t *num_groups)
{
#if CONFIG_PINCTRL_RUNTIME_DTS
	const struct pinctrl_driver_api *api =
		(const struct pinctrl_driver_api *)dev->driver_api;

	return api->control.get_function_groups(dev, func, groups, num_groups);
#else
	__ASSERT(0, "To be enabled by CONFIG_PINCTRL_RUNTIME_DTS.");
	return -ENOTSUP;
#endif /* CONFIG_PINCTRL_RUNTIME_DTS */
}

/**
 * Get the state of given name that can be applied to given function.
 *
 * @note To be enabled by CONFIG_PINCTRL_RUNTIME_DTS.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param func Function
 * @param name State name
 * @param[out] state State
 * @retval 0 on success
 * @retval -ENODEV if function is unknown to pin controller
 * @retval -ENOTSUP if there is no state with the requested name for
 *                  the function
 */
__syscall int pinctrl_get_function_state(struct device *dev, u16_t func,
					 const char *name, u16_t *state);

/** @internal
 */
static inline int _impl_pinctrl_get_function_state(struct device *dev,
						   u16_t func,
						   const char *name,
						   u16_t *state)
{
#if CONFIG_PINCTRL_RUNTIME_DTS
	const struct pinctrl_driver_api *api =
		(const struct pinctrl_driver_api *)dev->driver_api;

	return api->control.get_function_state(dev, func, name, state);
#else
	__ASSERT(0, "To be enabled by CONFIG_PINCTRL_RUNTIME_DTS.");
	return -ENOTSUP;
#endif /* CONFIG_PINCTRL_RUNTIME_DTS */
}

/**
 * Get the states that can be applied to the function.
 *
 * Returns an array of state numbers. The state number can be used with
 * pinctrl_get_state_group() to retrieve the associated pin group.
 * The applicable states are returned in @p states and the number of states
 * in @p num_states. The number of states returned is bounded by the initial
 * value of @p num_states when called.
 *
 * @note To be enabled by CONFIG_PINCTRL_RUNTIME_DTS.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param func Function
 * @param[out] states Array of states.
 * @param[in,out] num_states Size of the array of states.
 * @retval 0 on success
 * @retval -ENODEV if function is unknown to pin controller
 * @retval -ENOTSUP if there is no state
 * @retval -EINVAL if the number of available states exceeds array size
 */
__syscall int pinctrl_get_function_states(struct device *dev, u16_t func,
					  u16_t *states, u16_t *num_states);

/** @internal
 */
static inline int _impl_pinctrl_get_function_states(struct device *dev,
						    u16_t func,
						    u16_t *states,
						    u16_t *num_states)
{
#if CONFIG_PINCTRL_RUNTIME_DTS
	const struct pinctrl_driver_api *api =
		(const struct pinctrl_driver_api *)dev->driver_api;

	return api->control.get_function_states(dev, func, states, num_states);
#else
	__ASSERT(0, "To be enabled by CONFIG_PINCTRL_RUNTIME_DTS.");
	return -ENOTSUP;
#endif /* CONFIG_PINCTRL_RUNTIME_DTS */
}

/**
 * Get the function that is associated to a client device.
 *
 * @note To be enabled by CONFIG_PINCTRL_RUNTIME_DTS.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param client Pointer to the device structure of the client driver instance.
 * @param[out] func Function
 * @retval 0 on success
 * @retval -ENOTSUP if there is no function for the requested device
 */
__syscall int pinctrl_get_device_function(struct device *dev,
					  struct device *client, u16_t *func);

/** @internal
 */
static inline int _impl_pinctrl_get_device_function(struct device *dev,
						    struct device *client,
						    u16_t *func)
{
#if CONFIG_PINCTRL_RUNTIME_DTS
	const struct pinctrl_driver_api *api =
		(const struct pinctrl_driver_api *)dev->driver_api;

	return api->control.get_device_function(dev, client, func);
#else
	__ASSERT(0, "To be enabled by CONFIG_PINCTRL_RUNTIME_DTS.");
	return -ENOTSUP;
#endif /* CONFIG_PINCTRL_RUNTIME_DTS */
}

/**
 * Is the function a client device multiplex control.
 *
 * @param func Function
 * @return >0 if the function denotes a client device, 0 otherwise.
 */
static inline int pinctrl_is_device_function(u16_t func)
{
	return (func >= PINCTRL_FUNCTION_DEVICE_BASE);
}

/**
 * Is the function a hardware pinmux control.
 *
 * @param func Function
 * @return >0 if the function is a hardware pinmux control, 0 otherwise.
 */
static inline int pinctrl_is_pinmux_function(u16_t func)
{
	return (func < PINCTRL_FUNCTION_DEVICE_BASE);
}

/**
 * Get the pin controller pin number and pin mapping for a GPIO pin.
 *
 * @note Pin controller pins are numbered consecutively.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param gpio Pointer to the device structure of the GPIO driver instance.
 * @param gpio_pin GPIO pin in GPIO number space.
 * @param[out] pin GPIO pin in pin-controller number space.
 * @param[out] base_pin Base pin of gpio range in pin-controller number space.
 * @param[out] num_pins Number of pins in gpio range.
 * @retval 0 on success
 * @retval -ENOTSUP if there is no gpio range for the requested GPIO pin.
 */
__syscall int pinctrl_get_gpio_range(struct device *dev, struct device *gpio,
				     u32_t gpio_pin, u16_t *pin,
				     u16_t *base_pin, u8_t *num_pins);

/** @internal
 */
static inline int _impl_pinctrl_get_gpio_range(struct device *dev,
					       struct device *gpio,
					       u32_t gpio_pin, u16_t *pin,
					       u16_t *base_pin, u8_t *num_pins)
{
	const struct pinctrl_driver_api *api =
		(const struct pinctrl_driver_api *)dev->driver_api;

	return api->control.get_gpio_range(dev, gpio, gpio_pin, pin, base_pin,
					   num_pins);
}

/**
 * Get the configuration of a pin.
 *
 * The configuration returned may be different to the configuration set by
 * pinctrl_config_set(). Some configuration options may be set or reset by
 * the current function selection.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pin Pin
 * @param[out] config
 * @retval 0 on success
 * @retval -ENOTSUP if requested pin is not available on this controller
 * @retval -EINVAL if requested pin is available but disabled
 */
__syscall int pinctrl_config_get(struct device *dev, u16_t pin, u32_t *config);

/** @internal
 */
static inline int _impl_pinctrl_config_get(struct device *dev, u16_t pin,
					   u32_t *config)
{
	const struct pinctrl_driver_api *api =
		(const struct pinctrl_driver_api *)dev->driver_api;

	return api->config.get(dev, pin, config);
}

/**
 * @brief Configure a pin.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pin Pin
 * @param config
 * @retval 0 on success
 * @retval -ENOTSUP if requested pin is not available on this controller
 */
__syscall int pinctrl_config_set(struct device *dev, u16_t pin, u32_t config);

/** @internal
 */
static inline int _impl_pinctrl_config_set(struct device *dev, u16_t pin,
					   u32_t config)
{
	const struct pinctrl_driver_api *api =
		(const struct pinctrl_driver_api *)dev->driver_api;

	return api->config.set(dev, pin, config);
}

/**
 * @brief Get the configuration of a pin group.
 *
 * @note To be enabled by CONFIG_PINCTRL_RUNTIME_DTS.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param group Pin group
 * @param[out] configs set to the configuration of the pins in the group
 * @param[in,out] num_configs number of pin configurations requested/ set.
 * @retval 0 on success
 * @retval -ENOTSUP if configuration read out is not available for this group
 * @retval -EINVAL if the number of available configurations exceeds requested
 */
__syscall int pinctrl_config_group_get(struct device *dev,
				       u16_t group, u32_t *configs,
				       u16_t *num_configs);

/** @internal
 */
static inline int _impl_pinctrl_config_group_get(struct device *dev,
						 u16_t group, u32_t *configs,
						 u16_t *num_configs)
{
#if CONFIG_PINCTRL_RUNTIME_DTS
	const struct pinctrl_driver_api *api =
		(const struct pinctrl_driver_api *)dev->driver_api;

	return api->config.group_get(dev, group, configs, num_configs);
#else
	__ASSERT(0, "To be enabled by CONFIG_PINCTRL_RUNTIME_DTS.");
	return -ENOTSUP;
#endif /* CONFIG_PINCTRL_RUNTIME_DTS */
}


/**
 * @brief Configure a group of pins.
 *
 * @note To be enabled by CONFIG_PINCTRL_RUNTIME_DTS.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param group Pin group
 * @param configs The configurations of the pins in the group
 * @param num_configs number of pin configurations to set.
 * @retval 0 on success
 * @retval -ENOTSUP if configuration is not available for this group
 * @retval -EINVAL if number of configurations does not match pins in group
 */
__syscall int pinctrl_config_group_set(struct device *dev,
				       u16_t group, const u32_t *configs,
				       u16_t num_configs);

/** @internal
 */
static inline int _impl_pinctrl_config_group_set(struct device *dev,
						 u16_t group,
						 const u32_t *configs,
						 u16_t num_configs)
{
#if CONFIG_PINCTRL_RUNTIME_DTS
	const struct pinctrl_driver_api *api =
		(const struct pinctrl_driver_api *)dev->driver_api;

	return api->config.group_set(dev, group, configs, num_configs);
#else
	__ASSERT(0, "To be enabled by CONFIG_PINCTRL_RUNTIME_DTS.");
	return -ENOTSUP;
#endif /* CONFIG_PINCTRL_RUNTIME_DTS */
}

/**
 * @brief Request a pin for muxing.
 *
 * @note To be enabled by CONFIG_PINCTRL_RUNTIME_DTS.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pin Pin
 * @param owner A representation of the owner; typically the device
 *		name that controls its mux function, or the requested GPIO name
 * @retval 0 on success
 * @retval -EBUSY pin already used
 * @retval -ENOTSUP requested pin is not available on this controller
 * @retval -ENOMEM can not handle request
 */
__syscall int pinctrl_mux_request(struct device *dev, u16_t pin,
				  const char *owner);

/** @internal
 */
static inline int _impl_pinctrl_mux_request(struct device *dev, u16_t pin,
					    const char *owner)
{
#if CONFIG_PINCTRL_RUNTIME_DTS
	const struct pinctrl_driver_api *api =
		(const struct pinctrl_driver_api *)dev->driver_api;

	return api->mux.request(dev, pin, owner);
#else
	__ASSERT(0, "To be enabled by CONFIG_PINCTRL_RUNTIME_DTS.");
	return -ENOTSUP;
#endif /* CONFIG_PINCTRL_RUNTIME_DTS */
}

/**
 * @brief Release a pin for muxing.
 *
 * After release another may become owner of the pin.
 *
 * Only the owner may release a muxed pin.
 *
 * @note To be enabled by CONFIG_PINCTRL_RUNTIME_DTS.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pin Pin
 * @param owner A representation of the owner; typically the device
 *		name that controls its mux function, or the requested GPIO name
 * @retval 0 on success
 * @retval -EACCES owner does not own the pin
 * @retval -ENOTSUP requested pin is not available on this controller
 */
__syscall int pinctrl_mux_free(struct device *dev, u16_t pin,
			       const char *owner);

/** @internal
 */
static inline int _impl_pinctrl_mux_free(struct device *dev, u16_t pin,
					 const char *owner)
{
#if CONFIG_PINCTRL_RUNTIME_DTS
	const struct pinctrl_driver_api *api =
		(const struct pinctrl_driver_api *)dev->driver_api;

	return api->mux.free(dev, pin, owner);
#else
	__ASSERT(0, "To be enabled by CONFIG_PINCTRL_RUNTIME_DTS.");
	return -ENOTSUP;
#endif /* CONFIG_PINCTRL_RUNTIME_DTS */
}

/**
 * @brief Get muxing function at pin
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pin Pin
 * @param[out] func Muxing function
 * @retval 0 on success
 * @retval -ENOTSUP if requested pin is not available on this controller
 */
__syscall int pinctrl_mux_get(struct device *dev, u16_t pin, u16_t *func);

/** @internal
 */
static inline int _impl_pinctrl_mux_get(struct device *dev, u16_t pin,
					u16_t *func)
{
	const struct pinctrl_driver_api *api =
		(const struct pinctrl_driver_api *)dev->driver_api;

	return api->mux.get(dev, pin, func);
}

/**
 * @brief Set muxing function at pin
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pin Pin
 * @param func Muxing function
 * @retval 0 on success
 * @retval -ENOTSUP if requested pin is not available on this controller
 */
__syscall int pinctrl_mux_set(struct device *dev, u16_t pin, u16_t func);

/** @internal
 */
static inline int _impl_pinctrl_mux_set(struct device *dev, u16_t pin,
					u16_t func)
{
	const struct pinctrl_driver_api *api =
		(const struct pinctrl_driver_api *)dev->driver_api;

	return api->mux.set(dev, pin, func);
}

/**
 * @brief Set muxing function for a group of pins
 *
 * @note To be enabled by CONFIG_PINCTRL_RUNTIME_DTS.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param group Group
 * @param func Muxing function
 * @retval 0 on success
 * @retval -ENOTSUP if requested function or group is not available
 *                  on this controller
 * @retval -EINVAL if group does no belong to function
 */
__syscall int pinctrl_mux_group_set(struct device *dev, u16_t group,
				    u16_t func);

/** @internal
 */
static inline int _impl_pinctrl_mux_group_set(struct device *dev, u16_t group,
					      u16_t func)
{
#if CONFIG_PINCTRL_RUNTIME_DTS
	const struct pinctrl_driver_api *api =
		(const struct pinctrl_driver_api *)dev->driver_api;

	return api->mux.group_set(dev, group, func);
#else
	__ASSERT(0, "To be enabled by CONFIG_PINCTRL_RUNTIME_DTS.");
	return -ENOTSUP;
#endif /* CONFIG_PINCTRL_RUNTIME_DTS */
}

/**
 * @brief Set pinctrl state.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param state State
 * @retval 0 on success
 * @retval -ENOTSUP if requested state is not available on this controller
 */
__syscall int pinctrl_state_set(struct device *dev, u16_t state);

/** @internal
 */
static inline int _impl_pinctrl_state_set(struct device *dev, u16_t state)
{
	const struct pinctrl_driver_api *api =
		(const struct pinctrl_driver_api *)dev->driver_api;

	return api->state.set(dev, state);
}


#if defined(CONFIG_PINCTRL_PINMUX)

/**
 * @brief PINCTRL PINMUX interface
 * @defgroup pinctrl_interface_pinmux PINCTRL PINMUX interface
 * @ingroup pinctrl_interface
 * @{
 */

#define CONFIG_PINMUX_NAME CONFIG_PINCTRL_NAME

#define PINMUX_FUNC_A		PINCTRL_FUNCTION_HARDWARE_0
#define PINMUX_FUNC_B		PINCTRL_FUNCTION_HARDWARE_1
#define PINMUX_FUNC_C		PINCTRL_FUNCTION_HARDWARE_2
#define PINMUX_FUNC_D		PINCTRL_FUNCTION_HARDWARE_3
#define PINMUX_FUNC_E		PINCTRL_FUNCTION_HARDWARE_4
#define PINMUX_FUNC_F		PINCTRL_FUNCTION_HARDWARE_5
#define PINMUX_FUNC_G		PINCTRL_FUNCTION_HARDWARE_6
#define PINMUX_FUNC_H		PINCTRL_FUNCTION_HARDWARE_7

#define PINMUX_PULLUP_ENABLE	(0x1)
#define PINMUX_PULLUP_DISABLE	(0x0)

#define PINMUX_INPUT_ENABLED	(0x1)
#define PINMUX_OUTPUT_ENABLED	(0x0)

/**
 * @brief Set pin function.
 *
 * @deprecated Use pinctrl_mux_set() instead.
 */
static inline int pinmux_pin_set(struct device *dev, u32_t pin, u16_t func)
{
	return pinctrl_mux_set(dev, (u16_t)pin, func);
}

/**
 * @brief Get pin function.
 *
 * @deprecated Use pinctrl_mux_get() instead.
 */
static inline int pinmux_pin_get(struct device *dev, u32_t pin, u16_t *func)
{
	return pinctrl_mux_get(dev, (u16_t)pin, func);
}

/**
 * @brief Set pin pullup.
 *
 * @deprecated Use pinctrl_config_set() instead.
 */
static inline int pinmux_pin_pullup(struct device *dev, u32_t pin, u8_t func)
{
	int ret;
	u32_t config;

	ret = pinctrl_config_get(dev, (u16_t)pin, &config);
	if (ret != 0) {
		return ret;
	}
	config &= ~PINCTRL_CONFIG_BIAS_MASK;
	if (func == PINMUX_PULLUP_ENABLE) {
		config |= PINCTRL_CONFIG_BIAS_PULL_UP;
	} else {
		config |= PINCTRL_CONFIG_BIAS_PULL_PIN_DEFAULT;
	}
	return pinctrl_config_set(dev, (u16_t)pin, config);
}

/**
 * @brief Enable pin input.
 *
 * @deprecated Use pinctrl_config_set() instead.
 */
static inline int pinmux_pin_input_enable(struct device *dev, u32_t pin,
					  u8_t func)
{
	int ret;
	u32_t config;

	ret = pinctrl_config_get(dev, (u16_t)pin, &config);
	if (ret != 0) {
		return ret;
	}
	config &= ~PINCTRL_CONFIG_INPUT_MASK;
	if (func == PINMUX_INPUT_ENABLED) {
		config |= PINCTRL_CONFIG_INPUT_ENABLE;
	} else {
		config |= PINCTRL_CONFIG_INPUT_DISABLE;
	}
	return pinctrl_config_set(dev, (u16_t)pin, config);
}

/**
 * @} pinctrl_interface_pinmux
 */

#endif /* defined(CONFIG_PINCTRL_PINMUX) */

#ifdef __cplusplus
}
#endif

/**
 * @} pinctrl_interface
 */

#include <syscalls/pinctrl.h>

#endif /* __INCLUDE_PINCTRL_H */
