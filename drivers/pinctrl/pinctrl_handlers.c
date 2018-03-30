/*
 * Copyright (c) 2018 Bobby Noelte
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pinctrl.h>
#include <syscall_handler.h>
#include <toolchain.h>

_SYSCALL_HANDLER(pinctrl_get_pins_count, dev)
{
	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_PINCTRL);
	return _impl_pinctrl_get_pins_count((struct device *)dev);
}

_SYSCALL_HANDLER(pinctrl_get_groups_count, dev)
{
	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_PINCTRL);
	return _impl_pinctrl_get_groups_count((struct device *)dev);
}

_SYSCALL_HANDLER(pinctrl_get_group_pins, dev, group, pins, num_pins)
{
	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_PINCTRL);
	_SYSCALL_MEMORY_WRITE(pins, sizeof(u16_t) * *num_pins);
	_SYSCALL_MEMORY_WRITE(num_pins, sizeof(u16_t));
	return _impl_pinctrl_get_group_pins(
		(struct device *)dev, group, (u16_t *)pins, (u16_t *)num_pins);
}

_SYSCALL_HANDLER(pinctrl_get_functions_count, dev)
{
	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_PINCTRL);
	return _impl_pinctrl_get_functions_count((struct device *)dev);
}

_SYSCALL_HANDLER(pinctrl_get_function_group, dev, func, name, group)
{
	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_PINCTRL);
	_SYSCALL_MEMORY_WRITE(group, sizeof(u32_t));
	return _impl_pinctrl_get_function_group(
		(struct device *)dev, func, name, (u32_t *)group);
}

_SYSCALL_HANDLER(pinctrl_get_function_groups, dev, func, groups, num_groups)
{
	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_PINCTRL);
	_SYSCALL_MEMORY_WRITE(groups, sizeof(u32_t) * *num_groups);
	_SYSCALL_MEMORY_WRITE(num_groups, sizeof(u16_t));
	return _impl_pinctrl_get_function_groups((struct device *)dev,
						 func,
						 (u32_t *)groups,
						 (u16_t *)num_groups);
}

_SYSCALL_HANDLER(pinctrl_get_function_state, dev, func, name, state)
{
	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_PINCTRL);
	_SYSCALL_MEMORY_WRITE(state, sizeof(u16_t));
	return _impl_pinctrl_get_function_state(
		(struct device *)dev, func, name, (u16_t *)state);
}

_SYSCALL_HANDLER(pinctrl_get_device_function, dev, other, func)
{
	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_PINCTRL);
	_SYSCALL_MEMORY_WRITE(func, sizeof(u32_t));
	return _impl_pinctrl_get_device_function(
		(struct device *)dev, (struct device *)other, (u32_t *)func);
}

_SYSCALL_HANDLER(pinctrl_config_get, dev, pin, config)
{
	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_PINCTRL);
	_SYSCALL_MEMORY_WRITE(config, sizeof(u32_t));
	return _impl_pinctrl_config_get(
		(struct device *)dev, pin, (u32_t *)config);
}

_SYSCALL_HANDLER(pinctrl_config_set, dev, pin, config)
{
	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_PINCTRL);
	return _impl_pinctrl_config_set((struct device *)dev, pin, config);
}

_SYSCALL_HANDLER(pinctrl_config_group_get, dev, group, configs, num_configs)
{
	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_PINCTRL);
	_SYSCALL_MEMORY_WRITE(configs, sizeof(u32_t) * *num_configs);
	_SYSCALL_MEMORY_WRITE(num_configs, sizeof(u16_t));
	return _impl_pinctrl_config_group_get((struct device *)dev,
					      group,
					      (u32_t *)configs,
					      (u16_t *)num_configs);
}

_SYSCALL_HANDLER(pinctrl_config_group_set, dev, group, configs, num_configs)
{
	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_PINCTRL);
	return _impl_pinctrl_config_group_get(
		(struct device *)dev, group, configs, num_configs);
}

_SYSCALL_HANDLER(pinctrl_mux_request, dev, pin, owner)
{
	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_PINCTRL);
	return _impl_pinctrl_mux_request((struct device *)dev, pin, owner);
}

_SYSCALL_HANDLER(pinctrl_mux_free, dev, pin, owner)
{
	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_PINCTRL);
	return _impl_pinctrl_mux_free((struct device *)dev, pin, owner);
}

_SYSCALL_HANDLER(pinctrl_mux_get, dev, pin, func)
{
	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_PINCTRL);
	_SYSCALL_MEMORY_WRITE(func, sizeof(u32_t));
	return _impl_pinctrl_mux_get((struct device *)dev, pin, (u32_t *)func);
}

_SYSCALL_HANDLER(pinctrl_mux_set, dev, pin, func)
{
	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_PINCTRL);
	return _impl_pinctrl_mux_set((struct device *)dev, pin, func);
}

_SYSCALL_HANDLER(pinctrl_mux_group_set, dev, group, func)
{
	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_PINCTRL);
	return _impl_pinctrl_mux_group_set((struct device *)dev, group, func);
}

_SYSCALL_HANDLER(pinctrl_state_set, dev, state)
{
	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_PINCTRL);
	return _impl_pinctrl_mux_get((struct device *)dev, state);
}
