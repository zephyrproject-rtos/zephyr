#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/fuel_gauge.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include "ltc2959.h"

#define DT_DRV_COMPAT adi_ltc2959

LOG_MODULE_REGISTER(LTC2959);

/* NOTE: QLSB = 533 nAh = ~0.533 µAh → raw × (34952 / 65536) */
#define LTC2959_CHARGE_UAH_SF (34952)
#define LTC2959_TEMP_K_SF (8250)
#define LTC2959_VOLT_UV_SF (955) /* µV per full-scale (16-bit) */
#define LTC2959_GPIO_BIPOLAR_UV_SF (97500)
#define LTC2959_GPIO_UNIPOLAR_UV_SF (1560000)
#define LTC2959_VOLT_THRESH_UV_SCALAR (62600000)

/* CONTROL Register Bit Masks */
#define LTC2959_CTRL_ADC_MODE_MASK GENMASK(7, 5)
#define LTC2959_CTRL_GPIO_MODE_MASK GENMASK(4, 3)
#define LTC2959_CTRL_VIN_SEL_BIT BIT(2)
#define LTC2959_CTRL_RESERVED_MASK GENMASK(1, 0)

#define LTC2959_CC_WRITABLE_MASK 0xD8 /* bits 7,6,3 are writable (4 must be 1) */
#define LTC2959_CC_RESERVED_MASK (0b01 << 5)

/* Used when ACR is controlled via firmware */
#define LTC2959_ACR_CLR (0xFFFFFFFF)

/* Voltage source selection (bit 2 of Control Register) */
#define LTC2959_VIN_VDD (0x0 << 2)
#define LTC2959_VIN_SENSEN (0x1 << 2)

/* STATUS Register Bit Definitions (0x00) */
enum ltc2959_status_flags {
	LTC2959_STATUS_GPIO_ALERT = BIT(7),        /* Default: 0 */
	LTC2959_STATUS_CURRENT_ALERT = BIT(6),     /* Default: 0 */
	LTC2959_STATUS_CHARGE_OVER_UNDER = BIT(5), /* Default: 0 */
	LTC2959_STATUS_TEMP_ALERT = BIT(4),        /* Default: 0 */
	LTC2959_STATUS_CHARGE_HIGH = BIT(3),       /* Default: 0 */
	LTC2959_STATUS_CHARGE_LOW = BIT(2),        /* Default: 0 */
	LTC2959_STATUS_VOLTAGE_ALERT = BIT(1),     /* Default: 0 */
	LTC2959_STATUS_UVLO = BIT(0)               /* Default: 1 */
};

/* ADC mode values (bits 7:5 of CONTROL register 0x01) */
enum ltc2959_adc_modes {
	LTC2959_ADC_MODE_SLEEP = 0x00,
	LTC2959_ADC_MODE_SMART_SLEEP = 0x20,
	LTC2959_ADC_MODE_CONT_V = 0x40,
	LTC2959_ADC_MODE_CONT_I = 0x60,
	LTC2959_ADC_MODE_CONT_VI = 0x80,
	LTC2959_ADC_MODE_SINGLE_SHOT = 0xA0,
	LTC2959_ADC_MODE_CONT_VIT = 0xC0 /* recommended for full telemetry */
};

/* GPIO mode bits (bits 4:3 of CONTROL register 0x01) */
enum ltc2959_gpio_modes {
	LTC2959_GPIO_MODE_ALERT = 0x00,
	LTC2959_GPIO_MODE_CHGCOMP = 0x08,
	LTC2959_GPIO_MODE_BIPOLAR = 0x10,
	LTC2959_GPIO_MODE_UNIPOLAR = 0x18,
};

/* CC Control bits (CC register 0x02)*/
enum ltc2959_cc_options {
	LTC2959_CC_DEADBAND_0UV = (0b00 << 6),
	LTC2959_CC_DEADBAND_20UV = (0b01 << 6),
	LTC2959_CC_DEADBAND_40UV = (0b10 << 6),
	LTC2959_CC_DEADBAND_80UV = (0b11 << 6),
	LTC2959_CC_DO_NOT_COUNT = BIT(3),
};

struct ltc2959_config {
	struct i2c_dt_spec i2c;
	uint32_t rsense_mohms;
	int32_t current_lsb_uv;
};

static int ltc2959_read16(const struct device *dev, uint8_t reg, uint16_t *value)
{
	uint8_t buf[2];
	const struct ltc2959_config *cfg = dev->config;
	int ret = i2c_burst_read_dt(&cfg->i2c, reg, buf, sizeof(buf));

	if (ret < 0) {
		LOG_ERR("Failed to read 16-bit register 0x%02X", reg);
		return ret;
	}

	*value = sys_get_be16(buf);
	return 0;
}

static int ltc2959_read32(const struct device *dev, uint8_t reg, uint32_t *value)
{
	uint8_t buf[4];
	const struct ltc2959_config *cfg = dev->config;
	int ret = i2c_burst_read_dt(&cfg->i2c, reg, buf, sizeof(buf));

	if (ret < 0) {
		LOG_ERR("Failed to read 32-bit register 0x%02X", reg);
		return ret;
	}

	*value = sys_get_be32(buf);
	return 0;
}

static int ltc2959_get_adc_mode(const struct device *dev, uint8_t *mode)
{
	const struct ltc2959_config *cfg = dev->config;

	return i2c_reg_read_byte_dt(&cfg->i2c, LTC2959_REG_ADC_CONTROL, mode);
}

static int ltc2959_set_adc_mode(const struct device *dev, uint8_t mode)
{
	const struct ltc2959_config *cfg = dev->config;
	int ret = i2c_reg_write_byte_dt(&cfg->i2c, LTC2959_REG_ADC_CONTROL, mode);

	if (ret < 0) {
		LOG_ERR("Failed to set ADC mode: 0x%02x", mode);
		return ret;
	}

	return 0;
}

static int ltc2959_get_cc_config(const struct device *dev, uint8_t *value)
{
	const struct ltc2959_config *cfg = dev->config;

	return i2c_reg_read_byte_dt(&cfg->i2c, LTC2959_REG_CC_CONTROL, value);
}

static int ltc2959_set_cc_config(const struct device *dev, uint8_t value)
{
	const struct ltc2959_config *cfg = dev->config;
	uint8_t mask = (value & LTC2959_CC_WRITABLE_MASK) | LTC2959_CC_RESERVED_MASK;

	LOG_DBG("setting cc to: 0x%02X", mask);
	return i2c_reg_write_byte_dt(&cfg->i2c, LTC2959_REG_CC_CONTROL, mask);
}

static int ltc2959_write_acr(const struct device *dev, uint32_t value)
{
	const struct ltc2959_config *cfg = dev->config;
	uint8_t buf[4];

	sys_put_be32(value, buf);
	return i2c_burst_write_dt(&cfg->i2c, LTC2959_REG_ACC_CHARGE_3, buf, sizeof(buf));
}

static int ltc2959_hold_accumulator(const struct device *dev, bool hold)
{
	uint8_t ctrl;
	int ret = ltc2959_get_cc_config(dev, &ctrl);

	if (ret < 0) {
		return ret;
	}

	ctrl = hold ? (ctrl | LTC2959_CC_DO_NOT_COUNT) : (ctrl & ~LTC2959_CC_DO_NOT_COUNT);
	return ltc2959_set_cc_config(dev, ctrl);
}

static int ltc2959_get_gpio_voltage_uv(const struct device *dev, int32_t *value_uv)
{
	uint8_t ctrl;
	uint16_t raw;

	ltc2959_get_adc_mode(dev, &ctrl);

	int ret = ltc2959_read16(dev, LTC2959_REG_GPIO_VOLTAGE_MSB, &raw);

	if (ret < 0) {
		return ret;
	}

	int16_t raw_signed = (int16_t) raw;
	uint8_t gpio_mode = ctrl & LTC2959_CTRL_GPIO_MODE_MASK;

	switch (gpio_mode) {
	case LTC2959_GPIO_MODE_BIPOLAR:
		*value_uv = ((int64_t) raw_signed * LTC2959_GPIO_BIPOLAR_UV_SF) >> 15;
		break;

	case LTC2959_GPIO_MODE_UNIPOLAR:
		*value_uv = ((uint32_t) raw * LTC2959_GPIO_UNIPOLAR_UV_SF) >> 15;
		break;

	default:
		LOG_ERR("Unsupported GPIO analog mode: 0x%x", gpio_mode);
		return -EINVAL;
	}

	return 0;
}

static int ltc2959_get_gpio_threshold_uv(const struct device *dev, bool high, int32_t *value_uv)
{
	uint8_t reg = high ? LTC2959_REG_GPIO_THRESH_HIGH_MSB : LTC2959_REG_GPIO_THRESH_LOW_MSB;
	const struct ltc2959_config *cfg = dev->config;
	uint8_t ctrl;

	int ret = i2c_reg_read_byte_dt(&cfg->i2c, LTC2959_REG_ADC_CONTROL, &ctrl);

	if (ret < 0) {
		LOG_ERR("NO CTRL: %i", ret);
		return ret;
	}

	uint16_t raw;

	ret = ltc2959_read16(dev, reg, &raw);

	if (ret < 0) {
		return ret;
	}

	int16_t raw_signed = (int16_t) raw;
	uint8_t gpio_mode = ctrl & LTC2959_CTRL_GPIO_MODE_MASK;

	switch (gpio_mode) {
	case LTC2959_GPIO_MODE_BIPOLAR:
		*value_uv = ((int64_t) raw_signed * LTC2959_GPIO_BIPOLAR_UV_SF) >> 15;
		break;

	case LTC2959_GPIO_MODE_UNIPOLAR:
		*value_uv = ((uint32_t) raw * LTC2959_GPIO_UNIPOLAR_UV_SF) >> 15;
		break;

	default:
		LOG_ERR("Unsupported GPIO mode: 0x%x", gpio_mode);
		return -ENOTSUP;
}
	return 0;
}

static int ltc2959_set_gpio_threshold_uv(const struct device *dev, bool high, int32_t value_uv)
{
	uint8_t reg = high ? LTC2959_REG_GPIO_THRESH_HIGH_MSB : LTC2959_REG_GPIO_THRESH_LOW_MSB;

	const struct ltc2959_config *cfg = dev->config;

	uint8_t ctrl;
	int ret = i2c_reg_read_byte_dt(&cfg->i2c, LTC2959_REG_ADC_CONTROL, &ctrl);

	if (ret < 0) {
		return ret;
	}

	uint8_t gpio_mode = ctrl & LTC2959_CTRL_GPIO_MODE_MASK;

	switch (gpio_mode) {
	case LTC2959_GPIO_MODE_BIPOLAR: {
		int16_t raw = (int16_t) (((int64_t) value_uv * 32768) /
			LTC2959_GPIO_BIPOLAR_UV_SF);
		uint8_t buf[2];

		sys_put_be16(raw, buf);
		return i2c_burst_write_dt(&cfg->i2c, reg, buf, sizeof(buf));
	}
	case LTC2959_GPIO_MODE_UNIPOLAR: {
		uint16_t raw = (uint16_t) (((int64_t) value_uv * 32768) /
			LTC2959_GPIO_UNIPOLAR_UV_SF);
		uint8_t buf[2];

		sys_put_be16(raw, buf);
		return i2c_burst_write_dt(&cfg->i2c, reg, buf, sizeof(buf));
	}
	default:
		break;
	}

	LOG_ERR("Unsupported GPIO mode: 0x%02x", gpio_mode);
	return -ENOTSUP;
}

static int ltc2959_get_voltage_threshold_uv(const struct device *dev, bool high, uint32_t *value)
{
	uint8_t reg = high ? LTC2959_REG_VOLT_THRESH_HIGH_MSB : LTC2959_REG_VOLT_THRESH_LOW_MSB;
	uint16_t raw;
	int ret = ltc2959_read16(dev, reg, &raw);

	if (ret < 0) {
		LOG_ERR("Failed to get voltage threshold: %i", ret);
		return ret;
	}

	*value = ((uint64_t) raw * LTC2959_VOLT_THRESH_UV_SCALAR) >> 15;

	return 0;
}

static int ltc2959_set_voltage_threshold_uv(const struct device *dev, bool high, uint32_t value)
{
	uint8_t reg = high ? LTC2959_REG_VOLT_THRESH_HIGH_MSB : LTC2959_REG_VOLT_THRESH_LOW_MSB;
	const uint16_t raw = (((uint64_t) value << 15) / LTC2959_VOLT_THRESH_UV_SCALAR);

	uint8_t buf[2];

	sys_put_be16(raw, buf);
	const struct ltc2959_config *cfg = dev->config;

	return i2c_burst_write_dt(&cfg->i2c, reg, buf, sizeof(buf));
}

static int ltc2959_get_current_threshold_ua(const struct device *dev, bool high, int32_t *value_ua)
{
	uint8_t reg = high ? LTC2959_REG_CURR_THRESH_HIGH_MSB : LTC2959_REG_CURR_THRESH_LOW_MSB;

	uint16_t raw;
	int ret = ltc2959_read16(dev, reg, &raw);

	if (ret < 0) {
		return ret;
	}

	const struct ltc2959_config *cfg = dev->config;
	int16_t signed_raw = (int16_t) raw;
	*value_ua = signed_raw * cfg->current_lsb_uv;

	return 0;
}

static int ltc2959_set_current_threshold_ua(const struct device *dev, bool high, int32_t value_ua)
{
	uint8_t reg = high ? LTC2959_REG_CURR_THRESH_HIGH_MSB : LTC2959_REG_CURR_THRESH_LOW_MSB;
	const struct ltc2959_config *cfg = dev->config;
	int32_t raw32 = value_ua / cfg->current_lsb_uv;

	/* To account for cases where current thresholds are +-2A */
	int16_t raw = CLAMP(raw32, INT16_MIN, INT16_MAX);
	uint8_t buf[2];

	sys_put_be16(raw, buf);
	return i2c_burst_write_dt(&cfg->i2c, reg, buf, sizeof(buf));
}

static int ltc2959_get_temp_threshold_dK(const struct device *dev, bool high, uint16_t *value_dK)
{
	uint8_t reg = high ? LTC2959_REG_TEMP_THRESH_HIGH_MSB : LTC2959_REG_TEMP_THRESH_LOW_MSB;
	uint16_t raw;
	int ret = ltc2959_read16(dev, reg, &raw);

	if (ret < 0) {
		return ret;
	}

	*value_dK = ((uint32_t) raw * LTC2959_TEMP_K_SF) >> 16;

	return 0;
}

static int ltc2959_set_temp_threshold_dK(const struct device *dev, bool high, uint16_t value_dK)
{
	uint8_t reg = high ? LTC2959_REG_TEMP_THRESH_HIGH_MSB : LTC2959_REG_TEMP_THRESH_LOW_MSB;
	uint16_t raw = ((uint32_t) value_dK << 16) / LTC2959_TEMP_K_SF;
	uint8_t buf[2];

	sys_put_be16(raw, buf);
	const struct ltc2959_config *cfg = dev->config;

	return i2c_burst_write_dt(&cfg->i2c, reg, buf, sizeof(buf));
}

static int ltc2959_cc_clear(const struct device *dev, fuel_gauge_prop_t prop,
			    union fuel_gauge_prop_val val)
{

	uint8_t adc_mode;
	int ret = ltc2959_get_adc_mode(dev, &adc_mode);

	if (ret < 0) {
		return 0;
	}

	uint8_t gpio_mode = adc_mode & LTC2959_CTRL_GPIO_MODE_MASK;

	if (gpio_mode == LTC2959_GPIO_MODE_CHGCOMP) {
		/* NOTE: In CHGCOMP mode, ACR is cleared by pulling GPIO low externally */
		LOG_WRN("ACR not cleared in firmware: GPIO is in charge-complete input mode");
		return -ENOTSUP;
	}

	/* NOTE: clearing to 0xFFFFFFFF to match GPIO behavior */
	return ltc2959_write_acr(dev, LTC2959_ACR_CLR);
}


static int ltc2959_cc_hold(const struct device *dev, fuel_gauge_prop_t prop, bool hold)
{
	return ltc2959_hold_accumulator(dev, hold);
}

static int ltc2959_get_prop(
	const struct device *dev, fuel_gauge_prop_t prop, union fuel_gauge_prop_val *val)
{
	const struct ltc2959_config *cfg = dev->config;
	int ret;
	uint8_t raw;

	switch (prop) {
	case FUEL_GAUGE_STATUS:
		ret = i2c_reg_read_byte_dt(&cfg->i2c, LTC2959_REG_STATUS, &raw);

		if (ret < 0) {
			return ret;
		}

		val->fg_status = raw;

		break;

	case FUEL_GAUGE_VOLTAGE:
		uint16_t raw_voltage;

		ret = ltc2959_read16(dev, LTC2959_REG_VOLTAGE_MSB, &raw_voltage);

		if (ret < 0) {
			return ret;
		}
		/**
		 * NOTE: LSB = 62.6V / 65536 = ~955 µV
		 * Zephyr's API expects this value in microvolts
		 * https://docs.zephyrproject.org/latest/doxygen/html/group__fuel__gauge__interface.html
		 */
		val->voltage = raw_voltage * LTC2959_VOLT_UV_SF;

		return 0;

	case FUEL_GAUGE_CURRENT:
		uint16_t raw_current;

		ret = ltc2959_read16(dev, LTC2959_REG_CURRENT_MSB, &raw_current);

		if (ret < 0) {
			return ret;
		}

		/* Signed 16-bit value from ADC */
		int16_t current_raw = (int16_t) raw_current;

		val->current = current_raw * cfg->current_lsb_uv;

		break;

	case FUEL_GAUGE_TEMPERATURE:
		uint16_t raw_temp;

		ret = ltc2959_read16(dev, LTC2959_REG_TEMP_MSB, &raw_temp);

		if (ret < 0) {
			return ret;
		}
		/**
		 * NOTE:
		 * Temp is in deciKelvin as per API requirements.
		 * from the datasheet:
		 * T(°C) = 825 * (raw / 65536) - 273.15
		 * T(dK) = 8250 * (raw / 65536)
		 * 65536 = 2 ^ 16, so we can avoid division altogether.
		 */
		val->temperature = ((uint32_t) raw_temp * LTC2959_TEMP_K_SF) >> 16;
		break;

	case FUEL_GAUGE_CHARGE_CURRENT:
		uint32_t raw;

		ret = ltc2959_read32(dev, LTC2959_REG_ACC_CHARGE_3, &raw);

		if (ret < 0) {
			return ret;
		}

		val->chg_current = ((uint64_t) raw * LTC2959_CHARGE_UAH_SF) >> 16;
		break;

	case FUEL_GAUGE_ADC_MODE:
		ret = ltc2959_get_adc_mode(dev, val->adc_mode);
		break;

	case FUEL_GAUGE_HIGH_VOLTAGE_ALARM:
		ret = ltc2959_get_voltage_threshold_uv(dev, true, &val->high_voltage_alarm);
		break;

	case FUEL_GAUGE_LOW_VOLTAGE_ALARM:
		ret = ltc2959_get_voltage_threshold_uv(dev, false, &val->low_voltage_alarm);
		break;

	case FUEL_GAUGE_HIGH_CURRENT_ALARM:
		ret = ltc2959_get_current_threshold_ua(dev, true, &val->high_current_alarm);
		break;

	case FUEL_GAUGE_LOW_CURRENT_ALARM:
		ret = ltc2959_get_current_threshold_ua(dev, false, &val->low_current_alarm);
		break;

	case FUEL_GAUGE_HIGH_TEMPERATURE_ALARM:
		ret = ltc2959_get_temp_threshold_dK(dev, true, &val->high_temperature_alarm);
		break;

	case FUEL_GAUGE_LOW_TEMPERATURE_ALARM:
		ret = ltc2959_get_temp_threshold_dK(dev, false, &val->low_temperature_alarm);
		break;

	case FUEL_GAUGE_GPIO_VOLTAGE:
		ret = ltc2959_get_gpio_voltage_uv(dev, &val->gpio_voltage);
		break;

	case FUEL_GAUGE_HIGH_GPIO_ALARM:
		ret = ltc2959_get_gpio_threshold_uv(dev, true, &val->high_gpio_alarm);
		break;

	case FUEL_GAUGE_LOW_GPIO_ALARM:
		ret = ltc2959_get_gpio_threshold_uv(dev, false, &val->low_gpio_alarm);
		break;

	case FUEL_GAUGE_CC_CONFIG:
		ret = ltc2959_get_cc_config(dev, &val->cc_config);
		break;

	default:
		return -ENOTSUP;
	}
	return ret;
}

static int ltc2959_set_prop(
	const struct device *dev, fuel_gauge_prop_t prop, union fuel_gauge_prop_val val)
{
	int ret = 0;

	switch (prop) {
	case FUEL_GAUGE_ADC_MODE:
		ret = ltc2959_set_adc_mode(dev, val.adc_mode);
		break;

	case FUEL_GAUGE_LOW_VOLTAGE_ALARM:
		ret = ltc2959_set_voltage_threshold_uv(dev, false, val.low_voltage_alarm);
		break;

	case FUEL_GAUGE_HIGH_VOLTAGE_ALARM:
		ret = ltc2959_set_voltage_threshold_uv(dev, true, val.high_voltage_alarm);
		break;

	case FUEL_GAUGE_LOW_CURRENT_ALARM:
		ret = ltc2959_set_current_threshold_ua(dev, false, val.low_current_alarm);
		break;

	case FUEL_GAUGE_HIGH_CURRENT_ALARM:
		ret = ltc2959_set_current_threshold_ua(dev, true, val.high_current_alarm);
		break;

	case FUEL_GAUGE_LOW_TEMPERATURE_ALARM:
		ret = ltc2959_set_temp_threshold_dK(dev, false, val.low_temperature_alarm);
		break;

	case FUEL_GAUGE_HIGH_TEMPERATURE_ALARM:
		ret = ltc2959_set_temp_threshold_dK(dev, true, val.high_temperature_alarm);
		break;

	case FUEL_GAUGE_HIGH_GPIO_ALARM:
		ret = ltc2959_set_gpio_threshold_uv(dev, true, val.high_gpio_alarm);
		break;
	case FUEL_GAUGE_LOW_GPIO_ALARM:
		ret = ltc2959_set_gpio_threshold_uv(dev, false, val.low_gpio_alarm);
		break;

	case FUEL_GAUGE_CC_CONFIG:
		LOG_DBG("config stats: 0x%02X", val.cc_config);
		ret = ltc2959_set_cc_config(dev, val.cc_config);
		break;

	default:
		ret = -ENOTSUP;
		break;
	}
	return ret;
}


static int ltc2959_init(const struct device *dev)
{
	const struct ltc2959_config *cfg = dev->config;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	return 0;
}

static DEVICE_API(fuel_gauge, ltc2959_driver_api) = {
	.get_property = &ltc2959_get_prop,
	.set_property = &ltc2959_set_prop,
	.cc_hold = &ltc2959_cc_hold,
	.cc_clear = &ltc2959_cc_clear
};

#define LTC2959_DEFINE(inst)                                                                       \
static const struct ltc2959_config ltc2959_config_##inst = {                               \
	.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
	.rsense_mohms = DT_INST_PROP(inst, rsense_milliohms),                              \
	.current_lsb_uv = (97500000 / (DT_INST_PROP(inst, rsense_milliohms) * 32768))};    \
DEVICE_DT_INST_DEFINE(inst, ltc2959_init, NULL, NULL, &ltc2959_config_##inst, POST_KERNEL, \
		      CONFIG_FUEL_GAUGE_INIT_PRIORITY, &ltc2959_driver_api);

DT_INST_FOREACH_STATUS_OKAY(LTC2959_DEFINE)
