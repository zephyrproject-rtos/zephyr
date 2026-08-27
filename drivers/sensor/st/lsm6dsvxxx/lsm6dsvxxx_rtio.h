/* ST Microelectronics LSM6DSVXXX family IMU sensor
 *
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lsm6dsv320x.pdf
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_LSM6DSVXXX_RTIO_H_
#define ZEPHYR_DRIVERS_SENSOR_LSM6DSVXXX_RTIO_H_

#include <zephyr/device.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/rtio/work.h>

void lsm6dsvxxx_submit(const struct device *sensor, struct rtio_iodev_sqe *iodev_sqe);
void lsm6dsvxxx_submit_stream(const struct device *sensor, struct rtio_iodev_sqe *iodev_sqe);
void lsm6dsvxxx_stream_irq_handler(const struct device *dev);

int lsm6dsvxxx_gbias_config(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr,
			    const struct sensor_value *val);
int lsm6dsvxxx_gbias_get_config(const struct device *dev, enum sensor_channel chan,
				enum sensor_attribute attr, struct sensor_value *val);
#endif /* ZEPHYR_DRIVERS_SENSOR_LSM6DSVXXX_RTIO_H_ */
