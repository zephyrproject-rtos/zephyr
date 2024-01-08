/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/drivers/misc/timeaware_gpio/timeaware_gpio.h>
#include <zephyr/syscall_handler.h>

static inline int z_vrfy_tgpio_port_get_time(const struct device *port, uint64_t *current_time)
{
	K_OOPS(Z_SYSCALL_DRIVER_TGPIO(port, get_time));
	K_OOPS(Z_SYSCALL_MEMORY_WRITE(current_time, sizeof(uint64_t)));
	return z_impl_tgpio_port_get_time((const struct device *)port, (uint64_t *)current_time);
}
#include <syscalls/tgpio_port_get_time_mrsh.c>

static inline int z_vrfy_tgpio_port_get_cycles_per_second(const struct device *port,
							   uint32_t *cycles)
{
	K_OOPS(Z_SYSCALL_DRIVER_TGPIO(port, cyc_per_sec));
	K_OOPS(Z_SYSCALL_MEMORY_WRITE(cycles, sizeof(uint32_t)));
	return z_impl_tgpio_port_get_cycles_per_second((const struct device *)port,
							(uint32_t *)cycles);
}
#include <syscalls/tgpio_port_get_cycles_per_second_mrsh.c>

static inline int z_vrfy_tgpio_pin_periodic_output(const struct device *port, uint32_t pin,
						    uint64_t start_time, uint64_t repeat_interval,
						    bool periodic_enable)
{
	K_OOPS(Z_SYSCALL_DRIVER_TGPIO(port, set_perout));
	return z_impl_tgpio_pin_periodic_output((const struct device *)port, pin, start_time,
						 repeat_interval, periodic_enable);
}
#include <syscalls/tgpio_pin_periodic_output_mrsh.c>

static inline int z_vrfy_tgpio_pin_disable(const struct device *port, uint32_t pin)
{
	K_OOPS(Z_SYSCALL_DRIVER_TGPIO(port, pin_disable));
	return z_impl_tgpio_pin_disable((const struct device *)port, pin);
}
#include <syscalls/tgpio_pin_disable_mrsh.c>

static inline int z_vrfy_tgpio_pin_config_ext_timestamp(const struct device *port, uint32_t pin,
							 uint32_t event_polarity)
{
	K_OOPS(Z_SYSCALL_DRIVER_TGPIO(port, config_ext_ts));
	return z_impl_tgpio_pin_config_ext_timestamp((const struct device *)port, pin,
						      event_polarity);
}
#include <syscalls/tgpio_pin_config_ext_timestamp_mrsh.c>

static inline int z_vrfy_tgpio_pin_read_ts_ec(const struct device *port, uint32_t pin,
					       uint64_t *timestamp, uint64_t *event_count)
{
	K_OOPS(Z_SYSCALL_DRIVER_TGPIO(port, read_ts_ec));
	return z_impl_tgpio_pin_read_ts_ec((const struct device *)port, pin, (uint64_t *)timestamp,
					    (uint64_t *)event_count);
}
#include <syscalls/tgpio_pin_read_ts_ec_mrsh.c>
