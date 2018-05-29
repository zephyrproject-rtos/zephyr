/*
 * Copyright (c) 2018 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <syscall_handler.h>
#include <can.h>

_SYSCALL_HANDLER(can_configure, dev, mode, bitrate) {

	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_CAN);

	return _impl_can_configure((struct device *)dev, mode, bitrate);
}

_SYSCALL_HANDLER(can_send, dev, msg) {
	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_CAN);

	/* We need to be able to read the data */
	_SYSCALL_MEMORY_ARRAY_READ(msg->data, msg->dlc, sizeof(msg->data));

	return _impl_can_send((struct device *)dev, (struct can_msg *)msg)
}

_SYSCALL_HANDLER(can_attach_msgq, dev, msgq, filter) {
	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_CAN);

	/* We need to be able to write the data */
	_SYSCALL_MEMORY_WRITE(msgq, sizeof(msgq));
	_SYSCALL_MEMORY_WRITE(msgq->buffer_start,
			      msgq->buffer_end - msgq->buffer_start);

	return _impl_can_attach_msgq((struct device *)dev,
				     (struct k_msgq *)msgq,
				     (const struct can_filter *) filter);
}

_SYSCALL_HANDLER(can_attach_isr, dev, isr, filter) {
	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_CAN);

	return _impl_can_attach_isr((struct device *)dev,
				    (can_rx_callback_t)isr,
				    (const struct can_filter *) filter);
}

_SYSCALL_HANDLER(can_detach, dev, filter_id) {
	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_CAN);

	return _impl_can_detach((struct device *)dev, (int)filter_id);
}
