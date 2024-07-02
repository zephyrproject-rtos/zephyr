/*
 * Copyright (c) 2024 Brill Power Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <errno.h>
#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/init.h>
#include <zephyr/drivers/crc.h>
#include "stm32_ll_crc.h"

LOG_MODULE_REGISTER(crc_stm32);

#define DT_DRV_COMPAT st_stm32_crc

struct crc_cfg {
	CRC_TypeDef *base;
	struct stm32_pclken pclken;
};

static int crc_stm32_setup_crc_8_ccitt(const struct device *dev)
{
	const struct crc_cfg *cfg = dev->config;

	LL_CRC_SetInputDataReverseMode(cfg->base, LL_CRC_INDATA_REVERSE_NONE);
	LL_CRC_SetOutputDataReverseMode(cfg->base, LL_CRC_OUTDATA_REVERSE_NONE);
	LL_CRC_ResetCRCCalculationUnit(cfg->base);
	LL_CRC_SetPolynomialSize(cfg->base, LL_CRC_POLYLENGTH_8B);
	LL_CRC_SetPolynomialCoef(cfg->base, 0x07);

	return 0;
}

static uint8_t crc_stm32_crc_8_ccitt(const struct device *dev, uint8_t initial_value,
				     const void *buf, size_t len)
{
	const struct crc_cfg *cfg = dev->config;
	const uint8_t *num_data = buf;

	LL_CRC_SetInitialData(cfg->base, initial_value);

	register uint32_t index = 0;

	for (index = 0; index < len; index++) {
		LL_CRC_FeedData8(cfg->base, num_data[index]);
	}

	return LL_CRC_ReadData8(cfg->base);
}

static inline uint8_t crc_stm32_verify_crc_8_ccitt(const struct device *dev, uint8_t expected,
						   uint8_t initial_value, const void *buf,
						   size_t len)
{
	return (expected == crc_stm32_crc_8_ccitt(dev, initial_value, buf, len)) ? 1 : 0;
}

static int crc_stm32_setup_crc_32_ieee(const struct device *dev)
{
	const struct crc_cfg *cfg = dev->config;

	LL_CRC_SetInputDataReverseMode(cfg->base, LL_CRC_INDATA_REVERSE_WORD);
	LL_CRC_SetOutputDataReverseMode(cfg->base, LL_CRC_OUTDATA_REVERSE_BIT);
	LL_CRC_ResetCRCCalculationUnit(cfg->base);
	LL_CRC_SetPolynomialSize(cfg->base, LL_CRC_POLYLENGTH_32B);
	LL_CRC_SetPolynomialCoef(cfg->base, LL_CRC_DEFAULT_CRC32_POLY);
	LL_CRC_SetInitialData(cfg->base, LL_CRC_DEFAULT_CRC_INITVALUE);

	return 0;
}

static uint32_t crc_stm32_crc_32_ieee(const struct device *dev, const void *buf, size_t len)
{
	const struct crc_cfg *cfg = dev->config;
	const uint8_t *num_data = buf;

	register uint32_t data = 0;
	register uint32_t index = 0;

	/* Compute the CRC of Data Buffer array */
	for (index = 0; index < (len / 4); index++) {
		data = (uint32_t)((num_data[4 * index + 3] << 24) | (num_data[4 * index + 2] << 16)
				  (num_data[4 * index + 1] << 8) | num_data[4 * index]);
		LL_CRC_FeedData32(cfg->base, data);
	}

	/* Last bytes specific handling */
	if ((len & 3) != 0) {
		if ((len & 3) == 1) {
			LL_CRC_FeedData8(cfg->base, num_data[4 * index]);
		}
		if ((len & 3) == 2) {
			LL_CRC_FeedData16(cfg->base,
				(uint16_t)((num_data[4 * index + 1] << 8) | num_data[4 * index]));
		}
		if ((len & 3) == 3) {
			LL_CRC_FeedData16(cfg->base,
				(uint16_t)((num_data[4 * index + 1] << 8) | num_data[4 * index]));
			LL_CRC_FeedData8(cfg->base, num_data[4 * index + 2]);
		}
	}

	/* Return computed inverted CRC value */
	return (~LL_CRC_ReadData32(cfg->base));
}

static uint32_t crc_stm32_verify_crc_32_ieee(const struct device *dev, uint32_t expected,
					     const void *buf, size_t len)
{
	return (expected == crc_stm32_crc_32_ieee(dev, buf, len)) ? 1 : 0;
}

/**
 * @brief Called by Zephyr when instantiating device during boot
 * @param dev Pointer to the device structure for the driver instance.
 * @retval 0       On success
 * @retval -ENODEV Clock control device is not ready
 * @retval -EIO    Fail to turn on appropriate clock for CRC instance
 */
static int crc_stm32_init(const struct device *dev)
{
	const struct crc_cfg *cfg = dev->config;
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	if (!device_is_ready(clk)) {
		LOG_ERR("CRC: Clock control device not ready");
		return -ENODEV;
	}

	if (clock_control_on(clk, (clock_control_subsys_t *)&cfg->pclken) != 0) {
		LOG_ERR("CRC: Clock control device could not initialise");
		return -EIO;
	}

#ifdef CONFIG_CRC_THREADSAFE
	struct crc_data *data = dev->data;

	if (DT_PROP_OR(DT_NODELABEL(crc1), crc_8_timeout, 0) == 0) {
		data->crc_8_timeout = K_NO_WAIT;
	} else if (DT_PROP_OR(DT_NODELABEL(crc1), crc_8_timeout, 0) > 10000) {
		data->crc_8_timeout = K_FOREVER;
	} else {
		data->crc_8_timeout = K_MSEC(DT_PROP(DT_NODELABEL(crc1), crc_8_timeout));
	}

	if (DT_PROP_OR(DT_NODELABEL(crc1), crc_32_timeout, 0) == 0) {
		data->crc_32_timeout = K_NO_WAIT;
	} else if (DT_PROP_OR(DT_NODELABEL(crc1), crc_32_timeout, 0) > 10000) {
		data->crc_32_timeout = K_FOREVER;
	} else {
		data->crc_32_timeout = K_MSEC(DT_PROP(DT_NODELABEL(crc1), crc_32_timeout));
	}

	k_mutex_init(&data->mutex);

#endif /* CONFIG_CRC_THREADSAFE* */

	return 0;
}

static const struct crc_driver_api crc_stm32_driver_api = {
	.setup_crc_8_ccitt = crc_stm32_setup_crc_8_ccitt,
	.crc_8_ccitt = crc_stm32_crc_8_ccitt,
	.verify_crc_8_ccitt = crc_stm32_verify_crc_8_ccitt,
	.setup_crc_32_ieee = crc_stm32_setup_crc_32_ieee,
	.crc_32_ieee = crc_stm32_crc_32_ieee,
	.verify_crc_32_ieee = crc_stm32_verify_crc_32_ieee,
};

#define STM32_CRC_INIT(index) \
\
	static const struct crc_cfg crc_cfg_##index = {		\
		.base = (CRC_TypeDef *)DT_INST_REG_ADDR(index),			\
		.pclken = {							\
			.enr = DT_INST_CLOCKS_CELL(index, bits),		\
			.bus = DT_INST_CLOCKS_CELL(index, bus),			\
		},                    \
	}; \
\
	static struct crc_data crc_stm32_data_##index = {}; \
\
	DEVICE_DT_INST_DEFINE(index, &crc_stm32_init, NULL, &crc_stm32_data_##index,\
			      &crc_cfg_##index, POST_KERNEL, CONFIG_CRC_INIT_PRIORITY, \
			      &crc_stm32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(STM32_CRC_INIT)
