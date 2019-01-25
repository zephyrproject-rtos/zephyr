/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <i2c.h>
#include <dt-bindings/i2c/i2c.h>
#include <nrfx_twim.h>

#define LOG_DOMAIN "i2c_nrfx_twim"
#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(i2c_nrfx_twim);

struct i2c_nrfx_twim_data {
	struct k_sem transfer_sync;
	struct k_sem completion_sync;
	volatile nrfx_err_t res;
};

struct i2c_nrfx_twim_config {
	nrfx_twim_t twim;
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

static int i2c_nrfx_twim_transfer(struct device *dev, struct i2c_msg *msgs,
				  u8_t num_msgs, u16_t addr)
{
	int ret = 0;

	k_sem_take(&(get_dev_data(dev)->transfer_sync), K_FOREVER);
	nrfx_twim_enable(&get_dev_config(dev)->twim);

	for (size_t i = 0; i < num_msgs; i++) {
		if (I2C_MSG_ADDR_10_BITS & msgs[i].flags) {
			ret = -ENOTSUP;
			break;
		}

		nrfx_twim_xfer_desc_t cur_xfer = {
			.p_primary_buf  = msgs[i].buf,
			.primary_length = msgs[i].len,
			.address	= addr,
			.type		= (msgs[i].flags & I2C_MSG_READ) ?
					  NRFX_TWIM_XFER_RX : NRFX_TWIM_XFER_TX
		};

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

		k_sem_take(&(get_dev_data(dev)->completion_sync), K_FOREVER);
		res = get_dev_data(dev)->res;
		if (res != NRFX_SUCCESS) {
			LOG_ERR("Error %d occurred for message %d", res, i);
			ret = -EIO;
			break;
		}
	}

	nrfx_twim_disable(&get_dev_config(dev)->twim);
	k_sem_give(&(get_dev_data(dev)->transfer_sync));

	return ret;
}

static void event_handler(nrfx_twim_evt_t const *p_event, void *p_context)
{
	struct device *dev = p_context;
	struct i2c_nrfx_twim_data *dev_data = get_dev_data(dev);

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
		LOG_ERR("unsupported speed");
		return -EINVAL;
	}

	return 0;
}

static const struct i2c_driver_api i2c_nrfx_twim_driver_api = {
	.configure = i2c_nrfx_twim_configure,
	.transfer  = i2c_nrfx_twim_transfer,
};

static int init_twim(struct device *dev, const nrfx_twim_config_t *config)
{

	nrfx_err_t result = nrfx_twim_init(&get_dev_config(dev)->twim, config,
					   event_handler, dev);
	if (result != NRFX_SUCCESS) {
		LOG_ERR("Failed to initialize device: %s",
			    dev->config->name);
		return -EBUSY;
	}

	return 0;
}

#define I2C_NRFX_TWIM_INVALID_FREQUENCY  ((nrf_twim_frequency_t)-1)
#define I2C_NRFX_TWIM_FREQUENCY(bitrate)				       \
	 (bitrate == I2C_BITRATE_STANDARD ? NRF_TWIM_FREQ_100K		       \
	: bitrate == 250000               ? NRF_TWIM_FREQ_250K		       \
	: bitrate == I2C_BITRATE_FAST     ? NRF_TWIM_FREQ_400K		       \
					  : I2C_NRFX_TWIM_INVALID_FREQUENCY)

#define I2C_NRFX_TWIM_DEVICE(idx)					       \
	BUILD_ASSERT_MSG(						       \
		I2C_NRFX_TWIM_FREQUENCY(				       \
			DT_NORDIC_NRF_I2C_I2C_##idx##_CLOCK_FREQUENCY)	       \
		!= I2C_NRFX_TWIM_INVALID_FREQUENCY,			       \
		"Wrong I2C " #idx " frequency setting in dts");		       \
	static int twim_##idx##_init(struct device *dev)		       \
	{								       \
		IRQ_CONNECT(DT_NORDIC_NRF_I2C_I2C_##idx##_IRQ,		       \
			    DT_NORDIC_NRF_I2C_I2C_##idx##_IRQ_PRIORITY,	       \
			    nrfx_isr, nrfx_twim_##idx##_irq_handler, 0);       \
		const nrfx_twim_config_t config = {			       \
			.scl       = DT_NORDIC_NRF_I2C_I2C_##idx##_SCL_PIN,    \
			.sda       = DT_NORDIC_NRF_I2C_I2C_##idx##_SDA_PIN,    \
			.frequency = I2C_NRFX_TWIM_FREQUENCY(		       \
				DT_NORDIC_NRF_I2C_I2C_##idx##_CLOCK_FREQUENCY) \
		};							       \
		return init_twim(dev, &config);				       \
	}								       \
	static struct i2c_nrfx_twim_data twim_##idx##_data = {		       \
		.transfer_sync = _K_SEM_INITIALIZER(                           \
			twim_##idx##_data.transfer_sync, 1, 1),                \
		.completion_sync = _K_SEM_INITIALIZER(                         \
			twim_##idx##_data.completion_sync, 0, 1)               \
	};								       \
	static const struct i2c_nrfx_twim_config twim_##idx##_config = {       \
		.twim = NRFX_TWIM_INSTANCE(idx)				       \
	};								       \
	DEVICE_AND_API_INIT(twim_##idx,					       \
			    DT_NORDIC_NRF_I2C_I2C_##idx##_LABEL,	       \
			    twim_##idx##_init,				       \
			    &twim_##idx##_data,				       \
			    &twim_##idx##_config,			       \
			    POST_KERNEL,				       \
			    CONFIG_I2C_INIT_PRIORITY,			       \
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
