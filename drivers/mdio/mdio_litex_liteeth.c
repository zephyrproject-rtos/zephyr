/*
 * Copyright (c) 2024 Vogl Electronic GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT litex_liteeth_mdio

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/mdio.h>

#include <soc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(litex_liteeth_mdio, CONFIG_MDIO_LOG_LEVEL);

#define LITEX_MDIO_CLK BIT(0)
#define LITEX_MDIO_OE  BIT(1)
#define LITEX_MDIO_DO  BIT(2)

#define LITEX_MDIO_DI BIT(0)

#define LITEX_MDIO_READ_OP  0
#define LITEX_MDIO_WRITE_OP 1
#define LITEX_MDIO_MSB      0x80000000

struct mdio_litex_data {
	struct k_sem sem;
};

struct mdio_litex_config {
	uint32_t w_addr;
	uint32_t r_addr;
};

static void mdio_litex_read(const struct mdio_litex_config *dev_cfg, uint16_t *pdata)
{
	uint16_t data = 0;

	for (int i = 0; i < 16; i++) {
		data <<= 1;
		if (litex_read8(dev_cfg->r_addr) & LITEX_MDIO_DI) {
			data |= 1;
		}
		litex_write8(LITEX_MDIO_CLK, dev_cfg->w_addr);
		k_busy_wait(1);
		litex_write8(0, dev_cfg->w_addr);
		k_busy_wait(1);
	}

	LOG_DBG("Read data: 0x%04x", data);

	*pdata = data;
}

static void mdio_litex_write(const struct mdio_litex_config *dev_cfg, uint32_t data, uint8_t len)
{
	uint32_t v_data = data;
	uint32_t v_len = len;

	LOG_DBG("Write data: 0x%08x", data);

	v_data <<= 32 - v_len;
	while (v_len > 0) {
		if (v_data & LITEX_MDIO_MSB) {
			litex_write8(LITEX_MDIO_DO | LITEX_MDIO_OE, dev_cfg->w_addr);
			k_busy_wait(1);
			litex_write8(LITEX_MDIO_CLK | LITEX_MDIO_DO | LITEX_MDIO_OE,
				     dev_cfg->w_addr);
			k_busy_wait(1);
			litex_write8(LITEX_MDIO_DO | LITEX_MDIO_OE, dev_cfg->w_addr);
		} else {
			litex_write8(LITEX_MDIO_OE, dev_cfg->w_addr);
			k_busy_wait(1);
			litex_write8(LITEX_MDIO_CLK | LITEX_MDIO_OE, dev_cfg->w_addr);
			k_busy_wait(1);
			litex_write8(LITEX_MDIO_OE, dev_cfg->w_addr);
		}
		v_data <<= 1;
		v_len--;
	}
}

static void mdio_litex_turnaround(const struct mdio_litex_config *dev_cfg)
{
	k_busy_wait(1);
	litex_write8(LITEX_MDIO_CLK, dev_cfg->w_addr);
	k_busy_wait(1);
	litex_write8(0, dev_cfg->w_addr);
	k_busy_wait(1);
	litex_write8(LITEX_MDIO_CLK, dev_cfg->w_addr);
	k_busy_wait(1);
	litex_write8(0, dev_cfg->w_addr);
}

static int mdio_litex_transfer(const struct device *dev, uint8_t prtad, uint8_t devad, uint8_t rw,
			       uint16_t data_in, uint16_t *data_out)
{
	const struct mdio_litex_config *const dev_cfg = dev->config;
	struct mdio_litex_data *const dev_data = dev->data;

	k_sem_take(&dev_data->sem, K_FOREVER);

	litex_write8(LITEX_MDIO_OE, dev_cfg->w_addr);
	/* PRE32: 32 bits '1' for sync*/
	mdio_litex_write(dev_cfg, 0xFFFFFFFF, 32);
	/* ST: 2 bits start of frame */
	mdio_litex_write(dev_cfg, 0x1, 2);
	/* OP: 2 bits opcode, read '10' or write '01' */
	mdio_litex_write(dev_cfg, rw ? 0x1 : 0x2, 2);
	/* PA5: 5 bits PHY address */
	mdio_litex_write(dev_cfg, prtad, 5);
	/* RA5: 5 bits register address */
	mdio_litex_write(dev_cfg, devad, 5);

	if (rw) { /* Write data */
		/* TA: 2 bits turn-around */
		mdio_litex_write(dev_cfg, 0x2, 2);
		mdio_litex_write(dev_cfg, data_in, 16);
	} else { /* Read data */
		mdio_litex_turnaround(dev_cfg);
		mdio_litex_read(dev_cfg, data_out);
	}

	mdio_litex_turnaround(dev_cfg);

	k_sem_give(&dev_data->sem);

	return 0;
}

static int mdio_litex_read_mmi(const struct device *dev, uint8_t prtad, uint8_t devad,
			       uint16_t *data)
{
	return mdio_litex_transfer(dev, prtad, devad, LITEX_MDIO_READ_OP, 0, data);
}

static int mdio_litex_write_mmi(const struct device *dev, uint8_t prtad, uint8_t devad,
				uint16_t data)
{
	return mdio_litex_transfer(dev, prtad, devad, LITEX_MDIO_WRITE_OP, data, NULL);
}

static int mdio_litex_initialize(const struct device *dev)
{
	struct mdio_litex_data *const dev_data = dev->data;

	k_sem_init(&dev_data->sem, 1, 1);

	return 0;
}

static DEVICE_API(mdio, mdio_litex_driver_api) = {
	.read = mdio_litex_read_mmi,
	.write = mdio_litex_write_mmi,
};

#define MDIO_LITEX_DEVICE(inst)                                                                    \
	static struct mdio_litex_config mdio_litex_dev_config_##inst = {                           \
		.w_addr = DT_INST_REG_ADDR_BY_NAME(inst, mdio_w),                                  \
		.r_addr = DT_INST_REG_ADDR_BY_NAME(inst, mdio_r),                                  \
	};                                                                                         \
	static struct mdio_litex_data mdio_litex_dev_data_##inst;                                  \
	DEVICE_DT_INST_DEFINE(inst, &mdio_litex_initialize, NULL, &mdio_litex_dev_data_##inst,     \
			      &mdio_litex_dev_config_##inst, POST_KERNEL,                          \
			      CONFIG_MDIO_INIT_PRIORITY, &mdio_litex_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MDIO_LITEX_DEVICE)
