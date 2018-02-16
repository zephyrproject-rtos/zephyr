/* spi_intel.c - Driver implementation for Intel SPI controller */

/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_DOMAIN "SPI Intel"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SPI_LEVEL
#include <logging/sys_log.h>

#include <errno.h>

#include <kernel.h>
#include <arch/cpu.h>

#include <misc/__assert.h>
#include <board.h>
#include <init.h>

#include <sys_io.h>
#include <power.h>

#include <spi.h>
#include "spi_intel.h"

#ifdef CONFIG_IOAPIC
#include <drivers/ioapic.h>
#endif

static void completed(struct device *dev, u32_t error)
{
	struct spi_intel_data *spi = dev->driver_data;

	if (error) {
		goto out;
	}

	if (spi_context_tx_on(&spi->ctx) ||
	    spi_context_rx_on(&spi->ctx)) {
		return;
	}
out:
	write_sscr1(spi->sscr1, spi->regs);
	clear_bit_sscr0_sse(spi->regs);

	spi_context_cs_control(&spi->ctx, false);

	SYS_LOG_DBG("SPI transaction completed %s error",
		    error ? "with" : "without");

	spi_context_complete(&spi->ctx, error ? -EIO : 0);
}

static void pull_data(struct device *dev)
{
	struct spi_intel_data *spi = dev->driver_data;

	while (read_sssr(spi->regs) & INTEL_SPI_SSSR_RNE) {
		u32_t data = read_ssdr(spi->regs);

		if (spi_context_rx_buf_on(&spi->ctx)) {
			switch (spi->dfs) {
			case 1:
				UNALIGNED_PUT(data, (u8_t *)spi->ctx.rx_buf);
				break;
			case 2:
				UNALIGNED_PUT(data, (u16_t *)spi->ctx.rx_buf);
				break;
			case 4:
				UNALIGNED_PUT(data, (u32_t *)spi->ctx.rx_buf);
				break;
			}

			spi_context_update_rx(&spi->ctx, spi->dfs, 1);
		}
	}
}

static void push_data(struct device *dev)
{
	struct spi_intel_data *spi = dev->driver_data;
	u32_t status;

	while ((status = read_sssr(spi->regs)) & INTEL_SPI_SSSR_TNF) {
		u32_t data = 0;

		if (status & INTEL_SPI_SSSR_RFS) {
			break;
		}

		if (spi_context_tx_buf_on(&spi->ctx)) {
			switch (spi->dfs) {
			case 1:
				data = UNALIGNED_GET((u8_t *)
						     (spi->ctx.tx_buf));
			case 2:
				data = UNALIGNED_GET((u16_t *)
						     (spi->ctx.tx_buf));
				break;
			case 4:
				data = UNALIGNED_GET((u32_t *)
						     (spi->ctx.tx_buf));
				break;
			}
		}

		write_ssdr(data, spi->regs);
		spi_context_update_tx(&spi->ctx, spi->dfs, 1);
	}

	if (!spi_context_tx_on(&spi->ctx)) {
		clear_bit_sscr1_tie(spi->regs);
	}
}

static int spi_intel_configure(struct device *dev,
			       const struct spi_config *config)
{
	struct spi_intel_data *spi = dev->driver_data;

	SYS_LOG_DBG("%p (0x%x), %p", dev, spi->regs, config);

	if (spi_context_configured(&spi->ctx, config)) {
		/* Nothing to do */
		return 0;
	}

	if (config->operation & (SPI_OP_MODE_SLAVE || SPI_TRANSFER_LSB
				 || SPI_LINES_DUAL || SPI_LINES_QUAD ||
				 SPI_LINES_OCTAL)) {
		return -EINVAL;
	}

	/* Determine how many bytes are required per-frame */
	spi->dfs = SPI_WS_TO_DFS(SPI_WORD_SIZE_GET(config->operation));

	/* Pre-configuring the registers to a clean state*/
	write_sscr0(0, spi->regs);
	write_sscr1(0, spi->regs);

	/* Word size and clock rate */
	spi->sscr0 = INTEL_SPI_SSCR0_DSS(SPI_WORD_SIZE_GET(config->operation)) |
		INTEL_SPI_SSCR0_SCR(config->operation);

	/* Tx/Rx thresholds
	 * Note: Rx thresholds needs to be 1, it does not seem to be able
	 * to trigger reliably any interrupt with another value though the
	 * rx fifo would be full
	 */
	spi->sscr1 = INTEL_SPI_SSCR1_TFT(INTEL_SPI_SSCR1_TFT_DFLT) |
		INTEL_SPI_SSCR1_RFT(INTEL_SPI_SSCR1_RFT_DFLT);

	/* SPI mode */
	if (SPI_MODE_GET(config->operation) & SPI_MODE_CPOL) {
		spi->sscr1 |= INTEL_SPI_SSCR1_SPO;
	}

	if (SPI_MODE_GET(config->operation) & SPI_MODE_CPHA) {
		spi->sscr1 |= INTEL_SPI_SSCR1_SPH;
	}

	if (SPI_MODE_GET(config->operation) & SPI_MODE_LOOP) {
		spi->sscr1 |= INTEL_SPI_SSCR1_LBM;
	}

	/* Configuring the rate */
	write_dds_rate(INTEL_SPI_DSS_RATE(config->frequency), spi->regs);

	spi_context_cs_configure(&spi->ctx);

	return 0;
}

static int transceive(struct device *dev,
		      const struct spi_config *config,
		      const struct spi_buf_set *tx_bufs,
		      const struct spi_buf_set *rx_bufs,
		      bool asynchronous,
		      struct k_poll_signal *signal)
{
	struct spi_intel_data *spi = dev->driver_data;
	int ret;

	/* Check status */
	if (test_bit_sscr0_sse(spi->regs) && test_bit_sssr_bsy(spi->regs)) {
		SYS_LOG_DBG("Controller is busy");
		return -EBUSY;
	}

	spi_context_lock(&spi->ctx, asynchronous, signal);

	ret = spi_intel_configure(dev, config);
	if (ret) {
		goto out;
	}

	/* Set buffers info */
	spi_context_buffers_setup(&spi->ctx, tx_bufs, rx_bufs, spi->dfs);

	spi_context_cs_control(&spi->ctx, true);

	/* Installing and Enabling the controller */
	write_sscr0(spi->sscr0 | INTEL_SPI_SSCR0_SSE, spi->regs);
	write_sscr1(spi->sscr1 | INTEL_SPI_SSCR1_RIE | INTEL_SPI_SSCR1_TIE,
		    spi->regs);

	ret = spi_context_wait_for_completion(&spi->ctx);
out:
	spi_context_release(&spi->ctx, ret);

	return ret;
}

static int spi_intel_transceive(struct device *dev,
				const struct spi_config *config,
				const struct spi_buf_set *tx_bufs,
				const struct spi_buf_set *rx_bufs)
{
	SYS_LOG_DBG("%p, %p, %p", dev, tx_bufs, rx_bufs);

	return transceive(dev, config, tx_bufs, rx_bufs, false, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int spi_intel_transceive_async(struct device *dev,
				      const struct spi_config *config,
				      const struct spi_buf_set *tx_bufs,
				      const struct spi_buf_set *rx_bufs,
				      struct k_poll_signal *async)
{
	SYS_LOG_DBG("%p, %p, %p, %p", dev, tx_bufs, rx_bufs, async);

	return transceive(dev, config, tx_bufs, rx_bufs, true, async);
}
#endif /* CONFIG_SPI_ASYNC */

static int spi_intel_release(struct device *dev,
			     const struct spi_config *config)
{
	struct spi_intel_data *spi = dev->driver_data;

	if (test_bit_sscr0_sse(spi->regs) && test_bit_sssr_bsy(spi->regs)) {
		SYS_LOG_DBG("Controller is busy");
		return -EBUSY;
	}

	spi_context_unlock_unconditionally(&spi->ctx);

	return 0;
}

void spi_intel_isr(struct device *dev)
{
	struct spi_intel_data *spi = dev->driver_data;
	u32_t error = 0;
	u32_t status;

	SYS_LOG_DBG("%p", dev);

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
	.transceive = spi_intel_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_intel_transceive_async,
#endif /* CONFIG_SPI_ASYNC */
	.release = spi_intel_release,
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

static void spi_intel_set_power_state(struct device *dev, u32_t power_state)
{
	struct spi_intel_data *spi = dev->driver_data;

	spi->device_power_state = power_state;
}
#else
#define spi_intel_set_power_state(...)
#endif

int spi_intel_init(struct device *dev)
{
	const struct spi_intel_config *info = dev->config->config_info;

	if (!spi_intel_setup(dev)) {
		return -EPERM;
	}

	info->config_func();

	spi_intel_set_power_state(dev, DEVICE_PM_ACTIVE_STATE);

	irq_enable(info->irq);

	SYS_LOG_DBG("SPI Intel Driver initialized on device: %p", dev);

	return 0;
}

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT

static u32_t spi_intel_get_power_state(struct device *dev)
{
	struct spi_intel_data *spi = dev->driver_data;

	return spi->device_power_state;
}

static int spi_intel_suspend(struct device *dev)
{
	const struct spi_intel_config *info = dev->config->config_info;
	struct spi_intel_data *spi = dev->driver_data;

	SYS_LOG_DBG("%p", dev);

	clear_bit_sscr0_sse(spi->regs);
	irq_disable(info->irq);

	spi_intel_set_power_state(dev, DEVICE_PM_SUSPEND_STATE);

	return 0;
}

static int spi_intel_resume_from_suspend(struct device *dev)
{
	const struct spi_intel_config *info = dev->config->config_info;
	struct spi_intel_data *spi = dev->driver_data;

	SYS_LOG_DBG("%p", dev);

	set_bit_sscr0_sse(spi->regs);
	irq_enable(info->irq);

	spi_intel_set_power_state(dev, DEVICE_PM_ACTIVE_STATE);

	return 0;
}

/*
* Implements the driver control management functionality
* the *context may include IN data or/and OUT data
*/
static int spi_intel_device_ctrl(struct device *dev, u32_t ctrl_command,
				 void *context)
{
	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
		if (*((u32_t *)context) == DEVICE_PM_SUSPEND_STATE) {
			return spi_intel_suspend(dev);
		} else if (*((u32_t *)context) == DEVICE_PM_ACTIVE_STATE) {
			return spi_intel_resume_from_suspend(dev);
		}
	} else if (ctrl_command == DEVICE_PM_GET_POWER_STATE) {
		*((u32_t *)context) = spi_intel_get_power_state(dev);
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
	SPI_CONTEXT_INIT_LOCK(spi_intel_data_port_0, ctx),
	SPI_CONTEXT_INIT_SYNC(spi_intel_data_port_0, ctx),
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
	.config_func = spi_config_0_irq
};

DEVICE_DEFINE(spi_intel_port_0, CONFIG_SPI_0_NAME, spi_intel_init,
	      spi_intel_device_ctrl, &spi_intel_data_port_0,
	      &spi_intel_config_0, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,
	      &intel_spi_api);

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
	SPI_CONTEXT_INIT_LOCK(spi_intel_data_port_1, ctx),
	SPI_CONTEXT_INIT_SYNC(spi_intel_data_port_1, ctx),
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
	.config_func = spi_config_1_irq
};

DEVICE_DEFINE(spi_intel_port_1, CONFIG_SPI_1_NAME, spi_intel_init,
	      spi_intel_device_ctrl, &spi_intel_data_port_1,
	      &spi_intel_config_1, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,
	      &intel_spi_api);

void spi_config_1_irq(void)
{
	IRQ_CONNECT(SPI_INTEL_PORT_1_IRQ, CONFIG_SPI_1_IRQ_PRI,
		    spi_intel_isr, DEVICE_GET(spi_intel_port_1),
		    SPI_INTEL_IRQ_FLAGS);
}

#endif /* CONFIG_SPI_1 */
