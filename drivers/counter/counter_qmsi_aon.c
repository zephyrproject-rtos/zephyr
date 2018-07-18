/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <device.h>
#include <init.h>

#include <counter.h>

#include "qm_aon_counters.h"

static int aon_counter_qmsi_start(struct device *dev)
{
	if (qm_aonc_enable(QM_AONC_0)) {
		return -EIO;
	}

	return 0;
}

static int aon_counter_qmsi_stop(struct device *dev)
{
	qm_aonc_disable(QM_AONC_0);

	return 0;
}

static u32_t aon_counter_qmsi_read(struct device *dev)
{
	u32_t value;

	qm_aonc_get_value(QM_AONC_0, (uint32_t *)&value);

	return value;
}

static int aon_counter_qmsi_set_alarm(struct device *dev,
				      counter_callback_t callback,
				      u32_t count, void *user_data)
{
	return -ENODEV;
}

static const struct counter_driver_api aon_counter_qmsi_api = {
	.start = aon_counter_qmsi_start,
	.stop = aon_counter_qmsi_stop,
	.read = aon_counter_qmsi_read,
	.set_alarm = aon_counter_qmsi_set_alarm,
};

static int aon_counter_init(struct device *dev)
{
	return 0;
}

DEVICE_AND_API_INIT(aon_counter, CONFIG_AON_COUNTER_QMSI_DEV_NAME,
		    aon_counter_init, NULL, NULL, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &aon_counter_qmsi_api);
