#include "ltc2959.h"
#include "zephyr/device.h"
#include "zephyr/drivers/i2c.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/fuel_gauge.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#define DT_DRV_COMPAT adi_ltc2959

LOG_MODULE_REGISTER(LTC2959);

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

static int ltc2959_get_adc_mode(const struct device *dev, uint8_t* mode)
{
	const struct ltc2959_config* cfg = dev->config;
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

static int ltc2959_set_cc_config(const struct device *dev, uint8_t mask)
{
	const struct ltc2959_config *cfg = dev->config;
	mask &= LTC2959_CC_WRITABLE_MASK ;
	mask |= LTC2959_CC_RESERVED_VALUE;
	LOG_INF("setting cc to: 0x%02X", mask);
	return i2c_reg_write_byte_dt(&cfg->i2c, LTC2959_REG_CC_CONTROL, mask);
}

static int ltc2959_write_acr(const struct device *dev, uint32_t value)
{
	const struct ltc2959_config *cfg = dev->config;
	uint8_t buf[4] = {
		(value >> 24) & 0xFF,
		(value >> 16) & 0xFF,
		(value >> 8) & 0xFF,
		value & 0xFF
	};
	return i2c_burst_write_dt(&cfg->i2c, LTC2959_REG_ACC_CHARGE_3, buf, sizeof(buf));
}

static int ltc2959_hold_accumulator(const struct device *dev, bool hold)
{
	uint8_t ctrl;
	int ret = ltc2959_get_cc_config(dev, &ctrl);
	if (ret < 0) {
		return ret;
	}
	ctrl = hold ? (ctrl | LTC2959_CC_HOLD_BIT) : (ctrl & ~LTC2959_CC_HOLD_BIT);
	return ltc2959_set_cc_config(dev, ctrl);
}

static int ltc2959_get_gpio_voltage_uv(const struct device *dev, int32_t *value_uv)
{
	uint8_t ctrl;
	uint16_t raw;
	int ret;
	ltc2959_get_adc_mode(dev, &ctrl);

	ret = ltc2959_read16(dev, LTC2959_REG_GPIO_VOLTAGE_MSB, &raw);
	if (ret < 0) {
		return ret;
	}

	int16_t raw_signed = (int16_t)raw;
	uint8_t gpio_mode = ctrl & LTC2959_CTRL_GPIO_MODE_MASK;

	switch (gpio_mode) {
		case LTC2959_GPIO_MODE_BIPOLAR:
			/**
			 * Bipolar ±97.5 mV
			 * Voltage = (97.5 mV × raw_signed) / 32768 = raw × 2970 / 1000
			 */
			*value_uv = (raw_signed * LTC2959_GPIO_BIPOLAR_UV_SF) >> 15;
			break;

		case LTC2959_GPIO_MODE_UNIPOLAR:
			/**
			 * Unipolar 0–1.56 V
			 * Voltage = (1.56 V × raw_unsigned) / 65536 = raw × 1560000 / 65536
			 * = raw × 23.8 µV
			 */
			*value_uv = (raw_signed * LTC2959_GPIO_UNIPOLAR_UV_SF) >> 16;
			break;

		default:
			LOG_ERR("Unsupported GPIO analog mode: 0x%x", gpio_mode);
			return -EINVAL;
	}
	return 0;
}

static int ltc2959_get_gpio_threshold_uv(const struct device *dev, bool high, int32_t *value_uv)
{
	uint8_t reg = high ? LTC2959_REG_GPIO_THRESH_HIGH_MSB
	                   : LTC2959_REG_GPIO_THRESH_LOW_MSB;

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

	int16_t raw_signed = (int16_t)raw;
	uint8_t gpio_mode = ctrl & LTC2959_CTRL_GPIO_MODE_MASK;

	switch (gpio_mode) {
	case LTC2959_GPIO_MODE_BIPOLAR:
		*value_uv = (raw_signed * LTC2959_GPIO_BIPOLAR_UV_SF) >> 15;
		break;
	case LTC2959_GPIO_MODE_UNIPOLAR:
		*value_uv = (raw_signed * LTC2959_GPIO_UNIPOLAR_UV_SF) >> 16;
		break;
	default:
		LOG_ERR("Unsupported GPIO mode: 0x%x", gpio_mode);
		return -ENOTSUP;
	}
	return 0;
}

static int ltc2959_set_gpio_threshold_uv(const struct device *dev, bool high, int32_t value_uv)
{
	uint8_t reg = high ? LTC2959_REG_GPIO_THRESH_HIGH_MSB
	                   : LTC2959_REG_GPIO_THRESH_LOW_MSB;

	const struct ltc2959_config *cfg = dev->config;

	uint8_t ctrl;
	int ret = i2c_reg_read_byte_dt(&cfg->i2c, LTC2959_REG_ADC_CONTROL, &ctrl);
	if (ret < 0) {
		return ret;
	}

	uint8_t gpio_mode = ctrl & LTC2959_CTRL_GPIO_MODE_MASK;
	int16_t raw;

	switch (gpio_mode) {
	case LTC2959_GPIO_MODE_BIPOLAR:
		raw = ((int32_t)value_uv << 15) / LTC2959_GPIO_BIPOLAR_UV_SF;
		break;

	case LTC2959_GPIO_MODE_UNIPOLAR:
		raw = (value_uv << 16) / LTC2959_GPIO_UNIPOLAR_UV_SF;
		break;

	case LTC2959_GPIO_MODE_ALERT:
	case LTC2959_GPIO_MODE_CHGCOMP:
	default:
		LOG_ERR("Unsupported GPIO mode: 0x%02x", gpio_mode);
		return -ENOTSUP;
	}

	uint8_t buf[2] = { raw >> 8, raw & 0xFF };
	return i2c_burst_write_dt(&cfg->i2c, reg, buf, sizeof(buf));
}

static int ltc2959_get_voltage_threshold_mv(const struct device *dev, bool high, uint16_t* value)
{
	uint8_t reg = high ? LTC2959_REG_VOLT_THRESH_HIGH_MSB : LTC2959_REG_VOLT_THRESH_LOW_MSB;
	uint16_t raw;
	int ret = ltc2959_read16(dev, reg, &raw);
	if (ret < 0) {
		LOG_ERR("ERROR: %i", ret);
		return ret;
	}
	*value = (uint32_t)(raw * LTC2959_VOLT_MV_SF) >> 16;
	return 0;
}

static int ltc2959_set_voltage_threshold_mv(const struct device *dev, bool high, uint16_t value_mv)
{
	uint8_t reg = high ? LTC2959_REG_VOLT_THRESH_HIGH_MSB
	                   : LTC2959_REG_VOLT_THRESH_LOW_MSB;

	// NOTE: use either version based on needs/accuracy.
	// const uint16_t raw = ((uint32_t)value_mv << 16) / 62600;
	const uint16_t raw = ((uint32_t)value_mv * LTC2959_RAW_TO_MV_SF) >> 16;

	uint8_t buf[2] = {raw >> 8, raw & 0xFF};
	const struct ltc2959_config *cfg = dev->config;
	return i2c_burst_write_dt(&cfg->i2c, reg, buf, sizeof(buf));
}

static int ltc2959_get_current_threshold_ua(const struct device *dev, bool high, int16_t *value_ua)
{
	uint8_t reg = high ? LTC2959_REG_CURR_THRESH_HIGH_MSB
	                   : LTC2959_REG_CURR_THRESH_LOW_MSB;

	uint16_t raw;
	int ret = ltc2959_read16(dev, reg, &raw);
	if (ret < 0) {
		return ret;
	}

	int16_t signed_raw = (int16_t)raw;
	*value_ua = signed_raw * LTC2959_CURRENT_LSB_UV;
	return 0;
}

static int ltc2959_set_current_threshold_ua(const struct device *dev, bool high, int16_t value_ua)
{
	uint8_t reg = high ? LTC2959_REG_CURR_THRESH_HIGH_MSB
	                   : LTC2959_REG_CURR_THRESH_LOW_MSB;

	/* Reverse of: current = raw * LSB → raw = current / LSB */
	int16_t raw = value_ua / LTC2959_CURRENT_LSB_UV;

	uint8_t buf[2] = { raw >> 8, raw & 0xFF };
	const struct ltc2959_config *cfg = dev->config;
	return i2c_burst_write_dt(&cfg->i2c, reg, buf, sizeof(buf));
}

static int ltc2959_get_temp_threshold_dK(const struct device *dev, bool high, uint16_t *value_dK)
{
	uint8_t reg = high ? LTC2959_REG_TEMP_THRESH_HIGH_MSB
	                   : LTC2959_REG_TEMP_THRESH_LOW_MSB;

	uint16_t raw;
	int ret = ltc2959_read16(dev, reg, &raw);
	if (ret < 0) {
		return ret;
	}

	*value_dK = ((uint32_t)raw * LTC2959_TEMP_K_SF) >> 16;
	return 0;
}

static int ltc2959_set_temp_threshold_dK(const struct device *dev, bool high, uint16_t value_dK)
{
	uint8_t reg = high ? LTC2959_REG_TEMP_THRESH_HIGH_MSB
	                   : LTC2959_REG_TEMP_THRESH_LOW_MSB;

	uint16_t raw = ((uint32_t)value_dK << 16) / LTC2959_TEMP_K_SF;

	uint8_t buf[2] = { raw >> 8, raw & 0xFF };
	const struct ltc2959_config *cfg = dev->config;
	return i2c_burst_write_dt(&cfg->i2c, reg, buf, sizeof(buf));
}


static int ltc2959_get_prop(const struct device *dev,
			    fuel_gauge_prop_t prop,
			    union fuel_gauge_prop_val *val)
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
			return 0;

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
			val->voltage = raw_voltage * LTC2959_RAW_TO_UVOLT_SF;
			return 0;

		case FUEL_GAUGE_CURRENT:
			uint16_t raw_current;
			ret = ltc2959_read16(dev, LTC2959_REG_CURRENT_MSB, &raw_current);
			if (ret < 0) {
				return ret;
			}
			/* Signed 16-bit value from ADC */
			int16_t current_raw = (int16_t)raw_current;
			val->current = current_raw * LTC2959_CURRENT_LSB_UV;
			return 0;

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
			val->temperature = ((uint32_t)raw_temp * LTC2959_TEMP_K_SF) >> 16;
			return 0;

		case FUEL_GAUGE_CHARGE_CURRENT:
			uint32_t raw;
			ret = ltc2959_read32(dev, LTC2959_REG_ACC_CHARGE_3, &raw);
			if (ret < 0) {
				return ret;
			}
			val->chg_current = ((uint64_t)raw * LTC2959_CHARGE_UAH_SF) >> 16;
			return 0;

		case FUEL_GAUGE_ADC_MODE:
			ret = ltc2959_get_adc_mode(dev, (uint8_t*)val->flags);
			break;

		case FUEL_GAUGE_VOLTAGE_HIGH_THRESHOLD:
			ret = ltc2959_get_voltage_threshold_mv(dev, true, (uint16_t*)&val->design_volt);
			break;

		case FUEL_GAUGE_VOLTAGE_LOW_THRESHOLD:
			ret = ltc2959_get_voltage_threshold_mv(dev, false, (uint16_t*)&val->design_volt);
			break;

		case FUEL_GAUGE_CURRENT_HIGH_THRESHOLD:
			int16_t hi_curr;
			ret = ltc2959_get_current_threshold_ua(dev, true, &hi_curr);
			if (ret == 0) {
				val->current = hi_curr;
			}
			break;

		case FUEL_GAUGE_CURRENT_LOW_THRESHOLD:
			int16_t lo_curr;
			ret = ltc2959_get_current_threshold_ua(dev, false, &lo_curr);
			if (ret == 0) {
				val->current = lo_curr;
			}
			break;

		case FUEL_GAUGE_TEMPERATURE_HIGH_THRESHOLD:
			ret = ltc2959_get_temp_threshold_dK(dev, true, &val->temperature);
			break;

		case FUEL_GAUGE_TEMPERATURE_LOW_THRESHOLD:
			ret = ltc2959_get_temp_threshold_dK(dev, false, &val->temperature);
			break;

		case FUEL_GAUGE_GPIO_VOLTAGE_UV:
			ret = ltc2959_get_gpio_voltage_uv(dev, &val->voltage);
			break;

		case FUEL_GAUGE_GPIO_HIGH_THRESHOLD:
			int32_t hi_gpio;
			ret = ltc2959_get_gpio_threshold_uv(dev, true, &hi_gpio);
			if (ret == 0) {
				val->voltage = hi_gpio;
			}
			break;

		case FUEL_GAUGE_GPIO_LOW_THRESHOLD:
			int32_t lo_gpio;
			ret = ltc2959_get_gpio_threshold_uv(dev, false, &lo_gpio);
			if (ret == 0) {
				val->voltage = lo_gpio;
			}
			break;

		case FUEL_GAUGE_CC_CONFIG:
			uint8_t cc_ctrl;
			ret = ltc2959_get_cc_config(dev, &cc_ctrl);
			if (ret == 0) {
				val->fg_status = cc_ctrl;
			}
			break;

		default:
			return -ENOTSUP;
	}
	return 0;
}

//TODO: bind to DEVICE_API when supported
#if 0
static int ltc2959_get_props(const struct device *dev,
			     fuel_gauge_prop_t *props,
			     union fuel_gauge_prop_val *vals,
			     size_t len)
{

	int ret;
	for (size_t i = 0; i < len; ++i) {
		ret = ltc2959_get_prop(dev, props[i], &vals[i]);
		if (ret < 0) {
			return ret;
		}
	}
	return 0;

}
#endif

static int ltc2959_set_prop(const struct device *dev,
			    fuel_gauge_prop_t prop,
			    union fuel_gauge_prop_val val)
{
	int ret = 0;

	switch (prop) {
	case FUEL_GAUGE_ADC_MODE:
		ret = ltc2959_set_adc_mode(dev, val.fg_status);
		break;

	case FUEL_GAUGE_VOLTAGE_LOW_THRESHOLD:
		ret = ltc2959_set_voltage_threshold_mv(dev, false, val.voltage);
		break;

	case FUEL_GAUGE_VOLTAGE_HIGH_THRESHOLD:
		ret = ltc2959_set_voltage_threshold_mv(dev, true, val.voltage);
		break;

	case FUEL_GAUGE_CURRENT_LOW_THRESHOLD:
		ret = ltc2959_set_current_threshold_ua(dev, false, val.current);
		break;

	case FUEL_GAUGE_CURRENT_HIGH_THRESHOLD:
		ret = ltc2959_set_current_threshold_ua(dev, true, val.current);
		break;

	case FUEL_GAUGE_TEMPERATURE_LOW_THRESHOLD:
		ret = ltc2959_set_temp_threshold_dK(dev, false, val.temperature);
		break;

	case FUEL_GAUGE_TEMPERATURE_HIGH_THRESHOLD:
		ret = ltc2959_set_temp_threshold_dK(dev, true, val.temperature);
		break;

	case FUEL_GAUGE_GPIO_HIGH_THRESHOLD:
		ret = ltc2959_set_gpio_threshold_uv(dev, true, val.voltage);
		break;
	case FUEL_GAUGE_GPIO_LOW_THRESHOLD:
		ret = ltc2959_set_gpio_threshold_uv(dev, false, val.voltage);
		break;

	case FUEL_GAUGE_CC_CONFIG:
		LOG_INF("config stats: 0x%02X", val.fg_status);
		ret = ltc2959_set_cc_config(dev, val.fg_status);
		break;

	case FUEL_GAUGE_CC_CLEAR:
		uint8_t adc_mode;
		ret = ltc2959_get_adc_mode(dev, &adc_mode);
		if (ret < 0) {
			break;
		}

		uint8_t gpio_mode = adc_mode & LTC2959_CTRL_GPIO_MODE_MASK;
		if (gpio_mode == LTC2959_GPIO_MODE_CHGCOMP) {
			// NOTE: In CHGCOMP mode, ACR is cleared by pulling GPIO low externally
			LOG_WRN("ACR not cleared in firmware: GPIO is in charge-complete input mode");
			break;
		}
		// NOTE: clearing to 0xFFFFFFFF to match GPIO behavior
		ret = ltc2959_write_acr(dev, LTC2959_ACR_CLR);
		break;

	case FUEL_GAUGE_CC_HOLD:
		ret = ltc2959_hold_accumulator(dev, val.flags);  // use flags = true/false
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
	.set_property = &ltc2959_set_prop
};

#define LTC2959_DEFINE(inst)							\
	static const struct ltc2959_config ltc2959_config_##inst = {		\
		.i2c = I2C_DT_SPEC_INST_GET(inst)				\
	};									\
	DEVICE_DT_INST_DEFINE(inst,						\
			      ltc2959_init,					\
			      NULL,						\
			      NULL, &ltc2959_config_##inst,			\
		      POST_KERNEL, CONFIG_FUEL_GAUGE_INIT_PRIORITY,		\
		      &ltc2959_driver_api);

DT_INST_FOREACH_STATUS_OKAY(LTC2959_DEFINE)

