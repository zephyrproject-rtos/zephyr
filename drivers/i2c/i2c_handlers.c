/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <i2c.h>
#include <string.h>
#include <syscall_handler.h>

Z_SYSCALL_HANDLER(i2c_configure, dev, dev_config)
{
	Z_OOPS(Z_SYSCALL_DRIVER_I2C(dev, configure));
	return _impl_i2c_configure((struct device *)dev, dev_config);
}

static u32_t copy_msgs_and_transfer(struct device *dev,
				    const struct i2c_msg *msgs,
				    u8_t num_msgs,
				    u16_t addr,
				    void *ssf)
{
	struct i2c_msg copy[num_msgs];
	u8_t i;

	/* Use a local copy to avoid switcheroo attacks. */
	memcpy(copy, msgs, num_msgs * sizeof(*msgs));

	/* Validate the buffers in each message struct. Read options require
	 * that the target buffer be writable
	 */
	for (i = 0; i < num_msgs; i++) {
		Z_OOPS(Z_SYSCALL_MEMORY(copy[i].buf, copy[i].len,
					copy[i].flags & I2C_MSG_READ));
	}

	return _impl_i2c_transfer(dev, copy, num_msgs, addr);
}

Z_SYSCALL_HANDLER(i2c_transfer, dev, msgs, num_msgs, addr)
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

	return copy_msgs_and_transfer((struct device *)dev,
				      (struct i2c_msg *)msgs,
				      (u8_t)num_msgs, (u16_t)addr,
				      ssf);
}

Z_SYSCALL_HANDLER1_SIMPLE(i2c_slave_driver_register, K_OBJ_DRIVER_I2C,
			  struct device *);

Z_SYSCALL_HANDLER1_SIMPLE(i2c_slave_driver_unregister, K_OBJ_DRIVER_I2C,
			  struct device *);
