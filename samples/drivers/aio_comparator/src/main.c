/*
 * Copyright (c) 2016 Intel Corporation
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

/**
 * @file main.c - DesignWare AIO/Comparator demo on Arduino 101
 *
 * This is used to demo the DesignWare AIO/Comparator. The voltage input
 * pin is analog in A0 on circuit board, which maps to AIN[10] on chip.
 *
 * The comparion is using the internal 3.3V as reference voltage,
 * so it needs a higher voltage to trigger comparator.
 *
 * To test:
 * 1. Connect the A0 pin to ground via a resistor. Any larger than
 *    1k Ohm would be fine. This is to avoid floating pin.
 * 2. Turn on the device.
 * 3. Wait for device to boot, until "app started" line appeared.
 * 4. Connect a voltage source higher than 3.3V (the 5V line would work).
 *    The line "*** A0, AIN[10] triggered rising." should appear.
 * 5. Remove the voltage source.
 *    The line "*** A0, AIN[10] triggered falling." should appear.
 */

#include <zephyr.h>
#include <stdint.h>
#include <stdio.h>
#include <device.h>
#include <aio_comparator.h>

/* specify delay between greetings (in ms) */
#define SLEEPTIME  5000

struct cb_data_t {
	uint8_t ain_idx;
	enum aio_cmp_ref ref;
	enum aio_cmp_polarity pol;
	char name[50];
};

struct cb_data_t cb_data = {
	.ain_idx = 10,
	.ref = AIO_CMP_REF_A,
	.pol = AIO_CMP_POL_RISE,
	.name = "A0, AIN[10]",
};

void cb(void *param)
{
	struct device *aio_cmp_dev;
	struct cb_data_t *p = (struct cb_data_t *)param;

	aio_cmp_dev = device_get_binding("AIO_CMP_0");

	printf("*** %s triggered %s.\n", &p->name,
	      (p->pol == AIO_CMP_POL_RISE) ? "rising" : "falling"
	);

	if (p->pol == AIO_CMP_POL_RISE)
		p->pol = AIO_CMP_POL_FALL;
	else
		p->pol = AIO_CMP_POL_RISE;

	aio_cmp_configure(aio_cmp_dev, p->ain_idx, p->pol, p->ref, cb, p);
}

void main(void)
{
	struct device *aio_cmp_dev;
	int i, ret;
	int cnt = 0;

	aio_cmp_dev = device_get_binding("AIO_CMP_0");

	printf("===== app started ========\n");

	for (i = 0; i < 4; i++) {
		/* REF_A is to use AREF for reference */
		ret = aio_cmp_configure(aio_cmp_dev, cb_data.ain_idx,
					cb_data.pol, cb_data.ref,
					cb, &cb_data);
		if (ret)
			printf("ERROR registering callback for %s (%d)\n",
			      &cb_data.name, ret);
	}

	while (1) {
		printf("... waiting for event! (%d)\n", ++cnt);

		/* wait a while */
		k_sleep(SLEEPTIME);
	}
}

