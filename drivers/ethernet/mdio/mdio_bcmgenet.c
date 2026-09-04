/*
 * Copyright (c) 2026 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT brcm_genet_mdio

#include <zephyr/kernel.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/sys/util.h>

#define BCMGENET_MDIO_START  BIT(29)
#define BCMGENET_MDIO_READ   BIT(27)
#define BCMGENET_MDIO_WRITE  BIT(26)
#define BCMGENET_MDIO_PMD(x) ((x << 21) & GENMASK(25, 21))
#define BCMGENET_MDIO_REG(x) ((x << 16) & GENMASK(20, 16))

struct mdio_bcmgenet_data {
	struct k_mutex mdio_transfer_lock;
};

struct mdio_bcmgenet_config {
	const struct device *eth_dev;
	uint32_t mdio_reg;
};

static inline uint32_t mdio_bcmgenet_read_reg(const struct device *dev)
{
	const struct mdio_bcmgenet_config *cfg = dev->config;

	return sys_read32(DEVICE_MMIO_GET(cfg->eth_dev) + cfg->mdio_reg);
}

static inline void mdio_bcmgenet_write_reg(const struct device *dev, uint32_t value)
{
	const struct mdio_bcmgenet_config *cfg = dev->config;

	sys_write32(value, DEVICE_MMIO_GET(cfg->eth_dev) + cfg->mdio_reg);
}

static inline void mdio_bcmgenet_start_xfer(const struct device *dev)
{
	const struct mdio_bcmgenet_config *cfg = dev->config;

	sys_set_bits(DEVICE_MMIO_GET(cfg->eth_dev) + cfg->mdio_reg, BCMGENET_MDIO_START);
}

static int mdio_bcmgenet_transfer(const struct device *dev, uint8_t prtad, uint8_t regad,
				  bool write, uint16_t data_in, uint16_t *data_out)
{
	struct mdio_bcmgenet_data *data = dev->data;
	uint32_t value;
	int ret = 0;

	if (prtad > 31 || regad > 31) {
		return -EINVAL;
	}

	k_mutex_lock(&data->mdio_transfer_lock, K_FOREVER);
	if (write) {
		value = BCMGENET_MDIO_WRITE | BCMGENET_MDIO_PMD(prtad) | BCMGENET_MDIO_REG(regad) |
			data_in;
	} else {
		value = BCMGENET_MDIO_READ | BCMGENET_MDIO_PMD(prtad) | BCMGENET_MDIO_REG(regad);
	}

	mdio_bcmgenet_write_reg(dev, value);
	mdio_bcmgenet_start_xfer(dev);

	if (!WAIT_FOR((mdio_bcmgenet_read_reg(dev) & BCMGENET_MDIO_START) == 0, 50,
		      k_busy_wait(1))) {
		ret = -ETIMEDOUT;
		goto release_lock;
	}

	if (!write) {
		*data_out = mdio_bcmgenet_read_reg(dev) & UINT16_MAX;
	}

release_lock:
	k_mutex_unlock(&data->mdio_transfer_lock);
	return ret;
}

static int mdio_bcmgenet_read(const struct device *dev, uint8_t prtad, uint8_t regad,
			      uint16_t *data)
{
	return mdio_bcmgenet_transfer(dev, prtad, regad, false, 0, data);
}

static int mdio_bcmgenet_write(const struct device *dev, uint8_t prtad, uint8_t regad,
			       uint16_t data)
{
	return mdio_bcmgenet_transfer(dev, prtad, regad, true, data, NULL);
}

static int mdio_bcmgenet_init(const struct device *dev)
{
	struct mdio_bcmgenet_data *data = dev->data;

	k_mutex_init(&data->mdio_transfer_lock);

	return 0;
}

static DEVICE_API(mdio, mdio_bcmgenet_api) = {
	.read = mdio_bcmgenet_read,
	.write = mdio_bcmgenet_write,
};

#define MDIO_BCMGENET_INIT(n)                                                                      \
	static struct mdio_bcmgenet_data mdio_bcmgenet_data##n;                                    \
                                                                                                   \
	static const struct mdio_bcmgenet_config mdio_bcmgenet_config_##n = {                      \
		.eth_dev = DEVICE_DT_GET(DT_INST_PARENT(n)),                                       \
		.mdio_reg = DT_INST_REG_ADDR(n),                                                   \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &mdio_bcmgenet_init, NULL, &mdio_bcmgenet_data##n,                \
			      &mdio_bcmgenet_config_##n, POST_KERNEL, CONFIG_MDIO_INIT_PRIORITY,   \
			      &mdio_bcmgenet_api);

DT_INST_FOREACH_STATUS_OKAY(MDIO_BCMGENET_INIT)
