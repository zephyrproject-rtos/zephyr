/*
 * Copyright (c) 2025 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT intel_igc_mdio

#include <zephyr/kernel.h>
#include <zephyr/net/mdio.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/drivers/pcie/pcie.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(intel_igc_mdio, CONFIG_MDIO_LOG_LEVEL);

#define INTEL_IGC_MDIC_OFFSET                  0x00020
#define INTEL_IGC_MDIC_DATA_MASK               GENMASK(15, 0)
#define INTEL_IGC_MDIC_REG_MASK                GENMASK(20, 16)
#define INTEL_IGC_MDIC_PHY_MASK                GENMASK(25, 21)
#define INTEL_IGC_MDIC_OP_MASK                 GENMASK(27, 26)
#define INTEL_IGC_MDIC_READY                   BIT(28)
#define INTEL_IGC_MMDCTRL                      0xD
#define INTEL_IGC_MMDCTRL_ACTYPE_MASK          GENMASK(15, 14)
#define INTEL_IGC_MMDCTRL_DEVAD_MASK           GENMASK(4, 0)
#define INTEL_IGC_MMDDATA                      0xE
#define INTEL_IGC_DEFAULT_DEVNUM               0

struct intel_igc_mdio_cfg {
	const struct device *const platform;
};

struct intel_igc_mdio_data {
	struct k_mutex mdio_lock;
	mm_reg_t base;
};

static int intel_igc_mdio(struct intel_igc_mdio_data *data, uint32_t command)
{
	uint32_t mdic = data->base + INTEL_IGC_MDIC_OFFSET;
	int ret;

	k_mutex_lock(&data->mdio_lock, K_FOREVER);

	sys_write32(command, mdic);
	/* Wait for the read or write transaction to complete */
	if (!WAIT_FOR((sys_read32(mdic) & INTEL_IGC_MDIC_READY),
		      CONFIG_MDIO_INTEL_BUSY_CHECK_TIMEOUT, k_usleep(1))) {
		LOG_ERR("MDIC operation timed out");
		k_mutex_unlock(&data->mdio_lock);
		return -ETIMEDOUT;
	}

	ret = sys_read32(mdic);
	k_mutex_unlock(&data->mdio_lock);

	return ret;
}

static int intel_igc_mdio_read(const struct device *dev, uint8_t prtad,
			       uint8_t regad, uint16_t *user_data)
{
	struct intel_igc_mdio_data *const dev_data = dev->data;
	int ret;

	uint32_t command = FIELD_PREP(INTEL_IGC_MDIC_PHY_MASK, prtad) |
			   FIELD_PREP(INTEL_IGC_MDIC_REG_MASK, regad) |
			   FIELD_PREP(INTEL_IGC_MDIC_OP_MASK, MDIO_OP_C22_READ);

	ret = intel_igc_mdio(dev_data, command);
	if (ret < 0) {
		return ret;
	}

	*user_data = FIELD_GET(INTEL_IGC_MDIC_DATA_MASK, ret);

	return 0;
}

static int intel_igc_mdio_write(const struct device *dev, uint8_t prtad,
				uint8_t regad, uint16_t user_data)
{
	struct intel_igc_mdio_data *const dev_data = dev->data;
	int ret;

	uint32_t command = FIELD_PREP(INTEL_IGC_MDIC_PHY_MASK, prtad) |
			   FIELD_PREP(INTEL_IGC_MDIC_REG_MASK, regad) |
			   FIELD_PREP(INTEL_IGC_MDIC_OP_MASK, MDIO_OP_C22_WRITE) |
			   FIELD_PREP(INTEL_IGC_MDIC_DATA_MASK, user_data);

	ret = intel_igc_mdio(dev_data, command);

	return ret < 0 ? ret : 0;
}

static int intel_igc_mdio_pre_handle_c45(const struct device *dev, uint8_t prtad,
					 uint8_t devnum, uint16_t regad)
{
	int ret;

	/* Set device number using MMDCTRL */
	ret = intel_igc_mdio_write(dev, prtad, INTEL_IGC_MMDCTRL,
				   (uint16_t)(FIELD_PREP(INTEL_IGC_MMDCTRL_DEVAD_MASK, devnum)));
	if (ret < 0) {
		return ret;
	}

	/* Set register address using MMDDATA */
	ret = intel_igc_mdio_write(dev, prtad, INTEL_IGC_MMDDATA, regad);
	if (ret < 0) {
		return ret;
	}

	/* Set device number and access type as data using MMDCTRL */
	return intel_igc_mdio_write(dev, prtad, INTEL_IGC_MMDCTRL,
				    (uint16_t)(FIELD_PREP(INTEL_IGC_MMDCTRL_ACTYPE_MASK, 1) |
					       FIELD_PREP(INTEL_IGC_MMDCTRL_DEVAD_MASK, devnum)));
}

static int intel_igc_mdio_post_handle_c45(const struct device *dev, uint8_t prtad)
{
	/* Restore default device number using MMDCTRL */
	return intel_igc_mdio_write(dev, prtad, INTEL_IGC_MMDCTRL, INTEL_IGC_DEFAULT_DEVNUM);
}

static int intel_igc_mdio_read_c45(const struct device *dev, uint8_t prtad,
				   uint8_t devnum, uint16_t regad, uint16_t *user_data)
{
	int ret = intel_igc_mdio_pre_handle_c45(dev, prtad, devnum, regad);

	if (ret < 0) {
		return ret;
	}

	/* Read user data using MMDDATA */
	ret = intel_igc_mdio_read(dev, prtad, INTEL_IGC_MMDDATA, user_data);
	if (ret < 0) {
		return ret;
	}

	return intel_igc_mdio_post_handle_c45(dev, prtad);
}

static int intel_igc_mdio_write_c45(const struct device *dev, uint8_t prtad,
				    uint8_t devnum, uint16_t regad, uint16_t user_data)
{
	int ret = intel_igc_mdio_pre_handle_c45(dev, prtad, devnum, regad);

	if (ret < 0) {
		return ret;
	}

	/* Write the user_data using MMDDATA */
	ret = intel_igc_mdio_write(dev, prtad, INTEL_IGC_MMDDATA, user_data);
	if (ret < 0) {
		return ret;
	}

	return intel_igc_mdio_post_handle_c45(dev, prtad);
}

static void intel_igc_mdio_bus_enable(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static void intel_igc_mdio_bus_disable(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static const struct mdio_driver_api mdio_api = {
	.read = intel_igc_mdio_read,
	.write = intel_igc_mdio_write,
	.read_c45 = intel_igc_mdio_read_c45,
	.write_c45 = intel_igc_mdio_write_c45,
	.bus_enable = intel_igc_mdio_bus_enable,
	.bus_disable = intel_igc_mdio_bus_disable,
};

static int intel_igc_mdio_init(const struct device *dev)
{
	const struct intel_igc_mdio_cfg *cfg = dev->config;
	struct intel_igc_mdio_data *data = dev->data;

	data->base = DEVICE_MMIO_GET(cfg->platform);

	return k_mutex_init(&data->mdio_lock);
}

#define INTEL_IGC_MDIO_INIT(n) \
	static struct intel_igc_mdio_data mdio_data_##n; \
	static struct intel_igc_mdio_cfg mdio_cfg_##n = { \
		.platform = DEVICE_DT_GET(DT_INST_PARENT(n)), \
	}; \
	\
	DEVICE_DT_INST_DEFINE(n, intel_igc_mdio_init, NULL, &mdio_data_##n, &mdio_cfg_##n, \
			      POST_KERNEL, CONFIG_MDIO_INIT_PRIORITY, &mdio_api);

DT_INST_FOREACH_STATUS_OKAY(INTEL_IGC_MDIO_INIT)
