/*
 * Copyright (c) 2018 Bobby Noelte
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pinctrl.h>
#include <syscall_handlers.h>
#include <toolchain.h>

Z_SYSCALL_HANDLER(pinctrl_get_pins_count, dev)
{
	Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_PINCTRL);
	return _impl_pinctrl_get_pins_count((struct device *)dev);
}

Z_SYSCALL_HANDLER(pinctrl_get_groups_count, dev)
{
	Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_PINCTRL);
	return _impl_pinctrl_get_groups_count((struct device *)dev);
}

Z_SYSCALL_HANDLER(pinctrl_get_group_pins, dev, group, pins, num_pins)
{
	Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_PINCTRL);
	Z_SYSCALL_MEMORY_WRITE(pins, sizeof(u16_t) * *num_pins);
	Z_SYSCALL_MEMORY_WRITE(num_pins, sizeof(u16_t));
	return _impl_pinctrl_get_group_pins(
		(struct device *)dev, group, (u16_t *)pins, (u16_t *)num_pins);
}

Z_SYSCALL_HANDLER(pinctrl_get_states_count, dev)
{
	Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_PINCTRL);
	return _impl_pinctrl_get_states_count((struct device *)dev);
}

Z_SYSCALL_HANDLER(pinctrl_get_state_group, dev, state, group)
{
	Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_PINCTRL);
	Z_SYSCALL_MEMORY_WRITE(group, sizeof(u16_t));
	return pinctrl_get_state_group(
		(struct device *)dev, func, name, (u16_t *)group);
}

Z_SYSCALL_HANDLER(pinctrl_get_functions_count, dev)
{
	Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_PINCTRL);
	return _impl_pinctrl_get_functions_count((struct device *)dev);
}

Z_SYSCALL_HANDLER(pinctrl_get_function_group, dev, func, name, group)
{
	Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_PINCTRL);
	Z_SYSCALL_MEMORY_WRITE(group, sizeof(u16_t));
	return _impl_pinctrl_get_function_group(
		(struct device *)dev, func, name, (u16_t *)group);
}

Z_SYSCALL_HANDLER(pinctrl_get_function_groups, dev, func, groups, num_groups)
{
	Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_PINCTRL);
	Z_SYSCALL_MEMORY_WRITE(groups, sizeof(u16_t) * *num_groups);
	Z_SYSCALL_MEMORY_WRITE(num_groups, sizeof(u16_t));
	return _impl_pinctrl_get_function_groups((struct device *)dev,
						 func,
						 (u16_t *)groups,
						 (u16_t *)num_groups);
}

Z_SYSCALL_HANDLER(pinctrl_get_function_state, dev, func, name, state)
{
	Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_PINCTRL);
	Z_SYSCALL_MEMORY_WRITE(state, sizeof(u16_t));
	return _impl_pinctrl_get_function_state(
		(struct device *)dev, func, name, (u16_t *)state);
}

Z_SYSCALL_HANDLER(pinctrl_get_function_states, dev, func, states, num_states)
{
	Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_PINCTRL);
	Z_SYSCALL_MEMORY_WRITE(states, sizeof(u16_t) * *num_states);
	Z_SYSCALL_MEMORY_WRITE(num_states, sizeof(u16_t));
	return _impl_pinctrl_get_function_states((struct device *)dev,
						 func,
						 (u16_t *)states,
						 (u16_t *)num_states);
}

Z_SYSCALL_HANDLER(pinctrl_get_device_function, dev, other, func)
{
	Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_PINCTRL);
	Z_SYSCALL_MEMORY_WRITE(func, sizeof(u16_t));
	return _impl_pinctrl_get_device_function(
		(struct device *)dev, (struct device *)other, (u16_t *)func);
}

Z_SYSCALL_HANDLER(pinctrl_get_gpio_range, dev, gpio, gpio_pin, pin, base_pin,
		  num_pins)
{
	Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_PINCTRL);
	Z_SYSCALL_OBJ(gpio, K_OBJ_DRIVER_GPIO);
	Z_SYSCALL_MEMORY_WRITE(pin, sizeof(u16_t));
	Z_SYSCALL_MEMORY_WRITE(base_pin, sizeof(u16_t));
	Z_SYSCALL_MEMORY_WRITE(num_pins, sizeof(u8_t));
	return _impl_pinctrl_get_gpio_range((struct device *)dev,
					    (struct device *)gpio,
					    gpio_pin,
					    (u16_t *)pin,
					    (u16_t *)base_pin,
					    (u8_t *)num_pins);
}

Z_SYSCALL_HANDLER(pinctrl_config_get, dev, pin, config)
{
	Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_PINCTRL);
	Z_SYSCALL_MEMORY_WRITE(config, sizeof(u32_t));
	return _impl_pinctrl_config_get(
		(struct device *)dev, pin, (u32_t *)config);
}

Z_SYSCALL_HANDLER(pinctrl_config_set, dev, pin, config)
{
	Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_PINCTRL);
	return _impl_pinctrl_config_set((struct device *)dev, pin, config);
}

Z_SYSCALL_HANDLER(pinctrl_config_group_get, dev, group, configs, num_configs)
{
	Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_PINCTRL);
	Z_SYSCALL_MEMORY_WRITE(configs, sizeof(u32_t) * *num_configs);
	Z_SYSCALL_MEMORY_WRITE(num_configs, sizeof(u16_t));
	return _impl_pinctrl_config_group_get((struct device *)dev,
					      group,
					      (u32_t *)configs,
					      (u16_t *)num_configs);
}

Z_SYSCALL_HANDLER(pinctrl_config_group_set, dev, group, configs, num_configs)
{
	Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_PINCTRL);
	return _impl_pinctrl_config_group_get(
		(struct device *)dev, group, configs, num_configs);
}

Z_SYSCALL_HANDLER(pinctrl_mux_request, dev, pin, owner)
{
	Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_PINCTRL);
	return _impl_pinctrl_mux_request((struct device *)dev, pin, owner);
}

Z_SYSCALL_HANDLER(pinctrl_mux_free, dev, pin, owner)
{
	Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_PINCTRL);
	return _impl_pinctrl_mux_free((struct device *)dev, pin, owner);
}

Z_SYSCALL_HANDLER(pinctrl_mux_get, dev, pin, func)
{
	Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_PINCTRL);
	Z_SYSCALL_MEMORY_WRITE(func, sizeof(u32_t));
	return _impl_pinctrl_mux_get((struct device *)dev, pin, (u32_t *)func);
}

Z_SYSCALL_HANDLER(pinctrl_mux_set, dev, pin, func)
{
	Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_PINCTRL);
	return _impl_pinctrl_mux_set((struct device *)dev, pin, func);
}

Z_SYSCALL_HANDLER(pinctrl_mux_group_set, dev, group, func)
{
	Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_PINCTRL);
	return _impl_pinctrl_mux_group_set((struct device *)dev, group, func);
}

Z_SYSCALL_HANDLER(pinctrl_state_set, dev, state)
{
	Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_PINCTRL);
	return _impl_pinctrl_mux_get((struct device *)dev, state);
}
