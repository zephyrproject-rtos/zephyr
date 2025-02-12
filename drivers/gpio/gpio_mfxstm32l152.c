/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_mfxstm32l152

/**
 * @file Driver for ST MFXstm32l152 I2C-based GPIO driver.
 */
#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#define LOG_LEVEL CONFIG_GPIO_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mfxstm32l152);

/* Register definitions */
#define REG_ID                  0x00 /* const 0x7b */
#define REG_GPIO_IRQ_PEND       0x0c /* GPIO irq pending */
#define REG_GPIO_STATE          0x10 /* GPIO state */
#define REG_SYS_CTRL            0x40 /* System control */
#define REG_SYS_IRQ_MODE        0x41 /* System irq mode */
#define SYS_IRQ_MODE_OPEN_DRAIN (0 << 0)
#define SYS_IRQ_MODE_PUSH_PULL  (1 << 0)
#define SYS_IRQ_MODE_POL_LOW    (0 << 1)
#define SYS_IRQ_MODE_POL_HIGH   (1 << 1)
#define REG_SYS_IRQ_EN          0x42 /* System irq enable */
#define REG_GPIO_IRQ_EN         0x48 /* GPIO irq enable */
#define REG_GPIO_IRQ_EVT        0x4c /* GPIO irq event */
#define REG_GPIO_IRQ_TYPE       0x50 /* GPIO irq type */
#define REG_GPIO_IRQ_ACK        0x54 /* GPIO irq ack */
#define REG_GPIO_DIR            0x60 /* GPIO direction control */
#define REG_GPIO_PUPD           0x68 /* GPIO pull-up/pull-down control */
#define REG_GPIO_SET            0x6c /* GPIO set control */
#define REG_GPIO_CLR            0x70 /* GPIO clear control */

#define MFXSTM32L152_ID 0x7b

/** Configuration data */
struct mfxstm32l152_drv_cfg {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;

	/** Master I2C DT specification */
	struct i2c_dt_spec i2c_spec;
	struct gpio_dt_spec int_gpio;
};

/** Cache of the pins configuration */
struct mfxstm32l152_pins_state {
	uint32_t direction;
	uint32_t pupd;
	uint32_t irq_enabled;
};

/** Runtime driver data */
struct mfxstm32l152_drv_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;

	/** Driver lock */
	struct k_sem lock;

	sys_slist_t callbacks;
	struct k_work work;

	const struct device *dev;
	struct gpio_callback int_gpio_cb;
	struct mfxstm32l152_pins_state pins_state;
};

/**
 * @brief read a register value from the MFX
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param reg Address of the register
 * @param buf Pointer to the place to store the value
 *
 * @retval 0 if successful.
 * @retval Negative value for error code.
 */
static int read_reg(const struct device *dev, uint8_t reg, uint8_t *buf)
{
	const struct mfxstm32l152_drv_cfg *const config = dev->config;
	uint8_t value;
	int ret;

	ret = i2c_burst_read_dt(&config->i2c_spec, reg, (uint8_t *)&value, 1);
	if (ret != 0) {
		LOG_ERR("%s: error reading register 0x%X (%d)", dev->name, reg, ret);
		return ret;
	}

	*buf = value;
	LOG_DBG("%s: Read: REG[0x%X] = 0x%X", dev->name, reg, value);

	return 0;
}

/**
 * @brief write a register of the MFX
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param reg Address of the register to be written
 * @param value Value to be written into the register
 *
 * @retval 0 if successful.
 * @retval Negative value for error code.
 */
static int write_reg(const struct device *dev, uint8_t reg, uint8_t value)
{
	const struct mfxstm32l152_drv_cfg *const config = dev->config;
	uint8_t buf[2];
	int ret;

	LOG_DBG("%s: Write: REG[0x%X] = 0x%X", dev->name, reg, value);

	buf[0] = reg;
	buf[1] = value;
	ret = i2c_write_dt(&config->i2c_spec, buf, sizeof(buf));
	if (ret != 0) {
		LOG_ERR("%s: error writing to register 0x%X (%d)", dev->name, reg, ret);
	}

	return ret;
}

/**
 * @brief Gets the state of a specified block of 3 registers from the MFX
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param reg Address of the first of 3 registers to be read.
 * @param buf Pointer to the buffer to output the register.
 *
 * @retval 0 if successful.
 * @retval Negative value for error code.
 */
static int read_port_regs(const struct device *dev, uint8_t reg, uint32_t *buf)
{
	const struct mfxstm32l152_drv_cfg *const config = dev->config;
	uint32_t port_data, value;
	int ret;

	ret = i2c_burst_read_dt(&config->i2c_spec, reg, (uint8_t *)&port_data, 3);
	if (ret != 0) {
		LOG_ERR("%s: error reading register 0x%X (%d)", dev->name, reg, ret);
		return ret;
	}

	value = sys_le24_to_cpu(port_data);
	*buf = value;
	LOG_DBG("%s: Read: REG[0x%X] = 0x%X, REG[0x%X] = 0x%X, REG[0x%X] = 0x%X", dev->name, reg,
		(*buf & 0xFF), (reg + 1), ((*buf >> 8) & 0xFF), (reg + 2), ((*buf >> 16) & 0xFF));

	return 0;
}

/**
 * @brief writes to a specified block of 3 registers into the MFX
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param reg Address of the first of 3 registers to be written.
 * @param value The pin value to be written into the registers.
 *
 * @retval 0 if successful.
 * @retval Negative value for error code.
 */
static int write_port_regs(const struct device *dev, uint8_t reg, uint32_t value)
{
	const struct mfxstm32l152_drv_cfg *const config = dev->config;
	uint8_t buf[4];
	int ret;

	LOG_DBG("%s: Write: REG[0x%X] = 0x%X, REG[0x%X] = 0x%X, REG[0x%X] = 0x%X", dev->name, reg,
		(value & 0xFF), (reg + 1), ((value >> 8) & 0xFF), (reg + 2),
		((value >> 16) & 0xFF));

	buf[0] = reg;
	sys_put_le24(value, &buf[1]);
	ret = i2c_write_dt(&config->i2c_spec, buf, sizeof(buf));
	if (ret != 0) {
		LOG_ERR("%s: error writing to register 0x%X (%d)", dev->name, reg, ret);
	}

	return ret;
}

/**
 * @brief Handles interrupt triggered by the interrupt pin of MFXSTM32L152.
 *
 * If int_gpios is configured in device tree then this will be triggered each
 * time a gpio configured as an input changes state. The gpio input states are
 * read in this function which clears the interrupt.
 *
 * @param dev Pointer to the device structure for the driver instance.
 */
static void mfxstm32l152_handle_interrupt(const struct device *dev)
{
	struct mfxstm32l152_drv_data *drv_data = dev->data;
	uint32_t irq_status;
	int ret;

	k_sem_take(&drv_data->lock, K_FOREVER);

	/* Any interrupts enabled? */
	if (!drv_data->pins_state.irq_enabled) {
		k_sem_give(&drv_data->lock);
		return;
	}

	/* Check pending irq status */
	ret = read_port_regs(dev, REG_GPIO_IRQ_PEND, &irq_status);
	if (ret != 0) {
		k_sem_give(&drv_data->lock);
		return;
	}

	if (irq_status == 0) {
		k_sem_give(&drv_data->lock);
		return;
	}

	/* Ack everything */
	ret = write_port_regs(dev, REG_GPIO_IRQ_ACK, irq_status);
	if (ret != 0) {
		k_sem_give(&drv_data->lock);
		return;
	}

	k_sem_give(&drv_data->lock);

	gpio_fire_callbacks(&drv_data->callbacks, dev, irq_status);
}

/**
 * @brief Work handler for MFXSTM32L152 interrupt
 *
 * @param work Work struct that contains pointer to interrupt handler function
 */
static void mfxstm32l152_work_handler(struct k_work *work)
{
	struct mfxstm32l152_drv_data *drv_data =
		CONTAINER_OF(work, struct mfxstm32l152_drv_data, work);

	mfxstm32l152_handle_interrupt(drv_data->dev);
}

/**
 * @brief ISR for interrupt pin of MFXSTM32L152
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param gpio_cb Pointer to callback function struct
 * @param pins Bitmask of pins that triggered interrupt
 */
static void mfxstm32l152_int_gpio_handler(const struct device *dev, struct gpio_callback *gpio_cb,
					  uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pins);

	struct mfxstm32l152_drv_data *drv_data =
		CONTAINER_OF(gpio_cb, struct mfxstm32l152_drv_data, int_gpio_cb);

	k_work_submit(&drv_data->work);
}

static int set_pin_dir_mode(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	struct mfxstm32l152_drv_data *const drvdata =
		(struct mfxstm32l152_drv_data *const)dev->data;
	uint32_t *dir_cache = &drvdata->pins_state.direction;
	uint32_t *mode_cache = &drvdata->pins_state.pupd;
	bool need_update = false;
	uint32_t dir, mode;
	int ret = 0;

	/* In case of configure in output mode first set initial state */
	if ((flags & GPIO_OUTPUT) != 0U) {
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
			ret = write_port_regs(dev, REG_GPIO_SET, BIT(pin));
			if (ret != 0) {
				goto out;
			}
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0U) {
			ret = write_port_regs(dev, REG_GPIO_CLR, BIT(pin));
			if (ret != 0) {
				goto out;
			}
		}
	}

	/* Configure direction */
	if ((flags & GPIO_OUTPUT) && ((*mode_cache & BIT(pin)) == 0)) {
		dir = *dir_cache | BIT(pin);
		need_update = true;
	} else if ((flags & GPIO_INPUT) && ((*mode_cache & BIT(pin)) != 0)) {
		dir = *dir_cache & ~BIT(pin);
		need_update = true;
	}
	if (need_update) {
		ret = write_port_regs(dev, REG_GPIO_DIR, dir);
		if (ret != 0) {
			goto out;
		}
		*dir_cache = dir;
	}

	/* In case of input mode, configure PullUp/ PullDown */
	need_update = false;
	if ((flags & GPIO_INPUT) != 0U) {
		if ((flags & GPIO_PULL_UP) && ((*mode_cache & BIT(pin)) == 0)) {
			mode = *mode_cache | BIT(pin);
			need_update = true;
		} else if ((flags & GPIO_PULL_DOWN) && ((*mode_cache & BIT(pin)) != 0)) {
			mode = *mode_cache & ~BIT(pin);
			need_update = true;
		}
	}
	if (need_update) {
		ret = write_port_regs(dev, REG_GPIO_PUPD, mode);
		if (ret != 0) {
			goto out;
		}
		*mode_cache = mode;
	}

out:
	return ret;
}

static int mfxstm32l152_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	struct mfxstm32l152_drv_data *const drvdata =
		(struct mfxstm32l152_drv_data *const)dev->data;
	int ret;

	/* No support for disconnected pin, single ended and simultaneous input / output */
	if ((flags & (GPIO_INPUT | GPIO_OUTPUT)) == GPIO_DISCONNECTED ||
	    (flags & GPIO_SINGLE_ENDED) != 0 ||
	    (((flags & GPIO_INPUT) != 0) && ((flags & GPIO_OUTPUT) != 0))) {
		return -ENOTSUP;
	}

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drvdata->lock, K_FOREVER);

	ret = set_pin_dir_mode(dev, pin, flags);
	if (ret != 0) {
		LOG_ERR("%s: error setting pin direction and mode (%d)", dev->name, ret);
	}

	k_sem_give(&drvdata->lock);

	return ret;
}

static int mfxstm32l152_port_get_raw(const struct device *dev, uint32_t *value)
{
	struct mfxstm32l152_drv_data *const drvdata =
		(struct mfxstm32l152_drv_data *const)dev->data;
	uint32_t reg_value;
	int ret;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drvdata->lock, K_FOREVER);
	ret = read_port_regs(dev, REG_GPIO_STATE, &reg_value);
	k_sem_give(&drvdata->lock);

	if (ret == 0) {
		*value = reg_value;
	}

	return ret;
}

static int mfxstm32l152_port_set_bits_raw(const struct device *dev, uint32_t mask)
{
	struct mfxstm32l152_drv_data *const drvdata =
		(struct mfxstm32l152_drv_data *const)dev->data;
	int ret;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drvdata->lock, K_FOREVER);
	ret = write_port_regs(dev, REG_GPIO_SET, mask);
	k_sem_give(&drvdata->lock);

	return ret;
}

static int mfxstm32l152_port_clear_bits_raw(const struct device *dev, uint32_t mask)
{
	struct mfxstm32l152_drv_data *const drvdata =
		(struct mfxstm32l152_drv_data *const)dev->data;
	int ret;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drvdata->lock, K_FOREVER);
	ret = write_port_regs(dev, REG_GPIO_CLR, mask);
	k_sem_give(&drvdata->lock);

	return ret;
}

static int mfxstm32l152_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
						enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	struct mfxstm32l152_drv_data *drv_data = dev->data;
	uint32_t irq_event, irq_type;
	int ret;

	k_sem_take(&drv_data->lock, K_FOREVER);

	if (mode == GPIO_INT_MODE_DISABLED) {
		drv_data->pins_state.irq_enabled &= ~BIT(pin);
		ret = write_port_regs(dev, REG_GPIO_IRQ_EN, drv_data->pins_state.irq_enabled);
		if (ret != 0) {
			k_sem_give(&drv_data->lock);
			return ret;
		}

		if (drv_data->pins_state.irq_enabled == 0) {
			ret = write_reg(dev, REG_SYS_IRQ_EN, 0);
			k_sem_give(&drv_data->lock);
			return ret;
		}

		k_sem_give(&drv_data->lock);

		return ret;
	}

	/* Set mode (EDGE / LEVEL */
	ret = read_port_regs(dev, REG_GPIO_IRQ_EVT, &irq_event);
	if (ret != 0) {
		k_sem_give(&drv_data->lock);
		return ret;
	}

	if (mode == GPIO_INT_MODE_EDGE) {
		irq_event |= BIT(pin);
	} else {
		irq_event &= ~BIT(pin);
	}

	ret = write_port_regs(dev, REG_GPIO_IRQ_EVT, irq_event);
	if (ret != 0) {
		k_sem_give(&drv_data->lock);
		return ret;
	}

	/* Set High / Rising or Low / Falling */
	ret = read_port_regs(dev, REG_GPIO_IRQ_TYPE, &irq_type);
	if (ret != 0) {
		k_sem_give(&drv_data->lock);
		return ret;
	}

	/* We cannot handle BOTH edge so if BOTH is asked, we set it as HIGH */
	if (trig == GPIO_INT_TRIG_HIGH || trig == GPIO_INT_TRIG_BOTH) {
		irq_type |= BIT(pin);
	} else {
		irq_type &= ~BIT(pin);
	}

	ret = write_port_regs(dev, REG_GPIO_IRQ_TYPE, irq_type);
	if (ret != 0) {
		k_sem_give(&drv_data->lock);
		return ret;
	}

	/* Enable the interrupt */
	drv_data->pins_state.irq_enabled |= BIT(pin);
	ret = write_port_regs(dev, REG_GPIO_IRQ_EN, drv_data->pins_state.irq_enabled);
	if (ret != 0) {
		k_sem_give(&drv_data->lock);
		return ret;
	}

	ret = write_reg(dev, REG_SYS_IRQ_EN, 1);

	k_sem_give(&drv_data->lock);

	return ret;
}

static int mfxstm32l152_manage_callback(const struct device *dev, struct gpio_callback *callback,
					bool set)
{
	struct mfxstm32l152_drv_data *drv_data = dev->data;

	return gpio_manage_callback(&drv_data->callbacks, callback, set);
}

static int mfxstm32l152_init(const struct device *dev)
{
	struct mfxstm32l152_drv_data *const drvdata =
		(struct mfxstm32l152_drv_data *const)dev->data;
	const struct mfxstm32l152_drv_cfg *drv_cfg = dev->config;
	uint8_t chip_id, int_pin = 0;
	int ret;

	if (!device_is_ready(drv_cfg->i2c_spec.bus)) {
		LOG_ERR("I2C device not found");
		return -ENODEV;
	}

	k_sem_init(&drvdata->lock, 1, 1);

	ret = read_reg(dev, REG_ID, &chip_id);
	if (ret != 0) {
		LOG_ERR("%s: Unable to read Chip ID", dev->name);
		return ret;
	}

	if (chip_id != MFXSTM32L152_ID) {
		LOG_ERR("%s: Invalid Chip ID", dev->name);
		return -EINVAL;
	}

	ret = read_port_regs(dev, REG_GPIO_DIR, &drvdata->pins_state.direction);
	if (ret != 0) {
		LOG_ERR("%s: Unable to read initial directions", dev->name);
		return ret;
	}

	ret = read_port_regs(dev, REG_GPIO_PUPD, &drvdata->pins_state.pupd);
	if (ret != 0) {
		LOG_ERR("%s: Unable to read initial directions", dev->name);
		return ret;
	}

	ret = write_reg(dev, REG_SYS_CTRL, 0x01);
	if (ret != 0) {
		LOG_ERR("%s: Failed to enable GPIO", dev->name);
		return ret;
	}

	/* If the INT line is available, configure the callback for it. */
	if (drv_cfg->int_gpio.port) {
		if (!gpio_is_ready_dt(&drv_cfg->int_gpio)) {
			LOG_ERR("Cannot get pointer to gpio interrupt device %s init failed",
				dev->name);
			return -EINVAL;
		}

		drvdata->dev = dev;

		k_work_init(&drvdata->work, mfxstm32l152_work_handler);

		ret = gpio_pin_configure_dt(&drv_cfg->int_gpio, GPIO_INPUT);
		if (ret != 0) {
			LOG_ERR("%s init failed: %d", dev->name, ret);
			return ret;
		}

		ret = gpio_pin_interrupt_configure_dt(&drv_cfg->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
		if (ret != 0) {
			LOG_ERR("%s init failed: %d", dev->name, ret);
			return ret;
		}

		gpio_init_callback(&drvdata->int_gpio_cb, mfxstm32l152_int_gpio_handler,
				   BIT(drv_cfg->int_gpio.pin));

		ret = gpio_add_callback(drv_cfg->int_gpio.port, &drvdata->int_gpio_cb);
		if (ret != 0) {
			LOG_ERR("%s init failed: %d", dev->name, ret);
			return ret;
		}

		/* Configure the INT_OUT pin based on int_gpio dt_flags */
		if ((drv_cfg->int_gpio.dt_flags & GPIO_OPEN_DRAIN) != 0) {
			int_pin |= SYS_IRQ_MODE_OPEN_DRAIN;
		} else {
			int_pin |= SYS_IRQ_MODE_PUSH_PULL;
		}
		if ((drv_cfg->int_gpio.dt_flags & GPIO_ACTIVE_LOW) != 0) {
			int_pin |= SYS_IRQ_MODE_POL_LOW;
		} else {
			int_pin |= SYS_IRQ_MODE_POL_HIGH;
		}

		ret = write_reg(dev, REG_SYS_IRQ_MODE, int_pin);
	}

	return ret;
}

static DEVICE_API(gpio, mfxstm32l152_drv_api) = {
	.pin_configure = mfxstm32l152_configure,
	.port_get_raw = mfxstm32l152_port_get_raw,
	.port_set_masked_raw = NULL,
	.port_set_bits_raw = mfxstm32l152_port_set_bits_raw,
	.port_clear_bits_raw = mfxstm32l152_port_clear_bits_raw,
	.port_toggle_bits = NULL,
	.pin_interrupt_configure = mfxstm32l152_pin_interrupt_configure,
	.manage_callback = mfxstm32l152_manage_callback,
};

#define MFXSTM32L152_INIT(inst)                                                                    \
	static struct mfxstm32l152_drv_cfg mfxstm32l152_##inst##_config = {                        \
		.common = {.port_pin_mask = 0x0fff},                                               \
		.i2c_spec = I2C_DT_SPEC_INST_GET(inst),                                            \
		.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),                        \
	};                                                                                         \
                                                                                                   \
	static struct mfxstm32l152_drv_data mfxstm32l152_##inst##_drv_data;                        \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, mfxstm32l152_init, NULL, &mfxstm32l152_##inst##_drv_data,      \
			      &mfxstm32l152_##inst##_config, POST_KERNEL,                          \
			      CONFIG_GPIO_MFXSTM32L152_INIT_PRIORITY, &mfxstm32l152_drv_api);

DT_INST_FOREACH_STATUS_OKAY(MFXSTM32L152_INIT)
