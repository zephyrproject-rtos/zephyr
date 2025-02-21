/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcx_spip

#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi/rtio.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_npcx_spip, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"

/* Transfer this NOP value when tx buf is null */
#define SPI_NPCX_SPIP_TX_NOP                 0x00
#define SPI_NPCX_SPIP_WAIT_STATUS_TIMEOUT_US 1000

/* The max allowed prescaler divider */
#define SPI_NPCX_MAX_PRESCALER_DIV INT8_MAX

struct spi_npcx_spip_data {
	struct spi_context ctx;
	uint32_t src_clock_freq;
	uint8_t bytes_per_frame;
};

struct spi_npcx_spip_cfg {
	struct spip_reg *reg_base;
	struct npcx_clk_cfg clk_cfg;
#ifdef CONFIG_SPI_NPCX_SPIP_INTERRUPT
	/* routine for configuring SPIP ISR */
	void (*irq_cfg_func)(const struct device *dev);
#endif
	const struct pinctrl_dev_config *pcfg;
};

static int spi_npcx_spip_configure(const struct device *dev, const struct spi_config *spi_cfg)
{
	uint8_t prescaler_divider;
	const struct spi_npcx_spip_cfg *const config = dev->config;
	struct spi_npcx_spip_data *const data = dev->data;
	struct spip_reg *const reg_base = config->reg_base;
	spi_operation_t operation = spi_cfg->operation;
	uint8_t frame_size;

	if (spi_context_configured(&data->ctx, spi_cfg)) {
		/* This configuration is already in use */
		return 0;
	}

	if (operation & SPI_HALF_DUPLEX) {
		LOG_ERR("Half duplex mode is not supported");
		return -ENOTSUP;
	}

	if (SPI_OP_MODE_GET(operation) != SPI_OP_MODE_MASTER) {
		LOG_ERR("Only SPI controller mode is supported");
		return -ENOTSUP;
	}

	if (operation & SPI_MODE_LOOP) {
		LOG_ERR("Loopback mode is not supported");
		return -ENOTSUP;
	}

	/*
	 * If the GPIO CS configuration is not present, return error because the hardware CS is
	 * not supported.
	 */
	if (!spi_cs_is_gpio(spi_cfg)) {
		LOG_ERR("Only GPIO CS is supported");
		return -ENOTSUP;
	}

	/* Get the frame length */
	frame_size = SPI_WORD_SIZE_GET(operation);
	if (frame_size == 8) {
		data->bytes_per_frame = 1;
		reg_base->SPIP_CTL1 &= ~BIT(NPCX_SPIP_CTL1_MOD);
	} else if (frame_size == 16) {
		reg_base->SPIP_CTL1 |= BIT(NPCX_SPIP_CTL1_MOD);
		data->bytes_per_frame = 2;
	} else {
		LOG_ERR("Only support word sizes either 8 or 16 bits");
		return -ENOTSUP;
	}

	if (IS_ENABLED(CONFIG_SPI_EXTENDED_MODES) &&
	    (operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
		LOG_ERR("Only single line mode is supported");
		return -ENOTSUP;
	}

	/* Set the endianness */
	if (operation & SPI_TRANSFER_LSB) {
		LOG_ERR("Shift out with LSB is not supported");
		return -ENOTSUP;
	}

	/*
	 * Set CPOL and CPHA.
	 * The following is how to map npcx spip control register to CPOL and CPHA
	 *   CPOL    CPHA  |  SCIDL    SCM
	 *   -----------------------------
	 *    0       0    |    0       0
	 *    0       1    |    0       1
	 *    1       0    |    1       1
	 *    1       1    |    1       0
	 */
	if (operation & SPI_MODE_CPOL) {
		reg_base->SPIP_CTL1 |= BIT(NPCX_SPIP_CTL1_SCIDL);
	} else {
		reg_base->SPIP_CTL1 &= ~BIT(NPCX_SPIP_CTL1_SCIDL);
	}
	if (((operation & SPI_MODE_CPOL) == SPI_MODE_CPOL) !=
	    ((operation & SPI_MODE_CPHA) == SPI_MODE_CPHA)) {
		reg_base->SPIP_CTL1 |= BIT(NPCX_SPIP_CTL1_SCM);
	} else {
		reg_base->SPIP_CTL1 &= ~BIT(NPCX_SPIP_CTL1_SCM);
	}

	/* Set the SPI frequency */
	prescaler_divider = data->src_clock_freq / 2 / spi_cfg->frequency;
	if (prescaler_divider >= 1) {
		prescaler_divider -= 1;
	}
	if (prescaler_divider >= SPI_NPCX_MAX_PRESCALER_DIV) {
		LOG_ERR("SPI divider %d exceeds the max allowed value %d.", prescaler_divider,
			SPI_NPCX_MAX_PRESCALER_DIV);
		return -ENOTSUP;
	}
	SET_FIELD(reg_base->SPIP_CTL1, NPCX_SPIP_CTL1_SCDV, prescaler_divider);

	data->ctx.config = spi_cfg;

	return 0;
}

static void spi_npcx_spip_process_tx_buf(struct spi_npcx_spip_data *const data, uint16_t *tx_frame)
{
	/* Get the tx_frame from tx_buf only when tx_buf != NULL */
	if (spi_context_tx_buf_on(&data->ctx)) {
		if (data->bytes_per_frame == 1) {
			*tx_frame = UNALIGNED_GET((uint8_t *)(data->ctx.tx_buf));
		} else {
			*tx_frame = UNALIGNED_GET((uint16_t *)(data->ctx.tx_buf));
		}
	}
	/*
	 * The update is ignored if TX is off (tx_len == 0).
	 * Note: if tx_buf == NULL && tx_len != 0, the update still counts.
	 */
	spi_context_update_tx(&data->ctx, data->bytes_per_frame, 1);
}

static void spi_npcx_spip_process_rx_buf(struct spi_npcx_spip_data *const data, uint16_t rx_frame)
{
	if (spi_context_rx_buf_on(&data->ctx)) {
		if (data->bytes_per_frame == 1) {
			UNALIGNED_PUT(rx_frame, (uint8_t *)data->ctx.rx_buf);
		} else {
			UNALIGNED_PUT(rx_frame, (uint16_t *)data->ctx.rx_buf);
		}
	}
	spi_context_update_rx(&data->ctx, data->bytes_per_frame, 1);
}

#ifndef CONFIG_SPI_NPCX_SPIP_INTERRUPT
static int spi_npcx_spip_xfer_frame(const struct device *dev)
{
	const struct spi_npcx_spip_cfg *const config = dev->config;
	struct spip_reg *const reg_base = config->reg_base;
	struct spi_npcx_spip_data *const data = dev->data;
	uint16_t tx_frame = SPI_NPCX_SPIP_TX_NOP;
	uint16_t rx_frame;

	spi_npcx_spip_process_tx_buf(data, &tx_frame);

	if (WAIT_FOR(!IS_BIT_SET(reg_base->SPIP_STAT, NPCX_SPIP_STAT_BSY),
		     SPI_NPCX_SPIP_WAIT_STATUS_TIMEOUT_US, NULL) == false) {
		LOG_ERR("Check Status BSY Timeout");
		return -ETIMEDOUT;
	}

	reg_base->SPIP_DATA = tx_frame;

	if (WAIT_FOR(IS_BIT_SET(reg_base->SPIP_STAT, NPCX_SPIP_STAT_RBF),
		     SPI_NPCX_SPIP_WAIT_STATUS_TIMEOUT_US, NULL) == false) {
		LOG_ERR("Check Status RBF Timeout");
		return -ETIMEDOUT;
	}

	rx_frame = reg_base->SPIP_DATA;
	spi_npcx_spip_process_rx_buf(data, rx_frame);

	return 0;
}
#endif

static bool spi_npcx_spip_transfer_ongoing(struct spi_npcx_spip_data *data)
{
	return spi_context_tx_on(&data->ctx) || spi_context_rx_on(&data->ctx);
}

#ifdef CONFIG_SPI_NPCX_SPIP_INTERRUPT
static void spi_npcx_spip_isr(const struct device *dev)
{
	const struct spi_npcx_spip_cfg *const config = dev->config;
	struct spip_reg *const reg_base = config->reg_base;
	struct spi_npcx_spip_data *const data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint16_t tx_frame = SPI_NPCX_SPIP_TX_NOP;
	uint16_t rx_frame;
	uint8_t status;

	status = reg_base->SPIP_STAT;

	if (!IS_BIT_SET(status, NPCX_SPIP_STAT_BSY) && !IS_BIT_SET(status, NPCX_SPIP_STAT_RBF)) {
		reg_base->SPIP_CTL1 &= ~BIT(NPCX_SPIP_CTL1_EIW);

		spi_npcx_spip_process_tx_buf(data, &tx_frame);
		reg_base->SPIP_DATA = tx_frame;
	} else if (IS_BIT_SET(status, NPCX_SPIP_STAT_RBF)) {
		rx_frame = reg_base->SPIP_DATA;

		spi_npcx_spip_process_rx_buf(data, rx_frame);

		if (!spi_npcx_spip_transfer_ongoing(data)) {
			reg_base->SPIP_CTL1 &= ~BIT(NPCX_SPIP_CTL1_EIR);
			/*
			 * The CS might not de-assert if SPI_HOLD_ON_CS is configured.
			 * In this case, CS de-assertion reles on the caller to explicitly call
			 * spi_release() API.
			 */
			spi_context_cs_control(ctx, false);

			spi_context_complete(ctx, dev, 0);

		} else {
			spi_npcx_spip_process_tx_buf(data, &tx_frame);
			reg_base->SPIP_DATA = tx_frame;
		}
	}
}
#endif

static int transceive(const struct device *dev, const struct spi_config *spi_cfg,
		      const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs,
		      bool asynchronous, spi_callback_t cb, void *userdata)
{
	const struct spi_npcx_spip_cfg *const config = dev->config;
	struct spip_reg *const reg_base = config->reg_base;
	struct spi_npcx_spip_data *const data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int rc;

	if (!tx_bufs && !rx_bufs) {
		return 0;
	}

#ifndef CONFIG_SPI_NPCX_SPIP_INTERRUPT
	if (asynchronous) {
		return -ENOTSUP;
	}
#endif

	/* Lock the SPI Context */
	spi_context_lock(ctx, asynchronous, cb, userdata, spi_cfg);

	rc = spi_npcx_spip_configure(dev, spi_cfg);
	if (rc < 0) {
		spi_context_release(ctx, rc);
		return rc;
	}

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, data->bytes_per_frame);
	if (!spi_npcx_spip_transfer_ongoing(data)) {
		spi_context_release(ctx, 0);
		return 0;
	}

	/* Enable SPIP module */
	reg_base->SPIP_CTL1 |= BIT(NPCX_SPIP_CTL1_SPIEN);

	/* Cleaning junk data in the buffer */
	while (IS_BIT_SET(reg_base->SPIP_STAT, NPCX_SPIP_STAT_RBF)) {
		uint8_t unused __attribute__((unused));

		unused = reg_base->SPIP_DATA;
	}

	/* Assert the CS line */
	spi_context_cs_control(ctx, true);

#ifdef CONFIG_SPI_NPCX_SPIP_INTERRUPT
	reg_base->SPIP_CTL1 |= BIT(NPCX_SPIP_CTL1_EIR) | BIT(NPCX_SPIP_CTL1_EIW);
	rc = spi_context_wait_for_completion(&data->ctx);
#else
	do {
		rc = spi_npcx_spip_xfer_frame(dev);
		if (rc < 0) {
			break;
		}
	} while (spi_npcx_spip_transfer_ongoing(data));

	/*
	 * The CS might not de-assert if SPI_HOLD_ON_CS is configured.
	 * In this case, CS de-assertion reles on the caller to explicitly call spi_release() API.
	 */
	spi_context_cs_control(ctx, false);

#endif
	spi_context_release(ctx, rc);

	return rc;
}

static int spi_npcx_spip_transceive(const struct device *dev, const struct spi_config *spi_cfg,
				    const struct spi_buf_set *tx_bufs,
				    const struct spi_buf_set *rx_bufs)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, false, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int spi_npcx_spip_transceive_async(const struct device *dev,
					  const struct spi_config *spi_cfg,
					  const struct spi_buf_set *tx_bufs,
					  const struct spi_buf_set *rx_bufs, spi_callback_t cb,
					  void *userdata)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, true, cb, userdata);
}
#endif

static int spi_npcx_spip_release(const struct device *dev, const struct spi_config *spi_cfg)
{
	struct spi_npcx_spip_data *const data = dev->data;
	struct spi_context *ctx = &data->ctx;

	if (!spi_context_configured(ctx, spi_cfg)) {
		return -EINVAL;
	}

	spi_context_unlock_unconditionally(ctx);

	return 0;
}

static int spi_npcx_spip_init(const struct device *dev)
{
	int ret;
	struct spi_npcx_spip_data *const data = dev->data;
	const struct spi_npcx_spip_cfg *const config = dev->config;
	struct spip_reg *const reg_base = config->reg_base;
	const struct device *const clk_dev = DEVICE_DT_GET(NPCX_CLK_CTRL_NODE);

	if (!device_is_ready(clk_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	ret = clock_control_on(clk_dev, (clock_control_subsys_t)&config->clk_cfg);
	if (ret < 0) {
		LOG_ERR("Turn on SPIP clock fail %d", ret);
		return ret;
	}

	ret = clock_control_get_rate(clk_dev, (clock_control_subsys_t)&config->clk_cfg,
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

#ifdef CONFIG_SPI_NPCX_SPIP_INTERRUPT
	config->irq_cfg_func(dev);
#endif

	/* Enable SPIP module */
	reg_base->SPIP_CTL1 |= BIT(NPCX_SPIP_CTL1_SPIEN);

	return 0;
}

static DEVICE_API(spi, spi_npcx_spip_api) = {
	.transceive = spi_npcx_spip_transceive,
	.release = spi_npcx_spip_release,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_npcx_spip_transceive_async,
#endif
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
};

#ifdef CONFIG_SPI_NPCX_SPIP_INTERRUPT
#define NPCX_SPIP_IRQ_HANDLER(n)                                                                   \
	static void spi_npcx_spip_irq_cfg_func_##n(const struct device *dev)                       \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), spi_npcx_spip_isr,          \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

#define NPCX_SPIP_IRQ_HANDLER_FUNC(n) .irq_cfg_func = spi_npcx_spip_irq_cfg_func_##n,
#else
#define NPCX_SPIP_IRQ_HANDLER_FUNC(n)
#define NPCX_SPIP_IRQ_HANDLER(n)
#endif

#define NPCX_SPI_INIT(n)                                                                           \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	NPCX_SPIP_IRQ_HANDLER(n)                                                                   \
                                                                                                   \
	static struct spi_npcx_spip_data spi_npcx_spip_data_##n = {                                \
		SPI_CONTEXT_INIT_LOCK(spi_npcx_spip_data_##n, ctx),                                \
		SPI_CONTEXT_INIT_SYNC(spi_npcx_spip_data_##n, ctx),                                \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)};                             \
                                                                                                   \
	static struct spi_npcx_spip_cfg spi_npcx_spip_cfg_##n = {                                  \
		.reg_base = (struct spip_reg *)DT_INST_REG_ADDR(n),                                \
		.clk_cfg = NPCX_DT_CLK_CFG_ITEM(n),                                                \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		NPCX_SPIP_IRQ_HANDLER_FUNC(n)};                                                    \
                                                                                                   \
	SPI_DEVICE_DT_INST_DEFINE(n, spi_npcx_spip_init, NULL, &spi_npcx_spip_data_##n,            \
			      &spi_npcx_spip_cfg_##n, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,       \
			      &spi_npcx_spip_api);

DT_INST_FOREACH_STATUS_OKAY(NPCX_SPI_INIT)
