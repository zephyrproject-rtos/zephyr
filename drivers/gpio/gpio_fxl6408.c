/*
 * Copyright (c) 2021 Abel Sensors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(fxl6408, CONFIG_FXL6408_LOG_LEVEL);
#define DT_DRV_COMPAT             fcs_fxl6408

/* Register definitions */
#define REG_DEVICE_ID_CTRL        0x01
#define REG_DIRECTION             0x03
#define REG_OUTPUT                0x05
#define REG_OUTPUT_HIGH_Z         0x07
#define REG_INPUT_DEFAULT_STATE   0x09
#define REG_PUD_EN                0x0B
#define REG_PUD_SEL               0x0D
#define REG_INPUT_VALUE           0x0F
#define REG_INT_MASK              0x11
#define REG_INT_STATUS            0x13

#define SUPPORTED_FLAGS (GPIO_INPUT | GPIO_OUTPUT | GPIO_OUTPUT_INIT_LOW |\
			GPIO_OUTPUT_INIT_HIGH | GPIO_PULL_DOWN | GPIO_PULL_UP |\
			GPIO_ACTIVE_HIGH | GPIO_ACTIVE_LOW)

/** Configuration data*/
struct gpio_fxl6408_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;

	/** Controller I2C DT specification */
	struct i2c_dt_spec i2c;
};

/** Runtime driver data */
struct gpio_fxl6408_drv_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;

	struct {
		uint8_t input;
		uint8_t output;
		uint8_t dir;
		uint8_t high_z;
		uint8_t pud_en;
		uint8_t pud_sel;
	} reg_cache;

	struct k_sem lock;
};

/**
 * @brief Read the port of certain register function.
 *
 * @param dev Device struct of the FXL6408.
 * @param reg Register to read.
 * @param cache Pointer to the cache to be updated after successful read.
 *
 * @return 0 if successful, failed otherwise.
 */
static int read_port_regs(const struct device *dev, uint8_t reg, uint8_t *cache)
{
	const struct gpio_fxl6408_config *const config = dev->config;
	uint8_t port_data;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, reg, &port_data);
	if (ret != 0) {
		LOG_ERR("Error reading register 0x%X (%d)", reg, ret);
		return ret;
	}
	*cache = port_data;
	LOG_DBG("Read: REG[0x%X] = 0x%X", reg, *cache);

	return ret;
}

/**
 *  @brief Write to the port registers of certain register function.
 *
 * @param dev Device struct of the FXL6408.
 * @param reg Register to write into. Possible values:  REG_DEVICE_ID_CTRL,
 * REG_OUTPUT, REG_DIRECTION, REG_PUD_SEL, REG_PUD_EN, REG_OUTPUT_HIGH_Z and
 * REG_INPUT_DEFAULT.
 * @param cache Pointer to the cache to be updated after successful write.
 * @param value New value to set.
 *
 * @return 0 if successful, failed otherwise.
 */
static int write_port_regs(const struct device *dev, uint8_t reg,
			   uint8_t *cache, uint8_t value)
{
	const struct gpio_fxl6408_config *const config = dev->config;
	int ret = 0;

	if (*cache != value) {
		ret = i2c_reg_write_byte_dt(&config->i2c, reg, value);
		if (ret != 0) {
			LOG_ERR("error writing to register 0x%X (%d)",
				reg, ret);
			return ret;
		}
		*cache = value;
		LOG_DBG("Write: REG[0x%X] = 0x%X", reg, *cache);
	}

	return ret;
}

static inline int update_input_regs(const struct device *dev, uint8_t *buf)
{
	struct gpio_fxl6408_drv_data *const drv_data =
		(struct gpio_fxl6408_drv_data *const)dev->data;
	int ret = read_port_regs(dev, REG_INPUT_VALUE,
				 &drv_data->reg_cache.input);
	*buf = drv_data->reg_cache.input;

	return ret;
}

static inline int update_output_regs(const struct device *dev, uint8_t value)
{
	struct gpio_fxl6408_drv_data *const drv_data =
		(struct gpio_fxl6408_drv_data *const)dev->data;

	return write_port_regs(dev, REG_OUTPUT,
			&drv_data->reg_cache.output, value);
}

static inline int update_high_z_regs(const struct device *dev, uint8_t value)
{
	struct gpio_fxl6408_drv_data *const drv_data =
		(struct gpio_fxl6408_drv_data *const)dev->data;

	return write_port_regs(dev, REG_OUTPUT_HIGH_Z,
			&drv_data->reg_cache.high_z, value);
}

static inline int update_direction_regs(const struct device *dev, uint8_t value)
{
	struct gpio_fxl6408_drv_data *const drv_data =
		(struct gpio_fxl6408_drv_data *const)dev->data;

	return write_port_regs(dev, REG_DIRECTION,
			&drv_data->reg_cache.dir, value);
}

static inline int update_pul_sel_regs(const struct device *dev, uint8_t value)
{
	struct gpio_fxl6408_drv_data *const drv_data =
		(struct gpio_fxl6408_drv_data *const)dev->data;

	return write_port_regs(dev, REG_PUD_SEL,
			&drv_data->reg_cache.pud_sel, value);
}

static inline int update_pul_en_regs(const struct device *dev, uint8_t value)
{
	struct gpio_fxl6408_drv_data *const drv_data =
		(struct gpio_fxl6408_drv_data *const)dev->data;

	return write_port_regs(dev, REG_PUD_EN,
			&drv_data->reg_cache.pud_en, value);
}

static int setup_pin_dir(const struct device *dev, uint32_t pin, int flags)
{
	struct gpio_fxl6408_drv_data *const drv_data =
		(struct gpio_fxl6408_drv_data *const)dev->data;
	uint8_t reg_dir = drv_data->reg_cache.dir;
	uint8_t reg_out = drv_data->reg_cache.output;
	uint8_t reg_high_z = drv_data->reg_cache.high_z;
	int ret;

	if (((flags & GPIO_INPUT) != 0) && ((flags & GPIO_OUTPUT) != 0)) {
		return -ENOTSUP;
	}

	/* Update the driver data to the actual situation of the FXL6408 */
	if (flags & GPIO_OUTPUT) {
		if ((flags & GPIO_OUTPUT_INIT_HIGH)) {
			reg_out |= BIT(pin);
		} else if ((flags & GPIO_OUTPUT_INIT_LOW)) {
			reg_out &= ~BIT(pin);
		}
		reg_dir |= BIT(pin);
		reg_high_z &= ~BIT(pin);
	} else if (flags & GPIO_INPUT) {
		reg_dir &= ~BIT(pin);
		reg_high_z &= ~BIT(pin);
	} else {
		reg_high_z |= BIT(pin);
		reg_dir |= BIT(pin);
	}

	ret = update_output_regs(dev, reg_out);
	if (ret != 0) {
		return ret;
	}

	ret = update_high_z_regs(dev, reg_high_z);
	if (ret != 0) {
		return ret;
	}

	ret = update_direction_regs(dev, reg_dir);
	return ret;
}

/**
 * @brief Setup the pin pull up/pull down status
 *
 * @param dev Device struct of the FXL6408
 * @param pin The pin number
 * @param flags Flags of pin or port
 *
 * @return 0 if successful, failed otherwise
 */
static int setup_pin_pullupdown(const struct device *dev, uint32_t pin,
				int flags)
{
	struct gpio_fxl6408_drv_data *const drv_data =
		(struct gpio_fxl6408_drv_data *const)dev->data;
	uint8_t reg_pud;
	int ret;

	/* If disabling pull up/down, there is no need to set the selection
	 * register. Just go straight to disabling.
	 */
	if ((flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) != 0U) {
		/* Setup pin pull up or pull down */
		reg_pud = drv_data->reg_cache.pud_sel;

		/* Pull down == 0, pull up == 1 */
		WRITE_BIT(reg_pud, pin, (flags & GPIO_PULL_UP) != 0U);

		ret = update_pul_sel_regs(dev, reg_pud);
		if (ret != 0) {
			return ret;
		}
	}

	/* Enable/disable pull up/down */
	reg_pud = drv_data->reg_cache.pud_en;

	WRITE_BIT(reg_pud, pin,
		  (flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) != 0U);

	ret = update_pul_en_regs(dev, reg_pud);
	return ret;
}

static int gpio_fxl6408_pin_config(const struct device *dev, gpio_pin_t pin,
				   gpio_flags_t flags)
{
	struct gpio_fxl6408_drv_data *const drv_data =
		(struct gpio_fxl6408_drv_data *const)dev->data;
	int ret;

	/* Check if supported flag is set */
	if ((flags & ~SUPPORTED_FLAGS) != 0) {
		return -ENOTSUP;
	}

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}
	k_sem_take(&drv_data->lock, K_FOREVER);

	ret = setup_pin_dir(dev, pin, flags);
	if (ret != 0) {
		LOG_ERR("error setting pin direction (%d)", ret);
		goto done;
	}

	ret = setup_pin_pullupdown(dev, pin, flags);
	if (ret) {
		LOG_ERR("error setting pin pull up/down (%d)", ret);
		goto done;
	}

done:
	k_sem_give(&drv_data->lock);
	return ret;
}

static int gpio_fxl6408_port_get_raw(const struct device *dev, uint32_t *value)
{
	struct gpio_fxl6408_drv_data *const drv_data =
		(struct gpio_fxl6408_drv_data *const)dev->data;
	uint8_t buf = 0;
	int ret = 0;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	ret = update_input_regs(dev, &buf);
	if (ret != 0) {
		goto done;
	}
	*value = buf;

done:
	k_sem_give(&drv_data->lock);
	return ret;
}

static int gpio_fxl6408_port_set_masked_raw(const struct device *dev,
						uint32_t mask, uint32_t value)
{
	struct gpio_fxl6408_drv_data *const drv_data =
		(struct gpio_fxl6408_drv_data *const)dev->data;
	uint8_t reg_out;
	int ret;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	reg_out = drv_data->reg_cache.output;
	reg_out = (reg_out & ~mask) | (mask & value);

	ret = update_output_regs(dev, reg_out);

	k_sem_give(&drv_data->lock);

	return ret;
}

static int gpio_fxl6408_port_set_bits_raw(const struct device *dev,
					  uint32_t mask)
{
	return gpio_fxl6408_port_set_masked_raw(dev, mask, mask);
}

static int gpio_fxl6408_port_clear_bits_raw(const struct device *dev,
						uint32_t mask)
{
	return gpio_fxl6408_port_set_masked_raw(dev, mask, 0);
}

static int gpio_fxl6408_port_toggle_bits(const struct device *dev,
					 uint32_t mask)
{
	struct gpio_fxl6408_drv_data *const drv_data =
		(struct gpio_fxl6408_drv_data *const)dev->data;
	uint8_t reg_out;
	int ret;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	reg_out = drv_data->reg_cache.output;
	reg_out ^= mask;
	ret = update_output_regs(dev, reg_out);

	k_sem_give(&drv_data->lock);

	return ret;
}

int gpio_fxl6408_init(const struct device *dev)
{
	struct gpio_fxl6408_drv_data *const drv_data =
		(struct gpio_fxl6408_drv_data *const)dev->data;
	const struct gpio_fxl6408_config *const config = dev->config;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("%s is not ready", config->i2c.bus->name);
		return -ENODEV;
	}

	k_sem_init(&drv_data->lock, 1, 1);

	return 0;
}

static DEVICE_API(gpio, gpio_fxl_driver) = {
	.pin_configure = gpio_fxl6408_pin_config,
	.port_get_raw = gpio_fxl6408_port_get_raw,
	.port_set_masked_raw = gpio_fxl6408_port_set_masked_raw,
	.port_set_bits_raw = gpio_fxl6408_port_set_bits_raw,
	.port_clear_bits_raw = gpio_fxl6408_port_clear_bits_raw,
	.port_toggle_bits = gpio_fxl6408_port_toggle_bits,
};

#define GPIO_FXL6408_DEVICE_INSTANCE(inst)                                     \
	static const struct gpio_fxl6408_config gpio_fxl6408_##inst##_cfg = {  \
		.common = {                                                    \
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(inst),\
		},                                                             \
		.i2c = I2C_DT_SPEC_INST_GET(inst)                              \
	};                                                                     \
\
	static struct gpio_fxl6408_drv_data gpio_fxl6408_##inst##_drvdata = {  \
		.reg_cache = {                                                 \
			.input = 0x0,                                          \
			.output = 0x00,                                        \
			.dir = 0x0,                                            \
			.high_z = 0xFF,                                        \
			.pud_en = 0xFF,                                        \
			.pud_sel = 0x0                                         \
		}                                                              \
	};                                                                     \
\
	DEVICE_DT_INST_DEFINE(inst, gpio_fxl6408_init, NULL,                   \
		&gpio_fxl6408_##inst##_drvdata,                                \
		&gpio_fxl6408_##inst##_cfg, POST_KERNEL,                       \
		CONFIG_GPIO_FXL6408_INIT_PRIORITY,                             \
		&gpio_fxl_driver);

DT_INST_FOREACH_STATUS_OKAY(GPIO_FXL6408_DEVICE_INSTANCE)
