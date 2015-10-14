/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file Driver for the MMIO-based GPIO driver.
 */

#include <nanokernel.h>

#include <gpio.h>

#include "gpio_mmio.h"

#if defined(CONFIG_GPIO_MMIO_0_ACCESS_MM) \
    || defined(CONFIG_GPIO_MMIO_1_ACCESS_MM)

static uint32_t _mm_set_bit(uint32_t addr, uint32_t bit, uint32_t value)
{
	if (!value) {
		sys_clear_bit(addr, bit);
	} else {
		sys_set_bit(addr, bit);
	}

	return 0;
}

static uint32_t _mm_read(uint32_t addr, uint32_t bit, uint32_t value)
{
	ARG_UNUSED(bit);
	ARG_UNUSED(value);

	return sys_read32(addr);
}

static uint32_t _mm_write(uint32_t addr, uint32_t bit, uint32_t value)
{
	ARG_UNUSED(bit);

	sys_write32(value, addr);

	return 0;
}

#endif /* if direct memory access is needed */

#if defined(CONFIG_GPIO_MMIO_0_ACCESS_IO) \
    || defined(CONFIG_GPIO_MMIO_1_ACCESS_IO)

static uint32_t _io_set_bit(uint32_t addr, uint32_t bit, uint32_t value)
{
	uint32_t bit_mask = (1 << bit);
	uint32_t tmp;

	tmp = sys_in32(addr);
	tmp &= ~bit_mask;
	tmp |= (value << bit) & bit_mask;
	sys_out32(tmp, addr);

	return 0;
}

static uint32_t _io_read(uint32_t addr, uint32_t bit, uint32_t value)
{
	ARG_UNUSED(bit);
	ARG_UNUSED(value);

	return sys_in32(addr);
}

static uint32_t _io_write(uint32_t addr, uint32_t bit, uint32_t value)
{
	ARG_UNUSED(bit);

	sys_out32(value, addr);

	return 0;
}

#endif /* if io port access is needed */

/**
 * @brief Configurate pin or port
 *
 * @param dev Device struct
 * @param access_op Access operation (pin or port)
 * @param pin The pin number
 * @param flags Flags of pin or port
 *
 * @return DEV_OK if successful, failed otherwise
 */
static int gpio_mmio_config(struct device *dev, int access_op,
					 uint32_t pin, int flags)
{
	const struct gpio_mmio_config * const cfg =
		dev->config->config_info;
	uint32_t value = 0;

	/* Setup direction register */

	if (!cfg->reg.dir) {
		return DEV_INVALID_CONF;
	}

	if (cfg->cfg_flags & GPIO_MMIO_CFG_DIR_MASK) {
		/* Direction register is inverse
		 * INV: 0 - pin is input, 1 - pin is output
		 */
		if ((flags & GPIO_DIR_MASK) == GPIO_DIR_IN) {
			value = 0x0;
		} else {
			value = 0xFFFFFFFF;
		}
	} else {
		/* Direction register is normal.
		 * NORMAL: 0 - pin is output, 1 - pin is input
		 */
		if ((flags & GPIO_DIR_MASK) == GPIO_DIR_IN) {
			value = 0xFFFFFFFF;
		} else {
			value = 0x0;
		}
	}

	switch (access_op) {
	case GPIO_ACCESS_BY_PIN:
		cfg->access.set_bit(cfg->reg.dir, pin, value);
		break;
	case GPIO_ACCESS_BY_PORT:
		cfg->access.write(cfg->reg.dir, 0, value);
		break;
	default:
		return DEV_INVALID_OP;
	}

	/*
	 * Enable the GPIO pin(s), since the direction is
	 * also being setup. This indicates pin(s) is being used.
	 *
	 * This is not really necessary, so don't fail if
	 * register is not defined.
	 */

	if (!cfg->reg.en) {
		return DEV_OK;
	}

	if (cfg->cfg_flags & GPIO_MMIO_CFG_EN_MASK) {
		/* INV: 0 - enable, 1 - disable */
		value = 0x0;
	} else {
		/* NORMAL: 0 - disable, 1 - enable */
		value = 0xFFFFFFFF;
	}

	switch (access_op) {
	case GPIO_ACCESS_BY_PIN:
		cfg->access.set_bit(cfg->reg.en, pin, value);
		break;
	case GPIO_ACCESS_BY_PORT:
		cfg->access.write(cfg->reg.en, 0, value);
		break;
	default:
		return DEV_INVALID_OP;
	}

	return DEV_OK;
}

/**
 * @brief Set the pin or port output
 *
 * @param dev Device struct
 * @param access_op Access operation (pin or port)
 * @param pin The pin number
 * @param value Value to set (0 or 1)
 *
 * @return DEV_OK if successful, failed otherwise
 */
static int gpio_mmio_write(struct device *dev, int access_op,
			   uint32_t pin, uint32_t value)
{
	const struct gpio_mmio_config * const cfg =
		dev->config->config_info;

	if (!cfg->reg.output) {
		return DEV_INVALID_CONF;
	}

	switch (access_op) {
	case GPIO_ACCESS_BY_PIN:
		cfg->access.set_bit(cfg->reg.output, pin, value);
		break;
	case GPIO_ACCESS_BY_PORT:
		cfg->access.write(cfg->reg.output, 0, value);
		break;
	default:
		return DEV_INVALID_OP;
	}

	return DEV_OK;
}

/**
 * @brief Read the pin or port status
 *
 * @param dev Device struct
 * @param access_op Access operation (pin or port)
 * @param pin The pin number
 * @param value Value of input pin(s)
 *
 * @return DEV_OK if successful, failed otherwise
 */
static int gpio_mmio_read(struct device *dev, int access_op,
				       uint32_t pin, uint32_t *value)
{
	const struct gpio_mmio_config * const cfg =
		dev->config->config_info;

	if (!cfg->reg.input) {
		return DEV_INVALID_CONF;
	}

	switch (access_op) {
	case GPIO_ACCESS_BY_PIN:
		*value = cfg->access.read(cfg->reg.input, 0, 0);
		*value &= (1 << pin) >> pin;
		break;
	case GPIO_ACCESS_BY_PORT:
		*value = cfg->access.read(cfg->reg.input, 0, 0);
		break;
	default:
		return DEV_INVALID_OP;
	}

	return DEV_OK;
}

static int gpio_mmio_set_callback(struct device *dev,
					       gpio_callback_t callback)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(callback);

	return DEV_INVALID_OP;
}

static int gpio_mmio_enable_callback(struct device *dev,
						  int access_op, uint32_t pin)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(access_op);
	ARG_UNUSED(pin);

	return DEV_INVALID_OP;
}

static int gpio_mmio_disable_callback(struct device *dev,
						   int access_op, uint32_t pin)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(access_op);
	ARG_UNUSED(pin);

	return DEV_INVALID_OP;
}

static int gpio_mmio_suspend_port(struct device *dev)
{
	ARG_UNUSED(dev);

	return DEV_INVALID_OP;
}

static int gpio_mmio_resume_port(struct device *dev)
{
	ARG_UNUSED(dev);

	return DEV_INVALID_OP;
}

static struct gpio_driver_api gpio_mmio_drv_api_funcs = {
	.config = gpio_mmio_config,
	.write = gpio_mmio_write,
	.read = gpio_mmio_read,
	.set_callback = gpio_mmio_set_callback,
	.enable_callback = gpio_mmio_enable_callback,
	.disable_callback = gpio_mmio_disable_callback,
	.suspend = gpio_mmio_suspend_port,
	.resume = gpio_mmio_resume_port,
};

/**
 * @brief Initialization function of MMIO
 *
 * @param dev Device struct
 * @return DEV_OK if successful, failed otherwise.
 */
int gpio_mmio_init(struct device *dev)
{
	dev->driver_api = &gpio_mmio_drv_api_funcs;

	return DEV_OK;
}

/* Initialization for MMIO_0 */
#ifdef CONFIG_GPIO_MMIO_0
#include <device.h>
#include <init.h>

static struct gpio_mmio_config gpio_mmio_0_cfg = {
	.cfg_flags = CONFIG_GPIO_MMIO_0_CFG,

	.reg.en = CONFIG_GPIO_MMIO_0_EN,
	.reg.dir = CONFIG_GPIO_MMIO_0_DIR,
	.reg.input = CONFIG_GPIO_MMIO_0_INPUT,
	.reg.output = CONFIG_GPIO_MMIO_0_OUTPUT,

#ifdef CONFIG_GPIO_MMIO_0_ACCESS_IO
	.access.set_bit = _io_set_bit,
	.access.read = _io_read,
	.access.write = _io_write,
#else /* below is for CONFIG_GPIO_MMIO_0_ACCESS_MM=y or else */
	.access.set_bit = _mm_set_bit,
	.access.read = _mm_read,
	.access.write = _mm_write,
#endif
};

DECLARE_DEVICE_INIT_CONFIG(gpio_mmio_0,
			   CONFIG_GPIO_MMIO_0_DEV_NAME,
			   gpio_mmio_init,
			   &gpio_mmio_0_cfg);
pre_kernel_late_init(gpio_mmio_0, (void *)0);

#endif /* CONFIG_GPIO_MMIO_0 */

/* Initialization for MMIO_1 */
#ifdef CONFIG_GPIO_MMIO_1
#include <device.h>
#include <init.h>

static struct gpio_mmio_config gpio_mmio_1_cfg = {
	.cfg_flags = CONFIG_GPIO_MMIO_0_CFG,

	.reg.en = CONFIG_GPIO_MMIO_1_EN,
	.reg.dir = CONFIG_GPIO_MMIO_1_DIR,
	.reg.input = CONFIG_GPIO_MMIO_1_INPUT,
	.reg.output = CONFIG_GPIO_MMIO_1_OUTPUT,

#ifdef CONFIG_GPIO_MMIO_1_ACCESS_IO
	.access.set_bit = _io_set_bit,
	.access.read = _io_read,
	.access.write = _io_write,
#else /* below is for CONFIG_GPIO_MMIO_1_ACCESS_MM=y or else */
	.access.set_bit = _mm_set_bit,
	.access.read = _mm_read,
	.access.write = _mm_write,
#endif
};

DECLARE_DEVICE_INIT_CONFIG(gpio_mmio_1,
			   CONFIG_GPIO_MMIO_1_DEV_NAME,
			   gpio_mmio_init,
			   &gpio_mmio_1_cfg);
pre_kernel_late_init(gpio_mmio_1, (void *)0);

#endif /* CONFIG_GPIO_MMIO_1 */
