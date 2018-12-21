/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <can.h>

int _impl_can_send(struct device *dev, struct can_msg *msg,
		   s32_t timeout, can_tx_callback_t callback_isr)
{
	const struct can_driver_api *api = dev->driver_api;

	return api->send(dev, msg, timeout, callback_isr);
}

int _impl_can_attach_msgq(struct device *dev,
			  struct k_msgq *msg_q,
			  const struct can_filter *filter)
{
	const struct can_driver_api *api = dev->driver_api;

	return api->attach_msgq(dev, msg_q, filter);
}

int _impl_can_attach_isr(struct device *dev,
			 can_rx_callback_t isr,
			 const struct can_filter *filter)
{
	const struct can_driver_api *api = dev->driver_api;

	return api->attach_isr(dev, isr, filter);
}

void _impl_can_detach(struct device *dev, int filter_id)
{
	const struct can_driver_api *api = dev->driver_api;

	return api->detach(dev, filter_id);
}

int _impl_can_configure(struct device *dev, enum can_mode mode,
			u32_t bitrate)
{
	const struct can_driver_api *api = dev->driver_api;

	return api->configure(dev, mode, bitrate);
}
