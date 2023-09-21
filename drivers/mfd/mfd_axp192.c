/*
 * Copyright (c) 2023 Martin Kiepfer <mrmarteng@teleschirm.org>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT x_powers_axp192

#include <errno.h>
#include <stdbool.h>

#include <zephyr/drivers/mfd/axp192.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(mfd_axp192, CONFIG_MFD_LOG_LEVEL);

/* Chip ID value */
#define AXP192_CHIP_ID 0x03U

/* Registers definitions */
#define AXP192_REG_CHIP_ID 0x03U

/* AXP192 GPIO register addresses */
#define AXP192_GPIO0_REG_FUNC       0x90U
#define AXP192_GPIO1_REG_FUNC       0x92U
#define AXP192_GPIO2_REG_FUNC       0x93U
#define AXP192_GPIO34_REG_FUNC      0x95U
#define AXP192_GPIO012_REG_PINVAL   0x94U
#define AXP192_GPIO34_REG_PINVAL    0x96U
#define AXP192_GPIO012_REG_PULLDOWN 0x97U

/* GPIO function control parameters */
#define AXP192_GPIO012_FUNC_VAL_OUTPUT_OD  0x00U
#define AXP192_GPIO012_FUNC_VAL_INPUT      0x01U
#define AXP192_GPIO012_FUNC_VAL_LDO        0x02U /* only applicable for GPIO0 */
#define AXP192_GPIO012_FUNC_VAL_ADC        0x04U
#define AXP192_GPIO012_FUNC_VAL_OUTPUT_LOW 0x05U
#define AXP192_GPIO012_FUNC_VAL_FLOAT      0x06U
#define AXP192_GPIO012_FUNC_MASK                                                                   \
	(AXP192_GPIO012_FUNC_VAL_OUTPUT_OD | AXP192_GPIO012_FUNC_VAL_INPUT |                       \
	 AXP192_GPIO012_FUNC_VAL_LDO | AXP192_GPIO012_FUNC_VAL_ADC |                               \
	 AXP192_GPIO012_FUNC_VAL_OUTPUT_LOW | AXP192_GPIO012_FUNC_VAL_FLOAT)

#define AXP192_GPIO34_FUNC_ENA           0x80U
#define AXP192_GPIO3_FUNC_VAL_CHARGE_CTL 0x00U
#define AXP192_GPIO3_FUNC_VAL_OUTPUT_OD  0x01U
#define AXP192_GPIO3_FUNC_VAL_INPUT      0x02U
#define AXP192_GPIO3_FUNC_MASK                                                                     \
	(AXP192_GPIO34_FUNC_ENA | AXP192_GPIO3_FUNC_VAL_CHARGE_CTL |                               \
	 AXP192_GPIO3_FUNC_VAL_OUTPUT_OD | AXP192_GPIO3_FUNC_VAL_INPUT)

#define AXP192_GPIO4_FUNC_VAL_CHARGE_CTL 0x00U
#define AXP192_GPIO4_FUNC_VAL_OUTPUT_OD  0x04U
#define AXP192_GPIO4_FUNC_VAL_INPUT      0x08U
#define AXP192_GPIO4_FUNC_VAL_ADC        0x0CU
#define AXP192_GPIO4_FUNC_MASK                                                                     \
	(AXP192_GPIO34_FUNC_ENA | AXP192_GPIO4_FUNC_VAL_CHARGE_CTL |                               \
	 AXP192_GPIO4_FUNC_VAL_OUTPUT_OD | AXP192_GPIO4_FUNC_VAL_INPUT)

/* Pull-Down enable parameters */
#define AXP192_GPIO0_PULLDOWN_ENABLE 0x01U
#define AXP192_GPIO1_PULLDOWN_ENABLE 0x02U
#define AXP192_GPIO2_PULLDOWN_ENABLE 0x04U

/* GPIO Value parameters */
#define AXP192_GPIO0_INPUT_VAL      0x10U
#define AXP192_GPIO1_INPUT_VAL      0x20U
#define AXP192_GPIO2_INPUT_VAL      0x40U
#define AXP192_GPIO012_INTPUT_SHIFT 4U
#define AXP192_GPIO012_INTPUT_MASK                                                                 \
	(AXP192_GPIO0_INPUT_VAL | AXP192_GPIO1_INPUT_VAL | AXP192_GPIO2_INPUT_VAL)
#define AXP192_GPIO3_INPUT_VAL     0x10U
#define AXP192_GPIO4_INPUT_VAL     0x20U
#define AXP192_GPIO34_INTPUT_SHIFT 4U
#define AXP192_GPIO34_INTPUT_MASK  (AXP192_GPIO3_INPUT_VAL | AXP192_GPIO4_INPUT_VAL)

#define AXP192_GPIO0_OUTPUT_VAL 0x01U
#define AXP192_GPIO1_OUTPUT_VAL 0x02U
#define AXP192_GPIO2_OUTPUT_VAL 0x04U
#define AXP192_GPIO012_OUTPUT_MASK                                                                 \
	(AXP192_GPIO0_OUTPUT_VAL | AXP192_GPIO1_OUTPUT_VAL | AXP192_GPIO2_OUTPUT_VAL)
#define AXP192_GPIO3_OUTPUT_VAL   0x01U
#define AXP192_GPIO4_OUTPUT_VAL   0x02U
#define AXP192_GPIO34_OUTPUT_MASK (AXP192_GPIO3_OUTPUT_VAL | AXP192_GPIO4_OUTPUT_VAL)

struct mfd_axp192_config {
	struct i2c_dt_spec i2c;
};

struct mfd_axp192_data {
	const struct device *gpio_mask_used[AXP192_GPIO_MAX_NUM];
	uint8_t gpio_mask_output;
};

struct mfd_axp192_func_reg_desc {
	uint8_t reg;
	uint8_t mask;
};

const struct mfd_axp192_func_reg_desc gpio_reg_desc[AXP192_GPIO_MAX_NUM] = {
	{
		/* GPIO0 */
		.reg = AXP192_GPIO0_REG_FUNC,
		.mask = AXP192_GPIO012_FUNC_MASK,
	},
	{
		/* GPIO1 */
		.reg = AXP192_GPIO1_REG_FUNC,
		.mask = AXP192_GPIO012_FUNC_MASK,
	},
	{
		/* GPIO2 */
		.reg = AXP192_GPIO2_REG_FUNC,
		.mask = AXP192_GPIO012_FUNC_MASK,
	},
	{
		/* GPIO3 */
		.reg = AXP192_GPIO34_REG_FUNC,
		.mask = AXP192_GPIO3_FUNC_MASK,
	},
	{
		/* GPIO4 */
		.reg = AXP192_GPIO34_REG_FUNC,
		.mask = AXP192_GPIO4_FUNC_MASK,
	},
};

static int mfd_axp192_init(const struct device *dev)
{
	const struct mfd_axp192_config *config = dev->config;
	uint8_t chip_id;
	int ret;

	LOG_DBG("Initializing instance");

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	/* Check if axp192 chip is available */
	ret = i2c_reg_read_byte_dt(&config->i2c, AXP192_REG_CHIP_ID, &chip_id);
	if (ret < 0) {
		return ret;
	}

	if (chip_id != AXP192_CHIP_ID) {
		LOG_ERR("Invalid Chip detected (%d)", chip_id);
		return -EINVAL;
	}

	return 0;
}

int mfd_axp192_gpio_func_get(const struct device *dev, uint8_t gpio, enum axp192_gpio_func *func)
{
	const struct mfd_axp192_config *config = dev->config;
	int ret;
	uint8_t reg_fnc;

	if (gpio >= AXP192_GPIO_MAX_NUM) {
		LOG_ERR("Invalid gpio (%d)", gpio);
		return -EINVAL;
	}

	ret = i2c_reg_read_byte_dt(&(config->i2c), gpio_reg_desc[gpio].reg, &reg_fnc);
	if (ret != 0) {
		return ret;
	}

	switch (gpio) {
	case 0U:
		__fallthrough;
	case 1U:
		__fallthrough;
	case 2U:
		/* GPIO 0-2*/
		switch (reg_fnc) {
		case AXP192_GPIO012_FUNC_VAL_INPUT:
			*func = AXP192_GPIO_FUNC_INPUT;
			break;
		case AXP192_GPIO012_FUNC_VAL_OUTPUT_OD:
			*func = AXP192_GPIO_FUNC_OUTPUT_OD;
			break;
		case AXP192_GPIO012_FUNC_VAL_OUTPUT_LOW:
			*func = AXP192_GPIO_FUNC_OUTPUT_LOW;
			break;
		case AXP192_GPIO012_FUNC_VAL_LDO:
			if (gpio == 0) {
				/* LDO is only applicable on GPIO0 */
				*func = AXP192_GPIO_FUNC_LDO;
			} else {
				ret = -ENOTSUP;
			}
			break;
		case AXP192_GPIO012_FUNC_VAL_ADC:
			*func = AXP192_GPIO_FUNC_ADC;
			break;
		case AXP192_GPIO012_FUNC_VAL_FLOAT:
			*func = AXP192_GPIO_FUNC_FLOAT;
			break;
		default:
			ret = -ENOTSUP;
			break;
		}
		break;

	case 3U:
		/* GPIO3 */
		switch (reg_fnc) {
		case (AXP192_GPIO3_FUNC_VAL_INPUT | AXP192_GPIO34_FUNC_ENA):
			*func = AXP192_GPIO_FUNC_INPUT;
			break;
		case (AXP192_GPIO3_FUNC_VAL_OUTPUT_OD | AXP192_GPIO34_FUNC_ENA):
			*func = AXP192_GPIO_FUNC_OUTPUT_OD;
			break;
		case AXP192_GPIO3_FUNC_VAL_CHARGE_CTL:
			*func = AXP192_GPIO_FUNC_CHARGE_CTL;
			break;
		default:
			ret = -ENOTSUP;
			break;
		}
		break;

	case 4U:
		/* GPIO4 */
		switch (reg_fnc) {
		case (AXP192_GPIO4_FUNC_VAL_INPUT | AXP192_GPIO34_FUNC_ENA):
			*func = AXP192_GPIO_FUNC_INPUT;
			break;
		case (AXP192_GPIO4_FUNC_VAL_OUTPUT_OD | AXP192_GPIO34_FUNC_ENA):
			*func = AXP192_GPIO_FUNC_OUTPUT_OD;
			break;
		case (AXP192_GPIO4_FUNC_VAL_ADC | AXP192_GPIO34_FUNC_ENA):
			*func = AXP192_GPIO_FUNC_ADC;
			break;
		case AXP192_GPIO4_FUNC_VAL_CHARGE_CTL:
			*func = AXP192_GPIO_FUNC_CHARGE_CTL;
			break;
		default:
			ret = -ENOTSUP;
			break;
		}
		break;

	default:
		ret = -EINVAL;
	}

	return ret;
}

int mfd_axp192_gpio_func_ctrl(const struct device *dev, const struct device *client_dev,
			      uint8_t gpio, enum axp192_gpio_func func)
{
	const struct mfd_axp192_config *config = dev->config;
	struct mfd_axp192_data *data = dev->data;
	bool is_output = false;
	int ret = 0;
	uint8_t reg_cfg;

	if (!AXP192_GPIO_FUNC_VALID(func)) {
		LOG_ERR("Invalid function");
		return -EINVAL;
	}

	if (gpio >= AXP192_GPIO_MAX_NUM) {
		LOG_ERR("Invalid gpio (%d)", gpio);
		return -EINVAL;
	}

	if ((data->gpio_mask_used[gpio] != 0) && (data->gpio_mask_used[gpio] != client_dev)) {
		LOG_INF("Warning: Pin already configured. Please check dt configuration");
	}

	switch (gpio) {
	case 0U:
		__fallthrough;
	case 1U:
		__fallthrough;
	case 2U:
		/* GPIO 0-2*/
		switch (func) {
		case AXP192_GPIO_FUNC_INPUT:
			reg_cfg = AXP192_GPIO012_FUNC_VAL_INPUT;
			break;
		case AXP192_GPIO_FUNC_OUTPUT_OD:
			reg_cfg = AXP192_GPIO012_FUNC_VAL_OUTPUT_OD;
			is_output = true;
			break;
		case AXP192_GPIO_FUNC_OUTPUT_LOW:
			reg_cfg = AXP192_GPIO012_FUNC_VAL_OUTPUT_LOW;
			is_output = true;
			break;
		case AXP192_GPIO_FUNC_LDO:
			if (gpio == 0) {
				/* LDO is only applicable on GPIO0 */
				reg_cfg = AXP192_GPIO012_FUNC_VAL_LDO;
			} else {
				ret = -ENOTSUP;
			}
			break;
		case AXP192_GPIO_FUNC_ADC:
			reg_cfg = AXP192_GPIO012_FUNC_VAL_ADC;
			break;
		case AXP192_GPIO_FUNC_FLOAT:
			reg_cfg = AXP192_GPIO012_FUNC_VAL_FLOAT;
			break;
		default:
			ret = -ENOTSUP;
			break;
		}
		break;

	case 3U:
		/* GPIO3 */
		switch (func) {
		case AXP192_GPIO_FUNC_INPUT:
			reg_cfg = AXP192_GPIO3_FUNC_VAL_INPUT | AXP192_GPIO34_FUNC_ENA;
			break;
		case AXP192_GPIO_FUNC_OUTPUT_OD:
			reg_cfg = AXP192_GPIO3_FUNC_VAL_OUTPUT_OD | AXP192_GPIO34_FUNC_ENA;
			is_output = true;
			break;
		case AXP192_GPIO_FUNC_CHARGE_CTL:
			reg_cfg = AXP192_GPIO3_FUNC_VAL_CHARGE_CTL;
			break;
		default:
			ret = -ENOTSUP;
			break;
		}
		break;

	case 4U:
		/* GPIO4 */
		switch (func) {
		case AXP192_GPIO_FUNC_INPUT:
			reg_cfg = AXP192_GPIO4_FUNC_VAL_INPUT | AXP192_GPIO34_FUNC_ENA;
			break;
		case AXP192_GPIO_FUNC_OUTPUT_OD:
			reg_cfg = AXP192_GPIO4_FUNC_VAL_OUTPUT_OD | AXP192_GPIO34_FUNC_ENA;
			is_output = true;
			break;
		case AXP192_GPIO_FUNC_ADC:
			reg_cfg = AXP192_GPIO4_FUNC_VAL_ADC | AXP192_GPIO34_FUNC_ENA;
			break;
		case AXP192_GPIO_FUNC_CHARGE_CTL:
			reg_cfg = AXP192_GPIO4_FUNC_VAL_CHARGE_CTL;
			break;
		default:
			ret = -ENOTSUP;
			break;
		}
		break;

	default:
		ret = -EINVAL;
	}
	if (ret != 0) {
		LOG_ERR("Invalid function (0x%x) for gpio %d", func, gpio);
		return ret;
	}

	ret = i2c_reg_update_byte_dt(&(config->i2c), gpio_reg_desc[gpio].reg,
				     gpio_reg_desc[gpio].mask, reg_cfg);
	if (ret != 0) {
		return ret;
	}

	/* Save gpio configuration state */
	data->gpio_mask_used[gpio] = client_dev;
	if (is_output) {
		data->gpio_mask_output |= (1u << gpio);
	} else {
		data->gpio_mask_output &= ~(1u << gpio);
	}
	LOG_DBG("GPIO %d configured successfully (func=0x%x)", gpio, reg_cfg);

	return 0;
}

int mfd_axp192_gpio_pd_get(const struct device *dev, uint8_t gpio, bool *enabled)
{
	const struct mfd_axp192_config *config = dev->config;
	uint8_t gpio_val;
	uint8_t pd_reg_mask = 0;
	int ret = 0;

	switch (gpio) {
	case 0U:
		pd_reg_mask = AXP192_GPIO0_PULLDOWN_ENABLE;
		break;
	case 1U:
		pd_reg_mask = AXP192_GPIO1_PULLDOWN_ENABLE;
		break;
	case 2U:
		pd_reg_mask = AXP192_GPIO2_PULLDOWN_ENABLE;
		break;

	case 3U:
		__fallthrough;
	case 4U:
		__fallthrough;
	case 5U:
		LOG_DBG("Pull-Down not support on gpio %d", gpio);
		return -ENOTSUP;

	default:
		LOG_ERR("Invalid gpio (%d)", gpio);
		return -EINVAL;
	}

	ret = i2c_reg_read_byte_dt(&(config->i2c), AXP192_GPIO012_REG_PULLDOWN, &gpio_val);

	if (ret == 0) {
		*enabled = ((gpio_val & pd_reg_mask) != 0);
		LOG_DBG("Pull-Down stats of gpio %d: %d", gpio, *enabled);
	}

	return 0;
}

int mfd_axp192_gpio_pd_ctrl(const struct device *dev, uint8_t gpio, bool enable)
{
	uint8_t reg_pd_val = 0;
	uint8_t reg_pd_mask = 0;
	const struct mfd_axp192_config *config = dev->config;
	int ret = 0;

	/* Configure pull-down. Pull-down is only supported by GPIO3 and GPIO4 */
	switch (gpio) {
	case 0U:
		reg_pd_mask = AXP192_GPIO0_PULLDOWN_ENABLE;
		if (enable) {
			reg_pd_val = AXP192_GPIO0_PULLDOWN_ENABLE;
		}
		break;

	case 1U:
		reg_pd_mask = AXP192_GPIO1_PULLDOWN_ENABLE;
		if (enable) {
			reg_pd_val = AXP192_GPIO1_PULLDOWN_ENABLE;
		}
		break;

	case 2U:
		reg_pd_mask = AXP192_GPIO2_PULLDOWN_ENABLE;
		if (enable) {
			reg_pd_val = AXP192_GPIO2_PULLDOWN_ENABLE;
		}
		break;

	case 3U:
		__fallthrough;
	case 4U:
		__fallthrough;
	case 5U:
		LOG_ERR("Pull-Down not support on gpio %d", gpio);
		return -ENOTSUP;

	default:
		LOG_ERR("Invalid gpio (%d)", gpio);
		return -EINVAL;
	}

	ret = i2c_reg_update_byte_dt(&(config->i2c), AXP192_GPIO012_REG_PULLDOWN, reg_pd_mask,
				     reg_pd_val);

	return ret;
}

int mfd_axp192_gpio_read_port(const struct device *dev, uint8_t *value)
{
	const struct mfd_axp192_config *config = dev->config;
	const struct mfd_axp192_data *data = dev->data;
	int ret;
	uint8_t gpio012_val;
	uint8_t gpio34_val;
	uint8_t gpio_input_val;
	uint8_t gpio_output_val;

	/* read gpio0-2 */
	ret = i2c_reg_read_byte_dt(&(config->i2c), AXP192_GPIO012_REG_PINVAL, &gpio012_val);
	if (ret != 0) {
		return ret;
	}

	/* read gpio3-4 */
	ret = i2c_reg_read_byte_dt(&(config->i2c), AXP192_GPIO34_REG_PINVAL, &gpio34_val);
	if (ret != 0) {
		return ret;
	}
	LOG_DBG("GPIO012 pinval-reg=0x%x", gpio012_val);
	LOG_DBG("GPIO34 pinval-reg =0x%x", gpio34_val);
	LOG_DBG("Output-Mask       =0x%x", data->gpio_mask_output);

	gpio_input_val =
		((gpio012_val & AXP192_GPIO012_INTPUT_MASK) >> AXP192_GPIO012_INTPUT_SHIFT);
	gpio_input_val |=
		(((gpio34_val & AXP192_GPIO34_INTPUT_MASK) >> AXP192_GPIO34_INTPUT_SHIFT) << 3u);

	gpio_output_val = (gpio012_val & AXP192_GPIO012_OUTPUT_MASK);
	gpio_output_val |= ((gpio34_val & AXP192_GPIO34_OUTPUT_MASK) << 3u);

	*value = gpio_input_val & ~(data->gpio_mask_output);
	*value |= (gpio_output_val & data->gpio_mask_output);

	return 0;
}

int mfd_axp192_gpio_write_port(const struct device *dev, uint8_t value, uint8_t mask)
{
	const struct mfd_axp192_config *config = dev->config;
	int ret;
	uint8_t gpio_reg_val;
	uint8_t gpio_reg_mask;

	/* Write gpio0-2. Mask out other port pins */
	gpio_reg_val = (value & AXP192_GPIO012_OUTPUT_MASK);
	gpio_reg_mask = mask & AXP192_GPIO012_OUTPUT_MASK;
	if (gpio_reg_mask != 0) {
		ret = i2c_reg_update_byte_dt(&(config->i2c), AXP192_GPIO012_REG_PINVAL,
					     gpio_reg_mask, gpio_reg_val);
		if (ret != 0) {
			return ret;
		}
		LOG_DBG("GPIO012 pinval-reg=0x%x mask=0x%x", gpio_reg_val, gpio_reg_mask);
	}

	/* Write gpio3-4. Mask out other port pins */
	gpio_reg_val = value >> 3U;
	gpio_reg_mask = (mask >> 3U) & AXP192_GPIO34_OUTPUT_MASK;
	if (gpio_reg_mask != 0) {
		ret = i2c_reg_update_byte_dt(&(config->i2c), AXP192_GPIO34_REG_PINVAL,
					     gpio_reg_mask, gpio_reg_val);
		if (ret != 0) {
			return ret;
		}
		LOG_DBG("GPIO34 pinval-reg =0x%x mask=0x%x", gpio_reg_val, gpio_reg_mask);
	}

	return 0;
}

#define MFD_AXP192_DEFINE(inst)                                                                    \
	static const struct mfd_axp192_config config##inst = {                                     \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
	};                                                                                         \
                                                                                                   \
	static struct mfd_axp192_data data##inst;                                                  \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, mfd_axp192_init, NULL, &data##inst, &config##inst,             \
			      POST_KERNEL, CONFIG_MFD_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MFD_AXP192_DEFINE)
