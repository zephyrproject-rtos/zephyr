/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_npm10xx_adc

#include <math.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/npm10xx.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/linear_range.h>

LOG_MODULE_REGISTER(npm10xx_adc, CONFIG_SENSOR_LOG_LEVEL);

/* VBAT/IBAT Register Offsets */
#define NPM10_BAT_TASKS        0xC3U
#define NPM10_BAT_CONFIG0      0xC4U
#define NPM10_BAT_CONFIG1      0xC5U
#define NPM10_BAT_STATUS0      0xC6U
#define NPM10_BAT_STATUS1      0xC7U
#define NPM10_BAT_STATUS2      0xC8U
#define NPM10_BAT_STATUS3      0xC9U
#define NPM10_BAT_READVBAT     0xCAU
#define NPM10_BAT_READIBAT     0xCBU
#define NPM10_BAT_READLSBS     0xCCU
#define NPM10_BAT_CALDISCHARGE 0xCEU
#define NPM10_BAT_CALCHARGE    0xCFU
#define NPM10_BAT_CALLSBS      0xD0U

/* ADC Register Offsets */
#define NPM10_ADC_TASKS         0xADU
#define NPM10_ADC_OFFSET        0xB0U
#define NPM10_ADC_STATUS        0xB2U
#define NPM10_ADC_READVBAT      0xB3U
#define NPM10_ADC_READTEMP      0xB4U
#define NPM10_ADC_READNTC       0xB5U
#define NPM10_ADC_READLSBS0     0xB6U
#define NPM10_ADC_READVSYS      0xB7U
#define NPM10_ADC_READVSET      0xB8U
#define NPM10_ADC_READVBUS      0xB9U
#define NPM10_ADC_READLSBS1     0xBAU
#define NPM10_ADC_READOFFSET    0xBBU
#define NPM10_ADC_OFFSETFACTORY 0xBCU

/* CHARGER IBAT full scale register */
#define NPM10_CHARGER_IBATFS   0x3F
#define CHARGER_IBATFS_LVL_Msk (BIT_MASK(2) << 0)

/* IBAT/VBAT bit fields */
/* IBAT/VBAT TASKS (0xC3) */
#define BAT_TASKS_MEASURE BIT(0)

/* IBAT/VBAT CONFIG0 (0xC4) */
#define BAT_CONFIG0_SEL_Msk  (BIT_MASK(2) << 0)
#define BAT_CONFIG0_SEL_BOTH (0U << 0)
#define BAT_CONFIG0_SEL_IBAT (1U << 0)
#define BAT_CONFIG0_SEL_VBAT (2U << 0)

#define BAT_CONFIG0_TYPE        BIT(2)
#define BAT_CONFIG0_TYPE_SAMPLE 0U
#define BAT_CONFIG0_TYPE_LIVE   BIT(2)

#define BAT_CONFIG0_SAMPLEPER_Msk   (BIT_MASK(2) << 3)
#define BAT_CONFIG0_SAMPLEPER_50US  (0U << 3)
#define BAT_CONFIG0_SAMPLEPER_75US  (1U << 3)
#define BAT_CONFIG0_SAMPLEPER_100US (2U << 3)
#define BAT_CONFIG0_SAMPLEPER_150US (3U << 3)

#define BAT_CONFIG0_P2PER_Msk   (BIT_MASK(1) << 5)
#define BAT_CONFIG0_P2PER_75US  (0U << 5)
#define BAT_CONFIG0_P2PER_150US (1U << 5)

/* IBAT/VBAT CONFIG1 (0xC5) */
#define BAT_CONFIG1_ENABLE     BIT(0)
#define BAT_CONFIG1_SELECT     BIT(1)
#define BAT_CONFIG1_SELECTEXIT BIT(2)

/* IBAT/VBAT STATUS0 (0xC6) */
#define BAT_STATUS0_ICTRL_Msk (BIT_MASK(8) << 0)

/* IBAT/VBAT STATUS1 (0xC7) */
#define BAT_STATUS1_IRANGE_Msk      (BIT_MASK(3) << 0)
#define BAT_STATUS1_CHARGERMODE_Msk (BIT_MASK(3) << 3)
#define BAT_STATUS1_MEASURE         BIT(6)
#define BAT_STATUS1_SOURCE          BIT(7)

/* IBAT/VBAT STATUS2 (0xC8) */
#define BAT_STATUS2_SOURCECAPT_Msk (BIT_MASK(3) << 0)
#define BAT_STATUS2_SOURCELIVE_Msk (BIT_MASK(3) << 3)

/* IBAT/VBAT STATUS3 (0xC9) */
#define BAT_STATUS3_CHARGERSTATUS BIT(0)
#define BAT_STATUS3_IRANGE        BIT(1)
#define BAT_STATUS3_ICTRL         BIT(2)
#define BAT_STATUS3_DROPOUT       BIT(3)
#define BAT_STATUS3_DIRECTION     BIT(4)
#define BAT_STATUS3_SOURCE        BIT(5)

/* IBAT/VBAT READLSBS (0xCC) */
#define BAT_READLSBS_VBATLSB_Msk (BIT_MASK(2) << 0)
#define BAT_READLSBS_IBATLSB_Msk (BIT_MASK(2) << 2)

/* IBAT/VBAT CALLSBS (0xD0) */
#define BAT_CALLSBS_DISLSB_Msk (BIT_MASK(2) << 0)
#define BAT_CALLSBS_CHGLSB_Msk (BIT_MASK(2) << 2)

/* ADC bit fields */
/* ADC TASKS (0xAD) */
#define ADC_TASKS_VBAT    BIT(0)
#define ADC_TASKS_NTC     BIT(1)
#define ADC_TASKS_DIETEMP BIT(2)
#define ADC_TASKS_VSYS    BIT(3)
#define ADC_TASKS_VBUS    BIT(4)
#define ADC_TASKS_VSET    BIT(5)
#define ADC_TASKS_OFFSET  BIT(6)

/* ADC OFFSET (0xB0) */
#define ADC_OFFSET_ENABLE   BIT(0)
#define ADC_OFFSET_MEASURED BIT(1)

/* ADC STATUS (0xB2) */
#define ADC_STATUS_MEASURE BIT(0)

/* ADC result LSBs */
#define ADC_READLSBS0_VBATLSB_Msk   (BIT_MASK(2) << 0)
#define ADC_READLSBS0_DIETEMLSB_Msk (BIT_MASK(2) << 2)
#define ADC_READLSBS0_NTCLSB_Msk    (BIT_MASK(2) << 4)
#define ADC_READLSBS1_VSYSLSB_Msk   (BIT_MASK(2) << 0)
#define ADC_READLSBS1_VSETLSB_Msk   (BIT_MASK(2) << 2)
#define ADC_READLSBS1_VBUSLSB_Msk   (BIT_MASK(2) << 4)

static const int32_t ibat_fullscale_ua[] = {256000, 128000, 64000, 32000};

/* Equations: Ref. Datasheet - VBAT/IBAT and ADC chapters */
/* Voltages */
#define VBAT_MV(vbat) (5120 * (vbat) / 1023)
#define VBUS_MV(vbus) (12500 * (vbus) / 1023)
#define VSYS_MV(vsys) (6375 * (vsys) / 1023)
/* Charge/discharge current (µA) */
#define IBAT_CHG_UA(ictrl, ibat, cal)                                                              \
	(((ibat) < 69) ? 0                                                                         \
		       : 1000LL * ((int64_t)ictrl + 1LL) * (15LL * (int64_t)(ibat) - 1024LL) /     \
				 ((int64_t)(cal)))
#define IBAT_DIS_UA(ictrl, ibat, cal)                                                              \
	(((ibat) < 69) ? 0                                                                         \
		       : 1875LL * ((int64_t)ictrl + 1LL) * (15LL * (int64_t)(ibat) - 1024LL) /     \
				 (4LL * (int64_t)(cal)))
/* Battery temperature (milli-°C) */
#define BAT_TEMP_MC(beta, tbat)                                                                    \
	(lroundf(1000.f /                                                                          \
		 (1.f / 298.15f - 1.f / (float)(beta) * logf(1023.f / (float)(tbat) - 1.f))) -     \
	 273150)
/* Die temperature (milli-°C) */
#define DIE_TEMP_MC(tdie)             (390490 - 796 * (int32_t)(tdie))
/* Used to construct the 10-bit results from their MSB and LSB registers and LSB mask */
#define ADC_RESULT(msb, lsb, lsb_msk) ((msb << 2) | FIELD_GET((lsb_msk), (lsb)))

/* nPM10's ADC is not yet characterized - using a fixed value that's large enough */
#define CONV_TIME_MS 2
/* IBAT/VBAT result and status registers are read in burst starting at NPM10_BAT_STATUS0. Following
 * are the indexes of the relevant registers relative to STATUS0 (named ICTRL after the only field)
 */
#define IDX_ICTRL    0
#define IDX_STATUS1  1
/* STATUS2 is not used */
#define IDX_STATUS3  3
#define IDX_VBAT_MSB 4
#define IDX_IBAT_MSB 5
#define IDX_LSBS     6
#define BAT_RES_NUM  7

struct npm10xx_adc_data {
	int16_t calchg;
	int16_t caldis;
	uint16_t vbat_mv;
	uint16_t vsys_mv;
	uint16_t vbus_mv;
	uint16_t offset;
	int32_t die_mC;
	int32_t ntc_mC;
	int32_t ibat_ua;
};

struct npm10xx_adc_config {
	struct i2c_dt_spec i2c;
	int16_t beta;
};

enum npm10xx_chrg_mode {
	NPM10XX_CHRG_MODE_IDLE = 0,
	NPM10XX_CHRG_MODE_CHARGING,
	NPM10XX_CHRG_MODE_DROPOUT,
	NPM10XX_CHRG_MODE_SUPP,
	NPM10XX_CHRG_MODE_DISCH,
};

/* API implementation */
static int npm10xx_sensor_attr_set(const struct device *dev, enum sensor_channel chan,
				   enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct npm10xx_adc_config *config = dev->config;

	if (attr != SENSOR_ATTR_FULL_SCALE ||
	    (chan != SENSOR_CHAN_CURRENT && chan != SENSOR_CHAN_GAUGE_AVG_CURRENT)) {
		LOG_ERR("Only current full scale attribute is supported");
		return -ENOTSUP;
	}

	ARRAY_FOR_EACH(ibat_fullscale_ua, idx) {
		if (sensor_value_to_micro(val) == ibat_fullscale_ua[idx]) {
			return i2c_reg_write_byte_dt(&config->i2c, NPM10_CHARGER_IBATFS,
						     FIELD_PREP(CHARGER_IBATFS_LVL_Msk, idx));
		}
	}

	return -EINVAL;
}

static int npm10xx_sensor_attr_get(const struct device *dev, enum sensor_channel chan,
				   enum sensor_attribute attr, struct sensor_value *val)
{
	const struct npm10xx_adc_config *config = dev->config;
	int ret;
	uint8_t reg;

	if (attr != SENSOR_ATTR_FULL_SCALE ||
	    (chan != SENSOR_CHAN_CURRENT && chan != SENSOR_CHAN_GAUGE_AVG_CURRENT)) {
		LOG_ERR("Only current full scale attribute is supported");
		return -ENOTSUP;
	}

	ret = i2c_reg_read_byte_dt(&config->i2c, NPM10_CHARGER_IBATFS, &reg);
	if (ret < 0) {
		return ret;
	}

	return sensor_value_from_micro(val,
				       ibat_fullscale_ua[FIELD_GET(CHARGER_IBATFS_LVL_Msk, reg)]);
}

static int npm10xx_single_chan_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct npm10xx_adc_config *config = dev->config;
	struct npm10xx_adc_data *data = dev->data;
	uint8_t trig_task, msb_addr, msb, lsb;
	/* All other than Die Temp channels LSBs are in READLSBS1 */
	uint8_t lsb_addr = NPM10_ADC_READLSBS1;
	int ret;

	switch ((int16_t)chan) {
	case SENSOR_CHAN_DIE_TEMP:
		trig_task = ADC_TASKS_DIETEMP;
		msb_addr = NPM10_ADC_READTEMP;
		lsb_addr = NPM10_ADC_READLSBS0;
		break;

	case NPM10XX_SENSOR_CHAN_VSYS:
		trig_task = ADC_TASKS_VSYS;
		msb_addr = NPM10_ADC_READVSYS;
		break;

	case NPM10XX_SENSOR_CHAN_VBUS:
		trig_task = ADC_TASKS_VBUS;
		msb_addr = NPM10_ADC_READVBUS;
		break;

	case NPM10XX_SENSOR_CHAN_OFFSET:
		trig_task = ADC_TASKS_OFFSET;
		msb_addr = NPM10_ADC_READOFFSET;
		break;

	default:
		LOG_ERR("Fetching channel %d is not supported", chan);
		return -ENOTSUP;
	}

	ret = i2c_reg_write_byte_dt(&config->i2c, NPM10_ADC_TASKS, trig_task);
	if (ret < 0) {
		return ret;
	}

	k_msleep(CONV_TIME_MS);

	ret = i2c_reg_read_byte_dt(&config->i2c, msb_addr, &msb);
	if (ret < 0) {
		return ret;
	}

	if ((int16_t)chan == NPM10XX_SENSOR_CHAN_OFFSET) {
		data->offset = msb;
		return 0;
	}

	ret = i2c_reg_read_byte_dt(&config->i2c, lsb_addr, &lsb);
	if (ret < 0) {
		return ret;
	}

	if ((int16_t)chan == NPM10XX_SENSOR_CHAN_VSYS) {
		data->vsys_mv = VSYS_MV(ADC_RESULT(msb, lsb, ADC_READLSBS1_VSYSLSB_Msk));
	} else if ((int16_t)chan == NPM10XX_SENSOR_CHAN_VBUS) {
		data->vbus_mv = VBUS_MV(ADC_RESULT(msb, lsb, ADC_READLSBS1_VBUSLSB_Msk));
	} else {
		data->die_mC = DIE_TEMP_MC(ADC_RESULT(msb, lsb, ADC_READLSBS0_DIETEMLSB_Msk));
	}

	return 0;
}

static int npm10xx_sensor_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct npm10xx_adc_config *config = dev->config;
	struct npm10xx_adc_data *data = dev->data;
	int ret;
	uint8_t bat_res[BAT_RES_NUM];
	enum npm10xx_chrg_mode charger_mode;
	int16_t ictrl, ibat, vbat, ntc;
	uint8_t ntc_regs[2];

	if (chan != SENSOR_CHAN_ALL) {
		return npm10xx_single_chan_fetch(dev, chan);
	}

	/* For SENSOR_CHAN_ALL fetch battery current, voltage and temperature (if beta provided).
	 * The three measurements required for fuel gauge.
	 */
	ret = i2c_reg_write_byte_dt(&config->i2c, NPM10_BAT_TASKS, BAT_TASKS_MEASURE);
	if (ret < 0) {
		return ret;
	}
	if (config->beta != INT16_MIN) {
		ret = i2c_reg_write_byte_dt(&config->i2c, NPM10_ADC_TASKS, ADC_TASKS_NTC);
		if (ret < 0) {
			return ret;
		}
	}

	k_msleep(CONV_TIME_MS);

	ret = i2c_burst_read_dt(&config->i2c, NPM10_BAT_STATUS0, bat_res, sizeof(bat_res));
	if (ret < 0) {
		return ret;
	}

	if (FIELD_GET(BAT_STATUS1_MEASURE | BAT_STATUS1_SOURCE, bat_res[IDX_STATUS1])) {
		LOG_ERR("ADC still busy");
		return -EBUSY;
	}

	if (bat_res[IDX_STATUS3] & ~BAT_STATUS3_DROPOUT) {
		/* Bits other than DROPOUT in this register suggest an invalid measurement result */
		LOG_ERR("IBAT/VBAT results invalid: 0x%02X", bat_res[IDX_STATUS3]);
		return -EAGAIN;
	}

	charger_mode = FIELD_GET(BAT_STATUS1_CHARGERMODE_Msk, bat_res[IDX_STATUS1]);

	ibat = ADC_RESULT(bat_res[IDX_IBAT_MSB], bat_res[IDX_LSBS], BAT_READLSBS_IBATLSB_Msk);
	vbat = ADC_RESULT(bat_res[IDX_VBAT_MSB], bat_res[IDX_LSBS], BAT_READLSBS_VBATLSB_Msk);

	data->vbat_mv = VBAT_MV(vbat);

	switch (charger_mode) {
	case NPM10XX_CHRG_MODE_IDLE:
		data->ibat_ua = 0;
		break;

	case NPM10XX_CHRG_MODE_CHARGING:
	/* fall through */
	case NPM10XX_CHRG_MODE_DROPOUT:
		ictrl = bat_res[IDX_ICTRL];
		data->ibat_ua = (int32_t)IBAT_CHG_UA(ictrl, ibat, data->calchg);
		break;

	case NPM10XX_CHRG_MODE_DISCH:
	/* fall through */
	case NPM10XX_CHRG_MODE_SUPP:
		ictrl = charger_mode == NPM10XX_CHRG_MODE_DISCH ? bat_res[IDX_ICTRL] : 255;
		data->ibat_ua = -(int32_t)IBAT_DIS_UA(ictrl, ibat, data->caldis);
		break;

	default:
		LOG_ERR("Read invalid charger mode");
		return -EINVAL;
	}

	if (config->beta != INT16_MIN) {
		ret = i2c_burst_read_dt(&config->i2c, NPM10_ADC_READNTC, ntc_regs,
					ARRAY_SIZE(ntc_regs));
		if (ret < 0) {
			return ret;
		}

		ntc = ADC_RESULT(ntc_regs[0], ntc_regs[1], ADC_READLSBS0_NTCLSB_Msk);
		data->ntc_mC = BAT_TEMP_MC(config->beta, ntc);
	}

	return 0;
}

static int npm10xx_sensor_channel_get(const struct device *dev, enum sensor_channel chan,
				      struct sensor_value *val)
{
	const struct npm10xx_adc_data *data = dev->data;

	switch ((int16_t)chan) {
	case SENSOR_CHAN_CURRENT:
	/* fall through */
	case SENSOR_CHAN_GAUGE_AVG_CURRENT:
		return sensor_value_from_micro(val, data->ibat_ua);

	case SENSOR_CHAN_VOLTAGE:
	/* fall through */
	case SENSOR_CHAN_GAUGE_VOLTAGE:
		return sensor_value_from_milli(val, data->vbat_mv);

	case SENSOR_CHAN_GAUGE_TEMP:
		return sensor_value_from_milli(val, data->ntc_mC);

	case SENSOR_CHAN_DIE_TEMP:
		return sensor_value_from_milli(val, data->die_mC);

	case NPM10XX_SENSOR_CHAN_VSYS:
		return sensor_value_from_milli(val, data->vsys_mv);

	case NPM10XX_SENSOR_CHAN_VBUS:
		return sensor_value_from_milli(val, data->vbus_mv);

	case NPM10XX_SENSOR_CHAN_OFFSET:
		val->val1 = data->offset;
		return 0;

	default:
		return -ENOTSUP;
	}
}

/* Initializers */
static int npm10xx_sensor_init(const struct device *dev)
{
	const struct npm10xx_adc_config *config = dev->config;
	struct npm10xx_adc_data *data = dev->data;
	int ret;
	uint8_t cal_regs[3];

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C bus is not ready");
		return -ENODEV;
	}

	/* Calibration registers are static - read and calculate once */
	ret = i2c_burst_read_dt(&config->i2c, NPM10_BAT_CALDISCHARGE, cal_regs, sizeof(cal_regs));
	if (ret < 0) {
		return ret;
	}

	data->caldis = 15 * ADC_RESULT(cal_regs[0], cal_regs[2], BAT_CALLSBS_DISLSB_Msk) - 1024;
	data->calchg = 15 * ADC_RESULT(cal_regs[1], cal_regs[2], BAT_CALLSBS_DISLSB_Msk) - 1024;

	if (data->caldis < 0 || data->calchg < 0) {
		LOG_ERR("Read invalid calibrations for battery current");
		return -EINVAL;
	}

	return 0;
}

static DEVICE_API(sensor, npm10xx_adc_driver_api) = {
	.attr_set = npm10xx_sensor_attr_set,
	.attr_get = npm10xx_sensor_attr_get,
	.sample_fetch = npm10xx_sensor_sample_fetch,
	.channel_get = npm10xx_sensor_channel_get,
};

#define NPM10XX_ADC_INIT(n)                                                                        \
	static struct npm10xx_adc_data npm10xx_adc_data##n;                                        \
                                                                                                   \
	static const struct npm10xx_adc_config npm10xx_adc_config##n = {                           \
		.i2c = I2C_DT_SPEC_GET(DT_INST_PARENT(n)),                                         \
		.beta = DT_INST_PROP_OR(n, thermistor_beta, INT16_MIN),                            \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(n, npm10xx_sensor_init, NULL, &npm10xx_adc_data##n,           \
				     &npm10xx_adc_config##n, POST_KERNEL,                          \
				     CONFIG_SENSOR_INIT_PRIORITY, &npm10xx_adc_driver_api);

DT_INST_FOREACH_STATUS_OKAY(NPM10XX_ADC_INIT)
