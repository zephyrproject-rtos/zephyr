/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2026 Nuvoton Technology Corporation.
 */

#define DT_DRV_COMPAT nuvoton_numaker_usci_spi

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_numaker.h>
#include <zephyr/drivers/pinctrl.h>
#include "spi_rtio.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(spi_numaker_usci, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"
#include <NuMicro.h>

#define USPI_NUMAKER_TX_NOP 0x00

struct spi_numaker_usci_config {
	USPI_T *uspi;
	const struct reset_dt_spec reset;
	uint32_t clk_modidx;
	uint32_t clk_src;
	uint32_t clk_div;
	const struct device *clk_dev;
	const struct pinctrl_dev_config *pincfg;
};

struct spi_numaker_usci_data {
	struct spi_context ctx;
};

static const uint32_t uspi_mode_tbl[4] = {
	USPI_MODE_0, USPI_MODE_1, USPI_MODE_2, USPI_MODE_3
};

static int spi_numaker_usci_configure(const struct device *dev, const struct spi_config *config)
{
	int mode;
	struct spi_numaker_usci_data *data = dev->data;
	const struct spi_numaker_usci_config *dev_cfg = dev->config;

	LOG_DBG("%s", __func__);
	if (spi_context_configured(&data->ctx, config)) {
		return 0;
	}

	if (SPI_MODE_GET(config->operation) & SPI_MODE_LOOP) {
		LOG_ERR("Loop back mode not support");
		return -ENOTSUP;
	}

	if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_SLAVE) {
		LOG_ERR("Slave mode not support");
		return -ENOTSUP;
	}

	USPI_ClearRxBuf(dev_cfg->uspi);
	USPI_ClearTxBuf(dev_cfg->uspi);

	if (SPI_MODE_GET(config->operation) & SPI_MODE_CPOL) {
		mode = (SPI_MODE_GET(config->operation) & SPI_MODE_CPHA) ? 3 : 2;
	} else {
		mode = (SPI_MODE_GET(config->operation) & SPI_MODE_CPHA) ? 1 : 0;
	}

	USPI_Open(dev_cfg->uspi,
		USPI_MASTER,
		uspi_mode_tbl[mode],
		SPI_WORD_SIZE_GET(config->operation),
		config->frequency);

	if (config->operation & SPI_TRANSFER_LSB) {
		USPI_SET_LSB_FIRST(dev_cfg->uspi);
	} else {
		USPI_SET_MSB_FIRST(dev_cfg->uspi);
	}

	/* full/half duplex */
	if (config->operation & SPI_HALF_DUPLEX) {
		/* half duplex, which results in 3-wire usage */
		USPI_ENABLE_3WIRE_MODE(dev_cfg->uspi);
	} else {
		/* full duplex */
		USPI_DISABLE_3WIRE_MODE(dev_cfg->uspi);
	}

	if (config->operation & SPI_CS_ACTIVE_HIGH) {
		USPI_EnableAutoSS(dev_cfg->uspi, 0, USPI_SS_ACTIVE_HIGH);
	} else {
		USPI_EnableAutoSS(dev_cfg->uspi, 0, USPI_SS_ACTIVE_LOW);
	}

	data->ctx.config = config;

	return 0;
}

static int spi_numaker_usci_txrx(const struct device *dev, uint8_t spi_dfs)
{
	struct spi_numaker_usci_data *data = dev->data;
	const struct spi_numaker_usci_config *dev_cfg = dev->config;
	struct spi_context *ctx = &data->ctx;
	uint32_t tx_frame, rx_frame;
	uint32_t time_out_cnt;

	LOG_DBG("%s", __func__);

	if (spi_context_tx_on(ctx)) {
		tx_frame = ((ctx->tx_buf == NULL) ? USPI_NUMAKER_TX_NOP
			   : UNALIGNED_GET((uint32_t *)data->ctx.tx_buf));
		USPI_WRITE_TX(dev_cfg->uspi, tx_frame);
		spi_context_update_tx(ctx, spi_dfs, 1);
		LOG_DBG("%s --> TX [0x%x] done", __func__, tx_frame);
	} else {
		USPI_WRITE_TX(dev_cfg->uspi, 0x00U);
	}

	if (spi_context_rx_on(ctx)) {
		if (!USPI_GET_RX_EMPTY_FLAG(dev_cfg->uspi)) {
			rx_frame = USPI_READ_RX(dev_cfg->uspi);
			if (ctx->rx_buf != NULL) {
				if (spi_dfs > 2) {
					UNALIGNED_PUT(rx_frame, (uint32_t *)data->ctx.rx_buf);
				} else if (spi_dfs > 1) {
					UNALIGNED_PUT(rx_frame, (uint16_t *)data->ctx.rx_buf);
				} else {
					UNALIGNED_PUT(rx_frame, (uint8_t *)data->ctx.rx_buf);
				}
			}
			spi_context_update_rx(ctx, spi_dfs, 1);
			LOG_DBG("%s --> RX [0x%x] done", __func__, rx_frame);
		}
	}

	time_out_cnt = SystemCoreClock;
	while (dev_cfg->uspi->PROTSTS & USPI_PROTSTS_BUSY_Msk) {
		if (--time_out_cnt == 0) {
			LOG_ERR("Wait for USCI_SPI time-out");
			return -EIO;
		}
	}

	LOG_DBG("%s --> exit", __func__);
	return 0;
}

static bool spi_numaker_usci_remain_words(struct spi_numaker_usci_data *data)
{
	return spi_context_tx_on(&data->ctx) || spi_context_rx_on(&data->ctx);
}

static int spi_numaker_usci_transceive(const struct device *dev, const struct spi_config *config,
	const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs)
{
	struct spi_numaker_usci_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int ret;
	uint8_t word_size, spi_dfs;

	LOG_DBG("%s", __func__);
	spi_context_lock(ctx, false, NULL, NULL, config);

	ret = spi_numaker_usci_configure(dev, config);
	if (ret < 0) {
		goto done;
	}

	word_size = SPI_WORD_SIZE_GET(ctx->config->operation);

	switch (word_size) {
	case 8:
		spi_dfs = 1;
		break;
	case 16:
		spi_dfs = 2;
		break;
	case 24:
		spi_dfs = 3;
		break;
	case 32:
		spi_dfs = 4;
		break;
	default:
		spi_dfs = 0;
		LOG_ERR("Not support SPI WORD size as [%d] bits", word_size);
		ret = -ENOTSUP;
		goto done;
	}

	LOG_DBG("%s -->word_size [%d]", __func__, word_size);

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, spi_dfs);

	/* if cs is defined: software cs control, set active true */
	spi_context_cs_control(&data->ctx, true);

	do {
		ret = spi_numaker_usci_txrx(dev, spi_dfs);
		if (ret < 0) {
			break;
		}
	} while (spi_numaker_usci_remain_words(data));

	/* if cs is defined: software cs control, set active false */
	spi_context_cs_control(&data->ctx, false);

done:
	spi_context_release(ctx, ret);
	LOG_DBG("%s --> [%d]", __func__, ret);
	return ret;
}

static int spi_numaker_usci_release(const struct device *dev, const struct spi_config *config)
{
	struct spi_numaker_usci_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	if (!spi_context_configured(ctx, config)) {
		return -EINVAL;
	}
	spi_context_unlock_unconditionally(ctx);

	return 0;
}

static DEVICE_API(spi, spi_numaker_usci_driver_api) = {
	.transceive = spi_numaker_usci_transceive,
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
	.release = spi_numaker_usci_release
};

static int spi_numaker_usci_init(const struct device *dev)
{
	struct spi_numaker_usci_data *data = dev->data;
	const struct spi_numaker_usci_config *dev_cfg = dev->config;
	int err = 0;
	struct numaker_scc_subsys scc_subsys;

	SYS_UnlockReg();

	memset(&scc_subsys, 0x00, sizeof(scc_subsys));
	scc_subsys.subsys_id = NUMAKER_SCC_SUBSYS_ID_PCC;
	scc_subsys.pcc.clk_modidx = dev_cfg->clk_modidx;
	scc_subsys.pcc.clk_src = dev_cfg->clk_src;
	scc_subsys.pcc.clk_div = dev_cfg->clk_div;

	err = clock_control_on(dev_cfg->clk_dev, (clock_control_subsys_t)&scc_subsys);
	if (err != 0) {
		goto done;
	}
	err = clock_control_configure(dev_cfg->clk_dev, (clock_control_subsys_t)&scc_subsys, NULL);
	if (err != 0) {
		goto done;
	}

	err = pinctrl_apply_state(dev_cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		LOG_ERR("Failed to apply pinctrl state");
		goto done;
	}

	err = spi_context_cs_configure_all(&data->ctx);
	if (err < 0) {
		goto done;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	if (!device_is_ready(dev_cfg->reset.dev)) {
		LOG_ERR("reset controller not ready");
		err = -ENODEV;
		goto done;
	}

	reset_line_toggle_dt(&dev_cfg->reset);

done:
	SYS_LockReg();
	return err;
}

#define NUMAKER_USPI_INIT(inst)                                                   \
	PINCTRL_DT_INST_DEFINE(inst);                                             \
	static struct spi_numaker_usci_data spi_numaker_usci_data_##inst = {      \
		SPI_CONTEXT_INIT_LOCK(spi_numaker_usci_data_##inst, ctx),         \
		SPI_CONTEXT_INIT_SYNC(spi_numaker_usci_data_##inst, ctx),         \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(inst), ctx)};         \
	static struct spi_numaker_usci_config spi_numaker_usci_config_##inst = {  \
		.uspi = (USPI_T *)DT_INST_REG_ADDR(inst),                         \
		.reset = RESET_DT_SPEC_INST_GET(inst),                            \
		.clk_modidx = DT_INST_CLOCKS_CELL(inst, clock_module_index),      \
		.clk_src = DT_INST_CLOCKS_CELL(inst, clock_source),               \
		.clk_div = DT_INST_CLOCKS_CELL(inst, clock_divider),              \
		.clk_dev = DEVICE_DT_GET(DT_PARENT(DT_INST_CLOCKS_CTLR(inst))),   \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                   \
	};                                                                        \
	SPI_DEVICE_DT_INST_DEFINE(inst, spi_numaker_usci_init, NULL,              \
				  &spi_numaker_usci_data_##inst,                  \
				  &spi_numaker_usci_config_##inst,                \
				  POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,          \
				  &spi_numaker_usci_driver_api);

DT_INST_FOREACH_STATUS_OKAY(NUMAKER_USPI_INIT)
