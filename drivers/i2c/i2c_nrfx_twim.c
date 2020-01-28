/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <drivers/i2c.h>
#include <dt-bindings/i2c/i2c.h>
#include <nrfx_twim.h>
#include <logging/log.h>
#define LOG_MODULE_NAME twim
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_I2C_LOG_LEVEL);

struct i2c_nrfx_twim_data {
	struct i2c_common_data common;
	i2c_transfer_callback_t callback;
	void *user_data;
	struct k_sem transfer_sync;
	struct k_sem completion_sync;
	uint32_t dev_config;
	bool do_disable;
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	u32_t pm_state;
#endif
};

BUILD_ASSERT_MSG(offsetof(struct i2c_nrfx_twim_data, common) == 0,
			"Common part must be the first field.");

struct i2c_nrfx_twim_config {
	nrfx_twim_t twim;
	nrfx_twim_config_t config;
	u8_t *ram_buf;
	u32_t ram_buf_len;
};

static inline struct i2c_nrfx_twim_data *get_dev_data(struct device *dev)
{
	return dev->driver_data;
}

static inline
const struct i2c_nrfx_twim_config *get_dev_config(struct device *dev)
{
	return dev->config->config_info;
}

static int single_transfer(struct device *dev, const struct i2c_msg *msg,
				  u16_t addr, i2c_transfer_callback_t callback,
				  void *user_data)
{
	int ret = 0;
	u8_t *buf;

	if (I2C_MSG_ADDR_10_BITS & msg->flags) {
		return -ENOTSUP;
	}

	if (!atomic_cas((atomic_t *)&get_dev_data(dev)->callback,
			(atomic_val_t)NULL, (atomic_val_t)callback)) {
		return -EBUSY;
	}

	if (!nrfx_is_in_ram(msg->buf)) {
		if (get_dev_config(dev)->ram_buf == NULL) {
			return -EINVAL;
		}

		if (msg->len > get_dev_config(dev)->ram_buf_len) {
			I2C_ERR(dev, "Cannot transfer %d byte long ROM message,"
				"RAM buffer too small (%d bytes)",
				msg->len, get_dev_config(dev)->ram_buf_len);
			return -ENOMEM;
		}

		memcpy(get_dev_config(dev)->ram_buf, msg->buf, msg->len);
		buf = get_dev_config(dev)->ram_buf;
	} else {
		buf = msg->buf;
	}

	get_dev_data(dev)->user_data = user_data;
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
		get_dev_data(dev)->callback = NULL;
		ret = -EIO;
	}

	return ret;
}

static void transfer_callback(struct device *dev, int res, void *user_data)
{
	int *dst_res = user_data;

	*dst_res = res;
	k_sem_give(&(get_dev_data(dev)->completion_sync));
}

static int transfer(struct device *dev, struct i2c_msg *msgs,
				u8_t num_msgs,
				u16_t addr)
{
	int err;
	int res;

	I2C_DBG(dev, "Starting transfer (addr:%d, msgs:%d)", addr, num_msgs);

	k_sem_take(&(get_dev_data(dev)->transfer_sync), K_FOREVER);
	nrfx_twim_enable(&get_dev_config(dev)->twim);

	for (int i = 0; i < num_msgs; i++) {
		err = single_transfer(dev, &msgs[i], addr,
					transfer_callback, &res);
		if (err < 0) {
			I2C_ERR(dev, "Failed to start transfer (%d)", err);
			res = err;
			break;
		}

		k_sem_take(&(get_dev_data(dev)->completion_sync), K_FOREVER);
		if (res < 0) {
			I2C_ERR(dev, "Transfer completed with error (%d)", res);
			break;
		}
	}

	nrfx_twim_disable(&get_dev_config(dev)->twim);
	k_sem_give(&(get_dev_data(dev)->transfer_sync));

	I2C_DBG(dev, "Transfer done (addr: %d, err: %d)", addr, res);
	return res;
}

static void event_handler(nrfx_twim_evt_t const *p_event, void *p_context)
{
	struct device *dev = p_context;
	struct i2c_nrfx_twim_data *dev_data = get_dev_data(dev);
	i2c_transfer_callback_t callback = dev_data->callback;
	void *user_data = dev_data->user_data;
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
	dev_data->callback = NULL;
	callback(dev, res, user_data);
}

static int i2c_nrfx_twim_configure(struct device *dev, u32_t dev_config)
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
		I2C_ERR(dev, "unsupported speed");
		return -EINVAL;
	}
	get_dev_data(dev)->dev_config = dev_config;

	return 0;
}

static const struct i2c_driver_api i2c_nrfx_twim_driver_api = {
	.configure = i2c_nrfx_twim_configure,
	.single_transfer  = single_transfer,
	.transfer = transfer,
};

static int init_twim(struct device *dev)
{
	nrfx_err_t result = nrfx_twim_init(&get_dev_config(dev)->twim,
					   &get_dev_config(dev)->config,
					   event_handler,
					   dev);
	if (result != NRFX_SUCCESS) {
		I2C_ERR(dev, "Failed to initialize device: %s",
							dev->config->name);
		return -EBUSY;
	}

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	get_dev_data(dev)->pm_state = DEVICE_PM_ACTIVE_STATE;
#endif
	return z_i2c_mngr_init(dev);
}

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
static int twim_nrfx_pm_control(struct device *dev, u32_t ctrl_command,
				void *context, device_pm_cb cb, void *arg)
{
	int ret = 0;

	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
		u32_t new_state = *((const u32_t *)context);

		if (new_state != get_dev_data(dev)->pm_state) {
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
		assert(ctrl_command == DEVICE_PM_GET_POWER_STATE);
		*((u32_t *)context) = get_dev_data(dev)->pm_state;
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

#define I2C_NRFX_TWIM_DEVICE(idx)					       \
	BUILD_ASSERT_MSG(						       \
		I2C_NRFX_TWIM_FREQUENCY(				       \
			DT_NORDIC_NRF_TWIM_I2C_##idx##_CLOCK_FREQUENCY)	       \
		!= I2C_NRFX_TWIM_INVALID_FREQUENCY,			       \
		"Wrong I2C " #idx " frequency setting in dts");		       \
	static int twim_##idx##_init(struct device *dev)		       \
	{								       \
		IRQ_CONNECT(DT_NORDIC_NRF_TWIM_I2C_##idx##_IRQ_0,	       \
			    DT_NORDIC_NRF_TWIM_I2C_##idx##_IRQ_0_PRIORITY,     \
			    nrfx_isr, nrfx_twim_##idx##_irq_handler, 0);       \
		return init_twim(dev);					       \
	}								       \
	LOG_INSTANCE_REGISTER(LOG_MODULE_NAME, idx, CONFIG_I2C_LOG_LEVEL);     \
	static struct i2c_nrfx_twim_data twim_##idx##_data = {		       \
		.common = {						       \
			LOG_INSTANCE_PTR_INIT(log, LOG_MODULE_NAME, idx)       \
		},							       \
		.transfer_sync = Z_SEM_INITIALIZER(                            \
			twim_##idx##_data.transfer_sync, 1, 1),                \
		.completion_sync = Z_SEM_INITIALIZER(                          \
			twim_##idx##_data.completion_sync, 0, 1)               \
	};								       \
	static u8_t twim_##idx##_ram_buffer[16];			       \
	static const struct i2c_nrfx_twim_config twim_##idx##z_config = {      \
		.twim = NRFX_TWIM_INSTANCE(idx),			       \
		.config = {						       \
			.scl       = DT_NORDIC_NRF_TWIM_I2C_##idx##_SCL_PIN,   \
			.sda       = DT_NORDIC_NRF_TWIM_I2C_##idx##_SDA_PIN,   \
			.frequency = I2C_NRFX_TWIM_FREQUENCY(		       \
				DT_NORDIC_NRF_TWIM_I2C_##idx##_CLOCK_FREQUENCY)\
		},							       \
		.ram_buf = twim_##idx##_ram_buffer,			       \
		.ram_buf_len = sizeof(twim_##idx##_ram_buffer)		       \
	};								       \
	DEVICE_DEFINE(twim_##idx,					       \
		      DT_NORDIC_NRF_TWIM_I2C_##idx##_LABEL,		       \
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
