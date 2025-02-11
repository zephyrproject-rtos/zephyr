/*
 * Copyright (c) 2023 Centralp
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_tempmon

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>

/* OTP Controller Analog Register 1 for calibration values */
#define OCOTP_ANA1_ROOM_COUNT_SHIFT 20
#define OCOTP_ANA1_ROOM_COUNT_MASK  (BIT_MASK(12) << OCOTP_ANA1_ROOM_COUNT_SHIFT)
#define OCOTP_ANA1_HOT_COUNT_SHIFT  8
#define OCOTP_ANA1_HOT_COUNT_MASK   (BIT_MASK(12) << OCOTP_ANA1_HOT_COUNT_SHIFT)
#define OCOTP_ANA1_HOT_TEMP_SHIFT   0
#define OCOTP_ANA1_HOT_TEMP_MASK    (BIT_MASK(8) << OCOTP_ANA1_HOT_TEMP_SHIFT)

#define TEMPMON_ROOM_TEMP 25.0f

struct nxp_tempmon_data {
	uint8_t hot_temp;
	uint16_t hot_cnt;
	uint16_t room_cnt;
	uint16_t temp_cnt;
};

struct nxp_tempmon_config {
	TEMPMON_Type *base;
};

static int nxp_tempmon_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct nxp_tempmon_data *data = dev->data;
	const struct nxp_tempmon_config *cfg = dev->config;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	/* Start the measure */
	cfg->base->TEMPSENSE0 |= TEMPMON_TEMPSENSE0_MEASURE_TEMP_MASK;

	/* Wait until measure is ready */
	while (!(cfg->base->TEMPSENSE0 & TEMPMON_TEMPSENSE0_FINISHED_MASK)) {
	}
	data->temp_cnt = (cfg->base->TEMPSENSE0 & TEMPMON_TEMPSENSE0_TEMP_CNT_MASK) >>
			 TEMPMON_TEMPSENSE0_TEMP_CNT_SHIFT;

	/* Stop the measure */
	cfg->base->TEMPSENSE0 &= ~TEMPMON_TEMPSENSE0_MEASURE_TEMP_MASK;

	return 0;
}

static int nxp_tempmon_channel_get(const struct device *dev, enum sensor_channel chan,
				   struct sensor_value *val)
{
	const struct nxp_tempmon_data *data = dev->data;
	float temp;

	if (chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	temp = (float)data->hot_temp - ((float)data->temp_cnt - (float)data->hot_cnt) *
					       (((float)data->hot_temp - TEMPMON_ROOM_TEMP) /
						((float)data->room_cnt - (float)data->hot_cnt));

	return sensor_value_from_double(val, temp);
}

static const struct sensor_driver_api nxp_tempmon_driver_api = {
	.sample_fetch = nxp_tempmon_sample_fetch,
	.channel_get = nxp_tempmon_channel_get,
};

static int nxp_tempmon_init(const struct device *dev)
{
	struct nxp_tempmon_data *data = dev->data;
	const struct nxp_tempmon_config *cfg = dev->config;
	uint32_t ocotp_ana1;

	/* Enable the temperature sensor */
	cfg->base->TEMPSENSE0 &= ~TEMPMON_TEMPSENSE0_POWER_DOWN_MASK;

	/* Single measure with no repeat mode */
	cfg->base->TEMPSENSE1 = TEMPMON_TEMPSENSE1_MEASURE_FREQ(0);

	/* Read calibration data */
	ocotp_ana1 = OCOTP->ANA1;
	data->hot_temp = (ocotp_ana1 & OCOTP_ANA1_HOT_TEMP_MASK) >> OCOTP_ANA1_HOT_TEMP_SHIFT;
	data->hot_cnt = (ocotp_ana1 & OCOTP_ANA1_HOT_COUNT_MASK) >> OCOTP_ANA1_HOT_COUNT_SHIFT;
	data->room_cnt = (ocotp_ana1 & OCOTP_ANA1_ROOM_COUNT_MASK) >> OCOTP_ANA1_ROOM_COUNT_SHIFT;

	return 0;
}

static struct nxp_tempmon_data nxp_tempmon_dev_data = {
	.hot_temp = 0,
	.hot_cnt = 0,
	.room_cnt = 0,
	.temp_cnt = 0,
};

static const struct nxp_tempmon_config nxp_tempmon_dev_config = {
	.base = (TEMPMON_Type *)DT_INST_REG_ADDR(0),
};

SENSOR_DEVICE_DT_INST_DEFINE(0, nxp_tempmon_init, NULL, &nxp_tempmon_dev_data,
			     &nxp_tempmon_dev_config, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
			     &nxp_tempmon_driver_api);
