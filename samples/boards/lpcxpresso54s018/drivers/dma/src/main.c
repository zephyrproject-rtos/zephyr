/*
 * Copyright (c) 2025 Zephyr Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/uart.h>
#include <string.h>

#define DMA_DEVICE_NODE DT_NODELABEL(dma0)
#define UART_DEVICE_NODE DT_NODELABEL(uart0)

#define TRANSFER_SIZE 256
#define NUM_BLOCKS 3

static const struct device *dma_dev;
static const struct device *uart_dev;

static uint8_t src_buffer[TRANSFER_SIZE] __aligned(4);
static uint8_t dst_buffer[TRANSFER_SIZE] __aligned(4);
static uint8_t uart_rx_buffer[64] __aligned(4);

static struct k_sem dma_sem;
static volatile bool transfer_done;

static void dma_callback(const struct device *dev, void *user_data,
			uint32_t channel, int status)
{
	if (status == 0) {
		transfer_done = true;
		k_sem_give(&dma_sem);
	} else {
		printk("DMA transfer error: %d\n", status);
	}
}

static int test_memory_to_memory(void)
{
	struct dma_config dma_cfg = {0};
	struct dma_block_config dma_block = {0};
	int ret;

	printk("\nTesting memory-to-memory transfer...\n");

	/* Initialize source buffer */
	for (int i = 0; i < TRANSFER_SIZE; i++) {
		src_buffer[i] = i & 0xFF;
	}
	memset(dst_buffer, 0, TRANSFER_SIZE);

	/* Print first 16 bytes of source */
	printk("Source buffer: ");
	for (int i = 0; i < 16; i++) {
		printk("%02x ", src_buffer[i]);
	}
	printk("\n");

	/* Configure DMA block */
	dma_block.source_address = (uint32_t)src_buffer;
	dma_block.dest_address = (uint32_t)dst_buffer;
	dma_block.block_size = TRANSFER_SIZE;
	dma_block.next_block = NULL;

	/* Configure DMA */
	dma_cfg.channel_direction = MEMORY_TO_MEMORY;
	dma_cfg.source_data_size = 4; /* 32-bit transfers */
	dma_cfg.dest_data_size = 4;
	dma_cfg.source_burst_length = 1;
	dma_cfg.dest_burst_length = 1;
	dma_cfg.channel_priority = 0;
	dma_cfg.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	dma_cfg.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	dma_cfg.head_block = &dma_block;
	dma_cfg.dma_callback = dma_callback;
	dma_cfg.user_data = NULL;
	dma_cfg.dma_slot = -1; /* Software trigger */

	/* Configure channel 0 */
	ret = dma_config(dma_dev, 0, &dma_cfg);
	if (ret < 0) {
		printk("DMA config failed: %d\n", ret);
		return ret;
	}

	/* Start transfer */
	transfer_done = false;
	printk("DMA transfer started\n");
	ret = dma_start(dma_dev, 0);
	if (ret < 0) {
		printk("DMA start failed: %d\n", ret);
		return ret;
	}

	/* Wait for completion */
	k_sem_take(&dma_sem, K_SECONDS(1));
	if (!transfer_done) {
		printk("DMA transfer timeout\n");
		dma_stop(dma_dev, 0);
		return -ETIMEDOUT;
	}

	printk("DMA transfer completed\n");

	/* Verify transfer */
	printk("Destination buffer: ");
	for (int i = 0; i < 16; i++) {
		printk("%02x ", dst_buffer[i]);
	}
	printk("\n");

	if (memcmp(src_buffer, dst_buffer, TRANSFER_SIZE) == 0) {
		printk("Memory-to-memory transfer: PASSED\n");
		return 0;
	} else {
		printk("Memory-to-memory transfer: FAILED\n");
		return -EIO;
	}
}

static int test_linked_descriptors(void)
{
	struct dma_config dma_cfg = {0};
	struct dma_block_config dma_blocks[NUM_BLOCKS] = {0};
	int ret;

	printk("\nTesting linked descriptors...\n");

	/* Initialize source buffer with pattern */
	for (int i = 0; i < TRANSFER_SIZE; i++) {
		src_buffer[i] = (i / 64) + 0xA0; /* Different pattern per block */
	}
	memset(dst_buffer, 0, TRANSFER_SIZE);

	/* Configure DMA blocks - split transfer into 3 blocks */
	for (int i = 0; i < NUM_BLOCKS; i++) {
		int offset = i * (TRANSFER_SIZE / NUM_BLOCKS);
		int size = TRANSFER_SIZE / NUM_BLOCKS;
		
		if (i == NUM_BLOCKS - 1) {
			/* Last block may be larger due to rounding */
			size = TRANSFER_SIZE - offset;
		}

		dma_blocks[i].source_address = (uint32_t)(src_buffer + offset);
		dma_blocks[i].dest_address = (uint32_t)(dst_buffer + offset);
		dma_blocks[i].block_size = size;
		dma_blocks[i].next_block = (i < NUM_BLOCKS - 1) ? &dma_blocks[i + 1] : NULL;
	}

	/* Configure DMA */
	dma_cfg.channel_direction = MEMORY_TO_MEMORY;
	dma_cfg.source_data_size = 4;
	dma_cfg.dest_data_size = 4;
	dma_cfg.source_burst_length = 4;
	dma_cfg.dest_burst_length = 4;
	dma_cfg.channel_priority = 1;
	dma_cfg.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	dma_cfg.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	dma_cfg.head_block = &dma_blocks[0];
	dma_cfg.dma_callback = dma_callback;
	dma_cfg.user_data = NULL;
	dma_cfg.dma_slot = -1;

	/* Configure channel 1 */
	ret = dma_config(dma_dev, 1, &dma_cfg);
	if (ret < 0) {
		printk("DMA config failed: %d\n", ret);
		return ret;
	}

	/* Start transfer */
	transfer_done = false;
	ret = dma_start(dma_dev, 1);
	if (ret < 0) {
		printk("DMA start failed: %d\n", ret);
		return ret;
	}

	/* Wait for completion */
	k_sem_take(&dma_sem, K_SECONDS(1));
	if (!transfer_done) {
		printk("DMA transfer timeout\n");
		dma_stop(dma_dev, 1);
		return -ETIMEDOUT;
	}

	/* Verify transfer */
	if (memcmp(src_buffer, dst_buffer, TRANSFER_SIZE) == 0) {
		printk("Block 1 transferred\n");
		printk("Block 2 transferred\n");  
		printk("Block 3 transferred\n");
		printk("Linked descriptor transfer: PASSED\n");
		return 0;
	} else {
		printk("Linked descriptor transfer: FAILED\n");
		return -EIO;
	}
}

static int test_uart_dma(void)
{
	printk("\nTesting UART DMA transfers...\n");
	printk("Note: UART DMA test requires FLEXCOMM DMA support\n");
	printk("This is a placeholder for future implementation\n");
	printk("UART DMA transfer: SKIPPED\n");
	return 0;
}

int main(void)
{
	int ret;

	printk("\n*** LPC54S018 DMA Sample ***\n");

	/* Get DMA device */
	dma_dev = DEVICE_DT_GET(DMA_DEVICE_NODE);
	if (!device_is_ready(dma_dev)) {
		printk("DMA device not ready\n");
		return -ENODEV;
	}

	/* Get UART device */
	uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);
	if (!device_is_ready(uart_dev)) {
		printk("UART device not ready\n"); 
		return -ENODEV;
	}

	/* Initialize semaphore */
	k_sem_init(&dma_sem, 0, 1);

	/* Run tests */
	ret = test_memory_to_memory();
	if (ret < 0) {
		printk("Memory-to-memory test failed\n");
		return ret;
	}

	ret = test_linked_descriptors();
	if (ret < 0) {
		printk("Linked descriptor test failed\n");
		return ret;
	}

	ret = test_uart_dma();
	if (ret < 0) {
		printk("UART DMA test failed\n");
		return ret;
	}

	printk("\nAll DMA tests completed successfully!\n");
	return 0;
}