/* ST Microelectronics IIS3DWB accel sensor driver
 *
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_IIS3DWB_RTIO_H_
#define ZEPHYR_DRIVERS_SENSOR_IIS3DWB_RTIO_H_

#include <zephyr/device.h>
#include <zephyr/rtio/rtio.h>

void iis3dwb_submit(const struct device *sensor, struct rtio_iodev_sqe *iodev_sqe);
void iis3dwb_submit_stream(const struct device *sensor, struct rtio_iodev_sqe *iodev_sqe);
void iis3dwb_stream_irq_handler(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_SENSOR_IIS3DWB_RTIO_H_ */
