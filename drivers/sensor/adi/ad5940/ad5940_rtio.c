/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ad5940.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(ad5940, CONFIG_SENSOR_LOG_LEVEL);

void ad5940_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
#ifdef CONFIG_AD5940_STREAM
	const struct sensor_read_config *cfg =
		(const struct sensor_read_config *)iodev_sqe->sqe.iodev->data;

	if (cfg->is_streaming) {
		ad5940_submit_stream(dev, iodev_sqe);
		return;
	}
#endif

	/* One-shot RTIO path not supported — use sensor_sample_fetch instead */
	rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
}
