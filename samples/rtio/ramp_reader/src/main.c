/*
 * Copyright (c) 2019 Tom Burdick <tom.burdick@electromatic.us>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/rtio.h>
#include <drivers/rtio/ramp.h>
#include <stdio.h>
#include <sys/printk.h>
#include <sys/__assert.h>

#define MAX_BLOCK_SIZE 256

RTIO_MEMPOOL_ALLOCATOR_DEFINE(blockalloc, 64, MAX_BLOCK_SIZE, 32, 4);
K_FIFO_DEFINE(ramp_out_fifo);

void trigger_read(void *s1, void *s2, void *s3)
{
	struct device *ramp_dev = (struct device *)s1;
	int res = 0;

	while (true) {
		res = rtio_trigger_read(ramp_dev, K_NO_WAIT);
		__ASSERT_NO_MSG(res == 0);
		k_sleep(K_MSEC(5));
	}
}

#define TRIGGER_READ_STACK_SIZE 1024
K_THREAD_STACK_DEFINE(trigger_read_stack, TRIGGER_READ_STACK_SIZE);
struct k_thread trigger_read_thread;

int main(void)
{
	printk("RTIO Ramp Sample\n");

	struct device *ramp_dev = device_get_binding("RTIO_RAMP");

	/* effectively the same as a stereo CD quality audio stream */
	struct rtio_ramp_config ramp_dev_cfg = {
		.sample_rate = 100,
		.max_value = 1 << 16,
	};

	struct rtio_config config =  {
		.output_config = {
			.allocator = blockalloc,
			.fifo = &ramp_out_fifo,
			.timeout = K_FOREVER,
			.byte_size = MAX_BLOCK_SIZE
		},
		.driver_config = &ramp_dev_cfg
	};

	int res = rtio_configure(ramp_dev, &config);

	__ASSERT_NO_MSG(res == 0);

	k_thread_create(&trigger_read_thread,
			trigger_read_stack,
			K_THREAD_STACK_SIZEOF(trigger_read_stack),
			trigger_read,
			ramp_dev, NULL, NULL,
			5, 0, K_NO_WAIT);

	while (true) {
		struct rtio_block *block = k_fifo_get(&ramp_out_fifo, K_FOREVER);

		while (block->buf.len > sizeof(u32_t)) {
			printk("%d ", rtio_block_pull_le32(block));
		}
		printk("\n");
		rtio_block_free(blockalloc, block);
	}
}
