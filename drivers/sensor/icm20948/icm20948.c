/* bmp280.c - Driver for Bosch BMP280 temperature and pressure sensor */

/*
 * Copyright (c) 2016, 2017 Intel Corporation
 * Copyright (c) 2017 IpTronix S.r.l.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <sensor.h>
#include <init.h>
#include <gpio.h>
#include <misc/byteorder.h>
#include <misc/__assert.h>

#ifdef DT_TDK_ICM20948_BUS_I2C
#include <i2c.h>
#elif defined DT_TDK_ICM20948_BUS_SPI
#include <spi.h>
#endif
#include <logging/log.h>

#include "icm20948.h"

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
LOG_MODULE_REGISTER(ICM20948);


static int icm20948_reg_read(struct icm20948_data *data, u8_t start, u8_t *buf, int size){
#ifdef DT_TDK_ICM20948_BUS_I2C
    return i2c_burst_read(data->i2c_master, data->i2c_slave_addr,
			      start, buf, size);
#elif defined DT_TDK_ICM20948_BUS_SPI
    	u8_t addr;
	const struct spi_buf tx_buf = {
		.buf = &addr,
		.len = 1
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};
	struct spi_buf rx_buf[2];
	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = 2
	};
	int i;

	rx_buf[0].buf = NULL;
	rx_buf[0].len = 1;

	rx_buf[1].len = 1;

	for (i = 0; i < size; i++) {
		int ret;

		addr = (start + i) | 0x80;
		rx_buf[1].buf = &buf[i];

		ret = spi_transceive(data->spi, &data->spi_cfg, &tx, &rx);
		if (ret) {
			LOG_DBG("spi_transceive FAIL %d\n", ret);
			return ret;
		}
	}
#endif
    return 0;
}


static int icm20948_reg_write(struct icm20948_data *data,u8_t reg, u8_t val) {
#ifdef DT_TDK_ICM20948_BUS_I2C
    return i2c_reg_write_byte(data->i2c_master, data->i2c_slave_addr,reg, val);
#elif defined DT_TDK_ICM20948_BUS_SPI
    u8_t cmd[2] = { reg & 0x7F, val };
    const struct spi_buf tx_buf = {
        .buf = cmd,
        .len = 2
    };
    const struct spi_buf_set tx = {
        .buffers = &tx_buf,
        .count = 1
    };
    int ret;

    ret = spi_write(data->spi, &data->spi_cfg, &tx);
    if (ret) {
        LOG_DBG("spi_write FAIL %d\n", ret);
        return ret;
    }
#endif
    return 0;
}

static int icm20948_bank_reg_write(struct icm20948_data *data,u16_t reg, u8_t val) {
    u8_t bank = (u8_t)(reg >> 7);
    if (bank != data->bank) {
        data->bank = bank;
        icm20948_reg_write(data, ICM20948_REG_BANK_SEL, (bank << 4));
    }
    return icm20948_reg_write(data, (u8_t) (reg & 0xff), val);
}

static int icm20948_bank_reg_read(struct icm20948_data *data, u16_t start, u8_t *buf, int size) {
    u8_t bank = (u8_t)(start >> 7);
    if (bank != data->bank) {
        data->bank = bank;
        icm20948_reg_write(data, ICM20948_REG_BANK_SEL, (bank << 4));
    }
    return icm20948_reg_read(data, (u8_t)(start & 0xff), buf, size);
}


static inline int icm20948_set_gyro_fs( enum icm20948_gyro_fs gyro_fs){
    return 0;
}

static inline int icm20948_set_acc_fs( enum icm20948_gyro_fs gyro_fs){
    return 0;
}

static int icm20948_channel_get(struct device *dev,enum sensor_channel chan,struct sensor_value *val) {
//    struct icm20948_data *drv_data = dev->driver_data;

    return 0;
}

static int icm20948_sample_fetch(struct device *dev, enum sensor_channel chan) {
//    struct icm20948_data *data = dev->driver_data;

//    icm20948_bank_reg_write(data, ICM20948_REG_WHO_AM_I, 10);

    switch (chan) {
        case SENSOR_CHAN_ACCEL_X:
            break;

        default:
            break;
    }
    return 0;
}

int icm20948_init(struct device *dev)
{
    struct icm20948_data *data = (struct icm20948_data *) dev->driver_data;

    #if defined(DT_TDK_ICM20948_BUS_SPI)
		icm20948_spi_init(dev);
    #elif defined(DT_TDK_ICM20948_BUS_I2C)
	icm20948_i2c_init(dev);
    #else
    #error "BUS MACRO NOT DEFINED IN DTS"
    #endif

    u8_t value;
    if(data->hw_tf->read_reg(data,ICM20948_REG_WHO_AM_I,&value)){
    	LOG_ERR("Failed to read chip ID");
    	return -EIO;
    }

    if(val != ICM20948_WHO_AM_I){
    	LOG_ERR("Invalid Chip ID");
    }

    if(data->hw_tf->update_reg(data,ICM20948_REG_ACCEL_CONFIG,))

    return 0;
}

struct icm20948_data icm20948_data;



static const struct sensor_driver_api icm20498_driver_api = {
        .sample_fetch = icm20948_sample_fetch,
        .channel_get = icm20948_channel_get
};


DEVICE_AND_API_INIT(icm20948,CONFIG_ICM20948_NAME,icm20948_init,&icm20948_data,
                NULL,POST_KERNEL,CONFIG_SENSOR_INIT_PRIORITY,
                &icm20498_driver_api);


