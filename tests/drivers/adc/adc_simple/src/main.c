/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

#include <device.h>
#include <misc/byteorder.h>
#include <adc.h>
#include <misc/printk.h>

/* in millisecond */
#define SLEEPTIME  2000

#define ADC_DEVICE_NAME "ADC_0"

/*
 * The analog input pin and channel number mapping
 * for Arduino 101 board.
 * A0 Channel 10
 * A1 Channel 11
 * A2 Channel 12
 * A3 Channel 13
 * A4 Channel 14
 */
#define CHANNEL 10
#define BUFFER_SIZE 10

static u32_t seq_buffer[2][BUFFER_SIZE];

static struct adc_seq_entry sample = {
	.sampling_delay = 30,
	.channel_id = CHANNEL,
	.buffer_length = BUFFER_SIZE * sizeof(seq_buffer[0][0])
};

static struct adc_seq_table table = {
	.entries = &sample,
	.num_entries = 1,
};

static void _print_sample_in_hex(const u32_t *buf, u32_t length)
{
	const u32_t *top;

	printk("Buffer content:\n");
	for (top = buf + length; buf < top; buf++)
		printk("0x%x ", *buf);
	printk("\n");
}

static long _abs(long x)
{
	return x < 0 ? -x : x;
}


static void adc_test(void)
{
	int result = TC_FAIL;
	struct device *adc;
	unsigned int loops = 10;
	unsigned int bufi0 = ~0, bufi;

	adc = device_get_binding(ADC_DEVICE_NAME);
	zassert_not_null(adc, "Cannot get adc controller\n");

	adc_enable(adc);
	while (loops--) {
		bufi = loops & 0x1;
		/* .buffer should be void * ... */
		sample.buffer = (void *) seq_buffer[bufi];
		result = adc_read(adc, &table);
		zassert_equal(result, 0, "Sampling could not proceed, "
			"an error occurred\n");
		printk("loop %u: sampling done to buffer #%u\n", loops, bufi);
		_print_sample_in_hex(seq_buffer[bufi], BUFFER_SIZE);
		if (bufi0 != ~0) {
			unsigned int cnt;
			long delta;

			for (cnt = 0; cnt < BUFFER_SIZE; cnt++) {
				delta = _abs((long)seq_buffer[bufi][cnt]
					     - seq_buffer[bufi0][cnt]);
				printk("loop %u delta %u = %ld\n",
				       loops, cnt, delta);
			}
		}
		k_sleep(SLEEPTIME);
		bufi0 = bufi;
	}
	adc_disable(adc);
}


void test_main(void)
{
	ztest_test_suite(_adc_test,
			 ztest_unit_test(adc_test));
	ztest_run_test_suite(_adc_test);
}

