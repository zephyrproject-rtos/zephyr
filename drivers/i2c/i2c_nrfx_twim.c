/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <drivers/i2c.h>
#include <drivers/i2c_async.h>
#include <dt-bindings/i2c/i2c.h>
#include <nrfx_twim.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(twim, CONFIG_I2C_LOG_LEVEL);

struct i2c_nrfx_twim_data {
	struct i2c_async async;
	struct sys_notify *notify;
	void *user_data;
	uint32_t dev_config;
	bool do_disable;
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	uint32_t pm_state;
#endif
};

struct i2c_nrfx_twim_config {
	nrfx_twim_t twim;
	nrfx_twim_config_t config;
	uint8_t *ram_buf;
	uint32_t ram_buf_len;
};

static inline struct i2c_nrfx_twim_data *get_dev_data(struct device *dev)
{
	return dev->driver_data;
}

static inline
const struct i2c_nrfx_twim_config *get_dev_config(struct device *dev)
{
	return dev->config_info;
}

static struct queued_operation_manager *get_qop_mgr(struct device *dev)
{
	struct i2c_nrfx_twim_data *data = get_dev_data(dev);

	return &data->async.mgrs.qop_mgr;
}

static int single_transfer(struct device *dev, const struct i2c_msg *msg,
				  uint16_t addr, struct sys_notify *notify)
{
	int ret = 0;
	uint8_t *buf;

	if (I2C_MSG_ADDR_10_BITS & msg->flags) {
		return -ENOTSUP;
	}

	if (!atomic_cas((atomic_t *)&get_dev_data(dev)->notify,
			(atomic_val_t)NULL, (atomic_val_t)notify)) {
		return -EBUSY;
	}

	if (!nrfx_is_in_ram(msg->buf)) {
		if (get_dev_config(dev)->ram_buf == NULL) {
			return -EINVAL;
		}

		if (msg->len > get_dev_config(dev)->ram_buf_len) {
			LOG_ERR("Cannot transfer %d byte long ROM message,"
				"RAM buffer too small (%d bytes)",
				msg->len, get_dev_config(dev)->ram_buf_len);
			return -ENOMEM;
		}

		memcpy(get_dev_config(dev)->ram_buf, msg->buf, msg->len);
		buf = get_dev_config(dev)->ram_buf;
	} else {
		buf = msg->buf;
	}

	nrfx_twim_enable(&get_dev_config(dev)->twim);

	nrfx_twim_xfer_desc_t cur_xfer = {
		.p_primary_buf  = buf,
		.primary_length = msg->len,
		.address	= addr,
		.type		= (msg->flags & I2C_MSG_READ) ?
					NRFX_TWIM_XFER_RX : NRFX_TWIM_XFER_TX
	};

	get_dev_data(dev)->do_disable = (msg->flags & I2C_MSG_STOP);
	nrfx_err_t res = nrfx_twim_xfer(&get_dev_config(dev)->twim,
					&cur_xfer,
					(msg->flags & I2C_MSG_STOP) ?
					0 : NRFX_TWIM_FLAG_TX_NO_STOP);
	if (res != NRFX_SUCCESS) {
		__ASSERT(ret != NRFX_ERROR_BUSY,
			"Driver busy - multiple, unintended users?");
		get_dev_data(dev)->notify = NULL;
		ret = -EIO;
	}

	return ret;
}

static void event_handler(nrfx_twim_evt_t const *p_event, void *p_context)
{
	struct device *dev = p_context;
	struct i2c_nrfx_twim_data *dev_data = get_dev_data(dev);
	struct sys_notify *notify = dev_data->notify;
	i2c_transfer_callback_t cb;
	int res;

	__ASSERT_NO_MSG(callback);

	switch (p_event->type) {
	case NRFX_TWIM_EVT_DONE:
		res = 0;
		break;
	case NRFX_TWIM_EVT_ADDRESS_NACK:
		res = -EIO;
		break;
	case NRFX_TWIM_EVT_DATA_NACK:
		res = -EIO;
		break;
	default:
		res = -EFAULT;
		break;
	}

	if (dev_data->do_disable) {
		nrfx_twim_disable(&get_dev_config(dev)->twim);
	}

	dev_data->notify = NULL;
	cb = (i2c_transfer_callback_t)sys_notify_finalize(notify, res);
	if (cb) {
		cb(dev, notify, res);
	}
}

static int i2c_nrfx_twim_configure(struct device *dev, uint32_t dev_config)
{
	nrfx_twim_t const *inst = &(get_dev_config(dev)->twim);

	if (I2C_ADDR_10_BITS & dev_config) {
		return -EINVAL;
	}

	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		nrf_twim_frequency_set(inst->p_twim, NRF_TWIM_FREQ_100K);
		break;
	case I2C_SPEED_FAST:
		nrf_twim_frequency_set(inst->p_twim, NRF_TWIM_FREQ_400K);
		break;
	default:
		LOG_ERR("unsupported speed");
		return -EINVAL;
	}
	get_dev_data(dev)->dev_config = dev_config;

	return 0;
}

static const struct i2c_driver_api i2c_nrfx_twim_driver_api = {
	.configure = i2c_nrfx_twim_configure,
	.single_transfer  = single_transfer,
	.get_qop_mgr = get_qop_mgr
};

static int init_twim(struct device *dev)
{
	nrfx_err_t result = nrfx_twim_init(&get_dev_config(dev)->twim,
					   &get_dev_config(dev)->config,
					   event_handler,
					   dev);
	if (result != NRFX_SUCCESS) {
		LOG_ERR("Failed to initialize device: %s",
			dev->name);
		return -EBUSY;
	}

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	get_dev_data(dev)->pm_state = DEVICE_PM_ACTIVE_STATE;
#endif
	return i2c_async_init(&get_dev_data(dev)->async, dev);
}

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
static int twim_nrfx_pm_control(struct device *dev, uint32_t ctrl_command,
				void *context, device_pm_cb cb, void *arg)
{
	int ret = 0;
	uint32_t pm_current_state = get_dev_data(dev)->pm_state;

	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
		uint32_t new_state = *((const uint32_t *)context);

		if (new_state != pm_current_state) {
			switch (new_state) {
			case DEVICE_PM_ACTIVE_STATE:
				init_twim(dev);
				if (get_dev_data(dev)->dev_config) {
					i2c_nrfx_twim_configure(
						dev,
						get_dev_data(dev)->dev_config);
				}
				break;

			case DEVICE_PM_LOW_POWER_STATE:
			case DEVICE_PM_SUSPEND_STATE:
			case DEVICE_PM_OFF_STATE:
				if (pm_current_state != DEVICE_PM_ACTIVE_STATE) {
					break;
				}
				nrfx_twim_uninit(&get_dev_config(dev)->twim);
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
#endif /* CONFIG_DEVICE_POWER_MANAGEMENT */

#define I2C_NRFX_TWIM_INVALID_FREQUENCY  ((nrf_twim_frequency_t)-1)
#define I2C_NRFX_TWIM_FREQUENCY(bitrate)				       \
	 (bitrate == I2C_BITRATE_STANDARD ? NRF_TWIM_FREQ_100K		       \
	: bitrate == 250000               ? NRF_TWIM_FREQ_250K		       \
	: bitrate == I2C_BITRATE_FAST     ? NRF_TWIM_FREQ_400K		       \
					  : I2C_NRFX_TWIM_INVALID_FREQUENCY)

#define I2C(idx) DT_NODELABEL(i2c##idx)
#define I2C_FREQUENCY(idx)						       \
	I2C_NRFX_TWIM_FREQUENCY(DT_PROP(I2C(idx), clock_frequency))

#define I2C_NRFX_TWIM_DEVICE(idx)					       \
	BUILD_ASSERT(I2C_FREQUENCY(idx) !=				       \
		     I2C_NRFX_TWIM_INVALID_FREQUENCY,			       \
		     "Wrong I2C " #idx " frequency setting in dts");	       \
	static int twim_##idx##_init(struct device *dev)		       \
	{								       \
		IRQ_CONNECT(DT_IRQN(I2C(idx)), DT_IRQ(I2C(idx), priority),     \
			    nrfx_isr, nrfx_twim_##idx##_irq_handler, 0);       \
		return init_twim(dev);					       \
	}								       \
	static struct i2c_nrfx_twim_data twim_##idx##_data;		       \
	static uint8_t twim_##idx##_ram_buffer[32];			       \
	static const struct i2c_nrfx_twim_config twim_##idx##z_config = {      \
		.twim = NRFX_TWIM_INSTANCE(idx),			       \
		.config = {						       \
			.scl       = DT_PROP(I2C(idx), scl_pin),	       \
			.sda       = DT_PROP(I2C(idx), sda_pin),	       \
			.frequency = I2C_FREQUENCY(idx),		       \
		},							       \
		.ram_buf = twim_##idx##_ram_buffer,			       \
		.ram_buf_len = sizeof(twim_##idx##_ram_buffer)		       \
	};								       \
	DEVICE_DEFINE(twim_##idx,					       \
		      DT_LABEL(I2C(idx)),				       \
		      twim_##idx##_init,				       \
		      twim_nrfx_pm_control,				       \
		      &twim_##idx##_data,				       \
		      &twim_##idx##z_config,				       \
		      POST_KERNEL,					       \
		      CONFIG_I2C_INIT_PRIORITY,				       \
		      &i2c_nrfx_twim_driver_api)

#ifdef CONFIG_I2C_0_NRF_TWIM
I2C_NRFX_TWIM_DEVICE(0);
#endif

#ifdef CONFIG_I2C_1_NRF_TWIM
I2C_NRFX_TWIM_DEVICE(1);
#endif

#ifdef CONFIG_I2C_2_NRF_TWIM
I2C_NRFX_TWIM_DEVICE(2);
#endif

#ifdef CONFIG_I2C_3_NRF_TWIM
I2C_NRFX_TWIM_DEVICE(3);
#endif
