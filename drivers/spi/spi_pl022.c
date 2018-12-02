/*
 * Copyright (c) 2018 Zilogic Systems.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME spi_pl022
#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <errno.h>
#include <device.h>
#include <spi.h>
#include <soc.h>
#include <string.h>

#ifdef CONFIG_CLOCK_CONTROL_STELLARIS
#include <clock_control/stellaris_clock_control.h>
#endif

#include <clock_control.h>

#include "spi_pl022.h"

static void sys_set_bits(u32_t address,
			 u32_t mask, u32_t shift, u32_t data)
{
	u32_t temp;

	temp = sys_read32(address);
	temp &= ~(mask << shift);
	temp |= (data & mask) << shift;
	sys_write32(temp, address);
}

static void rx_buffer_flush(u32_t base)
{
	u32_t stat_reg = SPI_REG_ADDR(base, SPI_SR_OFFSET);

	/* Read data until Rx FIFO is Empty */
	while (sys_test_bit(stat_reg, RX_NOEMPTY)) {
		sys_read32(SPI_REG_ADDR(base, SPI_DR_OFFSET));
	}
}

static void wait_for_sync(u32_t base)
{
	u32_t stat_reg = SPI_REG_ADDR(base, SPI_SR_OFFSET);

	/* wait until busy flag is clear */
	while (sys_test_bit(stat_reg, BUSY)) {
	}
}

static bool spi_transfer_ongoing(struct device *dev)
{
	struct spi_pl022_data *data = dev->driver_data;

	return ((spi_context_tx_on(&data->ctx) ||
		 spi_context_rx_on(&data->ctx)));
}

static void pull_data(struct device *dev)
{
	const struct spi_pl022_config *cfg = dev->config->config_info;
	struct spi_pl022_data *data = dev->driver_data;
	u32_t base = cfg->base;
	u16_t value;

	/* Check Rx FIFO is Not Empty */
	while (sys_test_bit(SPI_REG_ADDR(base, SPI_SR_OFFSET), RX_NOEMPTY)) {
		value = sys_read16(SPI_REG_ADDR(base, SPI_DR_OFFSET));

		if (spi_context_rx_buf_on(&data->ctx)) {
			UNALIGNED_PUT(value, (u8_t *)data->ctx.rx_buf);
		}

		spi_context_update_rx(&data->ctx, 1, 1);
	}
}

static void push_data(struct device *dev)
{
	const struct spi_pl022_config *cfg = dev->config->config_info;
	struct spi_pl022_data *data = dev->driver_data;
	u32_t base = cfg->base;
	u16_t value;

	/* Check Tx FIFO is Not Full and Hold, if Rx FIFO Full */
	if (sys_test_bit(SPI_REG_ADDR(base, SPI_SR_OFFSET), TX_NOFULL) &&
	    !sys_test_bit(SPI_REG_ADDR(base, SPI_SR_OFFSET), RX_FULL)) {
		value = 0;

		if (spi_context_tx_buf_on(&data->ctx)) {
			value = UNALIGNED_GET((u8_t *)(data->ctx.tx_buf));
		}

		sys_write16(value, SPI_REG_ADDR(base, SPI_DR_OFFSET));
		spi_context_update_tx(&data->ctx, 1, 1);
	}

	/* Disabling Interrupt when no data to Transmit */
	if (!spi_context_tx_on(&data->ctx)) {
		sys_clear_bit(SPI_REG_ADDR(base, SPI_IMSC_OFFSET), SPI_TXMIS);
	}
}

static void set_frequency(struct device *dev, u32_t freq)
{
	const struct spi_pl022_config *cfg = dev->config->config_info;
	u32_t base = cfg->base;
	u32_t prescale;
	u32_t clk_rate;
	u32_t spi_clock;

	/* Get periperal clock frequency */
	clock_control_get_rate(device_get_binding(DT_CLOCK_CONTROL_LABEL),
			       (clock_control_subsys_t) &cfg->pclk, &spi_clock);

	for (prescale = CLK_PRESCALE_MIN;
	     prescale <= CLK_PRESCALE_MAX; prescale += 2) {
		for (clk_rate = SERIAL_CLKRATE_MIN;
		     clk_rate <= SERIAL_CLKRATE_MAX; clk_rate++) {
			if (freq >= (spi_clock / (prescale * clk_rate))) {
				break;
			}
		}
	}

	/* Set prescale */
	sys_set_bits(SPI_REG_ADDR(base, SPI_CPSR_OFFSET), BYTE_MASK,
		     CLK_PRESCALE_SHIFT, prescale);

	/* Set clkrate */
	sys_set_bits(SPI_REG_ADDR(base, SPI_CR0_OFFSET), BYTE_MASK,
		     SERIAL_CLKRATE_SHIFT, clk_rate);
}

static void completed(struct device *dev, u32_t error)
{
	const struct spi_pl022_config *cfg = dev->config->config_info;
	struct spi_pl022_data *data = dev->driver_data;
	u32_t base = cfg->base;

	if (error) {
		goto out;
	}

	if (spi_transfer_ongoing(dev)) {
		return;
	}
out:
	/* Disabling Interrupts */
	sys_write32(0, SPI_REG_ADDR(base, SPI_IMSC_OFFSET));

	/* Disabling SSP */
	sys_clear_bit(SPI_REG_ADDR(base, SPI_CR1_OFFSET), SSE);

	spi_context_cs_control(&data->ctx, false);

	LOG_DBG("SPI transaction complete %s error",
		    error ? "with" : "without");

	spi_context_complete(&data->ctx, error ? -EIO : 0);
}

static void spi_pl022_isr(struct device *dev)
{
	const struct spi_pl022_config *cfg = dev->config->config_info;
	u32_t base = cfg->base;
	u32_t error = 0;
	u32_t status;

	status = sys_read32(SPI_REG_ADDR(base, SPI_MIS_OFFSET));

	if (status & SPI_INT_MASK(SPI_RORMIS)) {
		sys_set_bit(SPI_REG_ADDR(base, SPI_ICR_OFFSET), SPI_RORMIS);
		error = 1;
		goto out;
	}

	if (status & SPI_INT_MASK(SPI_TXMIS)) {
		push_data(dev);
		pull_data(dev);
	}
out:
	completed(dev, error);
}

static int spi_pl022_configure(struct device *dev,
				 const struct spi_config *config)
{
	const struct spi_pl022_config *cfg = dev->config->config_info;
	struct spi_pl022_data *data = dev->driver_data;
	u32_t base = cfg->base;

	if (spi_context_configured(&data->ctx, config)) {
		/* Nothing to do */
		return 0;
	}

	if (SPI_OP_MODE_GET(config->operation) != SPI_OP_MODE_MASTER) {
		/* Slave mode is not implemented. */
		return -ENOTSUP;
	}

	/* Setting SSP in Master Mode */
	sys_clear_bit(SPI_REG_ADDR(base, SPI_CR1_OFFSET), MODE);

	/* Setting Frame Format to SPI */
	sys_set_bits(SPI_REG_ADDR(base, SPI_CR0_OFFSET),
		     TWOBIT_MASK, FRF_SHIFT, FRF_SPI);

	if ((config->operation & SPI_MODE_CPOL) != 0) {
		/* Set CPOL if configured */
		sys_set_bit(SPI_REG_ADDR(base, SPI_CR0_OFFSET), CPOL);
	}

	if ((config->operation & SPI_MODE_CPHA) != 0) {
		/* Set CPHA if configured */
		sys_set_bit(SPI_REG_ADDR(base, SPI_CR0_OFFSET), CPHA);
	}

	if ((config->operation & SPI_MODE_LOOP) != 0) {
		/* Setting SSP to Loopback Mode */
		sys_set_bit(SPI_REG_ADDR(base, SPI_CR1_OFFSET), LOOPBACK);
	}

	if (SPI_WORD_SIZE_GET(config->operation) != 8) {
		return -ENOTSUP;
	}

	/* 8 bits frame per transfer */
	sys_set_bits(SPI_REG_ADDR(base, SPI_CR0_OFFSET), NIBBLE_MASK,
		     DSS_SHIFT, DSS_8BITS);

	/* Setting the configured frequency */
	set_frequency(dev, config->frequency);

	/* At this point, it's mandatory to set this on the context! */
	data->ctx.config = config;

	spi_context_cs_configure(&data->ctx);

	return 0;
}

static int transceive(struct device *dev,
		      const struct spi_config *config,
		      const struct spi_buf_set *tx_bufs,
		      const struct spi_buf_set *rx_bufs,
		      bool asynchronous,
		      struct k_poll_signal *signal)
{
	const struct spi_pl022_config *cfg = dev->config->config_info;
	struct spi_pl022_data *data = dev->driver_data;
	u32_t base = cfg->base;
	int err;

	spi_context_lock(&data->ctx, asynchronous, signal);

	/* Wait for previous transfer complete */
	wait_for_sync(base);

	rx_buffer_flush(base);

	err = spi_pl022_configure(dev, config);
	if (err != 0) {
		goto done;
	}

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

	spi_context_cs_control(&data->ctx, true);

	/* Set SSE bit, enable SSP */
	sys_set_bit(SPI_REG_ADDR(base, SPI_CR1_OFFSET), SSE);

	/* Enabling Interrupts */
	sys_write32(0xF, SPI_REG_ADDR(base, SPI_IMSC_OFFSET));

	err = spi_context_wait_for_completion(&data->ctx);
done:
	spi_context_release(&data->ctx, err);
	return err;
}

static int spi_pl022_transceive(struct device *dev,
				const struct spi_config *config,
				const struct spi_buf_set *tx_bufs,
				const struct spi_buf_set *rx_bufs)

{
	return transceive(dev, config, tx_bufs, rx_bufs, false, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int spi_pl022_transceive_async(struct device *dev,
					const struct spi_config *config,
					const struct spi_buf_set *tx_bufs,
					const struct spi_buf_set *rx_bufs,
					struct k_poll_signal *async)
{
	return transceive(dev, config, tx_bufs, rx_bufs, true, async);
}
#endif /* CONFIG_SPI_ASYNC */

static int spi_pl022_release(struct device *dev,
			       const struct spi_config *config)
{
	const struct spi_pl022_config *cfg = dev->config->config_info;
	struct spi_pl022_data *data = dev->driver_data;
	u32_t base = cfg->base;

	/* Wait for previous transfer complete */
	wait_for_sync(base);

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static int spi_pl022_ssp_init(struct device *dev)
{
	const struct spi_pl022_config *cfg = dev->config->config_info;
	struct spi_pl022_data *data = dev->driver_data;

	/* Enabling Power to SSP */
	clock_control_on(device_get_binding(DT_CLOCK_CONTROL_LABEL),
			 (clock_control_subsys_t) &cfg->pclk);

	/* Enabling Interrupt in NVIC */
	cfg->config_func();

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static const struct spi_driver_api spi_pl022_driver_api = {
	.transceive = spi_pl022_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_pl022_transceive_async,
#endif
	.release = spi_pl022_release,
};

#ifdef CONFIG_SPI_0

void spi_pl022_config_ssp0_irq(void);

static const struct spi_pl022_config spi_pl022_ssp0_cfg = {
	.base = DT_SSP0_BASE_ADDRESS,
	.config_func = spi_pl022_config_ssp0_irq,
#ifdef CONFIG_CLOCK_CONTROL_STELLARIS
	.pclk = {
		.bus = DT_SSP0_CLOCK_BUS,
		.en = DT_SSP0_CLOCK_ENABLE
	}
#endif
};

static struct spi_pl022_data spi_pl022_ssp0_data = {
	SPI_CONTEXT_INIT_LOCK(spi_pl022_ssp0_data, ctx),
	SPI_CONTEXT_INIT_SYNC(spi_pl022_ssp0_data, ctx),
};

DEVICE_AND_API_INIT(spi_pl022_ssp0, DT_SSP0_NAME, &spi_pl022_ssp_init,
		    &spi_pl022_ssp0_data, &spi_pl022_ssp0_cfg,
		    POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,
		    &spi_pl022_driver_api);

void spi_pl022_config_ssp0_irq(void)
{
	IRQ_CONNECT(DT_SSP0_IRQ, DT_SSP0_IRQ_PRI,
		    spi_pl022_isr, DEVICE_GET(spi_pl022_ssp0),
		    DT_SSP_IRQ_FLAGS);
	irq_enable(DT_SSP0_IRQ);
}

#endif /* CONFIG_SPI_0 */

#ifdef CONFIG_SPI_1

void spi_pl022_config_ssp1_irq(void);

static const struct spi_pl022_config spi_pl022_ssp1_cfg = {
	.base = DT_SSP1_BASE_ADDRESS,
	.config_func = spi_pl022_config_ssp1_irq,
#ifdef CONFIG_CLOCK_CONTROL_STELLARIS
	.pclk = {
		.bus = DT_SSP1_CLOCK_BUS,
		.en = DT_SSP1_CLOCK_ENABLE
	}
#endif
};

static struct spi_pl022_data spi_pl022_ssp1_data = {
	SPI_CONTEXT_INIT_LOCK(spi_pl022_ssp1_data, ctx),
	SPI_CONTEXT_INIT_SYNC(spi_pl022_ssp1_data, ctx),
};

DEVICE_AND_API_INIT(spi_pl022_ssp1, DT_SSP1_NAME, &spi_pl022_ssp_init,
		    &spi_pl022_ssp1_data, &spi_pl022_ssp1_cfg,
		    POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,
		    &spi_pl022_driver_api);

void spi_pl022_config_ssp1_irq(void)
{
	IRQ_CONNECT(DT_SSP1_IRQ, DT_SSP1_IRQ_PRI,
		    spi_pl022_isr, DEVICE_GET(spi_pl022_ssp1),
		    DT_SSP_IRQ_FLAGS);
	irq_enable(DT_SSP1_IRQ);
}

#endif /* CONFIG_SPI_1 */
