/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2022 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_tach

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/arch/cpu.h>
#ifdef CONFIG_SOC_SERIES_MEC172X
#include <zephyr/drivers/clock_control/mchp_xec_clock_control.h>
#include <zephyr/drivers/interrupt_controller/intc_mchp_xec_ecia.h>
#endif
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/sensor.h>
#include <soc.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/logging/log.h>

#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>

LOG_MODULE_REGISTER(tach_xec, CONFIG_SENSOR_LOG_LEVEL);

struct tach_xec_config {
	struct tach_regs * const regs;
	uint8_t girq;
	uint8_t girq_pos;
	uint8_t pcr_idx;
	uint8_t pcr_pos;
	const struct pinctrl_dev_config *pcfg;
};

struct tach_xec_data {
	uint32_t control;
	uint16_t count;
};

#define FAN_STOPPED		0xFFFFU
#define COUNT_100KHZ_SEC	100000U
#define SEC_TO_MINUTE		60U
#define PIN_STS_TIMEOUT		20U
#define TACH_CTRL_EDGES		(CONFIG_TACH_XEC_EDGES << \
				 MCHP_TACH_CTRL_NUM_EDGES_POS)

int tach_xec_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	ARG_UNUSED(chan);

	const struct tach_xec_config * const cfg = dev->config;
	struct tach_xec_data * const data = dev->data;
	struct tach_regs * const tach = cfg->regs;
	uint8_t poll_count = 0;

	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);

	while (poll_count < PIN_STS_TIMEOUT) {
		/* See whether internal counter is already latched */
		if (tach->STATUS & MCHP_TACH_STS_CNT_RDY) {
			data->count =
				tach->CONTROL >> MCHP_TACH_CTRL_COUNTER_POS;
			break;
		}

		poll_count++;

		/* Allow other threads to run while we sleep */
		k_usleep(USEC_PER_MSEC);
	}

	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);

	if (poll_count == PIN_STS_TIMEOUT) {
		return -EINVAL;
	}

	/* We interpret a fan stopped or jammed as 0 */
	if (data->count == FAN_STOPPED) {
		data->count = 0U;
	}

	return 0;
}

static int tach_xec_channel_get(const struct device *dev,
				enum sensor_channel chan,
				struct sensor_value *val)
{
	struct tach_xec_data * const data = dev->data;

	if (chan != SENSOR_CHAN_RPM) {
		return -ENOTSUP;
	}

	/* Convert the count per 100khz cycles to rpm */
	if (data->count != FAN_STOPPED && data->count != 0U) {
		val->val1 = (SEC_TO_MINUTE * COUNT_100KHZ_SEC)/data->count;
		val->val2 = 0U;
	} else {
		val->val1 =  0U;
	}

	val->val2 = 0U;

	return 0;
}

static void tach_xec_sleep_clr(const struct device *dev)
{
	const struct tach_xec_config * const cfg = dev->config;
	struct pcr_regs * const pcr = (struct pcr_regs * const)(
		DT_REG_ADDR_BY_IDX(DT_NODELABEL(pcr), 0));

#ifdef CONFIG_SOC_SERIES_MEC172X
	pcr->SLP_EN[cfg->pcr_idx] &= ~BIT(cfg->pcr_pos);
#else
	uintptr_t addr = (uintptr_t)&pcr->SLP_EN0 + (4u * cfg->pcr_idx);
	uint32_t pcr_val = sys_read32(addr) & ~BIT(cfg->pcr_pos);

	sys_write32(pcr_val, addr);
#endif
}

#ifdef CONFIG_PM_DEVICE
static int tach_xec_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct tach_xec_config * const cfg = dev->config;
	struct tach_xec_data * const data = dev->data;
	struct tach_regs * const tach = cfg->regs;
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		if (data->control & MCHP_TACH_CTRL_EN) {
			tach->CONTROL |= MCHP_TACH_CTRL_EN;
			data->control &= (~MCHP_TACH_CTRL_EN);
		}
	break;
	case PM_DEVICE_ACTION_SUSPEND:
		if (tach->CONTROL & MCHP_TACH_CTRL_EN) {
			/* Take a backup */
			data->control = tach->CONTROL;
			tach->CONTROL &= (~MCHP_TACH_CTRL_EN);
		}
	break;
	default:
		ret = -ENOTSUP;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

static int tach_xec_init(const struct device *dev)
{
	const struct tach_xec_config * const cfg = dev->config;
	struct tach_regs * const tach = cfg->regs;

	int ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);

	if (ret != 0) {
		LOG_ERR("XEC TACH pinctrl init failed (%d)", ret);
		return ret;
	}

	tach_xec_sleep_clr(dev);

	tach->CONTROL = MCHP_TACH_CTRL_READ_MODE_100K_CLOCK	|
			TACH_CTRL_EDGES	                        |
			MCHP_TACH_CTRL_FILTER_EN		|
			MCHP_TACH_CTRL_EN;

	return 0;
}

static const struct sensor_driver_api tach_xec_driver_api = {
	.sample_fetch = tach_xec_sample_fetch,
	.channel_get = tach_xec_channel_get,
};

#define XEC_TACH_CONFIG(inst)						\
	static const struct tach_xec_config tach_xec_config_##inst = {	\
		.regs = (struct tach_regs * const)DT_INST_REG_ADDR(inst),	\
		.girq = DT_INST_PROP_BY_IDX(inst, girqs, 0),		\
		.girq_pos = DT_INST_PROP_BY_IDX(inst, girqs, 1),	\
		.pcr_idx = DT_INST_PROP_BY_IDX(inst, pcrs, 0),		\
		.pcr_pos = DT_INST_PROP_BY_IDX(inst, pcrs, 1),		\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),		\
	}

#define TACH_XEC_DEVICE(id)						\
	static struct tach_xec_data tach_xec_data_##id;			\
									\
	PINCTRL_DT_INST_DEFINE(id);					\
									\
	XEC_TACH_CONFIG(id);						\
									\
	PM_DEVICE_DT_INST_DEFINE(id, tach_xec_pm_action);		\
									\
	SENSOR_DEVICE_DT_INST_DEFINE(id,				\
			    tach_xec_init,				\
			    PM_DEVICE_DT_INST_GET(id),			\
			    &tach_xec_data_##id,			\
			    &tach_xec_config_##id,			\
			    POST_KERNEL,				\
			    CONFIG_SENSOR_INIT_PRIORITY,		\
			    &tach_xec_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TACH_XEC_DEVICE)
