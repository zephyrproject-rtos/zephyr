/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/i2c.h>
#include <string.h>
#include <zephyr/syscall_handler.h>

static inline int z_vrfy_i2c_configure(const struct device *dev,
				       uint32_t dev_config)
{
	Z_OOPS(Z_SYSCALL_DRIVER_I2C(dev, configure));
	return z_impl_i2c_configure((const struct device *)dev, dev_config);
}
#include <syscalls/i2c_configure_mrsh.c>

static inline int z_vrfy_i2c_get_config(const struct device *dev,
					uint32_t *dev_config)
{
	Z_OOPS(Z_SYSCALL_DRIVER_I2C(dev, get_config));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(dev_config, sizeof(uint32_t)));

	return z_impl_i2c_get_config(dev, dev_config);
}
#include <syscalls/i2c_get_config_mrsh.c>

static uint32_t copy_msgs_and_transfer(const struct device *dev,
				       const struct i2c_msg *msgs,
				       uint8_t num_msgs,
				       uint16_t addr)
{
	struct i2c_msg copy[num_msgs];
	uint8_t i;

	/* Use a local copy to avoid switcheroo attacks. */
	memcpy(copy, msgs, num_msgs * sizeof(*msgs));

	/* Validate the buffers in each message struct. Read options require
	 * that the target buffer be writable
	 */
	for (i = 0U; i < num_msgs; i++) {
		Z_OOPS(Z_SYSCALL_MEMORY(copy[i].buf, copy[i].len,
					copy[i].flags & I2C_MSG_READ));
	}

	return z_impl_i2c_transfer(dev, copy, num_msgs, addr);
}

static inline int z_vrfy_i2c_transfer(const struct device *dev,
				      struct i2c_msg *msgs, uint8_t num_msgs,
				      uint16_t addr)
{
	Z_OOPS(Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_I2C));

	/* copy_msgs_and_transfer() will allocate a copy on the stack using
	 * VLA, so ensure this won't blow the stack.  Most functions defined
	 * in i2c.h use only a handful of messages, so up to 32 messages
	 * should be more than sufficient.
	 */
	Z_OOPS(Z_SYSCALL_VERIFY(num_msgs >= 1 && num_msgs < 32));

	/* We need to be able to read the overall array of messages */
	Z_OOPS(Z_SYSCALL_MEMORY_ARRAY_READ(msgs, num_msgs,
					   sizeof(struct i2c_msg)));

	return copy_msgs_and_transfer((const struct device *)dev,
				      (struct i2c_msg *)msgs,
				      (uint8_t)num_msgs, (uint16_t)addr);
}
#include <syscalls/i2c_transfer_mrsh.c>

static inline int z_vrfy_i2c_target_driver_register(const struct device *dev)
{
	Z_OOPS(Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_I2C));
	return z_impl_i2c_target_driver_register(dev);
}
#include <syscalls/i2c_target_driver_register_mrsh.c>

static inline int z_vrfy_i2c_target_driver_unregister(const struct device *dev)
{
	Z_OOPS(Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_I2C));
	return z_impl_i2c_target_driver_unregister(dev);
}
#include <syscalls/i2c_target_driver_unregister_mrsh.c>

static inline int z_vrfy_i2c_recover_bus(const struct device *dev)
{
	Z_OOPS(Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_I2C));
	return z_impl_i2c_recover_bus(dev);
}
#include <syscalls/i2c_recover_bus_mrsh.c>
