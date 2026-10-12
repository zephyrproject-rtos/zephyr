/*
 * Copyright (c) 2026 Mahendra Sondagar <mahendra@dotcom.co.in>
 * Copyright (c) 2026 Parin Baudhanwala <parin.baudhanwala@dnkmail.in>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief STHS34PF80 infrared presence and motion sensor driver.
 *
 * Reports ambient and object temperature, the presence, motion and
 * temperature-shock algorithm outputs and the matching detection flags
 * through the sensor Read & Decode API.
 */

#define DT_DRV_COMPAT st_sths34pf80

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/rtio/rtio.h>
#include "sths34pf80.h"
#include "sths34pf80_decoder.h"
#include "sths34pf80_rtio.h"

LOG_MODULE_REGISTER(STHS34PF80, CONFIG_SENSOR_LOG_LEVEL);

#define STHS34PF80_BOOT_DELAY_MS 5

static int sths34pf80_init(const struct device *dev)
{
	const struct sths34pf80_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	uint8_t id;
	int ret;

	if (!cfg->bus_ready(cfg)) {
		LOG_ERR("Bus not ready");
		return -ENODEV;
	}

	ret = sths34pf80_device_id_get(ctx, &id);
	if (ret != 0) {
		LOG_ERR("Failed to read WHO_AM_I (%d)", ret);
		return -EIO;
	}

	if (id != STHS34PF80_ID) {
		LOG_ERR("Invalid WHO_AM_I: 0x%02x, expected 0x%02x", id, STHS34PF80_ID);
		return -ENODEV;
	}

	/* Reload trimming parameters from OTP */
	ret = sths34pf80_boot_set(ctx, PROPERTY_ENABLE);
	if (ret != 0) {
		LOG_ERR("Failed to reboot OTP (%d)", ret);
		return -EIO;
	}

	k_msleep(STHS34PF80_BOOT_DELAY_MS);

	/* Averaging is configured before the ODR, which depends on it */
	ret = sths34pf80_avg_tobject_num_set(ctx, STHS34PF80_AVG_TMOS_32);
	ret |= sths34pf80_avg_tambient_num_set(ctx, STHS34PF80_AVG_T_8);
	ret |= sths34pf80_block_data_update_set(ctx, PROPERTY_ENABLE);
	if (ret != 0) {
		LOG_ERR("Failed to configure averaging/BDU (%d)", ret);
		return -EIO;
	}

	/*
	 * Restart the embedded algorithms from a clean state. The datasheet
	 * requires the reset to be done in power-down mode, so it must happen
	 * before the ODR is programmed (the device is in power-down after boot).
	 */
	ret = sths34pf80_algo_reset(ctx);
	if (ret != 0) {
		LOG_ERR("Failed to reset algorithms (%d)", ret);
		return -EIO;
	}

	ret = sths34pf80_odr_set(ctx, (sths34pf80_odr_t)cfg->odr);
	if (ret != 0) {
		LOG_ERR("Failed to set ODR %u (%d)", cfg->odr, ret);
		return -EIO;
	}

	LOG_DBG("STHS34PF80 initialized");

	return 0;
}

static int sths34pf80_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(chan);

	return -ENOTSUP;
}

static int sths34pf80_channel_get(const struct device *dev, enum sensor_channel chan,
				  struct sensor_value *val)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(chan);
	ARG_UNUSED(val);

	return -ENOTSUP;
}

static DEVICE_API(sensor, sths34pf80_api) = {
	.sample_fetch = sths34pf80_sample_fetch,
	.channel_get = sths34pf80_channel_get,
	.get_decoder = sths34pf80_get_decoder,
	.submit = sths34pf80_submit,
};

#define STHS34PF80_SPI_OPERATION                                                                   \
	(SPI_WORD_SET(8) | SPI_OP_MODE_CONTROLLER | SPI_MODE_CPOL | SPI_MODE_CPHA)

static bool __maybe_unused sths34pf80_i2c_ready(const struct sths34pf80_config *cfg)
{
	return i2c_is_ready_dt(&cfg->stmemsc_cfg.i2c);
}

static bool __maybe_unused sths34pf80_spi_ready(const struct sths34pf80_config *cfg)
{
	return spi_is_ready_dt(&cfg->stmemsc_cfg.spi);
}

#define STHS34PF80_DEFINE_I2C(inst) I2C_DT_IODEV_DEFINE(sths34pf80_iodev_##inst, DT_DRV_INST(inst))
#define STHS34PF80_DEFINE_SPI(inst)                                                                \
	SPI_DT_IODEV_DEFINE(sths34pf80_iodev_##inst, DT_DRV_INST(inst), STHS34PF80_SPI_OPERATION)

#define STHS34PF80_CFG_I2C(inst)                                                                   \
	STMEMSC_CTX_I2C(&sths34pf80_config_##inst.stmemsc_cfg),                                    \
		.stmemsc_cfg = {.i2c = I2C_DT_SPEC_INST_GET(inst)},                                \
		.bus_ready = sths34pf80_i2c_ready, .read_mask = 0x00, .is_spi = false
#define STHS34PF80_CFG_SPI(inst)                                                                   \
	STMEMSC_CTX_SPI(&sths34pf80_config_##inst.stmemsc_cfg),                                    \
		.stmemsc_cfg = {.spi = SPI_DT_SPEC_INST_GET(inst, STHS34PF80_SPI_OPERATION)},      \
		.bus_ready = sths34pf80_spi_ready, .read_mask = 0x80, .is_spi = true

#define STHS34PF80_DEFINE(inst)                                                                    \
	COND_CODE_1(DT_INST_ON_BUS(inst, spi),                                         \
		    (STHS34PF80_DEFINE_SPI(inst);), (STHS34PF80_DEFINE_I2C(inst);))           \
	RTIO_DEFINE(sths34pf80_rtio_ctx_##inst, 16, 16);                                           \
	static struct sths34pf80_data sths34pf80_data_##inst = {                                   \
		.rtio_ctx = &sths34pf80_rtio_ctx_##inst,                                           \
		.iodev = &sths34pf80_iodev_##inst,                                                 \
	};                                                                                         \
	static const struct sths34pf80_config sths34pf80_config_##inst = {                         \
		COND_CODE_1(DT_INST_ON_BUS(inst, spi),                                 \
			    (STHS34PF80_CFG_SPI(inst)), (STHS34PF80_CFG_I2C(inst))), \
					    .odr = DT_INST_PROP(inst, odr),                        \
		};                                                                                 \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, sths34pf80_init, NULL, &sths34pf80_data_##inst,         \
				     &sths34pf80_config_##inst, POST_KERNEL,                       \
				     CONFIG_SENSOR_INIT_PRIORITY, &sths34pf80_api);

DT_INST_FOREACH_STATUS_OKAY(STHS34PF80_DEFINE)
