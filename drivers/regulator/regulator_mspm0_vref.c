/*
 * Copyright 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_mspm0_vref

#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/clock/mspm0_clock.h>
#include <zephyr/dt-bindings/regulator/mspm0_vref.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(vref, CONFIG_LOG_DEFAULT_LEVEL);

/* vref output voltages in microvolts */
#define VREF_1_4V 1400000
#define VREF_2_5V 2500000

struct regulator_mspm0_vref_gprcm {
	volatile uint32_t pwren;      /**< Power Enable register, offset: 0x00 */
	volatile uint32_t rstctl;     /**< Reset Control register, offset: 0x04 */
	uint8_t RESERVED_1[0xC];      /**< Reserved, offset: 0x08 - 0x14 */
	volatile const uint32_t stat; /**< Status register, offset: 0x14 */
};

struct regulator_mspm0_vref_regs {
	uint8_t RESERVED_1[0x800];                        /**< Reserved, offset: 0x000 - 0x800 */
	volatile struct regulator_mspm0_vref_gprcm gprcm; /**< Power/reset control, offset: 0x800 */
	uint8_t RESERVED_2[0x7E8];                        /**< Reserved, offset: 0x818 - 0x1000 */
	volatile uint32_t clkdiv; /**< Clock Divider register, offset: 0x1000 */
	uint8_t RESERVED_3[0x4];  /**< Reserved, offset: 0x1004 - 0x1008 */
	volatile uint32_t clksel; /**< Clock Selection register, offset: 0x1008 */
	uint8_t RESERVED_4[0xF4]; /**< Reserved, offset: 0x100C - 0x1100 */
	volatile uint32_t ctl0;   /**< Control 0 register, offset: 0x1100 */
	volatile uint32_t ctl1;   /**< Control 1 register, offset: 0x1104 */
	volatile uint32_t ctl2;   /**< Control 2 register, offset: 0x1108 */
	volatile uint32_t v2ien;  /**< V2I Enable register, offset: 0x110C */
};

/* pwren bits */
#define VREF_PWREN_ENABLE     BIT(0)
#define VREF_PWREN_KEY        GENMASK(31, 24)
#define VREF_PWREN_KEY_UNLOCK 0x26U

/* rstctl bits */
#define VREF_RSTCTL_RESETASSERT  BIT(0)
#define VREF_RSTCTL_RESETSTKYCLR BIT(1)
#define VREF_RSTCTL_KEY          GENMASK(31, 24)
#define VREF_RSTCTL_KEY_UNLOCK   0xB1U

/* stat bits */
#define VREF_STAT_RESETSTKY BIT(16)

/* clkdiv bits */
#define VREF_CLKDIV_RATIO        GENMASK(2, 0)
#define VREF_CLKDIV_RATIO_VAL(x) ((x) - 1)

/* clksel bits */
#define VREF_CLKSEL_LFCLK_SEL  BIT(1)
#define VREF_CLKSEL_MFCLK_SEL  BIT(2)
#define VREF_CLKSEL_BUSCLK_SEL BIT(3)

/* ctl0 bits */
#define VREF_CTL0_ENABLE           BIT(0)
#define VREF_CTL0_COMP_VREF_ENABLE BIT(1)
#define VREF_CTL0_BUFCONFIG        BIT(7)
#define VREF_CTL0_BUFCONFIG_2_5V   0U
#define VREF_CTL0_BUFCONFIG_1_4V   VREF_CTL0_BUFCONFIG
#define VREF_CTL0_SHMODE           BIT(8)

/* ctl1 bits */
#define VREF_CTL1_READY BIT(0)

/* ctl2 bits */
#define VREF_CTL2_SHCYCLE GENMASK(15, 0)
#define VREF_CTL2_HCYCLE  GENMASK(31, 16)

/* v2ien bits */
#define VREF_V2IEN_V2I_EN BIT(0)

struct regulator_mspm0_vref_data {
	struct regulator_common_data common;
	uint32_t buf_config;
	bool sh_mode_enable;
	uint16_t sample_cycle_count;
	uint16_t hold_cycle_count;
};

struct regulator_mspm0_vref_config {
	struct regulator_common_config common;
	const struct pinctrl_dev_config *vref_pin;
	struct regulator_mspm0_vref_regs *regs;
	uint32_t clock_sel;
	uint32_t clock_div;
};

static void regulator_mspm0_vref_configure(const struct regulator_mspm0_vref_config *config,
					   struct regulator_mspm0_vref_data *data)
{
	config->regs->ctl0 = data->buf_config | (data->sh_mode_enable ? VREF_CTL0_SHMODE : 0);
	config->regs->ctl2 =
		FIELD_PREP(VREF_CTL2_SHCYCLE, data->sample_cycle_count + data->hold_cycle_count) |
		FIELD_PREP(VREF_CTL2_HCYCLE, data->hold_cycle_count);
}

static int regulator_mspm0_vref_enable(const struct device *dev)
{
	const struct regulator_mspm0_vref_config *config = dev->config;

	config->regs->ctl0 |= VREF_CTL0_ENABLE;

	return 0;
}

static int regulator_mspm0_vref_disable(const struct device *dev)
{
	const struct regulator_mspm0_vref_config *config = dev->config;

	config->regs->ctl0 &= ~VREF_CTL0_ENABLE;

	return 0;
}

static int regulator_mspm0_vref_get_voltage(const struct device *dev, int32_t *volt_uv)
{
	const struct regulator_mspm0_vref_config *config = dev->config;
#ifdef CONFIG_REGULATOR_THREAD_SAFE_REFCNT
	struct regulator_mspm0_vref_data *data = dev->data;
#endif

	if (volt_uv == NULL) {
		return -EINVAL;
	}

#ifdef CONFIG_REGULATOR_THREAD_SAFE_REFCNT
	k_mutex_lock(&data->common.lock, K_FOREVER);
#endif

	if (config->regs->ctl0 & VREF_CTL0_BUFCONFIG) {
		*volt_uv = VREF_1_4V;
	} else {
		*volt_uv = VREF_2_5V;
	}

#ifdef CONFIG_REGULATOR_THREAD_SAFE_REFCNT
	k_mutex_unlock(&data->common.lock);
#endif

	return 0;
}

static int regulator_mspm0_vref_set_voltage(const struct device *dev, int32_t min_uv,
					    int32_t max_uv)
{
	const struct regulator_mspm0_vref_config *config = dev->config;
	struct regulator_mspm0_vref_data *data = dev->data;
	int32_t volt_set, volt_get;
	int ret = 0;

	if (min_uv <= VREF_2_5V && max_uv >= VREF_2_5V) {
		volt_set = VREF_2_5V;
	} else if (min_uv <= VREF_1_4V && max_uv >= VREF_1_4V) {
		volt_set = VREF_1_4V;
	} else {
		LOG_ERR("Invalid voltage range !!");
		return -EINVAL;
	}

#ifdef CONFIG_REGULATOR_THREAD_SAFE_REFCNT
	k_mutex_lock(&data->common.lock, K_FOREVER);
#endif

	if (data->common.refcnt != 0) {
		volt_get = (config->regs->ctl0 & VREF_CTL0_BUFCONFIG) ? VREF_1_4V : VREF_2_5V;
		if (volt_set != volt_get) {
			ret = -EBUSY;
			goto out;
		}
	} else {
		data->buf_config = (volt_set == VREF_2_5V) ? VREF_CTL0_BUFCONFIG_2_5V
							   : VREF_CTL0_BUFCONFIG_1_4V;
		regulator_mspm0_vref_configure(config, data);
	}
out:

#ifdef CONFIG_REGULATOR_THREAD_SAFE_REFCNT
	k_mutex_unlock(&data->common.lock);
#endif

	return ret;
}

static int regulator_mspm0_vref_get_mode(const struct device *dev, regulator_mode_t *mode)
{
	const struct regulator_mspm0_vref_config *config = dev->config;
#ifdef CONFIG_REGULATOR_THREAD_SAFE_REFCNT
	struct regulator_mspm0_vref_data *data = dev->data;
#endif

	if (mode == NULL) {
		return -EINVAL;
	}

#ifdef CONFIG_REGULATOR_THREAD_SAFE_REFCNT
	k_mutex_lock(&data->common.lock, K_FOREVER);
#endif

	if (config->regs->ctl0 & VREF_CTL0_SHMODE) {
		*mode = MSPM0_VREF_MODE_SHMODE;
	} else {
		*mode = MSPM0_VREF_MODE_NORMAL;
	}

#ifdef CONFIG_REGULATOR_THREAD_SAFE_REFCNT
	k_mutex_unlock(&data->common.lock);
#endif

	return 0;
}

static int regulator_mspm0_vref_set_mode(const struct device *dev, regulator_mode_t mode)
{
	const struct regulator_mspm0_vref_config *config = dev->config;
	struct regulator_mspm0_vref_data *data = dev->data;
	regulator_mode_t mode_get;
	int ret = 0;

#ifdef CONFIG_REGULATOR_THREAD_SAFE_REFCNT
	k_mutex_lock(&data->common.lock, K_FOREVER);
#endif

	if (data->common.refcnt != 0) {
		mode_get = (config->regs->ctl0 & VREF_CTL0_SHMODE) ? MSPM0_VREF_MODE_SHMODE
								   : MSPM0_VREF_MODE_NORMAL;
		if (mode_get != mode) {
			ret = -EBUSY;
			goto out;
		}
	} else {
		switch (mode) {
		case MSPM0_VREF_MODE_SHMODE:
			data->sh_mode_enable = true;
			break;
		case MSPM0_VREF_MODE_NORMAL:
			data->sh_mode_enable = false;
			break;
		default:
			ret = -EINVAL;
			goto out;
		}
		regulator_mspm0_vref_configure(config, data);
	}
out:

#ifdef CONFIG_REGULATOR_THREAD_SAFE_REFCNT
	k_mutex_unlock(&data->common.lock);
#endif

	return ret;
}

static int regulator_mspm0_vref_init(const struct device *dev)
{
	const struct regulator_mspm0_vref_config *config = dev->config;
	struct regulator_mspm0_vref_data *data = dev->data;
	int ret;

	regulator_common_data_init(dev);

	ret = pinctrl_apply_state(config->vref_pin, PINCTRL_STATE_DEFAULT);
	if (ret) {
		LOG_ERR("Pinctrl apply state Failed");
		return ret;
	}

	/* Enable power */
	config->regs->gprcm.pwren =
		FIELD_PREP(VREF_PWREN_KEY, VREF_PWREN_KEY_UNLOCK) | VREF_PWREN_ENABLE;
	k_busy_wait(k_cyc_to_us_ceil32(CONFIG_MSPM0_PERIPH_STARTUP_DELAY));
	regulator_mspm0_vref_configure(config, data);
	config->regs->clksel = config->clock_sel;
	config->regs->clkdiv =
		FIELD_PREP(VREF_CLKDIV_RATIO, VREF_CLKDIV_RATIO_VAL(config->clock_div));

	ret = regulator_common_init(dev, false);
	if (ret) {
		LOG_ERR("Regulator common init Failed");
		return ret;
	}

	return 0;
}

static DEVICE_API(regulator, mspm0_vref_api) = {
	.enable = regulator_mspm0_vref_enable,
	.disable = regulator_mspm0_vref_disable,
	.set_voltage = regulator_mspm0_vref_set_voltage,
	.get_voltage = regulator_mspm0_vref_get_voltage,
	.set_mode = regulator_mspm0_vref_set_mode,
	.get_mode = regulator_mspm0_vref_get_mode,
};

#define REGULATOR_MSPM0_VREF_DEFINE(n)                                                             \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static struct regulator_mspm0_vref_data data_##n = {                                       \
		.buf_config =                                                                      \
			(DT_INST_PROP(n, regulator_uv) == VREF_1_4V ? VREF_CTL0_BUFCONFIG_1_4V     \
								    : VREF_CTL0_BUFCONFIG_2_5V),   \
		.sh_mode_enable = DT_INST_PROP(n, ti_sample_hold_enable),                          \
		.sample_cycle_count = DT_INST_PROP(n, ti_sample_cycles),                           \
		.hold_cycle_count = DT_INST_PROP(n, ti_hold_cycles),                               \
	};                                                                                         \
                                                                                                   \
	static const struct regulator_mspm0_vref_config config_##n = {                             \
		.common = REGULATOR_DT_INST_COMMON_CONFIG_INIT(n),                                 \
		.vref_pin = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                     \
		.regs = (struct regulator_mspm0_vref_regs *)DT_INST_REG_ADDR(n),                   \
		.clock_sel = MSPM0_CLOCK_PERIPH_REG_MASK(DT_INST_CLOCKS_CELL(n, clk)),             \
		.clock_div = DT_INST_PROP(n, ti_clk_div),                                          \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, regulator_mspm0_vref_init, NULL, &data_##n, &config_##n,          \
			      POST_KERNEL, CONFIG_REGULATOR_MSPM0_VREF_INIT_PRIORITY,              \
			      &mspm0_vref_api);

DT_INST_FOREACH_STATUS_OKAY(REGULATOR_MSPM0_VREF_DEFINE)
