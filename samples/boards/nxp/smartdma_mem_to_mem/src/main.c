/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * SmartDMA memory-to-memory sample.
 *
 * This sample uses the NXP SmartDMA coprocessor to move and transform a
 * buffer from one memory location to another without any CPU involvement
 * in the data path. It installs the standard MCUX SmartDMA firmware and
 * invokes one of its memory-to-memory routines, which reads a source
 * buffer, applies the firmware's fixed byte transformation, and writes
 * the result to a destination buffer.
 *
 * The SmartDMA firmware routine is selected through the Zephyr DMA API
 * dma_slot field. The firmware reads its parameter block (source buffer,
 * destination buffer, size and a private stack) from the address the
 * SmartDMA driver programs from dma_config.head_block. We therefore alias
 * head_block to point at the firmware parameter structure.
 *
 * The kSMARTDMA_RGB565To888 routine and the smartdma_rgb565_rgb888_param_t
 * type are SDK-defined identifiers for one such memory-to-memory routine;
 * they are used here purely as the mechanism to exercise a SmartDMA
 * memory-to-memory transfer.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_mcux_smartdma.h>
#include <zephyr/logging/log.h>

#include <fsl_smartdma.h>

LOG_MODULE_REGISTER(smartdma_m2m, LOG_LEVEL_INF);

/*
 * The SmartDMA firmware routine consumes its input in chunks of 64 bytes
 * (same constraint the FlexIO MCULCD SmartDMA driver applies to
 * buffersize). We therefore use a 64-byte source buffer so the size is a
 * valid multiple of 64.
 */
#define SRC_WORD_COUNT 32U

/*
 * Source buffer, treated as a generic block of data. It is filled at
 * runtime with a simple incrementing pattern so the sample does not depend
 * on any particular hand-crafted values.
 */
static uint16_t in_buf[SRC_WORD_COUNT];

/*
 * Destination buffer. The firmware routine expands every source word to
 * three output bytes, so the destination is three bytes per source word.
 */
static uint8_t out_buf[SRC_WORD_COUNT * 3U] __aligned(4);

/* Private stack used by the SmartDMA firmware, must be at least 64 bytes. */
static uint32_t smartdma_stack[32];

/* Parameter block handed to the SmartDMA firmware routine. */
static smartdma_rgb565_rgb888_param_t sdma_param __aligned(4);

static volatile bool transfer_done;

static void smartdma_callback(const struct device *dev, void *user_data,
			      uint32_t channel, int status)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);
	ARG_UNUSED(channel);
	ARG_UNUSED(status);

	transfer_done = true;
}

int main(void)
{
	const struct device *smartdma = DEVICE_DT_GET(DT_NODELABEL(smartdma));
	struct dma_config dma_cfg = {0};
	int ret;

	/* Early banner printed directly, before touching SmartDMA, so the
	 * console shows life even if the SmartDMA path misbehaves.
	 */
	printk("SmartDMA memory-to-memory sample starting\n");

	if (!device_is_ready(smartdma)) {
		LOG_ERR("SmartDMA device not ready");
		return -ENODEV;
	}

	/* Fill the source buffer with a generic incrementing data pattern. */
	for (uint32_t i = 0U; i < SRC_WORD_COUNT; i++) {
		in_buf[i] = (uint16_t)(i * 0x0101U);
	}

	/* Install the standard MCUX SmartDMA firmware, which provides the
	 * memory-to-memory routine used below.
	 */
	dma_smartdma_install_fw(smartdma, (uint8_t *)s_smartdmaDisplayFirmware,
				s_smartdmaDisplayFirmwareSize);

	/* Fill the parameter block for the firmware routine. */
	sdma_param.inBuf = (uint32_t *)in_buf;
	sdma_param.outBuf = (uint32_t *)out_buf;
	sdma_param.buffersize = sizeof(in_buf);
	sdma_param.smartdma_stack = smartdma_stack;

	/* dma_slot selects the firmware routine index; head_block is passed
	 * to the firmware as its parameter block pointer.
	 */
	dma_cfg.dma_slot = kSMARTDMA_RGB565To888;
	dma_cfg.channel_direction = MEMORY_TO_MEMORY;
	dma_cfg.block_count = 1;
	dma_cfg.head_block = (struct dma_block_config *)&sdma_param;
	dma_cfg.dma_callback = smartdma_callback;
	dma_cfg.user_data = NULL;

	transfer_done = false;

	ret = dma_config(smartdma, 0, &dma_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to configure SmartDMA (%d)", ret);
		return ret;
	}

	LOG_INF("Starting SmartDMA memory-to-memory transfer");

	ret = dma_start(smartdma, 0);
	if (ret < 0) {
		LOG_ERR("Failed to start SmartDMA (%d)", ret);
		return ret;
	}

	/* Wait for the firmware to signal completion through the interrupt,
	 * but bound the wait so the sample can never hang silently.
	 */
	for (int i = 0; i < 1000 && !transfer_done; i++) {
		k_msleep(1);
	}

	if (!transfer_done) {
		LOG_ERR("Timed out waiting for SmartDMA completion IRQ");
		return -ETIMEDOUT;
	}

	LOG_INF("Transfer complete, results:");
	for (uint32_t i = 0U; i < SRC_WORD_COUNT; i++) {
		LOG_INF("word %u: in 0x%04x -> out %02x %02x %02x",
			i, in_buf[i],
			out_buf[i * 3U + 0U],
			out_buf[i * 3U + 1U],
			out_buf[i * 3U + 2U]);
	}

	/* Quick sanity check on the transformed data. Recompute the firmware's
	 * byte transformation on the CPU for each source word and compare it
	 * against what the SmartDMA firmware wrote, so the sample fails loudly
	 * if the transfer did not produce the expected result.
	 */
	for (uint32_t i = 0U; i < SRC_WORD_COUNT; i++) {
		uint16_t word = in_buf[i];
		uint8_t hi5 = (word >> 11) & 0x1FU;
		uint8_t mid6 = (word >> 5) & 0x3FU;
		uint8_t lo5 = word & 0x1FU;
		/*
		 * The firmware emits the three expanded bytes in low-to-high
		 * field order (lo5, mid6, hi5). Each field is simply shifted
		 * left to align with the top of its output byte; the firmware
		 * does not replicate the field's high bits into the low bits.
		 */
		uint8_t expected0 = lo5 << 3;
		uint8_t expected1 = mid6 << 2;
		uint8_t expected2 = hi5 << 3;

		if ((out_buf[i * 3U + 0U] != expected0) ||
		    (out_buf[i * 3U + 1U] != expected1) ||
		    (out_buf[i * 3U + 2U] != expected2)) {
			LOG_ERR("word %u mismatch: got %02x %02x %02x, expected %02x %02x %02x",
				i, out_buf[i * 3U + 0U], out_buf[i * 3U + 1U],
				out_buf[i * 3U + 2U], expected0, expected1, expected2);
			return -EIO;
		}
	}

	LOG_INF("Result check passed: all %u words transformed correctly", SRC_WORD_COUNT);
	LOG_INF("SmartDMA memory-to-memory sample done");

	return 0;
}
