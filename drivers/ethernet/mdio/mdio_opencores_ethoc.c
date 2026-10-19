/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT opencores_ethoc_mdio

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(mdio_opencores_ethoc, CONFIG_MDIO_LOG_LEVEL);

#define ETHOC_MIIMODER   0x28
#define ETHOC_MIICOMMAND 0x2c
#define ETHOC_MIIADDRESS 0x30
#define ETHOC_MIITX_DATA 0x34
#define ETHOC_MIIRX_DATA 0x38
#define ETHOC_MIISTATUS  0x3c

#define ETHOC_MIIMODER_NOPRE BIT(8)

#define ETHOC_MIICOMMAND_RSTAT     BIT(1)
#define ETHOC_MIICOMMAND_WCTRLDATA BIT(2)

#define ETHOC_MIIADDRESS_FIAD GENMASK(4, 0)
#define ETHOC_MIIADDRESS_RGAD GENMASK(12, 8)

#define ETHOC_MIIDATA GENMASK(15, 0)

#define ETHOC_MIISTATUS_BUSY BIT(1)

struct ethoc_mdio_config {
	const struct device *mac_dev;
	bool suppress_preamble;
};

struct ethoc_mdio_data {
	struct k_mutex lock;
};

static int ethoc_mdio_transfer(const struct device *dev, uint8_t prtad, uint8_t regad,
			       uint32_t command, uint16_t data_in, uint16_t *data_out)
{
	const struct ethoc_mdio_config *cfg = dev->config;
	struct ethoc_mdio_data *data = dev->data;
	mm_reg_t base = DEVICE_MMIO_GET(cfg->mac_dev);
	int ret = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	sys_write32(FIELD_PREP(ETHOC_MIIADDRESS_FIAD, prtad) |
			    FIELD_PREP(ETHOC_MIIADDRESS_RGAD, regad),
		    base + ETHOC_MIIADDRESS);
	if (command == ETHOC_MIICOMMAND_WCTRLDATA) {
		sys_write32(FIELD_PREP(ETHOC_MIIDATA, data_in), base + ETHOC_MIITX_DATA);
	}
	sys_write32(command, base + ETHOC_MIICOMMAND);

	if (!WAIT_FOR((sys_read32(base + ETHOC_MIISTATUS) & ETHOC_MIISTATUS_BUSY) == 0U,
		      CONFIG_MDIO_OPENCORES_ETHOC_BUSY_TIMEOUT, k_busy_wait(1))) {
		ret = -ETIMEDOUT;
	} else if (data_out != NULL) {
		*data_out = FIELD_GET(ETHOC_MIIDATA, sys_read32(base + ETHOC_MIIRX_DATA));
	}

	sys_write32(0U, base + ETHOC_MIICOMMAND);

	k_mutex_unlock(&data->lock);

	return ret;
}

static int ethoc_mdio_read(const struct device *dev, uint8_t prtad, uint8_t regad, uint16_t *data)
{
	return ethoc_mdio_transfer(dev, prtad, regad, ETHOC_MIICOMMAND_RSTAT, 0U, data);
}

static int ethoc_mdio_write(const struct device *dev, uint8_t prtad, uint8_t regad, uint16_t data)
{
	return ethoc_mdio_transfer(dev, prtad, regad, ETHOC_MIICOMMAND_WCTRLDATA, data, NULL);
}

static int ethoc_mdio_init(const struct device *dev)
{
	const struct ethoc_mdio_config *cfg = dev->config;
	struct ethoc_mdio_data *data = dev->data;
	mm_reg_t base = DEVICE_MMIO_GET(cfg->mac_dev);
	uint32_t moder;

	k_mutex_init(&data->lock);

	moder = sys_read32(base + ETHOC_MIIMODER);

	if (cfg->suppress_preamble) {
		moder |= ETHOC_MIIMODER_NOPRE;
	} else {
		moder &= ~ETHOC_MIIMODER_NOPRE;
	}

	sys_write32(moder, base + ETHOC_MIIMODER);

	return 0;
}

static DEVICE_API(mdio, ethoc_mdio_api) = {
	.read = ethoc_mdio_read,
	.write = ethoc_mdio_write,
};

#define ETHOC_MDIO_INIT(inst)                                                                      \
	static const struct ethoc_mdio_config ethoc_mdio_config_##inst = {                         \
		.mac_dev = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                    \
		.suppress_preamble = DT_INST_PROP(inst, suppress_preamble),                        \
	};                                                                                         \
                                                                                                   \
	static struct ethoc_mdio_data ethoc_mdio_data_##inst;                                      \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, ethoc_mdio_init, NULL, &ethoc_mdio_data_##inst,                \
			      &ethoc_mdio_config_##inst, POST_KERNEL, CONFIG_MDIO_INIT_PRIORITY,   \
			      &ethoc_mdio_api);

DT_INST_FOREACH_STATUS_OKAY(ETHOC_MDIO_INIT)
