/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <syscall_handler.h>
#include <i2c.h>

_SYSCALL_HANDLER(i2c_configure, dev, dev_config)
{
	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_I2C);
	return _impl_i2c_configure((struct device *)dev, dev_config);
}

_SYSCALL_HANDLER(i2c_transfer, dev, msgs, num_msgs, addr)
{
	int i;
	struct i2c_msg *msg;

	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_I2C);

	/* We need to be able to read the overall array of messages */
	_SYSCALL_MEMORY_ARRAY_READ(msgs, num_msgs, sizeof(struct i2c_msg));

	/* Validate the buffers in each message struct. Read options require
	 * that the target buffer be writable
	 */
	for (msg = (struct i2c_msg *)msgs, i = 0; i < num_msgs; i++, msg++) {
		_SYSCALL_MEMORY(msg->buf, msg->len, msg->flags & I2C_MSG_READ);
	}

	return _impl_i2c_transfer((struct device *)dev, (struct i2c_msg *)msgs,
				  (u8_t)num_msgs, (u16_t)addr);
}
