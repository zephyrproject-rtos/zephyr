/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for Realtek Ameba TRNG
 */
#define DT_DRV_COMPAT realtek_ameba_trng

/* Include <soc.h> before <ameba_soc.h> to avoid redefining unlikely() macro */
#include <soc.h>
#include <ameba_soc.h>

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/entropy.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ameba_trng, CONFIG_ENTROPY_LOG_LEVEL);

struct entropy_ameba_config {
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
};

static int entropy_ameba_get_entropy(const struct device *dev, uint8_t *buf, uint16_t len)
{
	ARG_UNUSED(dev);

	if (NULL == buf || 0 == len) {
		LOG_ERR("Param error 0x%08x %d", (uint32_t)buf, len);
		return -EIO;
	}

	/* Depend on the soc, maybe need some time to get the result */
	TRNG_get_random_bytes(buf, len);

	return 0;
}

static int entropy_ameba_get_entropy_isr(const struct device *dev, uint8_t *buf, uint16_t len,
					 uint32_t flags)
{
	ARG_UNUSED(flags);

	/* the TRNG may cost some time to generator the data, the speed is 10Mbps
	 *  we assume that it is fast enough for ISR use
	 */
	int ret = entropy_ameba_get_entropy(dev, buf, len);

	if (ret == 0) {
		return len;
	}

	return ret;
}

static int entropy_ameba_init(const struct device *dev)
{
	const struct entropy_ameba_config *config = dev->config;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("Clock control device not ready");
		return -ENODEV;
	}

	if (clock_control_on(config->clock_dev, config->clock_subsys)) {
		LOG_ERR("Could not enable TRNG clock");
		return -EIO;
	}

	return 0;
}

static DEVICE_API(entropy, entropy_ameba_api_funcs) = {
	.get_entropy = entropy_ameba_get_entropy,
	.get_entropy_isr = entropy_ameba_get_entropy_isr,
};

static const struct entropy_ameba_config entropy_config = {
	.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
	.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(0, idx),
};

DEVICE_DT_INST_DEFINE(0, entropy_ameba_init, NULL, NULL, &entropy_config, PRE_KERNEL_1,
		      CONFIG_ENTROPY_INIT_PRIORITY, &entropy_ameba_api_funcs);
