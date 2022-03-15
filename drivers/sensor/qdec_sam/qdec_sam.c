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
#include <sys/__assert.h>
#include <sys/util.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <drivers/sensor.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(qdec_sam, CONFIG_SENSOR_LOG_LEVEL);

/* Device constant configuration parameters */
struct qdec_sam_dev_cfg {
	Tc *regs;
	const struct soc_gpio_pin *pin_list;
	uint8_t pin_list_size;
	uint8_t periph_id[TCCHANNEL_NUMBER];
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

	/* Connect pins to the peripheral */
	soc_gpio_list_configure(dev_cfg->pin_list, dev_cfg->pin_list_size);

	for (int i = 0; i < ARRAY_SIZE(dev_cfg->periph_id); i++) {
		/* Enable module's clock */
		soc_pmc_peripheral_enable(dev_cfg->periph_id[i]);
	}

	qdec_sam_configure(dev);

	LOG_INF("Device %s initialized", dev->name);

	return 0;
}

static const struct sensor_driver_api qdec_sam_driver_api = {
	.sample_fetch = qdec_sam_fetch,
	.channel_get = qdec_sam_get,
};

#define QDEC_SAM_INIT(n)						\
	static const struct soc_gpio_pin pins_tc##n[] = ATMEL_SAM_DT_INST_PINS(n); \
									\
	static const struct qdec_sam_dev_cfg qdec##n##_sam_config = {	\
		.regs = (Tc *)DT_INST_REG_ADDR(n),			\
		.pin_list = pins_tc##n,					\
		.pin_list_size = ARRAY_SIZE(pins_tc##n),		\
		.periph_id = DT_INST_PROP(n, peripheral_id),		\
	};								\
									\
	static struct qdec_sam_dev_data qdec##n##_sam_data;		\
									\
	DEVICE_DT_INST_DEFINE(n, qdec_sam_initialize, NULL,		\
			    &qdec##n##_sam_data, &qdec##n##_sam_config, \
			    POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,	\
			    &qdec_sam_driver_api);

DT_INST_FOREACH_STATUS_OKAY(QDEC_SAM_INIT)
