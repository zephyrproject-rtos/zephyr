/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "max30009.h"
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(MAX30009);

void max30009_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg =
		(const struct sensor_read_config *)iodev_sqe->sqe.iodev->data;

	if (cfg->is_streaming && IS_ENABLED(CONFIG_MAX30009_STREAM)) {
		max30009_submit_stream(dev, iodev_sqe);
	} else {
		/*
		 * One-shot (non-streaming) fetch is not implemented: there is no
		 * synchronous FIFO read path and the decoder rejects non-stream
		 * buffers (returns -ENOTSUP). Fail cleanly rather than completing
		 * with an unread buffer. Use streaming mode instead.
		 */
		LOG_ERR("One-shot read not supported; use streaming mode");
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
	}
}
