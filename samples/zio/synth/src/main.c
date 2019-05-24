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
#include <drivers/zio/synth.h>


static ZIO_BUF_DEFINE(synthbuf);

static struct k_poll_event events[1] = {
	K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_SEM_AVAILABLE,
			K_POLL_MODE_NOTIFY_ONLY, &synthbuf.sem, 0),
};

void main(void)
{
	int cnt = 0;
	int poll_cnt = 0;
	struct synth_datum datum;
	char out_str[64];
	struct device *synth = device_get_binding("SYNTH");

	if (synth == NULL) {
		printk("Could not get Synth device\n");
		return;
	}

	/*
	 * TODO something like the following to adjust sample rate and
	 * synthesized waveform frequency
	 * zio_dev_set_attr(synth, SYNTH_SAMPLE_RATE_IDX, 100);
	 * zio_dev_set_chan_attr(synth, 0, SYNTH_FREQUENCY_IDX, 10);
	 * zio_dev_set_chan_attr(synth, 1, SYNTH_FREQUENCY_IDX, 10);
	 */

	zio_buf_attach(&synthbuf, synth);

	while (poll_cnt < 1000) {
		int res = k_poll(events, 1, 100);

		if (res == 0) {
			while (zio_buf_pull(&synthbuf, &datum)) {
				poll_cnt++;
				/* Erase previous */
				printf("%d, %d, %d\n",
						poll_cnt, datum.samples[0],
						datum.samples[1]);
			}
		} else {
			/* manually trigger synth sample generation */
			k_sleep(K_MSEC(500));
			zio_dev_trigger(synth);
		}
	}
}
