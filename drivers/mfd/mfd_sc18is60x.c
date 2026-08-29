/*
 * SPDX-FileCopyrightText: Copyright tinyvision.ai
 * SPDX-FileCopyrightText: Copyright Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/mfd/mfd_sc18is60x.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(nxp_sc18is60x, CONFIG_MFD_LOG_LEVEL);

struct sc18is60x_data {
	struct k_mutex lock;
	struct gpio_callback int_cb;
	struct k_sem int_sem;
	uint8_t spi_cfg;
	bool spi_cfg_valid;
};

struct sc18is60x_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec reset_gpios;
	struct gpio_dt_spec int_gpios;
};

#define SC18IS60X_CMD_CONFIG_SPI 0xF0

/* Datasheet: device NACKs its address until the previous command finishes. */
#define SC18IS60X_READY_TIMEOUT_MS 2
#define SC18IS60X_BUSY_WAIT_MS 10

static int sc18is60x_wait_ready(const struct device *dev)
{
	struct sc18is60x_data *data = dev->data;
	const struct sc18is60x_config *cfg = dev->config;

	if (cfg->int_gpios.port != NULL) {
		if (k_sem_take(&data->int_sem, K_MSEC(SC18IS60X_BUSY_WAIT_MS)) != 0) {
			LOG_ERR("INT wait timed out");
			return -EIO;
		}

	} else {
		k_msleep(SC18IS60X_READY_TIMEOUT_MS);
	}

	return 0;
}

static int sc18is60x_bus_write(const struct sc18is60x_config *cfg, const uint8_t *tx_data,
			       uint16_t tx_len)
{
	int ret = i2c_write_dt(&cfg->i2c, tx_data, tx_len);

	if (ret != 0) {
		LOG_ERR("I2C write failed: %d (addr=0x%x)", ret, cfg->i2c.addr);
	}

	return ret;
}

static int sc18is60x_bus_read(const struct sc18is60x_config *cfg, uint8_t *rx_data, uint16_t rx_len)
{
	int ret = i2c_read_dt(&cfg->i2c, rx_data, rx_len);

	if (ret != 0) {
		LOG_ERR("I2C read failed: %d (addr=0x%x)", ret, cfg->i2c.addr);
	}

	return ret;
}

int nxp_sc18is60x_lock(const struct device *dev)
{
	struct sc18is60x_data *data = dev->data;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	return k_mutex_lock(&data->lock, K_FOREVER);
}

void nxp_sc18is60x_unlock(const struct device *dev)
{
	struct sc18is60x_data *data = dev->data;

	k_mutex_unlock(&data->lock);
}

int nxp_sc18is60x_transfer_unlocked(const struct device *dev, const uint8_t *tx_data,
				    uint16_t tx_len, uint8_t *rx_data, uint16_t rx_len)
{
	const struct sc18is60x_config *cfg = dev->config;
	struct sc18is60x_data *data = dev->data;
	int ret;

	if (tx_data != NULL && tx_len > 0U) {
		if (cfg->int_gpios.port != NULL) {
			k_sem_reset(&data->int_sem);
		}

		ret = sc18is60x_bus_write(cfg, tx_data, tx_len);
		if (ret != 0) {
			return ret;
		}

		ret = sc18is60x_wait_ready(dev);
		if (ret != 0) {
			return ret;
		}
	}

	if (rx_data != NULL && rx_len > 0U) {
		return sc18is60x_bus_read(cfg, rx_data, rx_len);
	}

	return 0;
}

int nxp_sc18is60x_transfer(const struct device *dev, const uint8_t *tx_data, uint16_t tx_len,
			   uint8_t *rx_data, uint16_t rx_len)
{
	int ret;

	ret = nxp_sc18is60x_lock(dev);
	if (ret != 0) {
		return ret;
	}

	ret = nxp_sc18is60x_transfer_unlocked(dev, tx_data, tx_len, rx_data, rx_len);
	nxp_sc18is60x_unlock(dev);

	return ret;
}

int nxp_sc18is60x_configure_spi(const struct device *dev, uint8_t cfg_byte)
{
	struct sc18is60x_data *data = dev->data;
	uint8_t buf[] = {
		SC18IS60X_CMD_CONFIG_SPI,
		cfg_byte,
	};
	int ret;

	if (data->spi_cfg_valid && data->spi_cfg == cfg_byte) {
		return 0;
	}

	ret = nxp_sc18is60x_transfer_unlocked(dev, buf, sizeof(buf), NULL, 0);
	if (ret != 0) {
		return ret;
	}

	data->spi_cfg = cfg_byte;
	data->spi_cfg_valid = true;

	return 0;
}

static void sc18is60x_int_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct sc18is60x_data *data = CONTAINER_OF(cb, struct sc18is60x_data, int_cb);

	ARG_UNUSED(dev);
	ARG_UNUSED(pins);

	k_sem_give(&data->int_sem);
}

static int int_gpios_setup(const struct device *dev)
{
	struct sc18is60x_data *data = dev->data;
	const struct sc18is60x_config *cfg = dev->config;
	int ret;

	if (!gpio_is_ready_dt(&cfg->int_gpios)) {
		LOG_ERR("INT GPIO not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&cfg->int_gpios, GPIO_INPUT);
	if (ret != 0) {
		LOG_ERR("Failed to configure INT GPIO (%d)", ret);
		return ret;
	}

	ret = k_sem_init(&data->int_sem, 0, 1);
	if (ret != 0) {
		return ret;
	}

	gpio_init_callback(&data->int_cb, sc18is60x_int_isr, BIT(cfg->int_gpios.pin));

	ret = gpio_add_callback(cfg->int_gpios.port, &data->int_cb);
	if (ret != 0) {
		LOG_ERR("Failed to add INT callback (%d)", ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->int_gpios, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		LOG_ERR("Failed to configure INT edge (%d)", ret);
		return ret;
	}

	return 0;
}

static int sc18is60x_init(const struct device *dev)
{
	const struct sc18is60x_config *cfg = dev->config;
	struct sc18is60x_data *data = dev->data;
	int ret;

	k_mutex_init(&data->lock);

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("I2C controller not ready");
		return -ENODEV;
	}

	if (cfg->reset_gpios.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->reset_gpios)) {
			LOG_ERR("Reset GPIO not ready");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&cfg->reset_gpios, GPIO_OUTPUT_ACTIVE);
		if (ret != 0) {
			LOG_ERR("Failed to configure reset GPIO (%d)", ret);
			return ret;
		}

		k_msleep(SC18IS60X_BUSY_WAIT_MS);
		ret = gpio_pin_set_dt(&cfg->reset_gpios, 0);
		if (ret != 0) {
			LOG_ERR("Failed to release reset (%d)", ret);
			return ret;
		}
		k_msleep(SC18IS60X_BUSY_WAIT_MS);
	}

	if (cfg->int_gpios.port != NULL) {
		ret = int_gpios_setup(dev);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

#define MFD_SC18IS60X_DEFINE(inst, prefix)                                                         \
	static const struct sc18is60x_config prefix##_config_##inst = {                            \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.reset_gpios = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),                   \
		.int_gpios = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),                       \
	};                                                                                         \
	static struct sc18is60x_data prefix##_data_##inst;                                         \
	DEVICE_DT_INST_DEFINE(inst, sc18is60x_init, NULL, &prefix##_data_##inst,                   \
			      &prefix##_config_##inst, POST_KERNEL, CONFIG_MFD_INIT_PRIORITY,      \
			      NULL);

#define DT_DRV_COMPAT nxp_sc18is602
DT_INST_FOREACH_STATUS_OKAY_VARGS(MFD_SC18IS60X_DEFINE, sc18is602)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT nxp_sc18is606
DT_INST_FOREACH_STATUS_OKAY_VARGS(MFD_SC18IS60X_DEFINE, sc18is606)
