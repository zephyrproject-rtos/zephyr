/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcm_spip

#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_npcm_spip, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"

/* Transfer this NOP value when tx buf is null */
#define SPI_NPCM_SPIP_TX_NOP                 0x00
#define SPI_NPCM_SPIP_WAIT_STATUS_TIMEOUT_US 1000

#define SPI_NPCM_SPIP_SINGLE                 0x0
#define SPI_NPCM_SPIP_DUAL                   0x1
#define SPI_NPCM_SPIP_QUAD                   0x2

/* The max allowed prescaler divider */
#define SPI_NPCM_MAX_PRESCALER_DIV 1023

struct spi_npcm_spip_data {
	struct spi_context ctx;
	uint32_t src_clock_freq;
	uint8_t bytes_per_frame;
	uint8_t access_mode;
};

struct spi_npcm_spip_cfg {
	struct spip_reg *reg_base;
	uint32_t clk_cfg;
	const struct pinctrl_dev_config *pcfg;
};

static int spi_npcm_spip_configure(const struct device *dev, const struct spi_config *spi_cfg)
{
	uint8_t prescaler_divider;
	const struct spi_npcm_spip_cfg *const config = dev->config;
	struct spi_npcm_spip_data *const data = dev->data;
	struct spip_reg *const reg_base = config->reg_base;
	spi_operation_t operation = spi_cfg->operation;
	uint8_t frame_size;

	if (spi_context_configured(&data->ctx, spi_cfg)) {
		/* This configuration is already in use */
		return 0;
	}

	if (operation & SPI_FULL_DUPLEX) {
		if ((operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
			LOG_ERR("Full duplex mode only support for single line");
			return -ENOTSUP;
		}
	}

	if (SPI_OP_MODE_GET(operation) != SPI_OP_MODE_MASTER) {
		LOG_ERR("Only SPI controller mode is supported");
		return -ENOTSUP;
	}

	if (operation & SPI_MODE_LOOP) {
		LOG_ERR("Loopback mode is not supported");
		return -ENOTSUP;
	}

	/* Get the frame length */
	frame_size = SPI_WORD_SIZE_GET(operation);

	switch (frame_size) {
		case 8:
			data->bytes_per_frame = 1;
			break;
		case 16:
			data->bytes_per_frame = 2;
			break;
		case 24:
			data->bytes_per_frame = 3;
			break;
		case 32:
			data->bytes_per_frame = 4;
			break;
		default:
			LOG_ERR("Only support word sizes 8/16/32 bits");
			return -ENOTSUP;
	}

	if (frame_size == 32) {
		SET_FIELD(reg_base->SPIP_CTL, NPCM_CTL_DWIDTH, 0x0);
	} else {
		SET_FIELD(reg_base->SPIP_CTL, NPCM_CTL_DWIDTH,
				SPI_WORD_SIZE_GET(operation));
	}

	switch (operation & SPI_LINES_MASK) {
		case SPI_LINES_SINGLE:
			reg_base->SPIP_CTL &= ~BIT(NPCM_CTL_DUALIOEN);
			reg_base->SPIP_CTL &= ~BIT(NPCM_CTL_QUADIOEN);
			data->access_mode = SPI_NPCM_SPIP_SINGLE;
			break;
		case SPI_LINES_DUAL:
			reg_base->SPIP_CTL |= BIT(NPCM_CTL_DUALIOEN);
			reg_base->SPIP_CTL &= ~BIT(NPCM_CTL_QUADIOEN);
			data->access_mode = SPI_NPCM_SPIP_DUAL;
			break;
		case SPI_LINES_QUAD:
			reg_base->SPIP_CTL &= ~BIT(NPCM_CTL_DUALIOEN);
			reg_base->SPIP_CTL |= BIT(NPCM_CTL_QUADIOEN);
			data->access_mode = SPI_NPCM_SPIP_QUAD;
			break;
		default:
			LOG_ERR("Only single/dual/quad line mode is supported");
			return -ENOTSUP;
	}

	/* Set the endianness */
	if (operation & SPI_TRANSFER_LSB) {
		reg_base->SPIP_CTL |= BIT(NPCM_CTL_LSB);
	} else {
		reg_base->SPIP_CTL &= ~BIT(NPCM_CTL_LSB);
	}

	/*
	 * Set CPOL and CPHA.
	 * The following is how to map npcm spip control register to CPOL and CPHA
	 *   CPOL    CPHA  |  CLKPOL  TXNEG   RXNEG
	 *   --------------------------------------
	 *    0       0    |    0       1       0
	 *    0       1    |    0       0       1
	 *    1       0    |    1       0       1
	 *    1       1    |    1       1       0
	 */
	if (operation & SPI_MODE_CPOL) {
		reg_base->SPIP_CTL |= BIT(NPCM_CTL_CLKPOL);
		if (operation & SPI_MODE_CPHA) {
			reg_base->SPIP_CTL |= BIT(NPCM_CTL_TXNEG);
			reg_base->SPIP_CTL &= ~BIT(NPCM_CTL_RXNEG);
		} else {
			reg_base->SPIP_CTL &= ~BIT(NPCM_CTL_TXNEG);
			reg_base->SPIP_CTL |= BIT(NPCM_CTL_RXNEG);
		}
	} else {
		reg_base->SPIP_CTL &= ~BIT(NPCM_CTL_CLKPOL);
		if (operation & SPI_MODE_CPHA) {
			reg_base->SPIP_CTL &= ~BIT(NPCM_CTL_TXNEG);
			reg_base->SPIP_CTL |= BIT(NPCM_CTL_RXNEG);
		} else {
			reg_base->SPIP_CTL |= BIT(NPCM_CTL_TXNEG);
			reg_base->SPIP_CTL &= ~BIT(NPCM_CTL_RXNEG);
		}
	}

	/* Active high CS logic */
	if (operation & SPI_CS_ACTIVE_HIGH) {
		reg_base->SPIP_SSCTL |= BIT(NPCM_SSCTL_SSACTPOL);
	} else {
		reg_base->SPIP_SSCTL &= ~BIT(NPCM_SSCTL_SSACTPOL);
	}

	/* Disable AUTOSS */
	reg_base->SPIP_SSCTL &= ~BIT(NPCM_SSCTL_AUTOSS);

	/* Set the SPI frequency */
	prescaler_divider = data->src_clock_freq / spi_cfg->frequency;
	if (prescaler_divider >= 1) {
		prescaler_divider -= 1;
	}
	if (prescaler_divider >= SPI_NPCM_MAX_PRESCALER_DIV) {
		LOG_ERR("SPI divider %d exceeds the max allowed value %d.", prescaler_divider,
			SPI_NPCM_MAX_PRESCALER_DIV);
		return -ENOTSUP;
	}
	SET_FIELD(reg_base->SPIP_CLKDIV, NPCM_CLKDIV_DIVIDER, prescaler_divider);

	data->ctx.config = spi_cfg;

	return 0;
}

static void spi_npcm_spip_cs_control(const struct device *dev, bool on)
{
	const struct spi_npcm_spip_cfg *const config = dev->config;
	struct spip_reg *const reg_base = config->reg_base;

	if (on) {
		reg_base->SPIP_SSCTL |= BIT(NPCM_SSCTL_SS);
	} else {
		reg_base->SPIP_SSCTL &= ~BIT(NPCM_SSCTL_SS);
	}
}

static int spi_npcm_spip_process_tx_buf(struct spi_npcm_spip_data *const data, uint32_t *tx_frame)
{
	int ret = 0;

	/* Get the tx_frame from tx_buf only when tx_buf != NULL */
	if (spi_context_tx_buf_on(&data->ctx)) {
		*tx_frame = UNALIGNED_GET((uint8_t *)(data->ctx.tx_buf));
	} else {
		ret = -ENOBUFS;
	}

	/*
	 * The update is ignored if TX is off (tx_len == 0).
	 * Note: if tx_buf == NULL && tx_len != 0, the update still counts.
	 */
	spi_context_update_tx(&data->ctx, data->bytes_per_frame, 1);

	return ret;
}

static void spi_npcm_spip_process_rx_buf(struct spi_npcm_spip_data *const data, uint32_t rx_frame)
{
	if (spi_context_rx_buf_on(&data->ctx)) {
		UNALIGNED_PUT(rx_frame, (uint8_t *)data->ctx.rx_buf);
	}

	spi_context_update_rx(&data->ctx, data->bytes_per_frame, 1);
}

static int spi_npcm_spip_xfer_frame(const struct device *dev)
{
	const struct spi_npcm_spip_cfg *const config = dev->config;
	struct spip_reg *const reg_base = config->reg_base;
	struct spi_npcm_spip_data *const data = dev->data;
	uint32_t tx_frame = SPI_NPCM_SPIP_TX_NOP;
	uint32_t rx_frame;
	int ret;

	ret = spi_npcm_spip_process_tx_buf(data, &tx_frame);

	if (WAIT_FOR(!IS_BIT_SET(reg_base->SPIP_STATUS, NPCM_STATUS_BUSY),
		     SPI_NPCM_SPIP_WAIT_STATUS_TIMEOUT_US, NULL) == false) {
		LOG_ERR("Check Status BSY Timeout");
		return -ETIMEDOUT;
	}

	if (data->access_mode != SPI_NPCM_SPIP_SINGLE) {
		if (ret == -ENOBUFS) {
			/* Input mode */
			reg_base->SPIP_CTL &= ~BIT(NPCM_CTL_QDIODIR);
		} else {
			/* Output mode */
			reg_base->SPIP_CTL |= BIT(NPCM_CTL_QDIODIR);
		}
	}

	reg_base->SPIP_TX = tx_frame;

	if (WAIT_FOR(!IS_BIT_SET(reg_base->SPIP_STATUS, NPCM_STATUS_RXEMPTY),
		     SPI_NPCM_SPIP_WAIT_STATUS_TIMEOUT_US, NULL) == false) {
		LOG_ERR("Check Status RBF Timeout");
		return -ETIMEDOUT;
	}

	rx_frame = reg_base->SPIP_RX;
	spi_npcm_spip_process_rx_buf(data, rx_frame);

	return 0;
}

static bool spi_npcm_spip_transfer_ongoing(struct spi_npcm_spip_data *data)
{
	return spi_context_tx_on(&data->ctx) || spi_context_rx_on(&data->ctx);
}

static int spi_npcm_spip_transceive(const struct device *dev, const struct spi_config *spi_cfg,
				    const struct spi_buf_set *tx_bufs,
				    const struct spi_buf_set *rx_bufs)
{
	const struct spi_npcm_spip_cfg *const config = dev->config;
	struct spip_reg *const reg_base = config->reg_base;
	struct spi_npcm_spip_data *const data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int rc;

	if (!tx_bufs && !rx_bufs) {
		return 0;
	}

	/* Lock the SPI Context */
	spi_context_lock(ctx, false, NULL, NULL, spi_cfg);

	rc = spi_npcm_spip_configure(dev, spi_cfg);
	if (rc < 0) {
		spi_context_release(ctx, rc);
		return rc;
	}

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, data->bytes_per_frame);
	if (!spi_npcm_spip_transfer_ongoing(data)) {
		spi_context_release(ctx, 0);
		return 0;
	}

	/* Cleaning junk data in the buffer */
	while (!IS_BIT_SET(reg_base->SPIP_STATUS, NPCM_STATUS_RXEMPTY)) {
		uint8_t unused __attribute__((unused));

		unused = reg_base->SPIP_RX;
	}

	/* Enable SPIP module */
	reg_base->SPIP_CTL |= BIT(NPCM_CTL_SPIEN);

	/* if cs is defined: software cs control, set active true */
	if (spi_cs_is_gpio(spi_cfg)) {
		spi_context_cs_control(ctx, true);
	} else {
		spi_npcm_spip_cs_control(dev, true);
	}

	do {
		rc = spi_npcm_spip_xfer_frame(dev);
		if (rc < 0) {
			break;
		}
	} while (spi_npcm_spip_transfer_ongoing(data));

	if (!(spi_cfg->operation & SPI_HOLD_ON_CS)) {
		/* if cs is defined: software cs control, set active false */
		if (spi_cs_is_gpio(spi_cfg)) {
			spi_context_cs_control(ctx, false);
		} else {
			spi_npcm_spip_cs_control(dev, false);
		}
	}

	/* Disable SPIP module */
	reg_base->SPIP_CTL &= ~BIT(NPCM_CTL_SPIEN);

	spi_context_release(ctx, rc);

	return rc;
}

static int spi_npcm_spip_release(const struct device *dev, const struct spi_config *spi_cfg)
{
	struct spi_npcm_spip_data *const data = dev->data;
	struct spi_context *ctx = &data->ctx;

	if (!spi_context_configured(ctx, spi_cfg)) {
		return -EINVAL;
	}

	spi_context_unlock_unconditionally(ctx);

	return 0;
}

static int spi_npcm_spip_init(const struct device *dev)
{
	int ret;
	struct spi_npcm_spip_data *const data = dev->data;
	const struct spi_npcm_spip_cfg *const config = dev->config;
	const struct device *const clk_dev = DEVICE_DT_GET(DT_NODELABEL(pcc));

	if (!device_is_ready(clk_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	ret = clock_control_on(clk_dev, (clock_control_subsys_t)config->clk_cfg);
	if (ret < 0) {
		LOG_ERR("Turn on SPIP clock fail %d", ret);
		return ret;
	}

	ret = clock_control_get_rate(clk_dev, (clock_control_subsys_t)config->clk_cfg,
				     &data->src_clock_freq);
	if (ret < 0) {
		LOG_ERR("Get SPIP clock source rate error %d", ret);
		return ret;
	}

	ret = spi_context_cs_configure_all(&data->ctx);
	if (ret < 0) {
		return ret;
	}

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/* Make sure the context is unlocked */
	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static struct spi_driver_api spi_npcm_spip_api = {
	.transceive = spi_npcm_spip_transceive,
	.release = spi_npcm_spip_release,
};

#define NPCM_SPI_INIT(n)                                                                           \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static struct spi_npcm_spip_data spi_npcm_spip_data_##n = {                                \
		SPI_CONTEXT_INIT_LOCK(spi_npcm_spip_data_##n, ctx),                                \
		SPI_CONTEXT_INIT_SYNC(spi_npcm_spip_data_##n, ctx),                                \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)};                             \
                                                                                                   \
	static struct spi_npcm_spip_cfg spi_npcm_spip_cfg_##n = {                                  \
		.reg_base = (struct spip_reg *)DT_INST_REG_ADDR(n),                                \
		.clk_cfg = DT_INST_PHA(n, clocks, clk_cfg),                                        \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n)};                                        \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, spi_npcm_spip_init, NULL, &spi_npcm_spip_data_##n,                \
			      &spi_npcm_spip_cfg_##n, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,       \
			      &spi_npcm_spip_api);

DT_INST_FOREACH_STATUS_OKAY(NPCM_SPI_INIT)
