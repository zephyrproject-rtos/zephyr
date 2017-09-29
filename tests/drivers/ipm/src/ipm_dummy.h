/* ipm_dummy.c - Fake IPM driver */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _IPM_DUMMY_H_

#include <zephyr.h>
#include <device.h>
#include <ipm.h>

/* Arbitrary */
#define DUMMY_IPM_DATA_WORDS    4

struct ipm_dummy_regs {
	u32_t id;
	u32_t data[DUMMY_IPM_DATA_WORDS];
	u8_t busy;
	u8_t enabled;
};

struct ipm_dummy_driver_data {
	ipm_callback_t cb;
	void *cb_context;
	volatile struct ipm_dummy_regs regs;
};

int ipm_dummy_init(struct device *d);
#endif
