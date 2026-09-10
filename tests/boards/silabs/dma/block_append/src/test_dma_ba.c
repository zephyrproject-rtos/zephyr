/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Verify zephyr dma memory to memory transfer with block append during a transfer
 * @details
 * - 4 tests:
 *   -# restart_transfer test: Check that the function silabs ldma append block function restart
 *      the transfer if we append a block after the transfer is done.
 *   -# restart_in_isr test: Check that is a transfer is done during the append, the next dma isr
 *      will restart the transfer with the right append block.
 *   -# stress_in_isr test: Check that we can append the next block immediately after a
 *      DMA_STATUS_BLOCK callback.
 *   -# loopstress test: Check that we can continuously append block and check that the function
 *      return an error if we append a transfer that already has an append block
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_silabs_ldma.h>
#include <zephyr/ztest.h>

#define BLOCK_SIZE 4

/* this src memory shall be in RAM to support using as a DMA source pointer.*/
static __aligned(32) uint8_t tx_data[CONFIG_DMA_BA_XFER_SIZE];
static __aligned(32) uint8_t rx_data[CONFIG_DMA_BA_XFER_SIZE];

K_SEM_DEFINE(xfer_sem, 0, 1);

static struct dma_config dma_cfg = {0};
static struct dma_block_config dma_block_cfg;
static uint32_t rx_idx;
static uint32_t tx_idx;

static void dma_ba_callback_restart(const struct device *dma_dev, void *user_data, uint32_t channel,
				    int status)
{
	if (status < 0) {
		TC_PRINT("callback status %d\n", status);
	} else {
		TC_PRINT("giving xfer_sem\n");
		k_sem_give(&xfer_sem);
	}
}

static int test_ba_restart_transfer(void)
{
	const struct device *dma;
	static int chan_id;

	TC_PRINT("Preparing DMA Controller\n");

	memset(tx_data, 0, sizeof(tx_data));

	for (int i = 0; i < CONFIG_DMA_BA_XFER_SIZE; i++) {
		tx_data[i] = i;
	}

	memset(rx_data, 0, sizeof(rx_data));

	dma = DEVICE_DT_GET(DT_ALIAS(dma0));
	if (!device_is_ready(dma)) {
		TC_PRINT("dma controller device is not ready\n");
		return TC_FAIL;
	}

	dma_cfg.channel_direction = MEMORY_TO_MEMORY;
	dma_cfg.source_data_size = 1U;
	dma_cfg.dest_data_size = 1U;
	dma_cfg.source_burst_length = 1U;
	dma_cfg.dest_burst_length = 1U;
	dma_cfg.user_data = NULL;
	dma_cfg.dma_callback = dma_ba_callback_restart;
	dma_cfg.head_block = &dma_block_cfg;
	dma_cfg.complete_callback_en = true; /* per block completion */

	chan_id = dma_request_channel(dma, NULL);
	if (chan_id < 0) {
		return TC_FAIL;
	}

	memset(&dma_block_cfg, 0, sizeof(dma_block_cfg));
	dma_block_cfg.block_size = CONFIG_DMA_BA_XFER_SIZE / 2;
	dma_block_cfg.source_address = (uint32_t)(tx_data);
	dma_block_cfg.dest_address = (uint32_t)(rx_data);
	TC_PRINT("block_size %d, source addr %x, dest addr %x\n", CONFIG_DMA_BA_XFER_SIZE,
		 dma_block_cfg.source_address, dma_block_cfg.dest_address);

	if (dma_config(dma, chan_id, &dma_cfg)) {
		TC_PRINT("ERROR: transfer config (%d)\n", chan_id);
		return TC_FAIL;
	}

	TC_PRINT("Starting the transfer on channel %d\n", chan_id);

	if (dma_start(dma, chan_id)) {
		TC_PRINT("ERROR: transfer start (%d)\n", chan_id);
		return TC_FAIL;
	}

	/* Be sure that the DMA transfer is done */
	k_busy_wait(1000 * 100); /* busy wait for 100 ms*/

	/* Append a next block on the channel that is already done*/
	dma_block_cfg.source_address = (uint32_t)(tx_data) + CONFIG_DMA_BA_XFER_SIZE / 2;
	dma_block_cfg.dest_address = (uint32_t)(rx_data) + CONFIG_DMA_BA_XFER_SIZE / 2;

	dma_cfg.head_block = &dma_block_cfg;
	silabs_ldma_append_block(dma, chan_id, &dma_cfg);

	if (k_sem_take(&xfer_sem, K_MSEC(1000)) != 0) {
		TC_PRINT("Timed out waiting for xfers\n");
		return TC_FAIL;
	}

	TC_PRINT("Verify RX buffer should contain the full TX buffer string.\n");

	if (memcmp(tx_data, rx_data, CONFIG_DMA_BA_XFER_SIZE)) {
		return TC_FAIL;
	}

	TC_PRINT("Finished: DMA block append restart transfer\n");
	return TC_PASS;
}

static int test_ba_restart_in_isr(void)
{
	const struct device *dma;
	static int chan_id;
	unsigned int key;

	TC_PRINT("Preparing DMA Controller\n");

	memset(tx_data, 0, sizeof(tx_data));

	for (int i = 0; i < CONFIG_DMA_BA_XFER_SIZE; i++) {
		tx_data[i] = i;
	}

	memset(rx_data, 0, sizeof(rx_data));

	dma = DEVICE_DT_GET(DT_ALIAS(dma0));
	if (!device_is_ready(dma)) {
		TC_PRINT("dma controller device is not ready\n");
		return TC_FAIL;
	}

	dma_cfg.channel_direction = MEMORY_TO_MEMORY;
	dma_cfg.source_data_size = 1U;
	dma_cfg.dest_data_size = 1U;
	dma_cfg.source_burst_length = 1U;
	dma_cfg.dest_burst_length = 1U;
	dma_cfg.user_data = NULL;
	dma_cfg.dma_callback = dma_ba_callback_restart;
	dma_cfg.head_block = &dma_block_cfg;
	dma_cfg.complete_callback_en = true; /* per block completion */

	chan_id = dma_request_channel(dma, NULL);
	if (chan_id < 0) {
		return TC_FAIL;
	}

	memset(&dma_block_cfg, 0, sizeof(dma_block_cfg));
	dma_block_cfg.block_size = CONFIG_DMA_BA_XFER_SIZE / 2;
	dma_block_cfg.source_address = (uint32_t)(tx_data);
	dma_block_cfg.dest_address = (uint32_t)(rx_data);
	TC_PRINT("block_size %d, source addr %x, dest addr %x\n", CONFIG_DMA_BA_XFER_SIZE,
		 dma_block_cfg.source_address, dma_block_cfg.dest_address);

	if (dma_config(dma, chan_id, &dma_cfg)) {
		TC_PRINT("ERROR: transfer config (%d)\n", chan_id);
		return TC_FAIL;
	}

	TC_PRINT("Starting the transfer on channel %d and waiting completion\n", chan_id);

	/* Lock IRQ in order to not triger the DMA isr */
	key = irq_lock();
	if (dma_start(dma, chan_id)) {
		TC_PRINT("ERROR: transfer start (%d)\n", chan_id);
		return TC_FAIL;
	}

	/* Be sure that the DMA transfer is done */
	k_busy_wait(1000 * 100); /* busy wait for 100 ms*/

	/* Remove the done flag of the dma channel to simulate an append while a transfer is
	 * ongoing
	 */
	sys_clear_bit((mem_addr_t)&LDMA->CHDONE, chan_id);

	/* Append a next block on the channel that is already done*/
	dma_block_cfg.source_address = (uint32_t)(tx_data) + CONFIG_DMA_BA_XFER_SIZE / 2;
	dma_block_cfg.dest_address = (uint32_t)(rx_data) + CONFIG_DMA_BA_XFER_SIZE / 2;

	dma_cfg.head_block = &dma_block_cfg;
	silabs_ldma_append_block(dma, chan_id, &dma_cfg);

	/* Set the chdone bit to simulate that the DMA transfer was finished while appending a
	 * block
	 */
	sys_set_bit((mem_addr_t)&LDMA->CHDONE, chan_id);

	/* Check if the isr in dma driver will correctly restart the DMA engine with the new block
	 * that was append
	 */
	irq_unlock(key);

	if (k_sem_take(&xfer_sem, K_MSEC(1000)) != 0) {
		TC_PRINT("Timed out waiting for xfers\n");
		return TC_FAIL;
	}

	TC_PRINT("Verify RX buffer should contain the full TX buffer string.\n");

	if (memcmp(tx_data, rx_data, CONFIG_DMA_BA_XFER_SIZE)) {
		return TC_FAIL;
	}

	TC_PRINT("Finished: DMA block append restart in isr\n");
	return TC_PASS;
}

static void dma_ba_callback_stress_in_isr(const struct device *dma_dev, void *user_data,
					  uint32_t channel, int status)
{
	struct dma_config *dma_cfg = (struct dma_config *)user_data;

	if (status < 0) {
		TC_PRINT("callback status %d\n", status);
	} else {
		if (rx_idx <= CONFIG_DMA_BA_XFER_SIZE - BLOCK_SIZE) {
			dma_block_cfg.source_address = (uint32_t)(tx_data + tx_idx);
			dma_block_cfg.dest_address = (uint32_t)(rx_data + rx_idx);
			rx_idx += BLOCK_SIZE;
			tx_idx += BLOCK_SIZE;
			if (silabs_ldma_append_block(dma_dev, channel, dma_cfg)) {
				TC_PRINT("append block failed\n");
			}
		} else {
			TC_PRINT("giving xfer_sem\n");
			k_sem_give(&xfer_sem);
		}
	}
}

static int test_ba_stress_in_isr(void)
{
	const struct device *dma;
	static int chan_id;
	unsigned int key;

	TC_PRINT("Preparing DMA Controller\n");

	memset(tx_data, 0, sizeof(tx_data));

	rx_idx = 0;
	tx_idx = 0;

	for (int i = 0; i < CONFIG_DMA_BA_XFER_SIZE; i++) {
		tx_data[i] = i;
	}

	memset(rx_data, 0, sizeof(rx_data));

	dma = DEVICE_DT_GET(DT_ALIAS(dma0));
	if (!device_is_ready(dma)) {
		return TC_FAIL;
	}

	dma_cfg.channel_direction = MEMORY_TO_MEMORY;
	dma_cfg.source_data_size = 1U;
	dma_cfg.dest_data_size = 1U;
	dma_cfg.source_burst_length = 1U;
	dma_cfg.dest_burst_length = 1U;
	dma_cfg.user_data = &dma_cfg;
	dma_cfg.dma_callback = dma_ba_callback_stress_in_isr;
	dma_cfg.head_block = &dma_block_cfg;
	dma_cfg.complete_callback_en = true; /* per block completion */

	chan_id = dma_request_channel(dma, NULL);
	if (chan_id < 0) {
		return TC_FAIL;
	}

	memset(&dma_block_cfg, 0, sizeof(dma_block_cfg));

	/* Configure the first transfer block*/
	dma_block_cfg.block_size = BLOCK_SIZE;
	dma_block_cfg.source_address = (uint32_t)(tx_data + tx_idx);
	dma_block_cfg.dest_address = (uint32_t)(rx_data + rx_idx);
	rx_idx += BLOCK_SIZE;
	tx_idx += BLOCK_SIZE;

	TC_PRINT("dma block %d block_size %d, source addr %x, dest addr %x\n", 0, BLOCK_SIZE,
		 dma_block_cfg.source_address, dma_block_cfg.dest_address);

	if (dma_config(dma, chan_id, &dma_cfg)) {
		TC_PRINT("ERROR: transfer config (%d)\n", chan_id);
		return TC_FAIL;
	}

	TC_PRINT("Starting the transfer on channel %d and waiting completion\n", chan_id);

	/* Lock IRQ in order to not triger the DMA isr */
	key = irq_lock();
	if (dma_start(dma, chan_id)) {
		TC_PRINT("ERROR: transfer start (%d)\n", chan_id);
		return TC_FAIL;
	}

	/* Append a new block on the channel */
	dma_block_cfg.source_address = (uint32_t)(tx_data + tx_idx);
	dma_block_cfg.dest_address = (uint32_t)(rx_data + rx_idx);
	rx_idx += BLOCK_SIZE;
	tx_idx += BLOCK_SIZE;
	silabs_ldma_append_block(dma, chan_id, &dma_cfg);

	irq_unlock(key);

	/* All the next blocks will be append in the isr */

	if (k_sem_take(&xfer_sem, K_MSEC(1000)) != 0) {
		TC_PRINT("Timed out waiting for xfers\n");
		return TC_FAIL;
	}

	TC_PRINT("Verify RX buffer should contain the full TX buffer string.\n");

	if (memcmp(tx_data, rx_data, CONFIG_DMA_BA_XFER_SIZE)) {
		return TC_FAIL;
	}

	TC_PRINT("Finished: DMA block append stress in isr\n");
	return TC_PASS;
}

static void dma_ba_callback_loopstress(const struct device *dma_dev, void *user_data,
				       uint32_t channel, int status)
{
	if (status < 0) {
		TC_PRINT("callback status %d\n", status);
	} else {
		if (rx_idx == CONFIG_DMA_BA_XFER_SIZE) {
			TC_PRINT("giving xfer_sem\n");
			k_sem_give(&xfer_sem);
		}
	}
}

static int test_ba_loopstress(void)
{
	const struct device *dma;
	static int chan_id;

	TC_PRINT("Preparing DMA Controller\n");

	memset(tx_data, 0, sizeof(tx_data));

	rx_idx = 0;
	tx_idx = 0;

	for (int i = 0; i < CONFIG_DMA_BA_XFER_SIZE; i++) {
		tx_data[i] = i;
	}

	memset(rx_data, 0, sizeof(rx_data));

	dma = DEVICE_DT_GET(DT_ALIAS(dma0));
	if (!device_is_ready(dma)) {
		TC_PRINT("dma controller device is not ready\n");
		return TC_FAIL;
	}

	dma_cfg.channel_direction = MEMORY_TO_MEMORY;
	dma_cfg.source_data_size = 1U;
	dma_cfg.dest_data_size = 1U;
	dma_cfg.source_burst_length = 1U;
	dma_cfg.dest_burst_length = 1U;
	dma_cfg.user_data = &dma_cfg;
	dma_cfg.dma_callback = dma_ba_callback_loopstress;
	dma_cfg.head_block = &dma_block_cfg;
	dma_cfg.complete_callback_en = true; /* per block completion */

	chan_id = dma_request_channel(dma, NULL);
	if (chan_id < 0) {
		return TC_FAIL;
	}

	memset(&dma_block_cfg, 0, sizeof(dma_block_cfg));

	/* Setting the first DMA transfer block */
	dma_block_cfg.block_size = BLOCK_SIZE;
	dma_block_cfg.source_address = (uint32_t)(tx_data + tx_idx);
	dma_block_cfg.dest_address = (uint32_t)(rx_data + rx_idx);
	rx_idx += BLOCK_SIZE;
	tx_idx += BLOCK_SIZE;

	TC_PRINT("dma block %d block_size %d, source addr %x, dest addr %x\n", 0, BLOCK_SIZE,
		 dma_block_cfg.source_address, dma_block_cfg.dest_address);

	if (dma_config(dma, chan_id, &dma_cfg)) {
		TC_PRINT("ERROR: transfer config (%d)\n", chan_id);
		return TC_FAIL;
	}

	TC_PRINT("Starting the transfer on channel %d and waiting completion\n", chan_id);

	if (dma_start(dma, chan_id)) {
		TC_PRINT("ERROR: transfer start (%d)\n", chan_id);
		return TC_FAIL;
	}

	/* Append new blocks on the channel */
	while (rx_idx <= CONFIG_DMA_BA_XFER_SIZE - BLOCK_SIZE) {
		dma_block_cfg.source_address = (uint32_t)(tx_data + tx_idx);
		dma_block_cfg.dest_address = (uint32_t)(rx_data + rx_idx);

		if (!silabs_ldma_append_block(dma, chan_id, &dma_cfg)) {
			rx_idx += BLOCK_SIZE;
			tx_idx += BLOCK_SIZE;
		}
	}

	if (k_sem_take(&xfer_sem, K_MSEC(1000)) != 0) {
		TC_PRINT("Timed out waiting for xfers\n");
		return TC_FAIL;
	}

	TC_PRINT("Verify RX buffer should contain the full TX buffer string.\n");

	if (memcmp(tx_data, rx_data, CONFIG_DMA_BA_XFER_SIZE)) {
		return TC_FAIL;
	}

	TC_PRINT("Finished: DMA block append loopstress\n");
	return TC_PASS;
}

/* export test cases */
ZTEST(dma_m2m_ba, test_dma_m2m_ba_restart_transfer)
{
	zassert_true((test_ba_restart_transfer() == TC_PASS));
}

ZTEST(dma_m2m_ba, test_dma_m2m_ba_restart_in_isr)
{
	zassert_true((test_ba_restart_in_isr() == TC_PASS));
}

ZTEST(dma_m2m_ba, test_dma_m2m_stress_in_isr)
{
	zassert_true((test_ba_stress_in_isr() == TC_PASS));
}

ZTEST(dma_m2m_ba, test_dma_m2m_loopstress)
{
	zassert_true((test_ba_loopstress() == TC_PASS));
}
