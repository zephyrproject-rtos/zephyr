/*
 * Copyright 2023-2024 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_vref

#include <errno.h>

#include <zephyr/drivers/regulator.h>
#include <zephyr/dt-bindings/regulator/nxp_vref.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/linear_range.h>
#include <zephyr/sys/util.h>

#include <fsl_device_registers.h>

static const struct linear_range utrim_range = LINEAR_RANGE_INIT(1000000, 100000U, 0x0U, 0xBU);

struct regulator_nxp_vref_data {
	struct regulator_common_data common;
};

struct regulator_nxp_vref_config {
	struct regulator_common_config common;
	VREF_Type *base;
	uint16_t buf_start_delay;
	uint16_t bg_start_time;
};

static int regulator_nxp_vref_enable(const struct device *dev)
{
	const struct regulator_nxp_vref_config *config = dev->config;
	VREF_Type *const base = config->base;

	volatile uint32_t *const csr = &base->CSR;

	*csr |= VREF_CSR_LPBGEN_MASK | VREF_CSR_LPBG_BUF_EN_MASK;
	/* Wait for bandgap startup */
	k_busy_wait(config->bg_start_time);

	/* Enable high accuracy bandgap */
	*csr |= VREF_CSR_HCBGEN_MASK;

	/* Monitor until stable */
	while (!(*csr & VREF_CSR_VREFST_MASK)) {
		;
	}

	/* Enable output buffer */
	*csr |= VREF_CSR_BUF21EN_MASK;

	return 0;
}

static int regulator_nxp_vref_disable(const struct device *dev)
{
	const struct regulator_nxp_vref_config *config = dev->config;
	VREF_Type *const base = config->base;

	/*
	 * Disable HC Bandgap, LP Bandgap, and Buf21
	 * to achieve "Off" mode of VREF
	 */
	base->CSR &= ~(VREF_CSR_BUF21EN_MASK | VREF_CSR_HCBGEN_MASK | VREF_CSR_LPBGEN_MASK);

	return 0;
}

static int regulator_nxp_vref_set_mode(const struct device *dev, regulator_mode_t mode)
{
	const struct regulator_nxp_vref_config *config = dev->config;
	VREF_Type *const base = config->base;
	uint32_t csr = base->CSR;

	if (mode == NXP_VREF_MODE_STANDBY) {
		csr &= ~VREF_CSR_REGEN_MASK &
			~VREF_CSR_CHOPEN_MASK &
			~VREF_CSR_HI_PWR_LV_MASK &
			~VREF_CSR_BUF21EN_MASK;
	} else if (mode == NXP_VREF_MODE_LOW_POWER) {
		csr &= ~VREF_CSR_REGEN_MASK &
			~VREF_CSR_CHOPEN_MASK &
			~VREF_CSR_HI_PWR_LV_MASK;
		csr |= VREF_CSR_BUF21EN_MASK;
	} else if (mode == NXP_VREF_MODE_HIGH_POWER) {
		csr &= ~VREF_CSR_REGEN_MASK &
			~VREF_CSR_CHOPEN_MASK;
		csr |= VREF_CSR_HI_PWR_LV_MASK &
			VREF_CSR_BUF21EN_MASK;
	} else if (mode == NXP_VREF_MODE_INTERNAL_REGULATOR) {
		csr |= VREF_CSR_REGEN_MASK &
			VREF_CSR_CHOPEN_MASK &
			VREF_CSR_HI_PWR_LV_MASK &
			VREF_CSR_BUF21EN_MASK;
	} else {
		return -EINVAL;
	}

	base->CSR = csr;

	k_busy_wait(config->buf_start_delay);

	return 0;
}

static int regulator_nxp_vref_get_mode(const struct device *dev, regulator_mode_t *mode)
{
	const struct regulator_nxp_vref_config *config = dev->config;
	VREF_Type *const base = config->base;
	uint32_t csr = base->CSR;

	/* Check bits to determine mode */
	if (csr & VREF_CSR_REGEN_MASK) {
		*mode = NXP_VREF_MODE_INTERNAL_REGULATOR;
	} else if (csr & VREF_CSR_HI_PWR_LV_MASK) {
		*mode = NXP_VREF_MODE_HIGH_POWER;
	} else if (csr & VREF_CSR_BUF21EN_MASK) {
		*mode = NXP_VREF_MODE_LOW_POWER;
	} else {
		*mode = NXP_VREF_MODE_STANDBY;
	}

	return 0;
}

static inline unsigned int regulator_nxp_vref_count_voltages(const struct device *dev)
{
	return linear_range_values_count(&utrim_range);
}

static int regulator_nxp_vref_list_voltage(const struct device *dev,
						unsigned int idx, int32_t *volt_uv)
{
	return linear_range_get_value(&utrim_range, idx, volt_uv);
}

static int regulator_nxp_vref_set_voltage(const struct device *dev,
					int32_t min_uv, int32_t max_uv)
{
	const struct regulator_nxp_vref_config *config = dev->config;
	VREF_Type *const base = config->base;
	uint16_t idx;
	int ret;

	ret = linear_range_get_win_index(&utrim_range, min_uv, max_uv, &idx);
	if (ret < 0) {
		return ret;
	}

	base->UTRIM &= ~VREF_UTRIM_TRIM2V1_MASK;
	base->UTRIM |= VREF_UTRIM_TRIM2V1_MASK & idx;

	return 0;
}

static int regulator_nxp_vref_get_voltage(const struct device *dev,
						int32_t *volt_uv)
{
	const struct regulator_nxp_vref_config *config = dev->config;
	VREF_Type *const base = config->base;
	uint16_t idx;
	int ret;

	/* Linear range index is the register value */
	idx = (base->UTRIM & VREF_UTRIM_TRIM2V1_MASK) >> VREF_UTRIM_TRIM2V1_SHIFT;

	ret = linear_range_get_value(&utrim_range, idx, volt_uv);

	return ret;
}

static const struct regulator_driver_api api = {
	.enable = regulator_nxp_vref_enable,
	.disable = regulator_nxp_vref_disable,
	.set_mode = regulator_nxp_vref_set_mode,
	.get_mode = regulator_nxp_vref_get_mode,
	.set_voltage = regulator_nxp_vref_set_voltage,
	.get_voltage = regulator_nxp_vref_get_voltage,
	.list_voltage = regulator_nxp_vref_list_voltage,
	.count_voltages = regulator_nxp_vref_count_voltages,
};

static int regulator_nxp_vref_init(const struct device *dev)
{
	int ret;

	regulator_common_data_init(dev);

	ret = regulator_nxp_vref_disable(dev);
	if (ret < 0) {
		return ret;
	}

	return regulator_common_init(dev, false);
}

#define REGULATOR_NXP_VREF_DEFINE(inst)						\
	static struct regulator_nxp_vref_data data_##inst;			\
										\
	static const struct regulator_nxp_vref_config config_##inst = {		\
		.common = REGULATOR_DT_INST_COMMON_CONFIG_INIT(inst),		\
		.base = (VREF_Type *) DT_INST_REG_ADDR(inst),			\
		.buf_start_delay = DT_INST_PROP(inst,				\
				nxp_buffer_startup_delay_us),			\
		.bg_start_time = DT_INST_PROP(inst,				\
				nxp_bandgap_startup_time_us),			\
	};									\
										\
	DEVICE_DT_INST_DEFINE(inst, regulator_nxp_vref_init, NULL, &data_##inst,\
				&config_##inst, POST_KERNEL,			\
				CONFIG_REGULATOR_NXP_VREF_INIT_PRIORITY, &api);	\

DT_INST_FOREACH_STATUS_OKAY(REGULATOR_NXP_VREF_DEFINE)
