/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/drivers/dma/dma_stm32.h>
#include <zephyr/ztest.h>

static struct dma_config valid_config(void)
{
	return (struct dma_config){
		.channel_direction = MEMORY_TO_PERIPHERAL,
		.source_data_size = 1U,
		.dest_data_size = 1U,
		.dma_slot = LL_LPDMA1_REQUEST_SPI1_TX,
	};
}

static int convert(const struct dma_config *cfg, hal_dma_direct_xfer_config_t *hal_cfg,
		   uint16_t src_adj, uint16_t dest_adj)
{
	return dma_stm32_zcfg_to_halcfg(DEVICE_DT_GET(DT_NODELABEL(lpdma1)), cfg, hal_cfg, src_adj,
					dest_adj);
}

ZTEST(stm32_hal_config, test_directions_and_priorities)
{
	const hal_dma_direction_t directions[] = {
		HAL_DMA_DIRECTION_MEMORY_TO_MEMORY,
		HAL_DMA_DIRECTION_MEMORY_TO_PERIPH,
		HAL_DMA_DIRECTION_PERIPH_TO_MEMORY,
	};
	const hal_dma_priority_t priorities[] = {
		HAL_DMA_PRIORITY_LOW_WEIGHT_LOW,
		HAL_DMA_PRIORITY_LOW_WEIGHT_MID,
		HAL_DMA_PRIORITY_LOW_WEIGHT_HIGH,
		HAL_DMA_PRIORITY_HIGH,
	};
	struct dma_config cfg = valid_config();
	hal_dma_direct_xfer_config_t hal_cfg;

	for (size_t dir = 0U; dir < ARRAY_SIZE(directions); dir++) {
		for (size_t priority = 0U; priority < ARRAY_SIZE(priorities); priority++) {
			cfg.channel_direction = dir;
			cfg.channel_priority = priority;
			zassert_ok(convert(&cfg, &hal_cfg, DMA_ADDR_ADJ_INCREMENT,
					   DMA_ADDR_ADJ_NO_CHANGE));
			zassert_equal(hal_cfg.direction, directions[dir]);
			zassert_equal(hal_cfg.priority, priorities[priority]);
			zassert_equal(hal_cfg.request, HAL_LPDMA1_REQUEST_SPI1_TX);
		}
	}
}

ZTEST(stm32_hal_config, test_data_widths)
{
	const uint32_t widths[] = {1U, 2U, 4U};
	const hal_dma_src_data_width_t src_widths[] = {
		HAL_DMA_SRC_DATA_WIDTH_BYTE,
		HAL_DMA_SRC_DATA_WIDTH_HALFWORD,
		HAL_DMA_SRC_DATA_WIDTH_WORD,
	};
	const hal_dma_dest_data_width_t dest_widths[] = {
		HAL_DMA_DEST_DATA_WIDTH_BYTE,
		HAL_DMA_DEST_DATA_WIDTH_HALFWORD,
		HAL_DMA_DEST_DATA_WIDTH_WORD,
	};
	struct dma_config cfg = valid_config();
	hal_dma_direct_xfer_config_t hal_cfg;

	for (size_t src = 0U; src < ARRAY_SIZE(widths); src++) {
		for (size_t dest = 0U; dest < ARRAY_SIZE(widths); dest++) {
			cfg.source_data_size = widths[src];
			cfg.dest_data_size = widths[dest];
			zassert_ok(convert(&cfg, &hal_cfg, DMA_ADDR_ADJ_INCREMENT,
					   DMA_ADDR_ADJ_NO_CHANGE));
			zassert_equal(hal_cfg.src_data_width, src_widths[src]);
			zassert_equal(hal_cfg.dest_data_width, dest_widths[dest]);
		}
	}
}

ZTEST(stm32_hal_config, test_address_adjustments)
{
	struct dma_config cfg = valid_config();
	hal_dma_direct_xfer_config_t hal_cfg;

	zassert_ok(convert(&cfg, &hal_cfg, DMA_ADDR_ADJ_INCREMENT, DMA_ADDR_ADJ_NO_CHANGE));
	zassert_equal(hal_cfg.src_inc, HAL_DMA_SRC_ADDR_INCREMENTED);
	zassert_equal(hal_cfg.dest_inc, HAL_DMA_DEST_ADDR_FIXED);
	zassert_ok(convert(&cfg, &hal_cfg, DMA_ADDR_ADJ_NO_CHANGE, DMA_ADDR_ADJ_INCREMENT));
	zassert_equal(hal_cfg.src_inc, HAL_DMA_SRC_ADDR_FIXED);
	zassert_equal(hal_cfg.dest_inc, HAL_DMA_DEST_ADDR_INCREMENTED);
	zassert_equal(convert(&cfg, &hal_cfg, DMA_ADDR_ADJ_DECREMENT, DMA_ADDR_ADJ_INCREMENT),
		      -ENOTSUP);
	zassert_equal(convert(&cfg, &hal_cfg, DMA_ADDR_ADJ_INCREMENT, DMA_ADDR_ADJ_DECREMENT),
		      -ENOTSUP);
	zassert_equal(convert(&cfg, &hal_cfg, UINT16_MAX, DMA_ADDR_ADJ_INCREMENT), -EINVAL);
	zassert_equal(convert(&cfg, &hal_cfg, DMA_ADDR_ADJ_INCREMENT, UINT16_MAX), -EINVAL);
}

ZTEST(stm32_hal_config, test_invalid_direction_and_priority)
{
	struct dma_config cfg = valid_config();
	hal_dma_direct_xfer_config_t hal_cfg;

	cfg.channel_direction = PERIPHERAL_TO_PERIPHERAL;
	zassert_equal(convert(&cfg, &hal_cfg, DMA_ADDR_ADJ_INCREMENT, DMA_ADDR_ADJ_NO_CHANGE),
		      -EINVAL);
	cfg = valid_config();
	cfg.channel_priority = 4U;
	zassert_equal(convert(&cfg, &hal_cfg, DMA_ADDR_ADJ_INCREMENT, DMA_ADDR_ADJ_NO_CHANGE),
		      -EINVAL);
}

ZTEST(stm32_hal_config, test_unsupported_widths)
{
	const uint32_t widths[] = {0U, 3U, 8U};
	hal_dma_direct_xfer_config_t hal_cfg;

	for (size_t i = 0U; i < ARRAY_SIZE(widths); i++) {
		struct dma_config cfg = valid_config();

		cfg.source_data_size = widths[i];
		zassert_equal(
			convert(&cfg, &hal_cfg, DMA_ADDR_ADJ_INCREMENT, DMA_ADDR_ADJ_NO_CHANGE),
			-ENOTSUP);
		cfg = valid_config();
		cfg.dest_data_size = widths[i];
		zassert_equal(
			convert(&cfg, &hal_cfg, DMA_ADDR_ADJ_INCREMENT, DMA_ADDR_ADJ_NO_CHANGE),
			-ENOTSUP);
	}
}

ZTEST(stm32_hal_config, test_requests)
{
	const hal_dma_request_source_t requests[] = {
		HAL_LPDMA1_REQUEST_SPI1_RX,
		HAL_LPDMA1_REQUEST_SPI1_TX,
		HAL_LPDMA1_REQUEST_I2C1_RX,
		HAL_LPDMA1_REQUEST_I2C1_TX,
	};
	struct dma_config cfg = valid_config();
	hal_dma_direct_xfer_config_t hal_cfg;

	for (size_t i = 0U; i < ARRAY_SIZE(requests); i++) {
		cfg.dma_slot = requests[i];
		zassert_ok(convert(&cfg, &hal_cfg, DMA_ADDR_ADJ_INCREMENT, DMA_ADDR_ADJ_NO_CHANGE));
		zassert_equal(hal_cfg.request, requests[i]);
	}
}

ZTEST_SUITE(stm32_hal_config, NULL, NULL, NULL, NULL, NULL);
