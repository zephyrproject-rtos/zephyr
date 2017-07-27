/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <device.h>
#include <spi.h>
#include <gpio.h>
#include <board.h>

#include "qm_ss_spi.h"
#include "qm_ss_isr.h"
#include "ss_clk.h"

struct ss_pending_transfer {
	struct device *dev;
	qm_ss_spi_async_transfer_t xfer;
};

static struct ss_pending_transfer pending_transfers[2];

struct ss_spi_qmsi_config {
	qm_ss_spi_t spi;
#ifdef CONFIG_SPI_SS_CS_GPIO
	char *cs_port;
	u32_t cs_pin;
#endif
};

struct ss_spi_qmsi_runtime {
#ifdef CONFIG_SPI_SS_CS_GPIO
	struct device *gpio_cs;
#endif
	struct k_sem device_sync_sem;
	struct k_sem sem;
	qm_ss_spi_config_t cfg;
	int rc;
	bool loopback;
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	u32_t device_power_state;
	qm_ss_spi_context_t spi_ctx;
#endif
};

static inline qm_ss_spi_bmode_t config_to_bmode(u8_t mode)
{
	switch (mode) {
	case SPI_MODE_CPHA:
		return QM_SS_SPI_BMODE_1;
	case SPI_MODE_CPOL:
		return QM_SS_SPI_BMODE_2;
	case SPI_MODE_CPOL | SPI_MODE_CPHA:
		return QM_SS_SPI_BMODE_3;
	default:
		return QM_SS_SPI_BMODE_0;
	}
}

#ifdef CONFIG_SPI_SS_CS_GPIO
static void spi_control_cs(struct device *dev, bool active)
{
	struct ss_spi_qmsi_runtime *context = dev->driver_data;
	const struct ss_spi_qmsi_config *config = dev->config->config_info;
	struct device *gpio = context->gpio_cs;

	if (!gpio)
		return;

	gpio_pin_write(gpio, config->cs_pin, !active);
}
#endif

static int ss_spi_qmsi_configure(struct device *dev,
				 struct spi_config *config)
{
	struct ss_spi_qmsi_runtime *context = dev->driver_data;
	qm_ss_spi_config_t *cfg = &context->cfg;

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

static void spi_qmsi_callback(void *data, int error, qm_ss_spi_status_t status,
			      u16_t len)
{
	const struct ss_spi_qmsi_config *spi_config =
			       ((struct device *)data)->config->config_info;
	qm_ss_spi_t spi_id = spi_config->spi;
	struct ss_pending_transfer *pending = &pending_transfers[spi_id];
	struct device *dev = pending->dev;
	struct ss_spi_qmsi_runtime *context;

	if (!dev)
		return;

	context = dev->driver_data;

#ifdef CONFIG_SPI_SS_CS_GPIO
	spi_control_cs(dev, false);
#endif

	pending->dev = NULL;
	context->rc = error;
	k_sem_give(&context->device_sync_sem);
}

static int ss_spi_qmsi_slave_select(struct device *dev, u32_t slave)
{
	const struct ss_spi_qmsi_config *spi_config = dev->config->config_info;
	qm_ss_spi_t spi_id = spi_config->spi;

	return qm_ss_spi_slave_select(spi_id, 1 << (slave - 1)) ? -EIO : 0;
}

static inline u8_t frame_size_to_dfs(qm_ss_spi_frame_size_t frame_size)
{
	if (frame_size <= QM_SS_SPI_FRAME_SIZE_8_BIT) {
		return 1;
	}

	if (frame_size <= QM_SS_SPI_FRAME_SIZE_16_BIT) {
		return 2;
	}

	/* This should never happen, it will crash later on. */
	return 0;
}

static int ss_spi_qmsi_transceive(struct device *dev,
				  const void *tx_buf, u32_t tx_buf_len,
				  void *rx_buf, u32_t rx_buf_len)
{
	const struct ss_spi_qmsi_config *spi_config = dev->config->config_info;
	qm_ss_spi_t spi_id = spi_config->spi;
	struct ss_spi_qmsi_runtime *context = dev->driver_data;
	qm_ss_spi_config_t *cfg = &context->cfg;
	u8_t dfs = frame_size_to_dfs(cfg->frame_size);
	qm_ss_spi_async_transfer_t *xfer;
	int rc;

	k_sem_take(&context->sem, K_FOREVER);
	if (pending_transfers[spi_id].dev) {
		k_sem_give(&context->sem);
		return -EBUSY;
	}
	pending_transfers[spi_id].dev = dev;
	k_sem_give(&context->sem);

	device_busy_set(dev);

	xfer = &pending_transfers[spi_id].xfer;

	xfer->rx = rx_buf;
	xfer->rx_len = rx_buf_len / dfs;
	xfer->tx = (u8_t *)tx_buf;
	xfer->tx_len = tx_buf_len / dfs;
	xfer->callback_data = dev;
	xfer->callback = spi_qmsi_callback;

	if (tx_buf_len == 0) {
		cfg->transfer_mode = QM_SS_SPI_TMOD_RX;
	} else if (rx_buf_len == 0) {
		cfg->transfer_mode = QM_SS_SPI_TMOD_TX;
	} else {
		cfg->transfer_mode = QM_SS_SPI_TMOD_TX_RX;
	}

	if (context->loopback) {
		u32_t ctrl;

		if (spi_id == 0) {
			ctrl = __builtin_arc_lr(QM_SS_SPI_0_BASE +
						QM_SS_SPI_CTRL);
			ctrl |= BIT(11);
			__builtin_arc_sr(ctrl, QM_SS_SPI_0_BASE +
					 QM_SS_SPI_CTRL);
		} else {
			ctrl = __builtin_arc_lr(QM_SS_SPI_1_BASE +
						QM_SS_SPI_CTRL);
			ctrl |= BIT(11);
			__builtin_arc_sr(ctrl, QM_SS_SPI_1_BASE +
					 QM_SS_SPI_CTRL);
		}
	}

	rc = qm_ss_spi_set_config(spi_id, cfg);
	if (rc != 0) {
		device_busy_clear(dev);
		return -EINVAL;
	}

#ifdef CONFIG_SPI_SS_CS_GPIO
	spi_control_cs(dev, true);
#endif

	rc = qm_ss_spi_irq_transfer(spi_id, xfer);
	if (rc != 0) {
#ifdef CONFIG_SPI_SS_CS_GPIO
		spi_control_cs(dev, false);
#endif
		device_busy_clear(dev);
		return -EIO;
	}

	k_sem_take(&context->device_sync_sem, K_FOREVER);

	device_busy_clear(dev);
	return context->rc ? -EIO : 0;
}

static const struct spi_driver_api ss_spi_qmsi_api = {
	.configure = ss_spi_qmsi_configure,
	.slave_select = ss_spi_qmsi_slave_select,
	.transceive = ss_spi_qmsi_transceive,
};

#ifdef CONFIG_SPI_SS_CS_GPIO
static struct device *gpio_cs_init(const struct ss_spi_qmsi_config *config)
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
#endif

static int ss_spi_qmsi_init(struct device *dev);


#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
static void ss_spi_master_set_power_state(struct device *dev,
					  u32_t power_state)
{
	struct ss_spi_qmsi_runtime *context = dev->driver_data;

	context->device_power_state = power_state;
}

static u32_t ss_spi_master_get_power_state(struct device *dev)
{
	struct ss_spi_qmsi_runtime *context = dev->driver_data;

	return context->device_power_state;
}

static int ss_spi_master_suspend_device(struct device *dev)
{
	if (device_busy_check(dev)) {
		return -EBUSY;
	}

	const struct ss_spi_qmsi_config *config = dev->config->config_info;
	struct ss_spi_qmsi_runtime *drv_data = dev->driver_data;

	qm_ss_spi_save_context(config->spi, &drv_data->spi_ctx);

	ss_spi_master_set_power_state(dev, DEVICE_PM_SUSPEND_STATE);

	return 0;
}

static int ss_spi_master_resume_device_from_suspend(struct device *dev)
{
	const struct ss_spi_qmsi_config *config = dev->config->config_info;
	struct ss_spi_qmsi_runtime *drv_data = dev->driver_data;

	qm_ss_spi_restore_context(config->spi, &drv_data->spi_ctx);

	ss_spi_master_set_power_state(dev, DEVICE_PM_ACTIVE_STATE);

	return 0;
}

/*
* Implements the driver control management functionality
* the *context may include IN data or/and OUT data
*/
static int ss_spi_master_qmsi_device_ctrl(struct device *port,
				       u32_t ctrl_command, void *context)
{
	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
		if (*((u32_t *)context) == DEVICE_PM_SUSPEND_STATE) {
			return ss_spi_master_suspend_device(port);
		} else if (*((u32_t *)context) == DEVICE_PM_ACTIVE_STATE) {
			return ss_spi_master_resume_device_from_suspend(port);
		}
	} else if (ctrl_command == DEVICE_PM_GET_POWER_STATE) {
		*((u32_t *)context) = ss_spi_master_get_power_state(port);
	}
	return 0;
}
#else
#define ss_spi_master_set_power_state(...)
#endif /* CONFIG_DEVICE_POWER_MANAGEMENT */

#ifdef CONFIG_SPI_SS_0
static const struct ss_spi_qmsi_config spi_qmsi_mst_0_config = {
	.spi = QM_SS_SPI_0,
#ifdef CONFIG_SPI_SS_CS_GPIO
	.cs_port = CONFIG_SPI_SS_0_CS_GPIO_PORT,
	.cs_pin = CONFIG_SPI_SS_0_CS_GPIO_PIN,
#endif
};

static struct ss_spi_qmsi_runtime spi_qmsi_mst_0_runtime;

DEVICE_DEFINE(ss_spi_master_0, CONFIG_SPI_SS_0_NAME, ss_spi_qmsi_init,
	      ss_spi_master_qmsi_device_ctrl, &spi_qmsi_mst_0_runtime,
	      &spi_qmsi_mst_0_config, POST_KERNEL, CONFIG_SPI_SS_INIT_PRIORITY,
	      NULL);
#endif /* CONFIG_SPI_SS_0 */

#ifdef CONFIG_SPI_SS_1
static const struct ss_spi_qmsi_config spi_qmsi_mst_1_config = {
	.spi = QM_SS_SPI_1,
#ifdef CONFIG_SPI_SS_CS_GPIO
	.cs_port = CONFIG_SPI_SS_1_CS_GPIO_PORT,
	.cs_pin = CONFIG_SPI_SS_1_CS_GPIO_PIN,
#endif
};

static struct ss_spi_qmsi_runtime spi_qmsi_mst_1_runtime;

DEVICE_DEFINE(ss_spi_master_1, CONFIG_SPI_SS_1_NAME, ss_spi_qmsi_init,
	      ss_spi_master_qmsi_device_ctrl, &spi_qmsi_mst_1_runtime,
	      &spi_qmsi_mst_1_config, POST_KERNEL, CONFIG_SPI_SS_INIT_PRIORITY,
	      NULL);
#endif /* CONFIG_SPI_SS_1 */

static void ss_spi_err_isr(void *arg)
{
	struct device *dev = arg;
	const struct ss_spi_qmsi_config *spi_config = dev->config->config_info;

	if (spi_config->spi == QM_SS_SPI_0) {
		qm_ss_spi_0_error_isr(NULL);
	} else {
		qm_ss_spi_1_error_isr(NULL);
	}
}

static void ss_spi_rx_isr(void *arg)
{
	struct device *dev = arg;
	const struct ss_spi_qmsi_config *spi_config = dev->config->config_info;

	if (spi_config->spi == QM_SS_SPI_0) {
		qm_ss_spi_0_rx_avail_isr(NULL);
	} else {
		qm_ss_spi_1_rx_avail_isr(NULL);
	}
}

static void ss_spi_tx_isr(void *arg)
{
	struct device *dev = arg;
	const struct ss_spi_qmsi_config *spi_config = dev->config->config_info;

	if (spi_config->spi == QM_SS_SPI_0) {
		qm_ss_spi_0_tx_req_isr(NULL);
	} else {
		qm_ss_spi_1_tx_req_isr(NULL);
	}
}

static int ss_spi_qmsi_init(struct device *dev)
{
	const struct ss_spi_qmsi_config *spi_config = dev->config->config_info;
	struct ss_spi_qmsi_runtime *context = dev->driver_data;
	u32_t *scss_intmask = NULL;

	switch (spi_config->spi) {
#ifdef CONFIG_SPI_SS_0
	case QM_SS_SPI_0:
		IRQ_CONNECT(IRQ_SPI0_ERR_INT, CONFIG_SPI_SS_0_IRQ_PRI,
			    ss_spi_err_isr, DEVICE_GET(ss_spi_master_0), 0);
		irq_enable(IRQ_SPI0_ERR_INT);

		IRQ_CONNECT(IRQ_SPI0_RX_AVAIL, CONFIG_SPI_SS_0_IRQ_PRI,
			    ss_spi_rx_isr, DEVICE_GET(ss_spi_master_0), 0);
		irq_enable(IRQ_SPI0_RX_AVAIL);

		IRQ_CONNECT(IRQ_SPI0_TX_REQ, CONFIG_SPI_SS_0_IRQ_PRI,
			    ss_spi_tx_isr, DEVICE_GET(ss_spi_master_0), 0);
		irq_enable(IRQ_SPI0_TX_REQ);

		ss_clk_spi_enable(0);

		/* Route SPI interrupts to Sensor Subsystem */
		scss_intmask = (u32_t *)&QM_INTERRUPT_ROUTER->ss_spi_0_int;
		*scss_intmask &= ~BIT(8);
		scss_intmask++;
		*scss_intmask &= ~BIT(8);
		scss_intmask++;
		*scss_intmask &= ~BIT(8);
		break;
#endif /* CONFIG_SPI_SS_0 */

#ifdef CONFIG_SPI_SS_1
	case QM_SS_SPI_1:
		IRQ_CONNECT(IRQ_SPI1_ERR_INT, CONFIG_SPI_SS_1_IRQ_PRI,
			    ss_spi_err_isr, DEVICE_GET(ss_spi_master_1), 0);
		irq_enable(IRQ_SPI1_ERR_INT);

		IRQ_CONNECT(IRQ_SPI1_RX_AVAIL, CONFIG_SPI_SS_1_IRQ_PRI,
			    ss_spi_rx_isr, DEVICE_GET(ss_spi_master_1), 0);
		irq_enable(IRQ_SPI1_RX_AVAIL);

		IRQ_CONNECT(IRQ_SPI1_TX_REQ, CONFIG_SPI_SS_1_IRQ_PRI,
			    ss_spi_tx_isr, DEVICE_GET(ss_spi_master_1), 0);
		irq_enable(IRQ_SPI1_TX_REQ);

		ss_clk_spi_enable(1);

		/* Route SPI interrupts to Sensor Subsystem */
		scss_intmask = (u32_t *)&QM_INTERRUPT_ROUTER->ss_spi_1_int;
		*scss_intmask &= ~BIT(8);
		scss_intmask++;
		*scss_intmask &= ~BIT(8);
		scss_intmask++;
		*scss_intmask &= ~BIT(8);
		break;
#endif /* CONFIG_SPI_SS_1 */

	default:
		return -EIO;
	}

#ifdef CONFIG_SPI_SS_CS_GPIO
	context->gpio_cs = gpio_cs_init(spi_config);
#endif
	k_sem_init(&context->device_sync_sem, 0, UINT_MAX);
	k_sem_init(&context->sem, 1, UINT_MAX);

	ss_spi_master_set_power_state(dev, DEVICE_PM_ACTIVE_STATE);

	dev->driver_api = &ss_spi_qmsi_api;

	return 0;
}
