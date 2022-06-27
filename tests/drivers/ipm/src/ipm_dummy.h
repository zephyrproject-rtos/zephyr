/* ipm_dummy.c - Fake IPM driver */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _IPM_DUMMY_H_

#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/drivers/ipm.h>

/* Arbitrary */
#define DUMMY_IPM_DATA_WORDS    4

struct ipm_dummy_regs {
	uint32_t id;
	uint32_t data[DUMMY_IPM_DATA_WORDS];
	uint8_t busy;
	uint8_t enabled;
};

struct ipm_dummy_driver_data {
	ipm_callback_t cb;
	void *cb_context;
	volatile struct ipm_dummy_regs regs;
};

int ipm_dummy_init(const struct device *d);
#endif
