/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <drivers/i2c.h>
#include <dt-bindings/i2c/i2c.h>
#include <nrfx_twi.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(i2c_nrfx_twi, CONFIG_I2C_LOG_LEVEL);

#define I2C_TRANSFER_TIMEOUT_MSEC		K_MSEC(500)

struct i2c_nrfx_twi_data {
	struct k_sem transfer_sync;
	struct k_sem completion_sync;
	volatile nrfx_err_t res;
	uint32_t dev_config;
#ifdef CONFIG_PM_DEVICE
	uint32_t pm_state;
#endif
};

struct i2c_nrfx_twi_config {
	nrfx_twi_t twi;
	nrfx_twi_config_t config;
};

static inline struct i2c_nrfx_twi_data *get_dev_data(const struct device *dev)
{
	return dev->data;
}

static inline
const struct i2c_nrfx_twi_config *get_dev_config(const struct device *dev)
{
	return dev->config;
}

static int i2c_nrfx_twi_transfer(const struct device *dev,
				 struct i2c_msg *msgs,
				 uint8_t num_msgs, uint16_t addr)
{
	int ret = 0;

	k_sem_take(&(get_dev_data(dev)->transfer_sync), K_FOREVER);

	/* Dummy take on completion_sync sem to be sure that it is empty */
	k_sem_take(&(get_dev_data(dev)->completion_sync), K_NO_WAIT);

	nrfx_twi_enable(&get_dev_config(dev)->twi);

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

		res = nrfx_twi_xfer(&get_dev_config(dev)->twi,
				    &cur_xfer,
				    xfer_flags);
		if (res != NRFX_SUCCESS) {
			if (res == NRFX_ERROR_BUSY) {
				ret = -EBUSY;
				break;
			} else {
				ret = -EIO;
				break;
			}
		}

		ret = k_sem_take(&(get_dev_data(dev)->completion_sync),
				 I2C_TRANSFER_TIMEOUT_MSEC);
		if (ret != 0) {
			/* Whatever the frequency, completion_sync should have
			 * been give by the event handler.
			 *
			 * If it hasn't it's probably due to an hardware issue
			 * on the I2C line, for example a short between SDA and
			 * GND.
			 *
			 * Note to fully recover from this issue one should
			 * reinit nrfx twi.
			 */
			LOG_ERR("Error on I2C line occurred for message %d", i);
			ret = -EIO;
			break;
		}

		res = get_dev_data(dev)->res;
		if (res != NRFX_SUCCESS) {
			LOG_ERR("Error %d occurred for message %d", res, i);
			ret = -EIO;
			break;
		}
	}

	nrfx_twi_disable(&get_dev_config(dev)->twi);
	k_sem_give(&(get_dev_data(dev)->transfer_sync));

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
	nrfx_twi_t const *inst = &(get_dev_config(dev)->twi);

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
	get_dev_data(dev)->dev_config = dev_config;

	return 0;
}

static const struct i2c_driver_api i2c_nrfx_twi_driver_api = {
	.configure = i2c_nrfx_twi_configure,
	.transfer  = i2c_nrfx_twi_transfer,
};

static int init_twi(const struct device *dev)
{
	struct i2c_nrfx_twi_data *dev_data = get_dev_data(dev);
	nrfx_err_t result = nrfx_twi_init(&get_dev_config(dev)->twi,
					  &get_dev_config(dev)->config,
					  event_handler, dev_data);
	if (result != NRFX_SUCCESS) {
		LOG_ERR("Failed to initialize device: %s",
			    dev->name);
		return -EBUSY;
	}
#ifdef CONFIG_PM_DEVICE
	get_dev_data(dev)->pm_state = DEVICE_PM_ACTIVE_STATE;
#endif

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int twi_nrfx_pm_control(const struct device *dev,
				uint32_t ctrl_command,
				void *context, device_pm_cb cb, void *arg)
{
	int ret = 0;
	uint32_t pm_current_state = get_dev_data(dev)->pm_state;

	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
		uint32_t new_state = *((const uint32_t *)context);

		if (new_state != pm_current_state) {
			switch (new_state) {
			case DEVICE_PM_ACTIVE_STATE:
				init_twi(dev);
				if (get_dev_data(dev)->dev_config) {
					i2c_nrfx_twi_configure(
						dev,
						get_dev_data(dev)->dev_config);
				}
				break;

			case DEVICE_PM_LOW_POWER_STATE:
			case DEVICE_PM_SUSPEND_STATE:
			case DEVICE_PM_OFF_STATE:
				if (pm_current_state == DEVICE_PM_ACTIVE_STATE) {
					nrfx_twi_uninit(&get_dev_config(dev)->twi);
				}
				break;

			default:
				ret = -ENOTSUP;
			}
			if (!ret) {
				get_dev_data(dev)->pm_state = new_state;
			}
		}
	} else {
		__ASSERT_NO_MSG(ctrl_command == DEVICE_PM_GET_POWER_STATE);
		*((uint32_t *)context) = get_dev_data(dev)->pm_state;
	}

	if (cb) {
		cb(dev, ret, context, arg);
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

#define I2C_NRFX_TWI_DEVICE(idx)					       \
	BUILD_ASSERT(I2C_FREQUENCY(idx)	!=				       \
		     I2C_NRFX_TWI_INVALID_FREQUENCY,			       \
		     "Wrong I2C " #idx " frequency setting in dts");	       \
	static int twi_##idx##_init(const struct device *dev)		\
	{								       \
		IRQ_CONNECT(DT_IRQN(I2C(idx)), DT_IRQ(I2C(idx), priority),     \
			    nrfx_isr, nrfx_twi_##idx##_irq_handler, 0);	       \
		return init_twi(dev);					       \
	}								       \
	static struct i2c_nrfx_twi_data twi_##idx##_data = {		       \
		.transfer_sync = Z_SEM_INITIALIZER(                            \
			twi_##idx##_data.transfer_sync, 1, 1),                 \
		.completion_sync = Z_SEM_INITIALIZER(                          \
			twi_##idx##_data.completion_sync, 0, 1)		       \
	};								       \
	static const struct i2c_nrfx_twi_config twi_##idx##z_config = {	       \
		.twi = NRFX_TWI_INSTANCE(idx),				       \
		.config = {						       \
			.scl       = DT_PROP(I2C(idx), scl_pin),	       \
			.sda       = DT_PROP(I2C(idx), sda_pin),	       \
			.frequency = I2C_FREQUENCY(idx),		       \
		}							       \
	};								       \
	DEVICE_DT_DEFINE(I2C(idx),					       \
		      twi_##idx##_init,					       \
		      twi_nrfx_pm_control,				       \
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
