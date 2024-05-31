/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/drivers/i2c.h>
#include <zephyr/dt-bindings/i2c/i2c.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <soc.h>
#include <nrfx_twi.h>
#include "i2c_nrfx_twi_common.h"

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(i2c_nrfx_twi, CONFIG_I2C_LOG_LEVEL);

#if CONFIG_I2C_NRFX_TRANSFER_TIMEOUT
#define I2C_TRANSFER_TIMEOUT_MSEC K_MSEC(CONFIG_I2C_NRFX_TRANSFER_TIMEOUT)
#else
#define I2C_TRANSFER_TIMEOUT_MSEC K_FOREVER
#endif

struct i2c_nrfx_twi_data {
	uint32_t dev_config;
	struct k_sem transfer_sync;
	struct k_sem completion_sync;
	volatile nrfx_err_t res;
};

/* Enforce dev_config matches the same offset as the common structure,
 * otherwise common API won't be compatible with i2c_nrfx_twi.
 */
BUILD_ASSERT(
	offsetof(struct i2c_nrfx_twi_data, dev_config) ==
	offsetof(struct i2c_nrfx_twi_common_data, dev_config)
);

static int i2c_nrfx_twi_transfer(const struct device *dev,
				 struct i2c_msg *msgs,
				 uint8_t num_msgs, uint16_t addr)
{
	const struct i2c_nrfx_twi_config *config = dev->config;
	struct i2c_nrfx_twi_data *data = dev->data;
	int ret = 0;

	k_sem_take(&data->transfer_sync, K_FOREVER);

	/* Dummy take on completion_sync sem to be sure that it is empty */
	k_sem_take(&data->completion_sync, K_NO_WAIT);

	nrfx_twi_enable(&config->twi);

	for (size_t i = 0; i < num_msgs; i++) {
		bool more_msgs = ((i < (num_msgs - 1)) &&
				  !(msgs[i + 1].flags & I2C_MSG_RESTART));

		ret = i2c_nrfx_twi_msg_transfer(dev, msgs[i].flags,
						msgs[i].buf,
						msgs[i].len, addr,
						more_msgs);
		if (ret) {
			break;
		}

		ret = k_sem_take(&data->completion_sync,
				 I2C_TRANSFER_TIMEOUT_MSEC);
		if (ret != 0) {
			/* Whatever the frequency, completion_sync should have
			 * been given by the event handler.
			 *
			 * If it hasn't, it's probably due to an hardware issue
			 * on the I2C line, for example a short between SDA and
			 * GND.
			 * This is issue has also been when trying to use the
			 * I2C bus during MCU internal flash erase.
			 *
			 * In many situation, a retry is sufficient.
			 * However, some time the I2C device get stuck and need
			 * help to recover.
			 * Therefore we always call i2c_nrfx_twi_recover_bus()
			 * to make sure everything has been done to restore the
			 * bus from this error.
			 */
			nrfx_twi_disable(&config->twi);
			(void)i2c_nrfx_twi_recover_bus(dev);
			ret = -EIO;
			break;
		}

		if (data->res != NRFX_SUCCESS) {
			ret = -EIO;
			break;
		}
	}

	nrfx_twi_disable(&config->twi);
	k_sem_give(&data->transfer_sync);

	return ret;
}

static void event_handler(nrfx_twi_evt_t const *p_event, void *p_context)
{
	const struct device *dev = p_context;
	struct i2c_nrfx_twi_data *dev_data = (struct i2c_nrfx_twi_data *)dev->data;

	switch (p_event->type) {
	case NRFX_TWI_EVT_DONE:
		dev_data->res = NRFX_SUCCESS;
		break;
	case NRFX_TWI_EVT_ADDRESS_NACK:
		dev_data->res = NRFX_ERROR_DRV_TWI_ERR_ANACK;
		break;
	case NRFX_TWI_EVT_DATA_NACK:
		dev_data->res = NRFX_ERROR_DRV_TWI_ERR_DNACK;
		break;
	default:
		dev_data->res = NRFX_ERROR_INTERNAL;
		break;
	}

	k_sem_give(&dev_data->completion_sync);
}

static const struct i2c_driver_api i2c_nrfx_twi_driver_api = {
	.configure   = i2c_nrfx_twi_configure,
	.transfer    = i2c_nrfx_twi_transfer,
	.recover_bus = i2c_nrfx_twi_recover_bus,
};

#define I2C_NRFX_TWI_DEVICE(idx)					       \
	NRF_DT_CHECK_NODE_HAS_PINCTRL_SLEEP(I2C(idx));			       \
	BUILD_ASSERT(I2C_FREQUENCY(idx)	!=				       \
		     I2C_NRFX_TWI_INVALID_FREQUENCY,			       \
		     "Wrong I2C " #idx " frequency setting in dts");	       \
	static int twi_##idx##_init(const struct device *dev)		       \
	{								       \
		IRQ_CONNECT(DT_IRQN(I2C(idx)), DT_IRQ(I2C(idx), priority),     \
			    nrfx_isr, nrfx_twi_##idx##_irq_handler, 0);	       \
		const struct i2c_nrfx_twi_config *config = dev->config;	       \
		int err = pinctrl_apply_state(config->pcfg,		       \
					      PINCTRL_STATE_DEFAULT);	       \
		if (err < 0) {						       \
			return err;					       \
		}							       \
		return i2c_nrfx_twi_init(dev);				       \
	}								       \
	static struct i2c_nrfx_twi_data twi_##idx##_data = {		       \
		.transfer_sync = Z_SEM_INITIALIZER(                            \
			twi_##idx##_data.transfer_sync, 1, 1),                 \
		.completion_sync = Z_SEM_INITIALIZER(                          \
			twi_##idx##_data.completion_sync, 0, 1)		       \
	};								       \
	PINCTRL_DT_DEFINE(I2C(idx));					       \
	static const struct i2c_nrfx_twi_config twi_##idx##z_config = {	       \
		.twi = NRFX_TWI_INSTANCE(idx),				       \
		.config = {						       \
			.skip_gpio_cfg = true,				       \
			.skip_psel_cfg = true,				       \
			.frequency = I2C_FREQUENCY(idx),		       \
		},							       \
		.event_handler = event_handler,				       \
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(I2C(idx)),		       \
	};								       \
	PM_DEVICE_DT_DEFINE(I2C(idx), twi_nrfx_pm_action);		       \
	I2C_DEVICE_DT_DEFINE(I2C(idx),					       \
		      twi_##idx##_init,					       \
		      PM_DEVICE_DT_GET(I2C(idx)),			       \
		      &twi_##idx##_data,				       \
		      &twi_##idx##z_config,				       \
		      POST_KERNEL,					       \
		      CONFIG_I2C_INIT_PRIORITY,				       \
		      &i2c_nrfx_twi_driver_api)

#ifdef CONFIG_HAS_HW_NRF_TWI0
I2C_NRFX_TWI_DEVICE(0);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWI1
I2C_NRFX_TWI_DEVICE(1);
#endif
