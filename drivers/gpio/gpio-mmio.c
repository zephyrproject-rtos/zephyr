/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file Driver for the MMIO-based GPIO driver.
 */

#include <nanokernel.h>

#include <gpio.h>

#include "gpio-mmio.h"


static void _set_bit(uint32_t addr, uint32_t bit, uint8_t value)
{
	if (!value) {
		sys_clear_bit(addr, bit);
	} else {
		sys_set_bit(addr, bit);
	}
}

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
			_set_bit(cfg->reg.dir, pin, value);
			break;
		case GPIO_ACCESS_BY_PORT:
			sys_write32(value, cfg->reg.dir);
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
			_set_bit(cfg->reg.en, pin, value);
			break;
		case GPIO_ACCESS_BY_PORT:
			sys_write32(value, cfg->reg.en);
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
			_set_bit(cfg->reg.output, pin, value);
			break;
		case GPIO_ACCESS_BY_PORT:
			sys_write32(value, cfg->reg.output);
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
			*value = sys_read32(cfg->reg.input);
			*value &= (1 << pin) >> pin;
			break;
		case GPIO_ACCESS_BY_PORT:
			*value = sys_read32(cfg->reg.input);
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
};

DECLARE_DEVICE_INIT_CONFIG(gpio_mmio_0,
			   CONFIG_GPIO_MMIO_0_DEV_NAME,
			   gpio_mmio_init,
			   &gpio_mmio_0_cfg);
pure_late_init(gpio_mmio_0, (void *)0);

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
};

DECLARE_DEVICE_INIT_CONFIG(gpio_mmio_1,
			   CONFIG_GPIO_MMIO_1_DEV_NAME,
			   gpio_mmio_init,
			   &gpio_mmio_1_cfg);
pure_late_init(gpio_mmio_1, (void *)0);

#endif /* CONFIG_GPIO_MMIO_1 */
