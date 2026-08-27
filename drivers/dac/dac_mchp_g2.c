/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/device.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/mchp_clock_control.h>
#include <soc.h>

#define DT_DRV_COMPAT microchip_dac_g2
LOG_MODULE_REGISTER(dac_mchp_g2, CONFIG_DAC_LOG_LEVEL);

#define DAC_RESOLUTION      10
#define DAC_10BIT_DATA_MASK (0x03FFU)
#define TIMEOUT_VALUE_US    1000
#define DELAY_US            2

struct dac_mchp_clock {
	const struct device *clock_dev;
	clock_control_subsys_t mclk_sys;
	clock_control_subsys_t gclk_sys;
};

struct dac_mchp_dev_config {
	dac_registers_t *regs;
	uint8_t refsel;
	bool external_pin_en;
	bool run_in_standby;
	bool volt_pump_disable;
	uint8_t max_channels;
	const struct pinctrl_dev_config *pcfg;
	struct dac_mchp_clock dac_clock;
};

static inline void dac_wait_sync(dac_registers_t *dac_reg, uint32_t sync_flag)
{
	if (WAIT_FOR(((dac_reg->DAC_SYNCBUSY & sync_flag) == 0U), TIMEOUT_VALUE_US,
		     k_busy_wait(DELAY_US)) == false) {
		LOG_ERR("Timeout waiting for DAC_SYNCBUSY bits to clear");
	}
}

static int dac_mchp_channel_setup(const struct device *dev,
				  const struct dac_channel_cfg *channel_cfg)
{
	const struct dac_mchp_dev_config *dev_cfg = dev->config;
	dac_registers_t *dac_reg = dev_cfg->regs;

	if ((channel_cfg->resolution != DAC_RESOLUTION) || (channel_cfg->buffered) ||
	    (channel_cfg->internal)) {
		LOG_ERR("Unsupported configuration!");
		return -ENOTSUP;
	}

	/* Channel ID validity */
	if (channel_cfg->channel_id >= dev_cfg->max_channels) {
		LOG_ERR("Invalid Channel!");
		return -EINVAL;
	}

	/* Enable the DAC */
	dac_reg->DAC_CTRLA |= DAC_CTRLA_ENABLE_Msk;
	dac_wait_sync(dac_reg, DAC_SYNCBUSY_ENABLE_Msk);

	/* Wait for ready state */
	if (WAIT_FOR(((dac_reg->DAC_STATUS & DAC_STATUS_READY_Msk) == DAC_STATUS_READY_Msk),
		     TIMEOUT_VALUE_US, k_busy_wait(DELAY_US)) == false) {
		LOG_ERR("Timeout waiting for DAC_STATUS_READY (DAC_STATUS_READY_Msk=0x%x)",
			DAC_STATUS_READY_Msk);
		return -ETIMEDOUT;
	}

	return 0;
}

static int dac_mchp_write_value(const struct device *dev, uint8_t channel, uint32_t value)
{
	/* DAC consists of Single Channel */
	ARG_UNUSED(channel);

	const struct dac_mchp_dev_config *dev_cfg = dev->config;

	dev_cfg->regs->DAC_DATA = (uint16_t)(DAC_10BIT_DATA_MASK & DAC_DATA_DATA(value));
	dac_wait_sync(dev_cfg->regs, DAC_SYNCBUSY_DATA_Msk);

	return 0;
}

static int dac_mchp_init(const struct device *dev)
{
	const struct dac_mchp_dev_config *dev_cfg = dev->config;
	dac_registers_t *dac_reg = dev_cfg->regs;
	uint8_t regval;
	int ret;

	/* Enable GCLK */
	ret = clock_control_on(dev_cfg->dac_clock.clock_dev, dev_cfg->dac_clock.gclk_sys);
	if ((ret != 0) && (ret != -EALREADY)) {
		LOG_ERR("Failed to enable the GCLK for DAC: %d", ret);
		return ret;
	}

	/* Enable MCLK */
	ret = clock_control_on(dev_cfg->dac_clock.clock_dev, dev_cfg->dac_clock.mclk_sys);
	if ((ret != 0) && (ret != -EALREADY)) {
		LOG_ERR("Failed to enable the MCLK for DAC: %d", ret);
		return ret;
	}

	pinctrl_apply_state(dev_cfg->pcfg, PINCTRL_STATE_DEFAULT);

	/* DAC Reset */
	dac_reg->DAC_CTRLA = DAC_CTRLA_SWRST_Msk;
	dac_wait_sync(dac_reg, DAC_SYNCBUSY_SWRST_Msk);

	regval = dac_reg->DAC_CTRLB & ~DAC_CTRLB_REFSEL_Msk;

	/* Config DAC External pin */
	regval = (dev_cfg->external_pin_en) ? (regval | DAC_CTRLB_EOEN_Msk) : regval;

	/* Disable DAC Voltage Pump */
	regval = (dev_cfg->volt_pump_disable) ? (regval | DAC_CTRLB_VPD_Msk) : regval;

	/* DAC Reference voltage selection */
	dac_reg->DAC_CTRLB = regval | DAC_CTRLB_REFSEL(dev_cfg->refsel);

	/* Enable DAC RUN IN STANDBY Mode */
	if (dev_cfg->run_in_standby) {
		dac_reg->DAC_CTRLA |= DAC_CTRLA_RUNSTDBY_Msk;
	}

	return 0;
}

static DEVICE_API(dac, dac_mchp_api) = {.channel_setup = dac_mchp_channel_setup,
					.write_value = dac_mchp_write_value};

/* clang-format off */
#define DAC_MCHP_CONFIG_DEFN(n)                                                                    \
	static const struct dac_mchp_dev_config dac_mchp_config_##n = {                            \
		.regs = (dac_registers_t *)DT_INST_REG_ADDR(n),                                    \
		.refsel = DT_ENUM_IDX(DT_DRV_INST(n), refsel),                                     \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.external_pin_en = DT_INST_PROP(n, external_pin_en),                               \
		.run_in_standby = DT_INST_PROP(n, run_in_standby),                                 \
		.volt_pump_disable = DT_INST_PROP(n, volt_pump_disable),                           \
		.max_channels = DT_INST_PROP(n, max_channels),                                     \
		.dac_clock.clock_dev = DEVICE_DT_GET(DT_NODELABEL(clock)),                         \
		.dac_clock.mclk_sys = (void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, mclk, subsystem)),   \
		.dac_clock.gclk_sys = (void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, gclk, subsystem)),   \
	}
/* clang-format on */

#define DAC_MCHP_DEVICE_INIT(n)                                                                    \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	DAC_MCHP_CONFIG_DEFN(n);                                                                   \
	DEVICE_DT_INST_DEFINE(n, dac_mchp_init, NULL, NULL, &dac_mchp_config_##n, POST_KERNEL,     \
			      CONFIG_DAC_INIT_PRIORITY, &dac_mchp_api);

DT_INST_FOREACH_STATUS_OKAY(DAC_MCHP_DEVICE_INIT)
