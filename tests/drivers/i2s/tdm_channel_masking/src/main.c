/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Streams duplex TDM with different TX and RX channel masks over a wire from the data output to
 * the data input, and checks every received sample. Each enabled TX slot carries a tag in its top
 * 4 bits, its index among the enabled TX slots plus one, and the frame number in the other bits.
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define SLOTS        8U
#define WORD_SIZE    CONFIG_TDM_MASKING_TEST_WORD_SIZE
#define SAMPLE_BYTES (WORD_SIZE / 8U)
#define TAG_SHIFT    (WORD_SIZE - 4U)
#define FRAME_MASK   (BIT(TAG_SHIFT) - 1U)
#define FRAME_CLK    8000U
/* Frames in a full block, small enough for the 4 KB DMA region of nRF54H20 */
#define FRAMES       8U
#define BLOCKS       64U
#define BLOCK_MAX    (FRAMES * SLOTS * SAMPLE_BYTES)
/* Blocks queued before START, no more than the driver's TX queue length */
#define TX_PREFILL   4U
#define SLAB_COUNT   8U
#define TIMEOUT_MS   200

BUILD_ASSERT(WORD_SIZE == 16 || WORD_SIZE == 32, "word size must be 16 or 32 bits");
BUILD_ASSERT(FRAMES * BLOCKS <= FRAME_MASK, "frame numbers must fit below the tag");

static const struct device *const tdm_dev = DEVICE_DT_GET(DT_ALIAS(i2s_tdm));

K_MEM_SLAB_DEFINE_STATIC(tx_slab, BLOCK_MAX, SLAB_COUNT, 32);
K_MEM_SLAB_DEFINE_STATIC(rx_slab, BLOCK_MAX, SLAB_COUNT, 32);

/* RX block being checked, freed if the check fails */
static void *rx_block;

struct layout {
	uint8_t tx_slots;       /* Enabled TX slots, bit n for slot n */
	uint8_t rx_slots;       /* Enabled RX slots */
	uint8_t partial_frames; /* Frames in one short write mid-stream, 0 for none */
};

struct rx_check {
	uint8_t tag[SLOTS]; /* Tag expected at each RX position, 0 if TX does not send its slot */
	uint32_t first_pos; /* First RX position with a tag */
	uint32_t tx_n;
	uint32_t rx_n;
	uint32_t frames; /* Frames sent */
	uint32_t idx;    /* Stream index of the next RX sample */
	uint32_t lag;    /* RX frame of TX frame 0 */
	bool lag_known;
};

static uint32_t sample_get(const uint8_t *buf, size_t idx)
{
	uint16_t v16;
	uint32_t v32;

	if (WORD_SIZE == 16) {
		memcpy(&v16, &buf[idx * 2U], sizeof(v16));
		return v16;
	}

	memcpy(&v32, &buf[idx * 4U], sizeof(v32));
	return v32;
}

static void sample_put(uint8_t *buf, size_t idx, uint32_t value)
{
	uint16_t v16 = (uint16_t)value;

	if (WORD_SIZE == 16) {
		memcpy(&buf[idx * 2U], &v16, sizeof(v16));
		return;
	}

	memcpy(&buf[idx * 4U], &value, sizeof(value));
}

static int configure(enum i2s_dir dir, uint8_t slots, struct k_mem_slab *slab)
{
	struct i2s_config cfg = {
		.word_size = WORD_SIZE,
		.channels = SLOTS,
		.format = I2S_FMT_DATA_FORMAT_PCM_SHORT,
		.options = I2S_OPT_FRAME_CLK_CONTROLLER | I2S_OPT_BIT_CLK_CONTROLLER,
		.frame_clk_freq = FRAME_CLK,
		.mem_slab = slab,
		.block_size = FRAMES * POPCOUNT(slots) * SAMPLE_BYTES,
		.timeout = TIMEOUT_MS,
		.tdm.channel_disable_mask = (uint8_t)~slots,
	};

	return i2s_configure(tdm_dev, dir, &cfg);
}

static int unconfigure(void)
{
	const struct i2s_config cfg = {0};

	return i2s_configure(tdm_dev, I2S_DIR_BOTH, &cfg);
}

/* Write a block of @p frames frames numbered from @p first_frame. */
static int write_block(uint8_t tx_slots, uint32_t first_frame, uint32_t frames)
{
	const uint32_t tx_n = POPCOUNT(tx_slots);
	void *mem;
	int ret;

	ret = k_mem_slab_alloc(&tx_slab, &mem, K_MSEC(TIMEOUT_MS));
	if (ret < 0) {
		return ret;
	}

	for (uint32_t f = 0U; f < frames; f++) {
		for (uint32_t j = 0U; j < tx_n; j++) {
			sample_put(mem, f * tx_n + j,
				   ((j + 1U) << TAG_SHIFT) | ((first_frame + f) & FRAME_MASK));
		}
	}

	ret = i2s_write(tdm_dev, mem, frames * tx_n * SAMPLE_BYTES);
	if (ret < 0) {
		k_mem_slab_free(&tx_slab, mem);
	}

	return ret;
}

static void check_rx_block(struct rx_check *chk, const uint8_t *buf, size_t size, uint32_t block)
{
	for (size_t i = 0; i < size / SAMPLE_BYTES; i++, chk->idx++) {
		const uint32_t rx_frame = chk->idx / chk->rx_n;
		const uint32_t pos = chk->idx % chk->rx_n;
		const uint32_t value = sample_get(buf, i);
		const uint32_t tag = value >> TAG_SHIFT;
		const bool tagged = tag != 0U && tag <= chk->tx_n;
		uint32_t expected;

		if (chk->tag[pos] == 0U) {
			zassert_false(tagged,
				      "RX block %u sample %zu: TX data %x in a slot not sent",
				      block, i, value);
			continue;
		}

		if (!chk->lag_known) {
			if (!tagged) {
				continue;
			}
			zassert_true(pos == chk->first_pos && (value & FRAME_MASK) == 0U,
				     "RX block %u sample %zu: first TX data %x at RX position %u",
				     block, i, value, pos);
			chk->lag = rx_frame;
			chk->lag_known = true;
		}

		if (rx_frame - chk->lag >= chk->frames) {
			continue;
		}

		expected = ((uint32_t)chk->tag[pos] << TAG_SHIFT) | (rx_frame - chk->lag);
		zassert_equal(value, expected, "RX block %u sample %zu is %x, expected %x", block,
			      i, value, expected);
	}
}

static void stream(const struct layout *layout)
{
	static uint32_t tx_frames[BLOCKS];
	struct rx_check chk = {
		.first_pos = SLOTS,
		.tx_n = POPCOUNT(layout->tx_slots),
		.rx_n = POPCOUNT(layout->rx_slots),
	};
	uint32_t next_frame = 0U;
	uint32_t written = 0U;
	uint32_t read = 0U;
	uint32_t pos = 0U;
	bool draining = false;
	size_t size;
	int ret;

	TC_PRINT("TX slots 0x%02x, RX slots 0x%02x\n", layout->tx_slots, layout->rx_slots);

	for (uint32_t slot = 0U; slot < SLOTS; slot++) {
		if ((layout->rx_slots & BIT(slot)) == 0U) {
			continue;
		}
		if ((layout->tx_slots & BIT(slot)) != 0U) {
			chk.tag[pos] = POPCOUNT(layout->tx_slots & BIT_MASK(slot)) + 1U;
			chk.first_pos = MIN(chk.first_pos, pos);
		}
		pos++;
	}

	for (uint32_t k = 0U; k < BLOCKS; k++) {
		tx_frames[k] = (layout->partial_frames != 0U && k == BLOCKS / 2U)
				       ? layout->partial_frames
				       : FRAMES;
		chk.frames += tx_frames[k];
	}

	/* Unconfigure first, so that the new TX mask is not checked against the old RX mask. */
	zassert_ok(unconfigure());
	zassert_ok(configure(I2S_DIR_TX, layout->tx_slots, &tx_slab));
	zassert_ok(configure(I2S_DIR_RX, layout->rx_slots, &rx_slab));

	for (; written < TX_PREFILL; written++) {
		zassert_ok(write_block(layout->tx_slots, next_frame, tx_frames[written]));
		next_frame += tx_frames[written];
	}

	zassert_ok(i2s_trigger(tdm_dev, I2S_DIR_BOTH, I2S_TRIGGER_START));

	while (true) {
		ret = i2s_read(tdm_dev, &rx_block, &size);
		if (draining && ret == -EAGAIN) {
			/* The drained stream has stopped */
			break;
		}
		zassert_ok(ret, "RX block %u read failed", read);

		if (read < written) {
			zassert_equal(size, tx_frames[read] * chk.rx_n * SAMPLE_BYTES,
				      "RX block %u is %zu bytes", read, size);
			check_rx_block(&chk, rx_block, size, read);
		}
		k_mem_slab_free(&rx_slab, rx_block);
		rx_block = NULL;
		read++;

		if (written < BLOCKS) {
			zassert_ok(write_block(layout->tx_slots, next_frame, tx_frames[written]));
			next_frame += tx_frames[written];
			written++;
		} else if (!draining) {
			zassert_ok(i2s_trigger(tdm_dev, I2S_DIR_BOTH, I2S_TRIGGER_DRAIN));
			draining = true;
		}
	}

	zassert_true(read >= written, "%u TX blocks but %u RX blocks", written, read);
	zassert_true(chk.first_pos == SLOTS || chk.lag_known, "no TX data received");
}

static void stream_all(const struct layout *layouts, size_t count)
{
	for (size_t i = 0; i < count; i++) {
		stream(&layouts[i]);
	}
}

/** @brief TX sends every slot that RX receives. */
ZTEST(tdm_channel_masking, test_tx_covers_rx)
{
	static const struct layout layouts[] = {
		{0xFF, 0xFF}, {0xFF, 0x3F}, {0xFF, 0xFC}, {0x03, 0x03},
		{0xC0, 0xC0}, {0x3F, 0x3F}, {0x3F, 0x0C},
	};

	stream_all(layouts, ARRAY_SIZE(layouts));
}

/** @brief TX starts with or before RX and ends before it. */
ZTEST(tdm_channel_masking, test_tx_ends_before_rx)
{
	static const struct layout layouts[] = {
		{0x03, 0x3F}, {0x03, 0xFF}, {0x3F, 0xFF}, {0x3F, 0xFC}, {0x0F, 0x3C}, {0x03, 0xFC},
	};

	stream_all(layouts, ARRAY_SIZE(layouts));
}

/** @brief TX starts after RX and ends with it. */
ZTEST(tdm_channel_masking, test_tx_starts_after_rx)
{
	static const struct layout layouts[] = {
		{0xC0, 0xFC},
		{0xC0, 0xFF},
		{0xFC, 0xFF},
	};

	stream_all(layouts, ARRAY_SIZE(layouts));
}

/** @brief TX slots all after the RX slots, so only TX data in RX slots can be detected. */
ZTEST(tdm_channel_masking, test_tx_after_rx)
{
	static const struct layout layout = {0xFC, 0x03};

	stream(&layout);
}

/** @brief One 3-frame TX block mid-stream. */
ZTEST(tdm_channel_masking, test_short_block)
{
	static const struct layout layout = {0x03, 0x3F, 3U};

	stream(&layout);
}

/** @brief TX starting within the RX slots without ending with them is rejected. */
ZTEST(tdm_channel_masking, test_tx_within_rx_rejected)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_I2S_NRF_TDM_CHANNEL_MASK_WORKAROUND);

	zassert_ok(unconfigure());
	zassert_ok(configure(I2S_DIR_TX, 0xF0, &tx_slab));
	zassert_equal(configure(I2S_DIR_RX, 0x3C, &rx_slab), -EINVAL);
}

static void *tdm_setup(void)
{
	zassert_true(device_is_ready(tdm_dev), "%s is not ready", tdm_dev->name);

	return NULL;
}

static void tdm_after(void *fixture)
{
	ARG_UNUSED(fixture);

	/* Stop a stream left running by a failed check */
	(void)i2s_trigger(tdm_dev, I2S_DIR_BOTH, I2S_TRIGGER_DROP);
	if (rx_block != NULL) {
		k_mem_slab_free(&rx_slab, rx_block);
		rx_block = NULL;
	}
}

ZTEST_SUITE(tdm_channel_masking, NULL, tdm_setup, NULL, tdm_after, NULL);
