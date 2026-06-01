/*
 * Copyright (c) 2023 Aleksandr Senin
 * Copyright (c) 2026 Bas van Loon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_smi_gpio

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/smi.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(smi_gpio, CONFIG_SMI_LOG_LEVEL);

#define SMI_GPIO_READ_OP  0
#define SMI_GPIO_WRITE_OP 1
#define SMI_GPIO_MSB      0x80000000

struct smi_gpio_data {
	struct k_sem sem;
};

struct smi_gpio_config {
	struct gpio_dt_spec smi_clock_gpio;
	struct gpio_dt_spec smi_data_gpio;
};

static ALWAYS_INLINE void smi_gpio_clock_the_bit_read(const struct smi_gpio_config *dev_cfg)
{
	gpio_pin_set_dt(&dev_cfg->smi_clock_gpio, 1);
	k_busy_wait(1);
	gpio_pin_set_dt(&dev_cfg->smi_clock_gpio, 0);
	k_busy_wait(1);
}

static ALWAYS_INLINE void smi_gpio_clock_the_bit_write(const struct smi_gpio_config *dev_cfg)
{
	k_busy_wait(1);
	gpio_pin_set_dt(&dev_cfg->smi_clock_gpio, 1);
	k_busy_wait(1);
	gpio_pin_set_dt(&dev_cfg->smi_clock_gpio, 0);
}

static ALWAYS_INLINE void smi_gpio_dir(const struct smi_gpio_config *dev_cfg, uint8_t dir)
{
	gpio_pin_configure_dt(&dev_cfg->smi_data_gpio,
			      dir == SMI_GPIO_WRITE_OP ? GPIO_OUTPUT_ACTIVE : GPIO_INPUT);
	if (dir == SMI_GPIO_READ_OP) {
		smi_gpio_clock_the_bit_read(dev_cfg);
	}
}

static ALWAYS_INLINE void smi_gpio_read(const struct smi_gpio_config *dev_cfg, uint16_t *pdata,
					uint8_t len)
{
	uint16_t data = 0;

	for (uint16_t i = 0; i < len; i++) {
		data <<= 1;
		smi_gpio_clock_the_bit_read(dev_cfg);
		if (gpio_pin_get_dt(&dev_cfg->smi_data_gpio) == 1) {
			data |= 1;
		}
	}

	*pdata = data;
}

static ALWAYS_INLINE void smi_gpio_write(const struct smi_gpio_config *dev_cfg, uint32_t data,
					 uint8_t len)
{
	uint32_t v_data = data;
	uint32_t v_len = len;

	v_data <<= 32 - v_len;
	for (; v_len > 0; v_len--) {
		gpio_pin_set_dt(&dev_cfg->smi_data_gpio, (v_data & SMI_GPIO_MSB) ? 1 : 0);
		smi_gpio_clock_the_bit_write(dev_cfg);
		v_data <<= 1;
	}
}

static int smi_gpio_transfer(const struct device *dev, uint8_t prtad, uint8_t devad, uint8_t rw,
			     uint16_t data_in, uint16_t *data_out)
{
	const struct smi_gpio_config *const dev_cfg = dev->config;
	struct smi_gpio_data *const dev_data = dev->data;

	k_sem_take(&dev_data->sem, K_FOREVER);

	/* DIR: output */
	smi_gpio_dir(dev_cfg, SMI_GPIO_WRITE_OP);
	/* PRE32: 32 bits '1' for sync*/
	smi_gpio_write(dev_cfg, 0xFFFFFFFF, 32);
	/* ST: 2 bits start of frame */
	smi_gpio_write(dev_cfg, 0x1, 2);
	/* OP: 2 bits opcode, fixed 0 */
	smi_gpio_write(dev_cfg, 0, 2);

	/* PA5: 5 bits PHY address. Bit 4 set for read, clear for write */
	if (rw) {
		smi_gpio_write(dev_cfg, prtad, 5);
	} else {
		smi_gpio_write(dev_cfg, prtad | BIT(4), 5);
	}

	/* RA5: 5 bits register address */
	smi_gpio_write(dev_cfg, devad, 5);

	if (rw) { /* Write data */
		/* TA: 2 bits turn-around */
		smi_gpio_write(dev_cfg, 0x2, 2);
		smi_gpio_write(dev_cfg, data_in, 16);
	} else { /* Read data */
		/* Release the SMI line */
		smi_gpio_dir(dev_cfg, SMI_GPIO_READ_OP);
		smi_gpio_read(dev_cfg, data_out, 16);
	}

	/* DIR: input. Tristate SMI line */
	smi_gpio_dir(dev_cfg, SMI_GPIO_READ_OP);

	k_sem_give(&dev_data->sem);

	return 0;
}

static int smi_gpio_bus_enable(const struct device *dev)
{
	const struct smi_gpio_config *const dev_cfg = dev->config;
	int rc;

	rc = gpio_pin_configure_dt(&dev_cfg->smi_clock_gpio, GPIO_OUTPUT_INACTIVE);
	if (rc < 0) {
		LOG_ERR("Couldn't configure SMI clock pin; (%d)", rc);
		return rc;
	}

	rc = gpio_pin_configure_dt(&dev_cfg->smi_data_gpio, GPIO_INPUT);
	if (rc < 0) {
		LOG_ERR("Couldn't configure SMI data pin; (%d)", rc);
		return rc;
	}

	return 0;
}

static int smi_gpio_bus_disable(const struct device *dev)
{
	const struct smi_gpio_config *const dev_cfg = dev->config;
	int rc;

	rc = gpio_pin_configure_dt(&dev_cfg->smi_clock_gpio, GPIO_INPUT);
	if (rc < 0) {
		LOG_ERR("Couldn't configure SMI clock pin; (%d)", rc);
		return rc;
	}

	rc = gpio_pin_configure_dt(&dev_cfg->smi_data_gpio, GPIO_INPUT);
	if (rc < 0) {
		LOG_ERR("Couldn't configure SMI data pin; (%d)", rc);
		return rc;
	}

	return 0;
}

static int smi_gpio_read_mmi(const struct device *dev, uint8_t prtad, uint8_t devad, uint16_t *data)
{
	return smi_gpio_transfer(dev, prtad, devad, SMI_GPIO_READ_OP, 0, data);
}

static int smi_gpio_write_mmi(const struct device *dev, uint8_t prtad, uint8_t devad, uint16_t data)
{
	return smi_gpio_transfer(dev, prtad, devad, SMI_GPIO_WRITE_OP, data, NULL);
}

static int smi_gpio_initialize(const struct device *dev)
{
	const struct smi_gpio_config *const dev_cfg = dev->config;
	struct smi_gpio_data *const dev_data = dev->data;
	int rc;

	k_sem_init(&dev_data->sem, 1, 1);

	if (!gpio_is_ready_dt(&dev_cfg->smi_clock_gpio)) {
		LOG_ERR("GPIO port for SMI clock pin is not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&dev_cfg->smi_data_gpio)) {
		LOG_ERR("GPIO port for SMI data pin is not ready");
		return -ENODEV;
	}

	rc = smi_gpio_bus_enable(dev);
	if (rc < 0) {
		LOG_ERR("Failed to enable SMI bus (%d)", rc);
		return rc;
	}

	return 0;
}

static DEVICE_API(smi, smi_gpio_driver_api) = {
	.bus_disable = smi_gpio_bus_disable,
	.bus_enable = smi_gpio_bus_enable,
	.read = smi_gpio_read_mmi,
	.write = smi_gpio_write_mmi,
};

#define SMI_GPIO_CONFIG(inst)                                                                      \
	static struct smi_gpio_config smi_gpio_dev_config_##inst = {                               \
		.smi_clock_gpio = GPIO_DT_SPEC_INST_GET(inst, smi_clock_gpios),                    \
		.smi_data_gpio = GPIO_DT_SPEC_INST_GET(inst, smi_data_gpios),                      \
	};

#define SMI_GPIO_DEVICE(inst)                                                                      \
	SMI_GPIO_CONFIG(inst);                                                                     \
	static struct smi_gpio_data smi_gpio_dev_data_##inst;                                      \
	DEVICE_DT_INST_DEFINE(inst, &smi_gpio_initialize, NULL, &smi_gpio_dev_data_##inst,         \
			      &smi_gpio_dev_config_##inst, POST_KERNEL, CONFIG_SMI_INIT_PRIORITY,  \
			      &smi_gpio_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SMI_GPIO_DEVICE)
