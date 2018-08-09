/*
 * Copyright (c) 2017 comsuisse AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Atmel SAM MCU family Quadrature Decoder (QDEC/TC) driver.
 */

#include <errno.h>
#include <misc/__assert.h>
#include <misc/util.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <sensor.h>

#define SYS_LOG_DOMAIN "dev/qdec_sam"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SENSOR_LEVEL
#include <logging/sys_log.h>

/* Device constant configuration parameters */
struct qdec_sam_dev_cfg {
	Tc *regs;
	const struct soc_gpio_pin *pin_list;
	u8_t pin_list_size;
	u8_t periph_id[TCCHANNEL_NUMBER];
	u8_t steps_number;
};

/* Device run time data */
struct qdec_sam_dev_data {
	s16_t position;
	s16_t prev_position;
	s32_t turns;
};

#define DEV_NAME(dev) ((dev)->config->name)
#define DEV_CFG(dev) \
	((const struct qdec_sam_dev_cfg *const)(dev)->config->config_info)
#define DEV_DATA(dev) \
	((struct qdec_sam_dev_data *const)(dev)->driver_data)

static int qdec_sam_fetch(struct device *dev, enum sensor_channel chan)
{
	const struct qdec_sam_dev_cfg *const dev_cfg = DEV_CFG(dev);
	struct qdec_sam_dev_data *const dev_data = DEV_DATA(dev);
	Tc *const tc = dev_cfg->regs;
	TcChannel *tc_ch0 = &tc->TC_CHANNEL[0];

	dev_data->prev_position = dev_data->position;

	/* Read position register content */
	dev_data->position = tc_ch0->TC_CV;

	return 0;
}

static int qdec_sam_get(struct device *dev, enum sensor_channel chan,
			struct sensor_value *val)
{
	const struct qdec_sam_dev_cfg *const dev_cfg = DEV_CFG(dev);
	struct qdec_sam_dev_data *const dev_data = DEV_DATA(dev);
	s32_t degree;

	if (chan == SENSOR_ANGULAR_POSITION) {
		dev_data->turns += dev_data->position - dev_data->prev_position;
		dev_data->turns = dev_data->turns % (4 * dev_cfg->steps_number);
		degree = (dev_data->turns / 4) * 360 / dev_cfg->steps_number;
		if (degree < 0) {
			if (degree <= -180) {
				degree += 360;
			}
		} else {
			if (degree > 180) {
				degree -= 360;
			}
		}
		sensor_degrees_to_rad(degree, val);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static void qdec_sam_start(Tc *const tc)
{
	TcChannel *tc_ch0 = &tc->TC_CHANNEL[0];

	/* Enable Channel 0 Clock and reset counter*/
	tc_ch0->TC_CCR =  TC_CCR_CLKEN
			| TC_CCR_SWTRG;
}

static void qdec_sam_configure(struct device *dev)
{
	const struct qdec_sam_dev_cfg *const dev_cfg = DEV_CFG(dev);
	Tc *const tc = dev_cfg->regs;
	TcChannel *tc_ch0 = &tc->TC_CHANNEL[0];

	/* Clock, Trigger Edge, Trigger and Mode Selection */
	tc_ch0->TC_CMR =  TC_CMR_TCCLKS_XC0
			| TC_CMR_ETRGEDG_RISING
			| TC_CMR_ABETRG;

	/* Enable QDEC in Position Mode*/
	tc->TC_BMR =  TC_BMR_QDEN
		    | TC_BMR_POSEN
		    | TC_BMR_EDGPHA
		    | TC_BMR_MAXFILT(1);

	qdec_sam_start(tc);
}

static int qdec_sam_initialize(struct device *dev)
{
	__ASSERT_NO_MSG(dev != NULL);
	const struct qdec_sam_dev_cfg *const dev_cfg = DEV_CFG(dev);

	/* Connect pins to the peripheral */
	soc_gpio_list_configure(dev_cfg->pin_list, dev_cfg->pin_list_size);

	for (int i = 0; i < ARRAY_SIZE(dev_cfg->periph_id); i++) {
		/* Enable module's clock */
		soc_pmc_peripheral_enable(dev_cfg->periph_id[i]);
	}

	qdec_sam_configure(dev);

	SYS_LOG_INF("Device %s initialized", DEV_NAME(dev));

	return 0;
}

static const struct sensor_driver_api qdec_sam_driver_api = {
	.sample_fetch = qdec_sam_fetch,
	.channel_get = qdec_sam_get,
};

/* QDEC_0 */
#ifdef CONFIG_QDEC_0_SAM
static struct device DEVICE_NAME_GET(qdec0_sam);

static const struct soc_gpio_pin pins_tc0[] = {PIN_TC0_TIOA0, PIN_TC0_TIOB0};

static const struct qdec_sam_dev_cfg qdec0_sam_config = {
	.regs = TC0,
	.pin_list = pins_tc0,
	.pin_list_size = ARRAY_SIZE(pins_tc0),
	.periph_id = {ID_TC0, ID_TC1, ID_TC2},
	.steps_number = CONFIG_QDEC_0_STEPS_PER_UNIT,
};

static struct qdec_sam_dev_data qdec0_sam_data;

DEVICE_AND_API_INIT(qdec0_sam, CONFIG_QDEC_0_SAM_NAME, &qdec_sam_initialize,
		    &qdec0_sam_data, &qdec0_sam_config, POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY, &qdec_sam_driver_api);
#endif

/* QDEC_1 */

#ifdef CONFIG_QDEC_1_SAM
static struct device DEVICE_NAME_GET(qdec1_sam);

static const struct soc_gpio_pin pins_tc1[] = {PIN_TC1_TIOA0, PIN_TC1_TIOB0};

static const struct qdec_sam_dev_cfg qdec1_sam_config = {
	.regs = TC1,
	.pin_list = pins_tc1,
	.pin_list_size = ARRAY_SIZE(pins_tc1),
	.periph_id = {ID_TC3, ID_TC4, ID_TC5},
	.steps_number = CONFIG_QDEC_1_STEPS_PER_UNIT,
};

static struct qdec_sam_dev_data qdec1_sam_data;

DEVICE_AND_API_INIT(qdec1_sam, CONFIG_QDEC_1_SAM_NAME, &qdec_sam_initialize,
		    &qdec1_sam_data, &qdec1_sam_config, POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY, &qdec_sam_driver_api);
#endif

/* QDEC_2 */

#ifdef CONFIG_QDEC_2_SAM
static struct device DEVICE_NAME_GET(qdec2_sam);

static const struct soc_gpio_pin pins_tc2[] = {PIN_TC2_TIOA0, PIN_TC2_TIOB0};

static const struct qdec_sam_dev_cfg qdec2_sam_config = {
	.regs = TC2,
	.pin_list = pins_tc2,
	.pin_list_size = ARRAY_SIZE(pins_tc2),
	.periph_id = {ID_TC6, ID_TC7, ID_TC8},
	.steps_number = CONFIG_QDEC_2_STEPS_PER_UNIT,
};

static struct qdec_sam_dev_data qdec2_sam_data;

DEVICE_AND_API_INIT(qdec2_sam, CONFIG_QDEC_2_SAM_NAME, &qdec_sam_initialize,
		    &qdec2_sam_data, &qdec2_sam_config, POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY, &qdec_sam_driver_api);
#endif

/* QDEC_3 */

#ifdef CONFIG_QDEC_3_SAM
static struct device DEVICE_NAME_GET(qdec3_sam);

static const struct soc_gpio_pin pins_tc3[] = {PIN_TC3_TIOA0, PIN_TC3_TIOB0};

static const struct qdec_sam_dev_cfg qdec3_sam_config = {
	.regs = TC3,
	.pin_list = pins_tc3,
	.pin_list_size = ARRAY_SIZE(pins_tc3),
	.periph_id = {ID_TC9, ID_TC10, ID_TC11},
	.steps_number = CONFIG_QDEC_3_STEPS_PER_UNIT,
};

static struct qdec_sam_dev_data tc3_sam_data;

DEVICE_AND_API_INIT(qdec3_sam, CONFIG_QDEC_3_SAM_NAME, &qdec_sam_initialize,
		    &qdec3_sam_data, &qdec3_sam_config, POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY, &qdec_sam_driver_api);
#endif
