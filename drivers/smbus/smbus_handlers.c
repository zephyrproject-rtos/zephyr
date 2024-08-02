/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/slist.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/drivers/smbus.h>

static inline int z_vrfy_smbus_configure(const struct device *dev,
					 uint32_t dev_config)
{
	K_OOPS(K_SYSCALL_DRIVER_SMBUS(dev, configure));

	return z_impl_smbus_configure(dev, dev_config);
}
#include <zephyr/syscalls/smbus_configure_mrsh.c>

static inline int z_vrfy_smbus_get_config(const struct device *dev,
					  uint32_t *dev_config)
{
	K_OOPS(K_SYSCALL_DRIVER_SMBUS(dev, get_config));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(dev_config, sizeof(uint32_t)));

	return z_impl_smbus_get_config(dev, dev_config);
}
#include <zephyr/syscalls/smbus_get_config_mrsh.c>

static inline int z_vrfy_smbus_quick(const struct device *dev, uint16_t addr,
				     enum smbus_direction rw)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_SMBUS));

	return z_impl_smbus_quick(dev, addr, rw);
}
#include <zephyr/syscalls/smbus_quick_mrsh.c>

static inline int z_vrfy_smbus_byte_write(const struct device *dev,
					  uint16_t addr, uint8_t byte)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_SMBUS));

	return z_impl_smbus_byte_write(dev, addr, byte);
}
#include <zephyr/syscalls/smbus_byte_write_mrsh.c>

static inline int z_vrfy_smbus_byte_read(const struct device *dev,
					 uint16_t addr, uint8_t *byte)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_SMBUS));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(byte, sizeof(uint8_t)));

	return z_impl_smbus_byte_read(dev, addr, byte);
}
#include <zephyr/syscalls/smbus_byte_read_mrsh.c>

static inline int z_vrfy_smbus_byte_data_write(const struct device *dev,
					       uint16_t addr, uint8_t cmd,
					       uint8_t byte)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_SMBUS));

	return z_impl_smbus_byte_data_write(dev, addr, cmd, byte);
}
#include <zephyr/syscalls/smbus_byte_data_write_mrsh.c>

static inline int z_vrfy_smbus_byte_data_read(const struct device *dev,
					       uint16_t addr, uint8_t cmd,
					       uint8_t *byte)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_SMBUS));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(byte, sizeof(uint8_t)));

	return z_impl_smbus_byte_data_read(dev, addr, cmd, byte);
}
#include <zephyr/syscalls/smbus_byte_data_read_mrsh.c>

static inline int z_vrfy_smbus_word_data_write(const struct device *dev,
					       uint16_t addr, uint8_t cmd,
					       uint16_t word)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_SMBUS));

	return z_impl_smbus_word_data_write(dev, addr, cmd, word);
}
#include <zephyr/syscalls/smbus_word_data_write_mrsh.c>

static inline int z_vrfy_smbus_word_data_read(const struct device *dev,
					      uint16_t addr, uint8_t cmd,
					      uint16_t *word)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_SMBUS));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(word, sizeof(uint16_t)));

	return z_impl_smbus_word_data_read(dev, addr, cmd, word);
}
#include <zephyr/syscalls/smbus_word_data_read_mrsh.c>

static inline int z_vrfy_smbus_pcall(const struct device *dev,
				     uint16_t addr, uint8_t cmd,
				     uint16_t send_word, uint16_t *recv_word)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_SMBUS));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(recv_word, sizeof(uint16_t)));

	return z_impl_smbus_pcall(dev, addr, cmd, send_word, recv_word);
}
#include <zephyr/syscalls/smbus_pcall_mrsh.c>

static inline int z_vrfy_smbus_block_write(const struct device *dev,
					   uint16_t addr, uint8_t cmd,
					   uint8_t count, uint8_t *buf)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_SMBUS));
	K_OOPS(K_SYSCALL_MEMORY_READ(buf, count));

	return z_impl_smbus_block_write(dev, addr, cmd, count, buf);
}
#include <zephyr/syscalls/smbus_block_write_mrsh.c>

static inline int z_vrfy_smbus_block_read(const struct device *dev,
					  uint16_t addr, uint8_t cmd,
					  uint8_t *count, uint8_t *buf)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_SMBUS));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(count, sizeof(uint8_t)));

	return z_impl_smbus_block_read(dev, addr, cmd, count, buf);
}
#include <zephyr/syscalls/smbus_block_read_mrsh.c>

static inline int z_vrfy_smbus_block_pcall(const struct device *dev,
					   uint16_t addr, uint8_t cmd,
					   uint8_t snd_count, uint8_t *snd_buf,
					   uint8_t *rcv_count, uint8_t *rcv_buf)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_SMBUS));
	K_OOPS(K_SYSCALL_MEMORY_READ(snd_buf, snd_count));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(rcv_count, sizeof(uint8_t)));

	return z_impl_smbus_block_pcall(dev, addr, cmd, snd_count, snd_buf,
					rcv_count, rcv_buf);
}
#include <zephyr/syscalls/smbus_block_pcall_mrsh.c>

static inline int z_vrfy_smbus_smbalert_remove_cb(const struct device *dev,
						  struct smbus_callback *cb)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_SMBUS));

	return z_impl_smbus_smbalert_remove_cb(dev, cb);
}
#include <zephyr/syscalls/smbus_smbalert_remove_cb_mrsh.c>

static inline int z_vrfy_smbus_host_notify_remove_cb(const struct device *dev,
						     struct smbus_callback *cb)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_SMBUS));

	return z_impl_smbus_host_notify_remove_cb(dev, cb);
}
#include <zephyr/syscalls/smbus_host_notify_remove_cb_mrsh.c>
