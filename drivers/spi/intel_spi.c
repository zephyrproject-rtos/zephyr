/* intel_spi.c - Driver implementation for Intel SPI controller */

/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <nanokernel.h>
#include <arch/cpu.h>

#include <misc/__assert.h>
#include <board.h>

#include <sys_io.h>

#ifdef CONFIG_PCI
#include <pci/pci.h>
#include <pci/pci_mgr.h>
#endif

#include <spi.h>
#include <spi/intel_spi.h>
#include "intel_spi_priv.h"

#ifndef CONFIG_SPI_DEBUG
#define DBG(...) {;}
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
		DBG("Reading 0x%x\n", addr + __off);			\
		return sys_read##__sz(addr + __off);			\
	}
#define DEFINE_MM_REG_WRITE(__reg, __off, __sz)				\
	static inline void write_##__reg(uint32_t data, uint32_t addr)	\
	{								\
		DBG("Writing 0x%x\n", addr + __off);			\
		sys_write##__sz(data, addr + __off);			\
	}

DEFINE_MM_REG_WRITE(sscr0, INTEL_SPI_REG_SSCR0, 32)
DEFINE_MM_REG_WRITE(sscr1, INTEL_SPI_REG_SSRC1, 32)
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

static void completed(struct device *dev)
{
	struct spi_intel_config *info = dev->config->config_info;
	struct spi_intel_data *spi = dev->driver_data;
	enum spi_cb_type cb_type;

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

	spi->tx_buf = spi->rx_buf = NULL;
	spi->tx_buf_len = spi->rx_buf_len = 0;

	write_sscr1(spi->sscr1, info->regs);

	if (spi->callback) {
		spi->callback(dev, cb_type);
	}
}

static void push_data(struct device *dev)
{
	struct spi_intel_config *info = dev->config->config_info;
	struct spi_intel_data *spi = dev->driver_data;
	uint32_t cnt = 0;
	uint8_t data;

	DBG("spi: push_data\n");

	while(read_sssr(info->regs) & INTEL_SPI_SSSR_TNF) {
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
}

static void pull_data(struct device *dev)
{
	struct spi_intel_config *info = dev->config->config_info;
	struct spi_intel_data *spi = dev->driver_data;
	uint32_t cnt = 0;
	uint8_t data;

	while(read_sssr(info->regs) & INTEL_SPI_SSSR_RNE) {
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

static int spi_intel_configure(struct device *dev, struct spi_config *config)
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

	/* Installing the registers (enabling the controller as well) */
	write_sscr1(spi->sscr1, info->regs);
	write_sscr0(spi->sscr0 | INTEL_SPI_SSCR0_SSE, info->regs);

	push_data(dev);

	/* Enable receive interrupt */
	write_sscr1(spi->sscr1 | INTEL_SPI_SSCR1_RIE
			| INTEL_SPI_SSCR1_TIE, info->regs);

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
	uint32_t status;

	DBG("spi_intel_isr: %p\n", dev);

	status = read_sssr(info->regs);

	if (!(status & (INTEL_SPI_SSSR_TFS | INTEL_SPI_SSSR_RFS))) {
		DBG("None or unhandled interrupt\n");
		return;
	}

	if (status & INTEL_SPI_SSSR_RFS) {
		pull_data(dev);
	}

	if (status & INTEL_SPI_SSSR_TFS) {
		push_data(dev);
	}

	completed(dev);
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
	struct pci_dev_info spi_intel_pci = {
		.class = PCI_CLASS_SERIAL_BUS,
		.function = info->function,
		.vendor_id = 0x8086,
		.device_id = 0x935,
	};

	pci_bus_scan_init();

	if (!pci_bus_scan(&spi_intel_pci)) {
		DBG("Could not find device\n");
		return 0;
	}

	pci_enable_regs(&spi_intel_pci);

#ifdef CONFIG_PCI_DEBUG
	pci_show(&spi_intel_pci);
#endif
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

	irq_enable(info->irq);

	DBG("SPI Intel Driver initialized on device: %p\n", dev);

	return DEV_OK;
}
