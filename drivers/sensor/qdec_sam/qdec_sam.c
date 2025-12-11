/*
 * Copyright (c) 2021, Piotr Mienkowski
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam_tc_qdec

/** @file
 * @brief Atmel SAM MCU family Quadrature Decoder (QDEC/TC) driver.
 */

#include <errno.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/atmel_sam_pmc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(qdec_sam, CONFIG_SENSOR_LOG_LEVEL);

/* Device constant configuration parameters */
struct qdec_sam_dev_cfg {
	Tc *regs;
	const struct atmel_sam_pmc_config clock_cfg[TCCHANNEL_NUMBER];
	const struct pinctrl_dev_config *pcfg;
};

/* Device run time data */
struct qdec_sam_dev_data {
	uint16_t position;
};

static int qdec_sam_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct qdec_sam_dev_cfg *const dev_cfg = dev->config;
	struct qdec_sam_dev_data *const dev_data = dev->data;
	Tc *const tc = dev_cfg->regs;
	TcChannel *tc_ch0 = &tc->TcChannel[0];

	/* Read position register content */
	dev_data->position = tc_ch0->TC_CV;

	return 0;
}

static int qdec_sam_get(const struct device *dev, enum sensor_channel chan,
			struct sensor_value *val)
{
	struct qdec_sam_dev_data *const dev_data = dev->data;

	if (chan == SENSOR_CHAN_ROTATION) {
		val->val1 = dev_data->position;
		val->val2 = 0;
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static void qdec_sam_start(Tc *const tc)
{
	TcChannel *tc_ch0 = &tc->TcChannel[0];

	/* Enable Channel 0 Clock and reset counter*/
	tc_ch0->TC_CCR =  TC_CCR_CLKEN
			| TC_CCR_SWTRG;
}

static void qdec_sam_configure(const struct device *dev)
{
	const struct qdec_sam_dev_cfg *const dev_cfg = dev->config;
	Tc *const tc = dev_cfg->regs;
	TcChannel *tc_ch0 = &tc->TcChannel[0];

	/* Clock, Trigger Edge, Trigger and Mode Selection */
	tc_ch0->TC_CMR =  TC_CMR_TCCLKS_XC0
			| TC_CMR_ETRGEDG_NONE
			| TC_CMR_ABETRG;

	/* Enable QDEC in Position Mode*/
	tc->TC_BMR =  TC_BMR_QDEN
		    | TC_BMR_POSEN
		    | TC_BMR_EDGPHA
		    | TC_BMR_MAXFILT(1);

	qdec_sam_start(tc);
}

static int qdec_sam_initialize(const struct device *dev)
{
	__ASSERT_NO_MSG(dev != NULL);
	const struct qdec_sam_dev_cfg *const dev_cfg = dev->config;
	int retval;

	/* Connect pins to the peripheral */
	retval = pinctrl_apply_state(dev_cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (retval < 0) {
		return retval;
	}

	for (int i = 0; i < ARRAY_SIZE(dev_cfg->clock_cfg); i++) {
		/* Enable TC clock in PMC */
		(void)clock_control_on(SAM_DT_PMC_CONTROLLER,
				       (clock_control_subsys_t)&dev_cfg->clock_cfg[i]);
	}

	qdec_sam_configure(dev);

	LOG_INF("Device %s initialized", dev->name);

	return 0;
}

static DEVICE_API(sensor, qdec_sam_driver_api) = {
	.sample_fetch = qdec_sam_fetch,
	.channel_get = qdec_sam_get,
};

#define QDEC_SAM_INIT(n)						\
	PINCTRL_DT_INST_DEFINE(n);					\
	static const struct qdec_sam_dev_cfg qdec##n##_sam_config = {	\
		.regs = (Tc *)DT_REG_ADDR(DT_INST_PARENT(n)),		\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		\
		.clock_cfg = SAM_DT_CLOCKS_PMC_CFG(DT_INST_PARENT(n)),	\
	};								\
									\
	static struct qdec_sam_dev_data qdec##n##_sam_data;		\
									\
	SENSOR_DEVICE_DT_INST_DEFINE(n, qdec_sam_initialize, NULL,	\
			    &qdec##n##_sam_data, &qdec##n##_sam_config, \
			    POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,	\
			    &qdec_sam_driver_api);

DT_INST_FOREACH_STATUS_OKAY(QDEC_SAM_INIT)
