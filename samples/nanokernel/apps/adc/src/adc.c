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

#ifdef CONFIG_SOC_QUARK_SE_SS
	#define ADC_DEVICE_NAME CONFIG_ADC_DW_NAME_0
#elif CONFIG_BOARD_GALILEO
	#define ADC_DEVICE_NAME CONFIG_ADC_TI_ADC108S102_0_DRV_NAME
#endif

static uint8_t seq_buffer[100];
static struct adc_seq_entry sample = {
	.sampling_delay = 1,
	.channel_id = 0,
	.buffer = seq_buffer,
	.buffer_length = 100,
};

static struct adc_seq_table table = {
	.entries = &sample,
	.num_entries = 1,
};

static void _print_sample_in_hex(uint8_t *buf, uint32_t length)
{
	DBG("Buffer content:\n");
	for (; length > 0; length -= 2, buf += 2) {
		DBG("0x%x ", *((uint16_t *)buf));
	}
}

void main(void)
{
	struct device *adc;

	DBG("ADC sample started on %s\n", ADC_DEVICE_NAME);

	adc = device_get_binding(ADC_DEVICE_NAME);
	if (!adc) {
		DBG("Cannot get adc controller\n");
		return;
	}

	adc_enable(adc);

	if (adc_read(adc, &table) != DEV_OK) {
		DBG("Sampling could not proceed, an error occurred\n");
	} else {
		DBG("Sampling is done\n");
		_print_sample_in_hex(seq_buffer, 100);
	}

	adc_disable(adc);
}
