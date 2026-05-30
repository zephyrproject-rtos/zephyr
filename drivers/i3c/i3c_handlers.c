/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/i3c.h>
#include <string.h>
#include <zephyr/internal/syscall_handler.h>

static int copy_ccc_and_do(const struct device *dev,
			   struct i3c_ccc_payload *payload)
{
	struct i3c_ccc_payload copy;
	struct i3c_ccc_target_payload *user_payloads;
	struct i3c_ccc_target_payload *kernel_payloads = NULL;
	size_t i;
	int ret;

	/* Work from a local copy rather than the caller's live structure. */
	memcpy(&copy, payload, sizeof(copy));

	if (copy.ccc.data != NULL) {
		K_OOPS(K_SYSCALL_MEMORY_ARRAY_WRITE(copy.ccc.data,
						    copy.ccc.data_len,
						    sizeof(*copy.ccc.data)));
	}

	user_payloads = copy.targets.payloads;

	if (user_payloads != NULL) {
		K_OOPS(K_SYSCALL_VERIFY(copy.targets.num_targets >= 1 &&
					copy.targets.num_targets < 32));
		K_OOPS(K_SYSCALL_MEMORY_ARRAY_WRITE(user_payloads,
						    copy.targets.num_targets,
						    sizeof(*user_payloads)));

		/* Allocate a private copy of the target array rather than
		 * working directly off the caller-supplied pointer.
		 */
		kernel_payloads = k_usermode_alloc_from_copy(
			user_payloads,
			copy.targets.num_targets * sizeof(*user_payloads));
		K_OOPS(K_SYSCALL_VERIFY(kernel_payloads != NULL));

		copy.targets.payloads = kernel_payloads;

		for (i = 0; i < copy.targets.num_targets; i++) {
			if (kernel_payloads[i].data == NULL) {
				continue;
			}
			/* rnw=1: Read (driver writes buffer); rnw=0: Write (driver reads). */
			if (K_SYSCALL_MEMORY(kernel_payloads[i].data,
					     kernel_payloads[i].data_len,
					     kernel_payloads[i].rnw)) {
				k_free(kernel_payloads);
				K_OOPS(K_SYSCALL_VERIFY(false));
			}
		}

		ret = z_impl_i3c_do_ccc(dev, &copy);

		/*
		 * Propagate driver-written results back to the caller. Data
		 * buffers are written through directly (the pointers still
		 * reference user memory); only the scalar fields living in the
		 * snapshot need to be copied out.
		 */
		for (i = 0; i < copy.targets.num_targets; i++) {
			user_payloads[i].num_xfer = kernel_payloads[i].num_xfer;
			user_payloads[i].err = kernel_payloads[i].err;
		}

		k_free(kernel_payloads);
	} else {
		ret = z_impl_i3c_do_ccc(dev, &copy);
	}

	payload->ccc.num_xfer = copy.ccc.num_xfer;
	payload->ccc.err = copy.ccc.err;

	return ret;
}

static inline int z_vrfy_i3c_do_ccc(const struct device *dev,
				    struct i3c_ccc_payload *payload)
{
	K_OOPS(K_SYSCALL_DRIVER_I3C(dev, do_ccc));
	K_OOPS(K_SYSCALL_MEMORY_READ(payload, sizeof(*payload)));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(payload, sizeof(*payload)));

	return copy_ccc_and_do(dev, payload);
}
#include <zephyr/syscalls/i3c_do_ccc_mrsh.c>

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
		K_OOPS(K_SYSCALL_MEMORY(copy[i].buf, copy[i].len,
					copy[i].flags & I3C_MSG_READ));
	}

	return z_impl_i3c_transfer(target, copy, num_msgs);
}

static inline int z_vrfy_i3c_transfer(struct i3c_device_desc *target,
				      struct i3c_msg *msgs, uint8_t num_msgs)
{
	K_OOPS(K_SYSCALL_MEMORY_READ(target, sizeof(*target)));
	K_OOPS(K_SYSCALL_OBJ(target->bus, K_OBJ_DRIVER_I3C));

	/* copy_msgs_and_transfer() will allocate a copy on the stack using
	 * VLA, so ensure this won't blow the stack.  Most functions defined
	 * in i2c.h use only a handful of messages, so up to 32 messages
	 * should be more than sufficient.
	 */
	K_OOPS(K_SYSCALL_VERIFY(num_msgs >= 1 && num_msgs < 32));

	/* We need to be able to read the overall array of messages */
	K_OOPS(K_SYSCALL_MEMORY_ARRAY_READ(msgs, num_msgs,
					   sizeof(struct i3c_msg)));

	return copy_i3c_msgs_and_transfer((struct i3c_device_desc *)target,
					  (struct i3c_msg *)msgs,
					  (uint8_t)num_msgs);
}
#include <zephyr/syscalls/i3c_transfer_mrsh.c>
