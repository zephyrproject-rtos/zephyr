/* intel_spi.c - Driver implementation for Intel SPI controller */

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
#include <init.h>

#include <sys_io.h>

#include <spi.h>
#include <spi/intel_spi.h>
#include "intel_spi_priv.h"

#ifndef CONFIG_SPI_DEBUG
#define DBG(...) { ; }
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

DEFINE_MM_REG_WRITE(sscr0, INTEL_SPI_REG_SSCR0, 32)
DEFINE_MM_REG_WRITE(sscr1, INTEL_SPI_REG_SSCR1, 32)
DEFINE_MM_REG_READ(sssr, INTEL_SPI_REG_SSSR, 32)
DEFINE_MM_REG_READ(ssdr, INTEL_SPI_REG_SSDR, 32)
DEFINE_MM_REG_WRITE(ssdr, INTEL_SPI_REG_SSDR, 32)
DEFINE_MM_REG_WRITE(dds_rate, INTEL_SPI_REG_DDS_RATE, 32)

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

DEFINE_SET_BIT_OP(sscr0_sse, INTEL_SPI_REG_SSCR0, INTEL_SPI_SSCR0_SSE_BIT)
DEFINE_CLEAR_BIT_OP(sscr0_sse, INTEL_SPI_REG_SSCR0, INTEL_SPI_SSCR0_SSE_BIT)
DEFINE_TEST_BIT_OP(sscr0_sse, INTEL_SPI_REG_SSCR0, INTEL_SPI_SSCR0_SSE_BIT)
DEFINE_TEST_BIT_OP(sssr_bsy, INTEL_SPI_REG_SSSR, INTEL_SPI_SSSR_BSY_BIT)
DEFINE_CLEAR_BIT_OP(sscr1_tie, INTEL_SPI_REG_SSCR1, INTEL_SPI_SSCR1_TIE_BIT)

#ifdef CONFIG_SPI_INTEL_CS_GPIO

#include <gpio.h>

static inline void _spi_config_cs(struct device *dev)
{
	struct spi_intel_config *info = dev->config->config_info;
	struct spi_intel_data *spi = dev->driver_data;
	struct device *gpio;

	gpio = device_get_binding(info->cs_gpio_name);
	if (!gpio) {
		spi->cs_gpio_port = NULL;
		return;
	}

	gpio_pin_configure(gpio, info->cs_gpio_pin, GPIO_DIR_OUT);
	/* Default CS line to high (idling) */
	gpio_pin_write(gpio, info->cs_gpio_pin, 1);

	spi->cs_gpio_port = gpio;
}

static inline void _spi_control_cs(struct device *dev, int on)
{
	struct spi_intel_config *info = dev->config->config_info;
	struct spi_intel_data *spi = dev->driver_data;

	if (!spi->cs_gpio_port) {
		return;
	}

	gpio_pin_write(spi->cs_gpio_port, info->cs_gpio_pin, !on);
}
#else
#define _spi_control_cs(...) { ; }
#define _spi_config_cs(...) { ; }
#endif /* CONFIG_SPI_INTEL_CS_GPIO */

static void completed(struct device *dev, uint32_t error)
{
	struct spi_intel_config *info = dev->config->config_info;
	struct spi_intel_data *spi = dev->driver_data;
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

	write_sscr1(spi->sscr1, info->regs);
	clear_bit_sscr0_sse(info->regs);

	_spi_control_cs(dev, 0);

	if (spi->callback) {
		spi->callback(dev, cb_type, spi->user_data);
	}
}

static void push_data(struct device *dev)
{
	struct spi_intel_config *info = dev->config->config_info;
	struct spi_intel_data *spi = dev->driver_data;
	uint32_t cnt = 0;
	uint8_t data;

	DBG("spi: push_data\n");

	while (read_sssr(info->regs) & INTEL_SPI_SSSR_TNF) {
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
			/* Nothing to push anymore for now */
			break;
		}

		write_ssdr(data, info->regs);
		cnt++;
	}

	DBG("Pushed: %d\n", cnt);
	spi->t_len += cnt;

	if (!spi->tx_buf_len && !spi->rx_buf_len) {
		clear_bit_sscr1_tie(info->regs);
	}
}

static void pull_data(struct device *dev)
{
	struct spi_intel_config *info = dev->config->config_info;
	struct spi_intel_data *spi = dev->driver_data;
	uint32_t cnt = 0;
	uint8_t data;

	while (read_sssr(info->regs) & INTEL_SPI_SSSR_RNE) {
		data = (uint8_t) read_ssdr(info->regs);
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

static int spi_intel_configure(struct device *dev,
				struct spi_config *config, void *user_data)
{
	struct spi_intel_config *info = dev->config->config_info;
	struct spi_intel_data *spi = dev->driver_data;
	uint32_t flags = config->config;
	uint32_t mode;

	DBG("spi_intel_configure: %p (0x%x), %p\n", dev, info->regs, config);

	/* Check status */
	if (test_bit_sscr0_sse(info->regs) && test_bit_sssr_bsy(info->regs)) {
		DBG("spi_intel_transceive: Controller is busy\n");
		return DEV_USED;
	}

	/* Pre-configuring the registers to a clean state*/
	spi->sscr0 = spi->sscr1 = 0;
	write_sscr0(spi->sscr0, info->regs);
	write_sscr1(spi->sscr1, info->regs);

	DBG("spi_intel_configure: DDS_RATE: 0x%x SCR: %d\n",
				INTEL_SPI_DSS_RATE(config->max_sys_freq),
				INTEL_SPI_SSCR0_SCR(config->max_sys_freq));

	/* Word size and clock rate */
	spi->sscr0 = INTEL_SPI_SSCR0_DSS(SPI_WORD_SIZE_GET(flags)) |
				INTEL_SPI_SSCR0_SCR(config->max_sys_freq);

	/* SPI mode */
	mode = SPI_MODE(flags);
	if (mode & SPI_MODE_CPOL) {
		spi->sscr1 |= INTEL_SPI_SSCR1_SPO;
	}

	if (mode & SPI_MODE_CPHA) {
		spi->sscr1 |= INTEL_SPI_SSCR1_SPH;
	}

	if (mode & SPI_MODE_LOOP) {
		spi->sscr1 |= INTEL_SPI_SSCR1_LBM;
	}

	/* Tx/Rx Threshold */
	spi->sscr1 |= INTEL_SPI_SSCR1_TFT(INTEL_SPI_SSCR1_TFT_DFLT)
			| INTEL_SPI_SSCR1_RFT(INTEL_SPI_SSCR1_RFT_DFLT);

	/* Configuring the rate */
	write_dds_rate(INTEL_SPI_DSS_RATE(config->max_sys_freq), info->regs);

	spi->tx_buf = spi->rx_buf = NULL;
	spi->tx_buf_len = spi->rx_buf_len = spi->t_len = 0;
	spi->callback = config->callback;
	spi->user_data = user_data;

	return DEV_OK;
}

static int spi_intel_transceive(struct device *dev,
				uint8_t *tx_buf, uint32_t tx_buf_len,
				uint8_t *rx_buf, uint32_t rx_buf_len)
{
	struct spi_intel_config *info = dev->config->config_info;
	struct spi_intel_data *spi = dev->driver_data;

	DBG("spi_dw_transceive: %p, %p, %u, %p, %u\n",
			dev, tx_buf, tx_buf_len, rx_buf, rx_buf_len);

	/* Check status */
	if (test_bit_sscr0_sse(info->regs) && test_bit_sssr_bsy(info->regs)) {
		DBG("spi_intel_transceive: Controller is busy\n");
		return DEV_USED;
	}

	/* Set buffers info */
	spi->tx_buf = tx_buf;
	spi->tx_buf_len = tx_buf_len;
	spi->rx_buf = rx_buf;
	spi->rx_buf_len = rx_buf_len;

	_spi_control_cs(dev, 1);

	/* Installing the registers (enabling interrupts and controller) */
	write_sscr1(spi->sscr1 | INTEL_SPI_SSCR1_RIE |
				INTEL_SPI_SSCR1_TIE, info->regs);
	write_sscr0(spi->sscr0 | INTEL_SPI_SSCR0_SSE, info->regs);

	return DEV_OK;
}

static int spi_intel_suspend(struct device *dev)
{
	struct spi_intel_config *info = dev->config->config_info;

	DBG("spi_intel_suspend: %p\n", dev);

	clear_bit_sscr0_sse(info->regs);
	irq_disable(info->irq);

	return DEV_OK;
}

static int spi_intel_resume(struct device *dev)
{
	struct spi_intel_config *info = dev->config->config_info;

	DBG("spi_intel_resume: %p\n", dev);

	set_bit_sscr0_sse(info->regs);
	irq_enable(info->irq);

	return DEV_OK;
}

void spi_intel_isr(void *arg)
{
	struct device *dev = arg;
	struct spi_intel_config *info = dev->config->config_info;
	uint32_t error = 0;
	uint32_t status;

	DBG("spi_intel_isr: %p\n", dev);

	status = read_sssr(info->regs);

	if (status & INTEL_SPI_SSSR_ROR) {
		/* Unrecoverable error */
		error = 1;
		goto out;
	}

	if (status & INTEL_SPI_SSSR_RFS) {
		pull_data(dev);
	}

	if (status & INTEL_SPI_SSSR_TFS) {
		push_data(dev);
	}

out:
	completed(dev, error);
}

static struct spi_driver_api intel_spi_api = {
	.configure = spi_intel_configure,
	.slave_select = NULL,
	.transceive = spi_intel_transceive,
	.suspend = spi_intel_suspend,
	.resume = spi_intel_resume,
};

#ifdef CONFIG_PCI
static inline int spi_intel_setup(struct device *dev)
{
	struct spi_intel_config *info = dev->config->config_info;

	pci_bus_scan_init();

	if (!pci_bus_scan(&info->pci_dev)) {
		DBG("Could not find device\n");
		return 0;
	}

#ifdef CONFIG_PCI_ENUMERATION
	info->regs = info->pci_dev.addr;
	info->irq = info->pci_dev.irq;
#endif

	pci_enable_regs(&info->pci_dev);

	pci_show(&info->pci_dev);

	return 1;
}
#else
#define spi_intel_setup(_unused_) (1)
#endif /* CONFIG_PCI */

int spi_intel_init(struct device *dev)
{
	struct spi_intel_config *info = dev->config->config_info;

	dev->driver_api = &intel_spi_api;

	if (!spi_intel_setup(dev)) {
		return DEV_NOT_CONFIG;
	}

	info->config_func(dev);

	_spi_config_cs(dev);

	irq_enable(info->irq);

	DBG("SPI Intel Driver initialized on device: %p\n", dev);

	return DEV_OK;
}

/* system bindings */
#ifdef CONFIG_SPI_INTEL_PORT_0

void spi_config_0_irq(struct device *dev);

struct spi_intel_data spi_intel_data_port_0;

struct spi_intel_config spi_intel_config_0 = {
	.regs = CONFIG_SPI_INTEL_PORT_0_REGS,
	.irq = CONFIG_SPI_INTEL_PORT_0_IRQ,
#if CONFIG_PCI
	.pci_dev.class = CONFIG_SPI_INTEL_CLASS,
	.pci_dev.bus = CONFIG_SPI_INTEL_PORT_0_BUS,
	.pci_dev.dev = CONFIG_SPI_INTEL_PORT_0_DEV,
	.pci_dev.vendor_id = CONFIG_SPI_INTEL_VENDOR_ID,
	.pci_dev.device_id = CONFIG_SPI_INTEL_DEVICE_ID,
	.pci_dev.function = CONFIG_SPI_INTEL_PORT_0_FUNCTION,
#endif
#ifdef CONFIG_SPI_INTEL_CS_GPIO
	.cs_gpio_name = CONFIG_SPI_INTEL_PORT_0_CS_GPIO_PORT,
	.cs_gpio_pin = CONFIG_SPI_INTEL_PORT_0_CS_GPIO_PIN,
#endif
	.config_func = spi_config_0_irq
};

DECLARE_DEVICE_INIT_CONFIG(spi_intel_port_0, CONFIG_SPI_INTEL_PORT_0_DRV_NAME,
			   spi_intel_init, &spi_intel_config_0);

nano_early_init(spi_intel_port_0, &spi_intel_data_port_0);

void spi_intel_isr_0(void *unused)
{
	spi_intel_isr(&__initconfig_spi_intel_port_0);
}

IRQ_CONNECT_STATIC(spi_intel_irq_port_0, CONFIG_SPI_INTEL_PORT_0_IRQ,
		   CONFIG_SPI_INTEL_PORT_0_PRI, spi_intel_isr_0, 0);

void spi_config_0_irq(struct device *dev)
{
	struct spi_intel_config *config = dev->config->config_info;

	IRQ_CONFIG(spi_intel_irq_port_0, config->irq, 0);
}

#endif /* CONFIG_SPI_INTEL_PORT_0 */
#ifdef CONFIG_SPI_INTEL_PORT_1

void spi_config_1_irq(struct device *dev);

struct spi_intel_data spi_intel_data_port_1;

struct spi_intel_config spi_intel_config_1 = {
	.regs = CONFIG_SPI_INTEL_PORT_1_REGS,
	.irq = CONFIG_SPI_INTEL_PORT_1_IRQ,
#if CONFIG_PCI
	.pci_dev.class = CONFIG_SPI_INTEL_CLASS,
	.pci_dev.bus = CONFIG_SPI_INTEL_PORT_1_BUS,
	.pci_dev.dev = CONFIG_SPI_INTEL_PORT_1_DEV,
	.pci_dev.function = CONFIG_SPI_INTEL_PORT_1_FUNCTION,
	.pci_dev.vendor_id = CONFIG_SPI_INTEL_VENDOR_ID,
	.pci_dev.device_id = CONFIG_SPI_INTEL_DEVICE_ID,
#endif
#ifdef CONFIG_SPI_INTEL_CS_GPIO
	.cs_gpio_name = NULL,
#endif
	.config_func = spi_config_1_irq
};

DECLARE_DEVICE_INIT_CONFIG(spi_intel_port_1, CONFIG_SPI_INTEL_PORT_1_DRV_NAME,
			   spi_intel_init, &spi_intel_config_1);

pre_kernel_late_init(spi_intel_port_1, &spi_intel_data_port_1);

void spi_intel_isr_1(void *unused)
{
	spi_intel_isr(&__initconfig_spi_intel_port_1);
}

IRQ_CONNECT_STATIC(spi_intel_irq_port_1, CONFIG_SPI_INTEL_PORT_1_IRQ,
		   CONFIG_SPI_INTEL_PORT_1_PRI, spi_intel_isr_1, 0);

void spi_config_1_irq(struct device *dev)
{
	struct spi_intel_config *config = dev->config->config_info;

	IRQ_CONFIG(spi_intel_irq_port_1, config->irq, 0);
}

#endif /* CONFIG_SPI_INTEL_PORT_1 */
