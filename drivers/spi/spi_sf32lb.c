/*
 * Copyright (c) 2025, Qingsong Gou <gouqs@hotmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb_spi

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/clock_control/sf32lb.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>

#include <register.h>

LOG_MODULE_REGISTER(spi_sf32lb, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"

#define SPI_TOP_CTRL     offsetof(SPI_TypeDef, TOP_CTRL)
#define SPI_INTE         offsetof(SPI_TypeDef, INTE)
#define SPI_DATA         offsetof(SPI_TypeDef, DATA)
#define SPI_STATUS       offsetof(SPI_TypeDef, STATUS)
#define SPI_CLK_CTRL     offsetof(SPI_TypeDef, CLK_CTRL)
#define SPI_TRIWIRE_CTRL offsetof(SPI_TypeDef, TRIWIRE_CTRL)
#define SPI_FIFO_CTRL    offsetof(SPI_TypeDef, FIFO_CTRL)

#define SPI_FLAG_FRLVL  (SPI_STATUS_RFL_Msk | SPI_STATUS_RNE_Msk)
#define SPI_FRLVL_EMPTY (SPI_STATUS_RFL_Msk)

#define SPI_MAX_BUSY_WAIT_US 1000U

struct spi_sf32lb_config {
	uintptr_t base;
	struct sf32lb_clock_dt_spec clock;
	const struct pinctrl_dev_config *pcfg;
	void (*irq_config_func)(void);
};

struct spi_sf32lb_data {
	struct spi_context ctx;
};

static bool spi_sf32lb_transfer_ongoing(struct spi_sf32lb_data *data)
{
	return spi_context_tx_on(&data->ctx) || spi_context_rx_on(&data->ctx);
}

void spi_sf32lb_complete(const struct device *dev, int status)
{
	const struct spi_sf32lb_config *cfg = dev->config;
	struct spi_sf32lb_data *data = dev->data;

	sys_set_bits(cfg->base + SPI_STATUS, SPI_STATUS_ROR | SPI_STATUS_TUR);

	sys_clear_bits(cfg->base + SPI_INTE, SPI_INTE_RIE | SPI_INTE_TIE);

	spi_context_complete(&data->ctx, dev, status);
}

static void spi_sf32lb_isr(const struct device *dev)
{
	const struct spi_sf32lb_config *cfg = dev->config;
	struct spi_sf32lb_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t status = sys_read32(cfg->base + SPI_STATUS);
	uint16_t tx_frame, rx_frame;
	uint8_t word_size = SPI_WORD_SIZE_GET(ctx->config->operation);
	uint8_t dfs = word_size >> 3;
	uint8_t rne = FIELD_GET(SPI_STATUS_RNE_Msk, status);
	uint8_t tnf = FIELD_GET(SPI_STATUS_TNF_Msk, status);

	uint8_t rfs = FIELD_GET(SPI_STATUS_RFS_Msk, status);
	uint8_t tfs = FIELD_GET(SPI_STATUS_TFS_Msk, status);

	uint8_t tfl = FIELD_GET(SPI_STATUS_TFL_Msk, status);
	uint8_t rfl = FIELD_GET(SPI_STATUS_RFL_Msk, status);
	printk("spi slave: %d tnf=%u, tfs=%u TFL=%u, rne=%u, rfs=%u RFL=%u\n", spi_context_is_slave(ctx), tnf, tfs, tfl, rne, rfs, rfl);
	if (status & (SPI_STATUS_ROR | SPI_STATUS_TUR)) {
		spi_sf32lb_complete(dev, -EIO);
		return;
	}
//printk("tx left=%u, rx left=%u\n", spi_context_tx_len_left(ctx, dfs), spi_context_rx_len_left(ctx, dfs));
	if (IS_BIT_SET(status, SPI_STATUS_RFS_Pos)) {
		rx_frame = sys_read32(cfg->base + SPI_DATA);
		if (spi_context_rx_on(ctx)) {printk("spi slave:%d rx left=%d\n", spi_context_is_slave(ctx), spi_context_rx_len_left(ctx, dfs));
			if (spi_context_rx_buf_on(ctx)) {
				if (word_size == 8) {
					rx_frame &= 0xFF;
					UNALIGNED_PUT(rx_frame, (uint8_t *)data->ctx.rx_buf);
					spi_context_update_rx(ctx, 1, 1);
				} else {
					UNALIGNED_PUT(rx_frame, (uint16_t *)data->ctx.rx_buf);
					spi_context_update_rx(ctx, 2, 1);
				}
			} else {
				if (word_size == 8) {
					/* No RX buffer, read and discard */
					spi_context_update_rx(ctx, 1, 1);
				} else {
					/* No RX buffer, read and discard */
					spi_context_update_rx(ctx, 2, 1);
				}
			}
		}
	}

	if (IS_BIT_SET(status, SPI_STATUS_TNF_Pos)) {
		if (spi_context_tx_on(ctx)) {printk("spi slave:%d tx left=%d\n", spi_context_is_slave(ctx), spi_context_tx_len_left(ctx, dfs));
			if (spi_context_tx_buf_on(ctx)) {
				if (word_size == 8) {
					tx_frame = UNALIGNED_GET((uint8_t *)(data->ctx.tx_buf));
					sys_write8(tx_frame, cfg->base + SPI_DATA);
					spi_context_update_tx(ctx, 1, 1);
				} else {
					tx_frame = UNALIGNED_GET((uint16_t *)(data->ctx.tx_buf));
					sys_write32(tx_frame, cfg->base + SPI_DATA);
					spi_context_update_tx(ctx, 2, 1);
				}
			} else {
				/* No TX buffer, send dummy data */
				if (word_size == 8) {
					sys_write8(0xFF, cfg->base + SPI_DATA);
					spi_context_update_tx(ctx, 1, 1);
				} else {
					sys_write32(0xFFFF, cfg->base + SPI_DATA);
					spi_context_update_tx(ctx, 2, 1);
				}
			}
		} else {
			sys_clear_bit(cfg->base + SPI_INTE, SPI_INTE_TIE_Pos);
		}
	}

	if (!spi_sf32lb_transfer_ongoing(data)) {
		spi_sf32lb_complete(dev, 0);
	}
}

static int spi_sf32lb_configure(const struct device *dev, const struct spi_config *config)
{
	const struct spi_sf32lb_config *cfg = dev->config;
	struct spi_sf32lb_data *data = dev->data;
	int ret;
	uint32_t top_ctrl = 0;
	uint32_t clk_ctrl;
	uint32_t triwire_ctrl = 0;
	uint32_t clk_div;
	uint32_t clk_freq;
	uint8_t word_size;

	if (spi_context_configured(&data->ctx, config)) {
		return 0;
	}

	ret = sf32lb_clock_control_get_rate_dt(&cfg->clock, &clk_freq);
	if (ret < 0) {
		return ret;
	}

	if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_SLAVE) {
		top_ctrl |= SPI_TOP_CTRL_SFRMDIR | SPI_TOP_CTRL_SCLKDIR;
	}
	top_ctrl |= (config->operation & SPI_MODE_CPOL) ? SPI_TOP_CTRL_SPO : 0U;
	top_ctrl |= (config->operation & SPI_MODE_CPHA) ? SPI_TOP_CTRL_SPH : 0U;

	word_size = SPI_WORD_SIZE_GET(config->operation);
	if (word_size == 8U) {
		top_ctrl |= FIELD_PREP(SPI_TOP_CTRL_DSS_Msk, 8U - 1U);
	} else if (word_size == 16U) {
		top_ctrl |= FIELD_PREP(SPI_TOP_CTRL_DSS_Msk, 16U - 1U);
	} else {
		LOG_ERR("Unsupported word size: %u", word_size);
		return -ENOTSUP;
	}

	if (SPI_FRAME_FORMAT_TI == (config->operation & SPI_FRAME_FORMAT_TI)) {
		top_ctrl |= FIELD_PREP(SPI_TOP_CTRL_FRF_Msk, 0x00000001U);
	} else {
		top_ctrl |= FIELD_PREP(SPI_TOP_CTRL_FRF_Msk, 0x00000000U);
	}

	if (SPI_HALF_DUPLEX == (config->operation & SPI_HALF_DUPLEX)) {
		triwire_ctrl |= SPI_TRIWIRE_CTRL_SPI_TRI_WIRE_EN;
		top_ctrl |= SPI_TOP_CTRL_TTE;
	} else {
		triwire_ctrl &= ~SPI_TRIWIRE_CTRL_SPI_TRI_WIRE_EN;
		top_ctrl &= ~SPI_TOP_CTRL_TTE;
	}

	if (config->operation & SPI_HOLD_ON_CS) {
		return -ENOTSUP;
	}

	if (config->operation & SPI_LOCK_ON) {
		return -ENOTSUP;
	}

	sys_clear_bit(cfg->base + SPI_TOP_CTRL, SPI_TOP_CTRL_SSE_Pos);

	sys_write32(top_ctrl, cfg->base + SPI_TOP_CTRL);
	sys_write32(triwire_ctrl, cfg->base + SPI_TRIWIRE_CTRL);

	sys_set_bit(cfg->base + SPI_CLK_CTRL, SPI_CLK_CTRL_CLK_SSP_EN_Pos);

	if (SPI_OP_MODE_GET(config->operation) != SPI_OP_MODE_SLAVE) {
		clk_div = DIV_ROUND_UP(clk_freq,
				config->frequency); /* see Manual 7.2.6.2.4 clock freq settings */
		clk_ctrl = sys_read32(cfg->base + SPI_CLK_CTRL);
		clk_ctrl &= ~SPI_CLK_CTRL_CLK_DIV_Msk;
		clk_ctrl |= FIELD_PREP(SPI_CLK_CTRL_CLK_DIV_Msk, clk_div);
		sys_write32(clk_ctrl, cfg->base + SPI_CLK_CTRL);
	}

	/* For slave mode, set FIFO threshold */
	if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_SLAVE) {
		/* Set FIFO reception threshold to 5 (as in bf0_hal_spi.c) */
		uint32_t fifo_ctrl = sys_read32(cfg->base + SPI_FIFO_CTRL);
		fifo_ctrl &= ~SPI_FIFO_CTRL_TFT_Msk;
		fifo_ctrl |= FIELD_PREP(SPI_FIFO_CTRL_TFT_Msk, 6U);
		//fifo_ctrl |= FIELD_PREP(SPI_FIFO_CTRL_RFT_Msk, 6U);
		sys_write32(fifo_ctrl, cfg->base + SPI_FIFO_CTRL);
	}
	/* Issue 1401: Make SPO setting is valid before start transfer data*/
	sys_set_bit(cfg->base + SPI_TOP_CTRL, SPI_TOP_CTRL_SSE_Pos);
	sys_clear_bit(cfg->base + SPI_TOP_CTRL, SPI_TOP_CTRL_SSE_Pos);

	data->ctx.config = config;

	return ret;
}

static void spi_sf32lb_flush_rx_fifo(const struct device *dev)
{
	const struct spi_sf32lb_config *cfg = dev->config;
	uint32_t spi_status = sys_read32(cfg->base + SPI_STATUS);

	while ((spi_status & SPI_FLAG_FRLVL) != SPI_FRLVL_EMPTY) {
		(void)sys_read32(cfg->base + SPI_DATA);
		spi_status = sys_read32(cfg->base + SPI_STATUS);
	}
}

static void spi_sf32lb_reset_fifos(const struct device *dev)
{
	const struct spi_sf32lb_config *cfg = dev->config;

	/* Pulse both TX and RX FIFO reset bits to clear residual data */
	sys_set_bits(cfg->base + SPI_FIFO_CTRL, SPI_FIFO_CTRL_TSRE | SPI_FIFO_CTRL_RSRE);
	sys_clear_bits(cfg->base + SPI_FIFO_CTRL, SPI_FIFO_CTRL_TSRE | SPI_FIFO_CTRL_RSRE);
}

static int spi_sf32lb_wait_not_busy(const struct device *dev)
{
	const struct spi_sf32lb_config *cfg = dev->config;

	if (!WAIT_FOR(!sys_test_bit(cfg->base + SPI_STATUS, SPI_STATUS_BSY_Pos),
		SPI_MAX_BUSY_WAIT_US, NULL)) {
		return -ETIMEDOUT;
	}

	return 0;
}

static int transceive(const struct device *dev, const struct spi_config *config,
		      const struct spi_buf_set *tx_bufs,
		      const struct spi_buf_set *rx_bufs, bool asynchronous,
		      spi_callback_t cb,
		      void *userdata)
{
	const struct spi_sf32lb_config *cfg = dev->config;
	struct spi_sf32lb_data *data = dev->data;
	uint8_t dfs;
	int ret;

	if (tx_bufs == NULL && rx_bufs == NULL) {
		return 0;
	}

	spi_context_lock(&data->ctx, asynchronous, cb, userdata, config);

	ret = spi_sf32lb_configure(dev, config);
	if (ret < 0) {
		spi_context_release(&data->ctx, ret);
		return ret;
	}

	dfs = SPI_WORD_SIZE_GET(config->operation) >> 3;
	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, dfs);

	if (!spi_sf32lb_transfer_ongoing(data)) {
		spi_context_release(&data->ctx, 0);
		return 0;
	}

	spi_context_cs_control(&data->ctx, true);

	/* Restart peripheral to avoid residue between back-to-back transfers */
	sys_clear_bit(cfg->base + SPI_TOP_CTRL, SPI_TOP_CTRL_SSE_Pos);
	sys_set_bit(cfg->base + SPI_TOP_CTRL, SPI_TOP_CTRL_SSE_Pos);

	spi_sf32lb_reset_fifos(dev);
	spi_sf32lb_flush_rx_fifo(dev);
	sys_set_bits(cfg->base + SPI_STATUS, SPI_STATUS_ROR | SPI_STATUS_TUR | SPI_STATUS_TINT);

	/* Clear all interrupts */
	sys_clear_bits(cfg->base + SPI_INTE, SPI_INTE_RIE | SPI_INTE_TIE | SPI_INTE_TINTE);

	/* Enable interrupts based on transfer direction */
	if (spi_context_tx_on(&data->ctx)) {
		sys_set_bit(cfg->base + SPI_INTE, SPI_INTE_TIE_Pos);
	}
	if (spi_context_rx_on(&data->ctx)) {
		sys_set_bit(cfg->base + SPI_INTE, SPI_INTE_RIE_Pos);
	}

	/* Enable error interrupt */
	sys_set_bit(cfg->base + SPI_INTE, SPI_INTE_TINTE_Pos);

	/* Wait for transfer completion */
	ret = spi_context_wait_for_completion(&data->ctx);

	spi_context_cs_control(&data->ctx, false);

	spi_context_release(&data->ctx, ret);
spi_sf32lb_wait_not_busy(dev);
	return ret;
}

static int spi_sf32lb_transceive(const struct device *dev, const struct spi_config *config,
				 const struct spi_buf_set *tx_bufs,
				 const struct spi_buf_set *rx_bufs)
{
	return transceive(dev, config, tx_bufs, rx_bufs, false, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int spi_sf32lb_transceive_async(const struct device *dev, const struct spi_config *config,
				       const struct spi_buf_set *tx_bufs,
				       const struct spi_buf_set *rx_bufs, spi_callback_t cb,
				       void *userdata)
{
	return transceive(dev, config, tx_bufs, rx_bufs, true, cb, userdata);
}
#endif

static int spi_sf32lb_release(const struct device *dev, const struct spi_config *config)
{
	struct spi_sf32lb_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(spi, spi_sf32lb_api) = {
	.transceive = spi_sf32lb_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_sf32lb_transceive_async,
#endif
	.release = spi_sf32lb_release,
};

static int spi_sf32lb_init(const struct device *dev)
{
	const struct spi_sf32lb_config *cfg = dev->config;
	struct spi_sf32lb_data *data = dev->data;
	int err;

	if (!sf32lb_clock_is_ready_dt(&cfg->clock)) {
		LOG_ERR("Clock control device not ready");
		return -ENODEV;
	}

	err = sf32lb_clock_control_on_dt(&cfg->clock);
	if (err < 0) {
		LOG_ERR("Failed to enable clock");
		return err;
	}

	err = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		LOG_ERR("Failed to set pinctrl");
		return err;
	}

	err = spi_context_cs_configure_all(&data->ctx);
	if (err < 0) {
		return err;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	cfg->irq_config_func();

	return err;
}

#define SPI_SF32LB_DEFINE(n)                                                                       \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static void spi_sf32lb_irq_config_func_##n(void);                                          \
	static struct spi_sf32lb_data spi_sf32lb_data_##n = {                                      \
		SPI_CONTEXT_INIT_LOCK(spi_sf32lb_data_##n, ctx),                                   \
		SPI_CONTEXT_INIT_SYNC(spi_sf32lb_data_##n, ctx),                                   \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)};                             \
                                                                                                   \
	static const struct spi_sf32lb_config spi_sf32lb_config_##n = {                            \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.clock = SF32LB_CLOCK_DT_INST_SPEC_GET(n),                                         \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.irq_config_func = spi_sf32lb_irq_config_func_##n,                                 \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, spi_sf32lb_init, NULL, &spi_sf32lb_data_##n,                      \
			      &spi_sf32lb_config_##n, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,       \
			      &spi_sf32lb_api);                                                    \
	static void spi_sf32lb_irq_config_func_##n(void)                                           \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), spi_sf32lb_isr,             \
			DEVICE_DT_INST_GET(n), 0);                                                 \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

DT_INST_FOREACH_STATUS_OKAY(SPI_SF32LB_DEFINE)
