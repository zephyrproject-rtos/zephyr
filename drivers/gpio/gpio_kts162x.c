/*
 * Copyright (c) 2025 Atmosic
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(kts162x, CONFIG_GPIO_LOG_LEVEL);

#ifdef CONFIG_GPIO_KTS1620

#define REGS_IN_GROUP 3
#define BITS_IN_PORTS 24

#define KTS_REG_BASE_IN_VAL           0x00
#define KTS_REG_BASE_OUT_VAL          0x04
#define KTS_REG_BASE_INVERT           0x08
#define KTS_REG_BASE_IN_EN            0x0C
#define KTS_REG_BASE_OUT_STRENGTH     0x40
#define KTS_REG_BASE_IN_LATCH_EN      0x48
#define KTS_REG_BASE_PULL_EN          0x4C
#define KTS_REG_BASE_PULL_UP_DOWN_SEL 0x50
#define KTS_REG_BASE_INT_MASK         0x54
#define KTS_REG_BASE_INT_STATUS       0x58
#define KTS_REG_BASE_GROUP_ODENX      0x5C
#define KTS_REG_BASE_INT_EDGE         0x60
#define KTS_REG_BASE_INT_CLEAR        0x68
#define KTS_REG_BASE_IN_STATUS        0x6C
#define KTS_REG_BASE_IOCRX            0x70
#define KTS_REG_BASE_SWITCH_DEB_EN    0x74
#define KTS_REG_BASE_SWITCH_DEB_COUNT 0x76

#else // KTS1622

#define REGS_IN_GROUP 2
#define BITS_IN_PORTS 16

#define KTS_REG_BASE_IN_VAL           0x00
#define KTS_REG_BASE_OUT_VAL          0x02
#define KTS_REG_BASE_INVERT           0x04
#define KTS_REG_BASE_IN_EN            0x06
#define KTS_REG_BASE_OUT_STRENGTH     0x40
#define KTS_REG_BASE_IN_LATCH_EN      0x44
#define KTS_REG_BASE_PULL_EN          0x46
#define KTS_REG_BASE_PULL_UP_DOWN_SEL 0x48
#define KTS_REG_BASE_INT_MASK         0x4A
#define KTS_REG_BASE_INT_STATUS       0x4C
#define KTS_REG_BASE_GROUP_ODENX      0x4F
#define KTS_REG_BASE_INT_EDGE         0x50
#define KTS_REG_BASE_INT_CLEAR        0x54
#define KTS_REG_BASE_IN_STATUS        0x56
#define KTS_REG_BASE_IOCRX            0x58
#define KTS_REG_BASE_SWITCH_DEB_EN    0x5A
#define KTS_REG_BASE_SWITCH_DEB_COUNT 0x5C
#endif
#define DT_DRV_COMPAT kinetic_kts162x

/* runtime driver data of the kts162x */
struct kts162x_drv_data {
	// gpio_driver_data needs to be first
	struct gpio_driver_data common;
	struct k_sem lock;
	struct k_work work;
	struct gpio_callback gpio_cb;
	sys_slist_t callbacks;
	const struct device *dev;
	uint32_t in_en;
	uint32_t pull_en;
	uint32_t out_val;
	uint32_t int_mask;
};

/* Configuration data */
struct kts162x_drv_cfg {
	// gpio_driver_config needs to be first
	struct gpio_driver_config common;
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec gpio_int;
};

static int kts162x_set_reg(const struct device *dev, uint8_t *value, uint8_t reg)
{
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}
	const struct kts162x_drv_cfg *drv_cfg = dev->config;
	int ret = i2c_burst_write_dt(&drv_cfg->i2c, reg, value, 1);

	if (ret) {
		LOG_ERR("%s: failed to set reg(%#x): %d", dev->name, reg, ret);
		return -EIO;
	}

	return 0;
}

static int kts162x_set_regs(const struct device *dev, uint32_t value, uint8_t reg)
{
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}
	const struct kts162x_drv_cfg *drv_cfg = dev->config;
	int ret = i2c_burst_write_dt(&drv_cfg->i2c, reg, (uint8_t *)&value, REGS_IN_GROUP);

	if (ret) {
		LOG_ERR("%s: failed to set regs(%#x): %d", dev->name, reg, ret);
		return -EIO;
	}

	return 0;
}

static int kts162x_get_regs(const struct device *dev, uint32_t *value, uint8_t reg)
{
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}
	const struct kts162x_drv_cfg *drv_cfg = dev->config;
	uint8_t rx_buf[4];

	int ret = i2c_burst_read_dt(&drv_cfg->i2c, reg, rx_buf, REGS_IN_GROUP);
	if (ret) {
		return -EIO;
	}
	if (value) {
		*value = BIT_MASK(BITS_IN_PORTS) & sys_get_le32(rx_buf);
	}

	return 0;
}

static void kts162x_work_handler(struct k_work *work)
{
	struct kts162x_drv_data *drv_data = CONTAINER_OF(work, struct kts162x_drv_data, work);

	k_sem_take(&drv_data->lock, K_FOREVER);

	uint32_t int_sts;
	int ret = kts162x_get_regs(drv_data->dev, &int_sts, KTS_REG_BASE_INT_STATUS);
	k_sem_give(&drv_data->lock);

	if (ret) {
		LOG_ERR("Failed to read interrupt sources: %d", ret);
		return;
	}

	ret = kts162x_set_regs(drv_data->dev, int_sts, KTS_REG_BASE_INT_CLEAR);
	if (ret) {
		LOG_ERR("Failed to clear interrupt sources: %d", ret);
		return;
	}

	if (int_sts) {
		gpio_fire_callbacks(&drv_data->callbacks, drv_data->dev, int_sts);
	}
}

static void kts162x_int_gpio_handler(const struct device *dev, struct gpio_callback *gpio_cb,
				     uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pins);

	struct kts162x_drv_data *drv_data = CONTAINER_OF(gpio_cb, struct kts162x_drv_data, gpio_cb);
	k_work_submit(&drv_data->work);
}

static void kts162x_update_data(struct k_sem *lock, uint32_t *dst, uint32_t src)
{
	k_sem_take(lock, K_FOREVER);
	*dst = src;
	k_sem_give(lock);
}

static int kts162x_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	struct kts162x_drv_data *drv_data = dev->data;
	int ret;
	uint32_t tmp_in_en = drv_data->in_en;
	uint32_t tmp_pull_en = drv_data->pull_en;
	uint32_t tmp_out_val = drv_data->out_val;
	uint8_t out_val_reg_base = 0;

	if (flags & (GPIO_PULL_UP | GPIO_PULL_DOWN | GPIO_DISCONNECTED | GPIO_SINGLE_ENDED)) {
		return -ENOTSUP;
	}

	if (flags & GPIO_OUTPUT) {
		tmp_in_en &= ~BIT(pin);
		// check if initial condition is specified
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			tmp_out_val |= BIT(pin);
			out_val_reg_base = KTS_REG_BASE_OUT_VAL;
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			tmp_out_val &= ~BIT(pin);
			out_val_reg_base = KTS_REG_BASE_OUT_VAL;
		}
	}
	// the driver supports simultaneous in/out mode
	// flags may contain both GPIO_INPUT and GPIO_OUTPUT so here do not use "else if"
	if (flags & GPIO_INPUT) {
		tmp_in_en |= BIT(pin);
		out_val_reg_base = KTS_REG_BASE_PULL_UP_DOWN_SEL;

		// if pin is configured as simultaneous input/outout, set input with pull high/low
		if (flags & GPIO_OUTPUT) {
			tmp_pull_en |= BIT(pin);
		} else {
			tmp_pull_en &= ~BIT(pin);
		}
		if (tmp_pull_en != drv_data->pull_en) {
			ret = kts162x_set_regs(dev, tmp_pull_en, KTS_REG_BASE_PULL_EN);
			if (ret) {
				return ret;
			}
			kts162x_update_data(&drv_data->lock, &drv_data->pull_en, tmp_pull_en);
		}
	}

	// set initial value
	if (out_val_reg_base) {
		ret = kts162x_set_regs(dev, tmp_out_val, out_val_reg_base);
		if (ret) {
			return ret;
		}
		kts162x_update_data(&drv_data->lock, &drv_data->out_val, tmp_out_val);
	}

	// configure input or output
	if (tmp_in_en != drv_data->in_en) {
		ret = kts162x_set_regs(dev, tmp_in_en, KTS_REG_BASE_IN_EN);
		if (!ret) {
			kts162x_update_data(&drv_data->lock, &drv_data->in_en, tmp_in_en);
		}
	}

	return ret;
}

static int kts162x_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	struct kts162x_drv_data *drv_data = dev->data;
	k_sem_take(&drv_data->lock, K_FOREVER);

	// Reading of the input port also clears the interrupt status
	int ret = kts162x_get_regs(dev, value, KTS_REG_BASE_IN_VAL);

	k_sem_give(&drv_data->lock);

	return ret;
}

static int kts162x_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
				       gpio_port_value_t value)
{
	struct kts162x_drv_data *drv_data = dev->data;
	uint32_t out_val_reg_base = KTS_REG_BASE_OUT_VAL;
	uint32_t tmp_out_val = drv_data->out_val;

	tmp_out_val &= ~mask;
	tmp_out_val |= value;

	if (mask & drv_data->in_en) {
		out_val_reg_base = KTS_REG_BASE_PULL_UP_DOWN_SEL;
	}

	int ret = kts162x_set_regs(dev, tmp_out_val, out_val_reg_base);
	if (!ret) {
		kts162x_update_data(&drv_data->lock, &drv_data->out_val, tmp_out_val);
	}
	return ret;
}

static int kts162x_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	struct kts162x_drv_data *drv_data = dev->data;
	uint32_t out_val_reg_base = KTS_REG_BASE_OUT_VAL;
	uint32_t tmp_out_val = drv_data->out_val | pins;

	if (pins & drv_data->in_en) {
		out_val_reg_base = KTS_REG_BASE_PULL_UP_DOWN_SEL;
	}

	int ret = kts162x_set_regs(dev, tmp_out_val, out_val_reg_base);
	if (!ret) {
		kts162x_update_data(&drv_data->lock, &drv_data->out_val, tmp_out_val);
	}
	return ret;
}

static int kts162x_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	struct kts162x_drv_data *drv_data = dev->data;
	uint32_t out_val_reg_base = KTS_REG_BASE_OUT_VAL;
	uint32_t tmp_out_val = drv_data->out_val & ~pins;

	if (pins & drv_data->in_en) {
		out_val_reg_base = KTS_REG_BASE_PULL_UP_DOWN_SEL;
	}

	int ret = kts162x_set_regs(dev, tmp_out_val, out_val_reg_base);
	if (!ret) {
		kts162x_update_data(&drv_data->lock, &drv_data->out_val, tmp_out_val);
	}
	return ret;
}

static int kts162x_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	struct kts162x_drv_data *drv_data = dev->data;
	uint32_t out_val_reg_base = KTS_REG_BASE_OUT_VAL;
	uint32_t tmp_out_val = drv_data->out_val ^ pins;

	if (pins & drv_data->in_en) {
		out_val_reg_base = KTS_REG_BASE_PULL_UP_DOWN_SEL;
	}

	int ret = kts162x_set_regs(dev, tmp_out_val, out_val_reg_base);
	if (!ret) {
		kts162x_update_data(&drv_data->lock, &drv_data->out_val, tmp_out_val);
	}
	return ret;
}

#define KTS_INT_BY_LEVEL_CHANGE 0
#define KTS_INT_BY_POS_EDGE     1
#define KTS_INT_BY_NEG_EDGE     2
#define KTS_INT_BY_BOTH_EDGE    3

/* Each pin gives an interrupt at kts162x. */
static int kts162x_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					   enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct kts162x_drv_cfg *drv_cfg = dev->config;
	struct kts162x_drv_data *drv_data = dev->data;

	static uint8_t ie_reg[REGS_IN_GROUP * 2];

	if (pin >= BITS_IN_PORTS) {
		return -ENOTSUP;
	}

	if (!drv_cfg->gpio_int.port) {
		return -ENOTSUP;
	}

	uint32_t tmp_int_mask = drv_data->int_mask;

	if (mode == GPIO_INT_MODE_DISABLED) {
		tmp_int_mask |= BIT(pin);
	} else if (mode == GPIO_INT_MODE_LEVEL) {
		// This expander's level trigger mode requires "a level change on the pin"
		// instead of continuously causing interrupt events as long as the level is kept.
		// Basically it is identical to "both edge" mode.
		// Typical "level trigger mode" is not supported.
		return -ENOTSUP;
	} else if (mode == GPIO_INT_MODE_EDGE) {
		tmp_int_mask &= ~BIT(pin);
		uint8_t setting;

		switch (trig) {
		case GPIO_INT_TRIG_LOW:
			setting = KTS_INT_BY_NEG_EDGE;
			break;
		case GPIO_INT_TRIG_HIGH:
			setting = KTS_INT_BY_POS_EDGE;
			break;
		case GPIO_INT_TRIG_BOTH:
			setting = KTS_INT_BY_BOTH_EDGE;
			break;
		default:
			return -ENOTSUP;
		}

		uint8_t offset = pin / 4;
		uint8_t shift = 2 * (pin % 4);
		uint8_t mask = BIT_MASK(2) << shift;
		ie_reg[offset] &= ~mask;
		ie_reg[offset] |= (setting << shift);

		int ret = kts162x_set_reg(dev, ie_reg + offset, KTS_REG_BASE_INT_EDGE + offset);
		if (ret) {
			return ret;
		}
	} else {
		return -ENOTSUP;
	}

	// clear pending interrupt before unmask the source
	int ret = kts162x_set_regs(dev, ~tmp_int_mask, KTS_REG_BASE_INT_CLEAR);
	if (ret) {
		return ret;
	}

	ret = kts162x_set_regs(dev, tmp_int_mask, KTS_REG_BASE_INT_MASK);
	if (!ret) {
		kts162x_update_data(&drv_data->lock, &drv_data->int_mask, tmp_int_mask);
	}
	return ret;
}

// Register the callback in the callback list
static int kts162x_manage_callback(const struct device *dev, struct gpio_callback *callback,
				   bool set)
{
	struct kts162x_drv_data *drv_data = dev->data;
	return gpio_manage_callback(&drv_data->callbacks, callback, set);
}

static int kts162x_init(const struct device *dev)
{
	const struct kts162x_drv_cfg *drv_cfg = dev->config;
	struct kts162x_drv_data *drv_data = dev->data;

	if (!device_is_ready(drv_cfg->i2c.bus)) {
		return -ENODEV;
	}

	// configure the callback for the interrupt gpio
	if (!drv_cfg->gpio_int.port) {
		LOG_WRN("kts162x interrupt is NOT configured, basic in/output is still supported");
		return 0;
	}

	if (!gpio_is_ready_dt(&drv_cfg->gpio_int)) {
		LOG_ERR("gpio port is not ready");
		return -ENODEV;
	}

	int ret = gpio_pin_configure_dt(&drv_cfg->gpio_int, GPIO_INPUT | GPIO_PULL_UP);
	if (ret) {
		LOG_ERR("%s: failed to configure INT line: %d", dev->name, ret);
		return -EIO;
	}

	ret = gpio_pin_interrupt_configure_dt(&drv_cfg->gpio_int, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret) {
		LOG_ERR("%s: failed to configure INT interrupt: %d", dev->name, ret);
		return -EIO;
	}

	gpio_init_callback(&drv_data->gpio_cb, kts162x_int_gpio_handler,
			   BIT(drv_cfg->gpio_int.pin));
	ret = gpio_add_callback(drv_cfg->gpio_int.port, &drv_data->gpio_cb);
	if (ret) {
		LOG_ERR("%s: failed to add INT callback: %d", dev->name, ret);
		return -EIO;
	}

	LOG_DBG("kts162x gpio interrupt is ready");
	LOG_DBG("drv_cfg->gpio_int.port :%p ", drv_cfg->gpio_int.port);
	LOG_DBG("drv_cfg->gpio_int.pin :%d ", drv_cfg->gpio_int.pin);
	return 0;
}

// supported APIs in the zephyr/include/zephyr/drivers/gpio.h
static const struct gpio_driver_api kts162x_drv_api = {
	.pin_configure = kts162x_pin_configure,
	.port_get_raw = kts162x_port_get_raw,
	.port_set_masked_raw = kts162x_port_set_masked_raw,
	.port_set_bits_raw = kts162x_port_set_bits_raw,
	.port_clear_bits_raw = kts162x_port_clear_bits_raw,
	.port_toggle_bits = kts162x_port_toggle_bits,
	.pin_interrupt_configure = kts162x_pin_interrupt_configure,
	.manage_callback = kts162x_manage_callback,
};

#define GPIO_KTS162X_INST(idx)                                                                     \
	static const struct kts162x_drv_cfg kts162x_cfg##idx = {                                   \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(idx),             \
			},                                                                         \
		.gpio_int = GPIO_DT_SPEC_INST_GET_OR(idx, int_gpios, {0}),                         \
		.i2c = I2C_DT_SPEC_INST_GET(idx),                                                  \
	};                                                                                         \
	static struct kts162x_drv_data kts162x_data##idx = {                                       \
		.lock = Z_SEM_INITIALIZER(kts162x_data##idx.lock, 1, 1),                           \
		.work = Z_WORK_INITIALIZER(kts162x_work_handler),                                  \
		.dev = DEVICE_DT_INST_GET(idx),                                                    \
		.in_en = BIT_MASK(BITS_IN_PORTS),                                                  \
		.int_mask = BIT_MASK(BITS_IN_PORTS),                                               \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(idx, kts162x_init, NULL, &kts162x_data##idx, &kts162x_cfg##idx,      \
			      POST_KERNEL, CONFIG_GPIO_KTS162X_INIT_PRIORITY, &kts162x_drv_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_KTS162X_INST);
