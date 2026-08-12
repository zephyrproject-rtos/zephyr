/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/rtio/work.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>

#include "maxm86161.h"

LOG_MODULE_DECLARE(MAXM86161);

void maxm86161_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg =
		(const struct sensor_read_config *)iodev_sqe->sqe.iodev->data;

	if (cfg->is_streaming) {
		maxm86161_submit_stream(dev, iodev_sqe);
	} else {
		LOG_ERR("Only streaming mode supported");
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
	}
}
