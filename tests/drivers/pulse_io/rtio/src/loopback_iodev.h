/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PULSE_IO_TEST_LOOPBACK_IODEV_H_
#define PULSE_IO_TEST_LOOPBACK_IODEV_H_

#include <zephyr/rtio/rtio.h>

/* Initialise the test loopback iodev. Call once before submitting. */
void pulse_io_loopback_iodev_init(void);

/* Return the test loopback iodev. */
struct rtio_iodev *pulse_io_loopback_iodev(void);

#endif /* PULSE_IO_TEST_LOOPBACK_IODEV_H_ */
