/*
 * Copyright (c) 2026 Mahendra Sondagar <mahendra@dotcom.co.in>
 * Copyright (c) 2026 Parin Baudhanwala <parin.baudhanwala@dnkmail.in>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RTIO submit function of the STHS34PF80 sensor driver.
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_STHS34PF80_STHS34PF80_RTIO_H_
#define ZEPHYR_DRIVERS_SENSOR_STHS34PF80_STHS34PF80_RTIO_H_

#include <zephyr/device.h>
#include <zephyr/rtio/rtio.h>

/**
 * @brief Submit a read of the requested channels.
 *
 * The read is queued as asynchronous bus transfers on the bus iodev of the
 * device and completed from a callback, with no work queue. The raw bytes
 * are stored in a struct sths34pf80_rtio_data frame, to be parsed and
 * converted by the decoder. Streaming mode is not supported.
 *
 * Ownership: the driver owns @p iodev_sqe from the moment this function is
 * called and must complete it exactly once, with rtio_iodev_sqe_ok() on
 * success or rtio_iodev_sqe_err() on failure. Completion happens either
 * directly in this function (on early errors) or later from the completion
 * callback once the bus transfers are done. The request must not be
 * accessed after it has been completed.
 *
 * @param dev        Sensor device instance.
 * @param iodev_sqe  RTIO submission queue entry holding the sensor read configuration.
 */
void sths34pf80_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);

#endif /* ZEPHYR_DRIVERS_SENSOR_STHS34PF80_STHS34PF80_RTIO_H_ */
