/*
 * Copyright (c) 2026 Mario Paja
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Full duplex loopback test for I2S drivers that stream RX and TX of one device
 * independently of each other. The data output is wired to the data input, so
 * everything that is sent is received back.
 *
 * The test configures both directions at once with I2S_DIR_BOTH and then checks
 * that:
 *  - data is received intact, with the left and the right channel in place,
 *  - stopping one direction does not disturb the other one,
 *  - a direction can be started again while the other one keeps running.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/sys/printk.h>
#include <zephyr/linker/section_tags.h>

#define I2S_NODE DT_NODELABEL(i2s_rxtx)

#define SAMPLE_RATE  48000
#define FRAMES       128
#define NUM_CH       2
#define BLOCK_SIZE   (FRAMES * NUM_CH * sizeof(uint16_t))
/* One slab serves both directions: queued TX blocks plus queued RX blocks */
#define BLOCK_COUNT  12
#define TIMEOUT_MS   1000

/* Blocks to ignore after a start, while the first data travels through */
#define SETTLE_BLOCKS 4
#define CHECK_BLOCKS  10

/*
 * The blocks are written by the CPU and by the DMA, so they live in memory
 * that the data cache does not cover.
 */
K_MEM_SLAB_DEFINE_IN_SECT_STATIC(slab, __nocache, BLOCK_SIZE, BLOCK_COUNT, 32);

static const struct device *const i2s_dev = DEVICE_DT_GET(I2S_NODE);

/* Counter in the TX data, always even at the start of a block */
static uint16_t tx_counter;
static int failures;

#define CHECK(cond, ...)						\
	do {								\
		if (!(cond)) {						\
			printk("  FAIL: " __VA_ARGS__);		\
			printk("\n");					\
			failures++;					\
		}							\
	} while (0)

struct rx_stats {
	uint32_t frames;
	uint32_t breaks;
	uint32_t misaligned;
	uint16_t prev_left;
	bool have_prev;
};

static int configure(bool gated)
{
	struct i2s_config cfg = {
		.word_size = 16,
		.channels = NUM_CH,
		.format = I2S_FMT_DATA_FORMAT_I2S,
		.options = gated ? I2S_OPT_BIT_CLK_GATED : 0,
		.frame_clk_freq = SAMPLE_RATE,
		.mem_slab = &slab,
		.block_size = BLOCK_SIZE,
		.timeout = TIMEOUT_MS,
	};

	return i2s_configure(i2s_dev, I2S_DIR_BOTH, &cfg);
}

/* Queue one block of counter data for transmission */
static int tx_block(void)
{
	uint16_t *buf;
	int ret = k_mem_slab_alloc(&slab, (void **)&buf, K_MSEC(TIMEOUT_MS));

	if (ret < 0) {
		return ret;
	}

	for (int i = 0; i < FRAMES * NUM_CH; i++) {
		buf[i] = tx_counter++;
	}

	ret = i2s_write(i2s_dev, buf, BLOCK_SIZE);
	if (ret < 0) {
		k_mem_slab_free(&slab, buf);
	}

	return ret;
}

/*
 * Receive one block. When stats is not NULL, check that the frames carry the
 * counter in order: the left word is even and the right word follows it, and
 * each frame continues the previous one.
 */
static int rx_block(struct rx_stats *stats)
{
	const uint16_t *buf;
	size_t size;
	int ret = i2s_read(i2s_dev, (void **)&buf, &size);

	if (ret < 0) {
		return ret;
	}

	if (stats != NULL) {
		for (int f = 0; f < FRAMES; f++) {
			uint16_t left = buf[2 * f];
			uint16_t right = buf[2 * f + 1];

			if ((uint16_t)(left + 1) != right || (left & 1U) != 0U) {
				stats->misaligned++;
			} else if (stats->have_prev &&
				   (uint16_t)(stats->prev_left + 2) != left) {
				stats->breaks++;
			}

			stats->prev_left = left;
			stats->have_prev = true;
			stats->frames++;
		}
	}

	k_mem_slab_free(&slab, (void *)buf);

	return 0;
}

/* Send and receive in step: n blocks, the first skip of them are not checked */
static void exchange(int n, int skip, struct rx_stats *stats)
{
	for (int i = 0; i < n; i++) {
		int ret = tx_block();

		if (ret < 0) {
			CHECK(false, "TX block %d failed: %d", i, ret);
			return;
		}

		ret = rx_block(i < skip ? NULL : stats);
		if (ret < 0) {
			CHECK(false, "RX block %d failed: %d", i, ret);
			return;
		}
	}
}

static void check_stats(const char *what, const struct rx_stats *s)
{
	printk("  %s: %u frames, %u breaks, %u misaligned\n", what, s->frames, s->breaks,
	       s->misaligned);
	CHECK(s->frames > 0U, "%s: nothing was checked", what);
	CHECK(s->breaks == 0U, "%s: %u data breaks", what, s->breaks);
	CHECK(s->misaligned == 0U, "%s: %u misaligned frames", what, s->misaligned);
}

static void check_slab(const char *what)
{
	uint32_t free_blocks = k_mem_slab_num_free_get(&slab);

	CHECK(free_blocks == BLOCK_COUNT, "%s: %u of %d blocks free", what, free_blocks,
	      BLOCK_COUNT);
}

static void stop_all(void)
{
	i2s_trigger(i2s_dev, I2S_DIR_TX, I2S_TRIGGER_DROP);
	i2s_trigger(i2s_dev, I2S_DIR_RX, I2S_TRIGGER_DROP);
}

/* Both directions run, RX is started first so that TX starts late */
static void test_loopback(bool gated)
{
	struct rx_stats stats = {0};

	printk("T1 loopback, bit clock %s\n", gated ? "gated" : "continuous");
	CHECK(configure(gated) == 0, "configure(I2S_DIR_BOTH)");

	tx_counter = 0;
	CHECK(i2s_trigger(i2s_dev, I2S_DIR_RX, I2S_TRIGGER_START) == 0, "RX start");
	for (int i = 0; i < 2; i++) {
		CHECK(tx_block() == 0, "TX queue");
	}
	CHECK(i2s_trigger(i2s_dev, I2S_DIR_TX, I2S_TRIGGER_START) == 0, "TX start");

	exchange(SETTLE_BLOCKS + 2 * CHECK_BLOCKS, SETTLE_BLOCKS, &stats);
	check_stats("loopback", &stats);

	stop_all();
	check_slab("T1");
}

/* Stopping TX must not stop RX, and TX can start again while RX runs */
static void test_stop_tx(bool gated)
{
	struct rx_stats stats = {0};
	int ret;

	printk("T2 stop TX while RX runs, bit clock %s\n", gated ? "gated" : "continuous");
	CHECK(configure(gated) == 0, "configure(I2S_DIR_BOTH)");

	tx_counter = 0;
	CHECK(i2s_trigger(i2s_dev, I2S_DIR_RX, I2S_TRIGGER_START) == 0, "RX start");
	for (int i = 0; i < 2; i++) {
		CHECK(tx_block() == 0, "TX queue");
	}
	CHECK(i2s_trigger(i2s_dev, I2S_DIR_TX, I2S_TRIGGER_START) == 0, "TX start");
	exchange(SETTLE_BLOCKS + CHECK_BLOCKS, SETTLE_BLOCKS, &stats);
	check_stats("before the stop", &stats);

	/* Stop TX; RX has to keep delivering blocks without any TX data */
	CHECK(i2s_trigger(i2s_dev, I2S_DIR_TX, I2S_TRIGGER_DROP) == 0, "TX drop");
	for (int i = 0; i < CHECK_BLOCKS; i++) {
		ret = rx_block(NULL);
		if (ret < 0) {
			CHECK(false, "RX stalled after the TX stop: %d", ret);
			break;
		}
	}

	/* Start TX again while RX is still running */
	memset(&stats, 0, sizeof(stats));
	tx_counter = 0;
	for (int i = 0; i < 2; i++) {
		CHECK(tx_block() == 0, "TX queue");
	}
	CHECK(i2s_trigger(i2s_dev, I2S_DIR_TX, I2S_TRIGGER_START) == 0, "TX restart");
	exchange(SETTLE_BLOCKS + CHECK_BLOCKS, SETTLE_BLOCKS, &stats);
	check_stats("after the restart", &stats);

	stop_all();
	check_slab("T2");
}

/* Stopping RX must not stop TX, and RX can start again while TX runs */
static void test_stop_rx(bool gated)
{
	struct rx_stats stats = {0};
	int ret;

	printk("T3 stop RX while TX runs, bit clock %s\n", gated ? "gated" : "continuous");
	CHECK(configure(gated) == 0, "configure(I2S_DIR_BOTH)");

	tx_counter = 0;
	for (int i = 0; i < 2; i++) {
		CHECK(tx_block() == 0, "TX queue");
	}
	CHECK(i2s_trigger(i2s_dev, I2S_DIR_TX, I2S_TRIGGER_START) == 0, "TX start");
	CHECK(i2s_trigger(i2s_dev, I2S_DIR_RX, I2S_TRIGGER_START) == 0, "RX start");
	exchange(SETTLE_BLOCKS + CHECK_BLOCKS, SETTLE_BLOCKS, &stats);
	check_stats("before the stop", &stats);

	/* Stop RX; TX has to keep consuming blocks, paced by the hardware */
	CHECK(i2s_trigger(i2s_dev, I2S_DIR_RX, I2S_TRIGGER_DROP) == 0, "RX drop");
	for (int i = 0; i < CHECK_BLOCKS; i++) {
		ret = tx_block();
		if (ret < 0) {
			CHECK(false, "TX stalled after the RX stop: %d", ret);
			break;
		}
	}

	/* Start RX again while TX is still running */
	memset(&stats, 0, sizeof(stats));
	CHECK(i2s_trigger(i2s_dev, I2S_DIR_RX, I2S_TRIGGER_START) == 0, "RX restart");
	exchange(SETTLE_BLOCKS + CHECK_BLOCKS, SETTLE_BLOCKS, &stats);
	check_stats("after the restart", &stats);

	stop_all();
	check_slab("T3");
}

int main(void)
{
	printk("I2S full duplex loopback sample\n");

	if (!device_is_ready(i2s_dev)) {
		printk("%s is not ready\n", i2s_dev->name);
		return 0;
	}

	for (int gated = 0; gated <= 1; gated++) {
		test_loopback(gated != 0);
		test_stop_tx(gated != 0);
		test_stop_rx(gated != 0);
	}

	printk("Full duplex loopback: %s\n", failures == 0 ? "PASS" : "FAIL");

	return 0;
}
