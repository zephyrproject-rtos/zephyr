/*
 * Copyright (c) 2021 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcx_spi_fiu

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <soc.h>

LOG_MODULE_REGISTER(spi_npcx_fiu, LOG_LEVEL_ERR);

#include "spi_context.h"

/* Device config */
struct npcx_spi_fiu_config {
	/* flash interface unit base address */
	uintptr_t base;
	/* clock configuration */
	struct npcx_clk_cfg clk_cfg;
};

/* Device data */
struct npcx_spi_fiu_data {
	struct spi_context ctx;
};

/* Driver convenience defines */
#define HAL_INSTANCE(dev)                                                                          \
	((struct fiu_reg *)((const struct npcx_spi_fiu_config *)(dev)->config)->base)

static inline void spi_npcx_fiu_cs_level(const struct device *dev, int level)
{
	struct fiu_reg *const inst = HAL_INSTANCE(dev);

	/* Set chip select to high/low level */
	if (level == 0)
		inst->UMA_ECTS &= ~BIT(NPCX_UMA_ECTS_SW_CS1);
	else
		inst->UMA_ECTS |= BIT(NPCX_UMA_ECTS_SW_CS1);
}

static inline void spi_npcx_fiu_exec_cmd(const struct device *dev, uint8_t code,
					 uint8_t cts)
{
	struct fiu_reg *const inst = HAL_INSTANCE(dev);

#ifdef CONFIG_ASSERT
	struct npcx_spi_fiu_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	/* Flash mutex must be held while executing UMA commands */
	__ASSERT((k_sem_count_get(&ctx->lock) == 0), "UMA is not locked");
#endif

	/* set UMA_CODE */
	inst->UMA_CODE = code;
	/* execute UMA flash transaction */
	inst->UMA_CTS = cts;
	while (IS_BIT_SET(inst->UMA_CTS, NPCX_UMA_CTS_EXEC_DONE))
		continue;
}

static int spi_npcx_fiu_transceive(const struct device *dev,
				   const struct spi_config *spi_cfg,
				   const struct spi_buf_set *tx_bufs,
				   const struct spi_buf_set *rx_bufs)
{
	struct npcx_spi_fiu_data *data = dev->data;
	struct fiu_reg *const inst = HAL_INSTANCE(dev);
	struct spi_context *ctx = &data->ctx;
	size_t cur_xfer_len;
	int error = 0;

	spi_context_lock(ctx, false, NULL, NULL, spi_cfg);
	ctx->config = spi_cfg;

	/*
	 * Configure UMA lock/unlock only if tx buffer set and rx buffer set
	 * are both empty.
	 */
	if (tx_bufs == NULL && rx_bufs == NULL) {
		if (spi_cfg->operation & SPI_LOCK_ON)
			inst->UMA_ECTS |= BIT(NPCX_UMA_ECTS_UMA_LOCK);
		else
			inst->UMA_ECTS &= ~BIT(NPCX_UMA_ECTS_UMA_LOCK);
		spi_context_unlock_unconditionally(ctx);
		return 0;
	}

	/* Assert chip assert */
	spi_npcx_fiu_cs_level(dev, 0);
	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, 1);
	if (rx_bufs == NULL) {
		while (spi_context_tx_buf_on(ctx)) {
			spi_npcx_fiu_exec_cmd(dev, *ctx->tx_buf,
					      UMA_CODE_CMD_WR_ONLY);
			spi_context_update_tx(ctx, 1, 1);
		}
	} else {
		cur_xfer_len = spi_context_longest_current_buf(ctx);
		for (size_t i = 0; i < cur_xfer_len; i++) {
			spi_npcx_fiu_exec_cmd(dev, *ctx->tx_buf,
					      UMA_CODE_CMD_WR_ONLY);
			spi_context_update_tx(ctx, 1, 1);
			spi_context_update_rx(ctx, 1, 1);
		}
		while (spi_context_rx_buf_on(ctx)) {
			inst->UMA_CTS = UMA_CODE_RD_BYTE(1);
			while (IS_BIT_SET(inst->UMA_CTS,
					  NPCX_UMA_CTS_EXEC_DONE))
				continue;
			/* Get read transaction results */
			*ctx->rx_buf = inst->UMA_DB0;
			spi_context_update_tx(ctx, 1, 1);
			spi_context_update_rx(ctx, 1, 1);
		}
	}
	spi_npcx_fiu_cs_level(dev, 1);
	spi_context_release(ctx, error);

	return error;
}

int spi_npcx_fiu_release(const struct device *dev,
			 const struct spi_config *config)
{
	struct npcx_spi_fiu_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	if (!spi_context_configured(ctx, config)) {
		return -EINVAL;
	}

	spi_context_unlock_unconditionally(ctx);
	return 0;
}

static int spi_npcx_fiu_init(const struct device *dev)
{
	const struct npcx_spi_fiu_config *const config = dev->config;
	const struct device *const clk_dev = DEVICE_DT_GET(NPCX_CLK_CTRL_NODE);
	int ret;

	if (!device_is_ready(clk_dev)) {
		LOG_ERR("%s device not ready", clk_dev->name);
		return -ENODEV;
	}

	/* Turn on device clock first and get source clock freq. */
	ret = clock_control_on(clk_dev,
			       (clock_control_subsys_t)&config->clk_cfg);
	if (ret < 0) {
		LOG_ERR("Turn on FIU clock fail %d", ret);
		return ret;
	}

	/* Make sure the context is unlocked */
	spi_context_unlock_unconditionally(&((struct npcx_spi_fiu_data *)dev->data)->ctx);

	return 0;
}

static struct spi_driver_api spi_npcx_fiu_api = {
	.transceive = spi_npcx_fiu_transceive,
	.release = spi_npcx_fiu_release,
};

static const struct npcx_spi_fiu_config npcx_spi_fiu_config = {
	.base = DT_INST_REG_ADDR(0),
	.clk_cfg = NPCX_DT_CLK_CFG_ITEM(0),
};

static struct npcx_spi_fiu_data npcx_spi_fiu_data = {
	SPI_CONTEXT_INIT_LOCK(npcx_spi_fiu_data, ctx),
};

DEVICE_DT_INST_DEFINE(0, &spi_npcx_fiu_init, NULL, &npcx_spi_fiu_data,
		      &npcx_spi_fiu_config, POST_KERNEL,
		      CONFIG_SPI_INIT_PRIORITY, &spi_npcx_fiu_api);
