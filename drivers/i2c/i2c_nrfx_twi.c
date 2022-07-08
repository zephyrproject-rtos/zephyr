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

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_nrfx_twi, CONFIG_I2C_LOG_LEVEL);

#define I2C_TRANSFER_TIMEOUT_MSEC		K_MSEC(500)

struct i2c_nrfx_twi_data {
	struct k_sem transfer_sync;
	struct k_sem completion_sync;
	volatile nrfx_err_t res;
	uint32_t dev_config;
};

struct i2c_nrfx_twi_config {
	nrfx_twi_t twi;
	nrfx_twi_config_t config;
#ifdef CONFIG_PINCTRL
	const struct pinctrl_dev_config *pcfg;
#endif
};

static int i2c_nrfx_twi_recover_bus(const struct device *dev);

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
		if (I2C_MSG_ADDR_10_BITS & msgs[i].flags) {
			ret = -ENOTSUP;
			break;
		}

		nrfx_twi_xfer_desc_t cur_xfer = {
			.p_primary_buf  = msgs[i].buf,
			.primary_length = msgs[i].len,
			.address	= addr,
			.type		= (msgs[i].flags & I2C_MSG_READ) ?
					  NRFX_TWI_XFER_RX : NRFX_TWI_XFER_TX
		};
		uint32_t xfer_flags = 0;
		nrfx_err_t res;

		/* In case the STOP condition is not supposed to appear after
		 * the current message, check what is requested further:
		 */
		if (!(msgs[i].flags & I2C_MSG_STOP)) {
			/* - if the transfer consists of more messages
			 *   and the I2C repeated START is not requested
			 *   to appear before the next message, suspend
			 *   the transfer after the current message,
			 *   so that it can be resumed with the next one,
			 *   resulting in the two messages merged into
			 *   a continuous transfer on the bus
			 */
			if ((i < (num_msgs - 1)) &&
			    !(msgs[i + 1].flags & I2C_MSG_RESTART)) {
				xfer_flags |= NRFX_TWI_FLAG_SUSPEND;
			/* - otherwise, just finish the transfer without
			 *   generating the STOP condition, unless the current
			 *   message is an RX request, for which such feature
			 *   is not supported
			 */
			} else if (msgs[i].flags & I2C_MSG_READ) {
				ret = -ENOTSUP;
				break;
			} else {
				xfer_flags |= NRFX_TWI_FLAG_TX_NO_STOP;
			}
		}

		res = nrfx_twi_xfer(&config->twi, &cur_xfer, xfer_flags);
		if (res != NRFX_SUCCESS) {
			if (res == NRFX_ERROR_BUSY) {
				ret = -EBUSY;
				break;
			} else {
				ret = -EIO;
				break;
			}
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
			LOG_ERR("Error on I2C line occurred for message %d", i);
			nrfx_twi_disable(&config->twi);
			(void)i2c_nrfx_twi_recover_bus(dev);
			ret = -EIO;
			break;
		}

		res = data->res;
		if (res != NRFX_SUCCESS) {
			LOG_ERR("Error 0x%08X occurred for message %d", res, i);
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
	struct i2c_nrfx_twi_data *dev_data = p_context;

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

static int i2c_nrfx_twi_configure(const struct device *dev,
				  uint32_t dev_config)
{
	const struct i2c_nrfx_twi_config *config = dev->config;
	struct i2c_nrfx_twi_data *data = dev->data;
	nrfx_twi_t const *inst = &config->twi;

	if (I2C_ADDR_10_BITS & dev_config) {
		return -EINVAL;
	}

	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		nrf_twi_frequency_set(inst->p_twi, NRF_TWI_FREQ_100K);
		break;
	case I2C_SPEED_FAST:
		nrf_twi_frequency_set(inst->p_twi, NRF_TWI_FREQ_400K);
		break;
	default:
		LOG_ERR("unsupported speed");
		return -EINVAL;
	}
	data->dev_config = dev_config;

	return 0;
}

static int i2c_nrfx_twi_recover_bus(const struct device *dev)
{
	const struct i2c_nrfx_twi_config *config = dev->config;
	uint32_t scl_pin;
	uint32_t sda_pin;
	nrfx_err_t err;

#ifdef CONFIG_PINCTRL
	scl_pin = nrf_twi_scl_pin_get(config->twi.p_twi);
	sda_pin = nrf_twi_sda_pin_get(config->twi.p_twi);
#else
	scl_pin = config->config.scl;
	sda_pin = config->config.sda;
#endif

	err = nrfx_twi_bus_recover(scl_pin, sda_pin);
	return (err == NRFX_SUCCESS ? 0 : -EBUSY);
}

static const struct i2c_driver_api i2c_nrfx_twi_driver_api = {
	.configure   = i2c_nrfx_twi_configure,
	.transfer    = i2c_nrfx_twi_transfer,
	.recover_bus = i2c_nrfx_twi_recover_bus,
};

static int init_twi(const struct device *dev)
{
	const struct i2c_nrfx_twi_config *config = dev->config;
	struct i2c_nrfx_twi_data *dev_data = dev->data;
	nrfx_err_t result = nrfx_twi_init(&config->twi, &config->config,
					  event_handler, dev_data);
	if (result != NRFX_SUCCESS) {
		LOG_ERR("Failed to initialize device: %s",
			    dev->name);
		return -EBUSY;
	}

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int twi_nrfx_pm_action(const struct device *dev,
			      enum pm_device_action action)
{
	const struct i2c_nrfx_twi_config *config = dev->config;
	struct i2c_nrfx_twi_data *data = dev->data;
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
#ifdef CONFIG_PINCTRL
		ret = pinctrl_apply_state(config->pcfg,
					  PINCTRL_STATE_DEFAULT);
		if (ret < 0) {
			return ret;
		}
#endif
		init_twi(dev);
		if (data->dev_config) {
			i2c_nrfx_twi_configure(dev, data->dev_config);
		}
		break;

	case PM_DEVICE_ACTION_SUSPEND:
		nrfx_twi_uninit(&config->twi);

#ifdef CONFIG_PINCTRL
		ret = pinctrl_apply_state(config->pcfg,
					  PINCTRL_STATE_SLEEP);
		if (ret < 0) {
			return ret;
		}
#endif
		break;

	default:
		ret = -ENOTSUP;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

#define I2C_NRFX_TWI_INVALID_FREQUENCY  ((nrf_twi_frequency_t)-1)
#define I2C_NRFX_TWI_FREQUENCY(bitrate)					       \
	 (bitrate == I2C_BITRATE_STANDARD ? NRF_TWI_FREQ_100K		       \
	: bitrate == 250000               ? NRF_TWI_FREQ_250K		       \
	: bitrate == I2C_BITRATE_FAST     ? NRF_TWI_FREQ_400K		       \
					  : I2C_NRFX_TWI_INVALID_FREQUENCY)
#define I2C(idx) DT_NODELABEL(i2c##idx)
#define I2C_FREQUENCY(idx)						       \
	I2C_NRFX_TWI_FREQUENCY(DT_PROP(I2C(idx), clock_frequency))

#define I2C_NRFX_TWI_PIN_CFG(idx)			\
	COND_CODE_1(CONFIG_PINCTRL,			\
		(.skip_gpio_cfg = true,			\
		 .skip_psel_cfg = true,),		\
		(.scl = DT_PROP(I2C(idx), scl_pin),	\
		 .sda = DT_PROP(I2C(idx), sda_pin),))

#define I2C_NRFX_TWI_DEVICE(idx)					       \
	NRF_DT_CHECK_PIN_ASSIGNMENTS(I2C(idx), 1, scl_pin, sda_pin);	       \
	BUILD_ASSERT(I2C_FREQUENCY(idx)	!=				       \
		     I2C_NRFX_TWI_INVALID_FREQUENCY,			       \
		     "Wrong I2C " #idx " frequency setting in dts");	       \
	static int twi_##idx##_init(const struct device *dev)		       \
	{								       \
		IRQ_CONNECT(DT_IRQN(I2C(idx)), DT_IRQ(I2C(idx), priority),     \
			    nrfx_isr, nrfx_twi_##idx##_irq_handler, 0);	       \
		IF_ENABLED(CONFIG_PINCTRL, (				       \
			const struct i2c_nrfx_twi_config *config = dev->config;\
			int err = pinctrl_apply_state(config->pcfg,	       \
						      PINCTRL_STATE_DEFAULT);  \
			if (err < 0) {					       \
				return err;				       \
			}						       \
		))							       \
		return init_twi(dev);					       \
	}								       \
	static struct i2c_nrfx_twi_data twi_##idx##_data = {		       \
		.transfer_sync = Z_SEM_INITIALIZER(                            \
			twi_##idx##_data.transfer_sync, 1, 1),                 \
		.completion_sync = Z_SEM_INITIALIZER(                          \
			twi_##idx##_data.completion_sync, 0, 1)		       \
	};								       \
	IF_ENABLED(CONFIG_PINCTRL, (PINCTRL_DT_DEFINE(I2C(idx))));	       \
	static const struct i2c_nrfx_twi_config twi_##idx##z_config = {	       \
		.twi = NRFX_TWI_INSTANCE(idx),				       \
		.config = {						       \
			I2C_NRFX_TWI_PIN_CFG(idx)			       \
			.frequency = I2C_FREQUENCY(idx),		       \
		},							       \
		IF_ENABLED(CONFIG_PINCTRL,				       \
			(.pcfg = PINCTRL_DT_DEV_CONFIG_GET(I2C(idx)),))	       \
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

#ifdef CONFIG_I2C_0_NRF_TWI
I2C_NRFX_TWI_DEVICE(0);
#endif

#ifdef CONFIG_I2C_1_NRF_TWI
I2C_NRFX_TWI_DEVICE(1);
#endif
