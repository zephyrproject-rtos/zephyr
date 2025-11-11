/*
 * Copyright (c) 2024 sensry.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sensry_sy1xx_mdio

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sy1xx_mdio, CONFIG_MDIO_LOG_LEVEL);

#include <zephyr/drivers/mdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/pinctrl.h>
#include <udma.h>

struct sy1xx_mdio_dev_config {
	const struct pinctrl_dev_config *pcfg;
	uint32_t base_addr;
	uint32_t mdc_freq;
};

struct sy1xx_mdio_dev_data {
	struct k_sem sem;
};

/* mdio register offsets */
#define SY1XX_MDIO_CFG_REG        0x0000
#define SY1XX_MDIO_CTRL_REG       0x0004
#define SY1XX_MDIO_READ_DATA_REG  0x0008
#define SY1XX_MDIO_WRITE_DATA_REG 0x000c
#define SY1XX_MDIO_IRQ_REG        0x0010

/* mdio config register bit offsets */
#define SY1XX_MDIO_CFG_DIV_OFFS (0)
#define SY1XX_MDIO_CFG_EN_OFFS  (8)

/* mdio ctrl register bit offsets */
#define SY1XX_MDIO_CTRL_READY_OFFS    (0)
#define SY1XX_MDIO_CTRL_INIT_OFFS     (8)
#define SY1XX_MDIO_CTRL_REG_ADDR_OFFS (16)
#define SY1XX_MDIO_CTRL_PHY_ADDR_OFFS (24)
#define SY1XX_MDIO_CTRL_OP_OFFS       (30)

/* mdio ctrl operations */
#define SY1XX_MDIO_CTRL_OP_WRITE (0x1)
#define SY1XX_MDIO_CTRL_OP_READ  (0x2)

#define SY1XX_MDIO_READ_WRITE_WAIT_TIME_US (15)
#define SY1XX_MDIO_READ_WRITE_RETRY_COUNT  (5)

static int sy1xx_mdio_wait_for_ready(const struct device *dev);

static int sy1xx_mdio_initialize(const struct device *dev)
{

	struct sy1xx_mdio_dev_config *cfg = (struct sy1xx_mdio_dev_config *)dev->config;
	int ret;
	uint32_t divider;
	uint32_t reg;

	/* zero mdio controller regs */
	sys_write32(0x0, cfg->base_addr + SY1XX_MDIO_CFG_REG);
	sys_write32(0x0, cfg->base_addr + SY1XX_MDIO_CTRL_REG);
	sys_write32(0x0, cfg->base_addr + SY1XX_MDIO_READ_DATA_REG);
	sys_write32(0x0, cfg->base_addr + SY1XX_MDIO_WRITE_DATA_REG);
	sys_write32(0x0, cfg->base_addr + SY1XX_MDIO_IRQ_REG);

	/* prepare mdio clock and enable mdio controller */
	divider = (((sy1xx_soc_get_peripheral_clock() / cfg->mdc_freq) / 2) - 1) & 0xff;
	reg = (divider << SY1XX_MDIO_CFG_DIV_OFFS) | BIT(SY1XX_MDIO_CFG_EN_OFFS);

	LOG_DBG("config, div: %d, freq: %d", divider, cfg->mdc_freq);

	sys_write32(reg, cfg->base_addr + SY1XX_MDIO_CFG_REG);

	/* PAD config */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("failed to configure pins");
		return ret;
	}

	ret = sy1xx_mdio_wait_for_ready(dev);
	if (ret < 0) {
		LOG_ERR("not ready");
		return ret;
	}

	return 0;
}

static uint32_t sy1xx_mdio_is_ready(const struct device *dev)
{
	struct sy1xx_mdio_dev_config *cfg = (struct sy1xx_mdio_dev_config *)dev->config;
	uint32_t status = sys_read32(cfg->base_addr + SY1XX_MDIO_CTRL_REG);

	return (status & BIT(SY1XX_MDIO_CTRL_READY_OFFS));
}

static int sy1xx_mdio_wait_for_ready(const struct device *dev)
{
	uint32_t retries_left = SY1XX_MDIO_READ_WRITE_RETRY_COUNT;

	while (!sy1xx_mdio_is_ready(dev)) {
		k_sleep(K_USEC(SY1XX_MDIO_READ_WRITE_WAIT_TIME_US));
		retries_left--;
		if (!retries_left) {
			return -EINVAL;
		}
	}
	return 0;
}

static int sy1xx_mdio_read(const struct device *dev, uint8_t prtad, uint8_t regad, uint16_t *data)
{
	struct sy1xx_mdio_dev_config *cfg = (struct sy1xx_mdio_dev_config *)dev->config;
	int ret;
	uint32_t v;

	prtad &= 0x1f;
	regad &= 0x1f;

	v = (SY1XX_MDIO_CTRL_OP_READ << SY1XX_MDIO_CTRL_OP_OFFS) |
	    (prtad << SY1XX_MDIO_CTRL_PHY_ADDR_OFFS) | (regad << SY1XX_MDIO_CTRL_REG_ADDR_OFFS) |
	    BIT(SY1XX_MDIO_CTRL_INIT_OFFS);

	/* start the reading procedure */
	sys_write32(v, cfg->base_addr + SY1XX_MDIO_CTRL_REG);

	/* wait for the reading operation to finish */
	ret = sy1xx_mdio_wait_for_ready(dev);
	if (ret < 0) {
		*data = sys_read32(cfg->base_addr + SY1XX_MDIO_READ_DATA_REG);

		LOG_WRN("timeout while reading from phy: %d, reg: %d, val: %d", prtad, regad,
			*data);
		return ret;
	}

	/* get the data from the read result register */
	*data = sys_read32(cfg->base_addr + SY1XX_MDIO_READ_DATA_REG);

	return 0;
}

static int sy1xx_mdio_write(const struct device *dev, uint8_t prtad, uint8_t regad, uint16_t data)
{
	struct sy1xx_mdio_dev_config *cfg = (struct sy1xx_mdio_dev_config *)dev->config;
	int ret;
	uint32_t v;

	prtad &= 0x1f;
	regad &= 0x1f;

	/* put the data to the write register */
	sys_write32(data, cfg->base_addr + SY1XX_MDIO_WRITE_DATA_REG);

	v = (SY1XX_MDIO_CTRL_OP_WRITE << SY1XX_MDIO_CTRL_OP_OFFS) |
	    (prtad << SY1XX_MDIO_CTRL_PHY_ADDR_OFFS) | (regad << SY1XX_MDIO_CTRL_REG_ADDR_OFFS) |
	    BIT(SY1XX_MDIO_CTRL_INIT_OFFS);

	/* start the writing procedure */
	sys_write32(v, cfg->base_addr + SY1XX_MDIO_CTRL_REG);

	/* wait for the writing operation to finish */
	ret = sy1xx_mdio_wait_for_ready(dev);
	if (ret < 0) {
		LOG_WRN("timeout while writing to phy: %d, reg: %d, val: %d", prtad, regad, data);
		return ret;
	}

	return 0;
}

static DEVICE_API(mdio, sy1xx_mdio_driver_api) = {
	.read = sy1xx_mdio_read,
	.write = sy1xx_mdio_write,
};

#define SY1XX_MDIO_INIT(n)                                                                         \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static const struct sy1xx_mdio_dev_config sy1xx_mdio_dev_config_##n = {                    \
		.base_addr = DT_INST_REG_ADDR(n),                                                  \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.mdc_freq = DT_INST_PROP(n, clock_frequency),                                      \
	};                                                                                         \
                                                                                                   \
	static struct sy1xx_mdio_dev_data sy1xx_mdio_dev_data##n;                                  \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &sy1xx_mdio_initialize, NULL, &sy1xx_mdio_dev_data##n,            \
			      &sy1xx_mdio_dev_config_##n, POST_KERNEL, CONFIG_MDIO_INIT_PRIORITY,  \
			      &sy1xx_mdio_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SY1XX_MDIO_INIT)
