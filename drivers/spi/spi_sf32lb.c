/*
 * Copyright (c) 2025, Qingsong Gou <gouqs@hotmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb_spi

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/clock_control/sf32lb.h>
#include <zephyr/drivers/pinctrl.h>

#ifdef CONFIG_SPI_SF32LB_DMA
#include <zephyr/drivers/dma/dma_sf32lb.h>
#include <zephyr/drivers/dma.h>
#endif

#include <register.h>

LOG_MODULE_REGISTER(spi_sf32lb, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"

#define SPI_TOP_CTRL            offsetof(SPI_TypeDef, TOP_CTRL)
#define SPI_INTE                offsetof(SPI_TypeDef, INTE)
#define SPI_DATA                offsetof(SPI_TypeDef, DATA)
#define SPI_STATUS              offsetof(SPI_TypeDef, STATUS)
#define SPI_CLK_CTRL            offsetof(SPI_TypeDef, CLK_CTRL)
#define SPI_TRIWIRE_CTRL        offsetof(SPI_TypeDef, TRIWIRE_CTRL)

#ifdef CONFIG_SPI_SF32LB_DMA
struct spi_sf32lb_dma_config {
	const struct device *dev;
	uint32_t channel;
	uint32_t config;
	uint32_t slot;
	uint32_t fifo_threshold;
};
#endif

struct spi_sf32lb_config {
	uintptr_t base;
	struct sf32lb_clock_dt_spec clock;
	uint32_t irq_num;
	const struct pinctrl_dev_config *pcfg;
#ifdef CONFIG_SPI_SF32LB_INTERRUPT
	void (*irq_config_func)(const struct device *dev);
#endif
#ifdef CONFIG_SPI_SF32LB_DMA
	struct spi_sf32lb_dma_config tx_dma;
	struct spi_sf32lb_dma_config rx_dma;
#endif
};

struct spi_sf32lb_data {
	struct spi_context ctx;
};

static int spi_sf32lb_get_err(const struct device *dev)
{
	const struct spi_sf32lb_config *cfg = dev->config;
	uint32_t itflag = sys_read32(cfg->base + SPI_STATUS);
	uint32_t itsource = sys_read32(cfg->base + SPI_INTE);
#if 0
	if ((sys_test_bit(cfg->base + SPI_STATUS), SPI_STATUS_ROR) ||
		sys_test_bit(cfg->base + SPI_STATUS, SPI_STATUS_TUR)) {

	}

	if (((itflag & (SPI_STATUS_ROR | SPI_STATUS_TUR)) != RESET) &&
		((itsource & SPI_IT_ERR) != RESET)) {
		return -EIO;
	}
#endif
}

static int spi_sf32lb_configure(const struct device *dev, const struct spi_config *config)
{
	const struct spi_sf32lb_config *cfg = dev->config;
	struct spi_sf32lb_data *data = dev->data;
	uint32_t top_ctrl = 0;
	uint32_t clk_ctrl;
	uint32_t triwire_ctrl = 0;
	uint32_t div;
	uint8_t word_size;

	if (spi_context_configured(&data->ctx, config)) {
		return 0;
	}

	if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_SLAVE) {
		top_ctrl |= SPI_TOP_CTRL_SFRMDIR | SPI_TOP_CTRL_SCLKDIR;
	}
	top_ctrl |= (config->operation & SPI_MODE_CPOL) ? SPI_TOP_CTRL_SPO : 0U;
	top_ctrl |= (config->operation & SPI_MODE_CPHA) ? SPI_TOP_CTRL_SPH : 0U;

	word_size = SPI_WORD_SIZE_GET(config->operation);
	if (word_size == 8) {
		top_ctrl |= ((8 - 1) << SPI_TOP_CTRL_DSS_Pos) & SPI_TOP_CTRL_DSS_Msk;
	} else if (word_size == 16) {
		top_ctrl |= ((16 - 1) << SPI_TOP_CTRL_DSS_Pos) & SPI_TOP_CTRL_DSS_Msk;
	} else if (word_size == 24) {
		top_ctrl |= ((24 - 1) << SPI_TOP_CTRL_DSS_Pos) & SPI_TOP_CTRL_DSS_Msk;
	}

	if (SPI_FRAME_FORMAT_TI == (config->operation & SPI_FRAME_FORMAT_TI)) {
		top_ctrl |= (0x00000001U << SPI_TOP_CTRL_FRF_Pos);//SPI_FRAME_FORMAT_SSP;
	} else {
		top_ctrl |= (0x00000000U << SPI_TOP_CTRL_FRF_Pos);//SPI_FRAME_FORMAT_SPI;
	}

	if (SPI_HALF_DUPLEX == (config->operation & SPI_HALF_DUPLEX)) {
		triwire_ctrl |= SPI_TRIWIRE_CTRL_SPI_TRI_WIRE_EN;
		top_ctrl |= SPI_TOP_CTRL_TTE;
	} else {
		triwire_ctrl &= ~SPI_TRIWIRE_CTRL_SPI_TRI_WIRE_EN;
		top_ctrl &= ~SPI_TOP_CTRL_TTE;
	}

	sys_write32(top_ctrl, cfg->base + SPI_TOP_CTRL);
	sys_write32(triwire_ctrl, cfg->base + SPI_TRIWIRE_CTRL);

	div = 0x4C;//48000000U / config->frequency;
	clk_ctrl = sys_read32(cfg->base + SPI_CLK_CTRL);
	clk_ctrl &= ~SPI_CLK_CTRL_CLK_DIV_Msk;
	clk_ctrl |= (div << SPI_CLK_CTRL_CLK_DIV_Pos) & SPI_CLK_CTRL_CLK_DIV_Msk;
	sys_write32(clk_ctrl, cfg->base + SPI_CLK_CTRL);
#if 0
	/* Calculate baud rate prescaler */
	// uint32_t br = 0;
	// uint32_t div = data->pclk_freq / config->frequency;
	/// while (div > (1 << (br + 1)) && br < 7) {
	//	br++;
	//}
	hspi->Init.BaudRatePrescaler = 0xC;

	hspi->Init.SFRMPol = SPI_SFRMPOL_HIGH;
#endif
	/* Issue 1401: Make SPO setting is valid before start transfer data*/
	sys_set_bit(cfg->base + SPI_TOP_CTRL, SPI_TOP_CTRL_SSE_Pos);
	sys_clear_bit(cfg->base + SPI_TOP_CTRL, SPI_TOP_CTRL_SSE_Pos);

	return 0;
}

#ifdef CONFIG_SPI_SF32LB_DMA
static void spi_sf32lb_dma_rx_done(const struct device *dev, void *arg, uint32_t channel, int status)
{

	struct i2c_sf32lb_data *data = dev->data;

	printk("spi_sf32lb_dma_rx_done!\n");

	k_sem_give(&data->sync_sem);

	#if 0

struct max32_i2c_data *data = arg;

	const struct device *i2c_dev = data->dev;

	const struct max32_i2c_config *const cfg = i2c_dev->config;



	if (status < 0) {

	data->err = -EIO;

		Wrap_MXC_I2C_SetIntEn(cfg->regs, 0, 0);

		k_sem_give(&data->xfer);

		} else {

	if (data->flags & I2C_MSG_STOP) {

		Wrap_MXC_I2C_Stop(cfg->regs);

			} else if (!(data->flags & I2C_MSG_READ)) {

		MXC_I2C_EnableInt(cfg->regs, ADI_MAX32_I2C_INT_EN0_TX_THD, 0);

			}

		}

	#endif

}

static void spi_sf32lb_dma_tx_done(const struct device *dev, void *arg, uint32_t channel, int status)
{

	struct i2c_sf32lb_data *data = dev->data;

	printk("spi_sf32lb_dma_rx_done!\n");

	k_sem_give(&data->sync_sem);
}


static int spi_sf32lb_transceive_dma(const struct device *dev, const struct spi_config *config,
				 const struct spi_buf_set *tx_bufs,
				 const struct spi_buf_set *rx_bufs)
{
	const struct spi_sf32lb_config *cfg = dev->config;
	struct spi_sf32lb_data *data = dev->data;
	SPI_HandleTypeDef *hspi = &data->hspi;
	struct dma_config dma_cfg = {0};
	struct dma_block_config dma_blk = {0};
	int ret = 0;
#ifdef CONFIG_DCACHE
#endif
	//__HAL_SPI_DISABLE_IT(hspi, (SPI_INTE_TIE | SPI_INTE_RIE | SPI_IT_ERR));
	sys_clear_bits(cfg->base + SPI_INTE, (SPI_INTE_TIE | SPI_INTE_RIE | SPI_IT_ERR));

	//__HAL_SPI_ENABLE_IT(hspi, (SPI_INTE_RIE));
	sys_set_bits(cfg->base + SPI_INTE, SPI_INTE_RIE);
	//spi_sf32lb_rx_dma_setup(dev, tx_bufs, rx_bufs);
	{
		dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
		dma_cfg.dma_callback = spi_sf32lb_dma_rx_done;//
		dma_cfg.user_data = (void *)dev;
		dma_cfg.dma_slot = config->rx_dma.slot;
		dma_cfg.block_count = 1;
		dma_cfg.source_data_size = 1U;
		dma_cfg.source_burst_length = 1U;
		dma_cfg.dest_data_size = 1U;

		dma_cfg.head_block = &dma_blk;

		dma_blk.block_size = len;
		dma_blk.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		dma_blk.dest_address = (uint32_t)buf;
		dma_blk.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		dma_blk.source_address = (uint32_t)config->reg_base->FIFO;

		ret = dma_config(config->rx_dma.dev, config->rx_dma.channel, &dma_cfg);
		if (ret < 0) {
			return ret;
		}

		return dma_start(config->rx_dma.dev, config->rx_dma.channel);
	}
	//spi_sf32lb_rx_dma_setup(dev, tx_bufs, rx_bufs)
	{
		dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
		dma_cfg.source_data_size = 1;
		dma_cfg.dest_data_size = 1;
		dma_cfg.user_data = (void *)dev;
		dma_cfg.dma_callback = spi_sf32lb_dma_tx_done;//
		dma_cfg.block_count = 1;
		dma_cfg.dma_slot = config->tx_dma.slot;

		dma_cfg.head_block = &dma_blk;

		dma_blk.block_size = len;
		dma_blk.source_address = (uint32_t)buf;
		dma_blk.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		dma_blk.dest_address = (uint32_t)config->reg_base->FIFO;
		dma_blk.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;

		ret = dma_config(config->tx_dma.dev, config->tx_dma.channel, &dma_cfg);
		if (ret != 0) {
			LOG_ERR("Write DMA configure on %s failed: %d",
				dev->name, ret);
			return ret;
		}

		ret = dma_start(config->tx_dma.dev, config->tx_dma.channel);
		if (ret != 0) {
			LOG_ERR("Write DMA start on %s failed: %d",
				dev->name, ret);
			return ret;
		}
	}
}
#endif /* CONFIG_SPI_SF32LB_DMA */

static int spi_sf32lb_frame_exchange(const struct device *dev)
{
	struct spi_sf32lb_data *data = dev->data;
	const struct spi_sf32lb_config *cfg = dev->config;
	struct spi_context *ctx = &data->ctx;

	uint16_t tx_frame = 0U, rx_frame = 0U;
printk("spi_sf32lb_frame_exchange\r\n");
#if 0
	if (hspi->Init.Direction == SPI_DIRECTION_1LINE)
	{
		/*1line tx enable*/
		SPI_1LINE_TX(hspi);
	}
#endif
#if 1
	uint32_t itsource = sys_read32(cfg->base + SPI_INTE);
	uint32_t itflag = sys_read32(cfg->base + SPI_STATUS);
printk("mask=%x, it=%x  \r\n", itsource, itflag);

	/* Check if the SPI is already enabled */
	//if ((hspi->Instance->TOP_CTRL & SPI_TOP_CTRL_SSE) != SPI_TOP_CTRL_SSE) {
	if (sys_test_bit(cfg->base + SPI_TOP_CTRL, SPI_TOP_CTRL_SSE_Pos)) {
		/* Enable SPI peripheral */
		//__HAL_SPI_ENABLE(hspi);
		sys_set_bits(cfg->base + SPI_TOP_CTRL, SPI_TOP_CTRL_SSE);
	}

	if (SPI_WORD_SIZE_GET(ctx->config->operation) == 8) {
		if (spi_context_tx_buf_on(ctx) && sys_test_bit(cfg->base + SPI_STATUS, SPI_STATUS_TNF_Pos)) {
			tx_frame = UNALIGNED_GET((uint8_t *)(data->ctx.tx_buf));
			//*(__IO uint8_t *)(&hspi->Instance->DATA) = *((uint8_t *)&tx_frame);
			sys_write8(tx_frame, cfg->base + SPI_DATA);
			spi_context_update_tx(ctx, 1, 1);
			//printk("tx...\r\n");
		}
	} else {
		if (spi_context_tx_buf_on(ctx) && sys_test_bit(cfg->base + SPI_STATUS, SPI_STATUS_TNF_Pos)) {
			tx_frame = UNALIGNED_GET((uint16_t *)(data->ctx.tx_buf));
			//data->hspi.Instance->DATA = tx_frame;
			sys_write32(tx_frame, cfg->base + SPI_DATA);
			spi_context_update_tx(ctx, 2, 1);
			//printk("tx...\r\n");
		}
	}

	if (SPI_WORD_SIZE_GET(ctx->config->operation) == 8) {
		if (spi_context_rx_buf_on(ctx) && sys_test_bit(cfg->base + SPI_STATUS, SPI_STATUS_RNE_Pos)) {
			//rx_frame = *(__IO uint8_t *)&hspi->Instance->DATA;
			rx_frame = sys_read8(cfg->base + SPI_DATA);
			UNALIGNED_PUT(rx_frame, (uint8_t *)data->ctx.rx_buf);
			spi_context_update_rx(ctx, 1, 1);//printk("rx...\r\n");
		}
	} else {
		if (spi_context_rx_buf_on(ctx) && sys_test_bit(cfg->base + SPI_STATUS, SPI_STATUS_RNE_Pos)) {
			//rx_frame = hspi->Instance->DATA;
			rx_frame = sys_read32(cfg->base + SPI_DATA);
			UNALIGNED_PUT(rx_frame, (uint16_t *)data->ctx.rx_buf);
			spi_context_update_rx(ctx, 2, 1);
			//printk("rx...\r\n");
		}
	}
#endif
	return 0;
}

static bool spi_sf32lb_transfer_ongoing(struct spi_sf32lb_data *data)
{
	return spi_context_tx_on(&data->ctx) || spi_context_rx_on(&data->ctx);
}

static int spi_sf32lb_transceive(const struct device *dev, const struct spi_config *config,
				 const struct spi_buf_set *tx_bufs,
				 const struct spi_buf_set *rx_bufs)
{
	const struct spi_sf32lb_config *cfg = dev->config;
	struct spi_sf32lb_data *data = dev->data;
	int ret = 0;

	spi_context_lock(&data->ctx, false, NULL, NULL, config);

	ret = spi_sf32lb_configure(dev, config);
	if (ret != 0) {
		return ret;
	}

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

	spi_context_cs_control(&data->ctx, true);

	size_t len = spi_context_total_tx_len(&data->ctx);
	uint8_t *tx_buf = (uint8_t *)data->ctx.tx_buf;
	uint8_t *rx_buf = (uint8_t *)data->ctx.rx_buf;

#ifdef CONFIG_SPI_SF32LB_INTERRUPT
#ifdef CONFIG_SPI_SF32LB_DMA
	if (spi_sf32_dma_enabled(dev)) {
		ret = spi_sf32lb_transceive_dma(dev);
		if (ret < 0) {
			goto dma_error;
		}
	}
#else
	sys_clear_bits(cfg->base + SPI_INTE, (SPI_INTE_TIE | SPI_INTE_RIE | (SPI_INTE_EBCEI | SPI_INTE_TINTE | SPI_INTE_PINTE)));
	sys_set_bits(cfg->base + SPI_INTE, (SPI_INTE_EBCEI | SPI_INTE_TINTE | SPI_INTE_PINTE));
	if (rx_buf) {
		sys_set_bit(cfg->base + SPI_INTE, SPI_INTE_RIE);
	}

	if (tx_buf) {
		sys_set_bits(cfg->base + SPI_INTE, SPI_INTE_TIE);
	}

	/* Check if the SPI is already enabled */
	if (!sys_test_bit(cfg->base + SPI_TOP_CTRL, SPI_TOP_CTRL_SSE_Pos)) {
		/* Enable SPI peripheral */
		//__HAL_SPI_ENABLE(hspi);
		sys_set_bit(cfg->base + SPI_TOP_CTRL, SPI_TOP_CTRL_SSE_Pos);
	}
	ret = spi_context_wait_for_completion(&data->ctx);
#endif /* CONFIG_SPI_SF32LB_DMA */
#else
	//do {
		printk("spi_sf32lb_frame_exchange\n");
		ret = spi_sf32lb_frame_exchange(dev);
		//if (ret < 0) {
		//	break;
		//}
	//} while (spi_sf32lb_transfer_ongoing(data));
#endif

out:
	spi_context_release(&data->ctx, ret);

	return ret;
}

static int spi_sf32lb_release(const struct device *dev, const struct spi_config *config)
{
	struct spi_sf32lb_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static const struct spi_driver_api spi_sf32lb_api = {
	.transceive = spi_sf32lb_transceive,
	.release = spi_sf32lb_release,
};

#ifdef CONFIG_SPI_SF32LB_INTERRUPT
void spi_sf32lb_complete(const struct device *dev, int status)
{
	const struct spi_sf32lb_config *cfg = dev->config;
	struct spi_sf32lb_data *data = dev->data;
#if 0
	SPI_HandleTypeDef *hspi = &data->hspi;

	__HAL_SPI_CLEAR_OVRFLAG(hspi);
#endif
	//__HAL_SPI_CLEAR_UDRFLAG(hspi);
	sys_clear_bits(cfg->base + SPI_STATUS, SPI_STATUS_TUR);
	//__HAL_SPI_DISABLE_IT(hspi, SPI_INTE_RIE | SPI_INTE_TIE | SPI_IT_ERR);
	sys_clear_bits(cfg->base + SPI_INTE, SPI_INTE_RIE | SPI_INTE_TIE | (SPI_INTE_EBCEI | SPI_INTE_TINTE | SPI_INTE_PINTE));
	spi_context_complete(&data->ctx, dev, status);
}

static void spi_sf32lb_isr(const struct device *dev)
{
	const struct spi_sf32lb_config *cfg = dev->config;
	struct spi_sf32lb_data *data = dev->data;
	int err = 0;
	uint32_t itsource = sys_read32(cfg->base + SPI_INTE);
	uint32_t flag = sys_read32(cfg->base + SPI_STATUS);
	struct spi_context *ctx = &data->ctx;
	uint16_t tx_frame = 0U, rx_frame = 0U;
#if 0
	if (((flag & (SPI_STATUS_ROR | SPI_STATUS_TUR)) != 0U)
		&& ((itsource &  (SPI_INTE_EBCEI | SPI_INTE_TINTE | SPI_INTE_PINTE)) != 0U)) {
		printk("error!\r\n");
		err = -EIO;
		spi_sf32lb_complete(dev, err);
	}

    	if (((flag & SPI_STATUS_ROR) == RESET) &&
            ((flag & SPI_STATUS_RFS) != RESET) && ((itsource & SPI_INTE_RIE) != RESET))
   	{
		if (SPI_WORD_SIZE_GET(ctx->config->operation) == 8) {
			if (spi_context_rx_buf_on(ctx)) {
				rx_frame = *(__IO uint8_t *)&hspi->Instance->DATA;
				UNALIGNED_PUT(rx_frame, (uint8_t *)data->ctx.rx_buf);
				spi_context_update_rx(ctx, 1, 1);//printk("rx...\r\n");
			}
		} else {
			if (spi_context_rx_buf_on(ctx)) {
				rx_frame = hspi->Instance->DATA;
				UNALIGNED_PUT(rx_frame, (uint16_t *)data->ctx.rx_buf);
				spi_context_update_rx(ctx, 2, 1);//printk("rx...\r\n");
			}
		}
    	}
#endif
    /* SPI in mode Transmitter -------------------------------------------------*/
	if (((flag & SPI_STATUS_TUR) == 0U) &&
            ((flag & SPI_STATUS_TFS) != 0U) && ((itsource & SPI_INTE_TIE) != 0U)) {

		if (SPI_WORD_SIZE_GET(ctx->config->operation) == 8) {
			if (spi_context_tx_buf_on(ctx)) {
				tx_frame = UNALIGNED_GET((uint8_t *)(data->ctx.tx_buf));
				//*(__IO uint8_t *)(&hspi->Instance->DATA) = *((uint8_t *)&tx_frame);
				sys_write8(tx_frame, cfg->base + SPI_DATA);
				spi_context_update_tx(ctx, 1, 1);
				//printk("tx...\r\n");
			}
		} else {
			if (spi_context_tx_buf_on(ctx)) {
				tx_frame = UNALIGNED_GET((uint16_t *)(data->ctx.tx_buf));
				//data->hspi.Instance->DATA = tx_frame;
				sys_write16(tx_frame, cfg->base + SPI_DATA);
				spi_context_update_tx(ctx, 2, 1);
				//printk("tx...\r\n");
			}
		}
	}
printk("int=%x, status=%x  \r\n", itsource, flag);
	//err = spi_sf32lb_get_err(hspi);
	//if (err) {
	//	spi_sf32lb_complete(dev, err);
	//	return;
	//}

	//if (spi_sf32lb_transfer_ongoing(data)) {
	//	err = spi_sf32lb_frame_exchange(dev);
	//}

	if (err) {
		spi_sf32lb_complete(dev, err);
	}

	if (err || !spi_sf32lb_transfer_ongoing(data)) {
		spi_sf32lb_complete(dev, err);
	}

}
#endif

static int spi_sf32lb_init(const struct device *dev)
{
	const struct spi_sf32lb_config *cfg = dev->config;
	struct spi_sf32lb_data *data = dev->data;
	int err = 0;

	if (cfg->clock.dev != NULL) {
		if (!sf3232lb_clock_is_ready_dt(&cfg->clock)) {
			LOG_ERR("Clock control device not ready");
			return -ENODEV;
		}

		err = sf32lb_clock_control_on_dt(&cfg->clock);
		if (err < 0) {
			LOG_ERR("Failed to enable clock");
			return err;
		}
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
#ifdef CONFIG_SPI_SF32LB_INTERRUPT
	cfg->irq_config_func(dev);
#endif
	LOG_INF("%s Initialization OK", dev->name);

	return err;
}

#ifdef CONFIG_SPI_SF32LB_INTERRUPT
#define SF32LB_SPI_IRQ_HANDLER_DECL(id)	\
	static void spi_sf32lb_irq_config_func_##id(const struct device *dev);

#define SF32LB_SPI_IRQ_HANDLER_FUNC(id) \
	.irq_config_func = spi_sf32lb_irq_config_func_##id,

#define SF32LB_SPI_IRQ_HANDLER(id) \
static void spi_sf32lb_irq_config_func_##id(const struct device *dev) \
{ \
	IRQ_CONNECT(DT_INST_IRQN(id), \
		    DT_INST_IRQ(id, priority), \
		    spi_sf32lb_isr, DEVICE_DT_INST_GET(id), 0); \
	irq_enable(DT_INST_IRQN(id)); \
}
#else
#define SF32LB_SPI_IRQ_HANDLER_DECL(id)
#define SF32LB_SPI_IRQ_HANDLER_FUNC(id)
#define SF32LB_SPI_IRQ_HANDLER(id)
#endif /* CONFIG_SPI_SF32LB_INTERRUPT */

#ifdef CONFIG_SPI_SF32LB_DMA
#define SF32LB_SPI_DMA_INIT(n) \
	.tx_dma.dev = SF32LB_DT_INST_DMA_CTLR(n, tx), \
	.tx_dma.channel = SF32LB_DT_INST_DMA_CHANNEL(n, tx), \
	.tx_dma.slot = SF32LB_DT_INST_DMA_SLOT(n, rx), \
	.rx_dma.dev = SF32LB_DT_INST_DMA_CTLR(n, rx), \
	.rx_dma.channel = SF32LB_DT_INST_DMA_CHANNEL(n, rx), \
	.rx_dma.slot = SF32LB_DT_INST_DMA_SLOT(n, rx),
#else
#define SF32LB_SPI_DMA_INIT(n)
#endif /* CONFIG_SPI_SF32LB_DMA */

#define SPI_SF32LB_INIT(n)                                                                         \
	SF32LB_SPI_IRQ_HANDLER_DECL(n);                                                            \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static struct spi_sf32lb_data spi_sf32lb_data_##n = {                                      \
		SPI_CONTEXT_INIT_LOCK(spi_sf32lb_data_##n, ctx),                                   \
		SPI_CONTEXT_INIT_SYNC(spi_sf32lb_data_##n, ctx),                                   \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)};                             \
                                                                                                   \
	static const struct spi_sf32lb_config spi_sf32lb_config_##n = {                            \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.clock = SF32LB_CLOCK_DT_INST_SPEC_GET_OR(n, {}),                                  \
		.irq_num = DT_INST_IRQN(n),                                                        \
		SF32LB_SPI_IRQ_HANDLER_FUNC(n)                                                     \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		SF32LB_SPI_DMA_INIT(n)                                                             \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, spi_sf32lb_init, NULL, &spi_sf32lb_data_##n,                      \
			      &spi_sf32lb_config_##n, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,       \
			      &spi_sf32lb_api);                                                    \
                                                                                                   \
	SF32LB_SPI_IRQ_HANDLER(n)

DT_INST_FOREACH_STATUS_OKAY(SPI_SF32LB_INIT)
