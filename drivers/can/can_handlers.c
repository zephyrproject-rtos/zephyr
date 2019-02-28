/*
 * Copyright (c) 2018 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <syscall_handler.h>
#include <can.h>

Z_SYSCALL_HANDLER(can_configure, dev, mode, bitrate) {

	Z_OOPS(Z_SYSCALL_DRIVER_CAN(dev, configure));

	return _impl_can_configure((struct device *)dev, (enum can_mode)mode,
				   (u32_t)bitrate);
}

Z_SYSCALL_HANDLER(can_send, dev, msg, timeout, callback_isr) {

	Z_OOPS(Z_SYSCALL_DRIVER_CAN(dev, send));

	Z_OOPS(Z_SYSCALL_MEMORY_READ((const struct zcan_frame *)msg,
				      sizeof(struct zcan_frame)));
	Z_OOPS(Z_SYSCALL_MEMORY_READ(((struct zcan_frame *)msg)->data,
				     sizeof((struct zcan_frame *)msg)->data));

	return _impl_can_send((struct device *)dev,
			      (const struct zcan_frame *)msg,
			      (s32_t)timeout, (can_tx_callback_t) callback_isr);
}

Z_SYSCALL_HANDLER(can_attach_msgq, dev, msgq, filter) {

	Z_OOPS(Z_SYSCALL_DRIVER_CAN(dev, attach_msgq));

	Z_OOPS(Z_SYSCALL_MEMORY_READ((struct zcan_filter *)filter,
				     sizeof(struct zcan_filter)));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE((struct k_msgq *)msgq,
				      sizeof(struct k_msgq)));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(((struct k_msgq *)msgq)->buffer_start,
				      ((struct k_msgq *)msgq)->buffer_end -
				      ((struct k_msgq *)msgq)->buffer_start));

	return _impl_can_attach_msgq((struct device *)dev,
				     (struct k_msgq *)msgq,
				     (const struct zcan_filter *) filter);
}

Z_SYSCALL_HANDLER(can_attach_isr, dev, isr, filter) {

	Z_OOPS(Z_SYSCALL_DRIVER_CAN(dev, attach_isr));

	Z_OOPS(Z_SYSCALL_MEMORY_READ((struct zcan_filter *)filter,
				     sizeof(struct zcan_filter)));

	return _impl_can_attach_isr((struct device *)dev,
				    (can_rx_callback_t)isr,
				    (const struct zcan_filter *) filter);
}

Z_SYSCALL_HANDLER(can_detach, dev, filter_id) {

	Z_OOPS(Z_SYSCALL_DRIVER_CAN(dev, detach));

	_impl_can_detach((struct device *)dev, (int)filter_id);

	return 0;
}
