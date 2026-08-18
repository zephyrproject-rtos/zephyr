/*
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 */

#include <zephyr/drivers/board_id.h>
#include <zephyr/internal/syscall_handler.h>

static inline int z_vrfy_board_id_read(const struct device *dev, uint8_t *buffer, size_t length)
{
	K_OOPS(K_SYSCALL_DRIVER_BOARD_ID(dev, read));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(buffer, length));

	return z_impl_board_id_read(dev, buffer, length);
}

#include <zephyr/syscalls/board_id_read_mrsh.c>
