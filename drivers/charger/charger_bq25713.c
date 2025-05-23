/*
 * Copyright 2025 Palta Tech, S.A
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * BQ25713 Datasheet: https://www.ti.com/lit/ds/symlink/bq25713.pdf
 */

#define DT_DRV_COMPAT ti_bq25713

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/charger.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ti_bq25713, CONFIG_CHARGER_LOG_LEVEL);

/* Charge Option 0 */
#define BQ25713_REG_CO0_LOW          0x00
#define BQ25713_REG_CO0_INHIBIT      0x01
#define BQ25713_REG_CO0_INHIBIT_MASK BIT(0)

/* Charge Current */
#define BQ25713_REG_CC_LOW                    0x02
#define BQ25713_REG_CC_CHARGE_CURRENT_MASK    GENMASK(12, 6)
#define BQ25713_REG_CC_CHARGE_CURRENT_STEP_UA 64000
#define BQ25713_REG_CC_CHARGE_CURRENT_MIN_UA  0
#define BQ25713_REG_CC_CHARGE_CURRENT_MAX_UA  8128000

/* Charger Status */
#define BQ25713_REG_CS_HIGH            0x21
#define BQ25713_REG_CS_AC_STAT         0x01
#define BQ25713_REG_CS_AC_STAT_MASK    BIT(7)
#define BQ25713_REG_CS_PRE_FAST_CHARGE GENMASK(2, 1)
#define BQ25713_REG_CS_PRECHARGE       BIT(1)
#define BQ25713_REG_CS_FASTCHARGE      BIT(2)

/* Max Charge Voltage */
#define BQ25713_REG_CV_LOW                    0x04
#define BQ25713_REG_CV_CHARGE_VOLTAGE_MASK    GENMASK(14, 3)
#define BQ25713_REG_CV_CHARGE_VOLTAGE_STEP_UV 8000
#define BQ25713_REG_CV_CHARGE_VOLTAGE_MIN_UV  1024000
#define BQ25713_REG_CV_CHARGE_VOLTAGE_MAX_UV  19200000

/* Input current set by host IDPM */
#define BQ25713_REG_IIN_HOST_HIGH    0x0F
#define BQ25713_REG_IIN_HOST_MASK    GENMASK(7, 0)
#define BQ25713_REG_IIN_HOST_STEP_UA 50000
#define BQ25713_REG_IIN_HOST_MIN_UV  BQ25713_REG_IIN_HOST_STEP_UA
#define BQ25713_REG_IIN_HOST_MAX_UV  6400000

/*  Input voltage IDPM */
#define BQ25713_REG_IIN_DPM_HIGH    0x25
#define BQ25713_REG_IIN_DPM_MASK    BQ25713_REG_IIN_HOST_MASK
#define BQ25713_REG_IIN_DPM_STEP_UA BQ25713_REG_IIN_HOST_STEP_UA

/* Mininum system voltage */
#define BQ25713_REG_MIN_SYS_VOLTAGE_HI      0x0D
#define BQ25713_REG_MIN_SYS_VOLTAGE_MASK    GENMASK(5, 0)
#define BQ25713_REG_MIN_SYS_VOLTAGE_STEP_UV 256000
#define BQ25713_REG_MIN_SYS_VOLTAGE_MIN_UV  1024000
#define BQ25713_REG_MIN_SYS_VOLTAGE_MAX_UV  16128000

/*  Input voltage VDPM */
#define BQ25713_REG_VIN_LOW                0x0A
#define BQ25713_REG_VIN_DPM_MASK           GENMASK(13, 6)
#define BQ25713_REG_VIN_DPM_STEP_UV        64000
#define BQ25713_REG_VIN_DPM_OFFSET_UV      3200000
#define BQ25713_REG_VIN_DPM_VOLTAGE_MIN_UV BQ25713_REG_VIN_DPM_OFFSET_UV
#define BQ25713_REG_VIN_DPM_VOLTAGE_MAX_UV 195200000

/* Manufacture ID */
#define BQ25713_REG_ID_LOW       0x2E
#define BQ25713_REG_ID_PN_25713  0x4088
#define BQ25713_REG_ID_PN_25713B 0x408A

#define BQ25713_FACTOR_U_TO_M 1000

struct bq25713_config {
	struct i2c_dt_spec i2c;
	uint32_t vsys_min_uv;
	uint32_t ichg_ua;
	uint32_t vreg_uv;
};

static inline int bq25713_write8(const struct device *dev, uint8_t reg, uint8_t value)
{
	const struct bq25713_config *const config = dev->config;

	return i2c_reg_write_byte_dt(&config->i2c, reg, value);
}

static inline int bq25713_read8(const struct device *dev, uint8_t reg, uint8_t *value)
{
	const struct bq25713_config *const config = dev->config;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, reg, value);
	if (ret < 0) {
		LOG_ERR("Unable to read register");
	}
	return ret;
}

static inline int bq25713_update8(const struct device *dev, uint8_t reg, uint8_t mask,
				  uint8_t value)
{
	const struct bq25713_config *const config = dev->config;
	int ret;

	ret = i2c_reg_update_byte_dt(&config->i2c, reg, mask, value);
	if (ret < 0) {
		LOG_ERR("Unable to update register");
	}
	return ret;
}

static inline int bq25713_write16(const struct device *dev, uint8_t reg, uint16_t value)
{
	const struct bq25713_config *const config = dev->config;
	uint8_t buf[3];

	buf[0] = reg;
	sys_put_le16(value, &buf[1]);

	return i2c_write_dt(&config->i2c, buf, sizeof(buf));
}

static inline int bq25713_read16(const struct device *dev, uint8_t reg, uint16_t *value)
{
	const struct bq25713_config *config = dev->config;
	uint8_t i2c_data[2];
	int ret;

	ret = i2c_burst_read_dt(&config->i2c, reg, i2c_data, sizeof(i2c_data));
	if (ret < 0) {
		LOG_ERR("Unable to read register");
		return ret;
	}

	*value = sys_get_be16(i2c_data);

	return 0;
}

static int bq25713_set_minimum_system_voltage(const struct device *dev, uint32_t voltage_uv)
{
	if (!IN_RANGE(voltage_uv, BQ25713_REG_MIN_SYS_VOLTAGE_MIN_UV,
		      BQ25713_REG_MIN_SYS_VOLTAGE_MAX_UV)) {
		LOG_WRN("minimum system voltage out of range: %umV, "
			"clamping to the nearest limit",
			voltage_uv / BQ25713_FACTOR_U_TO_M);
	}

	uint32_t v;

	voltage_uv = CLAMP(voltage_uv, BQ25713_REG_MIN_SYS_VOLTAGE_MIN_UV,
			   BQ25713_REG_MIN_SYS_VOLTAGE_MAX_UV);
	v = voltage_uv / BQ25713_REG_MIN_SYS_VOLTAGE_STEP_UV;
	v = FIELD_PREP(BQ25713_REG_MIN_SYS_VOLTAGE_MASK, v);
	return bq25713_write8(dev, BQ25713_REG_MIN_SYS_VOLTAGE_HI, v);
}

static int bq25713_set_constant_charge_current(const struct device *dev, uint32_t current_ua)
{
	if (!IN_RANGE(current_ua, BQ25713_REG_CC_CHARGE_CURRENT_MIN_UA,
		      BQ25713_REG_CC_CHARGE_CURRENT_MAX_UA)) {
		LOG_WRN("charging current out of range: %umA, "
			"clamping to the nearest limit",
			current_ua / BQ25713_FACTOR_U_TO_M);
	}
	current_ua = CLAMP(current_ua, BQ25713_REG_CC_CHARGE_CURRENT_MIN_UA,
			   BQ25713_REG_CC_CHARGE_CURRENT_MAX_UA);
	uint32_t v;

	v = current_ua / BQ25713_REG_CC_CHARGE_CURRENT_STEP_UA;
	v = FIELD_PREP(BQ25713_REG_CC_CHARGE_CURRENT_MASK, v);

	return bq25713_write16(dev, BQ25713_REG_CC_LOW, v);
}

static int bq25713_set_constant_charge_voltage(const struct device *dev, uint32_t voltage_uv)
{
	if (!IN_RANGE(voltage_uv, BQ25713_REG_CV_CHARGE_VOLTAGE_MIN_UV,
		      BQ25713_REG_CV_CHARGE_VOLTAGE_MAX_UV)) {
		LOG_WRN("charging voltage out of range: %umV, "
			"clamping to the nearest limit",
			voltage_uv / BQ25713_FACTOR_U_TO_M);
	}

	uint32_t v;

	voltage_uv = CLAMP(voltage_uv, BQ25713_REG_CV_CHARGE_VOLTAGE_MIN_UV,
			   BQ25713_REG_CV_CHARGE_VOLTAGE_MAX_UV);
	v = voltage_uv / BQ25713_REG_CV_CHARGE_VOLTAGE_STEP_UV;
	v = FIELD_PREP(BQ25713_REG_CV_CHARGE_VOLTAGE_MASK, v);
	return bq25713_write16(dev, BQ25713_REG_CV_LOW, v);
}

static int bq25713_set_iindpm(const struct device *dev, uint32_t current_ua)
{
	if (!IN_RANGE(current_ua, BQ25713_REG_IIN_HOST_MIN_UV, BQ25713_REG_IIN_HOST_MAX_UV)) {
		LOG_WRN("input current regulation out of range: %umA, "
			"clamping to the nearest limit",
			current_ua / BQ25713_FACTOR_U_TO_M);
	}

	uint32_t v;

	current_ua = CLAMP(current_ua, BQ25713_REG_IIN_HOST_MIN_UV, BQ25713_REG_IIN_HOST_MAX_UV);
	v = current_ua / BQ25713_REG_IIN_HOST_STEP_UA;
	v = FIELD_PREP(BQ25713_REG_IIN_HOST_MASK, v);
	return bq25713_write8(dev, BQ25713_REG_IIN_HOST_HIGH, v);
}

static int bq25713_set_vindpm(const struct device *dev, uint32_t voltage_ua)
{
	if (!IN_RANGE(voltage_ua, BQ25713_REG_VIN_DPM_VOLTAGE_MIN_UV,
		      BQ25713_REG_VIN_DPM_VOLTAGE_MAX_UV)) {
		LOG_WRN("input voltage regulation of range: %umV, "
			"clamping to the nearest limit",
			voltage_ua / BQ25713_FACTOR_U_TO_M);
	}

	uint32_t v;

	voltage_ua = CLAMP(voltage_ua, BQ25713_REG_VIN_DPM_VOLTAGE_MIN_UV,
			   BQ25713_REG_VIN_DPM_VOLTAGE_MAX_UV);
	v = (voltage_ua - BQ25713_REG_VIN_DPM_OFFSET_UV) / BQ25713_REG_VIN_DPM_STEP_UV;
	v = FIELD_PREP(BQ25713_REG_VIN_DPM_MASK, v);
	return bq25713_write16(dev, BQ25713_REG_VIN_LOW, v);
}

static int bq25713_get_constant_charge_current(const struct device *dev, uint32_t *current_ua)
{
	uint16_t v;
	int ret;

	ret = bq25713_read16(dev, BQ25713_REG_CC_LOW, &v);
	if (ret < 0) {
		return ret;
	}

	v = FIELD_GET(BQ25713_REG_CC_CHARGE_CURRENT_MASK, v);

	*current_ua = v * BQ25713_REG_CC_CHARGE_CURRENT_STEP_UA;

	return 0;
}

static int bq25713_get_constant_charge_voltage(const struct device *dev, uint32_t *voltage_uv)
{
	uint16_t value;
	int ret;

	ret = bq25713_read16(dev, BQ25713_REG_CV_LOW, &value);
	if (ret < 0) {
		return ret;
	}
	value = FIELD_GET(BQ25713_REG_CV_CHARGE_VOLTAGE_MASK, value);

	*voltage_uv = value * BQ25713_REG_CV_CHARGE_VOLTAGE_STEP_UV;

	return 0;
}

static int bq25713_get_iindpm(const struct device *dev, uint32_t *current_ua)
{
	uint8_t value;
	int ret;

	ret = bq25713_read8(dev, BQ25713_REG_IIN_DPM_HIGH, &value);
	if (ret < 0) {
		return ret;
	}
	value = FIELD_GET(BQ25713_REG_IIN_DPM_MASK, value);

	*current_ua = value * BQ25713_REG_IIN_DPM_STEP_UA;

	return 0;
}

static int bq25713_get_vindpm(const struct device *dev, uint32_t *voltage_uv)
{
	uint16_t value;
	int ret;

	ret = bq25713_read16(dev, BQ25713_REG_VIN_LOW, &value);
	if (ret < 0) {
		return ret;
	}
	value = FIELD_GET(BQ25713_REG_VIN_DPM_MASK, value);

	*voltage_uv = (value * BQ25713_REG_VIN_DPM_STEP_UV) + BQ25713_REG_VIN_DPM_OFFSET_UV;

	return 0;
}

static int bq25713_get_status(const struct device *dev, enum charger_status *status)
{
	uint8_t charge_status;
	int ret;

	ret = bq25713_read8(dev, BQ25713_REG_CS_HIGH, &charge_status);
	if (ret < 0) {
		return ret;
	}

	switch (FIELD_GET(BQ25713_REG_CS_PRE_FAST_CHARGE, charge_status)) {
	case BQ25713_REG_CS_FASTCHARGE:
		__fallthrough;
	case BQ25713_REG_CS_PRECHARGE:
		*status = CHARGER_STATUS_CHARGING;
		break;
	default:
		*status = CHARGER_STATUS_UNKNOWN;
		break;
	}

	return 0;
}

static int bq25713_get_online(const struct device *dev, enum charger_online *online)
{
	uint8_t status;
	int ret;

	ret = bq25713_read8(dev, BQ25713_REG_CS_HIGH, &status);
	if (ret < 0) {
		return ret;
	}

	status = FIELD_GET(BQ25713_REG_CS_AC_STAT_MASK, status);
	if (status == BQ25713_REG_CS_AC_STAT) {
		*online = CHARGER_ONLINE_FIXED;
	} else {
		*online = CHARGER_ONLINE_OFFLINE;
	}

	return 0;
}

static int bq25713_charger_get_charge_type(const struct device *dev,
					   enum charger_charge_type *charge_type)
{
	*charge_type = CHARGER_CHARGE_TYPE_UNKNOWN;
	return 0;
}

static int bq25713_get_prop(const struct device *dev, charger_prop_t prop,
			    union charger_propval *value)
{
	switch (prop) {
	case CHARGER_PROP_ONLINE:
		return bq25713_get_online(dev, &value->online);
	case CHARGER_PROP_CHARGE_TYPE:
		return bq25713_charger_get_charge_type(dev, &value->charge_type);
	case CHARGER_PROP_STATUS:
		return bq25713_get_status(dev, &value->status);
	case CHARGER_PROP_CONSTANT_CHARGE_CURRENT_UA:
		return bq25713_get_constant_charge_current(dev, &value->const_charge_current_ua);
	case CHARGER_PROP_CONSTANT_CHARGE_VOLTAGE_UV:
		return bq25713_get_constant_charge_voltage(dev, &value->const_charge_voltage_uv);
	case CHARGER_PROP_INPUT_REGULATION_CURRENT_UA:
		return bq25713_get_iindpm(dev, &value->input_current_regulation_current_ua);
	case CHARGER_PROP_INPUT_REGULATION_VOLTAGE_UV:
		return bq25713_get_vindpm(dev, &value->input_voltage_regulation_voltage_uv);
	default:
		return -ENOTSUP;
	}
}

static int bq25713_set_prop(const struct device *dev, charger_prop_t prop,
			    const union charger_propval *value)
{
	switch (prop) {
	case CHARGER_PROP_CONSTANT_CHARGE_CURRENT_UA:
		return bq25713_set_constant_charge_current(dev, value->const_charge_current_ua);
	case CHARGER_PROP_CONSTANT_CHARGE_VOLTAGE_UV:
		return bq25713_set_constant_charge_voltage(dev, value->const_charge_voltage_uv);
	case CHARGER_PROP_INPUT_REGULATION_CURRENT_UA:
		return bq25713_set_iindpm(dev, value->input_current_regulation_current_ua);
	case CHARGER_PROP_INPUT_REGULATION_VOLTAGE_UV:
		return bq25713_set_vindpm(dev, value->input_voltage_regulation_voltage_uv);
	default:
		return -ENOTSUP;
	}
}

static int bq25713_charge_enable(const struct device *dev, const bool enable)
{
	uint8_t value = enable ? 0 : BQ25713_REG_CO0_INHIBIT;

	return bq25713_update8(dev, BQ25713_REG_CO0_LOW, BQ25713_REG_CO0_INHIBIT_MASK, value);
}

static int bq25713_set_config(const struct device *dev)
{
	const struct bq25713_config *const config = dev->config;
	union charger_propval value;
	int ret;

	value.const_charge_current_ua = config->ichg_ua;

	ret = bq25713_set_constant_charge_current(dev, value.const_charge_current_ua);
	if (ret < 0) {
		return ret;
	}

	value.const_charge_voltage_uv = config->vreg_uv;

	ret = bq25713_set_constant_charge_voltage(dev, value.const_charge_voltage_uv);
	if (ret < 0) {
		return ret;
	}

	return bq25713_set_minimum_system_voltage(dev, config->vsys_min_uv);
}

static int bq25713_init(const struct device *dev)
{
	uint16_t value;
	int ret;

	ret = bq25713_read16(dev, BQ25713_REG_ID_LOW, &value);
	if (ret < 0) {
		return ret;
	}

	switch (value) {
	case BQ25713_REG_ID_PN_25713:
		__fallthrough;
	case BQ25713_REG_ID_PN_25713B:
		break;
	default:
		LOG_ERR("Error unknown model: 0x%04x\n", value);
		return -ENODEV;
	}

	return bq25713_set_config(dev);
}

static DEVICE_API(charger, bq7513_driver_api) = {
	.get_property = bq25713_get_prop,
	.set_property = bq25713_set_prop,
	.charge_enable = bq25713_charge_enable,
};

#define BQ25713_INIT(inst)                                                                         \
                                                                                                   \
	static const struct bq25713_config bq25713_config_##inst = {                               \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.ichg_ua = DT_INST_PROP(inst, constant_charge_current_max_microamp),               \
		.vreg_uv = DT_INST_PROP(inst, constant_charge_voltage_max_microvolt),              \
		.vsys_min_uv = DT_INST_PROP(inst, system_voltage_min_threshold_microvolt),         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, bq25713_init, NULL, NULL, &bq25713_config_##inst, POST_KERNEL, \
			      CONFIG_CHARGER_INIT_PRIORITY, &bq7513_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BQ25713_INIT)
