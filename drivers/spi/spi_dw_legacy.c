/* spi_dw.c - Designware SPI driver implementation */

/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <kernel.h>
#include <arch/cpu.h>

#include <misc/__assert.h>
#include <board.h>
#include <device.h>
#include <init.h>

#include <sys_io.h>
#include <clock_control.h>
#include <misc/util.h>

#include <spi.h>
#include "spi_dw.h"

#ifdef CONFIG_IOAPIC
#include <drivers/ioapic.h>
#endif

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

static void completed(struct device *dev, int error)
{
	const struct spi_dw_config *info = dev->config->config_info;
	struct spi_dw_data *spi = dev->driver_data;

	if (error) {
		goto out;
	}

	/*
	* There are several situations here.
	* 1. spi_write w rx_buf - need last_tx && rx_buf_len zero to be done.
	* 2. spi_write w/o rx_buf - only need to determine when write is done.
	* 3. spi_read - need rx_buf_len zero.
	*/
	if (spi->tx_buf && spi->rx_buf) {
		if (!spi->last_tx || spi->rx_buf_len) {
			return;
		}
	} else if (spi->tx_buf) {
		if (!spi->last_tx) {
			return;
		}
	} else { /* or, spi->rx_buf!=0 */
		if (spi->rx_buf_len) {
			return;
		}
	}

out:
	/* need to give time for FIFOs to drain before issuing more commands */
	while (test_bit_sr_busy(info->regs)) {
	}

	spi->error = error;

	/* Disabling interrupts */
	write_imr(DW_SPI_IMR_MASK, info->regs);
	/* Disabling the controller */
	clear_bit_ssienr(info->regs);

	_spi_control_cs(dev, 0);

	SYS_LOG_DBG("SPI transaction completed %s error",
		    error ? "with" : "without");

	k_sem_give(&spi->device_sync_sem);
}

static void push_data(struct device *dev)
{
	const struct spi_dw_config *info = dev->config->config_info;
	struct spi_dw_data *spi = dev->driver_data;
	u32_t data = 0;
	u32_t f_tx;
	DBG_COUNTER_INIT();

	if (spi->rx_buf) {
		f_tx = DW_SPI_FIFO_DEPTH - read_txflr(info->regs) -
					read_rxflr(info->regs);
		if ((int)f_tx < 0) {
			f_tx = 0; /* if rx-fifo is full, hold off tx */
		}
	} else {
		f_tx = DW_SPI_FIFO_DEPTH - read_txflr(info->regs);
	}

	if (f_tx && (spi->tx_buf_len == 0)) {
		/* room in fifo, yet nothing to send */
		spi->last_tx = 1; /* setting last_tx indicates TX is done */
	}

	while (f_tx) {
		if (spi->tx_buf && spi->tx_buf_len > 0) {
			switch (spi->dfs) {
			case 1:
				data = UNALIGNED_GET((u8_t *)(spi->tx_buf));
				break;
			case 2:
				data = UNALIGNED_GET((u16_t *)(spi->tx_buf));
				break;
#ifndef CONFIG_ARC
			case 4:
				data = UNALIGNED_GET((u32_t *)(spi->tx_buf));
				break;
#endif
			}

			spi->tx_buf += spi->dfs;
			spi->tx_buf_len--;
		} else if (spi->rx_buf && spi->rx_buf_len > 0) {
			/* No need to push more than necessary */
			if (spi->rx_buf_len - spi->fifo_diff <= 0) {
				break;
			}

			data = 0;
		} else {
			/* Nothing to push anymore */
			break;
		}

		write_dr(data, info->regs);
		f_tx--;
		spi->fifo_diff++;
		DBG_COUNTER_INC();
	}

	if (spi->last_tx) {
		write_txftlr(0, info->regs);
		/* prevents any further interrupts demanding TX fifo fill */
	}

	SYS_LOG_DBG("Pushed: %d", DBG_COUNTER_RESULT());
}

static void pull_data(struct device *dev)
{
	const struct spi_dw_config *info = dev->config->config_info;
	struct spi_dw_data *spi = dev->driver_data;
	u32_t data = 0;
	DBG_COUNTER_INIT();

	while (read_rxflr(info->regs)) {
		data = read_dr(info->regs);
		DBG_COUNTER_INC();

		if (spi->rx_buf && spi->rx_buf_len > 0) {
			switch (spi->dfs) {
			case 1:
				UNALIGNED_PUT(data, (u8_t *)spi->rx_buf);
				break;
			case 2:
				UNALIGNED_PUT(data, (u16_t *)spi->rx_buf);
				break;
#ifndef CONFIG_ARC
			case 4:
				UNALIGNED_PUT(data, (u32_t *)spi->rx_buf);
				break;
#endif
			}

			spi->rx_buf += spi->dfs;
			spi->rx_buf_len--;
		}

		spi->fifo_diff--;
	}

	if (!spi->rx_buf_len && spi->tx_buf_len < DW_SPI_FIFO_DEPTH) {
		write_rxftlr(spi->tx_buf_len - 1, info->regs);
	} else if (read_rxftlr(info->regs) >= spi->rx_buf_len) {
		write_rxftlr(spi->rx_buf_len - 1, info->regs);
	}

	SYS_LOG_DBG("Pulled: %d", DBG_COUNTER_RESULT());
}

static inline bool _spi_dw_is_controller_ready(struct device *dev)
{
	const struct spi_dw_config *info = dev->config->config_info;

	if (test_bit_ssienr(info->regs) || test_bit_sr_busy(info->regs)) {
		return false;
	}

	return true;
}

static int spi_dw_configure(struct device *dev,
			    struct spi_config *config)
{
	const struct spi_dw_config *info = dev->config->config_info;
	struct spi_dw_data *spi = dev->driver_data;
	u32_t flags = config->config;
	u32_t ctrlr0 = 0;
	u32_t mode;

	SYS_LOG_DBG("%p (0x%x), %p", dev, info->regs, config);

	/* Check status */
	if (!_spi_dw_is_controller_ready(dev)) {
		SYS_LOG_DBG("Controller is busy");
		return -EBUSY;
	}

	/* Word size */
	ctrlr0 |= DW_SPI_CTRLR0_DFS(SPI_WORD_SIZE_GET(flags));

	/* Determine how many bytes are required per-frame */
	spi->dfs = SPI_WS_TO_DFS(SPI_WORD_SIZE_GET(flags));

	/* SPI mode */
	mode = SPI_MODE(flags);
	if (mode & SPI_MODE_CPOL) {
		ctrlr0 |= DW_SPI_CTRLR0_SCPOL;
	}

	if (mode & SPI_MODE_CPHA) {
		ctrlr0 |= DW_SPI_CTRLR0_SCPH;
	}

	if (mode & SPI_MODE_LOOP) {
		ctrlr0 |= DW_SPI_CTRLR0_SRL;
	}

	/* Installing the configuration */
	write_ctrlr0(ctrlr0, info->regs);

	/*
	 * Configure the rate. Use this small hack to allow the user to call
	 * spi_configure() with both a divider (as the driver was initially
	 * written) and a frequency (as the SPI API suggests to). The clock
	 * divider is a 16bit value, hence we can fairly, and safely, assume
	 * that everything above this value is a frequency. The trade-off is
	 * that if one wants to use a bus frequency of 64kHz (or less), it has
	 * the use a divider...
	 */
	if (config->max_sys_freq > 0xffff) {
		write_baudr(SPI_DW_CLK_DIVIDER(config->max_sys_freq),
			    info->regs);
	} else {
		write_baudr(config->max_sys_freq, info->regs);
	}

	return 0;
}

static int spi_dw_slave_select(struct device *dev, u32_t slave)
{
	struct spi_dw_data *spi = dev->driver_data;

	SYS_LOG_DBG("%p %d", dev, slave);

	if (slave == 0 || slave > 16) {
		return -EINVAL;
	}

	spi->slave = 1 << (slave - 1);

	return 0;
}

static int spi_dw_transceive(struct device *dev,
			     const void *tx_buf, u32_t tx_buf_len,
			     void *rx_buf, u32_t rx_buf_len)
{
	const struct spi_dw_config *info = dev->config->config_info;
	struct spi_dw_data *spi = dev->driver_data;
	u32_t rx_thsld = DW_SPI_RXFTLR_DFLT;
	u32_t imask;

	SYS_LOG_DBG("%p, %p, %u, %p, %u",
		    dev, tx_buf, tx_buf_len, rx_buf, rx_buf_len);

	/* Check status */
	if (!_spi_dw_is_controller_ready(dev)) {
		SYS_LOG_DBG("Controller is busy");
		return -EBUSY;
	}

	/* Set buffers info */
	spi->tx_buf = tx_buf;
	spi->tx_buf_len = tx_buf_len/spi->dfs;
	spi->rx_buf = rx_buf;
	if (rx_buf) {
		spi->rx_buf_len = rx_buf_len/spi->dfs;
	} else {
		spi->rx_buf_len = 0; /* must be zero if no buffer */
	}
	spi->fifo_diff = 0;
	spi->last_tx = 0;

	/* Tx Threshold */
	write_txftlr(DW_SPI_TXFTLR_DFLT, info->regs);

	/* Does Rx thresholds needs to be lower? */
	if (spi->rx_buf_len && spi->rx_buf_len < DW_SPI_FIFO_DEPTH) {
		rx_thsld = spi->rx_buf_len - 1;
	} else if (!spi->rx_buf_len && spi->tx_buf_len < DW_SPI_FIFO_DEPTH) {
		rx_thsld = spi->tx_buf_len - 1;
		/* TODO: why? */
	}

	write_rxftlr(rx_thsld, info->regs);

	/* Slave select */
	write_ser(spi->slave, info->regs);

	_spi_control_cs(dev, 1);

	/* Enable interrupts */
	imask = DW_SPI_IMR_UNMASK;
	if (!rx_buf) {
		/* if there is no rx buffer, keep all rx interrupts masked */
		imask &= DW_SPI_IMR_MASK_RX;
	}

	write_imr(imask, info->regs);

	/* Enable the controller */
	set_bit_ssienr(info->regs);

	k_sem_take(&spi->device_sync_sem, K_FOREVER);

	if (spi->error) {
		spi->error = 0;
		return -EIO;
	}

	return 0;
}

void spi_dw_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct spi_dw_config *info = dev->config->config_info;
	u32_t error = 0;
	u32_t int_status;

	int_status = read_isr(info->regs);

	SYS_LOG_DBG("SPI int_status 0x%x - (tx: %d, rx: %d)",
		    int_status, read_txflr(info->regs), read_rxflr(info->regs));

	if (int_status & DW_SPI_ISR_ERRORS_MASK) {
		error = 1;
		goto out;
	}

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
	.configure = spi_dw_configure,
	.slave_select = spi_dw_slave_select,
	.transceive = spi_dw_transceive,
};

int spi_dw_init(struct device *dev)
{
	const struct spi_dw_config *info = dev->config->config_info;
	struct spi_dw_data *spi = dev->driver_data;

	_clock_config(dev);
	_clock_on(dev);

	info->config_func();

	k_sem_init(&spi->device_sync_sem, 0, UINT_MAX);

	_spi_config_cs(dev);

	/* Masking interrupt and making sure controller is disabled */
	write_imr(DW_SPI_IMR_MASK, info->regs);
	clear_bit_ssienr(info->regs);

	SYS_LOG_DBG("Designware SPI driver initialized on device: %p", dev);

	return 0;
}


#ifdef CONFIG_SPI_0
void spi_config_0_irq(void);

struct spi_dw_data spi_dw_data_port_0;

const struct spi_dw_config spi_dw_config_0 = {
	.regs = SPI_DW_PORT_0_REGS,
#ifdef CONFIG_SPI_DW_CLOCK_GATE
	.clock_data = UINT_TO_POINTER(CONFIG_SPI_0_CLOCK_GATE_SUBSYS),
#endif /* CONFIG_SPI_DW_CLOCK_GATE */
#ifdef CONFIG_SPI_DW_CS_GPIO
	.cs_gpio_name = CONFIG_SPI_0_CS_GPIO_PORT,
	.cs_gpio_pin = CONFIG_SPI_0_CS_GPIO_PIN,
#endif
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

struct spi_dw_data spi_dw_data_port_1;

static const struct spi_dw_config spi_dw_config_1 = {
	.regs = SPI_DW_PORT_1_REGS,
#ifdef CONFIG_SPI_DW_CLOCK_GATE
	.clock_data = UINT_TO_POINTER(CONFIG_SPI_1_CLOCK_GATE_SUBSYS),
#endif /* CONFIG_SPI_DW_CLOCK_GATE */
#ifdef CONFIG_SPI_DW_CS_GPIO
	.cs_gpio_name = CONFIG_SPI_1_CS_GPIO_PORT,
	.cs_gpio_pin = CONFIG_SPI_1_CS_GPIO_PIN,
#endif
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
