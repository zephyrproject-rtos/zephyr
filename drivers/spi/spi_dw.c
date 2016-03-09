/* spi_dw.c - Designware SPI driver implementation */

/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <nanokernel.h>
#include <arch/cpu.h>

#include <misc/__assert.h>
#include <board.h>
#include <device.h>
#include <init.h>

#include <sys_io.h>
#include <clock_control.h>
#include <misc/util.h>

#include <spi.h>
#include <spi_dw.h>

#ifdef CONFIG_IOAPIC
#include <drivers/ioapic.h>
#endif

#ifndef CONFIG_SPI_DEBUG
#define DBG(...) {; }
#define DBG_COUNTER_INIT() {; }
#define DBG_COUNTER_INC() {; }
#define DBG_COUNTER_RESULT() {; }
#else
#define DBG_COUNTER_INIT()	\
	uint32_t __cnt = 0
#define DBG_COUNTER_INC()	\
	(__cnt++)
#define DBG_COUNTER_RESULT()	\
	(__cnt)
#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#define DBG printf
#else
#include <misc/printk.h>
#define DBG printk
#endif /* CONFIG_STDOUT_CONSOLE */
#endif /* CONFIG_SPI_DEBUG */

static void completed(struct device *dev, int error)
{
	struct spi_dw_config *info = dev->config->config_info;
	struct spi_dw_data *spi = dev->driver_data;

	if (error) {
		goto out;
	}

	if (spi->fifo_diff ||
	    !((spi->tx_buf && !spi->tx_buf_len && !spi->rx_buf) ||
	      (spi->rx_buf && !spi->rx_buf_len && !spi->tx_buf) ||
	      (spi->tx_buf && !spi->tx_buf_len &&
				spi->rx_buf && !spi->rx_buf_len))) {
		return;
	}

out:
	spi->error = error;

	/* Disabling interrupts */
	write_imr(DW_SPI_IMR_MASK, info->regs);
	/* Disabling the controller */
	clear_bit_ssienr(info->regs);

	_spi_control_cs(dev, 0);

	DBG("SPI transaction completed %s error\n",
	    error ? "with" : "without");

	device_sync_call_complete(&spi->sync);
}

static void push_data(struct device *dev)
{
	struct spi_dw_config *info = dev->config->config_info;
	struct spi_dw_data *spi = dev->driver_data;
	uint32_t data = 0;
	uint32_t f_tx;
	DBG_COUNTER_INIT();

	f_tx = DW_SPI_FIFO_DEPTH - read_txflr(info->regs) -
					read_rxflr(info->regs) - 1;
	while (f_tx) {
		if (spi->tx_buf && spi->tx_buf_len > 0) {
			switch (spi->dfs) {
			case 1:
				data = UNALIGNED_GET((uint8_t *)(spi->tx_buf));
				break;
			case 2:
				data = UNALIGNED_GET((uint16_t *)(spi->tx_buf));
				break;
#ifndef CONFIG_ARC
			case 4:
				data = UNALIGNED_GET((uint32_t *)(spi->tx_buf));
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

	if (!spi->tx_buf_len && !spi->rx_buf_len) {
		write_txftlr(0, info->regs);
	}

	DBG("Pushed: %d\n", DBG_COUNTER_RESULT());
}

static void pull_data(struct device *dev)
{
	struct spi_dw_config *info = dev->config->config_info;
	struct spi_dw_data *spi = dev->driver_data;
	uint32_t data = 0;
	DBG_COUNTER_INIT();

	while (read_rxflr(info->regs)) {
		data = read_dr(info->regs);
		DBG_COUNTER_INC();

		if (spi->rx_buf && spi->rx_buf_len > 0) {
			switch (spi->dfs) {
			case 1:
				UNALIGNED_PUT(data, (uint8_t *)spi->rx_buf);
				break;
			case 2:
				UNALIGNED_PUT(data, (uint16_t *)spi->rx_buf);
				break;
#ifndef CONFIG_ARC
			case 4:
				UNALIGNED_PUT(data, (uint32_t *)spi->rx_buf);
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

	DBG("Pulled: %d\n", DBG_COUNTER_RESULT());
}

static inline bool _spi_dw_is_controller_ready(struct device *dev)
{
	struct spi_dw_config *info = dev->config->config_info;

	if (test_bit_ssienr(info->regs) || test_bit_sr_busy(info->regs)) {
		return false;
	}

	return true;
}

static int spi_dw_configure(struct device *dev,
				struct spi_config *config)
{
	struct spi_dw_config *info = dev->config->config_info;
	struct spi_dw_data *spi = dev->driver_data;
	uint32_t flags = config->config;
	uint32_t ctrlr0 = 0;
	uint32_t mode;

	DBG("%s: %p (0x%x), %p\n", __func__, dev, info->regs, config);

	/* Check status */
	if (!_spi_dw_is_controller_ready(dev)) {
		DBG("%s: Controller is busy\n", __func__);
		return DEV_USED;
	}

	/* Word size */
	ctrlr0 |= DW_SPI_CTRLR0_DFS(SPI_WORD_SIZE_GET(flags));

	/* Determine how many bytes are required per-frame */
	spi->dfs = SPI_DFS_TO_BYTES(SPI_WORD_SIZE_GET(flags));

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

	/* Configuring the rate */
	write_baudr(config->max_sys_freq, info->regs);

	return 0;
}

static int spi_dw_slave_select(struct device *dev, uint32_t slave)
{
	struct spi_dw_data *spi = dev->driver_data;

	DBG("%s: %p %d\n", __func__, dev, slave);

	if (slave == 0 || slave > 4) {
		return DEV_INVALID_CONF;
	}

	spi->slave = 1 << (slave - 1);

	return 0;
}

static int spi_dw_transceive(struct device *dev,
			     const void *tx_buf, uint32_t tx_buf_len,
			     void *rx_buf, uint32_t rx_buf_len)
{
	struct spi_dw_config *info = dev->config->config_info;
	struct spi_dw_data *spi = dev->driver_data;
	uint32_t rx_thsld = DW_SPI_RXFTLR_DFLT;

	DBG("%s: %p, %p, %u, %p, %u\n",
	    __func__, dev, tx_buf, tx_buf_len, rx_buf, rx_buf_len);

	/* Check status */
	if (!_spi_dw_is_controller_ready(dev)) {
		DBG("%s: Controller is busy\n", __func__);
		return DEV_USED;
	}

	/* Set buffers info */
	spi->tx_buf = tx_buf;
	spi->tx_buf_len = tx_buf_len/spi->dfs;
	spi->rx_buf = rx_buf;
	spi->rx_buf_len = rx_buf_len/spi->dfs;
	spi->fifo_diff = 0;

	/* Tx Threshold, always at default */
	write_txftlr(DW_SPI_TXFTLR_DFLT, info->regs);

	/* Does Rx thresholds needs to be lower? */
	if (rx_buf_len && spi->rx_buf_len < DW_SPI_FIFO_DEPTH) {
		rx_thsld = spi->rx_buf_len - 1;
	} else if (!rx_buf_len && spi->tx_buf_len < DW_SPI_FIFO_DEPTH) {
		rx_thsld = spi->tx_buf_len - 1;
	}

	write_rxftlr(rx_thsld, info->regs);

	/* Slave select */
	write_ser(spi->slave, info->regs);

	_spi_control_cs(dev, 1);

	/* Enable interrupts */
	write_imr(DW_SPI_IMR_UNMASK, info->regs);

	/* Enable the controller */
	set_bit_ssienr(info->regs);

	device_sync_call_wait(&spi->sync);

	if (spi->error) {
		spi->error = 0;
		return DEV_FAIL;
	}

	return 0;
}

static int spi_dw_suspend(struct device *dev)
{
	DBG("%s: %p\n", __func__, dev);

	_clock_off(dev);

	return 0;
}

static int spi_dw_resume(struct device *dev)
{
	DBG("%se: %p\n", __func__, dev);

	_clock_on(dev);

	return 0;
}

void spi_dw_isr(void *arg)
{
	struct device *dev = arg;
	struct spi_dw_config *info = dev->config->config_info;
	uint32_t error = 0;
	uint32_t int_status;

	int_status = read_isr(info->regs);

	DBG("SPI int_status 0x%x - (tx: %d, rx: %d)\n",
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

static struct spi_driver_api dw_spi_api = {
	.configure = spi_dw_configure,
	.slave_select = spi_dw_slave_select,
	.transceive = spi_dw_transceive,
	.suspend = spi_dw_suspend,
	.resume = spi_dw_resume,
};

int spi_dw_init(struct device *dev)
{
	struct spi_dw_config *info = dev->config->config_info;
	struct spi_dw_data *spi = dev->driver_data;

	_clock_config(dev);
	_clock_on(dev);

#ifndef CONFIG_SOC_QUARK_SE_SS
	if (read_ssi_comp_version(info->regs) != DW_SSI_COMP_VERSION) {
		_clock_off(dev);
		return DEV_NOT_CONFIG;
	}
#endif

	dev->driver_api = &dw_spi_api;

	info->config_func();

	device_sync_call_init(&spi->sync);

	_spi_config_cs(dev);

	/* Masking interrupt and making sure controller is disabled */
	write_imr(DW_SPI_IMR_MASK, info->regs);
	clear_bit_ssienr(info->regs);

	DBG("Designware SPI driver initialized on device: %p\n", dev);

	return 0;
}

#if defined(CONFIG_IOAPIC) || defined(CONFIG_MVIC)
	#if defined(CONFIG_SPI_DW_FALLING_EDGE)
		#define SPI_DW_IRQ_FLAGS (IOAPIC_EDGE | IOAPIC_LOW)
	#elif defined(CONFIG_SPI_DW_RISING_EDGE)
		#define SPI_DW_IRQ_FLAGS (IOAPIC_EDGE | IOAPIC_HIGH)
	#elif defined(CONFIG_SPI_DW_LEVEL_HIGH)
		#define SPI_DW_IRQ_FLAGS (IOAPIC_LEVEL | IOAPIC_HIGH)
	#elif defined(CONFIG_SPI_DW_LEVEL_LOW)
		#define SPI_DW_IRQ_FLAGS (IOAPIC_LEVEL | IOAPIC_LOW)
	#endif
#else
	#define SPI_DW_IRQ_FLAGS 0
#endif /* CONFIG_IOAPIC */

#ifdef CONFIG_SPI_DW_PORT_0
void spi_config_0_irq(void);

struct spi_dw_data spi_dw_data_port_0;

struct spi_dw_config spi_dw_config_0 = {
	.regs = CONFIG_SPI_DW_PORT_0_REGS,
#ifdef CONFIG_SPI_DW_CLOCK_GATE
	.clock_data = UINT_TO_POINTER(CONFIG_SPI_DW_PORT_0_CLOCK_GATE_SUBSYS),
#endif /* CONFIG_SPI_DW_CLOCK_GATE */
#ifdef CONFIG_SPI_DW_CS_GPIO
	.cs_gpio_name = CONFIG_SPI_DW_PORT_0_CS_GPIO_PORT,
	.cs_gpio_pin = CONFIG_SPI_DW_PORT_0_CS_GPIO_PIN,
#endif
	.config_func = spi_config_0_irq
};

DEVICE_INIT(spi_dw_port_0, CONFIG_SPI_DW_PORT_0_DRV_NAME, spi_dw_init,
			&spi_dw_data_port_0, &spi_dw_config_0,
			SECONDARY, CONFIG_SPI_DW_INIT_PRIORITY);

void spi_config_0_irq(void)
{
#ifdef CONFIG_SPI_DW_INTERRUPT_SINGLE_LINE
	IRQ_CONNECT(CONFIG_SPI_DW_PORT_0_IRQ, CONFIG_SPI_DW_PORT_0_PRI,
		    spi_dw_isr, DEVICE_GET(spi_dw_port_0), SPI_DW_IRQ_FLAGS);
	irq_enable(CONFIG_SPI_DW_PORT_0_IRQ);
	_spi_int_unmask(SPI_DW_PORT_0_INT_MASK);
#else /* SPI_DW_INTERRUPT_SEPARATED_LINES */
	IRQ_CONNECT(CONFIG_SPI_DW_PORT_0_RX_IRQ, CONFIG_SPI_DW_PORT_0_PRI,
		    spi_dw_isr, DEVICE_GET(spi_dw_port_0), SPI_DW_IRQ_FLAGS);
	IRQ_CONNECT(CONFIG_SPI_DW_PORT_0_TX_IRQ, CONFIG_SPI_DW_PORT_0_PRI,
		    spi_dw_isr, DEVICE_GET(spi_dw_port_0), SPI_DW_IRQ_FLAGS);
	IRQ_CONNECT(CONFIG_SPI_DW_PORT_0_ERROR_IRQ, CONFIG_SPI_DW_PORT_0_PRI,
		    spi_dw_isr, DEVICE_GET(spi_dw_port_0), SPI_DW_IRQ_FLAGS);

	irq_enable(CONFIG_SPI_DW_PORT_0_RX_IRQ);
	irq_enable(CONFIG_SPI_DW_PORT_0_TX_IRQ);
	irq_enable(CONFIG_SPI_DW_PORT_0_ERROR_IRQ);

	_spi_int_unmask(SPI_DW_PORT_0_RX_INT_MASK);
	_spi_int_unmask(SPI_DW_PORT_0_TX_INT_MASK);
	_spi_int_unmask(SPI_DW_PORT_0_ERROR_INT_MASK);
#endif
}
#endif /* CONFIG_SPI_DW_PORT_0 */
#ifdef CONFIG_SPI_DW_PORT_1
void spi_config_1_irq(void);

struct spi_dw_data spi_dw_data_port_1;

struct spi_dw_config spi_dw_config_1 = {
	.regs = CONFIG_SPI_DW_PORT_1_REGS,
#ifdef CONFIG_SPI_DW_CLOCK_GATE
	.clock_data = UINT_TO_POINTER(CONFIG_SPI_DW_PORT_1_CLOCK_GATE_SUBSYS),
#endif /* CONFIG_SPI_DW_CLOCK_GATE */
#ifdef CONFIG_SPI_DW_CS_GPIO
	.cs_gpio_name = CONFIG_SPI_DW_PORT_1_CS_GPIO_PORT,
	.cs_gpio_pin = CONFIG_SPI_DW_PORT_1_CS_GPIO_PIN,
#endif
	.config_func = spi_config_1_irq
};

DEVICE_INIT(spi_dw_port_1, CONFIG_SPI_DW_PORT_1_DRV_NAME, spi_dw_init,
			&spi_dw_data_port_1, &spi_dw_config_1,
			SECONDARY, CONFIG_SPI_DW_INIT_PRIORITY);

void spi_config_1_irq(void)
{
#ifdef CONFIG_SPI_DW_INTERRUPT_SINGLE_LINE
	IRQ_CONNECT(CONFIG_SPI_DW_PORT_1_IRQ, CONFIG_SPI_DW_PORT_1_PRI,
		    spi_dw_isr, DEVICE_GET(spi_dw_port_1), SPI_DW_IRQ_FLAGS);
	irq_enable(CONFIG_SPI_DW_PORT_1_IRQ);
	_spi_int_unmask(SPI_DW_PORT_1_INT_MASK);
#else /* SPI_DW_INTERRUPT_SEPARATED_LINES */
	IRQ_CONNECT(CONFIG_SPI_DW_PORT_1_RX_IRQ, CONFIG_SPI_DW_PORT_1_PRI,
		    spi_dw_isr, DEVICE_GET(spi_dw_port_1), SPI_DW_IRQ_FLAGS);
	IRQ_CONNECT(CONFIG_SPI_DW_PORT_1_TX_IRQ, CONFIG_SPI_DW_PORT_1_PRI,
		    spi_dw_isr, DEVICE_GET(spi_dw_port_1), SPI_DW_IRQ_FLAGS);
	IRQ_CONNECT(CONFIG_SPI_DW_PORT_1_ERROR_IRQ, CONFIG_SPI_DW_PORT_1_PRI,
		    spi_dw_isr, DEVICE_GET(spi_dw_port_1), SPI_DW_IRQ_FLAGS);

	irq_enable(CONFIG_SPI_DW_PORT_1_RX_IRQ);
	irq_enable(CONFIG_SPI_DW_PORT_1_TX_IRQ);
	irq_enable(CONFIG_SPI_DW_PORT_1_ERROR_IRQ);

	_spi_int_unmask(SPI_DW_PORT_1_RX_INT_MASK);
	_spi_int_unmask(SPI_DW_PORT_1_TX_INT_MASK);
	_spi_int_unmask(SPI_DW_PORT_1_ERROR_INT_MASK);
#endif
}
#endif /* CONFIG_SPI_DW_PORT_1 */
