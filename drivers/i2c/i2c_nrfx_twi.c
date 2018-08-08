/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <i2c.h>
#include <nrfx_twi.h>

#define SYS_LOG_DOMAIN "i2c_nrfx_twi"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_I2C_LEVEL
#include <logging/sys_log.h>

struct i2c_nrfx_twi_data {
	struct k_sem sync;
	volatile nrfx_err_t res;
};

struct i2c_nrfx_twi_config {
	nrfx_twi_t twi;
};

static inline struct i2c_nrfx_twi_data *get_dev_data(struct device *dev)
{
	return dev->driver_data;
}

static inline
const struct i2c_nrfx_twi_config *get_dev_config(struct device *dev)
{
	return dev->config->config_info;
}

static int i2c_nrfx_twi_transfer(struct device *dev, struct i2c_msg *msgs,
				 u8_t num_msgs, u16_t addr)
{
	for (size_t i = 0; i < num_msgs; i++) {
		if (I2C_MSG_ADDR_10_BITS & msgs[i].flags) {
			return -ENOTSUP;
		}

		nrfx_twi_xfer_desc_t cur_xfer = {
			.p_primary_buf  = msgs[i].buf,
			.primary_length = msgs[i].len,
			.address	= addr,
			.type		= (msgs[i].flags & I2C_MSG_READ) ?
					  NRFX_TWI_XFER_RX : NRFX_TWI_XFER_TX
		};

		nrfx_err_t res = nrfx_twi_xfer(&get_dev_config(dev)->twi,
					       &cur_xfer,
					       (msgs[i].flags & I2C_MSG_STOP) ?
					       0 : NRFX_TWI_FLAG_TX_NO_STOP);
		if (res != NRFX_SUCCESS) {
			return -EIO;
		}

		k_sem_take(&(get_dev_data(dev)->sync), K_FOREVER);
		res = get_dev_data(dev)->res;
		if (res != NRFX_SUCCESS) {
			SYS_LOG_ERR("Error %d occurred for message %d", res, i);
			return -EIO;
		}
	}

	return 0;
}

static void event_handler(nrfx_twi_evt_t const *p_event, void *p_context)
{
	struct device *dev = p_context;
	struct i2c_nrfx_twi_data *dev_data = get_dev_data(dev);

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

	k_sem_give(&dev_data->sync);
}

static int i2c_nrfx_twi_configure(struct device *dev, u32_t dev_config)
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
		SYS_LOG_ERR("unsupported speed");
		return -EINVAL;
	}

	return 0;
}

static const struct i2c_driver_api i2c_nrfx_twi_driver_api = {
	.configure = i2c_nrfx_twi_configure,
	.transfer  = i2c_nrfx_twi_transfer,
};

static int init_twi(struct device *dev, const nrfx_twi_config_t *config)
{
	nrfx_err_t result = nrfx_twi_init(&get_dev_config(dev)->twi, config,
					  event_handler, dev);
	if (result != NRFX_SUCCESS) {
		SYS_LOG_ERR("Failed to initialize device: %s",
			    dev->config->name);
		return -EBUSY;
	}

	nrfx_twi_enable(&get_dev_config(dev)->twi);
	return 0;
}

#define I2C_NRFX_TWI_DEVICE(idx)					\
	static int twi_##idx##_init(struct device *dev)			\
	{								\
		IRQ_CONNECT(CONFIG_I2C_##idx##_IRQ,			\
			    CONFIG_I2C_##idx##_IRQ_PRI,			\
			    nrfx_isr, nrfx_twi_##idx##_irq_handler, 0);	\
		const nrfx_twi_config_t config = {			\
			.scl       = CONFIG_I2C_##idx##_SCL_PIN,	\
			.sda       = CONFIG_I2C_##idx##_SDA_PIN,	\
			.frequency = NRF_TWI_FREQ_100K,			\
		};							\
		return init_twi(dev, &config);				\
	}								\
	static struct i2c_nrfx_twi_data twi_##idx##_data = {		\
		.sync = _K_SEM_INITIALIZER(twi_##idx##_data.sync, 0, 1)	\
	};								\
	static const struct i2c_nrfx_twi_config twi_##idx##_config = {	\
		.twi = NRFX_TWI_INSTANCE(idx)				\
	};								\
	DEVICE_AND_API_INIT(twi_##idx, CONFIG_I2C_##idx##_NAME,		\
			    twi_##idx##_init, &twi_##idx##_data,	\
			    &twi_##idx##_config, POST_KERNEL,		\
			    CONFIG_I2C_INIT_PRIORITY,			\
			    &i2c_nrfx_twi_driver_api)

#ifdef CONFIG_I2C_0_NRF_TWI
I2C_NRFX_TWI_DEVICE(0);
#endif

#ifdef CONFIG_I2C_1_NRF_TWI
I2C_NRFX_TWI_DEVICE(1);
#endif

