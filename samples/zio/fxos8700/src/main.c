/*
 * Copyright (c) 2018 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <zio/zio_dev.h>
#include <zio/zio_buf.h>
#include <stdio.h>
#include <misc/util.h>
#include <drivers/zio/fxos8700.h>


static ZIO_BUF_DEFINE(fxosbuf);

static struct k_poll_event events[1] = {
	K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_SEM_AVAILABLE,
			K_POLL_MODE_NOTIFY_ONLY, &fxosbuf.sem, 0),
};

void main(void)
{
	int poll_cnt = 0;
	struct fxos8700_datum datum;
	struct device *fxos8700 = device_get_binding(DT_NXP_FXOS8700_0_LABEL);

	if (fxos8700 == NULL) {
		printk("Could not get FXOS8700 device\n");
		return;
	}

	zio_buf_attach(&fxosbuf, fxos8700);

	while (poll_cnt < 1000) {
		int res = k_poll(events, 1, 100);

		if (res == 0) {
			while (zio_buf_pull(&fxosbuf, &datum)) {
				poll_cnt++;
				/* Erase previous */
				printk("%d, %d, %d, %d\n",
						poll_cnt, datum.acc_x,
						datum.acc_y, datum.acc_z);
			}
		} else {
			/* manually trigger fxos8700 sample collection */
			k_sleep(K_MSEC(500));
			zio_dev_trigger(fxos8700);
		}
	}
}
