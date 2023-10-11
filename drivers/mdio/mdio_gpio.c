/*
 * Copyright (c) 2023 Aleksandr Senin
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_mdio_gpio

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mdio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mdio_gpio, CONFIG_MDIO_LOG_LEVEL);

#define MDIO_GPIO_READ_OP  0
#define MDIO_GPIO_WRITE_OP 1
#define MDIO_GPIO_MSB      0x80000000

struct mdio_gpio_data {
	struct k_sem sem;
};

struct mdio_gpio_config {
	struct gpio_dt_spec mdc_gpio;
	struct gpio_dt_spec mdio_gpio;
};

static ALWAYS_INLINE void mdio_gpio_clock_the_bit(const struct mdio_gpio_config *dev_cfg)
{
	k_busy_wait(1);
	gpio_pin_set_dt(&dev_cfg->mdc_gpio, 1);
	k_busy_wait(1);
	gpio_pin_set_dt(&dev_cfg->mdc_gpio, 0);
}

static ALWAYS_INLINE void mdio_gpio_dir(const struct mdio_gpio_config *dev_cfg, uint8_t dir)
{
	gpio_pin_configure_dt(&dev_cfg->mdio_gpio, dir ? GPIO_OUTPUT_ACTIVE : GPIO_INPUT);
	if (dir == 0) {
		mdio_gpio_clock_the_bit(dev_cfg);
	}
}

static ALWAYS_INLINE void mdio_gpio_read(const struct mdio_gpio_config *dev_cfg, uint16_t *pdata)
{
	uint16_t data = 0;

	for (uint16_t i = 0; i < 16; i++) {
		data <<= 1;
		mdio_gpio_clock_the_bit(dev_cfg);
		if (gpio_pin_get_dt(&dev_cfg->mdio_gpio) == 1) {
			data |= 1;
		}
	}

	*pdata = data;
}

static ALWAYS_INLINE void mdio_gpio_write(const struct mdio_gpio_config *dev_cfg,
					  uint32_t data, uint8_t len)
{
	uint32_t v_data = data;
	uint32_t v_len = len;

	v_data <<= 32 - v_len;
	for (; v_len > 0; v_len--) {
		gpio_pin_set_dt(&dev_cfg->mdio_gpio, (v_data & MDIO_GPIO_MSB) ? 1 : 0);
		mdio_gpio_clock_the_bit(dev_cfg);
		v_data <<= 1;
	}
}

static int mdio_gpio_transfer(const struct device *dev, uint8_t prtad, uint8_t devad, uint8_t rw,
			      uint16_t data_in, uint16_t *data_out)
{
	const struct mdio_gpio_config *const dev_cfg = dev->config;
	struct mdio_gpio_data *const dev_data = dev->data;

	k_sem_take(&dev_data->sem, K_FOREVER);

	/* DIR: output */
	mdio_gpio_dir(dev_cfg, MDIO_GPIO_WRITE_OP);
	/* PRE32: 32 bits '1' for sync*/
	mdio_gpio_write(dev_cfg, 0xFFFFFFFF, 32);
	/* ST: 2 bits start of frame */
	mdio_gpio_write(dev_cfg, 0x1, 2);
	/* OP: 2 bits opcode, read '10' or write '01' */
	mdio_gpio_write(dev_cfg, rw ? 0x1 : 0x2, 2);
	/* PA5: 5 bits PHY address */
	mdio_gpio_write(dev_cfg, prtad, 5);
	/* RA5: 5 bits register address */
	mdio_gpio_write(dev_cfg, devad, 5);

	if (rw) { /* Write data */
		/* TA: 2 bits turn-around */
		mdio_gpio_write(dev_cfg, 0x2, 2);
		mdio_gpio_write(dev_cfg, data_in, 16);
	} else { /* Read data */
		/* Release the MDIO line */
		mdio_gpio_dir(dev_cfg, MDIO_GPIO_READ_OP);
		mdio_gpio_read(dev_cfg, data_out);
	}

	/* DIR: input. Tristate MDIO line */
	mdio_gpio_dir(dev_cfg, MDIO_GPIO_READ_OP);

	k_sem_give(&dev_data->sem);

	return 0;
}

static int mdio_gpio_read_mmi(const struct device *dev, uint8_t prtad, uint8_t devad,
			      uint16_t *data)
{
	return mdio_gpio_transfer(dev, prtad, devad, MDIO_GPIO_READ_OP, 0, data);
}

static int mdio_gpio_write_mmi(const struct device *dev, uint8_t prtad, uint8_t devad,
			       uint16_t data)
{
	return mdio_gpio_transfer(dev, prtad, devad, MDIO_GPIO_WRITE_OP, data, NULL);
}

static int mdio_gpio_initialize(const struct device *dev)
{
	const struct mdio_gpio_config *const dev_cfg = dev->config;
	struct mdio_gpio_data *const dev_data = dev->data;
	int rc;

	k_sem_init(&dev_data->sem, 1, 1);

	if (!device_is_ready(dev_cfg->mdc_gpio.port)) {
		LOG_ERR("GPIO port for MDC pin is not ready");
		return -ENODEV;
	}

	if (!device_is_ready(dev_cfg->mdio_gpio.port)) {
		LOG_ERR("GPIO port for MDIO pin is not ready");
		return -ENODEV;
	}

	rc = gpio_pin_configure_dt(&dev_cfg->mdc_gpio, GPIO_OUTPUT_INACTIVE);
	if (rc < 0) {
		LOG_ERR("Couldn't configure MDC pin; (%d)", rc);
		return rc;
	}

	rc = gpio_pin_configure_dt(&dev_cfg->mdio_gpio, GPIO_INPUT);
	if (rc < 0) {
		LOG_ERR("Couldn't configure MDIO pin; (%d)", rc);
		return rc;
	}

	return 0;
}

static void mdio_gpio_bus_enable(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static void mdio_gpio_bus_disable(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static const struct mdio_driver_api mdio_gpio_driver_api = {
	.read = mdio_gpio_read_mmi,
	.write = mdio_gpio_write_mmi,
	.bus_enable = mdio_gpio_bus_enable,
	.bus_disable = mdio_gpio_bus_disable,
};

#define MDIO_GPIO_CONFIG(inst)                                                                     \
	static struct mdio_gpio_config mdio_gpio_dev_config_##inst = {                             \
		.mdc_gpio = GPIO_DT_SPEC_INST_GET(inst, mdc_gpios),                                \
		.mdio_gpio = GPIO_DT_SPEC_INST_GET(inst, mdio_gpios),                              \
	};

#define MDIO_GPIO_DEVICE(inst)                                                                     \
	MDIO_GPIO_CONFIG(inst);                                                                    \
	static struct mdio_gpio_data mdio_gpio_dev_data_##inst;                                    \
	DEVICE_DT_INST_DEFINE(inst, &mdio_gpio_initialize, NULL, &mdio_gpio_dev_data_##inst,       \
			      &mdio_gpio_dev_config_##inst, POST_KERNEL,                           \
			      CONFIG_MDIO_INIT_PRIORITY, &mdio_gpio_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MDIO_GPIO_DEVICE)
