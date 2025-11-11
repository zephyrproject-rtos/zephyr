/*
 * Copyright (c) 2023 TOKITA Hiroshi <tokita.hiroshi@fujitsu.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT raspberrypi_core_supply_regulator

#include <zephyr/devicetree.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/dt-bindings/regulator/rpi_pico.h>
#include <zephyr/sys/linear_range.h>
#include <zephyr/toolchain.h>
#include <hardware/regs/vreg_and_chip_reset.h>
#include <hardware/structs/vreg_and_chip_reset.h>

static const struct linear_range core_ranges[] = {
	LINEAR_RANGE_INIT(800000u, 0u, 0u, 5u),
	LINEAR_RANGE_INIT(850000u, 50000u, 6u, 15u),
};

static const size_t num_core_ranges = ARRAY_SIZE(core_ranges);

struct regulator_rpi_pico_config {
	struct regulator_common_config common;
	vreg_and_chip_reset_hw_t * const reg;
	const bool brown_out_detection;
	const uint32_t brown_out_threshold;
};

struct regulator_rpi_pico_data {
	struct regulator_common_data data;
};

/*
 * APIs
 */

static unsigned int regulator_rpi_pico_count_voltages(const struct device *dev)
{
	return linear_range_group_values_count(core_ranges, num_core_ranges);
}

static int regulator_rpi_pico_list_voltage(const struct device *dev, unsigned int idx,
					   int32_t *volt_uv)
{
	return linear_range_group_get_value(core_ranges, num_core_ranges, idx, volt_uv);
}

static int regulator_rpi_pico_set_voltage(const struct device *dev, int32_t min_uv, int32_t max_uv)
{
	const struct regulator_rpi_pico_config *config = dev->config;
	uint16_t idx;
	int ret;

	ret = linear_range_group_get_win_index(core_ranges, num_core_ranges, min_uv, max_uv, &idx);
	if (ret < 0) {
		return ret;
	}

	config->reg->vreg = ((config->reg->vreg & ~VREG_AND_CHIP_RESET_VREG_VSEL_BITS) |
			     (idx << VREG_AND_CHIP_RESET_VREG_VSEL_LSB));

	return 0;
}

static int regulator_rpi_pico_get_voltage(const struct device *dev, int32_t *volt_uv)
{
	const struct regulator_rpi_pico_config *config = dev->config;

	return linear_range_group_get_value(
		core_ranges, num_core_ranges,
		((config->reg->vreg & VREG_AND_CHIP_RESET_VREG_VSEL_BITS) >>
		 VREG_AND_CHIP_RESET_VREG_VSEL_LSB),
		volt_uv);
}

static int regulator_rpi_pico_enable(const struct device *dev)
{
	const struct regulator_rpi_pico_config *config = dev->config;

	config->reg->vreg |= BIT(VREG_AND_CHIP_RESET_VREG_EN_LSB);

	return 0;
}

static int regulator_rpi_pico_disable(const struct device *dev)
{
	const struct regulator_rpi_pico_config *config = dev->config;

	config->reg->vreg &= ~BIT(VREG_AND_CHIP_RESET_VREG_EN_LSB);

	return 0;
}

static int regulator_rpi_pico_set_mode(const struct device *dev, regulator_mode_t mode)
{
	const struct regulator_rpi_pico_config *config = dev->config;

	if (mode & REGULATOR_RPI_PICO_MODE_HI_Z) {
		config->reg->vreg |= REGULATOR_RPI_PICO_MODE_HI_Z;
	} else {
		config->reg->vreg &= (~REGULATOR_RPI_PICO_MODE_HI_Z);
	}

	return 0;
}

static int regulator_rpi_pico_get_mode(const struct device *dev, regulator_mode_t *mode)
{
	const struct regulator_rpi_pico_config *config = dev->config;

	*mode = (config->reg->vreg & REGULATOR_RPI_PICO_MODE_HI_Z);

	return 0;
}

static int regulator_rpi_pico_init(const struct device *dev)
{
	const struct regulator_rpi_pico_config *config = dev->config;

	if (config->brown_out_detection) {
		config->reg->bod =
			(BIT(VREG_AND_CHIP_RESET_BOD_EN_LSB) |
			 (config->brown_out_threshold << VREG_AND_CHIP_RESET_BOD_VSEL_LSB));
	} else {
		config->reg->bod &= ~BIT(VREG_AND_CHIP_RESET_BOD_EN_LSB);
	}

	regulator_common_data_init(dev);

	return regulator_common_init(dev, true);
}

static DEVICE_API(regulator, api) = {
	.enable = regulator_rpi_pico_enable,
	.disable = regulator_rpi_pico_disable,
	.count_voltages = regulator_rpi_pico_count_voltages,
	.list_voltage = regulator_rpi_pico_list_voltage,
	.set_voltage = regulator_rpi_pico_set_voltage,
	.get_voltage = regulator_rpi_pico_get_voltage,
	.set_mode = regulator_rpi_pico_set_mode,
	.get_mode = regulator_rpi_pico_get_mode,
};

#define REGULATOR_RPI_PICO_DEFINE_ALL(inst)                                                        \
	static struct regulator_rpi_pico_data data_##inst;                                         \
                                                                                                   \
	static const struct regulator_rpi_pico_config config_##inst = {                            \
		.common = REGULATOR_DT_COMMON_CONFIG_INIT(inst),                                   \
		.reg = (vreg_and_chip_reset_hw_t * const)DT_INST_REG_ADDR(inst),                   \
		.brown_out_detection = DT_INST_PROP(inst, raspberrypi_brown_out_detection),        \
		.brown_out_threshold = DT_INST_ENUM_IDX(inst, raspberrypi_brown_out_threshold),    \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, regulator_rpi_pico_init, NULL, &data_##inst, &config_##inst,   \
			      POST_KERNEL, CONFIG_REGULATOR_RPI_PICO_INIT_PRIORITY, &api);

DT_INST_FOREACH_STATUS_OKAY(REGULATOR_RPI_PICO_DEFINE_ALL)
