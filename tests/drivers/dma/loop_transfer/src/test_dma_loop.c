/* dma.c - DMA test source file */

/*
 * Copyright (c) 2016 Intel Corporation.
 * Copyright (c) 2021 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Verify zephyr dma memory to memory transfer loops
 * @details
 * - Test Steps
 *   -# Set dma channel configuration including source/dest addr, burstlen
 *   -# Set direction memory-to-memory
 *   -# Start transfer
 *   -# Move to next dest addr
 *   -# Back to first step
 * - Expected Results
 *   -# Data is transferred correctly from src to dest, for each loop
 */

#include <zephyr/kernel.h>

#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/pm/device.h>
#include <zephyr/ztest.h>

/* in millisecond */
#define SLEEPTIME 250

#define DATA                                                                                       \
	"The quick brown fox jumps over the lazy dog\n"                                            \
	"The quick brown fox jumps over the lazy dog\n"                                            \
	"The quick brown fox jumps over the lazy dog\n"                                            \
	"The quick brown fox jumps over the lazy dog\n"                                            \
	"The quick brown fox jumps over the lazy dog\n"                                            \
	"The quick brown fox jumps over the lazy dog\n"                                            \
	"The quick brown fox jumps over the lazy dog\n"                                            \
	"The quick brown fox jumps over the lazy dog\n"                                            \
	"The quick brown fox jumps over the lazy dog\n"                                            \
	"The quick brown fox jumps over the lazy dog\n"                                            \
	"The quick brown fox jumps over the lazy dog\n"                                            \
	"The quick brown fox jumps over the lazy dog\n"                                            \
	"The quick brown fox jumps over the lazy dog\n"                                            \
	"The quick brown fox jumps over the lazy dog\n"                                            \
	"The quick brown fox jumps over the lazy dog\n"                                            \
	"The quick brown fox jumps over the lazy dog\n"                                            \
	"The quick brown fox jumps over the lazy dog\n"                                            \
	"The quick brown fox jumps over the lazy dog\n"                                            \
	"The quick brown fox jumps over the lazy dog\n"                                            \
	"The quick brown fox jumps over the lazy dog\n"                                            \
	"The quick brown fox jumps over the lazy dog"

#define TRANSFER_LOOPS (4)
#define RX_BUFF_SIZE (1024)

#if CONFIG_NOCACHE_MEMORY
static const char TX_DATA[] = DATA;
static __aligned(32) char tx_data[1024] __used
	__attribute__((__section__(CONFIG_DMA_LOOP_TRANSFER_SRAM_SECTION)));
static __aligned(32) char rx_data[TRANSFER_LOOPS][RX_BUFF_SIZE] __used
	__attribute__((__section__(CONFIG_DMA_LOOP_TRANSFER_SRAM_SECTION".dma")));
#else
/* this src memory shall be in RAM to support usingas a DMA source pointer.*/
static char tx_data[] = DATA;
static __aligned(16) char rx_data[TRANSFER_LOOPS][RX_BUFF_SIZE] = { { 0 } };
#endif

volatile uint32_t transfer_count;
volatile uint32_t done;
static struct dma_config dma_cfg = {0};
static struct dma_block_config dma_block_cfg = {0};
static int test_case_id;

static void test_transfer(const struct device *dev, uint32_t id)
{
	transfer_count++;
	if (transfer_count < TRANSFER_LOOPS) {
		dma_block_cfg.block_size = strlen(tx_data);
		dma_block_cfg.source_address = (uint32_t)tx_data;
		dma_block_cfg.dest_address = (uint32_t)rx_data[transfer_count];

		zassert_false(dma_config(dev, id, &dma_cfg),
					"Not able to config transfer %d",
					transfer_count + 1);
		zassert_false(dma_start(dev, id),
					"Not able to start next transfer %d",
					transfer_count + 1);
	}
}

static void dma_user_callback(const struct device *dma_dev, void *arg,
			      uint32_t id, int error_code)
{
	/* test case is done so ignore the interrupt */
	if (done) {
		return;
	}

	zassert_false(error_code, "DMA could not proceed, an error occurred\n");

#ifdef CONFIG_DMAMUX_STM32
	/* the channel is the DMAMUX's one
	 * the device is the DMAMUX, given through
	 * the stream->user_data by the dma_stm32_irq_handler
	 */
	test_transfer((const struct device *)arg, id);
#else
	test_transfer(dma_dev, id);
#endif /* CONFIG_DMAMUX_STM32 */
}

static int test_loop(const struct device *dma)
{
	static int chan_id;

	test_case_id = 0;
	TC_PRINT("DMA memory to memory transfer started\n");

#if CONFIG_NOCACHE_MEMORY
	memset(tx_data, 0, sizeof(tx_data));
	memcpy(tx_data, TX_DATA, sizeof(TX_DATA));
#endif

	memset(rx_data, 0, sizeof(rx_data));

	if (!device_is_ready(dma)) {
		TC_PRINT("dma controller device is not ready\n");
		return TC_FAIL;
	}

	TC_PRINT("Preparing DMA Controller: %s\n", dma->name);
	dma_cfg.channel_direction = MEMORY_TO_MEMORY;
	dma_cfg.source_data_size = 1U;
	dma_cfg.dest_data_size = 1U;
	dma_cfg.source_burst_length = 1U;
	dma_cfg.dest_burst_length = 1U;
#ifdef CONFIG_DMAMUX_STM32
	dma_cfg.user_data = (void *)dma;
#else
	dma_cfg.user_data = NULL;
#endif /* CONFIG_DMAMUX_STM32 */
	dma_cfg.dma_callback = dma_user_callback;
	dma_cfg.block_count = 1U;
	dma_cfg.head_block = &dma_block_cfg;

#ifdef CONFIG_DMA_MCUX_TEST_SLOT_START
	dma_cfg.dma_slot = CONFIG_DMA_MCUX_TEST_SLOT_START;
#endif

	chan_id = dma_request_channel(dma, NULL);
	if (chan_id < 0) {
		TC_PRINT("this platform do not support the dma channel\n");
		chan_id = CONFIG_DMA_LOOP_TRANSFER_CHANNEL_NR;
	}
	transfer_count = 0;
	done = 0;
	TC_PRINT("Starting the transfer on channel %d and waiting for 1 second\n", chan_id);
	dma_block_cfg.block_size = strlen(tx_data);
	dma_block_cfg.source_address = (uint32_t)tx_data;
	dma_block_cfg.dest_address = (uint32_t)rx_data[transfer_count];

	if (dma_config(dma, chan_id, &dma_cfg)) {
		TC_PRINT("ERROR: transfer config (%d)\n", chan_id);
		return TC_FAIL;
	}

	if (dma_start(dma, chan_id)) {
		TC_PRINT("ERROR: transfer start (%d)\n", chan_id);
		return TC_FAIL;
	}

	k_sleep(K_MSEC(SLEEPTIME));

	if (transfer_count < TRANSFER_LOOPS) {
		transfer_count = TRANSFER_LOOPS;
		TC_PRINT("ERROR: unfinished transfer\n");
		if (dma_stop(dma, chan_id)) {
			TC_PRINT("ERROR: transfer stop\n");
		}
		return TC_FAIL;
	}

	TC_PRINT("Each RX buffer should contain the full TX buffer string.\n");

	for (int i = 0; i < TRANSFER_LOOPS; i++) {
		TC_PRINT("RX data Loop %d: %s\n", i, rx_data[i]);
		if (strncmp(tx_data, rx_data[i], sizeof(rx_data[i])) != 0) {
			return TC_FAIL;
		}
	}

	TC_PRINT("Finished DMA: %s\n", dma->name);
	return TC_PASS;
}

static int test_loop_suspend_resume(const struct device *dma)
{
	static int chan_id;
	int res = 0;

	test_case_id = 1;
	TC_PRINT("DMA memory to memory transfer started\n");

#if CONFIG_NOCACHE_MEMORY
	memset(tx_data, 0, sizeof(tx_data));
	memcpy(tx_data, TX_DATA, sizeof(TX_DATA));
#endif

	memset(rx_data, 0, sizeof(rx_data));

	if (!device_is_ready(dma)) {
		TC_PRINT("dma controller device is not ready\n");
		return TC_FAIL;
	}

	TC_PRINT("Preparing DMA Controller: %s\n", dma->name);
	dma_cfg.channel_direction = MEMORY_TO_MEMORY;
	dma_cfg.source_data_size = 1U;
	dma_cfg.dest_data_size = 1U;
	dma_cfg.source_burst_length = 1U;
	dma_cfg.dest_burst_length = 1U;
#ifdef CONFIG_DMAMUX_STM32
	dma_cfg.user_data = (struct device *)dma;
#else
	dma_cfg.user_data = NULL;
#endif /* CONFIG_DMAMUX_STM32 */
	dma_cfg.dma_callback = dma_user_callback;
	dma_cfg.block_count = 1U;
	dma_cfg.head_block = &dma_block_cfg;

#ifdef CONFIG_DMA_MCUX_TEST_SLOT_START
	dma_cfg.dma_slot = CONFIG_DMA_MCUX_TEST_SLOT_START;
#endif

	chan_id = dma_request_channel(dma, NULL);
	if (chan_id < 0) {
		TC_PRINT("this platform do not support the dma channel\n");
		chan_id = CONFIG_DMA_LOOP_TRANSFER_CHANNEL_NR;
	}
	transfer_count = 0;
	done = 0;
	TC_PRINT("Starting the transfer on channel %d and waiting for 1 second\n", chan_id);
	dma_block_cfg.block_size = strlen(tx_data);
	dma_block_cfg.source_address = (uint32_t)tx_data;
	dma_block_cfg.dest_address = (uint32_t)rx_data[transfer_count];

	unsigned int irq_key;

	if (dma_config(dma, chan_id, &dma_cfg)) {
		TC_PRINT("ERROR: transfer config (%d)\n", chan_id);
		return TC_FAIL;
	}

	if (dma_start(dma, chan_id)) {
		TC_PRINT("ERROR: transfer start (%d)\n", chan_id);
		return TC_FAIL;
	}

	/* Try multiple times to suspend the transfers */
	uint32_t tc = transfer_count;

	do {
		irq_key = irq_lock();
		res = dma_suspend(dma, chan_id);
		if (res == -ENOSYS) {
			done = 1;
			TC_PRINT("suspend not supported\n");
			dma_stop(dma, chan_id);
			return TC_PASS;
		}
		tc = transfer_count;
		irq_unlock(irq_key);
		k_busy_wait(100);
	} while (tc != transfer_count);

	/* If we failed to suspend we failed */
	if (transfer_count == TRANSFER_LOOPS) {
		TC_PRINT("ERROR: failed to suspend transfers\n");
		if (dma_stop(dma, chan_id)) {
			TC_PRINT("ERROR: transfer stop\n");
		}
		return TC_FAIL;
	}
	TC_PRINT("suspended after %d transfers occurred\n", transfer_count);

	/* Now sleep */
	k_sleep(K_MSEC(SLEEPTIME));

	/* If we failed to suspend we failed */
	if (transfer_count == TRANSFER_LOOPS) {
		TC_PRINT("ERROR: failed to suspend transfers\n");
		if (dma_stop(dma, chan_id)) {
			TC_PRINT("ERROR: transfer stop\n");
		}
		return TC_FAIL;
	}
	TC_PRINT("resuming after %d transfers occurred\n", transfer_count);

	res = dma_resume(dma, chan_id);
	TC_PRINT("Resumed transfers\n");
	if (res != 0) {
		TC_PRINT("ERROR: resume failed, channel %d, result %d", chan_id, res);
		if (dma_stop(dma, chan_id)) {
			TC_PRINT("ERROR: transfer stop\n");
		}
		return TC_FAIL;
	}

	k_sleep(K_MSEC(SLEEPTIME));

	TC_PRINT("Transfer count %d\n", transfer_count);
	if (transfer_count < TRANSFER_LOOPS) {
		transfer_count = TRANSFER_LOOPS;
		TC_PRINT("ERROR: unfinished transfer\n");
		if (dma_stop(dma, chan_id)) {
			TC_PRINT("ERROR: transfer stop\n");
		}
		return TC_FAIL;
	}

	TC_PRINT("Each RX buffer should contain the full TX buffer string.\n");

	for (int i = 0; i < TRANSFER_LOOPS; i++) {
		TC_PRINT("RX data Loop %d: %s\n", i, rx_data[i]);
		if (strncmp(tx_data, rx_data[i], sizeof(rx_data[i])) != 0) {
			return TC_FAIL;
		}
	}

	TC_PRINT("Finished DMA: %s\n", dma->name);
	return TC_PASS;
}

/**
 * @brief Check if the device is in valid power state.
 *
 * @param dev Device instance.
 * @param expected Device expected power state.
 *
 * @retval true If device is in correct power state.
 * @retval false If device is not in correct power state.
 */
static bool check_dev_power_state(const struct device *dev, enum pm_device_state expected)
{
#if CONFIG_PM_DEVICE_RUNTIME
	enum pm_device_state state;

	if (pm_device_state_get(dev, &state) == 0) {
		if (expected != state) {
			TC_PRINT("ERROR: device %s is incorrect power state"
				 " (current state = %s, expected = %s)\n",
				 dev->name, pm_device_state_str(state),
				 pm_device_state_str(expected));
			return false;
		}

		return true;
	}

	TC_PRINT("ERROR: unable to get power state of %s", dev->name);
	return false;
#else
	return true;
#endif /* CONFIG_PM_DEVICE_RUNTIME */
}

static int test_loop_repeated_start_stop(const struct device *dma)
{
	static int chan_id;
	enum pm_device_state init_state = pm_device_on_power_domain(dma) ?
					  PM_DEVICE_STATE_OFF : PM_DEVICE_STATE_SUSPENDED;

	test_case_id = 0;
	TC_PRINT("DMA memory to memory transfer started\n");
	TC_PRINT("Preparing DMA Controller\n");

#if CONFIG_NOCACHE_MEMORY
	memset(tx_data, 0, sizeof(tx_data));
	memcpy(tx_data, TX_DATA, sizeof(TX_DATA));
#endif

	memset(rx_data, 0, sizeof(rx_data));

	if (!device_is_ready(dma)) {
		TC_PRINT("dma controller device is not ready\n");
		return TC_FAIL;
	}

	dma_cfg.channel_direction = MEMORY_TO_MEMORY;
	dma_cfg.source_data_size = 1U;
	dma_cfg.dest_data_size = 1U;
	dma_cfg.source_burst_length = 1U;
	dma_cfg.dest_burst_length = 1U;
#ifdef CONFIG_DMAMUX_STM32
	dma_cfg.user_data = (void *)dma;
#else
	dma_cfg.user_data = NULL;
#endif /* CONFIG_DMAMUX_STM32 */
	dma_cfg.dma_callback = dma_user_callback;
	dma_cfg.block_count = 1U;
	dma_cfg.head_block = &dma_block_cfg;

#ifdef CONFIG_DMA_MCUX_TEST_SLOT_START
	dma_cfg.dma_slot = CONFIG_DMA_MCUX_TEST_SLOT_START;
#endif

	if (!check_dev_power_state(dma, PM_DEVICE_STATE_OFF)) {
		return TC_FAIL;
	}

	chan_id = dma_request_channel(dma, NULL);
	if (chan_id < 0) {
		TC_PRINT("this platform do not support the dma channel\n");
		chan_id = CONFIG_DMA_LOOP_TRANSFER_CHANNEL_NR;
	}
	transfer_count = 0;
	done = 0;
	TC_PRINT("Starting the transfer on channel %d and waiting for 1 second\n", chan_id);
	dma_block_cfg.block_size = strlen(tx_data);
	dma_block_cfg.source_address = (uint32_t)tx_data;
	dma_block_cfg.dest_address = (uint32_t)rx_data[transfer_count];

	if (dma_config(dma, chan_id, &dma_cfg)) {
		TC_PRINT("ERROR: transfer config (%d)\n", chan_id);
		return TC_FAIL;
	}

	if (dma_stop(dma, chan_id)) {
		TC_PRINT("ERROR: transfer stop on stopped channel (%d)\n", chan_id);
		return TC_FAIL;
	}

	if (!check_dev_power_state(dma, init_state)) {
		return TC_FAIL;
	}

	if (dma_start(dma, chan_id)) {
		TC_PRINT("ERROR: transfer start (%d)\n", chan_id);
		return TC_FAIL;
	}

	if (!check_dev_power_state(dma, PM_DEVICE_STATE_ACTIVE)) {
		return TC_FAIL;
	}

	k_sleep(K_MSEC(SLEEPTIME));

	if (transfer_count < TRANSFER_LOOPS) {
		transfer_count = TRANSFER_LOOPS;
		TC_PRINT("ERROR: unfinished transfer\n");
		if (dma_stop(dma, chan_id)) {
			TC_PRINT("ERROR: transfer stop\n");
		}
		return TC_FAIL;
	}

	TC_PRINT("Each RX buffer should contain the full TX buffer string.\n");

	for (int i = 0; i < TRANSFER_LOOPS; i++) {
		TC_PRINT("RX data Loop %d: %s\n", i, rx_data[i]);
		if (strncmp(tx_data, rx_data[i], sizeof(rx_data[i])) != 0) {
			return TC_FAIL;
		}
	}

	TC_PRINT("Finished: DMA\n");

	if (dma_stop(dma, chan_id)) {
		TC_PRINT("ERROR: transfer stop (%d)\n", chan_id);
		return TC_FAIL;
	}

	if (!check_dev_power_state(dma, init_state)) {
		return TC_FAIL;
	}

	if (dma_stop(dma, chan_id)) {
		TC_PRINT("ERROR: repeated transfer stop (%d)\n", chan_id);
		return TC_FAIL;
	}

	return TC_PASS;
}

#define DMA_NAME(i, _)	test_dma ## i
#define DMA_LIST	LISTIFY(CONFIG_DMA_LOOP_TRANSFER_NUMBER_OF_DMAS, DMA_NAME, (,))

#define TEST_LOOP(dma_name)                                                                        \
	ZTEST(dma_m2m_loop, test_ ## dma_name ## _m2m_loop)                                        \
	{                                                                                          \
		const struct device *dma = DEVICE_DT_GET(DT_NODELABEL(dma_name));                  \
		zassert_true((test_loop(dma) == TC_PASS));                                         \
	}

FOR_EACH(TEST_LOOP, (), DMA_LIST);

#define TEST_LOOP_SUSPEND_RESUME(dma_name)                                                         \
	ZTEST(dma_m2m_loop, test_ ## dma_name ## _m2m_loop_suspend_resume)                         \
	{                                                                                          \
		const struct device *dma = DEVICE_DT_GET(DT_NODELABEL(dma_name));                  \
		zassert_true((test_loop_suspend_resume(dma) == TC_PASS));                          \
	}

FOR_EACH(TEST_LOOP_SUSPEND_RESUME, (), DMA_LIST);

#define TEST_LOOP_REPEATED_START_STOP(dma_name)                                                    \
	ZTEST(dma_m2m_loop, test_ ## dma_name ## _m2m_loop_repeated_start_stop)                    \
	{                                                                                          \
		const struct device *dma = DEVICE_DT_GET(DT_NODELABEL(dma_name));                  \
		zassert_true((test_loop_repeated_start_stop(dma) == TC_PASS));                     \
	}

FOR_EACH(TEST_LOOP_REPEATED_START_STOP, (), DMA_LIST);
