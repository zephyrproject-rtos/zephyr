/*
 * Copyright (c) 2026, tinyvision.ai
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_sc18is606

#include <zephyr/logging/log.h>
#include "mfd_sc18is606.h"
LOG_MODULE_REGISTER(nxp_sc18is606, CONFIG_MFD_LOG_LEVEL);

int nxp_sc18is606_transfer(const struct device *dev, const uint8_t *tx_data, uint8_t tx_len,
			   uint8_t *rx_data, uint8_t rx_len, uint8_t *id_buf)
{
	struct sc18is606_data *data = dev->data;
	const struct sc18is606_config *info = dev->config;
	int ret;

	ret = k_mutex_lock(&data->bridge_lock, K_FOREVER);
	if (ret < 0) {
		return ret;
	}

	if (tx_data != NULL) {
		if (id_buf != NULL) {
			struct i2c_msg tx_msg[2] = {
				{
					.buf = id_buf,
					.len = 1,
					.flags = I2C_MSG_WRITE,
				},
				{
					.buf = (uint8_t *)tx_data,
					.len = tx_len,
					.flags = I2C_MSG_WRITE,
				},
			};

			ret = i2c_transfer_dt(&info->i2c_controller, tx_msg, 2);
		} else {
			struct i2c_msg tx_msg[1] = {{
				.buf = (uint8_t *)tx_data,
				.len = tx_len,
				.flags = I2C_MSG_WRITE,
			}};

			ret = i2c_transfer_dt(&info->i2c_controller, tx_msg, 1);
		}

		if (ret != 0) {
			LOG_ERR("SPI write failed: %d", ret);
			goto out;
		}
	}

	/*If interrupt pin is used wait before next transaction*/
	if (info->int_gpios.port) {
		ret = k_sem_take(&data->int_sem, K_MSEC(5));
		if (ret != 0) {
			LOG_WRN("Interrupt semaphore timedout, proceeding with read");
		}
	}

	if (rx_data != NULL) {
		/*What is the time*/
		k_timepoint_t end;

		/*Set a deadline in a second*/
		end = sys_timepoint_calc(K_MSEC(1));

		do {
			ret = i2c_read(info->i2c_controller.bus, rx_data, rx_len,
				       info->i2c_controller.addr);
			if (ret >= 0) {
				break;
			}
		} while (!sys_timepoint_expired(end)); /*Keep reading while in the deadline*/

		if (ret < 0) {
			LOG_ERR("Failed to read data (%d)", ret);
			goto out;
		}
	}

	ret = 0;

out:
	k_mutex_unlock(&data->bridge_lock);
	return ret;
}

static void sc18is606_int_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct sc18is606_data *data = CONTAINER_OF(cb, struct sc18is606_data, int_cb);

	k_sem_give(&data->int_sem);
}

static int int_gpios_setup(const struct device *dev)
{
	struct sc18is606_data *data = dev->data;
	const struct sc18is606_config *cfg = dev->config;
	int ret;

	if (!gpio_is_ready_dt(&cfg->int_gpios)) {
		LOG_ERR("SC18IS606 Int GPIO not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&cfg->int_gpios, GPIO_INPUT);
	if (ret != 0U) {
		LOG_ERR("Failed to configure SC18IS606 int gpio (%d)", ret);
		return ret;
	}

	ret = k_sem_init(&data->int_sem, 0, 1);
	if (ret != 0U) {
		LOG_ERR("Failed to Initialize Interrupt Semaphore (%d)", ret);
		return ret;
	}

	gpio_init_callback(&data->int_cb, sc18is606_int_isr, BIT(cfg->int_gpios.pin));

	ret = gpio_add_callback(cfg->int_gpios.port, &data->int_cb);
	if (ret != 0U) {
		LOG_ERR("Failed to assign the Interrupt callback (%d)", ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->int_gpios, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0U) {
		LOG_ERR("Failed to configure the GPIO interrupt edge (%d)", ret);
		return ret;
	}

	return ret;
}

static int sc18is606_init(const struct device *dev)
{
	const struct sc18is606_config *cfg = dev->config;
	int ret;

	if (!device_is_ready(cfg->i2c_controller.bus)) {
		LOG_ERR("I2C controller %s not found", cfg->i2c_controller.bus->name);
		return -ENODEV;
	}

	LOG_DBG("Using I2C controller: %s", cfg->i2c_controller.bus->name);

	if (cfg->reset_gpios.port) {
		if (!gpio_is_ready_dt(&cfg->reset_gpios)) {
			LOG_ERR("SC18IS606 Reset GPIO not ready");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&cfg->reset_gpios, GPIO_OUTPUT_ACTIVE);
		if (ret != 0U) {
			LOG_ERR("Failed to configure SC18IS606 reset GPIO (%d)", ret);
			return ret;
		}

		ret = gpio_pin_set_dt(&cfg->reset_gpios, 0);
		if (ret != 0U) {
			LOG_ERR("Failed to reset Bridge via Reset pin (%d)", ret);
			return ret;
		}
	}

	if (cfg->int_gpios.port) {
		ret = int_gpios_setup(dev);
		if (ret != 0U) {
			LOG_ERR("Could not set up device int_gpios (%d)", ret);
			return ret;
		}
	}
	LOG_DBG("SC18IS606 initialized");
	return 0;
}

#define MFD_SC18IS606_DEFINE(inst)                                                                 \
	static const struct sc18is606_config sc18is606_config_##inst = {                           \
		.i2c_controller = I2C_DT_SPEC_GET(DT_DRV_INST(inst)),                              \
		.reset_gpios = GPIO_DT_SPEC_GET_OR(DT_DRV_INST(inst), reset_gpios, {0}),           \
		.int_gpios = GPIO_DT_SPEC_GET_OR(DT_DRV_INST(inst), int_gpios, {0}),               \
	};                                                                                         \
                                                                                                   \
	static struct sc18is606_data data##inst;                                                   \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, sc18is606_init, NULL, &data##inst, &sc18is606_config_##inst,   \
			      POST_KERNEL, CONFIG_MFD_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MFD_SC18IS606_DEFINE)
