/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/i3c.h>
#include <string.h>
#include <zephyr/syscall_handler.h>

static inline int z_vrfy_i3c_do_ccc(const struct device *dev,
				    struct i3c_ccc_payload *payload)
{
	Z_OOPS(Z_SYSCALL_DRIVER_I3C(dev, do_ccc));
	Z_OOPS(Z_SYSCALL_MEMORY_READ(payload, sizeof(*payload)));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(payload, sizeof(*payload)));

	if (payload->ccc.data != NULL) {
		Z_OOPS(Z_SYSCALL_MEMORY_ARRAY_READ(payload->ccc.data,
						   payload->ccc.data_len,
						   sizeof(*payload->ccc.data)));
		Z_OOPS(Z_SYSCALL_MEMORY_ARRAY_WRITE(payload->ccc.data,
						    payload->ccc.data_len,
						    sizeof(*payload->ccc.data)));
	}

	if (payload->targets.payloads != NULL) {
		Z_OOPS(Z_SYSCALL_MEMORY_ARRAY_READ(payload->targets.payloads,
						   payload->targets.num_targets,
						   sizeof(*payload->targets.payloads)));
		Z_OOPS(Z_SYSCALL_MEMORY_ARRAY_WRITE(payload->targets.payloads,
						    payload->targets.num_targets,
						    sizeof(*payload->targets.payloads)));
	}

	return z_impl_i3c_do_ccc(dev, payload);
}
#include <syscalls/i3c_do_ccc_mrsh.c>

static uint32_t copy_i3c_msgs_and_transfer(struct i3c_device_desc *target,
					   const struct i3c_msg *msgs,
					   uint8_t num_msgs)
{
	struct i3c_msg copy[num_msgs];
	uint8_t i;

	/* Use a local copy to avoid switcheroo attacks. */
	memcpy(copy, msgs, num_msgs * sizeof(*msgs));

	/* Validate the buffers in each message struct. Read options require
	 * that the target buffer be writable
	 */
	for (i = 0U; i < num_msgs; i++) {
		Z_OOPS(Z_SYSCALL_MEMORY(copy[i].buf, copy[i].len,
					copy[i].flags & I3C_MSG_READ));
	}

	return z_impl_i3c_transfer(target, copy, num_msgs);
}

static inline int z_vrfy_i3c_transfer(struct i3c_device_desc *target,
				      struct i3c_msg *msgs, uint8_t num_msgs)
{
	Z_OOPS(Z_SYSCALL_MEMORY_READ(target, sizeof(*target)));
	Z_OOPS(Z_SYSCALL_OBJ(target->bus, K_OBJ_DRIVER_I3C));

	/* copy_msgs_and_transfer() will allocate a copy on the stack using
	 * VLA, so ensure this won't blow the stack.  Most functions defined
	 * in i2c.h use only a handful of messages, so up to 32 messages
	 * should be more than sufficient.
	 */
	Z_OOPS(Z_SYSCALL_VERIFY(num_msgs >= 1 && num_msgs < 32));

	/* We need to be able to read the overall array of messages */
	Z_OOPS(Z_SYSCALL_MEMORY_ARRAY_READ(msgs, num_msgs,
					   sizeof(struct i3c_msg)));

	return copy_i3c_msgs_and_transfer((struct i3c_device_desc *)target,
					  (struct i3c_msg *)msgs,
					  (uint8_t)num_msgs);
}
#include <syscalls/i3c_transfer_mrsh.c>
