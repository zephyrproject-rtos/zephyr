/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Test app to illustrate I2S transmission/reception on Intel S1000 CRB
 *
 * The i2s_cavs driver is being used.
 *
 * In this test app, I2S transmission and reception are tested as follows:
 * I2S port #3 of Intel S1000 is configured for birectional mode
 *     i.e., I2S_DIR_TX and I2S_DIR_RX
 * After each frame is received, it is sent/looped back on the same I2S
 * The transmit direction is started after 2 frames are queued. This is done to
 * ensure that there is enough data for the DMA and I2S are available when the
 * start operation is triggered
 */

#include <zephyr.h>
#include <sys/printk.h>

#include <device.h>
#include <drivers/i2s.h>

#define I2S_DEV_NAME "I2S_3"
#define NUM_I2S_BUFFERS 4

/* size of stack area used by each thread */
#define STACKSIZE               2048

/* scheduling priority used by each thread */
#define PRIORITY                7

/* delay between greetings (in ms) */
#define SLEEPTIME               500

extern struct k_sem thread_sem;

#define NUM_OF_CHANNELS		2
#define FRAME_CLK_FREQ		48000
#define I2S_WORDSIZE		32
#define BLOCK_SIZE		192
#define BLOCK_SIZE_BYTES	(BLOCK_SIZE * sizeof(s32_t))
#define FRAMES_PER_ITERATION	50
#define TIMEOUT			2000

static char __aligned(4) audio_buffers[BLOCK_SIZE_BYTES * NUM_I2S_BUFFERS];
static struct k_mem_slab i2s_mem_slab;

/** Configure I2S bidirectional transfer. */
void test_i2s_bidirectional_transfer_configure(void)
{
	int ret;
	struct device *dev_i2s;
	struct i2s_config i2s_cfg;

	k_mem_slab_init(&i2s_mem_slab, audio_buffers, BLOCK_SIZE_BYTES,
			NUM_I2S_BUFFERS);

	dev_i2s = device_get_binding(I2S_DEV_NAME);
	if (!dev_i2s) {
		printk("I2S: Device driver not found.\n");
		return;
	}

	/* Configure */
	i2s_cfg.word_size = I2S_WORDSIZE;
	i2s_cfg.channels = NUM_OF_CHANNELS;
	i2s_cfg.format = I2S_FMT_DATA_FORMAT_I2S | I2S_FMT_CLK_NF_NB;
	i2s_cfg.options = I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER;
	i2s_cfg.frame_clk_freq = FRAME_CLK_FREQ;
	i2s_cfg.block_size = BLOCK_SIZE_BYTES;
	i2s_cfg.mem_slab = &i2s_mem_slab;
	i2s_cfg.timeout = TIMEOUT;

	ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
	if (ret != 0) {
		printk("I2S_TX configuration failed with %d error\n", ret);
		return;
	}
	ret = i2s_configure(dev_i2s, I2S_DIR_RX, &i2s_cfg);
	if (ret != 0) {
		printk("I2S_RX configuration failed with %d error\n", ret);
		return;
	}
}

/** @brief Bi-Directional I2S transfer.
 *
 * - TX/RX stream START trigger starts transmission/reception.
 * - TX/RX stream STOP trigger stops the transmission/reception.
 */
void test_i2s_bidirectional_transfer(void)
{
	struct device *dev_i2s;
	int frames = 0;
	void *buffer;
	size_t size;
	int ret;

	printk("Testing I2S bidirectional transfer\n");
	dev_i2s = device_get_binding(I2S_DEV_NAME);
	if (!dev_i2s) {
		printk("I2S: Device driver not found.\n");
		return;
	}

	/* Start reception */
	ret = i2s_trigger(dev_i2s, I2S_DIR_RX, I2S_TRIGGER_START);
	if (ret != 0) {
		printk("RX Start failed with %d error\n", ret);
		return;
	}

	/* iteratively receive a frame and send it back */
	while (frames++ < FRAMES_PER_ITERATION) {
		ret = i2s_read(dev_i2s, &buffer, &size);
		if (ret != 0) {
			printk("i2s_read failed with %d error\n", ret);
			return;
		}

		/* send the buffer */
		ret = i2s_write(dev_i2s, buffer, size);
		if (ret != 0) {
			printk("i2s_write failed with %d error\n", ret);
			return;
		}

		/* Start transmission after 2 frames are queued */
		if (frames == 2) {
			ret = i2s_trigger(dev_i2s, I2S_DIR_TX,
					I2S_TRIGGER_START);
			if (ret != 0) {
				printk("TX Start failed with %d error\n", ret);
				return;
			}
		}
	}

	/* Stop transmission */
	ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_STOP);
	if (ret != 0) {
		printk("TX Stop failed with %d error\n", ret);
		return;
	}

	/* Stop reception */
	ret = i2s_trigger(dev_i2s, I2S_DIR_RX, I2S_TRIGGER_STOP);
	if (ret != 0) {
		printk("RX Stop failed with %d error\n", ret);
		return;
	}
	printk("Completed %d bidirectional frames on " I2S_DEV_NAME "\n",
			FRAMES_PER_ITERATION);
}

/* i2s_thread is a static thread that is spawned automatically */
void i2s_thread(void *dummy1, void *dummy2, void *dummy3)
{
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);

	test_i2s_bidirectional_transfer_configure();

	while (1) {
		k_sem_take(&thread_sem, K_FOREVER);

		test_i2s_bidirectional_transfer();

		/* let other threads have a turn */
		k_sem_give(&thread_sem);

		/* wait a while */
		k_sleep(SLEEPTIME);
	}
}

K_THREAD_DEFINE(i2s_thread_id, STACKSIZE, i2s_thread, NULL, NULL, NULL,
		PRIORITY, 0, K_NO_WAIT);
