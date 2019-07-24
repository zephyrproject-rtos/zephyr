/*
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <sensor.h>
#include <stdio.h>
#include <sys/util.h>
#include <zio/zio_dev.h>
#include <zio/zio_buf.h>
#include <drivers/zio/hts221.h>

static ZIO_BUF_DEFINE(hts221_buf);

static struct k_poll_event events[1] = {
	K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_SEM_AVAILABLE,
		K_POLL_MODE_NOTIFY_ONLY, &hts221_buf.sem, 0),
};

void main(void)
{
	struct hts221_datum datum;
	struct device *hts221 = device_get_binding(DT_INST_0_ST_HTS221_LABEL);

	if (hts221 == NULL) {
		printf("Could not get HTS221 device\n");
		return;
	}

	u32_t rate = 7;
	if (zio_dev_set_attr(hts221, HTS221_SAMPLE_RATE_IDX, rate) < 0) {
		printf("Could not set HTS221 rate\n");
		return;
	}

	zio_buf_attach(&hts221_buf, hts221);

	/* Get sensor samples */
	while (1) {
		int res = k_poll(events, 1, 100);

		if (res == 0) {
			while (zio_buf_pull(&hts221_buf, &datum)) {
				printf("\0033\014");

				printf("X-NUCLEO-IKS01A3 sensor dashboard\n\n");
				printf("hum --> %d\n", datum.hum_perc);
				printf("temp --> %d\n", datum.temp_degC);
			}
		}
	}
}
