/*
 * Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_mspm0_opamp

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/drivers/opamp.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/regulator.h>

/* OPA register offsets */
#define MSPM0_OA_PWREN_OFFSET   0x0800U
#define MSPM0_OA_CTL_OFFSET     0x1100U
#define MSPM0_OA_CFGBASE_OFFSET 0x1104U
#define MSPM0_OA_CFG_OFFSET     0x1108U
#define MSPM0_OA_STAT_OFFSET    0x1118U

/* OPA power control register */
#define MSPM0_OA_PWREN_KEY_UNLOCK 0x26000000U
#define MSPM0_OA_PWREN_ENABLE     BIT(0)

/* OPA enable control */
#define MSPM0_OA_CTL_ENABLE_ON  BIT(0)
#define MSPM0_OA_CTL_ENABLE_OFF 0U

/* OPA configuration base: gain-bandwidth setting */
#define MSPM0_OA_CFGBASE_GBW_MASK BIT(0)

/* OPA configuration fields */
#define MSPM0_OA_CFG_CHOP_MASK   GENMASK(1, 0)
#define MSPM0_OA_CFG_OUTPIN_MASK BIT(2)
#define MSPM0_OA_CFG_PSEL_MASK   GENMASK(6, 3)
#define MSPM0_OA_CFG_NSEL_MASK   GENMASK(9, 7)
#define MSPM0_OA_CFG_MSEL_MASK   GENMASK(12, 10)
#define MSPM0_OA_CFG_GAIN_MASK   GENMASK(15, 13)

/* OPA status: ready indication */
#define MSPM0_OA_STAT_RDY_MASK BIT(0)

#define MSPM0_OA_READY_TIMEOUT_US 20U

struct mspm0_opamp_config {
	mm_reg_t base;
	const struct pinctrl_dev_config *pinctrl;
	const struct device *vref;
	uint8_t psel;
	uint8_t nsel;
	uint8_t msel;
	uint8_t chop_mode;
	uint8_t gain;
	uint8_t gbw;
	bool outpin_enabled;
};

struct mspm0_opamp_data {
	struct k_mutex lock;
};

static int mspm0_opamp_set_gain(const struct device *dev, enum opamp_gain gain)
{
	const struct mspm0_opamp_config *config = dev->config;
	struct mspm0_opamp_data *data = dev->data;
	uint8_t gain_bits;
	uint32_t temp;

	switch (gain) {
	case OPAMP_GAIN_1:
	case OPAMP_GAIN_2:
		gain_bits = 1U;
		break;

	case OPAMP_GAIN_3:
	case OPAMP_GAIN_4:
		gain_bits = 2U;
		break;

	case OPAMP_GAIN_7:
	case OPAMP_GAIN_8:
		gain_bits = 3U;
		break;

	case OPAMP_GAIN_15:
	case OPAMP_GAIN_16:
		gain_bits = 4U;
		break;

	case OPAMP_GAIN_31:
	case OPAMP_GAIN_32:
		gain_bits = 5U;
		break;

	default:
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	temp = sys_read32(config->base + MSPM0_OA_CFG_OFFSET) & ~MSPM0_OA_CFG_GAIN_MASK;
	temp |= FIELD_PREP(MSPM0_OA_CFG_GAIN_MASK, gain_bits);

	sys_write32(temp, config->base + MSPM0_OA_CFG_OFFSET);

	k_mutex_unlock(&data->lock);

	return 0;
}

static DEVICE_API(opamp, mspm0_opamp_api) = {
	.set_gain = mspm0_opamp_set_gain,
};

static int mspm0_opamp_init(const struct device *dev)
{
	const struct mspm0_opamp_config *config = dev->config;
	struct mspm0_opamp_data *data = dev->data;
	int ret;
	uint32_t temp;

	k_mutex_init(&data->lock);

	if (config->pinctrl) {
		ret = pinctrl_apply_state(config->pinctrl, PINCTRL_STATE_DEFAULT);
		if (ret < 0) {
			return ret;
		}
	}

	if (config->vref) {
		ret = regulator_enable(config->vref);
		if (ret < 0) {
			return ret;
		}
	}

	sys_write32(MSPM0_OA_PWREN_KEY_UNLOCK | MSPM0_OA_PWREN_ENABLE,
		    config->base + MSPM0_OA_PWREN_OFFSET);

	sys_write32(MSPM0_OA_CTL_ENABLE_OFF, config->base + MSPM0_OA_CTL_OFFSET);

	sys_write32(FIELD_PREP(MSPM0_OA_CFG_CHOP_MASK, config->chop_mode) |
			    FIELD_PREP(MSPM0_OA_CFG_OUTPIN_MASK, config->outpin_enabled) |
			    FIELD_PREP(MSPM0_OA_CFG_PSEL_MASK, config->psel) |
			    FIELD_PREP(MSPM0_OA_CFG_NSEL_MASK, config->nsel) |
			    FIELD_PREP(MSPM0_OA_CFG_MSEL_MASK, config->msel) |
			    FIELD_PREP(MSPM0_OA_CFG_GAIN_MASK, config->gain),
		    config->base + MSPM0_OA_CFG_OFFSET);

	sys_write32(MSPM0_OA_CTL_ENABLE_ON, config->base + MSPM0_OA_CTL_OFFSET);

	if (!WAIT_FOR(sys_read32(config->base + MSPM0_OA_STAT_OFFSET) & MSPM0_OA_STAT_RDY_MASK,
		      MSPM0_OA_READY_TIMEOUT_US, k_busy_wait(1))) {
		return -ETIMEDOUT;
	}

	temp = sys_read32(config->base + MSPM0_OA_CFGBASE_OFFSET) & ~MSPM0_OA_CFGBASE_GBW_MASK;
	temp |= FIELD_PREP(MSPM0_OA_CFGBASE_GBW_MASK, config->gbw);

	sys_write32(temp, config->base + MSPM0_OA_CFGBASE_OFFSET);

	return 0;
}

#define MSPM0_OPA_PINCTRL_DT_INST_DEFINE(inst)                                                     \
	COND_CODE_1(						\
		DT_INST_PINCTRL_HAS_NAME(inst, default),	\
		(PINCTRL_DT_INST_DEFINE(inst)),			\
		())

#define MSPM0_OPA_PINCTRL_DT_INST_GET(inst)                                                        \
	COND_CODE_1(						\
		DT_INST_PINCTRL_HAS_NAME(inst, default),	\
		(PINCTRL_DT_INST_DEV_CONFIG_GET(inst)),		\
		(NULL))

#define MSPM0_OPAMP_INIT(inst)                                                                     \
	MSPM0_OPA_PINCTRL_DT_INST_DEFINE(inst);                                                    \
	static const struct mspm0_opamp_config mspm0_opamp_config_##inst = {                       \
		.base = DT_INST_REG_ADDR(inst),                                                    \
		.pinctrl = MSPM0_OPA_PINCTRL_DT_INST_GET(inst),                                    \
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, vref),	\
			(.vref = DEVICE_DT_GET(DT_PHANDLE(DT_DRV_INST(inst), vref)),), \
			(.vref = NULL,)) .psel = DT_INST_ENUM_IDX(inst, ti_psel),                  \
			 .nsel = DT_INST_ENUM_IDX(inst, ti_nsel),                                  \
			 .msel = DT_INST_ENUM_IDX(inst, ti_msel),                                  \
			 .chop_mode = DT_INST_ENUM_IDX(inst, ti_chop_mode),                        \
			 .gain = DT_INST_PROP(inst, ti_gain),                                      \
			 .gbw = DT_INST_ENUM_IDX(inst, ti_power_mode),                             \
			 .outpin_enabled = DT_INST_PROP(inst, ti_outpin_enable),                   \
	};                                                                                         \
	static struct mspm0_opamp_data mspm0_opamp_data_##inst;                                    \
	DEVICE_DT_INST_DEFINE(inst, mspm0_opamp_init, NULL, &mspm0_opamp_data_##inst,              \
			      &mspm0_opamp_config_##inst, POST_KERNEL, CONFIG_OPAMP_INIT_PRIORITY, \
			      &mspm0_opamp_api);

DT_INST_FOREACH_STATUS_OKAY(MSPM0_OPAMP_INIT)
