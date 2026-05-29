/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_PTP_CLOCK_PTP_CLOCK_NATIVE_BOTTOM_H_
#define ZEPHYR_DRIVERS_PTP_CLOCK_PTP_CLOCK_NATIVE_BOTTOM_H_

#include <stdint.h>

int ptp_clock_native_gettime(uint64_t *second, uint32_t *nanosecond);

#endif /* ZEPHYR_DRIVERS_PTP_CLOCK_PTP_CLOCK_NATIVE_BOTTOM_H_ */
