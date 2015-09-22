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
 * @file Driver for PCAL9535A I2C-based GPIO driver.
 */

#include <nanokernel.h>

#include <gpio.h>
#include <i2c.h>

#include "gpio-pcal9535a.h"

#ifndef CONFIG_GPIO_PCAL9535A_DEBUG
#define DBG(...) {;}
#else
#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#define DBG printf
#else
#include <misc/printk.h>
#define DBG printk
#endif /* CONFIG_STDOUT_CONSOLE */
#endif /* CONFIG_GPIO_PCAL9535A_DEBUG */

/* Register definitions */
#define REG_INPUT_PORT0			0x00
#define REG_INPUT_PORT1			0x01
#define REG_OUTPUT_PORT0		0x02
#define REG_OUTPUT_PORT1		0x03
#define REG_POL_INV_PORT0		0x04
#define REG_POL_INV_PORT1		0x05
#define REG_CONF_PORT0			0x06
#define REG_CONG_PORT1			0x07
#define REG_OUT_DRV_STRENGTH_PORT0_L	0x40
#define REG_OUT_DRV_STRENGTH_PORT0_H	0x41
#define REG_OUT_DRV_STRENGTH_PORT1_L	0x42
#define REG_OUT_DRV_STRENGTH_PORT1_H	0x43
#define REG_INPUT_LATCH_PORT0		0x44
#define REG_INPUT_LATCH_PORT1		0x45
#define REG_PUD_EN_PORT0		0x46
#define REG_PUD_EN_PORT1		0x47
#define REG_PUD_SEL_PORT0		0x48
#define REG_PUD_SEL_PORT1		0x49
#define REG_INT_MASK_PORT0		0x4A
#define REG_INT_MASK_PORT1		0x4B
#define REG_INT_STATUS_PORT0		0x4C
#define REG_INT_STATUS_PORT1		0x4D
#define REG_OUTPUT_PORT_CONF		0x4F

/** Store the port 0/1 data for each register pair. */
union port_data {
	uint16_t all;
	uint8_t port[2];
	uint8_t byte[2];
};

/**
 * @brief Check to see if a I2C master is identified for communication.
 *
 * @param dev Device struct.
 * @return DEV_OK if I2C master is identified, DEV_INVALID_CONF if not.
 */
static inline int _has_i2c_master(struct device *dev)
{
	struct gpio_pcal9535a_drv_data * const drv_data =
		(struct gpio_pcal9535a_drv_data * const)dev->driver_data;
	struct device * const i2c_master = drv_data->i2c_master;

	if (i2c_master)
		return DEV_OK;
	else
		return DEV_INVALID_CONF;
}

#define WAIT_10MS	(sys_clock_ticks_per_sec / 100)
/**
 * @brief Simply to wait 10ms.
 *
 * @param timer nano_timer
 */
static inline void _wait_10ms(struct nano_timer *timer)
{
	nano_fiber_timer_start(timer, WAIT_10MS);
	nano_fiber_timer_wait(timer);
}

/**
 * @brief Read both port 0 and port 1 registers of certain register function.
 *
 * Given the register in reg, read the pair of port 0 and port 1.
 *
 * @param dev Device struct of the PCAL9535A.
 * @param reg Register to read (the PORT0 of the pair of registers).
 * @param buf Buffer to read data into.
 *
 * @return DEV_OK if successful, failed otherwise.
 */
static int _read_port_regs(struct device *dev, uint8_t reg,
			  union port_data *buf)
{
	const struct gpio_pcal9535a_config * const config =
		dev->config->config_info;
	struct gpio_pcal9535a_drv_data * const drv_data =
		(struct gpio_pcal9535a_drv_data * const)dev->driver_data;
	struct device * const i2c_master = drv_data->i2c_master;
	uint16_t i2c_addr = config->i2c_slave_addr;
	int ret;
	int cnt;

	/* Tell the chip we want to read from a particular register. */
	buf->byte[0] = reg;
	buf->byte[1] = 0;
	ret = i2c_write(i2c_master, buf->byte, 1, i2c_addr);
	if (ret) {
		DBG("PCAL9535A[0x%X]: error writing to register 0x%X (%d)\n",
		    i2c_addr, reg, ret);
		goto error;
	}

	/* Then, read those register values back.
	 * The I2C bus may not be ready for read yet,
	 * so retry a few times.
	 */
	cnt = 5;
	do {
		/* wait for i2c master to idle */
		_wait_10ms(&drv_data->timer);

		ret = i2c_read(i2c_master, buf->byte, 2, i2c_addr);
		if (!ret) {
			break;
		}
	} while (cnt--);
	if (ret) {
		DBG("PCAL9535A[0x%X]: error reading from register 0x%X (%d)\n",
		    i2c_addr, reg, ret);
		goto error;
	}

	DBG("PCAL9535A[0x%X]: Read: REG[0x%X] = 0x%X, REG[0x%X] = 0x%X\n",
	    i2c_addr, reg, buf->byte[0], (reg + 1), buf->byte[1]);

error:
	return ret;
}

/**
 * @brief Write both port 0 and port 1 registers of certain register function.
 *
 * Given the register in reg, write the pair of port 0 and port 1.
 *
 * @param dev Device struct of the PCAL9535A.
 * @param reg Register to write into (the PORT0 of the pair of registers).
 * @param buf Buffer to write data from.
 *
 * @return DEV_OK if successful, failed otherwise.
 */
static int _write_port_regs(struct device *dev, uint8_t reg,
			  union port_data *buf)
{
	const struct gpio_pcal9535a_config * const config =
		dev->config->config_info;
	struct gpio_pcal9535a_drv_data * const drv_data =
		(struct gpio_pcal9535a_drv_data * const)dev->driver_data;
	struct device * const i2c_master = drv_data->i2c_master;
	uint16_t i2c_addr = config->i2c_slave_addr;
	uint8_t cmd[] = {reg, buf->byte[0], buf->byte[1]};

	return i2c_write(i2c_master, cmd, sizeof(cmd), i2c_addr);
}

/**
 * @brief Setup the pin direction (input or output)
 *
 * @param dev Device struct of the PCAL9535A
 * @param access_op Access operation (pin or port)
 * @param pin The pin number
 * @param flags Flags of pin or port
 *
 * @return DEV_OK if successful, failed otherwise
 */
static int _setup_pin_dir(struct device *dev, int access_op,
			  uint32_t pin, int flags)
{
	union port_data buf = { .all = 0 };
	uint16_t bit_mask;
	uint16_t new_value = 0;
	int ret;

	switch (access_op) {
		case GPIO_ACCESS_BY_PIN:
			ret = _read_port_regs(dev, REG_CONF_PORT0, &buf);
			if (ret != DEV_OK) {
				goto done;
			}
			bit_mask = 1 << pin;

			/* Config 0 == output, 1 == input */
			if ((flags & GPIO_DIR_MASK) == GPIO_DIR_IN) {
				new_value = 1 << pin;
			}

			buf.all &= ~bit_mask;
			buf.all |= new_value;

			break;
		case GPIO_ACCESS_BY_PORT:
			/* Config 0 == output, 1 == input */
			if ((flags & GPIO_DIR_MASK) == GPIO_DIR_IN) {
				buf.all = 0xFFFF;
			}
			break;
		default:
			ret = DEV_INVALID_OP;
			goto done;
	}

	ret = _write_port_regs(dev, REG_CONF_PORT0, &buf);

done:
	return ret;
}

/**
 * @brief Setup the pin pull up/pull down status
 *
 * @param dev Device struct of the PCAL9535A
 * @param access_op Access operation (pin or port)
 * @param pin The pin number
 * @param flags Flags of pin or port
 *
 * @return DEV_OK if successful, failed otherwise
 */
static int _setup_pin_pullupdown(struct device *dev, int access_op,
				 uint32_t pin, int flags)
{
	union port_data buf = { .all = 0 };
	uint16_t bit_mask;
	uint16_t new_value = 0;
	int ret;

	/* If disabling pull up/down, there is no need to set the selection
	 * register. Just go straight to disabling.
	 */
	if ((flags & GPIO_PUD_MASK) == GPIO_PUD_NORMAL) {
		goto en_dis;
	}

	/* Setup pin pull up or pull down */
	switch (access_op) {
		case GPIO_ACCESS_BY_PIN:
			ret = _read_port_regs(dev, REG_PUD_SEL_PORT0, &buf);
			if (ret != DEV_OK) {
				goto done;
			}
			bit_mask = 1 << pin;

			/* pull down == 0, pull up == 1*/
			if ((flags & GPIO_PUD_MASK) == GPIO_PUD_PULL_UP) {
				new_value = 1 << pin;
			}

			buf.all &= ~bit_mask;
			buf.all |= new_value;

			break;
		case GPIO_ACCESS_BY_PORT:
			/* pull down == 0, pull up == 1*/
			if ((flags & GPIO_PUD_MASK) == GPIO_PUD_PULL_UP) {
				buf.all = 0xFFFF;
			}
			break;
		default:
			ret = DEV_INVALID_OP;
			goto done;
	}

	ret = _write_port_regs(dev, REG_PUD_SEL_PORT0, &buf);
	if (ret) {
		goto done;
	}

en_dis:
	/* enable/disable pull up/down */
	switch (access_op) {
		case GPIO_ACCESS_BY_PIN:
			ret = _read_port_regs(dev, REG_PUD_EN_PORT0, &buf);
			if (ret != DEV_OK) {
				goto done;
			}
			bit_mask = 1 << pin;

			if ((flags & GPIO_PUD_MASK) != GPIO_PUD_NORMAL) {
				new_value = 1 << pin;
			}

			buf.all &= ~bit_mask;
			buf.all |= new_value;

			break;
		case GPIO_ACCESS_BY_PORT:
			if ((flags & GPIO_PUD_MASK) != GPIO_PUD_NORMAL) {
				buf.all = 0xFFFF;
			}
			break;
		default:
			ret = DEV_INVALID_OP;
			goto done;
	}

	ret = _write_port_regs(dev, REG_PUD_EN_PORT0, &buf);

done:
	return ret;
}

/**
 * @brief Setup the polarity of pin or port
 *
 * @param dev Device struct of the PCAL9535A
 * @param access_op Access operation (pin or port)
 * @param pin The pin number
 * @param flags Flags of pin or port
 *
 * @return DEV_OK if successful, failed otherwise
 */
static int _setup_pin_polarity(struct device *dev, int access_op,
			       uint32_t pin, int flags)
{
	struct gpio_pcal9535a_drv_data * const drv_data =
		(struct gpio_pcal9535a_drv_data * const)dev->driver_data;
	union port_data buf = { .all = 0 };
	uint16_t bit_mask;
	uint16_t new_value = 0;
	int ret;

	switch (access_op) {
		case GPIO_ACCESS_BY_PIN:
			ret = _read_port_regs(dev, REG_POL_INV_PORT0, &buf);
			if (ret != DEV_OK) {
				goto done;
			}
			bit_mask = 1 << pin;

			/* normal == 0, invert == 1 */
			if ((flags & GPIO_POL_MASK) == GPIO_POL_INV) {
				new_value = 1 << pin;
			}

			buf.all &= ~bit_mask;
			buf.all |= new_value;

			break;
		case GPIO_ACCESS_BY_PORT:
			/* normal == 0, invert == 1 */
			if ((flags & GPIO_POL_MASK) == GPIO_POL_INV) {
				buf.all = 0xFFFF;
			}

			break;
		default:
			ret = DEV_INVALID_OP;
			goto done;
	}

	ret = _write_port_regs(dev, REG_POL_INV_PORT0, &buf);
	if (!ret) {
		drv_data->out_pol_inv = buf.all;
	}

done:
	return ret;
}

/**
 * @brief Configurate pin or port
 *
 * @param dev Device struct of the PCAL9535A
 * @param access_op Access operation (pin or port)
 * @param pin The pin number
 * @param flags Flags of pin or port
 *
 * @return DEV_OK if successful, failed otherwise
 */
static int gpio_pcal9535a_config(struct device *dev, int access_op,
				 uint32_t pin, int flags)
{
	int ret;

#ifdef CONFIG_GPIO_PCAL9535A_DEBUG
	const struct gpio_pcal9535a_config * const config =
		dev->config->config_info;
	uint16_t i2c_addr = config->i2c_slave_addr;
#endif

	if (!_has_i2c_master(dev)) {
		return DEV_INVALID_CONF;
	}

	ret = _setup_pin_dir(dev, access_op, pin, flags);
	if (ret) {
		DBG("PCAL9535A[0x%X]: error setting pin direction (%d)\n",
		    i2c_addr, ret);
		goto done;
	}

	ret = _setup_pin_polarity(dev, access_op, pin, flags);
	if (ret) {
		DBG("PCAL9535A[0x%X]: error setting pin polarity (%d)\n",
		    i2c_addr, ret);
		goto done;
	}

	ret = _setup_pin_pullupdown(dev, access_op, pin, flags);
	if (ret) {
		DBG("PCAL9535A[0x%X]: error setting pin pull up/down (%d)\n",
		    i2c_addr, ret);
		goto done;
	}

done:
	return ret;
}

/**
 * @brief Set the pin or port output
 *
 * @param dev Device struct of the PCAL9535A
 * @param access_op Access operation (pin or port)
 * @param pin The pin number
 * @param value Value to set (0 or 1)
 *
 * @return DEV_OK if successful, failed otherwise
 */
static int gpio_pcal9535a_write(struct device *dev, int access_op,
				uint32_t pin, uint32_t value)
{
	struct gpio_pcal9535a_drv_data * const drv_data =
		(struct gpio_pcal9535a_drv_data * const)dev->driver_data;
	union port_data buf;
	uint16_t bit_mask;
	uint16_t new_value;
	int ret;

	if (!_has_i2c_master(dev)) {
		return DEV_INVALID_CONF;
	}

	/* Invert input value for pins configurated as active low. */
	switch (access_op) {
		case GPIO_ACCESS_BY_PIN:
			ret = _read_port_regs(dev, REG_OUTPUT_PORT0, &buf);
			if (ret != DEV_OK) {
				goto done;
			}
			bit_mask = 1 << pin;

			new_value = (value << pin) & bit_mask;
			new_value ^= (drv_data->out_pol_inv & bit_mask);
			new_value &= bit_mask;

			buf.all &= ~bit_mask;
			buf.all |= new_value;

			break;
		case GPIO_ACCESS_BY_PORT:
			buf.all = value;
			bit_mask = drv_data->out_pol_inv;

			new_value = value & bit_mask;
			new_value ^= drv_data->out_pol_inv;
			new_value &= bit_mask;

			buf.all &= ~bit_mask;
			buf.all |= new_value;

			break;
		default:
			ret = DEV_INVALID_OP;
			goto done;
	}

	ret = _write_port_regs(dev, REG_OUTPUT_PORT0, &buf);

done:
	return ret;
}

/**
 * @brief Read the pin or port status
 *
 * @param dev Device struct of the PCAL9535A
 * @param access_op Access operation (pin or port)
 * @param pin The pin number
 * @param value Value of input pin(s)
 *
 * @return DEV_OK if successful, failed otherwise
 */
static int gpio_pcal9535a_read(struct device *dev, int access_op,
			       uint32_t pin, uint32_t *value)
{
	union port_data buf;
	int ret;

	if (!_has_i2c_master(dev)) {
		return DEV_INVALID_CONF;
	}

	ret = _read_port_regs(dev, REG_INPUT_PORT0, &buf);
	if (ret != DEV_OK) {
		goto done;
	}

	switch (access_op) {
		case GPIO_ACCESS_BY_PIN:
			*value = (buf.all >> pin) & 0x01;
			break;
		case GPIO_ACCESS_BY_PORT:
			*value = buf.all;
			break;
		default:
			ret = DEV_INVALID_OP;
			break;
	}

done:
	return ret;
}

static int gpio_pcal9535a_set_callback(struct device *dev,
				       gpio_callback_t callback)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(callback);

	return DEV_INVALID_OP;
}

static int gpio_pcal9535a_enable_callback(struct device *dev,
					  int access_op, uint32_t pin)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(access_op);
	ARG_UNUSED(pin);

	return DEV_INVALID_OP;
}

static int gpio_pcal9535a_disable_callback(struct device *dev,
					   int access_op, uint32_t pin)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(access_op);
	ARG_UNUSED(pin);

	return DEV_INVALID_OP;
}

static int gpio_pcal9535a_suspend_port(struct device *dev)
{
	if (!_has_i2c_master(dev)) {
		return DEV_INVALID_CONF;
	}

	return DEV_INVALID_OP;
}

static int gpio_pcal9535a_resume_port(struct device *dev)
{
	if (!_has_i2c_master(dev)) {
		return DEV_INVALID_CONF;
	}

	return DEV_INVALID_OP;
}

static struct gpio_driver_api gpio_pcal9535a_drv_api_funcs = {
	.config = gpio_pcal9535a_config,
	.write = gpio_pcal9535a_write,
	.read = gpio_pcal9535a_read,
	.set_callback = gpio_pcal9535a_set_callback,
	.enable_callback = gpio_pcal9535a_enable_callback,
	.disable_callback = gpio_pcal9535a_disable_callback,
	.suspend = gpio_pcal9535a_suspend_port,
	.resume = gpio_pcal9535a_resume_port,
};

/**
 * @brief Initialization function of PCAL9535A
 *
 * @param dev Device struct
 * @return DEV_OK if successful, failed otherwise.
 */
int gpio_pcal9535a_init(struct device *dev)
{
	const struct gpio_pcal9535a_config * const config =
		dev->config->config_info;
	struct gpio_pcal9535a_drv_data * const drv_data =
		(struct gpio_pcal9535a_drv_data * const)dev->driver_data;
	struct device *i2c_master;

	dev->driver_api = &gpio_pcal9535a_drv_api_funcs;

	/* Find out the device struct of the I2C master */
	i2c_master = device_get_binding((char *)config->i2c_master_dev_name);
	if (!i2c_master) {
		return DEV_INVALID_CONF;
	}
	drv_data->i2c_master = i2c_master;

	nano_timer_init(&drv_data->timer, (void *) 0);

	return DEV_OK;
}

/* Initialization for PCAL9535A_0 */
#ifdef CONFIG_GPIO_PCAL9535A_0
#include <device.h>
#include <init.h>

static struct gpio_pcal9535a_config gpio_pcal9535a_0_cfg = {
	.i2c_master_dev_name = CONFIG_GPIO_PCAL9535A_0_I2C_MASTER_DEV_NAME,
	.i2c_slave_addr = CONFIG_GPIO_PCAL9535A_0_I2C_ADDR,
};

static struct gpio_pcal9535a_drv_data gpio_pcal9535a_0_drvdata;

DECLARE_DEVICE_INIT_CONFIG(gpio_pcal9535a_0,
			   CONFIG_GPIO_PCAL9535A_0_DEV_NAME,
			   gpio_pcal9535a_init, &gpio_pcal9535a_0_cfg);

/* This has to init after I2C master */
nano_early_init(gpio_pcal9535a_0, &gpio_pcal9535a_0_drvdata);

#endif /* CONFIG_GPIO_PCAL9535A_0 */

/* Initialization for PCAL9535A_1 */
#ifdef CONFIG_GPIO_PCAL9535A_1
#include <device.h>
#include <init.h>

static struct gpio_pcal9535a_config gpio_pcal9535a_1_cfg = {
	.i2c_master_dev_name = CONFIG_GPIO_PCAL9535A_1_I2C_MASTER_DEV_NAME,
	.i2c_slave_addr = CONFIG_GPIO_PCAL9535A_1_I2C_ADDR,
};

static struct gpio_pcal9535a_drv_data gpio_pcal9535a_1_drvdata;

DECLARE_DEVICE_INIT_CONFIG(gpio_pcal9535a_1,
			   CONFIG_GPIO_PCAL9535A_1_DEV_NAME,
			   gpio_pcal9535a_init, &gpio_pcal9535a_1_cfg);

/* This has to init after I2C master */
nano_early_init(gpio_pcal9535a_1, &gpio_pcal9535a_1_drvdata);

#endif /* CONFIG_GPIO_PCAL9535A_1 */

/* Initialization for PCAL9535A_2 */
#ifdef CONFIG_GPIO_PCAL9535A_2
#include <device.h>
#include <init.h>

static struct gpio_pcal9535a_config gpio_pcal9535a_2_cfg = {
	.i2c_master_dev_name = CONFIG_GPIO_PCAL9535A_2_I2C_MASTER_DEV_NAME,
	.i2c_slave_addr = CONFIG_GPIO_PCAL9535A_2_I2C_ADDR,
};

static struct gpio_pcal9535a_drv_data gpio_pcal9535a_2_drvdata;

DECLARE_DEVICE_INIT_CONFIG(gpio_pcal9535a_2,
			   CONFIG_GPIO_PCAL9535A_2_DEV_NAME,
			   gpio_pcal9535a_init, &gpio_pcal9535a_2_cfg);

/* This has to init after I2C master */
nano_early_init(gpio_pcal9535a_2, &gpio_pcal9535a_2_drvdata);

#endif /* CONFIG_GPIO_PCAL9535A_2 */

/* Initialization for PCAL9535A_3 */
#ifdef CONFIG_GPIO_PCAL9535A_3
#include <device.h>
#include <init.h>

static struct gpio_pcal9535a_config gpio_pcal9535a_3_cfg = {
	.i2c_master_dev_name = CONFIG_GPIO_PCAL9535A_3_I2C_MASTER_DEV_NAME,
	.i2c_slave_addr = CONFIG_GPIO_PCAL9535A_3_I2C_ADDR,
};

static struct gpio_pcal9535a_drv_data gpio_pcal9535a_3_drvdata;

DECLARE_DEVICE_INIT_CONFIG(gpio_pcal9535a_3,
			   CONFIG_GPIO_PCAL9535A_3_DEV_NAME,
			   gpio_pcal9535a_init, &gpio_pcal9535a_3_cfg);

/* This has to init after I2C master */
nano_early_init(gpio_pcal9535a_3, &gpio_pcal9535a_3_drvdata);

#endif /* CONFIG_GPIO_PCAL9535A_3 */
