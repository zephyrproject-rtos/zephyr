/*
 * Copyright (c) 2016 Intel Corporation.
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

#include <errno.h>

#include <device.h>
#include <drivers/ioapic.h>
#include <init.h>
#include <nanokernel.h>
#include <spi.h>
#include <gpio.h>

#include "qm_spi.h"
#include "clk.h"
#include "qm_isr.h"

struct pending_transfer {
	struct device *dev;
	qm_spi_async_transfer_t xfer;
};

static struct pending_transfer pending_transfers[QM_SPI_NUM];

struct spi_qmsi_config {
	qm_spi_t spi;
	char *cs_port;
	uint32_t cs_pin;
};

struct spi_qmsi_runtime {
	struct device *gpio_cs;
	device_sync_call_t sync;
	qm_spi_config_t cfg;
	int rc;
	bool loopback;
	struct nano_sem sem;
};

static inline qm_spi_bmode_t config_to_bmode(uint8_t mode)
{
	switch (mode) {
	case SPI_MODE_CPHA:
		return QM_SPI_BMODE_1;
	case SPI_MODE_CPOL:
		return QM_SPI_BMODE_2;
	case SPI_MODE_CPOL | SPI_MODE_CPHA:
		return QM_SPI_BMODE_3;
	default:
		return QM_SPI_BMODE_0;
	}
}

static void spi_control_cs(struct device *dev, bool active)
{
	struct spi_qmsi_runtime *context = dev->driver_data;
	struct spi_qmsi_config *config = dev->config->config_info;
	struct device *gpio = context->gpio_cs;

	if (!gpio)
		return;

	gpio_pin_write(gpio, config->cs_pin, !active);
}

static int spi_qmsi_configure(struct device *dev,
				struct spi_config *config)
{
	struct spi_qmsi_runtime *context = dev->driver_data;
	qm_spi_config_t *cfg = &context->cfg;

	cfg->frame_size = SPI_WORD_SIZE_GET(config->config) - 1;
	cfg->bus_mode = config_to_bmode(SPI_MODE(config->config));
	/* As loopback is implemented inside the controller,
	 * the bus mode doesn't matter.
	 */
	context->loopback = SPI_MODE(config->config) & SPI_MODE_LOOP;
	cfg->clk_divider = config->max_sys_freq;

	/* Will set the configuration before the transfer starts */
	return 0;
}

static void transfer_complete(void *data, int error, qm_spi_status_t status,
			      uint16_t len)
{
	struct spi_qmsi_config *spi_config =
			       ((struct device *)data)->config->config_info;
	qm_spi_t spi = spi_config->spi;
	struct pending_transfer *pending = &pending_transfers[spi];
	struct device *dev = pending->dev;
	struct spi_qmsi_runtime *context;

	if (!dev)
		return;

	context = dev->driver_data;

	spi_control_cs(dev, false);

	pending->dev = NULL;
	context->rc = error;
	device_sync_call_complete(&context->sync);
}

static int spi_qmsi_slave_select(struct device *dev, uint32_t slave)
{
	struct spi_qmsi_config *spi_config = dev->config->config_info;
	qm_spi_t spi = spi_config->spi;

	return qm_spi_slave_select(spi, 1 << (slave - 1)) ? -EIO : 0;
}

static inline uint8_t frame_size_to_dfs(qm_spi_frame_size_t frame_size)
{
	if (frame_size <= QM_SPI_FRAME_SIZE_8_BIT)
		return 1;
	if (frame_size <= QM_SPI_FRAME_SIZE_16_BIT)
		return 2;
	if (frame_size <= QM_SPI_FRAME_SIZE_32_BIT)
		return 4;

	/* This should never happen, it will crash later on. */
	return 0;
}

static int spi_qmsi_transceive(struct device *dev,
			       const void *tx_buf, uint32_t tx_buf_len,
			       void *rx_buf, uint32_t rx_buf_len)
{
	struct spi_qmsi_config *spi_config = dev->config->config_info;
	qm_spi_t spi = spi_config->spi;
	struct spi_qmsi_runtime *context = dev->driver_data;
	qm_spi_config_t *cfg = &context->cfg;
	uint8_t dfs = frame_size_to_dfs(cfg->frame_size);
	qm_spi_async_transfer_t *xfer;
	int rc;

	nano_sem_take(&context->sem, TICKS_UNLIMITED);
	if (pending_transfers[spi].dev) {
		nano_sem_give(&context->sem);
		return -EBUSY;
	}
	pending_transfers[spi].dev = dev;
	nano_sem_give(&context->sem);

	xfer = &pending_transfers[spi].xfer;

	xfer->rx = rx_buf;
	xfer->rx_len = rx_buf_len / dfs;
	/* This cast is necessary to drop the "const" modifier, since QMSI xfer
	 * does not take a const pointer.
	 */
	xfer->tx = (uint8_t *)tx_buf;
	xfer->tx_len = tx_buf_len / dfs;
	xfer->callback_data = dev;
	xfer->callback = transfer_complete;

	if (tx_buf_len == 0)
		cfg->transfer_mode = QM_SPI_TMOD_RX;
	else if (rx_buf_len == 0)
		cfg->transfer_mode = QM_SPI_TMOD_TX;
	else {
		/* FIXME: QMSI expects rx_buf_len and tx_buf_len to
		 * have the same size.
		 */
		cfg->transfer_mode = QM_SPI_TMOD_TX_RX;
	}

	if (context->loopback)
		QM_SPI[spi]->ctrlr0 |= BIT(11);

	rc = qm_spi_set_config(spi, cfg);
	if (rc != 0) {
		return -EINVAL;
	}

	spi_control_cs(dev, true);

	rc = qm_spi_irq_transfer(spi, xfer);
	if (rc != 0) {
		spi_control_cs(dev, false);
		return -EIO;
	}
	device_sync_call_wait(&context->sync);

	return context->rc ? -EIO : 0;
}

static int spi_qmsi_suspend(struct device *dev)
{
	/* FIXME */
	return 0;
}

static int spi_qmsi_resume(struct device *dev)
{
	/* FIXME */
	return 0;
}

static struct spi_driver_api spi_qmsi_api = {
	.configure = spi_qmsi_configure,
	.slave_select = spi_qmsi_slave_select,
	.transceive = spi_qmsi_transceive,
	.suspend = spi_qmsi_suspend,
	.resume = spi_qmsi_resume,
};

static struct device *gpio_cs_init(struct spi_qmsi_config *config)
{
	struct device *gpio;

	if (!config->cs_port)
		return NULL;

	gpio = device_get_binding(config->cs_port);
	if (!gpio)
		return NULL;

	gpio_pin_configure(gpio, config->cs_pin, GPIO_DIR_OUT);
	gpio_pin_write(gpio, config->cs_pin, 1);

	return gpio;
}

static int spi_qmsi_init(struct device *dev)
{
	struct spi_qmsi_config *spi_config = dev->config->config_info;
	struct spi_qmsi_runtime *context = dev->driver_data;

	switch (spi_config->spi) {
	case QM_SPI_MST_0:
		IRQ_CONNECT(QM_IRQ_SPI_MASTER_0,
			    CONFIG_SPI_0_IRQ_PRI, qm_spi_master_0_isr,
			    0, IOAPIC_LEVEL | IOAPIC_HIGH);
		irq_enable(QM_IRQ_SPI_MASTER_0);
		clk_periph_enable(CLK_PERIPH_CLK | CLK_PERIPH_SPI_M0_REGISTER);
		QM_SCSS_INT->int_spi_mst_0_mask &= ~BIT(0);
		break;

#ifdef CONFIG_SPI_1
	case QM_SPI_MST_1:
		IRQ_CONNECT(QM_IRQ_SPI_MASTER_1,
			    CONFIG_SPI_1_IRQ_PRI, qm_spi_master_1_isr,
			    0, IOAPIC_LEVEL | IOAPIC_HIGH);
		irq_enable(QM_IRQ_SPI_MASTER_1);
		clk_periph_enable(CLK_PERIPH_CLK | CLK_PERIPH_SPI_M1_REGISTER);
		QM_SCSS_INT->int_spi_mst_1_mask &= ~BIT(0);
		break;
#endif /* CONFIG_SPI_1 */

	default:
		return -EIO;
	}

	context->gpio_cs = gpio_cs_init(spi_config);

	device_sync_call_init(&context->sync);
	nano_sem_init(&context->sem);
	nano_sem_give(&context->sem);

	dev->driver_api = &spi_qmsi_api;

	return 0;
}

#ifdef CONFIG_SPI_0
static struct spi_qmsi_config spi_qmsi_mst_0_config = {
	.spi = QM_SPI_MST_0,
#ifdef CONFIG_SPI_CS_GPIO
	.cs_port = CONFIG_SPI_0_CS_GPIO_PORT,
	.cs_pin = CONFIG_SPI_0_CS_GPIO_PIN,
#endif
};

static struct spi_qmsi_runtime spi_qmsi_mst_0_runtime;

DEVICE_INIT(spi_master_0, CONFIG_SPI_0_NAME,
	    spi_qmsi_init, &spi_qmsi_mst_0_runtime, &spi_qmsi_mst_0_config,
	    SECONDARY, CONFIG_SPI_INIT_PRIORITY);


#endif /* CONFIG_SPI_0 */
#ifdef CONFIG_SPI_1

static struct spi_qmsi_config spi_qmsi_mst_1_config = {
	.spi = QM_SPI_MST_1,
#ifdef CONFIG_SPI_CS_GPIO
	.cs_port = CONFIG_SPI_1_CS_GPIO_PORT,
	.cs_pin = CONFIG_SPI_1_CS_GPIO_PIN,
#endif
};

static struct spi_qmsi_runtime spi_qmsi_mst_1_runtime;

DEVICE_INIT(spi_master_1, CONFIG_SPI_1_NAME,
	    spi_qmsi_init, &spi_qmsi_mst_1_runtime, &spi_qmsi_mst_1_config,
	    SECONDARY, CONFIG_SPI_INIT_PRIORITY);

#endif /* CONFIG_SPI_1 */
