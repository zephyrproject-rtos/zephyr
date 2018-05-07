/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Sample app to illustrate I2S transmission (playback) on Intel_S1000.
 *
 * Intel_S1000 - Xtensa
 * --------------------
 *
 * The i2s_cavs driver is being used.
 *
 * In this sample app, I2S communication is tested for the below 2 modes:
 * ping-pong mode - the buffer is statically allocated.
 * short block mode - the buffer is dynamically allocated. This example shows
 * only 2 blocks but more can be included.
 */

#include <zephyr.h>
#include <misc/printk.h>

#include <device.h>
#include <i2s.h>

#define I2S_DEV_NAME "I2S_1"
#define NUM_TX_BLOCKS 4

/* size of stack area used by each thread */
#define STACKSIZE               2048

/* scheduling priority used by each thread */
#define PRIORITY                7

/* delay between greetings (in ms) */
#define SLEEPTIME               500

extern struct k_sem thread_sem;

#define NUM_OF_CHANNELS		2
#define FRAME_CLK_FREQ		48000
#define BLOCK_SIZE		192
#define BLOCK_SIZE_BYTES	(BLOCK_SIZE * sizeof(s16_t))

#define SAMPLE_NO		(BLOCK_SIZE/NUM_OF_CHANNELS)
#define TIMEOUT			2000

K_MEM_SLAB_DEFINE(tx_0_mem_slab, BLOCK_SIZE_BYTES, NUM_TX_BLOCKS, 1);

static void fill_buf_const(s16_t *tx_block, s16_t val_l, s16_t val_r)
{
	for (int i = 0; i < SAMPLE_NO; i++) {
		tx_block[2 * i] = val_l;
		tx_block[2 * i + 1] = val_r;
	}
}

static int tx_block_write(struct device *dev_i2s, s16_t att_l,
			  s16_t att_r, int err)
{
	void *tx_block;
	int ret;

	ret = k_mem_slab_alloc(&tx_0_mem_slab, &tx_block, K_FOREVER);
	if (ret < 0) {
		printk("Error: failed to allocate tx_block\n");
		return -1;
	}
	fill_buf_const((u16_t *)tx_block, att_l, att_r);
	ret = i2s_write(dev_i2s, tx_block, BLOCK_SIZE_BYTES);
	if (ret < 0) {
		k_mem_slab_free(&tx_0_mem_slab, &tx_block);
	}
	if (ret != err) {
		printk("Error: i2s_write failed expected %d, actual %d\n",
			 err, ret);
		return -1;
	}

	return 0;
}

static s16_t ping_block[BLOCK_SIZE];
static s16_t pong_block[BLOCK_SIZE];

static int tx_pingpong_write(struct device *dev_i2s, s16_t *tx_block,
			     s16_t att_l, s16_t att_r, int err)
{
	int ret;

	fill_buf_const((s16_t *)tx_block, att_l, att_r);
	ret = i2s_write(dev_i2s, tx_block, BLOCK_SIZE_BYTES);
	if (ret != err) {
		printk("Error: i2s_write failed expected %d, actual %d\n",
			 err, ret);
		return -1;
	}

	return 0;
}

/** Configure I2S TX transfer. */
void test_i2s_tx_transfer_configure_0(u8_t mode)
{
	struct device *dev_i2s;
	struct i2s_config i2s_cfg;
	int ret;

	dev_i2s = device_get_binding(I2S_DEV_NAME);
	if (!dev_i2s) {
		printk("I2S: Device driver not found.\n");
		return;
	}

	/* Configure */
	i2s_cfg.word_size = 16;
	i2s_cfg.channels = NUM_OF_CHANNELS;
	i2s_cfg.format = I2S_FMT_DATA_FORMAT_I2S | I2S_FMT_CLK_NF_NB;
	i2s_cfg.options = I2S_OPT_FRAME_CLK_SLAVE | I2S_OPT_BIT_CLK_SLAVE;
	i2s_cfg.options |= mode;
	i2s_cfg.frame_clk_freq = FRAME_CLK_FREQ;
	i2s_cfg.block_size = BLOCK_SIZE_BYTES;
	i2s_cfg.mem_slab = &tx_0_mem_slab;
	i2s_cfg.timeout = TIMEOUT;

	ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
	if (ret != 0) {
		printk("i2s configuration failed with %d error\n", ret);
		return;
	}
}

/** @brief Short I2S transfer.
 *
 * - TX stream START trigger starts transmission.
 * - sending a short sequence of data returns success.
 * - TX stream DRAIN trigger empties the transmit queue.
 */
void test_i2s_transfer_short(void)
{
	struct device *dev_i2s;
	int ret;

	dev_i2s = device_get_binding(I2S_DEV_NAME);
	if (!dev_i2s) {
		printk("I2S: Device driver not found.\n");
		return;
	}

	/* Prefill TX queue */
	ret = tx_block_write(dev_i2s, 0xA0A0, 0xF0F0, 0);
	if (ret != 0) {
		printk("Prefill write failed with %d error\n", ret);
		return;
	}

	/* Start transmission */
	ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_START);
	if (ret != 0) {
		printk("Start Trigger failed with %d error\n", ret);
		return;
	}

	ret = tx_block_write(dev_i2s, 0x0A0A, 0x0F0F, 0);
	if (ret != 0) {
		printk("Post-start write failed with %d error\n", ret);
		return;
	}

	/* All data written, drain TX queue and stop the transmission */
	ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_DRAIN);
	if (ret != 0) {
		printk("Drain Trigger failed");
		return;
	}
}

/** @brief Ping-Pong I2S transfer.
 *
 * - TX stream START trigger starts transmission.
 * - sending / receiving a ping-pong sequence of data returns success.
 * - TX stream STOP trigger stops the transmission.
 */
void test_i2s_transfer_pingpong(void)
{
	struct device *dev_i2s;
	int ret;

	dev_i2s = device_get_binding(I2S_DEV_NAME);
	if (!dev_i2s) {
		printk("I2S: Device driver not found.\n");
		return;
	}

	/* Prefill the ping buffer */
	ret = tx_pingpong_write(dev_i2s, ping_block, 0xA0A0, 0xF0F0, 0);
	if (ret != 0) {
		printk("Ping-block write failed with %d error\n", ret);
		return;
	}

	/* Prefill the pong buffer */
	ret = tx_pingpong_write(dev_i2s, pong_block, 0x0A0A, 0x0F0F, 0);
	if (ret != 0) {
		printk("Ping-block write failed with %d error\n", ret);
		return;
	}

	/* Start transmission */
	ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_START);
	if (ret != 0) {
		printk("Start Trigger failed with %d error\n", ret);
		return;
	}

	/* The data in the ping-pong buffers will constantly be getting
	 * transmitted one after the other in this duration
	 */
	k_sleep(200);

	/* Stop transmission */
	ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_STOP);
	if (ret != 0) {
		printk("Stop Trigger failed with %d error\n", ret);
		return;
	}
}

/* i2s_thread is a static thread that is spawned automatically */
void i2s_thread(void *dummy1, void *dummy2, void *dummy3)
{
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);

	while (1) {
		printk("Testing I2S normal mode\n");
		k_sem_take(&thread_sem, K_FOREVER);

		test_i2s_tx_transfer_configure_0(0);
		test_i2s_transfer_short();

		/* let other threads have a turn */
		k_sem_give(&thread_sem);

		/* wait a while */
		k_sleep(SLEEPTIME);

		printk("Testing I2S ping-pong mode\n");
		k_sem_take(&thread_sem, K_FOREVER);

		test_i2s_tx_transfer_configure_0(I2S_OPT_PINGPONG);
		test_i2s_transfer_pingpong();

		/* let other threads have a turn */
		k_sem_give(&thread_sem);

		/* wait a while */
		k_sleep(SLEEPTIME);
	}
}

K_THREAD_DEFINE(i2s_thread_id, STACKSIZE, i2s_thread, NULL, NULL, NULL,
		PRIORITY, 0, K_NO_WAIT);
