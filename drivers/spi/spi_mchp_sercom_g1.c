/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi/rtio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/mchp_clock_control.h>
#if CONFIG_SPI_MCHP_DMA_DRIVEN
#include <zephyr/drivers/dma.h>
#include <mchp_dt_helper.h>
#endif /* CONFIG_SPI_MCHP_DMA_DRIVEN */

#define DT_DRV_COMPAT microchip_sercom_g1_spi

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
LOG_MODULE_REGISTER(spi_mchp_sercom_g1);

#include "spi_context.h"

#define SPI_MCHP_MAX_XFER_SIZE  65535
#define SUPPORTED_SPI_WORD_SIZE 8
#define SPI_PIN_CNT             4
#define TIMEOUT_VALUE_US        1000
#define DELAY_US                1
#define DMA_CHANNEL_INVALID     0xFF
#define DUMMY_DATA              0

struct mchp_spi_reg_config {
	sercom_registers_t *regs;
	uint32_t pads;
};

struct mchp_spi_clock {
	const struct device *clock_dev;
	clock_control_subsys_t mclk_sys;
	clock_control_subsys_t gclk_sys;
};

struct mchp_spi_dma {
	const struct device *dma_dev;
	uint8_t tx_dma_request;
	uint8_t tx_dma_channel;
	uint8_t rx_dma_request;
	uint8_t rx_dma_channel;
};

struct spi_mchp_dev_config {
	struct mchp_spi_reg_config reg_cfg;
	const struct pinctrl_dev_config *pcfg;

#if CONFIG_SPI_MCHP_DMA_DRIVEN
	struct mchp_spi_dma spi_dma;
#else
	void (*irq_config_func)(const struct device *dev);
#endif /* CONFIG_SPI_MCHP_DMA_DRIVEN */
	struct mchp_spi_clock spi_clock;
};

struct spi_mchp_dev_data {
	struct spi_context ctx;

#if CONFIG_SPI_MCHP_DMA_DRIVEN
	const struct device *dev;
	uint32_t dma_segment_len;
#else
	uint16_t dummysize;
#endif /* CONFIG_SPI_MCHP_DMA_DRIVEN */
};

/*Wait for synchronization*/
static inline void spi_wait_sync(const struct mchp_spi_reg_config *spi_reg_cfg, uint32_t sync_flag)
{
	sercom_spi_registers_t *spi = SPI_GET_BASE_ADDR(spi_reg_cfg->regs, SPI_OP_MODE_MASTER);

	if (WAIT_FOR(((spi->SERCOM_SYNCBUSY & sync_flag) == 0), TIMEOUT_VALUE_US,
		     k_busy_wait(DELAY_US)) == false) {
		LOG_ERR("Timeout waiting for SPI SYNCBUSY ENABLE clear");
	}
}

/*Enable the SPI peripheral*/
static void spi_enable(const struct mchp_spi_reg_config *spi_reg_cfg, spi_operation_t op)
{
	sercom_spi_registers_t *spi =
		SPI_GET_BASE_ADDR(spi_reg_cfg->regs, SPI_OP_MODE_GET(op) == SPI_OP_MODE_SLAVE);

	spi->SERCOM_CTRLA |= SERCOM_SPI_CTRLA_ENABLE_Msk;
	spi_wait_sync(spi_reg_cfg, SERCOM_SPI_SYNCBUSY_ENABLE_Msk);
}

/*Disable the SPI peripheral*/
static void spi_disable(const struct mchp_spi_reg_config *spi_reg_cfg)
{
	sercom_spi_registers_t *spi = SPI_GET_BASE_ADDR(spi_reg_cfg->regs, SPI_OP_MODE_MASTER);

	spi->SERCOM_CTRLA &= ~SERCOM_SPI_CTRLA_ENABLE_Msk;
	spi_wait_sync(spi_reg_cfg, SERCOM_SPI_SYNCBUSY_ENABLE_Msk);
}

/*Set the BAUD Rate value for SPI peripheral*/
static void spi_set_baudrate(const struct mchp_spi_reg_config *spi_reg_cfg,
			     const struct spi_config *config, uint32_t clk_freq_hz)
{
	uint32_t divisor = 2U * config->frequency;

	/* Use the requested or next highest possible frequency */
	uint32_t baud_value = (clk_freq_hz / divisor) - 1;

	sercom_spi_registers_t *spi = SPI_GET_BASE_ADDR(spi_reg_cfg->regs, SPI_OP_MODE_MASTER);

	if ((clk_freq_hz % divisor) >= (divisor / 2U)) {
		/* Round up the baud_value to ensures SPI clock is as close as possible to
		 * the requested frequency
		 */
		baud_value += 1U;
	}

	baud_value = CLAMP(baud_value, 0, UINT8_MAX);

	spi->SERCOM_BAUD = baud_value;
}

/*Write Data into DATA register*/
static inline void spi_write_data(const struct mchp_spi_reg_config *spi_reg_cfg, uint8_t data)
{
	sercom_spi_registers_t *spi = SPI_GET_BASE_ADDR(spi_reg_cfg->regs, SPI_OP_MODE_MASTER);

	spi->SERCOM_DATA = data;
}

/*Read Data from the SPI MASTER DATA register*/
static inline uint8_t spi_read_data(const struct mchp_spi_reg_config *spi_reg_cfg)
{
	sercom_spi_registers_t *spi = SPI_GET_BASE_ADDR(spi_reg_cfg->regs, SPI_OP_MODE_MASTER);

	return (uint8_t)spi->SERCOM_DATA;
}

/*Return true if data register empty flag is set*/
static inline bool spi_slave_is_data_reg_empty(const struct mchp_spi_reg_config *spi_reg_cfg)
{
	sercom_spi_registers_t *spi = SPI_GET_BASE_ADDR(spi_reg_cfg->regs, SPI_OP_MODE_SLAVE);

	return (spi->SERCOM_INTFLAG & SERCOM_SPI_INTFLAG_DRE_Msk) == SERCOM_SPI_INTFLAG_DRE_Msk;
}

/*Write Data into DATA register*/
static inline void spi_slave_write_data(const struct mchp_spi_reg_config *spi_reg_cfg, uint8_t data)
{
	sercom_spi_registers_t *spi = SPI_GET_BASE_ADDR(spi_reg_cfg->regs, SPI_OP_MODE_SLAVE);

	spi->SERCOM_DATA = data;
}

static int spi_configure_pinout(const struct mchp_spi_reg_config *spi_reg_cfg,
				const struct spi_config *config)
{
	sercom_spi_registers_t *spi = SPI_GET_BASE_ADDR(
		spi_reg_cfg->regs, SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_SLAVE);

	if ((config->operation & SPI_MODE_LOOP) != 0U) {
		if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_SLAVE) {
			LOG_ERR("For slave Loopback mode is not supported");

			return -ENOTSUP;
		}
		/*Set the pads for the SPI Transmission for loopback mode*/
		spi->SERCOM_CTRLA = (spi->SERCOM_CTRLA &
				     ~(SERCOM_SPI_CTRLA_DIPO_Msk | SERCOM_SPI_CTRLA_DOPO_Msk)) |
				    (SERCOM_SPI_CTRLA_DIPO_MUX0 | SERCOM_SPI_CTRLA_DOPO_MUX0);
	} else {
		/*Set the pads for the SPI Transmission*/
		spi->SERCOM_CTRLA = (spi->SERCOM_CTRLA &
				     ~(SERCOM_SPI_CTRLA_DIPO_Msk | SERCOM_SPI_CTRLA_DOPO_Msk)) |
				    spi_reg_cfg->pads;
	}

	return 0;
}

static void spi_configure_cpol(const struct mchp_spi_reg_config *spi_reg_cfg,
			       const struct spi_config *config)
{
	sercom_spi_registers_t *spi = SPI_GET_BASE_ADDR(
		spi_reg_cfg->regs, SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_SLAVE);

	uint32_t reg = spi->SERCOM_CTRLA;

	reg &= ~SERCOM_SPI_CTRLA_CPOL_Msk;

	if ((config->operation & SPI_MODE_CPOL) != 0U) {
		/*Set the SPI Clock Polarity Idle High*/
		reg |= SERCOM_SPI_CTRLA_CPOL_IDLE_HIGH;
	} else {
		/* Clear the CPOL bit field and set clock polarity to Idle Low */
		reg |= SERCOM_SPI_CTRLA_CPOL_IDLE_LOW;
	}
}

static void spi_configure_cpha(const struct mchp_spi_reg_config *spi_reg_cfg,
			       const struct spi_config *config)
{
	sercom_spi_registers_t *spi = SPI_GET_BASE_ADDR(
		spi_reg_cfg->regs, SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_SLAVE);

	uint32_t reg = spi->SERCOM_CTRLA;

	reg &= ~SERCOM_SPI_CTRLA_CPHA_Msk;

	if ((config->operation & SPI_MODE_CPHA) != 0U) {
		/*Set the SPI Clock Phase Trailing Edge*/
		reg |= SERCOM_SPI_CTRLA_CPHA_TRAILING_EDGE;
	} else {
		/* Clear the CPHA bit field and set clock phase to Leading Edge */
		reg |= SERCOM_SPI_CTRLA_CPHA_LEADING_EDGE;
	}
}

static void spi_configure_bit_order(const struct mchp_spi_reg_config *spi_reg_cfg,
				    const struct spi_config *config)
{
	sercom_spi_registers_t *spi = SPI_GET_BASE_ADDR(
		spi_reg_cfg->regs, SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_SLAVE);

	uint32_t reg = spi->SERCOM_CTRLA;

	reg &= ~SERCOM_SPI_CTRLA_DORD_Msk;

	if ((config->operation & SPI_TRANSFER_LSB) != 0U) {
		/*Set the SPI Data Order, LSB first*/
		reg |= SERCOM_SPI_CTRLA_DORD_LSB;
	} else {
		reg |= SERCOM_SPI_CTRLA_DORD_MSB;
	}
}

static int spi_configure(const struct device *dev, const struct spi_config *config)
{
	const struct spi_mchp_dev_config *cfg = dev->config;
	const struct mchp_spi_reg_config *spi_reg_cfg = &cfg->reg_cfg;
	struct spi_mchp_dev_data *const data = dev->data;
	int retval;
	uint32_t clock_rate;
	bool has_cs = false;
	sercom_spi_registers_t *spi = SPI_GET_BASE_ADDR(
		spi_reg_cfg->regs, SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_SLAVE);

	spi_disable(spi_reg_cfg);

	if (spi_context_configured(&data->ctx, config) == true) {
		spi_enable(spi_reg_cfg, config->operation);

		return 0;
	}

	/* Select the Character Size */
	if (SPI_WORD_SIZE_GET(config->operation) != SUPPORTED_SPI_WORD_SIZE) {
		LOG_ERR("Unsupported SPI word size: %d bits. Only 8-bit transfers are supported.",
			SPI_WORD_SIZE_GET(config->operation));

		return -ENOTSUP;
	}
	/* Clear the CHSIZE bit field and set character size to 8-bit */
	spi->SERCOM_CTRLB =
		(spi->SERCOM_CTRLB & ~SERCOM_SPI_CTRLB_CHSIZE_Msk) | SERCOM_SPI_CTRLB_CHSIZE_8_BIT;

	/*Enable the Receiver in SPI peripheral*/
	spi->SERCOM_CTRLB |= SERCOM_SPI_CTRLB_RXEN_Msk;
	spi_wait_sync(spi_reg_cfg, SERCOM_SPI_SYNCBUSY_CTRLB_Msk);
#if CONFIG_SPI_SLAVE
	if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_SLAVE) {
		/* Enable the preload slave data*/
		spi->SERCOM_CTRLB |= SERCOM_SPI_CTRLB_PLOADEN_Msk;
		/* Enable the slave select detection*/
		spi->SERCOM_CTRLB |= SERCOM_SPI_CTRLB_SSDE_Msk;
		/* Enable the Immediate buffer overflow*/
		spi->SERCOM_CTRLA |= SERCOM_SPI_CTRLA_IBON_Msk;
		/*Set the SPI Slave Mode*/
		spi->SERCOM_CTRLA = (spi->SERCOM_CTRLA & ~SERCOM_SPI_CTRLA_MODE_Msk) |
				    SERCOM_SPI_CTRLA_MODE_SPI_SLAVE;
	}
#endif /* CONFIG_SPI_SLAVE */

	if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_MASTER) {

#ifdef CONFIG_SPI_MCHP_INTER_CHARACTER_SPACE
		spi_reg_cfg->regs->SPIM.SERCOM_CTRLC =
			(spi_reg_cfg->regs->SPIM.SERCOM_CTRLC & ~SERCOM_SPIM_CTRLC_ICSPACE_Msk) |
			SERCOM_SPIM_CTRLC_ICSPACE(CONFIG_SPI_MCHP_INTER_CHARACTER_SPACE);
#endif /* CONFIG_SPI_MCHP_INTER_CHARACTER_SPACE */

		clock_control_get_rate(cfg->spi_clock.clock_dev, cfg->spi_clock.gclk_sys,
				       &clock_rate);

		if ((config->frequency != 0) && (clock_rate >= (2 * config->frequency))) {
			spi_set_baudrate(spi_reg_cfg, config, clock_rate);
		} else {
			return -ENOTSUP;
		}

		/* Clear the MODE bit field and set it to SPI Master mode */
		spi->SERCOM_CTRLA = (spi->SERCOM_CTRLA & ~SERCOM_SPI_CTRLA_MODE_Msk) |
				    SERCOM_SPI_CTRLA_MODE_SPI_MASTER;

#if !DT_SPI_CTX_HAS_NO_CS_GPIOS
		has_cs = (data->ctx.num_cs_gpios != 0);
#endif

		if (has_cs) {
			retval = spi_context_cs_configure_all(&data->ctx);
			if (retval < 0) {
				return retval;
			}
		} else if (cfg->pcfg->states->pin_cnt == SPI_PIN_CNT) {
			spi_wait_sync(spi_reg_cfg, SERCOM_SPI_SYNCBUSY_CTRLB_Msk);
			/* Enable Master Slave Select */
			spi->SERCOM_CTRLB |= SERCOM_SPI_CTRLB_MSSEN_Msk;
			spi_wait_sync(spi_reg_cfg, SERCOM_SPI_SYNCBUSY_CTRLB_Msk);
		} else {
			/* Handled by user */
		}
	}

	if ((config->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
		LOG_ERR("Only single line mode is supported");

		return -ENOTSUP;
	}

	/*Set the Data out and Pin out Configuration*/
	retval = spi_configure_pinout(spi_reg_cfg, config);
	if (retval < 0) {
		return retval;
	}

	spi_configure_cpol(spi_reg_cfg, config);
	spi_configure_cpha(spi_reg_cfg, config);
	spi_configure_bit_order(spi_reg_cfg, config);

	if ((config->operation & SPI_HALF_DUPLEX) != 0U) {
		return -ENOTSUP;
	}

	spi_enable(spi_reg_cfg, config->operation);

#if CONFIG_SPI_MCHP_DMA_DRIVEN
	if (device_is_ready(cfg->spi_dma.dma_dev) != true) {
		return -ENODEV;
	}
	data->dev = dev;
#else
	cfg->irq_config_func(dev);
#endif /* CONFIG_SPI_MCHP_DMA_DRIVEN */

	data->ctx.config = config;

	return 0;
}

#if CONFIG_SPI_MCHP_DMA_DRIVEN
static void spi_dma_rx_done(const struct device *dma_dev, void *arg, uint32_t id, int error_code);

static int spi_dma_tx_load(const struct device *dev, const uint8_t *buf, size_t len)
{
	const struct spi_mchp_dev_config *cfg = dev->config;
	const struct mchp_spi_reg_config *spi_reg_cfg = &cfg->reg_cfg;
	sercom_spi_registers_t *spi = SPI_GET_BASE_ADDR(spi_reg_cfg->regs, SPI_OP_MODE_MASTER);

	struct dma_config dma_cfg = {0};
	struct dma_block_config dma_blk = {0};
	int retval;

	dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
	dma_cfg.source_data_size = 1;
	dma_cfg.dest_data_size = 1;
	dma_cfg.block_count = 1;
	dma_cfg.head_block = &dma_blk;
	dma_cfg.dma_slot = cfg->spi_dma.tx_dma_request;

	dma_blk.block_size = len;

	if (buf != NULL) {
		dma_blk.source_address = (uint32_t)buf;
	} else {
		static const uint8_t dummy_data;

		dma_blk.source_address = (uint32_t)&dummy_data;
		dma_blk.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	}

	dma_blk.dest_address = (uint32_t)&spi->SERCOM_DATA;
	dma_blk.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;

	retval = dma_config(cfg->spi_dma.dma_dev, cfg->spi_dma.tx_dma_channel, &dma_cfg);

	if (retval != 0) {
		return retval;
	}

	retval = dma_start(cfg->spi_dma.dma_dev, cfg->spi_dma.tx_dma_channel);

	return retval;
}

static int spi_dma_rx_load(const struct device *dev, uint8_t *buf, size_t len)
{
	const struct spi_mchp_dev_config *cfg = dev->config;
	const struct mchp_spi_reg_config *spi_reg_cfg = &cfg->reg_cfg;
	struct spi_mchp_dev_data *data = dev->data;
	sercom_spi_registers_t *spi = SPI_GET_BASE_ADDR(spi_reg_cfg->regs, SPI_OP_MODE_MASTER);

	struct dma_config dma_cfg = {0};
	struct dma_block_config dma_blk = {0};
	int retval;

	dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
	dma_cfg.source_data_size = 1;
	dma_cfg.dest_data_size = 1;
	dma_cfg.user_data = data;
	dma_cfg.dma_callback = spi_dma_rx_done;
	dma_cfg.block_count = 1;
	dma_cfg.head_block = &dma_blk;
	dma_cfg.dma_slot = cfg->spi_dma.rx_dma_request;

	dma_blk.block_size = len;

	if (buf != NULL) {
		dma_blk.dest_address = (uint32_t)buf;
	} else {
		/* Use a static dummy variable if no buffer is provided */
		static uint8_t dummy_data;

		dma_blk.dest_address = (uint32_t)&dummy_data;
		dma_blk.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	}

	dma_blk.source_address = (uint32_t)&spi->SERCOM_DATA;
	dma_blk.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;

	retval = dma_config(cfg->spi_dma.dma_dev, cfg->spi_dma.rx_dma_channel, &dma_cfg);
	if (retval != 0) {
		return retval;
	}

	retval = dma_start(cfg->spi_dma.dma_dev, cfg->spi_dma.rx_dma_channel);

	return retval;
}

static bool spi_dma_select_segment(const struct device *dev)
{
	struct spi_mchp_dev_data *data = dev->data;
	uint32_t segment_len;

	/* Pick the shorter buffer of ones that have an actual length */
	if (data->ctx.rx_len != 0) {
		segment_len = data->ctx.rx_len;
		if (data->ctx.tx_len != 0) {
			segment_len = MIN(segment_len, data->ctx.tx_len);
		}
	} else {
		segment_len = data->ctx.tx_len;
	}

	if (segment_len == 0) {
		return false;
	}

	/* Ensure the segment length does not exceed the max allowed value
	 */
	segment_len = MIN(segment_len, SPI_MCHP_MAX_XFER_SIZE);

	data->dma_segment_len = segment_len;

	return true;
}

static int spi_dma_setup_buffers(const struct device *dev)
{
	struct spi_mchp_dev_data *data = dev->data;
	int retval;

	if (data->dma_segment_len == 0) {
		return -EINVAL;
	}

	/* Load receive buffer first to prepare for incoming data */
	if (data->ctx.rx_len != 0U) {
		retval = spi_dma_rx_load(dev, data->ctx.rx_buf, data->dma_segment_len);
	} else {
		retval = spi_dma_rx_load(dev, NULL, data->dma_segment_len);
	}

	if (retval != 0) {
		return retval;
	}

	/* Load transmit buffer, which starts SPI bus clocking */
	if (data->ctx.tx_len != 0U) {
		retval = spi_dma_tx_load(dev, data->ctx.tx_buf, data->dma_segment_len);
	} else {
		retval = spi_dma_tx_load(dev, NULL, data->dma_segment_len);
	}

	if (retval != 0) {
		return retval;
	}

	return 0;
}

static void spi_dma_rx_done(const struct device *dma_dev, void *arg, uint32_t id, int error_code)
{
	struct spi_mchp_dev_data *data = arg;
	const struct device *dev = data->dev;
	const struct spi_mchp_dev_config *cfg = dev->config;
	int retval;

	ARG_UNUSED(id);
	ARG_UNUSED(error_code);

	/* Update TX and RX context with the completed DMA segment */
	spi_context_update_tx(&data->ctx, 1, data->dma_segment_len);
	spi_context_update_rx(&data->ctx, 1, data->dma_segment_len);

	/* Check if more segments need to be transferred */
	if (spi_dma_select_segment(dev) == false) {
		if (spi_context_is_slave(&data->ctx) == false) {
			spi_context_cs_control(&data->ctx, false);
		}
		/* Transmission complete */
		spi_context_complete(&data->ctx, dev, 0);

		return;
	}

	/* Load the next DMA segment */
	retval = spi_dma_setup_buffers(dev);
	if (retval != 0) {
		/* Stop DMA and terminate the SPI transaction in case of failure */
		dma_stop(cfg->spi_dma.dma_dev, cfg->spi_dma.tx_dma_channel);
		dma_stop(cfg->spi_dma.dma_dev, cfg->spi_dma.rx_dma_channel);
		if (spi_context_is_slave(&data->ctx) == false) {
			spi_context_cs_control(&data->ctx, false);
		}
		spi_context_complete(&data->ctx, dev, retval);

		return;
	}
}
#endif /* CONFIG_SPI_MCHP_DMA_DRIVEN */

static int spi_check_buf_len(const struct spi_buf_set *buf_set)
{
	if ((buf_set == NULL) || (buf_set->buffers == NULL)) {
		return 0;
	}

	for (size_t i = 0; i < buf_set->count; i++) {
		if (buf_set->buffers[i].len > SPI_MCHP_MAX_XFER_SIZE) {
			LOG_ERR("SPI buffer length (%u) exceeds max allowed (%u)",
				buf_set->buffers[i].len, SPI_MCHP_MAX_XFER_SIZE);

			return -EINVAL;
		}
	}

	return 0;
}

#ifndef CONFIG_SPI_MCHP_DMA_DRIVEN
static int spi_transceive_interrupt(const struct device *dev, const struct spi_config *config,
				    const struct spi_buf_set *tx_bufs,
				    const struct spi_buf_set *rx_bufs)
{
	const struct spi_mchp_dev_config *cfg = dev->config;
	const struct mchp_spi_reg_config *spi_reg_cfg = &cfg->reg_cfg;
	struct spi_mchp_dev_data *const data = dev->data;
	uint8_t tx_data;
	sercom_spi_registers_t *spi = SPI_GET_BASE_ADDR(spi_reg_cfg->regs, SPI_OP_MODE_MASTER);

	/* Prepare first byte for transmission */
	if (spi_context_tx_buf_on(&data->ctx) == true) {
		tx_data = *data->ctx.tx_buf;
	} else {
		tx_data = 0U;
	}

	/*Clear the DATA register until the RXC flag is cleared*/
	if (WAIT_FOR(((spi->SERCOM_INTFLAG & SERCOM_SPI_INTFLAG_RXC_Msk) == 0), TIMEOUT_VALUE_US,
		     ((void)spi->SERCOM_DATA, k_busy_wait(DELAY_US))) == false) {
		LOG_ERR("Timeout while clearing RXC");
	}

	/* Get the dummysize */
	if ((data->ctx.rx_len) > (data->ctx.tx_len)) {
		data->dummysize = (data->ctx.rx_len) - (data->ctx.tx_len);
	} else {
		data->dummysize = 0;
	}

	/* Write first data byte to the SPI data register */
	spi_context_update_tx(&data->ctx, 1, 1);
	spi_write_data(spi_reg_cfg, tx_data);

	/* Enable SPI interrupts for RX, TX completion, and data empty events */
	if (data->ctx.rx_len > 0) {
		/*Enable the Receive Complete Interrupt*/
		spi->SERCOM_INTENSET = SERCOM_SPI_INTENSET_RXC_Msk;
	} else {
		/*Enable the Data Register Empty Interrupt*/
		spi->SERCOM_INTENSET = SERCOM_SPI_INTENSET_DRE_Msk;
	}

	return 0;
}

#if CONFIG_SPI_SLAVE
static void spi_slave_write(const struct device *dev)
{
	const struct spi_mchp_dev_config *cfg = dev->config;
	const struct mchp_spi_reg_config *spi_reg_cfg = &cfg->reg_cfg;
	struct spi_mchp_dev_data *const data = dev->data;
	sercom_spi_registers_t *spi = SPI_GET_BASE_ADDR(spi_reg_cfg->regs, SPI_OP_MODE_SLAVE);

	/* Prepare initial bytes for transmission */
	if (spi_context_tx_buf_on(&data->ctx) == true) {
		while ((spi_context_tx_buf_on(&data->ctx) == true) &&
		       (spi_slave_is_data_reg_empty(spi_reg_cfg) == true)) {
			spi_slave_write_data(spi_reg_cfg, *data->ctx.tx_buf);

			/* Write data byte to the SPI data register */
			spi_context_update_tx(&data->ctx, 1, 1);
		}
	} else {
		if (spi_slave_is_data_reg_empty(spi_reg_cfg) == true) {
			spi_slave_write_data(spi_reg_cfg, 0);
		}
	}
	/*Enable the Data Register Empty Interrupt*/
	spi->SERCOM_INTENSET = SERCOM_SPI_INTENSET_DRE_Msk;
}

static int spi_slave_transceive_interrupt(const struct device *dev, const struct spi_config *config,
					  const struct spi_buf_set *tx_bufs,
					  const struct spi_buf_set *rx_bufs)
{
	const struct spi_mchp_dev_config *cfg = dev->config;
	const struct mchp_spi_reg_config *spi_reg_cfg = &cfg->reg_cfg;
	int ret = 0;
	sercom_spi_registers_t *spi = SPI_GET_BASE_ADDR(spi_reg_cfg->regs, SPI_OP_MODE_SLAVE);

	/* Prepare for transmission */
	spi_slave_write(dev);

	/*Enable the Receive Complete Interrupt*/
	spi->SERCOM_INTENSET = SERCOM_SPI_INTENSET_RXC_Msk;
	/* Enable slave select line interrupt */
	spi->SERCOM_INTENSET = SERCOM_SPI_INTENSET_SSL_Msk;

	return ret;
}
#endif /* CONFIG_SPI_SLAVE */
#endif /* CONFIG_SPI_MCHP_DMA_DRIVEN*/

static int spi_transfer(const struct device *dev, const struct spi_config *config,
			const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs,
			spi_callback_t spi_callback, void *userdata, bool asynchronous)
{
	struct spi_mchp_dev_data *data = dev->data;
	int retval;

	retval = spi_check_buf_len(tx_bufs);
	if (retval < 0) {
		return retval;
	}

	retval = spi_check_buf_len(rx_bufs);
	if (retval < 0) {
		return retval;
	}

/*
 * Transmit clocks the output, and we use receive to
 * determine when the transmit is done, so we
 * always need both TX and RX DMA channels.
 */
#if CONFIG_SPI_MCHP_DMA_DRIVEN
	const struct spi_mchp_dev_config *cfg = dev->config;

	if (cfg->spi_dma.tx_dma_channel == DMA_CHANNEL_INVALID ||
	    cfg->spi_dma.rx_dma_channel == DMA_CHANNEL_INVALID) {
		return -ENOTSUP;
	}
#endif /* CONFIG_SPI_MCHP_DMA_DRIVEN */

	spi_context_lock(&data->ctx, asynchronous, spi_callback, userdata, config);

	retval = spi_configure(dev, config);
	if (retval != 0) {
		spi_context_release(&data->ctx, retval);

		return retval;
	}

	if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_MASTER) {
		spi_context_cs_control(&data->ctx, true);
	}

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

/* Prepare and start DMA transfers */
#if CONFIG_SPI_MCHP_DMA_DRIVEN
	spi_dma_select_segment(dev);
	retval = spi_dma_setup_buffers(dev);
#else
	if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_MASTER) {
		retval = spi_transceive_interrupt(dev, config, tx_bufs, rx_bufs);
	}
#if CONFIG_SPI_SLAVE
	if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_SLAVE) {
		retval = spi_slave_transceive_interrupt(dev, config, tx_bufs, rx_bufs);
	}
#endif /* CONFIG_SPI_SLAVE */
#endif /* CONFIG_SPI_MCHP_DMA_DRIVEN */

	retval = spi_context_wait_for_completion(&data->ctx);

	if (asynchronous == false) {
		if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_SLAVE) {
			spi_context_release(&data->ctx, retval);

			return -ENOTSUP;
		}

		if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_MASTER) {
			spi_context_cs_control(&data->ctx, false);
		}

		spi_context_release(&data->ctx, retval);
		return retval;
	}

	if (retval != 0) {
#if CONFIG_SPI_MCHP_DMA_DRIVEN
		/* Stop DMA transfers in case of failure */
		dma_stop(cfg->spi_dma.dma_dev, cfg->spi_dma.tx_dma_channel);
		dma_stop(cfg->spi_dma.dma_dev, cfg->spi_dma.rx_dma_channel);
#endif /* CONFIG_SPI_MCHP_DMA_DRIVEN */

		if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_MASTER) {
			spi_context_cs_control(&data->ctx, false);
		}

		spi_context_release(&data->ctx, retval);
	}

	return retval;
}

static int spi_mchp_transceive(const struct device *dev, const struct spi_config *config,
			       const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs)
{
	return spi_transfer(dev, config, tx_bufs, rx_bufs, NULL, NULL, false);
}

#if CONFIG_SPI_ASYNC
static int spi_mchp_transceive_async(const struct device *dev, const struct spi_config *config,
				     const struct spi_buf_set *tx_bufs,
				     const struct spi_buf_set *rx_bufs, spi_callback_t spi_callback,
				     void *userdata)
{
	if (spi_callback == NULL) {
		return -EINVAL;
	} else {
		return spi_transfer(dev, config, tx_bufs, rx_bufs, spi_callback, userdata, true);
	}
}
#endif /*CONFIG_SPI_ASYNC*/

static int spi_mchp_release(const struct device *dev, const struct spi_config *config)
{
	struct spi_mchp_dev_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#ifndef CONFIG_SPI_MCHP_DMA_DRIVEN
#if CONFIG_SPI_SLAVE
static void spi_mchp_isr_slave(const struct device *dev)
{
	struct spi_mchp_dev_data *data = dev->data;
	const struct spi_mchp_dev_config *cfg = dev->config;
	const struct mchp_spi_reg_config *spi_reg_cfg = &cfg->reg_cfg;
	sercom_spi_registers_t *spi = SPI_GET_BASE_ADDR(spi_reg_cfg->regs, SPI_OP_MODE_SLAVE);

	uint8_t intFlag = spi->SERCOM_INTFLAG;
	uint8_t tx_data = 0U;
	uint8_t rx_data = 0U;

	/* Handle slave select */
	if ((spi->SERCOM_INTFLAG & SERCOM_SPI_INTFLAG_SSL_Msk) == SERCOM_SPI_INTFLAG_SSL_Msk) {
		/* Clear the slave select line interrupt */
		spi->SERCOM_INTFLAG = SERCOM_SPI_INTFLAG_SSL_Msk;
		/* Enable the Transmit Complete Interrupt */
		spi->SERCOM_INTENSET = SERCOM_SPI_INTENSET_TXC_Msk;
		return;
	}

	/* Handle buffer overflow error */
	if ((spi->SERCOM_STATUS & SERCOM_SPI_STATUS_BUFOVF_Msk) == SERCOM_SPI_STATUS_BUFOVF_Msk) {
		/* Clear buffer overflow flag */
		spi->SERCOM_STATUS = SERCOM_SPI_STATUS_BUFOVF_Msk;
		/* Clear the DATA register */
		if (WAIT_FOR(((spi->SERCOM_INTFLAG & SERCOM_SPI_INTFLAG_RXC_Msk) == 0),
			     TIMEOUT_VALUE_US, (void)spi->SERCOM_DATA) == false) {
			LOG_ERR("Timeout while clearing RXC");
		}
		/*Clear the Error Interrupt Flag */
		spi->SERCOM_INTFLAG = (uint8_t)SERCOM_SPI_INTFLAG_ERROR_Msk;
		spi_context_complete(&data->ctx, dev, -EIO);
		return;
	}

	if ((spi->SERCOM_INTFLAG & SERCOM_SPI_INTFLAG_RXC_Msk) == SERCOM_SPI_INTFLAG_RXC_Msk) {
		/* Handle received data */
		rx_data = (uint8_t)spi->SERCOM_DATA;
		if (spi_context_rx_buf_on(&data->ctx)) {
			*data->ctx.rx_buf = rx_data;
			spi_context_update_rx(&data->ctx, 1, 1);
		}
	}

	/* Handle transmit data */
	if (spi_slave_is_data_reg_empty(spi_reg_cfg) == true) {
		if (spi_context_tx_on(&data->ctx) == true) {
			tx_data = *data->ctx.tx_buf;
			spi_context_update_tx(&data->ctx, 1, 1);
		} else {
			tx_data = 0x00;
			/*Disable DRE interrupt*/
			spi->SERCOM_INTENCLR = (uint8_t)SERCOM_SPI_INTENCLR_DRE_Msk;
		}
		spi_slave_write_data(spi_reg_cfg, tx_data);
	}

	/* Handle transaction complete */
	if ((intFlag & SERCOM_SPI_INTFLAG_TXC_Msk) == SERCOM_SPI_INTFLAG_TXC_Msk) {
		/*Clear transmit complete flag*/
		spi->SERCOM_INTFLAG = SERCOM_SPI_INTFLAG_TXC_Msk;

		/* If both TX and RX are done, complete the transaction */
		if ((spi_context_rx_on(&data->ctx) == false) &&
		    (spi_context_tx_on(&data->ctx) == false)) {
			/*Disable all SPI Interrupts*/
			spi->SERCOM_INTENCLR = SERCOM_SPI_INTENCLR_Msk;
			/*Clear all SPI Interrupt*/
			spi->SERCOM_INTFLAG = SERCOM_SPI_INTFLAG_Msk;
			spi_context_complete(&data->ctx, dev, 0);
		}
	}
}
#endif /* CONFIG_SPI_SLAVE */

static void spi_mchp_isr_master(const struct device *dev)
{
	struct spi_mchp_dev_data *data = dev->data;
	const struct spi_mchp_dev_config *cfg = dev->config;
	const struct mchp_spi_reg_config *spi_reg_cfg = &cfg->reg_cfg;
	sercom_spi_registers_t *spi = SPI_GET_BASE_ADDR(spi_reg_cfg->regs, SPI_OP_MODE_MASTER);

	uint8_t tx_data = 0U;

	if (spi->SERCOM_INTENSET == 0) {
		return;
	}

	uint8_t intflag = spi->SERCOM_INTFLAG;
	bool rx_ready = ((intflag & SERCOM_SPI_INTFLAG_RXC_Msk) != 0U);
	bool tx_ready = ((intflag & SERCOM_SPI_INTFLAG_DRE_Msk) != 0U);
	bool tx_complete = ((intflag & SERCOM_SPI_INTFLAG_TXC_Msk) != 0U);

	bool transmit_needed = (spi_context_tx_on(&data->ctx) == true) || (data->dummysize > 0);
	bool receive_needed = (spi_context_rx_buf_on(&data->ctx) == true) && (rx_ready == true);

	/* 1. Handle buffer overflow error (Early Return) */
	if ((spi->SERCOM_STATUS & SERCOM_SPI_STATUS_BUFOVF_Msk) != 0U) {
		spi->SERCOM_STATUS = SERCOM_SPI_STATUS_BUFOVF_Msk;

		if (WAIT_FOR(((spi->SERCOM_INTFLAG & SERCOM_SPI_INTFLAG_RXC_Msk) == 0),
			     TIMEOUT_VALUE_US, (void)spi->SERCOM_DATA) == false) {
			LOG_ERR("Timeout while clearing RXC");
		}

		spi->SERCOM_INTFLAG = (uint8_t)SERCOM_SPI_INTFLAG_ERROR_Msk;
		spi->SERCOM_INTENCLR = SERCOM_SPI_INTENCLR_RXC_Msk | SERCOM_SPI_INTENCLR_DRE_Msk |
				       SERCOM_SPI_INTENCLR_TXC_Msk;
		spi_context_cs_control(&data->ctx, false);
		spi_context_complete(&data->ctx, dev, -EIO);
		return;
	}

	/* 2. Handle received data */
	if (receive_needed == true) {
		*data->ctx.rx_buf = spi_read_data(spi_reg_cfg);
		spi_context_update_rx(&data->ctx, 1, 1);

		if (spi_context_rx_on(&data->ctx) != true) {
			if (transmit_needed == true) {
				spi->SERCOM_INTENCLR = SERCOM_SPI_INTENCLR_RXC_Msk;
				spi->SERCOM_INTENSET = SERCOM_SPI_INTENSET_DRE_Msk;
			} else {
				/* Receive done and no transmit pending */
				spi->SERCOM_INTFLAG = (uint8_t)SERCOM_SPI_INTFLAG_ERROR_Msk;
				spi->SERCOM_INTENCLR = SERCOM_SPI_INTENCLR_RXC_Msk |
						       SERCOM_SPI_INTENCLR_DRE_Msk |
						       SERCOM_SPI_INTENCLR_TXC_Msk;
				spi_context_cs_control(&data->ctx, false);
				spi_context_complete(&data->ctx, dev, 0);
				return;
			}
		}
	}

	/* 3. Handle transmit data */
	if ((tx_ready == true) && (transmit_needed == true)) {
		if (spi_context_tx_on(&data->ctx) == true) {
			tx_data = *data->ctx.tx_buf;
			spi_context_update_tx(&data->ctx, 1, 1);
		} else {
			tx_data = DUMMY_DATA;
			data->dummysize--;
		}

		if ((data->dummysize == 0) && (spi_context_tx_on(&data->ctx) != true)) {
			spi->SERCOM_INTENCLR = SERCOM_SPI_INTENCLR_DRE_Msk;
			spi->SERCOM_INTENSET = SERCOM_SPI_INTENSET_TXC_Msk;
		}
		spi_write_data(spi_reg_cfg, tx_data);
	}

	/* 4. Handle transmit complete (Final Completion) */
	if (tx_complete == true) {
		spi->SERCOM_INTENCLR = SERCOM_SPI_INTENCLR_TXC_Msk;

		if ((spi_context_rx_on(&data->ctx) != true) &&
		    (spi_context_tx_on(&data->ctx) != true)) {

			spi->SERCOM_INTFLAG = (uint8_t)SERCOM_SPI_INTFLAG_ERROR_Msk;
			spi->SERCOM_INTENCLR = SERCOM_SPI_INTENCLR_RXC_Msk |
					       SERCOM_SPI_INTENCLR_DRE_Msk |
					       SERCOM_SPI_INTENCLR_TXC_Msk;
			spi_context_cs_control(&data->ctx, false);
			spi_context_complete(&data->ctx, dev, 0);
		}
	}
}

static void spi_mchp_isr(const struct device *dev)
{
#if CONFIG_SPI_SLAVE
	struct spi_mchp_dev_data *data = dev->data;

	if (spi_context_is_slave(&data->ctx) == true) {
		spi_mchp_isr_slave(dev);

		return;
	}
#endif /* CONFIG_SPI_SLAVE */

	spi_mchp_isr_master(dev);
}
#endif /* CONFIG_SPI_MCHP_DMA_DRIVEN */

static int spi_mchp_init(const struct device *dev)
{
	int retval;
	const struct spi_mchp_dev_config *cfg = dev->config;
	const struct mchp_spi_reg_config *spi_reg_cfg = &cfg->reg_cfg;
	sercom_spi_registers_t *spi = SPI_GET_BASE_ADDR(spi_reg_cfg->regs, SPI_OP_MODE_MASTER);
	struct spi_mchp_dev_data *const data = dev->data;

	retval = clock_control_on(cfg->spi_clock.clock_dev, cfg->spi_clock.gclk_sys);
	if ((retval < 0) && (retval != -EALREADY)) {
		LOG_ERR("Failed to enable the gclk_sys for SPI: %d", retval);

		return retval;
	}

	retval = clock_control_on(cfg->spi_clock.clock_dev, cfg->spi_clock.mclk_sys);
	if ((retval < 0) && (retval != -EALREADY)) {
		LOG_ERR("Failed to enable the mclk_sys for SPI: %d", retval);

		return retval;
	}

	/* Disable all SPI Interrupts*/
	spi->SERCOM_INTENCLR = SERCOM_SPI_INTENCLR_Msk;

	retval = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (retval < 0) {
		LOG_ERR("pinctrl_apply_state Failed for SPI: %d", retval);

		return retval;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(spi, spi_mchp_api) = {
	.transceive = spi_mchp_transceive,

#if CONFIG_SPI_ASYNC
	.transceive_async = spi_mchp_transceive_async,
#endif /*CONFIG_SPI_ASYNC*/

#if CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif /*CONFIG_SPI_RTIO*/

	.release = spi_mchp_release,
};

#define SPI_MCHP_SERCOM_PADS(n)                                                                    \
	SERCOM_SPI_CTRLA_DIPO(DT_INST_PROP(n, dipo)) | SERCOM_SPI_CTRLA_DOPO(DT_INST_PROP(n, dopo))

#define SPI_MCHP_REG_CFG_DEFN(n)                                                                   \
	.reg_cfg.regs = (sercom_registers_t *)DT_INST_REG_ADDR(n),                                 \
	.reg_cfg.pads = SPI_MCHP_SERCOM_PADS(n),

#ifndef CONFIG_SPI_MCHP_DMA_DRIVEN
#if DT_INST_IRQ_HAS_IDX(0, 3)
#define SPI_MCHP_IRQ_HANDLER(n)                                                                    \
	static void spi_mchp_irq_config_##n(const struct device *dev)                              \
	{                                                                                          \
		MCHP_SPI_IRQ_CONNECT(n, 0);                                                        \
		MCHP_SPI_IRQ_CONNECT(n, 1);                                                        \
		MCHP_SPI_IRQ_CONNECT(n, 2);                                                        \
		MCHP_SPI_IRQ_CONNECT(n, 3);                                                        \
	}
#else
#define SPI_MCHP_IRQ_HANDLER(n)                                                                    \
	static void spi_mchp_irq_config_##n(const struct device *dev)                              \
	{                                                                                          \
		MCHP_SPI_IRQ_CONNECT(n, 0);                                                        \
	}
#endif
#else
#define SPI_MCHP_IRQ_HANDLER(n)
#endif /* CONFIG_SPI_MCHP_DMA_DRIVEN  */

#define SPI_MCHP_CLOCK_DEFN(n)                                                                     \
	.spi_clock.clock_dev = DEVICE_DT_GET(DT_NODELABEL(clock)),                                 \
	.spi_clock.mclk_sys = (void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, mclk, subsystem)),           \
	.spi_clock.gclk_sys = (void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, gclk, subsystem))

#ifndef CONFIG_SPI_MCHP_DMA_DRIVEN
#define MCHP_SPI_IRQ_CONNECT(n, m)                                                                 \
	do {                                                                                       \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, m, irq), DT_INST_IRQ_BY_IDX(n, m, priority),     \
			    spi_mchp_isr, DEVICE_DT_INST_GET(n), 0);                               \
		irq_enable(DT_INST_IRQ_BY_IDX(n, m, irq));                                         \
	} while (false)

#define SPI_MCHP_IRQ_HANDLER_DECL(n) static void spi_mchp_irq_config_##n(const struct device *dev)
#define SPI_MCHP_IRQ_HANDLER_FUNC(n) .irq_config_func = spi_mchp_irq_config_##n,
#else
#define SPI_MCHP_IRQ_HANDLER_DECL(n)
#define SPI_MCHP_IRQ_HANDLER_FUNC(n)
#endif /* CONFIG_SPI_MCHP_DMA_DRIVEN  */

#if CONFIG_SPI_MCHP_DMA_DRIVEN
#define SPI_MCHP_DMA_CHANNELS(n)                                                                   \
	.spi_dma.dma_dev = DEVICE_DT_GET(MCHP_DT_INST_DMA_CTLR(n, tx)),                            \
	.spi_dma.tx_dma_request = MCHP_DT_INST_DMA_TRIGSRC(n, tx),                                 \
	.spi_dma.tx_dma_channel = MCHP_DT_INST_DMA_CHANNEL(n, tx),                                 \
	.spi_dma.rx_dma_request = MCHP_DT_INST_DMA_TRIGSRC(n, rx),                                 \
	.spi_dma.rx_dma_channel = MCHP_DT_INST_DMA_CHANNEL(n, rx),
#else
#define SPI_MCHP_DMA_CHANNELS(n)
#endif /* CONFIG_SPI_MCHP_DMA_DRIVEN */

#define SPI_MCHP_CONFIG_DEFN(n)                                                                    \
	static const struct spi_mchp_dev_config spi_mchp_config_##n = {                            \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		SPI_MCHP_REG_CFG_DEFN(n) SPI_MCHP_IRQ_HANDLER_FUNC(n) SPI_MCHP_DMA_CHANNELS(n)     \
			SPI_MCHP_CLOCK_DEFN(n)}

#define SPI_MCHP_DEVICE_INIT(n)                                                                    \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	SPI_MCHP_IRQ_HANDLER_DECL(n);                                                              \
	SPI_MCHP_CONFIG_DEFN(n);                                                                   \
	static struct spi_mchp_dev_data spi_mchp_data_##n = {                                      \
		SPI_CONTEXT_INIT_LOCK(spi_mchp_data_##n, ctx),                                     \
		SPI_CONTEXT_INIT_SYNC(spi_mchp_data_##n, ctx),                                     \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)};                             \
	DEVICE_DT_INST_DEFINE(n, spi_mchp_init, NULL, &spi_mchp_data_##n, &spi_mchp_config_##n,    \
			      POST_KERNEL, CONFIG_SPI_INIT_PRIORITY, &spi_mchp_api);               \
	SPI_MCHP_IRQ_HANDLER(n)

DT_INST_FOREACH_STATUS_OKAY(SPI_MCHP_DEVICE_INIT)
