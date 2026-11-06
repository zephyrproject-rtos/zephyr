/*
 * Copyright (c) 2026 Powersoft S.p.A.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Cause a real eDMA error on one channel while a second channel runs, and
 * check the error handler of the driver:
 * - the channel with the error gets its callback with a negative status, and
 *   it is not busy;
 * - the other channel continues and completes with correct data;
 * - at the end, the eDMA is not halted and no error flag is set;
 * - the channel with the error can do a new transfer.
 *
 * The error is a source bus error: the source address is in a reserved area
 * of the i.MX RT1170 memory map. The test reads eDMA v2 registers, thus it
 * runs only on the i.MX RT1170 EVK. The checks do not depend on CR[HOE], so
 * the same source runs with and without halt-on-error.
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/kernel.h>
#include <zephyr/linker/section_tags.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define EDMA_NODE DT_NODELABEL(edma0)

/* eDMA v2 registers, i.MX RT1170 RM rev. 5, 6.5.5 */
#define EDMA_BASE          DT_REG_ADDR_BY_IDX(EDMA_NODE, 0)
#define EDMA_CR_ADDR       (EDMA_BASE + 0x00U)
#define EDMA_ES_ADDR       (EDMA_BASE + 0x04U)
#define EDMA_ERR_ADDR      (EDMA_BASE + 0x2CU)
#define EDMA_CR_HALT       BIT(5)
#define EDMA_CR_HOE        BIT(4)
#define EDMA_ES_SBE        BIT(1)
#define EDMA_ES_ERRCHN(es) (((es) >> 8) & 0x1FU)

/*
 * Source address of the transfer that fails. The range 0x4280_0000 to
 * 0x5FFF_FFFF is reserved (RM rev. 5, table 3-1), so the read gives a
 * source bus error (ES[SBE]).
 */
#define BAD_SRC_ADDR 0x50000000U

/*
 * The long transfer moves one byte in each minor loop. Each minor loop is a
 * new service request, so the eDMA arbitrates many times during the transfer.
 * The transfer runs for some milliseconds, the error occurs at the first read
 * of the other channel.
 */
#define LONG_SIZE     (16U * 1024U)
#define SHORT_SIZE    64U
#define SHORT_WIDTH   4U
#define ERROR_TIMEOUT K_MSEC(100)
#define LONG_TIMEOUT  K_MSEC(1000)

/* Status value that the driver never gives, to see a missing callback */
#define STATUS_NONE 1

struct transfer_result {
	struct k_sem sem;
	volatile int status;
	volatile uint32_t calls;
};

static const struct device *const dma_dev = DEVICE_DT_GET(EDMA_NODE);

/*
 * DTCM is not cacheable, so the test needs no cache maintenance. The eDMA
 * can access DTCM: the driver keeps its TCD pool there on this board
 * (CONFIG_DMA_MCUX_USE_DTCM_FOR_DMA_DESCRIPTORS).
 */
static __dtcm_noinit_section uint8_t long_src[LONG_SIZE] __aligned(32);
static __dtcm_noinit_section uint8_t long_dst[LONG_SIZE] __aligned(32);
static __dtcm_noinit_section uint8_t short_src[SHORT_SIZE] __aligned(32);
static __dtcm_noinit_section uint8_t short_dst[SHORT_SIZE] __aligned(32);

static struct transfer_result long_result;
static struct transfer_result bad_result;

/* Values that the callback of the failing channel reads in the error interrupt */
static volatile uint32_t cr_at_error;
static volatile uint32_t es_at_error;
static volatile bool long_complete_at_error;

static void transfer_callback(const struct device *dev, void *user_data, uint32_t channel,
			      int status)
{
	struct transfer_result *result = user_data;

	ARG_UNUSED(dev);
	ARG_UNUSED(channel);

	result->status = status;
	result->calls++;
	k_sem_give(&result->sem);
}

/*
 * The driver calls this callback in the error interrupt, after it cleared the
 * error flag of the channel. ES keeps the fields of the last error (SBE,
 * ERRCHN) after that. CR is read for the log only: on the RT1170 the eDMA
 * clears CR[HALT] when the last error flag is cleared.
 */
static void error_callback(const struct device *dev, void *user_data, uint32_t channel, int status)
{
	cr_at_error = sys_read32(EDMA_CR_ADDR);
	es_at_error = sys_read32(EDMA_ES_ADDR);
	long_complete_at_error = (long_result.calls != 0U);

	transfer_callback(dev, user_data, channel, status);
}

/**
 * @brief Configure a memory to memory transfer with one element for each minor loop.
 *
 * @param channel DMA channel.
 * @param src Source address.
 * @param dst Destination address.
 * @param size Number of bytes.
 * @param width Element size in bytes.
 * @param callback Callback of the transfer.
 * @param result Result of the transfer, reset here.
 */
static void config_transfer(uint32_t channel, uintptr_t src, uintptr_t dst, uint32_t size,
			    uint32_t width, dma_callback_t callback, struct transfer_result *result)
{
	struct dma_block_config block_cfg = {
		.source_address = src,
		.dest_address = dst,
		.block_size = size,
	};
	struct dma_config dma_cfg = {
		.channel_direction = MEMORY_TO_MEMORY,
		.source_data_size = width,
		.dest_data_size = width,
		.source_burst_length = width,
		.dest_burst_length = width,
		.block_count = 1U,
		.head_block = &block_cfg,
		.dma_callback = callback,
		.user_data = result,
	};

	k_sem_init(&result->sem, 0, 1);
	result->status = STATUS_NONE;
	result->calls = 0U;

	zassert_ok(dma_config(dma_dev, channel, &dma_cfg), "dma_config of channel %u failed",
		   channel);
}

static void fill_pattern(uint8_t *buf, size_t size, uint8_t seed)
{
	for (size_t i = 0; i < size; i++) {
		buf[i] = (uint8_t)(i + seed);
	}
}

ZTEST(mcux_edma_error_handler, test_error_on_one_channel)
{
	struct dma_status status;
	uint32_t cr = sys_read32(EDMA_CR_ADDR);
	bool hoe = (cr & EDMA_CR_HOE) != 0U;
	int long_chan;
	int bad_chan;

	TC_PRINT("before the test: CR = 0x%08x, HOE = %d\n", cr, hoe);
	zassert_true(device_is_ready(dma_dev), "eDMA device is not ready");
	zassert_equal(cr & EDMA_CR_HALT, 0U, "eDMA is halted before the test");

	fill_pattern(long_src, LONG_SIZE, 1U);
	(void)memset(long_dst, 0, LONG_SIZE);
	(void)memset(short_dst, 0, SHORT_SIZE);

	/*
	 * With the reset priorities, a higher channel number has a higher
	 * priority (RM rev. 5, 6.5.5.18). The failing channel must not wait
	 * for the end of the long transfer.
	 */
	long_chan = dma_request_channel(dma_dev, NULL);
	bad_chan = dma_request_channel(dma_dev, NULL);
	zassert_true(long_chan >= 0 && bad_chan >= 0, "no free DMA channel");
	zassert_true(bad_chan > long_chan, "unexpected channel order %d, %d", long_chan, bad_chan);

	config_transfer(long_chan, (uintptr_t)long_src, (uintptr_t)long_dst, LONG_SIZE, 1U,
			transfer_callback, &long_result);
	config_transfer(bad_chan, BAD_SRC_ADDR, (uintptr_t)short_dst, SHORT_SIZE, SHORT_WIDTH,
			error_callback, &bad_result);

	zassert_ok(dma_start(dma_dev, long_chan), "dma_start of the long transfer failed");
	zassert_ok(dma_start(dma_dev, bad_chan), "dma_start of the failing transfer failed");

	zassert_ok(k_sem_take(&bad_result.sem, ERROR_TIMEOUT),
		   "no callback for the channel with the error");
	zassert_ok(k_sem_take(&long_result.sem, LONG_TIMEOUT),
		   "the long transfer did not complete");

	TC_PRINT("at the error: CR = 0x%08x, ES = 0x%08x, long transfer complete = %d\n",
		 cr_at_error, es_at_error, long_complete_at_error);

	/* The channel with the error */
	zassert_true(bad_result.status < 0, "status %d is not an error", bad_result.status);
	zassert_equal(bad_result.calls, 1U, "%u callbacks", bad_result.calls);
	zassert_ok(dma_get_status(dma_dev, bad_chan, &status), "dma_get_status failed");
	zassert_false(status.busy, "the channel with the error is busy");

	/* RM rev. 5, 6.5.5.3: the last error is a source bus error on bad_chan */
	zassert_true((es_at_error & EDMA_ES_SBE) != 0U, "ES[SBE] is not set");
	zassert_equal(EDMA_ES_ERRCHN(es_at_error), (uint32_t)bad_chan, "ES[ERRCHN] is wrong");

	/* The other channel must run during the error, else the test proves nothing */
	zassert_false(long_complete_at_error, "the long transfer completed before the error");

	/* The other channel */
	zassert_equal(long_result.status, DMA_STATUS_COMPLETE, "long transfer status %d",
		      long_result.status);
	zassert_equal(long_result.calls, 1U, "%u callbacks", long_result.calls);
	zassert_mem_equal(long_dst, long_src, LONG_SIZE, "long transfer data mismatch");

	/* The driver removed the error and the halt */
	cr = sys_read32(EDMA_CR_ADDR);
	TC_PRINT("after the error: CR = 0x%08x, ERR = 0x%08x\n", cr, sys_read32(EDMA_ERR_ADDR));
	zassert_equal(cr & EDMA_CR_HALT, 0U, "eDMA is halted after the error");
	zassert_equal(sys_read32(EDMA_ERR_ADDR), 0U, "an error flag is set");

	/* A new transfer on the channel that had the error */
	fill_pattern(short_src, SHORT_SIZE, 0x40U);
	(void)memset(short_dst, 0, SHORT_SIZE);
	config_transfer(bad_chan, (uintptr_t)short_src, (uintptr_t)short_dst, SHORT_SIZE,
			SHORT_WIDTH, transfer_callback, &bad_result);
	zassert_ok(dma_start(dma_dev, bad_chan), "dma_start of the new transfer failed");
	zassert_ok(k_sem_take(&bad_result.sem, ERROR_TIMEOUT), "the new transfer timed out");
	zassert_equal(bad_result.status, DMA_STATUS_COMPLETE, "new transfer status %d",
		      bad_result.status);
	zassert_mem_equal(short_dst, short_src, SHORT_SIZE, "new transfer data mismatch");

	(void)dma_stop(dma_dev, long_chan);
	(void)dma_stop(dma_dev, bad_chan);
	dma_release_channel(dma_dev, long_chan);
	dma_release_channel(dma_dev, bad_chan);
}

ZTEST_SUITE(mcux_edma_error_handler, NULL, NULL, NULL, NULL, NULL);
