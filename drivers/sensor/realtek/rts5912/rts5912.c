/*
 * Copyright (c) 2025 Realtek, SIBG-SD7
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT realtek_rts5912_tach

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_rts5912.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/logging/log.h>

#include "reg/reg_tacho.h"

LOG_MODULE_REGISTER(tach_rts5912, CONFIG_SENSOR_LOG_LEVEL);

struct tach_rts5912_config {
	volatile struct tacho_regs *regs;
	uint32_t clk_grp;
	uint32_t clk_idx;
	uint32_t clk_src;
	uint32_t clk_div;
	const struct device *clk_dev;
	const struct pinctrl_dev_config *pcfg;
	int pulses_per_round;
};

struct tach_rts5912_data {
	uint16_t count;
};

#define COUNT_100KHZ_SEC  100000U
#define SEC_TO_MINUTE     60U
#define PIN_STUCK_TIMEOUT (100U * USEC_PER_MSEC)

int tach_rts5912_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct tach_rts5912_config *const cfg = dev->config;
	struct tach_rts5912_data *const data = dev->data;
	volatile struct tacho_regs *const tach = cfg->regs;

	if (chan != SENSOR_CHAN_RPM && chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	tach->status = TACHO_STS_CNTRDY;

	if (WAIT_FOR(tach->status & TACHO_STS_CNTRDY, PIN_STUCK_TIMEOUT, k_msleep(1))) {
		/* See whether internal counter is already latched */
		if (tach->status & TACHO_STS_CNTRDY) {
			tach->status = TACHO_STS_CNTRDY;
			data->count = (tach->ctrl & TACHO_CTRL_CNT_Msk) >> TACHO_CTRL_CNT_Pos;
		}
	} else {
		data->count = 0;
	}

	return 0;
}

static int tach_rts5912_channel_get(const struct device *dev, enum sensor_channel chan,
				    struct sensor_value *val)
{
	const struct tach_rts5912_config *const cfg = dev->config;
	struct tach_rts5912_data *const data = dev->data;

	if (chan != SENSOR_CHAN_RPM) {
		return -ENOTSUP;
	}

	/* Convert the count per 100khz cycles to rpm */
	if (data->count != 0U) {
		val->val1 =
			(SEC_TO_MINUTE * COUNT_100KHZ_SEC) / (cfg->pulses_per_round * data->count);
	} else {
		val->val1 = 0U;
	}

	val->val2 = 0U;

	return 0;
}

static int tach_rts5912_init(const struct device *dev)
{
	const struct tach_rts5912_config *const cfg = dev->config;
	volatile struct tacho_regs *const tach = cfg->regs;
	int ret;

	struct rts5912_sccon_subsys sccon_subsys = {
		.clk_grp = cfg->clk_grp,
		.clk_idx = cfg->clk_idx,
	};

	if (!device_is_ready(cfg->clk_dev)) {
		return -ENODEV;
	}

	ret = clock_control_on(cfg->clk_dev, (clock_control_subsys_t)&sccon_subsys);
	if (ret != 0) {
		LOG_ERR("RTS5912 Tachometer clock control failed (%d)", ret);
		return ret;
	}

#ifdef CONFIG_PINCTRL
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);

	if (ret != 0) {
		LOG_ERR("RTS5912 pinctrl failed (%d)", ret);
		return ret;
	}
#endif

	/* write one clear the status */
	tach->status = TACHO_STS_LIMIT | TACHO_STS_CHG | TACHO_STS_CNTRDY;
	tach->ctrl &= ~TACHO_CTRL_SELEDGE_Msk;
	tach->ctrl = ((0x01ul << TACHO_CTRL_SELEDGE_Pos) | TACHO_CTRL_READMODE | TACHO_CTRL_EN);

	return 0;
}

static DEVICE_API(sensor, tach_rts5912_driver_api) = {
	.sample_fetch = tach_rts5912_sample_fetch,
	.channel_get = tach_rts5912_channel_get,
};

#define REALTEK_TACH_INIT(inst)                                                                    \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
                                                                                                   \
	static const struct tach_rts5912_config tach_cfg_##inst = {                                \
		.regs = (struct tacho_regs *const)DT_INST_REG_ADDR(inst),                          \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
		.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                               \
		.clk_grp = DT_INST_CLOCKS_CELL_BY_NAME(inst, tacho, clk_grp),                      \
		.clk_idx = DT_INST_CLOCKS_CELL_BY_NAME(inst, tacho, clk_idx),                      \
		.pulses_per_round = DT_INST_PROP(inst, pulses_per_round),                          \
	};                                                                                         \
                                                                                                   \
	static struct tach_rts5912_data tach_data_##inst;                                          \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, tach_rts5912_init, NULL, &tach_data_##inst,             \
				     &tach_cfg_##inst, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,   \
				     &tach_rts5912_driver_api);

DT_INST_FOREACH_STATUS_OKAY(REALTEK_TACH_INIT)
