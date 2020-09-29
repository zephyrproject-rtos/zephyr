/*
 * Copyright (c) 2018 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <syscall_handler.h>
#include <drivers/can.h>

static inline int z_vrfy_can_configure(const struct device *dev,
				       enum can_mode mode,
				       uint32_t bitrate)
{

	Z_OOPS(Z_SYSCALL_DRIVER_CAN(dev, configure));

	return z_impl_can_configure((const struct device *)dev,
				    (enum can_mode)mode,
				    (uint32_t)bitrate);
}
#include <syscalls/can_configure_mrsh.c>

static inline int z_vrfy_can_send(const struct device *dev,
				  const struct zcan_frame *msg,
				  k_timeout_t timeout,
				  can_tx_callback_t callback_isr,
				  void *callback_arg)
{
	Z_OOPS(Z_SYSCALL_DRIVER_CAN(dev, send));

	Z_OOPS(Z_SYSCALL_MEMORY_READ((const struct zcan_frame *)msg,
				      sizeof(struct zcan_frame)));
	Z_OOPS(Z_SYSCALL_MEMORY_READ(((struct zcan_frame *)msg)->data,
				     sizeof((struct zcan_frame *)msg)->data));
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(callback_isr == 0,
				    "callbacks may not be set from user mode"));

	Z_OOPS(Z_SYSCALL_MEMORY_READ((void *)callback_arg, sizeof(void *)));

	return z_impl_can_send((const struct device *)dev,
			       (const struct zcan_frame *)msg,
			       (k_timeout_t)timeout,
			       (can_tx_callback_t) callback_isr,
			       (void *)callback_arg);
}
#include <syscalls/can_send_mrsh.c>

static inline int z_vrfy_can_attach_msgq(const struct device *dev,
					 struct k_msgq *msgq,
					 const struct zcan_filter *filter)
{
	Z_OOPS(Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_CAN));

	Z_OOPS(Z_SYSCALL_MEMORY_READ((struct zcan_filter *)filter,
				     sizeof(struct zcan_filter)));
	Z_OOPS(Z_SYSCALL_OBJ(msgq, K_OBJ_MSGQ));

	return z_impl_can_attach_msgq((const struct device *)dev,
				      (struct k_msgq *)msgq,
				      (const struct zcan_filter *) filter);
}
#include <syscalls/can_attach_msgq_mrsh.c>

static inline void z_vrfy_can_detach(const struct device *dev, int filter_id)
{

	Z_OOPS(Z_SYSCALL_DRIVER_CAN(dev, detach));

	z_impl_can_detach((const struct device *)dev, (int)filter_id);
}
#include <syscalls/can_detach_mrsh.c>

static inline
enum can_state z_vrfy_can_get_state(const struct device *dev,
				    struct can_bus_err_cnt *err_cnt)
{

	Z_OOPS(Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_CAN));

	if (err_cnt) {
		Z_OOPS(Z_SYSCALL_MEMORY_WRITE(err_cnt, sizeof(err_cnt)));
	}

	return z_impl_can_get_state(dev, err_cnt);
}
#include <syscalls/can_get_state_mrsh.c>


#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
static inline int z_vrfy_can_recover(const struct device *dev,
				     k_timeout_t timeout)
{

	Z_OOPS(Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_CAN));

	return z_impl_can_recover(dev, k_timeout_t timeout);
}
#include <syscalls/can_recover_mrsh.c>
#endif /* CONFIG_CAN_AUTO_BUS_OFF_RECOVERY */
