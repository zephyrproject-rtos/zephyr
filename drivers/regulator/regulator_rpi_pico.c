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

#ifdef CONFIG_SOC_SERIES_RP2350
#include <hardware/regs/powman.h>
#include <hardware/structs/powman.h>
#else
#include <hardware/regs/vreg_and_chip_reset.h>
#include <hardware/structs/vreg_and_chip_reset.h>
#endif

static const struct linear_range core_ranges[] = {
#ifdef CONFIG_SOC_SERIES_RP2350
	LINEAR_RANGE_INIT(550000u, 50000u, 0u, 17u),
	LINEAR_RANGE_INIT(1500000u, 100000u, 18u, 19u),
	LINEAR_RANGE_INIT(1650000u, 50000u, 20u, 21u),
	LINEAR_RANGE_INIT(1800000u, 100000u, 22u, 24u),
	LINEAR_RANGE_INIT(2350000u, 50000u, 25u, 25u),
	LINEAR_RANGE_INIT(2500000u, 150000u, 26u, 28u),
	LINEAR_RANGE_INIT(3000000u, 150000u, 29u, 31u),
#else
	LINEAR_RANGE_INIT(800000u, 0u, 0u, 5u),
	LINEAR_RANGE_INIT(850000u, 50000u, 6u, 15u),
#endif
};

static const size_t num_core_ranges = ARRAY_SIZE(core_ranges);

#ifdef CONFIG_SOC_SERIES_RP2350
#define REG_PICO_TYPE		powman_hw_t
#define REG_VSEL_POS		POWMAN_VREG_VSEL_LSB
#define REG_VSEL_MSK		POWMAN_VREG_VSEL_BITS
#define REG_VALIN(value)	(POWMAN_PASSWORD_BITS | (value))
#define REG_BOD_VSEL_POS	POWMAN_BOD_VSEL_LSB
#define REG_BOD_EN_POS		POWMAN_BOD_EN_LSB
#else
#define REG_PICO_TYPE		vreg_and_chip_reset_hw_t
#define REG_VSEL_POS		VREG_AND_CHIP_RESET_VREG_VSEL_LSB
#define REG_VSEL_MSK		VREG_AND_CHIP_RESET_VREG_VSEL_BITS
#define REG_VALIN(value)	(value)
#define REG_BOD_VSEL_POS	VREG_AND_CHIP_RESET_BOD_VSEL_LSB
#define REG_BOD_EN_POS		VREG_AND_CHIP_RESET_BOD_EN_LSB
#endif

struct regulator_rpi_pico_config {
	struct regulator_common_config common;
	REG_PICO_TYPE * const reg;
	const bool brown_out_detection;
	const uint32_t brown_out_threshold;
};

struct regulator_rpi_pico_data {
	struct regulator_common_data data;
};

#ifdef CONFIG_SOC_SERIES_RP2350
static void regulator_rpi_pico_wait_powman(const struct device *dev)
{
	const struct regulator_rpi_pico_config *config = dev->config;

	while (config->reg->vreg & POWMAN_VREG_UPDATE_IN_PROGRESS_BITS) {
		k_usleep(10);
	}
}
#endif

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

#ifdef CONFIG_SOC_SERIES_RP2350
	config->reg->vreg_ctrl |= REG_VALIN(POWMAN_VREG_CTRL_UNLOCK_BITS);
	regulator_rpi_pico_wait_powman(dev);
#endif

	config->reg->vreg = REG_VALIN((config->reg->vreg & ~REG_VSEL_MSK) | idx << REG_VSEL_POS);

#ifdef CONFIG_SOC_SERIES_RP2350
	regulator_rpi_pico_wait_powman(dev);
#endif

	return 0;
}

static int regulator_rpi_pico_get_voltage(const struct device *dev, int32_t *volt_uv)
{
	const struct regulator_rpi_pico_config *config = dev->config;

	return linear_range_group_get_value(
		core_ranges, num_core_ranges,
		((config->reg->vreg & REG_VSEL_MSK) >> REG_VSEL_POS), volt_uv);
}

static int regulator_rpi_pico_enable(const struct device *dev)
{
#ifdef CONFIG_SOC_SERIES_RP2040
	const struct regulator_rpi_pico_config *config = dev->config;

	config->reg->vreg |= BIT(VREG_AND_CHIP_RESET_VREG_EN_LSB);
#endif

	return 0;
}

static int regulator_rpi_pico_disable(const struct device *dev)
{
#ifdef CONFIG_SOC_SERIES_RP2040
	const struct regulator_rpi_pico_config *config = dev->config;

	config->reg->vreg &= ~BIT(VREG_AND_CHIP_RESET_VREG_EN_LSB);
#endif

	return 0;
}

static int regulator_rpi_pico_set_mode(const struct device *dev, regulator_mode_t mode)
{
	const struct regulator_rpi_pico_config *config = dev->config;

	if (mode & REGULATOR_RPI_PICO_MODE_HI_Z) {
		config->reg->vreg |= REG_VALIN(REGULATOR_RPI_PICO_MODE_HI_Z);
	} else {
		config->reg->vreg = REG_VALIN(config->reg->vreg & ~REGULATOR_RPI_PICO_MODE_HI_Z);
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
		config->reg->bod = REG_VALIN(BIT(REG_BOD_EN_POS) |
			 (config->brown_out_threshold << REG_BOD_VSEL_POS));
	} else {
		config->reg->bod = REG_VALIN(config->reg->bod & ~BIT(REG_BOD_EN_POS));
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
		.reg = (REG_PICO_TYPE * const)DT_INST_REG_ADDR(inst),                              \
		.brown_out_detection = DT_INST_PROP(inst, raspberrypi_brown_out_detection),        \
		.brown_out_threshold = DT_INST_ENUM_IDX(inst, raspberrypi_brown_out_threshold),    \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, regulator_rpi_pico_init, NULL, &data_##inst, &config_##inst,   \
			      POST_KERNEL, CONFIG_REGULATOR_RPI_PICO_INIT_PRIORITY, &api);         \
	IF_ENABLED(CONFIG_SOC_SERIES_RP2040,                                                       \
		   (BUILD_ASSERT(DT_INST_ENUM_IDX(inst, raspberrypi_brown_out_threshold) < 16,     \
		   "On RP2040, BOD threshold must be lower than 1161000");))                       \

DT_INST_FOREACH_STATUS_OKAY(REGULATOR_RPI_PICO_DEFINE_ALL)
