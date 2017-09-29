/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <device.h>
#include <drivers/ioapic.h>
#include <init.h>
#include <kernel.h>
#include <spi.h>
#include <gpio.h>
#include <power.h>

#include "qm_spi.h"
#include "clk.h"
#include "qm_isr.h"
#include "soc.h"

struct pending_transfer {
	struct device *dev;
	qm_spi_async_transfer_t xfer;
};

static struct pending_transfer pending_transfers[QM_SPI_NUM];

struct spi_qmsi_config {
	qm_spi_t spi;
	char *cs_port;
	u32_t cs_pin;
};

struct spi_qmsi_runtime {
	struct device *gpio_cs;
	struct k_sem device_sync_sem;
	qm_spi_config_t cfg;
	int rc;
	bool loopback;
	struct k_sem sem;
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	u32_t device_power_state;
	qm_spi_context_t spi_ctx;
#endif
};

static inline qm_spi_bmode_t config_to_bmode(u8_t mode)
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
	const struct spi_qmsi_config *config = dev->config->config_info;
	struct device *gpio = context->gpio_cs;

	if (!gpio) {
		return;
	}

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
			      u16_t len)
{
	const struct spi_qmsi_config *spi_config =
			       ((struct device *)data)->config->config_info;
	qm_spi_t spi = spi_config->spi;
	struct pending_transfer *pending = &pending_transfers[spi];
	struct device *dev = pending->dev;
	struct spi_qmsi_runtime *context;

	if (!dev) {
		return;
	}

	context = dev->driver_data;

	spi_control_cs(dev, false);

	pending->dev = NULL;
	context->rc = error;
	k_sem_give(&context->device_sync_sem);
}

static int spi_qmsi_slave_select(struct device *dev, u32_t slave)
{
	const struct spi_qmsi_config *spi_config = dev->config->config_info;
	qm_spi_t spi = spi_config->spi;

	return qm_spi_slave_select(spi, 1 << (slave - 1)) ? -EIO : 0;
}

static inline u8_t frame_size_to_dfs(qm_spi_frame_size_t frame_size)
{
	if (frame_size <= QM_SPI_FRAME_SIZE_8_BIT) {
		return 1;
	}

	if (frame_size <= QM_SPI_FRAME_SIZE_16_BIT) {
		return 2;
	}

	if (frame_size <= QM_SPI_FRAME_SIZE_32_BIT) {
		return 4;
	}

	/* This should never happen, it will crash later on. */
	return 0;
}

static int spi_qmsi_transceive(struct device *dev,
			       const void *tx_buf, u32_t tx_buf_len,
			       void *rx_buf, u32_t rx_buf_len)
{
	const struct spi_qmsi_config *spi_config = dev->config->config_info;
	qm_spi_t spi = spi_config->spi;
	struct spi_qmsi_runtime *context = dev->driver_data;
	qm_spi_config_t *cfg = &context->cfg;
	u8_t dfs = frame_size_to_dfs(cfg->frame_size);
	qm_spi_async_transfer_t *xfer;
	int rc;

	k_sem_take(&context->sem, K_FOREVER);
	if (pending_transfers[spi].dev) {
		k_sem_give(&context->sem);
		return -EBUSY;
	}
	pending_transfers[spi].dev = dev;
	k_sem_give(&context->sem);

	device_busy_set(dev);

	xfer = &pending_transfers[spi].xfer;

	xfer->rx = rx_buf;
	xfer->rx_len = rx_buf_len / dfs;
	/* This cast is necessary to drop the "const" modifier, since QMSI xfer
	 * does not take a const pointer.
	 */
	xfer->tx = (u8_t *)tx_buf;
	xfer->tx_len = tx_buf_len / dfs;
	xfer->callback_data = dev;
	xfer->callback = transfer_complete;

	if (tx_buf_len == 0) {
		cfg->transfer_mode = QM_SPI_TMOD_RX;
	} else if (rx_buf_len == 0) {
		cfg->transfer_mode = QM_SPI_TMOD_TX;
	} else {
		/* FIXME: QMSI expects rx_buf_len and tx_buf_len to
		 * have the same size.
		 */
		cfg->transfer_mode = QM_SPI_TMOD_TX_RX;
	}

	if (context->loopback) {
		QM_SPI[spi]->ctrlr0 |= BIT(11);
	}

	rc = qm_spi_set_config(spi, cfg);
	if (rc != 0) {
		device_busy_clear(dev);
		return -EINVAL;
	}

	spi_control_cs(dev, true);

	rc = qm_spi_irq_transfer(spi, xfer);
	if (rc != 0) {
		spi_control_cs(dev, false);
		device_busy_clear(dev);
		return -EIO;
	}
	k_sem_take(&context->device_sync_sem, K_FOREVER);

	device_busy_clear(dev);

	return context->rc ? -EIO : 0;
}

static const struct spi_driver_api spi_qmsi_api = {
	.configure = spi_qmsi_configure,
	.slave_select = spi_qmsi_slave_select,
	.transceive = spi_qmsi_transceive,
};

static struct device *gpio_cs_init(const struct spi_qmsi_config *config)
{
	struct device *gpio;

	if (!config->cs_port) {
		return NULL;
	}

	gpio = device_get_binding(config->cs_port);
	if (!gpio) {
		return NULL;
	}

	if (gpio_pin_configure(gpio, config->cs_pin, GPIO_DIR_OUT) != 0) {
		return NULL;
	}

	if (gpio_pin_write(gpio, config->cs_pin, 1) != 0) {
		return NULL;
	}

	return gpio;
}
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
static void spi_master_set_power_state(struct device *dev, u32_t power_state)
{
	struct spi_qmsi_runtime *context = dev->driver_data;

	context->device_power_state = power_state;
}

static u32_t spi_master_get_power_state(struct device *dev)
{
	struct spi_qmsi_runtime *context = dev->driver_data;

	return context->device_power_state;
}
#else
#define spi_master_set_power_state(...)
#endif

static int spi_qmsi_init(struct device *dev)
{
	const struct spi_qmsi_config *spi_config = dev->config->config_info;
	struct spi_qmsi_runtime *context = dev->driver_data;

	switch (spi_config->spi) {
	case QM_SPI_MST_0:
		IRQ_CONNECT(IRQ_GET_NUMBER(QM_IRQ_SPI_MASTER_0_INT),
			    CONFIG_SPI_0_IRQ_PRI, qm_spi_master_0_isr,
			    0, IOAPIC_LEVEL | IOAPIC_HIGH);
		irq_enable(IRQ_GET_NUMBER(QM_IRQ_SPI_MASTER_0_INT));
		clk_periph_enable(CLK_PERIPH_CLK | CLK_PERIPH_SPI_M0_REGISTER);
		QM_IR_UNMASK_INTERRUPTS(
				QM_INTERRUPT_ROUTER->spi_master_0_int_mask);
		break;

#ifdef CONFIG_SPI_1
	case QM_SPI_MST_1:
		IRQ_CONNECT(IRQ_GET_NUMBER(QM_IRQ_SPI_MASTER_1_INT),
			    CONFIG_SPI_1_IRQ_PRI, qm_spi_master_1_isr,
			    0, IOAPIC_LEVEL | IOAPIC_HIGH);
		irq_enable(IRQ_GET_NUMBER(QM_IRQ_SPI_MASTER_1_INT));
		clk_periph_enable(CLK_PERIPH_CLK | CLK_PERIPH_SPI_M1_REGISTER);
		QM_IR_UNMASK_INTERRUPTS(
				QM_INTERRUPT_ROUTER->spi_master_1_int_mask);
		break;
#endif /* CONFIG_SPI_1 */

	default:
		return -EIO;
	}

	context->gpio_cs = gpio_cs_init(spi_config);

	k_sem_init(&context->device_sync_sem, 0, UINT_MAX);
	k_sem_init(&context->sem, 1, UINT_MAX);

	spi_master_set_power_state(dev, DEVICE_PM_ACTIVE_STATE);

	dev->driver_api = &spi_qmsi_api;
	return 0;
}

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
static int spi_master_suspend_device(struct device *dev)
{
	if (device_busy_check(dev)) {
		return -EBUSY;
	}

	const struct spi_qmsi_config *config = dev->config->config_info;
	struct spi_qmsi_runtime *drv_data = dev->driver_data;

	qm_spi_save_context(config->spi, &drv_data->spi_ctx);

	spi_master_set_power_state(dev, DEVICE_PM_SUSPEND_STATE);

	return 0;
}

static int spi_master_resume_device_from_suspend(struct device *dev)
{
	const struct spi_qmsi_config *config = dev->config->config_info;
	struct spi_qmsi_runtime *drv_data = dev->driver_data;

	qm_spi_restore_context(config->spi, &drv_data->spi_ctx);

	spi_master_set_power_state(dev, DEVICE_PM_ACTIVE_STATE);

	return 0;
}

/*
* Implements the driver control management functionality
* the *context may include IN data or/and OUT data
*/
static int spi_master_qmsi_device_ctrl(struct device *port,
				       u32_t ctrl_command, void *context)
{
	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
		if (*((u32_t *)context) == DEVICE_PM_SUSPEND_STATE) {
			return spi_master_suspend_device(port);
		} else if (*((u32_t *)context) == DEVICE_PM_ACTIVE_STATE) {
			return spi_master_resume_device_from_suspend(port);
		}
	} else if (ctrl_command == DEVICE_PM_GET_POWER_STATE) {
		*((u32_t *)context) = spi_master_get_power_state(port);
		return 0;
	}
	return 0;
}
#endif /* CONFIG_DEVICE_POWER_MANAGEMENT */

#ifdef CONFIG_SPI_0
static const struct spi_qmsi_config spi_qmsi_mst_0_config = {
	.spi = QM_SPI_MST_0,
#ifdef CONFIG_SPI_CS_GPIO
	.cs_port = CONFIG_SPI_0_CS_GPIO_PORT,
	.cs_pin = CONFIG_SPI_0_CS_GPIO_PIN,
#endif
};

static struct spi_qmsi_runtime spi_qmsi_mst_0_runtime;

DEVICE_DEFINE(spi_master_0, CONFIG_SPI_0_NAME, spi_qmsi_init,
	      spi_master_qmsi_device_ctrl, &spi_qmsi_mst_0_runtime,
	      &spi_qmsi_mst_0_config, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,
	      NULL);
#endif /* CONFIG_SPI_0 */

#ifdef CONFIG_SPI_1
static const struct spi_qmsi_config spi_qmsi_mst_1_config = {
	.spi = QM_SPI_MST_1,
#ifdef CONFIG_SPI_CS_GPIO
	.cs_port = CONFIG_SPI_1_CS_GPIO_PORT,
	.cs_pin = CONFIG_SPI_1_CS_GPIO_PIN,
#endif
};

static struct spi_qmsi_runtime spi_qmsi_mst_1_runtime;

DEVICE_DEFINE(spi_master_1, CONFIG_SPI_1_NAME, spi_qmsi_init,
	      spi_master_qmsi_device_ctrl, &spi_qmsi_mst_1_runtime,
	      &spi_qmsi_mst_1_config, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,
	      NULL);
#endif /* CONFIG_SPI_1 */
