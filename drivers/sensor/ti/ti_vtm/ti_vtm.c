/*
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: Copyright (c) 2026 Texas Instruments Incorporated
 */

#define DT_DRV_COMPAT ti_vtm

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/ti_vtm.h>
#include <zephyr/irq.h>

LOG_MODULE_REGISTER(ti_vtm, CONFIG_SENSOR_LOG_LEVEL);

/* VTM Register Layout */
struct ti_vtm_cfg1_regs {
	uint32_t RESERVED_1;            /**< Reserved, offset: 0x00 */
	volatile uint32_t DEVINFO_PWR0; /**< Device info power domain 0, offset: 0x04 */
	uint8_t RESERVED_2[0xF8];       /**< Reserved, offset: 0x08 - 0xFF */

	volatile struct {
		volatile uint32_t DEVINFO;     /**< VD device info, offset: 0x100 + (0x20 * i) */
		volatile uint32_t OPPVID;      /**< Voltage OPP ID, offset: 0x104 + (0x20 * i) */
		volatile uint32_t EVT_STAT;    /**< Event status, offset: 0x108 + (0x20 * i) */
		volatile uint32_t EVT_SET;     /**< Event set, offset: 0x10C + (0x20 * i) */
		volatile uint32_t EVT_SEL_CLR; /**< Event clear, offset: 0x110 + (0x20 * i) */
		uint8_t RESERVED[0xC];         /**< Reserved, offset: 0x114 + (0x20 * i) */
	} VD[8];

	uint8_t RESERVED_3[0x4]; /**< Reserved, offset: 0x200 - 0x203 */

	volatile uint32_t GT_TH1_INT_RAW_STAT_SET; /**< GT TH1 Raw Status Set, offset: 0x204 */
	volatile uint32_t GT_TH1_INT_EN_STAT_CLR;  /**< GT TH1 Enable Status Clear, offset: 0x208 */
	uint8_t RESERVED_4[0x8];                   /**< Reserved, offset: 0x20C - 0x214 */
	volatile uint32_t GT_TH1_INT_EN_SET;       /**< GT TH1 Enable Set, offset: 0x214 */
	volatile uint32_t GT_TH1_INT_EN_CLR;       /**< GT TH1 Enable Clear, offset: 0x218 */
	uint8_t RESERVED_5[0x8];                   /**< Reserved, offset: 0x21C - 0x224 */

	volatile uint32_t GT_TH2_INT_RAW_STAT_SET; /**< GT TH2 Raw Status Set, offset: 0x224 */
	volatile uint32_t GT_TH2_INT_EN_STAT_CLR;  /**< GT TH2 Enable Status Clear, offset: 0x228 */
	uint8_t RESERVED_6[0x8];                   /**< Reserved, offset: 0x22C - 0x234 */
	volatile uint32_t GT_TH2_INT_EN_SET;       /**< GT TH2 Enable Set, offset: 0x234 */
	volatile uint32_t GT_TH2_INT_EN_CLR;       /**< GT TH2 Enable Clear, offset: 0x238 */
	uint8_t RESERVED_7[0x8];                   /**< Reserved, offset: 0x23C - 0x244 */

	volatile uint32_t LT_TH0_INT_RAW_STAT_SET; /**< LT TH0 Raw Status Set, offset: 0x244 */
	volatile uint32_t LT_TH0_INT_EN_STAT_CLR;  /**< LT TH0 Enable Status Clear, offset: 0x248 */
	uint8_t RESERVED_8[0x8];                   /**< Reserved, offset: 0x24C - 0x254 */
	volatile uint32_t LT_TH0_INT_EN_SET;       /**< LT TH0 Enable Set, offset: 0x254 */
	volatile uint32_t LT_TH0_INT_EN_CLR;       /**< LT TH0 Enable Clear, offset: 0x258 */
	uint8_t RESERVED_9[0xA4];                  /**< Reserved, offset: 0x25C - 0x300 */

	volatile struct {
		volatile uint32_t CTRL;   /**< Sensor Control, offset: 0x300 + (0x20 * i) */
		uint8_t RESERVED_9[0x4];  /**< Reserved, offset: 0x304 + (0x20 * i) */
		volatile uint32_t STAT;   /**< Sensor Status, offset: 0x308 + (0x20 * i) */
		volatile uint32_t TH;     /**< Sensor Threshold, offset: 0x30C + (0x20 * i) */
		volatile uint32_t TH2;    /**< Sensor Threshold 2, offset: 0x310 + (0x20 * i) */
		uint8_t RESERVED_10[0xC]; /**< Reserved, offset: 0x314 + (0x20 * i) */
	} TMPSENS[8];
};

struct ti_vtm_cfg2_regs {
	uint8_t RESERVED_1[0xC];      /**< Reserved, offset: 0x0 - 0xc */
	volatile uint32_t MISC_CTRL;  /**< Misc control, offset: 0x0C */
	volatile uint32_t MISC_CTRL2; /**< Misc control 2, offset: 0x10 */
	uint8_t RESERVED_2[0x2EC];    /**< Reserved, offset: 0x14 - 0x300 */
	volatile struct {
		volatile uint32_t CTRL;   /**< Sensor control, offset: 0x300 + (0x20 * i) */
		uint8_t RESERVED_3[0x1C]; /**< Reserved, offset: 0x304 + (0x20 * i) */
	} TMPSENS[8];
};

/* Register field definitions */
#define TI_VTM_DEVINFO_PWR0_TEMPSENS_CT           GENMASK(7, 4)
#define TI_VTM_CFG1_TMPSENS_STAT_EOC_FC_UPDATE    BIT(11)
#define TI_VTM_CFG1_TMPSENS_STAT_DATA_OUT         GENMASK(9, 0)
#define TI_VTM_CFG1_TMPSENS_STAT_MAXT_OUTRG_ALERT BIT(15)

/* CFG2 global control register bits */
#define TI_VTM_CFG2_MISC_CTRL_ANYMAXT_OUTRG_ALERT_EN BIT(0)

/* CFG2 MISC_CTRL2 fields */
#define TI_VTM_CFG2_MISC_CTRL2_MAXT_OUTRG_ALERT_THR  GENMASK(9, 0)
#define TI_VTM_CFG2_MISC_CTRL2_MAXT_OUTRG_ALERT_THR0 GENMASK(25, 16)

/* Per-sensor control register bits */
#define TI_VTM_CFG2_TMPSENS_CTRL_MAXT_OUTRG_EN BIT(11)
#define TI_VTM_CFG2_TMPSENS_CTRL_CLRZ          BIT(6)
#define TI_VTM_CFG2_TMPSENS_CTRL_SOC           BIT(5)
#define TI_VTM_CFG2_TMPSENS_CTRL_CONT          BIT(4)

#define TI_VTM_CFG1_TMPSENS_CTRL_GT_TH1_EN BIT(8)
#define TI_VTM_CFG1_TMPSENS_CTRL_GT_TH2_EN BIT(9)
#define TI_VTM_CFG1_TMPSENS_CTRL_LT_TH0_EN BIT(10)

/* VD event select register fields */
#define TI_VTM_CFG1_VD_EVT_SET_TSENS_EVT_SEL GENMASK(23, 16)

/* Per-sensor threshold register fields */
#define TI_VTM_CFG1_TMPSENS_TH_TH0_VAL  GENMASK(9, 0)
#define TI_VTM_CFG1_TMPSENS_TH_TH1_VAL  GENMASK(25, 16)
#define TI_VTM_CFG1_TMPSENS_TH2_TH2_VAL GENMASK(9, 0)

/* Temperature constants */
#define TI_VTM_TABLE_SIZE 1024

/* Timeout for first conversion after sensor enable (microseconds) */
#define TI_VTM_EOC_TIMEOUT_US 1000U

struct ti_vtm_sensor_data {
	int32_t temperature_mc;
	sensor_trigger_handler_t th0_handler;
	sensor_trigger_handler_t th1_handler;
	sensor_trigger_handler_t th2_handler;
};

struct ti_vtm_sensor_config {
	const struct device *vtm;
	uint8_t sensor_id;
	uint8_t vd_id;
};

struct ti_vtm_data {
	DEVICE_MMIO_NAMED_RAM(cfg1);
	DEVICE_MMIO_NAMED_RAM(cfg2);
};

struct ti_vtm_config {
	DEVICE_MMIO_NAMED_ROM(cfg1);
	DEVICE_MMIO_NAMED_ROM(cfg2);
	void (*irq_config)(void);
	const struct device **sensor_devices;
	uint8_t sensor_count;
	int32_t maxt_outrg_thresh_mc;
	int32_t maxt_outrg_thresh0_mc;
};

#define DEV_CFG(dev)       ((const struct ti_vtm_config *)dev->config)
#define DEV_DATA(dev)      ((struct ti_vtm_data *)dev->data)
#define DEV_CFG1_REGS(dev) ((struct ti_vtm_cfg1_regs *)DEVICE_MMIO_NAMED_GET(dev, cfg1))
#define DEV_CFG2_REGS(dev) ((struct ti_vtm_cfg2_regs *)DEVICE_MMIO_NAMED_GET(dev, cfg2))

#define SENSOR_DEV_DATA(dev) ((struct ti_vtm_sensor_data *)dev->data)
#define SENSOR_DEV_CFG(dev)  ((const struct ti_vtm_sensor_config *)dev->config)

/* VTM polynomial coefficients (scaled by 10^13) */
#define TI_VTM_COEF_REDUCTION_EXP (10000000000000LL)
#define TI_VTM_COEF_A0            (-490019999999999936LL)
#define TI_VTM_COEF_A1            (3251200000000000LL)
#define TI_VTM_COEF_A2            (-1705800000000LL)
#define TI_VTM_COEF_A3            (603730000LL)
#define TI_VTM_COEF_A4            (-92627LL)

#define TI_VTM_CALC_POLY(x, a4, a3, a2, a1, a0)                                                    \
	((int32_t)((a4 * (int64_t)(x) * (int64_t)(x) * (int64_t)(x) * (int64_t)(x) +               \
		    a3 * (int64_t)(x) * (int64_t)(x) * (int64_t)(x) +                              \
		    a2 * (int64_t)(x) * (int64_t)(x) + a1 * (int64_t)(x) + a0) /                   \
		   TI_VTM_COEF_REDUCTION_EXP))

static const int32_t ti_vtm_temp_table[TI_VTM_TABLE_SIZE] = {
	LISTIFY(TI_VTM_TABLE_SIZE, TI_VTM_CALC_POLY, (,),
		TI_VTM_COEF_A4, TI_VTM_COEF_A3, TI_VTM_COEF_A2, TI_VTM_COEF_A1, TI_VTM_COEF_A0),
};

static int ti_vtm_temp_to_adc_code(int32_t temp_mc)
{
	int low, high, mid;

	/* Bounds checking - table goes from low to high temperature */
	if (temp_mc < ti_vtm_temp_table[0] || temp_mc > ti_vtm_temp_table[TI_VTM_TABLE_SIZE - 1]) {
		return -EINVAL;
	}

	/* Binary search - temperature increases with increasing ADC code */
	low = 0;
	high = TI_VTM_TABLE_SIZE - 1;

	while (low < (high - 1)) {
		mid = (low + high) / 2;
		if (temp_mc <= ti_vtm_temp_table[mid]) {
			high = mid;
		} else {
			low = mid;
		}
	}

	return mid;
}

static int ti_vtm_adc_code_to_temp(int adc_code, int32_t *temp_mc)
{
	if (adc_code < 0 || adc_code >= TI_VTM_TABLE_SIZE) {
		return -EINVAL;
	}

	*temp_mc = ti_vtm_temp_table[adc_code];
	return 0;
}

static int ti_vtm_read_temp(const struct device *vtm, uint8_t sensor_id, int32_t *temp_mc)
{
	const struct ti_vtm_config *config = DEV_CFG(vtm);
	struct ti_vtm_cfg1_regs *cfg1_regs = DEV_CFG1_REGS(vtm);
	uint32_t adc_code;
	int ret;

	if (sensor_id >= config->sensor_count) {
		LOG_ERR("Invalid sensor ID %d (max: %d)", sensor_id, config->sensor_count - 1);
		return -EINVAL;
	}

	/* Read ADC result */
	adc_code = FIELD_GET(TI_VTM_CFG1_TMPSENS_STAT_DATA_OUT, cfg1_regs->TMPSENS[sensor_id].STAT);

	/* Convert to temperature */
	ret = ti_vtm_adc_code_to_temp(adc_code, temp_mc);
	if (ret < 0) {
		return ret;
	}

	LOG_DBG("Sensor %d: ADC=%d, Temp=%d mC", sensor_id, adc_code, *temp_mc);
	return 0;
}

static int ti_vtm_set_th0_threshold(const struct device *sensor, int32_t temp_mc)
{
	const struct ti_vtm_sensor_config *config = SENSOR_DEV_CFG(sensor);
	struct ti_vtm_cfg1_regs *cfg1_regs = DEV_CFG1_REGS(config->vtm);
	uint32_t th_reg;
	int adc;

	adc = ti_vtm_temp_to_adc_code(temp_mc);
	if (adc < 0) {
		return adc;
	}

	th_reg = cfg1_regs->TMPSENS[config->sensor_id].TH;
	th_reg &= ~TI_VTM_CFG1_TMPSENS_TH_TH0_VAL;
	th_reg |= FIELD_PREP(TI_VTM_CFG1_TMPSENS_TH_TH0_VAL, adc);
	cfg1_regs->TMPSENS[config->sensor_id].TH = th_reg;

	return 0;
}

static int ti_vtm_enable_th0_threshold(const struct device *sensor, bool enable)
{
	const struct ti_vtm_sensor_config *config = SENSOR_DEV_CFG(sensor);
	struct ti_vtm_cfg1_regs *cfg1_regs = DEV_CFG1_REGS(config->vtm);

	if (enable) {
		cfg1_regs->TMPSENS[config->sensor_id].CTRL |= TI_VTM_CFG1_TMPSENS_CTRL_LT_TH0_EN;
		cfg1_regs->LT_TH0_INT_EN_SET = BIT(config->vd_id);
	} else {
		cfg1_regs->TMPSENS[config->sensor_id].CTRL &= ~TI_VTM_CFG1_TMPSENS_CTRL_LT_TH0_EN;
		cfg1_regs->LT_TH0_INT_EN_CLR = BIT(config->vd_id);
	}

	return 0;
}

static int ti_vtm_set_th1_threshold(const struct device *sensor, int32_t temp_mc)
{
	const struct ti_vtm_sensor_config *config = SENSOR_DEV_CFG(sensor);
	struct ti_vtm_cfg1_regs *cfg1_regs = DEV_CFG1_REGS(config->vtm);
	uint32_t th_reg;
	int adc;

	adc = ti_vtm_temp_to_adc_code(temp_mc);
	if (adc < 0) {
		return adc;
	}

	th_reg = cfg1_regs->TMPSENS[config->sensor_id].TH;
	th_reg &= ~TI_VTM_CFG1_TMPSENS_TH_TH1_VAL;
	th_reg |= FIELD_PREP(TI_VTM_CFG1_TMPSENS_TH_TH1_VAL, adc);
	cfg1_regs->TMPSENS[config->sensor_id].TH = th_reg;

	return 0;
}

static int ti_vtm_enable_th1_threshold(const struct device *sensor, bool enable)
{
	const struct ti_vtm_sensor_config *config = SENSOR_DEV_CFG(sensor);
	struct ti_vtm_cfg1_regs *cfg1_regs = DEV_CFG1_REGS(config->vtm);

	if (enable) {
		cfg1_regs->TMPSENS[config->sensor_id].CTRL |= TI_VTM_CFG1_TMPSENS_CTRL_GT_TH1_EN;
		cfg1_regs->GT_TH1_INT_EN_SET = BIT(config->vd_id);
	} else {
		cfg1_regs->TMPSENS[config->sensor_id].CTRL &= ~TI_VTM_CFG1_TMPSENS_CTRL_GT_TH1_EN;
		cfg1_regs->GT_TH1_INT_EN_CLR = BIT(config->vd_id);
	}

	return 0;
}

static int ti_vtm_set_th2_threshold(const struct device *sensor, int32_t temp_mc)
{
	const struct ti_vtm_sensor_config *config = SENSOR_DEV_CFG(sensor);
	struct ti_vtm_cfg1_regs *cfg1_regs = DEV_CFG1_REGS(config->vtm);
	int adc;

	adc = ti_vtm_temp_to_adc_code(temp_mc);
	if (adc < 0) {
		return adc;
	}

	cfg1_regs->TMPSENS[config->sensor_id].TH2 =
		FIELD_PREP(TI_VTM_CFG1_TMPSENS_TH2_TH2_VAL, adc);

	return 0;
}

static int ti_vtm_enable_th2_threshold(const struct device *sensor, bool enable)
{
	const struct ti_vtm_sensor_config *config = SENSOR_DEV_CFG(sensor);
	struct ti_vtm_cfg1_regs *cfg1_regs = DEV_CFG1_REGS(config->vtm);

	if (enable) {
		cfg1_regs->TMPSENS[config->sensor_id].CTRL |= TI_VTM_CFG1_TMPSENS_CTRL_GT_TH2_EN;
		cfg1_regs->GT_TH2_INT_EN_SET = BIT(config->vd_id);
	} else {
		cfg1_regs->TMPSENS[config->sensor_id].CTRL &= ~TI_VTM_CFG1_TMPSENS_CTRL_GT_TH2_EN;
		cfg1_regs->GT_TH2_INT_EN_CLR = BIT(config->vd_id);
	}

	return 0;
}

static int ti_vtm_sensor_sample_fetch(const struct device *sensor, enum sensor_channel chan)
{
	struct ti_vtm_sensor_data *data = SENSOR_DEV_DATA(sensor);
	const struct ti_vtm_sensor_config *config = SENSOR_DEV_CFG(sensor);
	int ret;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	ret = ti_vtm_read_temp(config->vtm, config->sensor_id, &data->temperature_mc);
	if (ret < 0) {
		LOG_ERR("Failed to read temperature from vtm: %d", ret);
		return ret;
	}

	LOG_DBG("VTM sensor %s (ID %d): %d mC", sensor->name, config->sensor_id,
		data->temperature_mc);
	return 0;
}

static int ti_vtm_sensor_channel_get(const struct device *sensor, enum sensor_channel chan,
				     struct sensor_value *val)
{
	struct ti_vtm_sensor_data *data = SENSOR_DEV_DATA(sensor);

	if (chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	val->val1 = data->temperature_mc / 1000;
	val->val2 = (data->temperature_mc % 1000) * 1000;

	return 0;
}

static int ti_vtm_sensor_trigger_set(const struct device *sensor, const struct sensor_trigger *trig,
				     sensor_trigger_handler_t handler)
{
	struct ti_vtm_sensor_data *data = SENSOR_DEV_DATA(sensor);

	if (trig->chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	switch ((enum ti_vtm_sensor_trigger_type)trig->type) {
	case TI_VTM_TRIG_TH0: {
		int ret = ti_vtm_enable_th0_threshold(sensor, handler != NULL);

		if (ret < 0) {
			return ret;
		}
		data->th0_handler = handler;
		break;
	}
	case TI_VTM_TRIG_TH1: {
		int ret = ti_vtm_enable_th1_threshold(sensor, handler != NULL);

		if (ret < 0) {
			return ret;
		}
		data->th1_handler = handler;
		break;
	}
	case TI_VTM_TRIG_TH2: {
		int ret = ti_vtm_enable_th2_threshold(sensor, handler != NULL);

		if (ret < 0) {
			return ret;
		}
		data->th2_handler = handler;
		break;
	}
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int ti_vtm_sensor_attr_set(const struct device *sensor, enum sensor_channel chan,
				  enum sensor_attribute attr, const struct sensor_value *val)
{
	int32_t temp_mc = val->val1 * 1000 + val->val2 / 1000;
	int ret = 0;

	if (chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	switch ((enum vtm_sensor_attribute)attr) {
	case TI_VTM_ATTR_TH0_THRESH:
		ret = ti_vtm_set_th0_threshold(sensor, temp_mc);
		break;
	case TI_VTM_ATTR_TH1_THRESH:
		ret = ti_vtm_set_th1_threshold(sensor, temp_mc);
		break;
	case TI_VTM_ATTR_TH2_THRESH:
		ret = ti_vtm_set_th2_threshold(sensor, temp_mc);
		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}

static int ti_vtm_sensor_attr_get(const struct device *sensor, enum sensor_channel chan,
				  enum sensor_attribute attr, struct sensor_value *val)
{
	const struct ti_vtm_sensor_config *config = SENSOR_DEV_CFG(sensor);
	struct ti_vtm_cfg1_regs *cfg1_regs = DEV_CFG1_REGS(config->vtm);
	int32_t temp_mc;
	int adc;
	int ret;

	if (chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	switch ((enum vtm_sensor_attribute)attr) {
	case TI_VTM_ATTR_TH0_THRESH:
		adc = FIELD_GET(TI_VTM_CFG1_TMPSENS_TH_TH0_VAL,
				cfg1_regs->TMPSENS[config->sensor_id].TH);
		break;
	case TI_VTM_ATTR_TH1_THRESH:
		adc = FIELD_GET(TI_VTM_CFG1_TMPSENS_TH_TH1_VAL,
				cfg1_regs->TMPSENS[config->sensor_id].TH);
		break;
	case TI_VTM_ATTR_TH2_THRESH:
		adc = FIELD_GET(TI_VTM_CFG1_TMPSENS_TH2_TH2_VAL,
				cfg1_regs->TMPSENS[config->sensor_id].TH2);
		break;
	case TI_VTM_ATTR_OUTRG_ALERT:
		val->val1 = !!(cfg1_regs->TMPSENS[config->sensor_id].STAT &
			       TI_VTM_CFG1_TMPSENS_STAT_MAXT_OUTRG_ALERT);
		val->val2 = 0;
		return 0;
	default:
		return -ENOTSUP;
	}

	ret = ti_vtm_adc_code_to_temp(adc, &temp_mc);

	if (ret < 0) {
		return ret;
	}

	val->val1 = temp_mc / 1000;
	val->val2 = (temp_mc % 1000) * 1000;
	return 0;
}

static DEVICE_API(sensor, ti_vtm_sensor_api) = {
	.sample_fetch = ti_vtm_sensor_sample_fetch,
	.channel_get = ti_vtm_sensor_channel_get,
	.trigger_set = ti_vtm_sensor_trigger_set,
	.attr_set = ti_vtm_sensor_attr_set,
	.attr_get = ti_vtm_sensor_attr_get,
};

static int ti_vtm_sensor_init(const struct device *sensor)
{
	const struct ti_vtm_sensor_config *config = SENSOR_DEV_CFG(sensor);
	struct ti_vtm_cfg1_regs *cfg1_regs = DEV_CFG1_REGS(config->vtm);
	struct ti_vtm_cfg2_regs *cfg2_regs = DEV_CFG2_REGS(config->vtm);

	LOG_DBG("Initializing VTM sensor (ID %d, VD %d)", config->sensor_id, config->vd_id);

	cfg1_regs->VD[config->vd_id].EVT_SET =
		FIELD_PREP(TI_VTM_CFG1_VD_EVT_SET_TSENS_EVT_SEL, BIT(config->sensor_id));
	cfg2_regs->TMPSENS[config->sensor_id].CTRL = TI_VTM_CFG2_TMPSENS_CTRL_CLRZ |
						     TI_VTM_CFG2_TMPSENS_CTRL_CONT |
						     TI_VTM_CFG2_TMPSENS_CTRL_MAXT_OUTRG_EN;

	if (!WAIT_FOR(cfg1_regs->TMPSENS[config->sensor_id].STAT &
			      TI_VTM_CFG1_TMPSENS_STAT_EOC_FC_UPDATE,
		      TI_VTM_EOC_TIMEOUT_US, k_busy_wait(10))) {
		LOG_ERR("Sensor %d: timeout waiting for first conversion", config->sensor_id);
		return -ETIMEDOUT;
	}

	LOG_DBG("VTM sensor %s (ID %d) initialized", sensor->name, config->sensor_id);
	return 0;
}

static int ti_vtm_init(const struct device *vtm)
{
	const struct ti_vtm_config *config = DEV_CFG(vtm);
	struct ti_vtm_cfg1_regs *cfg1_regs;
	struct ti_vtm_cfg2_regs *cfg2_regs;
	uint8_t hw_count;
	int thr;
	int thr0;

	DEVICE_MMIO_NAMED_MAP(vtm, cfg1, K_MEM_CACHE_NONE);
	DEVICE_MMIO_NAMED_MAP(vtm, cfg2, K_MEM_CACHE_NONE);

	cfg1_regs = DEV_CFG1_REGS(vtm);
	cfg2_regs = DEV_CFG2_REGS(vtm);

	hw_count = FIELD_GET(TI_VTM_DEVINFO_PWR0_TEMPSENS_CT, cfg1_regs->DEVINFO_PWR0);

	if (config->sensor_count > hw_count) {
		LOG_ERR("DT defines %d sensors but HW reports only %d", config->sensor_count,
			hw_count);
		return -ENODEV;
	}

	thr = ti_vtm_temp_to_adc_code(config->maxt_outrg_thresh_mc);
	thr0 = ti_vtm_temp_to_adc_code(config->maxt_outrg_thresh0_mc);

	if (thr < 0 || thr0 < 0) {
		LOG_ERR("Invalid maxt outrange threshold in DT");
		return -EINVAL;
	}

	cfg2_regs->MISC_CTRL2 &= ~(TI_VTM_CFG2_MISC_CTRL2_MAXT_OUTRG_ALERT_THR |
				   TI_VTM_CFG2_MISC_CTRL2_MAXT_OUTRG_ALERT_THR0);
	cfg2_regs->MISC_CTRL2 |= FIELD_PREP(TI_VTM_CFG2_MISC_CTRL2_MAXT_OUTRG_ALERT_THR, thr) |
				 FIELD_PREP(TI_VTM_CFG2_MISC_CTRL2_MAXT_OUTRG_ALERT_THR0, thr0);
	cfg2_regs->MISC_CTRL |= TI_VTM_CFG2_MISC_CTRL_ANYMAXT_OUTRG_ALERT_EN;

	config->irq_config();

	return 0;
}

static void ti_vtm_handle_isr(const struct device *vtm, uint32_t status,
			      enum ti_vtm_sensor_trigger_type trig_type)
{
	const struct ti_vtm_config *config = DEV_CFG(vtm);
	struct sensor_trigger trig = {
		.type = (enum sensor_trigger_type)trig_type,
		.chan = SENSOR_CHAN_DIE_TEMP,
	};

	for (int i = 0; i < config->sensor_count; i++) {
		const struct device *sensor_dev = config->sensor_devices[i];
		struct ti_vtm_sensor_data *sensor_data = SENSOR_DEV_DATA(sensor_dev);
		const struct ti_vtm_sensor_config *sensor_config = SENSOR_DEV_CFG(sensor_dev);
		sensor_trigger_handler_t handler = NULL;

		if (!(status & BIT(sensor_config->vd_id))) {
			continue;
		}

		switch (trig_type) {
		case TI_VTM_TRIG_TH0:
			handler = sensor_data->th0_handler;
			break;
		case TI_VTM_TRIG_TH1:
			handler = sensor_data->th1_handler;
			break;
		case TI_VTM_TRIG_TH2:
			handler = sensor_data->th2_handler;
			break;
		default:
			break;
		}

		if (handler) {
			handler(sensor_dev, &trig);
		}
	}
}

static void ti_vtm_th0_isr(const struct device *vtm)
{
	struct ti_vtm_cfg1_regs *cfg1_regs = DEV_CFG1_REGS(vtm);
	uint32_t status = cfg1_regs->LT_TH0_INT_EN_STAT_CLR;

	cfg1_regs->LT_TH0_INT_EN_STAT_CLR = status;
	ti_vtm_handle_isr(vtm, status, TI_VTM_TRIG_TH0);
}

static void ti_vtm_th1_isr(const struct device *vtm)
{
	struct ti_vtm_cfg1_regs *cfg1_regs = DEV_CFG1_REGS(vtm);
	uint32_t status = cfg1_regs->GT_TH1_INT_EN_STAT_CLR;

	cfg1_regs->GT_TH1_INT_EN_STAT_CLR = status;
	ti_vtm_handle_isr(vtm, status, TI_VTM_TRIG_TH1);
}

static void ti_vtm_th2_isr(const struct device *vtm)
{
	struct ti_vtm_cfg1_regs *cfg1_regs = DEV_CFG1_REGS(vtm);
	uint32_t status = cfg1_regs->GT_TH2_INT_EN_STAT_CLR;

	cfg1_regs->GT_TH2_INT_EN_STAT_CLR = status;
	ti_vtm_handle_isr(vtm, status, TI_VTM_TRIG_TH2);
}

#define TI_VTM_SENSOR_DEVICE(sensor_node)                                                          \
	BUILD_ASSERT(DT_REG_ADDR(sensor_node) < DT_CHILD_NUM_STATUS_OKAY(DT_PARENT(sensor_node)),  \
		     "VTM sensor reg out of range (must be 0..(sensor_count-1))");                 \
                                                                                                   \
	static struct ti_vtm_sensor_data ti_vtm_sensor_data_##sensor_node;                         \
                                                                                                   \
	static const struct ti_vtm_sensor_config ti_vtm_sensor_config_##sensor_node = {            \
		.vtm = DEVICE_DT_GET(DT_PARENT(sensor_node)),                                      \
		.sensor_id = DT_REG_ADDR(sensor_node),                                             \
		.vd_id = DT_PROP(sensor_node, ti_voltage_domain),                                  \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(sensor_node, ti_vtm_sensor_init, NULL, &ti_vtm_sensor_data_##sensor_node, \
			 &ti_vtm_sensor_config_##sensor_node, POST_KERNEL,                         \
			 CONFIG_SENSOR_INIT_PRIORITY, &ti_vtm_sensor_api);

#define TI_VTM_DEVICE(n)                                                                           \
	static void ti_vtm_irq_config_##n(void)                                                    \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, th0, irq),                                      \
			    DT_INST_IRQ_BY_NAME(n, th0, priority), ti_vtm_th0_isr,                 \
			    DEVICE_DT_INST_GET(n), DT_INST_IRQ_BY_NAME(n, th0, flags));            \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, th1, irq),                                      \
			    DT_INST_IRQ_BY_NAME(n, th1, priority), ti_vtm_th1_isr,                 \
			    DEVICE_DT_INST_GET(n), DT_INST_IRQ_BY_NAME(n, th1, flags));            \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, th2, irq),                                      \
			    DT_INST_IRQ_BY_NAME(n, th2, priority), ti_vtm_th2_isr,                 \
			    DEVICE_DT_INST_GET(n), DT_INST_IRQ_BY_NAME(n, th2, flags));            \
		irq_enable(DT_INST_IRQ_BY_NAME(n, th0, irq));                                      \
		irq_enable(DT_INST_IRQ_BY_NAME(n, th1, irq));                                      \
		irq_enable(DT_INST_IRQ_BY_NAME(n, th2, irq));                                      \
	}                                                                                          \
                                                                                                   \
	static const struct device *ti_vtm_sensors_##n[] = {                                       \
		DT_INST_FOREACH_CHILD_SEP(n, DEVICE_DT_GET, (,))};                                 \
                                                                                                   \
	static struct ti_vtm_data ti_vtm_data_##n;                                                 \
                                                                                                   \
	static const struct ti_vtm_config ti_vtm_config_##n = {                                    \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(cfg1, DT_DRV_INST(n)),                          \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(cfg2, DT_DRV_INST(n)),                          \
		.irq_config = ti_vtm_irq_config_##n,                                               \
		.sensor_devices = ti_vtm_sensors_##n,                                              \
		.sensor_count = ARRAY_SIZE(ti_vtm_sensors_##n),                                    \
		.maxt_outrg_thresh_mc = DT_INST_PROP(n, ti_maxt_outrg_thresh),                     \
		.maxt_outrg_thresh0_mc = DT_INST_PROP(n, ti_maxt_outrg_thresh0),                   \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, ti_vtm_init, NULL, &ti_vtm_data_##n, &ti_vtm_config_##n,          \
			      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, NULL);                     \
                                                                                                   \
	DT_INST_FOREACH_CHILD(n, TI_VTM_SENSOR_DEVICE)

DT_INST_FOREACH_STATUS_OKAY(TI_VTM_DEVICE)
