/*
 * Copyright (c) 2023 PHOENIX CONTACT Electronics GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mdio_adin2111, CONFIG_MDIO_LOG_LEVEL);

#define DT_DRV_COMPAT adi_adin2111_mdio

#include <stdint.h>
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/drivers/ethernet/eth_adin2111.h>

/* MDIO ready check retry delay */
#define ADIN2111_MDIO_READY_AWAIT_DELAY_POLL_US		5U
/* Number of retries for MDIO ready check */
#define ADIN2111_MDIO_READY_AWAIT_RETRY_COUNT		10U

/* MDIO Access Register 1 */
#define ADIN2111_MDIOACC0				0x20U
/* MDIO Access Register 2 */
#define ADIN2111_MDIOACC1				0x21U

/* MDIO MDIOACC Transaction Done */
#define ADIN211_MDIOACC_MDIO_TRDONE			BIT(31)

struct mdio_adin2111_config {
	const struct device *adin;
};

static int mdio_adin2111_wait_ready(const struct device *dev, uint16_t reg,
				    uint32_t *out)
{
	const struct mdio_adin2111_config *const cfg = dev->config;
	uint32_t count;
	int ret;

	for (count = 0U; count < ADIN2111_MDIO_READY_AWAIT_RETRY_COUNT; ++count) {
		ret = eth_adin2111_reg_read(cfg->adin, reg, out);
		if (ret >= 0) {
			if ((*out) & ADIN211_MDIOACC_MDIO_TRDONE) {
				break;
			}
			ret = -ETIMEDOUT;
		}
		k_sleep(K_USEC(ADIN2111_MDIO_READY_AWAIT_DELAY_POLL_US));
	}

	return ret;
}


static int mdio_adin2111_read_c45(const struct device *dev, uint8_t prtad,
				  uint8_t devad, uint16_t regad,
				  uint16_t *data)
{
	const struct mdio_adin2111_config *const cfg = dev->config;
	uint32_t rdy;
	uint32_t cmd;
	int ret;

	/* address op */
	cmd = (prtad & 0x1FU) << 21;
	cmd |= (devad & 0x1FU) << 16;
	cmd |= regad;

	ret = eth_adin2111_reg_write(cfg->adin, ADIN2111_MDIOACC0, cmd);
	if (ret < 0) {
		return ret;
	}

	/* read op */
	cmd = (cmd & ~UINT16_MAX) | (0x3U << 26);

	ret = eth_adin2111_reg_write(cfg->adin, ADIN2111_MDIOACC1, cmd);
	if (ret < 0) {
		return ret;
	}

	ret = mdio_adin2111_wait_ready(dev, ADIN2111_MDIOACC1, &rdy);
	if (ret < 0) {
		return ret;
	}

	/* read out */
	ret = eth_adin2111_reg_read(cfg->adin, ADIN2111_MDIOACC1, &cmd);

	*data = cmd & UINT16_MAX;

	return ret;
}

static int mdio_adin2111_write_c45(const struct device *dev, uint8_t prtad,
				   uint8_t devad, uint16_t regad,
				   uint16_t data)
{
	const struct mdio_adin2111_config *const cfg = dev->config;

	uint32_t rdy;
	uint32_t cmd;
	int ret;

	/* address op */
	cmd = (prtad & 0x1FU) << 21;
	cmd |= (devad & 0x1FU) << 16;
	cmd |= regad;

	ret = eth_adin2111_reg_write(cfg->adin, ADIN2111_MDIOACC0, cmd);
	if (ret < 0) {
		return ret;
	}

	/* write op */
	cmd |= BIT(26);
	cmd = (cmd & ~UINT16_MAX) | data;

	ret = eth_adin2111_reg_write(cfg->adin, ADIN2111_MDIOACC1, cmd);
	if (ret < 0) {
		return ret;
	}

	ret = mdio_adin2111_wait_ready(dev, ADIN2111_MDIOACC1, &rdy);

	return ret;
}

static int mdio_adin2111_read(const struct device *dev, uint8_t prtad,
			      uint8_t regad, uint16_t *data)
{
	const struct mdio_adin2111_config *const cfg = dev->config;
	uint32_t read;
	uint32_t cmd;
	int ret;

	cmd = BIT(28);
	cmd |= 0x3U << 26;
	cmd |= (prtad & 0x1FU) << 21;
	cmd |= (regad & 0x1FU) << 16;

	ret = eth_adin2111_reg_write(cfg->adin, ADIN2111_MDIOACC0, cmd);
	if (ret >= 0) {
		ret = mdio_adin2111_wait_ready(dev, ADIN2111_MDIOACC0, &read);
		*data = read & UINT16_MAX;
	}

	return ret;
}

static int mdio_adin2111_write(const struct device *dev, uint8_t prtad,
			       uint8_t regad, uint16_t data)
{
	const struct mdio_adin2111_config *const cfg = dev->config;
	uint32_t cmd;
	uint32_t rdy;
	int ret;

	cmd = BIT(28);
	cmd |= BIT(26);
	cmd |= (prtad & 0x1FU) << 21;
	cmd |= (regad & 0x1FU) << 16;
	cmd |= data;

	ret = eth_adin2111_reg_write(cfg->adin, ADIN2111_MDIOACC0, cmd);
	if (ret >= 0) {
		ret = mdio_adin2111_wait_ready(dev, ADIN2111_MDIOACC0, &rdy);
	}

	return ret;
}

static void mdio_adin2111_bus_enable(const struct device *dev)
{
	const struct mdio_adin2111_config *const cfg = dev->config;

	eth_adin2111_lock(cfg->adin, K_FOREVER);
}

static void mdio_adin2111_bus_disable(const struct device *dev)
{
	const struct mdio_adin2111_config *const cfg = dev->config;

	eth_adin2111_unlock(cfg->adin);
}

static const struct mdio_driver_api mdio_adin2111_api = {
	.read = mdio_adin2111_read,
	.write = mdio_adin2111_write,
	.read_c45 = mdio_adin2111_read_c45,
	.write_c45 = mdio_adin2111_write_c45,
	.bus_enable = mdio_adin2111_bus_enable,
	.bus_disable = mdio_adin2111_bus_disable
};

#define ADIN2111_MDIO_INIT(n)							\
	static const struct mdio_adin2111_config mdio_adin2111_config_##n = {	\
		.adin = DEVICE_DT_GET(DT_INST_BUS(n)),				\
	};									\
	DEVICE_DT_INST_DEFINE(n, NULL, NULL,					\
			      NULL, &mdio_adin2111_config_##n,			\
			      POST_KERNEL, CONFIG_MDIO_INIT_PRIORITY,		\
			      &mdio_adin2111_api);

DT_INST_FOREACH_STATUS_OKAY(ADIN2111_MDIO_INIT)
