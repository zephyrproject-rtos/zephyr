/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2022 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_tach

#include <errno.h>
#include <soc.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/dt-bindings/interrupt-controller/mchp-xec-ecia.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>

#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>

LOG_MODULE_REGISTER(tach_xec, CONFIG_SENSOR_LOG_LEVEL);

struct tach_xec_config {
	struct tach_regs * const regs;
	uint8_t girq;
	uint8_t girq_pos;
	uint8_t enc_pcr;
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

static int tach_xec_sample_fetch(const struct device *dev, enum sensor_channel chan)
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

	soc_xec_pcr_sleep_en_clear(cfg->enc_pcr);
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

static DEVICE_API(sensor, tach_xec_driver_api) = {
	.sample_fetch = tach_xec_sample_fetch,
	.channel_get = tach_xec_channel_get,
};

#define DEV_CFG_GIRQ(inst)     MCHP_XEC_ECIA_GIRQ(DT_INST_PROP_BY_IDX(inst, girqs, 0))
#define DEV_CFG_GIRQ_POS(inst) MCHP_XEC_ECIA_GIRQ_POS(DT_INST_PROP_BY_IDX(inst, girqs, 0))

#define XEC_TACH_CONFIG(inst)						\
	static const struct tach_xec_config tach_xec_config_##inst = {	\
		.regs = (struct tach_regs * const)DT_INST_REG_ADDR(inst),	\
		.girq = DEV_CFG_GIRQ(inst),		\
		.girq_pos = DEV_CFG_GIRQ_POS(inst),	\
		.enc_pcr = DT_INST_PROP(inst, pcr_scr),             \
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
