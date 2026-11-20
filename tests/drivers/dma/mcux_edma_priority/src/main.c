/*
 * Copyright (c) 2026 Powersoft S.p.A.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Check the nxp,channel-priorities and nxp,channel-preemption properties of
 * the MCUX eDMA driver (eDMA version 2). The test reads DCHPRIn after boot
 * and compares it with values that are calculated by hand from the board
 * overlay, not with the algorithm of the driver. Then it does a memory to
 * memory transfer on each listed channel.
 */

#include <string.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/kernel.h>
#include <zephyr/linker/section_tags.h>
#include <zephyr/ztest.h>

#include <fsl_edma.h>

#define EDMA_NODE       DT_NODELABEL(tst_dma0)
#define EDMA_CHANNELS   DT_PROP(EDMA_NODE, dma_channels)
#define EDMA_GROUP_SIZE 16U

#define XFER_SIZE  256U
#define XFER_BURST 16U

BUILD_ASSERT(EDMA_CHANNELS == 32, "the expected values are for a 32-channel eDMA");
BUILD_ASSERT(DT_PROP(EDMA_NODE, nxp_channel_preemption),
	     "the overlay must set nxp,channel-preemption");

static const struct device *const edma_dev = DEVICE_DT_GET(EDMA_NODE);
static DMA_Type *const edma_base = (DMA_Type *)DT_REG_ADDR_BY_IDX(EDMA_NODE, 0);

/* The channels of nxp,channel-priorities in the overlay */
static const uint32_t listed_channels[] = {0, 1, 20, 31};

/* Not cached, so that the test needs no cache maintenance */
static __nocache __aligned(32) uint8_t tx_buf[XFER_SIZE];
static __nocache __aligned(32) uint8_t rx_buf[XFER_SIZE];

static K_SEM_DEFINE(xfer_done, 0, 1);
static int xfer_status;

/**
 * @brief Read DCHPRIn of a channel.
 *
 * The registers are bytes in big-endian order inside each word. The HAL
 * uses the same index to write them.
 */
static uint8_t read_dchpri(uint32_t channel)
{
	const volatile uint8_t *dchpri = &edma_base->DCHPRI3;

	return dchpri[DMA_DCHPRI_INDEX(channel)];
}

/**
 * @brief Expected priority of a channel after init.
 *
 * nxp,channel-priorities = <0 15>, <1 0>, <20 0>, <31 15>:
 * - <0 15>: channel 15 had 15 and gets 0, the old priority of channel 0.
 * - <1 0>: channel 15 now has 0 and gets 1, the old priority of channel 1.
 * - <20 0>: channel 16 had 0 and gets 4.
 * - <31 15>: channel 31 already has 15.
 * All other channels keep the reset priority, the channel number modulo 16.
 */
static uint8_t expected_priority(uint32_t channel)
{
	switch (channel) {
	case 0:
	case 31:
		return 15U;
	case 1:
	case 20:
		return 0U;
	case 15:
		return 1U;
	case 16:
		return 4U;
	default:
		return channel % EDMA_GROUP_SIZE;
	}
}

ZTEST(mcux_edma_priority, test_device_ready)
{
	zassert_true(device_is_ready(edma_dev), "eDMA init failed");
}

ZTEST(mcux_edma_priority, test_channel_priorities)
{
	for (uint32_t ch = 0; ch < EDMA_CHANNELS; ch++) {
		uint8_t chpri = read_dchpri(ch) & DMA_DCHPRI0_CHPRI_MASK;

		zassert_equal(chpri, expected_priority(ch), "channel %u: priority %u, expected %u",
			      ch, chpri, expected_priority(ch));
	}
}

ZTEST(mcux_edma_priority, test_priorities_unique_in_group)
{
	for (uint32_t first = 0; first < EDMA_CHANNELS; first += EDMA_GROUP_SIZE) {
		uint32_t used = 0;

		for (uint32_t ch = first; ch < first + EDMA_GROUP_SIZE; ch++) {
			uint8_t chpri = read_dchpri(ch) & DMA_DCHPRI0_CHPRI_MASK;

			zassert_false(used & BIT(chpri), "channel %u: priority %u is not unique",
				      ch, chpri);
			used |= BIT(chpri);
		}
	}
}

ZTEST(mcux_edma_priority, test_preemption_fields)
{
	for (uint32_t ch = 0; ch < EDMA_CHANNELS; ch++) {
		uint8_t dchpri = read_dchpri(ch);

		zassert_true(dchpri & DMA_DCHPRI0_ECP_MASK, "channel %u: ECP is 0", ch);
		zassert_false(dchpri & DMA_DCHPRI0_DPA_MASK, "channel %u: DPA is 1", ch);
	}
}

ZTEST(mcux_edma_priority, test_fixed_arbitration)
{
	/* Preemption needs fixed group and fixed channel arbitration */
	zassert_equal(edma_base->CR & (DMA_CR_ERCA_MASK | DMA_CR_ERGA_MASK), 0, "CR = 0x%08x",
		      edma_base->CR);
}

static void xfer_callback(const struct device *dev, void *user_data, uint32_t channel, int status)
{
	xfer_status = status;
	k_sem_give(&xfer_done);
}

static void run_transfer(uint32_t channel)
{
	struct dma_block_config block = {
		.source_address = (uintptr_t)tx_buf,
		.dest_address = (uintptr_t)rx_buf,
		.block_size = sizeof(tx_buf),
	};
	struct dma_config config = {
		.channel_direction = MEMORY_TO_MEMORY,
		.source_data_size = 4U,
		.dest_data_size = 4U,
		.source_burst_length = XFER_BURST,
		.dest_burst_length = XFER_BURST,
		.block_count = 1U,
		.head_block = &block,
		.dma_callback = xfer_callback,
	};

	for (size_t i = 0; i < sizeof(tx_buf); i++) {
		tx_buf[i] = (uint8_t)(i + channel);
	}
	(void)memset(rx_buf, 0, sizeof(rx_buf));
	k_sem_reset(&xfer_done);

	zassert_ok(dma_config(edma_dev, channel, &config), "channel %u: dma_config failed",
		   channel);
	zassert_ok(dma_start(edma_dev, channel), "channel %u: dma_start failed", channel);
	zassert_ok(k_sem_take(&xfer_done, K_MSEC(1000)), "channel %u: no callback", channel);
	zassert_equal(xfer_status, DMA_STATUS_COMPLETE, "channel %u: status %d", channel,
		      xfer_status);
	zassert_mem_equal(rx_buf, tx_buf, sizeof(tx_buf), "channel %u: data mismatch", channel);
}

ZTEST(mcux_edma_priority, test_mem_to_mem_transfer)
{
	for (size_t i = 0; i < ARRAY_SIZE(listed_channels); i++) {
		run_transfer(listed_channels[i]);
	}

	/* No error, so also no channel or group priority error (ES[CPE], ES[GPE]) */
	zassert_equal(edma_base->ES & DMA_ES_VLD_MASK, 0, "ES = 0x%08x", edma_base->ES);
}

ZTEST_SUITE(mcux_edma_priority, NULL, NULL, NULL, NULL, NULL);
