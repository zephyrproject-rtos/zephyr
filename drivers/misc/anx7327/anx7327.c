/*
 * Copyright (c) 2025 Applied Materials
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

#include "anx7327_reg.h"

#define DT_DRV_COMPAT analogix_anx7327

LOG_MODULE_REGISTER(anx7327, LOG_LEVEL_DBG);

struct anx7327_config {
	struct i2c_dt_spec bus;
	uint16_t i2c_dev_addr2;
	struct gpio_dt_spec int_pin;
};

struct anx7327_priv {
	uint8_t dev;
	uint16_t dev_addr;
	uint16_t dev_addr_2;
};

static struct gpio_callback anx7327_irq_data;

static void anx7327_irq(const struct device *dev, struct gpio_callback *cb,
			uint32_t pins)
{
	LOG_ERR("anx7327 IRQ\n");
}

int anx7327_wakeup(const struct device *dev)
{
	struct anx7327_priv *priv = (struct anx7327_priv *)(dev->data);
	const struct anx7327_config *cfg = dev->config;
	int err;

	err = i2c_reg_write_byte(cfg->bus.bus, priv->dev_addr, 0x00, 0x00);
	if (err)
		LOG_ERR("wake up anx7327 failed: %d ", err);

	return err;
}


static int anx7327_read_reg(const struct device *dev, uint8_t reg_addr,
		     uint8_t offset, uint8_t *data)
{
	const struct anx7327_config *cfg = dev->config;
	int err;

	err = anx7327_wakeup(dev);
	if (err)
		return err;

	err = i2c_reg_read_byte(cfg->bus.bus, reg_addr, offset, data);
	if (err < 0)
		LOG_ERR("failed read reg : i2c addr=%x\n", offset);

	return err;
}

static int anx7327_write_reg(const struct device *dev, uint8_t reg_addr,
		      uint8_t offset, uint8_t data)
{
	const struct anx7327_config *cfg = dev->config;
	int err;

	err = anx7327_wakeup(dev);
	if (err)
		return err;

	err = i2c_reg_write_byte(cfg->bus.bus, reg_addr, offset, data);
	if (err < 0)
		LOG_ERR("failed write reg : i2c addr=%x\n", offset);

	return err;
}

int anx7327_write_config(const struct device *dev)
{
	struct anx7327_priv *priv = (struct anx7327_priv *)(dev->data);
	uint8_t read_data = 0;
	uint8_t write_data;
	int err = 0;

	write_data = (1 << ANX7327_REMS_READ_EN) | (1 << ANX7327_RDID_READ_EN);
	LOG_INF("ANX7327 write data %x\n", write_data);
	err = anx7327_write_reg(dev, priv->dev_addr_2, ANX7327_REMS_REG, write_data);
	if (err) {
		LOG_ERR("error writting config to anx7327\n");
		return err;
	}

	err = anx7327_read_reg(dev, priv->dev_addr_2, ANX7327_REMS_REG, &read_data);
	if (err) {
		LOG_ERR("error reading config from anx7327\n");
		return err;
	}
	LOG_INF("ANX7327 config %x\n", read_data);
	return err;
}


static int anx7327_init_gpio(const struct device *dev)
{
	const struct anx7327_config *cfg = dev->config;
	int err;

	if (!gpio_is_ready_dt(&cfg->int_pin)) {
		LOG_ERR("Error: int pin is not ready\n");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&cfg->int_pin, GPIO_INPUT);
	if (err != 0) {
		LOG_ERR("Error %d: failed to configure int pin\n", err);
		return -ENODEV;
	}
	err = gpio_pin_interrupt_configure_dt(&cfg->int_pin,
					      GPIO_INT_EDGE_FALLING);
	if (err != 0) {
		LOG_ERR("Error %d: failed to configure interrupt pin\n", err);
		return -ENODEV;
	}

	gpio_init_callback(&anx7327_irq_data, anx7327_irq, BIT(cfg->int_pin.pin));
	gpio_add_callback(cfg->int_pin.port, &anx7327_irq_data);

	LOG_ERR("Setup int at %s pin %d\n", cfg->int_pin.port->name,
					    cfg->int_pin.pin);
	return 0;
}

static int anx7323_get_id(const struct device *dev)
{
	struct anx7327_priv *priv = (struct anx7327_priv *)(dev->data);
	uint8_t read_data = 0;
	int err;

	err = anx7327_read_reg(dev, priv->dev_addr_2, ANX7327_DEV_ID_REG,
			       &read_data);
	if (err) {
		LOG_ERR("error reading config from anx7327\n");
		return err;
	}
	LOG_INF("dev id %u\n", read_data);

	return err;
}

static int anx7327_init(const struct device *dev)
{
	struct anx7327_priv *priv = (struct anx7327_priv *)(dev->data);
	const struct anx7327_config *cfg = dev->config;
	int err = 0;

	LOG_DBG("ANX7327 initialize called");

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("ANX7327 i2c device not ready.");
		err = -ENODEV;
		goto out;
	}

	if (!device_is_ready(cfg->bus.bus)) {
		LOG_ERR("ANX7327 device not ready.");
		err = -ENODEV;
		goto out;
	}

	priv->dev_addr = (cfg->bus.addr >> 1);
	priv->dev_addr_2 = (cfg->i2c_dev_addr2 >> 1);

	err = anx7327_init_gpio(dev);
	if (err)
		LOG_ERR("Error initialzing gpio pins\n");

	anx7323_get_id(dev);

	err = anx7327_wakeup(dev);
	if (err)
		LOG_ERR("Error initialzing i2c bus\n");
	else {
		k_busy_wait(500);
		anx7327_write_config(dev);
	}

out:
	return err;
}

#if defined(CONFIG_SHELL)

static int anx7327_dump_reg(const struct shell *sh, size_t argc, char **argv)
{
	struct anx7327_priv *priv;
	const struct device *dev;
	uint8_t reg, reg2;

	shell_print(sh, "dump regs : %s\n", argv[1]);
	/* device is the only arg, hard coded to 1 */
	dev = shell_device_get_binding(argv[1]);
	if (!dev) {
		shell_error(sh, "anx7327 device not found");
		return -EINVAL;
	}
	priv = (struct anx7327_priv *)(dev->data);

	// read device ID.
	anx7327_read_reg(dev, priv->dev_addr_2, ANX7327_DEV_ID_REG, &reg);
	shell_print(sh, "anx7327 - DEV ID: %u\n", reg);

	// read manufacture ID
	anx7327_read_reg(dev, priv->dev_addr_2, ANX7327_MANF_ID_REG, &reg);
	shell_print(sh, "anx7327 - MANF ID: %u\n", reg);

	// HPD CTRL
	anx7327_read_reg(dev, priv->dev_addr_2, ANX7327_HPD_CTL0_REG, &reg);
	shell_print(sh, "anx7327 - HPD CTL0: %u\n", reg);

	// Interface and Status
	anx7327_read_reg(dev, priv->dev_addr_2, ANX7327_USBC_STATUS_REG, &reg);
	shell_print(sh, "anx7327 - USBC STATUS: %u\n", reg);

	// Reading CC signal
	anx7327_read_reg(dev, priv->dev_addr, ANX7327_CC_STATUS_REG, &reg);
	shell_print(sh, "anx7327 - CC STATUS REG: %u\n", reg);

	anx7327_read_reg(dev, priv->dev_addr_2, ANX7327_ADDR_INTP_SRC0_REG, &reg);
	shell_print(sh, "anx7327 - INTP SOURCE 0: %u\n", reg);

	anx7327_read_reg(dev, priv->dev_addr_2, ANX7327_ADDR_INTP_SRC1_REG, &reg2);
	shell_print(sh, "anx7327 - INTP SOURCE 1: %u\n", reg2);

	// tcpcVendorId
	anx7327_read_reg(dev, priv->dev_addr, ANX7327_TCPC_VENDOR_ID0_REG, &reg);
	shell_print(sh, "anx7327 - TCPC VENDOR ID 0: %u\n", reg);

	anx7327_read_reg(dev, priv->dev_addr, ANX7327_TCPC_VENDOR_ID1_REG, &reg2);
	shell_print(sh, "anx7327 - TCPC VENDOR ID 1: %u\n", reg2);

	// tcpcProductId
	anx7327_read_reg(dev, priv->dev_addr, ANX7327_TCPC_PRODUCT_ID0_REG, &reg);
	shell_print(sh, "anx7327 - TCPC PRODUCT ID 0: %u\n", reg);

	anx7327_read_reg(dev, priv->dev_addr, ANX7327_TCPC_PRODUCT_ID1_REG, &reg2);
	shell_print(sh, "anx7327 - TCPC PRODUCT ID 1: %u\n", reg2);

	// tcpcDeviceId
	anx7327_read_reg(dev, priv->dev_addr, ANX7327_TCPC_DEVICE_ID0_REG, &reg);
	shell_print(sh, "anx7327 - TCPC DEVICE ID 0: %u\n", reg);

	anx7327_read_reg(dev, priv->dev_addr, ANX7327_TCPC_DEVICE_ID1_REG, &reg2);
	shell_print(sh, "anx7327 - TCPC DEVICE ID 1: %u\n", reg2);

	// device alerts & faults
	anx7327_read_reg(dev, priv->dev_addr, ANX7327_ALERT0_REG, &reg);
	shell_print(sh, "anx7327 - ALERT 0: %u\n", reg);

	anx7327_read_reg(dev, priv->dev_addr, ANX7327_ALERT1_REG, &reg2);
	shell_print(sh, "anx7327 - ALERT 1: %u\n", reg2);

	anx7327_read_reg(dev, priv->dev_addr, ANX7327_FAULT_REG, &reg);
	shell_print(sh, "anx7327 - FAULT: %u\n", reg);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(anx7327_cmds,
	SHELL_CMD_ARG(dump_reg, NULL, "<device>", anx7327_dump_reg, 2, 0),
	SHELL_SUBCMD_SET_END
);

static int anx7327_cmd(const struct shell *sh, size_t argc, char **argv)
{
	shell_error(sh, "%s: unknown parameter: %s", argv[0], argv[1]);
	return -EINVAL;
}

SHELL_COND_CMD_ARG_REGISTER(CONFIG_ANX7327_SHELL, anx7327, &anx7327_cmds,
		       "anx7327 shell commands", anx7327_cmd, 2, 0);

#endif /* defined(CONFIG_SHELL) */

#define ANX7327_INIT(n)                                                        \
	static struct anx7327_priv anx7327_priv_##n = {                        \
		.dev = n,						       \
	};                                                                     \
                                                                               \
	static const struct anx7327_config anx7327_cfg_##n = {                 \
		.bus = I2C_DT_SPEC_INST_GET(n),				       \
		.int_pin = GPIO_DT_SPEC_INST_GET(n, int_pin_gpios),	       \
		.i2c_dev_addr2 = DT_INST_PROP(n, i2c_addr_2),		       \
	};								       \
                                                                               \
	DEVICE_DT_INST_DEFINE(n, anx7327_init, NULL, &anx7327_priv_##n,        \
			      &anx7327_cfg_##n, POST_KERNEL,		       \
			      CONFIG_ANX7327_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(ANX7327_INIT)
