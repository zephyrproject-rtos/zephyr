/*
 * Copyright (c) 2025 Linumiz
 * Copyright (c) 2026 Testo SE & Co. KGaA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_bq25620_charger

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/charger.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/mfd/bq25620.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "mfd_bq25620.h"

LOG_MODULE_REGISTER(charger_bq25620, CONFIG_CHARGER_LOG_LEVEL);

#define BQ25620_CE_GPIO DT_ANY_INST_HAS_PROP_STATUS_OKAY(ce_gpios)

/* Power-on reset values, used for optional properties that are not set */
#define BQ25620_IPRECHG_POR_UA 100000
#define BQ25620_ITERM_POR_UA   60000
#define BQ25620_VINDPM_POR_UV  4600000
#define BQ25620_VSYSMIN_POR_UV 3520000

/* Enum indexes of the power-on reset values in the binding */
#define BQ25620_VRECHG_POR           0 /* 100 mV */
#define BQ25620_VINDPM_BAT_TRACK_POR 1 /* 400 mV */
#define BQ25620_PRECHG_TMR_POR       0 /* 2.5 h */
#define BQ25620_CHG_TMR_POR          0 /* 14.5 h */
#define BQ25620_VBUS_OVP_POR         1 /* 18.5 V */
#define BQ25620_IBAT_PK_POR          1 /* 12 A */
#define BQ25620_CHG_RATE_POR         0 /* 1C */
#define BQ25620_TREG_POR             1 /* 120 C */
#define BQ25620_CONV_FREQ_POR        0 /* 1.5 MHz */
#define BQ25620_CONV_STRN_POR        2 /* strong */

#define BQ25620_EVENTS (MFD_BQ25620_EVENT_VBUS | MFD_BQ25620_EVENT_CHG)

/* 16-bit limit register, the value is (field * step) */
struct bq25620_limit {
	uint8_t reg;
	uint8_t shift;
	uint16_t mask;
	uint32_t step;
	uint32_t min;
	uint32_t max;
};

#define BQ25620_LIMIT(name, unit)                                                                  \
	{                                                                                          \
		.reg = BQ25620_REG_##name,                                                         \
		.shift = BQ25620_##name##_SHIFT,                                                   \
		.mask = BQ25620_##name##_MASK,                                                     \
		.step = BQ25620_##name##_STEP_##unit,                                              \
		.min = BQ25620_##name##_MIN_##unit,                                                \
		.max = BQ25620_##name##_MAX_##unit,                                                \
	}

static const struct bq25620_limit bq25620_ichg = BQ25620_LIMIT(ICHG, UA);
static const struct bq25620_limit bq25620_vreg = BQ25620_LIMIT(VREG, UV);
static const struct bq25620_limit bq25620_iindpm = BQ25620_LIMIT(IINDPM, UA);
static const struct bq25620_limit bq25620_vindpm = BQ25620_LIMIT(VINDPM, UV);
static const struct bq25620_limit bq25620_vsysmin = BQ25620_LIMIT(VSYSMIN, UV);
static const struct bq25620_limit bq25620_iprechg = BQ25620_LIMIT(IPRECHG, UA);
static const struct bq25620_limit bq25620_iterm = BQ25620_LIMIT(ITERM, UA);

struct bq25620_charger_config {
	struct i2c_dt_spec i2c;
	const struct device *mfd;
	struct gpio_dt_spec ce_gpio;
	uint32_t ichg_max_ua;
	uint32_t vreg_max_uv;
	uint32_t iprechg_ua;
	uint32_t iterm_ua;
	/* 0 if not set */
	uint32_t iindpm_ua;
	uint32_t vindpm_uv;
	uint32_t vsysmin_uv;
	/* Enum indexes of the binding */
	uint8_t vrechg;
	uint8_t vindpm_bat_track;
	uint8_t prechg_tmr;
	uint8_t chg_tmr;
	uint8_t vbus_ovp;
	uint8_t ibat_pk;
	uint8_t chg_rate;
	uint8_t treg;
	uint8_t conv_freq;
	uint8_t conv_strn;
	bool q1_fullon;
	bool q4_fullon;
	bool disable_safety_tmrs;
	bool disable_tmr2x;
	bool disable_auto_ibatdis;
	bool ts_ignore;
};

struct bq25620_charger_data {
	const struct device *dev;
	struct mfd_bq25620_callback cb;
	charger_status_notifier_t status_notifier;
	charger_online_notifier_t online_notifier;
	bool has_interrupt;
	bool ce_enabled;
};

static int bq25620_get_limit(const struct device *dev, const struct bq25620_limit *limit,
			     uint32_t *value)
{
	const struct bq25620_charger_config *config = dev->config;
	uint8_t buf[2];
	int ret;

	ret = i2c_burst_read_dt(&config->i2c, limit->reg, buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}

	*value = ((sys_get_le16(buf) & limit->mask) >> limit->shift) * limit->step;

	return 0;
}

static int bq25620_set_limit(const struct device *dev, const struct bq25620_limit *limit,
			     uint32_t max, uint32_t value)
{
	const struct bq25620_charger_config *config = dev->config;
	uint8_t buf[2];
	uint16_t reg;
	int ret;

	if (!IN_RANGE(value, limit->min, max)) {
		return -EINVAL;
	}

	ret = i2c_burst_read_dt(&config->i2c, limit->reg, buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}

	reg = sys_get_le16(buf) & ~limit->mask;
	reg |= ((value / limit->step) << limit->shift) & limit->mask;
	sys_put_le16(reg, buf);

	return i2c_burst_write_dt(&config->i2c, limit->reg, buf, sizeof(buf));
}

static int bq25620_charge_enable(const struct device *dev, const bool enable)
{
	const struct bq25620_charger_config *config = dev->config;
	struct bq25620_charger_data *data = dev->data;
	int ret;

	ret = i2c_reg_update_byte_dt(&config->i2c, BQ25620_REG_CHG_CTRL_1,
				     BQ25620_CHG_CTRL_1_EN_CHG,
				     enable ? BQ25620_CHG_CTRL_1_EN_CHG : 0);
	if (ret < 0) {
		return ret;
	}

#if BQ25620_CE_GPIO
	if (config->ce_gpio.port != NULL) {
		ret = gpio_pin_set_dt(&config->ce_gpio, enable ? 1 : 0);
		if (ret < 0) {
			return ret;
		}
	}
#endif

	data->ce_enabled = enable;

	return 0;
}

static int bq25620_is_charge_enabled(const struct device *dev, bool *enabled)
{
	const struct bq25620_charger_config *config = dev->config;
	struct bq25620_charger_data *data = dev->data;
	uint8_t val;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, BQ25620_REG_CHG_CTRL_1, &val);
	if (ret < 0) {
		return ret;
	}

	*enabled = ((val & BQ25620_CHG_CTRL_1_EN_CHG) != 0U) && data->ce_enabled;

	return 0;
}

static bool bq25620_vbus_present(uint8_t chg_stat_1)
{
	uint8_t vbus_stat = FIELD_GET(BQ25620_CHG_STAT_1_VBUS_STAT, chg_stat_1);

	/*
	 * VBUS_STAT reports the result of the USB adapter detection, which is
	 * enabled by default. 0 means that no qualified adapter is present.
	 */
	return (vbus_stat != BQ25620_VBUS_STAT_NONE) && (vbus_stat != BQ25620_VBUS_STAT_OTG);
}

static int bq25620_get_online(const struct device *dev, enum charger_online *online)
{
	const struct bq25620_charger_config *config = dev->config;
	uint8_t val;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, BQ25620_REG_CHG_STAT_1, &val);
	if (ret < 0) {
		return ret;
	}

	*online = bq25620_vbus_present(val) ? CHARGER_ONLINE_FIXED : CHARGER_ONLINE_OFFLINE;

	return 0;
}

static int bq25620_get_status(const struct device *dev, enum charger_status *status)
{
	const struct bq25620_charger_config *config = dev->config;
	bool enabled;
	uint8_t val;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, BQ25620_REG_CHG_STAT_1, &val);
	if (ret < 0) {
		return ret;
	}

	if (!bq25620_vbus_present(val)) {
		*status = CHARGER_STATUS_DISCHARGING;
		return 0;
	}

	ret = bq25620_is_charge_enabled(dev, &enabled);
	if (ret < 0) {
		return ret;
	}

	if (!enabled ||
	    (FIELD_GET(BQ25620_CHG_STAT_1_CHG_STAT, val) == BQ25620_CHG_STAT_NOT_CHARGING)) {
		*status = CHARGER_STATUS_NOT_CHARGING;
	} else {
		*status = CHARGER_STATUS_CHARGING;
	}

	return 0;
}

static int bq25620_get_charge_type(const struct device *dev, enum charger_charge_type *type)
{
	const struct bq25620_charger_config *config = dev->config;
	bool enabled;
	uint8_t val;
	int ret;

	ret = bq25620_is_charge_enabled(dev, &enabled);
	if (ret < 0) {
		return ret;
	}

	if (!enabled) {
		*type = CHARGER_CHARGE_TYPE_NONE;
		return 0;
	}

	ret = i2c_reg_read_byte_dt(&config->i2c, BQ25620_REG_CHG_STAT_1, &val);
	if (ret < 0) {
		return ret;
	}

	switch (FIELD_GET(BQ25620_CHG_STAT_1_CHG_STAT, val)) {
	case BQ25620_CHG_STAT_CC:
		/* Trickle charge, pre-charge or fast charge in CC mode */
		*type = CHARGER_CHARGE_TYPE_FAST;
		break;
	case BQ25620_CHG_STAT_CV:
		*type = CHARGER_CHARGE_TYPE_STANDARD;
		break;
	case BQ25620_CHG_STAT_TOP_OFF:
		*type = CHARGER_CHARGE_TYPE_TRICKLE;
		break;
	default:
		*type = CHARGER_CHARGE_TYPE_NONE;
		break;
	}

	return 0;
}

static enum charger_health bq25620_ts_health(uint8_t ts_stat)
{
	switch (ts_stat) {
	case BQ25620_TS_STAT_NORMAL:
		return CHARGER_HEALTH_GOOD;
	case BQ25620_TS_STAT_COLD:
		return CHARGER_HEALTH_COLD;
	case BQ25620_TS_STAT_HOT:
		return CHARGER_HEALTH_HOT;
	case BQ25620_TS_STAT_COOL:
	case BQ25620_TS_STAT_PRECOOL:
		return CHARGER_HEALTH_COOL;
	case BQ25620_TS_STAT_WARM:
	case BQ25620_TS_STAT_PREWARM:
		return CHARGER_HEALTH_WARM;
	default:
		/* TS pin bias reference fault */
		return CHARGER_HEALTH_UNSPEC_FAILURE;
	}
}

static int bq25620_get_health(const struct device *dev, enum charger_health *health)
{
	const struct bq25620_charger_config *config = dev->config;
	uint8_t chg_stat_0;
	uint8_t fault;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, BQ25620_REG_CHG_STAT_0, &chg_stat_0);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_reg_read_byte_dt(&config->i2c, BQ25620_REG_FAULT_STAT_0, &fault);
	if (ret < 0) {
		return ret;
	}

	if ((fault & BQ25620_FAULT_STAT_0_TSHUT) != 0U) {
		*health = CHARGER_HEALTH_OVERHEAT;
	} else if ((fault & BQ25620_FAULT_STAT_0_BAT_FAULT) != 0U) {
		/* Battery over voltage or over current protection */
		*health = CHARGER_HEALTH_OVERVOLTAGE;
	} else if ((fault & (BQ25620_FAULT_STAT_0_VBUS_FAULT | BQ25620_FAULT_STAT_0_SYS_FAULT |
			     BQ25620_FAULT_STAT_0_OTG_FAULT)) != 0U) {
		*health = CHARGER_HEALTH_UNSPEC_FAILURE;
	} else if ((chg_stat_0 & BQ25620_CHG_STAT_0_SAFETY_TMR_STAT) != 0U) {
		*health = CHARGER_HEALTH_SAFETY_TIMER_EXPIRE;
	} else if ((chg_stat_0 & BQ25620_CHG_STAT_0_WD_STAT) != 0U) {
		*health = CHARGER_HEALTH_WATCHDOG_TIMER_EXPIRE;
	} else {
		*health = bq25620_ts_health(FIELD_GET(BQ25620_FAULT_STAT_0_TS_STAT, fault));
	}

	return 0;
}

static int bq25620_get_prop(const struct device *dev, const charger_prop_t prop,
			    union charger_propval *val)
{
	switch (prop) {
	case CHARGER_PROP_ONLINE:
		return bq25620_get_online(dev, &val->online);
	case CHARGER_PROP_STATUS:
		return bq25620_get_status(dev, &val->status);
	case CHARGER_PROP_CHARGE_TYPE:
		return bq25620_get_charge_type(dev, &val->charge_type);
	case CHARGER_PROP_HEALTH:
		return bq25620_get_health(dev, &val->health);
	case CHARGER_PROP_CONSTANT_CHARGE_CURRENT_UA:
		return bq25620_get_limit(dev, &bq25620_ichg, &val->const_charge_current_ua);
	case CHARGER_PROP_CONSTANT_CHARGE_VOLTAGE_UV:
		return bq25620_get_limit(dev, &bq25620_vreg, &val->const_charge_voltage_uv);
	case CHARGER_PROP_PRECHARGE_CURRENT_UA:
		return bq25620_get_limit(dev, &bq25620_iprechg, &val->precharge_current_ua);
	case CHARGER_PROP_CHARGE_TERM_CURRENT_UA:
		return bq25620_get_limit(dev, &bq25620_iterm, &val->charge_term_current_ua);
	case CHARGER_PROP_INPUT_REGULATION_CURRENT_UA:
		return bq25620_get_limit(dev, &bq25620_iindpm,
					 &val->input_current_regulation_current_ua);
	case CHARGER_PROP_INPUT_REGULATION_VOLTAGE_UV:
		return bq25620_get_limit(dev, &bq25620_vindpm,
					 &val->input_voltage_regulation_voltage_uv);
	default:
		return -ENOTSUP;
	}
}

static int bq25620_set_prop(const struct device *dev, const charger_prop_t prop,
			    const union charger_propval *val)
{
	const struct bq25620_charger_config *config = dev->config;
	struct bq25620_charger_data *data = dev->data;

	switch (prop) {
	case CHARGER_PROP_CONSTANT_CHARGE_CURRENT_UA:
		return bq25620_set_limit(dev, &bq25620_ichg, config->ichg_max_ua,
					 val->const_charge_current_ua);
	case CHARGER_PROP_CONSTANT_CHARGE_VOLTAGE_UV:
		return bq25620_set_limit(dev, &bq25620_vreg, config->vreg_max_uv,
					 val->const_charge_voltage_uv);
	case CHARGER_PROP_PRECHARGE_CURRENT_UA:
		return bq25620_set_limit(dev, &bq25620_iprechg, bq25620_iprechg.max,
					 val->precharge_current_ua);
	case CHARGER_PROP_CHARGE_TERM_CURRENT_UA:
		return bq25620_set_limit(dev, &bq25620_iterm, bq25620_iterm.max,
					 val->charge_term_current_ua);
	case CHARGER_PROP_INPUT_REGULATION_CURRENT_UA:
		return bq25620_set_limit(dev, &bq25620_iindpm, bq25620_iindpm.max,
					 val->input_current_regulation_current_ua);
	case CHARGER_PROP_INPUT_REGULATION_VOLTAGE_UV:
		return bq25620_set_limit(dev, &bq25620_vindpm, bq25620_vindpm.max,
					 val->input_voltage_regulation_voltage_uv);
	case CHARGER_PROP_STATUS_NOTIFICATION:
		if (!data->has_interrupt) {
			return -ENOTSUP;
		}
		data->status_notifier = val->status_notification;
		return 0;
	case CHARGER_PROP_ONLINE_NOTIFICATION:
		if (!data->has_interrupt) {
			return -ENOTSUP;
		}
		data->online_notifier = val->online_notification;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static void bq25620_event_handler(const struct device *mfd, struct mfd_bq25620_callback *cb,
				  uint32_t events)
{
	struct bq25620_charger_data *data = CONTAINER_OF(cb, struct bq25620_charger_data, cb);
	charger_status_notifier_t status_notifier = data->status_notifier;
	charger_online_notifier_t online_notifier = data->online_notifier;
	enum charger_status status;
	enum charger_online online;

	ARG_UNUSED(mfd);

	/* Both a VBUS and a charge status change can change the charger status */
	if ((status_notifier != NULL) && (bq25620_get_status(data->dev, &status) == 0)) {
		status_notifier(status);
	}

	if (((events & MFD_BQ25620_EVENT_VBUS) != 0U) && (online_notifier != NULL) &&
	    (bq25620_get_online(data->dev, &online) == 0)) {
		online_notifier(online);
	}
}

static int bq25620_init_limits(const struct device *dev)
{
	const struct bq25620_charger_config *config = dev->config;
	const struct {
		const struct bq25620_limit *limit;
		uint32_t value;
	} limits[] = {
		{&bq25620_ichg, config->ichg_max_ua},   {&bq25620_vreg, config->vreg_max_uv},
		{&bq25620_iprechg, config->iprechg_ua}, {&bq25620_iterm, config->iterm_ua},
		{&bq25620_iindpm, config->iindpm_ua},   {&bq25620_vindpm, config->vindpm_uv},
		{&bq25620_vsysmin, config->vsysmin_uv},
	};
	int ret;

	ARRAY_FOR_EACH(limits, i) {
		/* Only the input current limit is optional, it is set by the adapter detection */
		if (limits[i].value == 0U) {
			continue;
		}

		ret = bq25620_set_limit(dev, limits[i].limit, limits[i].limit->max,
					limits[i].value);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int bq25620_init_ctrl(const struct device *dev)
{
	const struct bq25620_charger_config *config = dev->config;
	/* Drive strength register values, 0b10 is reserved */
	static const uint8_t conv_strn[] = {0x0, 0x1, 0x3};
	const struct {
		uint8_t reg;
		uint8_t mask;
		uint8_t value;
	} fields[] = {
		{
			BQ25620_REG_CHG_CTRL_0,
			BQ25620_CHG_CTRL_0_Q1_FULLON | BQ25620_CHG_CTRL_0_Q4_FULLON |
				BQ25620_CHG_CTRL_0_VINDPM_BAT_TRACK | BQ25620_CHG_CTRL_0_VRECHG,
			(config->q1_fullon ? BQ25620_CHG_CTRL_0_Q1_FULLON : 0U) |
				(config->q4_fullon ? BQ25620_CHG_CTRL_0_Q4_FULLON : 0U) |
				FIELD_PREP(BQ25620_CHG_CTRL_0_VINDPM_BAT_TRACK,
					   config->vindpm_bat_track) |
				FIELD_PREP(BQ25620_CHG_CTRL_0_VRECHG, config->vrechg),
		},
		{
			BQ25620_REG_TIMER_CTRL,
			BQ25620_TIMER_CTRL_TMR2X_EN | BQ25620_TIMER_CTRL_EN_SAFETY_TMRS |
				BQ25620_TIMER_CTRL_PRECHG_TMR | BQ25620_TIMER_CTRL_CHG_TMR,
			(config->disable_tmr2x ? 0U : BQ25620_TIMER_CTRL_TMR2X_EN) |
				(config->disable_safety_tmrs ? 0U
							     : BQ25620_TIMER_CTRL_EN_SAFETY_TMRS) |
				FIELD_PREP(BQ25620_TIMER_CTRL_PRECHG_TMR, config->prechg_tmr) |
				FIELD_PREP(BQ25620_TIMER_CTRL_CHG_TMR, config->chg_tmr),
		},
		{
			/* Charging is controlled by the charge enable pin, if present */
			BQ25620_REG_CHG_CTRL_1,
			BQ25620_CHG_CTRL_1_EN_AUTO_IBATDIS | BQ25620_CHG_CTRL_1_EN_CHG,
			(config->disable_auto_ibatdis ? 0U : BQ25620_CHG_CTRL_1_EN_AUTO_IBATDIS) |
				BQ25620_CHG_CTRL_1_EN_CHG,
		},
		{
			BQ25620_REG_CHG_CTRL_2,
			BQ25620_CHG_CTRL_2_TREG | BQ25620_CHG_CTRL_2_SET_CONV_FREQ |
				BQ25620_CHG_CTRL_2_SET_CONV_STRN | BQ25620_CHG_CTRL_2_VBUS_OVP,
			FIELD_PREP(BQ25620_CHG_CTRL_2_TREG, config->treg) |
				FIELD_PREP(BQ25620_CHG_CTRL_2_SET_CONV_FREQ, config->conv_freq) |
				FIELD_PREP(BQ25620_CHG_CTRL_2_SET_CONV_STRN,
					   conv_strn[config->conv_strn]) |
				FIELD_PREP(BQ25620_CHG_CTRL_2_VBUS_OVP, config->vbus_ovp),
		},
		{
			/* IBAT_PK 0b00 and 0b01 are reserved, the enum starts at 0b10 */
			BQ25620_REG_CHG_CTRL_4,
			BQ25620_CHG_CTRL_4_IBAT_PK | BQ25620_CHG_CTRL_4_CHG_RATE,
			FIELD_PREP(BQ25620_CHG_CTRL_4_IBAT_PK, config->ibat_pk + 2U) |
				FIELD_PREP(BQ25620_CHG_CTRL_4_CHG_RATE, config->chg_rate),
		},
		{
			BQ25620_REG_NTC_CTRL_0,
			BQ25620_NTC_CTRL_0_TS_IGNORE,
			config->ts_ignore ? BQ25620_NTC_CTRL_0_TS_IGNORE : 0U,
		},
	};
	int ret;

	ARRAY_FOR_EACH(fields, i) {
		ret = i2c_reg_update_byte_dt(&config->i2c, fields[i].reg, fields[i].mask,
					     fields[i].value);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int bq25620_charger_init(const struct device *dev)
{
	const struct bq25620_charger_config *config = dev->config;
	struct bq25620_charger_data *data = dev->data;
	int ret;

	data->dev = dev;

	if (!device_is_ready(config->mfd)) {
		LOG_ERR("MFD device not ready");
		return -ENODEV;
	}

	/* Without a charge enable pin, charging is only controlled by EN_CHG */
	data->ce_enabled = true;

#if BQ25620_CE_GPIO
	if (config->ce_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&config->ce_gpio)) {
			LOG_ERR("Charge enable GPIO not ready");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&config->ce_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			return ret;
		}

		data->ce_enabled = false;
	}
#endif

	ret = bq25620_init_limits(dev);
	if (ret < 0) {
		LOG_ERR("Failed to configure limits: %d", ret);
		return ret;
	}

	ret = bq25620_init_ctrl(dev);
	if (ret < 0) {
		LOG_ERR("Failed to configure charger: %d", ret);
		return ret;
	}

	data->cb.handler = bq25620_event_handler;
	data->cb.events = BQ25620_EVENTS;

	ret = mfd_bq25620_add_callback(config->mfd, &data->cb);
	if (ret == 0) {
		data->has_interrupt = true;
	} else if (ret != -ENOTSUP) {
		LOG_ERR("Failed to register event callback: %d", ret);
		return ret;
	}

	return 0;
}

static DEVICE_API(charger, bq25620_charger_api) = {
	.get_property = bq25620_get_prop,
	.set_property = bq25620_set_prop,
	.charge_enable = bq25620_charge_enable,
};

/* Optional properties that are not set pass the check */
#define BQ25620_ASSERT_RANGE(inst, prop, name, unit)                                               \
	BUILD_ASSERT(IN_RANGE(DT_INST_PROP_OR(inst, prop, BQ25620_##name##_MIN_##unit),            \
			      BQ25620_##name##_MIN_##unit, BQ25620_##name##_MAX_##unit),           \
		     "Property " #prop " out of range")

#define BQ25620_CHARGER_DEFINE(inst)                                                               \
	BQ25620_ASSERT_RANGE(inst, constant_charge_current_max_microamp, ICHG, UA);                \
	BQ25620_ASSERT_RANGE(inst, constant_charge_voltage_max_microvolt, VREG, UV);               \
	BQ25620_ASSERT_RANGE(inst, precharge_current_microamp, IPRECHG, UA);                       \
	BQ25620_ASSERT_RANGE(inst, charge_term_current_microamp, ITERM, UA);                       \
	BQ25620_ASSERT_RANGE(inst, ti_input_current_limit_microamp, IINDPM, UA);                   \
	BQ25620_ASSERT_RANGE(inst, ti_input_voltage_limit_microvolt, VINDPM, UV);                  \
	BQ25620_ASSERT_RANGE(inst, ti_min_sys_voltage_microvolt, VSYSMIN, UV);                     \
                                                                                                   \
	static const struct bq25620_charger_config bq25620_charger_config_##inst = {               \
		.i2c = I2C_DT_SPEC_GET(DT_INST_PARENT(inst)),                                      \
		.mfd = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                        \
		.ce_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, ce_gpios, {0}),                          \
		.ichg_max_ua = DT_INST_PROP(inst, constant_charge_current_max_microamp),           \
		.vreg_max_uv = DT_INST_PROP(inst, constant_charge_voltage_max_microvolt),          \
		.iprechg_ua =                                                                      \
			DT_INST_PROP_OR(inst, precharge_current_microamp, BQ25620_IPRECHG_POR_UA), \
		.iterm_ua =                                                                        \
			DT_INST_PROP_OR(inst, charge_term_current_microamp, BQ25620_ITERM_POR_UA), \
		.iindpm_ua = DT_INST_PROP_OR(inst, ti_input_current_limit_microamp, 0),            \
		.vindpm_uv = DT_INST_PROP_OR(inst, ti_input_voltage_limit_microvolt,               \
					     BQ25620_VINDPM_POR_UV),                               \
		.vsysmin_uv = DT_INST_PROP_OR(inst, ti_min_sys_voltage_microvolt,                  \
					      BQ25620_VSYSMIN_POR_UV),                             \
		.vrechg = DT_INST_ENUM_IDX_OR(inst, re_charge_threshold_microvolt,                 \
					      BQ25620_VRECHG_POR),                                 \
		.vindpm_bat_track = DT_INST_ENUM_IDX_OR(inst, ti_vindpm_bat_track_microvolt,       \
							BQ25620_VINDPM_BAT_TRACK_POR),             \
		.prechg_tmr =                                                                      \
			DT_INST_ENUM_IDX_OR(inst, ti_precharge_timer, BQ25620_PRECHG_TMR_POR),     \
		.chg_tmr = DT_INST_ENUM_IDX_OR(inst, ti_fast_charge_timer, BQ25620_CHG_TMR_POR),   \
		.vbus_ovp =                                                                        \
			DT_INST_ENUM_IDX_OR(inst, ti_vbus_ovp_microvolt, BQ25620_VBUS_OVP_POR),    \
		.ibat_pk = DT_INST_ENUM_IDX_OR(inst, ti_ibat_peak_current_microamp,                \
					       BQ25620_IBAT_PK_POR),                               \
		.chg_rate = DT_INST_ENUM_IDX_OR(inst, ti_charge_rate, BQ25620_CHG_RATE_POR),       \
		.treg = DT_INST_ENUM_IDX_OR(inst, ti_thermal_regulation_celsius,                   \
					    BQ25620_TREG_POR),                                     \
		.conv_freq = DT_INST_ENUM_IDX_OR(inst, ti_switching_frequency_hz,                  \
						 BQ25620_CONV_FREQ_POR),                           \
		.conv_strn = DT_INST_ENUM_IDX_OR(inst, ti_converter_drive_strength,                \
						 BQ25620_CONV_STRN_POR),                           \
		.q1_fullon = DT_INST_PROP(inst, ti_q1_fullon),                                     \
		.q4_fullon = DT_INST_PROP(inst, ti_q4_fullon),                                     \
		.disable_safety_tmrs = DT_INST_PROP(inst, ti_disable_safety_timers),               \
		.disable_tmr2x = DT_INST_PROP(inst, ti_disable_timer_2x),                          \
		.disable_auto_ibatdis = DT_INST_PROP(inst, ti_disable_auto_battery_discharge),     \
		.ts_ignore = DT_INST_PROP(inst, ti_ts_ignore),                                     \
	};                                                                                         \
                                                                                                   \
	static struct bq25620_charger_data bq25620_charger_data_##inst;                            \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, bq25620_charger_init, NULL, &bq25620_charger_data_##inst,      \
			      &bq25620_charger_config_##inst, POST_KERNEL,                         \
			      CONFIG_CHARGER_INIT_PRIORITY, &bq25620_charger_api);

DT_INST_FOREACH_STATUS_OKAY(BQ25620_CHARGER_DEFINE)
