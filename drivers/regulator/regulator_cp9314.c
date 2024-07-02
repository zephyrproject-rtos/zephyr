/*
 * Copyright (c) 2024 Cirrus Logic, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT cirrus_cp9314

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/sys_clock.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(CP9314, CONFIG_REGULATOR_LOG_LEVEL);

#define CP9314_REG_DEVICE_ID 0x0
#define CP9314_DEV_ID        0xA4

#define CP9314_REG_VOUT_UVP   0x2
#define CP9314_VOUT_UVP_DIS_0 BIT(7)
#define CP9314_VOUT_UVP_DIS_1 BIT(3)
#define CP9314_VOUT_UVP_DIS   CP9314_VOUT_UVP_DIS_0 | CP9314_VOUT_UVP_DIS_1
#define CP9314_VOUT_UVP       GENMASK(1, 0)

#define CP9314_REG_OPTION_REG_1 0x3
#define CP9314_LB1_DELAY_CFG    GENMASK(5, 4)
#define CP9314_LB1_DELTA_CFG_0  GENMASK(3, 0)

#define CP9314_REG_OPTION_REG_2     0x4
#define CP9314_LB2_DELTA_CFG_0      GENMASK(7, 5)
#define CP9314_MODE_CTRL_MIN_FREQ_0 GENMASK(2, 0)

#define CP9314_REG_IIN_OCP   0x5
#define CP9314_IIN_OCP_DIS   BIT(7)
#define CP9314_TM_IIN_OC_CFG GENMASK(2, 0)

#define CP9314_REG_IIN_PEAK_OCP 0x6
#define CP9314_IIN_PEAK_OCP_DIS BIT(7)
#define CP9314_IIN_PEAK_OCP     GENMASK(2, 0)

#define CP9314_REG_VIN2OUT_OVP 0x7
#define CP9314_VIN2OUT_OVP     GENMASK(1, 0)

#define CP9314_REG_VIN2OUT_UVP 0x8
#define CP9314_VIN2OUT_UVP     GENMASK(1, 0)

#define CP9314_REG_CONVERTER    0x9
#define CP9314_FASTSHDN_PIN_STS BIT(6)
#define CP9314_PGOOD_PIN_STS    BIT(5)
#define CP9314_ACTIVE_STS       BIT(1)

#define CP9314_REG_CTRL1    0xA
#define CP9314_CP_EN        BIT(7)
#define CP9314_MODE_CTRL_EN BIT(3)

#define CP9314_REG_CTRL4        0xD
#define CP9314_SYNC_FUNCTION_EN BIT(7)
#define CP9314_SYNC_HOST_EN     BIT(6)
#define CP9314_FRC_SYNC_MODE    BIT(5)
#define CP9314_FRC_OP_MODE      BIT(3)
#define CP9314_MODE_MASK        GENMASK(2, 0)
#define CP9314_MODE_2TO1        1
#define CP9314_MODE_3TO1        2

#define CP9314_REG_FLT_FLAG   0x12
#define CP9314_VIN_OVP_FLAG   BIT(1)
#define CP9314_VOUT_OVP_FLAG  BIT(0)

#define CP9314_REG_COMP_FLAG0 0x2A
#define CP9314_IIN_OCP_FLAG   BIT(4)

#define CP9314_REG_COMP_FLAG1 0x2C
#define CP9314_VIN2OUT_OVP_FLAG BIT(0)

#define CP9314_REG_LION_CFG_1  0x31
#define CP9314_LB2_DELTA_CFG_1 GENMASK(7, 5)

#define CP9314_REG_LION_INT_MASK_2 0x32
#define CP9314_CLEAR_INT           BIT(6)

#define CP9314_REG_LION_CFG_3        0x34
#define CP9314_LB_MIN_FREQ_SEL_0     GENMASK(7, 6)
#define CP9314_MODE_CTRL_UPDATE_BW_1 GENMASK(5, 3)
#define CP9314_ALLOW_HW_I2C_LOCK     BIT(0)

#define CP9314_REG_LB_CTRL       0x38
#define CP9314_LB1_DELTA_CFG_1   GENMASK(6, 3)
#define CP9314_LB_MIN_FREQ_SEL_1 GENMASK(2, 1)

#define CP9314_REG_CRUS_CTRL       0x40
#define CP9314_CRUS_KEY_LOCK       0x0
#define CP9314_CRUS_KEY_UNLOCK     0xAA
#define CP9314_CRUS_KEY_SOFT_RESET 0xC6

#define CP9314_REG_TRIM_5  0x46
#define CP9314_CSI_CHOP_EN BIT(2)

#define CP9314_REG_TRIM_8            0x49
#define CP9314_MODE_CTRL_UPDATE_BW_0 GENMASK(2, 0)

#define CP9314_REG_TRIM_9         0x4A
#define CP9314_FORCE_KEY_POLARITY BIT(2)
#define CP9314_TM_KEY_POLARITY    BIT(1)
#define CP9314_KEY_ACTIVE_LOW     0
#define CP9314_KEY_ACTIVE_HIGH    CP9314_TM_KEY_POLARITY

#define CP9314_REG_BST_CP_PD_CFG 0x58
#define CP9314_LB1_BLANK_CFG     BIT(5)

#define CP9314_REG_CFG_9            0x59
#define CP9314_VOUT_PCHG_TIME_CFG_0 GENMASK(2, 1)

#define CP9314_REG_CFG_10           0x5A
#define CP9314_VOUT_PCHG_TIME_CFG_1 GENMASK(1, 0)

#define CP9314_REG_BC_STS_C  0x62
#define CP9314_CHIP_REV_MASK GENMASK(7, 4)
#define CP9314_CHIP_REV_B1   0x3

#define CP9314_REG_FORCE_SC_MISC 0x69
#define CP9314_FORCE_CSI_EN      BIT(0)

#define CP9314_REG_TSBAT_CTRL     0x72
#define CP9314_LB1_STOP_PHASE_SEL BIT(4)

#define CP9314_REG_TEST_MODE_CTRL 0x66
#define CP9314_SOFT_RESET_REQ     BIT(0)

#define CP9314_REG_LION_COMP_CTRL_1 0x79
#define CP9314_VIN_SWITCH_OK_DIS_0  BIT(3)
#define CP9314_VOUT_OV_CFG_0        GENMASK(5, 4)
#define CP9314_VIN_SWITCH_OK_CFG    GENMASK(1, 0)

#define CP9314_REG_LION_COMP_CTRL_2 0x7A
#define CP9314_VOUT_OV_CFG_1        GENMASK(3, 2)

#define CP9314_REG_LION_COMP_CTRL_3 0x7B
#define CP9314_VIN_OV_CFG_0         GENMASK(4, 3)
#define CP9314_VIN_OV_CFG_1         GENMASK(1, 0)
#define CP9314_VIN_OV_CFG           CP9314_VIN_OV_CFG_0 | CP9314_VIN_OV_CFG_1

#define CP9314_REG_LION_COMP_CTRL_4 0x7C
#define CP9314_FORCE_IIN_OC_CFG     BIT(1)
#define CP9314_VIN_SWITCH_OK_DIS_1  BIT(5)

#define CP9314_REG_PTE_REG_2 0x8B
#define CP9314_PTE_2_MASK    GENMASK(7, 5)
#define CP9314_PTE_2_OTP_1   0x0
#define CP9314_PTE_2_OTP_2   0x1

#define CP9314_REG_BACKDOOR_CTRL 0x8C

#define CP9314_FAULT1_STS 0x9A
#define CP9314_VIN_OV_STS BIT(4)

#define CP9314_SYS_STS    0x98
#define CP9314_VIN_UV_STS BIT(7)

#define CP9314_REG_TM_SEQ_CTRL_1 0xAA
#define CP9314_TM_CSI_EN         BIT(5)

#define CP9314_REG_STS_PIN_ADC_0 0xB4
#define CP9314_STS_PROG_LVL      GENMASK(7, 4)
#define CP9314_STS_ADDR_LVL      GENMASK(3, 0)

#define CP9314_SOFT_RESET_DELAY_MSEC 200
#define CP9314_EN_DEBOUNCE_USEC      3000
#define CP9314_T_STARTUP_MSEC        120

#define CP9314_DEVICE_MODE_HOST_4GANG_0x78      0x0
#define CP9314_DEVICE_MODE_HOST_4GANG_0x72      0x1
#define CP9314_DEVICE_MODE_HOST_3GANG_0x78      0x2
#define CP9314_DEVICE_MODE_HOST_3GANG_0x72      0x3
#define CP9314_DEVICE_MODE_HOST_2GANG_0x78      0x4
#define CP9314_DEVICE_MODE_HOST_2GANG_0x72      0x5
#define CP9314_DEVICE_MODE_HOST_STANDALONE_0x78 0x6
#define CP9314_DEVICE_MODE_HOST_STANDALONE_0x72 0x7
#define CP9314_DEVICE_MODE_DEVICE_4_0x68        0x8
#define CP9314_DEVICE_MODE_DEVICE_4_0x54        0x9
#define CP9314_DEVICE_MODE_DEVICE_3_0x56        0xA
#define CP9314_DEVICE_MODE_DEVICE_3_0x53        0xB
#define CP9314_DEVICE_MODE_DEVICE_2_0x79        0xC
#define CP9314_DEVICE_MODE_DEVICE_2_0x73        0xD

enum cp9314_sync_roles {
	CP9314_ROLE_HOST,
	CP9314_ROLE_DEV2,
	CP9314_ROLE_DEV3,
	CP9314_ROLE_DEV4,
	CP9314_ROLE_STANDALONE,
};

enum cp9314_backdoor_keys {
	CP9314_BACKDOOR_LOCKED_KEY = 0x0,
	CP9314_BACKDOOR_PUBLIC_KEY = 0x0F,
};

struct regulator_cp9314_config {
	struct regulator_common_config common;
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec en_pin;
	struct gpio_dt_spec pgood_pin;
	uint8_t initial_op_mode_idx;
	bool hw_i2c_lock;
};

struct regulator_cp9314_data {
	struct regulator_common_data data;
	enum cp9314_sync_roles sync_role;
	uint8_t backdoor_key;
	bool allow_hw_i2c_lock;
};

struct cp9314_reg_patch {
	uint8_t reg_addr;
	uint8_t mask;
	uint8_t value;
};

/* OTP memory errata patch for OTP v1. Corrects trim errata. */
static struct cp9314_reg_patch otp_1_patch[3] = {
	{CP9314_REG_OPTION_REG_1, CP9314_LB1_DELAY_CFG, 0},
	{CP9314_REG_BST_CP_PD_CFG, CP9314_LB1_BLANK_CFG, CP9314_LB1_BLANK_CFG},
	{CP9314_REG_TSBAT_CTRL, CP9314_LB1_STOP_PHASE_SEL, CP9314_LB1_STOP_PHASE_SEL},
};

static int regulator_cp9314_get_error_flags(const struct device *dev,
					    regulator_error_flags_t *flags)
{
	const struct regulator_cp9314_config *config = dev->config;
	uint8_t val[3];
	int ret;

	*flags = 0U;

	ret = i2c_reg_read_byte_dt(&config->i2c, CP9314_REG_FLT_FLAG, &val[0]);
	if (ret < 0) {
		return ret;
	}

	if (FIELD_GET(CP9314_VIN_OVP_FLAG, val[0]) || FIELD_GET(CP9314_VOUT_OVP_FLAG, val[0])) {
		*flags |= REGULATOR_ERROR_OVER_VOLTAGE;
	}

	ret = i2c_reg_read_byte_dt(&config->i2c, CP9314_REG_COMP_FLAG0, &val[1]);
	if (ret < 0) {
		return ret;
	}

	if (FIELD_GET(CP9314_IIN_OCP_FLAG, val[1])) {
		*flags |= REGULATOR_ERROR_OVER_CURRENT;
	}

	ret = i2c_reg_read_byte_dt(&config->i2c, CP9314_REG_COMP_FLAG1, &val[2]);
	if (ret < 0) {
		return ret;
	}

	if (FIELD_GET(CP9314_VIN2OUT_OVP_FLAG, val[2])) {
		*flags |= REGULATOR_ERROR_OVER_VOLTAGE;
	}

	LOG_DBG("FLT_FLAG = 0x%x, COMP_FLAG0 = 0x%x, COMP_FLAG1 = 0x%x", val[0], val[1], val[2]);

	return 0;
}

static int regulator_cp9314_write_lock(const struct device *dev,
				       const enum cp9314_backdoor_keys key)
{
	const struct regulator_cp9314_config *config = dev->config;
	struct regulator_cp9314_data *data = dev->data;
	int ret;

	if (data->allow_hw_i2c_lock == 0U) {
		ret = i2c_reg_update_byte_dt(&config->i2c, CP9314_REG_LION_CFG_3,
					     CP9314_ALLOW_HW_I2C_LOCK, CP9314_ALLOW_HW_I2C_LOCK);
		if (ret < 0) {
			return ret;
		}

		data->allow_hw_i2c_lock = true;
	}

	if ((uint8_t)key == data->backdoor_key) {
		return 0;
	} else {
		return i2c_reg_write_byte_dt(&config->i2c, CP9314_REG_BACKDOOR_CTRL, (uint8_t)key);
	}
}

static int regulator_cp9314_write_lock_init(const struct device *dev)
{
	const struct regulator_cp9314_config *config = dev->config;
	struct regulator_cp9314_data *data = dev->data;
	uint8_t value;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, CP9314_REG_LION_CFG_3, &value);
	if (ret < 0) {
		return ret;
	}

	data->allow_hw_i2c_lock = FIELD_GET(CP9314_ALLOW_HW_I2C_LOCK, value);

	ret = i2c_reg_read_byte_dt(&config->i2c, CP9314_REG_BACKDOOR_CTRL, &data->backdoor_key);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int regulator_cp9314_disable(const struct device *dev)
{
	const struct regulator_cp9314_config *config = dev->config;
	int ret;

	if (config->en_pin.port != NULL) {
		return gpio_pin_set_dt(&config->en_pin, 0);
	}

	if (config->hw_i2c_lock != 0U) {
		ret = regulator_cp9314_write_lock(dev, CP9314_BACKDOOR_PUBLIC_KEY);
		if (ret < 0) {
			return ret;
		}

		ret = i2c_reg_update_byte_dt(&config->i2c, CP9314_REG_CTRL1, CP9314_CP_EN, 0);
		if (ret < 0) {
			return ret;
		}

		return regulator_cp9314_write_lock(dev, CP9314_BACKDOOR_LOCKED_KEY);
	}

	return i2c_reg_update_byte_dt(&config->i2c, CP9314_REG_CTRL1, CP9314_CP_EN, 0);
}

static int regulator_cp9314_enable(const struct device *dev)
{
	const struct regulator_cp9314_config *config = dev->config;
	uint8_t value;
	int ret;

	if (config->hw_i2c_lock != 0U) {
		ret = regulator_cp9314_write_lock(dev, CP9314_BACKDOOR_PUBLIC_KEY);
		if (ret < 0) {
			return ret;
		}
	}

	ret = i2c_reg_read_byte_dt(&config->i2c, CP9314_REG_CONVERTER, &value);
	if (ret < 0) {
		return ret;
	}

	if (value & CP9314_ACTIVE_STS) {
		return 0;
	}

	ret = i2c_reg_update_byte_dt(&config->i2c, CP9314_REG_LION_INT_MASK_2, CP9314_CLEAR_INT,
				     CP9314_CLEAR_INT);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_reg_update_byte_dt(&config->i2c, CP9314_REG_LION_INT_MASK_2, CP9314_CLEAR_INT, 0);
	if (ret < 0) {
		return ret;
	}

	if (config->en_pin.port != NULL) {
		ret = gpio_pin_set_dt(&config->en_pin, 1);
		if (ret < 0) {
			return ret;
		}
	} else {
		ret = i2c_reg_update_byte_dt(&config->i2c, CP9314_REG_CTRL1, CP9314_CP_EN,
					     CP9314_CP_EN);
		if (ret < 0) {
			LOG_ERR("Unable to set CP_EN");
			return ret;
		}
	}

	k_msleep(CP9314_T_STARTUP_MSEC);

	if (config->pgood_pin.port != NULL) {
		ret = gpio_pin_get_dt(&config->pgood_pin);
		if (ret < 0) {
			return ret;
		} else if (ret == 0) {
			return -EINVAL;
		}
	} else {
		ret = i2c_reg_read_byte_dt(&config->i2c, CP9314_REG_CONVERTER, &value);
		if (ret < 0) {
			return ret;
		} else if (FIELD_GET(CP9314_PGOOD_PIN_STS, value) == 0U) {
			return -EINVAL;
		}
	}

	if (config->hw_i2c_lock != 0U) {
		ret = regulator_cp9314_write_lock(dev, CP9314_BACKDOOR_LOCKED_KEY);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int cp9314_cfg_sync(const struct device *dev)
{
	const struct regulator_cp9314_config *config = dev->config;
	struct regulator_cp9314_data *data = dev->data;
	uint8_t value = 0;
	int ret;

	if (data->sync_role == CP9314_ROLE_HOST) {
		value = CP9314_SYNC_HOST_EN;
	}

	ret = i2c_reg_update_byte_dt(&config->i2c, CP9314_REG_CTRL4, CP9314_SYNC_HOST_EN, value);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_reg_update_byte_dt(&config->i2c, CP9314_REG_CTRL4, CP9314_SYNC_FUNCTION_EN,
				     CP9314_SYNC_FUNCTION_EN);
	if (ret < 0) {
		return ret;
	}

	return i2c_reg_update_byte_dt(&config->i2c, CP9314_REG_CTRL4, CP9314_FRC_SYNC_MODE,
				      CP9314_FRC_SYNC_MODE);
}

static int cp9314_do_soft_reset(const struct device *dev)
{
	const struct regulator_cp9314_config *config = dev->config;
	int ret;

	if (config->hw_i2c_lock != 0U) {
		ret = regulator_cp9314_write_lock(dev, CP9314_BACKDOOR_PUBLIC_KEY);
		if (ret < 0) {
			return ret;
		}

		ret = i2c_reg_update_byte_dt(&config->i2c, CP9314_REG_LION_CFG_3,
					     CP9314_ALLOW_HW_I2C_LOCK, 0);
		if (ret < 0) {
			return ret;
		}
	}

	ret = i2c_reg_write_byte_dt(&config->i2c, CP9314_REG_CRUS_CTRL, CP9314_CRUS_KEY_SOFT_RESET);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_reg_update_byte_dt(&config->i2c, CP9314_REG_TEST_MODE_CTRL, CP9314_SOFT_RESET_REQ,
				     CP9314_SOFT_RESET_REQ);
	if (ret < 0) {
		return ret;
	}

	k_msleep(CP9314_SOFT_RESET_DELAY_MSEC);

	if (config->hw_i2c_lock != 0U) {
		ret = regulator_cp9314_write_lock_init(dev);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int regulator_cp9314_otp_init(const struct device *dev)
{
	const struct regulator_cp9314_config *config = dev->config;
	uint8_t value;
	int i, ret;

	/*
	 * The PTE_2 field in the PTE_REG_2 register contains the value representing the OTP
	 * burned on the CP9314 device. The PTE_2 values in relation to the OTP table names
	 * are shown below.
	 *
	 * OTP-1 = 0x0, OTP-2 = 0x1, OTP-3 = 0x3, OTP-4 = 0x4
	 */

	ret = i2c_reg_read_byte_dt(&config->i2c, CP9314_REG_PTE_REG_2, &value);
	if (ret < 0) {
		return ret;
	}

	value = FIELD_GET(CP9314_PTE_2_MASK, value);

	ret = i2c_reg_write_byte_dt(&config->i2c, CP9314_REG_CRUS_CTRL, CP9314_CRUS_KEY_UNLOCK);
	if (ret < 0) {
		return ret;
	}

	if (value == CP9314_PTE_2_OTP_1) {
		for (i = 0; i < ARRAY_SIZE(otp_1_patch); i++) {
			i2c_reg_update_byte_dt(&config->i2c, otp_1_patch[i].reg_addr,
					       otp_1_patch[i].mask, otp_1_patch[i].value);
		}
	}

	if (value <= CP9314_PTE_2_OTP_2) {
		i2c_reg_update_byte_dt(&config->i2c, CP9314_REG_CFG_9, CP9314_VOUT_PCHG_TIME_CFG_0,
				       0);
	}

	return i2c_reg_write_byte_dt(&config->i2c, CP9314_REG_CRUS_CTRL, CP9314_CRUS_KEY_LOCK);
}

static int regulator_cp9314_init(const struct device *dev)
{
	const struct regulator_cp9314_config *config = dev->config;
	struct regulator_cp9314_data *data = dev->data;
	uint8_t value;
	int ret;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	ret = i2c_reg_read_byte_dt(&config->i2c, CP9314_REG_DEVICE_ID, &value);
	if (ret < 0) {
		LOG_ERR("No device found:%d\n", ret);
		return ret;
	}

	if (value != CP9314_DEV_ID) {
		LOG_ERR("Invalid device ID found:0x%x!\n", value);
		return -ENOTSUP;
	}

	if (config->pgood_pin.port != NULL) {
		if (!gpio_is_ready_dt(&config->pgood_pin)) {
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&config->pgood_pin, GPIO_INPUT);
		if (ret < 0) {
			return ret;
		}
	}

	if (config->en_pin.port != NULL) {
		if (!gpio_is_ready_dt(&config->en_pin)) {
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&config->en_pin, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			return ret;
		}

		k_usleep(CP9314_EN_DEBOUNCE_USEC);
	}

	if (config->hw_i2c_lock != 0U) {
		ret = regulator_cp9314_write_lock_init(dev);
		if (ret < 0) {
			return ret;
		}
	} else {
		data->allow_hw_i2c_lock = 0;
	}

	ret = cp9314_do_soft_reset(dev);
	if (ret < 0) {
		return ret;
	}

	if (data->allow_hw_i2c_lock != 0U) {
		ret = i2c_reg_update_byte_dt(&config->i2c, CP9314_REG_LION_CFG_3,
					     CP9314_ALLOW_HW_I2C_LOCK, 0x0);
		if (ret < 0) {
			return ret;
		}

		data->allow_hw_i2c_lock = false;
	}

	ret = i2c_reg_read_byte_dt(&config->i2c, CP9314_REG_BC_STS_C, &value);
	if (ret < 0) {
		return ret;
	}

	value = FIELD_GET(CP9314_CHIP_REV_MASK, value);

	switch (value) {
	case CP9314_CHIP_REV_B1:
		break;
	default:
		LOG_ERR("Invalid CP9314 REV:0x%x\n", value);
		return -ENOTSUP;
	}

	LOG_DBG("Found CP9314 REV:0x%x\n", value);

	ret = regulator_cp9314_otp_init(dev);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_reg_update_byte_dt(&config->i2c, CP9314_REG_CTRL4, CP9314_FRC_OP_MODE,
				     CP9314_FRC_OP_MODE);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_reg_read_byte_dt(&config->i2c, CP9314_REG_STS_PIN_ADC_0, &value);
	if (ret < 0) {
		return ret;
	}

	value = FIELD_PREP(CP9314_STS_ADDR_LVL, value);

	switch (value) {
	case CP9314_DEVICE_MODE_HOST_4GANG_0x78:
	case CP9314_DEVICE_MODE_HOST_4GANG_0x72:
	case CP9314_DEVICE_MODE_HOST_3GANG_0x78:
	case CP9314_DEVICE_MODE_HOST_3GANG_0x72:
	case CP9314_DEVICE_MODE_HOST_2GANG_0x78:
	case CP9314_DEVICE_MODE_HOST_2GANG_0x72:
		data->sync_role = CP9314_ROLE_HOST;
		break;
	case CP9314_DEVICE_MODE_HOST_STANDALONE_0x78:
	case CP9314_DEVICE_MODE_HOST_STANDALONE_0x72:
		data->sync_role = CP9314_ROLE_STANDALONE;
		break;
	case CP9314_DEVICE_MODE_DEVICE_4_0x68:
	case CP9314_DEVICE_MODE_DEVICE_4_0x54:
		data->sync_role = CP9314_ROLE_DEV4;
		break;
	case CP9314_DEVICE_MODE_DEVICE_3_0x56:
	case CP9314_DEVICE_MODE_DEVICE_3_0x53:
		data->sync_role = CP9314_ROLE_DEV3;
		break;
	case CP9314_DEVICE_MODE_DEVICE_2_0x79:
	case CP9314_DEVICE_MODE_DEVICE_2_0x73:
		data->sync_role = CP9314_ROLE_DEV2;
		break;
	default:
		return -EINVAL;
	}

	if (data->sync_role != CP9314_ROLE_STANDALONE) {
		ret = cp9314_cfg_sync(dev);
		if (ret < 0) {
			return ret;
		}
	}

	if (config->initial_op_mode_idx != 0) {
		ret = i2c_reg_update_byte_dt(&config->i2c, CP9314_REG_CTRL4, CP9314_MODE_MASK,
					     config->initial_op_mode_idx);
		if (ret < 0) {
			return ret;
		}
	}

	if (config->hw_i2c_lock != 0U) {
		ret = regulator_cp9314_write_lock(dev, CP9314_BACKDOOR_LOCKED_KEY);
		if (ret < 0) {
			return ret;
		}
	}

	regulator_common_data_init(dev);

	return regulator_common_init(dev, false);
}

static const struct regulator_driver_api api = {
	.enable = regulator_cp9314_enable,
	.disable = regulator_cp9314_disable,
	.get_error_flags = regulator_cp9314_get_error_flags,
};

#define REGULATOR_CP9314_DEFINE(inst)                                                              \
	static struct regulator_cp9314_data data_##inst;                                           \
                                                                                                   \
	static const struct regulator_cp9314_config config_##inst = {                              \
		.common = REGULATOR_DT_INST_COMMON_CONFIG_INIT(inst),                              \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.en_pin = GPIO_DT_SPEC_INST_GET_OR(inst, cirrus_en_gpios, {}),                     \
		.pgood_pin = GPIO_DT_SPEC_INST_GET_OR(inst, cirrus_pgood_gpios, {}),               \
		.initial_op_mode_idx =                                                             \
			DT_INST_ENUM_IDX_OR(inst, cirrus_initial_switched_capacitor_mode, -1) + 1, \
		.hw_i2c_lock = DT_INST_PROP(inst, cirrus_hw_i2c_lock),                             \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, regulator_cp9314_init, NULL, &data_##inst, &config_##inst,     \
			      POST_KERNEL, CONFIG_REGULATOR_CP9314_INIT_PRIORITY, &api);

DT_INST_FOREACH_STATUS_OKAY(REGULATOR_CP9314_DEFINE)
