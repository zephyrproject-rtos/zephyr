/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/internal/syscall_handler.h>
#include <zephyr/drivers/eeprom.h>

static inline int z_vrfy_eeprom_read(const struct device *dev, off_t offset,
				     void *data, size_t len)
{
	K_OOPS(K_SYSCALL_DRIVER_EEPROM(dev, read));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(data, len));
	return z_impl_eeprom_read((const struct device *)dev, offset,
				  (void *)data,
				  len);
}
#include <zephyr/syscalls/eeprom_read_mrsh.c>

static inline int z_vrfy_eeprom_write(const struct device *dev, off_t offset,
				      const void *data, size_t len)
{
	K_OOPS(K_SYSCALL_DRIVER_EEPROM(dev, write));
	K_OOPS(K_SYSCALL_MEMORY_READ(data, len));
	return z_impl_eeprom_write((const struct device *)dev, offset,
				   (const void *)data, len);
}
#include <zephyr/syscalls/eeprom_write_mrsh.c>

static inline size_t z_vrfy_eeprom_get_size(const struct device *dev)
{
	K_OOPS(K_SYSCALL_DRIVER_EEPROM(dev, size));
	return z_impl_eeprom_get_size((const struct device *)dev);
}
#include <zephyr/syscalls/eeprom_get_size_mrsh.c>
