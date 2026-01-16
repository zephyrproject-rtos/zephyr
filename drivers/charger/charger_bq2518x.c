/*
 * Copyright 2024 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * BQ25180 Datasheet: https://www.ti.com/lit/gpn/bq25180
 * BQ25186 Datasheet: https://www.ti.com/lit/gpn/bq25186
 * BQ25187 Datasheet: https://www.ti.com/lit/gpn/bq25188
 *
 * Notable Differences:
 *    BQ25180 CHARGE_CTRL0: VINDPM lowest value is 4.2V,
 *                          compared to VBAT + 300 mV for
 *                          other two.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/charger.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(bq2518x, CONFIG_CHARGER_LOG_LEVEL);

enum bq2518x_reg {
	BQ2518X_STAT0 = 0x00,
	BQ2518X_STAT1 = 0x01,
	BQ2518X_FLAG0 = 0x02,
	BQ2518X_VBAT_CTRL = 0x03,
	BQ2518X_ICHG_CTRL = 0x04,
	BQ2518X_CHARGE_CTRL0 = 0x05,
	BQ2518X_CHARGE_CTRL1 = 0x06,
	BQ2518X_IC_CTRL = 0x07,
	BQ2518X_TMR_ILIM = 0x08,
	BQ2518X_SHIP_RST = 0x09,
	BQ2518X_SYS_REG = 0x0A,
	BQ2518X_TS_CONTROL = 0x0B,
	BQ2518X_MASK_ID = 0x0c,
};

enum bq2518x_device_id {
	BQ25180_DEVICE_ID = 0x00,
	BQ25186_DEVICE_ID = 0x01,
	BQ25188_DEVICE_ID = 0x04,
};

#define BQ2518X_STAT0_CHG_STAT_MASK              GENMASK(6, 5)
#define BQ2518X_STAT0_CHG_STAT_NOT_CHARGING      0x00
#define BQ2518X_STAT0_CHG_STAT_CONSTANT_CURRENT  0x01
#define BQ2518X_STAT0_CHG_STAT_CONSTANT_VOLTAGE  0x02
#define BQ2518X_STAT0_CHG_STAT_DONE              0x03
#define BQ2518X_STAT0_VIN_PGOOD_STAT             BIT(0)
#define BQ2518X_VBAT_MSK                         GENMASK(6, 0)
#define BQ2518X_ICHG_CHG_DIS                     BIT(7)
#define BQ2518X_ICHG_MSK                         GENMASK(6, 0)
#define BQ2518X_CHARGE_CTRL1_DISCHARGE_OFFSET    6
#define BQ2518X_CHARGE_CTRL1_UNDERVOLTAGE_OFFSET 3
#define BQ2518X_CHARGE_CTRL1_CHG_STATUS_INT_MASK BIT(2)
#define BQ2518X_CHARGE_CTRL1_ILIM_INT_MASK       BIT(1)
#define BQ2518X_CHARGE_CTRL1_VINDPM_INT_MASK     BIT(0)
#define BQ2518X_IC_CTRL_WDOG_DISABLE             (BIT(0) | BIT(1))
#define BQ2518X_IC_CTRL_SAFETY_6_HOUR            BIT(2)
#define BQ2518X_IC_CTRL_VRCH_100                 0x00
#define BQ2518X_IC_CTRL_VRCH_200                 BIT(5)
#define BQ2518X_IC_CTRL_VLOWV_SEL_2_8            BIT(6)
#define BQ2518X_IC_CTRL_VLOWV_SEL_3_0            0x00
#define BQ2518X_IC_CTRL_TS_AUTO_EN               BIT(7)
#define BQ2518X_IC_CTRL_TS_AUTO_DIS              0x00
#define BQ2518X_SYS_REG_CTRL_OFFSET              5
#define BQ2518X_DEVICE_ID_MSK                    GENMASK(3, 0)
#define BQ2518X_DEVICE_ID                        0x00
#define BQ2518X_SHIP_RST_EN_RST_SHIP_MSK         GENMASK(6, 5)
#define BQ2518X_SHIP_RST_EN_RST_SHIP_ADAPTER     0x20
#define BQ2518X_SHIP_RST_EN_RST_SHIP_BUTTON      0x40

/* Charging current limits */
#define BQ2518X_CURRENT_MIN_MA 5
#define BQ2518X_CURRENT_MAX_MA 1000
#define BQ2518X_VOLTAGE_MIN_MV 3500
#define BQ2518X_VOLTAGE_MAX_MV 4650

#define BQ2518X_FACTOR_VBAT_TO_MV 10

struct bq2518x_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec int_gpio;
	uint32_t initial_current_microamp;
	uint32_t max_voltage_microvolt;
	enum bq2518x_device_id device_id;
	uint8_t reg_ic_ctrl;
	uint8_t reg_charge_control1;
	uint8_t reg_sys_regulation;
};

struct bq2518x_data {
	const struct bq2518x_config *config;
	struct gpio_callback int_cb;
	struct k_work int_work;
	charger_status_notifier_t charger_status_notifier;
	charger_online_notifier_t charger_online_notifier;
	uint8_t last_stat0;
};

/*
 * For ICHG <= 35mA = ICHGCODE + 5mA
 * For ICHG > 35mA = 40 + ((ICHGCODE-31)*10)mA.
 * Maximum programmable current = 1000mA
 *
 * Return: value between 0 and 127, negative on error.
 */
static int bq2518x_ma_to_ichg(uint32_t current_ma, uint8_t *ichg)
{
	if (!IN_RANGE(current_ma, BQ2518X_CURRENT_MIN_MA, BQ2518X_CURRENT_MAX_MA)) {
		LOG_WRN("charging current out of range: %dmA, "
			"clamping to the nearest limit",
			current_ma);
	}
	current_ma = CLAMP(current_ma, BQ2518X_CURRENT_MIN_MA, BQ2518X_CURRENT_MAX_MA);

	if (current_ma <= 35) {
		*ichg = current_ma - 5;
		return 0;
	}

	*ichg = (current_ma - 40) / 10 + 31;

	return 0;
}

static uint32_t bq2518x_ichg_to_ma(uint8_t ichg)
{
	ichg &= BQ2518X_ICHG_MSK;

	if (ichg <= 30) {
		return (ichg + 5);
	}

	return (ichg - 31) * 10 + 40;
}

static int bq2518x_mv_to_vbatreg(const struct bq2518x_config *cfg, uint32_t voltage_mv,
				 uint8_t *vbat)
{
	if (!IN_RANGE(voltage_mv, BQ2518X_VOLTAGE_MIN_MV, BQ2518X_VOLTAGE_MAX_MV)) {
		LOG_WRN("charging voltage out of range: %dmV, "
			"clamping to the nearest limit",
			voltage_mv);
	}
	voltage_mv = CLAMP(voltage_mv, BQ2518X_VOLTAGE_MIN_MV, BQ2518X_VOLTAGE_MAX_MV);

	*vbat = (voltage_mv - BQ2518X_VOLTAGE_MIN_MV) / BQ2518X_FACTOR_VBAT_TO_MV;

	return 0;
}

static uint32_t bq2518x_vbatreg_to_mv(uint8_t vbat)
{
	vbat &= BQ2518X_VBAT_MSK;

	return (vbat * BQ2518X_FACTOR_VBAT_TO_MV) + BQ2518X_VOLTAGE_MIN_MV;
}

static int bq2518x_charge_enable(const struct device *dev, const bool enable)
{
	const struct bq2518x_config *cfg = dev->config;
	uint8_t value = enable ? 0 : BQ2518X_ICHG_CHG_DIS;
	int ret;

	ret = i2c_reg_update_byte_dt(&cfg->i2c, BQ2518X_ICHG_CTRL, BQ2518X_ICHG_CHG_DIS, value);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int bq2518x_set_charge_current(const struct device *dev, uint32_t const_charge_current_ua)
{
	const struct bq2518x_config *cfg = dev->config;
	uint8_t val;
	int ret;

	ret = bq2518x_ma_to_ichg(const_charge_current_ua / 1000, &val);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_reg_update_byte_dt(&cfg->i2c, BQ2518X_ICHG_CTRL, BQ2518X_ICHG_MSK, val);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int bq2518x_get_charge_current(const struct device *dev, uint32_t *const_charge_current_ua)
{
	const struct bq2518x_config *cfg = dev->config;
	uint8_t val;
	int ret;

	ret = i2c_reg_read_byte_dt(&cfg->i2c, BQ2518X_ICHG_CTRL, &val);
	if (ret < 0) {
		return ret;
	}

	*const_charge_current_ua = bq2518x_ichg_to_ma(val) * 1000;

	return 0;
}

static int bq2518x_set_charge_voltage(const struct device *dev, uint32_t const_charge_voltage_uv)
{
	const struct bq2518x_config *cfg = dev->config;
	uint8_t val;
	int ret;

	ret = bq2518x_mv_to_vbatreg(cfg, const_charge_voltage_uv / 1000, &val);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_reg_update_byte_dt(&cfg->i2c, BQ2518X_VBAT_CTRL, BQ2518X_VBAT_MSK, val);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int bq2518x_get_charge_voltage(const struct device *dev, uint32_t *const_charge_voltage_uv)
{
	const struct bq2518x_config *cfg = dev->config;
	uint8_t val;
	int ret;

	ret = i2c_reg_read_byte_dt(&cfg->i2c, BQ2518X_VBAT_CTRL, &val);
	if (ret < 0) {
		return ret;
	}

	*const_charge_voltage_uv = bq2518x_vbatreg_to_mv(val) * 1000;

	return 0;
}

static int bq2518x_get_online(const struct device *dev, enum charger_online *online)
{
	const struct bq2518x_config *cfg = dev->config;
	uint8_t val;
	int ret;

	ret = i2c_reg_read_byte_dt(&cfg->i2c, BQ2518X_STAT0, &val);
	if (ret < 0) {
		return ret;
	}

	if ((val & BQ2518X_STAT0_VIN_PGOOD_STAT) != 0x00) {
		*online = CHARGER_ONLINE_FIXED;
	} else {
		*online = CHARGER_ONLINE_OFFLINE;
	}

	return 0;
}

static enum charger_status status_decode(uint8_t stat0, bool charging_disabled)
{
	if ((stat0 & BQ2518X_STAT0_VIN_PGOOD_STAT) == 0x00) {
		/* No input power, must be discharging */
		return CHARGER_STATUS_DISCHARGING;
	}
	if (charging_disabled) {
		/* Input present, but charging disabled */
		return CHARGER_STATUS_NOT_CHARGING;
	}

	switch (FIELD_GET(BQ2518X_STAT0_CHG_STAT_MASK, stat0)) {
	case BQ2518X_STAT0_CHG_STAT_NOT_CHARGING:
		return CHARGER_STATUS_NOT_CHARGING;
	case BQ2518X_STAT0_CHG_STAT_CONSTANT_CURRENT:
	case BQ2518X_STAT0_CHG_STAT_CONSTANT_VOLTAGE:
		return CHARGER_STATUS_CHARGING;
	case BQ2518X_STAT0_CHG_STAT_DONE:
		return CHARGER_STATUS_FULL;
	}
	/* Unreachable */
	return CHARGER_STATUS_UNKNOWN;
}

static int bq2518x_get_status(const struct device *dev, enum charger_status *status)
{
	const struct bq2518x_config *cfg = dev->config;
	uint8_t stat0;
	uint8_t ichg_ctrl;
	int ret;

	ret = i2c_reg_read_byte_dt(&cfg->i2c, BQ2518X_STAT0, &stat0);
	if (ret < 0) {
		return ret;
	}
	ret = i2c_reg_read_byte_dt(&cfg->i2c, BQ2518X_ICHG_CTRL, &ichg_ctrl);
	if (ret < 0) {
		return ret;
	}

	*status = status_decode(stat0, ichg_ctrl & BQ2518X_ICHG_CHG_DIS);
	return 0;
}

static void bq2518x_int_handle(struct k_work *work)
{
	struct bq2518x_data *data = CONTAINER_OF(work, struct bq2518x_data, int_work);
	uint8_t stat0_change;
	uint8_t stat0;
	int ret;

	/* All the useful information is in STAT0 */
	ret = i2c_reg_read_byte_dt(&data->config->i2c, BQ2518X_STAT0, &stat0);
	if (ret < 0) {
		return;
	}

	/* Bits that have changed */
	stat0_change = data->last_stat0 ^ stat0;
	LOG_DBG("Changed bits: %02X", stat0_change);

	if ((stat0_change & BQ2518X_STAT0_VIN_PGOOD_STAT) && (data->charger_online_notifier)) {
		enum charger_online online = stat0 & BQ2518X_STAT0_VIN_PGOOD_STAT
						     ? CHARGER_ONLINE_FIXED
						     : CHARGER_ONLINE_OFFLINE;
		LOG_INF("Online status: %d", online);
		data->charger_online_notifier(online);
	}
	if ((stat0_change & BQ2518X_STAT0_CHG_STAT_MASK) && (data->charger_status_notifier)) {
		enum charger_status status = status_decode(stat0, false);

		LOG_INF("Charging status: %d", status);
		data->charger_status_notifier(status);
	}
	data->last_stat0 = stat0;
}

static void bq2518x_gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct bq2518x_data *data = CONTAINER_OF(cb, struct bq2518x_data, int_cb);

	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	LOG_DBG("");

	/* Schedule work to handle the interrupt */
	k_work_submit(&data->int_work);
}

static int bq2518x_get_prop(const struct device *dev, charger_prop_t prop,
			    union charger_propval *val)
{
	switch (prop) {
	case CHARGER_PROP_ONLINE:
		return bq2518x_get_online(dev, &val->online);
	case CHARGER_PROP_STATUS:
		return bq2518x_get_status(dev, &val->status);
	case CHARGER_PROP_CONSTANT_CHARGE_CURRENT_UA:
		return bq2518x_get_charge_current(dev, &val->const_charge_current_ua);
	case CHARGER_PROP_CONSTANT_CHARGE_VOLTAGE_UV:
		return bq2518x_get_charge_voltage(dev, &val->const_charge_voltage_uv);
	default:
		return -ENOTSUP;
	}
}

static int bq2518x_set_prop(const struct device *dev, charger_prop_t prop,
			    const union charger_propval *val)
{
	const struct bq2518x_config *cfg = dev->config;
	struct bq2518x_data *data = dev->data;

	switch (prop) {
	case CHARGER_PROP_CONSTANT_CHARGE_CURRENT_UA:
		return bq2518x_set_charge_current(dev, val->const_charge_current_ua);
	case CHARGER_PROP_CONSTANT_CHARGE_VOLTAGE_UV:
		return bq2518x_set_charge_voltage(dev, val->const_charge_voltage_uv);
	case CHARGER_PROP_STATUS_NOTIFICATION:
		if (cfg->int_gpio.port == NULL) {
			return -ENOTSUP;
		}
		data->charger_status_notifier = val->status_notification;
		return 0;
	case CHARGER_PROP_ONLINE_NOTIFICATION:
		if (cfg->int_gpio.port == NULL) {
			return -ENOTSUP;
		}
		data->charger_online_notifier = val->online_notification;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static DEVICE_API(charger, bq2518x_api) = {
	.get_property = bq2518x_get_prop,
	.set_property = bq2518x_set_prop,
	.charge_enable = bq2518x_charge_enable,
};

static int bq2518x_init(const struct device *dev)
{
	const struct bq2518x_config *cfg = dev->config;
	struct bq2518x_data *data = dev->data;
	uint8_t val;
	int ret;

	ret = i2c_reg_read_byte_dt(&cfg->i2c, BQ2518X_MASK_ID, &val);
	if (ret < 0) {
		return ret;
	}

	val &= BQ2518X_DEVICE_ID_MSK;
	if (val != cfg->device_id) {
		LOG_ERR("Invalid device id: %02x", val);
		return -EINVAL;
	}

	/* Setup register IC_CTRL.
	 * Values from devicetree + device defaults
	 */
	val = BQ2518X_IC_CTRL_WDOG_DISABLE | BQ2518X_IC_CTRL_SAFETY_6_HOUR | cfg->reg_ic_ctrl;
	ret = i2c_reg_write_byte_dt(&cfg->i2c, BQ2518X_IC_CTRL, val);
	if (ret < 0) {
		return ret;
	}

	/* Setup VSYS regulation */
	ret = i2c_reg_write_byte_dt(&cfg->i2c, BQ2518X_SYS_REG, cfg->reg_sys_regulation);
	if (ret < 0) {
		return ret;
	}

	/* Setup battery discharge limits */
	ret = i2c_reg_write_byte_dt(&cfg->i2c, BQ2518X_CHARGE_CTRL1, cfg->reg_charge_control1);
	if (ret < 0) {
		return ret;
	}

	ret = bq2518x_set_charge_voltage(dev, cfg->max_voltage_microvolt);
	if (ret < 0) {
		LOG_ERR("Could not set the target voltage. (rc: %d)", ret);
		return ret;
	}

	if (cfg->initial_current_microamp > 0) {
		ret = bq2518x_set_charge_current(dev, cfg->initial_current_microamp);
		if (ret < 0) {
			return ret;
		}
	}

	if (cfg->int_gpio.port) {
		/* Configure the interrupts (if pin provided)  */
		data->config = cfg;
		k_work_init(&data->int_work, bq2518x_int_handle);
		gpio_init_callback(&data->int_cb, bq2518x_gpio_callback, BIT(cfg->int_gpio.pin));

		ret = gpio_pin_configure_dt(&cfg->int_gpio, GPIO_INPUT);
		if (ret < 0) {
			return ret;
		}
		ret = gpio_add_callback(cfg->int_gpio.port, &data->int_cb);
		if (ret < 0) {
			return ret;
		}
		ret = gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	}

	return ret;
}

#define CHARGER_BQ2518X_INIT(inst, _device_id)                                                     \
	static const struct bq2518x_config _device_id##_config_##inst = {                          \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),                        \
		.initial_current_microamp =                                                        \
			DT_INST_PROP(inst, constant_charge_current_max_microamp),                  \
		.max_voltage_microvolt =                                                           \
			DT_INST_PROP(inst, constant_charge_voltage_max_microvolt),                 \
		.device_id = _device_id##_DEVICE_ID,                                               \
		.reg_ic_ctrl =                                                                     \
			(DT_INST_PROP(inst, re_charge_threshold_millivolt) == 100                  \
				 ? BQ2518X_IC_CTRL_VRCH_100                                        \
				 : BQ2518X_IC_CTRL_VRCH_200) |                                     \
			(DT_INST_PROP(inst, precharge_voltage_threshold_microvolt) == 2800000      \
				 ? BQ2518X_IC_CTRL_VLOWV_SEL_2_8                                   \
				 : BQ2518X_IC_CTRL_VLOWV_SEL_3_0) |                                \
			(DT_INST_PROP_OR(inst, ntc_charger_control_disable, 0)                     \
				 ? BQ2518X_IC_CTRL_TS_AUTO_DIS                                     \
				 : BQ2518X_IC_CTRL_TS_AUTO_EN),                                    \
		.reg_charge_control1 =                                                             \
			(DT_INST_ENUM_IDX(inst, battery_discharge_current_limit_milliamp)          \
			 << BQ2518X_CHARGE_CTRL1_DISCHARGE_OFFSET) |                               \
			((DT_INST_ENUM_IDX(inst, battery_undervoltage_lockout_millivolt) + 2)      \
			 << BQ2518X_CHARGE_CTRL1_UNDERVOLTAGE_OFFSET) |                            \
			BQ2518X_CHARGE_CTRL1_ILIM_INT_MASK | BQ2518X_CHARGE_CTRL1_VINDPM_INT_MASK, \
		.reg_sys_regulation = DT_INST_ENUM_IDX(inst, vsys_target_regulation)               \
				      << BQ2518X_SYS_REG_CTRL_OFFSET,                              \
	};                                                                                         \
	static struct bq2518x_data _device_id##_data_##inst;                                       \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, bq2518x_init, NULL, &_device_id##_data_##inst,                 \
			      &_device_id##_config_##inst, POST_KERNEL,                            \
			      CONFIG_CHARGER_INIT_PRIORITY, &bq2518x_api);

#define DT_DRV_COMPAT ti_bq25180
DT_INST_FOREACH_STATUS_OKAY_VARGS(CHARGER_BQ2518X_INIT, BQ25180)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT ti_bq25186
DT_INST_FOREACH_STATUS_OKAY_VARGS(CHARGER_BQ2518X_INIT, BQ25186)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT ti_bq25188
DT_INST_FOREACH_STATUS_OKAY_VARGS(CHARGER_BQ2518X_INIT, BQ25188)
#undef DT_DRV_COMPAT
