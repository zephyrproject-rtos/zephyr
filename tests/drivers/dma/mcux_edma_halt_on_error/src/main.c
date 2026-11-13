/*
 * Copyright (c) 2026 Powersoft S.p.A.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Check that the nxp,no-halt-on-error property of the edma0 node controls
 * the halt-on-error bit of the eDMA, and that a memory to memory transfer
 * still completes. The same source runs with and without the property: the
 * expected value comes from the devicetree.
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

/* eDMA v2 Control Register (CR), i.MX RT1170 RM rev. 5, 6.5.5.2 */
#define EDMA_CR_ADDR DT_REG_ADDR_BY_IDX(EDMA_NODE, 0)
#define EDMA_CR_HALT BIT(5)
#define EDMA_CR_HOE  BIT(4)

/* Without the property, the driver keeps the HAL default: halt-on-error on */
#define HOE_EXPECTED (!DT_PROP(EDMA_NODE, nxp_no_halt_on_error))

#define TRANSFER_SIZE    64U
#define BURST_LENGTH     16U
#define TRANSFER_TIMEOUT K_MSEC(100)

static const struct device *const dma_dev = DEVICE_DT_GET(EDMA_NODE);

/* Non-cacheable buffers, so the test needs no cache maintenance */
static __nocache uint8_t src_buf[TRANSFER_SIZE] __aligned(32);
static __nocache uint8_t dst_buf[TRANSFER_SIZE] __aligned(32);

static K_SEM_DEFINE(transfer_done_sem, 0, 1);
static int transfer_status;

static void transfer_done(const struct device *dev, void *user_data, uint32_t channel, int status)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);
	ARG_UNUSED(channel);

	transfer_status = status;
	k_sem_give(&transfer_done_sem);
}

/**
 * @brief Check CR[HOE] against the devicetree and check that CR[HALT] is 0.
 *
 * @param when Text for the log, to tell the calls apart.
 */
static void check_control_register(const char *when)
{
	uint32_t cr = sys_read32(EDMA_CR_ADDR);

	TC_PRINT("%s: CR = 0x%08x, HOE expected %d\n", when, cr, HOE_EXPECTED);
	zassert_equal((cr & EDMA_CR_HOE) != 0U, HOE_EXPECTED, "CR[HOE] does not match devicetree");
	zassert_equal(cr & EDMA_CR_HALT, 0U, "eDMA is halted");
}

ZTEST(mcux_edma_halt_on_error, test_halt_on_error_after_init)
{
	check_control_register("after init");
}

ZTEST(mcux_edma_halt_on_error, test_mem_to_mem_transfer)
{
	struct dma_block_config block_cfg = {0};
	struct dma_config dma_cfg = {0};
	int channel;

	zassert_true(device_is_ready(dma_dev), "eDMA device is not ready");

	for (size_t i = 0; i < TRANSFER_SIZE; i++) {
		src_buf[i] = (uint8_t)(i + 1U);
	}
	(void)memset(dst_buf, 0, sizeof(dst_buf));

	block_cfg.source_address = (uintptr_t)src_buf;
	block_cfg.dest_address = (uintptr_t)dst_buf;
	block_cfg.block_size = TRANSFER_SIZE;

	dma_cfg.channel_direction = MEMORY_TO_MEMORY;
	dma_cfg.source_data_size = 1U;
	dma_cfg.dest_data_size = 1U;
	dma_cfg.source_burst_length = BURST_LENGTH;
	dma_cfg.dest_burst_length = BURST_LENGTH;
	dma_cfg.block_count = 1U;
	dma_cfg.head_block = &block_cfg;
	dma_cfg.dma_callback = transfer_done;
#ifdef CONFIG_DMA_MCUX_TEST_SLOT_START
	/* Parts without always-on DMAMUX slots need an always-enabled source */
	dma_cfg.dma_slot = CONFIG_DMA_MCUX_TEST_SLOT_START;
#endif

	channel = dma_request_channel(dma_dev, NULL);
	zassert_true(channel >= 0, "no free DMA channel");

	k_sem_reset(&transfer_done_sem);
	zassert_ok(dma_config(dma_dev, channel, &dma_cfg), "dma_config failed");
	zassert_ok(dma_start(dma_dev, channel), "dma_start failed");
	zassert_ok(k_sem_take(&transfer_done_sem, TRANSFER_TIMEOUT), "transfer timeout");

	(void)dma_stop(dma_dev, channel);
	dma_release_channel(dma_dev, channel);

	zassert_equal(transfer_status, DMA_STATUS_COMPLETE, "transfer status %d", transfer_status);
	zassert_mem_equal(dst_buf, src_buf, TRANSFER_SIZE, "data mismatch");

	check_control_register("after transfer");
}

ZTEST_SUITE(mcux_edma_halt_on_error, NULL, NULL, NULL, NULL, NULL);
