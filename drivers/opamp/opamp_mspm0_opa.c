/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_mspm0_opa

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/drivers/opamp.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <soc.h>

#ifdef CONFIG_REGULATOR_MSPM0_VREF
#include <zephyr/drivers/regulator.h>
#endif

/* OPA register offsets */
#define MSPM0_OA_PWREN_OFFSET   0x0800U
#define MSPM0_OA_RSTCTL_OFFSET  0x0804U
#define MSPM0_OA_CTL_OFFSET     0x1100U
#define MSPM0_OA_CFGBASE_OFFSET 0x1104U
#define MSPM0_OA_CFG_OFFSET     0x1108U
#define MSPM0_OA_STAT_OFFSET    0x1118U

/* Reset control */
#define MSPM0_OA_RSTCTL_UNLOCK_KEY       0xb1000000U
#define MSPM0_OA_RSTCTL_CLEAR_RESETSTKY  BIT(1)
#define MSPM0_OA_RSTCTL_ASSERT_RESET     BIT(0)

/* OPA power control register */
#define MSPM0_OA_PWREN_KEY_UNLOCK 0x26000000U
#define MSPM0_OA_PWREN_ENABLE     BIT(0)

/* OPA enable control */
#define MSPM0_OA_CTL_ENABLE_ON  BIT(0)
#define MSPM0_OA_CTL_ENABLE_OFF 0U

/* OPA configuration fields */
#define MSPM0_OA_CFGBASE_GBW_MASK  BIT(0)
#define MSPM0_OA_CFGBASE_RRI_MASK  BIT(2)
#define MSPM0_OA_CFG_CHOP_MASK     GENMASK(1, 0)
#define MSPM0_OA_CFG_OUTPIN_MASK   BIT(2)
#define MSPM0_OA_CFG_PSEL_MASK     GENMASK(6, 3)
#define MSPM0_OA_CFG_NSEL_MASK     GENMASK(9, 7)
#define MSPM0_OA_CFG_MSEL_MASK     GENMASK(12, 10)
#define MSPM0_OA_CFG_GAIN_MASK     GENMASK(15, 13)

/* OPA status: ready indication */
#define MSPM0_OA_STAT_RDY_MASK     BIT(0)

#define MSPM0_OA_READY_TIMEOUT_US     12U
#define MSPM0_OA_READY_POLL_DELAY_US  1U

struct mspm0_opa_config {
	mm_reg_t base;
	const struct pinctrl_dev_config *pinctrl;
#ifdef CONFIG_REGULATOR_MSPM0_VREF
	const struct device *vref;
#endif
	uint16_t cfg;
	uint8_t cfgbase;
};

struct mspm0_opa_data {
	struct k_mutex lock;
};

static int mspm0_opa_set_gain(const struct device *dev, enum opamp_gain gain)
{
	const struct mspm0_opa_config *config = dev->config;
	struct mspm0_opa_data *data = dev->data;
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

	temp = sys_read32(config->base + MSPM0_OA_CFG_OFFSET);
	temp &= ~MSPM0_OA_CFG_GAIN_MASK;
	temp |= FIELD_PREP(MSPM0_OA_CFG_GAIN_MASK, gain_bits);
	sys_write32(temp, config->base + MSPM0_OA_CFG_OFFSET);

	k_mutex_unlock(&data->lock);

	return 0;
}

static DEVICE_API(opamp, mspm0_opa_api) = {
	.set_gain = mspm0_opa_set_gain,
};

static int mspm0_opa_init(const struct device *dev)
{
	const struct mspm0_opa_config *config = dev->config;
	struct mspm0_opa_data *data = dev->data;
	int ret;

	k_mutex_init(&data->lock);

	if (config->pinctrl != NULL) {
		ret = pinctrl_apply_state(config->pinctrl, PINCTRL_STATE_DEFAULT);
		if (ret < 0) {
			return ret;
		}
	}

#ifdef CONFIG_REGULATOR_MSPM0_VREF
	if (config->vref != NULL) {
		ret = regulator_enable(config->vref);
		if (ret < 0) {
			return ret;
		}
	}
#endif

	sys_write32(MSPM0_OA_RSTCTL_UNLOCK_KEY | MSPM0_OA_RSTCTL_CLEAR_RESETSTKY |
			MSPM0_OA_RSTCTL_ASSERT_RESET, config->base + MSPM0_OA_RSTCTL_OFFSET);

	sys_write32(MSPM0_OA_PWREN_KEY_UNLOCK | MSPM0_OA_PWREN_ENABLE,
			config->base + MSPM0_OA_PWREN_OFFSET);
	msp_delay_peripheral_startup();

	sys_write32(MSPM0_OA_CTL_ENABLE_OFF, config->base + MSPM0_OA_CTL_OFFSET);
	msp_delay_peripheral_startup();

	sys_write32(config->cfg, config->base + MSPM0_OA_CFG_OFFSET);

	sys_write32(config->cfgbase, config->base + MSPM0_OA_CFGBASE_OFFSET);

	sys_write32(MSPM0_OA_CTL_ENABLE_ON, config->base + MSPM0_OA_CTL_OFFSET);

	if (!WAIT_FOR(sys_read32(config->base + MSPM0_OA_STAT_OFFSET) & MSPM0_OA_STAT_RDY_MASK,
			MSPM0_OA_READY_TIMEOUT_US, k_busy_wait(MSPM0_OA_READY_POLL_DELAY_US))) {
		return -ETIMEDOUT;
	}
	return 0;
}

#define MSPM0_OPA_PINCTRL_DT_INST_DEFINE(inst)                                      \
	COND_CODE_1(                                                                \
		DT_INST_PINCTRL_HAS_NAME(inst, default),                            \
		(PINCTRL_DT_INST_DEFINE(inst)),                                     \
		())

#define MSPM0_OPA_PINCTRL_DT_INST_GET(inst)                                         \
	COND_CODE_1(                                                                \
		DT_INST_PINCTRL_HAS_NAME(inst, default),                            \
		(PINCTRL_DT_INST_DEV_CONFIG_GET(inst)),                             \
		(NULL))

#define MSPM0_OPA_INIT(inst)                                                        \
	MSPM0_OPA_PINCTRL_DT_INST_DEFINE(inst);                                     \
	static const struct mspm0_opa_config mspm0_opa_config_##inst = {            \
		.base = DT_INST_REG_ADDR(inst),                                     \
		.pinctrl = MSPM0_OPA_PINCTRL_DT_INST_GET(inst),                     \
		IF_ENABLED(CONFIG_REGULATOR_MSPM0_VREF, (                           \
			.vref = COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, vref),      \
				DEVICE_DT_GET(DT_PHANDLE(DT_DRV_INST(inst), vref)), \
				(NULL)),))                                          \
		.cfg = FIELD_PREP(MSPM0_OA_CFG_CHOP_MASK,                           \
				DT_INST_ENUM_IDX(inst, ti_chop_mode)) |	            \
			FIELD_PREP(MSPM0_OA_CFG_OUTPIN_MASK,                        \
				DT_INST_PROP(inst, ti_outpin_enable)) |	            \
			FIELD_PREP(MSPM0_OA_CFG_PSEL_MASK,                          \
				DT_INST_ENUM_IDX(inst, ti_psel)) |                  \
			FIELD_PREP(MSPM0_OA_CFG_NSEL_MASK,                          \
				DT_INST_ENUM_IDX(inst, ti_nsel)) |                  \
			FIELD_PREP(MSPM0_OA_CFG_MSEL_MASK,                          \
				DT_INST_ENUM_IDX(inst, ti_msel)),                   \
		.cfgbase = FIELD_PREP(MSPM0_OA_CFGBASE_GBW_MASK,                    \
				DT_INST_ENUM_IDX(inst, ti_power_mode)) |            \
			FIELD_PREP(MSPM0_OA_CFGBASE_RRI_MASK,	                    \
				DT_INST_PROP(inst, ti_rri_enable))                  \
	};                                                                          \
	static struct mspm0_opa_data mspm0_opa_data_##inst;	                    \
	DEVICE_DT_INST_DEFINE(inst, mspm0_opa_init, NULL,                           \
			      &mspm0_opa_data_##inst,	                            \
			      &mspm0_opa_config_##inst, POST_KERNEL,                \
			      CONFIG_OPAMP_INIT_PRIORITY, &mspm0_opa_api);

DT_INST_FOREACH_STATUS_OKAY(MSPM0_OPA_INIT)
