/*
 * MDIO driver for Synopsys DesignWare Ethernet MAC (clause 22)
 *
 * Copyright (c) 2026 KylinSoft
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT snps_designware_mdio

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(mdio_designware, CONFIG_MDIO_LOG_LEVEL);

#define MAC_MDIO_ADDRESS 0x0200
#define MAC_MDIO_DATA    0x0204

#define MDIO_ADDR_PA  GENMASK(25, 21)
#define MDIO_ADDR_RDA GENMASK(20, 16)
#define MDIO_ADDR_CR  GENMASK(11, 8)
#define MDIO_ADDR_GB  BIT(0)
#define MDIO_DATA_GD  GENMASK(15, 0)

#define MDIO_GOC_READ  3
#define MDIO_GOC_WRITE 1

struct mdio_designware_config {
	uintptr_t base;
	uint32_t reg_size;
	uint32_t clk_hz;
};

struct mdio_designware_data {
	struct k_mutex lock;
	mm_reg_t mmio;
	bool mapped;
};

static int mdio_designware_wait_busy(mm_reg_t base)
{
	k_timepoint_t timeout = sys_timepoint_calc(K_MSEC(100));
	uint32_t gb_val;

	while ((gb_val = sys_read32(base + MAC_MDIO_ADDRESS)) & MDIO_ADDR_GB) {
		if (sys_timepoint_expired(timeout)) {
			return -ETIMEDOUT;
		}
	}

	return 0;
}

static int mdio_designware_c22(mm_reg_t base, uint32_t clk_hz, uint8_t prtad, uint8_t regad,
			       bool write, uint16_t data_in, uint16_t *data_out)
{
	uint32_t addr;
	uint32_t cr;
	int ret;

	if (clk_hz >= 250000000U) {
		cr = 0x5;
	} else if (clk_hz >= 150000000U) {
		cr = 0x4;
	} else if (clk_hz >= 100000000U) {
		cr = 0x1;
	} else {
		cr = 0x0;
	}

	ret = mdio_designware_wait_busy(base);
	if (ret != 0) {
		return ret;
	}

	if (write) {
		sys_write32(data_in & MDIO_DATA_GD, base + MAC_MDIO_DATA);
	}

	addr = FIELD_PREP(MDIO_ADDR_PA, prtad) | FIELD_PREP(MDIO_ADDR_RDA, regad) |
	       FIELD_PREP(MDIO_ADDR_CR, cr) | (write ? MDIO_GOC_WRITE : MDIO_GOC_READ) << 2 |
	       MDIO_ADDR_GB;
	sys_write32(addr, base + MAC_MDIO_ADDRESS);

	ret = mdio_designware_wait_busy(base);
	if (ret != 0) {
		return ret;
	}

	if (!write) {
		*data_out = sys_read32(base + MAC_MDIO_DATA) & MDIO_DATA_GD;
	}

	return 0;
}

static int mdio_designware_ensure_map(const struct device *dev)
{
	const struct mdio_designware_config *cfg = dev->config;
	struct mdio_designware_data *data = dev->data;

	if (data->mapped) {
		return 0;
	}

	data->mmio = (mm_reg_t)cfg->base;
	data->mapped = true;
	return 0;
}

static int mdio_designware_read(const struct device *dev, uint8_t prtad, uint8_t regad,
				uint16_t *data)
{
	const struct mdio_designware_config *cfg = dev->config;
	struct mdio_designware_data *dev_data = dev->data;
	int ret;

	ret = mdio_designware_ensure_map(dev);
	if (ret != 0) {
		return ret;
	}

	ret = k_mutex_lock(&dev_data->lock, K_FOREVER);
	if (ret != 0) {
		return ret;
	}

	ret = mdio_designware_c22(dev_data->mmio, cfg->clk_hz, prtad, regad, false, 0, data);
	k_mutex_unlock(&dev_data->lock);
	return ret;
}

static int mdio_designware_write(const struct device *dev, uint8_t prtad, uint8_t regad,
				 uint16_t data)
{
	const struct mdio_designware_config *cfg = dev->config;
	struct mdio_designware_data *dev_data = dev->data;
	int ret;

	ret = mdio_designware_ensure_map(dev);
	if (ret != 0) {
		return ret;
	}

	ret = k_mutex_lock(&dev_data->lock, K_FOREVER);
	if (ret != 0) {
		return ret;
	}

	ret = mdio_designware_c22(dev_data->mmio, cfg->clk_hz, prtad, regad, true, data, NULL);
	k_mutex_unlock(&dev_data->lock);
	return ret;
}

static int mdio_designware_init(const struct device *dev)
{
	struct mdio_designware_data *data = dev->data;

	k_mutex_init(&data->lock);
	data->mapped = false;
	return 0;
}

static DEVICE_API(mdio, mdio_designware_api) = {
	.read = mdio_designware_read,
	.write = mdio_designware_write,
};

#define MDIO_DESIGNWARE_INIT(n)                                                                    \
	static struct mdio_designware_data mdio_designware_data_##n;                               \
	static const struct mdio_designware_config mdio_designware_config_##n = {                  \
		.base = DT_REG_ADDR(DT_DRV_INST(n)),                                               \
		.reg_size = DT_REG_SIZE(DT_DRV_INST(n)),                                           \
		.clk_hz = DT_INST_PROP_OR(n, clock_frequency, 125000000),                          \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, mdio_designware_init, NULL, &mdio_designware_data_##n,            \
			      &mdio_designware_config_##n, POST_KERNEL, CONFIG_MDIO_INIT_PRIORITY, \
			      &mdio_designware_api);

DT_INST_FOREACH_STATUS_OKAY(MDIO_DESIGNWARE_INIT)
