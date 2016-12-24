/* ipm_dummy.c - Fake IPM driver */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _IPM_DUMMY_H_

#include <zephyr.h>
#include <device.h>
#include <ipm.h>

/* Arbitrary */
#define DUMMY_IPM_DATA_WORDS	4

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

int ipm_dummy_init(struct device *d);
#endif
