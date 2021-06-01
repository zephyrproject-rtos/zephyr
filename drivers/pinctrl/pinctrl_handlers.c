/*
 * Copyright (c) 2021 Piotr Mienkowski
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <syscall_handler.h>
#include <drivers/pinctrl.h>

int z_vrfy_pinctrl_pin_configure(const pinctrl_soc_pins_t *pin_spec)
{
	Z_SYSCALL_MEMORY_READ(pin_spec, sizeof(pinctrl_soc_pins_t));

	return z_impl_pinctrl_pin_configure(pin_spec);
}
#include <syscalls/pinctrl_pin_configure_mrsh.c>
