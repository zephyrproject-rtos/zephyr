/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Checks the DMA channel state machine around dma_stop(): a stopped channel must be
 * restartable with dma_reload() and dma_start() without another dma_config(), must
 * still deliver its completion callback, and must not deliver callbacks for a
 * transfer that was stopped.
 */

#include <limits.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/ztest.h>

#define XFER_SIZE 64U
#define BIG_SIZE  4096U
#define CHAN      CONFIG_DMA_RESTART_CHANNEL
#define CB_WAIT   K_MSEC(100)

static const struct device *const dma = DEVICE_DT_GET(DT_NODELABEL(tst_dma0));

static uint8_t tx_data[XFER_SIZE];
static uint8_t rx_data[XFER_SIZE];
static const uint8_t zero_data[XFER_SIZE];
static uint8_t big_src[BIG_SIZE];
static uint8_t big_dst[BIG_SIZE];
/* Source of the transfer that is never requested */
static uint32_t idle_periph_reg;

static struct dma_block_config blk;
static struct dma_config cfg;

static K_SEM_DEFINE(cb_sem, 0, 10);
static volatile int cb_count;
static volatile int cb_status;
static volatile uint8_t cb_big_last;

static void dma_cb(const struct device *dev, void *user_data, uint32_t channel, int status)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);
	ARG_UNUSED(channel);

	cb_count++;
	cb_status = status;
	cb_big_last = big_dst[BIG_SIZE - 1];
	k_sem_give(&cb_sem);
}

static int config_m2m_buf(uint8_t *src, uint8_t *dst, uint32_t size)
{
	memset(&blk, 0, sizeof(blk));
	memset(&cfg, 0, sizeof(cfg));
	blk.source_address = (uintptr_t)src;
	blk.dest_address = (uintptr_t)dst;
	blk.block_size = size;
	cfg.channel_direction = MEMORY_TO_MEMORY;
	cfg.source_data_size = 1U;
	cfg.dest_data_size = 1U;
	cfg.source_burst_length = 1U;
	cfg.dest_burst_length = 1U;
	cfg.dma_callback = dma_cb;
	cfg.block_count = 1U;
	cfg.head_block = &blk;

	return dma_config(dma, CHAN, &cfg);
}

static int config_m2m(void)
{
	return config_m2m_buf(tx_data, rx_data, XFER_SIZE);
}

/* A peripheral-to-memory transfer whose request never comes: it stays pending */
static int config_idle_p2m(void)
{
	memset(&blk, 0, sizeof(blk));
	memset(&cfg, 0, sizeof(cfg));
	blk.source_address = (uintptr_t)&idle_periph_reg;
	blk.dest_address = (uintptr_t)rx_data;
	blk.block_size = XFER_SIZE;
	blk.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	blk.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	cfg.channel_direction = PERIPHERAL_TO_MEMORY;
	cfg.dma_slot = CONFIG_DMA_RESTART_IDLE_SLOT;
	cfg.source_data_size = 1U;
	cfg.dest_data_size = 1U;
	cfg.source_burst_length = 1U;
	cfg.dest_burst_length = 1U;
	cfg.dma_callback = dma_cb;
	cfg.block_count = 1U;
	cfg.head_block = &blk;

	return dma_config(dma, CHAN, &cfg);
}

static void prepare(unsigned int seed)
{
	for (unsigned int i = 0U; i < XFER_SIZE; i++) {
		tx_data[i] = (uint8_t)(seed + i);
	}
	memset(rx_data, 0, sizeof(rx_data));
	k_sem_reset(&cb_sem);
	cb_count = 0;
	cb_status = INT_MAX;
}

static void expect_complete(const char *what)
{
	zassert_ok(k_sem_take(&cb_sem, CB_WAIT), "%s: no completion callback", what);
	zassert_equal(cb_status, DMA_STATUS_COMPLETE, "%s: callback status %d", what, cb_status);
	zassert_mem_equal(tx_data, rx_data, XFER_SIZE, "%s: data mismatch", what);
	k_msleep(2);
	zassert_equal(cb_count, 1, "%s: %d callbacks instead of 1", what, cb_count);
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	(void)dma_stop(dma, CHAN);
	k_msleep(2);
}

/* Reference: plain config and start */
ZTEST(dma_restart, test_config_start)
{
	prepare(1U);
	zassert_ok(config_m2m());
	zassert_ok(dma_start(dma, CHAN));
	expect_complete("config+start");
}

/* A completed transfer, not stopped, runs again after reload and start */
ZTEST(dma_restart, test_reload_without_stop)
{
	prepare(1U);
	zassert_ok(config_m2m());
	zassert_ok(dma_start(dma, CHAN));
	expect_complete("first");

	for (unsigned int i = 0U; i < 5U; i++) {
		prepare(10U + i);
		zassert_ok(
			dma_reload(dma, CHAN, (uintptr_t)tx_data, (uintptr_t)rx_data, XFER_SIZE));
		zassert_ok(dma_start(dma, CHAN));
		expect_complete("reload+start");
	}
}

/* A stopped channel restarts with reload and start, without dma_config() */
ZTEST(dma_restart, test_stop_reload_start)
{
	prepare(1U);
	zassert_ok(config_m2m());
	zassert_ok(dma_start(dma, CHAN));
	expect_complete("first");

	for (unsigned int i = 0U; i < 5U; i++) {
		zassert_ok(dma_stop(dma, CHAN));
		prepare(20U + i);
		zassert_ok(
			dma_reload(dma, CHAN, (uintptr_t)tx_data, (uintptr_t)rx_data, XFER_SIZE));
		zassert_ok(dma_start(dma, CHAN));
		expect_complete("stop+reload+start");
	}
}

/*
 * The same without an unconditional dma_start(). Drivers differ in whether dma_reload()
 * starts the transfer by itself; either is accepted, as long as the transfer completes
 * exactly once.
 */
ZTEST(dma_restart, test_stop_reload_only)
{
	prepare(1U);
	zassert_ok(config_m2m());
	zassert_ok(dma_start(dma, CHAN));
	expect_complete("first");

	zassert_ok(dma_stop(dma, CHAN));
	prepare(30U);
	zassert_ok(dma_reload(dma, CHAN, (uintptr_t)tx_data, (uintptr_t)rx_data, XFER_SIZE));
	k_msleep(20);
	if (cb_count == 0) {
		TC_PRINT("dma_reload() does not start the transfer\n");
		zassert_mem_equal(rx_data, zero_data, XFER_SIZE,
				  "data moved although no completion was reported");
		zassert_ok(dma_start(dma, CHAN));
	} else {
		TC_PRINT("dma_reload() starts the transfer by itself\n");
	}
	expect_complete("stop+reload");
}

/* Stopping a finished channel, also twice, leaves it reconfigurable */
ZTEST(dma_restart, test_stop_then_config)
{
	prepare(1U);
	zassert_ok(config_m2m());
	zassert_ok(dma_start(dma, CHAN));
	expect_complete("first");

	zassert_ok(dma_stop(dma, CHAN));
	zassert_ok(dma_stop(dma, CHAN), "repeated stop");
	prepare(40U);
	zassert_ok(config_m2m(), "config after stop");
	zassert_ok(dma_start(dma, CHAN));
	expect_complete("stop+config+start");
}

/* Stopping a pending transfer gives no callback, and the channel stays usable */
ZTEST(dma_restart, test_abort_pending)
{
	struct dma_status st;

	prepare(1U);
	zassert_ok(config_idle_p2m());
	zassert_ok(dma_start(dma, CHAN));
	k_msleep(5);
	zassert_equal(cb_count, 0, "callback without any DMA request (status %d)", cb_status);
	zassert_ok(dma_get_status(dma, CHAN, &st));
	zassert_true(st.busy, "pending transfer not reported busy");

	zassert_ok(dma_stop(dma, CHAN));
	k_msleep(5);
	zassert_equal(cb_count, 0, "callback after stop (status %d)", cb_status);
	zassert_ok(dma_get_status(dma, CHAN, &st));
	zassert_false(st.busy, "stopped channel still reported busy");

	/* Restart the pending transfer without config, and stop it again */
	zassert_ok(
		dma_reload(dma, CHAN, (uintptr_t)&idle_periph_reg, (uintptr_t)rx_data, XFER_SIZE));
	zassert_ok(dma_start(dma, CHAN));
	k_msleep(5);
	zassert_ok(dma_get_status(dma, CHAN, &st));
	zassert_true(st.busy, "restarted pending transfer not reported busy");
	zassert_ok(dma_stop(dma, CHAN));
	k_msleep(5);
	zassert_equal(cb_count, 0, "callback after second stop (status %d)", cb_status);

	prepare(50U);
	zassert_ok(config_m2m(), "config after abort");
	zassert_ok(dma_start(dma, CHAN));
	expect_complete("abort+config+start");
}

/*
 * dma_stop() right after completion, before the completion interrupt is taken, must not
 * lead to an error callback or leave the channel unusable.
 */
ZTEST(dma_restart, test_stop_races_completion)
{
	unsigned int key;
	int start_ret, stop_ret;

	for (unsigned int i = 0U; i < 5U; i++) {
		prepare(60U + i);
		zassert_ok(config_m2m(), "config in iteration %u", i);
		key = irq_lock();
		start_ret = dma_start(dma, CHAN);
		/* The transfer completes, its interrupt stays pending */
		k_busy_wait(200);
		stop_ret = dma_stop(dma, CHAN);
		irq_unlock(key);
		zassert_ok(start_ret, "start in iteration %u", i);
		zassert_ok(stop_ret, "stop in iteration %u", i);
		k_msleep(2);
		zassert_mem_equal(tx_data, rx_data, XFER_SIZE, "data mismatch");
		zassert_true(cb_count == 0 || cb_status == DMA_STATUS_COMPLETE,
			     "error callback after stop: %d callback(s), status %d", cb_count,
			     cb_status);
	}

	prepare(70U);
	zassert_ok(config_m2m(), "config after raced stop");
	zassert_ok(dma_start(dma, CHAN));
	expect_complete("raced stop+config+start");
}

/* Stopping a running transfer, then restarting with reload and start only */
ZTEST(dma_restart, test_abort_running_then_reload)
{
	struct dma_status st;
	unsigned int key;
	int start_ret, status_ret, stop_ret;

	for (unsigned int i = 0U; i < 3U; i++) {
		prepare(1U);
		zassert_ok(config_m2m_buf(big_src, big_dst, BIG_SIZE));

		key = irq_lock();
		start_ret = dma_start(dma, CHAN);
		status_ret = dma_get_status(dma, CHAN, &st);
		stop_ret = dma_stop(dma, CHAN);
		irq_unlock(key);
		zassert_ok(start_ret, "start in iteration %u", i);
		zassert_ok(status_ret, "get_status in iteration %u", i);
		zassert_ok(stop_ret, "stop in iteration %u", i);
		zassert_true(st.pending_length > 0, "transfer finished before it could be stopped");

		k_msleep(5);
		zassert_equal(cb_count, 0, "callback after abort (status %d)", cb_status);

		prepare(80U + i);
		zassert_ok(
			dma_reload(dma, CHAN, (uintptr_t)tx_data, (uintptr_t)rx_data, XFER_SIZE));
		zassert_ok(dma_start(dma, CHAN));
		expect_complete("abort+reload+start");
	}
}

/*
 * Stop a completed transfer and restart a longer one while the completion interrupt of
 * the first one is still pending: that interrupt must not be reported for the new
 * transfer.
 */
ZTEST(dma_restart, test_restart_before_stale_irq)
{
	unsigned int key;
	int start_ret, stop_ret, reload_ret, restart_ret;

	for (unsigned int i = 0U; i < BIG_SIZE; i++) {
		big_src[i] = (uint8_t)(i * 7U + 3U);
	}
	memset(big_dst, 0, sizeof(big_dst));

	prepare(1U);
	zassert_ok(config_m2m());
	key = irq_lock();
	start_ret = dma_start(dma, CHAN);
	/* The first transfer completes, its interrupt stays pending */
	k_busy_wait(200);
	stop_ret = dma_stop(dma, CHAN);
	reload_ret = dma_reload(dma, CHAN, (uintptr_t)big_src, (uintptr_t)big_dst, BIG_SIZE);
	restart_ret = dma_start(dma, CHAN);
	irq_unlock(key);
	zassert_ok(start_ret, "first start");
	zassert_ok(stop_ret, "stop");
	zassert_ok(reload_ret, "reload");
	zassert_ok(restart_ret, "restart");

	zassert_ok(k_sem_take(&cb_sem, CB_WAIT), "no callback for the restarted transfer");
	zassert_equal(cb_status, DMA_STATUS_COMPLETE, "restarted transfer reported status %d",
		      cb_status);
	zassert_equal(cb_big_last, big_src[BIG_SIZE - 1],
		      "completion reported before the restarted transfer finished");
	k_msleep(5);
	zassert_equal(cb_count, 1, "%d callbacks instead of 1 (last status %d)", cb_count,
		      cb_status);
	zassert_mem_equal(big_src, big_dst, BIG_SIZE, "data mismatch");
}

ZTEST_SUITE(dma_restart, NULL, NULL, before, NULL, NULL);
