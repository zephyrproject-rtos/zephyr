/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_spim

#include "spi_nrfx_spim_common.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi/rtio.h>

LOG_MODULE_DECLARE(spi_nrfx_spim);

struct driver_data {
	struct spi_nrfx_common_data common;
};

struct driver_config {
	struct spi_nrfx_common_config common;
	struct spi_rtio *spi_rtio_ctx;
	const struct gpio_dt_spec *cs_gpios;
	uint8_t cs_gpios_size;
};

static void iodev_end_curr(const struct device *dev);
static void iodev_end_txn(const struct device *dev, int result);

static void iodev_await_callback(struct rtio_iodev_sqe *iodev_sqe, void *userdata)
{
	const struct device *dev = userdata;

	ARG_UNUSED(iodev_sqe);

	iodev_end_curr(dev);
}

static int cs_set(const struct device *dev, int value)
{
	const struct driver_config *dev_config = dev->config;
	struct spi_rtio *spi_rtio_ctx = dev_config->spi_rtio_ctx;
	struct rtio_sqe *sqe = &spi_rtio_ctx->txn_curr->sqe;
	struct spi_dt_spec *spi_spec = sqe->iodev->data;
	struct spi_config *spi_cfg = &spi_spec->config;

	int ret;

	if (spi_cfg->cs.cs_is_gpio == false) {
		return 0;
	}

	if (value == 0) {
		k_busy_wait(spi_cfg->cs.delay);
	}

	ret = gpio_pin_set_dt(&spi_cfg->cs.gpio, value);
	if (ret) {
		return ret;
	}

	if (value) {
		k_busy_wait(spi_cfg->cs.delay);
	}

	return 0;
}

static int cs_init(const struct device *dev)
{
	const struct driver_config *dev_config = dev->config;
	const struct gpio_dt_spec *cs_gpios = dev_config->cs_gpios;
	const uint8_t cs_gpios_size = dev_config->cs_gpios_size;
	int ret;

	for (size_t i = 0; i < cs_gpios_size; i++) {
		ret = gpio_pin_configure_dt(&cs_gpios[i], GPIO_OUTPUT_INACTIVE);
		if (ret) {
			return ret;
		}
	}

	return 0;
}

static void iodev_start_curr(const struct device *dev)
{
	const struct driver_config *dev_config = dev->config;
	struct spi_rtio *spi_rtio_ctx = dev_config->spi_rtio_ctx;
	struct rtio_sqe *sqe = &spi_rtio_ctx->txn_curr->sqe;
	struct spi_dt_spec *spi_spec = sqe->iodev->data;
	struct spi_config *spi_cfg = &spi_spec->config;
	const uint8_t *tx_buf;
	size_t tx_buf_len;
	uint8_t *rx_buf;
	size_t rx_buf_len;
	struct rtio_iodev_sqe *iodev_sqe;
	int ret;

	switch (sqe->op) {
	case RTIO_OP_TX:
		tx_buf = sqe->tx.buf;
		tx_buf_len = sqe->tx.buf_len;
		rx_buf = NULL;
		rx_buf_len = 0;
		break;

	case RTIO_OP_RX:
		tx_buf = NULL;
		tx_buf_len = 0;
		rx_buf = sqe->rx.buf;
		rx_buf_len = sqe->rx.buf_len;
		break;

	case RTIO_OP_TINY_TX:
		tx_buf = sqe->tiny_tx.buf;
		tx_buf_len = sqe->tiny_tx.buf_len;
		rx_buf = NULL;
		rx_buf_len = 0;
		break;

	case RTIO_OP_TXRX:
		tx_buf = sqe->txrx.tx_buf;
		tx_buf_len = sqe->txrx.buf_len;
		rx_buf = sqe->txrx.rx_buf;
		rx_buf_len = sqe->txrx.buf_len;
		break;

	default:
		tx_buf = NULL;
		tx_buf_len = 0;
		rx_buf = NULL;
		rx_buf_len = 0;
		break;
	}

	switch (sqe->op) {
	case RTIO_OP_TX:
	case RTIO_OP_RX:
	case RTIO_OP_TINY_TX:
	case RTIO_OP_TXRX:
		ret = spi_nrfx_spim_common_configure(dev, spi_cfg);
		if (ret) {
			break;
		}

		ret = cs_set(dev, 1);
		if (ret) {
			break;
		}

		ret = spi_nrfx_spim_common_transfer_start(dev,
							  tx_buf,
							  tx_buf_len,
							  rx_buf,
							  rx_buf_len);
		break;

	case RTIO_OP_AWAIT:
		iodev_sqe = CONTAINER_OF(sqe, struct rtio_iodev_sqe, sqe);
		rtio_iodev_sqe_await_signal(iodev_sqe, iodev_await_callback, (void *)dev);
		ret = 0;
		break;

	default:
		ret = -ENOTSUP;
		break;
	}

	if (ret) {
		iodev_end_txn(dev, ret);
	}
}

static void iodev_end_txn(const struct device *dev, int result)
{
	const struct driver_config *dev_config = dev->config;
	struct spi_rtio *spi_rtio_ctx = dev_config->spi_rtio_ctx;

	if (spi_rtio_complete(spi_rtio_ctx, result) == false) {
		pm_device_runtime_put(dev);
		return;
	}

	iodev_start_curr(dev);
}

static void iodev_end_curr(const struct device *dev)
{
	const struct driver_config *dev_config = dev->config;
	struct spi_rtio *spi_rtio_ctx = dev_config->spi_rtio_ctx;

	spi_rtio_ctx->txn_curr = rtio_txn_next(spi_rtio_ctx->txn_curr);
	if (spi_rtio_ctx->txn_curr == NULL) {
		iodev_end_txn(dev, 0);
		return;
	}

	iodev_start_curr(dev);
}

static void spim_evt_handler(const struct device *dev, nrfx_spim_event_t *evt)
{
	spi_nrfx_spim_common_transfer_end(dev, &evt->xfer_desc);
	cs_set(dev, 0);
	iodev_end_curr(dev);
}

static int driver_api_transceive(const struct device *dev,
				 const struct spi_config *config,
				 const struct spi_buf_set *tx_bufs,
				 const struct spi_buf_set *rx_bufs)
{
	const struct driver_config *dev_config = dev->config;

	return spi_rtio_transceive(dev_config->spi_rtio_ctx, config, tx_bufs, rx_bufs);
}

static int driver_api_transceive_async(const struct device *dev,
				       const struct spi_config *config,
				       const struct spi_buf_set *tx_bufs,
				       const struct spi_buf_set *rx_bufs,
				       spi_callback_t cb,
				       void *userdata)
{
	const struct driver_config *dev_config = dev->config;

	return spi_rtio_transceive_async(dev_config->spi_rtio_ctx,
					 config,
					 tx_bufs,
					 rx_bufs,
					 cb,
					 userdata);
}

static void driver_api_iodev_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct driver_config *dev_config = dev->config;

	if (spi_rtio_submit(dev_config->spi_rtio_ctx, iodev_sqe)) {
		pm_device_runtime_get(dev);
		cs_set(dev, 1);
		iodev_start_curr(dev);
	}
}

static DEVICE_API(spi, driver_api) = {
	.transceive = driver_api_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = driver_api_transceive_async,
#endif
	.iodev_submit = driver_api_iodev_submit,
	.release = spi_rtio_release,
};

static int driver_init(const struct device *dev)
{
	const struct driver_config *dev_config = dev->config;
	int ret;

	ret = spi_nrfx_spim_common_init(dev);
	if (ret) {
		return ret;
	}

	ret = cs_init(dev);
	if (ret) {
		return ret;
	}

	spi_rtio_init(dev_config->spi_rtio_ctx, dev);

	return pm_device_driver_init(dev, spi_nrfx_spim_common_pm_action);
}

#ifdef CONFIG_DEVICE_DEINIT_SUPPORT
static int driver_deinit(const struct device *dev)
{
	return spi_nrfx_spim_common_deinit(dev);
}
#endif

#define DRIVER_CS_GPIOS_GET(inst) \
	CONCAT(cs_gpios, inst)

#define DRIVER_CS_GPIO_INIT(idx, inst) GPIO_DT_SPEC_INST_GET_BY_IDX(inst, cs_gpios, idx)

#define DRIVER_CS_GPIOS_DEFINE(inst)								\
	static const struct gpio_dt_spec DRIVER_CS_GPIOS_GET(inst)[] = {			\
		LISTIFY(									\
			DT_INST_PROP_LEN_OR(inst, cs_gpios, 0),					\
			DRIVER_CS_GPIO_INIT,							\
			(,),									\
			inst									\
		)										\
	}

#define DRIVER_DEFINE(inst)									\
	static struct driver_data CONCAT(data, inst) = {					\
		.common = SPI_NRFX_COMMON_DATA_INIT(inst),					\
	};											\
												\
	SPI_NRFX_COMMON_DEFINE(inst, &CONCAT(data, inst));					\
												\
	SPI_RTIO_DEFINE(									\
		CONCAT(spi_rtio_ctx, inst),							\
		CONFIG_SPI_NRFX_SPIM_RTIO_SQE_POOL_SIZE,					\
		CONFIG_SPI_NRFX_SPIM_RTIO_CQE_POOL_SIZE						\
	);											\
												\
	DRIVER_CS_GPIOS_DEFINE(inst);								\
												\
	static const struct driver_config CONCAT(config, inst) = {				\
		.common = SPI_NRFX_COMMON_CONFIG_INIT(inst, spim_evt_handler),			\
		.spi_rtio_ctx = &CONCAT(spi_rtio_ctx, inst),					\
		.cs_gpios = DRIVER_CS_GPIOS_GET(inst),						\
		.cs_gpios_size = ARRAY_SIZE(DRIVER_CS_GPIOS_GET(inst)),				\
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
