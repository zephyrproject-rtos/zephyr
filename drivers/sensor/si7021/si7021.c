/* si7021.c - Driver for the SI7021 Relative Humidity and Temperature Sensor */

/*
 * Copyright (c) 2018 Thomas Li Fredriksen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "si7021.h"

#include <gpio.h>
#include <i2c.h>
#include <sensor.h>
#include <logging/log.h>

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
LOG_MODULE_REGISTER(SI7021)

static int si7021_cmd(struct device *dev, u8_t cmd)
{
    struct si7021_data *data = dev->driver_data;

    return i2c_write(data->i2c_master, &cmd, sizeof(cmd), data->i2c_slave_addr);
}

static int si7021_read16(struct device *dev, u16_t *dst)
{
    struct si7021_data *data = dev->driver_data;

    return i2c_read(data->i2c_master, (u8_t*) dst, sizeof(*dst), data->i2c_slave_addr);
}

static int si7021_reset(struct device *dev)
{
    int err;

    err = si7021_cmd(dev, SI7021_RESET_CMD);
    if(err < 0)
    {
        return err;
    }

    return 0;
}

static int si7021_read_humidity(struct device *dev)
{
    u8_t rx[2];
    int err;

    struct si7021_data *data = dev->driver_data;

    int retries = 20;

    err = si7021_cmd(dev, SI7021_MEASRH_NOHOLD_CMD);
    if(err < 0)
    {
        return err;
    }

    do {
        err = si7021_read16(dev, (u16_t*) rx);
        if(err == 0)
        {
            break;
        }

        k_sleep(1);
    } while(--retries);

    if(err < 0)
    {
        return err;
    }

    data->meash = (rx[0] << 8) | rx[1];

    return 0;
}

static int si7021_read_temp(struct device *dev)
{
    u8_t rx[2];
    int err;

    struct si7021_data *data = dev->driver_data;

    int retries = 20;

    err = si7021_cmd(dev, SI7021_MEASTEMP_NOHOLD_CMD);
    if(err < 0)
    {
        return err;
    }

    do {
        err = si7021_read16(dev, (u16_t*) rx);
        if(err == 0)
        {
            break;
        }

        k_sleep(1);
    } while(--retries);

    if(err < 0)
    {
        return err;
    }
    
    data->meastemp = (rx[0] << 8) | rx[1];

    return 0;
}

/********************************************
 ******************* API ********************
 ********************************************/

static int si7021_sample_fetch(struct device *dev, enum sensor_channel chan)
{
    int err;

    err = si7021_read_humidity(dev);
    if(err < 0)
    {
        return err;
    }

    err = si7021_read_temp(dev);
    if(err < 0)
    {
        return err;
    }

    return 0;
}

static int si7021_channel_get(struct device *dev, enum sensor_channel chan, struct sensor_value *val)
{
    s32_t conv_val;

    struct si7021_data *data = dev->driver_data;

    switch(chan)
    {
        case SENSOR_CHAN_HUMIDITY:

            conv_val = 12500 * data->meash / 65536 - 600;
            break;
        case SENSOR_CHAN_AMBIENT_TEMP:

            conv_val = 17572 * data->meastemp / 65536 - 4685;
            break;
        default:
            return -EINVAL;
    }

    val->val1 = conv_val / 100;
    val->val2 = conv_val % 100;

    return 0;
}

static const struct sensor_driver_api si7021_api_funcs = {

    .sample_fetch = si7021_sample_fetch,
    .channel_get = si7021_channel_get
};

/********************************************
 ****************** INIT ********************
 ********************************************/

static int si7021_init(struct device *dev)
{
    int err;

    struct si7021_data *data = dev->driver_data;
    
    data->i2c_master = device_get_binding(CONFIG_SI7021_I2C_DEV_NAME);
    if(!data->i2c_master)
    {
        LOG_ERR("Failed to get I2C-master device");
        return -EINVAL;
    }

    data->i2c_slave_addr = CONFIG_SI7021_I2C_ADDR;

    err = si7021_reset(dev);
    if(err < 0)
    {
        LOG_ERR("Failed to reset device");
        return -EIO;
    }

    return 0;
}

static struct si7021_data si7021_data;

DEVICE_AND_API_INIT(si7021, CONFIG_SI7021_DEV_NAME, si7021_init, &si7021_data,
        NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &si7021_api_funcs);