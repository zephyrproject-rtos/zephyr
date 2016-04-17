/* adc.c - ADC test source file */

/*
 * Copyright (c) 2015 Intel Corporation.
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

#include <zephyr.h>

#include <device.h>
#include <misc/byteorder.h>
#include <adc.h>

#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#define DBG	printf
#else
#include <misc/printk.h>
#define DBG	printk
#endif

#define SLEEPTIME  2
#define SLEEPTICKS (SLEEPTIME * sys_clock_ticks_per_sec)

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
#define BUFFER_SIZE 40

static uint8_t seq_buffer[BUFFER_SIZE];

static struct adc_seq_entry sample = {
	.sampling_delay = 12,
	.channel_id = CHANNEL,
	.buffer = seq_buffer,
	.buffer_length = BUFFER_SIZE,
};

static struct adc_seq_table table = {
	.entries = &sample,
	.num_entries = 1,
};

static void _print_sample_in_hex(uint8_t *buf, uint32_t length)
{
	DBG("Buffer content:\n");
	for (; length > 0; length -= 4, buf += 4) {
		DBG("0x%x ", *((uint32_t *)buf));
	}
	DBG("\n");
}

void main(void)
{
	struct device *adc;
	struct nano_timer timer;
	uint32_t data[2] = {0, 0};

	DBG("ADC sample started on %s\n", ADC_DEVICE_NAME);

	adc = device_get_binding(ADC_DEVICE_NAME);
	if (!adc) {
		DBG("Cannot get adc controller\n");
		return;
	}

	nano_timer_init(&timer, data);
	adc_enable(adc);
	while (1) {
		if (adc_read(adc, &table) != 0) {
			DBG("Sampling could not proceed, an error occurred\n");
		} else {
			DBG("Sampling is done\n");
			_print_sample_in_hex(seq_buffer, BUFFER_SIZE);
		}
		nano_timer_start(&timer, SLEEPTICKS);
		nano_timer_test(&timer, TICKS_UNLIMITED);
	}
	adc_disable(adc);
}
