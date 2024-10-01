/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT intel_sedi_spi

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi/rtio.h>
#include <zephyr/pm/device.h>

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_sedi);

#include "sedi_driver_spi.h"
#include "spi_context.h"


struct spi_sedi_config {
	DEVICE_MMIO_ROM;
	sedi_spi_t spi_device;
	void (*irq_config)(void);
};

struct spi_sedi_data {
	DEVICE_MMIO_RAM;
	struct spi_context ctx;
	bool tx_data_updated;
	bool rx_data_updated;
	uint32_t tx_dummy_len;
	uint32_t rx_dummy_len;
};

static int spi_sedi_configure(const struct device *dev,
			      const struct spi_config *config)
{
	struct spi_sedi_data *data = dev->data;
	const struct spi_sedi_config *info = dev->config;
	uint32_t word_size, cpol, cpha, loopback;

	if (spi_context_configured(&data->ctx, config) == true) {
		return 0;
	}

	word_size = SPI_WORD_SIZE_GET(config->operation);
	sedi_spi_control(info->spi_device, SEDI_SPI_IOCTL_DATA_WIDTH,
			 word_size);

	/* CPOL and CPHA */
	cpol = SPI_MODE_GET(config->operation) & SPI_MODE_CPOL;
	cpha = SPI_MODE_GET(config->operation) & SPI_MODE_CPHA;

	if ((cpol == 0) && (cpha == 0)) {
		sedi_spi_control(info->spi_device, SEDI_SPI_IOCTL_CPOL0_CPHA0,
				 0);
	} else if ((cpol == 0) && (cpha == 1U)) {
		sedi_spi_control(info->spi_device, SEDI_SPI_IOCTL_CPOL0_CPHA1,
				 0);
	} else if ((cpol == 1) && (cpha == 0U)) {
		sedi_spi_control(info->spi_device, SEDI_SPI_IOCTL_CPOL1_CPHA0,
				 0);
	} else {
		sedi_spi_control(info->spi_device, SEDI_SPI_IOCTL_CPOL1_CPHA1,
				 0);
	}

	/* MSB and LSB */
	if (config->operation & SPI_TRANSFER_LSB) {
		sedi_spi_control(info->spi_device, SEDI_SPI_IOCTL_LSB, 0);
	}

	/* Set loopack */
	loopback = SPI_MODE_GET(config->operation) & SPI_MODE_LOOP;
	sedi_spi_control(info->spi_device, SEDI_SPI_IOCTL_LOOPBACK, loopback);

	/* Set baudrate */
	sedi_spi_control(info->spi_device, SEDI_SPI_IOCTL_SPEED_SET,
			 config->frequency);

	sedi_spi_control(info->spi_device, SEDI_SPI_IOCTL_CS_HW, config->slave);

	data->ctx.config = config;
	spi_context_cs_control(&data->ctx, true);

	return 0;
}

static int transceive(const struct device *dev, const struct spi_config *config,
		      const struct spi_buf_set *tx_bufs,
		      const struct spi_buf_set *rx_bufs, bool asynchronous,
		      spi_callback_t cb,
		      void *userdata)
{
	const struct spi_sedi_config *info = dev->config;
	struct spi_sedi_data *spi = dev->data;
	struct spi_context *ctx = &spi->ctx;
	int ret;
	uint32_t transfer_bytes = 0;
	uint8_t *data_out = NULL, *data_in = NULL;
	uint32_t i, dummy_len = 0;
	const struct spi_buf *buf;
	bool is_multibufs = false;

	spi_context_lock(&spi->ctx, asynchronous, cb, userdata, config);
	pm_device_busy_set(dev);

	/* Power up use default setting */
	ret = sedi_spi_set_power(info->spi_device, SEDI_POWER_FULL);
	if (ret) {
		goto out;
	}

	/* If need to configure, re-configure */
	spi_sedi_configure(dev, config);

	spi->tx_data_updated = false;
	spi->rx_data_updated = false;
	/* Set buffers info */
	spi_context_buffers_setup(&spi->ctx, tx_bufs, rx_bufs, 1);

	if ((ctx->tx_count > 1) || (ctx->rx_count > 1)) {
		is_multibufs = true;
	}

	if (ctx->tx_count > ctx->rx_count) {
		spi->tx_dummy_len = 0;
		for (i = ctx->rx_count; i < ctx->tx_count; i++) {
			buf = ctx->current_tx + i;
			dummy_len += buf->len;
		}
		spi->rx_dummy_len = dummy_len;
	} else if (ctx->tx_count < ctx->rx_count) {
		spi->rx_dummy_len = 0;
		for (i = ctx->tx_count; i < ctx->rx_count; i++) {
			buf = ctx->current_rx + i;
			dummy_len += buf->len;
		}
		spi->tx_dummy_len = dummy_len;
	} else {
		spi->tx_dummy_len = 0;
		spi->rx_dummy_len = 0;
	}

	if ((ctx->tx_len == 0) && (ctx->rx_len == 0)) {
		spi_context_cs_control(&spi->ctx, true);
		spi_context_complete(&spi->ctx, dev, 0);
		return 0;
	}

	/* For multiple buffers, using continuous mode */
	if (is_multibufs) {
		sedi_spi_control(info->spi_device, SEDI_SPI_IOCTL_BUFFER_SETS, 1);
	}

	if (ctx->tx_len == 0) {
		/* rx only, nothing to tx */
		data_out = NULL;
		data_in = (uint8_t *)ctx->rx_buf;
		transfer_bytes = ctx->rx_len;
		spi->tx_dummy_len -= transfer_bytes;
	} else if (ctx->rx_len == 0) {
		/* tx only, nothing to rx */
		data_out = (uint8_t *)ctx->tx_buf;
		data_in = NULL;
		transfer_bytes = ctx->tx_len;
		spi->rx_dummy_len -= transfer_bytes;
	} else if (ctx->tx_len == ctx->rx_len) {
		/* rx and tx are the same length */
		data_out = (uint8_t *)ctx->tx_buf;
		data_in = (uint8_t *)ctx->rx_buf;
		transfer_bytes = ctx->tx_len;
	} else if (ctx->tx_len > ctx->rx_len) {
		/* Break up the tx into multiple transfers so we don't have to
		 * rx into a longer intermediate buffer. Leave chip select
		 * active between transfers.
		 */
		data_out = (uint8_t *)ctx->tx_buf;
		data_in = ctx->rx_buf;
		transfer_bytes = ctx->rx_len;
	} else {
		/* Break up the rx into multiple transfers so we don't have to
		 * tx from a longer intermediate buffer. Leave chip select
		 * active between transfers.
		 */
		data_out = (uint8_t *)ctx->tx_buf;
		data_in = ctx->rx_buf;
		transfer_bytes = ctx->tx_len;
	}

	spi_context_cs_control(&spi->ctx, false);

	ret = sedi_spi_transfer(info->spi_device, data_out, data_in,
				transfer_bytes);

	if (ret != SEDI_DRIVER_OK) {
		goto out;
	}

	ret = spi_context_wait_for_completion(&spi->ctx);
	if (ret != 0) {
		sedi_spi_status_t spi_status = {0};

		sedi_spi_get_status(info->spi_device, &spi_status);

		/* SPI ABORT */
		sedi_spi_control(info->spi_device, SEDI_SPI_IOCTL_ABORT, 0);
		/* Toggle GPIO back */
		spi_context_cs_control(&spi->ctx, true);
	}
out:
	spi_context_release(&spi->ctx, ret);
	pm_device_busy_clear(dev);

	return ret;
}

static int spi_sedi_transceive(const struct device *dev,
			       const struct spi_config *config,
			       const struct spi_buf_set *tx_bufs,
			       const struct spi_buf_set *rx_bufs)
{
	return transceive(dev, config, tx_bufs, rx_bufs, false, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int spi_sedi_transceive_async(const struct device *dev,
				     const struct spi_config *config,
				     const struct spi_buf_set *tx_bufs,
				     const struct spi_buf_set *rx_bufs,
				     spi_callback_t cb,
				     void *userdata)
{
	return transceive(dev, config, tx_bufs, rx_bufs, true, cb, userdata);
}
#endif /* CONFIG_SPI_ASYNC */

static int spi_sedi_release(const struct device *dev,
			    const struct spi_config *config)
{
	struct spi_sedi_data *spi = dev->data;

	if (!spi_context_configured(&spi->ctx, config)) {
		return -EINVAL;
	}

	spi_context_unlock_unconditionally(&spi->ctx);

	return 0;
}

extern void spi_isr(sedi_spi_t device);

void spi_sedi_callback(uint32_t event, void *param)
{
	const struct device *dev = (const struct device *)param;
	const struct spi_sedi_config *info = dev->config;
	struct spi_sedi_data *spi = dev->data;
	struct spi_context *ctx = &spi->ctx;
	int error;

	if (event == SEDI_SPI_EVENT_DATA_LOST) {
		error = -EIO;
	} else {
		error = 0;
	}

	if ((event == SEDI_SPI_EVENT_COMPLETE) ||
	    (event == SEDI_SPI_EVENT_DATA_LOST)) {
		spi_context_cs_control(&spi->ctx, true);
		spi_context_complete(&spi->ctx, dev, error);
	} else if (event == SEDI_SPI_EVENT_TX_FINISHED) {
		spi_context_update_tx(ctx, 1, ctx->tx_len);
		if (ctx->tx_len != 0) {
			sedi_spi_update_tx_buf(info->spi_device, ctx->tx_buf,
					       ctx->tx_len);
			if ((ctx->rx_len == 0) &&
			    (spi->rx_data_updated == false)) {
				/* Update rx length if always no rx */
				sedi_spi_update_rx_buf(info->spi_device, NULL,
						       spi->rx_dummy_len);
				spi->rx_data_updated = true;
			}
		} else if (spi->tx_data_updated == false) {
			sedi_spi_update_tx_buf(info->spi_device, NULL,
					       spi->tx_dummy_len);
			spi->tx_data_updated = true;
		}
	} else if (event == SEDI_SPI_EVENT_RX_FINISHED) {
		spi_context_update_rx(ctx, 1, ctx->rx_len);
		if (ctx->rx_len != 0) {
			sedi_spi_update_rx_buf(info->spi_device, ctx->rx_buf,
					       ctx->rx_len);
		}
	}
}

static const struct spi_driver_api sedi_spi_api = {
	.transceive = spi_sedi_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_sedi_transceive_async,
#endif  /* CONFIG_SPI_ASYNC */
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
	.release = spi_sedi_release,
};

static int spi_sedi_init(const struct device *dev)
{
	const struct spi_sedi_config *info = dev->config;
	struct spi_sedi_data *spi = dev->data;
	int ret;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	ret = sedi_spi_init(info->spi_device, spi_sedi_callback, (void *)dev,
			DEVICE_MMIO_GET(dev));
	if (ret != SEDI_DRIVER_OK) {
		return -ENODEV;
	}

	/* Init and connect IRQ */
	info->irq_config();

	spi_context_unlock_unconditionally(&spi->ctx);

	return 0;
}

#ifdef CONFIG_PM_DEVICE

static int spi_suspend_device(const struct device *dev)
{
	const struct spi_sedi_config *config = dev->config;

	if (pm_device_is_busy(dev)) {
		return -EBUSY;
	}

	int ret = sedi_spi_set_power(config->spi_device, SEDI_POWER_SUSPEND);

	if (ret != SEDI_DRIVER_OK) {
		return -EIO;
	}

	return 0;
}

static int spi_resume_device_from_suspend(const struct device *dev)
{
	const struct spi_sedi_config *config = dev->config;
	int ret;

	ret = sedi_spi_set_power(config->spi_device, SEDI_POWER_FULL);
	if (ret != SEDI_DRIVER_OK) {
		return -EIO;
	}

	pm_device_busy_clear(dev);

	return 0;
}

static int spi_sedi_device_ctrl(const struct device *dev,
				enum pm_device_action action)
{
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		ret = spi_suspend_device(dev);
		break;
	case PM_DEVICE_ACTION_RESUME:
		ret = spi_resume_device_from_suspend(dev);
		break;
	default:
		ret = -ENOTSUP;
	}

	return ret;
}

#endif /* CONFIG_PM_DEVICE */

#define SPI_SEDI_IRQ_FLAGS_SENSE0(n) 0
#define SPI_SEDI_IRQ_FLAGS_SENSE1(n) DT_INST_IRQ(n, sense)
#define SPI_SEDI_IRQ_FLAGS(n) \
	_CONCAT(SPI_SEDI_IRQ_FLAGS_SENSE, DT_INST_IRQ_HAS_CELL(n, sense))(n)

#define CREATE_SEDI_SPI_INSTANCE(num)					       \
	static void spi_##num##_irq_init(void)			               \
	{								       \
		IRQ_CONNECT(DT_INST_IRQN(num),				       \
			    DT_INST_IRQ(num, priority),			       \
			    spi_isr, num, SPI_SEDI_IRQ_FLAGS(num));	       \
		irq_enable(DT_INST_IRQN(num));				       \
	}								       \
	static struct spi_sedi_data spi_##num##_data = {		       \
		SPI_CONTEXT_INIT_LOCK(spi_##num##_data, ctx),	               \
		SPI_CONTEXT_INIT_SYNC(spi_##num##_data, ctx),	               \
	};								       \
	const static struct spi_sedi_config spi_##num##_config = {	       \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(num)),                        \
		.spi_device = num, .irq_config = spi_##num##_irq_init,         \
	};								       \
	PM_DEVICE_DEFINE(spi_##num, spi_sedi_device_ctrl);		       \
	DEVICE_DT_INST_DEFINE(num,					       \
			      spi_sedi_init,				       \
			      PM_DEVICE_GET(spi_##num),		               \
			      &spi_##num##_data,			       \
			      &spi_##num##_config,			       \
			      POST_KERNEL,				       \
			      CONFIG_SPI_INIT_PRIORITY,			       \
			      &sedi_spi_api);

DT_INST_FOREACH_STATUS_OKAY(CREATE_SEDI_SPI_INSTANCE)
