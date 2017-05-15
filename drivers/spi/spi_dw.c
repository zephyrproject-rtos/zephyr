/* spi_dw.c - Designware SPI driver implementation */

/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define SYS_LOG_DOMAIN "SPI DW"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SPI_LEVEL
#include <logging/sys_log.h>

#if (CONFIG_SYS_LOG_SPI_LEVEL == 4)
#define DBG_COUNTER_INIT()	\
	u32_t __cnt = 0
#define DBG_COUNTER_INC()	\
	(__cnt++)
#define DBG_COUNTER_RESULT()	\
	(__cnt)
#else
#define DBG_COUNTER_INIT() {; }
#define DBG_COUNTER_INC() {; }
#define DBG_COUNTER_RESULT() 0
#endif

#include <errno.h>

#include <kernel.h>
#include <arch/cpu.h>

#include <board.h>
#include <device.h>
#include <init.h>

#include <sys_io.h>
#include <clock_control.h>
#include <misc/util.h>

#ifdef CONFIG_IOAPIC
#include <drivers/ioapic.h>
#endif

#include <spi.h>

#include "spi_dw.h"
#include "spi_context.h"

static void completed(struct device *dev, u8_t error)
{
	const struct spi_dw_config *info = dev->config->config_info;
	struct spi_dw_data *spi = dev->driver_data;

	if (error) {
		goto out;
	}

	if (spi_context_tx_on(&spi->ctx) ||
	    spi_context_rx_on(&spi->ctx)) {
		return;
	}

out:
	/* need to give time for FIFOs to drain before issuing more commands */
	while (test_bit_sr_busy(info->regs)) {
	}

	/* Disabling interrupts */
	write_imr(DW_SPI_IMR_MASK, info->regs);
	/* Disabling the controller */
	clear_bit_ssienr(info->regs);

	spi_context_cs_control(&spi->ctx, false);

	SYS_LOG_DBG("SPI transaction completed %s error",
		    error ? "with" : "without");

	spi_context_complete(&spi->ctx, error ? -EIO : 0);
}

static void push_data(struct device *dev)
{
	const struct spi_dw_config *info = dev->config->config_info;
	struct spi_dw_data *spi = dev->driver_data;
	u32_t data = 0;
	u32_t f_tx;

	DBG_COUNTER_INIT();

	if (spi_context_rx_on(&spi->ctx)) {
		f_tx = DW_SPI_FIFO_DEPTH - read_txflr(info->regs) -
			read_rxflr(info->regs);
		if ((int)f_tx < 0) {
			f_tx = 0; /* if rx-fifo is full, hold off tx */
		}
	} else {
		f_tx = DW_SPI_FIFO_DEPTH - read_txflr(info->regs);
	}

	while (f_tx) {
		if (spi_context_tx_buf_on(&spi->ctx)) {
			switch (spi->dfs) {
			case 1:
				data = UNALIGNED_GET((u8_t *)
						     (spi->ctx.tx_buf));
				break;
			case 2:
				data = UNALIGNED_GET((u16_t *)
						     (spi->ctx.tx_buf));
				break;
#ifndef CONFIG_ARC
			case 4:
				data = UNALIGNED_GET((u32_t *)
						     (spi->ctx.tx_buf));
				break;
#endif
			}
		} else if (spi_context_rx_on(&spi->ctx)) {
			/* No need to push more than necessary */
			if ((int)(spi->ctx.rx_len - spi->fifo_diff) <= 0) {
				break;
			}

			data = 0;
		} else {
			/* Nothing to push anymore */
			break;
		}

		write_dr(data, info->regs);

		spi_context_update_tx(&spi->ctx, spi->dfs, 1);
		spi->fifo_diff++;

		f_tx--;

		DBG_COUNTER_INC();
	}

	if (!spi_context_tx_on(&spi->ctx)) {
		/* prevents any further interrupts demanding TX fifo fill */
		write_txftlr(0, info->regs);
	}

	SYS_LOG_DBG("Pushed: %d", DBG_COUNTER_RESULT());
}

static void pull_data(struct device *dev)
{
	const struct spi_dw_config *info = dev->config->config_info;
	struct spi_dw_data *spi = dev->driver_data;

	DBG_COUNTER_INIT();

	while (read_rxflr(info->regs)) {
		u32_t data = read_dr(info->regs);

		DBG_COUNTER_INC();

		if (spi_context_rx_buf_on(&spi->ctx)) {
			switch (spi->dfs) {
			case 1:
				UNALIGNED_PUT(data, (u8_t *)spi->ctx.rx_buf);
				break;
			case 2:
				UNALIGNED_PUT(data, (u16_t *)spi->ctx.rx_buf);
				break;
#ifndef CONFIG_ARC
			case 4:
				UNALIGNED_PUT(data, (u32_t *)spi->ctx.rx_buf);
				break;
#endif
			}
		}

		spi_context_update_rx(&spi->ctx, spi->dfs, 1);
		spi->fifo_diff--;
	}

	if (!spi->ctx.rx_len && spi->ctx.tx_len < DW_SPI_FIFO_DEPTH) {
		write_rxftlr(spi->ctx.tx_len - 1, info->regs);
	} else if (read_rxftlr(info->regs) >= spi->ctx.rx_len) {
		write_rxftlr(spi->ctx.rx_len - 1, info->regs);
	}

	SYS_LOG_DBG("Pulled: %d", DBG_COUNTER_RESULT());
}

static int spi_dw_configure(const struct spi_dw_config *info,
			    struct spi_dw_data *spi,
			    const struct spi_config *config)
{
	u32_t ctrlr0 = 0;

	SYS_LOG_DBG("%p (prev %p)", config, spi->ctx.config);

	if (spi_context_configured(&spi->ctx, config)) {
		/* Nothing to do */
		return 0;
	}

	if (config->operation & (SPI_OP_MODE_SLAVE || SPI_TRANSFER_LSB
				 || SPI_LINES_DUAL || SPI_LINES_QUAD)) {
		return -EINVAL;
	}

	/* Word size */
	ctrlr0 |= DW_SPI_CTRLR0_DFS(SPI_WORD_SIZE_GET(config->operation));

	/* Determine how many bytes are required per-frame */
	spi->dfs = SPI_WS_TO_DFS(SPI_WORD_SIZE_GET(config->operation));

	/* SPI mode */
	if (SPI_MODE_GET(config->operation) & SPI_MODE_CPOL) {
		ctrlr0 |= DW_SPI_CTRLR0_SCPOL;
	}

	if (SPI_MODE_GET(config->operation) & SPI_MODE_CPHA) {
		ctrlr0 |= DW_SPI_CTRLR0_SCPH;
	}

	if (SPI_MODE_GET(config->operation) & SPI_MODE_LOOP) {
		ctrlr0 |= DW_SPI_CTRLR0_SRL;
	}

	/* Installing the configuration */
	write_ctrlr0(ctrlr0, info->regs);

	/* Setting up baud rate */
	write_baudr(SPI_DW_CLK_DIVIDER(config->frequency), info->regs);

	/* Slave select */
	write_ser(config->slave, info->regs);

	/* At this point, it's mandatory to set this on the context! */
	spi->ctx.config = config;

	spi_context_cs_configure(&spi->ctx);

	SYS_LOG_DBG("Installed config %p: freq %uHz (div = %u),"
		    " ws/dfs %u/%u, mode %u/%u/%u, slave %u",
		    config, config->frequency,
		    SPI_DW_CLK_DIVIDER(config->frequency), spi->dfs,
		    SPI_WORD_SIZE_GET(config->operation),
		    (SPI_MODE_GET(config->operation) & SPI_MODE_CPOL) ? 1 : 0,
		    (SPI_MODE_GET(config->operation) & SPI_MODE_CPHA) ? 1 : 0,
		    (SPI_MODE_GET(config->operation) & SPI_MODE_LOOP) ? 1 : 0,
		    config->slave);

	return 0;
}

static uint32_t spi_dw_compute_ndf(const struct spi_buf *rx_bufs,
				   size_t rx_count, u8_t dfs)
{
	u32_t len = 0;

	for (; rx_count; rx_bufs++, rx_count--) {
		if (len > (UINT16_MAX - rx_bufs->len)) {
			goto error;
		}

		len += rx_bufs->len;
	}

	if (len) {
		return (len / dfs) - 1;
	}
error:
	return UINT32_MAX;
}

static int transceive(struct device *dev,
		      const struct spi_config *config,
		      const struct spi_buf_set *tx_bufs,
		      const struct spi_buf_set *rx_bufs,
		      bool asynchronous,
		      struct k_poll_signal *signal)
{
	const struct spi_dw_config *info = dev->config->config_info;
	struct spi_dw_data *spi = dev->driver_data;
	u32_t tmod = DW_SPI_CTRLR0_TMOD_TX_RX;
	u32_t reg_data;
	int ret;

	/* Check status */
	if (test_bit_ssienr(info->regs) || test_bit_sr_busy(info->regs)) {
		SYS_LOG_DBG("Controller is busy");
		return -EBUSY;
	}

	spi_context_lock(&spi->ctx, asynchronous, signal);

	/* Configure */
	ret = spi_dw_configure(info, spi, config);
	if (ret) {
		goto out;
	}

	if (!rx_bufs || !rx_bufs->buffers) {
		tmod = DW_SPI_CTRLR0_TMOD_TX;
	} else if (!tx_bufs || !tx_bufs->buffers) {
		tmod = DW_SPI_CTRLR0_TMOD_RX;
	}

	/* ToDo: add a way to determine EEPROM mode */

	if (tmod >= DW_SPI_CTRLR0_TMOD_RX) {
		reg_data = spi_dw_compute_ndf(rx_bufs->buffers,
					      rx_bufs->count, spi->dfs);
		if (reg_data == UINT32_MAX) {
			ret = -EINVAL;
			goto out;
		}

		write_ctrlr1(reg_data, info->regs);
	} else {
		write_ctrlr1(0, info->regs);
	}

	/* Updating TMOD in CTRLR0 register */
	write_ctrlr0(read_ctrlr0(info->regs) | tmod, info->regs);

	/* Set buffers info */
	spi_context_buffers_setup(&spi->ctx, tx_bufs, rx_bufs, spi->dfs);

	spi->fifo_diff = 0;

	/* Tx Threshold */
	write_txftlr(DW_SPI_TXFTLR_DFLT, info->regs);

	/* Does Rx thresholds needs to be lower? */
	reg_data = DW_SPI_RXFTLR_DFLT;
	if (spi->ctx.rx_len && spi->ctx.rx_len < DW_SPI_FIFO_DEPTH) {
		reg_data = spi->ctx.rx_len - 1;
	}

	/* Rx Threshold */
	write_rxftlr(reg_data, info->regs);

	/* Enable interrupts */
	reg_data = !rx_bufs ?
		DW_SPI_IMR_UNMASK & DW_SPI_IMR_MASK_RX : DW_SPI_IMR_UNMASK;
	write_imr(reg_data, info->regs);

	spi_context_cs_control(&spi->ctx, true);

	/* Enable the controller */
	set_bit_ssienr(info->regs);

	ret = spi_context_wait_for_completion(&spi->ctx);
out:
	spi_context_release(&spi->ctx, ret);

	return ret;
}

static int spi_dw_transceive(struct device *dev,
		      const struct spi_config *config,
			     const struct spi_buf_set *tx_bufs,
			     const struct spi_buf_set *rx_bufs)
{
	SYS_LOG_DBG("%p, %p, %p", dev, tx_bufs, rx_bufs);

	return transceive(dev, config, tx_bufs, rx_bufs, false, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int spi_dw_transceive_async(struct device *dev,
				   const struct spi_config *config,
				   const struct spi_buf_set *tx_bufs,
				   const struct spi_buf_set *rx_bufs,
				   struct k_poll_signal *async)
{
	SYS_LOG_DBG("%p, %p, %p, %p", dev, tx_bufs, rx_bufs, async);

	return transceive(dev, config, tx_bufs, rx_bufs, true, async);
}
#endif /* CONFIG_SPI_ASYNC */

static int spi_dw_release(struct device *dev, const struct spi_config *config)
{
	const struct spi_dw_config *info = dev->config->config_info;
	struct spi_dw_data *spi = dev->driver_data;

	if (!spi_context_configured(&spi->ctx, config) ||
	    test_bit_ssienr(info->regs) || test_bit_sr_busy(info->regs)) {
		return -EBUSY;
	}

	spi_context_unlock_unconditionally(&spi->ctx);

	return 0;
}

void spi_dw_isr(struct device *dev)
{
	const struct spi_dw_config *info = dev->config->config_info;
	u32_t int_status;
	u8_t error;

	int_status = read_isr(info->regs);

	SYS_LOG_DBG("SPI int_status 0x%x - (tx: %d, rx: %d)",
		    int_status, read_txflr(info->regs), read_rxflr(info->regs));

	if (int_status & DW_SPI_ISR_ERRORS_MASK) {
		error = 1;
		goto out;
	}

	error = 0;

	if (int_status & DW_SPI_ISR_RXFIS) {
		pull_data(dev);
	}

	if (int_status & DW_SPI_ISR_TXEIS) {
		push_data(dev);
	}

out:
	clear_interrupts(info->regs);
	completed(dev, error);
}

static const struct spi_driver_api dw_spi_api = {
	.transceive = spi_dw_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_dw_transceive_async,
#endif /* CONFIG_SPI_ASYNC */
	.release = spi_dw_release,
};

int spi_dw_init(struct device *dev)
{
	const struct spi_dw_config *info = dev->config->config_info;
	struct spi_dw_data *spi = dev->driver_data;

	_clock_config(dev);
	_clock_on(dev);

	info->config_func();

	/* Masking interrupt and making sure controller is disabled */
	write_imr(DW_SPI_IMR_MASK, info->regs);
	clear_bit_ssienr(info->regs);

	SYS_LOG_DBG("Designware SPI driver initialized on device: %p", dev);

	spi_context_unlock_unconditionally(&spi->ctx);

	return 0;
}


#ifdef CONFIG_SPI_0
void spi_config_0_irq(void);

struct spi_dw_data spi_dw_data_port_0 = {
	SPI_CONTEXT_INIT_LOCK(spi_dw_data_port_0, ctx),
	SPI_CONTEXT_INIT_SYNC(spi_dw_data_port_0, ctx),
};

const struct spi_dw_config spi_dw_config_0 = {
	.regs = SPI_DW_PORT_0_REGS,
#ifdef CONFIG_SPI_DW_CLOCK_GATE
	.clock_data = UINT_TO_POINTER(CONFIG_SPI_0_CLOCK_GATE_SUBSYS),
#endif /* CONFIG_SPI_DW_CLOCK_GATE */
	.config_func = spi_config_0_irq
};

DEVICE_AND_API_INIT(spi_dw_port_0, CONFIG_SPI_0_NAME, spi_dw_init,
		    &spi_dw_data_port_0, &spi_dw_config_0,
		    POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,
		    &dw_spi_api);

void spi_config_0_irq(void)
{
#ifdef CONFIG_SPI_DW_INTERRUPT_SINGLE_LINE
	IRQ_CONNECT(SPI_DW_PORT_0_IRQ, CONFIG_SPI_0_IRQ_PRI,
		    spi_dw_isr, DEVICE_GET(spi_dw_port_0), SPI_DW_IRQ_FLAGS);
	irq_enable(SPI_DW_PORT_0_IRQ);
	_spi_int_unmask(SPI_DW_PORT_0_INT_MASK);
#else /* SPI_DW_INTERRUPT_SEPARATED_LINES */
	IRQ_CONNECT(IRQ_SPI0_RX_AVAIL, CONFIG_SPI_0_IRQ_PRI,
		    spi_dw_isr, DEVICE_GET(spi_dw_port_0), SPI_DW_IRQ_FLAGS);
	IRQ_CONNECT(IRQ_SPI0_TX_REQ, CONFIG_SPI_0_IRQ_PRI,
		    spi_dw_isr, DEVICE_GET(spi_dw_port_0), SPI_DW_IRQ_FLAGS);
	IRQ_CONNECT(IRQ_SPI0_ERR_INT, CONFIG_SPI_0_IRQ_PRI,
		    spi_dw_isr, DEVICE_GET(spi_dw_port_0), SPI_DW_IRQ_FLAGS);

	irq_enable(IRQ_SPI0_RX_AVAIL);
	irq_enable(IRQ_SPI0_TX_REQ);
	irq_enable(IRQ_SPI0_ERR_INT);

	_spi_int_unmask(SPI_DW_PORT_0_RX_INT_MASK);
	_spi_int_unmask(SPI_DW_PORT_0_TX_INT_MASK);
	_spi_int_unmask(SPI_DW_PORT_0_ERROR_INT_MASK);
#endif
}
#endif /* CONFIG_SPI_0 */
#ifdef CONFIG_SPI_1
void spi_config_1_irq(void);

struct spi_dw_data spi_dw_data_port_1 = {
	SPI_CONTEXT_INIT_LOCK(spi_dw_data_port_1, ctx),
	SPI_CONTEXT_INIT_SYNC(spi_dw_data_port_1, ctx),
};

static const struct spi_dw_config spi_dw_config_1 = {
	.regs = SPI_DW_PORT_1_REGS,
#ifdef CONFIG_SPI_DW_CLOCK_GATE
	.clock_data = UINT_TO_POINTER(CONFIG_SPI_1_CLOCK_GATE_SUBSYS),
#endif /* CONFIG_SPI_DW_CLOCK_GATE */
	.config_func = spi_config_1_irq
};

DEVICE_AND_API_INIT(spi_dw_port_1, CONFIG_SPI_1_NAME, spi_dw_init,
		    &spi_dw_data_port_1, &spi_dw_config_1,
		    POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,
		    &dw_spi_api);

void spi_config_1_irq(void)
{
#ifdef CONFIG_SPI_DW_INTERRUPT_SINGLE_LINE
	IRQ_CONNECT(SPI_DW_PORT_1_IRQ, CONFIG_SPI_1_IRQ_PRI,
		    spi_dw_isr, DEVICE_GET(spi_dw_port_1), SPI_DW_IRQ_FLAGS);
	irq_enable(SPI_DW_PORT_1_IRQ);
	_spi_int_unmask(SPI_DW_PORT_1_INT_MASK);
#else /* SPI_DW_INTERRUPT_SEPARATED_LINES */
	IRQ_CONNECT(IRQ_SPI1_RX_AVAIL, CONFIG_SPI_1_IRQ_PRI,
		    spi_dw_isr, DEVICE_GET(spi_dw_port_1), SPI_DW_IRQ_FLAGS);
	IRQ_CONNECT(IRQ_SPI1_TX_REQ, CONFIG_SPI_1_IRQ_PRI,
		    spi_dw_isr, DEVICE_GET(spi_dw_port_1), SPI_DW_IRQ_FLAGS);
	IRQ_CONNECT(IRQ_SPI1_ERR_INT, CONFIG_SPI_1_IRQ_PRI,
		    spi_dw_isr, DEVICE_GET(spi_dw_port_1), SPI_DW_IRQ_FLAGS);

	irq_enable(IRQ_SPI1_RX_AVAIL);
	irq_enable(IRQ_SPI1_TX_REQ);
	irq_enable(IRQ_SPI1_ERR_INT);

	_spi_int_unmask(SPI_DW_PORT_1_RX_INT_MASK);
	_spi_int_unmask(SPI_DW_PORT_1_TX_INT_MASK);
	_spi_int_unmask(SPI_DW_PORT_1_ERROR_INT_MASK);
#endif
}
#endif /* CONFIG_SPI_1 */
