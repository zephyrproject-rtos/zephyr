/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_spim

#include "spi_nrfx_spim_common.h"

LOG_MODULE_DECLARE(spi_nrfx_spim);

#include "spi_context.h"

struct driver_data {
	struct spi_nrfx_common_data common;
	struct spi_context ctx;
	struct k_sem wake_sem;
};

struct driver_config {
	struct spi_nrfx_common_config common;
};

static void transfer_end(const struct device *dev, int ret)
{
	struct driver_data *dev_data = dev->data;
	const struct spi_config *spi_cfg = dev_data->ctx.config;

	pm_device_runtime_put(dev);

	if (ret || (spi_cfg->operation & SPI_HOLD_ON_CS) == 0) {
		spi_nrfx_spim_common_cs_clear(dev, spi_cfg);
	}

	spi_context_complete(&dev_data->ctx, dev, ret);
}

static void transfer_start(const struct device *dev)
{
	struct driver_data *dev_data = dev->data;
	size_t chunk_len;
	const uint8_t *tx_buf;
	size_t tx_buf_len;
	uint8_t *rx_buf;
	size_t rx_buf_len;
	int ret;

	chunk_len = spi_context_max_continuous_chunk(&dev_data->ctx);
	if (chunk_len == 0) {
		transfer_end(dev, 0);
		return;
	}

	if (spi_context_tx_buf_on(&dev_data->ctx)) {
		tx_buf = dev_data->ctx.tx_buf;
		tx_buf_len = chunk_len;
	} else {
		tx_buf = NULL;
		tx_buf_len = 0;
	}

	if (spi_context_rx_buf_on(&dev_data->ctx)) {
		rx_buf = dev_data->ctx.rx_buf;
		rx_buf_len = chunk_len;
	} else {
		rx_buf = NULL;
		rx_buf_len = 0;
	}

	ret = spi_nrfx_spim_common_transfer_start(dev,
						  tx_buf,
						  tx_buf_len,
						  rx_buf,
						  rx_buf_len);
	if (ret) {
		transfer_end(dev, ret);
		return;
	}

	spi_context_update_tx(&dev_data->ctx, 1, chunk_len);
	spi_context_update_rx(&dev_data->ctx, 1, chunk_len);
}

static void spim_wake_handler(const struct device *dev)
{
	struct driver_data *dev_data = dev->data;

	k_sem_give(&dev_data->wake_sem);
}

static void spim_evt_handler(const struct device *dev, nrfx_spim_event_t *evt)
{
	spi_nrfx_spim_common_transfer_end(dev, &evt->xfer_desc);
	transfer_start(dev);
}

static int transceive(const struct device *dev,
		      const struct spi_config *spi_cfg,
		      const struct spi_buf_set *tx_bufs,
		      const struct spi_buf_set *rx_bufs,
		      bool asynchronous,
		      spi_callback_t cb,
		      void *userdata)
{
	struct driver_data *dev_data = dev->data;
	int ret;

	spi_context_lock(&dev_data->ctx, asynchronous, cb, userdata, spi_cfg);

	spi_context_buffers_setup(&dev_data->ctx, tx_bufs, rx_bufs, 1);
	if (!spi_context_tx_buf_on(&dev_data->ctx) &&
	    !spi_context_rx_buf_on(&dev_data->ctx)) {
		ret = -EINVAL;
		goto release_exit;
	}

	ret = pm_device_runtime_get(dev);
	if (ret) {
		goto release_exit;
	}

	ret = spi_nrfx_spim_common_configure(dev, spi_cfg);
	if (ret) {
		goto put_release_exit;
	}

	dev_data->ctx.config = spi_cfg;

	spi_nrfx_spim_common_wake_start(dev, spim_wake_handler);
	k_sem_take(&dev_data->wake_sem, K_FOREVER);

	spi_nrfx_spim_common_cs_set(dev, spi_cfg);
	transfer_start(dev);

	ret = spi_context_wait_for_completion(&dev_data->ctx);
	if (ret == 0) {
		if (spi_cfg->operation & SPI_LOCK_ON) {
			goto exit;
		} else {
			goto release_exit;
		}
	}

	if (ret == -ETIMEDOUT) {
		spi_nrfx_spim_common_transfer_stop(dev);
		spi_context_wait_for_completion(&dev_data->ctx);
	}

put_release_exit:
	pm_device_runtime_put(dev);

release_exit:
	spi_context_release(&dev_data->ctx, ret);

exit:
	return ret;
}

static int driver_api_transceive(const struct device *dev,
				 const struct spi_config *spi_cfg,
				 const struct spi_buf_set *tx_bufs,
				 const struct spi_buf_set *rx_bufs)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, false, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int driver_api_transceive_async(const struct device *dev,
				       const struct spi_config *spi_cfg,
				       const struct spi_buf_set *tx_bufs,
				       const struct spi_buf_set *rx_bufs,
				       spi_callback_t cb,
				       void *userdata)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, true, cb, userdata);
}
#endif

static int driver_api_release(const struct device *dev,
			      const struct spi_config *spi_cfg)
{
	struct driver_data *dev_data = dev->data;

	if (!spi_context_configured(&dev_data->ctx, spi_cfg)) {
		return -EPERM;
	}

	spi_context_unlock_unconditionally(&dev_data->ctx);
	return 0;
}

static DEVICE_API(spi, driver_api) = {
	.transceive = driver_api_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = driver_api_transceive_async,
#endif
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
	.release = driver_api_release,
};

static int driver_init(const struct device *dev)
{
	struct driver_data *dev_data = dev->data;
	int ret;

	ret = spi_nrfx_spim_common_init(dev);
	if (ret) {
		return ret;
	}

	k_sem_init(&dev_data->wake_sem, 0, 1);

	spi_context_unlock_unconditionally(&dev_data->ctx);

	return pm_device_driver_init(dev, spi_nrfx_spim_common_pm_action);
}

#ifdef CONFIG_DEVICE_DEINIT_SUPPORT
static int driver_deinit(const struct device *dev)
{
	return spi_nrfx_spim_common_deinit(dev);
}
#endif

#define DRIVER_DEFINE(inst)									\
	static struct driver_data CONCAT(data, inst) = {					\
		.common = SPI_NRFX_COMMON_DATA_INIT(inst),					\
		IF_ENABLED(									\
			CONFIG_MULTITHREADING,							\
			(SPI_CONTEXT_INIT_LOCK(CONCAT(data, inst), ctx),)			\
		)										\
		IF_ENABLED(									\
			CONFIG_MULTITHREADING,							\
			(SPI_CONTEXT_INIT_SYNC(CONCAT(data, inst), ctx),)			\
		)										\
	};											\
												\
	SPI_NRFX_COMMON_DEFINE(inst, &CONCAT(data, inst));					\
												\
	static const struct driver_config CONCAT(config, inst) = {				\
		.common = SPI_NRFX_COMMON_CONFIG_INIT(						\
			inst,									\
			spim_evt_handler							\
		),										\
	};											\
												\
	PM_DEVICE_DT_INST_DEFINE(inst, spi_nrfx_spim_common_pm_action, 1);			\
												\
	SPI_DEVICE_DT_INST_DEINIT_DEFINE(							\
		inst,										\
		driver_init,									\
		driver_deinit,									\
		PM_DEVICE_DT_INST_GET(inst),							\
		&CONCAT(data, inst),								\
		&CONCAT(config, inst),								\
		POST_KERNEL,									\
		CONFIG_SPI_INIT_PRIORITY,							\
		&driver_api									\
	)

DT_INST_FOREACH_STATUS_OKAY(DRIVER_DEFINE)
