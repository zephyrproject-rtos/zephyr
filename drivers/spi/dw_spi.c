/* dw_spi.c - Designware SPI driver implementation */

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
#include <dw_spi_priv.h>

#ifndef CONFIG_SPI_DEBUG
#define DBG(...) {; }
#else
#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#define DBG printf
#else
#include <misc/printk.h>
#define DBG printk
#endif /* CONFIG_STDOUT_CONSOLE */
#endif /* CONFIG_SPI_DEBUG */

#define DEFINE_MM_REG_READ(__reg, __off, __sz)				\
	static inline uint32_t read_##__reg(uint32_t addr)		\
	{								\
		return sys_read##__sz(addr + __off);			\
	}
#define DEFINE_MM_REG_WRITE(__reg, __off, __sz)				\
	static inline void write_##__reg(uint32_t data, uint32_t addr)	\
	{								\
		sys_write##__sz(data, addr + __off);			\
	}

DEFINE_MM_REG_WRITE(ctrlr0, DW_SPI_REG_CTRLR0, 16)
DEFINE_MM_REG_WRITE(ser, DW_SPI_REG_SER, 8)
DEFINE_MM_REG_WRITE(baudr, DW_SPI_REG_BAUDR, 16)
DEFINE_MM_REG_WRITE(txftlr, DW_SPI_REG_TXFTLR, 32)
DEFINE_MM_REG_WRITE(rxftlr, DW_SPI_REG_RXFTLR, 32)
DEFINE_MM_REG_READ(rxflr, DW_SPI_REG_RXFLR, 32)
DEFINE_MM_REG_READ(txflr, DW_SPI_REG_TXFLR, 32)
DEFINE_MM_REG_WRITE(imr, DW_SPI_REG_IMR, 8)
DEFINE_MM_REG_READ(isr, DW_SPI_REG_ISR, 8)
DEFINE_MM_REG_READ(dr, DW_SPI_REG_DR, 16)
DEFINE_MM_REG_WRITE(dr, DW_SPI_REG_DR, 16)
DEFINE_MM_REG_READ(ssi_comp_version, DW_SPI_REG_SSI_COMP_VERSION, 32)

#define DEFINE_SET_BIT_OP(__reg_bit, __reg_off, __bit)			\
	static inline void set_bit_##__reg_bit(uint32_t addr)		\
	{								\
		sys_set_bit(addr + __reg_off, __bit);			\
	}

#define DEFINE_CLEAR_BIT_OP(__reg_bit, __reg_off, __bit)		\
	static inline void clear_bit_##__reg_bit(uint32_t addr)		\
	{								\
		sys_clear_bit(addr + __reg_off, __bit);			\
	}

#define DEFINE_TEST_BIT_OP(__reg_bit, __reg_off, __bit)			\
	static inline int test_bit_##__reg_bit(uint32_t addr)		\
	{								\
		return sys_test_bit(addr + __reg_off, __bit);		\
	}

DEFINE_SET_BIT_OP(ssienr, DW_SPI_REG_SSIENR, DW_SPI_SSIENR_SSIEN_BIT)
DEFINE_CLEAR_BIT_OP(ssienr, DW_SPI_REG_SSIENR, DW_SPI_SSIENR_SSIEN_BIT)
DEFINE_TEST_BIT_OP(sr_busy, DW_SPI_REG_SR, DW_SPI_SR_BUSY_BIT)
DEFINE_TEST_BIT_OP(sr_tfnf, DW_SPI_REG_SR, DW_SPI_SR_TFNF_BIT)
DEFINE_TEST_BIT_OP(sr_rfne, DW_SPI_REG_SR, DW_SPI_SR_RFNE_BIT)
DEFINE_TEST_BIT_OP(icr, DW_SPI_REG_ICR, DW_SPI_SR_ICR_BIT)

#ifdef CONFIG_PLATFORM_QUARK_SE
#define int_unmask(__mask)						\
	sys_write32(sys_read32(__mask) & INT_UNMASK_IA, __mask)
#else
#define int_unmask(...) {; }
#endif

#ifdef CONFIG_SPI_DW_CLOCK_GATE
static inline void _clock_config(struct device *dev)
{
	struct device *clk;
	char *drv = CONFIG_SPI_DW_CLOCK_GATE_DRV_NAME;

	clk = device_get_binding(drv);
	if (clk) {
		struct spi_dw_data *spi = dev->driver_data;

		spi->clock = clk;
	}
}

static inline void _clock_on(struct device *dev)
{
	struct spi_dw_config *info = dev->config->config_info;
	struct spi_dw_data *spi = dev->driver_data;

	clock_control_on(spi->clock, info->clock_data);
}

static inline void _clock_off(struct device *dev)
{
	struct spi_dw_config *info = dev->config->config_info;
	struct spi_dw_data *spi = dev->driver_data;

	clock_control_off(spi->clock, info->clock_data);
}
#else
#define _clock_config(...)
#define _clock_on(...)
#define _clock_off(...)
#endif

static void completed(struct device *dev, int error)
{
	struct spi_dw_config *info = dev->config->config_info;
	struct spi_dw_data *spi = dev->driver_data;
	enum spi_cb_type cb_type;

	if (error) {
		cb_type = SPI_CB_ERROR;
		goto out;
	}

	if (spi->t_len) {
		return;
	}

	if (spi->tx_buf && spi->tx_buf_len == 0 && !spi->rx_buf) {
		cb_type = SPI_CB_WRITE;
	} else if (spi->rx_buf && spi->rx_buf_len == 0 && !spi->tx_buf) {
		cb_type = SPI_CB_READ;
	} else if (spi->tx_buf && spi->tx_buf_len == 0 &&
			spi->rx_buf && spi->rx_buf_len == 0) {
		cb_type = SPI_CB_TRANSCEIVE;
	} else {
		return;
	}

out:
	spi->tx_buf = spi->rx_buf = NULL;
	spi->tx_buf_len = spi->rx_buf_len = 0;

	/* Disabling interrupts */
	write_imr(DW_SPI_IMR_MASK, info->regs);

	if (spi->callback) {
		spi->callback(dev, cb_type, spi->user_data);
	}
}

static void push_data(struct device *dev)
{
	struct spi_dw_config *info = dev->config->config_info;
	struct spi_dw_data *spi = dev->driver_data;
	uint32_t cnt = 0;
	uint8_t data;

	DBG("spi: push_data\n");

	while (test_bit_sr_tfnf(info->regs)) {
		if (cnt == DW_SPI_RXFTLR_DFLT) {
			break;
		}

		if (spi->tx_buf && spi->tx_buf_len > 0) {
			data = *(uint8_t *)(spi->tx_buf);
			spi->tx_buf++;
			spi->tx_buf_len--;
		} else if (spi->rx_buf && spi->rx_buf_len > 0) {
			/* No need to push more than necessary */
			if (spi->rx_buf_len - cnt <= 0) {
				break;
			}

			data = 0;
		} else {
			/* Nothing to push anymore */
			break;
		}

		write_dr(data, info->regs);
		cnt++;
	}

	DBG("Pushed: %d\n", cnt);
	spi->t_len += cnt;
}

static void pull_data(struct device *dev)
{
	struct spi_dw_config *info = dev->config->config_info;
	struct spi_dw_data *spi = dev->driver_data;
	uint32_t cnt = 0;
	uint8_t data;

	DBG("spi: pull_data\n");

	while (test_bit_sr_rfne(info->regs)) {
		data = read_dr(info->regs);
		cnt++;

		if (spi->rx_buf && spi->rx_buf_len > 0) {
			*(uint8_t *)(spi->rx_buf) = data;
			spi->rx_buf++;
			spi->rx_buf_len--;
		}
	}

	DBG("Pulled: %d\n", cnt);
	spi->t_len -= cnt;
}

static int spi_dw_configure(struct device *dev,
				struct spi_config *config, void *user_data)
{
	struct spi_dw_config *info = dev->config->config_info;
	struct spi_dw_data *spi = dev->driver_data;
	uint32_t flags = config->config;
	uint32_t ctrlr0 = 0;
	uint32_t mode;

	DBG("spi_dw_configure: %p (0x%x), %p\n", dev, info->regs, config);

	/* Check status */
	if (test_bit_sr_busy(info->regs)) {
		DBG("spi_dw_read: %Controller is busy\n");
		return DEV_USED;
	}

	/* Disable the controller, to be able to set it up */
	clear_bit_ssienr(info->regs);

	/* Word size */
	ctrlr0 |= DW_SPI_CTRLR0_DFS(SPI_WORD_SIZE_GET(flags));

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

	/* Tx/Rx Threshold */
	write_txftlr(DW_SPI_TXFTLR_DFLT, info->regs);
	write_rxftlr(DW_SPI_RXFTLR_DFLT, info->regs);

	/* Configuring the rate */
	write_baudr(config->max_sys_freq, info->regs);

	spi->tx_buf = spi->rx_buf = NULL;
	spi->tx_buf_len = spi->rx_buf_len = spi->t_len = 0;
	spi->callback = config->callback;
	spi->user_data = user_data;

	/* Mask SPI interrupts */
	write_imr(DW_SPI_IMR_MASK, info->regs);

	/* Enable the controller */
	set_bit_ssienr(info->regs);

	return DEV_OK;
}

static int spi_dw_slave_select(struct device *dev, uint32_t slave)
{
	struct spi_dw_data *spi = dev->driver_data;

	if (slave == 0 || slave > 4) {
		return DEV_INVALID_CONF;
	}

	spi->slave = 1 << (slave - 1);

	return DEV_OK;
}

static int spi_dw_transceive(struct device *dev,
			     uint8_t *tx_buf, uint32_t tx_buf_len,
			     uint8_t *rx_buf, uint32_t rx_buf_len)
{
	struct spi_dw_config *info = dev->config->config_info;
	struct spi_dw_data *spi = dev->driver_data;

	DBG("spi_dw_transceive: %p, %p, %u, %p, %u\n",
			dev, tx_buf, tx_buf_len, rx_buf, rx_buf_len);

	/* Check status */
	if (test_bit_sr_busy(info->regs)) {
		DBG("spi_dw_transceive: %Controller is busy\n");
		return DEV_USED;
	}

	/* Disable the controller */
	clear_bit_ssienr(info->regs);

	/* Set buffers info */
	spi->tx_buf = tx_buf;
	spi->tx_buf_len = tx_buf_len;
	spi->rx_buf = rx_buf;
	spi->rx_buf_len = rx_buf_len;

	/* Slave select */
	write_ser(spi->slave, info->regs);

	/* Enable interrupts */
	write_imr(DW_SPI_IMR_UNMASK, info->regs);

	/* Enable the controller */
	set_bit_ssienr(info->regs);

	return DEV_OK;
}

static int spi_dw_suspend(struct device *dev)
{
	struct spi_dw_config *info = dev->config->config_info;

	DBG("spi_dw_suspend: %p\n", dev);

	write_imr(DW_SPI_IMR_MASK, info->regs);
	clear_bit_ssienr(info->regs);
	irq_disable(info->irq);

	_clock_off(dev);

	return DEV_OK;
}

static int spi_dw_resume(struct device *dev)
{
	struct spi_dw_config *info = dev->config->config_info;

	DBG("spi_dw_resume: %p\n", dev);

	_clock_on(dev);

	irq_enable(info->irq);
	set_bit_ssienr(info->regs);
	write_imr(DW_SPI_IMR_UNMASK, info->regs);

	return DEV_OK;
}

void spi_dw_isr(void *arg)
{
	struct device *dev = arg;
	struct spi_dw_config *info = dev->config->config_info;
	uint32_t error = 0;
	uint32_t int_status;

	DBG("spi_dw_isr: %p\n", dev);

	int_status = read_isr(info->regs);
	test_bit_icr(info->regs);

	DBG("int_status 0x%x - (tx: %d, rx: %d)\n",
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

	_clock_config(dev);

	if (read_ssi_comp_version(info->regs) != DW_SSI_COMP_VERSION) {
		_clock_off(dev);
		return DEV_NOT_CONFIG;
	}

	dev->driver_api = &dw_spi_api;

	info->config_func(dev);

	write_imr(DW_SPI_IMR_MASK, info->regs);
	clear_bit_ssienr(info->regs);

	int_unmask(info->int_mask);

	DBG("Designware SPI driver initialized on device: %p\n", dev);

	return DEV_OK;
}

#ifdef CONFIG_SPI_DW_PORT_0

void spi_config_0_irq(struct device *dev);

struct spi_dw_data spi_dw_data_port_0;

struct spi_dw_config spi_dw_config_0 = {
	.regs = CONFIG_SPI_DW_PORT_0_REGS,
	.irq = CONFIG_SPI_DW_PORT_0_IRQ,
	.int_mask = SPI_DW_PORT_0_INT_MASK,
#ifdef CONFIG_SPI_DW_CLOCK_GATE
	.clock_data = UINT_TO_POINTER(CONFIG_SPI_DW_PORT_0_CLOCK_GATE_SUBSYS),
#endif /* CONFIG_SPI_DW_CLOCK_GATE */
	.config_func = spi_config_0_irq
};

DECLARE_DEVICE_INIT_CONFIG(spi_dw_port_0, CONFIG_SPI_DW_PORT_0_DRV_NAME,
			   spi_dw_init, &spi_dw_config_0);

SYS_DEFINE_DEVICE(spi_dw_port_0, &spi_dw_data_port_0, SECONDARY,
					CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
struct device *spi_dw_isr_port_0 = SYS_GET_DEVICE(spi_dw_port_0);

IRQ_CONNECT_STATIC(spi_dw_irq_port_0, CONFIG_SPI_DW_PORT_0_IRQ,
		   CONFIG_SPI_DW_PORT_0_PRI, spi_dw_isr, 0,
		   SPI_DW_IRQ_FLAGS);

void spi_config_0_irq(struct device *dev)
{
	struct spi_dw_config *config = dev->config->config_info;

	IRQ_CONFIG(spi_dw_irq_port_0, config->irq);
	irq_enable(config->irq);
}

#endif /* CONFIG_SPI_DW_PORT_0 */
#ifdef CONFIG_SPI_DW_PORT_1

void spi_config_1_irq(struct device *dev);

struct spi_dw_data spi_dw_data_port_1;

struct spi_dw_config spi_dw_config_1 = {
	.regs = CONFIG_SPI_DW_PORT_1_REGS,
	.irq = CONFIG_SPI_DW_PORT_1_IRQ,
	.int_mask = SPI_DW_PORT_1_INT_MASK,
#ifdef CONFIG_SPI_DW_CLOCK_GATE
	.clock_data = UINT_TO_POINTER(CONFIG_SPI_DW_PORT_1_CLOCK_GATE_SUBSYS),
#endif /* CONFIG_SPI_DW_CLOCK_GATE */
	.config_func = spi_config_1_irq
};

DECLARE_DEVICE_INIT_CONFIG(spi_dw_port_1, CONFIG_SPI_DW_PORT_1_DRV_NAME,
			   spi_dw_init, &spi_dw_config_1);

SYS_DEFINE_DEVICE(spi_dw_port_1, &spi_dw_data_port_1, SECONDARY,
					CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
struct device *spi_dw_isr_port_1 = SYS_GET_DEVICE(spi_dw_port_1);

IRQ_CONNECT_STATIC(spi_dw_irq_port_1, CONFIG_SPI_DW_PORT_1_IRQ,
		   CONFIG_SPI_DW_PORT_1_PRI, spi_dw_isr, 0,
		   SPI_DW_IRQ_FLAGS);

void spi_config_1_irq(struct device *dev)
{
	struct spi_dw_config *config = dev->config->config_info;

	IRQ_CONFIG(spi_dw_irq_port_1, config->irq);
	irq_enable(config->irq);
}

#endif /* CONFIG_SPI_DW_PORT_1 */
