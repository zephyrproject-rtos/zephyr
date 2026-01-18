/*
 * Copyright (c) 2025 Synaptics, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT syna_sr100_reset

#include <zephyr/arch/common/sys_io.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/dt-bindings/reset/syna_sr100_reset.h>

struct reset_syna_config {
	uint32_t base;
};

static int syna_reset_validate_sticky_reg(uint32_t reg)
{
	int ret = -EINVAL;

	switch (reg) {
	case TOP_STICKY_RST:
	case PER_STICKY_RST:
		ret = 0;
		break;
	default:
		break;
	}

	return ret;
}

static int syna_reset_validate_reset_reg(uint32_t reg)
{
	int ret = -EINVAL;

	switch (reg) {
	case IMGPROC_RST:
	case NPU_RST:
	case DMA0_RST:
	case DMA1_RST:
	case AXI_RST:
	case PERIF_RST:
	case APB_PERIF_RST:
		ret = 0;
		break;
	default:
		break;
	}

	return ret;
}

static int syna_reset_status(const struct device *dev, uint32_t id, uint8_t *status)
{
	const struct reset_syna_config *config = dev->config;
	uint32_t val;
	uint32_t sticky_reg;
	uint32_t rst_reg;
	uint32_t rst_status = 0;
	int ret;

	sticky_reg = (id >> STI_REG) & 0xff;
	if (sticky_reg) {
		uint32_t sticky_mask = (id >> STI_MASK) & 0xff;

		ret = syna_reset_validate_sticky_reg(sticky_reg);
		if (ret < 0) {
			return ret;
		}

		val = sys_read32(config->base + sticky_reg);
		if (!(val & sticky_mask)) {
			rst_status = 1;
		}
	}

	rst_reg = (id >> RST_REG) & 0xff;
	if (rst_reg) {
		uint32_t rst_mask = (id >> RST_MASK) & 0xff;

		ret = syna_reset_validate_reset_reg(rst_reg);
		if (ret < 0) {
			return ret;
		}

		val = sys_read32(config->base + rst_reg);
		if (val & rst_mask) {
			rst_status = 1;
		}
	}

	*status = rst_status;

	return 0;
}

static int syna_reset_line_set(const struct device *dev, uint32_t id,
			       uint32_t assert)
{
	const struct reset_syna_config *config = dev->config;
	uint32_t val;
	uint32_t sticky_reg;
	uint32_t rst_reg;
	int ret;

	sticky_reg = (id >> STI_REG) & 0xff;
	if (sticky_reg) {
		uint32_t sticky_mask = (id >> STI_MASK) & 0xff;

		ret = syna_reset_validate_sticky_reg(sticky_reg);
		if (ret < 0) {
			return ret;
		}

		val = sys_read32(config->base + sticky_reg);
		if (assert) {
			val &= ~sticky_mask;
		} else {
			val |= sticky_mask;
		}
		sys_write32(val, config->base + sticky_reg);
	}

	rst_reg = (id >> RST_REG) & 0xff;
	if (rst_reg) {
		uint32_t rst_mask = (id >> RST_MASK) & 0xff;

		ret = syna_reset_validate_reset_reg(rst_reg);
		if (ret < 0) {
			return ret;
		}

		val = sys_read32(config->base + rst_reg);
		if (assert) {
			val |= rst_mask;
		} else {
			val &= ~rst_mask;
		}
		sys_write32(val, config->base + rst_reg);
	}

	return 0;
}

static int syna_reset_line_assert(const struct device *dev, uint32_t id)
{
	return syna_reset_line_set(dev, id, 1);
}

static int syna_reset_line_deassert(const struct device *dev, uint32_t id)
{
	return syna_reset_line_set(dev, id, 0);
}

static int syna_reset_line_toggle(const struct device *dev, uint32_t id)
{
	int ret;

	ret = syna_reset_line_set(dev, id, 1);
	if (ret == 0) {
		ret = syna_reset_line_set(dev, id, 0);
	}

	return ret;
}

static DEVICE_API(reset, syna_reset_api) = {
	.status = syna_reset_status,
	.line_assert = syna_reset_line_assert,
	.line_deassert = syna_reset_line_deassert,
	.line_toggle = syna_reset_line_toggle
};

#define SYNA_RESET_INIT(n)                                                      \
	static const struct reset_syna_config reset_syna_cfg_##n = {            \
		.base = DT_INST_REG_ADDR(n),                                    \
	};                                                                      \
	DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, &reset_syna_cfg_##n,         \
			      PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, \
			      &syna_reset_api);

DT_INST_FOREACH_STATUS_OKAY(SYNA_RESET_INIT)
