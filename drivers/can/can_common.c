/*
 * Copyright (c) 2019 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <can.h>
#include <kernel.h>

#define LOG_LEVEL CONFIG_CAN_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(can_driver);

#define WORK_BUF_FULL 0xFFFF

static void can_msgq_put(struct zcan_frame *frame, void *arg)
{
	struct k_msgq *msgq= (struct k_msgq*)arg;
	int ret;

	__ASSERT_NO_MSG(msgq);

	ret = k_msgq_put(msgq, frame, K_NO_WAIT);
	if (ret) {
		LOG_ERR("Msgq %p overflowed. Frame ID: 0x%x", arg,
			frame->id_type == CAN_STANDARD_IDENTIFIER ? 
				frame->std_id : frame->ext_id);
	}
}

int z_impl_can_attach_msgq(struct device *dev, struct k_msgq *msg_q,
			   const struct zcan_filter *filter)
{
	const struct can_driver_api *api = dev->driver_api;

	return api->attach_isr(dev, can_msgq_put, msg_q, filter);
}
