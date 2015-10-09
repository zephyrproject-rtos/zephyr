/* adc.c - ADC test source file */

/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <nanokernel.h>
#include <arch/cpu.h>

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

#define SLEEPTIME  10
#define SLEEPTICKS (SLEEPTIME * sys_clock_ticks_per_sec)

static int cb_count;

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

static void callback(struct device *dev, enum adc_callback_type cb_type)
{
	DBG("ADC callback %d - type %d\n", cb_count++, cb_type);

	if (cb_type == ADC_CB_DONE) {
		DBG("Sampling is done\n");
		_print_sample_in_hex(seq_buffer, 100);
	} else {
		DBG("Sampling could not proceed, an error occurred\n");
	}
}

void main(void)
{
	struct device *adc;
	struct nano_timer timer;
	uint32_t data[2] = {0, 0};

	cb_count = 0;

	nano_timer_init(&timer, data);

	adc = device_get_binding(CONFIG_ADC_TI_ADC108S102_0_DRV_NAME);
	if (!adc) {
		DBG("Cannot get adc controller\n");
		goto done;
	}

	adc_set_callback(adc, callback);

	adc_enable(adc);

	adc_disable(adc);

	if (adc_read(adc, &table) != DEV_OK) {
		DBG("Could not call adc_read\n");
	}

done:
	while (1) {
		DBG("Waiting...");

		nano_task_timer_start(&timer, SLEEPTICKS);
		nano_task_timer_wait(&timer);
	}
}
