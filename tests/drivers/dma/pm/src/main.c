/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * Copyright (c) 2024 STMicroelectronics
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/cache.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/policy.h>
#include <zephyr/ztest.h>

#define DMA_NODE             DT_NODELABEL(tst_dma0)
#define DMA_DATA_ALIGNMENT   DT_PROP_OR(DMA_NODE, dma_buf_addr_alignment, 32)
#define GUARD_BUF_SIZE       16U
#define TEST_BUF_SIZE        48U
#define STATE_NODE           DT_PHANDLE(DMA_NODE, zephyr_disabling_power_states)
#define TEST_STATE           PM_STATE_DT_INIT(STATE_NODE)
#define TEST_SUBSTATE        DT_PROP(STATE_NODE, substate_id)
#define NO_CALLBACK_CHANNEL  (DT_PROP(DMA_NODE, dma_channels) - 1U)
#define REPEAT_START_CHANNEL (NO_CALLBACK_CHANNEL - 1U)

#if CONFIG_NOCACHE_MEMORY
static __aligned(DMA_DATA_ALIGNMENT) char tx_data[TEST_BUF_SIZE] __used
	__attribute__((__section__(".nocache")));
static __aligned(DMA_DATA_ALIGNMENT) char rx_data[TEST_BUF_SIZE + GUARD_BUF_SIZE] __used
	__attribute__((__section__(".nocache.dma")));
#else
static __aligned(DMA_DATA_ALIGNMENT) char tx_data[TEST_BUF_SIZE];
static __aligned(DMA_DATA_ALIGNMENT) char rx_data[TEST_BUF_SIZE + GUARD_BUF_SIZE];
#endif

static const struct device *const dma = DEVICE_DT_GET(DMA_NODE);
static K_SEM_DEFINE(done, 0, 1);
static int completion_status;
static uint32_t test_channel;
static bool configured;

const struct pm_state_info *pm_policy_next_state(uint8_t cpu, int32_t ticks)
{
	ARG_UNUSED(cpu);
	ARG_UNUSED(ticks);
	/* Exercise the constraints without entering a CPU power state. */
	return NULL;
}

static bool state_locked(void)
{
	return pm_policy_state_lock_is_active(TEST_STATE, TEST_SUBSTATE);
}

static void transfer_done(const struct device *dev, void *arg, uint32_t channel, int status)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(arg);
	ARG_UNUSED(channel);
	completion_status = status;
	k_sem_give(&done);
}

static void configure_transfer(uint32_t channel, bool callback)
{
	struct dma_block_config block = {
		.source_address = (uintptr_t)tx_data,
		.dest_address = (uintptr_t)rx_data,
		.block_size = TEST_BUF_SIZE,
	};
	struct dma_config config = {
		.channel_direction = MEMORY_TO_MEMORY,
		.source_data_size = 1U,
		.dest_data_size = 1U,
		.source_burst_length = 1U,
		.dest_burst_length = 1U,
		.block_count = 1U,
		.head_block = &block,
		.dma_callback = callback ? transfer_done : NULL,
	};

	for (size_t i = 0U; i < sizeof(tx_data); i++) {
		tx_data[i] = (char)(i + 1U);
	}
	memset(rx_data, 0, sizeof(rx_data));
	sys_cache_data_flush_range(tx_data, sizeof(tx_data));
	sys_cache_data_flush_and_invd_range(rx_data, sizeof(rx_data));
	zassert_equal(dma_config(dma, channel, &config), 0);
	test_channel = channel;
	configured = true;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	configured = false;
	k_sem_reset(&done);
	completion_status = -EINPROGRESS;
	zassert_true(device_is_ready(dma));
	zassert_true(pm_policy_device_is_disabling_state(dma, TEST_STATE, TEST_SUBSTATE));
	zassert_false(state_locked());
}

static void after(void *fixture)
{
	int ret = 0;

	ARG_UNUSED(fixture);
	if (configured) {
		ret = dma_stop(dma, test_channel);
	}
	/* Recover the isolated test constraint after a failed assertion. */
	for (uint32_t i = 0U; i < 2U && state_locked(); i++) {
		pm_policy_state_lock_put(TEST_STATE, TEST_SUBSTATE);
	}
	zassert_equal(ret, 0);
	zassert_false(state_locked());
}

ZTEST(dma_pm, test_repeated_start_stop)
{
	int first;
	int second;
	int stopped;
	bool locked;
	unsigned int key;

	configure_transfer(REPEAT_START_CHANNEL, true);
	/* Keep completion callbacks from clearing busy between the starts. */
	key = irq_lock();
	first = dma_start(dma, test_channel);
	second = dma_start(dma, test_channel);
	locked = state_locked();
	stopped = dma_stop(dma, test_channel);
	irq_unlock(key);

	zassert_equal(first, 0);
	zassert_equal(second, 0);
	zassert_true(locked);
	zassert_equal(stopped, 0);
	zassert_false(state_locked());
}

ZTEST(dma_pm, test_repeated_start_completion)
{
	int first;
	int second;
	unsigned int key;

	configure_transfer(REPEAT_START_CHANNEL, true);
	key = irq_lock();
	first = dma_start(dma, test_channel);
	second = dma_start(dma, test_channel);
	irq_unlock(key);

	zassert_equal(first, 0);
	zassert_equal(second, 0);
	zassert_equal(k_sem_take(&done, K_SECONDS(2)), 0);
	zassert_equal(completion_status, DMA_STATUS_COMPLETE);
	zassert_false(state_locked());
	sys_cache_data_invd_range(rx_data, sizeof(rx_data));
	zassert_mem_equal(tx_data, rx_data, TEST_BUF_SIZE);
}

ZTEST(dma_pm, test_completion_without_callback)
{
	struct dma_status status;
	int64_t deadline;
	unsigned int key;
	bool locked;
	int ret;

	/* Keep a separate channel whose first configuration has no callback. */
	configure_transfer(NO_CALLBACK_CHANNEL, false);
	key = irq_lock();
	ret = dma_start(dma, test_channel);
	locked = state_locked();
	irq_unlock(key);
	zassert_equal(ret, 0);
	zassert_true(locked);
	deadline = k_uptime_get() + 2000;
	do {
		zassert_equal(dma_get_status(dma, test_channel, &status), 0);
		if (!status.busy) {
			break;
		}
		k_msleep(1);
	} while (k_uptime_get() < deadline);

	zassert_false(status.busy, "DMA completion timed out");
	zassert_false(state_locked());
	zassert_equal(k_sem_count_get(&done), 0U);
	sys_cache_data_invd_range(rx_data, sizeof(rx_data));
	zassert_mem_equal(tx_data, rx_data, TEST_BUF_SIZE);
}

ZTEST_SUITE(dma_pm, NULL, NULL, before, after, NULL);
