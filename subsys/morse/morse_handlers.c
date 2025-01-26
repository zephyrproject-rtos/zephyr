/*
 * Copyright (c) 2024-2025 Freedom Veiculos Eletricos
 * Copyright (c) 2024-2025 O.S. Systems Software LTDA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/internal/syscall_handler.h>
#include <zephyr/morse/morse_device.h>

static inline int z_vrfy_morse_set_tx_bit_state(const struct device *dev,
						enum morse_bit_state state)
{
	K_OOPS(K_SYSCALL_DRIVER_MORSE(dev, set_tx_bit_state));
	return z_impl_morse_set_tx_bit_state((const struct device *)dev, state);
}
#include <zephyr/syscalls/morse_set_tx_bit_state_mrsh.c>
