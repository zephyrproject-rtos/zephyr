/* spi_intel.c - Driver implementation for Intel SPI controller */

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
#include <init.h>

#include <sys_io.h>
#include <power.h>

#include <spi.h>
#include <spi/spi_intel.h>
#include "spi_intel.h"

#ifdef CONFIG_IOAPIC
#include <drivers/ioapic.h>
#endif

#define SYS_LOG_DOMAIN "SPI Intel"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SPI_LEVEL
#include <logging/sys_log.h>


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
DEFINE_TEST_BIT_OP(sscr1_tie, INTEL_SPI_REG_SSCR1, INTEL_SPI_SSCR1_TIE_BIT)
DEFINE_CLEAR_BIT_OP(sssr_ror, INTEL_SPI_REG_SSSR, INTEL_SPI_SSSR_ROR_BIT)

#ifdef CONFIG_SPI_CS_GPIO

#include <gpio.h>

static inline void _spi_config_cs(struct device *dev)
{
	const struct spi_intel_config *info = dev->config->config_info;
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
	const struct spi_intel_config *info = dev->config->config_info;
	struct spi_intel_data *spi = dev->driver_data;

	if (!spi->cs_gpio_port) {
		return;
	}

	gpio_pin_write(spi->cs_gpio_port, info->cs_gpio_pin, !on);
}
#else
#define _spi_control_cs(...) { ; }
#define _spi_config_cs(...) { ; }
#endif /* CONFIG_SPI_CS_GPIO */

static void completed(struct device *dev, uint32_t error)
{
	struct spi_intel_data *spi = dev->driver_data;

	/* if received == trans_len, then transmitted == trans_len */
	if (!(spi->received == spi->trans_len) && !error) {
		return;
	}

	spi->error = error;

	_spi_control_cs(dev, 0);

	write_sscr1(spi->sscr1, spi->regs);
	clear_bit_sscr0_sse(spi->regs);

	k_sem_give(&spi->device_sync_sem);
}

static void pull_data(struct device *dev)
{
	struct spi_intel_data *spi = dev->driver_data;
	uint32_t cnt = 0;
	uint8_t data = 0;

	while (read_sssr(spi->regs) & INTEL_SPI_SSSR_RNE) {
		data = (uint8_t) read_ssdr(spi->regs);
		cnt++;
		spi->received++;

		if ((spi->received - 1) < spi->r_buf_len) {
			*(uint8_t *)(spi->rx_buf) = data;
			spi->rx_buf++;
		}
	}

	SYS_LOG_DBG("Pulled: %d (total: %d)",	cnt, spi->received);
}

static void push_data(struct device *dev)
{
	struct spi_intel_data *spi = dev->driver_data;
	uint32_t cnt = 0;
	uint8_t data;
	uint32_t status;

	while ((status = read_sssr(spi->regs)) & INTEL_SPI_SSSR_TNF) {
		if (status & INTEL_SPI_SSSR_RFS) {
			break;
		}
		if (spi->tx_buf && (spi->transmitted < spi->t_buf_len)) {
			data = *(uint8_t *)(spi->tx_buf);
			spi->tx_buf++;
		} else if (spi->transmitted < spi->trans_len) {
			data = 0;
		} else {
			/* Nothing to push anymore for now */
			break;
		}

		cnt++;
		SYS_LOG_DBG("Pushing 1 byte (total: %d)", cnt);
		write_ssdr(data, spi->regs);
		spi->transmitted++;
	}

	SYS_LOG_DBG("Pushed: %d (total: %d)", cnt, spi->transmitted);

	if (spi->transmitted == spi->trans_len) {
		clear_bit_sscr1_tie(spi->regs);
	}
}

static int spi_intel_configure(struct device *dev,
				struct spi_config *config)
{
	struct spi_intel_data *spi = dev->driver_data;
	uint32_t flags = config->config;
	uint32_t mode;

	SYS_LOG_DBG("spi_intel_configure: %p (0x%x), %p", dev, spi->regs,
		    config);

	/* Check status */
	if (test_bit_sscr0_sse(spi->regs) && test_bit_sssr_bsy(spi->regs)) {
		SYS_LOG_DBG("spi_intel_configure: Controller is busy");
		return -EBUSY;
	}

	/* Pre-configuring the registers to a clean state*/
	spi->sscr0 = spi->sscr1 = 0;
	write_sscr0(spi->sscr0, spi->regs);
	write_sscr1(spi->sscr1, spi->regs);

	SYS_LOG_DBG("spi_intel_configure: WS: %d, DDS_RATE: 0x%x SCR: %d",
			SPI_WORD_SIZE_GET(flags),
			INTEL_SPI_DSS_RATE(config->max_sys_freq),
			INTEL_SPI_SSCR0_SCR(config->max_sys_freq) >> 8);

	/* Word size and clock rate */
	spi->sscr0 = INTEL_SPI_SSCR0_DSS(SPI_WORD_SIZE_GET(flags)) |
				INTEL_SPI_SSCR0_SCR(config->max_sys_freq);

	/* Tx/Rx thresholds
	 * Note: Rx thresholds needs to be 1, it does not seem to be able
	 * to trigger reliably any interrupt with another value though the
	 * rx fifo would be full
	 */
	spi->sscr1 |= INTEL_SPI_SSCR1_TFT(INTEL_SPI_SSCR1_TFT_DFLT) |
		      INTEL_SPI_SSCR1_RFT(INTEL_SPI_SSCR1_RFT_DFLT);

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

	/* Configuring the rate */
	write_dds_rate(INTEL_SPI_DSS_RATE(config->max_sys_freq), spi->regs);

	return 0;
}

static int spi_intel_transceive(struct device *dev,
				const void *tx_buf, uint32_t tx_buf_len,
				void *rx_buf, uint32_t rx_buf_len)
{
	struct spi_intel_data *spi = dev->driver_data;

	SYS_LOG_DBG("spi_dw_transceive: %p, %p, %u, %p, %u",
			dev, tx_buf, tx_buf_len, rx_buf, rx_buf_len);

	/* Check status */
	if (test_bit_sscr0_sse(spi->regs) && test_bit_sssr_bsy(spi->regs)) {
		SYS_LOG_DBG("spi_intel_transceive: Controller is busy");
		return -EBUSY;
	}

	/* Set buffers info */
	spi->tx_buf = tx_buf;
	spi->rx_buf = rx_buf;
	spi->t_buf_len = tx_buf_len;
	spi->r_buf_len = rx_buf_len;
	spi->transmitted = 0;
	spi->received = 0;
	spi->trans_len = max(tx_buf_len, rx_buf_len);

	_spi_control_cs(dev, 1);

	/* Enabling the controller */
	write_sscr0(spi->sscr0 | INTEL_SPI_SSCR0_SSE, spi->regs);

	/* Installing the registers */
	write_sscr1(spi->sscr1 | INTEL_SPI_SSCR1_RIE |
				INTEL_SPI_SSCR1_TIE, spi->regs);

	k_sem_take(&spi->device_sync_sem, K_FOREVER);

	if (spi->error) {
		spi->error = 0;
		return -EIO;
	}

	return 0;
}

void spi_intel_isr(void *arg)
{
	struct device *dev = arg;
	struct spi_intel_data *spi = dev->driver_data;
	uint32_t error = 0;
	uint32_t status;

	SYS_LOG_DBG("spi_intel_isr: %p", dev);

	status = read_sssr(spi->regs);

	if (status & INTEL_SPI_SSSR_ROR) {
		/* Unrecoverable error, ack it */
		clear_bit_sssr_ror(spi->regs);
		error = 1;
		goto out;
	}

	if (status & INTEL_SPI_SSSR_RFS) {
		pull_data(dev);
	}

	if (test_bit_sscr1_tie(spi->regs)) {
		if (status & INTEL_SPI_SSSR_TFS) {
			push_data(dev);
		}
	}
out:
	completed(dev, error);
}

static const struct spi_driver_api intel_spi_api = {
	.configure = spi_intel_configure,
	.slave_select = NULL,
	.transceive = spi_intel_transceive,
};

#ifdef CONFIG_PCI
static inline int spi_intel_setup(struct device *dev)
{
	struct spi_intel_data *spi = dev->driver_data;

	pci_bus_scan_init();

	if (!pci_bus_scan(&spi->pci_dev)) {
		SYS_LOG_DBG("Could not find device");
		return 0;
	}

#ifdef CONFIG_PCI_ENUMERATION
	spi->regs = spi->pci_dev.addr;
#endif

	pci_enable_regs(&spi->pci_dev);

	pci_show(&spi->pci_dev);

	return 1;
}
#else
#define spi_intel_setup(_unused_) (1)
#endif /* CONFIG_PCI */
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT

static void spi_intel_set_power_state(struct device *dev, uint32_t power_state)
{
	struct spi_intel_data *context = dev->driver_data;

	context->device_power_state = power_state;
}
#else
#define spi_intel_set_power_state(...)
#endif

int spi_intel_init(struct device *dev)
{
	const struct spi_intel_config *info = dev->config->config_info;
	struct spi_intel_data *spi = dev->driver_data;

	if (!spi_intel_setup(dev)) {
		return -EPERM;
	}

	info->config_func();

	_spi_config_cs(dev);

	k_sem_init(&spi->device_sync_sem, 0, UINT_MAX);

	spi_intel_set_power_state(dev, DEVICE_PM_ACTIVE_STATE);

	irq_enable(info->irq);

	SYS_LOG_DBG("SPI Intel Driver initialized on device: %p", dev);

	dev->driver_api = &intel_spi_api;

	return 0;
}

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT

static uint32_t spi_intel_get_power_state(struct device *dev)
{
	struct spi_intel_data *context = dev->driver_data;

	return context->device_power_state;
}

static int spi_intel_suspend(struct device *dev)
{
	const struct spi_intel_config *info = dev->config->config_info;
	struct spi_intel_data *spi = dev->driver_data;

	SYS_LOG_DBG("spi_intel_suspend: %p", dev);

	clear_bit_sscr0_sse(spi->regs);
	irq_disable(info->irq);

	spi_intel_set_power_state(dev, DEVICE_PM_SUSPEND_STATE);

	return 0;
}

static int spi_intel_resume_from_suspend(struct device *dev)
{
	const struct spi_intel_config *info = dev->config->config_info;
	struct spi_intel_data *spi = dev->driver_data;

	SYS_LOG_DBG("spi_intel_resume: %p", dev);

	set_bit_sscr0_sse(spi->regs);
	irq_enable(info->irq);

	spi_intel_set_power_state(dev, DEVICE_PM_ACTIVE_STATE);

	return 0;
}

/*
* Implements the driver control management functionality
* the *context may include IN data or/and OUT data
*/
static int spi_intel_device_ctrl(struct device *dev, uint32_t ctrl_command,
				 void *context)
{
	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
		if (*((uint32_t *)context) == DEVICE_PM_SUSPEND_STATE) {
			return spi_intel_suspend(dev);
		} else if (*((uint32_t *)context) == DEVICE_PM_ACTIVE_STATE) {
			return spi_intel_resume_from_suspend(dev);
		}
	} else if (ctrl_command == DEVICE_PM_GET_POWER_STATE) {
		*((uint32_t *)context) = spi_intel_get_power_state(dev);
		return 0;
	}

	return 0;
}
#else
#define spi_intel_set_power_state(...)
#endif

/* system bindings */
#ifdef CONFIG_SPI_0

void spi_config_0_irq(void);

struct spi_intel_data spi_intel_data_port_0 = {
	.regs = SPI_INTEL_PORT_0_REGS,
#if CONFIG_PCI
	.pci_dev.class_type = SPI_INTEL_CLASS,
	.pci_dev.bus = SPI_INTEL_PORT_0_BUS,
	.pci_dev.dev = SPI_INTEL_PORT_0_DEV,
	.pci_dev.vendor_id = SPI_INTEL_VENDOR_ID,
	.pci_dev.device_id = SPI_INTEL_DEVICE_ID,
	.pci_dev.function = SPI_INTEL_PORT_0_FUNCTION,
#endif
};

const struct spi_intel_config spi_intel_config_0 = {
	.irq = SPI_INTEL_PORT_0_IRQ,
#ifdef CONFIG_SPI_CS_GPIO
	.cs_gpio_name = CONFIG_SPI_0_CS_GPIO_PORT,
	.cs_gpio_pin = CONFIG_SPI_0_CS_GPIO_PIN,
#endif
	.config_func = spi_config_0_irq
};

/* SPI may use GPIO pin for CS, thus it needs to be initialized after GPIO */
DEVICE_DEFINE(spi_intel_port_0, CONFIG_SPI_0_NAME, spi_intel_init,
	      spi_intel_device_ctrl, &spi_intel_data_port_0,
	      &spi_intel_config_0, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY, NULL);

void spi_config_0_irq(void)
{
	IRQ_CONNECT(SPI_INTEL_PORT_0_IRQ, CONFIG_SPI_0_IRQ_PRI,
		    spi_intel_isr, DEVICE_GET(spi_intel_port_0),
		    SPI_INTEL_IRQ_FLAGS);
}

#endif /* CONFIG_SPI_0 */
#ifdef CONFIG_SPI_1

void spi_config_1_irq(void);

struct spi_intel_data spi_intel_data_port_1 = {
	.regs = SPI_INTEL_PORT_1_REGS,
#if CONFIG_PCI
	.pci_dev.class_type = SPI_INTEL_CLASS,
	.pci_dev.bus = SPI_INTEL_PORT_1_BUS,
	.pci_dev.dev = SPI_INTEL_PORT_1_DEV,
	.pci_dev.function = SPI_INTEL_PORT_1_FUNCTION,
	.pci_dev.vendor_id = SPI_INTEL_VENDOR_ID,
	.pci_dev.device_id = SPI_INTEL_DEVICE_ID,
#endif
};

const struct spi_intel_config spi_intel_config_1 = {
	.irq = SPI_INTEL_PORT_1_IRQ,
#ifdef CONFIG_SPI_CS_GPIO
	.cs_gpio_name = CONFIG_SPI_1_CS_GPIO_PORT,
	.cs_gpio_pin = CONFIG_SPI_1_CS_GPIO_PIN,
#endif
	.config_func = spi_config_1_irq
};

/* SPI may use GPIO pin for CS, thus it needs to be initialized after GPIO */
DEVICE_DEFINE(spi_intel_port_1, CONFIG_SPI_1_NAME, spi_intel_init,
	      spi_intel_device_ctrl, &spi_intel_data_port_1,
	      &spi_intel_config_1, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY, NULL);

void spi_config_1_irq(void)
{
	IRQ_CONNECT(SPI_INTEL_PORT_1_IRQ, CONFIG_SPI_1_IRQ_PRI,
		    spi_intel_isr, DEVICE_GET(spi_intel_port_1),
		    SPI_INTEL_IRQ_FLAGS);
}

#endif /* CONFIG_SPI_1 */
