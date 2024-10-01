/*
 * Copyright 2023 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_cat1_spi

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cat1_spi);

#include "spi_context.h"

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi/rtio.h>
#include <zephyr/kernel.h>

#include <cyhal_scb_common.h>
#include <cyhal_spi.h>

#define IFX_CAT1_SPI_LOCK_TMOUT_MS      (30 * 1000)
#define IFX_CAT1_SPI_DEFAULT_OVERSAMPLE (4)
#define IFX_CAT1_SPI_MIN_DATA_WIDTH     (8)
#define IFX_CAT1_SPI_MAX_DATA_WIDTH     (32)

/* Device config structure */
struct ifx_cat1_spi_config {
	CySCB_Type *reg_addr;
	const struct pinctrl_dev_config *pcfg;
	cy_stc_scb_spi_config_t scb_spi_config;
	uint8_t irq_priority;
};

/* Data structure */
struct ifx_cat1_spi_data {
	struct spi_context ctx;
	cyhal_spi_t obj; /* SPI CYHAL object */
	cyhal_resource_inst_t hw_resource;
	uint8_t dfs_value;
	size_t chunk_len;
};

static int32_t get_hw_block_num(CySCB_Type *reg_addr)
{
	uint32_t i;

	for (i = 0u; i < _SCB_ARRAY_SIZE; i++) {
		if (_CYHAL_SCB_BASE_ADDRESSES[i] == reg_addr) {
			return i;
		}
	}

	return -ENOMEM;
}

static uint8_t get_dfs_value(struct spi_context *ctx)
{
	switch (SPI_WORD_SIZE_GET(ctx->config->operation)) {
	case 8:
		return 1;
	case 16:
		return 2;
	case 32:
		return 4;
	default:
		return 1;
	}
}

static void transfer_chunk(const struct device *dev)
{
	struct ifx_cat1_spi_data *const data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int ret = 0;
	size_t chunk_len = spi_context_max_continuous_chunk(ctx);

	if (chunk_len == 0) {
		goto exit;
	}

	data->chunk_len = chunk_len;

	cy_rslt_t result = cyhal_spi_transfer_async(
		&data->obj, ctx->tx_buf, spi_context_tx_buf_on(ctx) ? chunk_len : 0, ctx->rx_buf,
		spi_context_rx_buf_on(ctx) ? chunk_len : 0);
	if (result == CY_RSLT_SUCCESS) {
		return;
	}

	ret = -EIO;

exit:
	spi_context_cs_control(ctx, false);
	spi_context_complete(ctx, dev, ret);
}

static void spi_interrupt_callback(void *arg, cyhal_spi_event_t event)
{
	const struct device *dev = (const struct device *)arg;
	struct ifx_cat1_spi_data *const data = dev->data;
	struct spi_context *ctx = &data->ctx;

	if (event & CYHAL_SPI_IRQ_ERROR) {
#if defined(CONFIG_SPI_ASYNC)
		cyhal_spi_abort_async(&data->obj);
#endif
		spi_context_cs_control(ctx, false);
		spi_context_complete(ctx, dev, -EIO);
	}

	if (event & CYHAL_SPI_IRQ_DONE) {
		spi_context_update_tx(ctx, data->dfs_value, data->chunk_len);
		spi_context_update_rx(ctx, data->dfs_value, data->chunk_len);

		transfer_chunk(dev);
	}
}

int spi_config(const struct device *dev, const struct spi_config *spi_cfg)
{
	cy_rslt_t result;
	struct ifx_cat1_spi_data *const data = dev->data;
	const struct ifx_cat1_spi_config *const config = dev->config;
	cy_stc_scb_spi_config_t scb_spi_config = config->scb_spi_config;
	struct spi_context *ctx = &data->ctx;
	bool spi_mode_cpol = false;
	bool spi_mode_cpha = false;

	/* check if configuration was changed from previous run, if so skip setup again */
	if (spi_context_configured(ctx, spi_cfg)) {
		/* Already configured. No need to do it again. */
		return 0;
	}

	if (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_LOOP) {
		return -ENOTSUP;
	}

	if (SPI_WORD_SIZE_GET(spi_cfg->operation) > IFX_CAT1_SPI_MAX_DATA_WIDTH) {
		LOG_ERR("Word size %d is greater than %d", SPI_WORD_SIZE_GET(spi_cfg->operation),
			IFX_CAT1_SPI_MAX_DATA_WIDTH);
		return -EINVAL;
	}

	if (SPI_WORD_SIZE_GET(spi_cfg->operation) < IFX_CAT1_SPI_MIN_DATA_WIDTH) {
		LOG_ERR("Word size %d is less than %d", SPI_WORD_SIZE_GET(spi_cfg->operation),
			IFX_CAT1_SPI_MIN_DATA_WIDTH);
		return -EINVAL;
	}

	if (SPI_OP_MODE_GET(spi_cfg->operation) == SPI_OP_MODE_SLAVE) {
		scb_spi_config.spiMode = CY_SCB_SPI_SLAVE;
		scb_spi_config.oversample = 0;
		scb_spi_config.enableMisoLateSample = false;
	} else {
		scb_spi_config.spiMode = CY_SCB_SPI_MASTER;
	}

	if (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPOL) {
		spi_mode_cpol = true;
	}

	if (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPHA) {
		spi_mode_cpha = true;
	}

	if (SPI_WORD_SIZE_GET(spi_cfg->operation)) {
		scb_spi_config.txDataWidth = SPI_WORD_SIZE_GET(spi_cfg->operation);
		scb_spi_config.rxDataWidth = SPI_WORD_SIZE_GET(spi_cfg->operation);
	}

	if (spi_mode_cpha) {
		scb_spi_config.sclkMode =
			spi_mode_cpol ? CY_SCB_SPI_CPHA1_CPOL1 : CY_SCB_SPI_CPHA1_CPOL0;
	} else {
		scb_spi_config.sclkMode =
			spi_mode_cpol ? CY_SCB_SPI_CPHA0_CPOL1 : CY_SCB_SPI_CPHA0_CPOL0;
	}

	scb_spi_config.enableMsbFirst = (spi_cfg->operation & SPI_TRANSFER_LSB) ? false : true;

	/* Force free resource */
	if (data->obj.base != NULL) {
		cyhal_spi_free(&data->obj);
	}

	/* Initialize the SPI peripheral */
	cyhal_spi_configurator_t spi_init_cfg = {.resource = &data->hw_resource,
						 .config = &scb_spi_config,
						 .gpios = {NC, {NC, NC, NC, NC}, NC, NC}};

	result = cyhal_spi_init_cfg(&data->obj, &spi_init_cfg);
	if (result != CY_RSLT_SUCCESS) {
		return -ENOTSUP;
	}

	/* Assigns a programmable divider to a selected IP block */
	en_clk_dst_t clk_idx = _cyhal_scb_get_clock_index(spi_init_cfg.resource->block_num);

	result = _cyhal_utils_peri_pclk_assign_divider(clk_idx, &data->obj.clock);
	if (result != CY_RSLT_SUCCESS) {
		return -ENOTSUP;
	}

	/* Configure Slave select polarity */
	if (SPI_OP_MODE_GET(spi_cfg->operation) == SPI_OP_MODE_SLAVE) {
		Cy_SCB_SPI_SetActiveSlaveSelectPolarity(data->obj.base, CY_SCB_SPI_SLAVE_SELECT0,
							scb_spi_config.ssPolarity);
	}

	/* Set the data rate */
	result = cyhal_spi_set_frequency(&data->obj, spi_cfg->frequency);
	if (result != CY_RSLT_SUCCESS) {
		return -EIO;
	}

	/* Write 0 when NULL buffer is provided for Tx/Rx */
	data->obj.write_fill = 0;

	/* Register common SPI callback */
	cyhal_spi_register_callback(&data->obj, spi_interrupt_callback, (void *)dev);
	cyhal_spi_enable_event(&data->obj, CYHAL_SPI_IRQ_DONE, config->irq_priority, true);

	/* Store spi config in context */
	ctx->config = spi_cfg;

	data->dfs_value = get_dfs_value(ctx);

	return 0;
}

static int transceive(const struct device *dev, const struct spi_config *spi_cfg,
		      const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs,
		      bool asynchronous, spi_callback_t cb, void *userdata)
{
	int result;
	struct ifx_cat1_spi_data *const data = dev->data;
	struct spi_context *ctx = &data->ctx;

	spi_context_lock(ctx, asynchronous, cb, userdata, spi_cfg);

	result = spi_config(dev, spi_cfg);
	if (result) {
		LOG_ERR("Error in SPI Configuration (result: 0x%x)", result);
		return result;
	}

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, data->dfs_value);

	spi_context_cs_control(ctx, true);

	transfer_chunk(dev);

	result = spi_context_wait_for_completion(&data->ctx);

	spi_context_release(ctx, result);

	return result;
}

static int ifx_cat1_spi_transceive_sync(const struct device *dev, const struct spi_config *spi_cfg,
					const struct spi_buf_set *tx_bufs,
					const struct spi_buf_set *rx_bufs)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, false, NULL, NULL);
}

#if defined(CONFIG_SPI_ASYNC)
static int ifx_cat1_spi_transceive_async(const struct device *dev, const struct spi_config *spi_cfg,
					 const struct spi_buf_set *tx_bufs,
					 const struct spi_buf_set *rx_bufs, spi_callback_t cb,
					 void *userdata)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, true, cb, userdata);
}
#endif

static int ifx_cat1_spi_release(const struct device *dev, const struct spi_config *spi_cfg)
{
	struct ifx_cat1_spi_data *const data = dev->data;

	cyhal_spi_free(&data->obj);

	return 0;
}

static const struct spi_driver_api ifx_cat1_spi_api = {
	.transceive = ifx_cat1_spi_transceive_sync,
#if defined(CONFIG_SPI_ASYNC)
	.transceive_async = ifx_cat1_spi_transceive_async,
#endif
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
	.release = ifx_cat1_spi_release,
};

static int ifx_cat1_spi_init(const struct device *dev)
{
	struct ifx_cat1_spi_data *const data = dev->data;
	const struct ifx_cat1_spi_config *const config = dev->config;
	int ret;

	/* Dedicate SCB HW resource */
	data->hw_resource.type = CYHAL_RSC_SCB;
	data->hw_resource.block_num = get_hw_block_num(config->reg_addr);

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/* Configure slave select (master) */
	spi_context_cs_configure_all(&data->ctx);

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#define IFX_CAT1_SPI_INIT(n)                                                                       \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static struct ifx_cat1_spi_data spi_cat1_data_##n = {                                      \
		SPI_CONTEXT_INIT_LOCK(spi_cat1_data_##n, ctx),                                     \
		SPI_CONTEXT_INIT_SYNC(spi_cat1_data_##n, ctx),                                     \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)};                             \
	static struct ifx_cat1_spi_config spi_cat1_config_##n = {                                  \
		.reg_addr = (CySCB_Type *)DT_INST_REG_ADDR(n),                                     \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.scb_spi_config =                                                                  \
			{.spiMode = CY_SCB_SPI_MASTER,       /* overwrite by cfg  */               \
			 .sclkMode = CY_SCB_SPI_CPHA0_CPOL0, /* overwrite by cfg  */               \
			 .rxDataWidth = 8,                   /* overwrite by cfg  */               \
			 .txDataWidth = 8,                   /* overwrite by cfg  */               \
			 .enableMsbFirst = true,             /* overwrite by cfg  */               \
			 .subMode = CY_SCB_SPI_MOTOROLA,                                           \
			 .oversample = IFX_CAT1_SPI_DEFAULT_OVERSAMPLE,                            \
			 .enableMisoLateSample = true,                                             \
			 .ssPolarity = CY_SCB_SPI_ACTIVE_LOW,                                      \
		},                                                                                 \
		.irq_priority = DT_INST_IRQ(n, priority),                                          \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, ifx_cat1_spi_init, NULL, &spi_cat1_data_##n,                      \
			      &spi_cat1_config_##n, POST_KERNEL,                                   \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &ifx_cat1_spi_api);

DT_INST_FOREACH_STATUS_OKAY(IFX_CAT1_SPI_INIT)
