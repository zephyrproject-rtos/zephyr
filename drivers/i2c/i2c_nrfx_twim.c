/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <drivers/i2c.h>
#include <dt-bindings/i2c/i2c.h>
#include <nrfx_twim.h>
#include <sys/util.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(i2c_nrfx_twim, CONFIG_I2C_LOG_LEVEL);

#define I2C_TRANSFER_TIMEOUT_MSEC		K_MSEC(500)

struct i2c_nrfx_twim_data {
	struct k_sem transfer_sync;
	struct k_sem completion_sync;
	volatile nrfx_err_t res;
	uint32_t dev_config;
	uint16_t concat_buf_size;
	uint8_t *concat_buf;
#ifdef CONFIG_PM_DEVICE
	enum pm_device_state pm_state;
#endif
};

struct i2c_nrfx_twim_config {
	nrfx_twim_t twim;
	nrfx_twim_config_t config;
};

static inline struct i2c_nrfx_twim_data *get_dev_data(const struct device *dev)
{
	return dev->data;
}

static inline
const struct i2c_nrfx_twim_config *get_dev_config(const struct device *dev)
{
	return dev->config;
}

static int i2c_nrfx_twim_transfer(const struct device *dev,
				  struct i2c_msg *msgs,
				  uint8_t num_msgs, uint16_t addr)
{
	int ret = 0;
	uint32_t concat_len = 0;
	uint8_t *concat_buf = get_dev_data(dev)->concat_buf;
	uint16_t concat_buf_size = get_dev_data(dev)->concat_buf_size;
	nrfx_twim_xfer_desc_t cur_xfer = {
		.address = addr
	};

	k_sem_take(&(get_dev_data(dev)->transfer_sync), K_FOREVER);

	/* Dummy take on completion_sync sem to be sure that it is empty */
	k_sem_take(&(get_dev_data(dev)->completion_sync), K_NO_WAIT);

	nrfx_twim_enable(&get_dev_config(dev)->twim);

	for (size_t i = 0; i < num_msgs; i++) {
		if (I2C_MSG_ADDR_10_BITS & msgs[i].flags) {
			ret = -ENOTSUP;
			break;
		}

		/* Merge this fragment with the next if we have a buffer, this
		 * isn't the last fragment, it doesn't end a bus transaction,
		 * the next one doesn't start a bus transaction, and the
		 * direction of the next fragment is the same as this one.
		 */
		bool concat_next = (concat_buf_size > 0)
			&& ((i + 1) < num_msgs)
			&& !(msgs[i].flags & I2C_MSG_STOP)
			&& !(msgs[i + 1].flags & I2C_MSG_RESTART)
			&& ((msgs[i].flags & I2C_MSG_READ)
			    == (msgs[i + 1].flags & I2C_MSG_READ));

		/* If we need to concatenate the next message, or we've
		 * already committed to concatenate this message, add it to
		 * the buffer after verifying there's room.
		 */
		if (concat_next || (concat_len != 0)) {
			if ((concat_len + msgs[i].len) > concat_buf_size) {
				LOG_ERR("concat-buf overflow: %u + %u > %u",
					concat_len, msgs[i].len, concat_buf_size);
				ret = -ENOSPC;
				break;
			}
			if (!(msgs[i].flags & I2C_MSG_READ)) {
				memcpy(concat_buf + concat_len,
				       msgs[i].buf,
				       msgs[i].len);
			}
			concat_len += msgs[i].len;
		}

		if (concat_next) {
			continue;
		}

		if (concat_len == 0) {
			cur_xfer.p_primary_buf = msgs[i].buf;
			cur_xfer.primary_length = msgs[i].len;
		} else {
			cur_xfer.p_primary_buf = concat_buf;
			cur_xfer.primary_length = concat_len;
		}
		cur_xfer.type = (msgs[i].flags & I2C_MSG_READ) ?
			NRFX_TWIM_XFER_RX : NRFX_TWIM_XFER_TX;

		nrfx_err_t res = nrfx_twim_xfer(&get_dev_config(dev)->twim,
						&cur_xfer,
						(msgs[i].flags & I2C_MSG_STOP) ?
						 0 : NRFX_TWIM_FLAG_TX_NO_STOP);
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
			 * Therefore we always call nrfx_twim_bus_recover() to
			 * make sure everything has been done to restore the
			 * bus from this error.
			 */
			LOG_ERR("Error on I2C line occurred for message %d", i);
			nrfx_twim_disable(&get_dev_config(dev)->twim);
			nrfx_twim_bus_recover(get_dev_config(dev)->config.scl,
					      get_dev_config(dev)->config.sda);
			ret = -EIO;
			break;
		}

		res = get_dev_data(dev)->res;

		if (res != NRFX_SUCCESS) {
			LOG_ERR("Error 0x%08X occurred for message %d", res, i);
			ret = -EIO;
			break;
		}

		/* If concatenated messages were I2C_MSG_READ type, then
		 * content of concatenation buffer has to be copied back into
		 * buffers provided by user. */
		if ((msgs[i].flags & I2C_MSG_READ)
		    && cur_xfer.p_primary_buf == concat_buf) {
			int j = i;

			while (concat_len >= msgs[j].len) {
				concat_len -= msgs[j].len;
				memcpy(msgs[j].buf,
				       concat_buf + concat_len,
				       msgs[j].len);
				j--;
			}

		}

		concat_len = 0;
	}

	nrfx_twim_disable(&get_dev_config(dev)->twim);
	k_sem_give(&(get_dev_data(dev)->transfer_sync));

	return ret;
}

static void event_handler(nrfx_twim_evt_t const *p_event, void *p_context)
{
	struct i2c_nrfx_twim_data *dev_data = p_context;

	switch (p_event->type) {
	case NRFX_TWIM_EVT_DONE:
		dev_data->res = NRFX_SUCCESS;
		break;
	case NRFX_TWIM_EVT_ADDRESS_NACK:
		dev_data->res = NRFX_ERROR_DRV_TWI_ERR_ANACK;
		break;
	case NRFX_TWIM_EVT_DATA_NACK:
		dev_data->res = NRFX_ERROR_DRV_TWI_ERR_DNACK;
		break;
	default:
		dev_data->res = NRFX_ERROR_INTERNAL;
		break;
	}

	k_sem_give(&dev_data->completion_sync);
}

static int i2c_nrfx_twim_configure(const struct device *dev,
				   uint32_t dev_config)
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
	.transfer  = i2c_nrfx_twim_transfer,
};

static int init_twim(const struct device *dev)
{
	struct i2c_nrfx_twim_data *dev_data = get_dev_data(dev);
	nrfx_err_t result = nrfx_twim_init(&get_dev_config(dev)->twim,
					   &get_dev_config(dev)->config,
					   event_handler,
					   dev_data);
	if (result != NRFX_SUCCESS) {
		LOG_ERR("Failed to initialize device: %s",
			dev->name);
		return -EBUSY;
	}

#ifdef CONFIG_PM_DEVICE
	get_dev_data(dev)->pm_state = PM_DEVICE_STATE_ACTIVE;
#endif

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int twim_nrfx_pm_control(const struct device *dev,
				uint32_t ctrl_command,
				enum pm_device_state *state)
{
	int ret = 0;
	enum pm_device_state pm_current_state = get_dev_data(dev)->pm_state;

	if (ctrl_command == PM_DEVICE_STATE_SET) {
		enum pm_device_state new_state = *state;

		if (new_state != pm_current_state) {
			switch (new_state) {
			case PM_DEVICE_STATE_ACTIVE:
				init_twim(dev);
				if (get_dev_data(dev)->dev_config) {
					i2c_nrfx_twim_configure(
						dev,
						get_dev_data(dev)->dev_config);
				}
				break;

			case PM_DEVICE_STATE_LOW_POWER:
			case PM_DEVICE_STATE_SUSPEND:
			case PM_DEVICE_STATE_OFF:
				if (pm_current_state != PM_DEVICE_STATE_ACTIVE) {
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
		__ASSERT_NO_MSG(ctrl_command == PM_DEVICE_STATE_GET);
		*state = get_dev_data(dev)->pm_state;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

#define I2C_NRFX_TWIM_INVALID_FREQUENCY  ((nrf_twim_frequency_t)-1)
#define I2C_NRFX_TWIM_FREQUENCY(bitrate)				       \
	 (bitrate == I2C_BITRATE_STANDARD ? NRF_TWIM_FREQ_100K		       \
	: bitrate == 250000               ? NRF_TWIM_FREQ_250K		       \
	: bitrate == I2C_BITRATE_FAST     ? NRF_TWIM_FREQ_400K		       \
					  : I2C_NRFX_TWIM_INVALID_FREQUENCY)

#define I2C(idx) DT_NODELABEL(i2c##idx)
#define I2C_FREQUENCY(idx)						       \
	I2C_NRFX_TWIM_FREQUENCY(DT_PROP(I2C(idx), clock_frequency))

#define CONCAT_BUF_SIZE(idx) DT_PROP(I2C(idx), zephyr_concat_buf_size)

#define I2C_CONCAT_BUF(idx)						       \
	COND_CODE_1(DT_NODE_HAS_PROP(I2C(idx), zephyr_concat_buf_size),	       \
	(.concat_buf = twim_##idx##_concat_buf,				       \
	.concat_buf_size = CONCAT_BUF_SIZE(idx),), ())

#define I2C_NRFX_TWIM_DEVICE(idx)					       \
	BUILD_ASSERT(I2C_FREQUENCY(idx) !=				       \
		     I2C_NRFX_TWIM_INVALID_FREQUENCY,			       \
		     "Wrong I2C " #idx " frequency setting in dts");	       \
	static int twim_##idx##_init(const struct device *dev)		       \
	{								       \
		IRQ_CONNECT(DT_IRQN(I2C(idx)), DT_IRQ(I2C(idx), priority),     \
			    nrfx_isr, nrfx_twim_##idx##_irq_handler, 0);       \
		return init_twim(dev);					       \
	}								       \
	COND_CODE_1(DT_NODE_HAS_PROP(I2C(idx), zephyr_concat_buf_size),	       \
	(static uint8_t twim_##idx##_concat_buf[CONCAT_BUF_SIZE(idx)];),       \
	())								       \
									       \
	static struct i2c_nrfx_twim_data twim_##idx##_data = {		       \
		.transfer_sync = Z_SEM_INITIALIZER(                            \
			twim_##idx##_data.transfer_sync, 1, 1),                \
		.completion_sync = Z_SEM_INITIALIZER(                          \
			twim_##idx##_data.completion_sync, 0, 1),              \
		I2C_CONCAT_BUF(idx)					       \
	};								       \
	static const struct i2c_nrfx_twim_config twim_##idx##z_config = {      \
		.twim = NRFX_TWIM_INSTANCE(idx),			       \
		.config = {						       \
			.scl       = DT_PROP(I2C(idx), scl_pin),	       \
			.sda       = DT_PROP(I2C(idx), sda_pin),	       \
			.frequency = I2C_FREQUENCY(idx),		       \
		}							       \
	};								       \
	DEVICE_DT_DEFINE(I2C(idx),					       \
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
