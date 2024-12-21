/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2023 Nuvoton Technology Corporation.
 */

#define DT_DRV_COMPAT nuvoton_numaker_spi

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_numaker.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi/rtio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(spi_numaker, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"
#include <NuMicro.h>

#define SPI_NUMAKER_TX_NOP 0x00

struct spi_numaker_config {
	SPI_T *spi;
	bool is_qspi;
	const struct reset_dt_spec reset;
	/* clock configuration */
	uint32_t clk_modidx;
	uint32_t clk_src;
	uint32_t clk_div;
	const struct device *clk_dev;
	const struct pinctrl_dev_config *pincfg;
};

struct spi_numaker_data {
	struct spi_context ctx;
};

/*
 * CPOL/CPHA = 0/0 --> SPI_MODE_0
 * CPOL/CPHA = 0/1 --> SPI_MODE_1
 * CPOL/CPHA = 1/0 --> SPI_MODE_2
 * CPOL/CPHA = 1/1 --> SPI_MODE_3
 */
static const uint32_t smode_tbl[4] = {
	SPI_MODE_0, SPI_MODE_1, SPI_MODE_2, SPI_MODE_3
};

static const uint32_t qsmode_tbl[4] = {
	QSPI_MODE_0, QSPI_MODE_1, QSPI_MODE_2, QSPI_MODE_3
};

static int spi_numaker_configure(const struct device *dev, const struct spi_config *config)
{
	int mode;
	struct spi_numaker_data *data = dev->data;
	const struct spi_numaker_config *dev_cfg = dev->config;

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

	/* Clear FIFO */
	SPI_ClearRxFIFO(dev_cfg->spi);
	SPI_ClearTxFIFO(dev_cfg->spi);

	if (SPI_MODE_GET(config->operation) & SPI_MODE_CPOL) {
		mode = (SPI_MODE_GET(config->operation) & SPI_MODE_CPHA) ? 3 : 2;
	} else {
		mode = (SPI_MODE_GET(config->operation) & SPI_MODE_CPHA) ? 1 : 0;
	}

	/* Make SPI module be ready to transfer */
	if (dev_cfg->is_qspi) {
		QSPI_Open((QSPI_T *)dev_cfg->spi,
			  (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_SLAVE) ? QSPI_SLAVE
										    : QSPI_MASTER,
			  qsmode_tbl[mode],
			  SPI_WORD_SIZE_GET(config->operation), config->frequency);
	} else {
		SPI_Open(dev_cfg->spi,
			 (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_SLAVE) ? SPI_SLAVE
										   : SPI_MASTER,
			 smode_tbl[mode],
			 SPI_WORD_SIZE_GET(config->operation), config->frequency);
	}

	/* Set Transfer LSB or MSB first */
	if ((config->operation) & SPI_TRANSFER_LSB) {
		SPI_SET_LSB_FIRST(dev_cfg->spi);
	} else {
		SPI_SET_MSB_FIRST(dev_cfg->spi);
	}

	/* full/half duplex */
	if (config->operation & SPI_HALF_DUPLEX) {
		/* half duplex, which results in 3-wire usage */
		SPI_ENABLE_3WIRE_MODE(dev_cfg->spi);
	} else {
		/* full duplex */
		SPI_DISABLE_3WIRE_MODE(dev_cfg->spi);
	}

	/* Active high CS logic */
	if (config->operation & SPI_CS_ACTIVE_HIGH) {
		SPI_SET_SS_HIGH(dev_cfg->spi);
	} else {
		SPI_SET_SS_LOW(dev_cfg->spi);
	}

	/* Enable the automatic hardware slave select function. Select the SS pin and configure as
	 * low-active.
	 */
	if (data->ctx.num_cs_gpios != 0) {
		SPI_EnableAutoSS(dev_cfg->spi, SPI_SS, SPI_SS_ACTIVE_LOW);
	} else {
		SPI_DisableAutoSS(dev_cfg->spi);
	}

	/* Be able to set TX/RX FIFO threshold, for ex: SPI_SetFIFO(dev_cfg->spi, 2, 2) */

	data->ctx.config = config;

	return 0;
}

static int spi_numaker_txrx(const struct device *dev)
{
	struct spi_numaker_data *data = dev->data;
	const struct spi_numaker_config *dev_cfg = dev->config;
	struct spi_context *ctx = &data->ctx;
	uint32_t tx_frame, rx_frame;
	uint8_t word_size, spi_dfs;
	uint32_t time_out_cnt;

	LOG_DBG("%s", __func__);
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
		return -EIO;
	}

	LOG_DBG("%s -->word_size [%d]", __func__, word_size);

	if (spi_context_tx_on(ctx)) {
		tx_frame = ((ctx->tx_buf == NULL) ? SPI_NUMAKER_TX_NOP
						  : UNALIGNED_GET((uint8_t *)(data->ctx.tx_buf)));
		/* Write to TX register */
		SPI_WRITE_TX(dev_cfg->spi, tx_frame);
		spi_context_update_tx(ctx, spi_dfs, 1);

		/* Check SPI busy status */
		time_out_cnt = SystemCoreClock; /* 1 second time-out */
		while (SPI_IS_BUSY(dev_cfg->spi)) {
			if (--time_out_cnt == 0) {
				LOG_ERR("Wait for SPI time-out");
				return -EIO;
			}
		}

		LOG_DBG("%s --> TX [0x%x] done", __func__, tx_frame);
	} else {
		/* Write dummy data to TX register */
		SPI_WRITE_TX(dev_cfg->spi, 0x00U);
		time_out_cnt = SystemCoreClock; /* 1 second time-out */
		while (SPI_IS_BUSY(dev_cfg->spi)) {
			if (--time_out_cnt == 0) {
				LOG_ERR("Wait for SPI time-out");
				return -EIO;
			}
		}
	}

	/* Read received data */
	if (spi_context_rx_on(ctx)) {
		if (SPI_GET_RX_FIFO_COUNT(dev_cfg->spi) > 0) {
			rx_frame = SPI_READ_RX(dev_cfg->spi);
			if (ctx->rx_buf != NULL) {
				UNALIGNED_PUT(rx_frame, (uint8_t *)data->ctx.rx_buf);
			}
			spi_context_update_rx(ctx, spi_dfs, 1);
			LOG_DBG("%s --> RX [0x%x] done", __func__, rx_frame);
		}
	}

	LOG_DBG("%s --> exit", __func__);
	return 0;
}

/* Remain TX/RX Data in spi_context TX/RX buffer */
static bool spi_numaker_remain_words(struct spi_numaker_data *data)
{
	return spi_context_tx_on(&data->ctx) || spi_context_rx_on(&data->ctx);
}

static int spi_numaker_transceive(const struct device *dev, const struct spi_config *config,
				  const struct spi_buf_set *tx_bufs,
				  const struct spi_buf_set *rx_bufs)
{
	struct spi_numaker_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	const struct spi_numaker_config *dev_cfg = dev->config;
	int ret;

	LOG_DBG("%s", __func__);
	spi_context_lock(ctx, false, NULL, NULL, config);
	ctx->config = config;

	ret = spi_numaker_configure(dev, config);
	if (ret < 0) {
		goto done;
	}

	SPI_ENABLE(dev_cfg->spi);

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

	/* if cs is defined: software cs control, set active true */
	if (spi_cs_is_gpio(config)) {
		spi_context_cs_control(&data->ctx, true);
	}

	/* transceive tx/rx data */
	do {
		ret = spi_numaker_txrx(dev);
		if (ret < 0) {
			break;
		}
	} while (spi_numaker_remain_words(data));

	/* if cs is defined: software cs control, set active false */
	if (spi_cs_is_gpio(config)) {
		spi_context_cs_control(&data->ctx, false);
	}
	SPI_DISABLE(dev_cfg->spi);

done:
	spi_context_release(ctx, ret);
	LOG_DBG("%s --> [%d]", __func__, ret);
	return ret;
}

static int spi_numaker_release(const struct device *dev, const struct spi_config *config)
{
	struct spi_numaker_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	if (!spi_context_configured(ctx, config)) {
		return -EINVAL;
	}
	spi_context_unlock_unconditionally(ctx);

	return 0;
}

static const struct spi_driver_api spi_numaker_driver_api = {
	.transceive = spi_numaker_transceive,
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
	.release = spi_numaker_release
};

static int spi_numaker_init(const struct device *dev)
{
	struct spi_numaker_data *data = dev->data;
	const struct spi_numaker_config *dev_cfg = dev->config;
	int err = 0;
	struct numaker_scc_subsys scc_subsys;

	SYS_UnlockReg();

	/* CLK controller */
	memset(&scc_subsys, 0x00, sizeof(scc_subsys));
	scc_subsys.subsys_id = NUMAKER_SCC_SUBSYS_ID_PCC;
	scc_subsys.pcc.clk_modidx = dev_cfg->clk_modidx;
	scc_subsys.pcc.clk_src = dev_cfg->clk_src;
	scc_subsys.pcc.clk_div = dev_cfg->clk_div;

	/* Equivalent to CLK_EnableModuleClock() */
	err = clock_control_on(dev_cfg->clk_dev, (clock_control_subsys_t)&scc_subsys);
	if (err != 0) {
		goto done;
	}
	/* Equivalent to CLK_SetModuleClock() */
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

	/* Reset this module, same as BSP's SYS_ResetModule(id_rst) */
	if (!device_is_ready(dev_cfg->reset.dev)) {
		LOG_ERR("reset controller not ready");
		err = -ENODEV;
		goto done;
	}

	/* Reset SPI to default state */
	reset_line_toggle_dt(&dev_cfg->reset);

done:
	SYS_LockReg();
	return err;
}

#define NUMAKER_SPI_INIT(inst)                                                                     \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	static struct spi_numaker_data spi_numaker_data_##inst = {                                 \
		SPI_CONTEXT_INIT_LOCK(spi_numaker_data_##inst, ctx),                               \
		SPI_CONTEXT_INIT_SYNC(spi_numaker_data_##inst, ctx),                               \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(inst), ctx)};                          \
	static struct spi_numaker_config spi_numaker_config_##inst = {                             \
		.spi = (SPI_T *)DT_INST_REG_ADDR(inst),                                            \
		.is_qspi = DT_INST_NODE_HAS_PROP(inst, qspi),                                      \
		.reset = RESET_DT_SPEC_INST_GET(inst),                                             \
		.clk_modidx = DT_INST_CLOCKS_CELL(inst, clock_module_index),                       \
		.clk_src = DT_INST_CLOCKS_CELL(inst, clock_source),                                \
		.clk_div = DT_INST_CLOCKS_CELL(inst, clock_divider),                               \
		.clk_dev = DEVICE_DT_GET(DT_PARENT(DT_INST_CLOCKS_CTLR(inst))),                    \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, spi_numaker_init, NULL, &spi_numaker_data_##inst,              \
			      &spi_numaker_config_##inst, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,   \
			      &spi_numaker_driver_api);

DT_INST_FOREACH_STATUS_OKAY(NUMAKER_SPI_INIT)
