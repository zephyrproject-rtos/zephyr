/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_saadc

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/adc/rtio.h>

#include "adc_nrfx_saadc_common.h"

LOG_MODULE_DECLARE(adc_nrfx_saadc);

struct driver_data {
	struct adc_nrfx_saadc_common_data common;
};

struct driver_config {
	struct adc_nrfx_saadc_common_config common;
	struct adc_rtio *adc_rtio_ctx;
};

static void iodev_end_txn(const struct device *dev, int ret);
static void iodev_end_curr(const struct device *dev);

static void iodev_await_callback(struct rtio_iodev_sqe *iodev_sqe, void *userdata)
{
	const struct device *dev = userdata;

	ARG_UNUSED(iodev_sqe);

	iodev_end_curr(dev);
}

static void iodev_start_curr(const struct device *dev)
{
	const struct driver_config *dev_config = dev->config;
	struct adc_rtio *adc_rtio_ctx = dev_config->adc_rtio_ctx;
	struct rtio_sqe *sqe = &adc_rtio_ctx->txn_curr->sqe;
	const struct adc_channel_cfg *channel_cfg;
	const struct adc_sequence *sequence;
	struct rtio_iodev_sqe *iodev_sqe;
	int ret;

	switch (sqe->op) {
	case RTIO_OP_AWAIT:
		iodev_sqe = CONTAINER_OF(sqe, struct rtio_iodev_sqe, sqe);
		rtio_iodev_sqe_await_signal(iodev_sqe, iodev_await_callback, (void *)dev);
		ret = 0;
		break;

	case RTIO_OP_ADC_CHANNEL_SETUP:
		channel_cfg = sqe->adc_channel_setup.channel_cfg;
		ret = adc_nrfx_saadc_common_channel_setup(dev, channel_cfg);
		if (ret == 0) {
			iodev_end_curr(dev);
			return;
		}
		break;

	case RTIO_OP_ADC_READ:
		sequence = sqe->adc_read.sequence;
		ret = adc_nrfx_saadc_common_start_read(dev, sequence);
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
	struct adc_rtio *adc_rtio_ctx = dev_config->adc_rtio_ctx;

	if (adc_rtio_complete(adc_rtio_ctx, result) == false) {
		return;
	}

	iodev_start_curr(dev);
}

static void iodev_end_curr(const struct device *dev)
{
	const struct driver_config *dev_config = dev->config;
	struct adc_rtio *adc_rtio_ctx = dev_config->adc_rtio_ctx;

	adc_rtio_ctx->txn_curr = rtio_txn_next(adc_rtio_ctx->txn_curr);
	if (adc_rtio_ctx->txn_curr == NULL) {
		iodev_end_txn(dev, 0);
		return;
	}

	iodev_start_curr(dev);
}

static void read_complete_handler(const struct device *dev, int ret)
{
	if (ret) {
		iodev_end_txn(dev, ret);
	} else {
		iodev_end_curr(dev);
	}
}

#if CONFIG_ADC_RTIO_WRAPPER
static int driver_api_channel_setup(const struct device *dev,
				    const struct adc_channel_cfg *channel_cfg)
{
	const struct driver_config *dev_config = dev->config;
	struct adc_rtio *adc_rtio_ctx = dev_config->adc_rtio_ctx;

	return adc_rtio_channel_setup(adc_rtio_ctx, channel_cfg);
}

static int driver_api_read(const struct device *dev,
			   const struct adc_sequence *sequence)
{
	const struct driver_config *dev_config = dev->config;
	struct adc_rtio *adc_rtio_ctx = dev_config->adc_rtio_ctx;

	return adc_rtio_read(adc_rtio_ctx, sequence);
}

#if CONFIG_ADC_ASYNC
static int driver_api_read_async(const struct device *dev,
				 const struct adc_sequence *sequence,
				 struct k_poll_signal *async)
{
	const struct driver_config *dev_config = dev->config;
	struct adc_rtio *adc_rtio_ctx = dev_config->adc_rtio_ctx;

	return adc_rtio_read_async(adc_rtio_ctx, sequence, async);
}
#endif /* CONFIG_ADC_ASYNC */
#endif /* CONFIG_ADC_RTIO_WRAPPER */

static void driver_api_submit(const struct device *dev,
			      struct rtio_iodev_sqe *iodev_sqe)
{
	const struct driver_config *dev_config = dev->config;
	struct adc_rtio *adc_rtio_ctx = dev_config->adc_rtio_ctx;

	if (adc_rtio_submit(adc_rtio_ctx, iodev_sqe)) {
		iodev_start_curr(dev);
	}
}

static DEVICE_API(adc, driver_api) = {
#ifdef CONFIG_ADC_RTIO_WRAPPER
	.channel_setup = driver_api_channel_setup,
	.read = driver_api_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = driver_api_read_async,
#endif /* CONFIG_ADC_ASYNC */
#endif /* CONFIG_ADC_RTIO_WRAPPER */
	.ref_internal  = NRFX_SAADC_REF_INTERNAL_VALUE,
	.submit = driver_api_submit,
};

static int driver_init(const struct device *dev)
{
	const struct driver_config *dev_config = dev->config;
	struct adc_rtio *adc_rtio_ctx = dev_config->adc_rtio_ctx;
	int ret;

	ret = adc_nrfx_saadc_common_init(dev);
	if (ret) {
		return ret;
	}

	adc_rtio_init(adc_rtio_ctx, dev);

	return 0;
}

#ifdef CONFIG_DEVICE_DEINIT_SUPPORT
static int driver_deinit(const struct device *dev)
{
	return adc_nrfx_saadc_common_deinit(dev);
}
#endif /* CONFIG_DEVICE_DEINIT_SUPPORT */

#define DRIVER_DEFINE(inst)									\
	ADC_NRFX_SAADC_COMMON_DEFINE(inst);							\
												\
	static struct driver_data CONCAT(data, inst) = {					\
		.common = ADC_NRFX_SAADC_COMMON_DATA_INIT(inst),				\
	};											\
												\
	ADC_RTIO_DEFINE(CONCAT(adc_rtio_ctx, inst));						\
												\
	static const struct driver_config CONCAT(config, inst) = {				\
		.common = ADC_NRFX_SAADC_COMMON_CONFIG_INIT(inst, read_complete_handler),	\
		.adc_rtio_ctx = &CONCAT(adc_rtio_ctx, inst),					\
	};											\
												\
	DEVICE_DT_INST_DEINIT_DEFINE(								\
		inst,										\
		driver_init,									\
		driver_deinit,									\
		NULL,										\
		&CONCAT(data, inst),								\
		&CONCAT(config, inst),								\
		POST_KERNEL,									\
		CONFIG_ADC_INIT_PRIORITY,							\
		&driver_api									\
	)

DT_INST_FOREACH_STATUS_OKAY(DRIVER_DEFINE)
