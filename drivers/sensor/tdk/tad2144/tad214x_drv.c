/*
 * Copyright (c) 2025 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>

#include <zephyr/logging/log.h>

#include "tad214x_drv.h"

LOG_MODULE_REGISTER(TAD214X, CONFIG_SENSOR_LOG_LEVEL);

void tad214x_mutex_lock(const struct device *dev);
void tad214x_mutex_unlock(const struct device *dev);
int tad214x_trigger_init(const struct device *dev);

void memswap16( void* ptr1, unsigned int bytes )
{
  unsigned char* s1 = (unsigned char*)ptr1;
  unsigned char tmp;
  for( unsigned int i = 0; i < bytes; i=i+2 )
  {
    tmp = s1[i];
    s1[i] = s1[i+1];
    s1[i+1] = tmp;
  }
}

// Polynomial for CRC-8-SAE J1850: x^8 + x^4 + x^3 + x^2 + 1 (0x1D)
#define POLYNOMIAL 0x1D
#define INITIAL_VALUE 0xFF

unsigned char crc8_sae_j1850(const unsigned char *data, unsigned int length) {
    unsigned char crc = INITIAL_VALUE;
    for (unsigned int i = 0; i < length; i++) {
        crc ^= data[i];
        for (unsigned char bit = 0; bit < 8; bit++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ POLYNOMIAL;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc ^ 0xFF;
}


void inv_tad214x_sleep_us(int us)
{
	k_sleep(K_USEC(us));
}

static int inv_io_hal_read_reg(void *ctx, uint8_t reg, uint16_t *rbuffer, uint32_t rlen)
{
	struct device *dev = (struct device *)ctx;
	const struct tad214x_config *cfg = (const struct tad214x_config *)dev->config;

	return cfg->bus_io->read(&cfg->bus, reg, rbuffer, rlen);
}

static int inv_io_hal_write_reg(void *ctx, uint8_t reg, const uint16_t *wbuffer, uint32_t wlen)
{
	struct device *dev = (struct device *)ctx;
	const struct tad214x_config *cfg = (const struct tad214x_config *)dev->config;

    return cfg->bus_io->write(&cfg->bus, reg, wbuffer, wlen);
}

static int tad214x_sample_fetch(const struct device *dev, const enum sensor_channel chan)
{
	struct tad214x_data *data = (struct tad214x_data *)dev->data;
    const struct tad214x_config *cfg = dev->config;
    
    uint16_t TMRData = 0;
    int16_t TempData = 0;

	tad214x_mutex_lock(dev);

	if (cfg->if_mode == IF_ENC) {
        data->angle = (((float)data->encoder_position*360.0)/16384.0 - 180.0)*100;
    }else{
        TAD214x_GetData(&data->tad214x_device, &TMRData, &TempData);
        data->angle = (fmod((TMRData*360.0)/65536.0 + 180.0,360.0) - 180)*100;
        data->temperature = (25.0 + TempData* 0.00625)*100;
    }
	tad214x_mutex_unlock(dev);
	return 0;
}

static void tad214x_convert_angle(struct sensor_value *val, int32_t raw_val)
{
	val->val1 = raw_val;
	val->val2 = 0;
}

static void tad214x_convert_temperature(struct sensor_value *val, int32_t raw_val)
{
	val->val1 = raw_val;
	val->val2 = 0;
}

static int tad214x_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct tad214x_data *data = (struct tad214x_data *)dev->data;

	if (!(chan == SENSOR_CHAN_AMBIENT_TEMP || chan == SENSOR_CHAN_MAGN_XYZ)) {
		return -ENOTSUP;
	}

	tad214x_mutex_lock(dev);

	if (chan == SENSOR_CHAN_MAGN_XYZ) {
		tad214x_convert_angle(val, data->angle);
	} else if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		tad214x_convert_temperature(val, data->temperature);
	} else {
		tad214x_mutex_unlock(dev);
		return -ENOTSUP;
	}

	tad214x_mutex_unlock(dev);
	return 0;
}

static inline int tad214x_bus_check(const struct device *dev)
{
	const struct tad214x_config *cfg = dev->config;

	return cfg->bus_io->check(&cfg->bus);
}

static int tad214x_setODR(const struct device *dev, TAD214X_ODR_t odr)
{
    int rc = INV_ERROR_SUCCESS;
    struct tad214x_data *data = dev->data;  
    /* TAD214x Set Odr at 100Hz and LPM Mode */
    rc = TAD214x_SetODR(&data->tad214x_device, odr);
    return rc;
}

static int tad214x_getODR(const struct device *dev)
{
    int rc = INV_ERROR_SUCCESS;
    TAD214X_ODR_t odr = TAD214X_ODR_10;
    struct tad214x_data *data = dev->data; 
    
    /* TAD214x Set Odr at 100Hz and LPM Mode */
    rc = TAD214x_GetODR(&data->tad214x_device, &odr);
    return (int)odr;
}


static int tad214x_setMode(const struct device *dev, TAD214X_PowerMode_t mode)
{
    int rc = INV_ERROR_SUCCESS;
    struct tad214x_data *data = dev->data;  
  
    /* TAD214x Set Mode */
    rc = TAD214x_SetMode(&data->tad214x_device, mode);
    return rc;
}

static int tad214x_getMode(const struct device *dev)
{
    int rc = INV_ERROR_SUCCESS;
    TAD214X_PowerMode_t mode =TAD214X_MODE_SBY;
     struct tad214x_data *data = dev->data;  
     
    /* TAD214x Set Odr at 100Hz and LPM Mode */
    rc = TAD214x_GetMode(&data->tad214x_device, &mode);
    return (int)mode;
}

static int tad214x_sensor_init(const struct device *dev)
{
	struct tad214x_data *data = dev->data;
	const struct tad214x_config *config = dev->config;
	int err = 0;

   	memset(&(data->tad214x_device), 0, sizeof(data->tad214x_device));

	/* Initialize serial interface and device */
	data->serif.context = (struct device *)dev;
	data->serif.read_reg = inv_io_hal_read_reg;
	data->serif.write_reg = inv_io_hal_write_reg;
	data->serif.max_read = 8;
	data->serif.max_write = 6;
	err = TAD214x_Init(&data->tad214x_device, &data->serif);
	if (err < 0) {
		LOG_ERR("Init failed: %d", err);
		return err;
	}
    inv_tad214x_sleep_us(30000);
    TAD214x_SetMode(&data->tad214x_device, TAD214X_MODE_CONT);
    inv_tad214x_sleep_us(30000);    
    TAD214x_Unlock(&data->tad214x_device);
    TAD214x_ProgramModeEnable(&data->tad214x_device);
    TAD214x_PredictionDisable(&data->tad214x_device);
    TAD214x_ProgramModeDisable(&data->tad214x_device);
    TAD214x_Lock(&data->tad214x_device);

	return 0;
}


static int tad214x_init(const struct device *dev)
{
	struct tad214x_data *data = (struct tad214x_data *)dev->data;
	struct tad214x_config *config = (struct tad214x_config *)dev->config;
	struct tad214x_serif icp_serif;
	int rc = 0;
	uint8_t icp_version;
    uint16_t temp =0, tdata = 0;

    if(config->if_mode != IF_ENC) {
    	if (tad214x_bus_check(dev) < 0) {
    		LOG_ERR("bus check failed");
    		return -ENODEV;
    	}

        if(config->if_mode == IF_I2C) {
            gpio_pin_configure_dt(&config->sa1_gpio, GPIO_OUTPUT_ACTIVE);
            gpio_pin_configure_dt(&config->sa2_gpio, GPIO_OUTPUT_ACTIVE);
        }

        inv_tad214x_sleep_us(30000);

        rc = tad214x_sensor_init(dev);

    	if (rc != INV_ERROR_SUCCESS) {
    		LOG_ERR("Init error");
    		return rc;
    	}

        inv_tad214x_sleep_us(30000);
    }
  
	if (IS_ENABLED(CONFIG_TAD2144_TRIGGER)) {
		rc = tad214x_trigger_init(dev);
		if (rc < 0) {
			LOG_ERR("Failed to initialize interrupt.");
			return rc;
		}
	}

	/* successful init, return 0 */
	return 0;
}

void tad214x_mutex_lock(const struct device *dev)
{
#ifdef CONFIG_TAD2144_TRIGGER
	const struct tad214x_config *config = dev->config;

	if (config->gpio_int.port) {
		struct tad214x_data *data = dev->data;

		k_mutex_lock(&data->mutex, K_FOREVER);
	}
#else
	ARG_UNUSED(dev);
#endif
}

void tad214x_mutex_unlock(const struct device *dev)
{
#ifdef CONFIG_TAD2144_TRIGGER
	const struct tad214x_config *config = dev->config;

	if (config->gpio_int.port) {
		struct tad214x_data *data = dev->data;

		k_mutex_unlock(&data->mutex);
	}
#else
	ARG_UNUSED(dev);
#endif
}

static int tad214x_attr_set(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, const struct sensor_value *val)
{
   int rc = 0;
   struct tad214x_config *config = (struct tad214x_config *)dev->config;
	__ASSERT_NO_MSG(val != NULL);

    if(config->if_mode == IF_ENC)
        return -ENOMSG;

    if (chan != SENSOR_CHAN_MAGN_XYZ)
        return -ENOTSUP;

	tad214x_mutex_lock(dev);
	if (attr == SENSOR_ATTR_CONFIGURATION) {
		rc = tad214x_setMode(dev, val->val1);
	} else if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
		rc = tad214x_setODR(dev, val->val1);
	} else {
		LOG_ERR("Unsupported attribute");
		rc = -EINVAL;
	}
	tad214x_mutex_unlock(dev);

	return rc;
}

static int tad214x_attr_get(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, struct sensor_value *val)
{
    int rc = 0;
	struct tad214x_config *config = (struct tad214x_config *)dev->config;
     __ASSERT_NO_MSG(val != NULL);

     if(config->if_mode == IF_ENC)
         return -ENOMSG;

     tad214x_mutex_lock(dev);
     if (attr == SENSOR_ATTR_CONFIGURATION) {
         val->val1 = tad214x_getMode(dev);
     } else if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
         val->val1 = tad214x_getODR(dev);
     } else {
         LOG_ERR("Unsupported attribute");
         rc = -EINVAL;
     }
     tad214x_mutex_unlock(dev);
    
     return rc;
}


static DEVICE_API(sensor, tad214x_api_funcs) = {.sample_fetch = tad214x_sample_fetch,
							    .channel_get = tad214x_channel_get,
							    .attr_set = tad214x_attr_set,
                            	.attr_get = tad214x_attr_get,
#ifdef CONFIG_TAD2144_TRIGGER
							    .trigger_set = tad214x_trigger_set
#endif
};

/* Common initialization for tad214x_config */
#define TAD214X_CONFIG(inst)                                              \
	IF_ENABLED(CONFIG_TAD2144_TRIGGER,                                  \
		(.gpio_int = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0})))

/* Initializes a struct tad214x_config for an instance on an I2C bus. */
#define TAD214X_CONFIG_I2C(inst)                          \
	{                                                      \
		.if_mode = IF_I2C,                        \
		.bus.i2c = I2C_DT_SPEC_INST_GET(inst),                 \
		.bus_io = &tad214x_bus_io_i2c,                    \
        .sa1_gpio = GPIO_DT_SPEC_INST_GET(inst, sa1_gpios),     \
        .sa2_gpio = GPIO_DT_SPEC_INST_GET(inst, sa2_gpios),     \		
		TAD214X_CONFIG(inst)                                  \
	}

/* Initializes a struct tad214x_config for an instance on a SPI bus. */
#define TAD214X_CONFIG_SPI(inst)                                  \
	{                                                              \
		.if_mode = IF_SPI,                         \
		.bus.spi = SPI_DT_SPEC_INST_GET(inst,                      \
			/*SPI_OP_MODE_MASTER | */SPI_WORD_SET(8) |    \
			SPI_TRANSFER_MSB, 0), \
		.bus_io = &tad214x_bus_io_spi,                            \
		TAD214X_CONFIG(inst)                                          \
	}

/* Initializes a struct tad214x_config for an instance on a encoder. */
#define TAD214X_CONFIG_GPIO(inst)                                  \
	{                                                              \
		.if_mode = IF_ENC,                         \
        .miso_gpio = GPIO_DT_SPEC_INST_GET(inst, miso_gpios), /* encB */ \
        .mosi_gpio = GPIO_DT_SPEC_INST_GET(inst, mosi_gpios), /* encA */ \
        .sck_gpio  = GPIO_DT_SPEC_INST_GET(inst, sck_gpios),  /* encZ */ \
		TAD214X_CONFIG(inst)                                          \
	}

/*
 * Main instantiation macro, which selects the correct bus-specific
 * instantiation macros for the instance.
 */
#define TAD214X_DEFINE(inst)                                            \
        static struct tad214x_data tad214x_drv_##inst;                       \
        static const struct tad214x_config tad214x_config_##inst =           \
            COND_CODE_1(DT_INST_ON_BUS(inst, spi),                           \
                (TAD214X_CONFIG_SPI(inst)),                                  \
                (COND_CODE_1(DT_INST_ON_BUS(inst, i2c),                      \
                    (TAD214X_CONFIG_I2C(inst)),                              \
                    (TAD214X_CONFIG_GPIO(inst))                              \
                ))                                                           \
            );                                                               \
                                                                             \
        SENSOR_DEVICE_DT_INST_DEFINE(inst, tad214x_init, NULL,               \
            &tad214x_drv_##inst, &tad214x_config_##inst, POST_KERNEL,       \
            CONFIG_SENSOR_INIT_PRIORITY, &tad214x_api_funcs);


#define DT_DRV_COMPAT invensense_tad2144
DT_INST_FOREACH_STATUS_OKAY(TAD214X_DEFINE)

