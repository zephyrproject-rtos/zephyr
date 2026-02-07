/* pcm.c - PCM implementation for Bluetooth Hands-Free Profile */

/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/toolchain.h>

#include <zephyr/bluetooth/bluetooth.h>

#include "pcm.h"

#if DT_HAS_ALIAS(pcm_rxtx) || (DT_HAS_ALIAS(pcm_rx) && DT_HAS_ALIAS(pcm_tx))
#if DT_NODE_EXISTS(DT_ALIAS(pcm_rxtx))
#define PCM_RX DT_ALIAS(pcm_rxtx)
#define PCM_TX DT_ALIAS(pcm_rxtx)
#else
#define PCM_RX DT_ALIAS(pcm_rx)
#define PCM_TX DT_ALIAS(pcm_tx)
#endif

#define MAX_SAMPLE_FREQ      16000
#define MAX_SAMPLE_BIT_WIDTH 16
#define MAX_CHANNELS         1

#define SAMPLES_PER_BLOCK ((MAX_SAMPLE_FREQ * CONFIG_AUDIO_TRANSFER_INTERVAL / 1000) * MAX_CHANNELS)

#define BLOCK_SIZE     (MAX_SAMPLE_BIT_WIDTH * SAMPLES_PER_BLOCK / 8)
#define RX_BLOCK_COUNT (CONFIG_PCM_BUFFERS)
#define TX_BLOCK_COUNT (CONFIG_PCM_BUFFERS)
#define TIMEOUT        (2 * 1000 / CONFIG_AUDIO_TRANSFER_INTERVAL)

static const struct device *pcm_rx_dev = DEVICE_DT_GET(PCM_RX);
static const struct device *pcm_tx_dev = DEVICE_DT_GET(PCM_TX);

K_MEM_SLAB_DEFINE_IN_SECT_STATIC(rx_mem_slab, __nocache, BLOCK_SIZE, RX_BLOCK_COUNT, 4);
K_MEM_SLAB_DEFINE_IN_SECT_STATIC(tx_mem_slab, __nocache, BLOCK_SIZE, TX_BLOCK_COUNT, 4);

static int configure_pcm_streams(const struct device *pcm_rx_dev, const struct device *pcm_tx_dev,
				 struct i2s_config *config)
{
	int err;

	config->mem_slab = &rx_mem_slab;
	err = i2s_configure(pcm_rx_dev, I2S_DIR_RX, config);
	if (err < 0) {
		printk("Failed to configure PCM RX stream: %d\n", err);
		return err;
	}

	config->mem_slab = &tx_mem_slab;
	err = i2s_configure(pcm_tx_dev, I2S_DIR_TX, config);
	if (err < 0) {
		printk("Failed to configure PCM TX stream: %d\n", err);
		return err;
	}

	return 0;
}

static struct k_sem pcm_rx_thread_notify;
static pcm_rx_cb_t pcm_rx_cb;

#define PCM_RX_FLAG_STOPPED  0
#define PCM_RX_FLAG_STARTED  1

static atomic_t pcm_rx_flag[1];

static void pcm_rx_task(void *p1, void *p2, void *p3)
{
	int err;
	void *mem_block;
	uint32_t block_size;

	while (true) {
		err = k_sem_take(&pcm_rx_thread_notify, K_FOREVER);
		if (err < 0) {
			continue;
		}

		if (!atomic_test_bit(pcm_rx_flag, PCM_RX_FLAG_STARTED)) {
			continue;
		} else {
			k_sem_give(&pcm_rx_thread_notify);
		}

		err = i2s_read(pcm_rx_dev, &mem_block, &block_size);
		if (err < 0) {
			continue;
		}

		if (pcm_rx_cb != NULL) {
			pcm_rx_cb((const uint8_t *)mem_block, block_size);
		}
		k_mem_slab_free(&rx_mem_slab, (void *)mem_block);
	}
}

static K_KERNEL_STACK_MEMBER(pcm_rx_thread_stack, CONFIG_PCM_RX_THREAD_STACK_SIZE);

int pcm_init(uint8_t air_mode)
{
	struct i2s_config config;
	int err;
	uint8_t word_size;
	uint8_t channels;
	uint32_t sample_rate;

	static bool is_initiated;
	static struct k_thread pcm_rx_thread;
	static k_tid_t pcm_rx_thread_id;

	if (is_initiated) {
		return 0;
	}

	if (!device_is_ready(pcm_rx_dev)) {
		printk("%s is not ready\n", pcm_rx_dev->name);
		return -EINVAL;
	}

	if (pcm_rx_dev != pcm_tx_dev && !device_is_ready(pcm_tx_dev)) {
		printk("%s is not ready\n", pcm_tx_dev->name);
		return -EINVAL;
	}

	switch (air_mode) {
	case BT_HCI_CODING_FORMAT_CVSD:
		word_size = 16;
		channels = 1;
		sample_rate = 8000;
		break;
	case BT_HCI_CODING_FORMAT_TRANSPARENT:
		word_size = 16;
		channels = 1;
		sample_rate = 16000;
		break;
	case BT_HCI_CODING_FORMAT_ULAW_LOG:
	case BT_HCI_CODING_FORMAT_ALAW_LOG:
	case BT_HCI_CODING_FORMAT_LINEAR_PCM:
	case BT_HCI_CODING_FORMAT_MSBC:
	case BT_HCI_CODING_FORMAT_LC3:
	case BT_HCI_CODING_FORMAT_G729A:
	case BT_HCI_CODING_FORMAT_VS:
	default:
		printk("Unsupported air mode: %d\n", air_mode);
		return -ENOTSUP;
	}

	config.word_size = word_size;
	config.channels = channels;
	config.format = I2S_FMT_DATA_FORMAT_PCM_SHORT;
	config.options = I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET;
	config.frame_clk_freq = sample_rate;
	config.block_size = BLOCK_SIZE;
	config.timeout = TIMEOUT;

	err = configure_pcm_streams(pcm_rx_dev, pcm_tx_dev, &config);
	if (err < 0) {
		return err;
	}

	if (pcm_rx_thread_id == NULL) {
		k_sem_init(&pcm_rx_thread_notify, 0, 1);
		pcm_rx_thread_id = k_thread_create(&pcm_rx_thread, pcm_rx_thread_stack,
						   K_KERNEL_STACK_SIZEOF(pcm_rx_thread_stack),
						   pcm_rx_task, NULL, NULL, NULL,
						   K_PRIO_COOP(CONFIG_PCM_RX_THREAD_PRIO), 0,
						   K_NO_WAIT);
		k_thread_name_set(pcm_rx_thread_id, "HFP PCM RX");
	}

	atomic_set_bit(pcm_rx_flag, PCM_RX_FLAG_STOPPED);
	is_initiated = true;

	return 0;
}

int pcm_rx_start(pcm_rx_cb_t cb)
{
	int err;

	if (!atomic_test_bit(pcm_rx_flag, PCM_RX_FLAG_STOPPED)) {
		return 0;
	}

	pcm_rx_cb = cb;

	err = i2s_trigger(pcm_rx_dev, I2S_DIR_RX, I2S_TRIGGER_START);
	if (err < 0) {
		printk("Failed to trigger start on RX: %d\n", err);
		return err;
	}

	atomic_clear_bit(pcm_rx_flag, PCM_RX_FLAG_STOPPED);
	atomic_set_bit(pcm_rx_flag, PCM_RX_FLAG_STARTED);
	k_sem_give(&pcm_rx_thread_notify);

	return 0;
}

int pcm_rx_stop(void)
{
	/* TODO: Stop I2S RX. */
	return 0;
}

int pcm_tx(const uint8_t *data, uint32_t len)
{
	int err;
	void *mem_block;

	if (len != (BLOCK_SIZE * 2)) {
		printk("Invalid data len %u != %u\n", len, BLOCK_SIZE * 2);
		return -EINVAL;
	}

	err = k_mem_slab_alloc(&tx_mem_slab, &mem_block, K_NO_WAIT);
	if (err < 0) {
		printk("Failed to allocate TX block: %d\n", err);
		return err;
	}

	for (uint32_t i = 0; i < BLOCK_SIZE; i += 2) {
		uint16_t *dst;
		const uint16_t *src;

		dst = (uint16_t *)&((uint8_t *)mem_block)[i];
		src = (const uint16_t *)&data[i * 2 + 2];

		*dst = *src;
	}

	err = i2s_write(pcm_tx_dev, mem_block, BLOCK_SIZE);
	if (err < 0) {
		k_mem_slab_free(&tx_mem_slab, mem_block);
		printk("Failed to write block: %d\n", err);
		return err;
	}

	/* Try to trigger TX start each time, because TX may have stopped if there is no more
	 * data in pending.
	 * Ignore the error.
	 */
	(void)i2s_trigger(pcm_tx_dev, I2S_DIR_TX, I2S_TRIGGER_START);

	return 0;
}
#else
int pcm_init(uint8_t air_mode)
{
	printk("PCM is unsupported\n");
	return 0;
}

int pcm_tx(const uint8_t *data, uint32_t len)
{
	return 0;
}

int pcm_rx_start(pcm_rx_cb_t cb)
{
	return 0;
}

int pcm_rx_stop(void)
{
	return 0;
}
#endif /* DT_HAS_ALIAS(pcm_rxtx) || (DT_HAS_ALIAS(pcm_rx) && DT_HAS_ALIAS(pcm_tx)) */
